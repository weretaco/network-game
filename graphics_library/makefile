CC = g++
LIB_FLAGS = -lGL -lglut -lGLEW `pkg-config glfw3 --static --cflags --libs`
FLAGS = -Wall
DEPENDENCIES = common/texture.o common/controls.o common/shader.o

graphics_engine : main.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS) $(LIB_FLAGS)

%.o : %.cpp
	$(CC) -c -o $@ $? $(FLAGS)

clean:
	rm -f *.o
	rm -f common/*.o
	rm -f graphics_engine
