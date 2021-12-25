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
#include <time.h>
#include <unistd.h>

#define TIME_STR_LEN 100

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
	int total_block_count;
} FirstRun;

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

static int compare_files_by_name(const FTSENT **ftsent_1, const FTSENT **ftsent_2)
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

static bool should_skip_file(FTSENT *file, Options *options)
{
	if (file->fts_info == FTS_DOT && !options->show_all) {
		return true;
	}
	if (file->fts_name[0] == '.' && !options->show_all) {
		return true;
	}
	return false;
}

static void do_first_run(FTSENT *first_file, FirstRun *first_run, Options *options)
{
	first_run->user_width = 0;
	first_run->group_width = 0;
	first_run->total_block_count = 0;

	int biggest_file_size = 0;
	FTSENT *current = first_file;
	while (current != NULL) {
		if (should_skip_file(current, options)) {
			current = current->fts_link;
			continue;
		}
		if (current->fts_statp->st_size > biggest_file_size)
			biggest_file_size = current->fts_statp->st_size;

		struct passwd *user = getpwuid(current->fts_statp->st_uid);
		struct group *group = getgrgid(current->fts_statp->st_gid);
		size_t user_len = strlen(user->pw_name);
		size_t group_len = strlen(group->gr_name);
		if ((signed)user_len > first_run->user_width)
			first_run->user_width = user_len;
		if ((signed)group_len > first_run->group_width)
			first_run->group_width = group_len;
		first_run->total_block_count += (current->fts_statp->st_blocks / 2);

		// fprintf(stderr, "%s: %zu\n", current->fts_name,
		// 	current->fts_statp->st_blocks / 2);

		current = current->fts_link;
	}
	first_run->size_width = number_of_digits(biggest_file_size);
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
	file_description[10] = '\0';
}

static void get_time_str(FTSENT *file, char *time_str_buffer)
{
	time_t raw_time = file->fts_statp->st_mtime;
	struct tm *time_info = localtime(&raw_time);
	strftime(time_str_buffer, TIME_STR_LEN, "%b %d %R", time_info);
}

static void print_file(FTSENT *file, FirstRun *first_run, Options *options)
{
	if (options->long_listing) {
		struct stat *file_stat = file->fts_statp;
		char file_description[11];
		set_file_description(file_stat, file_description);

		struct passwd *user = getpwuid(file_stat->st_uid);
		struct group *group = getgrgid(file_stat->st_gid);

		char time_str[TIME_STR_LEN];
		get_time_str(file, time_str);

		printf("%s %zu %*s %*s %*zu %s %s\n", file_description,
		       file_stat->st_nlink, first_run->user_width, user->pw_name,
		       first_run->group_width, group->gr_name, first_run->size_width,
		       file_stat->st_size, time_str, file->fts_name);
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

	FirstRun first_run;
	if (options->long_listing) {
		do_first_run(current, &first_run, options);
		printf("total %d\n", first_run.total_block_count);
	}

	while (current != NULL) {
		if (!should_skip_file(current, options)) {
			print_file(current, &first_run, options);
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