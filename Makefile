CFLAGS  = -Wall -O2 -g -std=gnu99 -pedantic
LDFLAGS = -pthread
PROGN   = temper2csv

USB_FLAGS = $(shell pkg-config --cflags libusb-1.0)
USB_LIBS  = $(shell pkg-config --libs libusb-1.0)

.PHONY: all clean

all: bin/$(PROGN)


bin/$(PROGN): obj/main.o libtemper1/lib/libtemper1.a
	$(CC) $(LDFLAGS) -o $@ $+ $(USB_LIBS)
	
obj/main.o: src/main.c
	$(CC) $(CFLAGS) -DPROGN=\"$(PROGN)\" -I libtemper1/include -c -o $@ $<

libtemper1/lib/libtemper1.a: libtemper1
	cd libtemper1 && make


clean:
	rm -f bin/$(PROGN) obj/*.o
