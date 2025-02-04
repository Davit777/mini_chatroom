#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 256

volatile sig_atomic_t input_ready = 0;
int sockfd;

void input_handler(int sig) {
    input_ready = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <IP>\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9034);
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    
    // Setup SIGIO for socket
    fcntl(sockfd, F_SETOWN, getpid());
    fcntl(sockfd, F_SETFL, O_ASYNC | O_NONBLOCK);
    signal(SIGIO, input_handler);

    // Setup terminal input
    fcntl(0, F_SETFL, O_ASYNC | O_NONBLOCK);
    signal(SIGIO, input_handler);

    printf("Connected to %s\n", argv[1]);
    
    while (1) {
        pause();  // Wait for signal
        
        // Handle socket input
        char buf[BUF_SIZE];
        ssize_t n = recv(sockfd, buf, BUF_SIZE, 0);
        if (n > 0) {
            write(1, buf, n);
        } else if (n == 0) {
            printf("Connection closed\n");
            break;
        }

        // Handle user input
        if (input_ready) {
            ssize_t n = read(0, buf, BUF_SIZE);
            if (n > 0) {
                send(sockfd, buf, n, 0);
            }
            input_ready = 0;
        }
    }
    close(sockfd);
    return 0;
}
