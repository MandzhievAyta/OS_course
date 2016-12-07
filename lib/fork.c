// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;addr=addr;
	uint32_t err = utf->utf_err;err=err;
  int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 9: Your code here.
  if (!(err & FEC_WR && uvpt[PGNUM(addr)] & PTE_COW)) {
    panic("pgfault addr=%p, err=%d, pte=%x", addr, err, uvpt[PGNUM(addr)]);
  }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 9: Your code here.
  if ((r = sys_page_alloc(0, PFTEMP, PTE_W | PTE_U)) < 0) {
    panic("pgfault error %d", r);
  }
  memcpy(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);
  if ((r = sys_page_map(0, PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_U | PTE_W)) < 0) {
    panic("pgfault error %d", r);
  }
  sys_page_unmap(0, PFTEMP);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
  int r;
  void *addr = (void *) (pn * PGSIZE);


	// LAB 9: Your code here.
  if (uvpt[pn] & PTE_SHARE) {
    r = sys_page_map(0, addr, envid, addr, uvpt[pn] & PTE_SYSCALL);
    return r;
  }

  int cow = uvpt[pn] & PTE_COW || uvpt[pn] & PTE_W;
  r = sys_page_map(0, addr, envid, addr, (cow ? PTE_COW : 0) | PTE_U);
  if (!r && cow) {
    r = sys_page_map(0, addr, 0, addr, PTE_COW | PTE_U);
  }
  return r;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 9: Your code here.
  set_pgfault_handler(pgfault);
  envid_t child;
  if (!(child = sys_exofork())) {
    set_pgfault_handler(pgfault);
    thisenv = &envs[ENVX(sys_getenvid())];
    return 0;
  }
  int r;
  if (child > 0) {
    size_t i;
    for (i = 0; i < PGSIZE / sizeof(pde_t); i++) {
      if (!(uvpd[i] & PTE_P)) {
        continue;
      }
      size_t j;
      for (j = 0; j < PGSIZE / sizeof(pte_t); ++j) {
        size_t pgnum = PGNUM(PGADDR(i, j, 0));
        if (pgnum >= PGNUM(UTOP) || pgnum == PGNUM(UXSTACKTOP - PGSIZE) || !(uvpt[pgnum] & PTE_P)) {
          continue;
        }
        if ((r = duppage(child, pgnum)) < 0) {
          return r;
        }
      }
    }

    if ((r = sys_env_set_pgfault_upcall(child, thisenv->env_pgfault_upcall)) < 0
      || (r = sys_page_alloc(child, (void*) UXSTACKTOP - PGSIZE, PTE_W | PTE_U)) < 0
      || (r = sys_env_set_status(child, ENV_RUNNABLE)) < 0) {
      return r;
    }
  }
  return child;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
