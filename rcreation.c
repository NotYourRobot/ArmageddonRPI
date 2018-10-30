/*
 * File: RCREATION.C
 * Usage: Routines and commands for room creation and zone maintenance.
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
#include <ctype.h>
#include <stdlib.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "limits.h"
#include "guilds.h"
#include "cities.h"
#include "event.h"
#include "creation.h"
#include "wagon_save.h"
#include "modify.h"
#include "utility.h"
#include "reporting.h"
#include "special.h"

void
cmd_rupdate(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    ROOM_DATA *rm;
    /* char buf[256];   unused -Morg */

    SWITCH_BLOCK(ch);

    if (!ch->desc)
        return;

    rm = ch->in_room;

    if (!rm) {
        send_to_char("You are not in a room, that is never good.\n\r", ch);
        return;
    };

    if (!has_privs_in_zone(ch, (rm->number) / 1000, MODE_STR_EDIT)) {
        send_to_char("You do not have STR_EDIT privs in that zone.\n\r", ch);
        return;
    };

    /* not working yet, send appropriate message -Morg
     * zone_update_on_room (rm);
     * send_to_char ("Done.\n\r", ch);
     */

    send_to_char("Not working yet.\n\r", ch);
};

/* create a room structure */
void
cmd_rcreate(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char msg[MAX_STRING_LENGTH];
    int virt_nr, tmp, flag = 0;
    int virt_nr2, i;
    ROOM_DATA *room;

    SWITCH_BLOCK(ch);

    if (!*argument) {
        send_to_char("You can't do that.\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (STRINGS_SAME(arg1, "-m")) {
        flag = 1;
        argument = one_argument(argument, arg1, sizeof(arg1));
        argument = one_argument(argument, arg2, sizeof(arg2));
        if (!*arg2)
            flag = 0;
    }

    sprintf(msg, "Flag is: %d\n\r", flag);
    send_to_char(msg, ch);

    virt_nr = atoi(arg1);

    if (!has_privs_in_zone(ch, virt_nr / 1000, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    if (!zone_table[(virt_nr / 1000)].is_booted) {
        send_to_char("That zone is not booted.\n\r", ch);
        return;
    }

    if (!flag) {

        if (get_room_num(virt_nr)) {
            send_to_char("That room number already exists.\n\r", ch);
            return;
        }

        CREATE(room, ROOM_DATA, 1);
        room->next = room_list;
        room_list = room;
        room->next_in_zone = zone_table[virt_nr / 1000].rooms;
        zone_table[virt_nr / 1000].rooms = room;

        room->number = virt_nr;
        room->name = strdup("New Room");
        room->specials = NULL;
        room->description = strdup("Empty.\n\r");
        room->room_flags = 0;
        MUD_SET_BIT(room->room_flags, RFL_UNSAVED);
        MUD_SET_BIT(room->room_flags, RFL_IMMORTAL_ROOM);
        room->sector_type = 0;
        room->ex_description = 0;
        room->zone = virt_nr / 1000;
        room->light = 0;
        room->city = CITY_NONE;
        room->contents = 0;
        room->people = 0;
        for (tmp = 0; tmp < 6; tmp++)
            room->direction[tmp] = 0;
        zone_table[room->zone].rcount++;

    } else {
        if (*arg2)
            virt_nr2 = atoi(arg2);
        else {
            send_to_char("You must supply an ending number.\n\r", ch);
            return;
        }

        if (virt_nr > virt_nr2) {
            send_to_char("Beginning room must be smaller than ending " "room.\n\r", ch);
            return;
        }
        if ((virt_nr / 1000) != (virt_nr2 / 1000)) {
            send_to_char("All rooms must be in same zone.\n\r", ch);
            return;
        }

        for (i = virt_nr; i <= virt_nr2; i++) {
            if (get_room_num(i))
                continue;

            CREATE(room, ROOM_DATA, 1);
            room->next = room_list;
            room_list = room;
            room->next_in_zone = zone_table[i / 1000].rooms;
            zone_table[i / 1000].rooms = room;

            room->number = i;
            room->name = strdup("New Room");
            room->specials = NULL;
            room->description = strdup("Empty.\n\r");
            room->room_flags = 0;
            MUD_SET_BIT(room->room_flags, RFL_UNSAVED);
            MUD_SET_BIT(room->room_flags, RFL_IMMORTAL_ROOM);
            room->sector_type = 0;
            room->ex_description = 0;
            room->zone = i / 1000;
            room->light = 0;
            room->city = CITY_NONE;
            room->contents = 0;
            room->people = 0;
            for (tmp = 0; tmp < 6; tmp++)
                room->direction[tmp] = 0;
            zone_table[room->zone].rcount++;

        }

    }

    send_to_char("Done.\n\r", ch);
}


/* delete a room structure */
void
cmd_rdelete(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH];
    ROOM_DATA *room;

    SWITCH_BLOCK(ch);

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!*arg1) {
        send_to_char("Usage: rdelete <room>\n\r", ch);
        return;
    }

    if (!(room = get_room_num(atoi(arg1)))) {
        send_to_char("That room does not exist.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    while (room->watched_by) {
        set_room_not_watching_room(room, room->watched_by->room_watching,
                                   room->watched_by->watch_type);
    }

    extract_room(room, 1);
    send_to_char("Done.\n\r", ch);
}


/* name a room */
void
set_rname(CHAR_DATA * ch, const char *argument)
{

    while (*argument == ' ')
        argument++;

    if (!*argument) {
        send_to_char("You have to name it something.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }
    free(ch->in_room->name);
    ch->in_room->name = strdup(argument);

    send_to_char("Done.\n\r", ch);
}


/* set the index of the room's background description */
void
set_rbdesc(CHAR_DATA * ch, const char *argument)
{
    int n, zone, index;
    char buf[256];

    argument = one_argument(argument, buf, sizeof(buf));
    n = atoi(buf);

    zone = n / MAX_BDESC;
    index = n % MAX_BDESC;

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    if (zone != ch->in_room->zone)
        send_to_char("The background must be in the same zone as the room.\n\r", ch);
    else if ((index < 0) || (index >= MAX_BDESC))
        send_to_char("That is an illegal index.\n\r", ch);
    else {
        ch->in_room->bdesc_index = index;
        send_to_char("Ok.\n\r", ch);
    }

    return;
}


/* set the room's description */
void
set_rfdesc(CHAR_DATA * ch, const char *argument)
{

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }
    act("$n has entered CREATING mode (updating room description).", TRUE, ch, 0, 0, TO_ROOM);
    if (ch->in_room->description && strlen(ch->in_room->description) > 0) {
        send_to_char("Continue editing description, use '.c' to clear.\n\r", ch);
        string_append(ch, &ch->in_room->description, MAX_DESC);
    } else {
        send_to_char("\n\rEnter a new description.\n\r", ch);
        string_edit(ch, &ch->in_room->description, MAX_DESC);
    }
}


/* create an extra description for a room */
void
set_redesc(CHAR_DATA * ch, const char *argument)
{
    struct extra_descr_data *tmp, *newdesc;

    if (!*(argument)) {
        send_to_char("You can't do that.\n\r", ch);
        return;
    }
    while (*argument == ' ')
        argument++;

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }
    for (tmp = ch->in_room->ex_description; tmp; tmp = tmp->next) {
        if (!stricmp(tmp->keyword, argument)) {
            break;
        }
    }

    if (!tmp) {
        global_game_stats.edescs++;
        CREATE(newdesc, struct extra_descr_data, 1);
        newdesc->next = ch->in_room->ex_description;
        ch->in_room->ex_description = newdesc;
        newdesc->keyword = strdup(argument);
        newdesc->description = (char *) 0;
    } else {
        newdesc = tmp;
    }

    act("$n has entered CREATING mode (updating room edesc).", TRUE, ch, 0, 0, TO_ROOM);
    if (newdesc->description && strlen(newdesc->description) > 0) {
        send_to_char("Continue editing description, use '.c' to clear.\n\r", ch);
        string_append(ch, &newdesc->description, MAX_DESC);
    } else {
        send_to_char("Enter a new description.\n\r", ch);
        string_edit(ch, &newdesc->description, MAX_DESC);
    }
}

/* remove an extra description from a room */
void
unset_redesc(CHAR_DATA * ch, const char *argument)
{
    if (!*argument) {
        send_to_char("You can't do that.\n\r", ch);
        return;
    }
    while (*argument == ' ')
        argument++;

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    if (!rem_room_extra_desc_value(ch->in_room, argument)) {
        send_to_char("There is no extra description using those keywords.\n\r", ch);
        return;
    }
    send_to_char("Done.\n\r", ch);
}


/* create a two-way link between rooms */
void
set_rlink(CHAR_DATA * ch, const char *argument)
{

    char buf1[256], buf2[256];
    int dir;
    ROOM_DATA *cha_rm, *tar_rm;

    argument = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

    switch (*buf1) {
    case 'n':
        dir = 0;
        break;
    case 'e':
        dir = 1;
        break;
    case 's':
        dir = 2;
        break;
    case 'w':
        dir = 3;
        break;
    case 'u':
        dir = 4;
        break;
    case 'd':
        dir = 5;
        break;
    default:
        dir = -1;
        break;
    }

    if (dir == -1) {
        send_to_char("What direction is that?\n\r", ch);
        return;
    }

    cha_rm = ch->in_room;
    tar_rm = get_room_num(atoi(buf2));

    if (!tar_rm) {
        send_to_char("There's no room of that number.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, cha_rm->zone, MODE_MODIFY)
        || !has_privs_in_zone(ch, tar_rm->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    if (ch->in_room->direction[dir]) {
        free(ch->in_room->direction[dir]->general_description);
        free(ch->in_room->direction[dir]->keyword);

        if (ch->in_room->direction[dir]->to_room->direction[rev_dir[dir]]) {
            free(ch->in_room->direction[dir]->to_room->direction[rev_dir[dir]]->general_description);
            free(ch->in_room->direction[dir]->to_room->direction[rev_dir[dir]]->keyword);
            free(ch->in_room->direction[dir]->to_room->direction[rev_dir[dir]]);
            ch->in_room->direction[dir]->to_room->direction[rev_dir[dir]] = 0;
        }
        free(ch->in_room->direction[dir]);
        ch->in_room->direction[dir] = 0;
    }

    CREATE(cha_rm->direction[dir], struct room_direction_data, 1);
    cha_rm->direction[dir]->general_description = 0;
    cha_rm->direction[dir]->keyword = 0;
    cha_rm->direction[dir]->exit_info = 0;
    cha_rm->direction[dir]->key = -1;
    
    cha_rm->direction[dir]->to_room = tar_rm;
    cha_rm->direction[dir]->to_room_num = NOWHERE;

    CREATE(tar_rm->direction[rev_dir[dir]], struct room_direction_data, 1);
    tar_rm->direction[rev_dir[dir]]->general_description = 0;
    tar_rm->direction[rev_dir[dir]]->keyword = 0;
    tar_rm->direction[rev_dir[dir]]->exit_info = 0;
    tar_rm->direction[rev_dir[dir]]->key = -1;
  
    tar_rm->direction[rev_dir[dir]]->to_room = cha_rm;
    tar_rm->direction[rev_dir[dir]]->to_room_num = NOWHERE;

    send_to_char("Done.\n\r", ch);
}


/* remove a two-way link */
void
unset_rlink(CHAR_DATA * ch, const char *argument)
{

    char buf[256];
    int dir;
    ROOM_DATA *cha_rm, *old_rm;

    argument = one_argument(argument, buf, sizeof(buf));

    switch (*buf) {
    case 'n':
        dir = 0;
        break;
    case 'e':
        dir = 1;
        break;
    case 's':
        dir = 2;
        break;
    case 'w':
        dir = 3;
        break;
    case 'u':
        dir = 4;
        break;
    case 'd':
        dir = 5;
        break;
    default:
        dir = -1;
        break;
    }

    if (dir == -1) {
        send_to_char("What direction is that?\n\r", ch);
        return;
    }

    cha_rm = ch->in_room;
    if (cha_rm->direction[dir]) {
        old_rm = ch->in_room->direction[dir]->to_room;
    } else {
        send_to_char("There is no exit in that direction.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, cha_rm->zone, MODE_MODIFY)
        || !has_privs_in_zone(ch, old_rm->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }
    if ((old_rm->zone != cha_rm->zone) && (GET_LEVEL(ch) < HIGHLORD)) {
        send_to_char("You cannot remove an inter-zone link unless you're an Overlord.\n\r", ch);
        return;
    }

    free(cha_rm->direction[dir]->keyword);
    cha_rm->direction[dir]->keyword = 0;
    free(cha_rm->direction[dir]->general_description);
    cha_rm->direction[dir]->general_description = 0;
    free(cha_rm->direction[dir]);
    cha_rm->direction[dir] = 0;

    if (old_rm->direction[rev_dir[dir]]) {
        free(old_rm->direction[rev_dir[dir]]->keyword);
        old_rm->direction[rev_dir[dir]]->keyword = 0;
        free(old_rm->direction[rev_dir[dir]]->general_description);
        old_rm->direction[rev_dir[dir]]->general_description = 0;
        free(old_rm->direction[rev_dir[dir]]);
        old_rm->direction[rev_dir[dir]] = 0;
    }
    send_to_char("Done.\n\r", ch);
}


/* set a one-way exit out of a room */
void
set_rexit(CHAR_DATA * ch, const char *argument)
{
    char buf1[256], buf2[256];
    int dir;
    ROOM_DATA *cha_rm, *tar_rm;

    argument = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

    switch (*buf1) {
    case 'n':
        dir = 0;
        break;
    case 'e':
        dir = 1;
        break;
    case 's':
        dir = 2;
        break;
    case 'w':
        dir = 3;
        break;
    case 'u':
        dir = 4;
        break;
    case 'd':
        dir = 5;
        break;
    default:
        dir = -1;
        break;
    }

    if (dir == -1) {
        send_to_char("What direction is that?\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    cha_rm = ch->in_room;
    tar_rm = get_room_num(atoi(buf2));

    if (!tar_rm) {
        send_to_char("There's no room of that number.\n\r", ch);
        return;
    }

    CREATE(cha_rm->direction[dir], struct room_direction_data, 1);
    cha_rm->direction[dir]->general_description = 0;
    cha_rm->direction[dir]->keyword = 0;
    cha_rm->direction[dir]->exit_info = 0;
    cha_rm->direction[dir]->key = -1;
 
    cha_rm->direction[dir]->to_room = tar_rm;
    cha_rm->direction[dir]->to_room_num = NOWHERE;

    send_to_char("Done.\n\r", ch);
}


/* remove a one-way exit from a room */
void
unset_rexit(CHAR_DATA * ch, const char *argument)
{
    char buf[256];
    int dir;
    ROOM_DATA *cha_rm;

    argument = one_argument(argument, buf, sizeof(buf));

    switch (*buf) {
    case 'n':
        dir = 0;
        break;
    case 'e':
        dir = 1;
        break;
    case 's':
        dir = 2;
        break;
    case 'w':
        dir = 3;
        break;
    case 'u':
        dir = 4;
        break;
    case 'd':
        dir = 5;
        break;
    default:
        dir = -1;
        break;
    }

    if (dir == -1) {
        send_to_char("What direction is that?\n\r", ch);
        return;
    }

    if (!ch->in_room->direction[dir]) {
        send_to_char("There is no exit in that direction.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    cha_rm = ch->in_room;

    free(cha_rm->direction[dir]->keyword);
    cha_rm->direction[dir]->keyword = 0;
    free(cha_rm->direction[dir]->general_description);
    cha_rm->direction[dir]->general_description = 0;
    free(cha_rm->direction[dir]);
    cha_rm->direction[dir] = 0;
    send_to_char("Done.\n\r", ch);
}


/* describe a direction for 'look {dir}' */
void
set_rddesc(CHAR_DATA * ch, const char *argument)
{
    int dir;
    char buf[256];

    argument = one_argument(argument, buf, sizeof(buf));

    switch (*buf) {
    case 'n':
        dir = 0;
        break;
    case 'e':
        dir = 1;
        break;
    case 's':
        dir = 2;
        break;
    case 'w':
        dir = 3;
        break;
    case 'u':
        dir = 4;
        break;
    case 'd':
        dir = 5;
        break;
    default:
        dir = -1;
        break;
    }

    if (dir == -1) {
        send_to_char("What direction is that?\n\r", ch);
        return;
    }
    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }
    if (!ch->in_room->direction[dir]) {
        send_to_char("There is no exit in that direction.\n\r", ch);
        return;
    }
    if (ch->in_room->direction[dir]->general_description
        && strlen(ch->in_room->direction[dir]->general_description) > 0) {
        send_to_char("Continue editing description, use '.c' to clear.\n\r", ch);
        string_append(ch, &ch->in_room->direction[dir]->general_description, MAX_DESC);
    } else {
        send_to_char("\n\rEnter a new description.\n\r", ch);
        string_edit(ch, &ch->in_room->direction[dir]->general_description, MAX_DESC);
    }
}


/* set the "sector type", how many moves it takes */
void
set_rsector(CHAR_DATA * ch, const char *argument)
{
    int num;
    char buf[256];

    argument = one_argument(argument, buf, sizeof(buf));

    if (!*buf) {
        send_to_char("You may set the following room sectors:\n\r", ch);
        send_attribs_to_char(room_sector, ch);
        return;
    }

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    num = get_attrib_num(room_sector, buf);
    if (num == -1) {
        send_to_char("That's not a sector.\n\r", ch);
        return;
    }

    ch->in_room->sector_type = room_sector[num].val;
    send_to_char("Done.\n\r", ch);
}

/* set the "city" for criminial/economy issues */
void
set_rcity(CHAR_DATA * ch, const char *argument)
{
    int num;
    char old_arg[256];
    char buf[256];

    strcpy(old_arg, argument);
    argument = one_argument(argument, buf, sizeof(buf));

    if (!*buf) {
        send_to_char("You may set the following cities:\n\r", ch);
        cmd_citystat(ch, "", CMD_CITYSTAT, 0);
        return;
    }

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    if (is_number(buf))
        num = atoi(buf);
    else
        for (num = 1; num < MAX_CITIES; num++)
            if (!str_prefix(old_arg, city[num].name))
                break;

    if (num < 0 || num >= MAX_CITIES) {
        send_to_char("No such city.\n\r", ch);
        return;
    }

    ch->in_room->city = num;
    send_to_char("Done.\n\r", ch);
}


/* set flags for a whole room */
void
set_rflags(CHAR_DATA * ch, const char *argument)
{
    int flags, loc;
    char buf[256];

    argument = one_argument(argument, buf, sizeof(buf));

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    if (!*buf) {
        send_to_char("You may set the following room flags:\n\r", ch);
        send_flags_to_char(room_flag, ch);
        return;
    }

    loc = get_flag_num(room_flag, buf);
    if (loc == -1) {
        send_to_char("That flag doesn't exist.\n\r", ch);
        return;
    }

    flags = room_flag[loc].bit;

    if (IS_SET(ch->in_room->room_flags, flags)) {
        sprintf(buf, "You have removed the '%s' flag.\n\r", room_flag[loc].name);
        REMOVE_BIT(ch->in_room->room_flags, flags);
        send_to_char(buf, ch);
    } else {
        sprintf(buf, "You have set the '%s' flag.\n\r", room_flag[loc].name);
        MUD_SET_BIT(ch->in_room->room_flags, flags);
        send_to_char(buf, ch);
    }
}


/* set flags for a door */
void
set_rdflags(CHAR_DATA * ch, const char *argument)
{
    int flags, loc, dir;
    char tmp[256], buf1[256], buf2[256];

    argument = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

    switch (*buf1) {
    case 'n':
        dir = 0;
        break;
    case 'e':
        dir = 1;
        break;
    case 's':
        dir = 2;
        break;
    case 'w':
        dir = 3;
        break;
    case 'u':
        dir = 4;
        break;
    case 'd':
        dir = 5;
        break;
    default:
        dir = -1;
        break;
    }

    if (dir == -1) {
        send_to_char("What direction is that?\n\r", ch);
        return;
    }

    if (!ch->in_room->direction[dir]) {
        send_to_char("That exit does not exist.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    if (!*buf2) {
        send_to_char("You may set the following direction flags or hidden door class:\n\r", ch);
        send_flags_to_char(exit_flag, ch);
        // if hidden levels then precede by h
        send_to_char("\nFor hidden doors set their level with rset dflags <dir> -level.\n The - will change the correct value.\n", ch);
        send_to_char("Can use any number 0 (no skill to find) to 100 (ouch) or 110 which is hidden special.\n", ch);
        return;
    }

    loc = get_flag_num(exit_flag, buf2);
    if (loc == -1) {
        send_to_char("That flag doesn't exist.\n\r", ch);
        return;
    }

    flags = exit_flag[loc].bit;

    if (IS_SET(ch->in_room->direction[dir]->exit_info, flags)) {
        sprintf(tmp, "You have removed the '%s' flag.\n\r", exit_flag[loc].name);
        REMOVE_BIT(ch->in_room->direction[dir]->exit_info, flags);
        send_to_char(tmp, ch);
    } else {
        sprintf(tmp, "You have set the '%s' flag.\n\r", exit_flag[loc].name);
        MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, flags);
        send_to_char(tmp, ch);
    }
}


/* make a door in an exit */
void
set_rdoor(CHAR_DATA * ch, const char *argument)
{
    char buf1[256], buf2[256];
    int dir;

    argument = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

    switch (*buf1) {
    case 'n':
        dir = 0;
        break;
    case 'e':
        dir = 1;
        break;
    case 's':
        dir = 2;
        break;
    case 'w':
        dir = 3;
        break;
    case 'u':
        dir = 4;
        break;
    case 'd':
        dir = 5;
        break;
    default:
        dir = -1;
        break;
    }

    if (dir == -1) {
        send_to_char("What direction is that?\n\r", ch);
        return;
    }
    if (!ch->in_room->direction[dir]) {
        send_to_char("There is no exit in that direction.\n\r", ch);
        return;
    }
    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_ISDOOR);
    MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_RSCLOSED);
    MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_RSLOCKED);
    ch->in_room->direction[dir]->keyword = strdup(buf2);

    send_to_char("Done.\n\r", ch);
}


/* set a key for a door */
void
set_rdkey(CHAR_DATA * ch, const char *argument)
{
    char buf1[256], buf2[256];
    int dir, rm;

    argument = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

    switch (*buf1) {
    case 'n':
        dir = 0;
        break;
    case 'e':
        dir = 1;
        break;
    case 's':
        dir = 2;
        break;
    case 'w':
        dir = 3;
        break;
    case 'u':
        dir = 4;
        break;
    case 'd':
        dir = 5;
        break;
    default:
        dir = -1;
        break;
    }

    if (dir == -1) {
        send_to_char("What direction is that?\n\r", ch);
        return;
    }
    if (!ch->in_room->direction[dir]) {
        send_to_char("That exit does not exist.  You must create it with RLINK or REXIT.\n\r", ch);
        return;
    }
    rm = FALSE;
    if (!*buf2)
        rm = TRUE;

    if (!isdigit(*buf2) && *buf2) {
        send_to_char("You must use the object number.\n\r", ch);
        return;
    }
    if ((real_object(atoi(buf2)) == -1) && (!rm)) {
        send_to_char("There is no object of that number.\n\r", ch);
        return;
    }
    if (!rm) {
        ch->in_room->direction[dir]->key = atoi(buf2);
        send_to_char("Done.\n\r", ch);
    } else {
        ch->in_room->direction[dir]->key = -1;
        send_to_char("Removed.\n\r", ch);
    }
}


void
set_rprog(CHAR_DATA * ch, const char *args)
{
    bool had_pulse = has_special_on_cmd(ch->in_room->specials, NULL, CMD_PULSE);

    ch->in_room->specials = set_prog(IS_IMMORTAL(ch) ? ch : 0, ch->in_room->specials, args);

    if (!had_pulse && has_special_on_cmd(ch->in_room->specials, NULL, CMD_PULSE))
        new_event(EVNT_ROOM_PULSE, PULSE_ROOM, 0, 0, 0, ch->in_room, 0);

    if (IS_IMMORTAL(ch))
        send_to_char("Done.\n\r", ch);
}


void
set_rwatching(CHAR_DATA * ch, const char *args)
{
    ROOM_DATA *watched_room;
    char buf1[256];
    int i;
    args = one_argument(args, buf1, sizeof(buf1));


    i = atoi(buf1);
    if (!strlen(args) || !strlen(buf1) || (i == 0)) {
        send_to_char("format: rset watching <roomnum> <message>\n\r", ch);
        send_to_char(" use an # in the string to insert message\n\r", ch);
        return;
    };


    watched_room = get_room_num(i);
    if (!watched_room) {
        send_to_char("That room # was invalid.\n\r", ch);
        return;

    }

    if (set_room_to_watch_room(watched_room, ch->in_room, args, WATCH_VIEW)) {
        send_to_char("I set this room to watch other room.\n\r", ch);
    } else {
        send_to_char("I could not set this room to watch other room.\n\r", ch);
    }
}

void
set_rlistening(CHAR_DATA * ch, const char *args)
{
    ROOM_DATA *watched_room;
    char buf1[256];
    int i;
    args = one_argument(args, buf1, sizeof(buf1));


    i = atoi(buf1);
    if (!strlen(args) || !strlen(buf1) || (i == 0)) {
        send_to_char("format: rset listening <roomnum> <message>\n\r", ch);
        send_to_char(" use an # in the string to insert message\n\r", ch);
        return;
    };


    watched_room = get_room_num(i);
    if (!watched_room) {
        send_to_char("That room # was invalid.\n\r", ch);
        return;

    }
    if (set_room_to_watch_room(watched_room, ch->in_room, args, WATCH_LISTEN)) {
        send_to_char("I set this room to listen to other room.\n\r", ch);
    } else {
        send_to_char("I could not set this room to listen to other room.\n\r", ch);
    }
}


/* here comes RSET */
void
cmd_rset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], tmp1[256];
    int loc, i;

    const char * const field[] = {
        "name",
        "bdescription",
        "description",
        "link",
        "exit",
        "edesc",
        "sector",
        "flags",
        "dflags",
        "ddesc",
        "door",
        "key",
        "program",
        "watching",
        "city",
        //      "tracks", 
        "listening",
        "\n"
    };

    char *desc[] = {
        "(room name)",
        "(index of background decription)",
        "(foreground room description)",
        "(two-way exit to another room)",
        "(one-way exit to another room)",
        "(extra description)",
        "(sector type)",
        "(room flags)",
        "(direction flags)",
        "(direction description)",
        "(door information)",
        "(key for door)",
        "(attach a program)",
        "(room this room is watching)",
        "(city the room is in, type citystat for a list)",
        //      "(Tracks that should appear in the room for the hunt skill.");
        "(room this room is listening)",
        "\n"
    };

    /* bleh it didn't like how i tried an array of function pointers */
    struct func_data
    {
        void (*func) (CHAR_DATA * ch, const char *argument);
    };

    struct func_data func[] = {
        {set_rname},
        {set_rbdesc},
        {set_rfdesc},
        {set_rlink},
        {set_rexit},
        {set_redesc},
        {set_rsector},
        {set_rflags},
        {set_rdflags},
        {set_rddesc},
        {set_rdoor},
        {set_rdkey},
        {set_rprog},
        {set_rwatching},
        {set_rcity},
        {set_rlistening},
        {0}
    };

    argument = one_argument(argument, arg1, sizeof(arg1));

    if (!*arg1) {
        for (i = 0; *field[i] != '\n'; i++) {
            sprintf(tmp1, "%-15s%s\n\r", field[i], desc[i]);
            send_to_char(tmp1, ch);
        }
        return;
    }

    loc = (old_search_block(arg1, 0, strlen(arg1), field, 0));

    if (loc == -1) {
        send_to_char("That field does not exist.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You do not have creation privs in this zone.\n\r", ch);
        return;
    }

    (*(func[loc - 1].func)) (ch, argument);
}

void
unset_rprog(CHAR_DATA * ch, const char *args)
{
    ch->in_room->specials = unset_prog(ch, ch->in_room->specials, args);
    if (IS_IMMORTAL(ch))
        send_to_char("Done.\n\r", ch);
}

void
unset_rwatching(CHAR_DATA * ch, const char *args)
{
    ROOM_DATA *watched_room;
    char buf1[256];
    int i;

    args = one_argument(args, buf1, sizeof(buf1));

    i = atoi(buf1);
    if (i == 0) {
        send_to_char("format: runset watching <roomnum>\n\r", ch);
        return;
    };


    watched_room = get_room_num(i);

    if (!watched_room) {
      cprintf(ch, "no such room, bucko\n\r");
      return;
    }

    if (set_room_not_watching_room(watched_room, ch->in_room, WATCH_VIEW)) {
        send_to_char("Set to not watch other room.\n\r", ch);
    } else {
        send_to_char("I could not stop this room from watching other room.\n\r", ch);
    };
}


void
unset_rlistening(CHAR_DATA * ch, const char *args)
{
    ROOM_DATA *watched_room;
    char buf1[256];
    int i;

    args = one_argument(args, buf1, sizeof(buf1));

    i = atoi(buf1);
    if (i == 0) {
        send_to_char("format: runset listening <roomnum>\n\r", ch);
        return;
    };

    watched_room = get_room_num(i);

    if (set_room_not_watching_room(watched_room, ch->in_room, WATCH_LISTEN)) {
        send_to_char("Set to not to listen to other room.\n\r", ch);
    } else {
        send_to_char("I could not stop this room from listening to other" " room.\n\r", ch);
    }
}




/* the counterpart, RUNSET */
void
cmd_runset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], tmp1[256], tmp2[256];
    int loc, i, j, spaces;

    const char * const field[] = {
        "link",
        "exit",
        "edesc",
        "program",
        "watching",
        "listening",
        "\n"
    };

    char *desc[] = {
        "(remove a two-way exit)",
        "(remove a one-way exit)",
        "(remove an extra description)",
        "(unatttach a program)",
        "(stop room watching from watching another)",
        "(stop room watching from listening to another)",
        "\n"
    };

    /* bleh it didn't like how i tried an array of function pointers */
    struct func_data
    {
        void (*func) (CHAR_DATA * ch, const char *argument);
    };

    struct func_data func[] = {
        {unset_rlink},
        {unset_rexit},
        {unset_redesc},
        {unset_rprog},
        {unset_rwatching},
        {unset_rlistening},
        {0}
    };

    argument = one_argument(argument, arg1, sizeof(arg1));

    if (!*arg1) {
        for (i = 0; *field[i] != '\n'; i++) {
            spaces = 15 - strlen(field[i]);
            for (j = 0; j < spaces; j++)
                tmp2[j] = ' ';
            tmp2[j] = '\0';
            sprintf(tmp1, "%s%s%s\n\r", field[i], tmp2, desc[i]);
            send_to_char(tmp1, ch);
        }
        return;
    }

    loc = (old_search_block(arg1, 0, strlen(arg1), field, 0));

    if (loc == -1) {
        send_to_char("That field does not exist.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)) {
        send_to_char("You do not have creation privs in this zone.\n\r", ch);
        return;
    }

    (*(func[loc - 1].func)) (ch, argument);
}


/* copy one room structure into another */
void
cmd_rcopy(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf1[256], buf2[256];
    ROOM_DATA *source, *target;

    argument = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));
    if (!*buf1) {
        send_to_char("Usage: rcopy <source> [<target>]\n\r", ch);
        return;
    }

    source = get_room_num(atoi(buf1));
    if (*buf2)
        target = get_room_num(atoi(buf2));
    else
        target = ch->in_room;

    if (!source || !target) {
        send_to_char("There is no room of that number.\n\r", ch);
        return;
    }

    if ((*buf2 && (atoi(buf1) == atoi(buf2))) || (!*buf2 && (atoi(buf1) == ch->in_room->number))) {
        send_to_char("Foolish one, rcopying should only be done to" " another room.\n\r", ch);
        return;
    }


    if (!has_privs_in_zone(ch, target->zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    free(target->name);
    target->name = source->name ? strdup(source->name) : strdup("");
    free(target->description);
    target->description = source->description ? strdup(source->description) : strdup("");

    target->bdesc_index = source->bdesc_index;
    target->sector_type = source->sector_type;
    target->room_flags = source->room_flags;
    target->city = room_in_city(source);

    free_edesc_list(target->ex_description);
    target->ex_description = copy_edesc_list(source->ex_description);

    send_to_char("Done.\n\r", ch);
}


/* save all rooms in a zone */
void
cmd_rsave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], err[256];
    int zone;

    SWITCH_BLOCK(ch);

    if (test_boot) {
        send_to_char("Saving rooms after a test boot can be bad.  Please reboot first.\n\r", ch);
        return;
    }

    SWITCH_BLOCK(ch);

    if (!ch->desc)
        return;

    argument = one_argument(argument, buf, sizeof(buf));

    if ((*buf) && (!is_number(buf))) {
        sprintf(err, "Rsave requires a number, '%s' is not a number.\n\r", buf);
        send_to_char(err, ch);
        return;
    }

    zone = atoi(buf);

    if (!*buf)
        zone = ch->in_room->zone;

    if (!has_privs_in_zone(ch, zone, MODE_MODIFY)) {
        send_to_char("You don't have creation privs in that zone.\n\r", ch);
        return;
    }

    if (!zone_table[zone].is_booted) {
        send_to_char("That zone is not booted.\n\r", ch);
        return;
    }

    write_to_descriptor(ch->desc->descriptor, "Room save in progress...\n\r");
    save_rooms(&zone_table[zone], zone);
    write_to_descriptor(ch->desc->descriptor, "Room save completed.\n\r");
}


/* zone commands... */
/* save the positions of things in a zone */
void
cmd_zsave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int zone;
    char buf[256];

    if (test_boot) {
        send_to_char
            ("Saving a zone during a test boot can be bad.  Please reboot with the full world first.\n\r",
             ch);
        return;
    }

    argument = one_argument(argument, buf, sizeof(buf));
    zone = atoi(buf);

    if (!*buf) {
        send_to_char("The zone number must be specified, to prevent accidental " "saving.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, zone, MODE_MODIFY)) {
        send_to_char("You don't have creation privs in that zone.\n\r", ch);
        return;
    }

    if (!zone_table[zone].is_booted) {
        send_to_char("That zone is not booted.\n\r", ch);
        return;
    }
    send_to_char("Zone save in progress...\n\r", ch);

    ROOM_DATA *room;
    for (room = zone_table[zone].rooms; room; room = room->next_in_zone) {
        if (IS_SET(room->room_flags, RFL_UNSAVED)) {
            cprintf(ch,
                    "Room %d is marked unsaved, writing areas file for zone, prior to saving commands\n\r",
                    room->number);
            save_rooms(&zone_table[zone], zone);
            break;
        }
    }

    save_zone(&zone_table[zone], zone);
    send_to_char("Zone save completed.\n\r", ch);
}


extern bool unbooting_db;

/* clear a zone of all objects and npcs */
void
cmd_zclear(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *tch, *next_ch;
    struct obj_data *tob;
    ROOM_DATA *rm;
    int zone;
    char buf[256];

    SWITCH_BLOCK(ch);

    argument = one_argument(argument, buf, sizeof(buf));
    zone = atoi(buf);

    if (!*buf)
        zone = ch->in_room->zone;

    if (!has_privs_in_zone(ch, zone, MODE_STR_EDIT)) {
        send_to_char("You do not have creation privs in that zone.\n\r", ch);
        return;
    }

    if (zone == RESERVED_Z || zone == 33 || zone == 13 || zone == 3) {
        cprintf(ch, "You cannot zclear zone %d.\n\r", zone);
        return;
    }

    sprintf(buf, "Clearing zone %d: %s", zone, zone_table[zone].name);
    gamelog(buf);

    for (rm = zone_table[zone].rooms; rm; rm = rm->next_in_zone) {
        for (tch = rm->people; tch; tch = next_ch) {
            next_ch = tch->next_in_room;

            if (IS_NPC(tch)) {
                act("$n explodes into little bits.", FALSE, tch, 0, 0, TO_ROOM);
                act("You explode into little bits.", FALSE, tch, 0, 0, TO_CHAR);
                extract_char(tch);
            }
        }

        OBJ_DATA *next_obj;
        for (tob = rm->contents; tob; tob = next_obj) {
            next_obj = tob->next_content;
            // extract everything BUT wagons
            if (!IS_WAGON(tob))
                extract_obj(tob);
        }
    }
}

/* reset a zone */
void
cmd_zreset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];
    int zone;

    argument = one_argument(argument, buf, sizeof(buf));
    zone = atoi(buf);

    if (!*buf)
        zone = ch->in_room->zone;

    if (!has_privs_in_zone(ch, zone, MODE_STR_EDIT)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }
    if (!zone_table[zone].is_booted) {
        send_to_char("That zone is not booted.\n\r", ch);
        return;
    }
    reset_zone(zone, FALSE);
}

void
cmd_rzclear(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *tch, *next_ch;
    struct obj_data *tob, *next_obj;
    ROOM_DATA *rm;
    int zone;
    char buf[256];

    zone = ch->in_room->zone;

    if (!has_privs_in_zone(ch, zone, MODE_STR_EDIT)) {
        send_to_char("You do not have STR_EDIT creation privs in that zone.\n\r", ch);
        return;
    }

    if (zone == 13) {
        send_to_char("You cannot rzclear in zone 13.\n\r", ch);
        return;
    }

    if (zone == 3) {
        send_to_char("You cannot rzclear in zone 3.\n\r", ch);
        return;
    }

    sprintf(buf, "Clearing room %d", ch->in_room->number);
    gamelog(buf);

    rm = ch->in_room;

    for (tch = rm->people; tch; tch = next_ch) {
        next_ch = tch->next_in_room;

        if (IS_NPC(tch)) {
            act("$n explodes into little bits.", FALSE, tch, 0, 0, TO_ROOM);
            act("You explode into little bits.", FALSE, tch, 0, 0, TO_CHAR);
            extract_char(tch);
        }
    }
    for (tob = rm->contents; tob; tob = next_obj) {
	next_obj = tob->next_content;
        // extract everything BUT wagons
        if (tob->obj_flags.type != ITEM_WAGON)
            extract_obj(tob);
    }
}


/* name a zone, currently under zset, zap this soon */
void
cmd_zname(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char(TYPO_MSG, ch);
}


/* set various things for a zone */
void
cmd_zset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], buf1[256], buf2[256], buf3[256], privsstring[256];
    int zone, uid, i, j;
    struct zone_priv_data *priv, *newpriv;

    argument = one_argument(argument, buf1, sizeof(buf1));
    argument = one_argument(argument, buf2, sizeof(buf2));
    argument = one_argument(argument, buf3, sizeof(buf3));

    if (*buf1 && !*buf2) {
        /* display the privs for that zone */
        zone = atoi(buf1);
        if ((zone < 0) || (zone > top_of_zone_table)) {
            send_to_char("That is not a valid zone.\n\r", ch);
            return;
        }
        sprintf(buf, "Privs for zone %d\n\r", zone);
        send_to_char(buf, ch);
        send_to_char("--------------------------\n\r", ch);

        for (priv = zone_table[zone].privs; priv; priv = priv->next) {
            sprint_flag(priv->type, grp_priv, privsstring);
            sprintf(buf, "UserID:%d, privs = %s\n\r", priv->uid, privsstring);
            send_to_char(buf, ch);
        }
        return;
    };

    if (!*buf1) {
        send_to_char("zset <zone>                         " "to see the privs\n\r", ch);
        send_to_char("zset <zone> a <age number>          " "to set the age\n\r", ch);
        send_to_char("zset <zone> t <time number>         " "to set the lifespan\n\r", ch);
        send_to_char("zset <zone> m <0-3>                 " "to set the mode\n\r", ch);
        send_to_char("zset <zone> n <namestring>          " "to set the zone-name\n\r", ch);
        send_to_char("zset <zone> <UID> <Priv>            " "to toggle a  priv\n\r", ch);
        return;
    }

    zone = atoi(buf1);
    if ((zone < 0) || (zone > top_of_zone_table)) {
        send_to_char("That is not a valid zone.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    };

    switch (LOWER(*buf2)) {
    case 't':
        if ((atoi(buf3) < 1) || (atoi(buf3) > 300)) {
            send_to_char("The time must be between 1 minute and 300 minutes (5 hours).\n\r", ch);
            return;
        }
        zone_table[zone].lifespan = atoi(buf3);
        break;
    case 'm':
        if ((atoi(buf3) < 0) || (atoi(buf3) > 3)) {
            send_to_char("The reset mode must be 0, 1, 2, or 3.\n\r", ch);
            return;
        }
        zone_table[zone].reset_mode = atoi(buf3);
        cprintf(ch, "Setting zone %d to mode %d.\n\r", zone, atoi(buf3));
        break;
    case 'n':
    case 'N':
        while (*argument == ' ')
            argument++;
        if (*argument == '-')
            sprintf(buf, "%s", argument + 1);
        else
            sprintf(buf, "%s (%s)", argument, GET_NAME(ch));
        free(zone_table[zone].name);
        zone_table[zone].name = strdup(buf);
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        uid = atoi(buf2);

        if (uid <= 0) {
            send_to_char("User ID must be more than 0.\n\r", ch);
            return;
        }

        j = get_flag_num(grp_priv, buf3);
        if (!strcmp(buf3, "all")) {
        } else if (!strcmp(buf3, "none")) {
        } else if (j == -1) {
            send_to_char("That is not a valid flag.\n\r", ch);
            send_to_char("-=-=-=-=-=-=-=-=-=-=-=-=-.\n\r", ch);

            sprintf(buf, "%s\n\r", "all");
            send_to_char(buf, ch);
            sprintf(buf, "%s\n\r", "none");
            send_to_char(buf, ch);
            for (i = 0; strlen(grp_priv[i].name); i++) {

                sprintf(buf, "%s\n\r", grp_priv[i].name);
                send_to_char(buf, ch);
            }
            return;
        };

        for (priv = zone_table[zone].privs; priv && (priv->uid != uid); priv = priv->next) {    /* do nothing find the uid node */
        };
        if (priv) {
            if (!strcmp(buf3, "all")) {
                sprintf(buf, "UID %d set to all bits.\n\r", uid);
                send_to_char(buf, ch);
                priv->type = ~0;
            } else if (!strcmp(buf3, "none")) {
                sprintf(buf, "UID %d set to no bit.\n\r", uid);
                send_to_char(buf, ch);
                priv->type = 0;
            } else {
                if (IS_SET(priv->type, grp_priv[j].bit)) {
                    sprintf(buf, "UID %d had that bit.\n\r", uid);
                    send_to_char(buf, ch);

                    REMOVE_BIT(priv->type, grp_priv[j].bit);
                    sprintf(buf, "Removed bit %s for UID:%d\n\r", grp_priv[j].name, uid);
                    send_to_char(buf, ch);
                } else {
                    sprintf(buf, "UID %d didnt have that bit\n\r", uid);
                    send_to_char(buf, ch);
                    MUD_SET_BIT(priv->type, grp_priv[j].bit);
                    sprintf(buf, "Added bit %s for UID:%d\n\r", grp_priv[j].name, uid);
                    send_to_char(buf, ch);

                }
            }
        } else {

            sprintf(buf, "UID %d didnt have a node, adding one.\n\r", uid);
            send_to_char(buf, ch);
            newpriv = (struct zone_priv_data *) malloc(sizeof(struct zone_priv_data));

            newpriv->uid = uid;
            if (!strcmp(buf3, "all")) {
                newpriv->type = ~0;
            } else if (!strcmp(buf3, "none")) {
                newpriv->type = 0;
            } else
                newpriv->type = grp_priv[j].bit;

            newpriv->next = zone_table[zone].privs;
            zone_table[zone].privs = newpriv;

        };
        send_to_char("Done.\n\r", ch);
        break;
    case 'a':
        if (atoi(buf3) == 999) {
            zone_table[zone].age = atoi(buf3);
        } else if ((atoi(buf3) < 0) || (atoi(buf3) > zone_table[zone].lifespan)){
            sprintf(buf, "The age must be between 0 minutes and %d minutes.\n\r",
                    zone_table[zone].lifespan);
            send_to_char(buf, ch);
            return;
        }
        zone_table[zone].age = atoi(buf3);
        break;
    default:
        send_to_char("You may only set the mode, time, and name.\n\r", ch);
        return;
    }

    save_zone_data();

}


/* assign a zone, currently offline */
void
cmd_zassign(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char(TYPO_MSG, ch);
}


/* open a zone */
void
cmd_zopen(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], fn[256], buf[256];
    int zone;
    int index;
    FILE *fp;

    ROOM_DATA *room;


    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!*arg1) {
        send_to_char("Which zone do you wish to open?\n\r", ch);
        return;
    } else {
        if (!has_privs_in_zone(ch, (zone = atoi(arg1)), MODE_STR_EDIT)
            && GET_LEVEL(ch) < HIGHLORD) {
            send_to_char("You do not have privileges in that zone.\n\r", ch);
            return;
        }
    }

    if (zone_table[zone].is_booted) {
        send_to_char("That zone is already booted.\n\r", ch);
        return;
    }

    sprintf(fn, "background/z%d.bdesc", zone);
    if (!(fp = fopen(fn, "r"))) {
        send_to_char("Error: Failed to open background file.\n\r", ch);
        return;
    }

    /* load background file */
    CREATE(bdesc_list[zone], struct bdesc_data, MAX_BDESC);
    for (index = 0; index < MAX_BDESC; index++)
        read_background(zone, index, fp);
    fclose(fp);

/*    bdesc_list[zone][0].desc = strdup("");
    bdesc_list[zone][0].name = strdup("Reserved Empty.");
    for (index = 1; index < MAX_BDESC; index++)
      {
        bdesc_list[zone][index].desc = strdup("");    
        bdesc_list[zone][index].name = strdup("Empty.");
      }*/


    sprintf(fn, "areas/z%d.wld", zone);
    if (!(fp = fopen(fn, "r"))) {
        send_to_char("Error: Failed to open world file.\n\r", ch);
        return;
    }

    sprintf(buf, "Opening zone %d: %s", zone, zone_table[zone].name);
    gamelog(buf);

    /* load 'em up */
    while (read_room(zone, fp));
    fclose(fp);

    /* sort em */
    room_list = sort_rooms(room_list);
    zone_table[zone].rooms = sort_rooms_in_zone(zone_table[zone].rooms);

    /* fix exits */
    for (room = zone_table[zone].rooms; room; room = room->next_in_zone)
        fix_exits(room);

    zone_table[zone].is_booted = TRUE;


    /* resetting */
    reset_zone(zone, FALSE);
    send_to_char("Zone opened.\n\r", ch);
}


/* close a zone */
void
cmd_zclose(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], buf[256];
    int zone;
    ROOM_DATA *room, *next_room;

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!*arg1) {
        if (!has_privs_in_zone(ch, (zone = ch->in_room->zone), MODE_STR_EDIT)
            && GET_LEVEL(ch) < HIGHLORD) {
            send_to_char("You do not have privileges in that zone.\n\r", ch);
            return;
        }
    } else {
        if (!has_privs_in_zone(ch, (zone = atoi(arg1)), MODE_STR_EDIT)
            && GET_LEVEL(ch) < HIGHLORD) {
            send_to_char("You do not have privileges in that zone.\n\r", ch);
            return;
        }
    }

    if (!zone_table[zone].is_booted) {
        send_to_char("That zone isn't booted.\n\r", ch);
        return;
    }

    if (zone_table[zone].is_open) {
        send_to_char("That zone is linked, and may not be closed.\n\r", ch);
        return;
    }

    sprintf(buf, "Closing zone %d: %s", zone, zone_table[zone].name);
    gamelog(buf);

    for (room = zone_table[zone].rooms; room; room = next_room) {
        next_room = room->next_in_zone;
        extract_room(room, 1);
    }

    zone_table[zone].rcount = 0;
    zone_table[zone].rooms = 0;
    zone_table[zone].is_booted = FALSE;

    free(bdesc_list[zone]);
    bdesc_list[zone] = NULL;

    send_to_char("Zone closed.\n\r", ch);
}


/* world commands... */
/* clear the entire world */
void
cmd_wclear(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int i;
    char buf[256];

    send_to_char("World clear in progress...\n\r", ch);
    gamelog("Beginning worldwide zone clear.");
    for (i = 0; i <= top_of_zone_table; i++) {
        sprintf(buf, "%d", i);
        cmd_zclear(ch, buf, 0, 0);
    }
    gamelog("Worldwide zone clear completed.");
}

void
cmd_wreset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int i;
    char buf[256];

    send_to_char("World reset in progress...\n\r", ch);
    gamelog("Beginning world reset.");
    for (i = 0; i <= top_of_zone_table; i++) {
        sprintf(buf, "%d", i);
        cmd_zreset(ch, buf, 0, 0);
    }
    gamelog("World reset completed.");
}


void
cmd_wzsave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int i;
    char buf[256];

    send_to_char("World zone save in progress...\n\r", ch);
    gamelog("Beginning global zone save.");
    for (i = 0; i <= top_of_zone_table; i++) {
        sprintf(buf, "%d", i);
        cmd_zsave(ch, buf, 0, 0);
    }
    gamelog("Global zone save completed.");
}


/* update the entire world */
void
cmd_wupdate(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    cmd_wclear(ch, "", 0, 0);
    cmd_wreset(ch, "", 0, 0);
    cmd_wzsave(ch, "", 0, 0);
}


/* save the whole damn world */
void
cmd_wsave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    if (!ch->desc)
        return;

    write_to_descriptor(ch->desc->descriptor, "World save in progress...\n\r");
    save_world();
    write_to_descriptor(ch->desc->descriptor, "World save completed.\n\r");
}

/* bset routines (bram) */

/* set the name of the background description.  the name is for reference 
purposes only -- it should never be shown to the players. */

void
bset_name(int zone, int index, const char *name, CHAR_DATA * ch)
{
    if (!*name) {
        send_to_char("You must specify a name.\n\r", ch);
        return;
    } else {
        free(bdesc_list[zone][index].name);
        while (*name == ' ')
            name++;
        bdesc_list[zone][index].name = strdup(name);
        send_to_char("Ok.\n\r", ch);
    }

    return;
}

/* edit a background description */
void
bset_desc(int zone, int index, CHAR_DATA * ch)
{
    if (bdesc_list[zone][index].desc) {
    }
    act("$n has entered CREATING mode (updating background description).", TRUE, ch, 0, 0, TO_ROOM);
    if (bdesc_list[zone][index].desc && strlen(bdesc_list[zone][index].desc) > 0) {
        send_to_char("Continue editing background description, use '.c' to clear.\n\r", ch);
        string_append(ch, &bdesc_list[zone][index].desc, MAX_DESC);
    } else {
        send_to_char("Enter new background description:\n\r", ch);
        string_edit(ch, &bdesc_list[zone][index].desc, MAX_DESC);
    }

    return;
}


/* set name or description of background description (bram) */
void
cmd_bset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int n, zone, index;
    char buf[256];

    argument = one_argument(argument, buf, sizeof(buf));
    n = atoi(buf);

    zone = n / MAX_BDESC;
    index = n % MAX_BDESC;

    if (!isdigit(*buf)) {
        send_to_char("Syntax:   bset <number> description\n\r"
                     "          bset <number> name <name>\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, zone, MODE_MODIFY)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    if (!index) {
        send_to_char("That background is reserved as the empty background.\n\r", ch);
        return;
    }

    argument = one_argument(argument, buf, sizeof(buf));

    if (!strncmp("name", buf, strlen(buf)))
        bset_name(zone, index, argument, ch);
    else if (!strncmp("description", buf, strlen(buf)))
        bset_desc(zone, index, ch);
    else
        send_to_char("Usage: bset # description|name\n\r", ch);

    return;
}

/* save background description list to disk */
void
cmd_backsave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];
    int zone;

    argument = one_argument(argument, buf, sizeof(buf));
    zone = atoi(buf);

    if (!*buf)
        zone = ch->in_room->zone;

    if (!has_privs_in_zone(ch, zone, MODE_MODIFY)) {
        send_to_char("You don't have creation privs in that zone.\n\r", ch);
        return;
    }

    if (!zone_table[zone].is_booted) {
        send_to_char("That zone is not booted.\n\r", ch);
        return;
    }

    write_to_descriptor(ch->desc->descriptor, "Background save in progress . . .\n\r");
    save_background(zone);
    write_to_descriptor(ch->desc->descriptor, "Background save done.\n\r");

    return;
}

void
cmd_rzsave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
/*    int room_num, zone;
    istringstream args;
    args.str(argument);

    SWITCH_BLOCK(ch);

    args >> room_num;
    if (args.fail()) {
        args.clear();
        string text;
        args >> text;

        if (text != "room") {
            send_to_char("A room number must be specified to prevent accidental saves, or use \"rzsave room\" to save the room you're in.\n\r", ch);
            return;
        }

        room_num = ch->in_room->number;
    }

    zone = room_num < 1000 ? 0 : room_num / 1000;
    if (!has_privs_in_zone(ch, zone, MODE_MODIFY)) {
        send_to_char("You don't have creation privs in that zone.\n\r", ch);
        return;
    }

    if (!zone_table[zone].is_booted) {
        send_to_char("That zone is not booted.\n\r", ch);
        return;
    }

    if (!get_room_num(room_num)) {
        cprintf(ch, "Room %d does not exist\n\r", room_num);
        return;
    }

    if (IS_SET(ch->specials.quiet_level, QUIET_ZONE)) {
        parse_command(ch, "quiet zone");
    }

    ostringstream msg;
    msg << "Room-limited zsave for room(" << room_num << "), Zone(" << zone << ")\n\r";
    string outstr = msg.str();
    send_to_char(outstr.c_str(), ch);

    room_rsave(room_num);
    room_zsave(room_num);
    */
}


void
cmd_rzreset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
  /*  int room_num;
    istringstream args;
    args.str(argument);

    SWITCH_BLOCK(ch);

    args >> room_num;
    if (args.fail()) {
        args.clear();
        string text;
        args >> text;

        if (text != "room") {
            send_to_char("A room number must be specified to prevent accidental saves, or use \"rzsave room\" to save the room you're in.\n\r", ch);
            return;
        }

        room_num = ch->in_room->number;
    }

    int zone = room_num / 1000;
    if (!has_privs_in_zone(ch, zone, MODE_STR_EDIT)) {
        send_to_char("You are not authorized to create in that zone.\n\r", ch);
        return;
    }

    if (!zone_table[zone].is_booted) {
        send_to_char("That zone is not booted.\n\r", ch);
        return;
    }

    reset_zone(zone, FALSE);

    ROOM_DATA *target_room = get_room_num(room_num);
    if (!target_room) {
        cprintf(ch, "Could not find room %d to rzreset\n\r", room_num);
        return;
    }

    CHAR_DATA *tmp;
    for (tmp = character_list; tmp; tmp = tmp->next)  {
        if (!IS_NPC(tmp)) continue;

        if (tmp->saved_room == -1) continue;


        // Move people to this room who belong here
        if (tmp->in_room
         && tmp->in_room->number != room_num
         && tmp->saved_room == room_num) {
            cprintf(ch, "Moved M%d to room %d.\n\r", npc_index[tmp->nr].vnum, room_num);
            char_from_room(tmp);
            char_to_room(tmp, get_room_num(room_num));
        // Move people who don't belong here to their saved room
        } else if (tmp->in_room
                && tmp->in_room->number == room_num
                && tmp->saved_room != room_num) {
            cprintf(ch, "Moved M%d to room %d.\n\r", npc_index[tmp->nr].vnum, tmp->saved_room);
            char_from_room(tmp);
            char_to_room(tmp, get_room_num(tmp->saved_room));
        }
    }

    cprintf(ch, "rzreset complete.\n\r");
*/
}
void
room_zsave(int room_num)
{
/*    int zone = room_num < 1000 ? 0 : room_num / 1000;

    struct zone_data newcmds;
    copy_reset_com_excluding_room(&zone_table[zone], &newcmds, room_num);
    newcmds.num_cmd--;  // remove dumb, useless 'S' command, which we'll re-add later
    add_zone_data_for_room(get_room_num(room_num), &newcmds, FALSE); // Add the room we're saving

    // Terminate zone cmds with an 'S' command
    struct reset_com endzone;
    memset(&endzone, 0, sizeof(endzone));
    endzone.command = 'S';
    add_comm_to_zone(&endzone, &newcmds);

    unallocate_reset_com(&zone_table[zone]);

    zone_table[zone].cmd               = newcmds.cmd;
    zone_table[zone].num_cmd           = newcmds.num_cmd;
    zone_table[zone].num_cmd_allocated = newcmds.num_cmd_allocated;

    save_all_zone_items(&zone_table[zone], zone, room_num);
    */
}

///////////////////////////////////////////////////////////////////
//
// This is a mini-rsave
//
///////////////////////////////////////////////////////////////////
void
room_rsave(int room_num)
{
 /*   int zone = room_num < 1000 ? 0 : room_num / 1000;

    ostringstream zone_file, zf_bak, output_name;
    zone_file << string("areas/z") << zone << string(".wld");
    zf_bak << zone_file.str() << ".bak_pre_rzsave";
    output_name << "areas/z" << zone << ".wld.rzsave";

    // We do C++ style input here, but fallback to C-style
    // output so that we can still use the old save routines
    ifstream ifstr(zone_file.str().c_str());
    FILE *of = fopen(output_name.str().c_str(), "w");
    if (!of) {
        gamelogf("Couldn't open %s for writing", output_name.str().c_str());
        return;
    }

    bool written = false;
    while (ifstr.good()) {
        room_buf rb;
        ifstr >> rb;
        // Either replace the room in the file, or insert it in sorted order
        if (rb.room_num == -1 || rb.room_num == room_num || (rb.room_num > room_num)) {
            if (!written && get_room_num(room_num)) {  // Don't write a room twice
                ROOM_DATA *rm = get_room_num(room_num);
                REMOVE_BIT(rm->room_flags, RFL_UNSAVED);
                write_room(rm, of);
            }
            written = true;

            if (rb.room_num != room_num) { // Was an insert, not a replace, otherwise, these would be equal
                fwrite(rb.rbuf.str().c_str(), 1, rb.length(), of); // Write the room after the insertion
            }
        } else {
            fwrite(rb.rbuf.str().c_str(), 1, rb.length(), of);
        }

        if (rb.room_num == -1) {
            break;
        }
    }

    // FIXME:  This clause is unnecessary now?
    if (!written && get_room_num(room_num)) {  // Got to the end without writing new room
        write_room(get_room_num(room_num), of);
        fprintf(of, "x|");
    }
    ifstr.close();

    fclose(of);

    shhlogf("Renaming %s to %s", zone_file.str().c_str(), zf_bak.str().c_str() );
    rename( zone_file.str().c_str(), zf_bak.str().c_str() );
    shhlogf("Renaming %s to %s", output_name.str().c_str(), zone_file.str().c_str() );
    rename( output_name.str().c_str(), zone_file.str().c_str() );
   */
}


