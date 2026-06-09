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