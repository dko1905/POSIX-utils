/* Copyright (c) 2021, Jannis Adamek */
#include <fts.h>
#include <getopt.h>
#include <grp.h>
#include <math.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define LONG_LISTING_OPT 'l'
#define SHOW_ALL_OPT 'a'

typedef struct {
	bool long_listing;
	bool show_all;
} Options;

typedef struct {
	int user_width;
	int group_width;
	int size_width;
} LonglistingWidths;

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

static int compare_files_by_name(const FTSENT **ftsent_1,
				 const FTSENT **ftsent_2)
{
	return strcmp((*ftsent_1)->fts_name, (*ftsent_2)->fts_name);
}

static int number_of_digits(size_t num)
{
	int digits = 1;
	while (num > 9) {
		num /= 10;
		digits++;
	}
	return digits;
}

static void calculate_widths(FTSENT *first_file, LonglistingWidths *widths)
{
	widths->user_width = 0;
	widths->group_width = 0;

	int biggest_file_size = 0;
	FTSENT *current = first_file;
	while (current != NULL) {
		if (current->fts_statp->st_size > biggest_file_size)
			biggest_file_size = current->fts_statp->st_size;

		struct passwd *user = getpwuid(current->fts_statp->st_uid);
		struct group *group = getgrgid(current->fts_statp->st_gid);
		size_t user_len = strlen(user->pw_name);
		size_t group_len = strlen(group->gr_name);
		if ((signed)user_len > widths->user_width)
			widths->user_width = user_len;
		if ((signed)group_len > widths->group_width)
			widths->group_width = group_len;
		current = current->fts_link;
	}
	widths->size_width = number_of_digits(biggest_file_size);
}

// Usual UNIX file desciption, e.g "drwxrwxr-x"
static void set_file_description(struct stat *file_stat, char *file_description)
{
	if (S_ISDIR(file_stat->st_mode))
		file_description[0] = 'd';
	else if (S_ISBLK(file_stat->st_mode))
		file_description[0] = 'b';
	else if (S_ISCHR(file_stat->st_mode))
		file_description[0] = 'c';
	else if (S_ISFIFO(file_stat->st_mode))
		file_description[0] = 'p';
	else if (S_ISLNK(file_stat->st_mode))
		file_description[0] = 'l';
	else if (S_ISREG(file_stat->st_mode))
		file_description[0] = '-';
	else if (S_ISSOCK(file_stat->st_mode))
		file_description[0] = 's';
	else
		file_description[0] = '?';

	file_description[1] = (file_stat->st_mode & S_IRUSR) ? 'r' : '-';
	file_description[2] = (file_stat->st_mode & S_IWUSR) ? 'w' : '-';
	file_description[3] = (file_stat->st_mode & S_IXUSR) ? 'x' : '-';
	file_description[4] = (file_stat->st_mode & S_IRGRP) ? 'r' : '-';
	file_description[5] = (file_stat->st_mode & S_IWGRP) ? 'w' : '-';
	file_description[6] = (file_stat->st_mode & S_IXGRP) ? 'x' : '-';
	file_description[7] = (file_stat->st_mode & S_IROTH) ? 'r' : '-';
	file_description[8] = (file_stat->st_mode & S_IWOTH) ? 'w' : '-';
	file_description[9] = (file_stat->st_mode & S_IXOTH) ? 'x' : '-';
}

static void print_file(FTSENT *file, LonglistingWidths *widths, Options *options)
{
	if (options->long_listing) {
		struct stat *file_stat = file->fts_statp;
		char file_description[11];
		set_file_description(file_stat, file_description);

		struct passwd *user = getpwuid(file_stat->st_uid);
		struct group *group = getgrgid(file_stat->st_gid);

		printf("%s %zu %*s %*s %*zu %s\n", file_description,
		       file_stat->st_nlink, widths->user_width, user->pw_name, widths->group_width,
		       group->gr_name, widths->size_width, file_stat->st_size, file->fts_name);
	} else {
		printf("%s\n", file->fts_name);
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
	fts = fts_open(fts_args, FTS_LOGICAL | FTS_NOCHDIR | FTS_SEEDOT,
		       &compare_files_by_name);

	// Read the first file from the path, which is the path itself
	fts_read(fts);
	// Get all files within that folder
	FTSENT *current = fts_children(fts, 0);

	LonglistingWidths widths;
	if (options->long_listing) {
		calculate_widths(current, &widths);
	}

	while (current != NULL) {
		switch (current->fts_info) {
		case FTS_DOT:
			if (options->show_all) {
				print_file(current, &widths, options);
			}
			break;
		case FTS_D:
		case FTS_F:
			print_file(current, &widths, options);
			break;
		}
		current = current->fts_link;
	}

	fts_close(fts);
	free(cwd_buffer);
}

int main(int argc, char **argv)
{
	Options options;
	init_options(&options);
	parse_args(argc, argv, &options);
	list_files(&options);
	return EXIT_SUCCESS;
}