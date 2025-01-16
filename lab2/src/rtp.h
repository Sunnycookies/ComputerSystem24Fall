#ifndef __RTP_H
#define __RTP_H

#include <stdint.h>
#include "util.h"
#include "defs.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // flags in the rtp header
    typedef enum RtpHeaderFlag
    {
        RTP_SYN = 0b0001,
        RTP_ACK = 0b0010,
        RTP_FIN = 0b0100,
    } rtp_header_flag_t;

    typedef struct __attribute__((__packed__)) RtpHeader
    {
        uint32_t seq_num;  // Sequence number
        uint16_t length;   // Length of data; 0 for SYN, ACK, and FIN packets
        uint32_t checksum; // 32-bit CRC
        uint8_t flags;     // See at `RtpHeaderFlag`
    } rtp_header_t;

    typedef struct __attribute__((__packed__)) RtpPacket
    {
        rtp_header_t rtp;          // header
        char payload[PAYLOAD_MAX]; // data
    } rtp_packet_t;

    void show_packet(rtp_packet_t *packet, int sendrecv);

    void send_packet(int fd, struct sockaddr *sa, uint32_t sqn,
                     uint8_t flgs, int size, void *buf);

    int recv_packet(int fd, struct sockaddr *sa, socklen_t *len, rtp_packet_t *buf);

    void set_sock_recv_time(int fd, int sec, int msec);

#ifdef __cplusplus
}
#endif

#endif // __RTP_H