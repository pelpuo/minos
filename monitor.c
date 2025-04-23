// monitor.c
#include "user.h"

// // Must match kernel’s definition exactly:
// struct SyscallEvent {
//     int     pid;
//     int     num;
//     uint64_t args[4];
// };

int main(void) {
    struct SyscallEvent ev;
    // Simple rule: count writefile syscalls per-PID
    int write_counts[16] = {0};

    printf("monitor: started\n");
    while (1) {
        int n = ipc_recv(&ev, sizeof(ev));
        if (n != sizeof(ev)) {
            // malformed or partial, skip
            continue;
        }

        // Log every syscall
        
        
        // printf("monitor: pid=%d sysno=%d args=[%lx,%lx,%lx,%lx]\n",
        //        ev.pid, ev.num,
        //        ev.args[0], ev.args[1],
        //        ev.args[2], ev.args[3]);

        // Example vulnerability trigger: too many writefile syscalls
        if (ev.num == SYS_WRITEFILE) {
            write_counts[ev.pid]++;
            if (write_counts[ev.pid] > 3) {
                printf("monitor: pid %d did >3 writes → terminating\n", ev.pid);
                // ask the kernel to kill this PID:
                // for now we just exit our monitor, real impl could send a special RPC
                monitor_kill(ev.pid);
                break;
            }
        }
    }

    printf("monitor: exiting\n");
    return 0;
}
