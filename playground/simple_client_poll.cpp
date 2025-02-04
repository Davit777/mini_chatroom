#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT "9034"
#define BUF_SIZE 256

void *get_in_addr(struct sockaddr *sa) {
    return sa->sa_family == AF_INET 
        ? (void*)&(((struct sockaddr_in*)sa)->sin_addr)
        : (void*)&(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

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

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("Connected to %s\n", s);
    freeaddrinfo(servinfo);

    struct pollfd fds[2];
    char buf[BUF_SIZE];

    // Set up file descriptors to poll
    fds[0].fd = 0;       // stdin
    fds[0].events = POLLIN;
    fds[1].fd = sockfd;  // server socket
    fds[1].events = POLLIN;

    while (1) {
        int poll_count = poll(fds, 2, -1);
        if (poll_count == -1) {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {  // stdin input
            if (fgets(buf, BUF_SIZE, stdin) == NULL) break;
            if (send(sockfd, buf, strlen(buf), 0) == -1)
                perror("send");
        }

        if (fds[1].revents & POLLIN) {  // server message
            int numbytes = recv(sockfd, buf, BUF_SIZE - 1, 0);
            if (numbytes <= 0) {
                if (numbytes == 0) printf("Server disconnected\n");
                else perror("recv");
                break;
            }
            buf[numbytes] = '\0';
            printf("%s", buf);
        }
    }

    close(sockfd);
    return 0;
}
