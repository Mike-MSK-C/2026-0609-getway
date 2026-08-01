#define _GNU_SOURCE
#include "app_device.h"
#define BUFFER_SIZE 1024
static Device *device = NULL;

Device *app_device_init(char *filename)
{
    if (device != NULL)
        return device;
    device = (Device *)malloc(sizeof(Device));
    device->filename = filename;
    device->fd = open(filename, O_RDWR);
    device->down_buffer = app_buffer_init(BUFFER_SIZE);
    device->up_buffer = app_buffer_init(BUFFER_SIZE);
    device->pre_write = 0;
    device->is_running = 0;
    device->post_read = 0;
    // 初始化线程池 MQTT模块
    app_pool_init(6);
    app_mqtt_init();

    return device;
}

static int send_msg_func(void *arg)
{
    // 定义直接初始化为0，干净无脏数据
    char data_buffer[125] = {0};
    int data_len = app_buffer_read(device->up_buffer, data_buffer, 125);

    // 先判断长度合法，避免短数据报错
    if (data_len <= 0)
    {
        return -1;
    }

    char *json_data = app_message_chars2Json(data_buffer, data_len);
    if (!json_data)
        return -1;

    if (app_mqtt_send(json_data) == 0)
    {
        log_info("send message success");
        free(json_data); // 记得释放json内存防泄漏
        return 0;
    }
    free(json_data);
    return -1;
}

static void *read_pthread_func(void *arg)
{
    while (device->is_running)
    {
        char data_buffer[125];
        int size = read(device->fd, data_buffer, 125);
        if (size > 0)
        {
            log_info("串口上行原始数据到达, len=%d", size);
            for (int i = 0; i < size; ++i)
            {
                printf("%02X ", (unsigned char)data_buffer[i]);
            }
            printf("\n");
        }

        // 需要将蓝牙数据转化为字符数组
        if (size > 0 && device->post_read)
        {
            // 将数据写入上行缓冲区
            ssize_t len = device->post_read(data_buffer, size);
            if (len <= 0)
            {
                continue;
            }

            app_buffer_write(device->up_buffer, data_buffer, len);
            // 将上行缓冲区的数据交给线程池
            app_pool_add_task(send_msg_func, device->up_buffer->sub_buffer[device->up_buffer->read_index]);
            // 清空临时缓冲区
            for (size_t i = 0; i < size; i++)
            {
                data_buffer[i] = 0;
            }
        }
    }
    return NULL;
}

int write_data_task_func(void *arg);

// 读取下行缓冲区的数据到串口文件中
// 两次写入相差200ms
// int write_data_task_func(void *arg)
// {
//     Device *device = (Device *)arg;
//     char data_buffer[125];
//     int data_len = app_buffer_read(device->down_buffer, data_buffer, 125);
//     if (device->pre_write)
//     {
//         data_len = device->pre_write(data_buffer, data_len);
//         // 待续。。。。。。。。。。。。
//     }
//     long distance = app_common_getCurrentTime() - device->last_read_time;
//     if (distance < 200)
//     {
//         usleep((200 - distance) * 1000);
//     }
//     // 写入串口文件
//     int len = write(device->fd, data_buffer, data_len);
//     if (len != data_len)
//     {
//         log_info("write faill!");
//         return 0;
//     }

//     log_debug("写入串口文件成功 %d %s ", len, data_buffer);

//     device->last_read_time = app_common_getCurrentTime();

//     return 0;
// }

int write_data_task_func(void *arg)
{
    Device *device = (Device *)arg;
    char data_buffer[125] = {0};

    log_info("下行串口任务开始");

    int data_len = app_buffer_read(device->down_buffer, data_buffer, sizeof(data_buffer));
    log_info("下行缓冲区读取完成, len=%d", data_len);

    if (data_len <= 0)
    {
        log_error("下行缓冲区读取失败");
        return -1;
    }

    if (device->pre_write)
    {
        log_info("开始执行蓝牙预处理");
        data_len = device->pre_write(data_buffer, data_len);
        log_info("蓝牙预处理完成, len=%d", data_len);

        if (data_len <= 0)
        {
            log_error("蓝牙预处理失败");
            return -1;
        }
    }

    log_info("准备写串口, fd=%d, len=%d", device->fd, data_len);
    int len = write(device->fd, data_buffer, data_len);
    log_info("串口write返回值=%d", len);

    if (len != data_len)
    {
        log_error("串口写入失败, expect=%d, actual=%d", data_len, len);
        return -1;
    }

    log_info("写入串口文件成功, len=%d", len);
    return 0;
}

int receive_msg_func(const char *json);

// int receive_msg_func(const char *json)
// {
//     // 将消息转化为字符数组
//     char data_buffer[125];
//     int data_len = app_message_json2Chars(json, data_buffer, sizeof(data_buffer));

//     // 放入下行缓冲区
//     app_buffer_write(device->down_buffer, data_buffer, data_len);

//     // 将下行缓冲区的数据交给线程池
//     app_pool_add_task(write_data_task_func, device);

//     return 0;
// }
int receive_msg_func(const char *json)
{
    log_info("MQTT下行回调: %s", json);

    char data_buffer[125];
    int data_len = app_message_json2Chars(json, data_buffer, sizeof(data_buffer));
    if (data_len <= 0)
    {
        log_error("MQTT下行JSON解析失败, data_len=%d", data_len);
        return -1;
    }

    log_info("准备写入下行缓冲区, len=%d", data_len);

    if (app_buffer_write(device->down_buffer, data_buffer, data_len) != 0)
    {
        log_error("写入下行缓冲区失败");
        return -1;
    }

    log_info("已写入下行缓冲区, 准备提交串口写任务");
    app_pool_add_task(write_data_task_func, device);
    return 0;
}

int app_device_start()
{
    if (device->is_running == 1)
    {
        log_info("device is running");
        return 1;
    }
    device->is_running = 1;

    // 启动上行流程 启动读线程
    pthread_create(&device->read_thread, NULL, read_pthread_func, NULL);

    // 启动下行流程  MQTT订阅消息
    // 注册一个函数，接受消息的回调
    app_mqtt_register_callback(receive_msg_func);
    return 0;
}

void app_device_destroy()
{
    close(device->fd);
    app_buffer_free(device->down_buffer);
    app_buffer_free(device->up_buffer);
    device->is_running = 0;
    // 关闭线程池
    pthread_cancel(device->read_thread);
    pthread_join(device->read_thread, NULL);

    free(device);
    device = NULL;
    app_pool_destroy();
    app_mqtt_close();
}