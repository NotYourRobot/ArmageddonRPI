/*
 * File: WATCH.C
 * Usage: Routines for Watching directions & people
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
 * 03/17/2006: Created -Morgenes
 */

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
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
#include "watch.h"
#include "object_list.h"
#include "wagon_save.h"

/* Check to see if the char is watching anything */
bool
is_char_watching(CHAR_DATA * ch)
{
    return ch->specials.watching.dir > -1
     || ch->specials.watching.victim != NULL;
}

/* returns if the char is watching a particular direction */
bool
is_char_watching_dir(CHAR_DATA * ch, int dir)
{
    return ch->specials.watching.dir == dir;
}

/* returns if the character is watching in any direction */
bool
is_char_watching_any_dir(CHAR_DATA * ch)
{
    return ch->specials.watching.dir > -1;
}

/* Get the direction the character is watching */
int
get_char_watching_dir(CHAR_DATA * ch)
{
    return ch->specials.watching.dir;
}

/* Get the victim the character is watching */
CHAR_DATA *
get_char_watching_victim(CHAR_DATA * ch)
{
    return ch->specials.watching.victim;
}

/* returns if the character is watching the specified character */
bool
is_char_watching_char(CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (victim == NULL)
        return FALSE;

    return ch->specials.watching.victim == victim;
}

/* returns if the character is watching another character */
bool
is_char_watching_any_char(CHAR_DATA * ch)
{
    return ch->specials.watching.victim != NULL;
}

/* returns if the character is being watched */
bool
is_char_being_watched(CHAR_DATA * ch)
{
    CHAR_DATA *rch;

    if (!ch->in_room)
        return FALSE;

    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if (is_char_actively_watching_char(rch, ch))
            return TRUE;
    }

    return FALSE;
}

/* returns if the character is actively watch the person */
bool
is_char_actively_watching_char(CHAR_DATA * watcher, CHAR_DATA * watchee)
{
    /* make sure there is a watcher and a watchee */
    return (watcher && watchee && watcher->specials.watching.victim == watchee
     && CAN_SEE(watcher, watchee));
}

/* Stop all watching for a character 
 * ch - character to stop watching
 * show_failure - shows if they aren't watching anything
 */
void
stop_watching(CHAR_DATA * ch, bool show_failure)
{
    OBJ_DATA *tmpob;

    if (ch->specials.watching.dir > -1) {
        if (ch->specials.watching.dir == DIR_OUT) {
            tmpob = find_wagon_for_room(ch->in_room);
            if (tmpob)
                cprintf(ch, "You stop watching out of %s.\n\r", OSTR(tmpob, short_descr));
            else
                cprintf(ch, "You stop watching out.\n\r");
        } else
            cprintf(ch, "You stop watching the %s exit.\n\r", dirs[ch->specials.watching.dir]);
    }

    if (ch->specials.watching.victim) {
        if (ch->specials.watching.victim->in_room
            && ch->in_room == ch->specials.watching.victim->in_room) {
            cprintf(ch, "You stop watching %s.\n\r", PERS(ch, ch->specials.watching.victim));
        } else {
            cprintf(ch, "You stop watching them.\n\r");
        }
    }

    if (ch->specials.watching.dir == -1 && ch->specials.watching.victim == NULL && show_failure) {
        cprintf(ch, "You weren't watching anything to start with.\n\r");
    }

    stop_watching_raw(ch);
    return;
}

/* stop watching without any echoes */
void
stop_watching_raw(CHAR_DATA * ch)
{
    ch->specials.watching.dir = -1;
    ch->specials.watching.victim = NULL;
}


/* stop anyone who is watching the specified character */
void
stop_reverse_watching(CHAR_DATA * ch)
{
    CHAR_DATA *tch;

    for (tch = character_list; tch; tch = tch->next) {
        if (tch->specials.watching.victim == ch) {
            tch->specials.watching.victim = NULL;
        }
    }
}

/* start watching an exit */
void
add_exit_watched(CHAR_DATA * watcher, int dir)
{
    OBJ_DATA *tmpob;

    if (dir == DIR_OUT) {
        tmpob = find_wagon_for_room( watcher->in_room );
        if (tmpob)
            cprintf(watcher, "You begin watching out of %s.\n\r",
                    format_obj_to_char(tmpob, watcher, 1));
        else {
            cprintf(watcher, "You're not inside anything!\n\r");
            return;
        }
    } else
        cprintf(watcher, "You begin watching the %s exit.\n\r", dirs[dir]);

    watcher->specials.watching.dir = dir;
    return;
}


/* derive a skill penalty for ch watching you */
int
get_watched_penalty(CHAR_DATA * watcher)
{
    int penalty = 50;

    if (!watcher)
        return 0;

    if (has_skill(watcher, SKILL_WATCH))
        penalty = MAX(penalty, watcher->skills[SKILL_WATCH]->learned);

    penalty += wis_app[GET_WIS(watcher)].percep;

    return penalty;
}

/* start watching the victim */
void
start_watching_victim(CHAR_DATA * watcher, CHAR_DATA * target)
{
    CHAR_DATA *rch;

    for (rch = watcher->in_room->people; rch; rch = rch->next_in_room) {
        if (is_char_actively_watching_char(rch, watcher)) {
            if (rch == target) {
                cprintf(rch, "You notice %s start watching you.\n\r", PERS(rch, watcher));
            } else if (watch_success(rch, watcher, NULL, 0, SKILL_NONE)) {
                cprintf(rch, "You notice %s start watching %s.\n\r", PERS(rch, watcher),
                        PERS(rch, target));
            }
        }
    }

    cprintf(watcher, "You begin watching %s.\n\r", PERS(watcher, target));
    start_watching_victim_raw(watcher, target);
}

/* Start watching without any echoes */
void
start_watching_victim_raw(CHAR_DATA * watcher, CHAR_DATA * target)
{
    watcher->specials.watching.victim = target;
}

/* check to see if the person saw a furtive action */
bool
watch_success(CHAR_DATA * watcher, CHAR_DATA * actor, CHAR_DATA * target, int bonus, int oppSkill)
{
    int roll = number(1, 101);
    int skill;

    if (IS_IMMORTAL(watcher))
        return TRUE;

    /* never see immortals */
    if (IS_IMMORTAL(actor))
        return FALSE;

    /* if the same person, no */
    if (watcher == actor)
        return FALSE;

    /* if you can't see the person, no chance to watch them do something */
    if (!CAN_SEE(watcher, actor)) {
        return FALSE;
    }

    /* if you don't have the watch skill */
    if (!has_skill(watcher, SKILL_WATCH)) {
        /* initialize it as a learned skill */
        if (!init_learned_skill(watcher, SKILL_WATCH, 5)) {
            /* failed to 'learn' watching, so no chance to succeed */
            return FALSE;
        }
    }

    /* base it off their learned amount
     * (they should have the skill at this point 
     */
    skill = watcher->skills[SKILL_WATCH]->learned;

    /* add the wisdom-based perception bonus */
    skill += wis_app[GET_WIS(watcher)].percep;

    /* if there is an opposing skill */
    if (oppSkill != SKILL_NONE) {
        /* bonus if you have the skill that's being used */
        if (has_skill(watcher, oppSkill)) {
            /* TODO: watch for wilderness/city skill combos */
            skill += watcher->skills[oppSkill]->learned / 5;
        }
    }

    /* add the bonus that was passed in to us; limit it to 20 max */
    bonus = MIN(20, bonus);
    bonus = MAX(-20, bonus);
    skill += bonus;

    /* if you're not actively watching the actor or target */
    if (!is_char_actively_watching_char(watcher, actor)
        && !is_char_actively_watching_char(watcher, target)) {
        /* cut the skill in half */
        skill /= 2;

        /* cut it in quarter again if they're watching someone else */
        if (is_char_watching_any_char(watcher)) {
            skill /= 4;
        }
    }

    /* always a 1% chance of success */
    skill = MAX(1, skill);

    /* always a 1% chance of failure */
    skill = MIN(100, skill);

    /* compare the roll against the modified skill */
    if (roll > skill) {
/* #define DEBUG_WATCH */
#ifdef DEBUG_WATCH
        if (!IS_NPC(watcher)) {
            if (target)
                gamelogf("%s failed to see %s %s from %s (roll: %d; skill: %d)", GET_NAME(watcher),
                         GET_NAME(actor), skill_name[oppSkill], GET_NAME(target), roll, skill);
            else
                gamelogf("%s failed to see %s %s (roll: %d; skill: %d)", GET_NAME(watcher),
                         GET_NAME(actor), skill_name[oppSkill], roll, skill);
        }
#endif
        /* only gain if you're actively trying to watch */
        if (is_char_watching_char(watcher, actor)
            || is_char_watching_char(watcher, target)) {
            /* failure, let them gain */
            gain_skill(watcher, SKILL_WATCH, 1);
        }
        return FALSE;
    }
#ifdef DEBUG_WATCH
    if (!IS_NPC(watcher)) {
        if (target)
            gamelogf("%s saw %s %s from %s (roll: %d; skill: %d)", GET_NAME(watcher),
                     GET_NAME(actor), skill_name[oppSkill], GET_NAME(target), roll, skill);
        else
            gamelogf("%s saw %s %s (roll: %d; skill: %d)", GET_NAME(watcher), GET_NAME(actor),
                     skill_name[oppSkill], roll, skill);
    }
#endif

    /* success */
    return TRUE;
}

/* do a check to see if the person can still see the person */
void
watch_vis_check(CHAR_DATA * victim)
{
    CHAR_DATA *wch;

    /* Loop through every character in the game */
    for (wch = character_list; wch; wch = wch->next) {
        /* if they're trying to watch them */
        if (is_char_watching_char(wch, victim)) {
            /* stop watching (so that doesn't influence CAN_SEE) */
            stop_watching_raw(wch);

            /* if they can still see them */
            if (CAN_SEE(wch, victim)) {
                /* set them back to watching */
                start_watching_victim_raw(wch, victim);
            }
        }
    }
}

void
watch_status(CHAR_DATA * ch, CHAR_DATA * viewer)
{
    OBJ_DATA *obj;
    CHAR_DATA *victim;

    /* Watching a direction */
    if (ch->specials.watching.dir > -1) {
        if (ch->specials.watching.dir == DIR_OUT) {
            obj = find_wagon_for_room(ch->in_room);
            if (obj)
                cprintf(viewer, "%s %s watching out of %s.\n\r",
                        viewer == ch ? "You" : GET_NAME(ch), viewer == ch ? "are" : "is",
                        format_obj_to_char(obj, ch, 1));
            else
                cprintf(viewer, "%s %s watching out.\n\r", viewer == ch ? "You" : GET_NAME(ch),
                        viewer == ch ? "are" : "is");
        } else
            cprintf(viewer, "%s %s watching the %s exit.\n\r", viewer == ch ? "You" : GET_NAME(ch),
                    viewer == ch ? "are" : "is", dirs[ch->specials.watching.dir]);
        return;
    }
    /* watching a person */
    else if (ch->specials.watching.victim != NULL) {
        victim = ch->specials.watching.victim;

        /* only if they're in the same room */
        if (CAN_SEE(ch, victim) && victim->in_room == ch->in_room) {
            cprintf(viewer, "%s %s watching %s.\n\r", viewer == ch ? "You" : GET_NAME(ch),
                    viewer == ch ? "are" : "is", PERS(ch, victim));
        } else {
            cprintf(viewer, "%s %s watching someone who is not here.\n\r",
                    viewer == ch ? "You" : GET_NAME(ch), viewer == ch ? "are" : "is");
        }
        return;
    }

    /* Not watching anywhere */
    cprintf(viewer, "You aren't watching anything in particular.\n\r");
    return;
}

void
cmd_watch(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    char arg1[256];             /* used to break up the arguments */
    int dir = 0;                /* direction to watch */
    CHAR_DATA *victim;          /* potential person to be watched */

    /* get the first word typed */
    arg = one_argument(arg, arg1, sizeof(arg1));

    /* if no argument, or if it's 'status' */
    if (!*arg1 || (*arg1 && !strcasecmp(arg1, "status"))) {
        watch_status(ch, ch);
        return;
    }

    /* handle none */
    if (!strcasecmp(arg1, "none")) {
        stop_watching(ch, 1);
        return;
    }

    /* looking to watch, so let's look in the room first */
    victim = get_char_room_vis(ch, arg1);

    /* found a victim */
    if (victim != NULL) {
        if (victim == ch) {
            cprintf(ch, "How exactly do you plan on watching yourself?\n\r");
            return;
        }

        if (ch->specials.fighting != NULL && ch->specials.fighting != victim) {
            cprintf(ch,
                "You're too busy fighting for your life to worry about" " watching right now!\n\r");
            return;
        }

        /* if they're scanning, stop it and tell them */
        if (IS_AFFECTED(ch, CHAR_AFF_SCAN)) {
            cprintf(ch, "You stop scanning the immediate area.\n\r");
            affect_from_char(ch, SKILL_SCAN);
        }

        if (ch->specials.guarding == victim) {
            stop_guard(ch, 0);
        }

        stop_watching(ch, 0);
        start_watching_victim(ch, victim);
        return;
    }

    if (!strncasecmp(arg1, "north", strlen(arg1)))
        dir = DIR_NORTH;
    else if (!strncasecmp(arg1, "south", strlen(arg1)))
        dir = DIR_SOUTH;
    else if (!strncasecmp(arg1, "east", strlen(arg1)))
        dir = DIR_EAST;
    else if (!strncasecmp(arg1, "west", strlen(arg1)))
        dir = DIR_WEST;
    else if (!strncasecmp(arg1, "up", strlen(arg1)))
        dir = DIR_UP;
    else if (!strncasecmp(arg1, "down", strlen(arg1)))
        dir = DIR_DOWN;
    else if (!strncasecmp(arg1, "out", strlen(arg1)))
        dir = DIR_OUT;
    else {
        cprintf(ch, "You can't find any '%s' to watch.\n\r", arg1);
        return;
    }

    if (GET_POS(ch) < POSITION_SITTING) {
        switch (GET_POS(ch)) {
        case POSITION_RESTING:
            cprintf(ch, "You feel too relaxed to do that.\n\r");
            break;
        default:
            break;
        }
        return;
    }

    /* if they're scanning, stop it and tell them */
    if (IS_AFFECTED(ch, CHAR_AFF_SCAN)) {
        cprintf(ch, "You stop scanning the immediate area.\n\r");
        affect_from_char(ch, SKILL_SCAN);
    }

    /* can't watch & guard */
    if (ch->specials.dir_guarding > -1 && ch->specials.dir_guarding == dir)
        stop_guard(ch, 0);

    /* Make them un-watch everything else */
    stop_watching(ch, 0);

    /* sends messages and does checks */
    add_exit_watched(ch, dir);
    return;
}

