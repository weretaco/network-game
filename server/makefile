CC = g++
LIB_FLAGS = -lssl -lmysqlclient
FLAGS = $(LIB_FLAGS)
COMMON_PATH = ../common
DEPENDENCIES = Common.o Message.o Player.o DataAccess.o

server : server.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS)

Common.o : $(COMMON_PATH)/Common.cpp
	$(CC) -c -o $@ $?

Message.o : $(COMMON_PATH)/Message.cpp
	$(CC) -c -o $@ $?

Player.o : $(COMMON_PATH)/Player.cpp
	$(CC) -c -o $@ $?

%.o : %.cpp
	$(CC) -c -o $@ $?

clean:
	rm *.o
	rm server
