#include "app_serial.h"
#include "log.h"

// 设置波特率
int app_serial_setBaudrate(Device *device, baudrate_t baudrate)
{
    // 读取串口属性
    struct termios options;
    tcgetattr(device->fd, &options);

    // 修改波特率
    cfsetispeed(&options, baudrate);

    // 设置串口属性(当前不生效，设置好后 flash时生效 )
    int result = tcsetattr(device->fd, TCSAFLUSH, &options);

    if (result == -1)
    {
        log_error("set baudrate error");
        return -1;
    }
    else
    {
        log_info("set baudrate success");
        return 0;
    }
}

// 设置校验位
int app_serial_setParity(Device *device, parity_t parity)
{
    // 读取串口属性
    struct termios options;
    tcgetattr(device->fd, &options);

    // 修改校验位
    options.c_cflag &= ~(PARENB | PARODD); // 清除校验位
    switch (parity)
    {
    case PARITY_NONE:
        break; // 无校验位
    case PARITY_ODD:
        options.c_cflag |= PARENB; // 设置校验位
        options.c_cflag |= PARODD; // 设置奇校验
        options.c_iflag |= INPCK;  // 启用输入奇偶校验
        break;
    case PARITY_EVEN:

        options.c_cflag |= PARENB;  // 设置校验位
        options.c_cflag &= ~PARODD; // 设置偶校验
        options.c_iflag |= INPCK;   // 启用输入奇偶校验
        break;
    default:
        // 非法值，回退到无校验或报错
        options.c_cflag &= ~(PARENB | PARODD);
        break;
    }

    // 设置串口属性(当前不生效，设置好后 flash时生效 )
    int result = tcsetattr(device->fd, TCSAFLUSH, &options);
    if (result == -1)
    {
        log_error("set parity error");
        return -1;
    }
    else
    {
        log_info("set parity success");
        return 0;
    }
}

// 设置停止位
int app_serial_setStopbit(Device *device, stopbit_t stopbit)
{
    // 读取串口属性
    struct termios options;
    tcgetattr(device->fd, &options);

    // 清除停止位
    options.c_cflag &= ~CSTOPB;

    // 修改停止位
    switch (stopbit)
    {
    case STOPBIT_1:
        options.c_cflag &= ~CSTOPB; // 1位停止位
        break;
    case STOPBIT_2: // 2位停止位
        options.c_cflag |= CSTOPB;
        break;
    default:
        // 非法值，回退到1位停止位或报错
        options.c_cflag &= ~CSTOPB;
        break;
    }

    // 设置串口属性(当前不生效，设置好后 flash时生效 )
    int result = tcsetattr(device->fd, TCSAFLUSH, &options);
    if (result == -1)
    {
        log_error("set stopbit error");
        return -1;
    }
    else
    {
        log_info("set stopbit success");
        return 0;
    }
}

// 设置是否为阻塞模式
int app_serial_setBlock(Device *device, int is_block)
{
    // 读取串口属性
    struct termios options;
    tcgetattr(device->fd, &options);

    // 清空阻塞标志位
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    // 修改阻塞模式
    if (is_block)
    {
        options.c_cc[VMIN] = 1;  // 阻塞模式
        options.c_cc[VTIME] = 0; // 超时时间 0s (其实就是一直等)

        // 设置串口属性(当前不生效，设置好后 flash时生效 )
        int result = tcsetattr(device->fd, TCSAFLUSH, &options);
        if (result == -1)
        {
            log_error("set block error");
            return -1;
        }
        else
        {
            log_info("set block success");
            return 0;
        }
    }
    else
    {
        options.c_cc[VMIN] = 0;  // 非阻塞模式
        options.c_cc[VTIME] = 2; // 超时时间 0.2s

        // 设置串口属性(当前不生效，设置好后 flash时生效 )
        int result = tcsetattr(device->fd, TCSAFLUSH, &options);
        if (result == -1)
        {
            log_error("set none_block error");
            return -1;
        }
        else
        {
            log_info("set none_block success");
            return 0;
        }
    }
}

// 设置是否为原始模式
int app_serial_setRaw(Device *device)
{
    // 读取串口属性
    struct termios options;
    tcgetattr(device->fd, &options);

    // 设置原始模式
    cfmakeraw(&options);

    // 设置串口属性(当前不生效，设置好后 flash时生效 )
    int result = tcsetattr(device->fd, TCSAFLUSH, &options);
    if (result == -1)
    {

        log_error("set raw error");
        return -1;
    }
    else
    {
        log_info("set raw success");
        return 0;
    }

}

// 串口初始化
int app_serial_init(Device *device)
{
    // 设置串口属性
    app_serial_setBaudrate(device, BAUDRATE_9600);
    app_serial_setParity(device, PARITY_NONE);
    app_serial_setStopbit(device, STOPBIT_1);
    // app_serial_setBlock(device, 0);
    app_serial_setRaw(device);

    int result =tcflush(device->fd, TCIOFLUSH);
    if(result == -1)
    {
        log_error("tcflush error");
        return -1;
    }
    else
    {
        log_info("tcflush success");
        return 0;
    }
}



int app_serial_flash(Device * device)
{
    
    return tcflush(device->fd, TCIOFLUSH);

}