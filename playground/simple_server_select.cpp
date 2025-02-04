#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034"

void *get_in_addr(struct sockaddr *sa) {
    return sa->sa_family == AF_INET 
        ? (void*)&(((struct sockaddr_in*)sa)->sin_addr)
        : (void*)&(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main() {
    fd_set master, read_fds;
    int fdmax, listener, newfd, nbytes;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    char buf[256], remoteIP[INET6_ADDRSTRLEN];
    int yes = 1;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

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
    FD_SET(listener, &master);
    fdmax = listener;

    for (;;) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(3);
        }

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master);
                        fdmax = (newfd > fdmax) ? newfd : fdmax;
                        printf("New connection from %s on socket %d\n",
                               inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),
                                         remoteIP, INET6_ADDRSTRLEN), newfd);
                    }
                } else {
                    nbytes = recv(i, buf, sizeof buf, 0);
                    if (nbytes <= 0) {
                        if (nbytes == 0)
                            printf("Socket %d closed\n", i);
                        else
                            perror("recv");
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                        for (int j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master) && j != listener && j != i) {
                                if (send(j, buf, nbytes, 0) == -1) {
                                    perror("send");
                                    close(j);
                                    FD_CLR(j, &master);
                                    if (j == fdmax) {
                                        while (fdmax > 0 && !FD_ISSET(fdmax, &master))
                                            fdmax--;
                                    }
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
