/*
 * File: pc_file.h
 * Usage: interface for pc file reading and writing
 *
 * Copyright (C) 1996 all original source by Jim Tripp (Tenebrius), Mark
 * Tripp (Nessalin), Hal Roberts (Bram), and Mike Zeman (Azroen).  May 
 * favourable winds fill the sails of their skimmers.
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 *
 *                       Version 4.0  Generation 2 
 * ---------------------------------------------------------------------
 *
 * Original DikuMUD 1.0 courtesy of Katja Nyboe, Tom Madsen, Hans
 * Staerfeldt, Michael Seifert, and Sebastian Hammer.  Kudos.
 */


struct file_obj_list
{
    char *label;
    int ref;
    struct char_field_data *stats;
    struct char_field_data *stats_last;
    struct file_obj_list *next;
};

struct char_field_data
{
    char *field;
    char *val;
    struct char_field_data *next;
};

struct obj_ptr_table
{
    struct obj_data *obj;
    int ref;
    struct obj_ptr_table *next;
};

struct char_ptr_table
{
    struct char_data *ch;
    int ref;
    struct char_ptr_table *next;
};

struct char_file_data
{
    FILE *fd;
    struct char_data *ch;
    char *name;
    char opened;
    char mode;
    char *dir;
    struct obj_ptr_table *obj_table;
    struct char_ptr_table *char_table;
    struct file_obj_list *file;
    struct file_obj_list *file_last;
};

struct char_file_data current;

struct obj_data *obj_from_ref(int ref);
int ref_from_obj(struct obj_data *obj);
int add_obj_to_table(struct obj_data *obj);
struct char_data *char_from_ref(int ref);
int ref_from_char(struct char_data *ch);
int add_char_to_table(struct char_data *ch);
void init_tables(void);
void free_tables(void);
void change_char_dir(char *dir);
int open_char_file(struct char_data *ch, char *dir);
void free_file_obj_list(void);
int close_char_file(void);
struct file_obj_list *find_label(char *label, int ref);
struct char_field_data *find_field(char *label, int ref, char *field);
#ifndef SWIG
int write_char_field_f(char *label, int ref, char *field, char *val, ...)
    __attribute__ ((format(printf, 4, 5)));
#else
int write_char_field_f(char *label, int ref, char *field, char *val, ...);
#endif

char *minus_brackets(char *s);
struct file_obj_list *read_file_label(char **line, int *pos);
struct char_field_data *read_file_field(char **line, int *pos);
int read_char_file(void);
int read_char_field_f(char *label, int ref, char *field, char *val, ...);
int read_char_field_num(char *label, int ref, char *field);

/* end basic file stuff */

