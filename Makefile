.POSIX:
.SUFFIXES:
.SUFFIXES: .c .zig

include config.mk

PROGS=cat ls echo false true link unlink tee id
ZPROGS=zid

all: progs
progs: $(PROGS)
zprogs: $(ZPROGS)

.c:
	$(CC) $(MYCFLAGS) $(MYLDFLAGS) $< -o $@

.zig:
	$(ZCC) $(MYZFLAGS) $<
zid: zid.zig # id.zig uses libc
	$(ZCC) $(MYZFLAGS) -lc $<

clean:
	rm -f $(PROGS) $(ZPROGS)
