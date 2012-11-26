CC = g++
LIB_FLAGS = -lssl -lmysqlclient
FLAGS = $(LIB_FLAGS)
COMMON_PATH = ../common
DEPENDENCIES = message.o player.o

server : server.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS)

message.o : $(COMMON_PATH)/message.cpp
	$(CC) -c -o $@ $?

%.o : %.cpp
	$(CC) -c -o $@ $?

clean:
	rm *.o
	rm server
