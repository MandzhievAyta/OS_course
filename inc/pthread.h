#ifndef JOS_INC_PTHREAD_H
#define JOS_INC_PTHREAD_H

typedef int32_t pthread_t;
struct pthread_attr_t {
  int pthread_type;
  int sched_policy;
  int priority;
};

enum { PTHREAD_CREATE_JOINABLE = 0,  PTHREAD_CREATE_DETACHED = 1,
       SCHED_RR = 2, SCHED_FIFO = 3, JOINABLE = 4, DETACHED = 5,
       JOINABLE_FINISHED = 6 };


enum { PTHREAD = 1, PROCESS = 0 };
enum { ERR_MAX_PTHREADS = -50 };
enum { MAX_PTHREADS = 5 };
//static struct Env *list_join_waiting = NULL;
#endif
