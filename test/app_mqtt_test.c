#include "app_mqtt.h"
#include "log.h"
#include "unistd.h"
int app_mqtt_recv(const char *json)
{   
    printf("=== app_mqtt_recv 被调用 ===\n");
    log_debug("app_mqtt_recv: %s", json);
    return 0;
}
int main(int argc, char *argv[])
{
    app_mqtt_init();

    app_mqtt_register_callback(app_mqtt_recv);

    app_mqtt_send("{\"conn_type\":1,\"id\":\"5858\",\"msg\":\"61626364\"}");

    sleep(50);

    app_mqtt_close();

    return 0;
}