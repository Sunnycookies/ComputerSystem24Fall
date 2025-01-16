#include <defs.h>
#include <ftp_utils.hpp>

static const char *cmdnames[] = {
    "open",
    "ls",
    "cd",
    "get",
    "put",
    "sha256",
    "quit",
};
const int cmdnum = sizeof(cmdnames) / sizeof(char *);

static const char *default_prompt = "Client(None)>";
char prompt[MAXLINE] = "Client(None)>";

bool connected = false;
bool running = true;
int sock;
type m_type;
status m_status;

int do_open(char *args)
{
    // get ip and port from args
    char *ip = args;
    char *p = strstr(args, " ");
    *p = '\0';
    int port = atoi(p + 1);

    // create socket and connect to server
    sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &addr.sin_addr) < 0)
    {
        close(sock);
        return serror("inet_pton error");
    }
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sock);
        return serror("connect error");
    }

    // send post
    if (send_post(sock, OPEN_REQUEST) < 0)
    {
        close(sock);
        return serror("send open request error");
    }

    // recv post
    char buf[MAXBUF];
    if (recv_post(sock, buf, &m_type, &m_status) < 0)
    {
        close(sock);
        return serror("recv open reply error");
    }
    if (m_type != OPEN_REPLY || m_status != 1)
    {
        close(sock);
        return serror("bad open reply");
    }

    // change states
    connected = true;
    sprintf(prompt, "Client(%s:%d)>", ip, port);
    printf("connection established\n");
    return 0;
}

int do_quit(char *args = nullptr)
{
    // close client if not connected
    if (connected == false)
    {
        running = false;
        return 0;
    }

    // send post
    if (send_post(sock, QUIT_REQUEST) < 0)
    {
        return serror("send quit request error");
    }

    // recv post
    char buf[MAXBUF];
    if (recv_post(sock, buf, &m_type, &m_status) < 0)
    {
        return serror("recv quit reply error");
    }
    if (m_type != QUIT_REPLY)
    {
        return serror("bad quit reply");
    }

    // release resources and change states
    if (close(sock) < 0)
    {
        return serror("close socket error");
    }
    connected = false;
    strcpy(prompt, default_prompt);
    printf("connection close ok\n");
    return 0;
}

int do_ls(char *args = nullptr)
{
    // checkout if connected
    if (connected == false)
    {
        return serror("ls not supported offline");
    }

    // send post
    if (send_post(sock, LIST_REQUEST) < 0)
    {
        return serror("send ls request error");
    }

    // recv post
    char buf[MAXBUF];
    if (recv_post(sock, buf, &m_type, &m_status) < 0)
    {
        return serror("recv ls reply error");
    }
    if (m_type != LIST_REPLY)
    {
        return serror("bad ls reply");
    }

    // show data
    printf("%s", buf);
    return 0;
}

int do_cd(char *args)
{
    // checkout if connected
    if (connected == false)
    {
        return serror("cd not supported offline");
    }

    //  send post
    if (send_post(sock, CD_REQUEST, args, strlen(args) + 1) < 0)
    {
        return serror("send cd request error");
    }

    // recv post
    char buf[MAXBUF];
    if (recv_post(sock, buf, &m_type, &m_status) < 0)
    {
        return serror("recv cd reply error");
    }
    if (m_type != CD_REPLY || m_status != 1)
    {
        return serror("bad cd reply");
    }

    return 0;
}

int do_get(char *args)
{
    // check if connected
    if (connected == false)
    {
        return serror("get not supported offline");
    }

    // send post
    if (send_post(sock, GET_REQUEST, args, strlen(args) + 1) < 0)
    {
        return serror("send get request error");
    }

    // recv post
    char buf[MAXBUF];
    int size;
    if ((size = recv_post(sock, buf, &m_type, &m_status)) < 0)
    {
        return serror("recv get reply error");
    }
    if (m_type != GET_REPLY || m_status != 1)
    {
        return serror("bad get reply");
    }

    // recv file
    if ((size = recv_post(sock, buf, &m_type)) < 0)
    {
        return serror("recv file data error");
    }
    if (m_type != FILE_DATA)
    {
        return serror("bad file data");
    }

    // save file data
    if (write_file(args, buf, size) < 0)
    {
        return -1;
    }

    return 0;
}

int do_put(char *args)
{
    // check if connected
    if (connected == false)
    {
        return serror("put not supported offline");
    }

    // send post
    if (send_post(sock, PUT_REQUEST, args, strlen(args) + 1) < 0)
    {
        return serror("send put request error");
    }

    // recv post
    char buf[MAXBUF];
    if (recv_post(sock, buf, &m_type, &m_status) < 0)
    {
        return serror("recv put reply error");
    }
    if (m_type != PUT_REPLY)
    {
        return serror("bad put reply");
    }

    // send data
    int size;
    if ((size = read_file(args, buf, MAXBUF)) < 0)
    {
        return -1;
    }
    if (send_post(sock, FILE_DATA, buf, size) < 0)
    {
        return serror("send data file error");
    }

    return 0;
}

int do_sha(char *args)
{
    // check if connected
    if (connected == false)
    {
        return serror("sha256 not supported offline");
    }

    // send post
    if (send_post(sock, SHA_REQUEST, args, strlen(args) + 1) < 0)
    {
        return serror("send sha256 request error");
    }

    // recv post
    char buf[MAXBUF];
    if (recv_post(sock, buf, &m_type, &m_status) < 0)
    {
        return serror("recv sha256 reply error");
    }
    if (m_type != SHA_REPLY || m_status != 1)
    {
        return serror("bad sha256 reply");
    }

    // recv file
    int size;
    if ((size = recv_post(sock, buf, &m_type, &m_status)) < 0)
    {
        return serror("recv file data error");
    }
    if (m_type != FILE_DATA)
    {
        return serror("bad file data");
    }

    // show data
    printf("%s", buf);
    return 0;
}

int (*cmdfuncs[])(char *) = {
    do_open,
    do_ls,
    do_cd,
    do_get,
    do_put,
    do_sha,
    do_quit,
};

int parseline(char *cmdline)
{
    char *p = strstr(cmdline, " ");
    for (int i = 0; i < cmdnum; ++i)
    {
        if (strncasecmp(cmdline, cmdnames[i], strlen(cmdnames[i])) == 0)
        {
            return cmdfuncs[i](p + 1);
        }
    }
    return 1;
}

int main()
{
    char cmdline[MAXLINE];
    while(running)
    {
        fflush(stdout);
        printf("%s", prompt);
        fflush(stdout);
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
        {
            serror("get command line error");
        }
        if (feof(stdin))
        {
            printf("\n");
            fflush(stdout);
            exit(0);
        }
        cmdline[strlen(cmdline) - 1] = '\0';
        if (parseline(cmdline) > 0)
        {
            serror("command not supported");
        }
    }
    return 0;
}