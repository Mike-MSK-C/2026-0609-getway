#include "log.h"

int main(int argc, char *argv[])
{   
    //设置日志输出级别
    log_set_level(LOG_DEBUG);
    //设置日志输出文件
    //设置不同级别日至输出
    log_trace("trace log");
    log_debug("debug log");
    log_info("info log");
    log_warn("warn log");
    log_error("error log");
    log_fatal("fatal log");

    return 0;
}