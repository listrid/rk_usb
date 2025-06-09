#include "rk_common.h"
#include "rk_usb.h"
#include "rk_bin.h"
#include "./libusb/libusb.h"
#ifndef _WIN32
#include <unistd.h>
#endif
static const char* manufacturer[] =
{
    "Samsung",
    "Toshiba",
    "Hynix",
    "Infineon",
    "Micron",
    "Renesas",
    "ST",
    "Intel",
    "SanDisk",
};

static void usage(void)
{
    printf("rk_usb - https://github.com/listrid/rk_usb\r\n");
    printf(" fork xrock(v1.1.3) - https://github.com/xboot/xrock\r\n");
    printf("usage:\r\n");
    printf("    rk_usb maskrom <ddr> <usbplug> [--rc4]        - Initial chip using ddr and usbplug in maskrom mode\r\n");
    printf("    rk_usb download <loader>                      - Initial chip using loader in maskrom mode\r\n");
    printf("    rk_usb upgrade <loader>                       - Upgrade loader to flash in loader mode\r\n");
    printf("    rk_usb ready                                  - Show chip ready or not\r\n");
    printf("    rk_usb version                                - Show chip version\r\n");
    printf("    rk_usb capability                             - Show capability information\r\n");
    printf("    rk_usb reset [maskrom]                        - Reset chip to normal or maskrom mode\n");
    printf("    rk_usb dump <address> <length>                - Dump memory region in hex format\r\n");
    printf("    rk_usb read <address> <length> <file>         - Read memory to file\r\n");
    printf("    rk_usb write <address> <file>                 - Write file to memory\r\n");
    printf("    rk_usb exec <address> [dtb]                   - Call function address(Recommend to use extra command)\r\n");
    printf("    rk_usb otp <length>                           - Dump otp memory in hex format\r\n");
    printf("    rk_usb sn                                     - Read serial number\r\n");
    printf("    rk_usb sn <string>                            - Write serial number\r\n");
    printf("    rk_usb vs dump <index> <length> [type]        - Dump vendor storage in hex format\r\n");
    printf("    rk_usb vs read <index> <length> <file> [type] - Read vendor storage\r\n");
    printf("    rk_usb vs write <index> <file> [type]         - Write vendor storage\r\n");
    printf("    rk_usb storage                                - Read storage media list\r\n");
    printf("    rk_usb storage <index>                        - Switch storage media and show list\r\n");
    printf("    rk_usb flash                                  - Detect flash and show information\r\n");
    printf("    rk_usb flash erase <sector> <count>           - Erase flash sector (count min 131072)\r\n");
    printf("    rk_usb flash read <sector> <count> <file>     - Read flash sector to file\r\n");
    printf("    rk_usb flash write <sector> <file>            - Write file to flash sector\r\n");

    printf("extra:\r\n");
    printf("    rk_usb extra maskrom [--rc4] [--sram <file> --delay <ms>] [--dram <file> --delay <ms>] [...]\r\n");
    printf("    rk_usb extra dump-arm32 [--rc4] --uart <register> <address> <length>\r\n");
    printf("    rk_usb extra dump-arm64 [--rc4] --uart <register> <address> <length>\r\n");
    printf("    rk_usb extra write-arm32 [--rc4] <address> <file>\r\n");
    printf("    rk_usb extra write-arm64 [--rc4] <address> <file>\r\n");
    printf("    rk_usb extra exec-arm32 [--rc4] <address>\r\n");
    printf("    rk_usb extra exec-arm64 [--rc4] <address>\r\n");
}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static inline uint32_t __get_unaligned_le32(const uint8_t* p) { return (p[0] << 0) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
static inline uint32_t get_unaligned_le32(const void* p) { return __get_unaligned_le32((const uint8_t*)p); }

int main(int argc, char* argv[])
{
    RK_usb ctx;

    if(argc < 2)
    {
        usage();
        return 0;
    }
    for(int i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            usage();
            return 0;
        }
    }
    if(!ctx.init())
    {
        printf("ERROR: Can't found any supported rockchip chips\r\n");
        return -1;
    }
    if(!strcmp(argv[1], "maskrom"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 2 || argc == 3)
        {
            if(ctx.is_maskrom())
            {
                bool rc4 = false;
                if((argc == 3) && !strcmp(argv[2], "--rc4"))
                    rc4 = true;
                if(!ctx.maskrom_upload_file(0x471, argv[0], rc4))
                {
                    printf("Error sram upload '%s'\r\n", argv[0]);
                    return -1;
                }
#ifdef _WIN32
                Sleep(10);
#else
                usleep(10*1000);
#endif
                if(!ctx.maskrom_upload_file(0x472, argv[1], rc4))
                {
                    printf("Error dram upload '%s'\r\n", argv[1]);
                    return -1;
                }
#ifdef _WIN32
                Sleep(10);
#else
                usleep(10*1000);
#endif
                return 0;
            }else{
                printf("ERROR: The chip '%s' does not in maskrom mode\r\n", ctx.chip_name());
            }
        }else{
            usage();
        }
        return -1;
    }
    
    if(!strcmp(argv[1], "download"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 1)
        {
            if(ctx.is_maskrom())
            {
                RK_bin rk_bin;
                if(rk_bin.load(argv[0]))
                {
                    for(uint32_t i = 0; i < rk_bin.m_nentry; i++)
                    {
                        rk_bin_entry_t* e = rk_bin.m_entry[i];
                        char str[256];
                        if(e->type == RKLOADER_ENTRY_471)
                        {
                            void* buf = (char*)rk_bin.m_buffer + get_unaligned_le32(&e->data_offset);
                            uint64_t len = get_unaligned_le32(&e->data_size);
                            uint32_t delay = get_unaligned_le32(&e->data_delay);

                            printf("Downloading '%s'\r\n", wide2str(str, (uint8_t*)&e->name[0], sizeof(e->name)));
                            if(!ctx.maskrom_upload_memory(0x471, buf, len, rk_bin.m_is_rc4on))
                            {
                                printf("Error\r\n");
                                return -1;
                            }
#ifdef _WIN32
                            Sleep(delay);
#else
                            usleep(delay*1000);
#endif
                        }else if(e->type == RKLOADER_ENTRY_472)
                        {
                            void* buf = (char*)rk_bin.m_buffer + get_unaligned_le32(&e->data_offset);
                            uint64_t len = get_unaligned_le32(&e->data_size);
                            uint32_t delay = get_unaligned_le32(&e->data_delay);

                            printf("Downloading '%s'\r\n", wide2str(str, (uint8_t*)&e->name[0], sizeof(e->name)));
                            if(!ctx.maskrom_upload_memory(0x472, buf, len, rk_bin.m_is_rc4on))
                            {
                                printf("Error\r\n");
                                return -1;
                            }
#ifdef _WIN32
                            Sleep(delay);
#else
                            usleep(delay*1000);
#endif
                        }else{
                            printf("Error: RKLOADER_ENTRY type\r\n");
                            return -1;
                        }
                    }
                    return 0;
                }else
                    printf("ERROR: Not a valid loader '%s'\r\n", argv[0]);
            }else
                printf("ERROR: The chip '%s' does not in maskrom mode\r\n", ctx.chip_name());
        }else
            usage();
        return -1;
    }

    if(!strcmp(argv[1], "upgrade"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 1)
        {
            RK_bin rk_bin;
            if(rk_bin.load(argv[0]))
            {
                uint32_t sec;
                enum storage_type_t type = ctx.storage_read();
                switch(type)
                {
//                    case STORAGE_TYPE_FLASH: sec = 64; break;
//                    case STORAGE_TYPE_EMMC: sec = 64; break;
//                    case STORAGE_TYPE_SD: sec = 64; break;
//                    case STORAGE_TYPE_SD1: sec = 64; break;
                    case STORAGE_TYPE_SPINOR: sec = 128; break;
                    case STORAGE_TYPE_SPINAND: sec = 512; break;
//                    case STORAGE_TYPE_RAM: sec = 64; break;
//                    case STORAGE_TYPE_USB: sec = 64; break;
//                    case STORAGE_TYPE_SATA: sec = 64; break;
//                    case STORAGE_TYPE_PCIE: sec = 64; break;
                    default: sec = 64; break;
                }
                flash_info_t info;
                if(ctx.flash_detect(&info))
                {
                    if(!ctx.flash_write_lba_progress(sec, uint32_t(rk_bin.m_idblen / 512), rk_bin.m_idbbuf))
                    {
                        printf("Failed to write flash\r\n");
                        return -1;
                    }
                    return 0;
                }else
                    printf("Failed to detect flash\r\n");
            }else
                printf("ERROR: Not a valid loader '%s'\r\n", argv[0]);
        }else
            usage();
        return -1;
    }
    
    if(!strcmp(argv[1], "ready"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 0)
        {
            if(ctx.ready())
            {
                printf("The chip is ready\r\n");
                return 0;
            }else
                printf("Failed to show chip ready status\r\n");
        }else
            usage();
        return -1;
    }
    
    if(!strcmp(argv[1], "version"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 0)
        {
            uint8_t buf[16];
            if(ctx.version(buf))
            {
                printf("%s(%c%c%c%c): 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x\r\n", ctx.chip_name(),
                    buf[ 3], buf[ 2], buf[ 1], buf[ 0],
                    buf[ 3], buf[ 2], buf[ 1], buf[ 0],
                    buf[ 7], buf[ 6], buf[ 5], buf[ 4],
                    buf[11], buf[10], buf[ 9], buf[ 8],
                    buf[15], buf[14], buf[13], buf[12]);
                return 0;
            }else
                printf("Failed to get chip version\r\n");
        }else
            usage();
        return -1;
    }
    
    if(!strcmp(argv[1], "capability"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 0)
        {
            uint8_t buf[8];
            if(ctx.capability(buf))
            {
                printf("Capability: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
                printf("    Direct LBA: %s\r\n", (buf[0] & (1 << 0)) ? "enabled" : "disabled");
                printf("    Vendor Storage: %s\r\n", (buf[0] & (1 << 1)) ? "enabled" : "disabled");
                printf("    First 4M Access: %s\r\n", (buf[0] & (1 << 2)) ? "enabled" : "disabled");
                printf("    Read LBA: %s\r\n", (buf[0] & (1 << 3)) ? "enabled" : "disabled");
                printf("    New Vendor Storage: %s\r\n", (buf[0] & (1 << 4)) ? "enabled" : "disabled");
                printf("    Read Com Log: %s\r\n", (buf[0] & (1 << 5)) ? "enabled" : "disabled");
                printf("    Read IDB Config: %s\r\n", (buf[0] & (1 << 6)) ? "enabled" : "disabled");
                printf("    Read Secure Mode: %s\r\n", (buf[0] & (1 << 7)) ? "enabled" : "disabled");
                printf("    New IDB: %s\r\n", (buf[1] & (1 << 0)) ? "enabled" : "disabled");
                printf("    Switch Storage: %s\r\n", (buf[1] & (1 << 1)) ? "enabled" : "disabled");
                printf("    LBA Parity: %s\r\n", (buf[1] & (1 << 2)) ? "enabled" : "disabled");
                printf("    Read OTP Chip: %s\r\n", (buf[1] & (1 << 3)) ? "enabled" : "disabled");
                printf("    Switch USB3: %s\r\n", (buf[1] & (1 << 4)) ? "enabled" : "disabled");
                return 0;
            }else
                printf("Failed to show capability information\r\n");
        }else
            usage();
        return -1;
    }
    
    if(!strcmp(argv[1], "reset"))
    {
        argc -= 2;
        argv += 2;
        if(argc > 0)
        {
            if(!strcmp(argv[0], "maskrom"))
            {
                if(ctx.reset(true))
                    return 0;
            }else
                usage();
        }else{
            if(ctx.reset(false))
                return 0;
        }
        return -1;
    }
    
    if(!strcmp(argv[1], "dump"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 2)
        {
            uint32_t addr = strtoul(argv[0], NULL, 0);
            size_t len = strtoul(argv[1], NULL, 0);
            char* buf = (char*)malloc(len);
            if(buf)
            {
                if(ctx.read(addr, buf, len))
                {
                    hexdump(addr, buf, len);
                    free(buf);
                    return 0;
                }
                free(buf);
            }
        }else
            usage();
        return -1;
    }
    
    if(!strcmp(argv[1], "read"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 3)
        {
            uint32_t addr = strtoul(argv[0], NULL, 0);
            size_t len = strtoul(argv[1], NULL, 0);
            char* buf = (char*)malloc(len);
            if(buf)
            {
                if(ctx.read_progress(addr, buf, len))
                {
                    file_save(argv[2], buf, len);
                    free(buf);
                    return 0;
                }
                free(buf);
            }
        }else
            usage();
        return -1;
    }
    
    if(!strcmp(argv[1], "write"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 2)
        {
            uint32_t addr = strtoul(argv[0], NULL, 0);
            uint64_t len;
            void* buf = file_load(argv[1], &len);
            if(buf)
            {
                if(ctx.write_progress(addr, buf, len))
                {
                    free(buf);
                    return 0;
                }
                free(buf);
            }
        }else
            usage();
        return -1;
    }
    
    if(!strcmp(argv[1], "exec"))
    {
        argc -= 2;
        argv += 2;
        if(argc >= 1)
        {
            uint32_t addr = strtoul(argv[0], NULL, 0);
            uint32_t dtb = (argc >= 2) ? strtoul(argv[1], NULL, 0) : 0;
            if(ctx.exec(addr, dtb))
                return 0;
        }else
            usage();
        return -1;
    }
    
    if(!strcmp(argv[1], "otp"))
    {
        if(ctx.capability_support(CAPABILITY_TYPE_READ_OTP_CHIP))
        {
            argc -= 2;
            argv += 2;
            if(argc == 1)
            {
                size_t len = strtoul(argv[0], NULL, 0);
                if(len > 0)
                {
                    uint8_t* otp = (uint8_t*)malloc(len);
                    if(otp)
                    {
                        if(ctx.otp_read(otp, len))
                        {
                            hexdump(0, otp, len);
                            free(otp);
                            return 0;
                        }
                        free(otp);
                    }
                }
            }else
                usage();
        }else
            printf("The loader don't support dump otp\r\n");
        return -1;
    }
    
    if(!strcmp(argv[1], "sn"))
    {
        if(ctx.capability_support(CAPABILITY_TYPE_VENDOR_STORAGE) || ctx.capability_support(CAPABILITY_TYPE_NEW_VENDOR_STORAGE))
        {
            argc -= 2;
            argv += 2;
            if(argc == 0)
            {
                char sn[512 - 8 + 1];
                if(ctx.sn_read(sn))
                {
                    printf("SN: %s\r\n", sn);
                    return 0;
                }else
                    printf("No serial number\r\n");
            }else{
                if(argc == 1)
                {
                    if(ctx.sn_write(argv[0]))
                    {
                        printf("Write serial number '%s'\r\n", argv[0]);
                        return 0;
                    }else
                        printf("Failed to write serial number\r\n");
                }else
                    usage();
            }
        }else
            printf("The loader don't support vendor storage\r\n");
        return -1;
    }
    
    if(!strcmp(argv[1], "vs"))
    {
        if(ctx.capability_support(CAPABILITY_TYPE_VENDOR_STORAGE) || ctx.capability_support(CAPABILITY_TYPE_NEW_VENDOR_STORAGE))
        {
            argc -= 2;
            argv += 2;
            if(argc > 0)
            {
                if(!strcmp(argv[0], "dump") && (argc >= 3))
                {
                    uint16_t index = (uint16_t)strtoul(argv[1], NULL, 0);
                    size_t len = strtoul(argv[2], NULL, 0);
                    if(len > 0x10000000)
                        len = 0x10000000;
                    size_t type = (argc == 4) ? strtoul(argv[3], NULL, 0) : 0;
                    if(len > 0)
                    {
                        uint8_t* buf = (uint8_t*)malloc(len);
                        if(ctx.vs_read((uint16_t)type, index, buf, len))
                        {
                            hexdump(0, buf, len);
                            free(buf);
                            return 0;
                        }
                        free(buf);
                    }
                }else if(!strcmp(argv[0], "read") && (argc >= 4))
                {
                    uint16_t index = (uint16_t)strtoul(argv[1], NULL, 0);
                    size_t len = strtoul(argv[2], NULL, 0);
                    if(len > 0x10000000)
                        len = 0x10000000;
                    size_t type = (argc == 5) ? strtoul(argv[4], NULL, 0) : 0;
                    if(len > 0)
                    {
                        uint8_t* buf = (uint8_t*)malloc(len);
                        if(ctx.vs_read((uint16_t)type, index, buf, len))
                        {
                            if(file_save(argv[3], buf, len))
                            {
                                free(buf);
                                return 0;
                            }
                        }
                        free(buf);
                    }
                }else if(!strcmp(argv[0], "write") && (argc >= 3))
                {
                    uint16_t index = (uint16_t)strtoul(argv[1], NULL, 0);
                    size_t type = (argc == 4) ? strtoul(argv[3], NULL, 0) : 0;
                    uint64_t len;
                    void* buf = file_load(argv[2], &len);
                    if(buf && (len > 0))
                    {
                        if(ctx.vs_write((uint16_t)type, index, (uint8_t*)buf, (len > 512) ? 512 : len))
                        {
                            free(buf);
                            return 0;
                        }
                        printf("Failed to write vendor storage\r\n");
                        free(buf);
                    }
                }else
                    usage();
            }else
                usage();
        }else
            printf("The loader don't support vendor storage\r\n");
        return -1;
    }
    
    if(!strcmp(argv[1], "storage"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 0)
        {
            enum storage_type_t type = ctx.storage_read();
            printf("%s 0.UNKNOWN\r\n", (type == STORAGE_TYPE_UNKNOWN) ? "-->" : "   ");
            printf("%s 1.FLASH\r\n", (type == STORAGE_TYPE_FLASH) ? "-->" : "   ");
            printf("%s 2.EMMC\r\n", (type == STORAGE_TYPE_EMMC) ? "-->" : "   ");
            printf("%s 3.SD\r\n", (type == STORAGE_TYPE_SD) ? "-->" : "   ");
            printf("%s 4.SD1\r\n", (type == STORAGE_TYPE_SD1) ? "-->" : "   ");
            printf("%s 5.SPINOR\r\n", (type == STORAGE_TYPE_SPINOR) ? "-->" : "   ");
            printf("%s 6.SPINAND\r\n", (type == STORAGE_TYPE_SPINAND) ? "-->" : "   ");
            printf("%s 7.RAM\r\n", (type == STORAGE_TYPE_RAM) ? "-->" : "   ");
            printf("%s 8.USB\r\n", (type == STORAGE_TYPE_USB) ? "-->" : "   ");
            printf("%s 9.SATA\r\n", (type == STORAGE_TYPE_SATA) ? "-->" : "   ");
            printf("%s10.PCIE\r\n", (type == STORAGE_TYPE_PCIE) ? "-->" : "   ");
            return 0;
        }
        if(ctx.capability_support(CAPABILITY_TYPE_SWITCH_STORAGE))
        {
            if(argc == 1)
            {
                enum storage_type_t type;
                size_t index = strtol(argv[0], NULL, 0);
                switch(index)
                {
                    case 0: type = STORAGE_TYPE_UNKNOWN; break;
                    case 1: type = STORAGE_TYPE_FLASH; break;
                    case 2: type = STORAGE_TYPE_EMMC; break;
                    case 3: type = STORAGE_TYPE_SD; break;
                    case 4: type = STORAGE_TYPE_SD1; break;
                    case 5: type = STORAGE_TYPE_SPINOR; break;
                    case 6: type = STORAGE_TYPE_SPINAND; break;
                    case 7: type = STORAGE_TYPE_RAM; break;
                    case 8: type = STORAGE_TYPE_USB; break;
                    case 9: type = STORAGE_TYPE_SATA; break;
                    case 10:type = STORAGE_TYPE_PCIE; break;
                    default:type = STORAGE_TYPE_UNKNOWN;break;
                }
                if(!ctx.storage_switch(type))
                {
                    printf("Error: storage_switch\r\n");
                    return -1;
                }
                type = ctx.storage_read();
                printf("%s 0.UNKNOWN\r\n", (type == STORAGE_TYPE_UNKNOWN) ? "-->" : "   ");
                printf("%s 1.FLASH\r\n", (type == STORAGE_TYPE_FLASH) ? "-->" : "   ");
                printf("%s 2.EMMC\r\n", (type == STORAGE_TYPE_EMMC) ? "-->" : "   ");
                printf("%s 3.SD\r\n", (type == STORAGE_TYPE_SD) ? "-->" : "   ");
                printf("%s 4.SD1\r\n", (type == STORAGE_TYPE_SD1) ? "-->" : "   ");
                printf("%s 5.SPINOR\r\n", (type == STORAGE_TYPE_SPINOR) ? "-->" : "   ");
                printf("%s 6.SPINAND\r\n", (type == STORAGE_TYPE_SPINAND) ? "-->" : "   ");
                printf("%s 7.RAM\r\n", (type == STORAGE_TYPE_RAM) ? "-->" : "   ");
                printf("%s 8.USB\r\n", (type == STORAGE_TYPE_USB) ? "-->" : "   ");
                printf("%s 9.SATA\r\n", (type == STORAGE_TYPE_SATA) ? "-->" : "   ");
                printf("%s10.PCIE\r\n", (type == STORAGE_TYPE_PCIE) ? "-->" : "   ");
                return 0;
            }else
                usage();
        }else
            printf("The loader don't support switch storage\r\n");
        return -1;
    }
    
    if(!strcmp(argv[1], "flash"))
    {
        argc -= 2;
        argv += 2;
        if(argc == 0)
        {
            flash_info_t info;
            if(ctx.flash_detect(&info))
            {
                printf("Flash info:\r\n");
                printf("    Manufacturer: %s (%d)\r\n", (info.manufacturer_id < ARRAY_SIZE(manufacturer))
                                ? manufacturer[info.manufacturer_id] : "Unknown", info.manufacturer_id);
                printf("    Capacity: %dMB\r\n", info.sector_total >> 11);
                printf("    Sector size: %d\r\n", 512);
                printf("    Sector count: %d\r\n", info.sector_total);
                printf("    Block size: %dKB\r\n", info.block_size >> 1);
                printf("    Page size: %dKB\r\n", info.page_size >> 1);
                printf("    ECC bits: %d\r\n", info.ecc_bits);
                printf("    Access time: %d\r\n", info.access_time);
                printf("    Flash CS: %s%s%s%s\r\n",
                                info.chip_select & 1 ? "<0>" : "",
                                info.chip_select & 2 ? "<1>" : "",
                                info.chip_select & 4 ? "<2>" : "",
                                info.chip_select & 8 ? "<3>" : "");
                printf("    Flash ID: %02x %02x %02x %02x %02x\r\n",
                                info.id[0], info.id[1],    info.id[2],    info.id[3],    info.id[4]);
                return 0;
            }else
                printf("Failed to detect flash\r\n");
        }else{
            if(!strcmp(argv[0], "erase") && (argc == 3))
            {
                argc -= 1;
                argv += 1;
                flash_info_t info;
                uint32_t sec = strtoul(argv[0], NULL, 0);
                uint32_t cnt = strtoul(argv[1], NULL, 0);
                if(ctx.flash_detect(&info))
                {
                    if(sec < info.sector_total)
                    {
                        if(cnt <= 0)
                            cnt = info.sector_total - sec;
                        else if(cnt > info.sector_total - sec)
                            cnt = info.sector_total - sec;
                        if(!ctx.flash_erase_lba_progress(sec, cnt))
                        {
                            printf("Failed to erase flash\r\n");
                            return -1;
                        }
                        return 0;
                    }else
                        printf("The start sector is out of range\r\n");
                }else
                    printf("Failed to detect flash\r\n");
            }else if(!strcmp(argv[0], "read") && (argc == 4))
            {
                argc -= 1;
                argv += 1;
                flash_info_t info;
                uint32_t sec = strtoul(argv[0], NULL, 0);
                uint32_t cnt = strtoul(argv[1], NULL, 0);
                if(ctx.flash_detect(&info))
                {
                    if(sec < info.sector_total)
                    {
                        if(cnt <= 0)
                            cnt = info.sector_total - sec;
                        else if(cnt > info.sector_total - sec)
                            cnt = info.sector_total - sec;
                        if(!ctx.flash_read_lba_to_file_progress(sec, cnt, argv[2]))
                        {
                            printf("Failed to read flash\r\n");
                            return -1;
                        }
                        return 0;
                    }else
                        printf("The start sector is out of range\r\n");
                }else
                    printf("Failed to detect flash\r\n");
            }else if(!strcmp(argv[0], "write") && (argc == 3))
            {
                argc -= 1;
                argv += 1;
                flash_info_t info;
                uint32_t sec = strtoul(argv[0], NULL, 0);
                if(ctx.flash_detect(&info))
                {
                    if(sec < info.sector_total)
                    {
                        if(ctx.flash_write_lba_from_file_progress(sec, info.sector_total, argv[1]))
                            return 0;
                        printf("Failed to write flash\r\n");
                    }else
                        printf("The start sector is out of range\r\n");
                }else
                    printf("Failed to detect flash\r\n");
            }else
                usage();
        }
        return -1;
    }
    
    if(!strcmp(argv[1], "extra"))
    {
        argc -= 2;
        argv += 2;
        if(!strcmp(argv[0], "maskrom"))
        {
            argc -= 1;
            argv += 1;
            if(argc >= 2)
            {
                if(ctx.is_maskrom())
                {
                    bool rc4 = false;
                    for(int i = 0; i < argc; i++)
                    {
                        if(!strcmp(argv[i], "--rc4") && (argc > i + 1))
                        {
                            rc4 = true;
                        }else if(!strcmp(argv[i], "--sram") && (argc > i + 1))
                        {
                            if(!ctx.maskrom_upload_file(0x471, argv[i + 1], rc4))
                            {
                                printf("Error sram upload '%s'\r\n", argv[i + 1]);
                                return -1;
                            }
                            i++;
                        }else if(!strcmp(argv[i], "--dram") && (argc > i + 1))
                        {
                            if(!ctx.maskrom_upload_file(0x472, argv[i + 1], rc4))
                            {
                                printf("Error dram upload '%s'\r\n", argv[i + 1]);
                                return -1;
                            }
                            i++;
                        }else if(!strcmp(argv[i], "--delay") && (argc > i + 1))
                        {
                            uint32_t delay = strtoul(argv[i + 1], NULL, 0);
#ifdef _WIN32
                            Sleep(delay);
#else
                            usleep(delay*1000);
#endif
                            i++;
                        }else if(*argv[i] == '-')
                        {
                            usage();
                        }else if(*argv[i] != '-' && strcmp(argv[i], "-") != 0)
                        {
                            usage();
                        }
                    }
                    return 0;
                }else
                    printf("ERROR: The chip '%s' does not in maskrom mode\r\n", ctx.chip_name());
            }else
                usage();
            return -1;
        }
        
        if(!strcmp(argv[0], "dump-arm32"))
        {
            argc -= 1;
            argv += 1;
            if(argc >= 2)
            {
                if(ctx.is_maskrom())
                {
                    bool rc4 = false;
                    uint32_t uart = 0x0;
                    uint32_t addr = 0x0;
                    uint32_t len  = 0x0;
                    for(int i = 0, idx = 0; i < argc; i++)
                    {
                        if(!strcmp(argv[i], "--rc4") && (argc > i + 1))
                        {
                            rc4 = true;
                        }else if(!strcmp(argv[i], "--uart") && (argc > i + 1))
                        {
                            uart = strtoul(argv[i + 1], NULL, 0);
                            i++;
                        }else if(*argv[i] == '-')
                        {
                            usage();
                        }else if(*argv[i] != '-' && strcmp(argv[i], "-") != 0)
                        {
                            if(idx == 0)
                                addr = strtoul(argv[i], NULL, 0);
                            else if(idx == 1)
                                len = strtoul(argv[i], NULL, 0);
                            idx++;
                        }
                    }
                    if(ctx.maskrom_dump_arm32(uart, addr, len, rc4))
                        return 0;
                    printf("Error\r\n");
                }else
                    printf("ERROR: The chip '%s' does not in maskrom mode\r\n", ctx.chip_name());
            }else
                usage();
            return -1;
        }
        
        if(!strcmp(argv[0], "dump-arm64"))
        {
            argc -= 1;
            argv += 1;
            if(argc >= 2)
            {
                if(ctx.is_maskrom())
                {
                    bool rc4 = false;
                    uint32_t uart = 0x0;
                    uint32_t addr = 0x0;
                    uint32_t len  = 0x0;
                    for(int i = 0, idx = 0; i < argc; i++)
                    {
                        if(!strcmp(argv[i], "--rc4") && (argc > i + 1))
                        {
                            rc4 = true;
                        }else if(!strcmp(argv[i], "--uart") && (argc > i + 1))
                        {
                            uart = strtoul(argv[i + 1], NULL, 0);
                            i++;
                        }else if(*argv[i] == '-')
                        {
                            usage();
                        }else if(*argv[i] != '-' && strcmp(argv[i], "-") != 0)
                        {
                            if(idx == 0)
                                addr = strtoul(argv[i], NULL, 0);
                            else if(idx == 1)
                                len = strtoul(argv[i], NULL, 0);
                            idx++;
                        }
                    }
                    if(ctx.maskrom_dump_arm64(uart, addr, len, rc4))
                        return 0;
                    printf("Error\r\n");
                }else
                    printf("ERROR: The chip '%s' does not in maskrom mode\r\n", ctx.chip_name());
            }else
                usage();
            return -1;
        }
        
        if(!strcmp(argv[0], "write-arm32"))
        {
            argc -= 1;
            argv += 1;
            if(argc >= 2)
            {
                if(ctx.is_maskrom())
                {
                    bool rc4 = false;
                    char* filename = NULL;
                    uint32_t addr = 0x0;
                    for(int i = 0, idx = 0; i < argc; i++)
                    {
                        if(!strcmp(argv[i], "--rc4") && (argc > i + 1))
                        {
                            rc4 = true;
                        }else if(*argv[i] == '-')
                        {
                            usage();
                        }else if(*argv[i] != '-' && strcmp(argv[i], "-") != 0)
                        {
                            if(idx == 0)
                                addr = strtoul(argv[i], NULL, 0);
                            else if(idx == 1)
                                filename = argv[i];
                            idx++;
                        }
                    }
                    uint64_t len;
                    void* buf = file_load(filename, &len);
                    if(buf)
                    {
                        if(ctx.maskrom_write_arm32_progress(addr, buf, len, rc4))
                        {
                            free(buf);
                            return 0;
                        }
                        free(buf);
                    }
                    printf("Error\r\n");
                }else
                    printf("ERROR: The chip '%s' does not in maskrom mode\r\n", ctx.chip_name());
            }else
                usage();
            return -1;
        }
        
        if(!strcmp(argv[0], "write-arm64"))
        {
            argc -= 1;
            argv += 1;
            if(argc >= 2)
            {
                if(ctx.is_maskrom())
                {
                    bool rc4 = false;
                    char* filename = NULL;
                    uint32_t addr = 0x0;
                    for(int i = 0, idx = 0; i < argc; i++)
                    {
                        if(!strcmp(argv[i], "--rc4") && (argc > i + 1))
                        {
                            rc4 = true;
                        }else if(*argv[i] == '-')
                        {
                            usage();
                        }else if(*argv[i] != '-' && strcmp(argv[i], "-") != 0)
                        {
                            if(idx == 0)
                                addr = strtoul(argv[i], NULL, 0);
                            else if(idx == 1)
                                filename = argv[i];
                            idx++;
                        }
                    }
                    uint64_t len;
                    void* buf = file_load(filename, &len);
                    if(buf)
                    {
                        if(ctx.maskrom_write_arm64_progress(addr, buf, len, rc4))
                        {
                            free(buf);
                            return 0;
                        }
                        free(buf);
                    }
                    printf("Error\r\n");
                }else
                    printf("ERROR: The chip '%s' does not in maskrom mode\r\n", ctx.chip_name());
            }else
                usage();
            return -1;
        }
        
        if(!strcmp(argv[0], "exec-arm32"))
        {
            argc -= 1;
            argv += 1;
            if(argc >= 2)
            {
                if(ctx.is_maskrom())
                {
                    bool rc4 = false;
                    uint32_t addr = 0x0;
                    for(int i = 0; i < argc; i++)
                    {
                        if(!strcmp(argv[i], "--rc4") && (argc > i + 1))
                        {
                            rc4 = true;
                        }else if(*argv[i] == '-')
                        {
                            usage();
                        }else if(*argv[i] != '-' && strcmp(argv[i], "-") != 0)
                        {
                            addr = strtoul(argv[i], NULL, 0);
                        }
                    }
                    if(ctx.maskrom_exec_arm32(addr, rc4))
                        return 0;
                    printf("Error\r\n");
                }else
                    printf("ERROR: The chip '%s' does not in maskrom mode\r\n", ctx.chip_name());
            }else
                usage();
            return -1;
        }
        
        if(!strcmp(argv[0], "exec-arm64"))
        {
            argc -= 1;
            argv += 1;
            if(argc >= 2)
            {
                if(ctx.is_maskrom())
                {
                    bool rc4 = false;
                    uint32_t addr = 0x0;
                    for(int i = 0; i < argc; i++)
                    {
                        if(!strcmp(argv[i], "--rc4") && (argc > i + 1))
                        {
                            rc4 = true;
                        }else if(*argv[i] == '-')
                        {
                            usage();
                        }else if(*argv[i] != '-' && strcmp(argv[i], "-") != 0)
                        {
                            addr = strtoul(argv[i], NULL, 0);
                        }
                    }
                    if(ctx.maskrom_exec_arm64(addr, rc4))
                        return 0;
                    printf("Error\r\n");
                }else
                    printf("ERROR: The chip '%s' does not in maskrom mode\r\n", ctx.chip_name());
            }else
                usage();
        }else
            usage();
    }else
        usage();
    return -1;
}
