#include "rk_common.h"

#ifdef _MSC_VER
#include <windows.h>
static int gettimeofday(timeval* tp, struct timezone* tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);
    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;
    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time =  ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;
    tp->tv_sec  = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}
#else
#include <sys/time.h>
#endif

static double gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

static const char* format_eta(double remaining)
{
    static char result[6] = "";
    int32_t seconds = int(remaining + 0.5);
    if(seconds >= 0 && seconds < 6000)
    {
        snprintf(result, sizeof(result), "%02d:%02d", seconds / 60, seconds % 60);
        return result;
    }
    return "--:--";
}

static char* ssize(char* buf, double size)
{
    const char* unit[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    size_t count = 0;
    while((size > 1024) && (count < 8))
    {
        size /= 1024;
        count++;
    }
    sprintf(buf, "%5.3f %s", size, unit[count]);
    return buf;
}

progress_t::progress_t()
{
    m_done  = 0;
    m_start = 0;
    m_total = 1;
}

void progress_t::start(uint64_t total)
{
    if(total == 0)
        total = 1;
    m_total = total;
    m_done  = 0;
    m_start = gettime();
}

void progress_t::update(uint64_t bytes)
{
    char buf1[32], buf2[32];
    m_done += bytes;
    double ratio = m_total > 0 ? (double)m_done / (double)m_total : 0.0;
    double speed = (double)m_done / (gettime() - m_start);
    double eta = speed > 0 ? (m_total - m_done) / speed : 0;
    size_t pos = size_t(48.0 * ratio);
    printf("\r%3.0f%% [", ratio * 100);
    for(size_t i = 0; i < pos; i++)
        putchar('=');
    for(size_t i = pos; i < 48; i++)
        putchar(' ');
    if(m_done < m_total)
        printf("] %s/s, ETA %s        \r", ssize(buf1, speed), format_eta(eta));
    else
        printf("] %s, %s/s        \r", ssize(buf1, (double)m_done), ssize(buf2, speed));
    fflush(stdout);
}

void progress_t::stop()
{
    if(m_start)
        printf("\r\n");
    m_start = 0;
}


void rc4_ctx::setkey(const uint8_t* key, size_t len)
{
    uint8_t k[256];
    for(size_t i = 0; i < 256; i++)
    {
        m_s[i] = (uint8_t)i;
        k[i] = key[i % len];
    }
    for(size_t i = 0, j = 0; i < 256; i++)
    {
        j = (j + m_s[i] + k[i]) & 0xff;
        uint8_t t = m_s[i];
        m_s[i] = m_s[j];
        m_s[j] = t;
    }
    m_i = 0;
    m_j = 0;
}


void rc4_ctx::crypt(uint8_t* buf, size_t len)
{
    for(size_t o = 0; o < len; o++)
    {
        m_i = (m_i + 1) & 0xff;
        m_j = (m_j + m_s[m_i]) & 0xff;
        uint8_t t = m_s[m_i];
        m_s[m_i] = m_s[m_j];
        m_s[m_j] = t;
        buf[o] ^= m_s[(m_s[m_i] + m_s[m_j]) & 0xff];
    }
}


size_t file_save(const char* filename, void* buf, size_t len)
{
    FILE* out;
    size_t r;

    if(strcmp(filename, "-") == 0)
        out = stdout;
    else
        out = fopen(filename, "wb");
    if(!out)
    {
        printf("Failed to open output file '%s'", filename);
        return 0;
    }
    r = (size_t)fwrite(buf, len, 1, out);
    if(out != stdout)
        fclose(out);
    return r;
}

void* file_load(const char* filename, size_t* len)
{
    uint64_t offset = 0, bufsize = 8192;
    char* buf = (char*)malloc(bufsize);
    FILE* in;
    if(strcmp(filename, "-") == 0)
        in = stdin;
    else
        in = fopen(filename, "rb");
    if(!in)
    {
        printf("Failed to open input file '%s'", filename);
        return NULL;
    }
    while(1)
    {
        uint64_t len = bufsize - offset;
        uint64_t n = fread(buf + offset, 1, len, in);
        offset += n;
        if(n < len)
            break;
        bufsize *= 2;
        buf = (char*)realloc(buf, bufsize);
        if(!buf)
        {
            perror("Failed to resize load_file() buffer");
            return NULL;
        }
    }
    if(len)
        *len = offset;
    if(in != stdin)
        fclose(in);
    return buf;
}

static inline unsigned char hex_to_bin(char c)
{
    if((c >= 'a') && (c <= 'f'))
        return c - 'a' + 10;
    if((c >= '0') && (c <= '9'))
        return c - '0';
    if((c >= 'A') && (c <= 'F'))
        return c - 'A' + 10;
    return 0;
}

void hexdump(uint32_t addr, void* buf, size_t len)
{
    uint8_t* p = (uint8_t*)buf;
    size_t i, j;

    for(j = 0; j < len; j += 16)
    {
        printf("%08x: ", (uint32_t)(addr + j));
        for(i = 0; i < 16; i++)
        {
            if(j + i < len)
                printf("%02x ", p[j + i]);
            else
                printf("   ");
        }
        putchar(' ');
        for(i = 0; i < 16; i++)
        {
            if(j + i >= len)
                putchar(' ');
            else
                putchar(isprint(p[j + i]) ? p[j + i] : '.');
        }
        printf("\r\n");
    }
}


uint32_t rk_crc32(uint32_t crc, const void* buf, size_t len)
{
#if 0
    static const uint32_t crc32_tbl[256] =
    {
        0x00000000, 0x04c10db7, 0x09821b6e, 0x0d4316d9, 0x130436dc, 0x17c53b6b, 0x1a862db2, 0x1e472005,
        0x26086db8, 0x22c9600f, 0x2f8a76d6, 0x2b4b7b61, 0x350c5b64, 0x31cd56d3, 0x3c8e400a, 0x384f4dbd,
        0x4c10db70, 0x48d1d6c7, 0x4592c01e, 0x4153cda9, 0x5f14edac, 0x5bd5e01b, 0x5696f6c2, 0x5257fb75,
        0x6a18b6c8, 0x6ed9bb7f, 0x639aada6, 0x675ba011, 0x791c8014, 0x7ddd8da3, 0x709e9b7a, 0x745f96cd,
        0x9821b6e0, 0x9ce0bb57, 0x91a3ad8e, 0x9562a039, 0x8b25803c, 0x8fe48d8b, 0x82a79b52, 0x866696e5,
        0xbe29db58, 0xbae8d6ef, 0xb7abc036, 0xb36acd81, 0xad2ded84, 0xa9ece033, 0xa4aff6ea, 0xa06efb5d,
        0xd4316d90, 0xd0f06027, 0xddb376fe, 0xd9727b49, 0xc7355b4c, 0xc3f456fb, 0xceb74022, 0xca764d95,
        0xf2390028, 0xf6f80d9f, 0xfbbb1b46, 0xff7a16f1, 0xe13d36f4, 0xe5fc3b43, 0xe8bf2d9a, 0xec7e202d,
        0x34826077, 0x30436dc0, 0x3d007b19, 0x39c176ae, 0x278656ab, 0x23475b1c, 0x2e044dc5, 0x2ac54072,
        0x128a0dcf, 0x164b0078, 0x1b0816a1, 0x1fc91b16, 0x018e3b13, 0x054f36a4, 0x080c207d, 0x0ccd2dca,
        0x7892bb07, 0x7c53b6b0, 0x7110a069, 0x75d1adde, 0x6b968ddb, 0x6f57806c, 0x621496b5, 0x66d59b02,
        0x5e9ad6bf, 0x5a5bdb08, 0x5718cdd1, 0x53d9c066, 0x4d9ee063, 0x495fedd4, 0x441cfb0d, 0x40ddf6ba,
        0xaca3d697, 0xa862db20, 0xa521cdf9, 0xa1e0c04e, 0xbfa7e04b, 0xbb66edfc, 0xb625fb25, 0xb2e4f692,
        0x8aabbb2f, 0x8e6ab698, 0x8329a041, 0x87e8adf6, 0x99af8df3, 0x9d6e8044, 0x902d969d, 0x94ec9b2a,
        0xe0b30de7, 0xe4720050, 0xe9311689, 0xedf01b3e, 0xf3b73b3b, 0xf776368c, 0xfa352055, 0xfef42de2,
        0xc6bb605f, 0xc27a6de8, 0xcf397b31, 0xcbf87686, 0xd5bf5683, 0xd17e5b34, 0xdc3d4ded, 0xd8fc405a,
        0x6904c0ee, 0x6dc5cd59, 0x6086db80, 0x6447d637, 0x7a00f632, 0x7ec1fb85, 0x7382ed5c, 0x7743e0eb,
        0x4f0cad56, 0x4bcda0e1, 0x468eb638, 0x424fbb8f, 0x5c089b8a, 0x58c9963d, 0x558a80e4, 0x514b8d53,
        0x25141b9e, 0x21d51629, 0x2c9600f0, 0x28570d47, 0x36102d42, 0x32d120f5, 0x3f92362c, 0x3b533b9b,
        0x031c7626, 0x07dd7b91, 0x0a9e6d48, 0x0e5f60ff, 0x101840fa, 0x14d94d4d, 0x199a5b94, 0x1d5b5623,
        0xf125760e, 0xf5e47bb9, 0xf8a76d60, 0xfc6660d7, 0xe22140d2, 0xe6e04d65, 0xeba35bbc, 0xef62560b,
        0xd72d1bb6, 0xd3ec1601, 0xdeaf00d8, 0xda6e0d6f, 0xc4292d6a, 0xc0e820dd, 0xcdab3604, 0xc96a3bb3,
        0xbd35ad7e, 0xb9f4a0c9, 0xb4b7b610, 0xb076bba7, 0xae319ba2, 0xaaf09615, 0xa7b380cc, 0xa3728d7b,
        0x9b3dc0c6, 0x9ffccd71, 0x92bfdba8, 0x967ed61f, 0x8839f61a, 0x8cf8fbad, 0x81bbed74, 0x857ae0c3,
        0x5d86a099, 0x5947ad2e, 0x5404bbf7, 0x50c5b640, 0x4e829645, 0x4a439bf2, 0x47008d2b, 0x43c1809c,
        0x7b8ecd21, 0x7f4fc096, 0x720cd64f, 0x76cddbf8, 0x688afbfd, 0x6c4bf64a, 0x6108e093, 0x65c9ed24,
        0x11967be9, 0x1557765e, 0x18146087, 0x1cd56d30, 0x02924d35, 0x06534082, 0x0b10565b, 0x0fd15bec,
        0x379e1651, 0x335f1be6, 0x3e1c0d3f, 0x3add0088, 0x249a208d, 0x205b2d3a, 0x2d183be3, 0x29d93654,
        0xc5a71679, 0xc1661bce, 0xcc250d17, 0xc8e400a0, 0xd6a320a5, 0xd2622d12, 0xdf213bcb, 0xdbe0367c,
        0xe3af7bc1, 0xe76e7676, 0xea2d60af, 0xeeec6d18, 0xf0ab4d1d, 0xf46a40aa, 0xf9295673, 0xfde85bc4,
        0x89b7cd09, 0x8d76c0be, 0x8035d667, 0x84f4dbd0, 0x9ab3fbd5, 0x9e72f662, 0x9331e0bb, 0x97f0ed0c,
        0xafbfa0b1, 0xab7ead06, 0xa63dbbdf, 0xa2fcb668, 0xbcbb966d, 0xb87a9bda, 0xb5398d03, 0xb1f880b4,
    };
#else
    static uint32_t crc32_tbl[256] ={ };
    if(crc32_tbl[255] == 0)
    {
        const uint32_t Poly = 0x04C10DB7;
        for(size_t i = 0; i < 256; i++)
        {
            uint32_t c = i << 24;
            for(size_t j = 0; j < 8; j++)
            {
                bool b = (c & 0x80000000);
                c <<= 1;
                if(b)
                    c ^= Poly;
            }
            crc32_tbl[i] = c;
        }
    }
#endif
    const uint8_t* pBytes = (const uint8_t*)buf;
    for(size_t i = 0; i < len; i++)
        crc = (crc << 8) ^ crc32_tbl[(crc >> 24) ^ pBytes[i]];
    return crc;
}


uint16_t rk_crc16(uint16_t crc, const void* buf, size_t len)
{
#if 0
    static const uint16_t crc16_tbl[256] =
    {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
        0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
        0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
        0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
        0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
        0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
        0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
        0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
        0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
        0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
        0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
        0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
        0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
        0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
        0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
        0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
        0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
        0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
        0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
        0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
        0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
        0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
    };
#else
    static uint16_t crc16_tbl[256] ={ };
    if(crc16_tbl[255] == 0)
    {
        const uint16_t poly = 0x1021;
        for(size_t i = 0; i < 256; i++)
        {
            uint16_t c = i << 8;
            for(size_t j = 0; j < 8; j++)
            {
                bool b = (c & 0x8000);
                c <<= 1;
                if(b)
                    c ^= poly;
            }
            crc16_tbl[i] = c;
        }
    }
#endif
    const uint8_t* pBytes = (const uint8_t*)buf;
    for(size_t i = 0; i < len; i++)
        crc = (crc << 8) ^ crc16_tbl[(crc >> 8) ^ pBytes[i]];
    return crc;
}


void rk_rc4(const void* buf, size_t len)
{
    rc4_ctx rc4;
    static const uint8_t key[16] = { 124, 78, 3, 4, 85, 5, 9, 7, 45, 44, 123, 56, 23, 13, 23, 17 };
    rc4.setkey(key, sizeof(key));
    rc4.crypt((uint8_t*)buf, len);
}
