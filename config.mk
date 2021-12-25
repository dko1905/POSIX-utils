
#CC=c99
ZCC=zig build-exe

MYCPPFLAGS = -D_POSIX_C_SOURCE=200112L
MYCFLAGS = -std=c99 -Wall -Wextra -pedantic \
		   $(MYCPPFLAGS) $(CPPFLAGS) $(CFLAGS)
MYLDFLAGS = $(LDFLAGS)

MYZFLAGS = $(ZFLAGS) # -O ReleaseSafe --strip
