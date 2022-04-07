/*
 * @Author: SuBonan
 * @Date: 2022-04-07 14:37:53
 * @LastEditTime: 2022-04-07 16:28:57
 * @FilePath: \xv6-labs-2021\kernel\buf.h
 * @Github: https://github.com/SugarSBN
 * これなに、これなに、これない、これなに、これなに、これなに、ねこ！ヾ(*´∀｀*)ﾉ
 */
struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  uchar data[BSIZE];
  uint timestamps;
};

