# Linux
CC=g++
LIBS= -lglut -lGLEW -lGL

# Compiler flags
CXXFLAGS= -g -Wall -std=c++0x

all: ../bin/Matrix

../bin/Matrix: ../src/main.cpp
	$(CC) $(CXXFLAGS) ../src/main.cpp -o ../bin/Matrix $(LIBS)
