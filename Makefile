.POSIX:
.SUFFIXES:
.SUFFIXES: .zig

include config.mk

PROGS=id

all: progs
progs: $(PROGS)

.zig:
	$(ZCC) $(MYZFLAGS) $<
id: id.zig # id.zig uses libc
	$(ZCC) $(MYZFLAGS) -lc $<

clean:
	rm -f $(PROGS) $(ZPROGS)
