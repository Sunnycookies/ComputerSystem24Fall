#ifndef _DEFS_H_
#define _DEFS_H_

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <filesystem>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAXLINE 2048
#define MAXBUF  1 << 21
#define MAXEPOLL 64
#define MAXCONN 64
#define LISTENQ 64

#define OPEN_REQUEST    0xA1
#define OPEN_REPLY      0xA2
#define LIST_REQUEST    0xA3
#define LIST_REPLY      0xA4
#define CD_REQUEST      0xA5
#define CD_REPLY        0xA6
#define GET_REQUEST     0xA7
#define GET_REPLY       0xA8
#define PUT_REQUEST     0xA9
#define PUT_REPLY       0xAA
#define SHA_REQUEST     0xAB
#define SHA_REPLY       0xAC
#define QUIT_REQUEST    0xAD
#define QUIT_REPLY      0xAE
#define FILE_DATA       0xFF

#endif