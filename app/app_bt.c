#define _GNU_SOURCE
#include "app_bt.h"
#include "string.h"
#include "log.h"
#include "stdlib.h"
#include "unistd.h"
#include "app_serial.h"

static int init_bt(Device *device)
{
    // 初始化串口
    app_serial_init(device);
    // 设置非阻塞模式
    app_serial_setBlock(device, 0);
    // flush ,使配置生效
    app_serial_flash(device);

    // 判断串口是否可用
    if (app_bt_status(device) == 0)
    {
        log_error("当前的蓝牙波特率为9600,串口可用");
        // 将蓝牙的波特率设置为115200
        app_bt_setBraudRate(device, BT_BAUD_115200);
        // 重启蓝牙
        app_bt_reset(device);
        // 等待蓝牙重启完成
        sleep(1);
    }
    // 将串口的波特率设置为115200
    app_serial_setBaudrate(device, BAUDRATE_115200);
    // flush使得配置生效
    app_serial_flash(device);

    // 判断蓝牙是否可用
    if (app_bt_status(device) != 0)
    {
        log_error("串口不可用");
        return -1;
    }

    // 设置蓝牙的MADDR (组内唯一)
    app_bt_setmaddr(device, "0101");
    // 设置netid 组内相同，不同组之间不同
    app_bt_setNetId(device, "1234");

    // 设置串口为阻塞模式
    app_serial_setBlock(device, 1);
    // flush ,使配置生效
    app_serial_flash(device);

    // 蓝牙配置成功
    log_debug("蓝牙配置成功");

    return 0;
}

// 蓝牙初始化函数
void app_bt_init(Device *device)
{
    device->post_read = app_bt_postRead;
    device->pre_write = app_bt_preWrite;

    // 初始化蓝牙
    return init_bt(device);
}

/*
 例子：41 54 2b 4d 45 53 48 00 ff ff 61 62 63 0d 0a
        41 54 2b 4d 45 53 48 00： AT+MESH（固定头部）
        ff ff: 对端的MADDR（如果是FFFF代表群发）
        61 62 63: 要发送的数据（不超过12字节）
        0d 0a：\r\n（固定结尾）
        AT+MESH XX abc \r\n
*/
// 蓝牙数据写前预处理函数
// data数据格式请前往app_message.c中查看app_message_json2Chars函数中的注释
int app_bt_preWrite(char *data, int len)
{
    if (len < 6)
    {
        log_error("data len is too short");
        return -1;
    }

    // 计算蓝牙数据的长度
    int blu_len = 12 + data[2];

    // 创建蓝牙数据数组容器
    char blu_data[blu_len];
    // 将固定头部拷贝到蓝牙数据数组app_message_json2Chars容器中
    memcpy(blu_data, "AT+MESH", 8);
    // 拷贝id
    memcpy(blu_data + 8, data + 3, 2);
    // 拷贝数据
    memcpy(blu_data + 10, data + 5, data[2]);

    // 组装蓝牙数据
    memcpy(blu_data + 10 + data[2], "\r\n", 2);

    // 清空data中的数据，将蓝牙数据拷贝到data中，返回蓝牙数据长度
    memset(data, 0, len);
    memcpy(data, blu_data, blu_len);

    return blu_len;
}

/*

从蓝牙读取到数据后的处理 postRead
    接收方得到数据（3 + [2]）：f1 dd 07 23 23 ff ff 41 42 43
        f1 dd : 固定的头部
        07： 之后数据的长度（5-16之间）
        23 23：对端（发送方）的MADDR
        ff ff: 我的MADDR或ffff(群发)
        41 42 43：发送的数据
    处理后的数据格式：conn_type id_len msg_len id msg
*/

static char read_buffer[1024];
static int read_len;
static char fixed_head[2] = {0xf1, 0xdd};

// 移除缓存中指定的数据
static void remove_data(char *data, int len)
{
    memmove(data, data + len, read_len - len);
    read_len -= len;
}

// 读取的蓝牙数据，处理成为json格式
int app_bt_postRead(char *data, int len)
{
    memcpy(read_buffer + read_len, data, len);
    read_len += len;

    // 如果接受的长度小于8,就是还没有接受完
    if (read_len < 8)
    {
        log_debug("还没读完蓝牙数据!");
        return 0;
    }
    size_t i;
    // 查找蓝牙数据的固定头部
    for (i = 0; i < read_len - 7; i++)
    {
        // 查找开头
        if (memcmp(read_buffer + i, fixed_head, 2) == 0)
        {
            // 丢弃无效的数据
            if (i > 0)
            {
                remove_data(read_buffer, i);
            }

            // 如果接受的数据长度小于固定头部+数据长度，就是还没有接受完
            int blue_len = read_buffer[2] + 3;
            if (read_len < blue_len)
            {
                log_debug("还没读完蓝牙数据!");
                return 0;
            }

            memset(data, 0, len);
            data[0] = 1;                          // conn_type
            data[1] = 2;                          // id_len
            data[2] = read_buffer[2] - 4;         // msg_len
            memcpy(data + 3, read_buffer + 3, 2); // id
            memcpy(data + 5, read_buffer + 7, data[2]);

            // 移除缓存中已经处理的数据
            remove_data(read_buffer, blue_len);

            // 返回消息长度
            return data[2] + 5;
        }
    }

    // 如果没有找到固定头部，丢弃缓存中的数据
    remove_data(read_buffer, i);
    return 0;
}

// 定义一个 判断是否收到 ACK指令
static int app_bt_isAck(int fd)
{
    usleep(1000);
    char data_buffer[4];
    read(fd, data_buffer, 4);

    if (memcmp(data_buffer, "OK\r\n", 4) == -1)
    {
        log_error("没有收到ACK指令");
        return -1;
    }

    log_debug("收到ACK指令");
    return 0;
}

/* 测试蓝牙是否可用
 */
int app_bt_status(Device *device)
{
    // 像蓝牙串口文件写入"AT\r\n"
    write(device->fd, "AT\r\n", 4);
    // 通过读取"OK\r\n"判断蓝牙是否可用

    return app_bt_isAck(device->fd);
}

/**
 * 修改蓝牙名称
 */
int app_bt_rename(Device *device, char *name)
{
    // 拼接指令
    char cmd[20];
    sprintf(cmd, "AT+NAME%s\r\n", name);
    // 写入指令
    write(device->fd, cmd, strlen(cmd));

    // 等待 ACK
    return app_bt_isAck(device->fd);
}

/**
 * 设置波特率
 */
int app_bt_setBraudRate(Device *device, BTBraudRate braud_rate)
{
    // 拼接指令
    char cmd[20];
    sprintf(cmd, "AT+BAUD%c\r\n", braud_rate);
    // 写入指令
    write(device->fd, cmd, strlen(cmd));

    // 等待 ACK
    return app_bt_isAck(device->fd);
}

/**
 * 重置(重启蓝牙)
 */
int app_bt_reset(Device *device)
{
    // 拼接指令
    char cmd[20];
    sprintf(cmd, "AT+RESET\r\n");
    // 写入指令
    write(device->fd, cmd, strlen(cmd));

    // 等待 ACK
    return app_bt_isAck(device->fd);
}

/**
 * 设置组网id(同一个组的多个设备组网id相同)
 * net_id： 4位十六进制字符串
 */
int app_bt_setNetId(Device *device, char *net_id)
{
    // 拼接指令
    char cmd[20];
    sprintf(cmd, "AT+NETID%s\r\n", net_id);
    // 写入指令
    write(device->fd, cmd, strlen(cmd));

    // 等待 ACK
    return app_bt_isAck(device->fd);
}

/**
 * 设置蓝牙MAC地址(同一个组的多个设备MAC地址不同)
 * maddr: 4位十六进制字符串
 */
int app_bt_setmaddr(Device *device, char *maddr)
{
    // 拼接指令
    char cmd[20];
    sprintf(cmd, "AT+MADDR%s\r\n", maddr);
    // 写入指令
    write(device->fd, cmd, strlen(cmd));

    // 等待 ACK
    return app_bt_isAck(device->fd);
}
