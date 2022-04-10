## 关于mmap的实现

我觉得我的方法比市面上的方法要好（指百度

就是我的VMA地址是从$MAXVA-3*PGSIZE$往下分布

我选择了在PTE中加了一位PTE_M表示mmap的lazy allocation

然后在fork时候就比较简单

而且不需要在exit里写什么，只需要在freewalk时，发现PTE_V & PTE_M的PTE就不要panic了，直接free掉

总之就是好

但是我不知道make grade为啥读不到time.txt，我懒得调了（或许忘了sudo

我计算机真的好慢==