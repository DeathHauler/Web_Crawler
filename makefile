OBJS = code/main.cpp
CC = g++
COMPILER_FLAGS = -w $(xml2-config --cflags --libs)
LINKER_FLAGS = -lcurl -I/usr/include/libxml2 -lcrypto -lz -lxml2 -lssl
OBJ_NAME = Web_Crawler
all : compile run

compile :
	@$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

run :
	@./$(OBJ_NAME)





