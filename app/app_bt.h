#ifndef __APP_BT_H_
#define __APP_BT_H_
#include "app_device.h"

typedef enum {
    BT_BAUD_9600 = '4',
    BT_BAUD_115200 = '8'
} BTBraudRate;

//蓝牙初始化函数
void app_bt_init(Device *device);
//蓝牙数据写前预处理函数
int app_bt_preWrite(char *data, int len);
//蓝牙数据读后处理
int app_bt_postRead(char *data, int len);


/* 测试蓝牙是否可用
 */
int app_bt_status(Device *device);

/**
 * 修改蓝牙名称
 */
int app_bt_rename(Device *device, char *name);

/**
 * 设置波特率
 */
int app_bt_setBraudRate(Device *device, BTBraudRate braud_rate);

/**
 * 重置(修改的配置才生效)
 */
int app_bt_reset(Device *device);

/**
 * 设置组网id(同一个组的多个设备组网id相同)
 * net_id： 4位十六进制字符串
 */
int app_bt_setNetId(Device *device, char *net_id);

/**
 * 设置蓝牙MAC地址(同一个组的多个设备MAC地址不同)
 * maddr: 4位十六进制字符串
 */
int app_bt_setmaddr(Device *device, char *maddr);



#endif // __APP_BT_H_
