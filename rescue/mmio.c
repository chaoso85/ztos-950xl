#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
int main(int c,char**v){int fd=open("/dev/mem",O_RDONLY|O_SYNC);if(fd<0){perror("mem");return 1;}
for(int i=1;i<c;i++){unsigned long a=strtoul(v[i],0,0);unsigned long pg=a&~0xfffUL;
volatile uint32_t*m=mmap(0,0x1000,PROT_READ,MAP_SHARED,fd,pg);if(m==MAP_FAILED){perror("mmap");continue;}
printf("%lx=%08x ",a,m[(a-pg)/4]);munmap((void*)m,0x1000);}puts("");return 0;}
