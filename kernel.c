#include "kernel.h"
#include "common.h"
#include <stdint.h>

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

__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void) {
  __asm__ __volatile__("csrw sscratch, sp\n"
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



struct process procs[PROCS_MAX]; // All process control structures.

extern char __kernel_base[];

// ↓ __attribute__((naked)) is very important!
__attribute__((naked)) void user_entry(void) {
  __asm__ __volatile__(
      "csrw sepc, %[sepc]        \n"
      "csrw sstatus, %[sstatus]  \n"
      "sret                      \n"
      :
      : [sepc] "r" (USER_BASE),
        [sstatus] "r" (SSTATUS_SPIE)
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
    // *--sp = (uint64_t) pc;          // ra
    *--sp = (uint64_t) user_entry;  // ra (changed!)

    uint64_t *page_table = (uint64_t *) alloc_pages(1);

    for (paddr_t paddr = (paddr_t) __kernel_base;
         paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);

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

struct process *current_proc; // Currently running process
struct process *idle_proc;    // Idle process

void yield(void) {
    // Search for a runnable process
    struct process *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    // If there's no runnable process other than the current one, return and continue processing
    if (next == current_proc)
        return;

    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        : [satp] "r" (SATP_SV39 | ((uint64_t) next->page_table / PAGE_SIZE)), 
        [sscratch] "r" ((uint64_t) &next->stack[sizeof(next->stack)])
    );

    // Context switch
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


void handle_syscall(struct trap_frame *f) {
  switch (f->a3) {
      case SYS_PUTCHAR:
          putchar(f->a0);
          break;
      default:
          PANIC("unexpected syscall a3=%x\n", f->a3);
  }
}


void handle_trap(struct trap_frame *f) {
  uint64_t scause = READ_CSR(scause);
  uint64_t stval = READ_CSR(stval);
  uint64_t user_pc = READ_CSR(sepc);

  if (scause == SCAUSE_ECALL) {
      handle_syscall(f);
      user_pc += 4;
  } else {
      PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
  }

  WRITE_CSR(sepc, user_pc);

}




void kernel_main(void) {
  memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

  WRITE_CSR(stvec, (uint64_t) kernel_entry); // new
  // __asm__ __volatile__("unimp"); // new

//   paddr_t paddr0 = alloc_pages(2);
//   paddr_t paddr1 = alloc_pages(1);
//   printf("alloc_pages test: paddr0=%x\n", paddr0);
//   printf("alloc_pages test: paddr1=%x\n", paddr1);

//   proc_a = create_process((uint64_t) proc_a_entry);
//   proc_b = create_process((uint64_t) proc_b_entry);
//   proc_a_entry();

//   PANIC("booted!");
//   printf("unreachable here!\n");

    // idle_proc = create_process((uint64_t) NULL);
    idle_proc = create_process(NULL, 0); // updated!
    idle_proc->pid = 0; // idle
    current_proc = idle_proc;

    // proc_a = create_process((uint64_t) proc_a_entry);
    // proc_b = create_process((uint64_t) proc_b_entry);

   // new!
   create_process(_binary_shell_bin_start, (uintptr_t)_binary_shell_bin_end - (uintptr_t)_binary_shell_bin_start);

    yield();
    PANIC("switched to idle process");
}

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
  __asm__ __volatile__(
      "mv sp, %[stack_top]\n" // Set the stack pointer
      "j kernel_main\n"       // Jump to the kernel main function
      :
      : [stack_top] "r"(
          __stack_top) // Pass the stack top address as %[stack_top]
  );
}
