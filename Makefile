.PHONY: all clean

CC=g++

LDFLAGS=-lcv -lhighgui -lX11
TARGET=track
SOURCE=$(TARGET).cpp
HEADERS=config.h

all: $(TARGET)

$(TARGET): $(SOURCE) config.h
	$(CC) -o $(TARGET) $(CFLAGS) $(SOURCE) $(LDFLAGS)

clean:
	rm -f *~ *.o $(TARGET)
