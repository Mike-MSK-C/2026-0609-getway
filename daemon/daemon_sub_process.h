#ifndef DAEMON_SUB_PROCESS_H_
#define DAEMON_SUB_PROCESS_H_

#include "sys/types.h"

// 被守护的子进程结构体
typedef struct
{
    pid_t pid;       // 子进程的pid
    char *cmd_param; // 运行子进程的命令参数  app|ota
    int fail_count;  // 失败(非正常退出)的次数
} SubProcess;

#define MAX_FAIL_COUNT 6 // 最大失败次数

#define EXIT_FAILURE -1
/**
 * 初始化子进程
 */
SubProcess *daemon_sub_process_init(char *cmd_param);

/**
 * 查检并启动子进程
 */
int daemon_sub_process_checkStart(SubProcess *sub_process);

/**
 * 结束子进程
 */
void daemon_sub_process_exit(SubProcess *sub_process);

#endif // DAEMON_SUB_PROCESS_H_