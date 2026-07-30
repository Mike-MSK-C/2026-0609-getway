#include "ota_http.h"
#include "log.h"
#include "curl/curl.h"
#include <stdlib.h>
#include <string.h>
/**
 * 请求指定的URL，获取json数据
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total_size = size * nmemb;
    char *json_buff = (char *)userp;
    memcpy(json_buff, contents, total_size);
    json_buff[total_size] = '\0';
    return total_size;
}

char *ota_http_get_json(const char *url)
{
    CURL *curl = curl_easy_init();

    // 配置curl

    curl_easy_setopt(curl, CURLOPT_URL, url);
    // 设置返回数据的回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    // 设置返回数据的回调函数的参数
    char *json_buff = (char *)malloc(1024);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, json_buff);

    // 执行请求
    CURLcode res = curl_easy_perform(curl);
    // 检查请求是否成功
    if (res != CURLE_OK)
    {
        log_error("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        // 释放curl
        curl_easy_cleanup(curl);
        return NULL;
    }
    // 释放curl
    curl_easy_cleanup(curl);
    return json_buff;
}

/**
 * 请求指定RUL，下载到指定路径
 */

int ota_http_download(const char *url, const char *file_path)
{
    CURL *curl = curl_easy_init();

    // 配置curl

    curl_easy_setopt(curl, CURLOPT_URL, url);
    // 设置返回数据的回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    // 设置返回数据的回调函数的参数
    FILE *file = fopen(file_path, "wb");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    // 执行请求
    CURLcode res = curl_easy_perform(curl);
    // 检查请求是否成功
    if (res != CURLE_OK)
    {
        log_error("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        // 释放curl
        curl_easy_cleanup(curl);
        fclose(file);
        return -1;
    }
    // 释放curl
    curl_easy_cleanup(curl);
    fclose(file);
    return 0;
}