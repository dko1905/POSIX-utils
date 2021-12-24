#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* The OpenBSD version uses usefull BSD functions that we don't have,
 * we must therefore recreate some, like progname. */
static char *progname = "";

#define die(...) { \
	fprintf(stderr, "%s: ", progname); \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, "\n"); \
	exit(1); \
}

static void cat(const char *name, FILE *fp);

int main(int argc, char *argv[])
{
	FILE *fp = NULL;
	int ch = 0;

	progname = argv[0];

	/* Parse flags. */
	while ((ch = getopt(argc, argv, "u")) != -1) {
		switch (ch) {
		case 'u':
			setvbuf(stdout, NULL, _IONBF, 0);
			break;
		case '-':
			goto break_getopt;
		default:
			fprintf(stderr, "usage: %s [-u] [file ...]\n", progname);
			return 1;
		}

		argc--;
		argv++;
	}
	/* Remove progname from argc & argv. */
	argc--;
	argv++;
	break_getopt:

	if (argc == 0) {
		cat("stdin", stdin);
		return 0;
	}

	for (; *argv != NULL; argv++) {
		/* Open file or stdin. */
		if (0 == strcmp("-", *argv)) {
			fp = stdin;
		} else {
			fp = fopen(*argv, "r");
			if (fp == NULL)
				die("%s: %s", *argv, strerror(errno));
		}

		cat(*argv, fp);
	}

	return 0;
}

static void cat(const char *name, FILE *fp)
{
	char buffer[BUFSIZ] = {0};
	size_t bytes = 0;

	/* R/W loop */
	while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) != 0) {
		if (fwrite(buffer, 1, bytes, stdout) != bytes) {
			die("stdout: %s", strerror(errno));
		}
	}

	if (ferror(fp))
		die("%s: %s", name, strerror(errno));
	if (fp != stdin)
		fclose(fp);
}
