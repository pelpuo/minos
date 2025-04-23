// // monitor.c
// #include "user.h"

// // // Must match kernel’s definition exactly:
// // struct SyscallEvent {
// //     int     pid;
// //     int     num;
// //     uint64_t args[4];
// // };

// int main(void) {
//     struct SyscallEvent ev;
//     // Simple rule: count writefile syscalls per-PID
//     int write_counts[16] = {0};

//     printf("monitor: started\n");
//     while (1) {
//         int n = ipc_recv(&ev, sizeof(ev));
//         if (n != sizeof(ev)) {
//             // malformed or partial, skip
//             continue;
//         }

//         // Log every syscall
        
//         // printf("monitor: pid=%d sysno=%d args=[%lx,%lx,%lx,%lx]\n",
//         //        ev.pid, ev.num,
//         //        ev.args[0], ev.args[1],
//         //        ev.args[2], ev.args[3]);

//         // Example vulnerability trigger: too many writefile syscalls
//         if (ev.num == SYS_WRITEFILE) {
//             write_counts[ev.pid]++;
//             if (write_counts[ev.pid] > 3) {
//                 printf("monitor: pid %d did >3 writes → terminating\n", ev.pid);
//                 // ask the kernel to kill this PID:
//                 // for now we just exit our monitor, real impl could send a special RPC
//                 monitor_kill(ev.pid);
//                 break;
//             }
//         }
//     }

//     printf("monitor: exiting\n");
//     return 0;
// }

// monitor.c
#include "user.h"
#include "common.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_PIDS         16
#define MAX_HISTORY      32
#define MAX_PATTERNS     2  // now two signatures: heavy‐writes and reverse‐shell IPC
#define MAX_PATTERN_LEN  4  // longest pattern length (SYS_CONNECT + 3*SYS_DUP2)

// Control‐channel syscall for monitor → kernel commands
#define SYS_MONITOR_CONTROL  9
#define MON_CTRL_KILL        1

// Malware pattern: sequence of syscall numbers
typedef struct {
    int len;
    int seq[MAX_PATTERN_LEN];
} pattern_t;

// Two patterns:
// 1) Reverse‑shell style: connect, dup2, dup2, dup2
// 2) Heavy writes: writefile, writefile, writefile
static const pattern_t malware_patterns[MAX_PATTERNS] = {
    { .len = 4, .seq = { SYS_CONNECT, SYS_DUP2, SYS_DUP2, SYS_DUP2 } },
    { .len = 3, .seq = { SYS_READFILE, SYS_CONNECT, SYS_WRITEFILE } }
};

// Per‑PID circular history buffer
static int history[MAX_PIDS][MAX_HISTORY];
static int hist_idx[MAX_PIDS] = {0};

// Record a syscall into that PID’s history
static void record_syscall(int pid, int sysno) {
    hist_idx[pid] = (hist_idx[pid] + 1) % MAX_HISTORY;
    history[pid][hist_idx[pid]] = sysno;
}

// Check if the last `p->len` entries match the pattern
static bool matches_pattern(int pid, const pattern_t *p) {
    if (p->len <= 0 || p->len > MAX_HISTORY) return false;
    int idx = hist_idx[pid];
    for (int i = p->len - 1; i >= 0; i--) {
        if (history[pid][idx] != p->seq[i])
            return false;
        idx = (idx - 1 + MAX_HISTORY) % MAX_HISTORY;
    }
    return true;
}

// Scan all patterns for this PID
static bool check_malware(int pid) {
    for (int i = 0; i < MAX_PATTERNS; i++) {
        if (matches_pattern(pid, &malware_patterns[i]))
            return true;
    }
    return false;
}

int main(void) {
    struct SyscallEvent ev;
    printf("monitor: started\n");
    while (true) {
        if (ipc_recv(&ev, sizeof(ev)) != sizeof(ev))
            continue;

        // Skip trivial console I/O
        if (ev.num == SYS_GETCHAR || ev.num == SYS_PUTCHAR)
            continue;

        // 1) record history
        record_syscall(ev.pid, ev.num);

        // 2) debug print
        printf("monitor: pid=%d sys=%d\n", ev.pid, ev.num);

        // 3) detect malware patterns
        if (check_malware(ev.pid)) {
            printf("monitor: pid %d matched malware pattern → terminating\n", ev.pid);
            monitor_kill(ev.pid);
        }
    }

    return 0;
}
