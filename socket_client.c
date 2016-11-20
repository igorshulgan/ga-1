//
// Created by Ihar Shulhan on 20/11/2016.
//
#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

int sockfd, portno;

void client_close_queue(Heap *heap) {
    int n = write(sockfd, "close_queue", strlen("close_queue"));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    n = write(sockfd, heap, strlen(heap));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

}

void *client_deque(Heap *heap) {
    int n = write(sockfd, "deque", strlen("deque"));

    n = write(sockfd, heap, sizeof(heap));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    void *result;
    n = read(sockfd, result, sizeof(result));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    return result;
}

int client_enqueue(Heap *heap, int priority, void *data) {

    int n = write(sockfd, "enqueue", strlen("enqueue"));

    n = write(sockfd, heap, sizeof(heap));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    n = write(sockfd, priority, sizeof(priority));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    n = write(sockfd, data, sizeof(data));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    /* Now read server response */
    int result;
    n = read(sockfd, result, sizeof(result));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    return result;
}
Heap *client_init_queue(int size) {

    int n = write(sockfd, "init_queue", strlen("init_queue"));

    n = write(sockfd, size, sizeof(size));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    Heap *result;
    n = read(sockfd, result, sizeof(result));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    return result;

}

void socket_client_connect() {

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    portno = 5002;

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    server = gethostbyname("localhost");

    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }
}
