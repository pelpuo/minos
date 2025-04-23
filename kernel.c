#include "kernel.h"
#include "common.h"
#include <stdint.h>

#include "monitor_bin.h"

#define READ_CSR(reg)                                                          \
  ({                                                                           \
    unsigned long __tmp;                                                       \
    __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));                      \
    __tmp;                                                                     \
  })

#define WRITE_CSR(reg, value)                                                  \
  do {                                                                         \
    uint64_t __tmp = (value);                                                  \
    __asm__ __volatile__("csrw " #reg ", %0" ::"r"(__tmp));                    \
  } while (0)

extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];
extern char _binary_shell_bin_start[], _binary_shell_bin_size[], _binary_shell_bin_end[];
// extern char _binary_monitor_bin_start[], _binary_monitor_bin_size[];
extern char __kernel_base[];

struct virtio_virtq *blk_request_vq;
struct virtio_blk_req *blk_req;
paddr_t blk_req_paddr;
unsigned blk_capacity;

static int monitor_pid = -1;

struct file files[FILES_MAX];
uint8_t disk[DISK_MAX_SIZE];

struct process procs[PROCS_MAX]; // All process control structures.
struct process *current_proc; // Currently running process
struct process *idle_proc;    // Idle process

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = fid;
  register long a7 __asm__("a7") = eid;

  __asm__ __volatile__("ecall"
                       : "=r"(a0), "=r"(a1)
                       : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                         "r"(a6), "r"(a7)
                       : "memory");
  return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
  sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}

long getchar(void) {
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
  return ret.error;
}

__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void) {
  __asm__ __volatile__(
                        "csrrw sp, sscratch, sp\n"
                        // "csrw sscratch, sp\n"
                        "addi sp, sp, -8 * 31\n"
                        "sd ra,  8 * 0(sp)\n"
                        "sd gp,  8 * 1(sp)\n"
                        "sd tp,  8 * 2(sp)\n"
                        "sd t0,  8 * 3(sp)\n"
                        "sd t1,  8 * 4(sp)\n"
                        "sd t2,  8 * 5(sp)\n"
                        "sd t3,  8 * 6(sp)\n"
                        "sd t4,  8 * 7(sp)\n"
                        "sd t5,  8 * 8(sp)\n"
                        "sd t6,  8 * 9(sp)\n"
                        "sd a0,  8 * 10(sp)\n"
                        "sd a1,  8 * 11(sp)\n"
                        "sd a2,  8 * 12(sp)\n"
                        "sd a3,  8 * 13(sp)\n"
                        "sd a4,  8 * 14(sp)\n"
                        "sd a5,  8 * 15(sp)\n"
                        "sd a6,  8 * 16(sp)\n"
                        "sd a7,  8 * 17(sp)\n"
                        "sd s0,  8 * 18(sp)\n"
                        "sd s1,  8 * 19(sp)\n"
                        "sd s2,  8 * 20(sp)\n"
                        "sd s3,  8 * 21(sp)\n"
                        "sd s4,  8 * 22(sp)\n"
                        "sd s5,  8 * 23(sp)\n"
                        "sd s6,  8 * 24(sp)\n"
                        "sd s7,  8 * 25(sp)\n"
                        "sd s8,  8 * 26(sp)\n"
                        "sd s9,  8 * 27(sp)\n"
                        "sd s10, 8 * 28(sp)\n"
                        "sd s11, 8 * 29(sp)\n"
                        // Retrieve and save the sp at the time of exception.
                        "csrr a0, sscratch\n"
                        "sd a0, 8 * 30(sp)\n"

                        // Reset the kernel stack.
                        "addi a0, sp, 8 * 31\n"
                        "csrw sscratch, a0\n"

                        "mv a0, sp\n"
                        "call handle_trap\n"

                        "ld ra,  8 * 0(sp)\n"
                        "ld gp,  8 * 1(sp)\n"
                        "ld tp,  8 * 2(sp)\n"
                        "ld t0,  8 * 3(sp)\n"
                        "ld t1,  8 * 4(sp)\n"
                        "ld t2,  8 * 5(sp)\n"
                        "ld t3,  8 * 6(sp)\n"
                        "ld t4,  8 * 7(sp)\n"
                        "ld t5,  8 * 8(sp)\n"
                        "ld t6,  8 * 9(sp)\n"
                        "ld a0,  8 * 10(sp)\n"
                        "ld a1,  8 * 11(sp)\n"
                        "ld a2,  8 * 12(sp)\n"
                        "ld a3,  8 * 13(sp)\n"
                        "ld a4,  8 * 14(sp)\n"
                        "ld a5,  8 * 15(sp)\n"
                        "ld a6,  8 * 16(sp)\n"
                        "ld a7,  8 * 17(sp)\n"
                        "ld s0,  8 * 18(sp)\n"
                        "ld s1,  8 * 19(sp)\n"
                        "ld s2,  8 * 20(sp)\n"
                        "ld s3,  8 * 21(sp)\n"
                        "ld s4,  8 * 22(sp)\n"
                        "ld s5,  8 * 23(sp)\n"
                        "ld s6,  8 * 24(sp)\n"
                        "ld s7,  8 * 25(sp)\n"
                        "ld s8,  8 * 26(sp)\n"
                        "ld s9,  8 * 27(sp)\n"
                        "ld s10, 8 * 28(sp)\n"
                        "ld s11, 8 * 29(sp)\n"
                        "ld sp,  8 * 30(sp)\n"
                       "sret\n");
}

void myfunc(){
  PANIC("MY YIELD");
}

__attribute__((naked)) void switch_context(uint64_t *prev_sp,
                                           uint64_t *next_sp) {
  __asm__ __volatile__(
      // Save callee-saved registers onto the current process's stack.
      "addi sp, sp, -13 * 8\n" // Allocate stack space for 13 4-byte registers
      "sd ra,  0  * 8(sp)\n"   // Save callee-saved registers only
      "sd s0,  1  * 8(sp)\n"
      "sd s1,  2  * 8(sp)\n"
      "sd s2,  3  * 8(sp)\n"
      "sd s3,  4  * 8(sp)\n"
      "sd s4,  5  * 8(sp)\n"
      "sd s5,  6  * 8(sp)\n"
      "sd s6,  7  * 8(sp)\n"
      "sd s7,  8  * 8(sp)\n"
      "sd s8,  9  * 8(sp)\n"
      "sd s9,  10 * 8(sp)\n"
      "sd s10, 11 * 8(sp)\n"
      "sd s11, 12 * 8(sp)\n"

      // Switch the stack pointer.
      "sd sp, (a0)\n" // *prev_sp = sp;
      "ld sp, (a1)\n" // Switch stack pointer (sp) here
      
      // Restore callee-saved registers from the next process's stack.
      "ld ra,  0  * 8(sp)\n" // Restore callee-saved registers only
      "ld s0,  1  * 8(sp)\n"
      "ld s1,  2  * 8(sp)\n"
      "ld s2,  3  * 8(sp)\n"
      "ld s3,  4  * 8(sp)\n"
      "ld s4,  5  * 8(sp)\n"
      "ld s5,  6  * 8(sp)\n"
      "ld s6,  7  * 8(sp)\n"
      "ld s7,  8  * 8(sp)\n"
      "ld s8,  9  * 8(sp)\n"
      "ld s9,  10 * 8(sp)\n"
      "ld s10, 11 * 8(sp)\n"
      "ld s11, 12 * 8(sp)\n"
      "addi sp, sp, 13 * 8\n" // We've popped 13 4-byte registers from the stack
      "ret\n");
}

paddr_t alloc_pages(uint64_t n) {
    static paddr_t next_paddr = (paddr_t)__free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;
  
    if (next_paddr > (paddr_t)__free_ram_end)
      PANIC("out of memory");
  
    memset((void *)paddr, 0, n * PAGE_SIZE);
    return paddr;
  }

// void map_page(uint64_t *table1, uint64_t vaddr, paddr_t paddr, uint64_t flags) {
//     if (!is_aligned(vaddr, PAGE_SIZE))
//         PANIC("unaligned vaddr %x", vaddr);

//     if (!is_aligned(paddr, PAGE_SIZE))
//         PANIC("unaligned paddr %x", paddr);

//     uint64_t vpn1 = (vaddr >> 22) & 0x3ff;
//     if ((table1[vpn1] & PAGE_V) == 0) {
//         // Create the non-existent 2nd level page table.
//         uint64_t pt_paddr = alloc_pages(1);
//         table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
//     }

//     // Set the 2nd level page table entry to map the physical page.
//     uint64_t vpn0 = (vaddr >> 12) & 0x3ff;
//     uint64_t *table0 = (uint64_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
//     table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
// }

void map_page(uint64_t *root_table, uint64_t vaddr, paddr_t paddr, uint64_t flags) {
  uint64_t vpn2 = (vaddr >> 30) & 0x1FF;
  uint64_t vpn1 = (vaddr >> 21) & 0x1FF;
  uint64_t vpn0 = (vaddr >> 12) & 0x1FF;

  if (!is_aligned(vaddr, PAGE_SIZE))
      PANIC("unaligned vaddr %x", vaddr);

  if (!is_aligned(paddr, PAGE_SIZE))
      PANIC("unaligned paddr %x", paddr);

  // Level 2
  if ((root_table[vpn2] & PAGE_V) == 0) {
      paddr_t pt1 = alloc_pages(1);
      root_table[vpn2] = ((pt1 >> 12) << 10) | PAGE_V;
  }
  uint64_t *table1 = (uint64_t *) ((root_table[vpn2] >> 10) << 12);

  // Level 1
  if ((table1[vpn1] & PAGE_V) == 0) {
      paddr_t pt0 = alloc_pages(1);
      table1[vpn1] = ((pt0 >> 12) << 10) | PAGE_V;
  }
  uint64_t *table0 = (uint64_t *) ((table1[vpn1] >> 10) << 12);

  // Level 0 (leaf)
  table0[vpn0] = ((paddr >> 12) << 10) | flags | PAGE_V;
}


uint32_t virtio_reg_read32(unsigned offset) {
  return *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset));
}

uint64_t virtio_reg_read64(unsigned offset) {
  return *((volatile uint64_t *) (VIRTIO_BLK_PADDR + offset));
}

void virtio_reg_write32(unsigned offset, uint32_t value) {
  *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset)) = value;
}

void virtio_reg_fetch_and_or32(unsigned offset, uint32_t value) {
  virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}


// ↓ __attribute__((naked)) is very important!
__attribute__((naked)) void user_entry(void) {
  __asm__ __volatile__(
      "csrw sepc, %[sepc]        \n"
      "csrw sstatus, %[sstatus]  \n"
      // "li a0, 'U'\n"
      // "call putchar\n"
      "csrw sscratch, sp\n"
      "sret                      \n"
      :
      : [sepc] "r" (USER_BASE),
        // [sstatus] "r" (SSTATUS_SPIE)
        [sstatus] "r" (SSTATUS_SPIE | SSTATUS_SUM) // updated
  );
}

struct process *create_process(const void *image, size_t image_size) {
    // Find an unused process control structure.
    struct process *proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }
    
    if (!proc)
        PANIC("no free process slots");

    // Stack callee-saved registers. These register values will be restored in
    // the first context switch in switch_context.
    uint64_t *sp = (uint64_t *) &proc->stack[sizeof(proc->stack)];
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    // *--sp = (uint64_t) pc;       // ra
    *--sp = (uint64_t) user_entry;  // ra (changed!)

    uint64_t *page_table = (uint64_t *) alloc_pages(1);

    for (paddr_t paddr = (paddr_t) __kernel_base;
         paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);

    map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W); // new

    // Map user pages.
    for (uint64_t off = 0; off < image_size; off += PAGE_SIZE) {
      paddr_t page = alloc_pages(1);

      // Handle the case where the data to be copied is smaller than the
      // page size.
      size_t remaining = image_size - off;
      size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

      // Fill and map the page.
      memcpy((void *) page, image + off, copy_size);
      map_page(page_table, USER_BASE + off, page,
              PAGE_U | PAGE_R | PAGE_W | PAGE_X);
    }

    // Initialize fields.
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint64_t) sp;
    proc->page_table = page_table;
    return proc;
}

// void yield(void) {
//     // Search for a runnable process
//     int cur = current_proc->pid;
//     struct process *next = idle_proc;
//     for (int i = 0; i < PROCS_MAX; i++) {
//         struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
//         if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
//             next = proc;
//             break;
//         }
//     }

//     printf("yield: %d → %d\n", cur, next->pid);
//     // If there's no runnable process other than the current one, return and continue processing
//     if (next == current_proc)
//         return;

//     __asm__ __volatile__(
//         "sfence.vma\n"
//         "csrw satp, %[satp]\n"
//         "sfence.vma\n"
//         "csrw sscratch, %[sscratch]\n"
//         :
//         : [satp] "r" (SATP_SV39 | ((uint64_t) next->page_table / PAGE_SIZE)), 
//         [sscratch] "r" ((uint64_t) &next->stack[sizeof(next->stack)])
//     );

//     // Context switch
//     struct process *prev = current_proc;
//     current_proc = next;
//     switch_context(&prev->sp, &next->sp);
// }

void yield(void) {
  // 1) Figure out the current_proc’s slot index in the `procs[]` array
  int cur_idx = current_proc - procs;  
  int next_idx = -1;

  // 2) Round-robin search for the next PROC_RUNNABLE & pid>0
  for (int i = 1; i < PROCS_MAX; i++) {
      int idx = (cur_idx + i) % PROCS_MAX;
      if (procs[idx].state == PROC_RUNNABLE && procs[idx].pid > 0) {
          next_idx = idx;
          break;
      }
  }

  // 3) If we didn’t find any user process to run, fall back to idle
  // if (next_idx < 0) {
  //     next_idx = 0;  // we conventionally use slot 0 for idle
  // }

  if (next_idx < 0) {
      return;
  }

  // 4) If that’s still the current process, nothing to do
  struct process *next = &procs[next_idx];
  if (next == current_proc) {
      return;
  }

  // 5) Switch address space to the next process
  __asm__ __volatile__(
      "sfence.vma\n"
      "csrw satp, %[satp]\n"
      "sfence.vma\n"
      "csrw sscratch, %[sscratch]\n"
      :
      : [satp]    "r"(SATP_SV39 | ((uint64_t) next->page_table / PAGE_SIZE)),
        [sscratch]"r"((uint64_t)&next->stack[sizeof(next->stack)])
  );

  // 6) Perform the context switch
  struct process *prev = current_proc;
  current_proc = next;
  switch_context(&prev->sp, &next->sp);
}


void delay(void) {
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop"); // do nothing
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        yield();
        delay();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        yield();
        delay();
    }
}


struct virtio_virtq *virtq_init(unsigned index) {
  // Allocate a region for the virtqueue.
  paddr_t virtq_paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
  struct virtio_virtq *vq = (struct virtio_virtq *) virtq_paddr;
  vq->queue_index = index;
  vq->used_index = (volatile uint16_t *) &vq->used.index;
  // 1. Select the queue writing its index (first queue is 0) to QueueSel.
  virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);
  // 5. Notify the device about the queue size by writing the size to QueueNum.
  virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);
  // 6. Notify the device about the used alignment by writing its value in bytes to QueueAlign.
  virtio_reg_write32(VIRTIO_REG_QUEUE_ALIGN, 0);
  // 7. Write the physical number of the first page of the queue to the QueuePFN register.
  virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr);
  return vq;
}

// Notifies the device that there is a new request. `desc_index` is the index
// of the head descriptor of the new request.
void virtq_kick(struct virtio_virtq *vq, int desc_index) {
  vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
  vq->avail.index++;
  __sync_synchronize();
  virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
  vq->last_used_index++;
}

// Returns whether there are requests being processed by the device.
bool virtq_is_busy(struct virtio_virtq *vq) {
  return vq->last_used_index != *vq->used_index;
}

// Reads/writes from/to virtio-blk device.
void read_write_disk(void *buf, unsigned sector, int is_write) {
  if (sector >= blk_capacity / SECTOR_SIZE) {
      printf("virtio: tried to read/write sector=%d, but capacity is %d\n",
            sector, blk_capacity / SECTOR_SIZE);
      return;
  }

  // Construct the request according to the virtio-blk specification.
  blk_req->sector = sector;
  blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
  if (is_write)
      memcpy(blk_req->data, buf, SECTOR_SIZE);

  // Construct the virtqueue descriptors (using 3 descriptors).
  struct virtio_virtq *vq = blk_request_vq;
  vq->descs[0].addr = blk_req_paddr;
  vq->descs[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
  vq->descs[0].flags = VIRTQ_DESC_F_NEXT;
  vq->descs[0].next = 1;

  vq->descs[1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
  vq->descs[1].len = SECTOR_SIZE;
  vq->descs[1].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
  vq->descs[1].next = 2;

  vq->descs[2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
  vq->descs[2].len = sizeof(uint8_t);
  vq->descs[2].flags = VIRTQ_DESC_F_WRITE;

  // Notify the device that there is a new request.
  virtq_kick(vq, 0);

  // Wait until the device finishes processing.
  while (virtq_is_busy(vq))
      ;

  // virtio-blk: If a non-zero value is returned, it's an error.
  if (blk_req->status != 0) {
      printf("virtio: warn: failed to read/write sector=%d status=%d\n",
             sector, blk_req->status);
      return;
  }

  // For read operations, copy the data into the buffer.
  if (!is_write)
      memcpy(buf, blk_req->data, SECTOR_SIZE);
}

void virtio_blk_init(void) {
  if (virtio_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976)
      PANIC("virtio: invalid magic value");
  if (virtio_reg_read32(VIRTIO_REG_VERSION) != 1)
      PANIC("virtio: invalid version");
  if (virtio_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK)
      PANIC("virtio: invalid device id");

  // 1. Reset the device.
  virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);
  // 2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device.
  virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
  // 3. Set the DRIVER status bit.
  virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);
  // 5. Set the FEATURES_OK status bit.
  virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_FEAT_OK);
  // 7. Perform device-specific setup, including discovery of virtqueues for the device
  blk_request_vq = virtq_init(0);
  // 8. Set the DRIVER_OK status bit.
  virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

  // Get the disk capacity.
  blk_capacity = virtio_reg_read64(VIRTIO_REG_DEVICE_CONFIG + 0) * SECTOR_SIZE;
  // printf("virtio-blk: capacity is %d bytes\n", blk_capacity);

  // Allocate a region to store requests to the device.
  blk_req_paddr = alloc_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
  blk_req = (struct virtio_blk_req *) blk_req_paddr;
}

struct file *fs_lookup(const char *filename) {
  for (int i = 0; i < FILES_MAX; i++) {
      struct file *file = &files[i];
      if (!strcmp(file->name, filename))
          return file;
  }

  return NULL;
}

void fs_flush(void) {
  // Copy all file contents into `disk` buffer.
  memset(disk, 0, sizeof(disk));
  unsigned off = 0;
  for (int file_i = 0; file_i < FILES_MAX; file_i++) {
      struct file *file = &files[file_i];
      if (!file->in_use)
          continue;

      struct tar_header *header = (struct tar_header *) &disk[off];
      memset(header, 0, sizeof(*header));
      strcpy(header->name, file->name);
      strcpy(header->mode, "000644");
      strcpy(header->magic, "ustar");
      strcpy(header->version, "00");
      header->type = '0';

      // Turn the file size into an octal string.
      int filesz = file->size;
      for (int i = sizeof(header->size); i > 0; i--) {
          header->size[i - 1] = (filesz % 8) + '0';
          filesz /= 8;
      }

      // Calculate the checksum.
      int checksum = ' ' * sizeof(header->checksum);
      for (unsigned i = 0; i < sizeof(struct tar_header); i++)
          checksum += (unsigned char) disk[off + i];

      for (int i = 5; i >= 0; i--) {
          header->checksum[i] = (checksum % 8) + '0';
          checksum /= 8;
      }

      // Copy file data.
      memcpy(header->data, file->data, file->size);
      off += align_up(sizeof(struct tar_header) + file->size, SECTOR_SIZE);
  }

  // Write `disk` buffer into the virtio-blk.
  for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
      read_write_disk(&disk[sector * SECTOR_SIZE], sector, true);

  printf("wrote %d bytes to disk\n", sizeof(disk));
}


/// Find the process control block for a given PID, or NULL if not found.
static struct process *
find_proc_by_pid(int pid) {
    for (int i = 0; i < PROCS_MAX; i++) {
        if (procs[i].pid == pid)
            return &procs[i];
    }
    return NULL;
}

/// Safely read one byte from user‐space at uaddr.
/// (Here we just trust the user pointer; in a real OS you'd validate the vaddr
/// against the process’s page table.)
static uint8_t
safe_load_byte(const uint8_t *uaddr) {
    return *uaddr;
}

/// Safely write one byte to user‐space at uaddr.
static void
safe_store_byte(uint8_t *uaddr, uint8_t val) {
    *uaddr = val;
}


// Internal IPC send for kernel use (non-blocking)
static int ipc_do_send(int dest, const void *msg, int len) {
  struct process *dst = find_proc_by_pid(dest);
  if (!dst || len < 0 || len > IPC_BUF_SIZE) return -1;
  if (dst->ipc_count + len > IPC_BUF_SIZE) return -2;
  const uint8_t *src = msg;
  for (int i = 0; i < len; i++) {
      dst->ipc_buf[dst->ipc_tail] = safe_load_byte(src + i);  
      dst->ipc_tail = (dst->ipc_tail + 1) % IPC_BUF_SIZE;
  }
  dst->ipc_count += len;
  if (dst->ipc_blocked) {
      dst->ipc_blocked = false;
      dst->state = PROC_RUNNABLE;
  }
  return len;
}


static long dispatch_syscall(int no, struct trap_frame *f) {
  switch (no) {
      case SYS_PUTCHAR:
          putchar(f->a0);
          return 0;

      case SYS_GETCHAR: {
          long c;
          do {
              c = getchar();
              yield();
          } while (c < 0);
          f->a0 = c;
          return c;
      }

      case SYS_EXIT:
          current_proc->state = PROC_EXITED;
          yield();
          // unreachable
     
      case SYS_READFILE:
      case SYS_WRITEFILE: {
          const char *filename = (const char *)f->a0;
          char *buf             = (char *)       f->a1;
          int   len             =                f->a2;
          struct file *file     = fs_lookup(filename);
          if (!file) {
              f->a0 = -1;
              return -1;
          }
          if (f->a3 == SYS_WRITEFILE) {
              if (len > (int)sizeof(file->data)) len = file->size;
              memcpy(file->data, buf, len);
              file->size = len;
              fs_flush();
          } else {
              if (len > (int)sizeof(file->data)) len = file->size;
              memcpy(buf, file->data, len);
          }
          f->a0 = len;
          yield();
          return len;
      }

      case SYS_IPC_SEND:
          return ipc_do_send(
              /* dest */   f->a0,
              /* msg ptr */(void *)f->a1,
              /* length */ f->a2
          );

      case SYS_IPC_RECV: {
          uint8_t *buf    = (void *)f->a0;
          int       maxlen=                f->a1;

          if (maxlen < 0)
              return -1;

          if (current_proc->ipc_count == 0) {
              current_proc->ipc_blocked = true;
              current_proc->state       = PROC_UNUSED;
              yield();
          }

          int got = current_proc->ipc_count;
          if (got > maxlen) got = maxlen;

          for (int i = 0; i < got; i++) {
              safe_store_byte(
                  buf + i,
                  current_proc->ipc_buf[current_proc->ipc_head]
              );
              current_proc->ipc_head =
                  (current_proc->ipc_head + 1) % IPC_BUF_SIZE;
          }
          current_proc->ipc_count -= got;
          return got;
      }
      break;
      case SYS_SPAWN: {
        const void *image = (void*)f->a0;
        int         sz    = (int)   f->a1;
        if (!image || sz <= 0) {
            f->a0 = -1;
        } else {
            struct process *p = create_process(image, sz);
            f->a0 = p ? p->pid : -1;
        }
        // optionally preempt so the new process can run immediately:
        yield();
        return f->a0;
      }
      case SYS_MONITOR_KILL:
          // kill the process with the given PID
          if (f->a0 == 0) {
              f->a0 = -1;
              return -1;
          }
          struct process *p = find_proc_by_pid(f->a0);
          if (!p) {
              f->a0 = -1;
              return -1;
          }
          p->state = PROC_EXITED;
          printf("Malware Detected: Killing process %d\n", f->a0);
          yield();
          return 0;
      break;
      case SYS_CONNECT:
          yield();
          return 0;
      break;
      case SYS_DUP2:
        yield();  
        return 0;
      break;
      case SYS_EXECVE:
        yield();  
        return 0;
      break;

      default:
          PANIC("unexpected syscall %d", no);
  }
}


// static void post_syscall_hook(int pid, int sysno, uint64_t *args) {
//   struct SyscallEvent ev = {.pid = pid, .num = sysno};
//   for (int i = 0; i < 4; i++) ev.args[i] = args[i];
//   // drop on full
//   (void)ipc_do_send(monitor_pid, &ev, sizeof(ev));
// }

static void post_syscall_hook(int pid, int sysno, uint64_t *args) {
  // don’t notify the monitor about *its own* syscalls
  if (pid == monitor_pid)
      return;

  struct SyscallEvent ev = { .pid = pid, .num = sysno };
  for (int i = 0; i < 4; i++)
      ev.args[i] = args[i];

  // send it into the monitor’s IPC buffer
  (void)ipc_do_send(monitor_pid, &ev, sizeof(ev));
}


// // Trap entry stub
// void handle_trap(struct trap_frame *f) {
//   uint64_t sc = READ_CSR(scause);
//   uint64_t pc = READ_CSR(sepc);
//   if ((sc >> 63) != 0) {
//     // interrupts ... as before ...
//   } else if ((sc & 0xfff) == SCAUSE_ECALL) {
//     int no = f->a3;
//     uint64_t args[] = {f->a0,f->a1,f->a2,f->a3};
//     long ret = dispatch_syscall(no, f);
//     f->a0 = ret;
//     post_syscall_hook(current_proc->pid, no, args);
//     WRITE_CSR(sepc, pc + 4);
//   } else {
//     PANIC("unexpected trap scause=%lx", sc);
//   }
// }


// kernel.c

void handle_trap(struct trap_frame *f) {
  uint64_t sc = READ_CSR(scause);
  uint64_t pc = READ_CSR(sepc);

  // —— Interrupt? (high bit set) —————————————————————————————————
  if (sc >> 63) {
      uint64_t cause = sc & 0xfff;
      switch (cause) {
          case INTR_STIMER: {
              // Re-arm next tick (~10 ms on QEMU’s 10 MHz CLINT)
              uint64_t now  = READ_CSR(time);
              uint64_t when = now + 10000000;
              sbi_call(when, when >> 32, 0,0,0,0, 0,0);

              // Preempt current process
              yield();
              return;
          }

          case INTR_SEXT:   // supervisor-external (e.g. virtio-blk)
          case INTR_MEXT:   // machine-external (delegated into S-mode)
              // No PLIC driver yet → just drop it
              return;

          default:
              PANIC("unexpected interrupt cause=%lx", cause);
      }
  }
  // —— Syscall (S-mode ECALL) —————————————————————————————————
  else if ((sc & 0xfff) == SCAUSE_ECALL) {
      int     no   = f->a3;
      uint64_t args[4] = { f->a0, f->a1, f->a2, f->a3 };

      long ret = dispatch_syscall(no, f);
      f->a0 = ret;

      post_syscall_hook(current_proc->pid, no, args);

      // Advance past the ECALL instruction
      WRITE_CSR(sepc, pc + 4);
      return;
  }
  // —— Everything else is fatal ————————————————————————————————
  else {
      PANIC("unexpected trap scause=%lx, sepc=%lx", sc, pc);
  }
}

int oct2int(char *oct, int len) {
    int dec = 0;
    for (int i = 0; i < len; i++) {
        if (oct[i] < '0' || oct[i] > '7')
            break;

        dec = dec * 8 + (oct[i] - '0');
    }
    return dec;
}

void fs_init(void) {
    for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
        read_write_disk(&disk[sector * SECTOR_SIZE], sector, false);

    unsigned off = 0;
    for (int i = 0; i < FILES_MAX; i++) {
        struct tar_header *header = (struct tar_header *) &disk[off];
        if (header->name[0] == '\0')
            break;

        if (strcmp(header->magic, "ustar") != 0)
            PANIC("invalid tar header: magic=\"%s\"", header->magic);

        int filesz = oct2int(header->size, sizeof(header->size));
        struct file *file = &files[i];
        file->in_use = true;
        strcpy(file->name, header->name);
        memcpy(file->data, header->data, filesz);
        file->size = filesz;
        // printf("file: %s, size=%d\n", file->name, file->size);

        off += align_up(sizeof(struct tar_header) + filesz, SECTOR_SIZE);
    }
}


void kernel_main(void) {
  // Zero BSS
  memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

  // 1) Point traps at kernel_entry
  WRITE_CSR(stvec, (uint64_t)kernel_entry);

  // 2) Enable timer & external interrupts in S-mode

  // uint64_t _sie = READ_CSR(sie);
  // _sie |= SIE_STIE    // timer
  //       | SIE_SEIE;   // external (UART, virtio, etc.)
  // WRITE_CSR(sie, _sie);
  uint64_t _sie = READ_CSR(sie);
  // _sie |= SIE_STIE;   // only timer interrupts
  _sie &= ~SIE_STIE;   // only timer interrupts

  uint64_t _sst = READ_CSR(sstatus);
  _sst |= SSTATUS_SIE;  // global enable
  WRITE_CSR(sstatus, _sst);


  // 3) Arm the first timer tick (~10 ms on QEMU virt’s 10 MHz CLINT)
  
  uint64_t now  = READ_CSR(time);
  uint64_t when = now + 10000000;
  // SBI legacy set_timer:
  sbi_call(when, when >> 32, 0,0,0,0, /*fid=*/0, /*eid=*/0);
  

  // 4) Init devices & filesystem
  virtio_blk_init();
  fs_init();

  char buf[SECTOR_SIZE];
  read_write_disk(buf, 0, false);
  // printf("first sector: %s\n", buf);

  strcpy(buf, "hello from kernel!!!\n");
  read_write_disk(buf, 0, true);

  // 5) Create idle, monitor, shell in that order
  idle_proc = create_process(NULL, 0);
  idle_proc->pid = 0;
  current_proc = idle_proc;

  // create_process(monitor_bin,        monitor_bin_len);  // PID 1
  
  struct process *mp = create_process(monitor_bin, monitor_bin_len);
  monitor_pid = mp->pid;
  
  create_process(_binary_shell_bin_start,
                 (size_t)(_binary_shell_bin_end - _binary_shell_bin_start));  // PID 2

  // 6) Enter the scheduler
  yield();
  PANIC("Returned to idle. Likely user code crashed.");
}



__attribute__((section(".text.boot"))) 
__attribute__((naked)) 
void boot(void) {
  __asm__ __volatile__(
      "mv sp, %[stack_top]\n" // Set the stack pointer
      "j kernel_main\n"       // Jump to the kernel main function
      :
      : [stack_top] "r"(
          __stack_top) // Pass the stack top address as %[stack_top]
  );
}
