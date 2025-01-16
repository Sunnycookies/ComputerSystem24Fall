#include <string.h>
#include <iostream>
#include <fstream>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#define MAGIC_NUMBER_LEN 6

// #define DEBUG

typedef char byte;
typedef uint8_t type;
typedef uint8_t status;

FILE *dfp = stdout;

struct ftp_header
{
    byte m_protocol[MAGIC_NUMBER_LEN];
    type m_type;
    status m_status;
    uint32_t m_length;

    ftp_header(type type_, uint32_t length_, status status_)
    {
        static char MAGIC_NUMBER[] = "\xc1\xa1\x10"
                                     "ftp";
        memcpy((void *)m_protocol, (void *)MAGIC_NUMBER, MAGIC_NUMBER_LEN);
        m_status = status_;
        m_type = type_;
        m_length = htonl(length_);
    }

    ftp_header() {}

    void show(int sendrecv, FILE *fp = dfp)
    {
        if (sendrecv == 0)
        {
            fprintf(fp, "SEND FTP HEADER\n");
        }
        else
        {
            fprintf(fp, "RECV FTP HEADER\n");
        }
        fprintf(fp, "* protocol: %02x%02x%02x%c%c%c\n",
                (unsigned char)m_protocol[0], (unsigned char)m_protocol[1],
                (unsigned char)m_protocol[2], m_protocol[3], m_protocol[4], m_protocol[5]);
        fprintf(fp, "* type: %02x\n", (unsigned char)m_type);
        fprintf(fp, "* status: %d\n", m_status);
        fprintf(fp, "* length: %d\n\n", ntohl(m_length));
    }
} __attribute__((packed));

const size_t HEADER_SIZE = sizeof(ftp_header);

int serror(const char *msg, FILE *fp = dfp)
{
    fprintf(fp, "<== error ==>: %s\n", msg);
    return -1;
}

int ssend(int fd, void *buf, int size)
{
    size_t ret = 0;
    while (ret < size)
    {
        ssize_t b = send(fd, buf + ret, size - ret, 0);
        if (b == 0)
        {
            return serror("socket closed");
        }
        if (b < 0)
        {
            return serror("ssend error");
        }
        ret += b;
    }
    return ret;
}

int srecv(int fd, void *buf, int size)
{
    size_t ret = 0;
    while (ret < size)
    {
        ssize_t b = recv(fd, buf + ret, size - ret, 0);
        if (b == 0)
        {
            return serror("socket closed");
        }
        if (b < 0)
        {
            return serror("srecv error");
        }
        ret += b;
    }
    return ret;
}

void show_data(void *buf, int size, FILE *fp = dfp)
{
    char *str = (char *)buf;
    for (int i = 0; i < size; ++i)
    {
        fprintf(fp, "%c", str[i]);
    }
    fprintf(fp, "\n");
}

int send_post(int fd, type type, void *buf = nullptr, int size = 0, status status = 0)
{
    struct ftp_header header(type, HEADER_SIZE + size, status);
#ifdef DEBUG
    header.show(0);
    show_data(buf, size);
#endif
    int scode;
    if ((scode = ssend(fd, (void *)&header, HEADER_SIZE)) <= 0)
    {
        return scode;
    }
    return ssend(fd, buf, size);
}

int recv_post(int fd, void *buf, type *ptype, status *pstatus = nullptr)
{
    struct ftp_header header;
    int scode;
    int length;
    if ((scode = srecv(fd, (void *)&header, HEADER_SIZE)) <= 0)
    {
        return scode;
    }
    *ptype = header.m_type;
    length = ntohl(header.m_length) - HEADER_SIZE;
    if (pstatus != nullptr)
    {
        *pstatus = header.m_status;
    }
    int size = srecv(fd, buf, length);
#ifdef DEBUG
    header.show(1);
    show_data(buf, size);
#endif
    return size;
}

int write_file(char *filename, void *buf, int size)
{
    std::ofstream ofs(filename, std::ios::out | std::ios::binary);
    if (!ofs)
    {
        return serror("open file error (w)");
    }
#ifdef DEBUG
    fprintf(dfp, "WRITE DATA\n");
    show_data(buf, size);
#endif
    ofs.write((char *)buf, size);
    ofs.close();
    return 0;
}

int read_file(char *filename, void *buf, int size)
{
    std::ifstream ifs(filename, std::ios::in | std::ios::binary);
    if (!ifs)
    {
        return serror("open file error (r)");
    }
    ifs.seekg(0, std::ios::end);
    int fsize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    fsize = std::min(size, fsize);
    ifs.read((char *)buf, fsize);
#ifdef DEBUG
    fprintf(dfp, "READ DATA\n");
    show_data(buf, size);
#endif
    ifs.close();
    return fsize;
}