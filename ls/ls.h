#ifndef LS_H
#define LS_H

// Possible options:
#define LONG_LISTING_OPT 'l'
#define SHOW_ALL_OPT     'a'

#include <stdbool.h>

typedef struct {
    bool long_listing;
    bool show_all;
} Options;

void init_options(Options *options);

#endif