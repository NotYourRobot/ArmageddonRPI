/*
 * File: cmd_getopt.h */
typedef enum
{
    ARG_PARSER_STRING,
    ARG_PARSER_REQUIRED_STRING,
    ARG_PARSER_INT,
    ARG_PARSER_REQUIRED_INT,
    ARG_PARSER_FLAG_INT,
    ARG_PARSER_REQUIRED_FLAG_INT,
    ARG_PARSER_BOOL,
    ARG_PARSER_ROOM,
    ARG_PARSER_REQUIRED_ROOM,
    ARG_PARSER_CHAR,
    ARG_PARSER_REQUIRED_CHAR,
    ARG_PARSER_OBJ,
    ARG_PARSER_REQUIRED_OBJ,
}
arg_type;

typedef struct __arg_struct
{
    char shortcut;              // The shortcut of the argument
    const char *name;           // The name of the argument
    arg_type type;              // The type of the argument (like a format)
    union
    {                           // Where to put the resultant data
        void *addr;
        char *string;
        int *intval;
        ROOM_DATA **room;
        CHAR_DATA **ch;
        OBJ_DATA **obj;
    } u1;

    union
    {
        void *extra;
        size_t size;
        CHAR_DATA *ex_ch;
        struct flag_data *flags;
    } u2;

    const char *description;
} arg_struct;

int string_to_argv(const char *string, char ***result);
int cmd_getopt(const arg_struct * argl, size_t argslen, const char *arg_text);
void page_help(CHAR_DATA * ch, const arg_struct * argl, size_t argslen);

