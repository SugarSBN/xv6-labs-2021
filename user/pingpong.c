/*
 * @Author: SuBonan
 * @Date: 2022-03-20 10:39:56
 * @LastEditTime: 2022-03-20 10:48:57
 * @FilePath: \xv6-labs-2021\user\pingpong.c
 * @Github: https://github.com/SugarSBN
 * これなに、これなに、これない、これなに、これなに、これなに、ねこ！ヾ(*´∀｀*)ﾉ
 */
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char const *argv[]){
  int p[2];
  pipe(p);
  if (fork() == 0){
    char c;

    if (read(p[0], &c, 1) != 1){
      fprintf(2, "child process failed to read!\n");
      exit(1);
    }

    printf("%d: received ping\n", getpid());
    close(p[0]);
    
    if (write(p[1], &c, 1) != 1){
      fprintf(2, "child process failed to write!\n");
      exit(1);
    }
    
    exit(0);
  }else{
    char c = 'a';
    
    if (write(p[1], &c, 1) != 1){
      fprintf(2, "parent process failed to write!\n");
      exit(1);  
    }
    close(p[1]);
    wait(0);
    
    if (read(p[0], &c, 1) != 1){
      fprintf(2, "parent process failed to read!\n");
      exit(1);
    }

    printf("%d: received pong\n", getpid());
    exit(0);
  }
  return 0;
}
