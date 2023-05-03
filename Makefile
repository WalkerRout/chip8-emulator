UNAME = $(shell uname)

CC = gcc
OBJS = src/*.c
OBJ = bin/chip8
CFLAGS = -Wall -Wextra -Wshadow #ALL WARNINGS!

ifeq ($(UNAME), Linux)
  LIBS = -lSDL2main -lSDL2
  INCS = -I"include/"
else
  LIBS = -L"C:/Development/SDL2_MinGW_64Bit/lib" -lmingw32 -lSDL2main -lSDL2
  INCS = -I"include/" -I"C:/Development/SDL2_MinGW_64Bit/include"
endif

all: build run

build:
	@$(CC) $(OBJS) $(CFLAGS) $(LIBS) $(INCS) -o $(OBJ)

run: build
	@./$(OBJ)

clean:
	@rm ./$(OBJ)
	@echo "Cleaned!"
