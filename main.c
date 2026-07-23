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
        app_runner_init();
    }
    else
    {
        return -1;
    }

    return 0;
}