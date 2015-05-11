CFLAGS  = -Wall -O2 -g -std=gnu99 -pedantic
LDFLAGS = -pthread
PROGN   = temper2csv

USB_FLAGS = $(shell pkg-config --cflags libusb-1.0)
USB_LIBS  = $(shell pkg-config --libs libusb-1.0)

.PHONY: all clean

all: bin/$(PROGN)


bin/$(PROGN): obj/libtemper.o obj/main.o
	$(CC) $(LDFLAGS) -o $@ $+ $(USB_LIBS)
	
obj/main.o: src/main.c
	$(CC) $(CFLAGS) -DPROGN=\"$(PROGN)\" -c -o $@ $<

obj/libtemper.o: src/libtemper.c src/libtemper.h
	$(CC) $(CFLAGS) $(USB_FLAGS) -c -o $@ $<


clean:
	rm -f bin/$(PROGN) obj/*.o
