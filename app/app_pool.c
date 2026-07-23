#define _GNU_SOURCE
#include "app_pool.h"
#include "log.h"
#include "pthread.h"
#include "stdlib.h"
#include "mqueue.h"

// 线程池线程数量
static int pthread_number;
// 线程池
static pthread_t *pthread_pool;
// 消息队列
static mqd_t mq_fd;

static char *mq_name = "/app_pool_mq"; // 消息队列名字

// 线程函数
void *app_pool_task(void *arg)
{
    Task task;
    while (1)
    {
        int len = mq_receive(mq_fd, (char *)&task, sizeof(Task), NULL);
        if (len == sizeof(Task))
        {
            task.task_func(task.arg);
        }
        sched_yield();
    }
}

int app_pool_init(int max_task_num)
{
    pthread_number = max_task_num;
    pthread_pool = (pthread_t *)malloc(sizeof(pthread_t) * max_task_num);
    if (pthread_pool == NULL)
    {
        log_error("malloc pthread pool failed");
        return -1;
    }

    // 创建消息队列 ,将消息队列文件描述符赋值给全局变量mq_fd
    struct mq_attr mq_attr;
    mq_attr.mq_maxmsg = 10;
    mq_attr.mq_msgsize = sizeof(Task);
    mq_fd = mq_open(mq_name, O_CREAT | O_RDWR, 0644, &mq_attr);
    if (mq_fd == -1)
    {
        log_error("create mq failed");
    }
    // 创建线程池
    for (int i = 0; i < max_task_num; i++)
    {
        if (pthread_create(&pthread_pool[i], NULL, app_pool_task, NULL) != 0)
        {
            log_error("create pthread failed");
            return -1;
        }
    }
    log_debug("app pool init success\r\n");

    return 0;
}

// 销毁线程池
int app_pool_destroy(void)
{
    // 关闭消息队列
    mq_close(mq_fd);
    mq_unlink(mq_name);
    // 销毁线程池
    for (size_t i = 0; i < pthread_number; i++)
    {
        pthread_cancel(pthread_pool[i]);
        pthread_join(pthread_pool[i], NULL);
    }
    free(pthread_pool);
    pthread_pool = NULL;
    return 0;
}

// 添加任务
int app_pool_add_task(int (*task_func)(void *arg), void *arg)
{
    // 创建队列
    Task task;
    task.task_func = task_func;
    task.arg = arg;
    // 发送到消息队列
    return mq_send(mq_fd, (char *)&task, sizeof(Task), 0);
}
