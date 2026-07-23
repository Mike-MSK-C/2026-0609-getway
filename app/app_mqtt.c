#include "app_mqtt.h"
#include "log.h"
#include "string.h"
#include "stdio.h"

#define ADDRESS "tcp://192.168.50.37:1883"
#define CLIENTID "b253ba38-daf6-4b37-984f-5d8fdc6a1cfa"
#define TOPIC_PULL "pull"
#define TOPIC_PUSH "push"
#define PAYLOAD "Hello World!"
#define QOS 1
#define TIMEOUT 10000L

static MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient client;                                                           // MQTT客户端对象
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer; // 连接选项结构体
static int (*receive_callback)(const char *json) =NULL;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    log_debug("Message transmit success !"); // 记录调试日志

}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{   
    int result =0;
    if(receive_callback!=NULL)
    {
        result=receive_callback((char *)message->payload)==0 ? 1:0;
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return result;
}

/**
 * @brief 连接丢失回调函数
 * @param context 上下文参数，用于传递用户自定义数据
 * @param cause 连接丢失的原因字符串
 */
void connlost(void *context, char *cause)
{
    log_error("Connection lost: %s\n", cause); // 记录错误日志
}

// MQTT初始化
int app_mqtt_init(void)
{
    // 初始化MQTT客户端
    if (MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS)
    {
        printf("MQTTClient_create failed\n");
        goto exit; // 如果创建客户端失败，则跳转到exit标签处
    }

    // 设置回调函数
    if (MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered) != MQTTCLIENT_SUCCESS)
    {
        printf("MQTTClient_setCallbacks failed\n");
        goto destroy_exit; // 如果设置回调函数失败，则跳转到destroy_exit标签处
    }

    // 连接MQTT服务器
    if (MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS)
    {
        printf("MQTTClient_connect failed\n");
        goto destroy_exit; // 如果连接服务器失败，则跳转到destroy_exit标签处
    }

    // 订阅消息
    if (MQTTClient_subscribe(client, TOPIC_PULL, QOS) != MQTTCLIENT_SUCCESS)
    {
        printf("MQTTClient_subscribe failed\n");
        MQTTClient_disconnect(client, 10000);
        goto destroy_exit; // 如果订阅消息失败，则跳转到destroy_exit标签处
    }
    
    log_debug("MQTT initialized successfully !"); // 记录调试日志
    return 0;

destroy_exit: // 如果需要取消订阅，则跳转到destroy_exit标签处
    MQTTClient_destroy(&client);
exit:
    return -1;
}

// MQTT关闭
void app_mqtt_close(void)
{
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    log_debug("MQTT closed successfully !"); // 记录调试日志
}

// MQTT发送数据
int app_mqtt_send(const char *json)
{   
    pubmsg.payload = (void *)json;
    pubmsg.payloadlen = strlen(json);
    pubmsg.qos = QOS;
    if (MQTTClient_publishMessage(client, TOPIC_PUSH, &pubmsg, NULL) != MQTTCLIENT_SUCCESS)
    {
        log_error("MQTTClient_publishMessage failed\n");
        return -1;
    }
    log_debug("MQTT send successfully !"); // 记录调试日志
    return 0;

}

// 注册接受远程数据的回调函数
void app_mqtt_register_callback(int (*callback)(const char *json))
{
    receive_callback=callback;
}