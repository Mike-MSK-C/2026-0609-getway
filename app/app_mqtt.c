#include "app_mqtt.h"
#include "log.h"
#include "string.h"
#include "stdio.h"
#include "signal.h"
#include "unistd.h"

#define ADDRESS "tcp://192.168.50.104:1883"
#define CLIENTID "b253ba38-daf6-4b37-984f-5d8fdc6a1cfa"
#define TOPIC_PULL "pull"
#define TOPIC_PUSH "push"
#define PAYLOAD "Hello World!"
#define QOS 1
#define RECONNECT_MAX_DELAY_SECONDS 8

static MQTTClient client;
static MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
static int (*receive_callback)(const char *json) = NULL;
static volatile sig_atomic_t mqtt_closing;

static int app_mqtt_connect_and_subscribe(void)
{
    int result = MQTTClient_connect(client, &conn_opts);
    if (result != MQTTCLIENT_SUCCESS)
    {
        log_error("MQTTClient_connect failed, result=%d", result);
        return -1;
    }

    result = MQTTClient_subscribe(client, TOPIC_PULL, QOS);
    if (result != MQTTCLIENT_SUCCESS)
    {
        log_error("MQTTClient_subscribe failed, result=%d", result);
        MQTTClient_disconnect(client, 1000);
        return -1;
    }

    return 0;
}

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
    int delay_seconds = 1;
    (void)context;
    log_error("Connection lost: %s", cause != NULL ? cause : "unknown");

    while (!mqtt_closing && app_mqtt_connect_and_subscribe() != 0)
    {
        log_error("MQTT reconnect failed, retrying in %d seconds", delay_seconds);
        sleep(delay_seconds);
        if (delay_seconds < RECONNECT_MAX_DELAY_SECONDS)
        {
            delay_seconds *= 2;
        }
    }

    if (!mqtt_closing)
    {
        log_info("MQTT reconnected successfully");
    }
}

// MQTT初始化
int app_mqtt_init(void)
{
    mqtt_closing = 0;

    // 初始化MQTT客户端
    if (MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS)
    {
        printf("MQTTClient_create failed\n");
        goto exit; // 如果创建客户端失败，则跳转到exit标签处
    }

    conn_opts.keepAliveInterval = 60;
    conn_opts.connectTimeout = 5;
    conn_opts.retryInterval = 0;

    // 设置回调函数
    if (MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered) != MQTTCLIENT_SUCCESS)
    {
        printf("MQTTClient_setCallbacks failed\n");
        goto destroy_exit; // 如果设置回调函数失败，则跳转到destroy_exit标签处
    }

    if (app_mqtt_connect_and_subscribe() != 0)
    {
        goto destroy_exit;
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
    mqtt_closing = 1;
    if (MQTTClient_isConnected(client))
    {
        MQTTClient_disconnect(client, 10000);
    }
    MQTTClient_destroy(&client);
    log_debug("MQTT closed successfully !"); // 记录调试日志
}

// MQTT发送数据
int app_mqtt_send(const char *json)
{   
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    if (!MQTTClient_isConnected(client))
    {
        log_error("MQTT is not connected");
        return -1;
    }

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