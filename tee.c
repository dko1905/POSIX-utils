/* Copyright (c) 2021, Daniel Florescu */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define INITIAL_SIZE 3

static char *progname = "";

#define die(...)                                                                       \
	{                                                                              \
		fprintf(stderr, "%s: ", progname);                                     \
		fprintf(stderr, __VA_ARGS__);                                          \
		fprintf(stderr, "\n");                                                 \
		exit(1);                                                               \
	}
#define warn(...)                                                                      \
	{                                                                              \
		fprintf(stderr, "%s: ", progname);                                     \
		fprintf(stderr, __VA_ARGS__);                                          \
		fprintf(stderr, "\n");                                                 \
	}

static bool a_flag = false;

static int tee(const char **name_arr, FILE **fp_arr, size_t fp_len);

int main(int argc, char *argv[])
{
	FILE **fp_arr = NULL;
	size_t fp_len = 0;
	const char **name_arr = NULL;
	int ch = 0, code = 0;

	progname = argv[0];

	while ((ch = getopt(argc, argv, "ai")) != -1) {
		switch (ch) {
		case 'a':
			a_flag = true;
			break;
		case 'i':;
			sigset_t mask;
			sigaddset(&mask, SIGINT);
			sigprocmask(SIG_BLOCK, &mask, NULL);
			break;
		default:
			fprintf(stderr, "usage: %s [-ai] [file ...]\n", progname);
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	/* +1 Count stdin. */
	fp_len = argc + 1;
	fp_arr = calloc(fp_len, sizeof(FILE*));
	if (fp_arr == NULL)
		die("failed to allocate: %s", strerror(errno));
	name_arr = calloc(fp_len, sizeof(char*));
	if (name_arr == NULL)
		die("failed to allocate: %s", strerror(errno));

	/* Open files. */
	name_arr[0] = "stdout";
	fp_arr[0] = stdout;
	setvbuf(stdout, NULL, _IONBF, 0); /* Disable buffering. */
	for (size_t i = 1; i < fp_len; i++) {
		name_arr[i] = argv[i - 1];
		fp_arr[i] = fopen(argv[i - 1], a_flag ? "a" : "w");
		if (fp_arr[i] == NULL) {
			warn("%s: %s", argv[i - 1], strerror(errno));
			i--; /* Start at same index as last. */
			fp_len--; /* One less file opened. */
			argv[i - 1] = NULL; /* Hide name. */
			continue;
		}
		setvbuf(fp_arr[i], NULL, _IONBF, 0); /* Disable buffering. */
	}

	/* R/W loop. */
	code = tee(name_arr, fp_arr, fp_len);

	/* Cleanup */
	for (size_t i = 1; i < fp_len; i++) {
		fclose(fp_arr[i]);
		fp_arr[i] = NULL;
	}
	free(fp_arr);

	return code;
}

static int tee(const char **name_arr, FILE **fp_arr, size_t fp_len)
{
	char buffer[BUFSIZ] = {0};
	size_t bytes = 0, failed_writes = 0;
	int code = 0;

	/* R/W loop */
	while ((bytes = fread(buffer, 1, sizeof(buffer), stdin)) != 0) {
		failed_writes = 0;
		for (size_t i = 0; i < fp_len; i++) {
			if (fwrite(buffer, 1, bytes, fp_arr[i]) != bytes) {
				warn("%s: %s", name_arr[i], strerror(errno));
				failed_writes++;
			}
		}
		if (failed_writes == fp_len) {
			die("failed to write to all");
		}
	}

	if (ferror(stdin))
		die("stdin: %s", strerror(errno));

	return code;
}

