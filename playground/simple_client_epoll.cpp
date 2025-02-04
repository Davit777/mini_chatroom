#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT "9034"
#define BUF_SIZE 256

void *get_in_addr(struct sockaddr *sa) {
    return sa->sa_family == AF_INET 
        ? (void*)&(((struct sockaddr_in*)sa)->sin_addr)
        : (void*)&(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, epoll_fd;
    struct addrinfo hints, *servinfo, *p;
    struct epoll_event event, events[2];
    char buf[BUF_SIZE];

    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rv;
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (!p) {
        fprintf(stderr, "Failed to connect\n");
        return 2;
    }

    char s[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("Connected to %s\n", s);
    freeaddrinfo(servinfo);

    // Set up epoll
    epoll_fd = epoll_create1(0);
    event.events = EPOLLIN;
    
    // Monitor stdin (0) and socket
    event.data.fd = 0;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event);
    
    event.data.fd = sockfd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, 2, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == 0) {  // stdin
                if (fgets(buf, BUF_SIZE, stdin) == NULL) goto done;
                if (send(sockfd, buf, strlen(buf), 0) == -1)
                    perror("send");
            } else {  // socket
                int numbytes = recv(sockfd, buf, BUF_SIZE - 1, 0);
                if (numbytes <= 0) {
                    if (numbytes == 0) printf("Server disconnected\n");
                    else perror("recv");
                    goto done;
                }
                buf[numbytes] = '\0';
                printf("%s", buf);
            }
        }
    }
done:
    close(sockfd);
    close(epoll_fd);
    return 0;
}
