CC = g++
LIB_FLAGS = `pkg-config --cflags --libs --static allegro-static-5.0 allegro_ttf-static-5.0 allegro_primitives-static-5.0`
FLAGS = $(LIB_FLAGS)
COMMON_PATH = ../common
DEPENDENCIES = Common.o MessageProcessor.o Player.o WorldMap.o Projectile.o chat.o GuiComponent.o Window.o Textbox.o Button.o RadioButtonList.o TextLabel.o

gameClient : Client/main.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS)

Common.o : $(COMMON_PATH)/Common.cpp
	$(CC) -c -o $@ $? $(FLAGS)

MessageProcessor.o : $(COMMON_PATH)/MessageProcessor.cpp
	$(CC) -c -o $@ $? $(FLAGS)

Player.o : $(COMMON_PATH)/Player.cpp
	$(CC) -c -o $@ $? $(FLAGS)

WorldMap.o : $(COMMON_PATH)/WorldMap.cpp
	$(CC) -c -o $@ $? $(FLAGS)

Projectile.o : $(COMMON_PATH)/Projectile.cpp
	$(CC) -c -o $@ $? $(FLAGS)

%.o : Client/%.cpp
	$(CC) -c -o $@ $? $(FLAGS)

clean:
	rm *.o
	rm gameClient
