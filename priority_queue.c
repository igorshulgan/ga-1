#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#define QUEUE_SIZE 100

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
            // printf("\nindex is %d, max_child is %d, size is %d, left_child is %d, right_child is %d\n", index, max_child, heap->size, left_child, right_child );
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
        printf("Blocked deque\n");
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
    printf("\nIT IS A HEAP\n");
    for (i = 0; i < heap->size; i++) {
        printf("%6d", heap->queue[i].priority);
    }
    printf("\n");
    for (i = 0; i < heap->size; i++) {
        printf("%6d", heap->queue[i].data);
    }
    pthread_mutex_unlock(&heap->lock_on_data);
}
