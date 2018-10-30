/*
 * File: cmd_getopt.c */
#include <alloca.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>

#include "structs.h"
#include "gamelog.h"
#include "handler.h"
#include "cmd_getopt.h"

#define MIN(x,y) ((x) < (y) ? (x) : (y))


// Returns argc
int
string_to_argv(const char *string, char ***result)
{
    char *newstring = alloca(strlen(string) + 1);
    const char *read = string;
    char *write = newstring;
    int saveargs[256];
    int i, argc = 1;

    memcpy(newstring, string, strlen(string) + 1);

    while (*read) {
        while (*read && isspace(*read))
            read++;             // Jump spaces

        if (*read)
            saveargs[argc++] = write - newstring;       // Offset

        while (*read && !isspace(*read)) {
            if (*read && *read == '"') {
                read++;
                while (*read && *read != '"')
                    *write++ = *read++;
                *write = '\0';
                read++;
            } else {
                *write++ = *read++;
            }
        }

        *write++ = '\0';

        if (*read)
            read++;
    }

    if (argc) {
        char *retstr;
        char **argv;
        *result = malloc(sizeof(char *) * argc + strlen(string) + 1);
        argv = *result;
        retstr = (char *) *result + sizeof(char *) * argc;
        memcpy(retstr, newstring, strlen(string) + 1);
        for (i = 0; i < argc; i++) {
            argv[i] = retstr + saveargs[i];     // Real address
        }
        argv[0] = "bogus_command_name";
    } else {
        *result = 0;
    }

    return argc;
}

void
free_argv(char **argv)
{
    free(argv);
}

int
split_flags(const char *flagstr, const struct flag_data *flag_array)
{
    const char delims[] = "|+, ";
    int retval = 0;
    const char *tmp = flagstr;
    if (!flagstr)
        return 0;

    while (*tmp != 0) {
        size_t toklen = strcspn(tmp, delims);
        int i;
        for (i = 0; *flag_array[i].name; i++) {
            if (strncasecmp(flag_array[i].name, tmp, toklen) == 0) {
                retval |= flag_array[i].bit;
            }
        }
        tmp += toklen + strspn(tmp + toklen, delims);
    }

    return retval;
}

int
cmd_getopt(const arg_struct * argl, size_t argslen, const char *arg_text)
{
    int i, val = 0, index = 0;
    char **argv;
    char *optshorts = (char *) alloca(argslen * 4 + 1);
    struct option *goopts = (struct option *) alloca(sizeof(struct option) * (argslen + 1));

    int argc = string_to_argv(arg_text, &argv);
    int shortindex = 0;

    for (i = 0; i < argslen; i++) {
        optshorts[shortindex++] = argl[i].shortcut;
        goopts[i].name = argl[i].name;
        goopts[i].has_arg = no_argument;        // for now
        goopts[i].flag = 0;
        goopts[i].val = argl[i].shortcut;

        // Two colons for argtypes that might have results but don't require them
        if (argl[i].type == ARG_PARSER_STRING || argl[i].type == ARG_PARSER_INT
            || argl[i].type == ARG_PARSER_FLAG_INT || argl[i].type == ARG_PARSER_CHAR
            || argl[i].type == ARG_PARSER_ROOM || argl[i].type == ARG_PARSER_OBJ) {
            optshorts[shortindex++] = ':';
            optshorts[shortindex++] = ':';
            goopts[i].has_arg = optional_argument;
        }
        // One colon for argtypes that might have results and require them
        else if (argl[i].type == ARG_PARSER_REQUIRED_STRING
                 || argl[i].type == ARG_PARSER_REQUIRED_INT
                 || argl[i].type == ARG_PARSER_REQUIRED_FLAG_INT
                 || argl[i].type == ARG_PARSER_REQUIRED_CHAR
                 || argl[i].type == ARG_PARSER_REQUIRED_ROOM
                 || argl[i].type == ARG_PARSER_REQUIRED_OBJ) {
            optshorts[shortindex++] = ':';
            goopts[i].has_arg = required_argument;
        }
    }
    optshorts[shortindex] = '\0';
    memset(&goopts[argslen], 0, sizeof(struct option));

    optarg = 0;
    optind = 0;
    opterr = 0;
    do {
        int armoptind = 0;
//        val = getopt(argc, argv, optshorts);
        val = getopt_long(argc, argv, optshorts, goopts, &index);
        if (val == -1)
            break;

        while (armoptind < argslen && val != argl[armoptind].shortcut)
            armoptind++;
        if (argl[armoptind].shortcut != val)
            break;

        switch (argl[armoptind].type) {
        case ARG_PARSER_STRING:
        case ARG_PARSER_REQUIRED_STRING:
            if (optarg && *optarg)
                snprintf(argl[armoptind].u1.string, argl[armoptind].u2.size, "%s", optarg);
            break;
        case ARG_PARSER_FLAG_INT:
        case ARG_PARSER_REQUIRED_FLAG_INT:
            if (optarg && *optarg)
                *(int *) argl[armoptind].u1.intval = split_flags(optarg, argl[armoptind].u2.flags);
            break;
        case ARG_PARSER_INT:
        case ARG_PARSER_REQUIRED_INT:
            if (optarg && *optarg)
                *(int *) argl[armoptind].u1.intval = atoi(optarg);
            break;
        case ARG_PARSER_BOOL:
            *(int *) argl[armoptind].u1.intval = 1;
            break;
        case ARG_PARSER_ROOM:
        case ARG_PARSER_REQUIRED_ROOM:
            if (optarg && *optarg) {
                ROOM_DATA *tmp = get_room_num(atoi(optarg));
                *argl[armoptind].u1.room = tmp;
            }
            break;
        case ARG_PARSER_CHAR:
        case ARG_PARSER_REQUIRED_CHAR:
            if (optarg && *optarg) {
                CHAR_DATA *ch = (CHAR_DATA *) argl[armoptind].u2.ex_ch;
                CHAR_DATA *tmp = get_char_world(ch, optarg);
                *argl[armoptind].u1.ch = tmp;
            }
            break;
        case ARG_PARSER_OBJ:
        case ARG_PARSER_REQUIRED_OBJ:
            gamelog("argument parsing for objects not implemented");
            break;
        default:
            break;
        }
    } while (val != -1);

    while (optind < argc) {
        shhlogf("[%d] %s", optind, argv[optind]);
        optind++;
    }

    free_argv(argv);
    return argc;
}


void
page_help(CHAR_DATA * ch, const arg_struct * argl, size_t argslen)
{
    int i, total_len = 0;
    char helpstr[MAX_STRING_LENGTH] = "";
    char *tmp = helpstr;

    for (i = 0; i < argslen; i++) {
        int added_ch = snprintf(tmp, sizeof(helpstr) - total_len, "    -%c,   --%-15s   %s\n\r",
                                argl[i].shortcut, argl[i].name, argl[i].description);
        total_len += added_ch;
        tmp = helpstr + total_len;
    }

    page_string(ch->desc, helpstr, 1);
}

