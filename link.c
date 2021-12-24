/* Copyright (c) 2021, Daniel Florescu */
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
		fprintf(stderr, "usage: %s to from\n", progname);                      \
		exit(1);                                                               \
	}

int main(int argc, char *argv[])
{
	int ret = 0;

	progname = argv[0];

	if (argc < 3) {
		die_usage("Not enough arguments");
	} else if (argc > 3) {
		die_usage("Too many arguments");
	}

	ret = link(argv[1], argv[2]);
	if (ret != 0) {
		die("cannot link '%s' to '%s': %s", argv[2], argv[1], strerror(errno));
	}

	return 0;
}
