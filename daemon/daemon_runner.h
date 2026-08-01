#ifndef DAEMON_RUNNER_H_
#define DAEMON_RUNNER_H_

#define LOG_FILE_PATH "/home/mneg/daemon.log" //守护进程日志位置
#define SUB_PROCESS_COUNT 2 //守护进程启动的子进程数量

int daemon_runner_run();

#endif // DAEMON_RUNNER_H_ 