
#include "queue.h"
#include "sched.h"
#include "timer.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t slot_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
#endif

int queue_empty(void) {
#ifdef MLQ_SCHED
    unsigned long prio;
    for (prio = 0; prio < MAX_PRIO; prio++)
        if (!empty(&mlq_ready_queue[prio]))
            return -1;

#endif
    return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i;

    for (i = 0; i < MAX_PRIO; i++)
        mlq_ready_queue[i].size = 0;
#endif
    ready_queue.size = 0;
    run_queue.size = 0;
    // pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/*
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */

struct pcb_t* get_mlq_proc(void) {
    struct pcb_t* proc = NULL;
    unsigned long prio;
    static unsigned long curr_slot = 0;  // static variable for stateful design

    static int curr_queue = 0;  // static variable for stateful design
    static int flag = 0;        // true=1 flase=0

    if (curr_slot == MAX_PRIO - curr_queue) {
        flag = 0;
        pthread_mutex_lock(&slot_lock);
        curr_slot = 0;
        pthread_mutex_unlock(&slot_lock);
    }
FIND_PROCESS:
    if (flag == 0) {
        pthread_mutex_lock(&slot_lock);
        curr_slot = 0;
        pthread_mutex_unlock(&slot_lock);
        if (curr_queue == 140) curr_queue = 0;
        for (prio = curr_queue; prio < MAX_PRIO; prio++) {
            if (!empty(&mlq_ready_queue[prio])) {
                curr_queue = prio;
                flag = 1;

                pthread_mutex_lock(&queue_lock);
                proc = dequeue(&mlq_ready_queue[prio]);
                pthread_mutex_unlock(&queue_lock);

                return proc;
            }
        }
        for (prio = 0; prio < curr_queue; prio++) {
            if (!empty(&mlq_ready_queue[prio])) {
                curr_queue = prio;
                flag = 1;

                pthread_mutex_lock(&queue_lock);
                proc = dequeue(&mlq_ready_queue[prio]);
                pthread_mutex_unlock(&queue_lock);

                return proc;
            }
        }
        return NULL;
    } else {
        pthread_mutex_lock(&slot_lock);
        curr_slot++;
        pthread_mutex_unlock(&slot_lock);
        if (!empty(&mlq_ready_queue[curr_queue])) {
            pthread_mutex_lock(&queue_lock);
            proc = dequeue(&mlq_ready_queue[curr_queue]);
            pthread_mutex_unlock(&queue_lock);
            return proc;
        } else {
            curr_queue++;
            flag = 0;
            goto FIND_PROCESS;
        }
    }
    return proc;
}

void put_mlq_proc(struct pcb_t* proc) {
    pthread_mutex_lock(&queue_lock);
    enqueue(&mlq_ready_queue[proc->prio], proc);
    pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t* proc) {
    pthread_mutex_lock(&queue_lock);
    enqueue(&mlq_ready_queue[proc->prio], proc);
    pthread_mutex_unlock(&queue_lock);
}

struct pcb_t* get_proc(void) {
    return get_mlq_proc();
}

void put_proc(struct pcb_t* proc) {
    return put_mlq_proc(proc);
}

void add_proc(struct pcb_t* proc) {
    return add_mlq_proc(proc);
}
#else
struct pcb_t* get_proc(void) {
    struct pcb_t* proc = NULL;
    /*TODO: get a process from [ready_queue].
     * Remember to use lock to protect the queue.
     * */
    pthread_mutex_lock(&queue_lock);
    if (!empty(&ready_queue)) {
        proc = dequeue(&ready_queue);
    }
    pthread_mutex_unlock(&queue_lock);
    return proc;
}

void put_proc(struct pcb_t* proc) {
    pthread_mutex_lock(&queue_lock);
    enqueue(&run_queue, proc);
    pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t* proc) {
    pthread_mutex_lock(&queue_lock);
    enqueue(&ready_queue, proc);
    pthread_mutex_unlock(&queue_lock);
}
#endif
