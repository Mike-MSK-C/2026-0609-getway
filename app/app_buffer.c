#include "app_buffer.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "pthread.h"

/**
 * 初始化一个子缓冲区
 * @param size 缓冲区的总大小
 * @return 指向新创建的SubBuffer结构的指针，如果分配失败则返回NULL
 */
static SubBuffer *init_buffer(int size)
{
    SubBuffer *sub_buffer = (SubBuffer *)malloc(sizeof(SubBuffer));
    if (sub_buffer == NULL) {
        return NULL;
    }
    sub_buffer->ptr = (char *)malloc(size);
    if (sub_buffer->ptr == NULL) {
        free(sub_buffer);
        return NULL;
    }
    sub_buffer->total_size = size;
    sub_buffer->used_size = 0;
    return sub_buffer;
}

/**
 * @brief 初始化应用层缓冲区
 * @param size 缓冲区大小
 * @return Buffer* 返回初始化后的缓冲区指针，失败返回NULL
 */
Buffer *app_buffer_init(int size)
{
    // 分配 Buffer 结构体
    Buffer *buffer = (Buffer *)malloc(sizeof(Buffer));
    if (buffer == NULL) {
        return NULL;
    }

    // 初始化两个子缓冲区
    buffer->sub_buffer[0] = init_buffer(size);
    if (buffer->sub_buffer[0] == NULL) {
        goto cleanup_buffer;
    }

    buffer->sub_buffer[1] = init_buffer(size);
    if (buffer->sub_buffer[1] == NULL) {
        goto cleanup_sub0;
    }

    buffer->read_index = 0;
    buffer->write_index = 1;

    // 初始化互斥锁
    if (pthread_mutex_init(&(buffer->read_mutex), NULL) != 0) {
        goto cleanup_sub1;
    }
    if (pthread_mutex_init(&(buffer->write_mutex), NULL) != 0) {
        goto cleanup_read_mutex;
    }

    return buffer;

cleanup_read_mutex:
    pthread_mutex_destroy(&(buffer->read_mutex));
cleanup_sub1:
    free(buffer->sub_buffer[1]->ptr);
    free(buffer->sub_buffer[1]);
cleanup_sub0:
    free(buffer->sub_buffer[0]->ptr);
    free(buffer->sub_buffer[0]);
cleanup_buffer:
    free(buffer);
    return NULL;
}

/**
 * @brief 释放缓冲区
 * @param buffer 要释放的缓冲区指针
 */
void app_buffer_free(Buffer *buffer)
{
    if (buffer == NULL) {
        return;
    }
    free(buffer->sub_buffer[0]->ptr);
    free(buffer->sub_buffer[1]->ptr);
    free(buffer->sub_buffer[0]);
    free(buffer->sub_buffer[1]);
    pthread_mutex_destroy(&(buffer->read_mutex));
    pthread_mutex_destroy(&(buffer->write_mutex));
    free(buffer);
}

/**
 * @brief 向缓冲区写入数据，每条消息格式：1字节长度(uint8_t) + 数据
 * @param buffer 缓冲区指针
 * @param data 要写入的数据
 * @param data_size 数据长度，最大 255 字节
 * @return 成功返回 0，失败返回 -1
 */
int app_buffer_write(Buffer *buffer, char *data, int data_size)
{
    int ret = -1;

    // 长度字段为 1 字节(uint8_t)，最大只能表示 255
    if (data_size <= 0 || data_size > 255) {
        log_error("data size is invalid, must be 1~255");
        return -1;
    }

    pthread_mutex_lock(&(buffer->write_mutex));
    log_debug("加写锁");

    SubBuffer *write_buffer = buffer->sub_buffer[buffer->write_index];

    // 需要 1 字节长度 + data_size 字节数据
    if (write_buffer->used_size + 1 + data_size > write_buffer->total_size) {
        log_error("buffer is full");
        goto cleanup;
    }

    // 先写长度（1字节），再写数据内容
    write_buffer->ptr[write_buffer->used_size] = (uint8_t)data_size;
    write_buffer->used_size += 1;
    memcpy(write_buffer->ptr + write_buffer->used_size, data, data_size);
    write_buffer->used_size += data_size;

    ret = 0;

cleanup:
    pthread_mutex_unlock(&(buffer->write_mutex));
    log_debug("解写锁");
    return ret;
}

/**
 * @brief 从缓冲区读取一条消息
 * @param buffer 缓冲区指针
 * @param data 存放读取数据的 buffer
 * @param buffer_size data 缓冲区的大小
 * @return 成功返回数据长度，失败返回 -1
 */
int app_buffer_read(Buffer *buffer, char *data, int buffer_size)
{
    int ret = -1;

    pthread_mutex_lock(&(buffer->read_mutex));
    log_debug("加读锁");

    SubBuffer *read_buffer = buffer->sub_buffer[buffer->read_index];

    // 如果读缓冲区为空，尝试切换双缓冲
    if (read_buffer->used_size == 0) {
        pthread_mutex_lock(&(buffer->write_mutex));

        // 双重检查：拿到写锁后再次确认
        read_buffer = buffer->sub_buffer[buffer->read_index];
        if (read_buffer->used_size == 0) {
            // 交换读写索引
            buffer->read_index = buffer->write_index;
            buffer->write_index = (buffer->write_index + 1) % 2;

            // 重新指向新的读缓冲区（原写缓冲区）
            read_buffer = buffer->sub_buffer[buffer->read_index];

            if (read_buffer->used_size == 0) {
                log_error("both buffers are empty");
                pthread_mutex_unlock(&(buffer->write_mutex));
                goto read_cleanup;
            }
        }

        pthread_mutex_unlock(&(buffer->write_mutex));
    }

    // 用户提供的 buffer 是否够大
    if (read_buffer->used_size > buffer_size) {
        log_error("buffer size is too small");
        goto read_cleanup;
    }

    // 第一条消息：ptr[0] 是长度(uint8_t)，随后是数据
    int data_len = (uint8_t)read_buffer->ptr[0];
    log_debug("data_len = %d", data_len);
    memcpy(data, read_buffer->ptr + 1, data_len);

    // 将未消费的数据左移，覆盖已读走的消息
    int remaining = read_buffer->used_size - data_len - 1;
    if (remaining > 0) {
        memmove(read_buffer->ptr, read_buffer->ptr + data_len + 1, remaining);
    }
    read_buffer->used_size = remaining;

    ret = data_len;

read_cleanup:
    pthread_mutex_unlock(&(buffer->read_mutex));
    log_debug("解读锁");
    return ret;
}














// #include "app_buffer.h"
// #include "log.h"
// #include <string.h>
// #include <stdlib.h>
// #include "pthread.h"

// /**
//  * 初始化一个子缓冲区
//  * @param size 缓冲区的总大小
//  * @return 指向新创建的SubBuffer结构的指针，如果分配失败则返回NULL
//  */
// static SubBuffer *init_buffer(int size)
// {
//     // 分配SubBuffer结构体所需的内存空间
//     SubBuffer *sub_buffer = (SubBuffer *)malloc(sizeof(SubBuffer));
//     // 为缓冲区数据分配指定大小的内存空间
//     sub_buffer->ptr = (char *)malloc(size);
//     // 设置缓冲区的总大小
//     sub_buffer->total_size = size;
//     // 初始化已使用大小为0
//     sub_buffer->used_size = 0;
//     // 返回初始化完成的缓冲区指针
//     return sub_buffer;
// }
// // 初始化缓冲区
// /**
//  * @brief 初始化应用层缓冲区
//  * @param size 缓冲区大小
//  * @return Buffer* 返回初始化后的缓冲区指针
//  */
// Buffer *app_buffer_init(int size)
// {
//     Buffer *buffer = (Buffer *)malloc(sizeof(Buffer)); // 为Buffer结构体分配内存
//     // 初始化Buffer各个字段
//     buffer->sub_buffer[0] = init_buffer(size);
//     buffer->sub_buffer[1] = init_buffer(size);
//     buffer->read_index = 0;
//     buffer->write_index = 1;
//     // 初始化互斥锁
//     pthread_mutex_init(&(buffer->read_mutex), NULL);
//     pthread_mutex_init(&(buffer->write_mutex), NULL);
//     // 返回初始化后的缓冲区指针
//     return buffer;
// }

// // 释放缓冲区
// void app_buffer_free(Buffer *buffer)
// {
//     // 释放两个子缓冲区的内存
//     free(buffer->sub_buffer[0]->ptr);
//     free(buffer->sub_buffer[1]->ptr);
//     // 释放子缓冲区结构体
//     free(buffer->sub_buffer[0]);
//     free(buffer->sub_buffer[1]);
//     // 释放Buffer结构体
//     free(buffer);
//     return;
// }

// // 向缓冲区写入数据
// int app_buffer_write(Buffer *buffer, char *data, int data_size)
// {
//     // 写入数据不要超过一个字节
//     if (data_size > 256)
//     {
//         log_error("data size is too large");
//         return -1;
//     }
//     // 加锁
//     pthread_mutex_lock(&(buffer->write_mutex));
//     log_debug("加写锁");
//     // 获取写缓冲区
//     SubBuffer *write_buffer = buffer->sub_buffer[buffer->write_index];
//     // 如果写缓冲区已满，则返回错误
//     if (write_buffer->used_size + data_size > write_buffer->total_size)
//     {
//         log_error("buffer is full");
//         pthread_mutex_unlock(&(buffer->write_mutex));
//         return -1;
//     }

//     // 将数据写入写缓冲区,先写数据量大小，在写数据内容
//     write_buffer->ptr[write_buffer->used_size] = data_size;
//     write_buffer->used_size += 1;
//     memcpy(write_buffer->ptr + write_buffer->used_size, data, data_size);
//     write_buffer->used_size += data_size;
//     // 解锁
//     pthread_mutex_unlock(&(buffer->write_mutex));
//     log_debug("解写锁");

// }

// // 从缓冲区读取数据
// int app_buffer_read(Buffer *buffer, char *data, int buffer_size)
// {

//     // 加锁
//     pthread_mutex_lock(&(buffer->read_mutex));
//     log_debug("加读锁"); 
//     // 获取读缓冲区
//     SubBuffer *read_buffer = buffer->sub_buffer[buffer->read_index];
//     // 如果读缓冲区为空，切换缓冲区
//     pthread_mutex_lock(&(buffer->write_mutex));
//     if (read_buffer->used_size == 0)
//     {
//         buffer->read_index = buffer->write_index;
//         buffer->write_index = (buffer->write_index + 1) % 2;
//         // 在一次获取读缓冲区
//         read_buffer = buffer->sub_buffer[buffer->read_index];
//         if (read_buffer->used_size == 0)
//         {
//             log_error("buffer is empty");
//             pthread_mutex_unlock(&(buffer->write_mutex));
//             pthread_mutex_unlock(&(buffer->read_mutex));
//             return -1;
//         }
//     }
//     pthread_mutex_unlock(&(buffer->write_mutex));

//     if (read_buffer->used_size > buffer_size)
//     {
//         log_error("buffer size is too small");
//         pthread_mutex_unlock(&(buffer->read_mutex));
//         return -1;
//     }

//     // 获取长度 ，和数据到data数组中
//     int data_len = read_buffer->ptr[0];
//     memcpy(data, read_buffer->ptr + 1, data_len);
//     // 数据左移动
//     memmove(read_buffer->ptr, read_buffer->ptr + data_len + 1, read_buffer->used_size - data_len - 1);
//     // 减少used_size长度
//     read_buffer->used_size -= data_len + 1;
//     // 解锁
//     pthread_mutex_unlock(&(buffer->read_mutex));
//     log_debug("解读锁");
//     // 返回数据长度
//     return data_len;
// }
