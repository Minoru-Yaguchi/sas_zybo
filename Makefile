INC := -I./include/ -I./include_usr/ -I./
OPT := -Wall -O0 -lm -std=c++0x -lpthread -lrt
GCC := arm-linux-gnueabihf-g++
SRC := ./src/main.c ./src/msgQLib.cpp ./src/range.c ./src/motor.c ./src/cameramain.cpp ./src/hog_soft.cpp ./src/camera.cpp ./src/fpgadrv.c
REF := ./lib/libAtrjlnx.a ./lib/libjpeg.a
TARGET := sas

all:$(TARGET)

$(TARGET):$(SRC)
	$(GCC) $(OPT) $(SRC) $(REF) $(INC) -o $(TARGET)

clean:
	$(RM) $(TARGET)  

