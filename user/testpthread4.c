//test of sending the result
//JOIN after the target pthread is finished.
#include <inc/lib.h>
void *func1(void *a) {
  int *res;
  res = malloc(4);
  *res = 777;
  cprintf("I am POSIX thread! number [%08x] and I send value: %d\n", *(pthread_t*)a, *res);
  sys_pthread_exit((void*)res);
  return NULL;
}

void *func2(void *a) {
  int *res;
  cprintf("I am POSIX thread! And I join [%08x]\n", *(pthread_t*)a);
  cprintf("I'am joining\n");
  sys_pthread_join(*(pthread_t*)a, (void**)&res);
  cprintf("I'VE GOT RESULT OF FIRST PTHREAD: %d\n", *res);
  return NULL;
}
void
umain(int argc, char **argv)
{
  pthread_t t1, t2;
  sys_pthread_create(&t1, NULL, &func1, &t1);
  sys_pthread_create(&t2, NULL, &func2, &t1);
  sys_yield();
}
