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

#define PORT_NUM 5018
Heap *heap;


typedef struct Item {
    int id;
    int consume_time;
    int produce_time;
    int priority;
} Item;

void print_item(Item *item) {
    printf("Item[%d] with priority %d, c_time: %d, p_time: %d\n", item->id, item->priority, item->consume_time,
           item->produce_time);
}

void server_size(int sock) {
    write(sock, &(heap->size), sizeof(int));
}

void server_max_size(int sock) {
    write(sock, &heap->MAX_SIZE, sizeof(int));
}

void server_deque(int sock) {

    Item *temp1 = (Item *) malloc(sizeof(Item));
    temp1 = deque(heap);

    printf("Server Dequeued ");
    print_item(temp1);

    int n = write(sock, temp1, sizeof(Item));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    return;
}

void server_enqueue(int sock) {
    Item *temp = (Item *) malloc(sizeof(Item));

    int n = read(sock, temp, sizeof(Item));

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    printf("Server enqueing ");
    print_item(temp);

    enqueue(heap, temp->priority, temp);

    return;
}

void server_init_queue(int sock) {

    int size;
    int n = read(sock, &size, sizeof(size));
    printf("size %d\n", size);

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    heap = init_queue(size);

    printf("Heap created. Heap size: %d\n", heap->size);

    return;
}

int sockfd, newsockfd, portno, clilen;

int doprocessing(int sock) {
    int n;
    const char *buffer[256];
    bzero(buffer, 256);
    n = read(sock, buffer, 255);

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    //printf("Do processing called: %s \n", buffer);

    if (strcmp(buffer, "init_queue") == 0) {
        write(sock, "ready", 5);
        server_init_queue(sock);
    }
    if (strcmp(buffer, "enqueue") == 0) {
        write(sock, "ready", 5);
        server_enqueue(sock);
    }
    if (strcmp(buffer, "deque") == 0) {
        server_deque(sock);
    }
    if (strcmp(buffer, "queue_size") == 0) {
        server_size(sock);
    }
    if (strcmp(buffer, "close_queue") == 0) {
        close_queue(heap);
    }
    if (strcmp(buffer, "max_size") == 0) {
        server_max_size(sock);
    }
    if (strcmp(buffer, "stop") == 0) {
        return 0;
    }

    return 1;
};

void socket_server_start(pthread_mutex_t mutex) {


    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = PORT_NUM;

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

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);


    printf("Server created\n");
    pthread_mutex_unlock(&mutex);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        int res = doprocessing(newsockfd);
        close(newsockfd);
        if (!res) {
            close(sockfd);
            return;
        }

    } /* end of while */

}