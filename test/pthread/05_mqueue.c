#define _GNU_SOURCE // 声明可以使用GNU扩展
#include "pthread.h"
#include "stdio.h"
#include "unistd.h"
#include "mqueue.h"
void *pt1_func(void *arg);
void *pt2_func(void *arg);

mqd_t mq ; //定义消息队列描述符

int main(int argc, char *argv[])
{
    printf("主线程开始执行 id:%d\n", getpid());
    pthread_t pt1, pt2; // 定义两个线程标识变量
    //创建消息队列结构体，并且初始化
    struct mq_attr attr;
    //消息最大个数
    attr.mq_maxmsg = 10;
    //消息最大大小
    attr.mq_msgsize = 1024;
    //创建消息队列
    mq = mq_open("/test", O_CREAT | O_RDWR, 0666, &attr);
    //判断队列描述符是否创建成功
    if (mq == -1)
    {
        perror("mq_open");
        return -1;
    }
    pthread_create(&pt1, NULL, pt1_func, NULL);
    pthread_create(&pt2, NULL, pt2_func, NULL);

    pthread_join(pt1, NULL);
    pthread_join(pt2, NULL);
    printf("主线程结束执行 id:%d\n", getpid());
    mq_close(mq);
    mq_unlink("/test");
    return 0;
}
void *pt1_func(void *arg)
{
    printf("线程1开始执行 id:%d\n", getpid());
    mq_send(mq, "hello", 6, 0);
    printf("线程1发送消息成功\n");
}

void *pt2_func(void *arg)
{
    printf("线程2开始执行 id:%d\n", getpid());
    char buf[1024];
    ssize_t len= mq_receive(mq,buf,1024,0);
    printf("线程2%ld\n",len );
    printf("线程2接收消息成功\n");
    printf("线程2接收到的消息是:%s\n",buf);
}