#ifndef __APP_COMMON_H_
#define __APP_COMMON_H_
#include "sys/time.h"
/*
返回时间戳
*/
long app_common_getCurrentTime(void);

//返回字符数组，长度为len 16进制
char * app_common_chars2hex(char *chars,int len);
//返回字符数组，长度为len 16进制转换为字符
char * app_common_hex2chars(char *hex,int *len);

#endif // __APP_COMMON_H_