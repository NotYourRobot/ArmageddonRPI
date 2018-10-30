/*
 * File: MODIFY.C
 * Usage: Modification of strings from within the game.
 * Copyright (C) 1993 by Dan Brumleve and the creators of DikuMUD.
 *
 * Part of:
 *   /\    ____                                     __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 *
 * Unauthorized distribution of this source code is punishable by 
 * eternal damnation to the hell pits of Suk-Krath.  Have a nice day.
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "structs.h"
#include "modify.h"
#include "utils.h"
#include "parser.h"
#include "handler.h"
#include "utility.h"
#include "db.h"
#include "comm.h"
#include "guilds.h"
#include "vt100c.h"

#define REBOOT_AT    10         /* 0-23, time of optional reboot if -e lib/reboot */
#define TP_MOB    0
#define TP_OBJ	   1
#define TP_ERROR  2

/* Add user input to the 'current' string (as defined by d->str) */
void
string_add(struct descriptor_data *d, char *str)
{
    char *scan;
    int terminator = 0, lines = 0, rmlines = 0, oldlines = 0;
    char edit_end;
    bool terminate = FALSE;

    if (d->player_info)
        edit_end = d->player_info->edit_end;
    else
        edit_end = '~';

    if ((*str == '.') && (*(str + 1) == 'd')) {
        if (!*d->str) {
            send_to_char("You are still on the first line.\n\r", d->character);
            return;
        }
        if (!*(str + 2) || !isdigit(*(str + 2))) {
            send_to_char("You must give the number of lines that you wish to delete.\n\r",
                         d->character);
            return;
        }
        if ((rmlines = atoi(str + 2)) < 1) {
            send_to_char("You can't do that,\n\r", d->character);
            return;
        }
        if ((oldlines = count_lines_in_str(*d->str)) < rmlines) {
            send_to_char("There aren't that many lines in the text.\n\r", d->character);
            return;
        }


        lines = oldlines - rmlines;
        trunc_str_by_lines(*d->str, lines);
        send_to_char("Text truncated to the following:\n\r\n\r", d->character);
        send_to_char(*d->str, d->character);
        return;
    } else if (*str == '.') {
        char arg[MAX_STRING_LENGTH];

        str = (char *) one_argument(str, arg, sizeof(arg));

        if (!strcmp(arg, ".c")) {       /* clear the string */
            send_to_char("Text cleared.\n\r", d->character);
            free(*d->str);
            *d->str = strdup("");
            return;
        } else if (!strcmp(arg, ".s")) {        /* show the string */
            send_to_char("Text so far:\n\r", d->character);
            send_to_char(*d->str, d->character);
            return;
        } else if (!strcmp(arg, ".f")) {        /* format the string */
            *d->str = format_string(*d->str);
            send_to_char("Text formatted.\n\r", d->character);
            return;
        } else if (!strcmp(arg, ".i")) {        /* format the string */
            *d->str = indented_format_string(*d->str);
            send_to_char("Text formatted and indented.\n\r", d->character);
            return;
        } else if (!strcmp(arg, ".x")) {        /* execute a command */
            if (d->character && d->connected == CON_PLYNG) {
                parse_command(d->character, str);
            } else {
                send_to_char("Not outside the game.\n\r", d->character);
            }
            return;
        } else if (!strcmp(arg, ".p")) {        /* proof the string (spellcheck) */
#if defined(unix)
            FILE *out;
            char *pf;
            char buf[MAX_STRING_LENGTH];

            out = fopen(".tmpspell", "w");
            if (*str && str[0] != '\0')
                fprintf(out, "%s\n", str);
            else
                fprintf(out, "%s\n", *d->str);

            fflush(out);
            fclose(out);

            out = popen("ispell -l < .tmpspell", "r");
            if (!out) {
                cprintf(d->character, "The system can't help you with your spelling today.\n\r");
                return;
            }
          
            send_to_char("The following words are misspelled:\n\r", d->character);

            while (fgets(buf, MAX_STRING_LENGTH - 1, out) != NULL) {
                pf = format_string(strdup(buf));
                send_to_char(pf, d->character);
                free(pf);
            }

            pclose(out);
#else
            send_to_char("Unix systems only.\n\r", ch);
#endif
            return;
        } else if (!strcmp(arg, ".q")) {        /* quit/terminate editing */
            terminate = TRUE;
        } else if (!strcmp(arg, ".r")) {        /* ruler for line length */
            cprintf(d->character,
                    "[---|----|----|----|----|----|----|----|----|----|----|----|----|----|----]\n\r");
            return;
        } else if (!strcmp(arg, ".h")) {        /* help for string editing */
            send_to_char("Help on text editing:\n\r", d->character);
            send_to_char(".c          clear the text.\n\r", d->character);
            send_to_char(".d#         delete # lines from the end of the text.\n\r", d->character);
            send_to_char(".f          format the text into ONE paragraph.\n\r", d->character);
            send_to_char(".h          show this message.\n\r", d->character);
            send_to_char(".i          format the text into one or more, indented paragraphs.\n\r",
                         d->character);
            send_to_char(".p          proof / spell check the text.\n\r", d->character);
            cprintf(d->character, ".q | '%c'    terminate editing.\n\r", d->player_info->edit_end);
            send_to_char(".r          display a 75-character ruler.\n\r", d->character);
            send_to_char(".s          show the text so far.\n\r", d->character);
            if (d->character && d->connected == CON_PLYNG) {
                send_to_char(".x <cmd>    execute command <cmd>.\n\r", d->character);
            }
            return;
        }
    }

    /* The ".q" option allows us to terminate also, so if it's already been
     * specified, there's no need to scan for it here.
     */
    if (!terminate) {
        /* determine if this is the terminal string, and truncate if so */
        for (scan = str; *scan; scan++)
            if ((terminator = (*scan == edit_end))) {
                *scan = '\0';
                break;
            }


        if (!(*d->str)) {
            if (strlen(str) > d->max_str) {
                send_to_char("Text too long - Truncated.\n\r", d->character);
                *(str + d->max_str) = '\0';
            }
            CREATE(*d->str, char, strlen(str) + 3);
            strcpy(*d->str, str);
        } else {
            if (strlen(str) + strlen(*d->str) > d->max_str) {
                send_to_char("Text too long. Last line skipped.\n\r", d->character);
                return;
            } else {
                if (!(*d->str = (char *) realloc(*d->str, strlen(*d->str) + strlen(str) + 3))) {
                    perror("string_add");
                    exit(1);
                }
                strcat(*d->str, str);
            }
        }
    }
    // We're done editing, time to save and move on
    if (terminator || terminate) {

        /* uncomment if this is causing problems - tenebrius  2006/04/01 */
        /*  #define LOG_EDITS HALASTER_MESSED_UP */
#ifndef LOG_EDITS
        if (d->editing_display) {
            char logstr[MAX_STRING_LENGTH + 200];
            sprintf(logstr, "Edited %s result = %s", d->editing_display, *d->str);
            shhlog(logstr);

            sprintf(logstr, "Edited '%s' set to \n\r---\n\r%s\n\r---\n\r", d->editing_display,
                    *d->str);
            send_to_char(logstr, d->character);
            free(d->editing_display);
            d->editing_display = 0;
        }
#endif

        else if (d->editing) {
            char *arg = *d->str;
            for (; *arg == ' '; arg++);
            trim_string(arg);

            if (strlen(arg) > 0) {
                if (d->editing_header) {
                    fputs(d->editing_header, d->editing);
                    free(d->editing_header);
                    d->editing_header = NULL;
                }
                fputs(*d->str, d->editing);
                fclose(d->editing);
            } else {
                send_to_char("Empty text, nothing saved.\n\r", d->character);

                /* if at the beginning of the file, unlink the file */
                if (ftell(d->editing) == 0L) {
                    fclose(d->editing);
                    if (d->editing_filename != NULL) {
                        unlink(d->editing_filename);
                        free(d->editing_filename);
                        d->editing_filename = NULL;
                    }
                } else {
                    fclose(d->editing);
                }
            }

            /* cleanup */
            d->editing = NULL;
            if (d->tmp_str) {
                free(d->tmp_str);
                d->tmp_str = NULL;
            }
        }
        d->str = NULL;

        if (d->connected == CON_EXDSCR) {
            show_main_menu(d);
            d->connected = CON_SLCT;
        } else if (d->connected == CON_AUTO_DESC_END) {
            if (d->character->application_status == APPLICATION_REVISING) {
                print_revise_char_menu(d);
                d->connected = CON_REVISE_CHAR_MENU;
                return;
            }

            SEND_TO_Q("\n\r", d);
#define AUTO_FORMAT_DESC
#ifdef AUTO_FORMAT_DESC
            if (strlen(d->character->description) > 0) {
                d->character->description = format_string(d->character->description);
                SEND_TO_Q("Your description has been automatically formatted for you.\n\r", d);
                SEND_TO_Q("You can revise it later if you like.\n\r", d);
                SEND_TO_Q("\n\r", d);
            }
#endif
            SEND_TO_Q(short_descr_info, d);
            d->connected = CON_AUTO_SDESC;
        } else if (d->connected == CON_AUTO_BACKGROUND_END) {
            if (d->character->application_status != APPLICATION_REVISING) {
#define AUTO_FORMAT_BACKGROUND
#ifdef AUTO_FORMAT_BACKGROUND
                if (strlen(d->character->background) > 0) {
                    d->character->background = format_string(d->character->background);
                    SEND_TO_Q("\n\r", d);
                    SEND_TO_Q("Your background has been automatically formatted for" " you.\n\r",
                              d);
                    SEND_TO_Q("You can revise it later if you like.", d);
                }
#endif
                d->character->application_status = APPLICATION_REVISING;
                update_char_info(d->player_info, d->character->name, APPLICATION_REVISING);
                save_player_info(d->player_info);
                SEND_TO_Q("\n\r\n\rNow review what you entered so far.  When you"
                          " are satisfied with your\n\rapplication, type 'U' to submit the"
                          " application.\n\r", d);

            }
            print_revise_char_menu(d);
            d->connected = CON_REVISE_CHAR_MENU;
            return;
        }
    } else
        strcat(*d->str, "\n\r");
}


int
count_lines_in_str(char *str)
{
    int lines = 0, i;

    for (i = 0; i < strlen(str); i++)
        if (str[i] == '\n')
            lines++;

    return lines;
}


int
trunc_str_by_lines(char *str, int l)
{
    int i, j = 0;

    for (i = 0; j < l; i++)
        if (str[i] == '\n')
            j++;
    for (; i < strlen(str); i++)
        str[i] = '\0';
    return 1;
}


#undef MAX_STR


/* db stuff *********************************************** */


/* One_Word is like one_argument, execpt that words in quotes "" are */
/* regarded as ONE word                                              */

char *
one_word(char *argument, char *first_arg)
{
    int found, begin, look_at;

    found = begin = 0;

    do {
        for (; isspace(*(argument + begin)); begin++);

        if (*(argument + begin) == '\"') {      /* is it a quote */

            begin++;

            for (look_at = 0;
                 (*(argument + begin + look_at) >= ' ') && (*(argument + begin + look_at) != '\"');
                 look_at++)
                *(first_arg + look_at) = LOWER(*(argument + begin + look_at));

            if (*(argument + begin + look_at) == '\"')
                begin++;

        } else {

            for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
                *(first_arg + look_at) = LOWER(*(argument + begin + look_at));

        }

        *(first_arg + look_at) = '\0';
        begin += look_at;
    }
    while (fill_word(first_arg));

    return (argument + begin);
}


struct help_index_element *
build_help_index(FILE * fl, int *num)
{
    int nr = -1;
    struct help_index_element *list = 0;
    char buf[81], tmp[81], *scan;
    long pos;

    for (;;) {
        pos = ftell(fl);
        fgets(buf, 81, fl);
        *(buf + strlen(buf) - 1) = '\0';
        scan = buf;
        for (;;) {
            /* extract the keywords */
            scan = one_word(scan, tmp);

            if (!*tmp)
                break;

            if (!list) {
                CREATE(list, struct help_index_element, 1);
                nr = 0;
            } else {
                RECREATE(list, struct help_index_element, ++nr + 1);
                memset(&list[nr], 0, sizeof(struct help_index_element));
            }

            list[nr].pos = pos;
            CREATE(list[nr].keyword, char, strlen(tmp) + 1);
            strcpy(list[nr].keyword, tmp);
        }
        /* skip the text */
        do
            fgets(buf, 81, fl);
        while (*buf != '#');
        if (*(buf + 1) == '~')
            break;
    }

    /* sorting procedure removed, already sorted with external indexer 
     * --brian 
     
     do {
     issorted = 1;
     for (i = 0; i < nr; i++)
     if (strcmp(list[i].keyword, list[i + 1].keyword) > 0) {
     mem = list[i];
     list[i] = list[i + 1];
     list[i + 1] = mem;
     issorted = 0;
     }
     }
     while (!issorted);
     
     */

    *num = nr;
    return (list);
}

void
page_string(struct descriptor_data *d, const char *str, int keep_internal)
{
    if (!d)
        return;

    if (!str)
        return;
    if (strlen(str) > MAX_PAGE_STRING) {
        gamelogf("Bad strlen() of %d (%d) in page_string().", strlen(str), MAX_PAGE_STRING);
        return;
    }

    if (keep_internal) {
        CREATE(d->showstr_head, char, strlen(str) + 1);
        strcpy(d->showstr_head, str);
        d->showstr_point = d->showstr_head;
    } else
        d->showstr_point = str;

    show_string(d, "");
}


void
show_string(struct descriptor_data *d, const char *input)
{
    char buffer[MAX_PAGE_STRING], buf[MAX_INPUT_LENGTH];
    register char *scan;
    const char *chk;
    int maxl, lines = 0, toggle = 1, pagelength;

    input = one_argument(input, buf, sizeof(buf));

    if (*buf) {
        if (d->showstr_head) {
            free(d->showstr_head);
            d->showstr_head = 0;
        }
        d->showstr_point = 0;
        return;
    }

    if (strlen(d->showstr_point) > MAX_PAGE_STRING) {
        gamelog("Bad strlen, show_string().");
        if (d->showstr_head) {
            free(d->showstr_head);
            d->showstr_head = 0;
        }
        d->showstr_point = 0;
        return;
    }

    pagelength = d->character ? GET_PAGELENGTH(d->character) : d->pagelength;

    maxl = (d->character && IS_SET(d->character->specials.act, CFL_INFOBAR)
            ? pagelength - 4 : pagelength - 2);
    /* show a chunk */

    strcpy(buffer, d->showstr_point);

    for (scan = buffer;; scan++, d->showstr_point++) {
        if (((*scan == '\n') || (*scan == '\r')) && ((toggle = -toggle) < 0)) {
            lines++;
        } else if (!*scan || (lines >= maxl)) {
            *scan = '\0';
            SEND_TO_Q(buffer, d);

            /* see if this is the end (or near the end) of the string */
            if (!d->showstr_point)
                return;
            for (chk = d->showstr_point; isspace(*chk); chk++);
            if (!*chk) {
                if (d->showstr_head) {
                    free(d->showstr_head);
                    d->showstr_head = 0;
                }
                d->showstr_point = 0;
                if (d->connected)
                    SEND_TO_Q("***** PRESS RETURN *****", d);
            }
            return;
        }
    }
}

/* STRING JUGGLERS    -- brian */

/* locate the position of a string in a string */
int
string_pos(char *find, char *source)
{
    int n1, n2, n3;

    for (n1 = 0; *(source + n1) != '\0'; n1++) {
        if (*(source + n1) == *find) {
            for (n2 = n1, n3 = 0; (*(find + n3) != '\0') && (*(source + n2) != '\0')
                 && (*(source + n2) == *(find + n3)); n2++, n3++);
            if (*(find + n3) == '\0')
                return (n1);
        }
    }
    return (-1);
}

/* snag out the portion of a string from start_pos for length chars */
void
sub_string(const char *source, char *target, int start_pos, int length)
{
    int n1, n2, maxlen;

    if (start_pos < 0)
        return;

    /* no copying past the end of the string */
    maxlen = strlen(source);
    if ((start_pos > maxlen) || ((start_pos + length) > maxlen)) {
        *target = '\0';
        return;
    }

    for (n1 = start_pos, n2 = 0; n1 < (start_pos + length); n1++, n2++)
        *(target + n2) = *(source + n1);

    *(target + n2) = '\0';
}

/* delete a substring from a string */
void
delete_substr(char *source, int start_pos, int length)
{
    int len_src;
    len_src = strlen(source);

    memmove(&source[start_pos], &source[start_pos + length], len_src - start_pos - length + 1);
}

/* insert a substring into a source string.
 * maxlen is the total allocated space for source, and is
 * usually the same as strlen(source). */
void
insert_substr(const char *sub, char *source, int start_pos, int maxlen)
{
#define USE_TENEBRIUSS_INSERT_SUBSTR
#ifdef USE_TENEBRIUSS_INSERT_SUBSTR
    int len_sub, len_src;
    len_sub = strlen(sub);
    len_src = strlen(source);

    if ((start_pos < 0) || ((len_sub + start_pos) > maxlen))
        return;
    /* move the part of source that is in the way of sub to the end 
     * plus one for the '\0' */
    memmove(&source[start_pos + len_sub], &source[start_pos], len_src - start_pos + 1);
    /* copy sub into the opened up space */
    memcpy(&source[start_pos], sub, len_sub);

#else
    int n1, n2, n3;
    char *tmp;

    if (start_pos < 0)
        return;

    CREATE(tmp, char, strlen(sub) + 1);

    n1 = n2 = n3 = 0;
    while (*(source + n1) != '\0') {
        if (n1 == start_pos) {
            while (*(sub + n2) != '\0') {
                *(tmp + n3) = *(sub + n2);
                n2++;
                n3++;
            }
            *(tmp + n3) = *(source + n1);
            n1++;
            n3++;
        } else {
            *(tmp + n3) = *(source + n1);
            n1++;
            n3++;
        }
    }
    *(tmp + n3) = '\0';

    /* copy only as much as you can */

    if ((maxlen - 1) < strlen(tmp)) {
        for (n1 = 0; n1 < (maxlen - 1); n1++)
            *(source + n1) = *(tmp + n1);
        *(source + maxlen) = '\0';
    } else {
        strcpy(source, tmp);
    }

    free(tmp);
#endif
}


/* get the first two arguments from a string */
const char *
two_arguments(const char *source, char *first, size_t first_len, char *second, size_t second_len)
{
    source = one_argument(source, first, first_len);
    source = one_argument(source, second, second_len);

    return source;
}

/*
 * one_argument_delim grab off the first argument, using a given delimiter
 * '(' ends with ')'
 * '[' ends with ']'
 * '*' ends with '*'
 * all other starts end with space
 *
 * Replaced with version like one_argument 1/7/05 -Morgenes
 */
const char *
one_argument_delim(const char *source, char *target, size_t target_len, char cStart)
{
    char cEnd[2];
    const char *begin;
    int len_s;                  /* length of non-spaces */

    target[0] = '\0';
    if (!source || !strlen(source)) {
        return source;
    }

    /* move to first non-space */
    begin = source;
    while (isspace(*begin))
        begin++;

    /* determine what our end character should be */
    if (cStart == '(')
        cEnd[0] = ')';
    else if (cStart == '[')
        cEnd[0] = ']';
    else if (cStart == '*')
        cEnd[0] = '*';
    else
        cEnd[0] = ' ';
    cEnd[1] = '\0';

    /* Skip over the start character if it's the first */
    if (*begin == cStart)
        begin++;

    /* length until end character */
    len_s = strcspn(begin, cEnd);

    /* copy it */
    memcpy(target, begin, MIN(len_s, target_len));

    /* null terminate */
    target[MIN(len_s, target_len)] = '\0';

    /* move ahead past argument */
    begin += len_s;

    /* skip over the end character */
    if (*begin == cEnd[0])
        begin++;

    /* skip out of white-space */
    while (isspace(*begin))
        begin++;

    return begin;
}

/* prepare a how message */
void
prepare_emote_message(const char *pre, const char *message, const char *post, char *buf)
{
    sprintf(buf, "%s%s%s%s%s\n\r", pre[0] != '\0' ? pre : "", pre[0] != '\0' ? ", " : "", message,
            post[0] != '\0' ? ", " : "", post[0] != '\0' ? post : "");
}

/*
 * Used to extract how a command is being done from the arguments given
 * to the command.
 *
 * Expects the arguments to be in the format:
 *
 *   [pre_emote] [arguments] [post_emote] [more arguments...]
 *
 * example:
 *
 *   sit -farting at table (his face turning red)
 *
 *   Farting, Morgenes sits at the table, his face turning red.
 *
 * The following would be passed to this argument as source:
 *   -farting at table (his face turning red)
 *
 * It will pull out 'farting' and 'his face turning red' as pre_emote and
 * post_emote respectively.  'at table' will be returned as the true arguments
 * to the command.
 *
 * Utilizes one_argument_delim() to pull the argument with a given start
 * delimiter.
 */
char *
extract_emote_arguments(const char *source, char *pre_emote, size_t pre_len, char *args,
                        size_t args_len, char *post_emote, size_t post_len)
{
    /* where to start looking each time */
    const char *begin = source;

    /* start at the begging of args */
    char *argsBegin = args;

    /* keep track of if our last character was a space */
    bool was_space = TRUE;

    /* keep track of if our first was a bracket */
    bool first_was_bracket = FALSE;

    /* pre-initialize our output variables */
    pre_emote[0] = '\0';
    post_emote[0] = '\0';
    args[0] = '\0';

    /* if no args given, return an empty string */
    if (!source || !strlen(source)) {
        return args;
    }

    /* move to first non-space */
    while (isspace(*begin))
        begin++;

    /* extract arguments up to the next how */
    /* loop through the remainder of source until we run into the
     * end, or a 'how' start character
     */
    while (*begin != '\0') {    /* not at the end of the string */
        /* needs to have been a space before these special chars */
        if (was_space
            /* and if we haven't already pulled two how's */
            && (pre_emote[0] == '\0' || post_emote[0] == '\0')
            /* and if it's a 'how' start character */
            && (*begin == '(' || *begin == '-' || *begin == '[' || *begin == '*')) {
            /* if it's a bracket */
            if (*begin == '[') {
                /* and we don't have a pre_emote or post_emote */
                if (pre_emote[0] == '\0' && post_emote[0] == '\0') {
                    /* save that our first was a bracket */
                    first_was_bracket = TRUE;
                } else if (first_was_bracket) { /* if first was a bracket */
                    /* swap post back into first, and clear post */
                    strcpy(pre_emote, post_emote);
                    post_emote[0] = '\0';
                }
                begin = one_argument_delim(begin, post_emote, post_len, *begin);
            } else if (pre_emote[0] == '\0') {  /* if no pre, put in pre */
                begin = one_argument_delim(begin, pre_emote, pre_len, *begin);
            } else {            /* put it in post */
                begin = one_argument_delim(begin, post_emote, post_len, *begin);
            }
            continue;
        }

        /* end if it's 'how' */
        /* remember if this is a space */
        if (!isspace(*begin)) {
            was_space = FALSE;
        } else {
            was_space = TRUE;
        }

        /* if still space on args */
        if (argsBegin - args < args_len) {
            *argsBegin = *begin;
            argsBegin++;
            begin++;
        } else {                /* no space, stop looking */
            break;
        }
    }

    /* terminate args */
    *argsBegin = '\0';

    return args;
}

void
string_editor(CHAR_DATA * ch, char **pString, int maxlen, int mode)
{
    char *mode_name[] = {
        "create",
        "append",
        ""
    };

    cprintf(ch, "Text Editor: (%d character maximum) [%s mode]\n\r", maxlen, mode_name[mode]);
    cprintf(ch, "Type .h on a new line for help.  Terminate with '%c' or .q on a new line.\n\r",
            ch->desc->player_info->edit_end);
    cprintf(ch, "[---|----|----|----|----|----|----|----|----|----|----|----|----|----|----]\n\r");

    if (*pString == NULL) {
        *pString = strdup("");
    }

    switch (mode) {
    case EDITOR_APPEND_MODE:
        cprintf(ch, "%s", *pString);
        break;
    case EDITOR_CREATE_MODE:
    default:
        if (*pString == NULL) {
            free(*pString);
            *pString = strdup("");
        }
        break;
    }
    ch->desc->str = pString;
    ch->desc->max_str = maxlen;
}

void
string_edit(CHAR_DATA * ch, char **pString, int maxlen)
{
    string_editor(ch, pString, maxlen, EDITOR_CREATE_MODE);
}

void
string_edit_verbose(CHAR_DATA * ch, char **pString, int maxlen, char *text)
{
    ch->desc->editing_display = strdup(text);
    if (!ch->desc->editing_display) {
        shhlog("Error, could not allocate memory for editing_display");
    };

    string_editor(ch, pString, maxlen, EDITOR_CREATE_MODE);
}

void
string_append(CHAR_DATA * ch, char **pString, int maxlen)
{
    string_editor(ch, pString, maxlen, EDITOR_APPEND_MODE);
}

void
string_append_verbose(CHAR_DATA * ch, char **pString, int maxlen, char *text)
{
    ch->desc->editing_display = strdup(text);
    if (!ch->desc->editing_display) {
        shhlog("Error, could not allocate memory for editing_display");
    };

    string_editor(ch, pString, maxlen, EDITOR_APPEND_MODE);
}

void
string_edit_new(CHAR_DATA * ch, char **pString, int maxlen)
{
    string_editor(ch, pString, maxlen, EDITOR_CREATE_MODE);
}

void
strdupcat(char **str, const char *add)
{
    if (!add)
        return;

    if (!*str) {
        *str = strdup(add);
        return;
    }

    size_t newlen = strlen(*str) + strlen(add) + 1;
    char *newstr = malloc(newlen);
    snprintf(newstr, newlen, "%s%s", *str, add);        // Should also NULL-terminate
    free(*str);
    *str = newstr;
}


/*****************************************************************************
 Name:          first_arg
 Purpose:       Pick off one argument from a string and return the rest.
                Understands quates, parenthesis (barring ) ('s) and
                percentages.
 ****************************************************************************/
char *
first_arg(char *argument, char *arg_first, bool fCase)
{
    char cEnd;

    while (*argument == ' ')
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"' || *argument == '%' || *argument == '(') {
        if (*argument == '(') {
            cEnd = ')';
            argument++;
        } else
            cEnd = *argument++;
    }

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        if (fCase)
            *arg_first = LOWER(*argument);
        else
            *arg_first = *argument;
        arg_first++;
        argument++;
    }
    *arg_first = '\0';

    while (*argument == ' ')
        argument++;

    return argument;
}

