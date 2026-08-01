#ifndef OTA_VERSION_H_
#define OTA_VERSION_H_

#define VERSION_MAJOR 3 // 主版本号
#define VERSION_MINOR 0 // 次版本号
#define VERSION_PATCH 0 // 修订版本号

/// @brief ota检查ota版本是否需要升级
/// @param
/// @return
int ota_version_checkUpdata(void);

/***
 * @breif ota每隔一天检查一次
 */

int ota_version_checkUpdata_day(void);

/**
 * @breif ota输出版本号
 */

void ota_version_print(void);

#endif // OTA_VERSION_H_