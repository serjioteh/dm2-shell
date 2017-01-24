CC=g++
CFLAGS=-std=c++11
LDFLAGS=-std=c++11
SOURCES=main.cpp Entity.cpp

all: main

main: $(SOURCES)
	$(CC) $(LDFLAGS) -o shell $(SOURCES)

clean:
	rm -f *.o shell file* ./tests/*.run.*

