CC=gcc
FLAGS=-c -ggdb3 --std=gnu99 -Wall -Werror
TAGS=ctags -R

raidsim: main.o disk.o disk-array.o raid0.o
	$(CC) main.o disk.o disk-array.o raid0.o -o raidsim
	$(TAGS)
	
debug: main.o disk.o disk-array.o raid0.o
	$(CC) main.o disk.o disk-array.o raid0.o -d -o raidsim
	$(TAGS)
	
main.o: main.c
	$(CC) $(FLAGS) main.c -o main.o

disk.o: disk.c

disk-array.o: disk-array.c
	$(CC) $(FLAGS) disk-array.c -o disk-array.o
	
raid0.o: raid0.c
	$(CC) $(FLAGS) raid0.c -o raid0.o

clean:
	rm -f *.o raidsim
