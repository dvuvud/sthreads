/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
   following define.
   */
#define _XOPEN_SOURCE 700

/* On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
   flag must also be used to suppress compiler warnings.
   */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <stdbool.h> 
#include <sys/time.h>
#include "sthreads.h"

/* -- constants -- */

#define STACK_SIZE    (SIGSTKSZ * 100) // stack size for each context
#define TIME_SLICE_US 100              // preemption interval

/* -- global state -- */

static thread_t *current          = NULL;
static thread_t *ready_queue      = NULL;
static thread_t *terminated_queue = NULL;
static thread_t *all_threads      = NULL;
static tid_t     next_tid         = 0;

/* -- ready queue -- */

static void enqueue(thread_t *t) {
    t->next = NULL;
    if (!ready_queue) {
        ready_queue = t;
        return;
    }

    thread_t *tail = ready_queue;
    while (tail->next) tail = tail->next;
    tail->next = t;

}

static thread_t *dequeue() {
    if (!ready_queue) return NULL;
    thread_t *t  = ready_queue;
    ready_queue  = ready_queue->next;
    t->next      = NULL;
    return t;
}

/* -- thread lifecycle helpers -- */

static void remove_from_all(thread_t *t) {
    for (thread_t **p = &all_threads; *p; p = &(*p)->all_next) {
        if (*p == t) {
            *p = t->all_next;
            return;
        }
    }
}

static void flush_terminated() {
    thread_t *t = terminated_queue;
    while (t) {
        thread_t *next = t->next;
        remove_from_all(t);
        free(t->stack);
        free(t);
        t = next;
    }
    terminated_queue = NULL;
}

static void mark_terminated(thread_t *t) {
    if (!t) return;
    flush_terminated();          // clean up previous batch first
    if (t->stack) {
        t->next = terminated_queue;
        terminated_queue = t;
    }
    else {  // the initial thread
        free(t);
    }
}

static thread_t *find_by_tid(tid_t tid) {
    for (thread_t *t = all_threads; t; t = t->all_next)
        if (t->tid == tid) return t;
    return NULL;
}

/* -- scheduler -- */

static void schedule() {
    block_sigalrm();

    thread_t *prev = current;

    if      (prev->state == running)    { prev->state = ready; enqueue(prev); }
    else if (prev->state == terminated) { mark_terminated(prev); }

    thread_t *next = dequeue();
    if (!next) exit(0);  // no threads left — we're done

    next->state = running;
    current     = next;

    swapcontext(&prev->ctx, &next->ctx);

    // we resume here when a thread is switched back to
    unblock_sigalrm();
}

/* -- timer & preemption -- */

static void timer_handler(int _sig) { (void)_sig; schedule(); }

static void set_timer() {
    /** setup signal handling **/
    struct sigaction sa = {0};
    sa.sa_handler = timer_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGALRM, &sa, NULL);

    /** set timer **/
    struct itimerval tv = {
        .it_value    = { .tv_sec = 0, .tv_usec = TIME_SLICE_US },
        .it_interval = { .tv_sec = 0, .tv_usec = TIME_SLICE_US },
    };
    if (setitimer(ITIMER_REAL, &tv, NULL) < 0) { perror("setitimer"); exit(1); }
}

static void thread_wrapper() {
    // any new thread must start off with preemption blocked to make sure the `swapcontext` call in schedule is safe
    unblock_sigalrm();  
    current->start();
    done();
}

/* -- API -- */

int init() {
    ready_queue = all_threads = NULL;

    thread_t *t = calloc(1, sizeof(thread_t));
    if (!t) return -1;

    getcontext(&t->ctx);
    t->state    = running;
    t->tid      = next_tid++;
    t->all_next = all_threads;
    all_threads = t;
    current     = t;

    set_timer();
    return 1;
}

tid_t spawn(void (*start)()) {
    // we must block signals when handling shared resources (malloc&enqueue)
    block_sigalrm();

    thread_t *t     = malloc(sizeof(thread_t));
    void     *stack = malloc(STACK_SIZE);
    if (!t || !stack) {
        free(t);
        free(stack);
        unblock_sigalrm();
        return -1;
    }

    getcontext(&t->ctx);
    t->ctx.uc_stack.ss_sp    = stack;
    t->ctx.uc_stack.ss_size  = STACK_SIZE;
    t->ctx.uc_stack.ss_flags = 0;
    t->ctx.uc_link           = NULL;

    makecontext(&t->ctx, thread_wrapper, 0);

    t->stack     = stack;
    t->start     = start;
    t->state     = ready;
    t->tid       = next_tid++;
    t->next      = NULL;
    t->join_next = NULL;
    t->all_next  = all_threads;
    all_threads  = t;

    enqueue(t);

    unblock_sigalrm();
    return t->tid;
}

void yield() {
    schedule();
}

void done() {
    block_sigalrm(); // block preemption while mid-done

    current->state = terminated;

    // wake every thread that was join()-ing on us.
    for (thread_t *w = current->join_next; w; ) {
        thread_t *next = w->join_next;
        w->state = ready;
        enqueue(w);
        w = next;
    }
    current->join_next = NULL;

    schedule();
}

tid_t join(tid_t tid) {
    block_sigalrm();

    thread_t *target = find_by_tid(tid);

    if (!target)                     { unblock_sigalrm(); return -1; }   // thread doesn't exist
    if (!ready_queue)                { unblock_sigalrm(); return -1; }   // joining would deadlock
    if (target->state == terminated) { unblock_sigalrm(); return tid; }

    current->state     = waiting;
    current->join_next = target->join_next;
    target->join_next  = current;

    schedule();
    return tid;
}

void block_sigalrm() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

void unblock_sigalrm() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}
