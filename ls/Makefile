CC      := gcc
FLAGS   := -std=c99 -pedantic -Wall -g
HEADERS := ls.h

all: ls.o
	$(CC) -o ls ls.o
	chmod +x ls

%.o: %.c ${HEADERS}
	$(CC) -c -o $@ $< ${FLAGS}

clean:
	find -name '*.o' -delete
	find -name 'ls' -delete
