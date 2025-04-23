#pragma once
#include "common.h"

// Interrupt causes for S-mode (when bit-63 of scause is 1)
// at top of kernel.c or kernel.h
#define INTR_SSOFT 1       // supervisor software
#define INTR_STIMER 5      // supervisor timer
#define INTR_SEXT   9      // supervisor external
#define INTR_MEXT  11      // machine external (delegated)
#define SIP_SSIP   (1UL << 1)

#define INTR_USOFT  1    // supervisor software
#define INTR_UTIMER 5    // supervisor timer
#define INTR_UEXT   9    // supervisor external

#define INTR_STIMER 5    // supervisor timer
#define INTR_SEXT   9    // supervisor external
#define INTR_MEXT  11    // machine external, delegated

// Bits in the sie CSR
#define SIE_STIE    (1UL << 5)  // supervisor-timer interrupt enable
#define SIE_SEIE    (1UL << 9)  // supervisor-external interrupt enable

// Bit in the sstatus CSR
#define SSTATUS_SIE (1UL << 1)  // global S-mode interrupt enable


#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0)


#define SECTOR_SIZE       512
#define VIRTQ_ENTRY_NUM   16
#define VIRTIO_DEVICE_BLK 2
#define VIRTIO_BLK_PADDR  0x10001000
#define VIRTIO_REG_MAGIC         0x00
#define VIRTIO_REG_VERSION       0x04
#define VIRTIO_REG_DEVICE_ID     0x08
#define VIRTIO_REG_QUEUE_SEL     0x30
#define VIRTIO_REG_QUEUE_NUM_MAX 0x34
#define VIRTIO_REG_QUEUE_NUM     0x38
#define VIRTIO_REG_QUEUE_ALIGN   0x3c
#define VIRTIO_REG_QUEUE_PFN     0x40
#define VIRTIO_REG_QUEUE_READY   0x44
#define VIRTIO_REG_QUEUE_NOTIFY  0x50
#define VIRTIO_REG_DEVICE_STATUS 0x70
#define VIRTIO_REG_DEVICE_CONFIG 0x100
#define VIRTIO_STATUS_ACK       1
#define VIRTIO_STATUS_DRIVER    2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEAT_OK   8
#define VIRTQ_DESC_F_NEXT          1
#define VIRTQ_DESC_F_WRITE         2
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
#define VIRTIO_BLK_T_IN  0
#define VIRTIO_BLK_T_OUT 1

#define FILES_MAX      2
#define DISK_MAX_SIZE  align_up(sizeof(struct file) * FILES_MAX, SECTOR_SIZE)

#define SSTATUS_SUM  (1 << 18)

#define PROCS_MAX 8       // Maximum number of processes

#define PROC_UNUSED   0   // Unused process control structure
#define PROC_RUNNABLE 1   // Runnable process
#define PROC_EXITED   2

#define MONITOR_PID 1

#define IPC_BUF_SIZE  256   // adjust as you like

struct SyscallEvent {
    int pid;
    int num;
    uint64_t args[4];
};

struct process {
    int pid;             // Process ID
    int state;           // Process state: PROC_UNUSED or PROC_RUNNABLE
    vaddr_t sp;          // Stack pointer
    uint64_t *page_table;
    uint8_t stack[8192]; // Kernel stack

    // ── NEW for IPC ──────────────────────────────────────────
    uint8_t  ipc_buf[IPC_BUF_SIZE];
    int      ipc_head;      // next read index
    int      ipc_tail;      // next write index
    int      ipc_count;     // number of bytes queued
    bool     ipc_blocked;   // true if task is blocked in SYS_IPC_RECV
};

struct sbiret {
    long error;
    long value;
};

// SATP mode encoding for Sv39: mode = 8 in bits [63:60]
#define SATP_SV39 (8UL << 60)
// Page table entry flag bits (still the same in RISCV64)
#define PAGE_V    (1UL << 0)   // Valid
#define PAGE_R    (1UL << 1)   // Readable
#define PAGE_W    (1UL << 2)   // Writable
#define PAGE_X    (1UL << 3)   // Executable
#define PAGE_U    (1UL << 4)   // User

// The base virtual address of an application image. This needs to match the
// starting address defined in `user.ld`.
#define USER_BASE 0x1000000

#define SSTATUS_SPIE (1ul << 5)
#define SCAUSE_ECALL 8


struct trap_frame {
    uint64_t ra;
    uint64_t gp;
    uint64_t tp;
    uint64_t t0;
    uint64_t t1;
    uint64_t t2;
    uint64_t t3;
    uint64_t t4;
    uint64_t t5;
    uint64_t t6;
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t a4;
    uint64_t a5;
    uint64_t a6;
    uint64_t a7;
    uint64_t s0;
    uint64_t s1;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
    uint64_t sp;
} __attribute__((packed));


// Virtqueue Descriptor area entry.
struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

// Virtqueue Available Ring.
struct virtq_avail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// Virtqueue Used Ring entry.
struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

// Virtqueue Used Ring.
struct virtq_used {
    uint16_t flags;
    uint16_t index;
    struct virtq_used_elem ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// Virtqueue.
// struct virtio_virtq {
//     struct virtq_desc descs[VIRTQ_ENTRY_NUM];
//     struct virtq_avail avail;
//     struct virtq_used used __attribute__((aligned(PAGE_SIZE)));
//     int queue_index;
//     volatile uint16_t *used_index;
//     uint16_t last_used_index;
// } __attribute__((packed));

struct virtio_virtq {
    struct virtq_desc descs[VIRTQ_ENTRY_NUM];
    struct virtq_avail avail;
    struct virtq_used used __attribute__((aligned(PAGE_SIZE)));
    int queue_index;
    volatile uint16_t *used_index;
    uint16_t last_used_index;
};


// Virtio-blk request.
struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
    uint8_t data[512];
    uint8_t status;
} __attribute__((packed));


struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
    char data[];      // Array pointing to the data area following the header
                      // (flexible array member)
} __attribute__((packed));

struct file {
    bool in_use;      // Indicates if this file entry is in use
    char name[100];   // File name
    char data[1024];  // File content
    size_t size;      // File size
};