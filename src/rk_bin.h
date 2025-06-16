#pragma once
#include <stddef.h>
#include <stdint.h>

enum RK_Boot_Entry_Type_t
{
    RKLOADER_ENTRY_471    = (1 << 0),
    RKLOADER_ENTRY_472    = (1 << 1),
    RKLOADER_ENTRY_LOADER = (1 << 2),
};

#pragma pack(push, 1)

struct RK_Boot_Time_t
{
    uint16_t m_year;
    uint8_t  m_month;
    uint8_t  m_day;
    uint8_t  m_hour;
    uint8_t  m_minute;
    uint8_t  m_second;
};

struct RK_Boot_Entry_t
{
    uint8_t  m_entry_size;
    RK_Boot_Entry_Type_t m_type;
    uint16_t m_name[20];
    uint32_t m_offset;
    uint32_t m_size;
    uint32_t m_delay;

    size_t GetName(char* out24)
    {
        size_t len;
        for(len = 0; len < 20; len++)
        {
            if(!m_name[len])
                break;
            out24[len] = m_name[len] & 0xff;
        }
        out24[len] = '\0';
        return len;
    }
};


struct RK_Boot_Header_t
{
    uint32_t m_magic;  // 'LDR '
    uint16_t m_size;
    uint32_t m_version;
    uint32_t m_merger_version;
    RK_Boot_Time_t m_release_time;
    uint32_t m_chiptype;
    uint8_t  m_code471_num;
    uint32_t m_code471_offset;
    uint8_t  m_code471_size;
    uint8_t  m_code472_num;
    uint32_t m_code472_offset;
    uint8_t  m_code472_size;
    uint8_t  m_loader_num;
    uint32_t m_loader_offset;
    uint8_t  m_loader_size;
    uint8_t  m_sign_flag;
    uint8_t  m_rc4_flag;
    uint8_t  m_reserved[57];
};

#pragma pack(pop)

class RK_bin
{
    void mkidb();
public:
    RK_bin();
    ~RK_bin();
    bool load(const char* filename);

    void* m_buffer;
    uint64_t m_length;

    bool m_is_newidb;
    bool m_is_sign;

    RK_Boot_Header_t* m_header;
    RK_Boot_Entry_t*  m_entry[32];
    uint32_t m_count_entry;

    void*    m_flash_data;
    uint64_t m_flash_len;
};


