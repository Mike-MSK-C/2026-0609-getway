#ifndef __APP_MQTT_H_
#define __APP_MQTT_H_
#include "MQTTClient.h"

//MQTT初始化
int app_mqtt_init(void);
//MQTT关闭
void app_mqtt_close(void);
//MQTT发送数据
int app_mqtt_send(const char *json);

//注册接受远程数据的回调函数
void app_mqtt_register_callback(int (*callback)(const char *json));



#endif // __APP_MQTT_H_