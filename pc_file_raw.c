/*
 * File: pc_file_raw.c
 * Usage: implementation of routines to save pc's to disk
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

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


#include "structs.h"
#include "guilds.h"
#include "cities.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "event.h"
#include "pc_file.h"
#include "gamelog.h"

extern char *first_name(char *namelist, char *into);

/* begin object and character table routines.  These routines
   associate a unqiue integer with each object and a unique integer
   for each character. */

struct obj_data *
obj_from_ref(int ref)
{

    struct obj_ptr_table *p;

    for (p = current.obj_table->next; p; p = p->next)
        if (p->ref == ref)
            return p->obj;

    return NULL;
}

int
ref_from_obj(struct obj_data *obj)
{

    struct obj_ptr_table *p;

    for (p = current.obj_table->next; p; p = p->next)
        if (p->obj == obj)
            return p->ref;

    return 0;
}

int
add_obj_to_table(struct obj_data *obj)
{

    int ref = 0;
    struct obj_ptr_table *new;

    while (obj_from_ref(++ref));


    new = (struct obj_ptr_table *) malloc(sizeof *new);
    new->obj = obj;
    new->ref = ref;
    new->next = current.obj_table->next;
    current.obj_table->next = new;

    return ref;
}

CHAR_DATA *
char_from_ref(int ref)
{

    struct char_ptr_table *p;

    for (p = current.char_table->next; p; p = p->next)
        if (p->ref == ref)
            return p->ch;

    return NULL;
}

int
ref_from_char(CHAR_DATA * ch)
{

    struct char_ptr_table *p;

    for (p = current.char_table->next; p; p = p->next)
        if (p->ch == ch)
            return p->ref;

    return 0;
}

int
add_char_to_table(CHAR_DATA * ch)
{

    int ref = 0;
    struct char_ptr_table *new;

    while (char_from_ref(++ref));


    new = (struct char_ptr_table *) malloc(sizeof *new);
    new->ch = ch;
    new->ref = ref;
    new->next = current.char_table->next;
    current.char_table->next = new;

    return ref;
}

void
init_tables(void)
{

    current.obj_table = (struct obj_ptr_table *) malloc(sizeof *current.obj_table);
    current.obj_table->next = NULL;
    current.char_table = (struct char_ptr_table *) malloc(sizeof *current.char_table);
    current.char_table->next = NULL;
}

void
free_tables(void)
{

    struct obj_ptr_table *o;
    struct char_ptr_table *c;

    while ((o = current.obj_table)) {
        current.obj_table = current.obj_table->next;
        free(o);
    }
    while ((c = current.char_table)) {
        current.char_table = current.char_table->next;
        free(c);
    }

    current.obj_table = NULL;
    current.char_table = NULL;

    return;
}

/* end object and character table routines */

/* begin primitive file routines */

void
change_char_dir(char *dir)
{

    if (!current.opened) {
        fprintf(stderr, "change_char_dir: Attempt to change dir without an open character.");
        return;
    }

    free(current.dir);
    current.dir = strdup(dir);

    return;
}


/* open character file and initialize the current structure */
int
open_char_file(CHAR_DATA * ch, char *dir)
{
    if (!(ch->name) || !strlen(ch->name)) {
        fprintf(stderr, "open_char_file: Tried to open nameless character");

        return 0;
    }


    if (current.opened) {
        fprintf(stderr,
                "open_char_file: tried to open char '%s' before closing %s first.", ch->name,
                current.name);
        return (0);
    }

    current.ch = ch;
    current.name = strdup(ch->name);
    current.opened = 1;

    init_tables();

    current.file = NULL;
    current.dir = strdup(dir);

    return 1;
}


void
free_file_obj_list(void)
{

    struct file_obj_list *f;
    struct char_field_data *p;

    while ((f = current.file)) {
        current.file = current.file->next;
        if (f->label)
            free(f->label);
        while ((p = f->stats)) {
            f->stats = f->stats->next;
            if (p->field)
                free(p->field);
            if (p->val)
                free(p->val);
            free(p);
        }
        free(f);
    }
    current.file = NULL;

}

/* close the character file and free up the necessary structures */
int
close_char_file(void)
{

    if (!current.opened)
        return 1;

    free(current.name);
    free(current.dir);

    free_file_obj_list();

    free_tables();

    current.opened = 0;

    return 1;
}

struct file_obj_list *
find_label(char *label, int ref)
{

    struct file_obj_list *o;

    for (o = current.file; o; o = o->next)
        if (!strcmp(o->label, label) && (o->ref == ref))
            break;

    return o;
}

struct char_field_data *
find_field(char *label, int ref, char *field)
{

    struct file_obj_list *o;
    struct char_field_data *p;

    if (!(o = find_label(label, ref)))
        return NULL;

    for (p = o->stats; p; p = p->next)
        if (!strcmp(p->field, field))
            break;

    return p;
}

/* print_safe_string()
 *
 * Takes in a string and removes any unprintable characters from it.
 * Returns the string as a convenience.
 */
char *
print_safe_string(char *val)
{
    int i;

    for (i = 0; i < strlen(val); i++) {
        if (!isprint(val[i]) && !isspace(val[i])) {
            memmove(&val[i], &val[i + 1], strlen(val) - i);
        }
    }
    return val;
}

void
write_char_file(void)
{

    char fn[256], buf[256];
    char *brac;
    struct file_obj_list *f;
    struct char_field_data *p;
    FILE *fd;
    int i;

    if (!current.opened) {
        gamelogf("write_char_file: Tried to write without first opening a character");
        return;
    }

    first_name(current.ch->name, buf);
    for (i = 0; i < strlen(buf); i++)
        buf[i] = tolower(buf[i]);
    snprintf(fn, sizeof(fn), "%s/%s", current.dir, buf);

    if (!(fd = fopen(fn, "w"))) {
        gamelogf("write_char_file: Unable to open char file for '%s'", fn);
        return;
    }

    for (f = current.file; f; f = f->next) {
        fprintf(fd, "{*%s*}\n%d\n", f->label, f->ref);
        for (p = f->stats; p; p = p->next) {
            /* replace '{'s with '['s to prevent field name / value
             * confusion when reading the file */
            if (p->val) {
                while ((brac = strstr(p->val, "{")))
                    *brac = '[';
                while ((brac = strstr(p->val, "}")))
                    *brac = ']';
            }

            fprintf(fd, "\t{*%s*}\n\t%s\n", p->field, p->val);
        }
    }

    fclose(fd);

    chmod(fn,
          S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

    return;
}

/* write a character or object label to file.  Should be used to start
   a character or object section of the file and asociate the item
   with a reference number */
struct file_obj_list *
write_char_label(char *label, int ref)
{

    struct file_obj_list *new;

    if (!current.opened) {
        fprintf(stderr, "write_char_label: Tried to write without first opening the file");
        return 0;
    }

    new = (struct file_obj_list *) malloc(sizeof *new);
    new->label = strdup(label);
    new->ref = ref;
    new->next = NULL;
    new->stats = NULL;
    if (!current.file) {
        current.file = new;
        current.file_last = new;
    } else {
        current.file_last->next = new;
        current.file_last = new;
    }

    return new;
}

/* write a field and value to struct.  Should be used for saving the
   stats of an object or character started with a label */

int
write_char_field(char *label, int ref, char *field, char *val)
{

    struct char_field_data *new;
    struct file_obj_list *o;

    if (!current.opened) {
        fprintf(stderr, "write_char_field: Tried to write without first opening the file");
        return 0;
    }

    if (!(o = find_label(label, ref))) {
        if (!(o = write_char_label(label, ref))) {
            fprintf(stderr, "write_char_field: Unable to find label %s, ref %d", label, ref);
            return 0;
        }
    }

    if ((new = find_field(label, ref, field))) {
        free(new->val);
        new->val = print_safe_string(strdup(val));
    } else {
        new = (struct char_field_data *) malloc(sizeof *new);
        new->field = print_safe_string(strdup(field));
        new->val = print_safe_string(strdup(val));
        new->next = NULL;
        if (!o->stats) {
            o->stats = new;
            o->stats_last = new;
        } else {
            o->stats_last->next = new;
            o->stats_last = new;
        }
    }

    return 1;
}

int
write_char_field_f(char *label, int ref, char *field, char *val, ...)
{

    char *p, *sval;
    char tmp[MAX_STRING_LENGTH];
    char *buf = tmp;
    va_list argp;
    int ival;

    /*sprintf(tmp, "write_char_field_f:label%s", label);
     * shhlog(tmp);
     * 
     * sprintf(tmp, "write_char_field_f:field:%s", field);
     * shhlog(tmp);
     * 
     * sprintf(tmp, "write_char_field_f:val:%s", val);
     * shhlog(tmp); */



    if (!val)
        return write_char_field(label, ref, field, val);

    va_start(argp, val);

    bzero(tmp, MAX_STRING_LENGTH);
    for (p = val; p && *p; p++) {
        if (*p != '%') {
            *buf++ = *p;
            continue;
        }

        switch (*++p) {
        case ('d'):
            ival = va_arg(argp, int);
            sprintf(buf, "%d", ival);
            while (*buf++);
            buf--;
            continue;
        case ('s'):
            sval = va_arg(argp, char *);
            sprintf(buf, "%s~", sval);
            while (*buf++);
            buf--;
            continue;
        default:
            *buf++ = *p;
            continue;
        }
    }

    va_end(argp);


    return write_char_field(label, ref, field, tmp);
}

char *
minus_brackets(char *s)
{
    char *tmp;
    if ((*s != '{') || (*(s + 1) != '*'))
        return (strdup(s));

    s = s + 2;

    tmp = strstr(s, "*}");
    if (tmp)
        *tmp = '\0';

    return strdup(s);

    /* old, crashed because S was "VALUE", and strstr returned null
     * -Tenebrius
     * s = s + 2;
     * *strstr(s, "*}") = '\0';
     * 
     * return strdup(s);
     */
}

struct file_obj_list *
read_file_label(char **line, int *pos)
{

    struct file_obj_list *new;
    int p = *pos;

    if (!line[p])
        return 0;

    if (strncmp(line[p], "{*", 2)) {
        fprintf(stderr, "read_char_label: Illegal label %s", line[p]);
        return 0;
    }

    new = (struct file_obj_list *) malloc(sizeof *new);
    new->label = minus_brackets(line[p++]);
    sscanf(line[p++], "%d", &new->ref);
    new->next = NULL;
    new->stats = NULL;

    *pos = p;

    return new;
}

struct char_field_data *
read_file_field(char **line, int *pos)
{

    struct char_field_data *new;
    char val[MAX_STRING_LENGTH];
    int p = *pos;
    int start = 0;

    if (!line[p])
        return 0;

    if (strncmp(line[p], "\t{*", 3))
        return 0;

    new = (struct char_field_data *) malloc(sizeof *new);
    new->field = minus_brackets(&line[p++][1]);
    val[0] = '\0';
    // skip the first tab only on the first line of a value.
    start = 1;

    while (line[p] && (line[p][0] != '{') && !((strlen(line[p]) > 1) && (line[p][1] == '{'))) {
        strcat(val, &line[p++][start]);
        start = 0;
    }
    new->val = strdup(val);
    if (new->val[strlen(new->val) - 1] == '\n')
        new->val[strlen(new->val) - 1] = '\0';
    new->next = NULL;

    *pos = p;

    return new;
}

int
read_char_file(void)
{
    char s[MAX_LINE_LENGTH], buf[256], fn[256];
    int i, max_lines = 500;
    char **file_str = (char **) malloc(max_lines * sizeof *file_str);
    int pos;
    struct file_obj_list *new_list;
    struct char_field_data *new_stat;
    FILE *fd;

    /* reset the object list and reference tables */
    free_file_obj_list();
    free_tables();
    init_tables();

    first_name(current.ch->name, buf);
    for (i = 0; i < strlen(buf); i++)
        buf[i] = tolower(buf[i]);
    snprintf(fn, sizeof(fn), "%s/%s", current.dir, buf);

    if (!(fd = fopen(fn, "r"))) {
        gamelogf("read_char_file: Unable to open file %s: %s", fn, strerror(errno));
        return 0;
    }

    for (i = 0; fgets(s, MAX_LINE_LENGTH, fd); i++) {
        if (i == max_lines)
            file_str = (char **) realloc(file_str, (max_lines += 500) * (sizeof *file_str));
        file_str[i] = strdup(s);
    }
    file_str = (char **) realloc(file_str, (i + 1) * (sizeof *file_str));
    file_str[i] = NULL;

    fclose(fd);

    pos = 0;
    while ((new_list = read_file_label(file_str, &pos))) {
        while ((new_stat = read_file_field(file_str, &pos))) {
            if (!new_list->stats) {
                new_list->stats = new_stat;
                new_list->stats_last = new_stat;
            } else {
                new_list->stats_last->next = new_stat;
                new_list->stats_last = new_stat;
            }
        }
        if (!current.file) {
            current.file = new_list;
            current.file_last = new_list;
        } else {
            current.file_last->next = new_list;
            current.file_last = new_list;
        }
    }

    for (; i >= 0; i--)
        if (file_str[i])
            free(file_str[i]);
    free(file_str);

    return 1;
}

/* #define TEN_DOESNT_LIKE_ELIPSES */
#ifdef TEN_DOESNT_LIKE_ELIPSES
int
read_char_field_f(char *label, int ref, char *field, char *val, ...)
{

    char buf[MAX_LINE_LENGTH];
    va_list argp;
    struct file_obj_list *o;
    struct char_field_data *p;
    int *iptr;
    char **sptr;
    char *c, *scan, *aptr;
    int i;
    int bad;

    va_start(argp, val);

    bad = (0 == (p = find_field(label, ref, field)));

    scan = p->val;
    for (c = val; *scan && *c; c++) {
        if (*c != '%') {
            scan++;
            continue;
        }

        switch (*++c) {
        case ('d'):
            iptr = va_arg(argp, int *);
            if (bad) {
                *iptr = 0;
            } else {
                *iptr = atoi(scan);
            }
            /*scanf(scan, "%d", iptr); */
            while (*scan && isspace(*scan))
                scan++;
            if ((*scan == '+') || (*scan == '-'))
                scan++;
            while (*scan && isdigit(*scan))
                scan++;

            break;
        case ('s'):
            sptr = va_arg(argp, char **);
            if (bad) {
                *sptr = strdup("(null)");
            } else {
                for (i = 0; *scan && (scan[i] != '\0') && (scan[i] != '~'); i++);
                strncpy(buf, scan, i);
                buf[i] = '\0';
                if (!strcmp(buf, "(null)"))
                    *sptr = NULL;
                else
                    *sptr = strdup(buf);
            }
            scan += (i + 1);
            break;

        case ('a'):
            aptr = va_arg(argp, char *);
            if (bad) {
                *aptr = '\0';
            } else {
                for (i = 0; *scan && (scan[i] != '~'); i++);
                strncpy(aptr, scan, i);
                aptr[i] = '\0';
                if (!strcmp(aptr, "(null)"))
                    *aptr = '\0';
            }
            scan += (i + 1);
            break;
        default:
            scan++;
            break;
        }
    }

    va_end(argp);

    return 1;
}
#else
int
read_char_field_f(char *label, int ref, char *field, char *val, ...)
{

    char buf[MAX_STRING_LENGTH];
    va_list argp;
    struct char_field_data *p;
    int *iptr;
    char **sptr;
    char *c, *scan, *aptr;
    int i;

    if (!(p = find_field(label, ref, field))) {
        /*gamelogf("read_char_field_f: Unable to read field '%s' for label '%s', "
         * "ref %d", field, label, ref); */
        return 0;
    }

    va_start(argp, val);

    if (*p->val) {
        /* BEGIN HERE - fill the argp ptrs */
    }

    scan = p->val;
    for (c = val; *scan && *c; c++) {
        if (*c != '%') {
            scan++;
            continue;
        }

        switch (*++c) {
        case ('d'):
            iptr = va_arg(argp, int *);
            *iptr = atoi(scan);
            /*scanf(scan, "%d", iptr); */
            while (*scan && isspace(*scan))
                scan++;
            if ((*scan == '+') || (*scan == '-'))
                scan++;
            while (*scan && isdigit(*scan))
                scan++;
            break;
        case ('s'):
            sptr = va_arg(argp, char **);
            for (i = 0; *scan && (scan[i] != '\0') && (scan[i] != '~'); i++);
            strncpy(buf, scan, i);
            buf[i] = '\0';
            if (!strcmp(buf, "(null)"))
                *sptr = NULL;
            else
                *sptr = strdup(buf);
            scan += (i + 1);
            break;
        case ('a'):
            aptr = va_arg(argp, char *);
            for (i = 0; *scan && (scan[i] != '~'); i++);
            strncpy(aptr, scan, i);
            aptr[i] = '\0';
            if (!strcmp(aptr, "(null)"))
                *aptr = '\0';
            scan += (i + 1);
            break;
        default:
            scan++;
            break;
        }
    }

    va_end(argp);

    return 1;
}
#endif
int
read_char_field_num(char *label, int ref, char *field)
{

    int i;

    if (!read_char_field_f(label, ref, field, "%d", &i))
        return 0;

    return i;
}

struct time_info_data
real_time_passed(time_t t2, time_t t1)
{
    long secs;
    struct time_info_data now;

    secs = (long) (t2 - t1);

    now.hours = (secs / SECS_PER_REAL_HOUR) % 24;       /* 0..23 hours */
    secs -= SECS_PER_REAL_HOUR * now.hours;

    now.day = (secs / SECS_PER_REAL_DAY);       /* 0..34 days  */
    secs -= SECS_PER_REAL_DAY * now.day;

    now.month = -1;
    now.year = -1;

    return now;
}

/* calculate game time passed between t1 and t2 */
struct time_info_data
mud_time_passed(time_t t1, time_t t2)
{
    struct time_info_data now;
    struct time_info_data tval1;
    struct time_info_data tval2;

    // Figure out which one is larger and set tval1 an tval2 accordingly
    if (t2 >= t1) {
        tval1 = convert_time(t1);
        tval2 = convert_time(t2);
    } else {
        tval1 = convert_time(t2);
        tval2 = convert_time(t1);
    }

    // Work our way down
    if (tval2.hours >= tval1.hours)
        now.hours = tval2.hours - tval1.hours;
    else {
        now.hours = (MAX_HOUR + tval2.hours) - tval1.hours;
        tval2.day--;
        if (tval2.day < 0) {
            tval2.day = MAX_DAY - 1;    // -1 since day is 0-based
            tval2.month--;
            if (tval2.month < 0) {
                tval2.month = MAX_MONTH - 1;    // -1 since month is 0-based
                tval2.year--;
            }
        }
    }

    if (tval2.day >= tval1.day)
        now.day = tval2.day - tval1.day;
    else {
        now.day = (MAX_DAY + tval2.day) - tval1.day;    // 0-based
        tval2.month--;
        if (tval2.month < 0) {
            tval2.month = MAX_MONTH - 1;        // -1 since month is 0-based
            tval2.year--;
        }
    }

    if (tval2.month >= tval1.month)
        now.month = tval2.month - tval1.month;
    else {
        now.month = (MAX_MONTH + tval2.month) - tval1.month;
        tval2.year--;
    }

    now.year = tval2.year - tval1.year;
    return now;
}

struct time_info_data
convert_time(time_t t1)
{
    struct time_info_data now;
    now.hours = (t1 / RT_ZAL_HOUR) % MAX_HOUR;
    now.day = (t1 / RT_ZAL_DAY) % MAX_DAY;
    now.month = (t1 / RT_ZAL_MONTH) % MAX_MONTH;
    now.year = ZT_EPOCH + t1 / RT_ZAL_YEAR;

    return now;
}

int
mud_hours_passed(time_t t1, time_t t2) {
    struct time_info_data time_diff = mud_time_passed(t1, t2);

    int hours_off = time_diff.hours;

    hours_off += time_diff.day * ZAL_HOURS_PER_DAY;
    hours_off += time_diff.month * ZAL_DAYS_PER_MONTH * ZAL_HOURS_PER_DAY;
    hours_off += time_diff.year * ZAL_MONTHS_PER_YEAR * ZAL_DAYS_PER_MONTH * ZAL_HOURS_PER_DAY;
    return hours_off;
}

/* end basic file stuff */
