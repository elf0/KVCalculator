CC := g++
CFLAGS := -std=c++11 -O3
LDFLAGS :=

all: kvc lazy

kvc: main.o
	$(CC) $(LDFLAGS) -o kvc main.o

main.o: main.cpp KVReader.h
	$(CC) $(CFLAGS) -c main.cpp

lazy: lazy.o
	$(CC) $(LDFLAGS) -o lazy lazy.o

lazy.o: lazy.cpp KVReader.h
	$(CC) $(CFLAGS) -c lazy.cpp

clean:
	rm -rf *.o
	rm kvc lazy
