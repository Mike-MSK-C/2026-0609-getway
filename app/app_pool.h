#ifndef __APP_POOL_H_
#define __APP_POOL_H_

typedef struct  
{
      int (*task_func)(void *arg);
      void *arg;
} Task;

//初始化线程池
int app_pool_init(int max_task_num);

//销毁线程池
int app_pool_destroy(void);

//添加任务
int app_pool_add_task(int (*task_func)(void *arg), void *arg);

//

#endif // __APP_POOL_H_