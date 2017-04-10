client: FrameRule.o client.o
	gcc -o client.exe client.o FrameRule.o -lpthread

server:FrameRule.o server.o
	gcc -o server.exe server.o FrameRule.o -lpthread

client.o: client.c
	gcc -o client.o -c client.c

server.o: server.c
	gcc -o server.o -c server.c

FrameRule.o: FrameRule.c
	gcc -o FrameRule.o -c FrameRule.c

db_server:
	gcc -o server.exe -g server.c FrameRule.c -lpthread

clean:
	rm *.o
	rm *.exe

