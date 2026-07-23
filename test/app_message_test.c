#include "app_message.h"
#include "log.h"

int main(void) {
    char buffer[100]={0};
    int len=0;
    char *json_message= "{\"conn_type\":1,\"id\":\"5858\",\"msg\":\"61626364\"}";
    //json消息转换为字符数组消息
    len= app_message_json2Chars(json_message,buffer,100);
    //可以明显看到char没办法显示某些字符，只有16进制才能显示所有字符
    log_debug("buffer:%s",buffer);
    log_debug("len:%d",len);

    //字符数组消息转化为json消息
    char * json_str= app_message_chars2Json(buffer,len);
    log_debug("json_str:%s",json_str);

    free(json_str);
    return 0;
}