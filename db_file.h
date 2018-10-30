/*
 * File: db_file.h
 * Usage: interface for file routines used for reading various db items
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


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char **buf);

void fwrite_string(char *txt, FILE * fl);

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl);

/* read and allocate space for a '~' terminated string from a given file,
   ignoring leading whitespace */
char *fread_string_no_whitespace(FILE * fl);

void fwrite_text(char *txt, FILE * fl);

char *fread_text(FILE * fl);

void fwrite_var(long num, FILE * fl);

int fread_var(FILE * fl);

int fread_number(FILE * fp);

float fread_float(FILE * fp);

void fread_to_eol(FILE * fp);
char *fread_word(FILE * fp);

