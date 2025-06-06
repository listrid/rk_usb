/* -*- Mode: C; c-basic-offset:8 ; indent-tabs-mode:t -*- */
/*
 * Linux usbfs backend for libusb
 * Copyright (C) 2007-2009 Daniel Drake <dsd@gentoo.org>
 * Copyright (c) 2001 Johannes Erdfelt <johannes@erdfelt.com>
 * Copyright (c) 2012-2013 Nathan Hjelm <hjelmn@mac.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "../libusbi.h"
#include "linux_usbfs.h"

#include <errno.h>
#include <fcntl.h>
#include <libudev.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

/* udev context */
static struct udev *udev_ctx = NULL;
static int udev_monitor_fd = -1;
static usbi_event_t udev_control_event = USBI_INVALID_EVENT;
static struct udev_monitor *udev_monitor = NULL;
static pthread_t linux_event_thread;

static void udev_hotplug_event(struct udev_device *udev_dev);
static void *linux_udev_event_thread_main(void *arg);

int linux_udev_start_event_monitor(void)
{
	int r;

	assert(udev_ctx == NULL);
	udev_ctx = udev_new();
	if (!udev_ctx) {
		usbi_err(NULL, "could not create udev context");
		goto err;
	}

	udev_monitor = udev_monitor_new_from_netlink(udev_ctx, "udev");
	if (!udev_monitor) {
		usbi_err(NULL, "could not initialize udev monitor");
		goto err_free_ctx;
	}

	r = udev_monitor_filter_add_match_subsystem_devtype(udev_monitor, "usb", "usb_device");
	if (r) {
		usbi_err(NULL, "could not initialize udev monitor filter for \"usb\" subsystem");
		goto err_free_monitor;
	}

	if (udev_monitor_enable_receiving(udev_monitor)) {
		usbi_err(NULL, "failed to enable the udev monitor");
		goto err_free_monitor;
	}

	udev_monitor_fd = udev_monitor_get_fd(udev_monitor);

#if defined(FD_CLOEXEC)
	/* Make sure the udev file descriptor is marked as CLOEXEC */
	r = fcntl(udev_monitor_fd, F_GETFD);
	if (r == -1) {
		usbi_err(NULL, "failed to get udev monitor fd flags, errno=%d", errno);
		goto err_free_monitor;
	}
	if (!(r & FD_CLOEXEC)) {
		if (fcntl(udev_monitor_fd, F_SETFD, r | FD_CLOEXEC) == -1) {
			usbi_err(NULL, "failed to set udev monitor fd flags, errno=%d", errno);
			goto err_free_monitor;
		}
	}
#endif

	/* Some older versions of udev are not non-blocking by default,
	 * so make sure this is set */
	r = fcntl(udev_monitor_fd, F_GETFL);
	if (r == -1) {
		usbi_err(NULL, "failed to get udev monitor fd status flags, errno=%d", errno);
		goto err_free_monitor;
	}
	if (!(r & O_NONBLOCK)) {
		if (fcntl(udev_monitor_fd, F_SETFL, r | O_NONBLOCK) == -1) {
			usbi_err(NULL, "failed to set udev monitor fd status flags, errno=%d", errno);
			goto err_free_monitor;
		}
	}

	r = usbi_create_event(&udev_control_event);
	if (r) {
		usbi_err(NULL, "failed to create udev control event");
		goto err_free_monitor;
	}

	r = pthread_create(&linux_event_thread, NULL, linux_udev_event_thread_main, NULL);
	if (r) {
		usbi_err(NULL, "failed to create hotplug event thread (%d)", r);
		goto err_destroy_event;
	}

	return LIBUSB_SUCCESS;

err_destroy_event:
	usbi_destroy_event(&udev_control_event);
	udev_control_event = (usbi_event_t)USBI_INVALID_EVENT;
err_free_monitor:
	udev_monitor_unref(udev_monitor);
	udev_monitor = NULL;
	udev_monitor_fd = -1;
err_free_ctx:
	udev_unref(udev_ctx);
err:
	udev_ctx = NULL;
	return LIBUSB_ERROR_OTHER;
}

int linux_udev_stop_event_monitor(void)
{
	int r;

	assert(udev_ctx != NULL);
	assert(udev_monitor != NULL);
	assert(udev_monitor_fd != -1);

	/* Signal the control event and wait for the thread to exit */
	usbi_signal_event(&udev_control_event);

	r = pthread_join(linux_event_thread, NULL);
	if (r)
		usbi_warn(NULL, "failed to join hotplug event thread (%d)", r);

	usbi_destroy_event(&udev_control_event);
	udev_control_event = (usbi_event_t)USBI_INVALID_EVENT;

	/* Release the udev monitor */
	udev_monitor_unref(udev_monitor);
	udev_monitor = NULL;
	udev_monitor_fd = -1;

	/* Clean up the udev context */
	udev_unref(udev_ctx);
	udev_ctx = NULL;

	return LIBUSB_SUCCESS;
}

static void *linux_udev_event_thread_main(void *arg)
{
	struct pollfd fds[] = {
		{ .fd = USBI_EVENT_OS_HANDLE(&udev_control_event),
		  .events = USBI_EVENT_POLL_EVENTS },
		{ .fd = udev_monitor_fd,
		  .events = POLLIN },
	};
	struct udev_device *udev_dev;
	int r;

	UNUSED(arg);

#if defined(HAVE_PTHREAD_SETNAME_NP)
	r = pthread_setname_np(pthread_self(), "libusb_event");
	if (r)
		usbi_warn(NULL, "failed to set hotplug event thread name, error=%d", r);
#endif

	usbi_dbg(NULL, "udev event thread entering");

	while (1) {
		r = poll(fds, 2, -1);
		if (r == -1) {
			/* check for temporary failure */
			if (errno == EINTR)
				continue;
			usbi_err(NULL, "poll() failed, errno=%d", errno);
			break;
		}
		if (fds[0].revents) {
			/* activity on control event, exit */
			break;
		}
		if (fds[1].revents) {
			usbi_mutex_static_lock(&linux_hotplug_lock);
			udev_dev = udev_monitor_receive_device(udev_monitor);
			if (udev_dev)
				udev_hotplug_event(udev_dev);
			usbi_mutex_static_unlock(&linux_hotplug_lock);
		}
	}

	usbi_dbg(NULL, "udev event thread exiting");

	return NULL;
}

static int udev_device_info(struct libusb_context *ctx, int detached,
			    struct udev_device *udev_dev, uint8_t *busnum,
			    uint8_t *devaddr, const char **sys_name) {
	const char *dev_node;

	dev_node = udev_device_get_devnode(udev_dev);
	if (!dev_node) {
		return LIBUSB_ERROR_OTHER;
	}

	*sys_name = udev_device_get_sysname(udev_dev);
	if (!*sys_name) {
		return LIBUSB_ERROR_OTHER;
	}

	return linux_get_device_address(ctx, detached, busnum, devaddr,
					dev_node, *sys_name, -1);
}

static void udev_hotplug_event(struct udev_device *udev_dev)
{
	const char *udev_action;
	const char *sys_name = NULL;
	uint8_t busnum = 0, devaddr = 0;
	int detached;
	int r;

	do {
		udev_action = udev_device_get_action(udev_dev);
		if (!udev_action) {
			break;
		}

		detached = !strncmp(udev_action, "remove", 6);

		r = udev_device_info(NULL, detached, udev_dev, &busnum, &devaddr, &sys_name);
		if (LIBUSB_SUCCESS != r) {
			break;
		}

		usbi_dbg(NULL, "udev hotplug event. action: %s.", udev_action);

		if (strncmp(udev_action, "add", 3) == 0) {
			linux_hotplug_enumerate(busnum, devaddr, sys_name);
		} else if (detached) {
			linux_device_disconnected(busnum, devaddr);
		} else if (strncmp(udev_action, "bind", 4) == 0) {
			/* silently ignore "known unhandled" action */
		} else {
			usbi_err(NULL, "ignoring udev action %s", udev_action);
		}
	} while (0);

	udev_device_unref(udev_dev);
}

int linux_udev_scan_devices(struct libusb_context *ctx)
{
	struct udev_enumerate *enumerator;
	struct udev_list_entry *devices, *entry;
	struct udev_device *udev_dev;
	const char *sys_name;
	int r;

	assert(udev_ctx != NULL);

	enumerator = udev_enumerate_new(udev_ctx);
	if (NULL == enumerator) {
		usbi_err(ctx, "error creating udev enumerator");
		return LIBUSB_ERROR_OTHER;
	}

	udev_enumerate_add_match_subsystem(enumerator, "usb");
	udev_enumerate_add_match_property(enumerator, "DEVTYPE", "usb_device");
	udev_enumerate_scan_devices(enumerator);
	devices = udev_enumerate_get_list_entry(enumerator);

	entry = NULL;
	udev_list_entry_foreach(entry, devices) {
		const char *path = udev_list_entry_get_name(entry);
		uint8_t busnum = 0, devaddr = 0;

		udev_dev = udev_device_new_from_syspath(udev_ctx, path);

		r = udev_device_info(ctx, 0, udev_dev, &busnum, &devaddr, &sys_name);
		if (r) {
			udev_device_unref(udev_dev);
			continue;
		}

		linux_enumerate_device(ctx, busnum, devaddr, sys_name);
		udev_device_unref(udev_dev);
	}

	udev_enumerate_unref(enumerator);

	return LIBUSB_SUCCESS;
}

void linux_udev_hotplug_poll(void)
{
	struct udev_device *udev_dev;

	usbi_mutex_static_lock(&linux_hotplug_lock);
	do {
		udev_dev = udev_monitor_receive_device(udev_monitor);
		if (udev_dev) {
			usbi_dbg(NULL, "Handling hotplug event from hotplug_poll");
			udev_hotplug_event(udev_dev);
		}
	} while (udev_dev);
	usbi_mutex_static_unlock(&linux_hotplug_lock);
}
