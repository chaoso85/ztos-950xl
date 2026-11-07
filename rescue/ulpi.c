#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#define VP 0x170
static volatile uint32_t *m;
static int wait_clear(uint32_t bit){for(int i=0;i<100000;i++){if(!(m[VP/4]&bit))return 0;usleep(10);}return -1;}
static int ulpi_rd(int a){ if(m[VP/4]&0) ; m[VP/4]=0x80000000; if(wait_clear(0x80000000))return -2; m[VP/4]=0x40000000|(a<<16); if(wait_clear(0x40000000))return -3; return m[VP/4]&0xff;}
static int ulpi_wr(int a,int v){ m[VP/4]=0x80000000; if(wait_clear(0x80000000))return -2; m[VP/4]=0x60000000|(a<<16)|(v<<8); if(wait_clear(0x40000000))return -3; return 0;}
int main(int c,char**v){int fd=open("/dev/mem",O_RDWR|O_SYNC);if(fd<0){perror("mem");return 1;}
m=mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0xf9a55000);if(m==MAP_FAILED){perror("mmap");return 1;}
if(c>2&&v[1][0]=='w'){for(int i=2;i+1<c;i+=2)printf("w %s=%s -> %d\n",v[i],v[i+1],ulpi_wr(strtol(v[i],0,0),strtol(v[i+1],0,0)));return 0;}
for(int i=2;i<c;i++){int a=strtol(v[i],0,0);printf("%02x=%02x ",a,ulpi_rd(a)&0xffff);}puts("");return 0;}
