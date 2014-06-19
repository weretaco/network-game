CC = g++
#LIB_FLAGS = `pkg-config --cflags --libs --static allegro-static-5.0 allegro_main-static-5.0 allegro_ttf-static-5.0 allegro_primitives-static-5.0`
# osx needs to link against some extra libraries
LIB_FLAGS = `pkg-config --cflags --libs --static allegro-static-5.0 allegro_main-static-5.0 allegro_ttf-static-5.0 allegro_primitives-static-5.0` -framework AppKit -framework IOKit -framework OpenGL -framework AGL -framework OpenAL
FLAGS = -Wall -g
COMMON_PATH = ../common
DEPENDENCIES = Common.o MessageContainer.o MessageProcessor.o Player.o WorldMap.o Projectile.o Game.o GameRender.o GameSummary.o chat.o GuiComponent.o Window.o Textbox.o Button.o RadioButtonList.o TextLabel.o

gameClient : Client/main.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS) $(LIB_FLAGS)

%.o : Client/%.cpp
	$(CC) -c -o $@ $? $(FLAGS)

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

clean:
	rm -f *.o
	rm -f gameClient
