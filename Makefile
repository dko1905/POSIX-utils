.POSIX:

include config.mk

PROGS=cat ls echo false true link unlink tee

all: $(PROGS)

.c:
	$(CC) $(MYCFLAGS) $(MYLDFLAGS) $< -o $@

clean:
	rm -f $(PROGS)
