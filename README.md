写一点我调试中踩得坑。

```c
pa = PTE2PA(*pte);
//*pte &= ~PTE_W;
//*pte |= PTE_COW;
flags = PTE_FLAGS(*pte);

flags &= ~PTE_W;			// 这两行在犯罪
flags |= PTW_COW;			// 这两行在犯罪

acquire(&ref_cnt_lock);
ref_cnt[pa >> 12] += 1;
release(&ref_cnt_lock);
/*if((mem = kalloc()) == 0)
      goto err;
    memmove(mem, (char*)pa, PGSIZE);*/

if(mappages(new, i, PGSIZE, (uint64)pa, flags) != 0){
    //kfree(mem);
    goto err;
}
```

刚开始在uvmcopy和copyout时，我都这么写的复制逻辑，显然这是错的应该删除犯罪的两行，然后取消注释前面两行。

因为父子进程的pagetable指向同一块physical address时，不仅子进程的flags需要取消PTE_W权限加上PTE_COW，父进程同样也需要。如果按照刚开始写，父进程有写入权的话，会导致physical address被污染==。

-----

其次就是FAILED --- some free pages are lost的问题

```c
int ref_cnt[PHYSTOP >> 12];
struct spinlock ref_cnt_lock;
```

刚开始脑子一抽觉得physical pages不会很多，就把ref_cnt开的只有40000。后来发现我是小猪。