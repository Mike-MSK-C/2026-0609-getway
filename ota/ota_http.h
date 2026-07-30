#ifndef __OTA_HTTP_H_
#define __OTA_HTTP_H_


#define OTA_URL_FILEINFO "http://192.168.50.104:8000/fileinfo.json"
#define OTA_URL_DOWNLOAD "http://192.168.50.104:8000/download/gateway"

#define OTA_LOCAL_FILE_PATH "/home/mneg/gateway.update"
/**
 * 请求指定的URL，获取json数据
 */

char * ota_http_get_json(const char *url);

/**
 * 请求指定RUL，下载到指定路径
 */

 int ota_http_download(const char *url, const char *file_path);

#endif // __OTA_HTTP_H_