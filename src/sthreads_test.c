#include <stdlib.h>   // exit(), EXIT_FAILURE, EXIT_SUCCESS
#include <stdio.h>    // printf(), fprintf(), stdout, stderr, perror(), _IOLBF
#include <stdbool.h>  // true, false
#include <limits.h>   // INT_MAX

#include "sthreads.h" // init(), spawn(), yield(), done()

/*******************************************************************************
  Functions to be used together with spawn()

  You may add your own functions or change these functions to your liking.
 ********************************************************************************/

/* Prints the sequence 0, 1, 2, .... INT_MAX over and over again.
*/
void numbers() {
    int n = 0;
    while (true) {
        block_sigalrm();
        printf(" n = %d\n", n);
        unblock_sigalrm();
        n = (n + 1) % (INT_MAX);
        if (n > 3) done();
        yield();
    }
}

/* Prints the sequence a, b, c, ..., z over and over again.
*/
void letters() {
    char c = 'a';

    while (true) {
        block_sigalrm();
        printf(" c = %c\n", c);
        unblock_sigalrm();
        if (c == 'f') done();
        yield();
        c = (c == 'z') ? 'a' : c + 1;
    }
}

/* Calculates the nth Fibonacci number using recursion.
*/
int fib(int n) {
    switch (n) {
        case 0:
            return 0;
        case 1:
            return 1;
        default:
            return fib(n-1) + fib(n-2);
    }
}

/* Print the Fibonacci number sequence over and over again.

https://en.wikipedia.org/wiki/Fibonacci_number

This is deliberately an unnecessary slow and CPU intensive
implementation where each number in the sequence is calculated recursively
from scratch.
*/

void fibonacci_slow() {
    int n = 0;
    int f;
    while (true) {
        f = fib(n);
        if (f < 0) {
            // Restart on overflow.
            n = 0;
        }
        block_sigalrm();
        printf(" fib(%02d) = %d\n", n, fib(n));
        unblock_sigalrm();
        n = (n + 1) % INT_MAX;
    }
}

/* Print the Fibonacci number sequence over and over again.

https://en.wikipedia.org/wiki/Fibonacci_number

This implementation is much faster than fibonacci().
*/
void fibonacci_fast() {
    int a = 0;
    int b = 1;
    int n = 0;
    int next = a + b;

    while(true) {
        block_sigalrm();
        printf(" fib(%02d) = %d\n", n, a);
        unblock_sigalrm();
        next = a + b;
        a = b;
        b = next;
        n++;
        if (a < 0) {
            // Restart on overflow.
            a = 0;
            b = 1;
            n = 0;
        }
    }
}

/* Prints the sequence of magic constants over and over again.

https://en.wikipedia.org/wiki/Magic_square
*/
void magic_numbers() {
    int n = 3;
    int m;
    while (true) {
        m = (n*(n*n+1)/2);
        if (m > 0) {
            block_sigalrm();
            printf(" magic(%d) = %d\n", n, m);
            unblock_sigalrm();
            n = (n+1) % INT_MAX;
        } else {
            // Start over when m overflows.
            n = 3;
        }
    }
}

/*******************************************************************************
  main()

  Here you should add code to test the Simple Threads API.
 ********************************************************************************/

int main(){
    puts("\n==== Test program for the Simple Threads API ====\n");

    init(); // Initialization

    tid_t t1 = spawn(numbers);
    tid_t t2 = spawn(letters);

    join(t1);
    join(t2);

    tid_t t3 = spawn(magic_numbers);
    tid_t t4 = spawn(fibonacci_fast);

    join(t3);
    join(t4);

    puts("\n==== Finished executing all thread!! ====\n");

    done();

    return 0;
}
