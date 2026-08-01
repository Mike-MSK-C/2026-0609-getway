#include "daemon_runner.h"
#include "log.h"
#include "ota_version.h"
#include "string.h"
#include "app_runner.h"

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        log_error("请输入参数");
        return -1;
    }

    if (strcmp(argv[1], "app") == 0)
    {   
        //启动app模块
        app_runner_init();
    }
    else if (strcmp(argv[1], "ota") == 0)
    {   
        //启动模块检查
        ota_version_checkUpdata_day();
    }
    else if (strcmp(argv[1], "daemon") == 0)
    {
        daemon_runner_run();
    }
    else
    {
        log_error("参数错误,参数必须为app|ota|daemon");
        return -1;
    }

    return 0;
}