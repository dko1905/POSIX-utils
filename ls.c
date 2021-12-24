/* Copyright (c) 2021, horenso */
#include <fts.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LONG_LISTING_OPT 'l'
#define SHOW_ALL_OPT 'a'

typedef struct {
	bool long_listing;
	bool show_all;
} Options;

static void init_options(Options *options)
{
	options->show_all = false;
	options->long_listing = false;
}

static void usage(void) { fprintf(stderr, "Usage: ls [-l -a] [files]\n"); }

static void parse_args(int argc, char **argv, Options *options)
{
	int opt;
	while ((opt = getopt(argc, argv, "al")) != -1) {
		switch (opt) {
		case SHOW_ALL_OPT:
			options->show_all = true;
			break;
		case LONG_LISTING_OPT:
			options->long_listing = true;
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}
}

static void list_files(Options *options)
{
	FTS *fts;

	// getcwd() allocates a buffer with an adequate length buffer if NULL is
	// supplied
	char *cwd_buffer = NULL;
	cwd_buffer = getcwd(NULL, 0);

	// fts_open expects a null-terminated list of paths
	char *fts_args[] = {cwd_buffer, NULL};
	fts = fts_open(fts_args, FTS_COMFOLLOW | FTS_PHYSICAL, NULL);

	FTSENT *current;
	while ((current = fts_read(fts)) != NULL) {
		switch (current->fts_info) {
		case FTS_D:
			printf("%s/\n", current->fts_name);
			break;
		case FTS_F:
			printf("%s\n", current->fts_name);
			break;
		}
	}

	free(cwd_buffer);
}

int main(int argc, char **argv)
{
	Options options;
	parse_args(argc, argv, &options);
	list_files(&options);
	return EXIT_SUCCESS;
}