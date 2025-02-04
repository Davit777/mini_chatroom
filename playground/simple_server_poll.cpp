#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#define PORT "9034"
#define MAX_CLIENTS 100
#define BUF_SIZE 256

void *get_in_addr(struct sockaddr *sa) {
    return sa->sa_family == AF_INET 
        ? (void*)&(((struct sockaddr_in*)sa)->sin_addr)
        : (void*)&(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main() {
    int listener, newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    char buf[BUF_SIZE], remoteIP[INET6_ADDRSTRLEN];
    int yes = 1;

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;  // Initially only listener
    int current_size;

    // Set up listening socket
    struct addrinfo hints, *ai, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &ai) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    for (p = ai; p; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) continue;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }

    if (!p) {
        fprintf(stderr, "Failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai);
    listen(listener, 10);

    // Initialize pollfd structure
    fds[0].fd = listener;
    fds[0].events = POLLIN;

    for (;;) {
        int poll_count = poll(fds, nfds, -1);
        if (poll_count == -1) {
            perror("poll");
            exit(3);
        }

        current_size = nfds;
        for (int i = 0; i < current_size; i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == listener) {
                    // Handle new connection
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        printf("New connection from %s on socket %d\n",
                               inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),
                                         remoteIP, INET6_ADDRSTRLEN), newfd);

                        if (nfds < MAX_CLIENTS + 1) {
                            fds[nfds].fd = newfd;
                            fds[nfds].events = POLLIN;
                            nfds++;
                        } else {
                            fprintf(stderr, "Too many connections\n");
                            close(newfd);
                        }
                    }
                } else {
                    // Handle client data
                    int nbytes = recv(fds[i].fd, buf, BUF_SIZE, 0);
                    if (nbytes <= 0) {
                        if (nbytes == 0)
                            printf("Socket %d closed\n", fds[i].fd);
                        else
                            perror("recv");
                        close(fds[i].fd);
                        // Remove from pollfd array
                        fds[i] = fds[nfds - 1];
                        nfds--;
                        i--;  // Re-check this index
                    } else {
                        // Broadcast to all other clients
                        for (int j = 1; j < nfds; j++) {
                            if (j != i && fds[j].fd != listener) {
                                if (send(fds[j].fd, buf, nbytes, 0) == -1) {
                                    perror("send");
                                    close(fds[j].fd);
                                    fds[j] = fds[nfds - 1];
                                    nfds--;
                                    j--;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
