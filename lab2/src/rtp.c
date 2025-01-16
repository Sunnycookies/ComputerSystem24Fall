#include "rtp.h"

const size_t RTP_HEADER_SIZE = sizeof(rtp_header_t);

void show_packet(rtp_packet_t *packet, int sendrecv)
{
    if (sendrecv == 0)
    {
        LOG_DEBUG("SEND RTP PACKET\n");
    }
    else
    {
        LOG_DEBUG("RECV RTP PACKET\n");
    }
    // LOG_DEBUG("* seq_num: %u\n", packet.rtp.seq_num);
    // LOG_DEBUG("* length: %d\n", packet.rtp.length);
    // LOG_DEBUG("* checksum: %x\n", packet.rtp.checksum);
    // LOG_DEBUG("* flags: %d\n", packet.rtp.flags);
    LOG_DEBUG("* seq_num: %u length: %d checksum: %x flags: %d\n",
              packet->rtp.seq_num, packet->rtp.length,
              packet->rtp.checksum, packet->rtp.flags);
    // LOG_DEBUG("* data: %s\n", packet->payload);
}

void send_packet(int fd, struct sockaddr *sa, uint32_t sqn,
                 uint8_t flgs, int size, void *buf)
{
    rtp_packet_t packet;
    packet.rtp.seq_num = sqn;
    packet.rtp.length = (uint16_t)size;
    packet.rtp.checksum = 0;
    packet.rtp.flags = flgs;
    if (buf)
    {
        memcpy((void *)packet.payload, buf, size);
    }
    int packet_size = size + RTP_HEADER_SIZE;
    packet.rtp.checksum = compute_checksum(&packet, packet_size);

#ifdef LDEBUG
    show_packet(&packet, 0);
#endif

    if (sendto(fd, (void *)&packet, packet_size, 0, sa, sizeof(*sa)) < 0)
    {
        LOG_FATAL("send packet error\n");
    }
}

int recv_packet(int fd, struct sockaddr *sa, socklen_t *len, rtp_packet_t *buf)
{
    int size = recvfrom(fd, (void *)buf, sizeof(*buf), 0, sa, len);

    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        errno = 0;
        LOG_DEBUG("recv packet timeout\n");
        return -2;
    }

    if (size < 0)
    {
        LOG_FATAL("recv packet error\n");
    }

#ifdef LDEBUG
    show_packet(buf, 1);
#endif

    uint32_t cksm = buf->rtp.checksum;
    buf->rtp.checksum = 0;
    if (cksm != compute_checksum(buf, size))
    {
        LOG_DEBUG("packet checksum error\n");
        return -1;
    }

    if (buf->rtp.length > PAYLOAD_MAX ||
        buf->rtp.length + RTP_HEADER_SIZE != size)
    {
        LOG_DEBUG("packet length error\n");
        return -1;
    }

    return 0;
}

void set_sock_recv_time(int fd, int sec, int msec)
{
    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = msec * 1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        LOG_FATAL("set socket recv timeout error\n");
    }
}