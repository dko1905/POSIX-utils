.POSIX:

include config.mk

PROGS=cat ls echo

all: $(PROGS)

.c:
	$(CC) $(MYCFLAGS) $(MYLDFLAGS) $< -o $@

clean:
	rm -f $(PROGS)

