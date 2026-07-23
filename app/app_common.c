#include "app_common.h"
#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
/**
 * 获取当前时间的毫秒数
 * @return 返回从1970年1月1日00:00:00到现在的毫秒数
 */
long app_common_getCurrentTime(void)
{
    struct timeval tv;  // 定义时间结构体变量，用于存储秒和微秒
    gettimeofday(&tv, NULL);  // 获取当前时间，保存在tv结构体中
    //转化为毫秒
    return tv.tv_sec *1000 + tv.tv_usec / 1000;
}

char * app_common_chars2hex(char *chars,int len)
{   
    char *hexstr= malloc(len*2+1);
    if(hexstr == NULL)
    {   
        return NULL;
    }
    //清空字符串
    memset(hexstr,0,len*2+1);
    //将字符数组转换为十六进制字符串
    for(int i=0;i<len;i++)
    {
        sprintf(hexstr+i*2,"%02x",chars[i]);
    }
    //添加字符串结束符
    hexstr[len*2]='\0';
    //返回十六进制字符串
    return hexstr;
}

char * app_common_hex2chars(char *hex ,int* len)
{   

    //计算十六进制字符串的长度
    int hexlen = strlen(hex);
    //计算字符数组的长度
    *len = hexlen/2;
    //分配字符数组的空间
    char *chars = malloc(*len);
    if(chars == NULL)
    {
        return NULL;
    }
    //两个16进制字符转换为一个char字符
    for(int i=0;i<hexlen;i+=2)
    {
        sscanf(hex+i,"%2hhx",&chars[i/2]);
    }
    
    return chars;
    
}