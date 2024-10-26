OBJS = code/main.cpp
CC = g++
COMPILER_FLAGS = -w
LINKER_FLAGS = -lcurl
OBJ_NAME = Web_Crawler
all : compile run

compile :
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

run :
	./$(OBJ_NAME)



