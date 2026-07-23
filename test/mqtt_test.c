#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"

#define ADDRESS "tcp://192.168.50.103:1883"
#define CLIENTID "b253ba38-daf6-4b37-984f-5d8fdc6a1cfa"
#define TOPIC_PULL "pull"
#define TOPIC_PUSH "push"
#define PAYLOAD "Hello World!"
#define QOS 1
#define TIMEOUT 10000L

static MQTTClient_message pubmsg = MQTTClient_message_initializer;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char *)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

/**
 * @brief 连接丢失回调函数
 * @param context 上下文参数，用于传递用户自定义数据
 * @param cause 连接丢失的原因字符串
 */
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");  // 打印连接丢失提示信息
    printf("     cause: %s\n", cause);  // 打印连接丢失的具体原因
}

/*
 * 主函数：创建MQTT客户端，设置回调函数，连接服务器，订阅主题，
 * 发布消息，断开连接并销毁客户端
 */
int main(int argc, char *argv[])
{
    MQTTClient client;           // MQTT客户端对象
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer; // 连接选项结构体
    int rc;                     // 返回码，用于判断操作是否成功

    // 创建MQTT客户端
    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID,
                                MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto exit;              // 如果创建失败，跳转到exit标签处
    }

    // 设置回调函数
    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;       // 如果设置回调失败，跳转到destroy_exit标签处
    }

    // 连接MQTT服务器
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;       // 如果连接失败，跳转到destroy_exit标签处
    }

    // 订阅主题
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n",
           TOPIC_PULL, CLIENTID, QOS);
    if ((rc = MQTTClient_subscribe(client, TOPIC_PULL, QOS)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to subscribe, return code %d\n", rc);
        rc = EXIT_FAILURE;
    }
    else
    {
        int ch;
        // 等待用户输入'Q'或'q'退出
        do
        {
            ch = getchar();
        } while (ch != 'Q' && ch != 'q');

        // 取消订阅
        if ((rc = MQTTClient_unsubscribe(client, TOPIC_PULL)) != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to unsubscribe, return code %d\n", rc);
            rc = EXIT_FAILURE;
        }
    }

    // 发布消息
    pubmsg.payload = PAYLOAD;
    pubmsg.payloadlen = (int)strlen(PAYLOAD);
    pubmsg.qos = QOS;
    if ((rc = MQTTClient_publishMessage(client, TOPIC_PUSH, &pubmsg, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to publish message, return code %d\n", rc);
        rc = EXIT_FAILURE;
    }

    // 断开连接
    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to disconnect, return code %d\n", rc);
        rc = EXIT_FAILURE;
    }
    return rc;

destroy_exit:                   // 销毁客户端标签
    MQTTClient_destroy(&client);
exit:                           // 退出标签
    return rc;
}
