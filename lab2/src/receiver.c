#include "rtp.h"
#include "util.h"

int sock;
struct sockaddr_in addr;
socklen_t addrlen;
uint32_t sqn;

void establish_connection()
{
    rtp_packet_t packet;
    int tries = 0;
    int scode;

    // recv the first handsake
    set_sock_recv_time(sock, WAIT_S, 0);
    while (1)
    {
        scode = recv_packet(sock, (struct sockaddr *)&addr, &addrlen, &packet);
        if (scode == -2)
        {
            LOG_FATAL("Receiver: recv conn packet error\n");
        }
        if (!scode && packet.rtp.flags == RTP_SYN) {
            break;
        }
    }

    // send the second handshake
    sqn = seqnum_add(packet.rtp.seq_num, 1);
    set_sock_recv_time(sock, 0, SEND_MS);
    LOG_DEBUG("Receiver: send conn reply packet (times: %d)\n", ++tries);
    send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_ACK | RTP_SYN, 0, NULL);
    while (1)
    {
        if (tries > SEND_TR)
        {
            LOG_FATAL("Receiver: send conn reply packet tries used up: %d\n", tries);
        }
        // recv the third handshake
        if (recv_packet(sock, (struct sockaddr *)&addr, &addrlen, &packet) == 0)
        {
            if (packet.rtp.seq_num == sqn && packet.rtp.flags == RTP_ACK)
            {
                LOG_DEBUG("Receiver: send conn reply packet success\n");
                break;
            }
        }
        LOG_DEBUG("Receiver: send conn reply packet (times: %d)\n", ++tries);
        send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_ACK | RTP_SYN, 0, NULL);
    }

    LOG_DEBUG("Receiver: connection established\n");
}

void break_connection()
{
    rtp_packet_t packet;
    int scode;

    // recv the first handsake
    set_sock_recv_time(sock, WAIT_S, 0);
    while (1)
    {
        scode = recv_packet(sock, (struct sockaddr *)&addr, &addrlen, &packet) < 0;
        if (scode == -2)
        {
            LOG_FATAL("Receiver: recv break packet error\n");
        }
        if (!scode && packet.rtp.flags == RTP_FIN)
        {
            break;
        }
    }

    // send the second handshake
    set_sock_recv_time(sock, RECV_S, 0);
    sqn = packet.rtp.seq_num;
    send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_ACK | RTP_FIN, 0, NULL);
    scode = recv_packet(sock, (struct sockaddr *)&addr, &addrlen, &packet);
    sqn = packet.rtp.seq_num;
    if (!scode && packet.rtp.seq_num == sqn && packet.rtp.flags == RTP_FIN)
    {
        send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_ACK | RTP_FIN, 0, NULL);
    }

    LOG_DEBUG("Receiver: connection stopped\n");
}

void transfer_data(char *filename, int wsize, int mode)
{
    rtp_packet_t packet;
    int fsize = 0, scode, shift = 0, mov;
    int window[wsize];
    char buf[MAXBUF];
    uint32_t wbase = sqn;

    memset(window, 0, sizeof(window));

    set_sock_recv_time(sock, WAIT_S, 0);
    while (1)
    {
        scode = recv_packet(sock, (struct sockaddr *)&addr, &addrlen, &packet);
        // data transfer terminated
        if (scode == -2 || packet.rtp.flags & RTP_FIN)
        {
            break;
        }
        // invalid packet
        if (scode || packet.rtp.flags != 0)
        {
            continue;
        }
        // SR
        if (mode)
        {
            if (seqnum_is_in(packet.rtp.seq_num, seqnum_add(wbase, -wsize), wbase))
            {
                send_packet(sock, (struct sockaddr *)&addr,
                            packet.rtp.seq_num, RTP_ACK, 0, NULL);
            }
            else if (seqnum_is_in(packet.rtp.seq_num, wbase, seqnum_add(wbase, wsize)))
            {
                mov = sqn_dis(wbase, packet.rtp.seq_num) + shift;
                LOG_DEBUG("Receiver: recv packet %d\n", mov + 1);
                memcpy((void *)buf + mov * PAYLOAD_MAX, (void *)packet.payload,
                       packet.rtp.length);
                if (fsize < mov * PAYLOAD_MAX + packet.rtp.length)
                {
                    fsize = mov * PAYLOAD_MAX + packet.rtp.length;
                }
                mov = update_window(window, wsize, mov - shift);
                shift += mov;
                wbase = seqnum_add(wbase, mov);
                send_packet(sock, (struct sockaddr *)&addr,
                            packet.rtp.seq_num, RTP_ACK, 0, NULL);
            }
        }
        // GBN
        else
        {
            if (packet.rtp.seq_num == sqn)
            {
                LOG_DEBUG("Receiver: recv packet %d\n", shift + 1);
                memcpy((void *)buf + (shift++) * PAYLOAD_MAX,
                       (void *)packet.payload, packet.rtp.length);
                fsize += packet.rtp.length;
                sqn = seqnum_add(sqn, 1);
            }
            send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_ACK, 0, NULL);
        }
    }

    LOG_DEBUG("Receiver: stop receiving data\n");
    write_file(filename, (void *)buf, fsize);
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        LOG_FATAL("Usage: ./receiver [listen port] [file path] [window size] "
                  "[mode]\n");
    }

    int port, wsize, mode;
    char *file_path;

    port = atoi(argv[1]);
    file_path = argv[2];
    wsize = atoi(argv[3]);
    mode = atoi(argv[4]);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        LOG_FATAL("create socket error\n");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, (socklen_t)sizeof(addr)) < 0)
    {
        LOG_FATAL("bind socket error\n");
    }

    addrlen = (socklen_t)sizeof(addr);

    establish_connection();

    transfer_data(file_path, wsize, mode);

    break_connection();

    if (close(sock) < 0)
    {
        LOG_FATAL("close socket error\n");
    }

    LOG_DEBUG("Receiver: exiting...\n");
    return 0;
}
