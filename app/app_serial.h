#ifndef APP_SERIAL_H_
#define APP_SERIAL_H_
#include "termios.h"
#include "app_device.h"

// 波特率
typedef enum
{
    BAUDRATE_9600 = B9600,
    BAUDRATE_115200 = B115200, // 默认波特率
} baudrate_t;

// 校验位
typedef enum
{
    PARITY_NONE = 0,              // 无校验
    PARITY_ODD = PARENB | PARODD, // 奇校验
    PARITY_EVEN = PARODD,         // 偶校验
} parity_t;

// 停止位
typedef enum
{
    STOPBIT_1 = 0,      // 1位停止位
    STOPBIT_2 = CSTOPB, // 2位停止位
} stopbit_t;

// 设置波特率
int app_serial_setBaudrate(Device *device, baudrate_t baudrate);

// 设置校验位
int app_serial_setParity(Device *device, parity_t parity);

// 设置停止位
int app_serial_setStopbit(Device *device, stopbit_t stopbit);

// 设置是否为阻塞模式
int app_serial_setBlock(Device *device, int is_block);

// 设置是否为原始模式
int app_serial_setRaw(Device *device);

// 串口初始化
int app_serial_init(Device *device);

int app_serial_flash(Device * device);

#endif // APP_SERIAL_H_
