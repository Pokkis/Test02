CC = gcc
CPP = g++
AR = ar
TARGET = libcommon.a

SOURCES = $(wildcard *.cpp)  $(wildcard *.c)
OBJS = $(patsubst %.cpp,%.o, $(SOURCES)) $(patsubst %.c,%.o, $(SOURCES))
INCLUDE = -I./
#CFLAGS = -Wall -Os
CFLAGS = -Wall -g
CFLAGS += $(INCLUDE)

all : $(TARGET)

$(TARGET) : $(OBJS) 
	$(AR) -r $(TARGET) $^
	
.c.o:
	$(CPP)  $(CFLAGS) -c -o $@ $<
.cpp.o:
	$(CPP) $(CFLAGS) -c -o $@ $<
	
.PHONY = clean
	
clean : 
	rm -rf *.o $(TARGET)