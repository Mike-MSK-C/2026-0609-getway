#include "ota_version.h"
#include "ota_http.h"
#include "cJSON/cJSON.h"
#include "log.h"
#include <openssl/evp.h>
#include <string.h>
#include <sys/reboot.h>
#include "unistd.h"
#include "stdio.h"
#include <stdlib.h>
static char *get_file_sha(char *filepath)
{
    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        perror("Failed to open file");
        return NULL;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        perror("Failed to create EVP context");
        fclose(file);
        return NULL;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha1(), NULL) != 1)
    {
        perror("Failed to initialize SHA1");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return NULL;
    }

    const int bufSize = 32768;
    unsigned char *buffer = (unsigned char *)malloc(bufSize);
    if (!buffer)
    {
        perror("Failed to allocate memory");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return NULL;
    }

    int bytesRead;
    while ((bytesRead = fread(buffer, 1, bufSize, file)) > 0)
    {
        if (EVP_DigestUpdate(mdctx, buffer, bytesRead) != 1)
        {
            perror("Failed to update hash");
            free(buffer);
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            return NULL;
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1)
    {
        perror("Failed to finalize hash");
        free(buffer);
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return NULL;
    }

    EVP_MD_CTX_free(mdctx);
    fclose(file);
    free(buffer);

    char *outputBuffer = (char *)malloc(hash_len * 2 + 1);
    if (!outputBuffer)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    for (unsigned int i = 0; i < hash_len; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }

    return outputBuffer;
}

/// @brief ota检查ota版本是否需要升级
/// @param
/// @return
int ota_version_checkUpdata(void)
{
    // 得到版本json
    char *json = ota_http_get_json(OTA_URL_FILEINFO);
    // 解析版本号 + 固件hash值
    cJSON *root = cJSON_Parse(json);

    int major = cJSON_GetObjectItem(root, "major")->valueint;
    int minor = cJSON_GetObjectItem(root, "minor")->valueint;
    int patch = cJSON_GetObjectItem(root, "patch")->valueint;

    // 输出线上版本号
    log_info("ota version %d.%d.%d", major, minor, patch);
    // 输出当前版本号
    log_info("current version %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    // 比较本地版本号 和远程版本号，确认是否需要升级
    if (major < VERSION_MAJOR || (major == VERSION_MAJOR && minor < VERSION_MINOR) || (major == VERSION_MAJOR && minor == VERSION_MINOR && patch <= VERSION_PATCH))
    {
        log_debug("当前是最新版本 ，无需跟新");
        cJSON_Delete(root);
        free(json);
        return 0;
    }

    // 确认升级后，下载固件
    int res = ota_http_download(OTA_URL_DOWNLOAD, OTA_LOCAL_FILE_PATH);
    // 下载失败，直接结束
    if (res != 0)
    {
        log_error("ota下载失败");
        cJSON_Delete(root);
        free(json);
        return -1;
    }
    // 下载成功，比较固件hash值，确认下载的固件是否正确
    log_info("ota下载成功");

    char *remote_hash = cJSON_GetObjectItem(root, "sha1")->valuestring;

    char *local_hash = get_file_sha(OTA_LOCAL_FILE_PATH);

    // 固件hash值错误 ，删除下载的固件，结束
    if (strcmp(remote_hash, local_hash) != 0)
    {
        log_error("ota下载的固件hash值错误");
        // 删除下载的固件,就是减少硬连接数
        unlink(OTA_LOCAL_FILE_PATH);
        free(local_hash);
        cJSON_Delete(root);
        free(json);
        return -1;
    }

    // 固件hash值正确，重启系统 运行新的固件

    // 这三个其实没有必要写 ，因为重启系统后，会重新加载这些值，也会自动释放掉
    // 不过当前是在测试环境，所以写上，因为当前没有root 权限 ，无法重启，不会自动释放掉资源
    free(local_hash);
    cJSON_Delete(root);
    free(json);

    log_info("ota下载的固件hash值正确,重启系统");
    // 需要root用户权限
    reboot(RB_AUTOBOOT);

    return 0;
}

/***
 * @breif ota每隔一天检查一次
 */

int ota_version_checkUpdata_day(void)
{
    // 每隔一天检查一次
    while (1)
    {
        int res = ota_version_checkUpdata();
        if (res == 0)
        {
            log_info("ota检测未发生错误，等待一天后再次检测");
        }
        sleep(24 * 60 * 60);
    }
    return -1;
}

/**
 * @breif ota输出版本号
 */

void ota_version_print(void)
{
    log_info("ota version %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}
