#include "user.h"

extern char __stack_top[];

int syscall(int sysno, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    register uint64_t a0 __asm__("a0") = arg0;
    register uint64_t a1 __asm__("a1") = arg1;
    register uint64_t a2 __asm__("a2") = arg2;
    register uint64_t a3 __asm__("a3") = sysno;

    __asm__ __volatile__("ecall"
                         : "=r"(a0)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3)
                         : "memory");

    return a0;
}

void putchar(char ch) {
    syscall(SYS_PUTCHAR, ch, 0, 0);
}

int getchar(void) {
    return syscall(SYS_GETCHAR, 0, 0, 0);
}

int readfile(const char *filename, char *buf, int len) {
    return syscall(SYS_READFILE, (int) filename, (int) buf, len);
}

int writefile(const char *filename, const char *buf, int len) {
    return syscall(SYS_WRITEFILE, (int) filename, (int) buf, len);
}

int ipc_send(int dest_pid, const void *msg, int len) {
    return syscall(SYS_IPC_SEND, dest_pid, (uint64_t)msg, len);
}

int monitor_kill(int pid) {
    return syscall(SYS_MONITOR_KILL, pid, 0, 0);
}
  
int ipc_recv(void *buf, int maxlen) {
    return syscall(SYS_IPC_RECV, (uint64_t)buf, maxlen, 0);
}

int spawn(const void *image, int size) {
    return syscall(SYS_SPAWN, (uint64_t)image, size, 0);
}  

__attribute__((noreturn)) void exit(void) {
    syscall(SYS_EXIT, 0, 0, 0);
    for (;;);
}



int _connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // pretend success
    syscall(SYS_CONNECT, sockfd, (uint64_t)addr, addrlen);
    return 0;
}

int _dup2(int oldfd, int newfd) {
    // no real FD dup; pretend success
    syscall(SYS_DUP2, oldfd, newfd, 0);
    return newfd;
}

int _execve(const char *pathname, char *const argv[], char *const envp[]) {
    (void)pathname;
    (void)argv;
    (void)envp;
    // simulate spawning a shell, but just exit
    // exit();
    syscall(SYS_EXECVE, (uint64_t)pathname, (uint64_t)argv, (uint64_t)envp);
    return -1; // never reached
}

// __attribute__((section(".text.start")))
// __attribute__((naked))
// void start(void) {
//     __asm__ __volatile__(
//         "mv sp, %[stack_top]\n"
//         "call main\n"
//         "call exit\n" ::[stack_top] "r"(__stack_top));
// }

__attribute__((section(".text.start")))
__attribute__((naked))
void start(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "call main\n"
        "call exit\n"
        :: [stack_top] "r"(__stack_top)
    );
}
