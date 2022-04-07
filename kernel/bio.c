// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13
#define BUCKETSIZE (MAXOPBLOCKS*3) // 这里我刚开始写的是 (NBUF/NBUCKETS + 1) 就是默认是均匀分布结果显然不对==，在usertests manywrites时会报"no buffers"。

struct bufBucket{
  struct spinlock lock;
  struct buf buf[BUCKETSIZE];
};

struct {
  struct bufBucket bufBucket[NBUCKETS];
  // 不整什么双向循环链表了，直接一个数组当作哈希表
} bcache;

void
binit(void)
{
  for (int i = 0;i < NBUCKETS;i++){
    initlock(&bcache.bufBucket[i].lock, "bufBucketlock");
    for (int j = 0;j < BUCKETSIZE;j++){
      // initsleeplock(&bcache.bufBucket[i].buf[j].lock, "bufBucketsleeplock");
      bcache.bufBucket[i].buf[j].refcnt = 0;
      bcache.bufBucket[i].buf[j].timestamps = ticks;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf* b;
  uint hashKey = blockno % NBUCKETS;
  
  acquire(&bcache.bufBucket[hashKey].lock);

  // Is the block already cached?
  for (int i = 0;i < BUCKETSIZE;i++){
    b = &bcache.bufBucket[hashKey].buf[i];
    if (b -> blockno == blockno && b -> dev == dev){
      b -> refcnt++;
      b -> timestamps = ticks;
      release(&bcache.bufBucket[hashKey].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  uint min_timestamps = __UINT32_MAX__;
  struct buf* min_b = 0;
  for (int i = 0;i < BUCKETSIZE;i++){
    b = &bcache.bufBucket[hashKey].buf[i];
    if (b->refcnt == 0 && b -> timestamps < min_timestamps) {
      min_b = b;
      min_timestamps = b -> timestamps;
    }
  }
  if (min_b){
    min_b->dev = dev;
    min_b->blockno = blockno;
    min_b->valid = 0;
    min_b->refcnt = 1;
    min_b -> timestamps = ticks;
    release(&bcache.bufBucket[hashKey].lock);
    acquiresleep(&min_b->lock);
    return min_b;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int hashKey = b -> blockno % NBUCKETS; 
  acquire(&bcache.bufBucket[hashKey].lock);
  b->refcnt--;
  release(&bcache.bufBucket[hashKey].lock);
}

void
bpin(struct buf *b) {
  int hashKey = b -> blockno % NBUCKETS; 
  acquire(&bcache.bufBucket[hashKey].lock);
  b->refcnt++;
  release(&bcache.bufBucket[hashKey].lock);
}

void
bunpin(struct buf *b) {
  int hashKey = b -> blockno % NBUCKETS; 
  acquire(&bcache.bufBucket[hashKey].lock);
  b->refcnt--;
  release(&bcache.bufBucket[hashKey].lock);
}


