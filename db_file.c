/*
 * File: db_file.c
 * Usage: file manipulation routines used for reading various db items
 *
 * Copyright (C) 1996 original source by Jim Tripp (Tenebrius), Mark
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"

/* needed to do the math for getting a float */
double pow(double x, double y);

/* replaces # with n.  this prevents a nasty gamelogf that is triggered by 
   lines that begin with a # */
void
replace_num(char *s)
{
    char *p = s;

    while (NULL != (p = strstr(p, "#")))
        *p = 'n';
}

/* read contents of a text file, and place in buf */
int
file_to_string(const char *name, char **buf)
{
    FILE *fl;
    int tmp;
    int max_len = MAX_STRING_LENGTH * 10;
    int len = 0;
    char readbuf[MAX_STRING_LENGTH * 50];
    char *t1 = readbuf;

    *t1 = '\0';
    if (!(fl = fopen(name, "r"))) {
        fprintf(stderr, "file-to-string() fopen(%s) failed: %s\n", name, strerror(errno));
        return (-1);
    }
    do {
        tmp = fgetc(fl);
        if (tmp != EOF) {
            if (((len + 2) > max_len && tmp == '\n') || (len + 1 > max_len && tmp != '\n')) {

                fprintf(stderr, "fl->strng: string too big (db.c, file_to_string)");
                *t1 = '\0';
                return (-1);
            }
            *t1++ = tmp;
            len++;
            if (tmp == '\n') {
                *t1++ = '\r';
                len++;
            }
            *t1 = '\0';
        }
    }
    while (tmp != EOF);
    fclose(fl);

    free(*buf);
    *buf = strdup(readbuf);
    return 0;
}

void
fwrite_string(char *txt, FILE * fl)
{
    int i, j = 0;
    char buf[16384];

    if (!txt) {
        fprintf(fl, "~\n");
        return;
    }

    replace_num(txt);

    for (i = 0; i < strlen(txt); i++)
        if (txt[i] != '\r')
            buf[j++] = txt[i];
    buf[j] = '\0';

    fprintf(fl, "%s~\n", buf);
}


/* read and allocate space for a '~'-terminated string from a given file */
char *
fread_string(FILE * fl)
{
    char realbuf[2 * MAX_STRING_LENGTH + 1];
    char *buf, tmp[2 * MAX_STRING_LENGTH + 1];
    char *rslt;
    register char *point;
    int flag;

    if (!fl) {                  /* No file */
        perror("fread_str: No file given");
        exit(0);
    }
    flag = 0;
    bzero(realbuf, 2 * MAX_STRING_LENGTH + 1);
    buf = realbuf + 16;

    do {
        if (!fgets(tmp, MAX_STRING_LENGTH, fl)) {
            fprintf(stderr, "fread_string(), fgets indicated premature feof()\n");
            exit(0);
        }

        if (strlen(tmp) + strlen(buf) > (2 * MAX_STRING_LENGTH)) {
            fprintf(stderr, "fread_string: string too large (db.c)");
            exit(0);
        } else
            strcat(buf, tmp);

        for (point = buf + strlen(buf) - 2; point >= buf && isspace(*point); point--);

        /*#define CHECK_UNDERRUN */
#ifdef CHECK_UNDERRUN
        /* potential underrun -Tenebrius 2003.12.23 */
        if (point < buf)
            point = buf;

        if ((flag = (*point == '~')))
            if (strlen(buf) > 3 && (*(buf + strlen(buf) - 3) == '\n')) {
                *(buf + strlen(buf) - 2) = '\r';
                *(buf + strlen(buf) - 1) = '\0';
            } else {
                if (strlen(buf) > 2)
                    *(buf + strlen(buf) - 2) = '\0';
        } else {
            *(buf + strlen(buf) + 1) = '\0';
            *(buf + strlen(buf)) = '\r';
        }
#else
        if ((flag = (*point == '~')))
            if (*(buf + strlen(buf) - 3) == '\n') {
                *(buf + strlen(buf) - 2) = '\r';
                *(buf + strlen(buf) - 1) = '\0';
            } else
                *(buf + strlen(buf) - 2) = '\0';
        else {
            *(buf + strlen(buf) + 1) = '\0';
            *(buf + strlen(buf)) = '\r';
        }
#endif
    }
    while (!flag);

    if (strlen(buf) > 0) {
        rslt = strdup(buf);
        /*CREATE(rslt, char, strlen(buf) + 1);
         * strcpy(rslt, buf); */
    } else
        rslt = 0;

    return (rslt);
}


/* similar to above, but...
   Need to skip whitespace from some variables */
char *
fread_string_no_whitespace(FILE * fp)
{
    char buf[MAX_STRING_LENGTH + 1];
    char *plast;
    char c;

    if (!fp) {
        fprintf(stderr, "fread_str: No file given");
        exit(0);
    }

    plast = buf;

    /*
     * Skip blanks.
     * Read first char.
     */
    do {
        c = getc(fp);
    }
    while (isspace(c));

    if ((*plast++ = c) == '~')
        return strdup("");

    for (;;) {
        /*
         * Back off the char type lookup,
         *   it was too dirty for portability.
         *   -- Furey
         */
        switch (*plast = getc(fp)) {
        default:
            plast++;
            break;

        case EOF:
            fprintf(stderr, "Fread_string: EOF");
            exit(1);
            break;

        case '\n':
            plast++;
            *plast++ = '\r';
            break;

        case '\r':
            break;

        case '~':
            *plast = '\0';
            return strdup(buf);
        }
    }
}

void
fwrite_text(char *txt, FILE * fl)
{
    int i, j = 0;
    char buf[256];

    if (!txt) {
        fprintf(fl, "|");
        return;
    }

    replace_num(txt);

    for (i = 0; i < strlen(txt); i++)
        if (txt[i] != '\r')
            buf[j++] = txt[i];
    buf[j] = '\0';

    fprintf(fl, "%s|", buf);
}

char *
fread_text(FILE * fl)
{
    char buf[MAX_STRING_LENGTH];
    char *rslt, c;

    bzero(buf, MAX_STRING_LENGTH);

    while (!feof(fl) && (c = fgetc(fl)) && (c != '|')) {
        *(buf + strlen(buf)) = c;

        if (*(buf + strlen(buf) - 1) == '\n')
            *(buf + strlen(buf)) = '\r';
    }

    if (strlen(buf) > 0) {
        rslt = strdup(buf);
/*        CREATE(rslt, char, strlen(buf) + 1);
   strcpy(rslt, buf); */
    } else
        rslt = (char *) 0;

    return rslt;
}

void
fwrite_var(long num, FILE * fl)
{
    fprintf(fl, "%ld|", num);
}

int
fread_var(FILE * fl)
{
    char buf[MAX_STRING_LENGTH];
    char c;

    bzero(buf, MAX_STRING_LENGTH);

    while (!feof(fl) && (c = fgetc(fl)) && (c != '|')) {
        *(buf + strlen(buf)) = c;

        if (*(buf + strlen(buf) - 1) == '\n')
            *(buf + strlen(buf)) = '\r';
    }

    return atoi(buf);
}

/*
 * Read a number from a file.
 */
int
fread_number(FILE * fp)
{
    int number;
    bool sign;
    char c;

    do {
        c = getc(fp);
    }
    while (isspace(c));

    number = 0;

    sign = FALSE;

    if (c == '+') {
        c = getc(fp);
    } else if (c == '-') {
        sign = TRUE;
        c = getc(fp);
    }

    if (!isdigit(c)) {
        fprintf(stderr, "Fread_number: bad format.");
        exit(1);
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (sign)
        number = 0 - number;

    if (c == '|')
        number += fread_number(fp);
    else if (c != ' ')
        ungetc(c, fp);

    return number;
}


/*
 * Read to end of line (for comments).
 */
void
fread_to_eol(FILE * fp)
{
    char c;

    c = getc(fp);
    while (c != '\n' && c != '\r')
        c = getc(fp);

    do {
        c = getc(fp);
    }
    while (c == '\n' || c == '\r');

    ungetc(c, fp);
    return;
}


/*
 * Read one word (into static buffer).
 */
char *
fread_word(FILE * fp)
{
    static char word[MAX_INPUT_LENGTH * 200];
    char *pword;
    char cEnd;

    do {
        cEnd = getc(fp);
    }
    while (isspace(cEnd));

    if (cEnd == '\'' || cEnd == '"') {
        pword = word;
    } else {
        word[0] = cEnd;
        pword = word + 1;
        cEnd = ' ';
    }

    for (; pword < word + (MAX_INPUT_LENGTH * 200); pword++) {
        *pword = getc(fp);
        if (cEnd == ' ' ? isspace(*pword) : *pword == cEnd) {
            if (cEnd == ' ')
                ungetc(*pword, fp);
            *pword = '\0';
            return word;
        }
    }

    fprintf(stderr, "Fread_word: word too long.");
/*    exit( 1 );   Took this out since we may not really want to crash */
    return NULL;
}



/* #define USE_SCANF_FOR_FREAD_FLOAT */
#ifdef USE_SCANF_FOR_FREAD_FLOAT
float
fread_float(FILE * fp)
{
    float number = 0.0;

    if (fscanf(fp, "%f", &number) != 1) {
        shhlog("Fread_float: bad format");
        exit(1);
    }

    return number;
}

#else

float
fread_float(FILE * fp)
{
    float number = 0.0, f = 0.0;
    int n = 0;
    bool sign, fPoint = FALSE;
    char c;

    do {
        c = getc(fp);
    }
    while (isspace(c));

    number = 0;

    sign = FALSE;
    if (c == '+') {
        c = getc(fp);
    } else if (c == '-') {
        sign = TRUE;
        c = getc(fp);
    }

    if (!isdigit(c) && c != '.') {
        fprintf(stderr, "Fread_float: bad format.");
        exit(1);
    }

    if (c == '.')
        fPoint = TRUE;

    while (isdigit(c) || (c == '.' && !fPoint)) {
        if (c == '.') {
            fPoint = TRUE;
            f += number;
            number = 0.0;
            c = getc(fp);
            continue;
        }

        if (!fPoint)
            number = (number * 10.0) + (c - '0');
        else
            number += ((float) (c - '0')) / (pow(10, ++n));

        c = getc(fp);
    }

    f += number;

    if (sign)
        f = 0.0 - f;

    if (c != ' ')
        ungetc(c, fp);

    return f;
}

#endif

