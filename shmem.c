#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>
#include "socket_server.c"
#include "socket_client.c"

#define ERROR_CREATE_THREAD -11
#define ERROR_JOIN_THREAD   -12
#define QUEUE_SIZE 100

typedef struct Item {
    int id;
    int consume_time;
    int produce_time;
    int priority;
} Item;

int COMMUNICATION_TYPE = 0;

int count;
int items_num;
int queue_size;
Item *items;
Heap *queue;
pthread_t consumer_t, producer_t, clien_t, server_t;
pthread_mutex_t mutex, mutex2;
pthread_cond_t condc, condp;


void *producer(void *arg) {
    printf("Producer started\n");
    printf("producer - loh1\n");
    for (count = 0; count < items_num; count++) {
        printf("producer - loh2\n");
        pthread_mutex_lock(&mutex);    /* protect buffer */
        printf("%d %d\n", queue->size, queue->MAX_SIZE);
        while (queue->size == queue->MAX_SIZE) {               /* If there is something in the buffer then wait */
            pthread_cond_wait(&condp, &mutex);
        }
        printf("producer - loh3\n");
        /*Sleep item producer milliseconds and enque items[count] to priority queue
        */
        usleep(items[count].produce_time * 1000);
        if (COMMUNICATION_TYPE) {
            client_enqueue(queue, items[count].priority, &items[count]);
        }
        else
        {
            enqueue(queue, items[count].priority, &items[count]);
        }
        printf("Producer produced item[%d] with priority %d. New size: %d \n", items[count].id, items[count].priority,
               queue->size);

        pthread_cond_signal(&condc);    /* wake up consumer */
        pthread_mutex_unlock(&mutex);    /* release the buffer */
    }
    pthread_exit(0);
}

void *consumer(void *arg) {
    int i;
    FILE *file = fopen("results", "w+");
    printf("Consumer started\n");
    for (i = 1; i <= items_num; i++) {
        pthread_mutex_lock(&mutex);    /* protect buffer */
        while (queue->size == 0) {        /* If there is nothing in the buffer then wait */
            pthread_cond_wait(&condc, &mutex);
        }
        /*sleep and dequeue item*/
        Item *dequed;
        if (COMMUNICATION_TYPE) {
            dequed = client_deque(queue);
        } else {
            dequed = deque(queue);
        }

        printf("Consumer ready to consume!. Queue size = %d \n", queue->size);
        printf("Dequeed item[%d] with  priority %d. New size: %d \n", dequed->id, dequed->priority, queue->size);
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
    printf("Number of items: %d\n", items_num);
    items = calloc(items_num, sizeof(Item));
    FILE *file = fopen("items", "r");
    int id, priority, c_time, p_time;
    for (i = 0; i < items_num; i++) {
        fscanf(file, "%d %d %d %d", &id, &p_time, &c_time, &priority);
        items[i].id = id;
        items[i].priority = priority;
        items[i].consume_time = c_time;
        items[i].produce_time = p_time;
    }
    fclose(file);
    for (i = 0; i < items_num; i++) {
        printf("Item [%d]: %d\n", items[i].id, items[i].priority);
    }
    queue_size = atoi(argv[1]);
    if (strcmp(argv[2], "shmem")) {
        COMMUNICATION_TYPE = 1;
        pthread_mutex_lock(&mutex2);
        status = pthread_create(&server_t, NULL, socket_server_start, &mutex2);
        if (status != 0) {
            printf("main error: can't create socker server thread, status = %d\n", status);
            exit(ERROR_CREATE_THREAD);
        }
        pthread_mutex_lock(&mutex2);
    }
    else {
        COMMUNICATION_TYPE = 0;
    }
    if (COMMUNICATION_TYPE) {
        queue = client_init_queue(queue_size);
        printf("Queue after creation %d",queue->size);
    } else {
        queue = init_queue(queue_size);
    }


    /*Initialize queue
    Start producer and consumer*/
    status = pthread_create(&producer_t, NULL, producer, NULL);
    if (status != 0) {
        printf("main error: can't create producer thread, status = %d\n", status);
        exit(ERROR_CREATE_THREAD);
    }

    status = pthread_create(&consumer_t, NULL, consumer, NULL);
    if (status != 0) {
        printf("main error: can't create consumer thread, status = %d\n", status);
        exit(ERROR_CREATE_THREAD);
    }
    status = pthread_join(consumer_t, NULL);
    if (status != 0) {
        printf("main error: can't join consumer thread, status = %d\n", status);
        exit(ERROR_JOIN_THREAD);
    }
    status = pthread_join(producer_t, NULL);
    if (status != 0) {
        printf("main error: can't join producer thread, status = %d\n", status);
        exit(ERROR_JOIN_THREAD);
    }
    pthread_join(server_t, NULL);

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

