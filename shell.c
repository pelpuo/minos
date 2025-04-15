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

void main(void) {
    // *((volatile int *) 0x80200000) = 0x1234; // new!
    // for (;;);
    printf("Hello World from shell!\n");
}