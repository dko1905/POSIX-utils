.POSIX:
.SUFFIXES:
.SUFFIXES: .c

include config.mk

PROGS=cat ls echo false true link unlink tee id

all: progs
progs: $(PROGS)

.c:
	$(CC) $(MYCFLAGS) $(MYLDFLAGS) $< -o $@

clean:
	rm -f $(PROGS)
