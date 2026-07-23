#include "app_buffer.h"
#include "log.h"

int main(int argc, char **argv)
{
    // 初始化缓冲区 ,超过缓冲区大小会报错
    Buffer *buffer2 = app_buffer_init(1000);
    char data[222] = {0};
    for (size_t i = 0; i < 222; i++)
    {
        data[i] = 0;
    }

    // 写入数据
    app_buffer_write(buffer2, data, 222);

    // 读数据
    char read_buf2[333];
    int read_len3 = app_buffer_read(buffer2, read_buf2, 333);
    log_info("Read %d bytes: %s", read_len3, read_buf2);

    // 销毁缓冲区
    app_buffer_free(buffer2);

    return 0;
}