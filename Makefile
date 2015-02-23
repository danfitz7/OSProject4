CC=gcc
CFLAGS=-lpthread -lrt -std=gnu99
DEPS = api.h Boolean.h 
OBJ = api.o Error.c

all: api

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

intersection: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm api *.o