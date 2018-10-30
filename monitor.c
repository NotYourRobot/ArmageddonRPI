/*
 * File: MONITOR.C
 * Usage: Routines for Monitoring people
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
 * 01/10/2001: Created -Morgenes
 */

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "limits.h"
#include "guilds.h"
#include "clan.h"
#include "event.h"
#include "cities.h"
#include "pc_file.h"
#include "vt100c.h"
#include "creation.h"
#include "modify.h"

struct flag_data monitor_flags[] = {
    {"comms", MONITOR_COMMS, CREATOR},
    {"merchant", MONITOR_MERCHANT, CREATOR},
    {"position", MONITOR_POSITION, CREATOR},
    {"rogue", MONITOR_ROGUE, CREATOR},
    {"fight", MONITOR_FIGHT, CREATOR},
    {"magick", MONITOR_MAGICK, CREATOR},
    {"movement", MONITOR_MOVEMENT, CREATOR},
    {"psionics", MONITOR_PSIONICS, CREATOR},
    {"crafting", MONITOR_CRAFTING, CREATOR},
    {"other", MONITOR_OTHER, CREATOR},
    {"others_in_room", MONITOR_IN_ROOM, CREATOR},
    {"include_npcs", MONITOR_INCLUDE_NPC, CREATOR},
    {"ignore", MONITOR_IGNORE, CREATOR},
    {"table", MONITOR_TABLE, CREATOR},
    {"", 0, 0}
};

/* look up a flag on name */
int
get_monitoring_flags(const char *flags)
{
    int i;
    char arg[MAX_STRING_LENGTH];
    int show = 0;

    flags = one_argument(flags, arg, sizeof(arg));

    while (*arg != '\0') {
        if (!strcasecmp(arg, "all"))
            show |= MONITOR_ALL;

        if (!strcasecmp(arg, "default"))
            show |= MONITOR_DEFAULT;

        for (i = 0; monitor_flags[i].name[0] != '\0'; i++)
            if (!str_prefix(arg, monitor_flags[i].name)) {
                show |= monitor_flags[i].bit;
                break;
            }

        flags = one_argument(flags, arg, sizeof(arg));
    }

    return (show);
}

MONITOR_LIST *
find_in_monitoring(MONITOR_LIST * cl, CHAR_DATA * ch, int flag)
{
    MONITOR_LIST *pPl;

    // If they're in the list with this flag OR are being
    // ignored, return the monitor list entry
    {
        MONITOR_LIST *m = 0;
        for (m = cl; m; m = m->next)
            if (IS_SET(m->show, flag) || IS_SET(m->show, MONITOR_IGNORE))
                if (m->ch == ch)
                    return m;
    }

    // Second pass covers all other possibilities
    for (pPl = cl; pPl; pPl = pPl->next) {
        if (flag && !IS_SET(pPl->show, flag))
            continue;

        /* if they're immortal, the rest don't check */
        if (IS_IMMORTAL(ch))
            continue;

        if (IS_NPC(ch) && !IS_SET(pPl->show, MONITOR_INCLUDE_NPC))
            continue;

        if (ch->in_room && pPl->room == ch->in_room)
            break;

        if( pPl->clan == 0 && ch->clan == NULL )
            break;

        if (pPl->clan != 0 && is_clan_member(ch->clan, pPl->clan))
            break;

        if (pPl->guild > -1 && GET_GUILD(ch) == pPl->guild)
            break;
        /* watching for others in the same room */
        if (IS_SET(pPl->show, MONITOR_IN_ROOM)) {
            /* if the person is in the same room as someone we're monitoring */
            if (pPl->ch && ch->in_room && pPl->ch->in_room == ch->in_room)
                break;

            /* if the person is in the same room as someone in the clan we're
             * monitoring
             */
            if (pPl->clan != -1 && ch->in_room) {
                CHAR_DATA *rch;

                for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
                    if (is_clan_member(rch->clan, pPl->clan))
                        break;
                }

                if (rch != NULL)
                    break;
            }
        }

        // monitoring tables, and they're at a table and monitoring someone
        if (IS_SET(pPl->show, MONITOR_TABLE) && ch->on_obj && pPl->ch) {
            OBJ_DATA *table;

            if (ch->on_obj->table)
                table = ch->on_obj->table;
            else
                table = ch->on_obj;

            /* if the person is at the same table */
            if (pPl->ch->on_obj == table || (pPl->ch->on_obj
             && pPl->ch->on_obj->table && pPl->ch->on_obj->table == table))
                break;
        }
    }

    return pPl;
}

MONITOR_LIST *
find_ch_in_monitoring(MONITOR_LIST * cl, CHAR_DATA * ch)
{
    MONITOR_LIST *pPl;

    for (pPl = cl; pPl; pPl = pPl->next) {
        if (pPl->ch == ch)
            break;
    }

    return pPl;
}

MONITOR_LIST *
find_room_in_monitoring(MONITOR_LIST * cl, ROOM_DATA * room)
{
    MONITOR_LIST *pPl;

    for (pPl = cl; pPl; pPl = pPl->next) {
        if (pPl->room == room)
            break;
    }

    return pPl;
}

MONITOR_LIST *
find_guild_in_monitoring(MONITOR_LIST *cl, int guild ) {
    MONITOR_LIST *pPl;

    for(pPl = cl; pPl; pPl = pPl->next) {
        if (pPl->guild == guild)
            break;
    }

    return pPl;
}

MONITOR_LIST *
find_clan_in_monitoring(MONITOR_LIST * cl, int clan)
{
    MONITOR_LIST *pPl;

    for (pPl = cl; pPl; pPl = pPl->next) {
        if (pPl->clan == clan)
            break;
    }

    return pPl;
}

void
add_monitoring(DESCRIPTOR_DATA * d, CHAR_DATA * ch, int show)
{
    MONITOR_LIST *pCl;

    if (ch == NULL) {
        shhlog("add_monitoring: no ch");
        return;
    }

    if (d == NULL) {
        shhlog("add_monitoring: no descriptor");
        return;
    }

    /* Don't let them set any other flags if they chose the ignore option */
    if (IS_SET(show, MONITOR_IGNORE))
        show = MONITOR_IGNORE;

    if ((pCl = find_ch_in_monitoring(d->monitoring, ch)) != NULL) {
        pCl->show |= show;
    } else {
        pCl = (MONITOR_LIST *) malloc(sizeof(MONITOR_LIST) * 1);
        pCl->ch = ch;
        pCl->room = NULL;
        pCl->clan = -1;
        pCl->guild = -1;
        pCl->show = show;
        pCl->next = d->monitoring;
        d->monitoring = pCl;
    }

    return;
}

void
add_monitoring_for_mobile(CHAR_DATA * ch, CHAR_DATA * mob)
{
    DESCRIPTOR_DATA *d;
    MONITOR_LIST *pCl;

    for (d = descriptor_list; d; d = d->next) {
        pCl = find_ch_in_monitoring(d->monitoring, ch);
        if (pCl)
            add_monitoring(d, mob, pCl->show);
    }
}

void
remove_monitoring(DESCRIPTOR_DATA * d, CHAR_DATA * ch, int show)
{
    MONITOR_LIST *pCl;
    MONITOR_LIST *pCl2;

    /* find them in the monitor list */
    if ((pCl = find_ch_in_monitoring(d->monitoring, ch)) == NULL)
        return;

    /* clear the requested show bits */
    REMOVE_BIT(pCl->show, show);

    /* if you're still seeing other things, leave it on */
    if (pCl->show)
        return;

    /* otherwise remove it */
    if (d->monitoring->ch == ch) {
        pCl = d->monitoring;
        d->monitoring = d->monitoring->next;
        free(pCl);
    } else {
        for (pCl = d->monitoring; pCl; pCl = pCl->next) {
            if (pCl->next && pCl->next->ch == ch) {
                pCl2 = pCl->next;
                pCl->next = pCl->next->next;
                free(pCl2);
                break;
            }
        }
    }
}

void
add_room_monitoring(DESCRIPTOR_DATA * d, ROOM_DATA * room, int show)
{
    MONITOR_LIST *pCl;

    if (room == NULL) {
        shhlog("add_monitoring: no room");
        return;
    }

    if (d == NULL) {
        shhlog("add_monitoring: no descriptor");
        return;
    }

    if ((pCl = find_room_in_monitoring(d->monitoring, room)) != NULL) {
        pCl->show |= show;
    } else {
        pCl = (MONITOR_LIST *) malloc(sizeof(MONITOR_LIST) * 1);
        pCl->room = room;
        pCl->ch = NULL;
        pCl->clan = -1;
        pCl->guild = -1;
        pCl->show = show;
        pCl->next = d->monitoring;
        d->monitoring = pCl;
    }

    return;
}


void
remove_room_monitoring(DESCRIPTOR_DATA * d, ROOM_DATA * room, int show)
{
    MONITOR_LIST *pCl;
    MONITOR_LIST *pCl2;

    /* find them in the monitor list */
    if ((pCl = find_room_in_monitoring(d->monitoring, room)) == NULL)
        return;

    /* clear the requested show bits */
    REMOVE_BIT(pCl->show, show);

    /* if you're still seeing other things, leave it on */
    if (pCl->show)
        return;

    /* otherwise remove it */
    if (d->monitoring->room == room) {
        pCl = d->monitoring;
        d->monitoring = d->monitoring->next;
        free(pCl);
    } else {
        for (pCl = d->monitoring; pCl; pCl = pCl->next) {
            if (pCl->next && pCl->next->room == room) {
                pCl2 = pCl->next;
                pCl->next = pCl->next->next;
                free(pCl2);
                break;
            }
        }
    }
}

void
add_guild_monitoring(DESCRIPTOR_DATA *d, int guild, int show) {
    MONITOR_LIST *pCl;

    if (guild == -1) {
        shhlog("add_guild_monitoring: no guild");
        return;
    }

    if (d == NULL) {
        shhlog("add_guild_monitoring: no descriptor");
        return;
    }

    if ((pCl = find_guild_in_monitoring(d->monitoring, guild)) != NULL) {
        pCl->show |= show;
    } else {
        pCl = (MONITOR_LIST *) malloc(sizeof(MONITOR_LIST) * 1);
        pCl->room = NULL;
        pCl->ch = NULL;
        pCl->clan = -1;
        pCl->guild = guild;
        pCl->show = show;
        pCl->next = d->monitoring;
        d->monitoring = pCl;
    }

    return;

}

void
remove_guild_monitoring(DESCRIPTOR_DATA * d, int guild, int show)
{
    MONITOR_LIST *pCl;
    MONITOR_LIST *pCl2;

    /* find them in the monitor list */
    if ((pCl = find_guild_in_monitoring(d->monitoring, guild)) == NULL)
        return;

    /* clear the requested show bits */
    REMOVE_BIT(pCl->show, show);

    /* if you're still seeing other things, leave it on */
    if (pCl->show)
        return;

    /* otherwise remove it */
    if (d->monitoring->guild == guild) {
        pCl = d->monitoring;
        d->monitoring = d->monitoring->next;
        free(pCl);
    } else {
        for (pCl = d->monitoring; pCl; pCl = pCl->next) {
            if (pCl->next && pCl->next->guild == guild) {
                pCl2 = pCl->next;
                pCl->next = pCl->next->next;
                free(pCl2);
                break;
            }
        }
    }
}


void
add_clan_monitoring(DESCRIPTOR_DATA * d, int clan, int show)
{
    MONITOR_LIST *pCl;

    if (clan == -1) {
        shhlog("add_clan_monitoring: no clan");
        return;
    }

    if (d == NULL) {
        shhlog("add_clan_monitoring: no descriptor");
        return;
    }

    if ((pCl = find_clan_in_monitoring(d->monitoring, clan)) != NULL) {
        pCl->show |= show;
    } else {
        pCl = (MONITOR_LIST *) malloc(sizeof(MONITOR_LIST) * 1);
        pCl->room = NULL;
        pCl->ch = NULL;
        pCl->clan = clan;
        pCl->guild = -1;
        pCl->show = show;
        pCl->next = d->monitoring;
        d->monitoring = pCl;
    }

    return;
}


void
remove_clan_monitoring(DESCRIPTOR_DATA * d, int clan, int show)
{
    MONITOR_LIST *pCl;
    MONITOR_LIST *pCl2;

    /* find them in the monitor list */
    if ((pCl = find_clan_in_monitoring(d->monitoring, clan)) == NULL)
        return;

    /* clear the requested show bits */
    REMOVE_BIT(pCl->show, show);

    /* if you're still seeing other things, leave it on */
    if (pCl->show)
        return;

    /* otherwise remove it */
    if (d->monitoring->clan == clan) {
        pCl = d->monitoring;
        d->monitoring = d->monitoring->next;
        free(pCl);
    } else {
        for (pCl = d->monitoring; pCl; pCl = pCl->next) {
            if (pCl->next && pCl->next->clan == clan) {
                pCl2 = pCl->next;
                pCl->next = pCl->next->next;
                free(pCl2);
                break;
            }
        }
    }
}

bool
is_monitoring(CHAR_DATA * ch, CHAR_DATA * target, int show)
{
    MONITOR_LIST *pCl = NULL;

    if (!ch)
        return FALSE;
    if (!target)
        return FALSE;
    if (!ch->desc)
        return FALSE;

    // Try to track them down in the monitorin list
    if (ch->desc->monitoring) {
        pCl = find_in_monitoring(ch->desc->monitoring, target, show);
    }
    // Now, are we ignoring them?
    if (pCl && IS_SET(pCl->show, MONITOR_IGNORE))
        return FALSE;

    if (!pCl && IS_IMMORTAL(target))
        return FALSE;           // Don't monitor staff in "all"

    // Is this "show" criteria in out "monitor_all"?
    if (ch->desc->monitoring_all && (ch->desc->monitor_all_show & show)
        && ((ch->desc->monitor_all_show & MONITOR_INCLUDE_NPC) || !IS_NPC(target)))
        return TRUE;

    // Monitor players wanting review
    if (ch->desc->monitoring_review 
     && target->desc
     && target->desc->player_info
     && IS_SET(target->desc->player_info->flags, PINFO_WANTS_RP_FEEDBACK)
     && (ch->desc->monitor_review_show & show)
     && ((ch->desc->monitor_review_show & MONITOR_INCLUDE_NPC) || !IS_NPC(target)))
        return TRUE;

    // Are we monitoring them directly?
    if (pCl)
        return TRUE;

    return FALSE;
}

/*
 * Sends the requested output to all people monitoring that character
 */
void
send_to_monitor(CHAR_DATA * ch, const char *output, int show)
{
    DESCRIPTOR_DATA *d;
    char out[MAX_STRING_LENGTH];

    sprintf(out, "-- %s", output);
    for (d = descriptor_list; d; d = d->next) {
        if (is_monitoring(d->character, ch, show))
            send_to_char(out, d->character);
    }
}

void
send_to_monitorf(CHAR_DATA * ch, int show, const char *message, ...)
{
    char buf[MAX_STRING_LENGTH];
    va_list argp;

    va_start(argp, message);
    vsnprintf(buf, sizeof(buf), message, argp);
    va_end(argp);

    send_to_monitor(ch, buf, show);
}

void
send_to_monitors(CHAR_DATA * ch, CHAR_DATA * tch, const char *output, int show)
{
    DESCRIPTOR_DATA *d;
    char out[MAX_STRING_LENGTH];

    sprintf(out, "-- %s", output);
    for (d = descriptor_list; d; d = d->next) {
        if (is_monitoring(d->character, ch, show) || is_monitoring(d->character, tch, show)) {
            send_to_char(out, d->character);
        }
    }
}

/* sends a parsed emote message to monitors */
void
send_to_monitor_parsed(CHAR_DATA * ch, const char *output, int show)
{
    char msg[MAX_STRING_LENGTH];
    char out[MAX_STRING_LENGTH];
    struct obj_list_type *p, *char_list = NULL;
    DESCRIPTOR_DATA *d;

    /* pass the arguments off to be parsed for us */
    if (!parse_emote_message(ch, output, TRUE, msg, sizeof(msg), &char_list))
        /* parse fails, we fail */
        return;

    /* prepend the monitor key with the parsed emote */
    sprintf(out, "-- %s", msg);

    /* look at everyone who is connected */
    for (d = descriptor_list; d; d = d->next) {
        /* if they're monitoring comms */
        if (is_monitoring(d->character, ch, show)) {
            /* use game value string print formatted to format emote */
            gvprintf(ch, d->character, NULL, out, char_list, 0, 0);
        }
    }

    /* free the char_list data */
    while (((p) = (char_list))) {
        char_list = char_list->next;
        free(p);
    }

    /* success */
    return;
}

void
send_comm_to_monitors(CHAR_DATA * ch, CHAR_DATA * tch, const char *how, const char *verb,
                      const char *exclaimVerb, const char *askVerb, const char *output)
{
    char real_name_buf[MAX_STRING_LENGTH];
    char msg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char out[MAX_STRING_LENGTH];
    char tmp[MAX_STRING_LENGTH];
    struct obj_list_type *p, *char_list = NULL;
    DESCRIPTOR_DATA *d;

    if (!ch || !tch) {
        gamelog("Error in monitor whisper, missing ch or tch");
        return;
    }

    /* prepare who is doing what */
    sprintf(tmp, "%s", REAL_NAME(ch));
    sprintf(buf, "%s%s%s %s %s", how[0] != '\0' ? how : "", how[0] != '\0' ? ", " : "", tmp,
            output[strlen(output) - 1] == '!' ? exclaimVerb : output[strlen(output) - 1] ==
            '?' ? askVerb : verb, REAL_NAME(tch));

    /* pass the arguments off to be parsed for us */
    if (!parse_emote_message(ch, buf, TRUE, msg, sizeof(msg), &char_list))
        /* parse fails, we fail */
        return;

    /* combine the parsed emote with the spoken output */
    sprintf(out, "-- %s: %s", msg, output);

    /* look at everyone who is connected */
    for (d = descriptor_list; d; d = d->next) {
        /* if they're monitoring comms */
        if (is_monitoring(d->character, ch, MONITOR_COMMS)
            || is_monitoring(d->character, tch, MONITOR_COMMS)) {
            /* use game value string print formatted to format emote */
            gvprintf(ch, d->character, NULL, out, char_list, 0, 0);
        }
    }

    /* free the char_list data */
    while (((p) = (char_list))) {
        char_list = char_list->next;
        free(p);
    }

    /* success */
    return;
}


void
send_comm_to_monitor(CHAR_DATA * ch, const char *how, const char *verb, const char *exclaimVerb,
                     const char *askVerb, const char *output)
{
    char real_name_buf[MAX_STRING_LENGTH];
    char msg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
//    char postHow[MAX_STRING_LENGTH];
    struct obj_list_type *p, *char_list = NULL;
    DESCRIPTOR_DATA *d;
    char out[MAX_STRING_LENGTH];

  //  postHow[0] = '\0';

    /* prepare who is doing what */
    sprintf(buf, "%s%s%s %s", how[0] != '\0' ? how : "", how[0] != '\0' ? ", " : "", REAL_NAME(ch),
            output[strlen(output) - 1] == '!' ? exclaimVerb : output[strlen(output) - 1] ==
            '?' ? askVerb : verb);

    /* pass the arguments off to be parsed for us */
    if (!parse_emote_message(ch, buf, TRUE, msg, sizeof(msg), &char_list))
        /* parse fails, we fail */
        return;

    /* combine the parsed emote with the spoken output */
    sprintf(out, "-- %s: %s", msg, output);

    /* look at everyone who is connected */
    for (d = descriptor_list; d; d = d->next) {
        /* if they're monitoring comms */
        if (is_monitoring(d->character, ch, MONITOR_COMMS)) {
            /* use game value string print formatted to format emote */
            gvprintf(ch, d->character, NULL, out, char_list, 0, 0);
        }
    }

    /* free the char_list data */
    while (((p) = (char_list))) {
        char_list = char_list->next;
        free(p);
    }

    /* success */
    return;
}


/*
 * Allow monitoring of a person or persons
 */
void
cmd_monitor(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    MONITOR_LIST *pCl;
    int default_flags = MONITOR_DEFAULT;
    int flags;
    int to_rem;
    char *flag_desc;

    if (get_char_extra_desc_value(ch, "[MONITOR_DEFAULT]", buf, 256)) {
        default_flags = get_monitoring_flags(buf);
        if( !default_flags ) {
            cprintf(ch, "No flags found in [MONITOR_DEFAULT], using global default instead.\n\r");
            default_flags = MONITOR_DEFAULT;
        }
    }

    argument = one_argument(argument, arg, sizeof(arg));

    if (arg[0] == '\0') {
        if (ch->desc->monitoring_all) {
            flag_desc = show_flags(monitor_flags, ch->desc->monitor_all_show);
            cprintf(ch, "Monitoring all: %s\n\r", flag_desc);
            free(flag_desc);
        }

        if (ch->desc->monitoring_review) {
            flag_desc = show_flags(monitor_flags, ch->desc->monitor_review_show);
            cprintf(ch, "Monitoring players wanting review:\n\r  %s\n\r",
             flag_desc);
            free(flag_desc);
        }

        send_to_char("You are monitoring:\n\r", ch);
        for (pCl = ch->desc->monitoring; pCl; pCl = pCl->next) {
            flag_desc = show_flags(monitor_flags, pCl->show);
            if (pCl->ch) {
                sprintf(buf, "%-15s - %s\n\r", MSTR(pCl->ch, name), flag_desc);
            } else if (pCl->room) {
                sprintf(arg, "Room %d", pCl->room->number);
                sprintf(buf, "%-15s - %s\n\r", arg, flag_desc);
            } else if (pCl->clan != -1) {
                sprintf(buf, "%-15s - %s\n\r", clan_table[pCl->clan].name, flag_desc);
            } else if (pCl->clan == -2) {
                sprintf(buf, "%-15s - %s\n\r", "Unclanned", flag_desc);
            } else {
                sprintf(buf, "Bad Monitoring\n\r");
            }

            send_to_char(buf, ch);
            free(flag_desc);
        }

        if (ch->desc->monitoring == NULL) {
            send_to_char("Noone\n\r", ch);
        }
        return;
    }

    if (!strcasecmp(arg, "none")) {
        MONITOR_LIST *pCl_next;

        for (pCl = ch->desc->monitoring; pCl; pCl = pCl_next) {
            pCl_next = pCl->next;
            free(pCl);
        }
        ch->desc->monitoring = NULL;
        ch->desc->monitoring_all = FALSE;
        ch->desc->monitor_all_show = 0;
        ch->desc->monitoring_review = FALSE;
        ch->desc->monitor_review_show = 0;

        send_to_char("All monitoring stopped.\n\r", ch);
        return;
    }

    if (!strcasecmp(arg, "all")) {
        char orig_args[MAX_INPUT_LENGTH];

        strcpy(orig_args, argument);
        argument = one_argument(argument, arg2, sizeof(arg2));

        if (arg2[0] == '\0') {
            flags = default_flags;
        } else if ((flags = get_monitoring_flags(orig_args)) == 0) {
            send_to_char("How do you want to monitor them?\n\r", ch);
            return;
        }

        ch->desc->monitor_all_show ^= flags;
        ch->desc->monitoring_all = (flags != 0);

        send_to_char("Monitoring all players.\n\r", ch);
        return;
    }

    if (!strcasecmp(arg, "review")){
        char orig_args[MAX_INPUT_LENGTH];

        if( ch->desc->monitoring_review ) {
            ch->desc->monitoring_review = FALSE;
            ch->desc->monitor_review_show = 0;
            send_to_char("Stopped monitoring players seeking review.\n\r", ch);
            return;
        }
        strcpy(orig_args, argument);
        argument = one_argument(argument, arg2, sizeof(arg2));

        if (arg2[0] == '\0') {
            flags = default_flags;
        } else if ((flags = get_monitoring_flags(orig_args)) == 0) {
            send_to_char("How do you want to monitor them?\n\r", ch);
            return;
        }

        ch->desc->monitor_review_show ^= flags;
        ch->desc->monitoring_review = (flags != 0);

        send_to_char("Monitoring players seeking review.\n\r", ch);
        return;
    }
   

    if (!str_prefix(arg, "room")) {
        ROOM_DATA *room;
        char arg[MAX_INPUT_LENGTH];

        flags = default_flags;

        room = ch->in_room;
        argument = one_argument(argument, arg, sizeof(arg));
        while (argument[0] != '\0') {
            int tmpflags = 0;

            if (arg[0] != '\0' && is_number(arg)) {

                if ((room = get_room_num(atoi(arg))) == NULL) {
                    send_to_char("No such room.\n\r", ch);
                    return;
                }
            } else if ((tmpflags = get_monitoring_flags(argument)) == 0) {
                send_to_char("How do you want to monitor them?\n\r", ch);
                return;
            }

            if (tmpflags != 0) {
                flags |= tmpflags;
                break;
            }
            argument = one_argument(argument, arg, sizeof(arg));
        }

        if (find_room_in_monitoring(ch->desc->monitoring, room))
            remove_room_monitoring(ch->desc, room, flags);
        else
            add_room_monitoring(ch->desc, room, flags);

        if ((pCl = find_room_in_monitoring(ch->desc->monitoring, room))
            != NULL) {
            flag_desc = show_flags(monitor_flags, pCl->show);
            sprintf(buf, "Monitoring room %d - %s\n\r", pCl->room->number, flag_desc);
            send_to_char(buf, ch);
            free(flag_desc);
        } else {
            sprintf(buf, "Stopped monitoring room %d.\n\r", room->number);
        }
        return;
    }

    if (!str_prefix(arg, "all_in_room")) {
        ROOM_DATA *room;
        char arg[MAX_INPUT_LENGTH];
        CHAR_DATA *rch;

        flags = default_flags;

        room = ch->in_room;
        argument = one_argument(argument, arg, sizeof(arg));

        while (arg[0] != '\0') {
            int tmpflags = 0;

            if (arg[0] != '\0' && is_number(arg)) {
                if ((room = get_room_num(atoi(arg))) == NULL) {
                    send_to_char("No such room.\n\r", ch);
                    return;
                }
            } else if ((tmpflags = get_monitoring_flags(argument)) == 0) {
                send_to_char("How do you want to monitor them?\n\r", ch);
                return;
            }

            if (tmpflags != 0) {
                flags |= tmpflags;
                break;
            }
            argument = one_argument(argument, arg, sizeof(arg));
        }

        for (rch = room->people; rch; rch = rch->next_in_room) {
            if (IS_NPC(rch) && !IS_SET(flags, MONITOR_INCLUDE_NPC))
                continue;

            if (IS_IMMORTAL(rch))
                continue;

            add_monitoring(ch->desc, rch, flags);
        }

        flag_desc = show_flags(monitor_flags, flags);
        sprintf(buf, "Monitoring everyone in room %d - %s\n\r", room->number, flag_desc);
        send_to_char(buf, ch);
        free(flag_desc);
        return;
    }

    if( !str_prefix( arg, "guild" ) ){
        int gld = -1;
        char g_name[MAX_STRING_LENGTH];
        char oldarg[MAX_INPUT_LENGTH];

        flags = default_flags;
        if (*argument == '\0') {
            send_to_char("Monitor which guild?\n\r", ch);
            return;
        }

        strcpy(oldarg, argument);
        argument = one_argument(argument, arg, sizeof(arg));

        g_name[0] = '\0';
    
        /* find the guild name in the argument */
        while (*arg != '\0') {
            int tmpguild;
    
            strcat(g_name, arg);
    
            for (tmpguild = 0; tmpguild < MAX_GUILDS; tmpguild++)
                if (guild[tmpguild].name 
                 && !str_prefix(g_name, guild[tmpguild].name))
                    break;
    
            if (tmpguild == MAX_GUILDS) {
                argument = oldarg;
                break;
            }
    
            gld = tmpguild;

            strcpy(oldarg, argument);
            argument = one_argument(argument, arg, sizeof(arg));
            strcat(g_name, " ");
        }
    
        if (gld == -1) {
            cprintf(ch, "No such guild named '%s'.\n\r", g_name);
            return;
        }
    
        if (*argument == '\0') {
            flags = default_flags;
        } else if ((flags = get_monitoring_flags(argument)) == 0) {
            send_to_char("How do you want to monitor them?\n\r", ch);
            return;
        }

        if (find_guild_in_monitoring(ch->desc->monitoring, gld))
            remove_guild_monitoring(ch->desc, gld, flags);
        else
            add_guild_monitoring(ch->desc, gld, flags);

        if ((pCl = find_guild_in_monitoring(ch->desc->monitoring, gld))
            != NULL) {
            flag_desc = show_flags(monitor_flags, pCl->show);
            sprintf(buf, "Monitoring guild %s - %s\n\r", guild[pCl->guild].name, flag_desc);
            send_to_char(buf, ch);
            free(flag_desc);
        } else {
            sprintf(buf, "Stopped monitoring guild %s.\n\r", guild[gld].name);
            send_to_char(buf, ch);
        }
        return;
    }

    if (!str_prefix(arg, "clan") || !str_prefix(arg, "unclanned")) {
        int clan = -1;
        char oldarg[MAX_INPUT_LENGTH];

        flags = default_flags;
        if( !str_prefix(arg, "unclanned") ) {
           clan = 0;
        }
        else if (*argument == '\0') {
            send_to_char("Monitor which clan?\n\r", ch);
            return;
        }
        else {
            strcpy(oldarg, argument);
            argument = one_argument(argument, arg, sizeof(arg));

            if (!is_number(arg)) {
                char c_name[MAX_STRING_LENGTH];
    
                c_name[0] = '\0';
    
                /* find the clan name in the argument */
                while (*arg != '\0') {
                    int tmpclan;
    
                    strcat(c_name, arg);
    
                    tmpclan = lookup_clan_by_name(c_name);
    
                    if (tmpclan == -1) {
                        argument = oldarg;
                        break;
                    }
    
                    clan = tmpclan;
    
                    strcpy(oldarg, argument);
                    argument = one_argument(argument, arg, sizeof(arg));
                    strcat(c_name, " ");
                }
    
                if (clan == -1) {
                    sprintf(buf, "No such clan named '%s'.\n\r", c_name);
                    send_to_char(buf, ch);
                    return;
                }
            } else {
                clan = atoi(arg);

                if (clan < 0 || clan > MAX_CLAN) {
                    send_to_char("No such clan.\n\r", ch);
                    return;
                }
            }
        }
    
        if (*argument == '\0') {
            flags = default_flags;
        } else if ((flags = get_monitoring_flags(argument)) == 0) {
            send_to_char("How do you want to monitor them?\n\r", ch);
            return;
        }

        if (find_clan_in_monitoring(ch->desc->monitoring, clan))
            remove_clan_monitoring(ch->desc, clan, flags);
        else
            add_clan_monitoring(ch->desc, clan, flags);

        if ((pCl = find_clan_in_monitoring(ch->desc->monitoring, clan))
            != NULL) {
            flag_desc = show_flags(monitor_flags, pCl->show);
            sprintf(buf, "Monitoring clan %s - %s\n\r", clan_table[pCl->clan].name, flag_desc);
            send_to_char(buf, ch);
            free(flag_desc);
        } else {
            sprintf(buf, "Stopped monitoring clan %s.\n\r", clan_table[clan].name);
            send_to_char(buf, ch);
        }
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL) {
        send_to_char("You can't find them anywhere.\n\r", ch);
        return;
    }

    if (victim->player.level >= ch->player.level && victim != ch) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    /* We are monitoring them already, let's (un)set whatever flags we want */
    if ((pCl = find_ch_in_monitoring(ch->desc->monitoring, victim)) != NULL) {
        /* Did they specify anything? */
        if (*argument == '\0') {
            flags = pCl->show;  /* Nope, they must want to stop completely */
        } else {
            flags = get_monitoring_flags(argument);     /* Yep, what are they? */
        }

        to_rem = flags & pCl->show;
        /* find the bits that are set in flags but aren't in pCl->show */
        flags = flags & (flags ^ pCl->show);

        remove_monitoring(ch->desc, victim, to_rem);
        if (!flags && !find_ch_in_monitoring(ch->desc->monitoring, victim)) {
            send_to_char("Monitoring stopped.\n\r", ch);
            return;
        }

    }

    if (*argument == '\0') {
        flags = default_flags;
    } else if ((flags = get_monitoring_flags(argument)) == 0) {
        send_to_char("How do you want to monitor them?\n\r", ch);
        return;
    }

    add_monitoring(ch->desc, victim, flags);
    pCl = find_ch_in_monitoring(ch->desc->monitoring, victim);
    flag_desc = show_flags(monitor_flags, pCl->show);
    sprintf(buf, "You are now monitoring %s'%s %s.\n\r", MSTR(victim, name),
            MSTR(victim, name)[strlen(MSTR(victim, name)) - 1] != 's' ? "s" : "", flag_desc);
    send_to_char(buf, ch);
    free(flag_desc);

    return;
}

