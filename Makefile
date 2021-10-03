CC = g++
CFLAGS = -std=c++0x

all: serverA serverB servermain client

serverA: serverA.cpp
	$(CC) $(CFLAGS) -o serverA serverA.cpp

serverB: serverB.cpp
	$(CC) $(CFLAGS) -o serverB serverB.cpp

servermain: servermain.cpp
	$(CC) $(CFLAGS) -o servermain servermain.cpp

client: client.cpp
	$(CC) $(CFLAGS) -o client client.cpp	

clean:
	rm -rf serverA serverB servermain client