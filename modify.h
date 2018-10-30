/*
 * File: MODIFY.H
 * Usage: Definitions and prototypes for parser functions.
 *
 * Copyright (C) 1993, 1994 by Dan Brumleve, Brian Takle, and the creators
 * of DikuMUD.  May Vivadu bring them good health and many children!
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 * ---------------------------------------------------------------------
 */

#ifndef __MODIFY_INCLUDED
#define __MODIFY_INCLUDED

#include "structs.h"
#include "utils.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "guilds.h"
#include "vt100c.h"

#define EDITOR_CREATE_MODE 0
#define EDITOR_APPEND_MODE 1

/* Add user input to the 'current' string (as defined by d->str) */
void string_add(struct descriptor_data *d, char *str);
void string_editor(CHAR_DATA * ch, char **pString, int maxlen, int mode);
void string_edit(CHAR_DATA * ch, char **pString, int maxlen);
void string_append(CHAR_DATA * ch, char **pString, int maxlen);
void string_edit_verbose(CHAR_DATA * ch, char **pString, int maxlen, char *text);
void string_append_verbose(CHAR_DATA * ch, char **pString, int maxlen, char *text);

void strdupcat(char **str, const char *add);

int count_lines_in_str(char *str);
int trunc_str_by_lines(char *str, int l);
char *one_word(char *argument, char *first_arg);
void show_string(struct descriptor_data *d, const char *input);
void strip(char *line);

/*
* This procedure is *heavily* system dependent. If your system is not set up
* properly for this particular way of reading the system load (It's weird all
* right - but I couldn't think of anything better), change it, or don't use -l.
* It shouldn't be necessary to use -l anyhow. It's oppressive and unchristian
* to harness man's desire to play. Who needs a friggin' degree, anyhow?
*/

int load(void);

#ifdef OLD_COMA
void coma(void);
#endif


/* STRING JUGGLERS    -- brian */
/* locate the position of a string in a string */
int string_pos(char *find, char *source);

/* snag out the portion of a string from start_pos for length chars */
void sub_string(const char *source, char *target, int start_pos, int length);

/* delete a substring from a string */
void delete_substr(char *source, int start_pos, int length);

/* insert a substring into a source string.
 * maxlen is the total allocated space for source, and is
 * usually the same as strlen(source). */
void insert_substr(const char *sub, char *source, int start_pos, int maxlen);

const char *one_argument(const char *source, char *target, size_t target_len);
/* get the first two arguments from a string */
const char *two_arguments(const char *source, char *first, size_t first_len, char *second,
                          size_t second_len);

/* get the first argument, with a given delimiter, return the rest */
const char *one_argument_delim(const char *source, char *target, size_t target_len, char delim);

/* get & handle emotes within a command arguemnts */
void prepare_emote_message(const char *pre, const char *message, const char *post, char *buf);
char *extract_emote_arguments(const char *source, char *pre, size_t pre_len, char *args,
                              size_t args_len, char *post, size_t post_len);


/* return an initial-capped string */
char *capitalize(const char *str);
char *lowercase(const char *str);

/* find the closing parenthesis (or whatever) on the same "level"
 * as the opening one, such that "((a + b) + c)" will return...
 *                                ^           ^--the pos of this one */
int close_parens(const char *exp, char type);

/*
 * Grab 'a' 'an's and 'the's and smash the living hell out of them.
 * Good for use with manipulation of object names.  Only destroys first
 * occurance.
 * -Result is a ptr to static memory
 * -Original is not freed
 */
char *smash_article(const char *text);

/*
 * Look for 'word' in 'text' and remove it.
 * -Case insensitive search
 * -Will find multiple copies
 * -Result is a ptr to static memory
 * -Original is not freed
 */
char *smash_word(char *text, char *word);

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix(const char *astr, const char *bstr);

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix(const char *astr, const char *bstr);

/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix(const char *astr, const char *bstr);

/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde(char *str);

/*
 * Removes any trailing blanks from a string.
 */
void smash_blanks(char *str);

/*
 * Removes the specified char from a string.
 * Completely removes it, and frees the original string (so must have
 * been allocated.  Returns the string without the given character.
 */
char *smash_char(char *str, char toRem);

/*
 * Replace the given character within the string and stores it in buf.
 */
void replace_char(const char *str, char from, char to, char *buf, int length );

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes, parenthesis (barring ) ('s) and
 * percentages.
 */
char *first_arg(char *argument, char *arg_first, bool fCase);
#endif

