/*
 * File: PLAYABLE.C
 * Usage: Playable Object functions.
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>

#include "structs.h"
#include "handler.h"
#include "modify.h"
#include "comm.h"
#include "creation.h"
#include "cities.h"
#include "utility.h"
#include "utils.h"
#include "guilds.h"
#include "skills.h"
#include "db.h"

const char dtable[6][50] = {
    "+-------+\0|       |\0|   *   |\0|       |\0+-------+",
    "+-------+\0| *     |\0|       |\0|     * |\0+-------+",
    "+-------+\0| *     |\0|   *   |\0|     * |\0+-------+",
    "+-------+\0| *   * |\0|       |\0| *   * |\0+-------+",
    "+-------+\0| *   * |\0|   *   |\0| *   * |\0+-------+",
    "+-------+\0| *   * |\0| *   * |\0| *   * |\0+-------+"
};

char *
show_more_die(int *die, int numdie) {
    int i, j, total = 0;
    static char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    buf[0] = '\0';

    while (total < numdie) {
        for (j = 0; j < 5; j++) {
            for (i = total; (i < numdie) && (i < 6 + total); i++) {
                sprintf(buf2, "%s ", &(dtable[die[i] - 1][j * 10]));
                string_safe_cat(buf, buf2, MAX_STRING_LENGTH);
            }
            string_safe_cat(buf, "\n\r", MAX_STRING_LENGTH);
        }
        total += 6;
    }

    return buf;
}


void
print_more_die(CHAR_DATA * ch, int *die, int numdie)
{
    cprintf( ch, "%s", show_more_die( die, numdie) );
    return;
}

char *
show_dice(int die1, int die2)
{
    int dice[2];

    dice[0] = die1;
    dice[1] = die2;
    return show_more_die(dice, 2);
}

void
print_dice(CHAR_DATA *ch, int die1, int die2) {
    cprintf( ch, "%s", show_dice(die1, die2));
}



const char *bones_table[] = {
    "Nothing",
    "Flying Lizard",
    "Gith-Spear",
    "The Shadowed Path",
    "Torch at the Cave-Mouth",
    "Balking Inix",
    "Sandstorm in the Pass",
    "The Black Crevice",
    "Unwrapped Dart",
    "Circle of Stones",
    "Mountains Dancing"
};


void
print_bones(CHAR_DATA * ch, OBJ_DATA * obj)
{
    char buf[MAX_STRING_LENGTH];
    int i;

    if (ch == NULL || obj == NULL) {
        gamelog("print_bones: null ch or obj");
        return;
    }

    if (obj->obj_flags.type != ITEM_PLAYABLE || obj->obj_flags.value[0] != PLAYABLE_BONES) {
        gamelog("print_bones: obj not playable or bones");
        return;
    }

    if (obj->obj_flags.value[1] == 0 || obj->obj_flags.value[2] == 0
        || obj->obj_flags.value[3] == 0)
        return;

    buf[0] = '\0';
    for (i = 1; i < 4; i++) {
        strcat(buf, bones_table[obj->obj_flags.value[i]]);

        if (i == 1)
            strcat(buf, ", ");
        else if (i == 2)
            strcat(buf, ", and ");
        else
            strcat(buf, ".\n\r");
    }

    send_to_char(buf, ch);
    return;
}


const char *halfling_stones_table[] = {
    "Nothing",
    "Rising Sun",
    "Tandu's Tail",
    "Shady Grove",
    "Movement in the Shadows",
    "Evening Feast",
    "Open Hands",
    "Bend in the Trail",
    "Burning Tree",
    "Distant Shout"
};

void
print_halfling_stones(CHAR_DATA * ch, OBJ_DATA * obj, bool roller)
{
    char buf[MAX_STRING_LENGTH];

    if (ch == NULL || obj == NULL) {
        gamelog("print_halfling_stones: null ch or obj");
        return;
    }

    if (obj->obj_flags.type != ITEM_PLAYABLE || obj->obj_flags.value[0] != PLAYABLE_HALFLING_STONES) {
        gamelog("print_halfling_stones: obj not playable or halfling stones");
        return;
    }

    if (obj->obj_flags.value[1] == 0)
        return;

    sprintf(buf, "You %srecognize the pattern of %s.\n\r", roller ? "open your eyes and " : "",
            halfling_stones_table[obj->obj_flags.value[1]]);

    send_to_char(buf, ch);
    return;
}


const char *gypsy_dice_table[] = {
    "Nothing",
    "Jihae",
    "Lirathu",
    "Knife",
    "Purse",
    "Noose",
    "Kank"
};

void
print_gypsy_dice(CHAR_DATA * ch, OBJ_DATA * obj)
{
    char buf[MAX_STRING_LENGTH];

    if (ch == NULL || obj == NULL) {
        gamelog("print_gypsy_dice: null ch or obj");
        return;
    }

    if (obj->obj_flags.type != ITEM_PLAYABLE || obj->obj_flags.value[0] != PLAYABLE_GYPSY_DICE) {
        gamelog("print_gypsy_dice: obj not playable or gypsy_dice");
        return;
    }

    if (obj->obj_flags.value[1] == 0 || obj->obj_flags.value[2] == 0)
        return;

    sprintf(buf, "%s and %s\n\r", gypsy_dice_table[obj->obj_flags.value[1]],
            gypsy_dice_table[obj->obj_flags.value[2]]);
    send_to_char(buf, ch);
    return;
}

/*
 * do_roll - allow you to 'roll' dice and knucklebones
 */
void
cmd_roll(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *obj;
    OBJ_DATA *table;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *rch;

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));

    table = NULL;
    obj = NULL;

    if (arg1[0] == '\0') {
        send_to_char("What do you want to roll?\n\r", ch);
        return;
    }

    if ((obj = ch->equipment[ES]) == NULL) {
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            if (isname(arg1, OSTR(obj, name)))
                break;
        }
    }

    if (!obj) {
        send_to_char("You must be holding something to roll it.\n\r", ch);
        return;
    }

    if (!isname(arg1, OSTR(obj, name))) {
        send_to_char("You aren't holding that!\n\r", ch);
        return;
    }

    if (obj->obj_flags.type != ITEM_PLAYABLE) {
        send_to_char("You can't roll that!\n\r", ch);
        return;
    }

    if (arg2[0] != '\0') {
        if ((table = get_obj_in_list_vis(ch, arg2, ch->in_room->contents))
            == NULL) {
            act("There's no $T here to roll $p on.", FALSE, ch, obj, arg2, TO_CHAR);
            return;
        }

        if (table->obj_flags.type != ITEM_FURNITURE || !IS_SET(table->obj_flags.value[1], FURN_PUT)) {
            act("There's not a big enough surface to roll on $p.", FALSE, ch, table, NULL, TO_CHAR);
            return;
        }
    }

    switch (obj->obj_flags.value[0]) {
    case PLAYABLE_GYPSY_DICE:
    case PLAYABLE_DICE:
    case PLAYABLE_DICE_WEIGHTED:
        obj->obj_flags.value[1] = number(1, 6);
        obj->obj_flags.value[2] = number(1, 6);
        if (obj->obj_flags.value[0] == PLAYABLE_DICE_WEIGHTED && obj->obj_flags.value[3] > 0
            && obj->obj_flags.value[3] < 7 && obj->obj_flags.value[4] > 0
            && obj->obj_flags.value[4] < 7) {
            obj->obj_flags.value[1] = obj->obj_flags.value[3];
            obj->obj_flags.value[2] = obj->obj_flags.value[4];
            obj->obj_flags.value[3] = obj->obj_flags.value[4] = 0;
        } else if (obj->obj_flags.value[3] || obj->obj_flags.value[4])
            obj->obj_flags.value[3] = obj->obj_flags.value[4] = 0;

        /* take the dice away from them */
        if (obj == ch->equipment[ES])
            unequip_char(ch, ES);
        else
            obj_from_char(obj);

        if (table) {
            OBJ_DATA *tobj;

            act("You roll $p on $P.", FALSE, ch, obj, table, TO_CHAR);
            act("$n rolls $p on $P.", FALSE, ch, obj, table, TO_ROOM);
            obj_to_obj(obj, table);

#define SHOW_DICE_AROUND_TABLE
#ifdef SHOW_DICE_AROUND_TABLE
            /* echo to people sitting around the table */
            for (tobj = table->around; tobj; tobj = tobj->next_content) {
                PLYR_LIST *tpl;

                for (tpl = tobj->occupants; tpl; tpl = tpl->next) {
                    if (tpl->ch == ch)
                        continue;

                    act("$p come up:", FALSE, tpl->ch, obj, NULL, TO_CHAR);
                    if (obj->obj_flags.value[0] == PLAYABLE_DICE
                        || obj->obj_flags.value[0] == PLAYABLE_DICE_WEIGHTED)
                        print_dice(tpl->ch, obj->obj_flags.value[1], obj->obj_flags.value[2]);
                    else
                        print_gypsy_dice(tpl->ch, obj);
                }
            }
#endif
        } else {
            act("You roll $p.", FALSE, ch, obj, NULL, TO_CHAR);
            act("$n rolls $p.", FALSE, ch, obj, NULL, TO_ROOM);
            obj_to_room(obj, ch->in_room);
        }

        act("$p come up:", FALSE, ch, obj, NULL, TO_CHAR);
        if (obj->obj_flags.value[0] == PLAYABLE_DICE
            || obj->obj_flags.value[0] == PLAYABLE_DICE_WEIGHTED)
            print_dice(ch, obj->obj_flags.value[1], obj->obj_flags.value[2]);
        else
            print_gypsy_dice(ch, obj);

        break;

    case PLAYABLE_BONES:
        obj->obj_flags.value[1] = number(1, 10);

        do
            obj->obj_flags.value[2] = number(1, 10);
        while (obj->obj_flags.value[2] == obj->obj_flags.value[1]);

        do
            obj->obj_flags.value[3] = number(1, 10);
        while (obj->obj_flags.value[3] == obj->obj_flags.value[1]
               || obj->obj_flags.value[3] == obj->obj_flags.value[2]);

        /* take the bones away from them */
        if (obj == ch->equipment[ES])
            unequip_char(ch, ES);
        else
            obj_from_char(obj);

        if (table) {
            act("You cast $p on $P.", FALSE, ch, obj, table, TO_CHAR);
            act("$n casts $p on $P.", FALSE, ch, obj, table, TO_ROOM);
            obj_to_obj(obj, table);
        } else {
            act("You cast $p.", FALSE, ch, obj, NULL, TO_CHAR);
            act("$n casts $p.", FALSE, ch, obj, NULL, TO_ROOM);
            obj_to_room(obj, ch->in_room);
        }
        act("$p come up:", FALSE, ch, obj, NULL, TO_CHAR);
        print_bones(ch, obj);
        if (obj->ldesc)
            free(obj->ldesc);

        obj->ldesc = strdup("is lying here in three small piles.");
        break;

    case PLAYABLE_HALFLING_STONES:
        obj->obj_flags.value[1] = number(1, 9);

        /* take the stones away from them */
        if (obj == ch->equipment[ES])
            unequip_char(ch, ES);
        else
            obj_from_char(obj);

        if (table) {
            act("You let $p tumble from your hand onto $P.", FALSE, ch, obj, table, TO_CHAR);
            act("$n lets $p tumble from $s hand.", FALSE, ch, obj, NULL, TO_ROOM);
            obj_to_obj(obj, table);
        } else {
            act("You let $p tumble from your hand.", FALSE, ch, obj, NULL, TO_CHAR);
            act("$n lets $p tumble from $s hand.", FALSE, ch, obj, NULL, TO_ROOM);
            obj_to_room(obj, ch->in_room);
        }

        for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
            if (GET_RACE(rch) == RACE_HALFLING)
                print_halfling_stones(rch, obj, ch == rch);
        }

        break;

    default:
        send_to_char("You can't roll that!\n\r", ch);
        break;
    }
    return;
}


void
print_spice_run_table(CHAR_DATA * ch, OBJ_DATA * obj)
{
    int num_wagers = 0;
    int board[9];
    int gridnum, i;
    char ename[MAX_STRING_LENGTH], grid_edesc[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if (!obj || !ch)
        return;

    /* Initialize the game board */
    for (gridnum = 3; gridnum < 12; gridnum++) {
        sprintf(ename, "[grid_%d]", gridnum);
        if (!get_obj_extra_desc_value(obj, ename, grid_edesc, MAX_STRING_LENGTH)) {
            /* Something's whacked, abort! */
            return;
        }
        board[gridnum - 3] = atoi(grid_edesc);
    }
    num_wagers = obj->obj_flags.value[5];

    /* No wagers on the board, so clear out whatever old info was there */
    if (num_wagers == 0)
        for (i = 0; i < 9; i++)
            board[i] = 0;

    send_to_char("\n\r", ch);
    send_to_char("            +------------+------------+------------+\n\r", ch);
    send_to_char("            |            |            |            |\n\r", ch);
    sprintf(buf, "            |%4d coins  |%4d coins  |%4d coins  |\n\r", board[0], board[1],
            board[2]);
    send_to_char(buf, ch);
    send_to_char("            |           3|           4|           5|\n\r", ch);
    send_to_char("            +------------+------------+------------+\n\r", ch);
    send_to_char("            |            |            |            |\n\r", ch);
    sprintf(buf, "            |%4d coins  |%4d coins  |%4d coins  |\n\r", board[3], board[4],
            board[5]);
    send_to_char(buf, ch);
    send_to_char("            |           6|           7|           8|\n\r", ch);
    send_to_char("            +------------+------------+------------+\n\r", ch);
    send_to_char("            |            |            |            |\n\r", ch);
    sprintf(buf, "            |%4d coins  |%4d coins  |%4d coins  |\n\r", board[6], board[7],
            board[8]);
    send_to_char(buf, ch);
    send_to_char("            |           9|          10|          11|\n\r", ch);
    send_to_char("            +------------+------------+------------+\n\r", ch);

    if (num_wagers < 4) {
        sprintf(buf, "           There are currently %d wagers on the board.\n\r", num_wagers);
        send_to_char(buf, ch);
    } else
        send_to_char("          There are enough wagers on the board to play.\n\r", ch);


    send_to_char("\n\r", ch);
}

