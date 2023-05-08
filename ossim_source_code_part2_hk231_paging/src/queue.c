#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

int empty(struct queue_t* q) {
    if (q == NULL) return 1;
    return (q->size == 0);
}

void enqueue(struct queue_t* q, struct pcb_t* proc) {
    /* TODO: put a new process to queue [q] */

    if (q == NULL || proc == NULL || q->size >= MAX_QUEUE_SIZE) {
        // printf("Error: queue is full or invalid arguments\n");
        return;
    }

    if (q->size == 0) {
        q->size = 1;
        q->proc[0] = proc;
    } else {
        // find the index to insert the new process based on priority
        int index = q->size;
        for (int i = 0; i < q->size; i++) {
            if (proc->priority < q->proc[i]->priority) {
                index = i;
                break;
            }
        }

        // shift the elements to the right to make room for the new process
        for (int i = q->size - 1; i >= index; i--) {
            q->proc[i + 1] = q->proc[i];
        }

        // insert the new process
        q->proc[index] = proc;
        q->size++;
    }
}

struct pcb_t* dequeue(struct queue_t* q) {
    /* TODO: return a pcb whose prioprity is the highest
     * in the queue [q] and remember to remove it from q
     * */

    if (q == NULL || q->size == 0) {
        // printf("Error: queue is empty or invalid arguments\n");
        return NULL;
    }

    // find the process with highest priority
    struct pcb_t* highest_proc = q->proc[0];
    int index = 0;
    // free(q->proc[0]);

    // shift the elements to the left to remove the process
    for (int i = index; i < q->size - 1; i++) {
        q->proc[i] = q->proc[i + 1];
    }
    q->proc[q->size - 1] = NULL;
    q->size--;
    return highest_proc;

    // return NULL;
}