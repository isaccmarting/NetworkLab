client:
	gcc -o client.o -c client.c
	gcc -o client.exe client.o

server: server.o myhttp.o myhttp.h
	gcc -o server.exe server.o myhttp.o -lpthread

server.o: server.c
	gcc -o server.o -c server.c

myhttp.o: myhttp.c myhttp.h
	gcc -o myhttp.o -c myhttp.c

clean:
	rm *.o
	rm *.exe

