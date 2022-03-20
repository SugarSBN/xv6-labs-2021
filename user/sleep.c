/*
 * @Author: SuBonan
 * @Date: 2022-03-20 10:38:26
 * @LastEditTime: 2022-03-20 10:38:26
 * @FilePath: \xv6-labs-2021\user\sleep.c
 * @Github: https://github.com/SugarSBN
 * これなに、これなに、これない、これなに、これなに、これなに、ねこ！ヾ(*´∀｀*)ﾉ
 */
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char const *argv[]){
	if (argc != 2){
		fprintf(2, "Usage: sleep seconds\n");
		exit(1);
	}
	int time = atoi(argv[1]);
	sleep(time);
	
	exit(0);
}
