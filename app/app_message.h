#ifndef __APP_MESSAGE_H_
#define __APP_MESSAGE_H_
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
//注释：字符数组转化为json json的字符串
char * app_message_chars2Json(char * chars, int len);
//注释：json转化为字符数组 存在于buffer中 buffer_size为buffer的大小，返回值为实际写入buffer的长度
int app_message_json2Chars(const char * json, char *buffer,int buffer_size);




#endif // __APP_MESSAGE_H_