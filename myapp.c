#include "user.h"

int main(void) {
    // *((volatile int *) 0x80200000) = 0x1234; // new!
    printf("Hello World from User App!\n");
    writefile("hello.txt", "Hello from User App!\n", 19);
    exit();
}