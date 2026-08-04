#define _GNU_SOURCE
#include "daemon_runner.h"
#include "daemon_sub_process.h"
#include "log.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

static SubProcess *subProcess[SUB_PROCESS_COUNT];
static char *params[2] = {"app", "ota"};

static volatile sig_atomic_t is_running = 1;

/**
 * 退出处理函数
 * 该函数用于处理程序退出信号，将运行状态标志设置为0，表示程序应该停止运行
 * @param signum 接收到的信号编号
 */
void exit_handler(int signum)
{
    is_running = 0;  // 将运行状态标志设置为0，表示程序应该停止运行
}

int daemon_runner_run()
{
    int test_mode = 0;
    const char *test_mode_env = getenv("GATEWAY_TEST_MODE");
    if (test_mode_env != NULL && strcmp(test_mode_env, "1") == 0)
    {
        test_mode = 1;
    }

    // 将当前进程变为守护进程
    if (daemon(0, 1) < 0)
    {
        return -1;
    }

    // 关闭控制台输入 将日志输出重定向到日志文件
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    if (open("/dev/null", O_RDWR) < 0)
    {
        return -1;
    }
    if (open(LOG_FILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644) < 0)
    {
        return -1;
    }
    if (open(LOG_FILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644) < 0)
    {
        return -1;
    }

    // 注册结束的信号
    signal(SIGTERM, exit_handler);

    if (test_mode)
    {
        log_info("test mode enabled: serial-dependent app process is disabled");
    }

    // 初始化守护进程数组
    for (size_t i = 0; i < SUB_PROCESS_COUNT; i++)
    {
        if (test_mode && strcmp(params[i], "app") == 0)
        {
            continue;
        }

        subProcess[i] = daemon_sub_process_init(params[i]);
        if (subProcess[i] == NULL)
        {
            // 初始化失败，清理已创建的子进程
            for (size_t j = 0; j < i; j++)
            {
                daemon_sub_process_exit(subProcess[j]);
            }
            return -1;
        }
    }

    // 主循环
    while (is_running)
    {
        for (size_t i = 0; i < SUB_PROCESS_COUNT; i++)
        {
            if (subProcess[i] != NULL)
            {
                daemon_sub_process_checkStart(subProcess[i]);
            }
        }
        sleep(3);
    }

    // 如果守护进程退出，则关闭子进程
    for (size_t i = 0; i < SUB_PROCESS_COUNT; i++)
    {
        if (subProcess[i] != NULL)
        {
            daemon_sub_process_exit(subProcess[i]);
        }
    }

    return 0;
}
