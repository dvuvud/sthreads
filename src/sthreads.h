#ifndef STHREADS_H
#define STHREADS_H

#define _XOPEN_SOURCE 700

#include <ucontext.h>

typedef enum { running, ready, waiting, terminated } state_t;

typedef int tid_t;

typedef struct thread thread_t;

struct thread {
    tid_t tid;
    state_t state;
    ucontext_t ctx;
    void *stack;
    void (*start)();        // store the start function pointer to allow for wrapping
    thread_t *next;         // ready queue linkage
    thread_t *all_next;     // global thread list linkage
    thread_t *join_next;    // join waiters
};

/* Initialization

   Initialization of global thread management data structures. A user program
   must call this function exactly once before calling any other functions in
   the Simple Threads API.

   Returns 1 on success and a negative value on failure.
*/
int init();

/* Creates a new thread executing the start function.

   start - a function with zero arguments returning void.

   On success the positive thread ID of the new thread is returned. On failure a
   negative value is returned.
*/
tid_t spawn(void (*start)());

/* Cooperative scheduling

   If there are other threads in the ready state, a thread calling yield() will
   trigger the thread scheduler to dispatch one of the threads in the ready
   state and change the state of the calling thread from running to ready.
*/
void yield();

/* Thread termination

   A thread calling done() will change state from running to terminated. If
   there are other threads waiting to join, these threads will change state from
   waiting to ready. Next, the thread scheduler will dispatch one of the ready
   threads.
*/
void done();

/* Join with a terminated thread

   The join() function waits for the specified thread to terminate.
   If the specified thread has already terminated, join() should return
   immediately.

   A thread calling join(thread) will be suspended and change state from running
   to waiting and trigger the thread scheduler to dispatch one of the ready
   threads. The suspended thread will change state from waiting to ready once
   the thread with thread id thread calls done and join() should then return the
   thread id of the terminated thread.
*/
tid_t join(tid_t thread);


/**
 * The following two functions are required to allow the use of non-reentrant
 * functions inside the threading library.
 *
 * Below is an explanation of why this is necessary.
 *
 * During the development of the library we encountered a problem where threads
 * could be preempted while executing functions that are not reentrant. A
 * non-reentrant function cannot safely be interrupted and then entered again
 * by another thread before the first invocation has completed.
 *
 * If such a preemption occurs, multiple threads may end up executing the same
 * function simultaneously while sharing internal state, which can lead to
 * undefined behavior, corrupted data, or program crashes.
 *
 * The only reliable solution to this problem is to ensure that threads are not
 * preempted while executing code that must not be interrupted.
 *
 * Therefore, the following two functions are used to temporarily disable the
 * SIGALRM signal (which drives the preemptive scheduler) when entering
 * critical sections or functions that must not be interrupted. Once the
 * critical section has finished, SIGALRM can be re-enabled.
 */
void block_sigalrm();
void unblock_sigalrm();

#endif
