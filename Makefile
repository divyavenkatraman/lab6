CC=gcc

all: segfault meltdown module1

meltdown: meltdown.c
	$(CC) meltdown.c -o meltdown 

module1: module1.c
	$(CC) module1.c -o module1 

segfault: segfault.c
	$(CC) segfault.c -o segfault

clean:
	rm -rf  module1 meltdown segfault