#include "app_device.h"
#include "unistd.h"
#include "app_bt.h"


int main(int argc, char *argv[])
{   
    //模拟一个串口文件
    Device * device= app_device_init("/home/mneg/Desktop/gatway/serial_file");
    //初始化蓝牙
    app_bt_init(device);


    app_device_start();

    sleep(100);

    app_device_destroy();

    return 0;


}