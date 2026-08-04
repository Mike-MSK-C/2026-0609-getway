#include "app_runner.h"

int is_running = 1;
void runner_exit(int signum)
{
    is_running = 0;

    // 释放资源
    app_device_destroy();
}

int app_runner_init(void)
{
    // 注册信号处理函数来实现结束前释放资源
    signal(SIGINT, runner_exit);
    signal(SIGTERM, runner_exit);

    // 初始化设备
    Device *device = app_device_init(DEVICE_FILE);
    if (device == NULL)
    {
        log_error("gateway initialization failed");
        return -1;
    }

    // 启动蓝牙
    app_bt_init(device);

    // 启动设备
    if (app_device_start() != 0)
    {
        log_error("device start failed");
        app_device_destroy();
        return -1;
    }

    // 不断运行(每隔一秒)
    while (is_running == 1)
    {
        sleep(1);
    }

    return 0;
}