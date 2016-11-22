CC := g++
CFLAGS := -std=c++11 -O3
LDFLAGS :=

all: kvc

kvc: main.o
	$(CC) $(LDFLAGS) -o kvc main.o

main.o: main.cpp KVReader.h
	$(CC) $(CFLAGS) -c main.cpp

clean:
	rm -rf *.o
	rm kvc
