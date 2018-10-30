/*
 * File: buffer.c
 * Usage: buffered i/o functions
 *
 * Copyright (C) 1995 all original source by Jim Tripp (Tenebrius), Mark
 * Tripp (Nessalin), Hal Roberts (Bram), and Brian Takle (Ur).  May favourable
 * winds fill the sails of their skimmers.
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"

char *file_buf;
char *orig_file_buf;
int file_size;


int
file_to_buffer(FILE * fl, char *file_name)
{
    struct stat stbuf;

    stat(file_name, &stbuf);
    file_size = stbuf.st_size;

    file_buf = (char *) malloc(file_size * (sizeof *file_buf));

    if (fread(file_buf, sizeof *file_buf, file_size, fl) < file_size)
        return 0;
    else {
        orig_file_buf = file_buf;
        return file_size;
    }
/*
   file_buf = strdup("");

   while (len >= maxlen)
   {
   maxlen *= 2;
   tmp = (char *) malloc (maxlen * (sizeof *tmp));
   len = fread(tmp, sizeof *tmp, maxlen, fl);
   file_buf = (char *)realloc((char *)file_buf, strlen(file_buf) + len);
   strcat(file_buf, tmp);
   }

   len = strlen(file_buf);
   file_buf = (char *)realloc((char *)file_buf, len);
   orig_file_buf = file_buf;

   return len; */
}

long
bufftell(void)
{
    return (int) (file_buf - orig_file_buf);
}

void
free_buff(void)
{
    free(file_buf);

    return;
}

char *
buffgets(char *s, int maxlen)
{
    char *p;
    int len = 1;

    p = file_buf;
    while ((*p != '\n') && (*p != '\0') && (len < maxlen)) {
        len++;
        p++;
    }

    if ((*p == '\0') || (*file_buf == '\0'))
        return NULL;
    else {
        strncpy(s, file_buf, len);
        s[len] = '\0';
        file_buf += len;
        return (s);
    }
}

int
buffeof(void)
{
    return (*file_buf == '\0');
}

int
buffgetc(void)
{
    unsigned char c;

    if (buffeof())
        return EOF;
    else {
        c = (unsigned char) *file_buf;
        file_buf++;
        return c;
    }
}

char *
buffread_string(void)
{
    char buf[MAX_STRING_LENGTH + 1], tmp[MAX_STRING_LENGTH + 1];
    char *rslt;
    register char *point;
    int flag;

    bzero(buf, MAX_STRING_LENGTH);

    do {
        if (!buffgets(tmp, MAX_STRING_LENGTH)) {
            perror("buffread_str");
            exit(0);
        }
        if (strlen(tmp) + strlen(buf) > MAX_STRING_LENGTH) {
            gamelog("buffread_string: string too large (db.c)");
            exit(0);
        } else
            strcat(buf, tmp);

        for (point = buf + strlen(buf) - 2; point >= buf && isspace(*point); point--);
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
    }
    while (!flag);

    if (strlen(buf) > 0) {
        CREATE(rslt, char, strlen(buf) + 1);
        strcpy(rslt, buf);
    } else
        rslt = 0;
    return (rslt);
}


char *
buffread_text(void)
{
    char buf[MAX_STRING_LENGTH];
    char *rslt, c;
/*  register char *point; */

    bzero(buf, MAX_STRING_LENGTH);

    while (!buffeof() && (c = buffgetc()) && (c != '|')) {
        *(buf + strlen(buf)) = c;

        if (*(buf + strlen(buf) - 1) == '\n')
            *(buf + strlen(buf)) = '\r';
    }

    if (strlen(buf) > 0) {
        CREATE(rslt, char, strlen(buf) + 1);
        strcpy(rslt, buf);
    } else
        rslt = (char *) 0;

    return rslt;
}

int
buffread_var(void)
{
    char buf[MAX_STRING_LENGTH];
    char c;
    /*   register char *point; */

    bzero(buf, MAX_STRING_LENGTH);

    while (!buffeof() && (c = buffgetc()) && (c != '|')) {
        *(buf + strlen(buf)) = c;

        if (*(buf + strlen(buf) - 1) == '\n')
            *(buf + strlen(buf)) = '\r';
    }

    return atoi(buf);
}

