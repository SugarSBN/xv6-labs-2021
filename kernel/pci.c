/*
 * @Author: SuBonan
 * @Date: 2022-04-04 15:41:39
 * @LastEditTime: 2022-04-04 19:01:13
 * @FilePath: \xv6-labs-2021\kernel\pci.c
 * @Github: https://github.com/SugarSBN
 * これなに、これなに、これない、これなに、これなに、これなに、ねこ！ヾ(*´∀｀*)ﾉ
 */
//
// simple PCI-Express initialization, only
// works for qemu and its e1000 card.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

void
pci_init()
{
  // we'll place the e1000 registers at this address.
  // vm.c maps this range.
  uint64 e1000_regs = 0x40000000L;

  // qemu -machine virt puts PCIe config space here.
  // vm.c maps this range.
  uint32  *ecam = (uint32 *) 0x30000000L;
  
  // look at each possible PCI device on bus 0.
  for(int dev = 0; dev < 32; dev++){
    int bus = 0;
    int func = 0;
    int offset = 0;
    // PCI Function Address (PFA):
    // | 31 (Enable bit) | 30~24 (Reserved bits) | 23~16 (Bus number) | 15~11 (Device number) | 10~8 (Function number) | 7~2 (offset) | 0 | 0
    uint32 off = (bus << 16) | (dev << 11) | (func << 8) | (offset);  // bus = 0 <=> 只找第0根主线上的设备
                                                                      // dev是枚举设备号  
                                                                      // func = 0 这个比较难解释，同一个设备不同func的接口比较难举例
                                                                      // offset 对应的地址用来登记一些设备信息，这里不管
    volatile uint32 *base = ecam + off;
    uint32 id = base[0];
    // PCI address space header:
	  // Byte Off   |   3   |   2   |   1   |   0   |
	  // base[0]: 0h|   Device ID   |   Vendor ID   |
    // | 10 0e (Device ID) | 80 86 (Vendor ID) | is an e1000
    if(id == 0x100e8086){
      // command and status register.
      // bit 0 : I/O access enable
      // bit 1 : memory access enable
      // bit 2 : enable mastering

      // PCI address space header:
      // Byte Off   |   3   |   2    |   1     |   0    |
      // base[1]: 4h|Status register | command register |

      // command register layout:
      // bits     |      description
      //  0       |     I/O Access Enable
      //  1       |     Memory Access Enable
      //  2       |     Enable Mastering. 
      // 实际上我们就打开了这前三位，base[1]=0b111.
      base[1] = 7;
      __sync_synchronize();

      for(int i = 0; i < 6; i++){
        // Byte Off              |   3   |   2    |   1     |   0    |
        // base[4]:          10h |           Base Address 0          |
        // base[5]:          14h |           Base Address 1          |
        // base[6]:          18h |           Base Address 2          |
        uint32 old = base[4+i];



        // writing all 1's to the BAR causes it to be
        // replaced with its size.
        base[4+i] = 0xffffffff;
        /* 
          这里 base address 的意思是注册一个内存地址给 PCI 设备，让设备把自己的 register 和NIC 的 flash 缓存内容往这个内存地址去 map。
          然后目前我们已经枚举到了e1000这个设备，对于每一个设备，PCI都给他了6个32位的Base Address Register(BAR)。根据手册：
          |  BAR  |  Addr  |                 31 ~ 4                        |  3   |  2 1  |  0  |
          |   0   |  10h   | Memory Register Base Address (31 : 4)         | pref | type  | mem |
          |   1   |  14h   |             Memory Register Base Address (63 : 32)                 |
          |   2   |  18h   | Memory Flash Register Base Address (31 : 4)   | pref | type  | mem |
          |   3   |  1Ch   |            Memory Flash Register Base Address (63 : 32)            |
          |   4   |  20h   |           I/O Register Base Address (31 : 2)         |  0 0  | mem |
          |   5   |  24h   |          Reserved (read as all 0s)                                 |
          mem : 0b indicates memory space, 1b indicates I/O
          type: indicates the address space size, 00b=32bits, 10b=64bits
          prefetch: 0b=non-prefetchable space, 1b=prefetchable space 
          Address: 低位其实会被硬件置0，只有高位可供读写。
                   因为在设备和PCI通信拷贝数据时，并不是一个bit一个bit 或一个byte一个byte地拷贝，
                   为了追求速度会尝试譬如8个byte8个byte这样拷，那么address最低三位都会被设成0，即做到8byte对齐，相当于分了个block。
                   根据手册，address可以分为：
                   | Flash size | Valid bits(R/W) | Zero bits(Read Only) |
                   | 64KB       | 31 ~ 16         | 15 ~ 4               |
                   | 128KB      | 31 ~ 17         | 16 ~ 4               |
                   | 256KB      | 31 ~ 18         | 17 ~ 4               |
                   | 512KB      | 31 ~ 19         | 18 ~ 4               |
          这个address只是给了个基址。
          譬如下面令base[0]=e1000_regs，那么就可以通过base[0]+offset去找到e1000_regs里的值了。
          关于这里为啥要设置成0xffffffff，我有两个猜想。第一，就是把未注册的base address全放到最高位去，这样不会影响其他注册了的base address。
          其次，也可以方便地计算出flash大小。譬如若base = 0xffffffff，然后硬件会把后4位置0，base=0xffff0000，这样直接取反加一就可以得到base=0x00010000得到size大小了。
        */
        __sync_synchronize();

        base[4+i] = old;
      }

      // tell the e1000 to reveal its registers at
      // physical address 0x40000000.
      base[4+0] = e1000_regs;

      e1000_init((uint32*)e1000_regs);
    }
  }
}
