CC = g++
#LIB_FLAGS = -lssl -lmysqlclient -lcrypt -lrt
LIB_FLAGS = -lmysqlclient -lcrypt -lrt
FLAGS = $(LIB_FLAGS)
COMMON_PATH = ../common
DEPENDENCIES = Common.o MessageContainer.o MessageProcessor.o Player.o WorldMap.o DataAccess.o Projectile.o Game.o GameSummary.o

server : server.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS)

Common.o : $(COMMON_PATH)/Common.cpp
	$(CC) -c -o $@ $?

MessageContainer.o : $(COMMON_PATH)/MessageContainer.cpp
	$(CC) -c -o $@ $?

MessageProcessor.o : $(COMMON_PATH)/MessageProcessor.cpp
	$(CC) -c -o $@ $?

Player.o : $(COMMON_PATH)/Player.cpp
	$(CC) -c -o $@ $?

WorldMap.o : $(COMMON_PATH)/WorldMap.cpp
	$(CC) -c -o $@ $?

Projectile.o : $(COMMON_PATH)/Projectile.cpp
	$(CC) -c -o $@ $?

Game.o : $(COMMON_PATH)/Game.cpp
	$(CC) -c -o $@ $?

GameSummary.o : $(COMMON_PATH)/GameSummary.cpp
	$(CC) -c -o $@ $?

%.o : %.cpp
	$(CC) -c -o $@ $?

clean:
	rm *.o
	rm server
