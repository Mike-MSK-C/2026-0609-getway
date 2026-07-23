#ifndef __APP_BUFFER_H_
#define __APP_BUFFER_H_

#include <pthread.h>
// 子缓冲区结构体
typedef struct
{
    char *ptr;
    int total_size;
    int used_size;
} SubBuffer;

// 大缓冲区结构体
// 两个子缓冲区
typedef struct
{
    SubBuffer *sub_buffer[2];
    int read_index;
    int write_index;
    pthread_mutex_t read_mutex;
    pthread_mutex_t write_mutex;

} Buffer;

// 初始化缓冲区
Buffer *app_buffer_init(int size);

// 释放缓冲区
void app_buffer_free(Buffer *buffer);

// 向缓冲区写入数据
int app_buffer_write(Buffer *buffer, char *data, int data_size);

// 从缓冲区读取数据
int app_buffer_read(Buffer *buffer, char *data, int buffer_size);

#endif // __APP_BUFFER_H_