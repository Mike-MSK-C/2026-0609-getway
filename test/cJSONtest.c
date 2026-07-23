#include "cJSON/cJSON.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[] )
{   //1.生成json字符串
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", "zhangsan");
    cJSON_AddNumberToObject(root, "age", 18);
    char * str= cJSON_PrintUnformatted(root);
    log_debug("json:%s",str);
    //2.解析字符串
    cJSON *json = cJSON_Parse(str);
    if (json==NULL)
    {
        log_error("json parse error");
        return -1;
    }
    char * name= cJSON_GetObjectItem(json, "name")->valuestring;
    int age = cJSON_GetObjectItem(json, "age")->valueint;
    log_debug("name:%s,age:%d",name,age);
    //3.释放内存
    free(str);
    cJSON_Delete(json);
    cJSON_Delete(root);
}