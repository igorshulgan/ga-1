//
// Created by Ihar Shulhan on 20/11/2016.
//
#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>


int client_queue_size() {
    int sockfd = socket_client_connect();

    write(sockfd, "queue_size", strlen("queue_size"));

    int size = -1;
    int n = read(sockfd, &size, sizeof(int));
    if (n < 0) { perror("Error reading size from server\n"); }
    closeSocket(sockfd);
    return size;
}

int client_max_queue_size() {
    int sockfd = socket_client_connect();

    write(sockfd, "max_size", strlen("max_size"));
    int max_size = -1;
    int n = read(sockfd, &max_size, sizeof(int));
    if (n < 0) {
        perror("Error reading MAX_SIZE from server \n");
    }
    closeSocket(sockfd);
    return max_size;
}

void client_close_queue() {
    int sockfd = socket_client_connect();
    int n = write(sockfd, "close_queue", strlen("close_queue"));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    closeSocket(sockfd);
}

void *client_deque(Item *item) {
    int sockfd = socket_client_connect();

    int n = write(sockfd, "deque", strlen("deque"));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    Item dequed;
    n = read(sockfd, &dequed, sizeof(Item));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    item->id = dequed.id;
    item->priority = dequed.priority;
    item->consume_time = dequed.consume_time;
    item->produce_time = dequed.produce_time;
    printf("Item[%d] dequed succesfully(client)!\n", dequed.id);

    closeSocket(sockfd);
    return 0;
}

int client_enqueue(Item item) {

    int sockfd = socket_client_connect();

    int n = write(sockfd, "enqueue", strlen("enqueue"));

    const char *res[255];
    n = read(sockfd, &res, 255);

    n = write(sockfd, &item, sizeof(Item));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    closeSocket(sockfd);
    return 0;
}

int *client_init_queue(int size) {

    int sockfd = socket_client_connect();

    int n = write(sockfd, "init_queue", strlen("init_queue"));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    const char *res[255];
    n = read(sockfd, res, 255);

    n = write(sockfd, &size, sizeof(int));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }

    closeSocket(sockfd);
    return 0;

}

int socket_client_connect() {

    int sockfd, portno;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    portno = PORT_NUM;

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    server = gethostbyname("localhost");

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }

    return sockfd;
}
