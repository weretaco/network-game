CC = g++
LIB_FLAGS = -lGL -lglut -lGLEW -lglfw
FLAGS = -Wall
DEPENDENCIES =

graphics_engine : main.cpp $(DEPENDENCIES)
	$(CC) -o $@ $+ $(FLAGS) $(LIB_FLAGS)

%.o : %.cpp
	$(CC) -c -o $@ $? $(FLAGS)

clean:
	rm graphics_engine
	rm *.o
