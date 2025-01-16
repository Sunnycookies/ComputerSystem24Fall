#include <defs.h>
#include <ftp_utils.hpp>

#define type2ind(m_type) ((m_type - OPEN_REQUEST) / 2)
#define fd2ind(fd) ((fd - 2))

namespace fs = std::filesystem;

int epfd;
struct epoll_event evt;
fs::path dft_path;
fs::path cwds[MAXCONN];

int do_open(int fd, char *args = nullptr)
{
    if (send_post(fd, OPEN_REPLY, nullptr, 0, 1) < 0)
    {
        return serror("send open reply error");
    }
    cwds[fd2ind(fd)] = dft_path;
    return 0;
}

int do_quit(int fd, char *args)
{
    if (send_post(fd, QUIT_REPLY) < 0)
    {
        return serror("send quit reply error");
    }
    evt.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &evt) < 0)
    {
        serror("delete epoll control error");
    }
    if (close(fd) < 0)
    {
        return serror("close socket error");
    }
    cwds[fd2ind(fd)] = fs::path("NULL");
    return 0;
}

int do_ls(int fd, char *args = nullptr)
{
    char buf[MAXBUF];
    FILE *fp;

    if ((fp = popen("ls", "r")) == nullptr)
    {
        return serror("popen ls error");
    }

    int nread = fread((void *)buf, 1, MAXBUF, fp);
    pclose(fp);
    buf[nread++] = '\0';

    if (send_post(fd, LIST_REPLY, buf, nread) < 0)
    {
        return serror("send ls reply error");
    }
    return 0;
}

int do_cd(int fd, char *args)
{
    status s = fs::exists(args);

    if (send_post(fd, CD_REPLY, nullptr, 0, s) < 0)
    {
        return serror("send cd reply error");
    }

    if (s == 1)
    {
        cwds[fd2ind(fd)] = fs::canonical(fs::absolute(args));
    }

    return 0;
}

int do_get(int fd, char *args)
{
    status s = fs::exists(args);

    if (send_post(fd, GET_REPLY, nullptr, 0, s) < 0)
    {
        return serror("send get reply error");
    }

    if (s == 0)
    {
        return 0;
    }

    char buf[MAXBUF];
    int size;
    if ((size = read_file(args, buf, MAXBUF)) < 0)
    {
        return -1;
    }

    if (send_post(fd, FILE_DATA, buf, size) < 0)
    {
        return serror("send file data error");
    }

    return 0;
}

int do_put(int fd, char *args)
{
    if (send_post(fd, PUT_REPLY) < 0)
    {
        return serror("send put reply error");
    }
    
    char buf[MAXBUF];
    int size;
    type m_type;
    if ((size = recv_post(fd, buf, &m_type)) < 0)
    {
        return serror("recv file data error");
    }
    if (m_type != FILE_DATA)
    {
        return serror("bad file data");
    }

    if (write_file(args, buf, size) < 0)
    {
        return -1;
    }
    return 0;
}

int do_sha(int fd, char *args)
{
    status s = fs::exists(args);

    if (send_post(fd, SHA_REPLY, nullptr, 0, s) < 0)
    {
        return serror("send sha256 reply error");
    }

    if (s == 0)
    {
        return 0;
    }

    fs::path p = fs::canonical(fs::absolute(args));
    char buf[MAXBUF];
    sprintf(buf, "sha256sum %s", p.c_str());
    FILE *fp;
    if ((fp = popen(buf, "r")) == nullptr)
    {
        return serror("popen sha256sum error");
    }
    int nread = fread((void *)buf, 1, MAXBUF, fp);
    pclose(fp);
    buf[nread++] = '\0';

    if (send_post(fd, FILE_DATA, buf, nread) < 0)
    {
        return serror("send file data error");
    }

    return 0;
}

int (*funcs[])(int, char *) = {
    do_open,
    do_ls,
    do_cd,
    do_get,
    do_put,
    do_sha,
    do_quit,
};

int main(int argc, char **argv)
{
    // check if command line is valid
    if (argc != 3)
    {
        printf("usage: ftp_server <IPaddr> <Port>\n");
        return 0;
    }

    // initialize listenfd
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_port = htons(port);
    servaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) != 1)
    {
        close(listenfd);
        return serror("inet_pton error");
    }
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        close(listenfd);
        return serror("bind error");
    }
    if (listen(listenfd, LISTENQ) < 0)
    {
        close(listenfd);
        return serror("listen error");
    }

    // initialize path settings
    dft_path = fs::current_path();
    for (int i = 0; i < MAXCONN; ++i)
    {
        cwds[i] = fs::path("NULL");
    }

    // initialize epoll
    epfd = epoll_create(1);
    evt.events = EPOLLIN;
    evt.data.fd = listenfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &evt))
    {
        serror("add listenfd epoll control error");
    }

    // listen
    int connfd;
    struct sockaddr_storage cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    struct epoll_event events[MAXEPOLL];
    type m_type;
    char buf[MAXBUF];
    while (true)
    {
        int nevents = epoll_wait(epfd, events, MAXEPOLL, -1);
        for (int i = 0; i < nevents; ++i)
        {
            connfd = events[i].data.fd;

            // recv new connection
            if (connfd == listenfd)
            {
                if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
                {
                    serror("accept error");
                    continue;
                }
                evt.data.fd = connfd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &evt))
                {
                    serror("add connfd epoll control error");
                }
                continue;
            }

            // recv request
            memset(buf, 0, sizeof(buf));
            if (recv_post(connfd, buf, &m_type) < 0)
            {
                serror("recv request error");
                continue;
            }

            // change working directory
            if (fs::exists(cwds[fd2ind(connfd)]))
            {
                if (chdir(cwds[fd2ind(connfd)].c_str()) < 0)
                {
                    serror("change to client directory error");
                    continue;
                }
            }
            funcs[type2ind(m_type)](connfd, buf);
            if (chdir(dft_path.c_str()) < 0)
            {
                serror("change to default directory error");
            }
        }
    }
    return 0;
}