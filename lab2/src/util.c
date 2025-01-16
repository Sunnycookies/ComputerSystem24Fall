#include "util.h"
#include "time.h"
#include "math.h"
#include <string.h>

static uint32_t crc32_for_byte(uint32_t r)
{
    for (int j = 0; j < 8; ++j)
        r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
    return r ^ (uint32_t)0xFF000000L;
}

static void crc32(const void *data, size_t n_bytes, uint32_t *crc)
{
    static uint32_t table[0x100];
    if (!*table)
        for (size_t i = 0; i < 0x100; ++i)
            table[i] = crc32_for_byte(i);
    for (size_t i = 0; i < n_bytes; ++i)
        *crc = table[(uint8_t)*crc ^ ((uint8_t *)data)[i]] ^ *crc >> 8;
}

// Computes checksum for `n_bytes` of data
//
// Hint 1: Before computing the checksum, you should set everything up
// and set the "checksum" field to 0. And when checking if a packet
// has the correct check sum, don't forget to set the "checksum" field
// back to 0 before invoking this function.
//
// Hint 2: `len + sizeof(rtp_header_t)` is the real length of a rtp
// data packet.
uint32_t compute_checksum(const void *pkt, size_t n_bytes)
{
    uint32_t crc = 0;
    crc32(pkt, n_bytes, &crc);
    return crc;
}

int read_file(char *filename, void *buf, int size)
{
    FILE *f;
    int fsize;

    if (!(f = fopen(filename, "r")))
    {
        LOG_FATAL("open file error (r)\n");
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize > size)
    {
        fsize = size;
    }

    if ((fsize = fread(buf, 1, fsize, f)) < 0)
    {
        LOG_FATAL("read file error\n");
    }

    LOG_DEBUG("READ DATA\n");

    if (fclose(f) < 0)
    {
        LOG_FATAL("close file error (r)\n");
    }
    return fsize;
}

void write_file(char *filename, void *buf, int size)
{
    FILE *f;

    if (!(f = fopen(filename, "w")))
    {
        LOG_FATAL("open file error (w)\n");
    }

    if (fwrite(buf, 1, size, f) < 0)
    {
        LOG_FATAL("write file error\n");
    }

    LOG_DEBUG("WRITE DATA\n");

    if (fclose(f) < 0)
    {
        LOG_FATAL("close file error (w)\n");
    }
}

uint32_t get_random_seqnum()
{
    return INT32_MAX;
}

uint32_t seqnum_add(uint32_t seqnum, int addnum)
{
    return (seqnum + addnum) & 0x7fffffff;
}

int sqn_dis(uint32_t l, uint32_t r)
{
    return (r - l) & 0x7fffffff;
}

int seqnum_is_in(uint32_t seqnum, uint32_t l, uint32_t r)
{
    if (l < r)
    {
        return seqnum >= l && seqnum < r;
    }
    return seqnum >= l || seqnum < r;
}

int update_window(int *window, int wsize, int ind)
{
    int shift = 0, lind = 0, rind;
    window[ind] = 1;
    while (shift < wsize && window[shift])
    {
        ++shift;
    }
    rind = shift;
    while (rind < wsize)
    {
        window[lind++] = window[rind++];
    }
    memset(window + lind, 0, (wsize - lind) * sizeof(int));
    return shift;
}