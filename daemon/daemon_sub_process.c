#include "daemon_sub_process.h"
#include "sys/wait.h"
#include "unistd.h"
#include <sys/reboot.h>
#include <string.h>
#include <linux/limits.h>
#include <stdlib.h>
#include "log.h"

/**
 * 初始化子进程
 */
SubProcess *daemon_sub_process_init(char *cmd_param)
{
    SubProcess *sub_process = (SubProcess *)malloc(sizeof(SubProcess));
    if (sub_process == NULL)
    {
        return NULL;
    }
    sub_process->cmd_param = strdup(cmd_param);
    if (sub_process->cmd_param == NULL)
    {
        free(sub_process);
        return NULL;
    }
    sub_process->pid = -1;
    sub_process->fail_count = 0;
    return sub_process;
}

/**
 * 查检并启动子进程
 */
int daemon_sub_process_checkStart(SubProcess *sub_process)
{
    // 检查子进程是否存在
    int status = 0;
    pid_t result = 0;

    if (sub_process->pid > 0)
    {
        result = waitpid(sub_process->pid, &status, WNOHANG);
        if (result == 0)
        {
            // 子进程仍在运行
            log_debug("sub process is running process=%s", sub_process->cmd_param);
            return 0;
        }
        else if (result > 0)
        {
            // 子进程已退出，检查退出状态
            if (WIFEXITED(status))
            {
                int exit_code = WEXITSTATUS(status);
                if (exit_code != 0)
                {
                    log_error("sub process exited with code %d, process=%s", exit_code, sub_process->cmd_param);
                    sub_process->fail_count++;
                }
                else
                {
                    log_info("sub process exited normally, process=%s", sub_process->cmd_param);
                }
            }
            else if (WIFSIGNALED(status))
            {
                int sig = WTERMSIG(status);
                log_error("sub process terminated by signal %d, process=%s", sig, sub_process->cmd_param);
                sub_process->fail_count++;
            }

            if (sub_process->fail_count > MAX_FAIL_COUNT)
            {
                log_error("sub process failed too many times, rebooting... process=%s", sub_process->cmd_param);
                reboot(RB_AUTOBOOT);
            }
        }
        // result == -1 表示 waitpid 出错，继续尝试重启
    }

    // 启动子进程
    sub_process->pid = fork();
    if (sub_process->pid == 0)
    {
        // 子进程
        // 优先使用 /proc/self/exe（即使原文件被删除也能工作）
        char exe_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

        char *path;
        if (len != -1)
        {
            exe_path[len] = '\0';
            path = exe_path;
        }
        else
        {
            // 降级到硬编码路径
            path = "/home/mneg/Desktop/gatway/gatway_test";
        }

        char *arg[] = {path, sub_process->cmd_param, NULL};
        execve(path, arg, NULL);
        // 如果 execve 失败，退出子进程
        _exit(EXIT_FAILURE);
    }
    else if (sub_process->pid < 0)
    {
        // fork 失败
        log_error("fork failed, process=%s", sub_process->cmd_param);
        return -1;
    }
    else
    {
        // 父进程，fork 成功
        log_info("sub process started, pid=%d, process=%s", sub_process->pid, sub_process->cmd_param);
        return 0;
    }
}

/**
 * 结束子进程
 */
void daemon_sub_process_exit(SubProcess *sub_process)
{
    if (sub_process == NULL)
    {
        return;
    }

    // 根据pid kill 进程
    if (sub_process->pid > 0)
    {
        pid_t pid = sub_process->pid;
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        sub_process->pid = -1;
    }

    // 释放内存
    if (sub_process->cmd_param != NULL)
    {
        free(sub_process->cmd_param);
    }
    free(sub_process);
}