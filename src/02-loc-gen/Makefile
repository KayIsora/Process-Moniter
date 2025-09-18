CC=gcc
CFLAGS=-Wall -g

all: loc_gen

loc_gen: room_manager.o data_collector.o main.o
	$(CC) $(CFLAGS) -o loc_gen room_manager.o data_collector.o main.o

room_manager.o: room_manager.c room_manager.h data_collector.h
	$(CC) $(CFLAGS) -c room_manager.c

data_collector.o: data_collector.c data_collector.h
	$(CC) $(CFLAGS) -c data_collector.c

main.o: main.c room_manager.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f *.o loc_gen
