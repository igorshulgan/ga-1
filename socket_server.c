//
// Created by Ihar Shulhan on 20/11/2016.
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>

#include "priority_queue.c"
#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

void server_close_queue(int sock) {

    Heap *heap;
    int n = read(sock, heap, sizeof(heap));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    close_queue(heap);

}

void server_deque(int sock) {
    Heap *heap;
    int n = read(sock, heap, sizeof(heap));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    void *result = deque(heap);
    n = write(sock, result, sizeof(result));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    return;
}

void server_enqueue(int sock) {

    Heap *heap;
    int n = read(sock, heap, sizeof(heap));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    int priority;

    n = read(sock, priority, sizeof(priority));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    void *data;
    n = read(sock, data, sizeof(data));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    /* Now read server response */
    int result = enqueue(heap, priority, data);
    n = write(sock, result, sizeof(result));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    return;
}

void server_init_queue(int sock) {

    int size;
    int n = read(sock, size, sizeof(size));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    Heap *result = init_queue(size);
    n = write(sock, result, sizeof(result));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    return;

}

int sockfd, newsockfd, portno, clilen;

void doprocessing (int sock) {
    int n;
    const char *buffer[256];
    bzero(buffer,256);
    n = read(sock,buffer,255);

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    if (strcmp(buffer, "enqueue"))
        server_init_queue(sock);
    if (strcmp(buffer, "enqueue"))
        server_enqueue(sock);
    if (strcmp(buffer, "deque"))
        server_deque(sock);
    if (strcmp(buffer, "close_queue"))
        server_close_queue(sock);

    return;
};

void socket_server_start() {

    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int  n;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5002;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Now start listening for the clients, here process will
       * go in sleep mode and will wait for the incoming connection
    */

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        /* Create child process */
        int pid = fork();

        if (pid < 0) {
            perror("ERROR on fork");
            exit(1);
        }

        if (pid == 0) {
            /* This is the client process */
            doprocessing(newsockfd);
            exit(0);
        }
        else {
            close(newsockfd);
        }

    } /* end of while */

}

void stop(){
    close(sockfd);
}