#pragma once
#include <stddef.h>
#include <stdint.h>

enum capability_type_t
{
    CAPABILITY_TYPE_DIRECT_LBA         = (0 << 0),
    CAPABILITY_TYPE_VENDOR_STORAGE     = (1 << 1),
    CAPABILITY_TYPE_FIRST_4M_ACCESS    = (1 << 2),
    CAPABILITY_TYPE_READ_LBA           = (1 << 3),
    CAPABILITY_TYPE_NEW_VENDOR_STORAGE = (1 << 4),
    CAPABILITY_TYPE_READ_COM_LOG       = (1 << 5),
    CAPABILITY_TYPE_READ_IDB_CONFIG    = (1 << 6),
    CAPABILITY_TYPE_READ_SECURE_MODE   = (1 << 7),
    CAPABILITY_TYPE_NEW_IDB            = (1 << 8),
    CAPABILITY_TYPE_SWITCH_STORAGE     = (1 << 9),
    CAPABILITY_TYPE_LBA_PARITY         = (1 << 10),
    CAPABILITY_TYPE_READ_OTP_CHIP      = (1 << 11),
    CAPABILITY_TYPE_SWITCH_USB3        = (1 << 12),
};

enum storage_type_t
{
    STORAGE_TYPE_UNKNOWN = (0 << 0),
    STORAGE_TYPE_FLASH   = (1 << 0),
    STORAGE_TYPE_EMMC    = (1 << 1),
    STORAGE_TYPE_SD      = (1 << 2),
    STORAGE_TYPE_SD1     = (1 << 3),
    STORAGE_TYPE_SPINOR  = (1 << 9),
    STORAGE_TYPE_SPINAND = (1 << 8),
    STORAGE_TYPE_RAM     = (1 << 6),
    STORAGE_TYPE_USB     = (1 << 7),
    STORAGE_TYPE_SATA    = (1 << 10),
    STORAGE_TYPE_PCIE    = (1 << 11),
};

struct chip_t
{
    uint16_t pid;
    const char* name;
};


#pragma pack(push, 1)

struct flash_info_t
{
    uint32_t sector_total;
    uint16_t block_size;
    uint8_t page_size;
    uint8_t ecc_bits;
    uint8_t access_time;
    uint8_t manufacturer_id;
    uint8_t chip_select;
    uint8_t id[5];
};
#pragma pack(pop)

typedef struct libusb_context libusb_context;
typedef struct libusb_device_handle libusb_device_handle;


class RK_usb
{
    bool usb_bulk_send(void* buf, size_t len);
    bool usb_bulk_recv(void* buf, size_t len);

    bool flash_read_lba_raw(uint32_t sec, uint32_t cnt, void* buf);
    bool flash_write_lba_raw(uint32_t sec, uint32_t cnt, void* buf);
    bool flash_erase_lba_raw(uint32_t sec, uint32_t cnt);

    bool write_raw(uint32_t addr, void* buf, size_t len);
    bool read_raw(uint32_t addr, void* buf, size_t len);

    bool maskrom_write_arm32(uint32_t addr, void* buf, size_t len, bool rc4);
    bool maskrom_write_arm64(uint32_t addr, void* buf, size_t len, bool rc4);

    libusb_context* m_context;
    libusb_device_handle* m_handle;
    uint8_t m_epout;
    uint8_t m_epin;
    const chip_t* m_chip;
    bool m_maskrom;

public:

    RK_usb();
    ~RK_usb();
    bool init();

    const char* chip_name();
    bool is_maskrom(){ return m_maskrom; };

    bool maskrom_upload_memory(uint32_t code, void* buf, uint64_t len, bool rc4);
    bool maskrom_upload_file(uint32_t code, const char* filename, bool rc4);
    bool maskrom_dump_arm32(uint32_t uart, uint32_t addr, uint32_t len, bool rc4);
    bool maskrom_dump_arm64(uint32_t uart, uint32_t addr, uint32_t len, bool rc4);
    bool maskrom_write_arm32_progress(uint32_t addr, void* buf, size_t len, bool rc4);
    bool maskrom_write_arm64_progress(uint32_t addr, void* buf, size_t len, bool rc4);
    bool maskrom_exec_arm32(uint32_t addr, bool rc4);
    bool maskrom_exec_arm64(uint32_t addr, bool rc4);
    bool ready();
    bool version(uint8_t* buf);
    bool capability(uint8_t* buf);
    bool capability_support(enum capability_type_t type);
    bool reset(bool maskrom);
    bool exec (uint32_t addr, uint32_t dtb);
    bool read (uint32_t addr, void* buf, size_t len);
    bool write(uint32_t addr, void* buf, size_t len);
    bool read_progress(uint32_t addr, void* buf, size_t len);
    bool write_progress(uint32_t addr, void* buf, size_t len);
    bool otp_read(uint8_t* buf, size_t len);
    bool sn_read(char* sn);
    bool sn_write(char* sn);
    bool vs_read(uint16_t type, uint16_t index, uint8_t* buf, size_t len);
    bool vs_write(uint16_t type, uint16_t index, uint8_t* buf, size_t len);
    enum storage_type_t storage_read();
    bool storage_switch(enum storage_type_t type);
    bool flash_detect(flash_info_t* info);
    bool flash_erase_lba(uint32_t sec, uint32_t cnt);
    bool flash_read_lba(uint32_t sec, uint32_t cnt, void* buf);
    bool flash_write_lba(uint32_t sec, uint32_t cnt, void* buf);
    bool flash_erase_lba_progress(uint32_t sec, uint32_t cnt);
    bool flash_read_lba_progress(uint32_t sec, uint32_t cnt, void* buf);
    bool flash_write_lba_progress(uint32_t sec, uint32_t cnt, void* buf);
    bool flash_read_lba_to_file_progress(uint32_t sec, uint32_t cnt, const char* filename);
    bool flash_write_lba_from_file_progress(uint32_t sec, uint32_t maxcnt, const char* filename);
};

