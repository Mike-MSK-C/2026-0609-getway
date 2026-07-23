#define _GNU_SOURCE  //声明可以使用GNU扩展
#include "pthread.h"
#include "stdio.h"
#include "unistd.h"
void *pt1_func(void *arg);
void *pt2_func(void *arg);

int main(int argc, char *argv[])
{   printf("主线程开始执行 id:%d\n",getpid());
    pthread_t pt1 ,pt2; //定义两个线程标识变量
    pthread_create(&pt1, NULL,pt1_func, NULL);
    pthread_create(&pt2, NULL,pt2_func, NULL);
    
    
    pthread_join(pt1,NULL);
    pthread_join(pt2,NULL);
    printf("主线程结束执行 id:%d\n",getpid());
    return 0;
}
void *pt1_func(void *arg)
{   
    printf("线程1开始执行 id:%d\n",getpid());
}

void *pt2_func(void *arg)
{
    printf("线程2开始执行 id:%d\n",getpid());
}