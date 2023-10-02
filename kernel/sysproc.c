#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  unsigned int abits = 0;
  struct proc* p = myproc();
  pagetable_t pagetable = p->pagetable;
  uint64 va; // virtual address of the first user page to check
  if(argaddr(0, &va) < 0) {
    return -1;
  }
  uint64 a = PGROUNDDOWN(va); // in case va is not page aligned
  int num; // number of user pages to check
  if(argint(1, &num) < 0) {
    return -1;
  }
  uint64 ua; // user address to store the bit mask
  if(argaddr(2, &ua) < 0) {
    return -1;
  }

  for(int i = 0; i < num; i++) {
    pte_t *pte = walk(pagetable, a, 0);
    if(*pte & PTE_A) {
      abits |= (1L << i); // set the bit of bitmask to be 1
      *pte = *pte & (~PTE_A); // set PTE_A from 1 to 0 (clear it)
    }
    a += PGSIZE;
  }
  if(copyout(pagetable, ua, (char*)&abits, sizeof(abits)) < 0) {
    return -1;
  }
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
