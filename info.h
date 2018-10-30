/*
 * File: info.h */
#include "core_structs.h"

/* HELP_DATA:  These are the fields stored in a help_data.
 */
struct help_data
{
  char *topic;
  char *category;
  char *body;
  char *syntax;
  char *notes;
  char *delay;
  char *see_also;
  char *example;
}
;

void free_help_data(HELP_DATA *help_entry);
HELP_DATA *create_help_data(void);
void reset_help_data(HELP_DATA *help_entry);

void show_char_to_char(CHAR_DATA * tmp_char, CHAR_DATA * ch, int i);
void gather_population_stats();
const char *indefinite_article(const char *str);


