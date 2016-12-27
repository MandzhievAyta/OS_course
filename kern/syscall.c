/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/pthread.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/kclock.h>
static struct Env *list_join_waiting = NULL;

void
print_trapframe1(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  cr2  0x%08x\n", rcr2());
	cprintf("  err  0x%08x", tf->tf_err);
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	cprintf("  esp  0x%08x\n", tf->tf_esp);
	cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 8: Your code here.
  user_mem_assert(curenv, s, len, PTE_U);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 9: Your code here.
  int result;
  struct Env *e;

  if ((result = env_alloc(&e, curenv->env_id, PROCESS, NULL)) < 0) {
    return result;
  }
  e->env_status = ENV_NOT_RUNNABLE;
  memcpy(&e->env_tf, &curenv->env_tf, sizeof(e->env_tf));
  e->env_tf.tf_regs.reg_eax = 0;
  return e->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 9: Your code here.
  int result;
  struct Env *e;
  if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE) {
    return -E_INVAL;
  }
  if ((result = envid2env(envid, &e, true)) < 0) {
    return result;
  }
  e->env_status = status;
  return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 11: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
  user_mem_assert(curenv, tf, sizeof(*tf), 0);

  struct Env *env;
  int r;
  if ((r = envid2env(envid, &env, true)) < 0) {
    return r;
  }

  tf->tf_eflags |= FL_IF;
  tf->tf_es = GD_UD | 3;
  tf->tf_ds = GD_UD | 3;
  tf->tf_ss = GD_UD | 3;
  tf->tf_cs = GD_UT | 3;

  env->env_tf = *tf;
  return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 9: Your code here.
  int result;
  struct Env *env;
  if ((result = envid2env(envid, &env, true)) < 0) {
    return result;
  }
  env->env_pgfault_upcall = func;
  return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 9: Your code here.
  struct Env *e;
  int result;
  if ((result = envid2env(envid, &e, true)) < 0) {
    return result;
  }
  if ((uintptr_t) va >= UTOP || ROUNDDOWN(va, PGSIZE) != va || perm & ~PTE_SYSCALL) {
    return -E_INVAL;
  }
  struct PageInfo *pp;
  if (!(pp = page_alloc(ALLOC_ZERO))) {
    return -E_NO_MEM;
  }
  if ((result = page_insert(e->env_pgdir, pp, va, perm)) < 0) {
    page_free(pp);
    return result;
  }
  return 0;
}

static int
_page_map(envid_t srcenvid, void *srcva,
          envid_t dstenvid, void *dstva, int perm, bool check_env_perm)
{
  int result;
  struct Env *srcenv, *dstenv;
  if ((result = envid2env(srcenvid, &srcenv, check_env_perm)) < 0 ||
      (result = envid2env(dstenvid, &dstenv, check_env_perm)) < 0) {
    return result;
  }

  if ((uintptr_t) srcva >= UTOP || ROUNDDOWN(srcva, PGSIZE) != srcva ||
      (uintptr_t) dstva >= UTOP || ROUNDDOWN(dstva, PGSIZE) != dstva ||
      perm & ~PTE_SYSCALL) {
    return -E_INVAL;
  }

  pte_t *srcpte;
  struct PageInfo *pp;
  if (!(pp = page_lookup(srcenv->env_pgdir, srcva, &srcpte))) {
    return -E_INVAL;
  }

  if (perm & PTE_W && !(*srcpte & PTE_W)) {
    return -E_INVAL;
  }

  return page_insert(dstenv->env_pgdir, pp, dstva, perm);
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 9: Your code here.
  return _page_map(srcenvid, srcva, dstenvid, dstva, perm, 1);
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 9: Your code here.
  int result;
  struct Env *env;

  if ((result = envid2env(envid, &env, 1)) < 0) {
    return result;
  }
  if ((uintptr_t) va >= UTOP || ROUNDDOWN(va, PGSIZE) != va) {
    return -E_INVAL;
  }
  page_remove(env->env_pgdir, va);
  return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 9: Your code here.
  struct Env *target;
  int r;
  if ((r = envid2env(envid, &target, false)) < 0) {
    return r;
  }
  if (!target->env_ipc_recving) {
    return -E_IPC_NOT_RECV;
  }
  bool is_transfer = (uintptr_t) srcva < UTOP && (uintptr_t) target->env_ipc_dstva < UTOP;
  if (is_transfer) {
    if ((r = _page_map(0, srcva, envid, target->env_ipc_dstva, perm, false)) < 0) {
      return r;
    }
  }
  target->env_ipc_recving = false;
  target->env_ipc_from = curenv->env_id;
  target->env_ipc_value = value;
  target->env_ipc_perm = is_transfer ? perm : 0;
  target->env_status = ENV_RUNNABLE;
  return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 9: Your code here.
  if ((uintptr_t) dstva < UTOP && ROUNDDOWN(dstva, PGSIZE) != dstva) {
    return -E_INVAL;
  }
  curenv->env_ipc_dstva = (uintptr_t) dstva < UTOP ? dstva : 0;
  curenv->env_ipc_recving = true;
  curenv->env_status = ENV_NOT_RUNNABLE;
  return 0;
}

// Return date and time in UNIX timestamp format: seconds passed
// from 1970-01-01 00:00:00 UTC.
static int
sys_gettime(void)
{
	// LAB 12: Your code here.
  return gettime();
}

static int
sys_pthread_exit(void *res, uint32_t res_hidden)
{
  if ((uint32_t)res == 1) {
    res = (void*)res_hidden;
  }
  if (res != NULL)
    cprintf("ADRESS RESULT OF EXIT: %p, RESULT:%d\n", res, *(int*)res);
  else
    cprintf("RESULT OF EXIT: NULL\n");
  curenv->res = res;
  if (curenv->pthread_type == JOINABLE) {
    struct Env **prev = &list_join_waiting;
    struct Env **cur;
    int was_found = 0;
    if (list_join_waiting == NULL) {
      env_free(curenv);
      sys_yield();
      return 0;
    }
    cur = &((*list_join_waiting).next_join_waiting);
    while (*cur) {
      if ((**prev).waitfor == curenv->env_id) {
        (*(**prev).putres) = res;
        (**prev).env_status = ENV_RUNNABLE;
        (**prev).next_join_waiting = NULL;
        was_found = 1;
        *prev = *cur;
        cur = &((**prev).next_join_waiting);
        continue;
      }
      prev = &((**prev).next_join_waiting);
      cur = &((**cur).next_join_waiting);
    }
    if ((**prev).waitfor == curenv->env_id) {
      (*(**prev).putres) = res;
      (**prev).env_status = ENV_RUNNABLE;
      (**prev).next_join_waiting = NULL;
      was_found = 1;
      *prev = NULL;
    }
    if (was_found) {
      env_free(curenv);
      env_free(curenv);
      sys_yield();
      return 0;
    } else {
      env_free(curenv);
      sys_yield();
      return 0;
    }
  }
  env_free(curenv);
  sys_yield();
  return 0;
}


static int
sys_pthread_create(uint32_t exit_adress, pthread_t *thread, const struct pthread_attr_t *attr, void *(*start_routine)(void*), uint32_t arg)
{
  struct Env *newenv;
  cprintf("PTHREAD_CREATE\n");
//  cprintf("CURRENT_ADRESS %p\n", (void*)curenv->env_tf.tf_eip);
//  print_trapframe1(&(curenv->env_tf));
//  cprintf("SYSTEM PTHREAD_EXIT ADRESS%p\n", (void*)exit_adress);
  env_alloc(&newenv, curenv->env_id, PTHREAD, curenv);
  newenv->env_tf.tf_eip = (uintptr_t) start_routine;
  if (attr == NULL) {
    newenv->priority = 1;
    newenv->sched_policy = SCHED_RR;
    newenv->pthread_type = JOINABLE;
  } else {
    newenv->priority = attr->priority;
    newenv->sched_policy = attr->sched_policy;
    if (attr->pthread_type == PTHREAD_CREATE_JOINABLE)
      newenv->pthread_type = JOINABLE;
    else
      newenv->pthread_type = DETACHED;
  }
  (*thread) = newenv->env_id;
  uint32_t *curframe;

  curframe = (uint32_t*)newenv->env_tf.tf_esp - 3;
  curframe[0] = exit_adress;
  curframe[1] = arg;
  curframe[2] = 1;//(uint32_t)((uint32_t*)newenv->env_tf.tf_esp);
//  cprintf("!!%p!!\n", (void*)curframe[1]);
  newenv->env_tf.tf_esp = (uintptr_t)((uint32_t*)(newenv->env_tf.tf_esp) - 3);
//  cprintf("!!%p!!", (void*)newenv->env_tf.tf_esp);
  return 0;
}

static int
sys_pthread_join(pthread_t thread, void **value_ptr)
{
  size_t  i;
  struct Env *target = NULL;
  cprintf("PTHREAD_JOIN [%08x]\n", thread);
  for (i = 0; i < NENV; i++) {
    if ((envs[i].env_status != ENV_FREE) &&
      (envs[i].is_pthread == PTHREAD) &&
      (envs[i].parent_proc == curenv->parent_proc) &&
      (envs[i].pthread_type != DETACHED) &&
      (envs[i].env_id == thread))
    {
      target = &(envs[i]);
    }
  }
  if (target == NULL) {
    return -1;
  } else {
    if (target->pthread_type == JOINABLE_FINISHED) {
      *value_ptr = target->res;
      env_free(target);
      return 0;
    } else {
      if (list_join_waiting == NULL) {
        cprintf("I AM FIRST IN LIST OF WAITING\n");
        list_join_waiting = &(*curenv);
        curenv->env_status = ENV_NOT_RUNNABLE;
        curenv->waitfor = target->env_id;
        curenv->putres = value_ptr;
        sys_yield();
      } else {
        struct Env *tmp = list_join_waiting;
        list_join_waiting = &(*curenv);
        curenv->next_join_waiting = tmp;
        curenv->env_status = ENV_NOT_RUNNABLE;
        curenv->waitfor = target->env_id;
        curenv->putres = value_ptr;
        sys_yield();
      }
    }
  }
  return 0;
}

static int
sys_sched_setparam(void)
{
  cprintf("SCHED_SETPARAM\n");
  return 0;
}

static int
sys_sched_setscheduler(void)
{
  cprintf("SCHED_SETSCHEDULER\n");
  return 0;
}

static int
sys_print_pthread_state(pthread_t id)
{
  size_t i;
  for (i = 0; i < NENV; i++) {
    if ((envs[i].env_status != ENV_FREE) && (envs[i].env_id == id)) {
      cprintf("Printing state of %s [%08x]:\n", (envs[i].is_pthread == PTHREAD)?"Pthread":"Process", id);
      if (envs[i].is_pthread == PTHREAD) {
        cprintf("parent id: [%08x]; pthread type: ", envs[i].parent_proc->env_id);
        if (envs[i].pthread_type == JOINABLE) {
          cprintf("JOINABLE; ");
        } else if(envs[i].pthread_type == DETACHED) {
          cprintf("DETACHED; ");
        } else {
          cprintf("JOINABLE_FINISHED ");
        }
        if (envs[i].pthread_type == JOINABLE_FINISHED) {
          cprintf("\n\tpointer of result: %p", envs[i].res);
        }
      } else {
        cprintf("Amount of Pthread which were created by this process: %d; ", envs[i].amnt_gen_pthreads);
      }
      cprintf("sched policy: %s;\n\tpriority: %d;\n", (envs[i].sched_policy == SCHED_RR)?"RR":"FIFO", envs[i].priority);
      return 0;
    }
  }
  cprintf("Environment [%08x] does not exist\n", id);
  return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 8: Your code here.
  switch (syscallno) {
    case SYS_yield:
      sys_yield();
      panic("sys_yield returned");
      break;
    case SYS_cputs:
      sys_cputs((const char *) a1, (size_t) a2);
      return 0;
    case SYS_cgetc:
      return sys_cgetc();
    case SYS_getenvid:
      return sys_getenvid();
    case SYS_env_destroy:
      return sys_env_destroy((envid_t) a1);
    case SYS_exofork:
      return sys_exofork();
    case SYS_page_alloc:
      return sys_page_alloc((envid_t) a1, (void *) a2, (int) a3);
    case SYS_page_map:
      return sys_page_map((envid_t) a1, (void *) a2, (envid_t) a3, (void *) a4, (int) a5);
    case SYS_page_unmap:
      return sys_page_unmap((envid_t) a1, (void *) a2);
    case SYS_env_set_status:
      return sys_env_set_status((envid_t) a1, (int) a2);
    case SYS_env_set_pgfault_upcall:
      return sys_env_set_pgfault_upcall((envid_t) a1, (void*) a2);
    case SYS_ipc_recv:
      return sys_ipc_recv((void *) a1);
    case SYS_ipc_try_send:
      return sys_ipc_try_send((envid_t) a1, (uint32_t) a2, (void *) a3, (unsigned) a4);
    case SYS_env_set_trapframe:
      return sys_env_set_trapframe((envid_t) a1, (struct Trapframe *) a2);
    case SYS_gettime:
      return sys_gettime();
    case SYS_pthreadcreate:
      return sys_pthread_create(a1, (pthread_t*)a2, (const struct pthread_attr_t*)a3, (void*(*)(void*)) a4, a5);
    case SYS_pthreadjoin:
      return sys_pthread_join((pthread_t)a1, (void**)a2);
    case SYS_pthreadexit:
      return sys_pthread_exit((void*)a1, a2);
    case SYS_schedsetparam:
      return sys_sched_setparam();
    case SYS_setscheduler:
      return sys_sched_setscheduler();
    case SYS_printpthreadstate:
      return sys_print_pthread_state((pthread_t)a1);
  }
  return -E_INVAL;
}

