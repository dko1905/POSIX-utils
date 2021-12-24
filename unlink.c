#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *progname = "";

#define die(...)                                                                       \
	{                                                                              \
		fprintf(stderr, "%s: ", progname);                                     \
		fprintf(stderr, __VA_ARGS__);                                          \
		fprintf(stderr, "\n");                                                 \
		exit(1);                                                               \
	}
#define die_usage(...)                                                                 \
	{                                                                              \
		fprintf(stderr, "%s: ", progname);                                     \
		fprintf(stderr, __VA_ARGS__);                                          \
		fprintf(stderr, "\n");                                                 \
		fprintf(stderr, "usage: %s file\n", progname);                         \
		exit(1);                                                               \
	}

int main(int argc, char *argv[])
{
	int ret = 0;

	progname = argv[0];

	if (argc < 2) {
		die_usage("Not enough arguments");
	} else if (argc > 2) {
		die_usage("Too many arguments");
	}

	ret = unlink(argv[1]);
	if (ret != 0) {
		die("cannot unlink '%s': %s", argv[1], strerror(errno));
	}

	return 0;
}
