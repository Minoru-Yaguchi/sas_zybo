INC := -I./include/ -I./include_usr/ -I./
OPT := -Wall -O0 -lm -std=c++0x -lpthread -lrt -L./lib -lcurl -lssl -lcrypto -lz -laws-cpp-sdk-core -laws-cpp-sdk-s3 -laws-c-common -laws-c-event-stream -laws-checksums
GCC := arm-linux-gnueabihf-g++
SRC := ./src/main.c ./src/msgQLib.cpp ./src/range.c ./src/motor.c ./src/cameramain.cpp ./src/hog_soft.cpp ./src/camera.cpp ./src/fpgadrv.c ./src/aws.cpp
REF := ./lib/libAtrjlnx.a ./lib/libjpeg.a
TARGET := sas

all:$(TARGET)

$(TARGET):$(SRC)
	$(GCC) $(OPT) $(SRC) $(REF) $(INC) -o $(TARGET)

clean:
	$(RM) $(TARGET)  

