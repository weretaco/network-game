CC = g++
LIB_FLAGS = -lssl -lmysqlclient
FLAGS = $(LIB_FLAGS)
COMMON_PATH = ../common
DEPENDENCIES = Message.o Player.o DataAccess.o

server : server.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS)

Message.o : $(COMMON_PATH)/Message.cpp
	$(CC) -c -o $@ $?

%.o : %.cpp
	$(CC) -c -o $@ $?

clean:
	rm *.o
	rm server
