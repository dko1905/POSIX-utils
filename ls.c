/* Copyright (c) 2021, Jannis Adamek */
#include <fts.h>
#include <getopt.h>
#include <grp.h>
#include <linux/limits.h>
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

#define TIME_STR_LEN 100 /* Length of the time-date string in the long listing. */

/* ls options*/
#define OPTIONS_STR "laC1"
#define LONG_LISTING_OPT 'l'
#define SHOW_ALL_OPT 'a'
#define USE_COLUMNS 'C'
#define USE_INDIVIDUAL_LISTING '1'

typedef struct {
	bool long_listing;
	bool show_all;
	bool use_columns;
} Options;

typedef struct {
	char **path_array; /* Array of all paths that should be listed */
	int paths_count;
} Paths;

/*
 * If neccessary, ls loops through the files twice.
 * Once the first to collect data like the total block count and the maximum widths
 * of certain elements in the table and once to actually list out the files.
 */
typedef struct {
	int user_width;
	int group_width;
	int size_width;
	int total_block_count;
} FirstRun;

static void usage(void) { fprintf(stderr, "Usage: ls [-l -a] [files...]\n"); }

static void parse_args(int argc, char **argv, Options *options, Paths *paths)
{
	options->show_all = false;
	options->long_listing = false;
	options->use_columns = true;

	int opt;
	while ((opt = getopt(argc, argv, OPTIONS_STR)) != -1) {
		switch (opt) {
		case SHOW_ALL_OPT:
			options->show_all = true;
			break;
		case LONG_LISTING_OPT:
			options->long_listing = true;
			break;
		case USE_COLUMNS:
			options->use_columns = true;
			break;
		case USE_INDIVIDUAL_LISTING:
			options->use_columns = false;
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}

	// Populate paths with an array of absolute paths
	int number_of_pos_args = argc - optind;
	if (number_of_pos_args == 0) {
		paths->path_array = (char **)malloc(sizeof(char *) * 2);
		paths->path_array[0] = realpath(".", NULL);
		paths->path_array[1] = NULL;
		paths->paths_count = 1;
	} else {
		paths->path_array =
		    (char **)malloc(sizeof(char *) * number_of_pos_args + 1);
		int option_index = 0;
		int arg_index = optind;
		while (arg_index < argc) {
			paths->path_array[option_index] =
			    realpath(argv[arg_index], NULL);
			arg_index++;
			option_index++;
		}
		paths->paths_count = number_of_pos_args;
	}
}

static void free_paths_array(Paths *paths)
{
	for (int i = 0; i < paths->paths_count; i++) {
		free(paths->path_array[i]);
	}
	free(paths->path_array);
}

/*
 * This can be passed to fts_open() to determine the order in which fts poccesses files.
 */
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
	if (!options->long_listing) {
		printf("%s\n", file->fts_name);
		return;
	}
	struct stat *file_stat = file->fts_statp;
	char file_description[11];
	set_file_description(file_stat, file_description);

	struct passwd *user = getpwuid(file_stat->st_uid);
	struct group *group = getgrgid(file_stat->st_gid);

	char time_str[TIME_STR_LEN];
	get_time_str(file, time_str);

	if (first_run == NULL) {
		printf("%s %zu %*s %*s %*zu %s %s\n", file_description,
		       file_stat->st_nlink, 0, user->pw_name, 0, group->gr_name, 0,
		       file_stat->st_size, time_str, file->fts_name);
		return;
	}
	printf("%s %zu %*s %*s %*zu %s %s\n", file_description, file_stat->st_nlink,
	       first_run->user_width, user->pw_name, first_run->group_width,
	       group->gr_name, first_run->size_width, file_stat->st_size, time_str,
	       file->fts_name);
}

static void list_files(Options *options, Paths *paths)
{
	int fts_options = FTS_LOGICAL | FTS_NOCHDIR | FTS_SEEDOT;
	bool previous_was_not_a_dir = false;
	for (int i = 0; i < paths->paths_count; i++) {
		FTS *fts;
		char *fts_args[] = {paths->path_array[i], NULL};
		fts = fts_open(fts_args, fts_options, &compare_files_by_name);

		FTSENT *parent = fts_read(fts);

		if (!S_ISDIR(parent->fts_statp->st_mode)) {
			print_file(parent, NULL, options);
			previous_was_not_a_dir = true;
			continue;
		}

		if (previous_was_not_a_dir)
			printf("\n");
		printf("%s:\n", paths->path_array[i]);

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

		if (i + 1 < paths->paths_count) {
			printf("\n");
		}
	}
}

int main(int argc, char **argv)
{
	Options options;
	Paths paths;
	parse_args(argc, argv, &options, &paths);
	list_files(&options, &paths);
	free_paths_array(&paths);
	return EXIT_SUCCESS;
}