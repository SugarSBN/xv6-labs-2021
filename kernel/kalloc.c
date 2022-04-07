/*
 * @Author: SuBonan
 * @Date: 2022-04-07 14:45:35
 * @LastEditTime: 2022-04-07 15:06:56
 * @FilePath: \xv6-labs-2021\kernel\kalloc.c
 * @Github: https://github.com/SugarSBN
 * これなに、これなに、これない、これなに、これなに、これなに、ねこ！ヾ(*´∀｀*)ﾉ
 */
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

struct {
  struct spinlock lock;
  struct run *freelist;
} kmems[NCPU];

void
kinit()
{
  for (int i = 0;i < NCPU;i++)
    initlock(&kmems[i].lock, "kmem");
  freerange(end, (void*)PHYSTOP);
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
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  int cpui = 0;
  push_off();
  cpui = cpuid();
  pop_off();
  acquire(&kmems[cpui].lock);
  r->next = kmems[cpui].freelist;
  kmems[cpui].freelist = r;
  release(&kmems[cpui].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int cpui = 0;
  push_off();
  cpui = cpuid();
  pop_off();
  
  acquire(&kmems[cpui].lock);
  r = kmems[cpui].freelist;
  if(r)
    kmems[cpui].freelist = r->next;
  else{
    for (int cpuj = 0;cpuj < NCPU;cpuj++) 
      if (kmems[cpuj].freelist){
        acquire(&kmems[cpuj].lock);
        r = kmems[cpuj].freelist;
        kmems[cpuj].freelist = r -> next;
        release(&kmems[cpuj].lock);
        break;
    }
  }

  release(&kmems[cpui].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
