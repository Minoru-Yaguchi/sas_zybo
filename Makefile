INC := -I./include/ -I./include_usr/
OPT := -Wall -O2 -lm -ljpeg -std=c++0x
GCC := g++
SRC := ./src/main.c ./src/msgQLib.cpp ./src/range.c ./src/motor.c ./src/cameramain.cpp ./src/hog_soft.cpp ./src/camera.cpp
REF := ./lib/libAtrjlnx.a ./lib/libjpeg.a
TARGET := sas

all:$(TARGET)

$(TARGET):$(SRC)
	$(GCC) $(OPT) $(SRC) $(REF) $(INC) -o $(TARGET)

clean:
	$(RM) $(TARGET)  

