/*
  * File: SPECIAL.C
  * Usage: Special routines for NPCs and objects.
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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/dir.h>

#include "constants.h"
#include "structs.h"
#include "barter.h"
#include "clan.h"
#include "dmpl.h"
#include "echo.h"
#include "utils.h"
#include "utility.h"
#include "limits.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "guilds.h"
#include "cities.h"
#include "creation.h"
#include "modify.h"
#include "memory.h"
#include "event.h"
#include "spells.h"
#include "psionics.h"
#include "object.h"
#include "info.h"
#include "spice.h"
#include "object_list.h"

#include "special_js.h"

int ready_to_animate(CHAR_DATA *ch);

extern int get_dir_from_name(char *arg);
/*   external vars  */

struct npc_brain_data *npc_brain_list = NULL;

// This is a sample special, copy & paste it to make new ones.  Please do not just hijack
// this one, however.

int
sample_special(struct char_data *ch, int cmd, const char *arg, struct char_data *npc,
               struct room_data *rm, struct obj_data *obj)
{
    return FALSE;               // False does NOT block the command.
}



int
incense_burner(struct char_data *ch, int cmd, const char *arg, struct char_data *npc,
               struct room_data *rm, struct obj_data *burner)
{
    OBJ_DATA *incense = NULL, *next_incense = NULL, *smoke, *tmp_obj = NULL, *next_tmp_obj = NULL;
    char *room_mess = NULL, *smoke_ldesc = NULL, *burner_hour_char = NULL;
    char edesc[256] = "", room_mess2[256] = "";
    int burner_hour_int, is_smoke = 0, hourly_mess = 0;

    if (!burner ||              // Cant' run if not attached to a burner
        (cmd != CMD_PULSE) ||   // Pulse only
        (burner->in_room == NULL) ||    // Won't work in inventories/equipped/etc..
        (burner->contains == NULL) ||   // Has to have incense in it
        (burner->obj_flags.type != ITEM_CONTAINER))     // Containers only
        return FALSE;

    if (!(burner_hour_char = find_ex_description("[BURNER_HOUR]", burner->ex_description, TRUE))) {
        // The -1 value ensures that it will run this time, since the hour is never -1
        set_obj_extra_desc_value(burner, "[BURNER_HOUR]", "-1");
        return FALSE;
    }

    burner_hour_int = atoi(burner_hour_char);

    if (burner_hour_int > 8)
        burner_hour_int = 8;
    if (burner_hour_int < 0)
        burner_hour_int = 0;

    if (burner_hour_int != time_info.hours) {
        hourly_mess = 1;        // Send the messages that only happen once/game hour
        sprintf(edesc, "%d", time_info.hours);
        set_obj_extra_desc_value(burner, "[BURNER_HOUR]", edesc);
    }

    for (incense = burner->contains; incense;) {
        next_incense = incense->next_content;
        if (incense->obj_flags.type == ITEM_TOOL && incense->obj_flags.value[0] == TOOL_INCENSE
            && (smoke_ldesc =
                find_ex_description("[INCENSE_LDESC]", incense->ex_description, TRUE))) {
            for (tmp_obj = burner->in_room->contents; tmp_obj;) {       // Means there's already smoke in the room, don't send messages
                next_tmp_obj = tmp_obj->next_content;
                if ((obj_index[tmp_obj->nr].vnum == 46)
                    && (!stricmp(OSTR(tmp_obj, long_descr), smoke_ldesc))) {
                    is_smoke = 1;
                    extract_obj(tmp_obj);
                    // I remove the smoke here instead of letting the event run out so they
                    // Don't see the "fades away" message the event generates ever 60 seconds
                }
                tmp_obj = next_tmp_obj;
            }


            if ((IS_SET(burner->in_room->room_flags, RFL_INDOORS))) {
                smoke = read_object(46, VIRTUAL);
                smoke->long_descr = (char *) strdup(smoke_ldesc);
                new_event(EVNT_REMOVE_OBJECT, 60, 0, 0, smoke, 0, 0);
                obj_to_room(smoke, burner->in_room);
            }

            if (!is_smoke || hourly_mess) {
                if ((room_mess =
                     find_ex_description("[INCENSE_SMOKE_MSG]", burner->ex_description, TRUE))) {
                    send_to_room(room_mess, burner->in_room);
                    send_to_room("\n\r", burner->in_room);
                    if (!IS_SET(burner->in_room->room_flags, RFL_INDOORS))
                        send_to_room("The smoke quickly drifts away.\n\r", burner->in_room);
                } else {
                    if (IS_SET(burner->in_room->room_flags, RFL_INDOORS))
                        sprintf(room_mess2, "A puff of smoke issues forth from %s.\n\r",
                                OSTR(burner, short_descr));
                    else
                        sprintf(room_mess2,
                                "A puff of smoke issues forth from %s and gets blown away by a light breeze.\n\r",
                                OSTR(burner, short_descr));
                    send_to_room(room_mess2, burner->in_room);
                }
            }
            // This should only be done once/hour
            if (hourly_mess) {
                if (incense->obj_flags.value[1] > 0)
                    incense->obj_flags.value[1] = incense->obj_flags.value[1] - 1;

                if (incense->obj_flags.value[1] < 1)
                    extract_obj(incense);
            }
        }
        incense = next_incense;
    }

    return FALSE;               // False does NOT block the command.

}

bool
has_dmpl_named(struct specials_t *specs, const char *match)
{
    if (!specs)
        return FALSE;

    for (int i = 0; i < specs->count; i++) {
        if (specs->specials[i].type == SPECIAL_DMPL && !strcmp(match, specs->specials[i].special.name))
            return TRUE;
    }

    return FALSE;
}

int
unused_dmpl(struct char_data *ch, int cmd, const char *arg, struct char_data *npc,
            struct room_data *rm, struct obj_data *obj)
{

    char list[MAX_STRING_LENGTH];
    char buf[256], prog_name[256];
    DIR *dp;
    struct room_data *room;
    struct direct *entry;
    int i;
    int found = 0;

    if (!ch->desc)
        return TRUE;

    if (!(dp = opendir("/armag/lib/dmpl"))) {
        send_to_char("Could not find the dmpl directory.\n\r", ch);
        return TRUE;
    }

    *list = '\0';

    while ((entry = readdir(dp))) {
        if ((*entry->d_name != '\0') && (*entry->d_name != '.')) {

            found = 0;

            strcpy(prog_name, entry->d_name);
            for (i = 0; i < top_of_npc_t; i++) {
                if (has_dmpl_named(npc_default[i].programs, prog_name)) {
                    found = 1;
                    break;
                }
            }

            if (found)
                continue;

            for (i = 0; i < top_of_obj_t; i++) {
                if (has_dmpl_named(obj_default[i].programs, prog_name)) {
                    found = 1;
                    break;
                }
            }

            if (found)
                continue;

                for (i = 0; i < 80000; i++) {
                    room = get_room_num(i);
                    if (room && has_dmpl_named(room->specials, prog_name)) {
                        found = 1;
                        break;
                    }
                }

            if (!found) {
                sprintf(buf, "%s\n\r", entry->d_name);
                strcat(list, buf);
            }
        }
    }

    closedir(dp);

    if (*list == '\0') {
        send_to_char("No dmpls found.\n\r", ch);
        return TRUE;
    }

    send_to_char(list, ch);

    return TRUE;                // True DOES block the command.
}

int
armor_info(struct char_data *ch, 
           int cmd, 
           const char *arg, 
           struct char_data *npc,
           struct room_data *rm, 
           struct obj_data *obj) {
  int i, flag_check;
  char tmp[MAX_STRING_LENGTH];
  char line[MAX_STRING_LENGTH];
  char material[MAX_STRING_LENGTH];

  //armor->obj_flags.material == MATERIAL_METAL

  for (i = 0; i < top_of_obj_t; i++) {
    if (obj_default[i].type == ITEM_ARMOR) {
      
      char line_str[MAX_STRING_LENGTH] = "";
      char material_str[MAX_STRING_LENGTH] = "";
      
      for (flag_check = 0; *obj_wear_flag[flag_check].name; flag_check++) {
        if (IS_SET(obj_default[i].wear_flags, obj_wear_flag[flag_check].bit)) {
          sprintf(line, "%s|", obj_wear_flag[flag_check].name);
          string_safe_cat(line_str, line, MAX_STRING_LENGTH);
        }
      }
      
      for (flag_check = 0; *obj_material[flag_check].name; flag_check++) {
        if (obj_default[i].material == obj_material[flag_check].val) {
          sprintf(material, "%s|", obj_material[flag_check].name);
          string_safe_cat(material_str, material, MAX_STRING_LENGTH);
        }
      }

      sprintf(tmp, "%d,%d,%d,%d,%s,%s,\"%s\"",
              obj_index[i].vnum, 
              obj_default[i].value[3], // max_points
              obj_default[i].value[2], // size
              obj_default[i].weight, 
              line_str,
              material_str,
              obj_default[i].short_descr);
      log_to_file(tmp, "armor_info");
    }
  }

  return TRUE;
}

// group_follower.c
// an upgrade to group_follower.dmpl
//
// If separated from their master, npc will travel back to him every tick.
// This should stop groups from getting separated in the long term.
//
// Also prevents groups from getting lumped together by checking for an
// existing leader before checking for another one.
// - Dyrinis

int
group_follower(struct char_data *ch, int cmd, const char *arg, struct char_data *npc,
               struct room_data *rm, struct obj_data *obj)
{
    char command[256];
    char message[MAX_INPUT_LENGTH] = "";
    struct char_data *leader;
    int ex = 0;

    if (npc)
        leader = npc->master;
    else
        return FALSE;

    if (!leader) {
        sprintf(command, "follow group_leader");
        parse_command(npc, command);
        return FALSE;
    }

    if (cmd == CMD_PULSE && ready_to_animate(npc)
     && npc->in_room != leader->in_room) {
        send_to_char("You must find your master before it is too late!", npc);
        ex = choose_exit_name_for(npc->in_room, leader->in_room, message, 100, npc);
        if (ex != -1)
            parse_command(npc, message);

        return FALSE;
    }

    return FALSE;
}

int
aggressive_switch(struct char_data *ch, int cmd, const char *arg, struct char_data *npc,
                  struct room_data *rm, struct obj_data *obj)
{
    struct char_data *sold;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char command[100];

    arg = one_argument(arg, arg1, sizeof(arg1));
    arg = one_argument(arg, arg2, sizeof(arg2));


    if (!*arg1) {
        gamelog("no arg1");
        return FALSE;
    }
    if (!*arg2) {
        gamelog("no arg2");
        return TRUE;
    }

    if (!(cmd == CMD_ASK))
        return FALSE;

    if ((stricmp(arg2, "angry") != 0) && (stricmp(arg2, "calm") != 0))
        return FALSE;

    sold = get_char_vis(ch, arg1);
    if (sold == 0)
        return FALSE;
    if (sold != npc)
        return FALSE;

    if (ch->player.tribe != npc->player.tribe) {
        strcpy(command, "say I won't follow your orders.");
        parse_command(npc, command);
        return TRUE;
    }

    act("$n gives $N an order.", TRUE, ch, 0, sold, TO_ROOM);
    act("You give $N an order.", TRUE, ch, 0, sold, TO_CHAR);
    act("$n gives you an order.", TRUE, ch, 0, sold, TO_VICT);

    if (strcmp(arg2, "angry") == 0) {
        //af.type = SPELL_NONE;
        //af.duration = -1;
        //af.power = 0;


        //af.magick_type = 0;
        //af.modifier = CFL_AGGRESSIVE;
        //af.location = CHAR_APPLY_CFLAGS;
        //af.bitvector = 0;
        //affect_to_char (npc, &af);
        MUD_SET_BIT(npc->specials.act, CFL_AGGRESSIVE);
        sprintf(command, "emote looks aggressive.");
        parse_command(npc, command);
        return FALSE;
    }
    if (strcmp(arg2, "calm") == 0) {
        REMOVE_BIT(npc->specials.act, CFL_AGGRESSIVE);
        sprintf(command, "emote looks peaceful.");
        parse_command(npc, command);
        return FALSE;
    }

    return FALSE;
};


int
alpha_list(struct char_data *ch, int cmd, const char *arg, struct char_data *npc,
           struct room_data *rm, struct obj_data *obj)
{

    //    struct a_list_struct
    //  {
    //    char name[256];
    //    struct a_list_struct *next;
    //  };
    //
    //  struct obj_list_type *new  = NULL;
    //  struct obj_list_type *alist  = NULL;
    //  struct obj_list_type *tmp  = NULL;
    char *temp_desc;

    //  alist = (struct obj_list_type *)malloc (sizeof *alist);

    temp_desc = find_ex_description("[DOOKIE]", ch->ex_description, TRUE);

    cprintf(ch, "%d", strlen(temp_desc));
    return TRUE;
}


int
test_act(struct char_data *ch, int cmd, const char *arg, struct char_data *npc,
         struct room_data *rm, struct obj_data *obj)
{
    if (cmd == CMD_SMILE) {
        act("TO_CHAR message.", TRUE, ch, 0, npc, TO_CHAR);
        act("TO_VICT message.", TRUE, ch, 0, npc, TO_VICT);
        act("TO_ROOM message.", TRUE, ch, 0, npc, TO_ROOM);
        act("TO_NOTVICT message.", TRUE, ch, 0, npc, TO_NOTVICT);
    }

    return TRUE;
}


int
find_tools(struct char_data *ch, int icmd, const char *arg, struct char_data *npc,
           struct room_data *rm, struct obj_data *obj)
{
    /*  struct obj_data *tmp_obj; */
    char tmpstr[100];
    int i;

    if (icmd != CMD_KICK)
        return FALSE;

    send_to_char("BEGIN : TREASURE - JEWELRY", ch);
    for (i = 0; i < top_of_obj_t; i++)
        if ((obj_default[i].type == ITEM_TREASURE) && (obj_default[i].value[0] == TREASURE_JEWELRY)) {
            //    sprintf (tmpstr, "[%d] %s\n\r",
            //             obj_index[i].vnum,
            //             obj_default[i].short_descr);
            sprintf(tmpstr, "ocreate %d\n\r", obj_index[i].vnum);

            send_to_char(tmpstr, ch);
        }
    send_to_char("END : TREASURE - JEWELRY", ch);

    return FALSE;
}


/* Started January, 2001 by Nessalin */
int
tattoo_parlor(struct char_data *ch, int cmd, const char *arg, struct char_data *npc,
              struct room_data *rm, struct obj_data *obj)
{
    char arg1[256], buy_loc[256], lookfor[256];
    char cost_str[256] = "", sdesc_str[256] = "", room_mess[256] = "";
    char list_string[256] = "", ldesc_str[MAX_STRING_LENGTH];
    char *message = 0;
    int buy_num, found = FALSE, i, counter, j, cost_num, k;

    if (!(cmd == CMD_LIST || cmd == CMD_BUY || cmd == CMD_VIEW))
        return FALSE;

    if (cmd == CMD_LIST) {
        message = find_ex_description("[TATTOO_PROG_1]", npc->ex_description, TRUE);

        if (!message)
            return FALSE;

        cprintf(ch, "You may buy the following tattoos:\n\r");

        for (i = 1; message; i++) {
            sprintf(lookfor, "[TATTOO_PROG_%d]", i);

            message = find_ex_description(lookfor, npc->ex_description, TRUE);

            if (message) {
                bzero(cost_str, sizeof(cost_str));

                for (j = 0; ((message[j] >= '0') && (message[j] <= '9')); j++)
                    cost_str[j] = message[j];

                cost_num = atoi(cost_str);

                bzero(sdesc_str, sizeof(sdesc_str));

                while ((message[j] == '\n') || (message[j] == '\r') || (message[j] == '\0'))
                    j++;

                for (k = 0; ((message[j] != '\n') && (message[j] != '\r')); j++) {
                    sdesc_str[k] = message[j];
                    k++;
                }

                sprintf(list_string, "%d) %s for %d obsidian coins.\n\r", i, sdesc_str, cost_num);

                send_to_char(list_string, ch);
            }
        }
        return TRUE;
    }

    if (cmd == CMD_VIEW) {
        arg = one_argument(arg, arg1, sizeof(arg1));

        if (*arg1 == '#')
            buy_num = atoi(arg1 + 1);
        else
            buy_num = atoi(arg1);

        sprintf(lookfor, "[TATTOO_PROG_%d]", buy_num);

        message = find_ex_description(lookfor, npc->ex_description, TRUE);

        if ((buy_num < 1) || (!message)) {
            send_to_char("That is not a valid number.\n\r", ch);
            return TRUE;
        }

        bzero(cost_str, sizeof(cost_str));
        for (j = 0; ((message[j] >= '0') && (message[j] <= '9')); j++)
            cost_str[j] = message[j];

        bzero(sdesc_str, sizeof(sdesc_str));
        j++;

        for (k = 0; ((message[j] != '\n')); j++, k++) {
            sdesc_str[k] = message[j];
        }

        bzero(ldesc_str, sizeof(ldesc_str));
        j++;

        for (k = 0; message[j] != '\0'; j++, k++) {
            ldesc_str[k] = message[j];
        }
        cprintf(ch, "%s", ldesc_str);

        return TRUE;
    }

    if (cmd == CMD_BUY) {
        arg = one_argument(arg, arg1, sizeof(arg1));
        sprintf(buy_loc, "%s", arg);

        if (*arg1 == '#')
            buy_num = atoi(arg1 + 1);
        else
            buy_num = atoi(arg1);

        sprintf(lookfor, "[TATTOO_PROG_%d]", buy_num);

        message = find_ex_description(lookfor, npc->ex_description, TRUE);

        if ((buy_num < 1) || (!message)) {
            send_to_char("That is not a valid number.\n\r", ch);
            return TRUE;
        }

        bzero(cost_str, sizeof(cost_str));
        for (j = 0; ((message[j] >= '0') && (message[j] <= '9')); j++)
            cost_str[j] = message[j];

        cost_num = atoi(cost_str);

        while ((message[j] == '\n') || (message[j] == '\r') || (message[j] == '\0'))
            j++;

        bzero(sdesc_str, sizeof(sdesc_str));
        for (k = 0; ((message[j] != '\n')); j++, k++) {
            sdesc_str[k] = message[j];
        }

        while ((message[j] == '\n') || (message[j] == '\r') || (message[j] == '\0'))
            j++;

        if (ch->points.obsidian < cost_num) {
            cprintf(ch, "You do not have the %d coins %s costs.\n\r", cost_num, sdesc_str);
            return TRUE;
        }

        bzero(ldesc_str, sizeof(ldesc_str));

        for (k = 0; message[j] != '\0'; j++, k++) {
            ldesc_str[k] = message[j];
        }

        if (!*buy_loc) {
            send_to_char
                ("Where would you like the tattoo?\n\r(Syntax:buy N Location)\n\r(Ex. buy 4 head)\n\r",
                 ch);
            return TRUE;
        }

        for (counter = 0; counter < MAX_WEAR_EDESC; counter++) {
            if (!strcmp(buy_loc, wear_edesc[counter].name)) {
                found = TRUE;
                set_char_extra_desc_value(ch, wear_edesc[counter].edesc_name, sdesc_str);
                set_char_extra_desc_value(ch, wear_edesc_long[counter].edesc_name, ldesc_str);
            }
        }

        if (!found) {
            send_to_char("That's not a place you can get a tattoo or scar.\n\r", ch);
            for (counter = 0; counter < MAX_WEAR_EDESC; counter++) {
                send_to_char(wear_edesc[counter].name, ch);
                send_to_char("\n\r", ch);
            }
            return TRUE;
        }


        cprintf(ch, "You are charged %d obsidian coins.\n\r", cost_num);
        ch->points.obsidian -= cost_num;
        cprintf(ch, "You receive %s on your %s.\n\r", sdesc_str, buy_loc);
        sprintf(room_mess, "$N uses a thin bone needle and dye to create %s on $n's skin.",
                sdesc_str);
        act(room_mess, TRUE, ch, 0, npc, TO_ROOM);

        return TRUE;
    }

    return FALSE;
}


/* Written by Nessalin on 1/13/2001 to keep people in send shadow
   from being retards. */

int
no_talk(struct char_data *ch, int cmd, const char *arg, struct char_data *npc, struct room_data *rm,
        struct obj_data *obj)
{
    if (!((cmd == CMD_SHOUT) || (cmd == CMD_TELL) || (cmd == CMD_ASK) || (cmd == CMD_TALK)
          || (cmd == CMD_SAY) || (cmd == CMD_WHISPER) || (cmd == CMD_EMOTE) || (cmd == CMD_PEMOTE)))

        return FALSE;

    if (ch != npc)
        return FALSE;

    send_to_char("You left your physical form behind and can't interact with the physical world.\n\r", ch);

    if ((cmd == CMD_EMOTE) || (cmd == CMD_PEMOTE))
        send_to_char("Use semote, hemote, psemote, or phemote instead.\n\r", ch);

    return TRUE;
}




/* Started on 5/31/99 by Nessalin
   By setting the edesc [talk_subject] the NPC will respond if someone
   types 'talk <npc> <subject>' */
int
npc_talk(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
         OBJ_DATA * obj)
{
    char *message = 0;
    char logmsg[256];
    char tmp[MAX_INPUT_LENGTH] = "";
    char tmp2[MAX_INPUT_LENGTH] = "";
    char topic[MAX_STRING_LENGTH] = "";
    char arg1[MAX_INPUT_LENGTH] = "";
    char arg2[MAX_INPUT_LENGTH] = "";
    int go_on = 1;
    int counter;
    //  int i;


    if ((cmd != CMD_TALK) || (!npc))
        return FALSE;

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));

    if ((!*arg1) || (!*arg2))
        return FALSE;

    if (!(npc == get_char_room_vis(ch, arg1)))
        return FALSE;

    sprintf(topic, "[talk_%s]", arg2);

    message = find_ex_description(topic, npc->ex_description, TRUE);

    if (!message)
        return FALSE;

    sprintf(tmp2, "You begin talking about %s.\n\r", arg2);

    send_to_char(tmp2, ch);

    strcat(tmp, "say ");

    if (strlen(message) > 256) {
        strncat(tmp, message, 256);
        sprintf(logmsg, "Warning: String too long in edesc '%s' on NPC#%d.", arg2,
                npc_index[npc->nr].vnum);
        gamelog(logmsg);
    } else
        strcat(tmp, message);

    parse_command(npc, tmp);

    counter = 1;

    while (go_on) {
        counter++;

        sprintf(topic, "[talk_%s_%d]", arg2, counter);

        message = find_ex_description(topic, npc->ex_description, TRUE);

        /* sprintf(tmp, ""); */
        tmp[0] = '\0';

        if (!message)
            go_on = 0;
        else {
            strcat(tmp, "say ");
            if (strlen(message) > 256) {
                strncat(tmp, message, 256);
                sprintf(logmsg, "Warning: String too long in edesc '%s' on NPC#%d.", arg2,
                        npc_index[ch->nr].vnum);
                gamelog(logmsg);
            } else
                strcat(tmp, message);
            parse_command(npc, tmp);

        }
    }

    return TRUE;
}

/* Started on 5/31/99 by Nessalin
   Adapted for clan-only stuff by Tiernan, 9/20/00
   By setting the edesc [talk_subject] the NPC will respond if someone
   types 'talk <npc> <subject>' */
int
npc_talk_clan(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
              OBJ_DATA * obj)
{
    if (ch && npc && IS_IN_SAME_TRIBE(ch, npc))
        return npc_talk(ch, cmd, argument, npc, rm, obj);

    return FALSE;
}


/* Started on 5/30/99 by Nessalin
   This program is for roof tops, if someone performs a given action in
   the room it will look for the room below it (stored in an edesc) and
   then send a message below */
int
rooftop(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
        OBJ_DATA * obj)
{
    int chance_noise = 0;       /* chance they'll make noise */
    ROOM_DATA *down_room;
    char noise_loud[MAX_STRING_LENGTH];
    char type_noise[MAX_STRING_LENGTH];
    char message[MAX_STRING_LENGTH];


    if (!((cmd == CMD_EAST) || (cmd == CMD_WEST) || (cmd == CMD_NORTH) || (cmd == CMD_SOUTH)
          || (cmd == CMD_UP) || (cmd == CMD_DOWN) || (cmd == CMD_LAND)))
        return FALSE;

    /* If it's not a person, abort */
    if (!ch)
        return FALSE;

    /* If they're not touching the roof, they're not making noise on it */
    if ((is_flying_char(ch) || (is_char_ethereal(ch))))
        return FALSE;

    /* exit if there's no down room to send mesages to */
    if (!(ch->in_room->direction[DIR_DOWN])) {
        gamelogf("Error in Rooftop() in Special.c: No down exit in room #%d.",
			ch->in_room->number);
        return FALSE;
    }

    if (!(down_room = ch->in_room->direction[DIR_DOWN]->to_room))
        return FALSE;

    if (cmd == CMD_LAND) {
        send_to_room("You hear a loud thump on the roof.\n\r", down_room);
        return FALSE;
    }

    /* begin n, s, e, w, u, and d */

    switch (GET_SPEED(ch)) {
    case SPEED_SNEAKING:
        strcpy(type_noise, "creaking");
        chance_noise += 15;
        break;
    case SPEED_WALKING:
        strcpy(type_noise, "thumping");
        chance_noise += 45;
        break;
    case SPEED_RUNNING:
        strcpy(type_noise, "pounding");
        chance_noise += 65;
        break;
    default:
        break;
    }

    chance_noise += ch->player.weight + calc_carried_weight(ch);

    /* 4 for a halfling
     * 8 for an elf
     * 9 for a dwarf
     * 80 for a half-giant
     * (on average) */

    if ((ch->player.weight + calc_carried_weight(ch)) < 4)
        strcpy(noise_loud, "barely audible");
    else if (ch->player.weight + calc_carried_weight(ch) < 10)
        strcpy(noise_loud, "light");
    else if (ch->player.weight + calc_carried_weight(ch) < 20)
        strcpy(noise_loud, "noisy");
    else if (ch->player.weight + calc_carried_weight(ch) > 20)
        strcpy(noise_loud, "thunderous");

    /* they didn't make noise */
    if (number(1, 100) > chance_noise)
        return FALSE;
    sprintf(message, "You hear a %s %s noise from above.\n\r", noise_loud, type_noise);

    send_to_room(message, down_room);

    return FALSE;

}

int
smelling_salts(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
               OBJ_DATA * obj)
{
    CHAR_DATA *victim;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if (cmd != CMD_USE && cmd != CMD_WAVE && cmd != CMD_PUT && cmd != CMD_LOOK && cmd != CMD_GET && cmd != CMD_SLIP
        && cmd != CMD_SNIFF)
        return FALSE;

    if (obj == NULL)
        return FALSE;

    two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    if ((cmd == CMD_PUT || cmd == CMD_SLIP) && isname(arg2, OSTR(obj, name))) {
        if (IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE)
            && IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) {
            act("$p is closed.", FALSE, ch, obj, NULL, TO_CHAR);
            return TRUE;
        }

        send_to_char("It's too small to put anything in.\n\r", ch);
        return TRUE;
    } else if (cmd == CMD_PUT)
        return FALSE;

    if (cmd == CMD_LOOK && !strncasecmp(arg1, "in", strlen(arg1))
        && isname(arg2, OSTR(obj, name))) {
        if (IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE)
            && IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) {
            act("$p is closed.", FALSE, ch, obj, NULL, TO_CHAR);
            return TRUE;
        }

        sprintf(buf, "In %s (%s) :\n\rsome odorous contents\n\r", format_obj_to_char(obj, ch, 1),
                ch->equipment[ES] == obj ? "held" : "carried");
        send_to_char(buf, ch);
        return TRUE;
    } else if (cmd == CMD_LOOK)
        return FALSE;

    if (cmd == CMD_GET && isname(arg2, OSTR(obj, name))) {
        if (IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE)
            && IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) {
            act("$p is closed.", FALSE, ch, obj, NULL, TO_CHAR);
            return TRUE;
        }

        send_to_char("You can't get anything from it.\n\r", ch);
        return TRUE;
    } else if (cmd == CMD_GET)
        return FALSE;

    if (!isname(arg1, OSTR(obj, name)))
        return FALSE;

    if (cmd == CMD_SNIFF) {
        if (IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE)
            && IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) {
            act("$p is closed.", FALSE, ch, obj, NULL, TO_CHAR);
            return TRUE;
        }

        if (obj->obj_flags.value[0]) {
            send_to_char("A pungent odor assaults your senses and you whip your"
                         " head back in response.\n\r", ch);
            act("$n sniffs at $p, then whips $s head back in response.", TRUE, ch, obj, NULL,
                TO_ROOM);
        } else {
            send_to_char("You sniff it, but smell nothing.\n\r", ch);
            act("$n sniffs at $p.", TRUE, ch, obj, NULL, TO_ROOM);
        }
        return TRUE;
    } else if (cmd == CMD_SNIFF)
        return FALSE;

    if (ch->equipment[ES] != obj) {
        send_to_char("You have to hold it to make it useful.\n\r", ch);
        return TRUE;
    }

    if ((victim = get_char_room_vis(ch, arg2)) == NULL) {
        send_to_char("You don't see anyone here like that.\n\r", ch);
        return TRUE;
    }

    if (IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE)
        && IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) {
        act("$p is closed.", FALSE, ch, obj, NULL, TO_CHAR);
        return TRUE;
    }

    if (GET_POS(victim) < POSITION_SLEEPING
        || (GET_POS(victim) == POSITION_SLEEPING && affected_by_spell(victim, SPELL_SLEEP)
            && !knocked_out(victim))) {
        act("You wave $p under $N's nose, but nothing seems to happen.", FALSE, ch, obj, victim,
            TO_CHAR);
        act("$n waves $p under $N's nose.", TRUE, ch, obj, victim, TO_ROOM);

        /* make it emptiable, so people won't keep using it forever */
        if (!obj->obj_flags.value[0])
            return TRUE;

        obj->obj_flags.value[0]--;
        return TRUE;
    } else if (GET_POS(victim) > POSITION_SLEEPING) {
        send_to_char("They already appear to be awake.\n\r", ch);
        return TRUE;
    } else {
        struct affected_type *af;

        act("You wave $p under $N's nose.", FALSE, ch, obj, victim, TO_CHAR);
        act("$n waves $p under $N's nose.", TRUE, ch, obj, victim, TO_ROOM);

        /* make it emptiable, so people won't keep using it forever */
        if (!obj->obj_flags.value[0])
            return TRUE;

        obj->obj_flags.value[0]--;

        send_to_char("A pungent odor fills your senses.\n\r", victim);
        act("$N chokes and wheezes slowly shaking $S head.", TRUE, ch, obj, victim, TO_ROOM);
        act("$N chokes and wheezes slowly shaking $S head.", TRUE, ch, obj, victim, TO_CHAR);

        if (knocked_out(victim)) {
            for (af = victim->affected; af; af = af->next) {
                if (af->type == SPELL_SLEEP && af->magick_type == MAGICK_NONE) {
                    if ((af->expiration -= RT_ZAL_HOUR) > time(NULL)) {
                        return TRUE;
                    } else {    /* use affect_remove so you don't swat magickal sleep */

                        affect_remove(victim, af);
                        set_stun(victim, 1);
                        set_char_position(victim, POSITION_RESTING);
                        break;
                    }
                }
            }
        } else if (GET_POS(victim) == POSITION_SLEEPING) {      /* wake sleepers */
            set_char_position(victim, POSITION_RESTING);
        }

        /* use update_pos so that hps wounds will keep them down */
        update_pos(victim);

        /* if they're not stunned or worse, go ahead and put them to resting */
        if (GET_POS(victim) > POSITION_SLEEPING)
            set_char_position(victim, POSITION_RESTING);

        return TRUE;
    }
}
void list_char_to_char(CHAR_DATA * list, CHAR_DATA * ch, int mode);

int
nilaz_chest_prog(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                 OBJ_DATA * obj)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char ename[MAX_STRING_LENGTH], hole_edesc[MAX_STRING_LENGTH];
    char chest_edesc[MAX_STRING_LENGTH];
    int chest_found = 0;
    OBJ_DATA *hole, *chest = NULL, *temp, *next_obj;
    ROOM_DATA *storage_room;

    if (cmd != CMD_GET && cmd != CMD_TAKE && cmd != CMD_PUT && cmd != CMD_LOOK && cmd != CMD_CLOSE)
        return FALSE;

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));

    /* Cmd_close only needs one argument, so people were having to type:
     * "close hole black" to make it work. -ness 8-18-1999 */
    if (cmd == CMD_CLOSE) {
        if (!(hole = get_obj_in_list_vis(ch, arg1, ch->in_room->contents)))
            return FALSE;
    } else {
        if (!(hole = get_obj_in_list_vis(ch, arg2, ch->in_room->contents)))
            return FALSE;
    }

    if (hole != obj)
        return FALSE;


    /* Find who's portable hole it is */
    strcpy(ename, "[Storage_name]");
    if (!get_obj_extra_desc_value(hole, ename, hole_edesc, MAX_STRING_LENGTH)) {
        errorlog("Portable hole object has no player name associated with it.");
        return FALSE;
    }

    /* Search for chest object in nilaz */
    if (!(storage_room = get_room_num(73999))) {
        gamelog("Error in nilaz_chest_prog special: Missing room 73999.");
        return FALSE;
    }

    for (temp = storage_room->contents; temp; temp = next_obj) {
        next_obj = temp->next;

        if (obj_index[temp->nr].vnum == 34502) {
            /* Loop to find chest(temp) with edesc of hole */
            if (get_obj_extra_desc_value(temp, ename, chest_edesc, MAX_STRING_LENGTH)) {
                if (STRINGS_SAME(chest_edesc, hole_edesc)) {
                    chest_found = 1;
                    chest = temp;
                    break;
                }
            }                   /* End if the objects edesc is same as on hole */
        }                       /* End if chest object is found in the room */
    }                           /* End for loop */

    if (!chest_found || !chest) {
        errorlogf("Portable hole without an owner, room #%d.", ch->in_room->number);
        return FALSE;
    }

    if (cmd == CMD_LOOK) {
        if (!STRINGS_SAME(arg1, "in"))
            return FALSE;
        act("You peer into the $p, and see:", FALSE, ch, hole, 0, TO_CHAR);
        sprintf(buf, "%s\n\r", list_obj_to_char(temp->contains, ch, 2, TRUE));
        send_to_char(buf, ch);

        return TRUE;
    }

    if (cmd == CMD_PUT) {
        if (!(temp = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
            send_to_char("You neither see nor have that item to put" " into.\n\r", ch);
            return TRUE;
        }
        act("$n places $p inside $P, and it vanishes.", TRUE, ch, temp, hole, TO_ROOM);
        act("You place $p inside $P, and it vanishes.", FALSE, ch, temp, hole, TO_CHAR);
        obj_from_char(temp);
        obj_to_obj(temp, chest);
        return TRUE;
    }

    if (cmd == CMD_CLOSE) {
        sprintf(buf, "%s", MSTR(ch, name));
        if (strcmp(hole_edesc, buf)) {
            /* only the owner can close it */
            send_to_char("You try and approach it, but it stays constantly out "
                         "of your grasp.\n\r", ch);
        } else {
            act("$n waves $s hand over $p, causing it to disappear.", FALSE, ch, hole, NULL,
                TO_NOTVICT);
            act("You wave your hand over $p, closing it.", FALSE, ch, hole, NULL, TO_CHAR);
            extract_obj(hole);
        }
        return TRUE;
    }

    /* cmd == CMD_TAKE || cmd == CMD_GET */
    if ((cmd == CMD_GET) || (cmd == CMD_TAKE)) {
        if (!(temp = get_obj_in_list_vis(ch, arg1, chest->contains))) {
            sprintf(buf, "The $p does not contain '%s.'\n\r", arg1);
            act(buf, TRUE, ch, hole, 0, TO_CHAR);
            return TRUE;
        }

        if ( !is_merchant(ch) && !IS_IMMORTAL(ch)) {
            if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
                act("You cannot carry $p, you have too many items.", FALSE, ch, 
                 temp, 0, TO_CHAR);
                return FALSE;
            }

            if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(temp) > CAN_CARRY_W(ch)) {
                act("You cannot carry $p, it is too heavy..", FALSE, ch, 
                 temp, 0, TO_CHAR);
                return FALSE;
            }
        }

        

        act("$n reaches inside the $p and pulls out $P.", TRUE, ch, hole, temp, TO_ROOM);
        act("You reach inside the $p and pull out $P.", FALSE, ch, hole, temp, TO_CHAR);
        obj_from_obj(temp);
        obj_to_char(temp, ch);
        return TRUE;
    }

    /* Dunno how they got down to here */
    errorlog("in special program nilaz_chest_prog: Hit end of function.");
    return FALSE;
}

OBJ_DATA *find_corpse_in_room_from_edesc(ROOM_DATA * rm, char *name, char *ainfo);
int
no_remove_item(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
               OBJ_DATA * obj)
{
    return FALSE;
}

int
random_room_messanger(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc,
                      ROOM_DATA * rm, OBJ_DATA * obj)
{
    CHAR_DATA *person = (CHAR_DATA *) 0;

    if (cmd != CMD_PULSE)
        return FALSE;

    if (!rm)
        return FALSE;

    if (rm->people)
        for (person = rm->people; person; person = person->next_in_room)
            act("Random message.", TRUE, person, 0, 0, TO_CHAR);

    return FALSE;
}

int
obj_dir_block(CHAR_DATA * ch,   /* Char trying to move    */
              int cmd,          /* Command used to move   */
              const char *argument,     /* Anything after command */
              CHAR_DATA * npc,  /* Not used               */
              ROOM_DATA * rm,   /* Not used               */
              OBJ_DATA * obj)
{                               /* Object doing blocking  */
    char buf1[256];             /* for sending messages        */

    cmd = cmd - 1;

    //    if (!(affected_by_spell(ch, SPELL_ETHEREAL))) {
    if ((!(is_char_ethereal(ch))) && (!(IS_IMMORTAL(ch)))) {
        switch (number(0, 1)) {
        case 0:
            /* Send message to room */
            sprintf(buf1, "$p blocks $N from going through the %s exit.", dirs[cmd]);
            act(buf1, TRUE, ch, obj, ch, TO_ROOM);

            /* Send message to mover */
            sprintf(buf1, "$p blocks you from going through the %s exit.", dirs[cmd]);
            act(buf1, TRUE, ch, obj, 0, TO_CHAR);
            break;
        case 1:
            /* Send message to room */
            sprintf(buf1, "$N tries to go %s but is blocked by $p.", dirs[cmd]);
            act(buf1, TRUE, ch, obj, ch, TO_ROOM);

            /* Send message to mover */
            sprintf(buf1, "You try to go %s but are blocked by $p.", dirs[cmd]);
            act(buf1, TRUE, ch, obj, 0, TO_CHAR);
            break;
        default:
            break;
        }
        /* Buh-buh now */
        return TRUE;
    }

    /* Buh-buh now */
    return FALSE;
}

int
mob_dir_block(CHAR_DATA * ch,   /* Char trying to move    */
              int cmd,          /* Command used to move   */
              const char *argument,     /* Anything after command */
              CHAR_DATA * npc,  /* Not used               */
              ROOM_DATA * rm,   /* Not used               */
              OBJ_DATA * obj)
{                               /* Object doing blocking  */
    char buf1[256];             /* for sending messages        */
    cmd = cmd - 1;

    if (!(affected_by_spell(ch, SPELL_ETHEREAL)))
        if (!(npc == ch)) {
            /* Send message to room */
            sprintf(buf1, "$n blocks $N from going through the %s exit.", dirs[cmd]);
            act(buf1, TRUE, npc, 0, ch, TO_ROOM);

            /* Send message to mover */
            sprintf(buf1, "$n blocks you from going through the %s exit.", dirs[cmd]);
            act(buf1, TRUE, npc, 0, ch, TO_CHAR);

            /* Buh-buh now */
            return TRUE;
        }

    /* Buh-buh now */
    return FALSE;
}

int
elemental_death(CHAR_DATA * ch, /* Char trying to move    */
                int cmd,        /* Command used to move   */
                const char *argument,   /* Anything after command */
                CHAR_DATA * npc,        /* NPC doing the blocking */
                ROOM_DATA * rm, /* Not used               */
                OBJ_DATA * obj)
{                               /* Not used               */
    if (cmd == CMD_DIE) {
        adjust_hit(npc, 20);
        update_pos(npc);
        parse_command(npc, "pemote eyes flitter open as it rises up from the ground.");
        parse_command(npc, "emote mutters, 'Krek Un Dorak Leeif Brych!'");
        parse_command(npc, "emote disappears in a cloud of billowing dust.");
        parse_command(npc, "wish all I died! returning to home base.");
        char_from_room(npc);
        char_to_room(npc, get_room_num(1000));
        return FALSE;
    }
    return FALSE;
}


int
component_special(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                  OBJ_DATA * obj)
{
    if (obj && (obj->obj_flags.type == ITEM_COMPONENT))
        return (TRUE);
    else
        return (FALSE);
}

int
merchant_helpers(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                 OBJ_DATA * obj)
{
    ROOM_DATA *home_room;
    CHAR_DATA *target_ch;
    char message[MAX_INPUT_LENGTH];
    char buf1[256], buf2[256], buf3[256];
    int ex;
    static int FIRST_RUN = TRUE;

    if (FIRST_RUN) {
        strcpy(message, "change language siri");
        parse_command(npc, message);

        FIRST_RUN = FALSE;
    }

    home_room = get_room_num(0);        /* where helpers return to */

    if (!((cmd == CMD_PULSE) || (cmd == CMD_ASK)))
        return FALSE;

    if (cmd == CMD_PULSE) {     /* pulse actions */
        if( !ready_to_animate(npc) ) return FALSE;
        /* If the person they're following leaves, just start walking home */
        if (npc->master)
            if (npc->master->in_room != npc->in_room) {
                strcpy(message, "follow me");
                parse_command(npc, message);
            }

        if (npc->specials.guarding)
            if (npc->specials.guarding != npc)
                if (npc->specials.guarding->in_room != npc->in_room) {
                    strcpy(message, "guard self");
                    parse_command(npc, message);
                }

        if ((home_room != npc->in_room) &&      /* not in homeroom       */
            (npc->specials.guarding == npc) &&  /* not guarding anyone   */
            (npc->specials.dir_guarding > -1) &&        /* not guarding an exit  */
            (!(npc->specials.obj_guarding)) &&  /* not guarding anything */
            (!(npc->master))) { /* not following someone */
            /* take two steps towards the camp */
            ex = choose_exit_name_for(npc->in_room, home_room, message, 100, npc);
            if (ex != -1)
                parse_command(npc, message);
            if (home_room != npc->in_room) {
                ex = choose_exit_name_for(npc->in_room, home_room, message, 100, npc);
                if (ex != -1)
                    parse_command(npc, message);
            }
            return TRUE;
        }

        if (!number(0, 29)) {
            strcpy(message, "scan");
            parse_command(npc, message);
        }

        return TRUE;
    }
    /* end pulse actions */
    if (cmd == CMD_ASK) {       /* cmd_ask */
        if (!IS_IN_SAME_TRIBE(ch, npc))
            return FALSE;

        argument = one_argument(argument, buf1, sizeof(buf1));
        argument = one_argument(argument, buf2, sizeof(buf2));
        argument = one_argument(argument, buf3, sizeof(buf3));

        if ((!*buf1) || (!*buf2))
            return FALSE;

        if (!(target_ch = get_char_room_vis(ch, buf1)))
            return FALSE;

        if (!(target_ch == npc))
            return FALSE;

        if (!(strcmp(buf2, "follow"))) {
            if (strlen(buf3) < 1)
                strcat(message, "say Follow who?");
            else {
                if (!(strcmp(buf3, "me")))
                    strcpy(buf3, MSTR(ch, name));

                strcpy(message, "follow ");
                strcat(message, buf3);
            }
            parse_command(npc, message);
            return TRUE;
        } else if (!(strcmp(buf2, "get"))) {
            strcpy(message, "get ");
            strcat(message, buf3);
            parse_command(npc, message);
            return TRUE;
        } else if (!(strcmp(buf2, "drop"))) {
            strcpy(message, "drop ");
            strcat(message, buf3);
            parse_command(npc, message);
            return TRUE;
        } else if (!(strcmp(buf2, "guard"))) {
            if (strlen(buf3) < 1)
                strcat(message, "say Guard who?");
            else {
                if (!(strcmp(buf3, "me")))
                    strcpy(buf3, MSTR(ch, name));

                strcpy(message, "Guard ");
                strcat(message, buf3);
            }
            parse_command(npc, message);
            return TRUE;
        }
        return TRUE;
    }

    /* cmd_ask */
    /* exit the program */
    return FALSE;
}

#define USE_KURAC_SPICE_MERCHANT
#ifdef USE_KURAC_SPICE_MERCHANT

int spice_power_levels[5] = { 1, 3, 12, 36, 144 };
int spice_num_to_next[4] = { 3, 4, 3, 4 };
int next_spice_level_vnum[4] = { 20200, 64160, 64170, 64180 };

/* code to get the kurac spice merchant to make knots from pinches */
int
kurac_spice_merchant(CHAR_DATA * pc, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                     OBJ_DATA * obj)
{
    OBJ_DATA *spice;
    int types[MAX_SPICE - MIN_SPICE + 1][5];
    int type, power, vnum, found;
    char buf[MAX_STRING_LENGTH];

    if (cmd != CMD_PULSE || !ready_to_animate(npc))
        return FALSE;

    if (!npc)
        return FALSE;

    if (!npc->in_room)
        return FALSE;

    if (number(0, 19))          /* 1 in 20 (5%) chance of it doing it on any pulse */
        return FALSE;

    for (type = 0; type < sizeof(types)/sizeof(types[0]); type++)
        for (power = 0; power < 5; power++)
            types[type][power] = 0;

    /* set up that we haven't found spice yet */
    found = FALSE;

    for (spice = npc->carrying; spice != NULL; spice = spice->next_content) {
        if (spice->obj_flags.type == ITEM_SPICE) {
            for (power = 0; power < 5; power++)
                if (spice_power_levels[power] == spice->obj_flags.value[0])
                    break;

            if (power == 5)     /* invalid power level */
                continue;

            type = spice->obj_flags.value[1] - MIN_SPICE;

            assert(type < sizeof(types)/sizeof(types[0]));
            types[type][power]++;
            found = TRUE;       /* found spice */
        }
    }

    /* were there any spice to work on? */
    if (!found)
        return FALSE;

    /* look for multiples, sending to refining or pressing as needed */
    for (type = 0; type < sizeof(types)/sizeof(types[0]); type++) {
        for (power = 0; power < 5; power++) {
            /* This was originally +2, however merchants cap out on buying
             * when then have 5 in their inventory.  For the ones which
             * take 4 pieces of spice to process, the NPC would need to
             * have 6 pieces in their inventory, which would never happen.
             * -Tiernan 7/16/2004 */
            /* 1 over needed to make next size, so that some will be left */
            if (types[type][power] >= spice_num_to_next[power] + 1) {
                int count = 0;
                OBJ_DATA *next_spice;

                for (spice = npc->carrying; spice && count < spice_num_to_next[power];
                     spice = next_spice) {
                    next_spice = spice->next_content;

                    if (spice->obj_flags.type == ITEM_SPICE
                        && spice->obj_flags.value[0] == spice_power_levels[power]
                        && spice->obj_flags.value[1] == type + MIN_SPICE) {
                        obj_from_char(spice);
                        sprintf(buf, "think Better send %s off for processing",
                                OSTR(spice, short_descr));
                        parse_command(npc, buf);
                        extract_obj(spice);
                        count++;
                    }
                }

                /* at this point we've found and removed spices of that type and
                 * power from inventory, now create and give object */

                vnum = next_spice_level_vnum[power] + type + MIN_SPICE;
                if ((spice = read_object(vnum, VIRTUAL)) == NULL) {
                    sprintf(buf, "kurac_spice_merchant: Unable to load vnum %d", vnum);
                    gamelog(buf);
                    return TRUE;
                }

                obj_to_char(spice, npc);
                sprintf(buf, "think Restocking %s", OSTR(spice, short_descr));
                parse_command(npc, buf);
                /* no echo */

                return TRUE;
            }
        }
    }

    return FALSE;
}
#endif

int
poison_curative(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                OBJ_DATA * obj)
{
    char arg1[250], arg2[250];
    OBJ_DATA *plant;
    CHAR_DATA *victim;

    if (cmd != CMD_APPLY)
        return FALSE;

    argument = one_argument(argument, arg1, sizeof(arg1));      /* arg1 = object */
    argument = one_argument(argument, arg2, sizeof(arg2));      /* arg2 = victim */

    if (!*arg1)
        return FALSE;
    if (!generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &victim, &plant))
        return FALSE;
    if (obj != plant)
        return FALSE;

    if (!*arg2) {
        send_to_char("Apply to whom?", ch);
        return FALSE;
    }
    if (!(victim = get_char_room_vis(ch, arg2))) {
        send_to_char("You do not see that person here.", ch);
        return FALSE;
    }
    /* We have a target, victim, and we have the object, plant  */
    if (ch != victim) {
        act("You rub $p on $N's skin.", FALSE, ch, plant, victim, TO_CHAR);
        act("$n rubs $p on $N's skin.", FALSE, ch, plant, victim, TO_ROOM);
        act("$n rubs $p on your skin.", FALSE, ch, plant, victim, TO_VICT);
    } else {
        act("$n rubs $p on $e skin.", FALSE, ch, plant, victim, TO_ROOM);
        act("You rub $p on your skin.", FALSE, ch, plant, victim, TO_CHAR);
    }

    if (affected_by_spell(victim, SPELL_POISON)) {
        affect_from_char(victim, SPELL_POISON);
        send_to_char("The burning in your veins ceases.", victim);
    }
    extract_obj(plant);
    return FALSE;
}
int
no_magick(CHAR_DATA * ch, int cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
          OBJ_DATA * obj)
{
    char command[100];

    if (cmd != CMD_CAST)
        return FALSE;

    if (GET_POS(ch) != POSITION_STANDING)
        return FALSE;

    if ((GET_GUILD(ch) == 5) && /* Templar */
        (IS_TRIBE(ch, 8) || IS_TRIBE(ch, TRIBE_ALLANAK_MIL)))  /* Tuluki or Allanki */
        return FALSE;

    switch (GET_GUILD(ch)) {
    case GUILD_DEFILER:
        send_to_char("Currents of magick begin to gather around you.\n\r", ch);
        act("Magickal currents begin to swirl around $n.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case GUILD_FIRE_ELEMENTALIST:
        send_to_char("Flames erupt in response to your summons.\n\r", ch);
        act("Flames erupt around $n as $e starts an incantation.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case GUILD_WATER_ELEMENTALIST:
        send_to_char("You call upon Vivadu for your magick.\n\r", ch);
        act("A fine mist condenses near $n as $e begins a spell.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case GUILD_STONE_ELEMENTALIST:
        send_to_char("The earth trembles in response to your call.\n\r", ch);
        act("$n begins a spell, and the earth trembles in response.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case GUILD_WIND_ELEMENTALIST:
        send_to_char("You draw upon the power of Whira for a spell.\n\r", ch);
        act("Winds swirl around $n as $e summons $s magick.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case GUILD_VOID_ELEMENTALIST:
        send_to_char("The air grows still and quiet around you as you focus your magick.\n\r", ch);
        act("The air grows still and quiet around $n as $e summons $s magick.", FALSE, ch, 0, 0,
            TO_ROOM);
        break;
    case GUILD_LIGHTNING_ELEMENTALIST:
        send_to_char("Sparks jump in the air around you as you focus your magick.\n\r", ch);
        act("Sparks jump in the air around $n as $e summons $s magick.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case GUILD_SHADOW_ELEMENTALIST:
        send_to_char("Shadows gather around you as you focus your magick.\n\r", ch);
        act("Shadows gather around $n as $e summons $s magick.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    default:
        send_to_char("You have no knowledge of such magicks.\n\r", ch);
        return TRUE;
    }

    act("$n hurls a mug at you, striking you in the head!", FALSE, npc, 0, ch, TO_VICT);
    act("$n hurls an empty mug at $N.", FALSE, npc, 0, ch, TO_NOTVICT);
    send_to_char("Oof, that smarts!\n\r", ch);

    strcpy(command, "say Practice your foul magicks elsewhere, rogue!");
    parse_command(npc, command);

    adjust_hit(ch, -number(1,3));
    update_pos(ch);
    return TRUE;
}

char *
how_good(int percent)
{
    static char buf[256];

#ifdef VERBAL_SKILL_RATING
    if (percent == 0)
        strcpy(buf, " (not learned)");
    else if (percent <= 10)
        strcpy(buf, " (awful)");
    else if (percent <= 20)
        strcpy(buf, " (bad)");
    else if (percent <= 40)
        strcpy(buf, " (poor)");
    else if (percent <= 55)
        strcpy(buf, " (average)");
    else if (percent <= 70)
        strcpy(buf, " (fair)");
    else if (percent <= 80)
        strcpy(buf, " (good)");
    else if (percent <= 85)
        strcpy(buf, " (very good)");
    else
        strcpy(buf, " (superb)");
#else
    sprintf(buf, " (%d%%)", percent);
#endif
    return buf;
}

int
parse_spell(const char *argument)
{

    int power, reach, spell;
    int i, qend = 0;
    char symbols[MAX_STRING_LENGTH];
    char tmparg[MAX_STRING_LENGTH];

    for (; *argument == ' '; argument++);

    strcpy(tmparg, argument);

    /* now for the all new, better than ever spell parser */
    if (argument && *argument == '\'') {
        if ((qend = close_parens(tmparg, '\''))) {
            sub_string(tmparg, symbols, 1, qend - 1);
            delete_substr(tmparg, 0, qend + 1);
            for (i = 0; *(symbols + i) != '\0'; i++)
                *(symbols + i) = LOWER(*(symbols + i));
            find_spell(symbols, &power, &reach, &spell);
        }
    }

    if (!qend)
        return (-1);
    else
        return (spell);
}


int
is_ok_starting_city(CHAR_DATA * ch, int city)
{
    return TRUE;
}

void
list_cities_to_char(CHAR_DATA * ch)
{
  int j;
  char buf[256];
  
  for (j = 0; j < MAX_CITIES; j++) {
 
    if (is_ok_starting_city(ch, j)) {
      if ((j != CITY_PTARKEN) || (j != CITY_NONE) || (j != CITY_DELF_OUTPOST)) {
        sprintf(buf, "%s\n\r", city[j].name);
        send_to_char(buf, ch);
      }
      
    }
  }
}

int
hall_ok_map(CHAR_DATA * ch, int cmd, const char *arg, CHAR_DATA * npc_in, ROOM_DATA * rm_in,
            OBJ_DATA * obj_in)
{
    OBJ_DATA *tmp_obj;
    char buf[256];

    if (!((cmd == CMD_LOOK) || (cmd == CMD_EXAMINE) || (cmd == CMD_READ)))
        return FALSE;

    arg = one_argument(arg, buf, sizeof(buf));

    if (!(tmp_obj = get_obj_room_vis(ch, buf)))
        return FALSE;

    if (!(tmp_obj == obj_in))
        return FALSE;

    act("Possible Starting Location Choices: (point to one of the following)", FALSE, ch, 0, 0, TO_CHAR);
    act("-------------------------", FALSE, ch, 0, 0, TO_CHAR);
    list_cities_to_char(ch);

    return TRUE;

}

int
hall_of_kings(CHAR_DATA * ch, int cmd, const char *arg, CHAR_DATA * npc_in, ROOM_DATA * rm_in,
              OBJ_DATA * obj_in)
{
    char edesc[256];
    char tmp[50];
    char buf[MAX_STRING_LENGTH];
    char oldarg[MAX_STRING_LENGTH];
    int clan, i, h;
    ROOM_DATA *to_room;
    OBJ_DATA *obj;
    long permflags = 0;

    struct city_names
    {
        int city_num;
        char *name;
        int ok_race;
        int start_room;
        int language;
        int accent;
        int accent_start;
    };

    struct city_names start_loc[] = {

        /* Southlands */
        {CITY_ALLANAK, "Allanak", 0, 1004, LANG_COMMON, LANG_SOUTH_ACCENT, 100},
        /* Terminator - none is the terminator now*/
        {CITY_NONE, "none", 0, 0, LANG_COMMON, LANG_BENDUNE_ACCENT, 100}   /* Unchooseable */
    };

    if (cmd != CMD_POINT)
        return FALSE;

    while (*arg == ' ')
        arg++;

    strcpy(oldarg, arg);
    arg = one_argument(arg, buf, sizeof(buf));

    /* if they didn't type 'point to' */
    if (strcmp(buf, "to")) {
        /* copy the oldargs back onto arg */
        arg = oldarg;
    }

   
    for (i = 0; !STRINGS_I_SAME(start_loc[i].name, "none"); i++) {
        if (STRINGS_I_SAME(start_loc[i].name, arg))
            break;
    }

    // Debug Info, don't delete I might need it later
    //  sprintf(buf, "Arg: %s, City: %d, Name: %s.\n\r",
    //      arg,
    //      start_loc[i].city_num,
    //      start_loc[i].name);
    //  send_to_char(buf, ch);

    if ((STRINGS_I_SAME(start_loc[i].name, "none")) || /* No match found */
        ((start_loc[i].ok_race != 0) && /* City is race selective */
         (start_loc[i].ok_race != GET_RACE(ch)))) {     /*Ch is not same race as city */
        send_to_char("That city is not available.\n\r", ch);
        return TRUE;
    }

    if (!(is_ok_starting_city(ch, start_loc[i].city_num))) {
        send_to_char("That city is not available.\n\r", ch);
        return TRUE;
    }


    sprintf(buf, "You point at %s on the wall-map.\n\r", start_loc[i].name);
    send_to_char(buf, ch);

    act("$n points at a wall-map.", FALSE, ch, 0, 0, TO_ROOM);
    act("$n disappears in a billowing cloud of dust!", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("Your surroundings fade away, then return again.\n\r", ch);

    // this is a hack, for some reason some characters have all 1s in their
    // abilities
    //                         -Tenebrius
    if ((GET_AGL(ch) < 7) && (GET_WIS(ch) < 7) && (GET_END(ch) < 7)) {
        roll_abilities(ch);
        init_hp(ch);
        init_stun(ch);
        shhlog("rerolling after pointing");
    } else {
        shhlog("did not reroll after pointing");
    };

    set_mana(ch, GET_MAX_MANA(ch));
    set_hit(ch, GET_MAX_HIT(ch));
    set_move(ch, GET_MAX_MOVE(ch));
    set_stun(ch, GET_MAX_STUN(ch));

    /* Reset skills to default when pointing for mortals*/
    if (!IS_IMMORTAL(ch)) {
        reset_char_skills(ch);
    }    


    /* init pain tolerance */
    init_skill(ch, TOL_PAIN, GET_END(ch));
    set_skill_taught(ch, TOL_PAIN);

    // Set default language and accent based on origin location and start_loc structure

    for (h = 0; !STRINGS_I_SAME(start_loc[h].name, "none"); h++) {
        if (start_loc[h].city_num == GET_HOME(ch))
            break;
    }

    init_skill(ch, start_loc[h].language, 100);
    init_skill(ch, start_loc[h].accent, start_loc[h].accent_start);
    set_skill_hidden(ch, start_loc[h].accent);

    ch->player.start_city = start_loc[i].city_num;

    // everyone gets a waterskin
    obj = read_object(9, VIRTUAL);       /* waterskin */
    // start them with a full waterskin
    obj->obj_flags.value[1] = obj->obj_flags.value[0];
    obj_to_char(obj, ch);

    // They're pointing, so let's set them to speaking with the correct
    // accent for their location.
    if (GET_ACCENT(ch) == LANG_NO_ACCENT && default_accent(ch) != LANG_NO_ACCENT
        && has_skill(ch, default_accent(ch)))
        ch->specials.accent = default_accent(ch);

    to_room = get_room_num(start_loc[i].start_room);

    char_from_room(ch);
    char_to_room(ch, to_room);

    parse_command(ch, "save");

    act("$n enters the world.", FALSE, ch, 0, 0, TO_ROOM);
    cmd_look(ch, "", 0, 0);
    return TRUE;
}


int
ready_to_animate(CHAR_DATA *ch) {
    if (!ch
     || !AWAKE(ch)
     || GET_POS(ch) == POSITION_FIGHTING
     || ch->specials.act_wait)
        return FALSE;
    return TRUE;
}


int post_master(CHAR_DATA * ch, int cmd, const char *arg, CHAR_DATA * npc_in, ROOM_DATA * rm_in,
                OBJ_DATA * obj_in);

void
auto_save(int zone)
{
    if (done_booting)
        save_zone(&zone_table[zone], zone);
}


void
assign_zones(void)
{
    /* Taking it out, I have a feeling this is the reason for crashes
     * (stayed up for 12 hours before put in, only like 4 after)
     * -Tenebrius
     * if (top_of_zone_table >= 35)
     * zone_table[35].func = auto_save;
     */
    /* added this 3/11/99 -Morgenes */
    if (top_of_zone_table >= RESERVED_Z)
        zone_table[RESERVED_Z].func = auto_save;
    if (top_of_zone_table >= 33)
        zone_table[33].func = auto_save;
    if (top_of_zone_table >= 13)
        zone_table[13].func = auto_save;
    if (top_of_zone_table >= 3)
        zone_table[3].func = auto_save;
}


void
free_specials(struct specials_t *specs)
{
    if (!specs)
        return;

    struct special *specials = specs->specials;
    for (int pnum = 0; pnum < specs->count; pnum++) {
        if (specials[pnum].type != SPECIAL_CODED) {
            free(specials[pnum].special.name);
        }
    }
    free(specs);

    return;
}

bool
has_special_on_cmd(struct specials_t *specs, const char *name, int cmd)
{
    if (!specs)
        return FALSE;

    int i;
    struct special *specials = specs->specials;
    for (i = 0; i < specs->count; i++) {
        if (cmd != -1 && specials[i].cmd != cmd)
            continue;

        const char *sname;
        if (specials[i].type == SPECIAL_CODED)
            sname = specials[i].special.cspec->name;
        else
            sname = specials[i].special.name;

        if (!name || !strcmp(name, sname))
            return TRUE;
    }
    return FALSE;
}

void
display_specials(char *buf, size_t bufsize, struct specials_t *specs, char stat)
{
    *buf = '\0';
    if (!specs)
        return;

    int i;
    char *start = buf;
    struct special *specials = specs->specials;
    for (i = 0; i < specs->count; i++) {
        // Too much data?
        if (buf + 50 > start + bufsize)
            break;

        char *term = "~\n";
        if (i == specs->count - 1)
            term = "";
        switch (specials[i].type) {
        case SPECIAL_CODED:
            if (stat)
                sprintf(buf, "C program: '%s', command '%s' \n\r", specials[i].special.cspec->name,
                        command_name[specials[i].cmd]);
            else
                sprintf(buf, "C %s %s%s", command_name[specials[i].cmd],
                        specials[i].special.cspec->name, term);
            break;
        case SPECIAL_DMPL:
            if (stat)
                sprintf(buf, "Dmpl program: '%s', command '%s' \n\r", specials[i].special.name,
                        command_name[specials[i].cmd]);
            else
                sprintf(buf, "dmpl %s %s%s", command_name[specials[i].cmd], specials[i].special.name, term);
            break;
        case SPECIAL_JAVASCRIPT:
            if (stat)
                sprintf(buf, "Javascript program: '%s', command '%s' \n\r",
                        specials[i].special.name, command_name[specials[i].cmd]);
            else
                sprintf(buf, "javascript %s %s%s", command_name[specials[i].cmd],
                        specials[i].special.name, term);
            break;
        default:
            if (stat)
                sprintf(buf, "Program: unknown program type\n\r");
            else
                *buf = '\0';
            break;
        }
        buf += strlen(buf);
    }
}


struct specials_t *
unset_prog(CHAR_DATA * ch, struct specials_t *specs, const char *arg)
{
    if (!specs) {
        if (ch)
            cprintf(ch, "No specials attached.\n\r");
        return NULL;
    }

    char ptype[MAX_INPUT_LENGTH];
    char pcmd[MAX_INPUT_LENGTH];
    char pname[MAX_INPUT_LENGTH];

    int cmd, type;
    int j, i;

    arg = one_argument(arg, ptype, sizeof(ptype));
    arg = one_argument(arg, pcmd, sizeof(pcmd));
    arg = one_argument(arg, pname, sizeof(pname));

    if (strlen(pname) < 1) {
        if (ch)
            cprintf(ch, "Usage: (r/c/o)unset <name> program <program type> <command> <program name>\n\r");
        return specs;
    }

    if ((cmd = get_command(pcmd)) < 0) {
        if (ch)
            cprintf(ch, "Unknown command %s\n\r", pcmd);
        return specs;
    }

    if (!strcmp("C", ptype)) {
        type = SPECIAL_CODED;
    } else if (!strcmp("dmpl", ptype)) {
        type = SPECIAL_DMPL;
    } else if (!strcmp("javascript", ptype)) {
        type = SPECIAL_JAVASCRIPT;
    } else {
        if (ch)
            cprintf(ch, "Unknown program type %s\n\r", ptype);
        return specs;
    }

    /* look for a matching program */
    struct special *programs = specs->specials;
    for (i = 0; i < specs->count; i++) {
        if ((cmd == programs[i].cmd) && (type == programs[i].type)) {
            if (type == SPECIAL_CODED) {
                if (!strcmp(programs[i].special.cspec->name, pname))
                    break;
            } else if (type == SPECIAL_DMPL) {
                if (!strcmp(programs[i].special.name, pname))
                    break;
            } else if (type == SPECIAL_JAVASCRIPT) {
                if (!strcmp(programs[i].special.name, pname))
                    break;
            }
        }
    }

    /* if no matching program was found, return */
    if (i == specs->count) {
        if (ch)
            cprintf(ch, "No matching programs.\n\r");
        return specs;
    }

    /* free any allocated stuff in the matching program entry */
    if (type != SPECIAL_CODED)
        free(programs[i].special.name);

    /* delete the program from the program list */
    specs->count--;
    for (j = i; j < specs->count; j++) {
        memcpy(&(programs[j]), &(programs[j + 1]), sizeof(programs[0]));
    }

    if (ch && IS_IMMORTAL(ch))
        cprintf(ch, "Program %s unset (%s).\n\r", pname, ptype);
    return specs;
}


/* set program for mobile / object */

struct specials_t *
set_prog(CHAR_DATA *ch, struct specials_t *specs, const char *arg)
{
    int j;
    char ptype[MAX_INPUT_LENGTH] = "";
    char pcmd[MAX_INPUT_LENGTH] = "";
    char pname[MAX_INPUT_LENGTH] = "";

    char mess[MAX_INPUT_LENGTH];

    struct special prog;
    memset(&prog, 0, sizeof(prog));

    arg = one_argument(arg, ptype, sizeof(ptype));
    arg = one_argument(arg, pcmd, sizeof(pcmd));
    arg = one_argument(arg, pname, sizeof(pname));

    if (strlen(pname) < 1) {
        if (ch)
            send_to_char("Usage: (r/c/o)set <name> program <program type> "
                         "<command> <program name>\n\r", ch);
        else
            gamelog("set_prog:error only 2 args");

        return specs;
    }

    if ((prog.cmd = get_command(pcmd)) < 0) {
        sprintf(mess, "Unknown command '%s'\n\r", pcmd);

        if (ch)
            send_to_char(mess, ch);
        else
            gamelog(mess);

        return specs;
    }

    if (!stricmp(ptype, "C")) {
        for (j = 0; (coded_specials_list[j].function) && (strcmp(pname, coded_specials_list[j].name)); j++)     /* do nothing search coded_specials */
            ;
        if (!coded_specials_list[j].function) {
            sprintf(mess, "Unable to find C program '%s'\n\r", pname);

            if (ch)
                send_to_char(mess, ch);
            else
                gamelog(mess);
            return specs;
        }

        prog.special.cspec = &coded_specials_list[j];
        prog.type = SPECIAL_CODED;
    } else if (!stricmp(ptype, "dmpl")) {
        prog.special.name = strdup(pname);
        prog.type = SPECIAL_DMPL;
    } else if (!stricmp(ptype, "javascript")) {
        prog.special.name = strdup(pname);
        prog.type = SPECIAL_JAVASCRIPT;
    } else {
        sprintf(mess,
                "Uknown program type %s.\n\rLegal program types are 'C', 'dmpl', and 'javascript'.\n\r",
                ptype);
        if (ch)
            send_to_char(mess, ch);
        else
            gamelogf("%s", mess);
        return specs;
    }

    size_t nprogs = 1;
    struct specials_t *newp;
    if (specs)
        nprogs = specs->count + 1;

    newp = realloc(specs, sizeof(size_t) + nprogs * sizeof(struct special));
    assert(newp);

    specs = newp;
    specs->count = nprogs;
    struct special *programs = specs->specials;
    memcpy(&(programs[nprogs - 1]), &prog, sizeof(struct special));

    if (ch && IS_IMMORTAL(ch))
        cprintf(ch, "Program %s set (%s).\n\r", pname, ptype);

    return specs;
}

int board(struct char_data *ch, int cmd, const char *arg, struct char_data *npc_in,
          struct room_data *rm_in, struct obj_data *obj_in);

/* this list is terminated by an element with a NULL pointer
   in the function pointer */

struct coded_special coded_specials_list[] = {
    {hall_ok_map, "hall_ok_map"},
    {hall_of_kings, "hall_of_kings"},
    {NULL, ""},
};

int
coded_name(char *name)
{
    int i;
    for (i = 0; coded_specials_list[i].function; i++)
        if (!strcmp(coded_specials_list[i].name, name))
            return (i);
    return (-1);
};


int
has_program(struct specials_t *specs, int cmd)
{
    if (!specs)
        return 0;

    for (int i = 0; i < specs->count; i++)
        if (cmd == specs->specials[i].cmd)
            return 1;

    return (0);

}

int javascript_running = FALSE;
CHAR_DATA *g_script_npc = NULL;
int
execute_npc_program(CHAR_DATA * npc, CHAR_DATA * ch, int cmd, const char *argument)
{
    int i;
    char arg[MAX_INPUT_LENGTH];
    struct special prog;

    if (!npc->specials.programs)
        return FALSE;               // Nothing to see here, move along

    strcpy(arg, argument);

    if ((cmd == CMD_PULSE) && ch)
        return FALSE;

    prog.type = -1;

    g_script_npc = npc;
    struct special *programs = npc->specials.programs->specials;
    /* keep executing until we run out of specials or one returns TRUE */
    for (i = 0; g_script_npc && i < npc->specials.programs->count; i++) {
        if (cmd != programs[i].cmd)
            continue;

        prog = programs[i];
        if (prog.type > -1) {
            int result;

            switch (prog.type) {
            case SPECIAL_CODED:
                /* if the result is false, go on */
                if ((result = prog.special.cspec->function(ch, cmd, arg, npc, 0, 0)) != FALSE) {
                    return result;
                }
                break;
            case SPECIAL_DMPL:
                /* checks to see if a dmpl program is already running */
                if (var_list)
                    continue;   /* if so, keep checking */

                if ((result = start_npc_prog(ch, npc, cmd, arg, prog.special.name)) != FALSE)
                    return result;
                break;
/* TURN OFF JAVASCRIPT FOR NOW
            case SPECIAL_JAVASCRIPT:
                if (javascript_running) // Don't want 2 javascripts running yet
                    continue;

                javascript_running = TRUE;
                if ((result = start_npc_javascript_prog(ch, npc, cmd, arg, prog.special.name)) != FALSE) {
                    javascript_running = FALSE;
                    return result;
                }
                javascript_running = FALSE;
                break;
*/
            default:
                break;
            }
        }
    }

    return FALSE;
}

OBJ_DATA *g_script_obj = NULL;
int
execute_obj_program(OBJ_DATA * obj, CHAR_DATA * ch, CHAR_DATA * vict, int cmd, const char *argument)
{
    int i;
    char arg[MAX_INPUT_LENGTH];
    struct special prog;

    if (!obj->programs)
        return 0;               // Nothing to see here, move along

    strcpy(arg, argument);

    if ((cmd == CMD_PULSE) && ch)
        return (0);

    prog.type = -1;

    // g_script_obj keeps track of whether this object gets destroyed
    g_script_obj = obj;
    struct special *programs = obj->programs->specials;
    for (i = 0; g_script_obj && i < obj->programs->count; i++) {
        if (cmd != programs[i].cmd)
            continue;

        prog = programs[i];

        if (prog.type > -1) {
            int result;

            switch (prog.type) {
            case (SPECIAL_CODED):
                if ((result = prog.special.cspec->function(ch, cmd, arg, 0, 0, obj)) != FALSE)
                    return result;
                break;
            case (SPECIAL_DMPL):
                /* checks to see if a dmpl program is already running */
                if (var_list)
                    continue;

                if ((result = start_obj_prog(ch, obj, cmd, arg, prog.special.name)) != FALSE)
                    return result;
                break;

            default:
                break;
            }
        }
    }
    g_script_obj = NULL;

    return (0);

}

int
execute_room_program(ROOM_DATA * room, CHAR_DATA * ch, int cmd, const char *argument)
{
    int i;
    int result;
    char arg[MAX_INPUT_LENGTH];

    if (!room->specials)
        return 0;

    strcpy(arg, argument);

    if ((cmd == CMD_PULSE) && ch)
        return 0;
    if (!room)
        return 0;

    struct special *programs = room->specials->specials;
    for (i = 0; i < room->specials->count; i++) {
        if (programs[i].cmd == cmd) {
            if (programs[i].type == SPECIAL_CODED) {
                if ((result = programs[i].special.cspec->function(ch, cmd, arg, 0, room, 0)) != FALSE)
                    return result;
            } else if (programs[i].type == SPECIAL_DMPL) {
                /* checks to see if a dmpl program is already running */
                if (var_list)
                    continue;   /* goes on to the next special if so */

                if ((result = start_room_prog(ch, room, cmd, arg, programs[i].special.name)) != FALSE)
                    return result;
            }
        }
    }
    return (0);

}
