# Linux
CC=g++
LIBS= -lglut -lGLEW -lGL -lassimp `Magick++-config --cppflags --cxxflags --ldflags --libs` -I/usr/include/bullet -lBulletDynamics -lBulletSoftBody -lBulletCollision -lLinearMath -lGLU

# Compiler flags
CXXFLAGS= -g -Wall -std=c++0x

all: ../bin/table.exe

../bin/table.exe: ../src/main.cpp
	$(CC) $(CXXFLAGS) ../src/main.cpp -o ../bin/table.exe $(LIBS)

clean: 
	  rm -f ../bin/table.exe
