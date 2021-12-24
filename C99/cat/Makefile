.POSIX:

include ../config.mk

all: cat

cat: main.c
	$(CC) $(MYCFLAGS) $(MYLDFLAGS) $< -o $@

clean:
	rm -f cat

