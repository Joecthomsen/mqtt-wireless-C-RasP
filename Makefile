# CXX		  := /bin/arm-linux-gnueabihf-g++
# CXX_FLAGS := -Wall -Wextra -std=c++17 -ggdb -lmosquitto
# BIN		:= bin
# SRC		:= src
# INCLUDE	:= include
# LIB		:= lib

# LIBRARIES	:=
# EXECUTABLE	:= main


# all: $(BIN)/$(EXECUTABLE)

# run: clean all
# 	clear
# 	./$(BIN)/$(EXECUTABLE)

# $(BIN)/$(EXECUTABLE): $(SRC)/*.cpp
# 	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES)

# clean:
# 	-rm $(BIN)/*
 
# _*_ MakeFile _*_

src/app: src/*.o
	arm-linux-gnueabihf-g++ main.o -o app -l m -l mosquitto -l pthread

src/main.o: src/*.cpp
	arm-linux-gnueabihf-g++ -c main.cpp
