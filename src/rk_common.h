#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


size_t file_save(const char* filename, void* buf, size_t len);
void* file_load(const char* filename, size_t* len);
void hexdump(uint32_t addr, void* buf, size_t len);


class progress_t
{
    uint64_t m_total;
    uint64_t m_done;
    double   m_start;
public:
    progress_t();
    progress_t(uint64_t total){ start(total); };
    ~progress_t(){ stop(); }
    void start(uint64_t total);
    void update(uint64_t bytes);
    void stop();
};

// !!! Not standard CRC32 CRC16
uint32_t rk_crc32(uint32_t crc, const void* buf, size_t len);
uint16_t rk_crc16(uint16_t crc, const void* buf, size_t len);

void rk_rc4(const void* buf, size_t len);

class rc4_ctx
{
    uint8_t m_s[256];
    size_t  m_i;
    size_t  m_j;
public:
    void setkey(const uint8_t* key, size_t len);
    void crypt(uint8_t* buf, size_t len);
};





