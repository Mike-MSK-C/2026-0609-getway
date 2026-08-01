CC:=gcc
CFLAGS:=-g -Wall
log:= thirdparty/log.c thirdparty/log.h
logtest: test/logtest.c $(log)
	-$(CC) -o $@ $^ -I thirdparty -lpthread
	-./$@
	-rm $@

# $^表示所有依赖文件
# $@表示目标文件
# -I表示头文件路径

json:=thirdparty/cJSON/cJSON.h thirdparty/cJSON/cJSON.c
cJSONtest: test/cJSONtest.c $(json) $(log)
	-$(CC) $(CFLAGS) -o $@ $^ -I thirdparty -lpthread

#	-./$@
#	-rm $@
# -lpthread表示链接pthread库
# -I表示头文件路径
# -o表示输出文件名

app_common:=app/app_common.c app/app_common.h
app_common_test: test/app_common_test.c $(app_common) $(log)
	-$(CC) -o $@ $^ -I app -I thirdparty -lpthread
	-./$@
	-rm $@


# app_message json chars转化测试
app_message:= app/app_message.c app/app_message.h
app_message_test: test/app_message_test.c $(app_common) $(app_message) $(log) $(json)
	-$(CC) -o $@ $^ -I app -I thirdparty -lpthread
	-./$@
	-rm $@


# mqtt测试
mqtt_test: test/mqtt_test.c
	-$(CC) $^ -o $@ -lpaho-mqtt3c
	-./$@
	-rm $@
# -lpaho-mqtt3c表示链接paho-mqtt3c库
# -l指定链接库

app_mqtt:= app/app_mqtt.c app/app_mqtt.h
app_mqtt_test: test/app_mqtt_test.c  $(app_mqtt) $(log)
	-$(CC) -o $@ $^ -I app -I thirdparty -lpaho-mqtt3c -lpthread
	-./$@
	-rm $@
# -lpthread表示链接pthread库
# -I表示头文件路径
# -o表示输出文件名
# -l指定链接库

app_pool:= app/app_pool.c app/app_pool.h
app_pool_test: test/app_pool_test.c  $(log) $(app_pool)
	-$(CC) -o $@ $^ -I app -I thirdparty  -lpthread
	-./$@
	-rm $@
# -lpthread表示链接pthread库
# -I表示头文件路径
# -o表示输出文件名

app_buffer:= app/app_buffer.c app/app_buffer.h
app_buffer_test: test/app_buffer_test.c  $(log) $(app_buffer)
	-$(CC) -o $@ $^ -I app -I thirdparty  -lpthread
	-./$@
	-rm $@
# -lpthread表示链接pthread库
# -I表示头文件路径
# -o表示输出文件名


app_device:=app/app_device.c app/app_device.h
app_bt:=app/app_bt.c app/app_bt.h
app_device_test: test/app_device_test.c $(app_device) $(log) $(app_buffer) $(app_message) $(app_bt) $(app_common) $(json) $(app_pool) $(app_mqtt) 
	-$(CC) -o $@ $^ -Ithirdparty -Iapp -lpaho-mqtt3c
	-./$@
	-rm $@
# -lpthread表示链接pthread库
# -I表示头文件路径
# -o表示输出文件名

ota_http:= ota/ota_http.c ota/ota_http.h
ota_http_test: test/ota_http_test.c $(ota_http) $(log)
	-$(CC) $^ -o $@ -Iapp -Iota -I thirdparty -lcurl
	-./$@
	-rm $@


ota_version:= ota/ota_version.c ota/ota_version.h
ota_version_test: test/ota_version_test.c $(ota_version) $(json) $(ota_http) $(log)
	-$(CC) $^ -o $@ -Iapp -Iota -I thirdparty -lcurl -lcrypto -lssl
	-./$@
	-rm $@
# -lpthread表示链接pthread库
# -I表示头文件路径
# -o表示输出文件名


app_serial:= app/app_serial.c app/app_serial.h
app_runner:= app/app_runner.c app/app_runner.h
daemon_sub_process:= daemon/daemon_sub_process.c daemon/daemon_sub_process.h
daemon_runner:= daemon/daemon_runner.c daemon/daemon_runner.h

IPATHS := -Ithirdparty -Iapp -Iota -Idaemon
LLIBS := -lpaho-mqtt3c -lcurl -lcrypto -lssl -lpthread

OBJS := $(app_common) $(log) $(json) $(app_message) $(app_mqtt) $(app_buffer) \
		$(app_pool) $(app_device) $(app_bt) $(app_serial) $(app_runner) \
		$(ota_http) $(ota_version) $(daemon_sub_process) $(daemon_runner)

gatway_test: test/gatway_test.c $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(IPATHS) $(LLIBS)

.PHONY: run
run: gatway_test
	./gatway_test daemon

.PHONY: clean
clean:
	rm -f gatway_test
