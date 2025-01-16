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

    // send the first handshake
    sqn = get_random_seqnum();
    set_sock_recv_time(sock, 0, SEND_MS);
    LOG_DEBUG("Sender: send conn packet (times: %d)\n", ++tries);
    send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_SYN, 0, NULL);
    while (1)
    {
        if (tries > SEND_TR)
        {
            LOG_FATAL("Sender: send conn packet tries used up: %d\n", tries);
        }
        // recv the second handshake
        if (recv_packet(sock, (struct sockaddr *)&addr, &addrlen, &packet) == 0)
        {
            if (packet.rtp.seq_num == seqnum_add(sqn, 1) && packet.rtp.flags == (RTP_ACK | RTP_SYN))
            {
                LOG_DEBUG("Sender: send conn packet success\n");
                break;
            }
        }
        LOG_DEBUG("Sender: send conn packet (times: %d)\n", ++tries);
        send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_SYN, 0, NULL);
    }

    // send the third handshake
    set_sock_recv_time(sock, RECV_S, 0);
    sqn = seqnum_add(sqn, 1);
    send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_ACK, 0, NULL);
    scode = recv_packet(sock, (struct sockaddr *)&addr, &addrlen, &packet);
    if (!scode && packet.rtp.seq_num == sqn && packet.rtp.flags == (RTP_ACK | RTP_SYN))
    {
        send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_ACK, 0, NULL);
    }

    LOG_DEBUG("Sender: connection established\n");
}

void break_connection()
{
    rtp_packet_t packet;
    int tries = 0;

    // send the first handshake
    set_sock_recv_time(sock, 0, SEND_MS);
    LOG_DEBUG("Sender: send break packet (times: %d)\n", ++tries);
    send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_FIN, 0, NULL);
    while (1)
    {
        if (tries > SEND_TR)
        {
            LOG_FATAL("Sender: send break packet tries used up: %d\n", tries);
        }
        // recv the second handshake
        if (recv_packet(sock, (struct sockaddr *)&addr, &addrlen, &packet) == 0)
        {
            if (packet.rtp.seq_num == sqn && packet.rtp.flags == (RTP_ACK | RTP_FIN))
            {
                LOG_DEBUG("Sender: send break packet success\n");
                break;
            }
        }
        LOG_DEBUG("Sender: send break packet (times: %d)\n", ++tries);
        send_packet(sock, (struct sockaddr *)&addr, sqn, RTP_FIN, 0, NULL);
    }

    LOG_DEBUG("Sender: connection stopped\n");
}

void transfer_data(char *filename, int wsize, int mode)
{
    rtp_packet_t packet;
    char buf[MAXBUF];
    int fsize = read_file(filename, buf, MAXBUF);
    int dsize, i, mov, scode, shift = 0, tsize = fsize % PAYLOAD_MAX;
    int npacket = (fsize + PAYLOAD_MAX - 1) / PAYLOAD_MAX;
    int window[wsize];
    uint32_t wbase = sqn, seqnum;

    memset(window, 0, sizeof(window));

    set_sock_recv_time(sock, 0, SEND_MS);
    while (shift < npacket)
    {
        if (wbase == sqn)
        {
            for (i = 0; i < wsize && i + shift < npacket; ++i)
            {
                LOG_DEBUG("Sender: send packet [%d/%d]\n", shift + i + 1, npacket);
                dsize = shift + i == npacket - 1 ? tsize : PAYLOAD_MAX;
                send_packet(sock, (struct sockaddr *)&addr, sqn,
                            0, dsize, (void *)buf + (shift + i) * PAYLOAD_MAX);
                sqn = seqnum_add(sqn, 1);
            }
        }
        scode = recv_packet(sock, (struct sockaddr *)&addr, &addrlen, &packet);
        // wait for packet timeout
        if (scode == -2)
        {
            for (i = 0; seqnum_add(wbase, i) < sqn; ++i)
            {
                if (mode && window[i])
                {
                    continue;
                }
                LOG_DEBUG("Sender: resend packet [%d/%d]\n", shift + i + 1, npacket);
                dsize = shift + i == npacket - 1 ? tsize : PAYLOAD_MAX;
                send_packet(sock, (struct sockaddr *)&addr, seqnum_add(wbase, i),
                            0, dsize, (void *)buf + (shift + i) * PAYLOAD_MAX);
            }
            continue;
        }
        // invalid packet
        if (scode || packet.rtp.flags != RTP_ACK)
        {
            continue;
        }
        seqnum = seqnum_add(packet.rtp.seq_num, -(mode ^ 1));
        // dropout
        if (!seqnum_is_in(seqnum, wbase, sqn))
        {
            continue;
        }
        // SR
        if (mode)
        {
            mov = update_window(window, wsize, sqn_dis(wbase, seqnum));
        }
        // GBN
        else
        {
            mov = sqn_dis(wbase, seqnum) + 1;
        }
        shift += mov;
        wbase = seqnum_add(wbase, mov);
    }

    LOG_DEBUG("Sender: stop sending data\n");
}

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        LOG_FATAL("Usage: ./sender [receiver ip] [receiver port] [file path] "
                  "[window size] [mode]\n");
    }

    char *ip, *file_path;
    int port, wsize, mode;

    ip = argv[1];
    port = atoi(argv[2]);
    file_path = argv[3];
    wsize = atoi(argv[4]);
    mode = atoi(argv[5]);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        LOG_FATAL("Sender: create socket error\n");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    addrlen = (socklen_t)sizeof(addr);

    establish_connection();

    transfer_data(file_path, wsize, mode);

    break_connection();

    if (close(sock) < 0)
    {
        LOG_FATAL("Sender: close socket error\n");
    }

    LOG_DEBUG("Sender: exiting...\n");
    return 0;
}
