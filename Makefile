.PHONY: all clean install uninstall

CC=g++

LDFLAGS=-lcv -lcvaux -lcxcore -lhighgui -lX11
TARGET=opencv-slideshow
SOURCE=$(TARGET).cpp
HEADERS=config.h

all: $(TARGET)

$(TARGET): $(SOURCE) config.h
	$(CC) -o $(TARGET) $(CFLAGS) $(SOURCE) $(LDFLAGS)

clean:
	rm -f *~ *.o $(TARGET)

install: $(TARGET)
	strip $(TARGET)
	test -d $(DESTDIR)/usr/bin/ || mkdir -p $(DESTDIR)/usr/bin/
	cp $(TARGET) $(DESTDIR)/usr/bin/
	test -d $(DESTDIR)/usr/share/opencv-slideshow/ || mkdir -p $(DESTDIR)/usr/share/opencv-slideshow/
	cp $(TARGET).svg $(DESTDIR)/usr/share/opencv-slideshow/
	test -d $(DESTDIR)/usr/share/applications/ || mkdir -p $(DESTDIR)/usr/share/applications/
	cp $(TARGET).desktop $(DESTDIR)/usr/share/applications/
	update-desktop-database || true

uninstall:
	rm -f $(DESTDIR)/usr/bin/$(TARGET)
	rm -f $(DESTDIR)/usr/share/applications/$(TARGET).desktop
	rm -fr $(DESTDIR)/usr/share/opencv-slideshow/
	update-desktop-database || true
