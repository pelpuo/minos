#pragma once
#include "common.h"

struct sysret {
    int a0;
    int a1;
    int a2;
};

struct in_addr { in_addr_t s_addr; };
struct sockaddr {
    uint16_t sa_family;
    char     sa_data[14];
};

struct sockaddr_in {
    uint16_t       sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};


void putchar(char ch);
int getchar(void);
int readfile(const char *filename, char *buf, int len);
int writefile(const char *filename, const char *buf, int len);
int ipc_send(int dest_pid, const void *msg, int len);
int ipc_recv(void *buf, int maxlen);
int spawn(const void *image, int size);

int _connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int _dup2(int oldfd, int newfd);
int _execve(const char *pathname, char *const argv[], char *const envp[]);

int monitor_kill(int pid);

__attribute__((noreturn)) void exit(void);
// user.h (append)
#define SYS_WRITEFILE 5
struct SyscallEvent { int pid; int num; uint64_t args[4]; };
