/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_SCHED_H
#define JOS_KERN_SCHED_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

enum { MIN_PRIORITY = 0, MAX_PRIORITY = 20, QUANTUM = 1000 };
void delete_from_queue(struct Env *pthread);
void add_in_head(struct Env *pthread, int remain_time);
void add_in_tail(struct Env *pthread, int remain_time);

// This function does not return.
void sched_yield(void) __attribute__((noreturn));

#endif	// !JOS_KERN_SCHED_H
