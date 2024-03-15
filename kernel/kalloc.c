// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU]; // each CPU has a lock and a freelist

char *cpu_lock_name[] = {
    "kmem_lock_0",
    "kmem_lock_1",
    "kmem_lock_2",
    "kmem_lock_3",
    "kmem_lock_4",
    "kmem_lock_5",
    "kmem_lock_6",
    "kmem_lock_7",
}; // lock name


void kinit()
{
  for (int i = 0; i < NCPU; i++)
  {
    initlock(&kmem[i].lock, cpu_lock_name[i]); // initialize locks
  }
  freerange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  push_off();

  r = (struct run *)pa;
  int cpu = cpuid();

  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  release(&kmem[cpu].lock); 

  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int cpu = cpuid();

  acquire(&kmem[cpu].lock);
  if (!kmem[cpu].freelist)
  {
    int steal_page_num = 64; // how many physical pages to steal
    for (int i = 0; i < NCPU; i++)
    {
      // don't steal when traverse the current CPU
      if (i == cpu)
      {
        continue;
      } 
      // ensure that we only hold one lock at anytime. To avoid dead locks
      if(holding(&kmem[cpu].lock))
        release(&kmem[cpu].lock);
      acquire(&kmem[i].lock);
      struct run *other_cpu_freepage = kmem[i].freelist;
      // stealing process
      while (other_cpu_freepage && steal_page_num)
      {
        kmem[i].freelist = other_cpu_freepage->next;
        release(&kmem[i].lock);
        acquire(&kmem[cpu].lock);
        other_cpu_freepage->next = kmem[cpu].freelist;
        kmem[cpu].freelist = other_cpu_freepage;
        release(&kmem[cpu].lock);
        acquire(&kmem[i].lock);
        other_cpu_freepage = kmem[i].freelist;
        steal_page_num--;
      } 
      release(&kmem[i].lock);
      // finish stealing, exit loop
      if (steal_page_num == 0)
      {
        break;
      }
    }
  }
  if(!holding(&kmem[cpu].lock))
    acquire(&kmem[cpu].lock);
  r = kmem[cpu].freelist;
  if (r)
    kmem[cpu].freelist = r->next;
  release(&kmem[cpu].lock);
  pop_off();

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}
