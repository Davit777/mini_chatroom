#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034"
#define MAX_EVENTS 100
#define BUF_SIZE 256

void *get_in_addr(struct sockaddr *sa) {
    return sa->sa_family == AF_INET 
        ? (void*)&(((struct sockaddr_in*)sa)->sin_addr)
        : (void*)&(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main() {
    int listener, epoll_fd, client_count = 0;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    char buf[BUF_SIZE], remoteIP[INET6_ADDRSTRLEN];
    int yes = 1;

    struct epoll_event event, events[MAX_EVENTS];
    int *clients = malloc(MAX_EVENTS * sizeof(int));

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
    listen(listener, SOMAXCONN);

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    event.events = EPOLLIN;
    event.data.fd = listener;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &event);

    printf("Server running on port %s\n", PORT);

    for (;;) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(3);
        }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            
            if (fd == listener) {
                addrlen = sizeof remoteaddr;
                int newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                if (newfd == -1) {
                    perror("accept");
                    continue;
                }

                printf("New connection from %s on socket %d\n",
                       inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),
                                 remoteIP, INET6_ADDRSTRLEN), newfd);

                event.events = EPOLLIN | EPOLLRDHUP;
                event.data.fd = newfd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newfd, &event);
                clients[client_count++] = newfd;
            } else {
                if (events[i].events & EPOLLRDHUP) {
                    // Client disconnected
                    printf("Socket %d closed\n", fd);
                    close(fd);
                    for (int j = 0; j < client_count; j++) {
                        if (clients[j] == fd) {
                            clients[j] = clients[--client_count];
                            break;
                        }
                    }
                    continue;
                }

                ssize_t nbytes = recv(fd, buf, BUF_SIZE, 0);
                if (nbytes <= 0) {
                    if (nbytes == 0) printf("Socket %d closed\n", fd);
                    else perror("recv");
                    close(fd);
                    for (int j = 0; j < client_count; j++) {
                        if (clients[j] == fd) {
                            clients[j] = clients[--client_count];
                            break;
                        }
                    }
                } else {
                    // Broadcast to all other clients
                    for (int j = 0; j < client_count; j++) {
                        if (clients[j] != fd && clients[j] != listener) {
                            if (send(clients[j], buf, nbytes, 0) == -1) {
                                perror("send");
                                close(clients[j]);
                                clients[j] = clients[--client_count];
                            }
                        }
                    }
                }
            }
        }
    }
    close(epoll_fd);
    free(clients);
    return 0;
}
