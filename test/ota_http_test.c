#include "ota_http.h"
#include "log.h"
#include <stdlib.h>
int main(int argc, char *argv[])
{
    char * json = ota_http_get_json(OTA_URL_FILEINFO);
    log_debug("json: %s", json);
    free(json);

    int res = ota_http_download(OTA_URL_DOWNLOAD,OTA_LOCAL_FILE_PATH);
    if (res != 0)
    {
        log_error("download failed");
        return -1;
    }
    log_debug("download success");
    
    return 0;
}
