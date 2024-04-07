TARGET = bg
LIBS = -lraylib
CC = gcc
CFLAGS = -Iinclude -Llib -g -fsanitize=undefined -fsanitize=address -Wl,-rpath=lib

ifeq ($(OS), Windows_NT)
	CFLAGS = -msse2 -Iinclude -Llib/windows -lole32 -lcomctl32 -lcomdlg32 -loleaut32 -luuid -lpthread -lopengl32 -lgdi32 -lwinmm -lm
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S), Darwin)
		CFLAGS = -Iinclude -Llib -g -L`pwd`/lib -Xlinker -rpath -Xlinker `pwd`/lib
	endif
endif

.PHONY = default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.hpp)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) $(CFLAGS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
