/*
 * File: BOARD.C
 * Usage: Routines and commands involving bulletin boards.
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
/* Revision history
 * Please list changes to this file here for everyone's benefit.
 * 12/16/1999 Revision history added.  -Sanvean
 */
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "parser.h"
#include "comm.h"
#include "db.h"
#include "vt100c.h"
#include "modify.h"
#include "handler.h"
#include "structs.h"
#include "immortal.h"
#include "skills.h"

#define MAX_MSGS 50             /* Max number of messages.          */
#define MAX_MESSAGE_LENGTH 4096 /* that should be enough            */

#define IMMORT_BOARD    1
#define NORMAL_BOARD    2
#define AQT_BOARD       3
#define JIHAE_BOARD     4
#define DEN_BOARD       5
#define ALLANAK_BOARD   6
#define TULUK_BOARD     7
#define MANTIS_BOARD    8
#define HALFLING_BOARD  9
#define BLACKWING_BOARD 10
#define TEMPLAR_BOARD   11
#define MERC_BOARD      12
#define MAL_KRIAN_BOARD 13
#define SALAARI_BOARD   14
#define RL_RIDE_BOARD   15
#define TULUKI_VILLAGES 16
#define GLADIATOR_BOARD 17
#define REBELLION_BOARD 18
#define UNDER_TULUK_BOARD 19

char *msgs[MAX_BOARDS][MAX_MSGS];
char *auth[MAX_BOARDS][MAX_MSGS];
char *date[MAX_BOARDS][MAX_MSGS];
char *head[MAX_BOARDS][MAX_MSGS];

int msg_num[MAX_BOARDS];
int has_loaded[MAX_BOARDS];


void board_write_msg(int which, struct char_data *ch, const char *arg, char *startwith, int where,
                     struct obj_data *obj);
int board_display_msg(int which, struct char_data *ch, const char *arg, struct obj_data *obj);
int board_remove_msg(int which, struct char_data *ch, const char *arg, struct obj_data *obj);
void board_save_board(int which);
void board_load_board(int which);
void board_unload_board(int which);
void board_reset_board(int which);

int char_can_del_message(struct char_data *ch, int board_num);
int board_show_board(int which, struct char_data *ch, const char *arg, struct obj_data *obj);
char *compile_reply(int which, struct char_data *ch, int msg, int quote);

int
char_can_use_board(CHAR_DATA * ch, OBJ_DATA * board)
{
    char edesc[256];
    char guild[80];
    int guild_num;
    char rank[80];
    int rank_num;
    int has_language = 0;
    int is_guild = 0;

    if (board) {
        if (get_obj_extra_desc_value(board, "[BOARD_LANGUAGE]", edesc, 100)) {
            if (!stricmp(edesc, "allundean") && has_skill(ch, LANG_ELVISH))
                has_language = 1;
            else if (!stricmp(edesc, "rw allundean") && has_skill(ch, RW_LANG_ELVISH))
                has_language = 1;
            else if (!stricmp(edesc, "sirihish") && has_skill(ch, LANG_COMMON))
                has_language = 1;
            else if (!stricmp(edesc, "rw sirihish") && has_skill(ch, RW_LANG_COMMON))
                has_language = 1;
            else if (!stricmp(edesc, "bendune") && has_skill(ch, LANG_NOMAD))
                has_language = 1;
            else if (!stricmp(edesc, "rw bendune") && has_skill(ch, RW_LANG_NOMAD))
                has_language = 1;
            else if (!stricmp(edesc, "mirukkim") && has_skill(ch, LANG_DWARVISH))
                has_language = 1;
            else if (!stricmp(edesc, "rw mirukkim") && has_skill(ch, RW_LANG_DWARVISH))
                has_language = 1;
            else if (!stricmp(edesc, "kentu") && has_skill(ch, LANG_HALFLING))
                has_language = 1;
            else if (!stricmp(edesc, "rw kentu") && has_skill(ch, RW_LANG_HALFLING))
                has_language = 1;
            else if (!stricmp(edesc, "nrizkt") && has_skill(ch, LANG_MANTIS))
                has_language = 1;
            else if (!stricmp(edesc, "rw nrizkt") && has_skill(ch, RW_LANG_MANTIS))
                has_language = 1;
            else if (!stricmp(edesc, "galith") && has_skill(ch, LANG_GALITH))
                has_language = 1;
            else if (!stricmp(edesc, "rw galith") && has_skill(ch, RW_LANG_GALITH))
                has_language = 1;
            else if (!stricmp(edesc, "cavalish") && has_skill(ch, LANG_MERCHANT))
                has_language = 1;
            else if (!stricmp(edesc, "rw cavalish") && has_skill(ch, RW_LANG_MERCHANT))
                has_language = 1;
            else if (!stricmp(edesc, "tatlum") && has_skill(ch, LANG_ANCIENT))
                has_language = 1;
            else if (!stricmp(edesc, "rw tatlum") && has_skill(ch, RW_LANG_ANCIENT))
                has_language = 1;
            else if (!stricmp(edesc, "anyar") && has_skill(ch, LANG_ANYAR))
                has_language = 1;
            else if (!stricmp(edesc, "rw anyar") && has_skill(ch, RW_LANG_ANYAR))
                has_language = 1;
            else if (!stricmp(edesc, "heshrak") && has_skill(ch, LANG_HESHRAK))
                has_language = 1;
            else if (!stricmp(edesc, "rw heshrak") && has_skill(ch, RW_LANG_HESHRAK))
                has_language = 1;
            else if (!stricmp(edesc, "vloran") && has_skill(ch, LANG_VLORAN))
                has_language = 1;
            else if (!stricmp(edesc, "rw vloran") && has_skill(ch, RW_LANG_VLORAN))
                has_language = 1;
            else if (!stricmp(edesc, "domat") && has_skill(ch, LANG_DOMAT))
                has_language = 1;
            else if (!stricmp(edesc, "rw domat") && has_skill(ch, RW_LANG_DOMAT))
                has_language = 1;
            else if (!stricmp(edesc, "ghatti") && has_skill(ch, RW_LANG_GHATTI))
                has_language = 1;
            else if (!stricmp(edesc, "rw ghatti") && has_skill(ch, RW_LANG_GHATTI))
                has_language = 1;

        } else                  // If the edesc isn't there, then no language required.
            has_language = 1;

        if (get_obj_extra_desc_value(board, "[BOARD_GUILD]", edesc, 100)) {
            //      gamelog (edesc);
            //      one_argument(edesc, guild);
            //      gamelog (guild);
            //      one_argument(edesc, rank);
            //      gamelog (rank);

            if (!is_number(guild)) {
                //              gamelog ("Guild is not a number.");
                is_guild = 0;
            } else if (!is_number(rank)) {
                //              gamelog ("Rank is not a number.");
                is_guild = 0;
            } else {
                guild_num = atoi(guild);
                rank_num = atoi(rank);
                if (get_clan_rank(ch->clan, guild_num) < rank_num) {
                    //              gamelog ("Rank is too low or not in clan.");
                    is_guild = 0;
                } else
                    is_guild = 1;
            }
        } else                  // If the edesc isn't there, then no guild is required.
            is_guild = 1;
        if (is_guild && has_language)
            return (TRUE);
        else
            return (FALSE);
    } else {
        errorlog("Var 'board' passed to char_can_use_bard() in board.c was null.");
        return (FALSE);
    }
}

int
char_can_del_message(struct char_data *ch, int board_num)
{
    int i;
    struct board_data
    {
        char *name;
        int num;
    };

    struct board_data board_ok[] = {
        {"Nen", BLACKWING_BOARD},
        {"Eris", BLACKWING_BOARD},
        {"\n", -1}
    };


    for (i = 0; board_ok[i].num != -1; i++) {
        if (STRINGS_I_SAME(GET_NAME(ch), board_ok[i].name) && (board_num == board_ok[i].num))
            return TRUE;
    }

    return FALSE;
}

int
is_board(int virt)
{
    return (virt == 21030 || virt == 50701 || virt == 50602 ||  /* Allanak templar */
            virt == 1710 || virt == 1709 || virt == 1708 || virt == 1707 || virt == 1704 || virt == 1703 || virt == 1702 || virt == 39601 || virt == 24099 || virt == 1701 || virt == 37805 || virt == 70004 || /* gith  */
            virt == 40400 ||    /* Kadian  */
            virt == 45995 || virt == 19754 || virt == 67999 ||  /* Blackmoon */
            virt == 1711 ||     /* Gladiator */
            virt == 17159 ||    /* Rebellion  */
            virt == 29999 ||    /* Northlands  */
            virt == 35019 ||    /* Oash  */
            virt == 10002 ||    /* Red Storm   */
            virt == 5607 ||     /* Allanak Soldiers */
            virt == 1712 ||     /* House Borsail */
            virt == 77502 ||    /* Hemach kurad */
            virt == 21227 ||    /* Desert Elf Outpost */
            virt == 35256 ||    /* Winrothol */
            virt == 35257 ||    /* Tennshi */
            virt == 997 ||      /* Byn north Board */
            virt == 10003 ||    /* RSE Board */
            virt == 35570 ||    /* Tor Academy Board */
            virt == 35557 ||    /* Winrothol Board */
            virt == 35560 ||    /* Winrothol Board (read only) */
            virt == 4153 ||     /* Sun RUnner */
            virt == 1714 ||     /* Senate Board */
            virt == 1713 ||     /* Tan Muark */
            virt == 35646 ||    /* Allanaki RPT board */
            virt == 35647 ||    /* Tuluki RPT board */
            virt == 17220);     /* New player / starter board */



};

void
unboot_boards(void)
{
    int which;

    for (which = 0; which < MAX_BOARDS; which++) {
        if (has_loaded[which]) {
            board_unload_board(which);
            has_loaded[which] = 0;
        }
    };
};


int
board(struct char_data *ch, int cmd, const char *arg, struct char_data *npc_in,
      struct room_data *rm_in, struct obj_data *obj_in)
{
    struct obj_data *obj;
    char buf[256], arg1[256], arg2[256];
    int msg, which, quote;
    char tmp[MAX_INPUT_LENGTH];

    if (!*arg)
        return FALSE;

    // short circuit on the command, save us looking for the object
    if (cmd != CMD_LOOK && cmd != CMD_LISTEN && cmd != CMD_WRITE && cmd != CMD_READ
        && cmd != CMD_DELETE && cmd != CMD_REPLY)
        return FALSE;

    // grab the first word
    arg = one_argument(arg, tmp, sizeof(tmp));

    // look for an object in the room with that word
    if (!(obj = get_obj_in_list_vis(ch, tmp, ch->in_room->contents)))
        return FALSE;

    // if it's not the object that triggered the special, bail
    if (obj != obj_in)
        return FALSE;

    // if we don't have a unique number???
    if (obj->nr <= 0)
        return FALSE;

    if (is_board(obj_index[obj->nr].vnum)) {
        obj->obj_flags.type = ITEM_RUMOR_BOARD;
    }
    // make sure it's a valid board
    if (obj->obj_flags.type != ITEM_RUMOR_BOARD)
        return FALSE;

    which = obj->obj_flags.value[0];

    if (!ch->desc)
        return (FALSE);

    if (!has_loaded[which]) {
        board_load_board(which);
        has_loaded[which] = 1;
    }

    /*
     * Stop mortals & legends from viewing the quest and immortal board)
     * - Morgenes
     */
    if ((which == IMMORT_BOARD || which == AQT_BOARD)
        && GET_LEVEL(ch) < STORYTELLER)
        return (FALSE);

    // Support for boards that restrict based on clan, rank, language
    // Added by Nessalin 3/20/2004
    //  if (!char_can_use_board(ch, obj))
    //    {
    //      send_to_char("You cannot use that board.\n\r", ch);
    //      return (TRUE);
    //    }

    switch (cmd) {
    case CMD_LOOK:
        return (board_show_board(which, ch, arg, obj));
    case CMD_LISTEN:
        arg = one_argument(arg, arg1, sizeof(arg1));
        if (isdigit(*arg1)) {
            return (board_display_msg(which, ch, arg1, obj));
        }
        return (board_show_board(which, ch, arg1, obj));
    case CMD_WRITE:
        if (*arg) {
            board_write_msg(which, ch, arg, 0, msg_num[which] - 1, obj);
        } else {
            board_save_board(which);
            send_to_char("The board is now saved to disk.\n\r", ch);
        }
        return TRUE;
    case CMD_READ:
        return (board_display_msg(which, ch, arg, obj));
    case CMD_DELETE:
        return (board_remove_msg(which, ch, arg, obj));
    case CMD_REPLY:
        if (!*arg) {
            send_to_char("Reply to what message number?\n\r", ch);
        } else {
            arg = two_arguments(arg, arg1, sizeof(arg1), arg2, sizeof(arg2));
            if (!isdigit(*arg1)) {
                send_to_char("Usage: reply <board> <msg> [q]\n\r", ch);
                return TRUE;
            }
            if (*arg2 == 'q')
                quote = TRUE;
            else if (!*arg2)
                quote = FALSE;
            else {
                send_to_char("Usage: reply <board> <msg> [q]\n\r", ch);
                return TRUE;
            }
            msg = atoi(arg1);
            if (msg > msg_num[which] || msg < 1) {
                send_to_char("That message exists only in your imagination.\n\r", ch);
                return TRUE;
            }
            sprintf(buf, ">%s", head[which][msg - 1]);
            board_write_msg(which, ch, buf, compile_reply(which, ch, msg - 1, quote), msg - 1, obj);
        }
        return TRUE;
    default:
        return 0;
    }
}

void
board_write_msg(int which, struct char_data *ch, const char *arg, char *startwith, int where,
                struct obj_data *obj)
{
    int ind;
    long ct;
    struct descriptor_data *d;

    if (!char_can_use_board(ch, obj)) {
        send_to_char("You cannot use that board.\n\r", ch);
        return;
    }

    if (msg_num[which] > MAX_MSGS - 1) {
        send_to_char("The board is full already.\n\r", ch);
        return;
    }

    if (ch->lifting) {
        if (!drop_lifting(ch, ch->lifting))
            return;
    }

    /* skip blanks */
    for (; isspace(*arg); arg++);
    if (!*arg) {
        send_to_char("We must have a headline!\n\r", ch);
        return;
    }

    for (ind = msg_num[which] - 1; ind > where; ind--) {
        auth[which][ind + 1] = auth[which][ind];
        head[which][ind + 1] = head[which][ind];
        date[which][ind + 1] = date[which][ind];
        for (d = descriptor_list; d; d = d->next)
            if (d->str == &msgs[which][ind + 1])
                d->str = &msgs[which][ind];
        msgs[which][ind + 1] = msgs[which][ind];
    }
    msg_num[which]++;
    auth[which][++where] = 0;
    head[which][where] = 0;
    date[which][where] = 0;
    msgs[which][where] = 0;

    char authorstr[1024];
    char *authname = GET_NAME(ch), *authacct = ch->account;

    if (!IS_IMMORTAL(ch)) {
        snprintf(authorstr, sizeof(authorstr), "^%s (%s)", (authname ? authname : "Unknown"),
                 (authacct ? authacct : "Unknown"));
    } else {
        snprintf(authorstr, sizeof(authorstr), "%s", (authname ? authname : "Unknown"));
    }
    auth[which][where] = strdup(authorstr);

    head[which][where] = strdup(arg);
    if (!auth[which][where] || !head[which][where]) {
        gamelog("Malloc for board header failed.\n\r");
        send_to_char("The board is malfunctioning - sorry.\n\r", ch);
        return;
    }

    ct = time(0);
    date[which][where] = strdup(asctime(localtime(&ct)));
    date[which][where][24] = '\0';

    if (startwith) {
        msgs[which][where] = startwith;
    } else {
        msgs[which][where] = NULL;
    }

    if (!IS_IMMORTAL(ch))
        send_to_char("*** YOU ARE WRITING ON A MESSAGE BOARD. ***\n\r", ch);

    send_to_char("Write your message.\n\r", ch);
    act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);
    string_edit(ch, &msgs[which][where], MAX_MESSAGE_LENGTH);
    return;
}


int
board_remove_msg(int which, struct char_data *ch, const char *arg, struct obj_data *obj)
{
    int ind, msg;
    char buf[256], number[256];
    struct descriptor_data *d;

    arg = one_argument(arg, number, sizeof(number));

    if (!char_can_use_board(ch, obj)) {
        send_to_char("You cannot use that board.\n\r", ch);
        return (TRUE);
    }

    if (!*number || !isdigit(*number))
        return (0);
    if (!(msg = atoi(number)))
        return (0);
    if (!msg_num[which]) {
        send_to_char("The board is empty!\n\r", ch);
        return (1);
    }
    if (msg < 1 || msg > msg_num[which]) {
        send_to_char("That message exists only in your imagination.\n\r", ch);
        return (1);
    }

    if (!IS_IMMORTAL(ch)) {
        send_to_char("Mortals may not remove messages.\n\r", ch);
        return 0;
    }

    if (stricmp(auth[which][--msg], GET_NAME(ch))
        && (GET_LEVEL(ch) < HIGHLORD
            && !has_extra_cmd(ch->granted_commands, CMD_DELETE, HIGHLORD, TRUE)))
/*
      (!has_privs_in_zone(ch, ch->in_room->zone, MODE_MODIFY)))
*/
    {
        if (!char_can_del_message(ch, which)) {
            send_to_char("You may not delete messages that you didn't write.\n\r", ch);
            return 0;
        }
    }

    ind = msg;
    free(head[which][ind]);
    free(auth[which][ind]);
    free(date[which][ind]);
    for (d = descriptor_list; d; d = d->next)
        if (d->str == &msgs[which][ind])
            d->str = 0;
    if (msgs[which][ind])
        free(msgs[which][ind]);

    for (; ind < msg_num[which] - 1; ind++) {
        auth[which][ind] = auth[which][ind + 1];
        head[which][ind] = head[which][ind + 1];
        date[which][ind] = date[which][ind + 1];
        for (d = descriptor_list; d; d = d->next)
            if (d->str == &msgs[which][ind])
                d->str = &msgs[which][ind + 1];
        msgs[which][ind] = msgs[which][ind + 1];
    }
    msg_num[which]--;
    send_to_char("Message removed.\n\r", ch);
    sprintf(buf, "$n deletes message %d from the board.", msg + 1);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    board_save_board(which);

    return (1);
}

void
board_save_board(int which)
{
    FILE *the_file;
    int ind, len;
    char fname[256];

    if (!msg_num[which]) {
        gamelog("No messages to save.\n\r");
        return;
    }

    sprintf(fname, "data_files/board%d.msg", which + 1);
    the_file = fopen(fname, "wb");

    if (!the_file) {
        gamelog("Unable to open/create savefile..\n\r");
        return;
    }
    fwrite(&msg_num[which], sizeof(int), 1, the_file);
    for (ind = 0; ind < msg_num[which]; ind++) {
        if (msgs[which][ind]) {
            len = strlen(auth[which][ind]) + 1;
            fwrite(&len, sizeof(int), 1, the_file);
            fwrite(auth[which][ind], sizeof(char), len, the_file);

            len = strlen(head[which][ind]) + 1;
            fwrite(&len, sizeof(int), 1, the_file);
            fwrite(head[which][ind], sizeof(char), len, the_file);

            len = strlen(date[which][ind]) + 1;
            fwrite(&len, sizeof(int), 1, the_file);
            fwrite(date[which][ind], sizeof(char), len, the_file);

            len = strlen(msgs[which][ind]) + 1;
            fwrite(&len, sizeof(int), 1, the_file);
            fwrite(msgs[which][ind], sizeof(char), len, the_file);
        }
    }
    fclose(the_file);
    return;
}

void
board_load_board(int which)
{
    FILE *the_file;
    int ind, len = 0;
    char fname[256];

    board_reset_board(which);
    sprintf(fname, "data_files/board%d.msg", which + 1);
    the_file = fopen(fname, "rb");
    if (!the_file) {
        gamelog("Can't open message file. Board will be empty.\n\r");
        return;
    }

    fread(&msg_num[which], sizeof(int), 1, the_file);

    if (msg_num[which] < 1 || msg_num[which] > MAX_MSGS || feof(the_file)) {
        gamelog("Board-message file corrupt or nonexistent.\n\r");
        fclose(the_file);
        return;
    }


    for (ind = 0; ind < msg_num[which]; ind++) {
        fread(&len, sizeof(int), 1, the_file);
        auth[which][ind] = (char *) malloc(len + 1);
        if (!auth[which][ind]) {
            gamelog("Malloc for board author failed.\n\r");
            board_reset_board(which);
            fclose(the_file);
            return;
        }
        fread(auth[which][ind], sizeof(char), len, the_file);

        fread(&len, sizeof(int), 1, the_file);
        head[which][ind] = (char *) malloc(len + 1);
        if (!head[which][ind]) {
            gamelog("Malloc for board header failed.\n\r");
            board_reset_board(which);
            fclose(the_file);
            return;
        }
        fread(head[which][ind], sizeof(char), len, the_file);

        fread(&len, sizeof(int), 1, the_file);
        date[which][ind] = (char *) malloc(len + 1);
        if (!date[which][ind]) {
            gamelog("Malloc for board message date failed.\n\r");
            board_reset_board(which);
            fclose(the_file);
            return;
        }
        fread(date[which][ind], sizeof(char), len, the_file);

        fread(&len, sizeof(int), 1, the_file);
        msgs[which][ind] = (char *) malloc(len + 1);
        if (!msgs[which][ind]) {
            gamelog("Malloc for board msg failed..\n\r");
            board_reset_board(which);
            fclose(the_file);
            return;
        }
        fread(msgs[which][ind], sizeof(char), len, the_file);
    }
    fclose(the_file);
    return;
}

void
board_unload_board(int which)
{
    int ind;

    for (ind = 0; ind < msg_num[which]; ind++) {
        free(auth[which][ind]);
        free(head[which][ind]);
        free(date[which][ind]);
        free(msgs[which][ind]);
    }
}

void
board_reset_board(int which)
{
    int ind;
    for (ind = 0; ind < MAX_MSGS; ind++) {
        free(auth[which][ind]);
        free(head[which][ind]);
        free(msgs[which][ind]);
    }
    msg_num[which] = 0;
    return;
}

int
board_display_msg(int which, struct char_data *ch, const char *arg, struct obj_data *obj)
{
    char number[256], buffer[MAX_STRING_LENGTH];
    int msg;

    arg = one_argument(arg, number, sizeof(number));

    if (!char_can_use_board(ch, obj)) {
        send_to_char("You cannot use that board.\n\r", ch);
        return (TRUE);
    }

    if (!*number || !isdigit(*number))
        return (0);
    if (!(msg = atoi(number)))
        return (0);
    if (!msg_num[which]) {
        send_to_char("The board is empty!\n\r", ch);
        return (1);
    }
    if (msg < 1 || msg > msg_num[which]) {
        send_to_char("That message exists only in your imagination.\n\r", ch);
        return (1);
    }

    char *authstr = auth[which][msg - 1];
    if (stricmp(authstr, " ") && (IS_IMMORTAL(ch) || authstr[0] != '^')) {      // Immortal is looking or post is BY an IMMORTAL
        sprintf(buffer, "%sMessage %d: [%s] (%s) %s%s\n\r\n\r%s", VT_BOLDTEX, msg,
                date[which][msg - 1], authstr[0] == '^' ? authstr + 1 : authstr,
                head[which][msg - 1], VT_NORMALT, msgs[which][msg - 1]);
    } else {
        sprintf(buffer, "%sMessage %d: [%s] %s%s\n\r\n\r%s", VT_BOLDTEX, msg, date[which][msg - 1],
                head[which][msg - 1], VT_NORMALT, msgs[which][msg - 1]);
    }

    page_string(ch->desc, buffer, 1);
    return (1);
}

int
board_show_board(int which, struct char_data *ch, const char *arg, struct obj_data *obj)
{
    int i;
    char buf[MAX_STRING_LENGTH];

    /*act("$n studies the board.", TRUE, ch, 0, 0, TO_ROOM); */

    if (!char_can_use_board(ch, obj)) {
        send_to_char("You cannot use that board.\n\r", ch);
        return (TRUE);
    }

    strcpy(buf,
           "This is a bulletin board. See 'help board' for information on using this.\n\r\n\r");
    if (!msg_num[which])
        strcat(buf, "The board is empty.\n\r");
    else {
        sprintf(buf + strlen(buf), "There are %d messages on the board.\n\r", msg_num[which]);
        for (i = 0; i < msg_num[which]; i++) {
            char *authstr = auth[which][i];
            if (stricmp(authstr, " ") && (IS_IMMORTAL(ch) || authstr[0] != '^')) {      // Immortal is looking or post is BY an IMMORTAL
                sprintf(buf + strlen(buf), "%-2d: [%s] %s (%s)\n\r", i + 1, date[which][i],
                        head[which][i], authstr[0] == '^' ? authstr + 1 : authstr);
            } else {            // Old format post by a MORTAL
                sprintf(buf + strlen(buf), "%-2d: [%s] %s\n\r", i + 1, date[which][i],
                        head[which][i]);
            }
        }
    }
    page_string(ch->desc, buf, 1);

    return (1);
}


char *
compile_reply(int which, struct char_data *ch, int msg, int quote)
{
    int c, d;
    char *newmsg, *oldmsg;
    char buf[MAX_STRING_LENGTH];

    oldmsg = msgs[which][msg];

    if (!quote)
        return 0;

    char *authstr = auth[which][msg];
    if (authstr[0] != ' ' && authstr[0] != '^') // Rule out either old or new-style mortal posts
        sprintf(buf, "%s writes: \n\r>", auth[which][msg]);
    else
        sprintf(buf, "Someone writes: \n\r>");
    c = strlen(buf);

    for (d = 0; d < strlen(oldmsg); d++) {
        buf[c++] = oldmsg[d];
        if (oldmsg[d] == '\r')
            buf[c++] = '>';
    }
    buf[c++] = '\n';
    buf[c++] = '\r';
    buf[c++] = '\n';
    buf[c++] = '\r';
    buf[c++] = '\0';

    CREATE(newmsg, char, c);
    strcpy(newmsg, buf);

    return (newmsg);
}

void
free_board(int board)
{
    int i;


    for (i = 0; i < MAX_MSGS; i++) {
        free(msgs[board][i]);
        free(auth[board][i]);
        free(date[board][i]);
        free(head[board][i]);
    }
}

void
uninit_boards(void)
{
    int i;

    for (i = 0; i < MAX_BOARDS; i++)
        if (has_loaded[i]) {
            free_board(i);
            has_loaded[i] = 0;
        }
}

void
init_boards(void)
{
    int i;

    for (i = 0; i < MAX_BOARDS; i++)
        has_loaded[i] = 0;
}

