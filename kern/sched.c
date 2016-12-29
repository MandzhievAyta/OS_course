#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/env.h>
#include <kern/monitor.h>
#include <kern/sched.h>
#include <kern/kclock.h>


struct Taskstate cpu_ts;
void sched_halt(void);
void find_and_run(void);

struct Env *heads[MAX_PRIORITY];

void add_in_tail(struct Env *pthread, int remain_time)
{
//  cprintf("Adding in tail [%08x] with prior: %d", pthread->env_id, pthread->priority);
  struct Env **cur = &(heads[pthread->priority]);
  while (*cur)
    cur = &((**cur).next_sched_queue);
//  cprintf("I AM HERE\n");
  *cur = pthread;
  pthread->next_sched_queue = NULL;
  if (remain_time != -1)
    pthread->remain_time = remain_time;
}

void add_in_head(struct Env *pthread, int remain_time)
{
  pthread->next_sched_queue = heads[pthread->priority];
  heads[pthread->priority] = pthread;
  if (remain_time != -1)
    pthread->remain_time = remain_time;
}

void delete_from_queue(struct Env *pthread)
{
  struct Env **p = &(heads[pthread->priority]);
  while (*p) {
    if ((**p).env_id == pthread->env_id) {
      struct Env *tmp;
      tmp = *p;
      *p = (**p).next_sched_queue;
      tmp->next_sched_queue = NULL;
    } else {
      p = &((**p).next_sched_queue);
    }
  }
}

void check_init_process(void)
{
  struct Env **cur = &(heads[1]);
  while (*cur) {
    cprintf("CHECKING WHETHER [%08x] is INIT\n", (**cur).env_id);
    if (((**cur).env_id == (pthread_t)0x1000) && ((**cur).env_status == ENV_RUNNABLE)) {
      cprintf("YEEEAAHHH INIT PROCESS I FOUND IT!!\n");
      delete_from_queue(*cur);
      env_run(*cur);
    } else {
      cur = &((**cur).next_sched_queue);
    }
  }
}

//seeking for pthread with max priority
void find_and_run(void)
{
  int i;
//  check_init_process();
  for (i = MAX_PRIORITY - 1; i >= MIN_PRIORITY; i--) {
    struct Env *tmp;
    while (heads[i] != NULL) {
      tmp = heads[i];
      heads[i] = tmp->next_sched_queue;
      if (tmp->env_status == ENV_RUNNABLE) {
        cprintf("SCHDULER: running [%08x] from %d priority queue\n", tmp->env_id, i);
        if (tmp->remain_time == 0 && tmp->priority != 1)
          tmp->remain_time = QUANTUM;
        tmp->remain_time += gettime();
        tmp->next_sched_queue = NULL;
        env_run(tmp);
      } else {
        cprintf("&&&&& %p '%d'\n", tmp, i);
        tmp->next_sched_queue = NULL;
      }
    }
  }
}

int check_in_queue(struct Env *env)
{
  struct Env **cur = &(heads[env->priority]);
  while (*cur) {
    if ((**cur).env_id == env->env_id)
      return 1;
    cur = &((**cur).next_sched_queue);
  }
  return 0;
}

void print_queues(void)
{
  size_t i;
  for (i = 0; i < MAX_PRIORITY; i++) {
    struct Env *tmp = heads[i];
    if (tmp != NULL) {
      cprintf("========\nPriority %d:\n", i);
    }
    while (tmp) {
      cprintf("[%08x] %s \n", tmp->env_id,
      tmp->env_status == ENV_RUNNABLE?"ENV_RUNNABLE":tmp->env_status == ENV_RUNNING?"ENV_RUNNING":
      tmp->env_status == ENV_FREE?"ENV_FREE":tmp->env_status == ENV_NOT_RUNNABLE?"ENV_NOT_RUNNABLE":"someting ELSE!");
      tmp = tmp->next_sched_queue;
    }
    if (heads[i] != NULL) {
      cprintf("========\n");
    }
  }
}

void sched_yield_from_clock(void)
{
  add_in_head(curenv, -1);
  sched_yield();
}

void sched_yield(void)
{
//  cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
//  print_queues();
//  cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

  if (curenv != NULL) {
    cprintf("CURENV [%08x]", curenv->env_id);
    int time_left = curenv->remain_time - gettime();
    if (time_left <= 0 && curenv->priority != 1) {
      cprintf("+!+!+!+!+!+!+ env [%08x] spent his quantum\n", curenv->env_id);
      delete_from_queue(curenv);
      add_in_tail(curenv, 0);
      if (curenv->env_status == ENV_RUNNING)
        curenv->env_status = ENV_RUNNABLE;
    } else {
      if (!check_in_queue(curenv)) {
        add_in_tail(curenv, time_left);
      } else {
        delete_from_queue(curenv);
        add_in_head(curenv, time_left);
      }
      if (curenv->env_status == ENV_RUNNING)
        curenv->env_status = ENV_RUNNABLE;
    }
  }
  find_and_run();

  sched_halt();
  for(;;) {}
}
/*
// Choose a user environment to run and run it.
void
sched_yield(void)
{
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// If there are no runnable environments,
	// simply drop through to the code
	// below to halt the cpu.

	// LAB 3: Your code here.
  size_t i = curenv ? (curenv - envs + 1) & (NENV - 1) : 0;
  size_t count = 0;

  for (; count++ < NENV; i = (i + 1) & (NENV - 1)) {
    if (envs[i].env_status == ENV_RUNNABLE) {
      //cprintf("envrun RUNNABLE %d\n", ENVX(envs[i].env_id));
      env_run(&envs[i]);
    }
  }

  if (curenv && curenv->env_status == ENV_RUNNING) {
    //cprintf("envrun RUNNING %d\n", ENVX(curenv->env_id));
    env_run(curenv);
  }
	// sched_halt never returns
	sched_halt();
}
*/
// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;
    for(i = 0; i < NENV; ++i) {
        if (envs[i].env_status == ENV_NOT_RUNNABLE) {
            //envs[i].env_status = ENV_RUNNABLE;
            //sched_yield();
        }
    }

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on CPU
	curenv = NULL;

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		"sti\n"
		"hlt\n"
	: : "a" (cpu_ts.ts_esp0));
}

