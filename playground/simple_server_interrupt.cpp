#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9034
#define MAX_CLIENTS 10
#define BUF_SIZE 256

volatile sig_atomic_t got_signal = 0;
int listener;
int clients[MAX_CLIENTS];
int n_clients = 0;

void io_handler(int sig) {
    got_signal = 1;
}

void setup_sigio(int fd) {
    fcntl(fd, F_SETOWN, getpid());
    fcntl(fd, F_SETFL, O_ASYNC | O_NONBLOCK);
    signal(SIGIO, io_handler);
}

void broadcast(int sender, char *buf, size_t len) {
    for (int i = 0; i < n_clients; i++) {
        if (clients[i] != sender && clients[i] != 0) {
            send(clients[i], buf, len, 0);
        }
    }
}

int main() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);
    
    setup_sigio(listener);
    printf("Server listening on port %d\n", PORT);

    while (1) {
        if (!got_signal) {
            pause();
            continue;
        }
        got_signal = 0;

        // Check listener
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int newfd = accept(listener, (struct sockaddr*)&client_addr, &addrlen);
        if (newfd > 0) {
            printf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));
            setup_sigio(newfd);
            if (n_clients < MAX_CLIENTS) {
                clients[n_clients++] = newfd;
            } else {
                close(newfd);
            }
        }

        // Check client sockets
        for (int i = 0; i < n_clients; i++) {
            if (clients[i] == 0) continue;
            
            char buf[BUF_SIZE];
            ssize_t n = recv(clients[i], buf, BUF_SIZE, 0);
            
            if (n > 0) {
                broadcast(clients[i], buf, n);
            } else {
                printf("Client %d disconnected\n", clients[i]);
                close(clients[i]);
                clients[i] = 0;
            }
        }
    }
    close(listener);
    return 0;
}
