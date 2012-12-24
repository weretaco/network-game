CC = g++
LIB_FLAGS = `pkg-config --cflags --libs --static allegro-static-5.0 allegro_ttf-static-5.0 allegro_primitives-static-5.0`
FLAGS = $(LIB_FLAGS)
COMMON_PATH = ../common
DEPENDENCIES = Message.o chat.o GuiComponent.o Window.o Textbox.o Button.o

gameClient : Client/main.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS)

Message.o : $(COMMON_PATH)/Message.cpp
	$(CC) -c -o $@ $? $(FLAGS)

%.o : Client/%.cpp
	$(CC) -c -o $@ $? $(FLAGS)

clean:
	rm *.o
	rm gameClient
