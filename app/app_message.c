#include "app_message.h"
#include "cJSON/cJSON.h"
#include "log.h"
#include "app_common.h"

//len 表示数据的长度
char *app_message_chars2Json(char *chars, int len)
{
    // 强转无符号字节，防止值为负算错长度
    int conn_type = (unsigned char)chars[0];
    int id_len = (unsigned char)chars[1];
    int msg_len = (unsigned char)chars[2];
    
    // 计算协议需要的最小长度
    int need_min_len = 3 + id_len + msg_len;
    // 只判断不够长，多余字节直接忽略，不再严格相等
    if (len < need_min_len)
    {
        log_error("len too short, need:%d, recv:%d", need_min_len, len);
        return NULL;
    }

    char id[id_len + 1];
    memcpy(id, chars + 3, id_len);
    id[id_len] = '\0';
    
    char msg[msg_len + 1];
    memcpy(msg, chars + 3 + id_len, msg_len);
    msg[msg_len] = '\0';

    // 生成16进制 id字符串 msg字符串
    char *idhex = app_common_chars2hex(id, id_len);
    char *msghex = app_common_chars2hex(msg, msg_len);

    // 生成json
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "conn_type", conn_type);
    cJSON_AddStringToObject(root, "id", idhex);
    cJSON_AddStringToObject(root, "msg", msghex);

    char *json = cJSON_PrintUnformatted(root);
    // 释放内存
    cJSON_Delete(root);
    free(idhex);
    free(msghex);
    return json;
}

int app_message_json2Chars(const char *json, char *buffer, int buffer_size)
{
    // 解析JSON 得到 conn_type idhex msghex
    cJSON *root = cJSON_Parse(json);
    int conn_type = cJSON_GetObjectItem(root, "conn_type")->valueint;
    char *idhex = cJSON_GetObjectItem(root, "id")->valuestring;
    char *msghex = cJSON_GetObjectItem(root, "msg")->valuestring;
    // 将16进制的idhex msghex 转换为字符串 id msg
    int id_len = 0;
    int msg_len = 0;
    char *id = app_common_hex2chars(idhex, &id_len);
    char *msg = app_common_hex2chars(msghex, &msg_len);
    // 判断长度是否足够
    if (buffer_size < 3 + id_len + msg_len)
    {
        free(id);
        free(msg);
        log_error("buffer size too short");
        return -1;
    }
    // 拼接我们的协议格式 conn_type id_len msg_len id msg 存放在buffer中
    buffer[0] = conn_type;
    buffer[1] = id_len;
    buffer[2] = msg_len;
    memcpy(buffer + 3, id, id_len);
    memcpy(buffer + 3 + id_len, msg, msg_len);
    // 释放内存
    free(id);
    free(msg);
    cJSON_Delete(root);

    // 返回拼接的长度
    return 3 + id_len + msg_len;
}