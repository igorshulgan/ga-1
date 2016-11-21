#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>
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
#include <netdb.h>
#include <netinet/in.h>


#define ERROR_CREATE_THREAD -11
#define ERROR_JOIN_THREAD   -12
#define QUEUE_SIZE 100

#define QUEUE_SIZE 100

void closeSocket(int fd) {      // *not* the Windows closesocket()
    if (fd >= 0) {

        if (shutdown(fd, SHUT_RDWR) < 0) // secondly, terminate the 'reliable' delivery
        if (errno != ENOTCONN && errno != EINVAL) // SGI causes EINVAL
            perror("shutdown");
        if (close(fd) < 0) // finally call close()
            perror("close");
    }
}

typedef struct Item {
    int id;
    int consume_time;
    int produce_time;
    int priority;
} Item;

int getParent(int index) {
    assert(index > 0);
    return (index - 1) / 2;
}

int getLeftChild(int index) {
    assert(index >= 0);
    return (index * 2) + 1;
}

int getRightChild(int index) {
    assert(index >= 0);
    return (index * 2) + 2;
}

typedef struct {
    int priority;
    void *data;
} Priority_node;

typedef struct {
    Priority_node* queue;
    int size;
    int MAX_SIZE;
    pthread_mutex_t lock_on_data;
    pthread_mutex_t lock_on_enqueue;
    pthread_mutex_t lock_on_deque;
} Heap;

Heap *init_queue(int size) {
    Heap *new_heap = malloc(sizeof(Heap));
    new_heap->queue = malloc(sizeof(Priority_node)*size);
    new_heap->MAX_SIZE = size;
    new_heap->size = 0;
    pthread_mutex_lock(&new_heap->lock_on_enqueue);
    pthread_mutex_lock(&new_heap->lock_on_deque);
    return new_heap;

}

void close_queue(Heap *heap) {
    free(heap->queue);
    free(heap);
}


int enqueue(Heap *heap, int priority, void *data) {
    if (heap->size == heap->MAX_SIZE) {
        pthread_mutex_lock(&heap->lock_on_enqueue);
    }
    pthread_mutex_lock(&heap->lock_on_data);
    int index = heap->size++;
    Priority_node *queue = heap->queue;
    queue[index].priority = priority;
    queue[index].data = data;
    while (index > 0) {
        int parent = getParent(index);
        if (queue[index].priority > queue[parent].priority) {
            Priority_node temp_node = queue[index];
            queue[index] = queue[parent];
            queue[parent] = temp_node;
            index = parent;
        } else {
            pthread_mutex_unlock(&heap->lock_on_deque);
            pthread_mutex_unlock(&heap->lock_on_data);
            return 0;
        }
    }
    pthread_mutex_unlock(&heap->lock_on_deque);
    pthread_mutex_unlock(&heap->lock_on_data);
    return 0;
}

int delete_max(Heap *heap) {
    // if(heap->size == 0){
    //  pthread_mutex_lock(&heap->lock_on_deque);
    // }
    // if(heap->size == 1){
    //  heap->size = 0;
    //  pthread_mutex_unlock(&heap->lock_on_data);
    //  return 0;
    // }
    Priority_node *queue = heap->queue;
    int index = 0, size = heap->size;
    queue[index] = queue[--size];
    while (index < size) {
        int right_child = getRightChild(index);
        int left_child = getLeftChild(index);
        if (right_child > size) {
            right_child = -1;
        }
        if (left_child > size) {
            left_child = -1;
        }
        int max_child;
        if ((right_child != -1) && (left_child != -1)) {
            if (queue[right_child].priority > queue[left_child].priority) {
                max_child = right_child;
            } else {
                max_child = left_child;
            }
        } else {
            if (right_child == left_child) {
                heap->size = size;
                // pthread_mutex_unlock(&heap->lock_on_enqueue);
                // pthread_mutex_unlock(&heap->lock_on_data);
                return 0;
            } else {
                if (right_child == -1) {
                    max_child = left_child;
                } else {
                    max_child = right_child;
                }
            }
        }
        if (queue[index].priority < queue[max_child].priority) {
            // print_heap(heap);
            // // printf("\nindex is %d, max_child is %d, size is %d, left_child is %d, right_child is %d\n", index, max_child, heap->size, left_child, right_child );
            Priority_node temp_node = queue[index];
            queue[index] = queue[max_child];
            queue[max_child] = temp_node;
            index = max_child;
        } else {
            heap->size = size;
            // pthread_mutex_unlock(&heap->lock_on_enqueue);
            // pthread_mutex_unlock(&heap->lock_on_data);
            return 0;
        }
    }
    // pthread_mutex_unlock(&heap->lock_on_enqueue);
    // pthread_mutex_unlock(&heap->lock_on_data);
    heap->size = size;
    return 0;
}

void *getMax(Heap *heap) {
    assert(heap->size != 0);
    void *result = heap->queue[0].data;
    return result;
}

void *deque(Heap *heap) {
    if (heap->size == 0) {
        // printf("Blocked deque\n");
        pthread_mutex_lock(&heap->lock_on_deque);
    }
    pthread_mutex_lock(&heap->lock_on_data);
    void *result = getMax(heap);
    delete_max(heap);
    pthread_mutex_unlock(&heap->lock_on_enqueue);
    pthread_mutex_unlock(&heap->lock_on_data);
    return result;
}

void print_heap(Heap *heap) {
    int i;
    pthread_mutex_lock(&heap->lock_on_data);
    // printf("\nIT IS A HEAP\n");
    for (i = 0; i < heap->size; i++) {
        // printf("%6d", heap->queue[i].priority);
    }
    // printf("\n");
    for (i = 0; i < heap->size; i++) {
        // printf("%6d", heap->queue[i].data);
    }
    pthread_mutex_unlock(&heap->lock_on_data);
}

void print_item(Item *item) {
    // printf("Item[%d] with priority %d, c_time: %d, p_time: %d\n", item->id, item->priority, item->consume_time, item->produce_time);
}

#define PORT_NUM 5018
Heap *heap;

void server_size(int sock) {
    write(sock, &(heap->size), sizeof(int));
}

void server_max_size(int sock) {
    write(sock, &heap->MAX_SIZE, sizeof(int));
}

void server_deque(int sock) {

    Item *temp1 = (Item *) malloc(sizeof(Item));
    temp1 = deque(heap);

    // printf("Server Dequeued ");
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

    // printf("Server enqueing ");
    print_item(temp);

    enqueue(heap, temp->priority, temp);

    return;
}

void server_init_queue(int sock) {

    int size;
    int n = read(sock, &size, sizeof(size));
    // printf("size %d\n", size);

    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }

    heap = init_queue(size);

    // printf("Heap created. Heap size: %d\n", heap->size);

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

void socket_server_start(sem_t* semvar) {


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


    // printf("Server created\n");
    sem_post(semvar);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        int res = doprocessing(newsockfd);
        closeSocket(newsockfd);
        if (!res) {
            closeSocket(sockfd);
            exit(0);
        }

    } /* end of while */

}

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
    // printf("Item[%d] dequed succesfully(client)!\n", dequed.id);

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

int COMMUNICATION_TYPE = 0;

int count;
int items_num;
int queue_size;
Item *items;
Heap *queue;
pthread_t consumer_t, producer_t, clien_t, server_t;
pthread_mutex_t mutex;
sem_t semvar;
pthread_cond_t condc, condp;


void *producer(void *arg) {
    // printf("Producer started\n");
    for (count = 0; count < items_num; count++) {

        pthread_mutex_lock(&mutex);    /* protect buffer */
        // printf("Working with buffer in producer %d %d\n", client_queue_size(), 5);
        while (client_queue_size() ==
               client_max_queue_size()) {               /* If there is something in the buffer then wait */
            pthread_cond_wait(&condp, &mutex);
        }
        // printf("\n\nProducer started!\n");
        /*Sleep item producer milliseconds and enque items[count] to priority queue
        */
        // printf("Producer try to enqueue ");
        print_item(&items[count]);
        // printf("New size is  %d\n", client_queue_size());
        usleep(items[count].produce_time * 1000);

        client_enqueue(items[count]);


        pthread_cond_signal(&condc);    /* wake up consumer */
        pthread_mutex_unlock(&mutex);    /* release the buffer */
    }
    pthread_exit(0);
}

void *consumer(void *arg) {
    int i;
    FILE *file = fopen("results", "w+");
    // printf("Consumer started\n");
    for (i = 1; i <= items_num; i++) {
        pthread_mutex_lock(&mutex);    /* protect buffer */
        while (client_queue_size() == 0) {        /* If there is nothing in the buffer then wait */
            pthread_cond_wait(&condc, &mutex);
        }
        /*sleep and dequeue item*/
        Item dequed;
        client_deque(&dequed);


        // printf("Consumer ready to consume!. Queue size = %d \n", client_queue_size());
        // printf("Dequeed item[%d] with  priority %d. New size: %d \n", dequed.id, dequed.priority, client_queue_size());
        usleep(dequed.consume_time * 1000);
        fprintf(file, "%d %d %d %d\n", dequed.id, dequed.produce_time, dequed.consume_time, dequed.priority);
        pthread_cond_signal(&condp);    /* wake up consumer */
        pthread_mutex_unlock(&mutex);    /* release the buffer */
    }
    fclose(file);
    pthread_exit(0);
}

void *shmem_producer(void *arg) {
    // printf("Producer started\n");
    // printf("producer - loh1\n");
    for (count = 0; count < items_num; count++) {
        // printf("producer - loh2\n");
        pthread_mutex_lock(&mutex);    /* protect buffer */
        // printf("%d %d\n", queue->size, queue->MAX_SIZE);
        while (queue->size == queue->MAX_SIZE) {               /* If there is something in the buffer then wait */
            pthread_cond_wait(&condp, &mutex);
        }
        // printf("producer - loh3\n");
        /*Sleep item producer milliseconds and enque items[count] to priority queue
        */
        usleep(items[count].produce_time * 1000);
        enqueue(queue, items[count].priority, &items[count]);

        // printf("Producer produced item[%d] with priority %d. New size: %d \n", items[count].id, items[count].priority,queue->size);

        pthread_cond_signal(&condc);    /* wake up consumer */
        pthread_mutex_unlock(&mutex);    /* release the buffer */
    }
    pthread_exit(0);
}

void *shmem_consumer(void *arg) {
    int i;
    FILE *file = fopen("results", "w+");
    // printf("Consumer started\n");
    for (i = 1; i <= items_num; i++) {
        pthread_mutex_lock(&mutex);    /* protect buffer */
        while (queue->size == 0) {        /* If there is nothing in the buffer then wait */
            pthread_cond_wait(&condc, &mutex);
        }
        /*sleep and dequeue item*/
        Item *dequed;
        dequed = deque(queue);

        // printf("Consumer ready to consume!. Queue size = %d \n", queue->size);
        // printf("Dequeed item[%d] with  priority %d. New size: %d \n", dequed->id, dequed->priority, queue->size);
        usleep(dequed->consume_time * 1000);
        fprintf(file, "%d %d %d %d\n", dequed->id, dequed->produce_time, dequed->consume_time, dequed->priority);
        pthread_cond_signal(&condp);    /* wake up consumer */
        pthread_mutex_unlock(&mutex);    /* release the buffer */
    }
    fclose(file);
    pthread_exit(0);
}

int main(int argc, char *argv[]) {

    int status;

    int i;
    items_num = countLines("items") - 1;
    // printf("Number of items: %d\n", items_num);
    items = calloc(items_num, sizeof(Item));
    FILE *file = fopen("items", "r");
    int id, priority, c_time, p_time;
    for (i = 0; i < items_num; i++) {
        fscanf(file, "%d %d %d %d", &id, &p_time, &c_time, &priority);
        items[i].id = id;
        items[i].priority = -priority;
        items[i].consume_time = c_time;
        items[i].produce_time = p_time;
    }
    fclose(file);
//    for (i = 0; i < items_num; i++) {
  //      // printf("Item [%d]: %d\n", items[i].id, items[i].priority);
  //  }
    if (strcmp(argv[2], "shmem")) {
        COMMUNICATION_TYPE = 1;
        sem_init(&semvar,1,1);
        sem_wait(&semvar);
        status = pthread_create(&server_t, NULL, socket_server_start, &semvar);
        if (status != 0) {
            // printf("main error: can't create socker server thread, status = %d\n", status);
            exit(ERROR_CREATE_THREAD);
        }
        sem_wait(&semvar);
    }
    else {
        COMMUNICATION_TYPE = 0;
    }
    queue_size = atoi(argv[1]);


    if (COMMUNICATION_TYPE) {
        queue = client_init_queue(queue_size);
        // printf("Queue size after creation %d\n", client_queue_size());
    } else {
        queue = init_queue(queue_size);
        // printf("Queue after creation %d",queue->size);
    }



    /*Initialize queue
    Start producer and consumer*/
    if (COMMUNICATION_TYPE){
        status = pthread_create(&producer_t, NULL, producer, NULL);
    } else {
        status = pthread_create(&producer_t, NULL, shmem_producer, NULL);
    }
    if (status != 0) {
        // printf("main error: can't create producer thread, status = %d\n", status);
        exit(ERROR_CREATE_THREAD);
    }
    if (COMMUNICATION_TYPE){
        status = pthread_create(&consumer_t, NULL, consumer, NULL);
    } else {
        status = pthread_create(&consumer_t, NULL, shmem_consumer, NULL);
    }
    if (status != 0) {
        // printf("main error: can't create consumer thread, status = %d\n", status);
        exit(ERROR_CREATE_THREAD);
    }
    status = pthread_join(consumer_t, NULL);
    if (status != 0) {
        // printf("main error: can't join consumer thread, status = %d\n", status);
        exit(ERROR_JOIN_THREAD);
    }
    status = pthread_join(producer_t, NULL);
    if (status != 0) {
        // printf("main error: can't join producer thread, status = %d\n", status);
        exit(ERROR_JOIN_THREAD);
    }

    //STOP_SERVER
    if (COMMUNICATION_TYPE) {
        int sock = socket_client_connect();
        write(sock, "stop", strlen("stop"));
        closeSocket(sock);
    } else {
        close_queue(queue);
    }

    return EXIT_SUCCESS;
}

void writeItem(FILE *file, struct Item item) {
    fprintf(file, "%d %d %d %d", &(item.id), &item.produce_time, &item.consume_time, &item.priority);
    //printf("Write: Item [%d] %d %d %d\n",item.id,item.produce_time,item.consume_time,item.priority);
}

int countLines(char *filename) {
    int lines = 0;
    char ch;
    FILE *file = fopen(filename, "r");
    while (!feof(file)) {
        fscanf(file, "%c", &ch);
        if (ch == '\n') {
            lines++;
        }
    }
    fclose(file);
    return lines;
}

