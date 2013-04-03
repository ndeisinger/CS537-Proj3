CC=gcc
FLAGS=-c -ggdb3 --std=gnu99 -Wall -Werror
TAGS=ctags

raidsim: main.o disk.o disk-array.o raid0.o raid10.o raid4.o
	$(CC) main.o disk.o disk-array.o raid0.o raid10.o raid4.o -o raidsim
	$(TAGS) -R
	
debug: main.o disk.o disk-array.o raid0.o
	$(CC) main.o disk.o disk-array.o raid0.o raid10.o -d -o raidsim
	$(TAGS) -R
	
main.o: main.c
	$(CC) $(FLAGS) main.c -o main.o

disk.o: disk.c

disk-array.o: disk-array.c
	$(CC) $(FLAGS) disk-array.c -o disk-array.o
	
raid0.o: raid0.c
	$(CC) $(FLAGS) raid0.c -o raid0.o

raid10.o: raid10.c
	$(CC) $(FLAGS) raid10.c -o raid10.o
	
raid4.o: raid4.c
	$(CC) $(FLAGS) raid4.c -o raid4.o

clean:
	rm -f *.o raidsim
