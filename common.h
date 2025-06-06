#pragma once 

typedef int bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
// typedef unsigned long long uint64_t;
typedef unsigned long uint64_t;
typedef uint64_t size_t;
typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;


typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;
typedef uint32_t socklen_t;

#define true  1
#define false 0
#define NULL  ((void *) 0)
#define align_up(value, align)   __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define offsetof(type, member)   __builtin_offsetof(type, member)
#define va_list  __builtin_va_list
#define va_start __builtin_va_start
#define va_end   __builtin_va_end
#define va_arg   __builtin_va_arg

#define PAGE_SIZE 4096
#define SYS_PUTCHAR 1
#define SYS_GETCHAR 2
#define SYS_EXIT    3
#define SYS_READFILE  4
#define SYS_WRITEFILE 5



#define SYS_IPC_SEND    6
#define SYS_IPC_RECV    7

#define SYS_MONITOR_KILL  8

#define SYS_SPAWN   9

#define SYS_CONNECT  10
#define SYS_DUP2     11
#define SYS_EXECVE   12
#define SYS_SOCKET   13

void *memset(void *buf, char c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
void printf(const char *fmt, ...);