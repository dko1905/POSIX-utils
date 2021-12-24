.POSIX:

include config.mk

PROGS=cat ls echo false true

all: $(PROGS)

.c:
	$(CC) $(MYCFLAGS) $(MYLDFLAGS) $< -o $@

clean:
	rm -f $(PROGS)

