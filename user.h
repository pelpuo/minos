#pragma once
#include "common.h"

struct sysret {
    int a0;
    int a1;
    int a2;
};

void putchar(char ch);
int getchar(void);
int readfile(const char *filename, char *buf, int len);
int writefile(const char *filename, const char *buf, int len);
int ipc_send(int dest_pid, const void *msg, int len);
int ipc_recv(void *buf, int maxlen);
int spawn(const void *image, int size);

int monitor_kill(int pid);

__attribute__((noreturn)) void exit(void);
// user.h (append)
#define SYS_WRITEFILE 5
struct SyscallEvent { int pid; int num; uint64_t args[4]; };
