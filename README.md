# Sthreads

Sthreads is a lightweight user-level threading library that implements
**preemptive scheduling** using a **many-to-one threading model**. In this model,
multiple user-level threads are multiplexed onto a single operating system
thread.

The scheduler uses a timer-driven preemption mechanism to periodically
interrupt the running thread and switch execution to another ready thread.
This allows threads to share CPU time without requiring explicit yielding.

The library provides a small API for creating and managing threads,
including functionality for:

- Initializing the threading system
- Spawning new threads
- Yielding execution to the scheduler
- Terminating threads
- Waiting for threads to finish (join)
- Temporarily disabling preemption for critical sections

All API functions and their behavior are documented in `sthreads.h`.
