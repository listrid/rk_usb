#pragma once
#include <stddef.h>
#include <stdint.h>

enum rk_bin_entry_type_t
{
    RKLOADER_ENTRY_471    = (1 << 0),
    RKLOADER_ENTRY_472    = (1 << 1),
    RKLOADER_ENTRY_LOADER = (1 << 2),
};

#pragma pack(push, 1)

struct rk_bin_time_t
{
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
};


struct rk_bin_header_t
{
    uint32_t tag;
    uint16_t size;
    uint32_t version;
    uint32_t merger_version;
    rk_bin_time_t release_time;
    uint32_t chiptype;
    uint8_t  code471_num;
    uint32_t code471_offset;
    uint8_t  code471_size;
    uint8_t  code472_num;
    uint32_t code472_offset;
    uint8_t  code472_size;
    uint8_t  loader_num;
    uint32_t loader_offset;
    uint8_t  loader_size;
    uint8_t  sign_flag;
    uint8_t  rc4_flag;
    uint8_t  reserved[57];
};

struct rk_bin_entry_t
{
    uint8_t  size;
    uint32_t type;
    uint16_t name[20];
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t data_delay;
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
    bool m_is_rc4on;
    bool m_is_sign;
    rk_bin_header_t* m_header;
    rk_bin_entry_t*  m_entry[32];
    uint32_t m_nentry;
    void* m_idbbuf;
    uint64_t m_idblen;
};


char* wide2str(char* str, uint8_t* wide, size_t len);



