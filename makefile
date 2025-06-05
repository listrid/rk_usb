source := rk_bin.cpp rk_common.cpp rk_usb.cpp \
 libusb/core.c libusb/descriptor.c libusb/hotplug.c libusb/io.c libusb/strerror.c libusb/sync.c \
 libusb/os/events_posix.c libusb/os/threads_posix.c libusb/os/linux_usbfs.c libusb/os/linux_netlink.c \
 libusb/os/linux_udev.c

objects      := $(source)
objects      := $(objects:.cpp=.o)
objects      := $(objects:.c=.o)
objects_tmp  := $(addprefix ./obj/, $(notdir $(objects)))
objects      := $(addprefix ./src/, $(objects))

compile_flags := -Wall -pipe  
build_flags  := -O3 -fomit-frame-pointer -Wstrict-aliasing=0
link_flags   := -s -pipe
#build_flags   := -g3
#link_flags    := -pipe
libraries     := -lpthread -lrt -ludev


.PHONY: clean all

all: obj_dirs $(objects)
	@make -s -j4 rk_usb

rk_usb:
	g++ ./src/main.cpp -o $@ $(objects_tmp) $(link_flags) $(libraries)

obj_dirs:
	@mkdir -p ./obj/

%.o : %.cpp
	g++ -o ./obj/$(notdir $@) -c $< $(compile_flags) $(build_flags)

%.o : %.c
	gcc -o ./obj/$(notdir $@) -c $< $(compile_flags) $(build_flags)

clean:
	rm -rf ./obj
