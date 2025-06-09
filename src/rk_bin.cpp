#include "rk_common.h"
#include "rk_bin.h"

#pragma pack(push, 1)

struct idblock0_t
{
    uint32_t signature;
    uint8_t  reserved0[4];
    uint32_t disable_rc4;
    uint16_t bootcode1_offset;
    uint16_t bootcode2_offset;
    uint8_t  reserved1[490];
    uint16_t flash_data_size;
    uint16_t flash_boot_size;
    uint16_t crc;
};


struct idblock1_t
{
    uint16_t sys_reserved_block;
    uint16_t disk0_size;
    uint16_t disk1_size;
    uint16_t disk2_size;
    uint16_t disk3_size;
    uint32_t chip_tag;
    uint32_t machine_id;
    uint16_t loader_year;
    uint16_t loader_date;
    uint16_t loader_ver;
    uint8_t reserved0[72];
    uint16_t flash_data_offset;
    uint16_t flash_data_len;
    uint8_t reserved1[384];
    uint32_t flash_chip_size;
    uint8_t reserved2;
    uint8_t access_time;
    uint16_t phy_block_size;
    uint8_t phy_page_size;
    uint8_t ecc_bits;

    uint8_t reserved3[8];
    uint16_t id_block0;
    uint16_t id_block1;
    uint16_t id_block2;
    uint16_t id_block3;
    uint16_t id_block4;
};


struct idblock2_t
{
    uint16_t chip_info_size;
    uint8_t  chip_info[16];
    uint8_t  reserved[473];
    uint8_t  sz_vc_tag[3];
    uint16_t sec0_crc;
    uint16_t sec1_crc;
    uint32_t boot_code_crc;
    uint16_t sec3_custom_data_offset;
    uint16_t sec3_custom_data_size;
    uint8_t  sz_crc_tag[4];
    uint16_t sec3_crc;
};


struct idblock3_t
{
    uint16_t sn_size;
    uint8_t sn[60];
    uint8_t reserved[382];
    uint8_t wifi_size;
    uint8_t wifi_addr[6];
    uint8_t imei_size;
    uint8_t imei[15];
    uint8_t uid_size;
    uint8_t uid[30];
    uint8_t bluetooth_size;
    uint8_t bluetooth_addr[6];
    uint8_t mac_size;
    uint8_t mac_addr[6];
};

#pragma pack(pop)

static inline uint32_t __get_unaligned_le32(const uint8_t* p)
{
    return (p[0] << 0) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}


static inline uint32_t get_unaligned_le32(const void* p)
{
    return __get_unaligned_le32((const uint8_t*)p);
}


static inline uint32_t __swab32(uint32_t x)
{
    return ((x << 24) | (x >> 24) | \
            ((x & (uint32_t)0x0000ff00UL)<<8) | \
            ((x & (uint32_t)0x00ff0000UL)>>8));
}


static inline bool cpu_is_big_endian(void)
{
    const uint16_t endian = 256;
    return (*(const uint8_t*)&endian) ? true : false;
}


static inline uint32_t le32_to_cpu(uint32_t x)
{
    if(cpu_is_big_endian())
        return __swab32(x);
    return x;
}


static inline uint32_t loader_align_size(uint32_t len)
{
    uint32_t t = (len - 1) / 512 + 1;
    return ((t - 1) / 4 + 1) * 2048;
}


static void idb_rc4(const void* buf, size_t len)
{
    rc4_ctx rc4;
    uint8_t key[16] = { 124, 78, 3, 4, 85, 5, 9, 7, 45, 44, 123, 56, 23, 13, 23, 17 };

    rc4.setkey(key, sizeof(key));
    rc4.crypt((uint8_t*)buf, len);
}


void RK_bin::mkidb()
{
    if(m_is_newidb)
    {
        m_idblen = 0;
        for(uint32_t i = 0; i < m_nentry; i++)
        {
            rk_bin_entry_t* e = m_entry[i];
            if(e->type == RKLOADER_ENTRY_LOADER)
                m_idblen += loader_align_size(get_unaligned_le32(&e->data_size));
        }
        if(m_idblen > 0)
        {
            m_idbbuf = calloc(1, m_idblen);
            if(m_idbbuf)
            {
                uint32_t flash_head_index = 0;
                uint32_t idblen = 0;

                for(flash_head_index = 0; flash_head_index < m_nentry; flash_head_index++)
                {
                    rk_bin_entry_t* e = m_entry[flash_head_index];
                    if(e->type == RKLOADER_ENTRY_LOADER)
                    {
                        char str[256];
                        wide2str(str, (uint8_t*)&e->name[0], sizeof(e->name));
                        if(strcmp(str, "FlashHead") == 0)
                        {
                            uint32_t len = loader_align_size(get_unaligned_le32(&e->data_size));
                            memset((char*)m_idbbuf + idblen, 0, len);
                            memcpy((char*)m_idbbuf + idblen, (char*)m_buffer + get_unaligned_le32(&e->data_offset), get_unaligned_le32(&e->data_size));
                            if(!m_is_rc4on)
                            {
                                for(size_t i = 0; i < (len / 512); i++)
                                    idb_rc4((char*)m_idbbuf + idblen + 512 * i, 512);
                            }
                            idblen += len;
                            break;
                        }
                    }
                }
                for(uint32_t idx = 0; idx < m_nentry; idx++)
                {
                    rk_bin_entry_t* e = m_entry[idx];
                    if((e->type == RKLOADER_ENTRY_LOADER) && (idx != flash_head_index))
                    {
                        uint32_t len = loader_align_size(get_unaligned_le32(&e->data_size));
                        memset((char*)m_idbbuf + idblen, 0, len);
                        memcpy((char*)m_idbbuf + idblen, (char*)m_buffer + get_unaligned_le32(&e->data_offset), get_unaligned_le32(&e->data_size));
                        if(!m_is_rc4on)
                        {
                            for(size_t i = 0; i < (len / 512); i++)
                                idb_rc4((char*)m_idbbuf + idblen + 512 * i, 512);
                        }
                        idblen += len;
                    }
                }
            }
        }
    }else{
        m_idblen = 0;
        for(uint32_t i = 0; i < m_nentry; i++)
        {
            rk_bin_entry_t* e = m_entry[i];
            if(e->type == RKLOADER_ENTRY_LOADER)
            {
                char str[256];
                wide2str(str, (uint8_t*)&e->name[0], sizeof(e->name));
                if((strcmp(str, "FlashBoot") == 0) || (strcmp(str, "FlashData") == 0))
                    m_idblen += loader_align_size(get_unaligned_le32(&e->data_size));
            }
        }
        if(m_idblen > 0)
        {
            m_idblen += sizeof(idblock0_t) + sizeof(idblock1_t) + sizeof(idblock2_t) + sizeof(idblock3_t);
            m_idbbuf = calloc(1, m_idblen);
            if(m_idbbuf)
            {
                rk_bin_entry_t* flash_data = NULL;
                rk_bin_entry_t* flash_boot = NULL;
                uint32_t idblen = 0;
                uint32_t len;

                for(uint32_t i = 0; i < m_nentry; i++)
                {
                    rk_bin_entry_t* e = m_entry[i];
                    if(e->type == RKLOADER_ENTRY_LOADER)
                    {
                        char str[256];
                        wide2str(str, (uint8_t*)&e->name[0], sizeof(e->name));
                        if(strcmp(str, "FlashData") == 0)
                            flash_data = e;
                        else if(strcmp(str, "FlashBoot") == 0)
                            flash_boot = e;
                    }
                }

                idblock0_t* idb0 = (idblock0_t*)((char*)m_idbbuf + idblen);
                idb0->signature = 0x0ff0aa55;
                idb0->disable_rc4 = m_is_rc4on ? 0 : 1;
                idb0->bootcode1_offset = 4;
                idb0->bootcode2_offset = 4;
                idb0->flash_data_size = loader_align_size(get_unaligned_le32(&flash_data->data_size)) / 512;
                idb0->flash_boot_size = loader_align_size(get_unaligned_le32(&flash_data->data_size)) / 512 + loader_align_size(get_unaligned_le32(&flash_boot->data_size)) / 512;
                idblen += 512;

                idblock1_t* idb1 = (idblock1_t*)((char*)m_idbbuf + idblen);
                idb1->sys_reserved_block = 0xc;
                idb1->disk0_size = 0xffff;
                idb1->chip_tag = 0x38324b52;
                idblen += 512;

                idblock2_t* idb2 = (idblock2_t*)((char*)m_idbbuf + idblen);
                strcpy((char*)idb2->sz_vc_tag, "VC");
                strcpy((char*)idb2->sz_crc_tag, "CRC");
                idblen += 512;

                idblock3_t* idb3 = (idblock3_t*)((char*)m_idbbuf + idblen);
                memset(idb3, 0, sizeof(idblock3_t));
                idblen += 512;

                len = loader_align_size(get_unaligned_le32(&flash_data->data_size));
                memset((char*)m_idbbuf + idblen, 0, len);
                memcpy((char*)m_idbbuf + idblen, (char*)m_buffer + get_unaligned_le32(&flash_data->data_offset), get_unaligned_le32(&flash_data->data_size));
                if(idb0->disable_rc4)
                {
                    for(size_t i = 0; i < (len / 512); i++)
                        idb_rc4((char*)m_idbbuf + idblen + 512 * i, 512);
                }
                idblen += len;

                len = loader_align_size(get_unaligned_le32(&flash_boot->data_size));
                memset((char*)m_idbbuf + idblen, 0, len);
                memcpy((char*)m_idbbuf + idblen, (char*)m_buffer + get_unaligned_le32(&flash_boot->data_offset), get_unaligned_le32(&flash_boot->data_size));
                if(idb0->disable_rc4)
                {
                    for(size_t i = 0; i < (len / 512); i++)
                        idb_rc4((char*)m_idbbuf + idblen + 512 * i, 512);
                }
                idblen += len;

                idb2->sec0_crc = crc16_sum(0xffff, (uint8_t*)idb0, 512);
                idb2->sec1_crc = crc16_sum(0xffff, (uint8_t*)idb1, 512);
                idb2->sec3_crc = crc16_sum(0xffff, (uint8_t*)idb3, 512);

                idb_rc4(idb0, sizeof(idblock0_t));
                idb_rc4(idb2, sizeof(idblock2_t));
                idb_rc4(idb3, sizeof(idblock3_t));
            }
        }
    }
}


char* wide2str(char* str, uint8_t* wide, size_t len)
{
    for(size_t i = 0; i < len; i++)
    {
        char c = wide[i * 2 + 0] & 0xff;
        str[i] = (c && isprint(c)) ? c : '\0';
    }
    str[len] = '\0';
    return str;
}


bool RK_bin::load(const char* filename)
{
    if(m_buffer)
        free(m_buffer);
    m_buffer = file_load(filename, &m_length);
    if(!m_buffer || m_length <= sizeof(rk_bin_header_t))
    {
        free(m_buffer);
        m_buffer =NULL;
        return false;
    }
    m_header = (rk_bin_header_t*)m_buffer;
    
    if((le32_to_cpu(m_header->tag) != 0x544f4f42) && (le32_to_cpu(m_header->tag) != 0x2052444c))// 'BOOT' && 'LDR '
    {
        free(m_buffer);
        m_buffer =NULL;
        return false;
    }

    m_is_newidb = (le32_to_cpu(m_header->tag) == 0x2052444c); //'LDR '

    if(m_header->rc4_flag)
        m_is_rc4on = 0;
    else
        m_is_rc4on = 1;

    m_is_sign = (m_header->sign_flag == 'S');

    m_nentry = m_header->code471_num + m_header->code472_num + m_header->loader_num;
    if(m_nentry <= 0)
    {
        free(m_buffer);
        m_buffer = NULL;
        return NULL;
    }

    for(size_t i = 0; i < m_nentry; i++)
    {
        m_entry[i] = (rk_bin_entry_t*)( (uint8_t*)m_buffer + sizeof(rk_bin_header_t) + sizeof(rk_bin_entry_t) * i);
    }

    rk_bin_entry_t* e = m_entry[m_nentry - 1];
    uint32_t len = get_unaligned_le32(&e->data_offset) + get_unaligned_le32(&e->data_size);
    if(m_length != len + 4)
    {
        free(m_buffer);
        m_buffer = NULL;
        return NULL;
    }

    uint32_t crc32 = 0x0;
    if(crc32_sum(crc32, (uint8_t*)m_buffer, len) != get_unaligned_le32((char*)m_buffer + len))
    {
        free(m_buffer);
        m_buffer = NULL;
        return NULL;
    }

    mkidb();
    return true;
}


RK_bin::RK_bin()
{
    m_buffer = NULL;
    m_idbbuf = NULL;
    m_length = 0;
    m_header = NULL;
    memset(m_entry, 0, sizeof(m_entry));
}


RK_bin::~RK_bin()
{
    free(m_buffer);
    free(m_idbbuf);
}

