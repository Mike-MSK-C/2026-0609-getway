#ifndef __APP_DEVICE_H_
#define __APP_DEVICE_H_
#include "pthread.h"
#include "app_message.h"
#include "app_common.h"
#include "app_mqtt.h"
#include "app_buffer.h"
#include "app_pool.h"
#include "stdio.h"
#include "stdlib.h"
#include "fcntl.h"
#include "unistd.h"
#include "log.h"


typedef struct
{
    char *filename;                        // 设备文件 接收了下游设备发过来数据的文件
    int fd;                                // 文件描述符
    Buffer *up_buffer;                     // 上行缓冲区
    Buffer *down_buffer;                   // 下行缓冲区
    pthread_t read_thread;                 // 读设备数据的线程
    int is_running;                        // 读线程是否运行
    long last_write_time;                  // 上次写数据的时间
    int (*post_read)(char *data, int len); // 从设备读取数据后，对数据的处理函数，将数据处理成字符数组消息
    int (*pre_write)(char *data, int len); // 写数据前，对数据的处理函数，将字符数组消息处理成设备需要的格式
    long last_read_time;                     // 上次读数据的时间
} Device;

Device *app_device_init(char *filename);

/**
 * 启动设备的读线程
 */
int app_device_start(void);

/**
 * 销毁设备
 */
void app_device_destroy(void);

#endif // __APP_DEVICE_H_