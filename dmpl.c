/*
 * File: DMPL.C
 * Usage: DMPL -- Dan's MUD Programming Language.
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

#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>

#include "constants.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "guilds.h"
#include "skills.h"
#include "creation.h"
#include "parser.h"
#include "handler.h"
#include "modify.h"
#include "utility.h"
#include "clan.h"

#define MAX		4096

#define MAX_LINE	5000

/* 4/9/98 - Tiernan */

/* local vars */
int lcount;
int dead;
int extracted;
char prog_name[MAX];

#define PROG_NPC	0
#define PROG_OBJECT	1
#define PROG_ROOM	2

struct program_data
{
    int nr;
    int type;
    char *name;
    float total_used;
    int num_times_ran;
    struct program_data *next;
};

struct program_data *program_list = (struct program_data *) 0;

void com_print();
void com_var();
void com_copy();
void com_command();
void com_store();
void com_fetch();
void com_rsend(const char *);
void com_zsend(const char *);
void com_csend();
void com_csend_ex();
void com_csend_ex_two();
void com_strlit();
void com_csend_nocap();
void com_csend_ex_nocap();
void com_csend_ex_two_nocap();
void com_log();

#define D_CMD_COMMENT	-2
#define D_CMD_NOT_FOUND	-1
#define D_CMD_IF 		3
#define D_CMD_SIF		4
#define D_CMD_ENDIF	5
#define D_CMD_RETURN	6
#define D_CMD_EXIT	7
#define D_CMD_LABEL	16
#define D_CMD_GOTO	17
#define D_CMD_SUB         21
#define D_CMD_GOSUB       22
#define D_CMD_ENDSUB      23

struct command_data
{
    char *name;
    void (*func) ();
};

struct command_data command[] = {
    /* be carefull with modifying this list, hardcoded values #defined above */
    {"print", com_print},
    {"var", com_var},
    {"copy", com_copy},
    {"if", 0},                  /* special case */
    {"sif", 0},                 /* special case */
    {"endif", 0},               /* special case */
    {"return", 0},              /* special case */
    {"exit", 0},                /* special case */
    {"command", com_command},
    {"store", com_store},
    {"fetch", com_fetch},
    {"rsend", com_rsend},
    {"csend", com_csend},
    {"csend_ex", com_csend_ex},
    {"csend_ex_two", com_csend_ex_two},
    {"strlit", com_strlit},
    {"label", 0},               /* special case */
    {"goto", 0},                /* special case */
    {"csend_nocap", com_csend_nocap},
    {"csend_ex_nocap", com_csend_ex_nocap},
    {"csend_ex_two_nocap", com_csend_ex_two_nocap},
    {"sub", 0},
    {"gosub", 0},
    {"endsub", 0},
    {"log", com_log},
    {"zsend", com_zsend},
    {"", 0}
};

struct value_data
{
    int val;
    void *ptr;
};

#define VAR_VOID	0
#define VAR_INT		1
#define VAR_ROOM	2
#define VAR_OBJECT	3
#define VAR_CHAR	4
#define VAR_STRING	5

struct var_data
{
    char *name;
    int type;
    struct value_data value;

    struct var_data *next;
};

struct var_data *var_list = (struct var_data *) 0;
struct var_data *pdf_list = (struct var_data *) 0;

int copy_string_check = 0;

#define MAX_ARGS	10

/* DMPL FUNCTIONS */
struct function_data
{
    char *name;
    void (*func) (struct value_data * val[MAX_ARGS], struct value_data * ret);
};

/*
 * To add a new function add a proto here put a d_ before the function name 
 * you're going to add.
 */
void d_char_to_room();
void d_char_from_room();
void d_travel();
void d_get_room_num();
void d_get_room_dir();
void d_number();
void d_dice();
void d_atoi();
void d_ch_can_see_ch();
void d_ch_can_see_obj();
void d_real_obj();
void d_real_npc();
void d_get_ch_world();
void d_get_ch_world_vis();
void d_get_ch_room();
void d_get_ch_room_num();
void d_get_ch_room_vis();
void d_get_arg_ch_room_vis();
void d_read_obj();
void d_read_npc();
void d_get_obj_world();
void d_get_obj_world_num();
void d_get_obj_world_vis();
void d_get_obj_equipped();
void d_get_obj_equipped_vis();
void d_get_obj_room();
void d_get_obj_obj_vis();
void d_get_obj_inv();
void d_poison();
void d_die();
void d_obj_from_room();
void d_obj_to_room();
void d_obj_from_char();
void d_obj_to_char();
void d_extract_char();
void d_extract_obj();
void d_game_time();
void d_is_affected();
void d_is_flagged();
void d_weight_change();
void spec_d_one_arg();
void d_is_name();
void d_size_match();
void d_obj_to_obj();
void d_obj_from_obj();
void d_equip_char();
void d_unequip_char();
void d_affect_to_char();
void d_send_translated();
void d_is_tribe();
void d_is_char_guarding_exit();
void d_is_in_same_tribe();
void d_add_tribe();
void d_remove_tribe();
void d_is_tribe_leader();
void d_cmp_tribe_rank();
void d_get_tribe_rank();
void d_set_tribe_rank();
void d_has_tribe_job();
void d_set_tribe_job();
void d_remove_tribe_job();
void d_knock_out();
void d_get_char_edesc();
void d_set_char_edesc();
void d_get_obj_edesc();
void d_set_obj_edesc();

#define FUNC_ONE_ARG	39

/*
 * Then add the name you want to use in DMPL programs into the table here, 
 * referring back to the d_ prototype you defined above.
 */
struct function_data function[] = {
    {"char_to_room", d_char_to_room},
    {"char_from_room", d_char_from_room},
    {"travel", d_travel},
    {"get_room_num", d_get_room_num},
    {"get_room_dir", d_get_room_dir},
    {"number", d_number},
    {"dice", d_dice},
    {"atoi", d_atoi},
    {"ch_can_see_ch", d_ch_can_see_ch},
    {"ch_can_see_obj", d_ch_can_see_obj},
    {"real_obj", d_real_obj},
    {"real_npc", d_real_npc},
    {"get_ch_world", d_get_ch_world},
    {"get_ch_world_vis", d_get_ch_world_vis},
    {"get_ch_room", d_get_ch_room},
    {"get_ch_room_num", d_get_ch_room_num},
    {"get_ch_room_vis", d_get_ch_room_vis},
    {"get_arg_ch_room_vis", d_get_arg_ch_room_vis},
    {"read_obj", d_read_obj},
    {"read_npc", d_read_npc},
    {"get_obj_world", d_get_obj_world},
    {"get_obj_world_num", d_get_obj_world_num},
    {"get_obj_world_vis", d_get_obj_world_vis},
    {"get_obj_equipped", d_get_obj_equipped},
    {"get_obj_equipped_vis", d_get_obj_equipped_vis},
    {"get_obj_room", d_get_obj_room},
    {"get_obj_inv", d_get_obj_inv},
    {"poison", d_poison},
    {"die", d_die},
    {"obj_from_room", d_obj_from_room},
    {"obj_to_room", d_obj_to_room},
    {"obj_from_char", d_obj_from_char},
    {"obj_to_char", d_obj_to_char},
    {"extract_char", d_extract_char},
    {"extract_obj", d_extract_obj},
    {"game_time", d_game_time},
    {"is_affected", d_is_affected},
    {"is_flagged", d_is_flagged},
    {"weight_change", d_weight_change},
    {"one_arg", 0},             /* special function */
    {"is_name", d_is_name},
    {"size_match", d_size_match},
    {"obj_to_obj", d_obj_to_obj},
    {"obj_from_obj", d_obj_from_obj},
    {"get_obj_obj_vis", d_get_obj_obj_vis},
    {"equip_char", d_equip_char},
    {"unequip_char", d_unequip_char},
    {"affect_to_char", d_affect_to_char},
    {"send_translated", d_send_translated},
    {"is_tribe", d_is_tribe},
    {"is_char_guarding_exit", d_is_char_guarding_exit},
    {"is_in_same_tribe", d_is_in_same_tribe},
    {"add_tribe", d_add_tribe},
    {"remove_tribe", d_remove_tribe},
    {"is_tribe_leader", d_is_tribe_leader},
    {"cmp_tribe_rank", d_cmp_tribe_rank},
    {"get_tribe_rank", d_get_tribe_rank},
    {"set_tribe_rank", d_set_tribe_rank},
    {"has_tribe_job", d_has_tribe_job},
    {"set_tribe_job", d_set_tribe_job},
    {"remove_tribe_job", d_remove_tribe_job},
    {"get_char_edesc", d_get_char_edesc},
    {"set_char_edesc", d_set_char_edesc},
    {"get_obj_edesc", d_get_obj_edesc},
    {"set_obj_edesc", d_set_obj_edesc},
    {"knock_out", d_knock_out},

    {"", 0}
};

/* local functions */
void error(const char *str, ...) __attribute__ ((format(printf, 1, 2)));

/* dmpl functions */
/*
 * To add a new one make sure you've defined a prototype (above) and create
 * an entry in the table (above) then create a corresponding function below.
 */
void
d_char_to_room(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    char_to_room((CHAR_DATA *) val[0]->ptr, (ROOM_DATA *) val[1]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

/* make a character walk to a room */
void
d_travel(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    CHAR_DATA *ch;
    int dist;
    char command[MAX_INPUT_LENGTH];

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ch = (CHAR_DATA *) val[0]->ptr;
    dist = choose_exit_name_for(ch->in_room, (ROOM_DATA *) val[1]->ptr, command, 100, ch);
    if (dist > -1) {
        parse_command(ch, command);
        ret->val = 1;
    } else {
        ret->val = -1;
    }
    ret->ptr = (void *) 0;
}

void
d_char_from_room(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    char_from_room((CHAR_DATA *) val[0]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

void
d_get_room_num(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    ret->val = 0;
    ret->ptr = get_room_num(val[0]->val);
}

void
d_get_room_dir(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    CHAR_DATA *ch;


    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!(ch = (CHAR_DATA *) val[0]->ptr)) {
        error("bad char struct");
        return;
    }

    ret->val = 0;
    ret->ptr = get_room_distance(ch, 1, val[1]->val);
}

void
d_number(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    ret->val =
        (val[0]->val > val[1]->val) ? number(val[1]->val, val[0]->val) : number(val[0]->val,
                                                                                val[1]->val);
    ret->ptr = (void *) 0;
}

void
d_dice(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (val[0]->val < 1 || val[1]->val < 1) {
        error("arguments greater than 1 expected");
        return;
    }

    ret->val = dice(val[0]->val, val[1]->val);
    ret->ptr = (void *) 0;
}

void
d_atoi(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    ret->val = atoi((char *) val[0]->ptr);
    ret->ptr = (void *) 0;
}

void
d_ch_can_see_ch(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = char_can_see_char((CHAR_DATA *) val[0]->ptr, (CHAR_DATA *) val[1]->ptr);
    ret->ptr = (void *) 0;
}

void
d_ch_can_see_obj(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = char_can_see_obj((CHAR_DATA *) val[0]->ptr, (OBJ_DATA *) val[1]->ptr);
    ret->ptr = (void *) 0;
}

void
d_real_obj(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (val[0]->val < 0) {
        error("argument greater than or equal to 0 expected");
        return;
    }

    ret->val = real_object(val[0]->val);
    ret->ptr = (void *) 0;
}

void
d_real_npc(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (val[0]->val < 0) {
        error("argument greater than or equal to 0 expected");
        return;
    }

    ret->val = real_mobile(val[0]->val);
    ret->ptr = (void *) 0;
}

void
d_get_ch_world(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr = get_char((char *) val[0]->ptr);
}

void
d_get_ch_world_vis(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr = get_char_vis((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr);
}

void
d_get_ch_room(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr = get_char_room((char *) val[0]->ptr, (ROOM_DATA *)
                             val[1]->ptr);
}

void
d_get_ch_room_num(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr = get_char_room_num(val[0]->val, (ROOM_DATA *) val[1]->ptr);
}

void
d_get_ch_room_vis(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr = get_char_room_vis((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr);
}

void
d_get_arg_ch_room_vis(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    char buffer[MAX];

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    val[2]->val = 0;
    if (val[2]->ptr) {
        free((char *) val[2]->ptr);
        val[2]->ptr = (char *) 0;
    }

    ret->val = 0;
    ret->ptr = (void *) 0;

    get_arg_char_room_vis((CHAR_DATA *) val[0]->ptr, (CHAR_DATA *) val[1]->ptr, buffer);

    CREATE(val[2]->ptr, char, strlen(buffer) + 1);
    strcpy((char *) val[2]->ptr, buffer);
}

void
d_read_npc(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (val[0]->val < 0) {
        error("argument greater than or equal to 0 expected");
        return;
    }

    ret->val = 0;
    ret->ptr = read_mobile(val[0]->val, VIRTUAL);
}

void
d_read_obj(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (val[0]->val < 0) {
        error("argument greater than or equal to 0 expected");
        return;
    }

    ret->val = 0;
    ret->ptr = read_object(val[0]->val, VIRTUAL);
}

void
d_get_obj_world(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr = get_obj((char *) val[0]->ptr);
}

void
d_get_obj_world_num(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (val[0]->val < 0) {
        error("argument greater than or equal to 0 expected");
        return;
    }

    ret->val = 0;
    ret->ptr = get_obj_num(val[0]->val);
}

void
d_get_obj_world_vis(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr || !val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr = get_obj_vis((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr);
}

void
d_get_obj_equipped(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr || !val[1]->ptr) {
        error("arg1 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr = get_obj_equipped((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr, FALSE);
}

void
d_get_obj_equipped_vis(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }


    ret->val = 0;
    ret->ptr = get_obj_equipped((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr, TRUE);
}

void
d_get_obj_room(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr =
        get_obj_in_list_vis((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr,
                            ((CHAR_DATA *) val[0]->ptr)->in_room->contents);
}

void
d_get_obj_inv(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = 0;
    ret->ptr =
        get_obj_in_list_vis((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr,
                            ((CHAR_DATA *) val[0]->ptr)->carrying);
}

void
d_get_obj_obj_vis(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    if (!val[2]->ptr) {
        error("arg3 null pointer");
        return;
    }


    ret->val = 0;
    ret->ptr =
        get_obj_in_list_vis((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr,
                            ((OBJ_DATA *) val[2]->ptr)->contains);
}


void
d_poison(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

/****************************************************************/
    /* val[1]->ptr is not used, only val[0]->ptr                    */
    /* removing this and replaceing it without the val[1]->ptr      */
    /* if ( !val[0]->ptr || !val[1]->ptr )                          */
    /*    {                                                         */
    /*      error( "null pointer" );                                */
    /*      return;                                                 */
    /*    }                                                         */
    /*                                                              */
    /*         -Tenebrius 8/6/97                                    */
/****************************************************************/

    if (!val[0]->ptr) {
        error("null pointer, arg[0], should be character pointer");
        return;
    }

    if (!does_save((CHAR_DATA *) val[0]->ptr, SAVING_PARA, 0)) {
/* Function now allows for non-generic poisons 
 * 4/9/98 - Tiernan */
        if (val[2]->val)        /* If we have a 3rd value passed in, its the type */
            poison_char((CHAR_DATA *) val[0]->ptr, (int) val[2]->val, (int) val[1]->val, 0);
        else
            poison_char((CHAR_DATA *) val[0]->ptr, 0, (int) val[1]->val, 0);
        ret->val = 1;
    } else {
        ret->val = 0;
    }
    ret->ptr = (void *) 0;
}

void
d_die(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    die((CHAR_DATA *) val[0]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

/* added some checking for valid eq positions - Turgon */
void
d_equip_char(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }
    if ((val[2]->val < 0) || (val[2]->val >= MAX_WEAR)) {
        error("invalid eq position value");
        return;
    }

    if (((CHAR_DATA *) val[0]->ptr)->equipment[val[2]->val])
        error("item already equipped at that position");
    else
        equip_char((CHAR_DATA *) val[0]->ptr, (OBJ_DATA *) val[1]->ptr, val[2]->val);
    ret->val = 0;
    ret->ptr = (void *) 0;
}

/* changed ret type to obj_data, added some checking - Turgon */
void
d_unequip_char(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if ((val[1]->val < 0) || (val[1]->val >= MAX_WEAR)) {
        error("invalid eq position value");
        return;
    }

    ret->val = 0;
    if (!((CHAR_DATA *) val[0]->ptr)->equipment[val[1]->val])
        ret->ptr = (OBJ_DATA *) 0;
    else
        ret->ptr = unequip_char((CHAR_DATA *) val[0]->ptr, val[1]->val);
}

void
d_obj_to_room(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    obj_to_room((OBJ_DATA *) val[0]->ptr, (ROOM_DATA *) val[1]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

void
d_obj_from_room(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    obj_from_room((OBJ_DATA *) val[0]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

void
d_obj_to_char(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    obj_to_char((OBJ_DATA *) val[0]->ptr, (CHAR_DATA *) val[1]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

void
d_obj_from_char(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    obj_from_char((OBJ_DATA *) val[0]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

void
d_obj_from_obj(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    obj_from_obj((OBJ_DATA *) val[0]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

void
d_obj_to_obj(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr || !val[1]->ptr) {
        error("arg1 null pointer");
        return;
    }

    obj_to_obj((OBJ_DATA *) val[0]->ptr, (OBJ_DATA *) val[1]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

void
d_extract_char(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    extract_char((CHAR_DATA *) val[0]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

void
d_extract_obj(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    extract_obj((OBJ_DATA *) val[0]->ptr);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

void
d_game_time(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    int type;

    type = val[0]->val;

    if (type == 0)
        ret->val = time_info.hours;
    else if (type == 1)
        ret->val = time_info.day;
    else if (type == 2)
        ret->val = time_info.month;
    else if (type == 3)
        ret->val = time_info.year;
#ifdef DOTW_SUPPORT
    else if (type == 4)
        ret->val = (time_info.day + 1) % MAX_DOTW;
#endif
    else {
        error("unknown time type");
        return;
    }

    ret->ptr = (void *) 0;
}

/* returns true if val0 is in val1 */
void
d_is_name(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = isallnamebracketsok((char *) val[0]->ptr, (char *) val[1]->ptr);
    ret->ptr = (void *) 0;
}


/* determine if a character is affected by... */
void
d_is_affected(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    CHAR_DATA *ch;
    struct affected_type *af;
    char buf[128];

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ch = (CHAR_DATA *) val[0]->ptr;

    for (af = ch->affected; af; af = af->next) {
        sprint_flag(af->bitvector, affect_flag, buf);
        /* was  if (!strstr (buf, (char *) val[1]->ptr)) -Tenebrius 11/18/99 */
        if (strstr(buf, (char *) val[1]->ptr)) {
            ret->val = af->duration;
            ret->ptr = (void *) 0;
            return;
        }
    }
    sprint_flag(ch->specials.affected_by, affect_flag, buf);
    if (strstr(buf, (char *) val[1]->ptr)) {
        ret->val = 0;
        ret->ptr = (void *) 0;
        return;
    }

    ret->val = -1;
    ret->ptr = (void *) 0;
}

void
d_affect_to_char(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    af.type = val[1]->val;
    af.duration = val[2]->val;
    af.modifier = val[3]->val;
    af.location = val[4]->val;
    af.bitvector = val[5]->val;

    if (val[6] && val[6]->val)
        af.power = val[6]->val;

    if (val[7] && val[7]->val)
        af.magick_type = val[7]->val;

    affect_to_char((CHAR_DATA *) val[0]->ptr, &af);

    ret->val = 0;
    ret->ptr = (void *) 0;
}

/* changes weight of object */
void
d_weight_change(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    ret->ptr = (void *) 0;
    ret->val = 0;
}

/* checks char size vs. object size */
void
d_size_match(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = size_match((CHAR_DATA *) val[0]->ptr, (OBJ_DATA *) val[1]->ptr);
    ret->ptr = (void *) 0;
}

/* boolean if char is flagged by named flag ... */
void
d_is_flagged(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    CHAR_DATA *ch;
    int n;

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ch = (CHAR_DATA *) val[0]->ptr;
    for (n = 0; n < 32; n++)
        if (!strcmp((char *) val[1]->ptr, char_flag[n].name))
            break;

    if ((n < 32) && IS_SET(ch->specials.act, char_flag[n].bit))
        ret->val = 1;
    else
        ret->val = 0;
    ret->ptr = (void *) 0;
}


/* return boolean if ch is in tribe */
void
d_is_tribe(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    ret->val = IS_TRIBE((CHAR_DATA *) val[0]->ptr, val[1]->val);
    ret->ptr = (void *) 0;
    return;
}

/* Returns true if ch is guarding the exit specified */
void
d_is_char_guarding_exit(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if ((((CHAR_DATA *) val[0]->ptr)->specials.dir_guarding != -1)
        && (((CHAR_DATA *) val[0]->ptr)->specials.dir_guarding == val[1]->val)
        && (((CHAR_DATA *) val[0]->ptr)->in_room->direction[val[1]->val])
        && IS_SET((((CHAR_DATA *) val[0]->ptr)->in_room->direction[val[1]->val]->exit_info),
                  EX_GUARDED)) {
        ret->val = TRUE;
        ret->ptr = (void *) 0;
    } else {
        ret->val = FALSE;
        ret->ptr = (void *) 0;
    }

    return;
}



/* return first tribe both var0 and var1 share in common */
void
d_is_in_same_tribe(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    ret->val = IS_IN_SAME_TRIBE((CHAR_DATA *) val[0]->ptr, (CHAR_DATA *) val[1]->ptr);
    ret->ptr = (void *) 0;
    return;
}


/* return boolean if ch is in tribe & leader of it */
void
d_is_tribe_leader(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    ret->val = IS_TRIBE_LEADER(((CHAR_DATA *) val[0]->ptr), val[1]->val);
    ret->ptr = (void *) 0;
    return;
}

/* return boolean if adding ch to tribe is successful */
void
d_add_tribe(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    /* only add if not already in the tribe */
    if (IS_TRIBE((CHAR_DATA *) val[0]->ptr, val[1]->val))
        ret->val = FALSE;
    else {
        ret->val = TRUE;
        add_clan((CHAR_DATA *) val[0]->ptr, val[1]->val);
    }

    ret->ptr = (void *) 0;
    return;
}

/* return boolean if removing ch from tribe is successful */
void
d_remove_tribe(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    /* only remove if in the tribe */
    if (IS_TRIBE((CHAR_DATA *) val[0]->ptr, val[1]->val)) {
        ret->val = TRUE;
        remove_clan((CHAR_DATA *) val[0]->ptr, val[1]->val);
    } else
        ret->val = FALSE;

    ret->val = TRUE;

    ret->ptr = (void *) 0;
    return;
}


/* compare the two character's rank in clan
 *  return -1 if val0 < val1
 *  return 0  if val0 == val1
 *  return 1  if val0 > val1
 */
void
d_cmp_tribe_rank(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }
    ret->val = clan_rank_cmp((CHAR_DATA *) val[0]->ptr, (CHAR_DATA *) val[1]->ptr, val[2]->val);
    ret->ptr = (void *) 0;
    return;
}

/* get a character's rank in a specified tribe, returns 0 if not in tribe */
void
d_get_tribe_rank(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }
    ret->val = get_clan_rank(((CHAR_DATA *) val[0]->ptr)->clan, val[1]->val);
    ret->ptr = (void *) 0;
    return;
}

/* get a character's extra desc  returns 0 if no edesc */
void
d_get_char_edesc(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    char buffer[MAX];

    if (!val[0]->ptr) {
        error("arg1 null pointer, should be character");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer, should be edesc label string");
        return;
    }

    ret->val = 0;
    ret->ptr = (void *) 0;


    if (get_char_extra_desc_value((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr, buffer, MAX)) {
        ret->val = 0;
        gamelog("found an edesc");
        gamelog(buffer);
        CREATE(ret->ptr, char, strlen(buffer) + 1);
        strcpy(ret->ptr, buffer);
    } else {
        ret->val = 0;
        gamelog("couldent find an edesc");
        CREATE(ret->ptr, char, strlen("") + 1);
        strcpy(ret->ptr, "");
    }
}

/* get a character's extra desc  returns 0 if no edesc */
void
d_get_obj_edesc(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    char buffer[MAX];

    if (!val[0]->ptr) {
        error("arg1 null pointer, should be object");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer, should be edesc label string");
        return;
    }

    ret->val = 0;
    ret->ptr = (void *) 0;

    if (get_obj_extra_desc_value((OBJ_DATA *) val[0]->ptr, (char *) val[1]->ptr, buffer, MAX)) {
        ret->val = 1;
        CREATE(ret->ptr, char, strlen(buffer) + 1);
        strcpy(ret->ptr, buffer);
    }
}


void
d_set_char_edesc(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    //  char buffer[MAX];

    if (!val[0]->ptr) {
        error("arg1 null pointer, should be character");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer, should be edesc label string");
        return;
    }
    if (!val[2]->ptr) {
        error("arg3 null pointer, should be edesc description string");
        return;
    }

    ret->val = 0;
    ret->ptr = (void *) 0;

    if (set_char_extra_desc_value
        ((CHAR_DATA *) val[0]->ptr, (char *) val[1]->ptr, (char *) val[2]->ptr)) {
        ret->val = 1;
    }


}

void
d_set_obj_edesc(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    //  char buffer[MAX];

    if (!val[0]->ptr) {
        error("arg1 null pointer, should be character");
        return;
    }
    if (!val[1]->ptr) {
        error("arg2 null pointer, should be edesc label string");
        return;
    }
    if (!val[2]->ptr) {
        error("arg3 null pointer, should be edesc description string");
        return;
    }

    ret->val = 0;
    ret->ptr = (void *) 0;

    if (set_obj_extra_desc_value
        ((OBJ_DATA *) val[0]->ptr, (char *) val[1]->ptr, (char *) val[2]->ptr)) {
        ret->val = 1;
    }


}

/* set a character's rank in a specified tribe, return boolean of success */
void
d_set_tribe_rank(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    char buf[MAX_STRING_LENGTH];

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (val[1]->val < 0 || val[1]->val > MAX_CLAN) {
        error("set_clan_rank: clan is in the range of 0 to MAX_CLAN");
        return;
    }

    if (val[2]->val < MIN_CLAN_RANK || val[2]->val > clan_ranks[val[1]->val].num_ranks) {
        sprintf(buf, "set_clan_rank: rank is in the range %d to %d for" " tribe %d.", MIN_CLAN_RANK,
                clan_ranks[val[1]->val].num_ranks, val[1]->val);

        error("%s", buf);
        return;
    }

    /* only set if in the tribe  */
    if (IS_TRIBE((CHAR_DATA *) val[0]->ptr, val[1]->val)) {
        ret->val = TRUE;
        set_clan_rank(((CHAR_DATA *) val[0]->ptr)->clan, val[1]->val, val[2]->val);
    } else
        ret->val = FALSE;
    ret->ptr = (void *) 0;
    return;
}


/* return boolean if ch has specified job in tribe */
void
d_has_tribe_job(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    int flag = 0;

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[2]->ptr) {
        error("arg3 null pointer");
        return;
    }

    if ((flag = get_flag_bit(clan_job_flags, val[2]->ptr)) == -1) {
        error("No such flag");
        return;
    }

    ret->val = is_set_clan_flag(((CHAR_DATA *) val[0]->ptr)->clan, val[1]->val, flag);
    ret->ptr = (void *) 0;
    return;
}


/* set a character with job in specified tribe, return boolean of success */
void
d_set_tribe_job(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    int flag = 0;

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[2]->ptr) {
        error("arg3 null pointer");
        return;
    }

    if ((flag = get_flag_bit(clan_job_flags, val[2]->ptr)) == -1) {
        error("No such flag");
        return;
    }

    if (is_set_clan_flag(((CHAR_DATA *) val[0]->ptr)->clan, val[1]->val, flag))
        ret->val = FALSE;
    else {
        ret->val = TRUE;
        set_clan_flag(((CHAR_DATA *) val[0]->ptr)->clan, val[1]->val, flag);
    }
    ret->ptr = (void *) 0;
    return;
}

/* remove a job in specified tribe from ch, return boolean of success */
void
d_remove_tribe_job(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    int flag = 0;

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[2]->ptr) {
        error("arg3 null pointer");
        return;
    }

    if ((flag = get_flag_bit(clan_job_flags, val[2]->ptr)) == -1) {
        error("No such flag");
        return;
    }

    if (is_set_clan_flag(((CHAR_DATA *) val[0]->ptr)->clan, val[1]->val, flag)) {
        ret->val = TRUE;
        remove_clan_flag(((CHAR_DATA *) val[0]->ptr)->clan, val[1]->val, flag);
    } else
        ret->val = FALSE;
    ret->ptr = (void *) 0;
    return;
}


void
d_knock_out(struct value_data *val[MAX_ARGS], struct value_data *ret)
{
    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (val[1]->val < -1 || val[1]->val == 0) {
        error("knock_out: hours is 1 or greater, use -1 for random");
        return;
    }

    if (val[1]->val == -1) {
        val[1]->val = number(1, 3);
    }

    knock_out((CHAR_DATA *) val[0]->ptr, " in dmpl ", val[1]->val);

    ret->val = val[1]->val;
    ret->ptr = (void *) 0;
    return;
}

void
spec_d_one_arg(struct var_data *var1, struct var_data *var2, struct value_data *ret)
{
    char buf[MAX];
    const char *tmp;
/* Neither kelvik nor I (Tenebrius) knew exactly what var1 is supposed
   so I am assumeing that var1 is supposed to be returned exactly as it
   was sent in.
 */

    if (!var1 || !var2) {
        error("unknown variable");
        return;
    }

    if (!var1->value.ptr) {
        error("arg1 null pointer");
        return;
    }
    if (var2->type == VAR_STRING) {
        free(var2->value.ptr);
        var2->value.ptr = 0;
    }
/* My Stuff */
    tmp = one_argument((const char *) var1->value.ptr, buf, sizeof(buf));
    memmove(var1->value.ptr, tmp, strlen(tmp) + 1);

    ret->val = 0;
    ret->ptr = 0;

    var2->type = VAR_STRING;
    var2->value.ptr = strdup(buf);
}


/* direct call to send_translated in comm.c - Steve */
void
d_send_translated(struct value_data *val[MAX_ARGS], struct value_data *ret)
{

    if (!val[0]->ptr) {
        error("arg1 null pointer");
        return;
    }

    if (!val[1]->ptr) {
        error("arg2 null pointer");
        return;
    }

    if (!val[2]->ptr) {
        error("arg3 null pointer");
        return;
    }

    send_translated((CHAR_DATA *) val[0]->ptr, (CHAR_DATA *) val[1]->ptr, (char *) val[2]->ptr);
    ret->val = 0;
    ret->ptr = (void *) 0;
}


/* dmpl -- the language */
void
error(const char *str, ...)
{
    char buf[2048];
    va_list lst;

    va_start(lst, str);
    vsnprintf(buf, sizeof(buf), str, lst);
    va_end(lst);

    gamelogf("DMPL error, program %s, line %d: %s", prog_name, lcount, buf);
    dead = TRUE;
}

void
declare_var(char *name, int type, struct value_data *value)
{
    struct var_data *var;

    CREATE(var, struct var_data, 1);
    var->name = strdup(name);
    var->type = type;
    var->value.val = value->val;
    var->value.ptr = value->ptr;

    var->next = var_list;
    var_list = var;
}

void
declare_pdf(char *name, int type, struct value_data *value)
{
    struct var_data *var;

    CREATE(var, struct var_data, 1);
    var->name = strdup(name);
    var->type = type;
    var->value.val = value->val;
    var->value.ptr = value->ptr;

    var->next = pdf_list;
    pdf_list = var;
}

struct var_data *
get_var(char *name)
{
    struct var_data *var;

    while (*name == ' ')
        name++;

    for (var = var_list; var; var = var->next) {
        if (!stricmp(var->name, name))
            return var;
    }
    for (var = pdf_list; var; var = var->next) {
        if (!stricmp(var->name, name))
            return var;
    }
    return (struct var_data *) 0;
}

int
validate_var_list(void)
{
    struct var_data *var;
    char error_msg[300];
    for (var = var_list; var; var = var->next) {
        switch (var->type) {
        case VAR_ROOM:
            if (var->value.ptr && !validate_room_in_roomlist((ROOM_DATA *) var->value.ptr)) {
                sprintf(error_msg, "variable %s is of type ROOM and was not" "in the room list",
                        var->name);
                error("%s", error_msg);
                return (FALSE);
            }
            break;
        case VAR_OBJECT:
            if (var->value.ptr && !validate_obj_in_objlist((OBJ_DATA *) var->value.ptr)) {
                sprintf(error_msg, "variable %s is of type OBJECT and was not" "in the object list",
                        var->name);
                error("%s", error_msg);
                return (FALSE);
            }
            break;
        case VAR_CHAR:
            if (var->value.ptr && !validate_char_in_charlist((CHAR_DATA *) var->value.ptr)) {
                sprintf(error_msg,
                        "variable %s is of type CHARACTER and was not" "in the character list",
                        var->name);
                error("%s", error_msg);
                return (FALSE);
            }
            break;
        }
    }

    return (TRUE);
}

void
uninit_predefs(void)
{
    struct var_data *var, *next_pdf;

    var = pdf_list;
    while (var) {
        next_pdf = var->next;
        free(var->name);
        free(var);
        var = next_pdf;
    }

};

void
init_predefs()
{
    int i;
    char buf[MAX];
    struct value_data value;



    for (i = 1; *command_name[i] != '\n'; i++) {
        value.val = i;
        value.ptr = (void *) 0;
        sprintf(buf, "cmd_%s", command_name[i]);
        declare_pdf(buf, VAR_INT, &value);
    }

    for (i = 1; i < MAX_GUILDS; i++) {
        value.val = i;
        value.ptr = (void *) 0;
        sprintf(buf, "guild_%s", guild[i].abbrev);
        declare_pdf(buf, VAR_INT, &value);
    }

    for (i = 1; i < MAX_RACES; i++) {
        value.val = i;
        value.ptr = (void *) 0;
        sprintf(buf, "race_%s", race[i].abbrev);
        declare_pdf(buf, VAR_INT, &value);
    }

    for (i = 1; i < MAX_CLAN; i++) {
        char clan_name[256];

        value.val = i;
        value.ptr = (void *) 0;

        replace_char(clan_table[i].name, ' ', '_', clan_name,sizeof(clan_name));
        replace_char(clan_name, '\'', '_', clan_name, sizeof(clan_name));
        sprintf(buf, "clan_%s", clan_name);
        declare_pdf(buf, VAR_INT, &value);
    };

    for (i = 1; i <= 5; i++) {
        value.val = 0;
        value.ptr = (void *) 0;
        sprintf(buf, "i%d", i);
        declare_pdf(buf, VAR_INT, &value);
    }

    for (i = 1; i <= 5; i++) {
        value.val = 0;
        value.ptr = (void *) 0;
        sprintf(buf, "s%d", i);
        declare_pdf(buf, VAR_STRING, &value);
    }
#ifdef DOTW_SUPPORT
    for (i = 0; i < MAX_DOTW; i++) {
        value.val = i;
        value.ptr = (void *) 0;
        sprintf(buf, "day_%s", weekday_names[i]);
        declare_pdf(buf, VAR_INT, &value);
    }
#endif

    value.val = CMD_PULSE;
    value.ptr = (void *) 0;
    declare_pdf("state_pulse", VAR_INT, &value);

    value.val = CMD_PULSE;
    value.ptr = (void *) 0;
    declare_pdf("auto_call", VAR_INT, &value);

    value.val = POSITION_STUNNED;
    value.ptr = (void *) 0;
    declare_pdf("pos_stunned", VAR_INT, &value);

    value.val = POSITION_SLEEPING;
    value.ptr = (void *) 0;
    declare_pdf("pos_sleeping", VAR_INT, &value);

    value.val = POSITION_RESTING;
    value.ptr = (void *) 0;
    declare_pdf("pos_resting", VAR_INT, &value);

    value.val = POSITION_SITTING;
    value.ptr = (void *) 0;
    declare_pdf("pos_sitting", VAR_INT, &value);

    value.val = POSITION_FIGHTING;
    value.ptr = (void *) 0;
    declare_pdf("pos_fighting", VAR_INT, &value);

    value.val = POSITION_STANDING;
    value.ptr = (void *) 0;
    declare_pdf("pos_standing", VAR_INT, &value);

    value.val = 0;
    value.ptr = (void *) 0;
    declare_pdf("char_null", VAR_CHAR, &value);
    declare_pdf("obj_null", VAR_OBJECT, &value);
    declare_pdf("room_null", VAR_ROOM, &value);
}

void
string_sub(char *pszLine)
{
    struct var_data *pvdVar;
    char *pszPos, *szBuf, szBuffer[MAX];
    int iLen;

    szBuf = (char *) &szBuffer[0];
    iLen = 0;
    for (pszPos = pszLine + 1; (*pszPos != '\0') && (*pszLine != '\0'); pszPos++) {
        if (*pszPos == '[')
            string_sub(pszPos);
        if (*pszPos == ']') {
            if (iLen == 0) {
                error("Nothing between [ and ]");
                return;
            } else {
                /* check if its a variable name */
                strncpy(szBuf, pszLine + 1, iLen);
                szBuf[iLen] = '\0';
                if (!(pvdVar = get_var(szBuf))) {
                    error("undeclared variable in substitution '%s'", szBuf);
                    return;
                } else
                    /* evaluate var and substitue */
                    switch (pvdVar->type) {
                    case VAR_INT:
                        sprintf(szBuf, "%d", pvdVar->value.val);
                        strcat(szBuf, pszPos + 1);
                        strcpy(pszLine, szBuf);
                        return;
                    case VAR_STRING:
                        if (pvdVar->value.ptr)
                            strcpy(szBuf, (char *) pvdVar->value.ptr);
                        else
                            strcpy(szBuf, "");

                        strcat(szBuf, pszPos + 1);
                        strcpy(pszLine, szBuf);
                        return;
                    default:
                        error("invalid data type for subsitution");
                        return;
                    }
            }
        }
        iLen++;
    }

    /* didn't find a close brace */
    error("Expected ']'");
    return;
}


void
preprocess(char *pszLine)
{
    char *pszPos, szBuf[MAX];

    strip(pszLine);

    for (pszPos = pszLine; *pszPos; pszPos++) {
        if ((*pszPos == '\\') && (*(pszPos + 1) == '[')) {
            /* shift line left 1 and move pszPos past the open bracket */
            strcpy(szBuf, pszPos + 1);
            strcpy(pszPos, szBuf);
            pszPos++;
        }
        if (*pszPos == '[')
            string_sub(pszPos);
    }
}

void eval_expr(char *exp, struct value_data *ret);

void
eval_function(const char *exp, struct value_data *ret)
{
    char name[MAX], arg[MAX_ARGS][MAX];
    char local_arg1[MAX], local_arg2[MAX];
    struct value_data val[MAX_ARGS], *vptr[MAX_ARGS];
    int i, j, k, n;

    exp = one_argument(exp, name, sizeof(name));
    for (n = 0; *function[n].name; n++)
        if (!stricmp(name, function[n].name))
            break;
    if (!*function[n].name) {
        error("unknown function");
        return;
    }

    for (i = 0; i < MAX_ARGS; i++) {
        for (j = 0; *(exp + j) && *(exp + j) != ','; j++)
            arg[i][j] = *(exp + j);
        arg[i][j] = '\0';

        if (*(exp + j) == ',')
            exp += j + 1;
        else {                  /* end of string */
            for (k = i + 1; k < MAX_ARGS; k++)
                strcpy(arg[k], "0");
            break;
        }
    }

    if (n == FUNC_ONE_ARG) {
        strcpy(local_arg1, arg[0]);
        strcpy(local_arg2, arg[1]);
    }

    for (i = 0; i < MAX_ARGS; i++) {
        eval_expr(arg[i], &val[i]);
        if (dead)
            return;
        vptr[i] = &val[i];
    }

    if (n == FUNC_ONE_ARG)
        spec_d_one_arg(get_var(local_arg1), get_var(local_arg2), ret);
    else
        ((*function[n].func) (vptr, ret));
}

/*
   int close_parens(char *exp, int mode)
   {
   int p, pos = 0;
   char *cp;

   for (p = 1, pos = 1; p > 0; pos++) {
   if (*(exp + pos) == '\0') return (0); 
   if (mode == BRACES) {
   if (*(exp + pos) == '{') p++;
   else if (*(exp + pos) == '}') p--;
   } else if (mode == PARENS) {
   if (*(exp + pos) == '(') p++;
   else if (*(exp + pos) == ')') p--;
   }
   }

   return (pos - 1);
   }
 */


void
eval_expr(char *exp, struct value_data *ret)
{
    struct var_data *var;
    struct value_data val1, val2, valf;
    int parens;
    char substr[MAX], op[MAX];

/* find matching parentheses and braces, get those substrings
   and evaluate them */

    while (*exp == ' ')
        exp++;

    if (*exp == '(') {
        if ((parens = close_parens(exp, '('))) {
            sub_string(exp, substr, 1, parens - 1);
            eval_expr(substr, &val1);
            delete_substr(exp, 0, parens + 1);
            if (dead)
                return;
        } else {
            error("Parentheses error");
            return;
        }
    } else if (*exp == '{') {
        if ((parens = close_parens(exp, '{'))) {
            sub_string(exp, substr, 1, parens - 1);
            eval_function(substr, &val1);
            delete_substr(exp, 0, parens + 1);
            if (dead)
                return;
        } else {
            error("Braces error");
            return;
        }
    } else {
        exp = (char *) one_argument(exp, substr, sizeof(substr));
        val1.val = (var = get_var(substr)) ? var->value.val : atoi(substr);
        val1.ptr = var ? var->value.ptr : (void *) 0;
        if (var)
            copy_string_check = (var->type == VAR_STRING) ? 1 : 0;
    }

    exp = (char *) one_argument(exp, op, sizeof(op));
    if (!*exp) {
        ret->val = val1.val;
        ret->ptr = val1.ptr;
        return;
    }

    while (*exp == ' ')
        exp++;

    if (*exp == '(') {
        if ((parens = close_parens(exp, '('))) {
            sub_string(exp, substr, 1, parens - 1);
            eval_expr(substr, &val2);
            delete_substr(exp, 0, parens + 1);
            if (dead)
                return;
        } else {
            error("Parentheses error");
            return;
        }
    } else if (*exp == '{') {
        if ((parens = close_parens(exp, '{'))) {
            sub_string(exp, substr, 1, parens - 1);
            eval_function(substr, &val2);
            delete_substr(exp, 0, parens + 1);
            if (dead)
                return;
        } else {
            error("Braces error");
            return;
        }
    } else {
        exp = (char *) one_argument(exp, substr, sizeof(substr));
        val2.val = (var = get_var(substr)) ? var->value.val : atoi(substr);
        val2.ptr = var ? var->value.ptr : (void *) 0;
    }


    if (!stricmp(op, "=") || !stricmp(op, "equals")) {
        valf.val = (val1.val == val2.val) && (val1.ptr == val2.ptr);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "cmp")) {
        valf.val = !strcmp((char *) val1.ptr, (char *) val2.ptr);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "icmp")) {
        valf.val = !stricmp((char *) val1.ptr, (char *) val2.ptr);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "!=")) {
        valf.val = (val1.val != val2.val) || (val1.ptr != val2.ptr);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, ">")) {
        valf.val = (val1.val > val2.val);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "<")) {
        valf.val = (val1.val < val2.val);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, ">=")) {
        valf.val = (val1.val >= val2.val);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "<=")) {
        valf.val = (val1.val <= val2.val);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "+") || !stricmp(op, "plus")) {
        valf.val = (val1.val + val2.val);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "-") || !stricmp(op, "minus")) {
        valf.val = (val1.val - val2.val);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "*") || !stricmp(op, "times")) {
        valf.val = (val1.val * val2.val);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "/") || !stricmp(op, "div")) {
        valf.val = (val1.val / val2.val);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "%") || !stricmp(op, "mod")) {
        valf.val = (val1.val % val2.val);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "&") || !stricmp(op, "and")) {
        valf.val = (val1.val && val2.val) || (val1.ptr && val2.ptr);
        valf.ptr = (void *) 0;
    } else if (!stricmp(op, "|") || !stricmp(op, "or")) {
        valf.val = (val1.val || val2.val) || (val1.ptr || val2.ptr);
        valf.ptr = (void *) 0;
    } else {
        error("invalid operator");
        return;
    }

    ret->val = valf.val;
    ret->ptr = valf.ptr;
}

void
com_print(char *arg)
{
    char buf[MAX];
    struct value_data val;

    eval_expr(arg, &val);
    if (dead)
        return;

    sprintf(buf, "%d", val.val);
    gamelog(buf);
    return;
}

void
com_log(char *arg)
{
    if (!arg || arg[0] == '\0')
        error("log given (null)");

    gamelog(arg);
    return;
}


void
com_var(const char *arg)
{
    char arg1[MAX], arg2[MAX], arg3[MAX];
    int type;
    struct value_data value;

    arg = one_argument(arg, arg1, sizeof(arg1));
    arg = one_argument(arg, arg2, sizeof(arg2));
    arg = one_argument(arg, arg3, sizeof(arg3));

    switch (*arg1) {
    case 'i':
        type = VAR_INT;
        break;
    case 's':
        type = VAR_STRING;
        break;
    case 'c':
        type = VAR_CHAR;
        break;
    case 'o':
        type = VAR_OBJECT;
        break;
    case 'r':
        type = VAR_ROOM;
        break;
    default:
        error("invalid variable type");
        return;
    }

    if (get_var(arg2)) {
        error("variable already exists");
        return;
    }

    value.val = *arg3 != '\0' && type == VAR_INT ? atoi(arg3) : 0;
    value.ptr = (void *) 0;
    declare_var(arg2, type, &value);
}

void
com_copy(const char *arg)
{
    char arg1[MAX], exp[MAX];
    struct var_data *var;
    struct value_data ret;

    arg = one_argument(arg, arg1, sizeof(arg1));
    strcpy(exp, arg);

    if (!(var = get_var(arg1))) {
        error("unknown variable");
        return;
    }

    eval_expr(exp, &ret);
    if (dead)
        return;

    if ((ret.val == var->value.val) && (ret.ptr == var->value.ptr))
        return;

    if (var->type == VAR_STRING) {
        free(var->value.ptr);
        var->value.ptr = 0;
    }

    if (var->type == VAR_STRING) {
        if (!copy_string_check) {
            error("Attempt to copy non-string to string.");
            return;
        }
        var->value.ptr = strdup(ret.ptr);
    } else {
        var->value.val = ret.val;
        var->value.ptr = ret.ptr;
    }
}

void
com_command(const char *arg)
{
    char arg1[MAX];
    struct var_data *var;

    arg = one_argument(arg, arg1, sizeof(arg1));
    if (!(var = get_var(arg1))) {
        error("undeclared variable");
        return;
    }
    if (var->type != VAR_CHAR) {
        error("invalid variable type");
        return;
    }
    if (var->value.ptr == (void *) 0) {
        error("null pointer");
        return;
    }

    parse_command((CHAR_DATA *) (var->value.ptr), arg);
}

void
com_store(const char *arg)
{
    char arg1[MAX], arg2[MAX], arg3[MAX];
    int i;
    struct var_data *var1, *var2;

    arg = one_argument(arg, arg1, sizeof(arg1));
    arg = one_argument(arg, arg2, sizeof(arg2));

    if (!(var1 = get_var(arg1))) {
        error("undeclared variable");
        return;
    }

    for (i = 0; arg2[i] != '\0' && arg2[i] != '.'; i++);
    if (arg2[i] == '\0') {
        error("syntax error");
        return;
    }
    arg2[i] = '\0';
    strcpy(arg3, &arg2[i + 1]);

    if (arg3[0] == '\0') {
        error("syntax error");
        return;
    }

    if (!(var2 = get_var(arg2))) {
        error("undeclared variable");
        return;
    }

    if (var2->value.ptr == NULL) {
        error("uninitialized variable");
        return;
    }

    switch (var2->type) {
    case VAR_ROOM:
        if (!stricmp(arg3, "name") && var1->type == VAR_STRING) {
            if (((ROOM_DATA *) var2->value.ptr)->name)
                free((char *) ((ROOM_DATA *) var2->value.ptr)->name);
            ((ROOM_DATA *) var2->value.ptr)->name = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "nr") && var1->type == VAR_INT)
            ((ROOM_DATA *) var2->value.ptr)->number = var1->value.val;
        else if (!stricmp(arg3, "sector") && var1->type == VAR_INT)
            ((ROOM_DATA *) var2->value.ptr)->sector_type = var1->value.val;
        else if (!stricmp(arg3, "rflags") && var1->type == VAR_INT)
            ((ROOM_DATA *) var2->value.ptr)->room_flags = var1->value.val;
        else if (!stricmp(arg3, "zone") && var1->type == VAR_INT)
            ((ROOM_DATA *) var2->value.ptr)->zone = var1->value.val;
        else if (!stricmp(arg3, "description") && var1->type == VAR_STRING) {
            if (((ROOM_DATA *) var2->value.ptr)->description)
                free((char *) ((ROOM_DATA *) var2->value.ptr)->description);
            ((ROOM_DATA *) var2->value.ptr)->description = strdup(var1->value.ptr);
        } else {
            error("invalid variable type or nonexistent member");
            return;
        }
        break;
    case VAR_OBJECT:
        if (!stricmp(arg3, "name") && var1->type == VAR_STRING) {
            if (((OBJ_DATA *) var2->value.ptr)->name)
                free((char *) ((OBJ_DATA *) var2->value.ptr)->name);
            ((OBJ_DATA *) var2->value.ptr)->name = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "sdesc") && var1->type == VAR_STRING) {
            if (((OBJ_DATA *) var2->value.ptr)->short_descr)
                free((char *) ((OBJ_DATA *) var2->value.ptr)->short_descr);
            ((OBJ_DATA *) var2->value.ptr)->short_descr = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "ldesc") && var1->type == VAR_STRING) {
            if (((OBJ_DATA *) var2->value.ptr)->long_descr)
                free((char *) ((OBJ_DATA *) var2->value.ptr)->long_descr);
            ((OBJ_DATA *) var2->value.ptr)->long_descr = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "type") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.type = var1->value.val;
        else if (!stricmp(arg3, "eflags") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.extra_flags = var1->value.val;
        else if (!stricmp(arg3, "wflags") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.wear_flags = var1->value.val;
        else if (!stricmp(arg3, "val1") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[0] = var1->value.val;
        else if (!stricmp(arg3, "val2") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[1] = var1->value.val;
        else if (!stricmp(arg3, "val3") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[2] = var1->value.val;
        else if (!stricmp(arg3, "val4") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[3] = var1->value.val;
        else if (!stricmp(arg3, "val5") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[4] = var1->value.val;
        else if (!stricmp(arg3, "val6") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[5] = var1->value.val;
        else if (!stricmp(arg3, "cost") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.cost = var1->value.val;
        else if (!stricmp(arg3, "material") && var1->type == VAR_INT)
            ((OBJ_DATA *) var2->value.ptr)->obj_flags.material = var1->value.val;
        else {
            error("invalid variable type or nonexistent member");
            return;
        }
        break;
    case VAR_CHAR:
        if (!stricmp(arg3, "name") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->name)
                free((char *) ((CHAR_DATA *) var2->value.ptr)->name);
            ((CHAR_DATA *) var2->value.ptr)->name = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "sdesc") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->short_descr)
                free((char *) ((CHAR_DATA *) var2->value.ptr)->short_descr);
            ((CHAR_DATA *) var2->value.ptr)->short_descr = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "ldesc") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->long_descr)
                free((char *) ((CHAR_DATA *) var2->value.ptr)->long_descr);
            ((CHAR_DATA *) var2->value.ptr)->long_descr = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "description") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->description)
                free((char *) ((CHAR_DATA *) var2->value.ptr)->description);
            ((CHAR_DATA *) var2->value.ptr)->description = strdup(var1->value.ptr);
/*  Two additions follow - Kel */
        } else if (!stricmp(arg3, "poofin") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->player.poofin)
                free((char *) ((CHAR_DATA *) var2->value.ptr)->player.poofin);
            ((CHAR_DATA *) var2->value.ptr)->player.poofin = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "poofout") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->player.poofout)
                free((char *) ((CHAR_DATA *) var2->value.ptr)->player.poofout);
            ((CHAR_DATA *) var2->value.ptr)->player.poofout = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "extra") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->player.extkwds)
                free((char *) ((CHAR_DATA *) var2->value.ptr)->player.extkwds);
            ((CHAR_DATA *) var2->value.ptr)->player.extkwds = strdup(var1->value.ptr);
        } else if (!stricmp(arg3, "cflags") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->specials.act = var1->value.val;

        /*  I AM TAKING OUT SETTING OF LEVELS -Tenebrius
         * else if (!stricmp(arg3, "level") && var1->type == VAR_INT)
         * ((CHAR_DATA *)var2->value.ptr)->player.level = var1->value.val;
         */

        else if (!stricmp(arg3, "off") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->abilities.off = var1->value.val;
        else if (!stricmp(arg3, "def") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->abilities.def = var1->value.val;
        else if (!stricmp(arg3, "armor") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->abilities.armor = var1->value.val;
        else if (!stricmp(arg3, "max_health") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->points.max_hit = var1->value.val;
        else if (!stricmp(arg3, "health") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->points.hit = var1->value.val;
        else if (!stricmp(arg3, "max_stun") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->points.max_stun = var1->value.val;
        else if (!stricmp(arg3, "stun") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->points.stun = var1->value.val;
        else if (!stricmp(arg3, "coins") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->points.obsidian = var1->value.val;
        else if (!stricmp(arg3, "pos") && var1->type == VAR_INT)
            set_char_position(((CHAR_DATA *) var2->value.ptr), var1->value.val);
        else if (!stricmp(arg3, "default_pos") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->specials.default_pos = var1->value.val;
        else if (!stricmp(arg3, "state") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->specials.state = var1->value.val;
        else if (!stricmp(arg3, "eco") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->specials.eco = var1->value.val;
        else if (!stricmp(arg3, "sex") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->player.sex = var1->value.val;
        else if (!stricmp(arg3, "guild") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->player.guild = var1->value.val;

        else if (!stricmp(arg3, "tribe") && var1->type == VAR_INT) {
            /* store ch.tribe <number> 
             * checks to make sure they're not already in the clan, then adds it
             * left in to maintain functionality, should use add_clan function
             */
            if (!IS_TRIBE(((CHAR_DATA *) var2->value.ptr), var1->value.val))
                add_clan((CHAR_DATA *) var2->value.ptr, var1->value.val);
        } else if (!stricmp(arg3, "hometown") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->player.hometown = var1->value.val;
        else if (!stricmp(arg3, "race") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->player.race = var1->value.val;
        else if (!stricmp(arg3, "weight") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->player.weight = var1->value.val;
        else if (!stricmp(arg3, "height") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->player.height = var1->value.val;
        else if (!stricmp(arg3, "stamina") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->points.move = var1->value.val;
        else if (!stricmp(arg3, "mana") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->points.mana = var1->value.val;
        else if (!stricmp(arg3, "str") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->abilities.str = var1->value.val;
        else if (!stricmp(arg3, "agl") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->abilities.agl = var1->value.val;
        else if (!stricmp(arg3, "wis") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->abilities.wis = var1->value.val;
        else if (!stricmp(arg3, "end") && var1->type == VAR_INT)
            ((CHAR_DATA *) var2->value.ptr)->abilities.end = var1->value.val;
        else {
            error("invalid variable type or nonexistent member");
            return;
        }
        break;
    default:
        error("invalid variable type");
        return;
    }
}

void
com_fetch(const char *arg)
{
    char arg1[MAX], arg2[MAX], arg3[MAX];
    int i;
    struct var_data *var1, *var2;

    arg = one_argument(arg, arg1, sizeof(arg1));
    arg = one_argument(arg, arg2, sizeof(arg2));

    if (!(var1 = get_var(arg1))) {
        error("undeclared variable");
        return;
    }

    for (i = 0; arg2[i] != '\0' && arg2[i] != '.'; i++);
    if (arg2[i] == '\0') {
        error("syntax error");
        return;
    }
    arg2[i] = '\0';
    strcpy(arg3, &arg2[i + 1]);

    if (arg3[0] == '\0') {
        error("syntax error");
        return;
    }

    if (!(var2 = get_var(arg2))) {
        error("undeclared variable");
        return;
    }

    if (var2->value.ptr == NULL) {
        error("uninitialized variable");
        return;
    }

    switch (var2->type) {
    case VAR_ROOM:
        if (!stricmp(arg3, "name") && var1->type == VAR_STRING) {
            free(var1->value.ptr);
            var1->value.ptr = strdup(((ROOM_DATA *) var2->value.ptr)->name);
        } else if (!stricmp(arg3, "description") && var1->type == VAR_STRING) {
            free(var1->value.ptr);
            var1->value.ptr = strdup(((ROOM_DATA *) var2->value.ptr)->description);
        } else if (!stricmp(arg3, "nr") && var1->type == VAR_INT)
            var1->value.val = ((ROOM_DATA *) var2->value.ptr)->number;
        else if (!stricmp(arg3, "sector") && var1->type == VAR_INT)
            var1->value.val = ((ROOM_DATA *) var2->value.ptr)->sector_type;
        else if (!stricmp(arg3, "rflags") && var1->type == VAR_INT)
            var1->value.val = ((ROOM_DATA *) var2->value.ptr)->room_flags;
        else if (!stricmp(arg3, "zone") && var1->type == VAR_INT)
            var1->value.val = ((ROOM_DATA *) var2->value.ptr)->zone;
        else if (!stricmp(arg3, "clist") && var1->type == VAR_CHAR)
            var1->value.ptr = ((ROOM_DATA *) var2->value.ptr)->people;
        else if (!stricmp(arg3, "olist") && var1->type == VAR_OBJECT)
            var1->value.ptr = ((ROOM_DATA *) var2->value.ptr)->contents;
        else {
            error("invalid variable type or nonexistent member");
            return;
        }
        break;
    case VAR_OBJECT:
        if (!stricmp(arg3, "name") && var1->type == VAR_STRING) {
            free(var1->value.ptr);
            var1->value.ptr = strdup(OSTR(((OBJ_DATA *) var2->value.ptr), name));
        } else if (!stricmp(arg3, "sdesc") && var1->type == VAR_STRING) {
            free(var1->value.ptr);
            var1->value.ptr = strdup(OSTR(((OBJ_DATA *) var2->value.ptr), short_descr));
        } else if (!stricmp(arg3, "ldesc") && var1->type == VAR_STRING) {
            free(var1->value.ptr);
            var1->value.ptr = strdup(OSTR(((OBJ_DATA *) var2->value.ptr), long_descr));
        } else if (!stricmp(arg3, "type") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.type;
        else if (!stricmp(arg3, "eflags") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.extra_flags;
        else if (!stricmp(arg3, "wflags") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.wear_flags;
        else if (!stricmp(arg3, "val1") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[0];
        else if (!stricmp(arg3, "val2") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[1];
        else if (!stricmp(arg3, "val3") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[2];
        else if (!stricmp(arg3, "val4") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[3];
        else if (!stricmp(arg3, "val5") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[4];
        else if (!stricmp(arg3, "val6") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.value[5];
        else if (!stricmp(arg3, "carried_by") && var1->type == VAR_CHAR)
            var1->value.ptr = ((OBJ_DATA *) var2->value.ptr)->carried_by;
        else if (!stricmp(arg3, "equipped_by") && var1->type == VAR_CHAR)
            var1->value.ptr = ((OBJ_DATA *) var2->value.ptr)->equipped_by;
        else if (!stricmp(arg3, "weight") && var1->type == VAR_INT)
            var1->value.val = GET_OBJ_WEIGHT((OBJ_DATA *) var2->value.ptr);
        else if (!stricmp(arg3, "cost") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.cost;
        else if (!stricmp(arg3, "material") && var1->type == VAR_INT)
            var1->value.val = ((OBJ_DATA *) var2->value.ptr)->obj_flags.material;
        else if (!stricmp(arg3, "in_room") && var1->type == VAR_ROOM)
            var1->value.ptr = ((OBJ_DATA *) var2->value.ptr)->in_room;
        else if (!stricmp(arg3, "nr") && var1->type == VAR_INT)
            var1->value.val = obj_index[((OBJ_DATA *) var2->value.ptr)->nr].vnum;
        else if (!stricmp(arg3, "prev") && var1->type == VAR_OBJECT)
            var1->value.ptr = ((OBJ_DATA *) var2->value.ptr)->prev;
        else if (!stricmp(arg3, "next") && var1->type == VAR_OBJECT)
            var1->value.ptr = ((OBJ_DATA *) var2->value.ptr)->next;
        else if (!stricmp(arg3, "prev_content") && var1->type == VAR_OBJECT)
            var1->value.ptr = ((OBJ_DATA *) var2->value.ptr)->prev_content;
        else if (!stricmp(arg3, "next_content") && var1->type == VAR_OBJECT)
            var1->value.ptr = ((OBJ_DATA *) var2->value.ptr)->next_content;
        else if (!stricmp(arg3, "contains") && var1->type == VAR_OBJECT)
            var1->value.ptr = ((OBJ_DATA *) var2->value.ptr)->contains;
        else {
            error("invalid variable type or nonexistent member");
            return;
        }

        break;
    case VAR_CHAR:
        if (var1->type == VAR_STRING) {
            free(var1->value.ptr);
            var1->value.ptr = 0;
        }

        if (!stricmp(arg3, "name") && var1->type == VAR_STRING) {
            var1->value.ptr = strdup(MSTR(((CHAR_DATA *) var2->value.ptr), name));
        } else if (!stricmp(arg3, "sdesc") && var1->type == VAR_STRING) {
            var1->value.ptr = strdup(MSTR(((CHAR_DATA *) var2->value.ptr), short_descr));
        } else if (!stricmp(arg3, "ldesc") && var1->type == VAR_STRING) {
            var1->value.ptr = strdup(MSTR(((CHAR_DATA *) var2->value.ptr), long_descr));
        } else if (!stricmp(arg3, "description") && var1->type == VAR_STRING) {
            var1->value.ptr = strdup(MSTR(((CHAR_DATA *) var2->value.ptr), description));
        } else if (!stricmp(arg3, "poofin") && var1->type == VAR_STRING) {
            var1->value.ptr = strdup(((CHAR_DATA *) var2->value.ptr)->player.poofin);
        } else if (!stricmp(arg3, "poofout") && var1->type == VAR_STRING) {
            var1->value.ptr = strdup(((CHAR_DATA *) var2->value.ptr)->player.poofout);
        } else if (!stricmp(arg3, "extra") && var1->type == VAR_STRING) {
            var1->value.ptr = strdup(((CHAR_DATA *) var2->value.ptr)->player.extkwds);
        } else if (!stricmp(arg3, "heshe") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->player.sex == SEX_MALE)
                var1->value.ptr = strdup("he");
            else if (((CHAR_DATA *) var2->value.ptr)->player.sex == SEX_FEMALE)
                var1->value.ptr = strdup("she");
            else
                var1->value.ptr = strdup("it");
        } else if (!stricmp(arg3, "himher") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->player.sex == SEX_MALE)
                var1->value.ptr = strdup("him");
            else if (((CHAR_DATA *) var2->value.ptr)->player.sex == SEX_FEMALE)
                var1->value.ptr = strdup("her");
            else
                var1->value.ptr = strdup("it");
        } else if (!stricmp(arg3, "hisher") && var1->type == VAR_STRING) {
            if (((CHAR_DATA *) var2->value.ptr)->player.sex == SEX_MALE)
                var1->value.ptr = strdup("his");
            else if (((CHAR_DATA *) var2->value.ptr)->player.sex == SEX_FEMALE)
                var1->value.ptr = strdup("her");
            else
                var1->value.ptr = strdup("its");
        } else if (!stricmp(arg3, "str") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->abilities.str;
        else if (!stricmp(arg3, "agl") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->abilities.agl;
        else if (!stricmp(arg3, "wis") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->abilities.wis;
        else if (!stricmp(arg3, "end") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->abilities.end;
        else if (!stricmp(arg3, "cflags") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->specials.act;
        else if (!stricmp(arg3, "level") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->player.level;
        else if (!stricmp(arg3, "off") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->abilities.off;
        else if (!stricmp(arg3, "def") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->abilities.def;
        else if (!stricmp(arg3, "armor") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->abilities.armor;
        else if (!stricmp(arg3, "max_health") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->points.max_hit;
        else if (!stricmp(arg3, "health") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->points.hit;
        else if (!stricmp(arg3, "max_stun") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->points.max_stun;
        else if (!stricmp(arg3, "stun") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->points.stun;
        else if (!stricmp(arg3, "coins") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->points.obsidian;
        else if (!stricmp(arg3, "pos") && var1->type == VAR_INT)
            var1->value.val = GET_POS(((CHAR_DATA *) var2->value.ptr));
        else if (!stricmp(arg3, "default_pos") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->specials.default_pos;
        else if (!stricmp(arg3, "sex") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->player.sex;
        else if (!stricmp(arg3, "state") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->specials.state;
        else if (!stricmp(arg3, "eco") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->specials.eco;
        else if (!stricmp(arg3, "guild") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->player.guild;
        else if (!stricmp(arg3, "tribe") && var1->type == VAR_INT)
            var1->value.val = GET_TRIBE(((CHAR_DATA *) var2->value.ptr));
        else if (!stricmp(arg3, "hometown") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->player.hometown;
        else if (!stricmp(arg3, "race") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->player.race;
        else if (!stricmp(arg3, "weight") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->player.weight;
        else if (!stricmp(arg3, "height") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->player.height;
        else if (!stricmp(arg3, "stamina") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->points.move;
        else if (!stricmp(arg3, "mana") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->points.mana;
        else if (!stricmp(arg3, "in_room") && var1->type == VAR_ROOM)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->in_room;
        else if (!stricmp(arg3, "nr") && var1->type == VAR_INT)
            var1->value.val = npc_index[((CHAR_DATA *) var2->value.ptr)->nr].vnum;
        else if (!stricmp(arg3, "dir_guarding") && var1->type == VAR_INT)
            var1->value.val = ((CHAR_DATA *) var2->value.ptr)->specials.dir_guarding;
        else if (!stricmp(arg3, "fighting") && var1->type == VAR_CHAR)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->specials.fighting;
        else if (!stricmp(arg3, "subduing") && var1->type == VAR_CHAR)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->specials.subduing;
        else if (!stricmp(arg3, "riding") && var1->type == VAR_CHAR)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->specials.riding;
        else if (!stricmp(arg3, "contact") && var1->type == VAR_CHAR)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->specials.contact;
        else if (!stricmp(arg3, "prev") && var1->type == VAR_CHAR)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->prev;
        else if (!stricmp(arg3, "next") && var1->type == VAR_CHAR)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->next;
        else if (!stricmp(arg3, "prev_in_room") && var1->type == VAR_CHAR)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->prev_in_room;
        else if (!stricmp(arg3, "next_in_room") && var1->type == VAR_CHAR)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->next_in_room;
        else if (!stricmp(arg3, "olist") && var1->type == VAR_OBJECT)
            var1->value.ptr = ((CHAR_DATA *) var2->value.ptr)->carrying;
        else {
            error("invalid variable type or nonexistent member");
            return;
        }
        break;
    default:
        error("invalid variable type");
        return;
    }
}


struct csend_data
{
    char *token;
    char endCh;
    struct var_data *var;
    struct csend_data *next;
};

void
free_csend_list(struct csend_data *token_head)
{
    struct csend_data *fp;

    while ((fp = token_head)) {
        token_head = token_head->next;
        if (fp->token)
            free(fp->token);
        free(fp);
    }
}

/* format_vars_ch
   show the faviables as the viewer would see them
   recognizes the following special characters:
           ~ - replace the following string with the named character's sdesc
           ! - replace the following string with the named character's
               third or second-person objective pronoun.
           % - as ~ but possessive
           ^ - as ! but possessive
           # - replace the following string with 'he/she'
*/

char *
format_vars_ch(const char *text, CHAR_DATA * ch)
{
    static char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char token[MAX_INPUT_LENGTH];
    struct csend_data *token_head = (struct csend_data *) malloc(sizeof *token_head);
    struct csend_data *tp, *tail;

    if (text == NULL || ch == NULL)
        return "";


    token_head->token = NULL;
    token_head->var = NULL;
    token_head->endCh = 0;
    token_head->next = NULL;
    tail = token_head;

    /* tokenize the list and stick it into the emote struct */
    text = one_argument(text, token, sizeof(token));
    while (text && text[0] != '\0') {
        tp = (struct csend_data *) malloc(sizeof *tp);
        tp->token = strdup(token);
        tp->endCh = 0;
        tp->var = NULL;
        tp->next = NULL;
        tail->next = tp;
        tail = tp;
        text = one_argument(text, token, sizeof(token));
    }

    /* do it one last time for the final word */
    tp = (struct csend_data *) malloc(sizeof *tp);
    tp->token = strdup(token);
    tp->endCh = 0;
    tp->var = NULL;
    tp->next = NULL;
    tail->next = tp;
    tail = tp;

    if (token_head->next == NULL)
        return "";

    /* find the variables to whom the tokens refer */
    for (tp = token_head->next; tp != NULL; tp = tp->next) {
        if (ispunct(tp->token[strlen(tp->token) - 1])) {
            tp->endCh = tp->token[strlen(tp->token) - 1];
            tp->token[strlen(tp->token) - 1] = '\0';
        }

        if (isspecial(tp->token[0]) && (strlen(tp->token) > 1)) {
            if ((tp->var = get_var(tp->token + 1)) == NULL) {
                sprintf(buf, "Unknown variable, '%s'", tp->token);
                free_csend_list(token_head);
                error("%s", buf);
                return "";
            }
        }
    }

    buf[0] = '\0';

    for (tp = token_head->next; tp; tp = tp->next) {
        if (tp->var) {
            switch (tp->var->type) {
            default:
            case VAR_ROOM:
                sprintf(buf, "Error bad variable type for format, '%s'", tp->token);
                free_csend_list(token_head);
                error("%s", buf);
                return "";
            case VAR_CHAR:
                if (!tp->var->value.ptr) {
                    sprintf(buf, "Unitialized variable, '%s'", tp->token);
                    free_csend_list(token_head);
                    error("%s", buf);
                    return "";
                }
                switch (tp->token[0]) {
                case '~':
                    sprintf(buf2, "%s", PERS(ch, (CHAR_DATA *) tp->var->value.ptr));
                    break;
                case '%':
                    sprintf(buf2, "%s", PERS(ch, (CHAR_DATA *) tp->var->value.ptr));
                    sprintf(buf2, "%s%s", PERS(ch, (CHAR_DATA *) tp->var->value.ptr),
                            buf2[strlen(buf2) - 1] == 's' ? "'" : "'s");
                    break;
                case '!':
                    sprintf(buf2, "%s", HMHR((CHAR_DATA *) tp->var->value.ptr));
                    break;
                case '^':
                    sprintf(buf2, "%s", HSHR((CHAR_DATA *) tp->var->value.ptr));
                    break;
                case '#':
                    sprintf(buf2, "%s", HSSH((CHAR_DATA *) tp->var->value.ptr));
                    break;
                }
                break;
            case VAR_OBJECT:
                if (!tp->var->value.ptr) {
                    sprintf(buf, "Unitialized variable, '%s'", tp->token);
                    free_csend_list(token_head);
                    error("%s", buf);
                    return "";
                }

                switch (tp->token[0]) {
                case '~':
                    sprintf(buf2, "%s", format_obj_to_char((OBJ_DATA *) tp->var->value.ptr, ch, 1));
                    break;
                case '%':
                    sprintf(buf2, "%s", format_obj_to_char((OBJ_DATA *) tp->var->value.ptr, ch, 1));
                    sprintf(buf2, "%s%s",
                            format_obj_to_char((OBJ_DATA *) tp->var->value.ptr, ch, 1),
                            buf2[strlen(buf2) - 1] == 's' ? "'" : "'s");
                    break;
                case '!':
                    sprintf(buf2, "it");
                    break;
                case '^':
                    sprintf(buf2, "it's");
                    break;
                case '#':
                    sprintf(buf2, "it");
                    break;
                }
                break;
            case VAR_STRING:
                if (!tp->var->value.ptr) {
                    sprintf(buf, "Unitialized variable, '%s'", tp->token);
                    free_csend_list(token_head);
                    error("%s", buf);
                    return "";
                }
                sprintf(buf2, "%s", (char *) tp->var->value.ptr);
                break;
            case VAR_INT:
                sprintf(buf2, "%d", tp->var->value.val);
                break;
            }
        } else
            sprintf(buf2, "%s", tp->token);

        strcat(buf, buf2);

        if (tp->endCh) {
            sprintf(buf2, "%c", tp->endCh);
            strcat(buf, buf2);
        }

        if (tp->next)
            strcat(buf, " ");
        else if (!tp->endCh) {
            strcat(buf, ".\n\r");
        } else
            strcat(buf, "\n\r");
    }

    free_csend_list(token_head);
    return buf;
}

/* format_vars_ch_for_act
   show the faviables as the viewer would see them
   recognizes the following special characters:
           ~ - replace the following string with the named character's sdesc
           ! - replace the following string with the named character's
               third or second-person objective pronoun.
           % - as ~ but possessive
           ^ - as ! but possessive
           # - replace the following string with 'he/she'
*/

char *
format_vars_ch_for_act(const char *text, CHAR_DATA * ch)
{
    static char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char token[MAX_INPUT_LENGTH];
    struct csend_data *token_head = (struct csend_data *) malloc(sizeof *token_head);
    struct csend_data *tp, *tail;

    CHAR_DATA *ch1 = NULL;
    CHAR_DATA *ch2 = NULL;

    /*    if( text == NULL || ch == NULL )
     * return "";
     */

    if (text == NULL)
        return "";

    token_head->token = NULL;
    token_head->var = NULL;
    token_head->endCh = 0;
    token_head->next = NULL;
    tail = token_head;

    /* tokenize the list and stick it into the emote struct */
    text = one_argument(text, token, sizeof(token));
    while (text && text[0] != '\0') {
        tp = (struct csend_data *) malloc(sizeof *tp);
        tp->token = strdup(token);
        tp->endCh = 0;
        tp->var = NULL;
        tp->next = NULL;
        tail->next = tp;
        tail = tp;
        text = one_argument(text, token, sizeof(token));
    }

    /* do it one last time for the final word */
    tp = (struct csend_data *) malloc(sizeof *tp);
    tp->token = strdup(token);
    tp->endCh = 0;
    tp->var = NULL;
    tp->next = NULL;
    tail->next = tp;
    tail = tp;

    if (token_head->next == NULL)
        return "";

    /* find the variables to whom the tokens refer */
    for (tp = token_head->next; tp != NULL; tp = tp->next) {
        if (ispunct(tp->token[strlen(tp->token) - 1])) {
            tp->endCh = tp->token[strlen(tp->token) - 1];
            tp->token[strlen(tp->token) - 1] = '\0';
        }

        if (isspecial(tp->token[0]) && (strlen(tp->token) > 1)) {
            if ((tp->var = get_var(tp->token + 1)) == NULL) {
                sprintf(buf, "Unknown variable, '%s'", tp->token);
                free_csend_list(token_head);
                error("%s", buf);
                return "";
            }
        }
    }

    buf[0] = '\0';

    for (tp = token_head->next; tp; tp = tp->next) {
        if (tp->var) {
            switch (tp->var->type) {
            default:
            case VAR_ROOM:
                sprintf(buf, "Error bad variable type for format, '%s'", tp->token);
                free_csend_list(token_head);
                error("%s", buf);
                return "";
            case VAR_CHAR:
                if (!tp->var->value.ptr) {
                    sprintf(buf, "Unitialized variable, '%s'", tp->token);
                    free_csend_list(token_head);
                    error("%s", buf);
                    return "";
                }
                switch (tp->token[0]) {
                case '~':
                case '%':
                case '!':
                case '^':
                case '#':
                    if (ch1 && (ch1 == (CHAR_DATA *) tp->var->value.ptr)) {     /* do nothing, this is cool */
                    } else if (ch2 && (ch2 == (CHAR_DATA *) tp->var->value.ptr)) {      /* do nothing, this is cool */
                    } else if (!ch1)
                        ch1 = (CHAR_DATA *) tp->var->value.ptr;
                    else if (!ch2)
                        ch2 = (CHAR_DATA *) tp->var->value.ptr;
                    else {
                        sprintf(buf,
                                "Error, can only referance 2 characters and 1 objects,"
                                " use a non _act function, '%s'", tp->token);
                        free_csend_list(token_head);
                        error("%s", buf);
                        return "";
                    };

                };

                switch (tp->token[0]) {
                case '~':
                    if (ch1 && (ch1 == (CHAR_DATA *) tp->var->value.ptr))
                        sprintf(buf2, "$N");
                    else if (ch2 && (ch2 == (CHAR_DATA *) tp->var->value.ptr))
                        sprintf(buf2, "$n");
                    else
                        sprintf(buf2, "ERRROR");
                    break;
                case '%':
                    sprintf(buf2, "%s", PERS(ch, (CHAR_DATA *) tp->var->value.ptr));
                    sprintf(buf2, "%s%s", PERS(ch, (CHAR_DATA *) tp->var->value.ptr),
                            buf2[strlen(buf2) - 1] == 's' ? "'" : "'s");
                    break;
                case '!':
                    sprintf(buf2, "%s", HMHR((CHAR_DATA *) tp->var->value.ptr));
                    break;
                case '^':
                    sprintf(buf2, "%s", HSHR((CHAR_DATA *) tp->var->value.ptr));
                    break;
                case '#':
                    sprintf(buf2, "%s", HSSH((CHAR_DATA *) tp->var->value.ptr));
                    break;
                }
                break;
            case VAR_OBJECT:
                if (!tp->var->value.ptr) {
                    sprintf(buf, "Unitialized variable, '%s'", tp->token);
                    free_csend_list(token_head);
                    error("%s", buf);
                    return "";
                }

                switch (tp->token[0]) {
                case '~':
                    sprintf(buf2, "%s", format_obj_to_char((OBJ_DATA *) tp->var->value.ptr, ch, 1));
                    break;
                case '%':
                    sprintf(buf2, "%s", format_obj_to_char((OBJ_DATA *) tp->var->value.ptr, ch, 1));
                    sprintf(buf2, "%s%s",
                            format_obj_to_char((OBJ_DATA *) tp->var->value.ptr, ch, 1),
                            buf2[strlen(buf2) - 1] == 's' ? "'" : "'s");
                    break;
                case '!':
                    sprintf(buf2, "it");
                    break;
                case '^':
                    sprintf(buf2, "it's");
                    break;
                case '#':
                    sprintf(buf2, "it");
                    break;
                }
                break;
            case VAR_STRING:
                if (!tp->var->value.ptr) {
                    sprintf(buf, "Unitialized variable, '%s'", tp->token);
                    free_csend_list(token_head);
                    error("%s", buf);
                    return "";
                }
                sprintf(buf2, "%s", (char *) tp->var->value.ptr);
                break;
            case VAR_INT:
                sprintf(buf2, "%d", tp->var->value.val);
                break;
            }
        } else
            sprintf(buf2, "%s", tp->token);

        strcat(buf, buf2);

        if (tp->endCh) {
            sprintf(buf2, "%c", tp->endCh);
            strcat(buf, buf2);
        }

        if (tp->next)
            strcat(buf, " ");
        else if (!tp->endCh) {
            strcat(buf, ".\n\r");
        } else
            strcat(buf, "\n\r");
    }

    free_csend_list(token_head);
    return buf;
}

void
com_rsend(const char *arg)
{
    char arg1[MAX];
    char buf[MAX_STRING_LENGTH];
    struct var_data *var;
    ROOM_DATA *rm;
    CHAR_DATA *rch;
    char *text;

    arg = one_argument(arg, arg1, sizeof(arg1));
    while (*arg == ' ')
        arg++;

    if (!(var = get_var(arg1))) {
        error("unknown variable");
        return;
    }

    if (var->type != VAR_ROOM) {
        sprintf(buf, "rsend to non room %s", arg1);
        error("%s", buf);
        return;
    }

    if (!(rm = var->value.ptr)) {
        error("null pointer");
        return;
    }

    /* I think format_vars_ch destroys the arg, so Making a tmep string   */
    /* it can bash on, and strcpy in between                              */

    text = strdup(arg);
    for (rch = rm->people; rch; rch = rch->next_in_room) {
        strcpy(text, arg);
        CAP(text);
        send_to_char(capitalize(format_vars_ch(text, rch)), rch);
    }

    free(text);
}

void
com_zsend(const char *arg)
{
    char arg1[MAX];
    char buf[MAX_STRING_LENGTH];
    struct var_data *var;
    ROOM_DATA *rm;
    CHAR_DATA *rch;
    char *text;
    int zone;
    struct descriptor_data *d;

    arg = one_argument(arg, arg1, sizeof(arg1));
    while (*arg == ' ')
        arg++;

    /* begin edits */
    if (!(var = get_var(arg1))) {
        sprintf(buf, "zsend: unknown variable, first argument '%s'", arg1);
        error("%s", buf);
        return;
    }

    zone = -1;

    if (var->type == VAR_ROOM) {
        if (!(rm = var->value.ptr)) {
            sprintf(buf, "zsend: first argument '%s' was a room, but a null pointer", arg1);
            error("%s", buf);
            return;
        }
        zone = rm->number / 1000;
    } else {
        if (var->type != VAR_INT) {
            sprintf(buf, "zsend: first argument '%s' must be room or int", arg1);
            error("%s", buf);
            return;
        }
        zone = var->value.val;
    };


    if ((zone < 0) || (zone > top_of_zone_table)) {
        sprintf(buf, "zsend: first argument '%s' out of range, must be between %d and %d", arg1, 0,
                top_of_zone_table);
        error("%s", buf);
        return;
    };


    /* end edits */


    /* I think format_vars_ch destroys the arg, so Making a temp string   */
    /* it can bash on, and strcpy in between                              */

    text = strdup(arg);

    for (d = descriptor_list; d; d = d->next)
        if ((d->character) && (d->character->in_room) && (d->character->in_room->zone == zone)) {
            rch = d->character;
            send_to_char(capitalize(format_vars_ch(text, rch)), rch);
        }

    free(text);
}


void
com_csend_ex(const char *arg)
{
    char arg1[MAX];
    struct var_data *var;
    CHAR_DATA *ch, *rch;
    ROOM_DATA *rm;
    char *text;


    arg = one_argument(arg, arg1, sizeof(arg1));
    while (*arg == ' ')
        arg++;

    if (!(var = get_var(arg1))) {
        error("unknown variable");
        return;
    }
    if (!(ch = var->value.ptr)) {
        error("null pointer");
        return;
    }

    rm = ch->in_room;


    text = strdup(arg);
    for (rch = rm->people; rch; rch = rch->next_in_room) {
        if (rch != ch) {
            strcpy(text, arg);
            send_to_char(capitalize(format_vars_ch(text, rch)), rch);
        }
    }
    free(text);
}

void
com_csend_ex_two(const char *arg)
{
    char arg1[MAX], arg2[MAX];
    struct var_data *var;
    CHAR_DATA *ch1, *ch2, *rch;
    char *text;

    arg = two_arguments(arg, arg1, sizeof(arg1), arg2, sizeof(arg2));
    while (*arg == ' ')
        arg++;

    if (!(var = get_var(arg1))) {
        error("unknown variable");
        return;
    }
    if (!(ch1 = var->value.ptr)) {
        error("null pointer");
        return;
    }

    if (!(var = get_var(arg2))) {
        error("unknown variable");
        return;
    }
    if (!(ch2 = var->value.ptr)) {
        error("null pointer");
        return;
    }

    text = strdup(arg);
    for (rch = ch1->in_room->people; rch; rch = rch->next_in_room) {
        if (rch != ch1 && rch != ch2) {
            strcpy(text, arg);
            send_to_char(capitalize(format_vars_ch(text, rch)), rch);
        }
    }
    free(text);
}

void
com_csend_ex_nocap(const char *arg)
{
    char arg1[MAX];
    struct var_data *var;
    CHAR_DATA *ch, *rch;
    char *text;

    arg = one_argument(arg, arg1, sizeof(arg1));
    while (*arg == ' ')
        arg++;

    if (!(var = get_var(arg1))) {
        error("unknown variable");
        return;
    }
    if (!(ch = var->value.ptr)) {
        error("null pointer");
        return;
    }

    text = strdup(arg);
    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if (rch != ch) {
            strcpy(text, arg);
            send_to_char(format_vars_ch(text, rch), rch);
        }
    }
    free(text);
}


void
com_csend_ex_two_nocap(const char *arg)
{
    char arg1[MAX], arg2[MAX];
    struct var_data *var;
    CHAR_DATA *ch1, *ch2, *rch;
    char *text;

    arg = two_arguments(arg, arg1, sizeof(arg1), arg2, sizeof(arg2));
    while (*arg == ' ')
        arg++;

    if (!(var = get_var(arg1))) {
        error("unknown variable");
        return;
    }
    if (!(ch1 = var->value.ptr)) {
        error("null pointer");
        return;
    }

    if (!(var = get_var(arg2))) {
        error("unknown variable");
        return;
    }
    if (!(ch2 = var->value.ptr)) {
        error("null pointer");
        return;
    }

    text = strdup(arg);
    for (rch = ch1->in_room->people; rch; rch = rch->next_in_room) {
        if (rch != ch1 && rch != ch2) {
            strcpy(text, arg);
            send_to_char(format_vars_ch(text, rch), rch);
        }
    }
    free(text);
}

void
com_csend(const char *arg)
{
    char arg1[MAX];
    struct var_data *var;
    CHAR_DATA *ch;
    char *text;

    arg = one_argument(arg, arg1, sizeof(arg1));
    while (*arg == ' ')
        arg++;

    if (!(var = get_var(arg1))) {
        error("unknown variable");
        return;
    }
    if (!(ch = var->value.ptr)) {
        error("null pointer");
        return;
    }

    text = strdup(arg);

    send_to_char(capitalize(format_vars_ch(text, ch)), ch);
    free(text);
}


void
com_csend_nocap(const char *arg)
{
    char arg1[MAX];
    struct var_data *var;
    CHAR_DATA *ch;
    char *text;

    arg = one_argument(arg, arg1, sizeof(arg1));
    while (*arg == ' ')
        arg++;

    if (!(var = get_var(arg1))) {
        error("unknown variable");
        return;
    }
    if (!(ch = var->value.ptr)) {
        error("null pointer");
        return;
    }

    text = strdup(arg);
    send_to_char(format_vars_ch(text, ch), ch);
    free(text);
}


void
com_strlit(const char *arg)
{
    char arg1[MAX], str[MAX];
    int i = 0, j = 0;
    struct var_data *var;

    arg = one_argument(arg, arg1, sizeof(arg1));
    while (*arg == ' ')
        arg++;

    while (arg[i]) {
        if (arg[i] == '\\' && arg[i + 1] == 'n') {
            str[j++] = '\n';
            str[j++] = '\r';
            i += 2;
        } else
            str[j++] = arg[i++];
    }
    str[j] = '\0';

    if (!(var = get_var(arg1))) {
        error("unknown variable");
        return;
    }

    if (var->type == VAR_STRING) {
        free(var->value.ptr);
        var->value.ptr = 0;
    }

    var->type = VAR_STRING;
    var->value.ptr = strdup(str);
}


int
find_com(char *cmd)
{
    int i;

    if (*cmd == '#' || *cmd == '\0')    /* comment, return */
        return D_CMD_COMMENT;

    for (i = 0; *command[i].name; i++)
        if (!stricmp(command[i].name, cmd))
            return i;

    return D_CMD_NOT_FOUND;
}

void
clean_up()
{
    struct var_data *var;

    while ((var = var_list)) {
        var_list = var_list->next;

        free(var->name);

        /* only strings are newly allocated, other pointers point to objects */
        if (var->type == VAR_STRING)
            free(var->value.ptr);
        free(var);
    }
}

int
exec_program(FILE * fp)
{
    char oldline[MAX], tmpbuf[MAX], line[MAX], cmd[MAX], buf[MAX], lbl[MAX];
    char *cp;
    const char *tmp;
    struct value_data value;
    int n, layers, lecount;
    long oldpos = 0;

    fgets(line, MAX - 1, fp);
    for (lcount = 1, lecount = 1; !feof(fp); lcount++, lecount++) {
        if (lecount > MAX_LINE) {
            error("too many lines executed, possible infinite loop");
            clean_up();
            return FALSE;
        }

        preprocess(line);
        while (strlen(line) && line[strlen(line) - 1] == '\\') {
            fgets(buf, MAX - 1, fp);
            preprocess(buf);
            line[strlen(line) - 1] = '\0';
            strcat(line, buf);
        }

        if (dead) {
            clean_up();
            return FALSE;
        }

        strcpy(oldline, line);
        tmp = line;
        tmp = one_argument(tmp, cmd, sizeof(cmd));
        memmove(line, tmp, strlen(tmp) + 1);
        n = find_com(cmd);

        switch (n) {
        case D_CMD_COMMENT:
            /* comment, do nothing */
            break;
        case D_CMD_IF:
            eval_expr(line, &value);
            if (dead) {
                clean_up();
                return FALSE;
            }

            if (value.val == 0 && value.ptr == (void *) 0) {
                for (layers = 1; layers > 0;) {
                    fgets(line, MAX - 1, fp);
                    strip(line);
                    while (strlen(line) && line[strlen(line) - 1] == '\\') {
                        fgets(buf, MAX - 1, fp);
                        strip(buf);
                        line[strlen(line) - 1] = '\0';
                        strcat(line, buf);
                    }

                    lcount++;
                    if (feof(fp)) {
                        error("premature eof");
                        clean_up();
                        return FALSE;
                    }

                    tmp = line;
                    tmp = one_argument(tmp, cmd, sizeof(cmd));
                    memmove(line, tmp, strlen(tmp) + 1);
                    n = find_com(cmd);

                    switch (n) {
                    case D_CMD_IF:
                        layers++;
                        break;
                    case D_CMD_ENDIF:
                        layers--;
                        break;
                    }
                }
            }
            break;
        case D_CMD_SIF:
            eval_expr(line, &value);
            if (dead) {
                clean_up();
                return FALSE;
            }

            if (value.val == 0 && value.ptr == (void *) 0) {
                fgets(line, MAX - 1, fp);
                strip(line);
                while (strlen(line) && line[strlen(line) - 1] == '\\') {
                    fgets(buf, MAX - 1, fp);
                    strip(buf);
                    line[strlen(line) - 1] = '\0';
                    strcat(line, buf);
                }

                lcount++;
                if (feof(fp)) {
                    error("premature eof");
                    clean_up();
                    return FALSE;
                }
            }
        case D_CMD_ENDIF:
            break;
        case D_CMD_RETURN:
            clean_up();
            return FALSE;
        case D_CMD_EXIT:
            clean_up();
            return TRUE;
        case D_CMD_LABEL:
        case D_CMD_SUB:
            break;
        case D_CMD_ENDSUB:
            if (!oldpos) {
                error("endsub before gosub");
                clean_up();
                return FALSE;
            }
            fseek(fp, oldpos, 0);
            oldpos = 0;
            break;
        case D_CMD_GOTO:
            for (cp = line; *cp == ' '; cp++);
            sprintf(tmpbuf, "%s", cp);
            strcpy(line, tmpbuf);
            strcpy(lbl, line);

            rewind(fp);

            fgets(line, MAX - 1, fp);
            strip(line);
            while (strlen(line) && line[strlen(line) - 1] == '\\') {
                fgets(buf, MAX - 1, fp);
                strip(buf);
                line[strlen(line) - 1] = '\0';
                strcat(line, buf);
            }

            for (lcount = 1; !feof(fp); lcount++) {
                tmp = line;
                tmp = one_argument(tmp, cmd, sizeof(cmd));
                memmove(line, tmp, strlen(tmp) + 1);

                for (cp = line; *cp == ' '; cp++);
                sprintf(tmpbuf, "%s", cp);
                strcpy(line, tmpbuf);

                n = find_com(cmd);
                if (n == D_CMD_LABEL)
                    if (!strcmp(line, lbl))
                        break;

                fgets(line, MAX - 1, fp);
                strip(line);
                while (strlen(line) && line[strlen(line) - 1] == '\\') {
                    fgets(buf, MAX - 1, fp);
                    strip(buf);
                    line[strlen(line) - 1] = '\0';
                    strcat(line, buf);
                }
            }

            if (feof(fp)) {
                error("label not found");
                clean_up();
                return FALSE;
            }
            break;
        case D_CMD_GOSUB:
            if (oldpos) {
                error("nested gosub");
                clean_up();
                return FALSE;
            }

            oldpos = ftell(fp);
            for (cp = line; *cp == ' '; cp++);
            sprintf(tmpbuf, "%s", cp);
            strcpy(line, tmpbuf);
            strcpy(lbl, line);

            rewind(fp);

            fgets(line, MAX - 1, fp);
            strip(line);
            while (strlen(line) && line[strlen(line) - 1] == '\\') {
                fgets(buf, MAX - 1, fp);
                strip(buf);
                line[strlen(line) - 1] = '\0';
                strcat(line, buf);
            }

            for (lcount = 1; !feof(fp); lcount++) {
                tmp = line;
                tmp = one_argument(tmp, cmd, sizeof(cmd));
                memmove(line, tmp, strlen(tmp) + 1);

                for (cp = line; *cp == ' '; cp++);
                sprintf(tmpbuf, "%s", cp);
                strcpy(line, tmpbuf);

                n = find_com(cmd);
                if (n == D_CMD_SUB)
                    if (!strcmp(line, lbl))
                        break;

                fgets(line, MAX - 1, fp);
                strip(line);
                while (strlen(line) && line[strlen(line) - 1] == '\\') {
                    fgets(buf, MAX - 1, fp);
                    strip(buf);
                    line[strlen(line) - 1] = '\0';
                    strcat(line, buf);
                }
            }

            if (feof(fp)) {
                error("sub not found");
                clean_up();
                return FALSE;
            }
            break;
        case D_CMD_NOT_FOUND:
            eval_expr(oldline, &value);
            if (dead) {
                clean_up();
                return FALSE;
            }
            break;
        default:
            (*command[n].func) (line);
            if (dead) {
                clean_up();
                return FALSE;
            }
            break;
        }

        fgets(line, MAX - 1, fp);
    }

    clean_up();
    return FALSE;
}

/*
   char *get_npc_program(int nr) {
   struct program_data *p;

   for (p = program_list; p; p = p->next)
   if (p->nr == nr && p->type == PROG_NPC)
   return p->name;
   return (char *)0;
   }

   char *get_obj_program(int nr) {
   struct program_data *p;

   for (p = program_list; p; p = p->next)
   if (p->nr == nr && p->type == PROG_OBJECT)
   return p->name;
   return (char *)0;
   }

   char *get_room_program(int nr) {
   struct program_data *p;

   for (p = program_list; p; p = p->next)
   if (p->nr == nr && p->type == PROG_ROOM)
   return p->name;
   return (char *)0;
   }
 */

void
write_programs_to_buffer(char *buffer)
{
    struct program_data *prog;
    char buffer2[1000];
    for (prog = program_list; prog; prog = prog->next) {
        sprintf(buffer2, "%20s nr:%8d ran:%5d tot-sec:%6.3f\n\r", prog->name, prog->nr,
                prog->num_times_ran, prog->total_used);
        strcat(buffer, buffer2);
    };
}

void
cmd_execute(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX];
    FILE *fp;

    if (!ch->desc || var_list)
        return;

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!*arg1) {
        send_to_char("Usage: execute program arg1 arg2 ...\n\r", ch);
        return;
    }

    chdir(ch->desc->path);

    if (!(fp = fopen(arg1, "r"))) {
        send_to_char("Unable to open file.\n\r", ch);
        chdir(DFLT_DIR);
        return;
    }
    fclose(fp);

/*  start_user_prog(ch, arg1, argument); */
    chdir(DFLT_DIR);
}

void
cmd_program(CHAR_DATA * ch, const char *argument, int cmd)
{
    char arg1[MAX], arg2[MAX], arg3[MAX];
    char name[MAX], fn[MAX], buf[MAX];
    int nr, type;
    FILE *fp;
    struct program_data *p;

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));

    if (!*arg1 || !*arg2 || !*arg3) {
        send_to_char("Usage: program <name> <number> npc|object|room\n\r", ch);
        return;
    }

    strcpy(name, arg1);
    sprintf(fn, "dmpl/%s", name);
    if (!(fp = fopen(fn, "r"))) {
        send_to_char("No such program.\n\r", ch);
        return;
    } else
        fclose(fp);

    nr = atoi(arg2);

    *arg3 = LOWER(*arg3);
    switch (*arg3) {
    case 'n':
        type = PROG_NPC;
        break;
    case 'o':
        type = PROG_OBJECT;
        break;
    case 'r':
        type = PROG_ROOM;
        break;
    default:
        send_to_char("No such type of DMPL program.\n\r", ch);
        return;
    }

    CREATE(p, struct program_data, 1);
    p->name = strdup(name);
    p->nr = nr;
    p->type = type;
    p->next = program_list;
    program_list = p;

    if (!(fp = fopen(PROGRAM_FILE, "a"))) {
        send_to_char("Unable to open program file.\n\r", ch);
        return;
    }

    fprintf(fp, "%s %d %c\n", name, nr, *arg3);
    fclose(fp);

    sprintf(buf, "Program %s added.\n\r", name);
    send_to_char(buf, ch);
}

void
add_time(char *prog_name, int nr, int type, float add_amount)
{
    struct program_data *prog;

    for (prog = program_list; prog; prog = prog->next)
        if ((prog->type == type) && (prog->nr == nr) && (!strcmp(prog->name, prog_name))) {
            prog->total_used += add_amount;
            prog->num_times_ran++;
            return;
        }
}

/*
 * start_user_prog deleted as start_npc_prog is used in all CHARACTER (npc & pc)
 * cases now - Morgenes
 */

int
start_npc_prog(CHAR_DATA * ch, CHAR_DATA * npc, Command cmd, char *arg, char *file)
{
    FILE *fp;
    int ret;
    char fn[MAX];
    struct value_data value;
    struct var_data *pVar;
#if defined(DMPL_LOGGING)
    char buf[MAX];
#endif

    /* float time_used; unused -Morg */
    struct rusage rusage_before;
    /* struct rusage rusage_after; unused -Morg */


    getrusage(RUSAGE_SELF, &rusage_before);

    if (!file || *file == '\0' || (IS_NPC(npc) && npc->desc))
        return (0);
    strcpy(prog_name, file);

#if defined(DMPL_LOGGING)
    sprintf(buf, "Running char program %s.", prog_name);
    shhgamelog(buf);
#endif

    sprintf(fn, "dmpl/%s", file);
    if (!(fp = fopen(fn, "r")))
        return (0);

    value.val = 0;
    value.ptr = (void *) 0;
    declare_var("obj", VAR_OBJECT, &value);
    value.val = 0;
    value.ptr = (void *) 0;
    declare_var("room", VAR_OBJECT, &value);

    value.val = 0;
    value.ptr = (void *) ch;
    declare_var("ch", VAR_CHAR, &value);
    value.val = 0;
    value.ptr = (void *) npc;
    declare_var("npc", VAR_CHAR, &value);
    value.val = cmd;
    value.ptr = (void *) 0;
    declare_var("cmd", VAR_INT, &value);
    value.val = 0;
    value.ptr = (void *) strdup(arg);
    declare_var("arg", VAR_STRING, &value);

    dead = FALSE;
    ret = exec_program(fp);

    fclose(fp);
#if defined(DMPL_LOGGING)
    sprintf(buf, "Ending char program %s.", prog_name);
    shhgamelog(buf);
#endif

    /* Make sure we've still got an npc (d_extract_char) */
    if ((pVar = get_var("npc"))) {
        if (pVar->value.val == 0)
            return ret;
        else
            return ret;
    }

    /*
     * getrusage(RUSAGE_SELF, &rusage_after);
     * time_used = 
     * (rusage_after.ru_utime.tv_sec - rusage_before.ru_utime.tv_sec +
     * (rusage_after.ru_utime.tv_usec - rusage_before.ru_utime.tv_usec) /
     * (float) CLOCKS_PER_SEC)
     * +
     * (rusage_after.ru_stime.tv_sec - rusage_before.ru_stime.tv_sec +
     * (rusage_after.ru_stime.tv_usec - rusage_before.ru_stime.tv_usec) /
     * (float) CLOCKS_PER_SEC);
     * 
     * 
     * add_time(prog_name, npc_index[npc->nr].vnum, PROG_NPC , time_used);
     */

    return ret;
}

int
start_obj_prog(CHAR_DATA * ch, OBJ_DATA * obj, Command cmd, char *arg, char *file)
{
    FILE *fp;
    int ret;
    char fn[MAX];
    struct value_data value;
#if defined(DMPL_LOGGING)
    char buf[MAX];
#endif

    /* float time_used; unused -Morg */
    struct rusage rusage_before;
    /* struct rusage rusage_after; unused -Morg */

    getrusage(RUSAGE_SELF, &rusage_before);

    if (!file || *file == '\0')
        return (0);

    strcpy(prog_name, file);
    sprintf(fn, "dmpl/%s", file);

#if defined(DMPL_LOGGING)
    sprintf(buf, "Running obj program %s.", prog_name);
    shhgamelog(buf);
#endif

    if (!(fp = fopen(fn, "r")))
        return (0);

    value.val = 0;
    value.ptr = (void *) ch;
    declare_var("ch", VAR_CHAR, &value);

    value.val = 0;
    value.ptr = (void *) 0;
    declare_var("npc", VAR_CHAR, &value);

    value.val = 0;
    value.ptr = (void *) obj;
    declare_var("obj", VAR_OBJECT, &value);

    value.val = cmd;
    value.ptr = (void *) 0;
    declare_var("cmd", VAR_INT, &value);
    value.val = 0;
    value.ptr = (void *) strdup(arg);
    declare_var("arg", VAR_STRING, &value);

    dead = FALSE;
    ret = exec_program(fp);
#if defined(DMPL_LOGGING)
    sprintf(buf, "Ending obj program %s.", prog_name);
    shhgamelog(buf);
#endif
    fclose(fp);

    /*
     * getrusage(RUSAGE_SELF, &rusage_after);
     * time_used = 
     * (rusage_after.ru_utime.tv_sec - rusage_before.ru_utime.tv_sec +
     * (rusage_after.ru_utime.tv_usec - rusage_before.ru_utime.tv_usec) /
     * ( float ) CLOCKS_PER_SEC)
     * +
     * (rusage_after.ru_stime.tv_sec - rusage_before.ru_stime.tv_sec +
     * (rusage_after.ru_stime.tv_usec - rusage_before.ru_stime.tv_usec) /
     * ( float ) CLOCKS_PER_SEC);
     * 
     * add_time(prog_name, obj_index[obj->nr].vnum, PROG_OBJECT, time_used);  */

    return ret;
}

int
start_room_prog(CHAR_DATA * ch, ROOM_DATA * rm, Command cmd, char *arg, char *file)
{
    FILE *fp;
    int ret;
    char fn[MAX];
    struct value_data value;
#if defined(DMPL_LOGGING)
    char buf[MAX];

#endif
    /* float time_used; unused -Morg */
    struct rusage rusage_before;
    /* struct rusage rusage_after; unused -Morg */
    getrusage(RUSAGE_SELF, &rusage_before);

    strcpy(prog_name, file);

#if defined(DMPL_LOGGING)
    sprintf(buf, "Running room program %s.", prog_name);
    shhgamelog(buf);
#endif

    if (!file || *file == '\0')
        return 0;


    sprintf(fn, "dmpl/%s", file);

    if (!(fp = fopen(fn, "r")))
        return (0);

    value.val = 0;
    value.ptr = (void *) ch;
    declare_var("ch", VAR_CHAR, &value);
    value.val = 0;
    value.ptr = (void *) rm;
    declare_var("rm", VAR_ROOM, &value);
    value.val = cmd;
    value.ptr = (void *) 0;
    declare_var("cmd", VAR_INT, &value);
    value.val = 0;
    value.ptr = (void *) strdup(arg);
    declare_var("arg", VAR_STRING, &value);

    dead = FALSE;
    ret = exec_program(fp);

    fclose(fp);
#if defined(DMPL_LOGGING)
    sprintf(buf, "Running room program %s.", prog_name);
    shhgamelog(buf);
#endif
    /*
     * getrusage(RUSAGE_SELF, &rusage_after);
     * time_used = 
     * (rusage_after.ru_utime.tv_sec - rusage_before.ru_utime.tv_sec +
     * (rusage_after.ru_utime.tv_usec - rusage_before.ru_utime.tv_usec) /
     * (float)CLOCKS_PER_SEC)
     * +
     * (rusage_after.ru_stime.tv_sec - rusage_before.ru_stime.tv_sec +
     * (rusage_after.ru_stime.tv_usec - rusage_before.ru_stime.tv_usec) /
     * (float)CLOCKS_PER_SEC);
     */

    return ret;
}

