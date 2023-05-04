CC=gcc
CFLAGS=-Wall -fPIC -shared -D_GNU_SOURCE
LDFLAGS=-ldl

all: hider.so

hider.so: hider.o
	$(CC) $(CFLAGS) hider.o -o hider.so $(LDFLAGS)

clean:
	rm hider.so hider.o
