# Linux
CC=g++
LIBS= -lglut -lGLEW -lGL -lassimp `Magick++-config --cppflags --cxxflags --ldflags --libs`

# Compiler flags
CXXFLAGS= -g -Wall -std=c++0x

all: ../bin/Matrix.exe

../bin/Matrix.exe: ../src/main.cpp
	$(CC) $(CXXFLAGS) ../src/main.cpp -o ../bin/Matrix.exe $(LIBS)
