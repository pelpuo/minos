// #include <stdio.h>

// int main(){
//     char *args[] = {"/bin/sh", NULL};
//     char *env[] = {NULL};

//     // Execute the shell
//     execve(args[0], args, env);

//     // If execve fails, print an error message
//     perror("execve failed");
//     return 1;

// }

#include "user.h"
#include "common.h"
#include "myapp_bin.h"    // ← from `xxd -i myapp.bin > myapp_bin.h`
#include "mal_bin.h"    // ← from `xxd -i myapp.bin > myapp_bin.h`

// extern const uint8_t myapp_bin[];    // linker symbols
// extern const size_t  myapp_bin_len;


// void main(void) {
//     // *((volatile int *) 0x80200000) = 0x1234; // new!

//     // putchar('S');
//     // putchar('H');
//     // putchar('E');
//     // putchar('L');
//     // putchar('L');
//     // putchar('\n');
//     printf("Hello World from shell!\n");
//     for (;;);
// }

void main(void) {
    while (1) {
prompt:
        printf("> ");
        char cmdline[128];
        for (int i = 0;; i++) {
            char ch = getchar();
            putchar(ch);
            if (i == sizeof(cmdline) - 1) {
                printf("command line too long\n");
                goto prompt;
            } else if (ch == '\r') {
                printf("\n");
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }

        if (strcmp(cmdline, "hello") == 0)
            printf("Hello world from shell!\n");
        else if (strcmp(cmdline, "exit") == 0)
            exit();
        else if (strcmp(cmdline, "readfile") == 0) {
            char buf[128];
            int len = readfile("hello.txt", buf, sizeof(buf));
            buf[len] = '\0';
            printf("%s\n", buf);
        }
        else if (strcmp(cmdline, "myapp") == 0) {
            int pid = spawn(myapp_bin, (int)myapp_bin_len);
            if (pid < 0)
                printf("shell: failed to spawn myapp\n");
            // else
                // printf("shell: launched myapp (pid=%d)\n", pid);
        }
        else if (strcmp(cmdline, "malware") == 0) {
            int pid = spawn(mal_bin, (int)mal_bin_len);
            if (pid < 0)
                printf("shell: failed to spawn myapp\n");
            // else
                // printf("shell: launched myapp (pid=%d)\n", pid);
        }
        else if (strcmp(cmdline, "writefile") == 0)
            writefile("hello.txt", "Hello from shell!\n", 19);
        else
            printf("unknown command: %s\n", cmdline);
    }
}

