objects := main.o hello.o
main: $(objects)
	gcc -o main $(objects)
	./main

main.o: main.c hello.h
	gcc -c -o main.o main.c

hello.o: hello.c hello.h
	gcc -c -o hello.o hello.c

clean:
	rm -f main $(objects)


log:= thirdparty/log.c thirdparty/log.h
ota_http:= ota/ota_http.c ota/ota_http.h
ota_http_test: /home/mneg/Desktop/gatway/test/ota_http_test.c $(log) $(ota_http)
	-$(CC) $^ -o $@ -Iapp -Iota -I thirdparty -lcurl
	-./$@
	-rm $@
