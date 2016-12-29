//test of sending the result
//JOIN before the target pthread is finished.
#include <inc/lib.h>
void *func1(void *a) {
  int *res;
  res = malloc(4);
  *res = 777;
  cprintf("1: I am POSIX thread! number [%08x] and I send value: %d\n", *(pthread_t*)a, *res);
  return (void*)res;
}

void *func2(void *a) {
  int *res;
  cprintf("2: I am POSIX thread! And I join [%08x]\n", *(pthread_t*)a);
  cprintf("2: I'am joining\n");
  sys_pthread_join(*(pthread_t*)a, (void**)&res);
  cprintf("2: I'VE GOT RESULT OF FIRST PTHREAD: %d\n", *res);
  return NULL;
}
void
umain(int argc, char **argv)
{
  pthread_t t1, t2;
  sys_sched_setscheduler(0, SCHED_FIFO, 1);
  sys_print_pthread_state(0);
  sys_pthread_create(&t1, NULL, &func1, &t1);
  sys_pthread_create(&t2, NULL, &func2, &t1);

  cprintf("0: -----------------1\n");
  sys_print_pthread_state(t1);
  sys_print_pthread_state(t2);
  cprintf("0: -----------------2\n");
  sys_sched_setscheduler(t2, SCHED_FIFO, 4);
  cprintf("0: -----------------3\n");
  sys_print_pthread_state(t1);
  sys_print_pthread_state(t2);
  cprintf("0: -----------------4\n");

  sys_yield();
  sys_yield();
}
