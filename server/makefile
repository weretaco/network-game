CC = g++
#LIB_FLAGS = -lssl -lmysqlclient -lcrypt -lrt
LIB_FLAGS = -lmysqlclient -lcrypt -lrt -llua5.2
FLAGS = -Wall -g
COMMON_PATH = ../common
DEPENDENCIES = Common.o MessageContainer.o MessageProcessor.o Player.o WorldMap.o DataAccess.o Projectile.o Game.o GameSummary.o LuaLoader.o

gameServer : server.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS) $(LIB_FLAGS)

Common.o : $(COMMON_PATH)/Common.cpp
	$(CC) -c -o $@ $? $(FLAGS)

MessageContainer.o : $(COMMON_PATH)/MessageContainer.cpp
	$(CC) -c -o $@ $? $(FLAGS)

MessageProcessor.o : $(COMMON_PATH)/MessageProcessor.cpp
	$(CC) -c -o $@ $? $(FLAGS)

Player.o : $(COMMON_PATH)/Player.cpp
	$(CC) -c -o $@ $? $(FLAGS)

WorldMap.o : $(COMMON_PATH)/WorldMap.cpp
	$(CC) -c -o $@ $? $(FLAGS)

Projectile.o : $(COMMON_PATH)/Projectile.cpp
	$(CC) -c -o $@ $? $(FLAGS)

Game.o : $(COMMON_PATH)/Game.cpp
	$(CC) -c -o $@ $? $(FLAGS)

GameSummary.o : $(COMMON_PATH)/GameSummary.cpp
	$(CC) -c -o $@ $? $(FLAGS)

%.o : %.cpp
	$(CC) -c -o $@ $? $(FLAGS)

clean:
	rm *.o
	rm gameServer
