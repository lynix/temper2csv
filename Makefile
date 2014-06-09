CFLAGS  = -Wall -O2 -g -std=gnu99 -pedantic
LDFLAGS = -pthread
PROGN   = temper2csv

.PHONY: all clean

all: bin/$(PROGN)


bin/$(PROGN): obj/libtemper.o obj/main.o
	$(CC) $(LDFLAGS) `pkg-config --libs libusb-1.0` -o $@ $+
	
obj/main.o: src/main.c
	$(CC) $(CFLAGS) -DPROGN=\"$(PROGN)\" -c -o $@ $<

obj/libtemper.o: src/libtemper.c src/libtemper.h
	$(CC) $(CFLAGS) `pkg-config --cflags libusb-1.0` -c -o $@ $<


clean:
	rm -f bin/$(PROGN) obj/*.o
