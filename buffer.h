/*
 * File: buffer.h 
 * Usage: declarations for file buffering & reading functions
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

/* This set of functions is designed to eliminate unecessary file i/o calls
 * by providing a function (file_to_buffer) that reads a file into one big
 * text file and a set of functions that read from that buffer as if reading
 * from a file as well emulations of many of the stdio functions that deal 
 * with files.                                        -bram
 */


/* Buffering and freeing functions */

/* file_to_buffer before using any of the buffer reading functions */
int file_to_buffer(FILE *, char *);

/* free_buff when you are done with the buffer */
void free_buff(void);


/* Stdio emulations -- these are all */
long bufftell(void);
int buffgetc(void);
char *buffgets(void);
int buffeof(void);


/* Armageddon i/o functions.  These should work exactly as their
 * counterparts, only they read from and write to the buffer instead of 
 * to and from a file.  Note that the functions automatically read to 
 * and write from whatever buffer was last opened with file_to_buffer()
 */

/* read and allocate space for a '|'-terminated string from the buffer*/
char *buffread_text(void);

/* read a '|'-terminated integer from the buffer*/
int buffread_var(void);

/* read and allocate space for a '~'-terminated string from the buffer*/
char *buffread_string(void);

