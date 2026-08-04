#include "app_runner.h"
#include "daemon_runner.h"
#include "ota_version.h"
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        log_error("参数必须为 app|ota|daemon");
        return -1;
    }

    if (strcmp(argv[1], "app") == 0)
    {
        return app_runner_init();
    }
    if (strcmp(argv[1], "ota") == 0)
    {
        return ota_version_checkUpdata_day();
    }
    if (strcmp(argv[1], "daemon") == 0)
    {
        return daemon_runner_run();
    }

    log_error("参数必须为 app|ota|daemon");
    return -1;
}