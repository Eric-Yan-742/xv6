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

#define NBUCKET 13
#define BUCKETSIZE 8

extern uint ticks;
struct {
  struct spinlock lock;
  struct buf buf[BUCKETSIZE];
} buckets[NBUCKET];

uint hash(uint blockno)
{
  return blockno % NBUCKET;
}

void
binit(void)
{
  for(int i = 0; i < NBUCKET; i++) {
    initlock(&buckets[i].lock, "bcache");
    for(int j = 0; j < BUCKETSIZE; j++) {
      initsleeplock(&buckets[i].buf[j].lock, "buffer");
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint bucketId = hash(blockno);
  acquire(&buckets[bucketId].lock);

  // Is the block already cached in this bucket?
  for(int i = 0; i < BUCKETSIZE; i++){
    b = &buckets[bucketId].buf[i];
    if(b->dev == dev && b->blockno == blockno){
      b->lastUse = ticks; // update the use time
      b->refcnt++;
      release(&buckets[bucketId].lock);

      acquiresleep(&b->lock);
      return b;
    }
  }

  uint leastTime = 0xffffffff;
  int leastIdx = -1;
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int i = 0; i < BUCKETSIZE; i++){
    b = &buckets[bucketId].buf[i];
    if(b->refcnt == 0 && b->lastUse < leastTime) {
      leastTime = b->lastUse;
      leastIdx = i;
    }
  }
  if(leastIdx == -1)
    panic("bget: no buffers can recycle");
  b = &buckets[bucketId].buf[leastIdx];
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->lastUse = ticks; // update the use time
  release(&buckets[bucketId].lock);
  acquiresleep(&b->lock);
  return b;
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

  uint bucketId = hash(b->blockno);
  acquire(&buckets[bucketId].lock);
  b->refcnt--;
  release(&buckets[bucketId].lock);
}

void
bpin(struct buf *b) {
  uint bucketId = hash(b->blockno);
  acquire(&buckets[bucketId].lock);
  b->refcnt++;
  release(&buckets[bucketId].lock);
}

void
bunpin(struct buf *b) {
  uint bucketId = hash(b->blockno);
  acquire(&buckets[bucketId].lock);
  b->refcnt--;
  release(&buckets[bucketId].lock);
}


