.POSIX:

include config.mk

all: cat ls

cat: cat.c
	$(CC) $(MYCFLAGS) $(MYLDFLAGS) $< -o $@
ls: ls.c
	$(CC) $(MYCFLAGS) $(MYLDFLAGS) $< -o $@

clean:
	rm -f cat ls

