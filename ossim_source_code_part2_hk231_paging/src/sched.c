
#include "queue.h"
#include "sched.h"
#include "timer.h"


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// static long timeOfQueue = 0;
// static long positionOfQueue = 0;

static long curr_slot = 0;
struct queue_t;

static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

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
    pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/*
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t *get_mlq_proc(void) {
    struct pcb_t *proc = NULL;
    /*TODO: get a process from PRIORITY [ready_queue].
     * Remember to use lock to protect the queue.
     * */
    unsigned long prio;

    /* loop through the MLQ ready queues according to the policy */
    #ifdef SYNC
    pthread_mutex_lock(&queue_lock);
    #endif
    for (prio = 0; prio < MAX_PRIO; prio++) {
        if (!empty(&mlq_ready_queue[prio])) {    // found a non-empty queue
            if (curr_slot == MAX_PRIO - prio) {  // used up current slot
                /* move to the next queue and reset the slot counter */
                curr_slot = 0;
            } else {
                /* get the process from the current queue and return */
                proc = dequeue(&mlq_ready_queue[prio]);
                curr_slot++;
                break;
            }
        }
    }
    #ifdef SYNC
    pthread_mutex_unlock(&queue_lock);
    #endif

    return proc;
}

void put_mlq_proc(struct pcb_t *proc) {
    #ifdef SYNC
    pthread_mutex_lock(&queue_lock);
    #endif
    enqueue(&mlq_ready_queue[proc->prio], proc);
    #ifdef SYNC
    pthread_mutex_unlock(&queue_lock);
    #endif
}

void add_mlq_proc(struct pcb_t *proc) {
    #ifdef SYNC
    pthread_mutex_lock(&queue_lock);
    #endif
    enqueue(&mlq_ready_queue[proc->prio], proc);
    #ifdef SYNC
    pthread_mutex_unlock(&queue_lock);
    #endif
}

struct pcb_t *get_proc(void) {
    return get_mlq_proc();
}

void put_proc(struct pcb_t *proc) {
    return put_mlq_proc(proc);
}

void add_proc(struct pcb_t *proc) {
    return add_mlq_proc(proc);
}
#else
struct pcb_t* get_proc(void) {
    struct pcb_t* proc = NULL;
    /*TODO: get a process from [ready_queue].
     * Remember to use lock to protect the queue.
     * */
    #ifdef SYNC
    pthread_mutex_lock(&queue_lock);
    #endif
    if (!empty(&ready_queue)) {
        proc = dequeue(&ready_queue);
    }
    #ifdef SYNC
    pthread_mutex_unlock(&queue_lock);
    #endif
    return proc;
}

void put_proc(struct pcb_t* proc) {
    #ifdef SYNC
    pthread_mutex_lock(&queue_lock);
    #endif
    enqueue(&run_queue, proc);
    #ifdef SYNC
    pthread_mutex_unlock(&queue_lock);
    #endif
}

void add_proc(struct pcb_t* proc) {
    #ifdef SYNC
    pthread_mutex_lock(&queue_lock);
    #endif
    enqueue(&ready_queue, proc);
    #ifdef SYNC
    pthread_mutex_unlock(&queue_lock);
    #endif
}
#endif
