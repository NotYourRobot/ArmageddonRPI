/*
 * File: SPICE.C
 * Usage: Functions for various types of spices.
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
 * 12/26/1999 Added halfling sniff to ghaati as well.  -Sanvean
 *  5/06/2000 Meddled with spice, increased duration, decreased
 *            detox.  -Sanvean
 * 04/26/2003 converted SPICE_HIT macro to spice_hit function - Tiernan
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "structs.h"
#include "comm.h"
#include "skills.h"
#include "limits.h"
#include "handler.h"
#include "guilds.h"
#include "utils.h"
#include "utility.h"
#include "db.h"
#include "modify.h"
#include "info.h"

#define MINPINCH 20341
#define MAXPINCH 20349

#define CHAR_APPLY_OFF_STAT  CHAR_APPLY_OFFENSE
#define CHAR_APPLY_DEF_STAT  CHAR_APPLY_DEFENSE
#define GET_OFF_STAT(ch)     ((ch)->abilities.off)
#define GET_DEF_STAT(ch)     ((ch)->abilities.def)
#define GET_MAX_HIT_STAT(ch) ((ch)->points.max_hit)
#define GET_NONE(ch)         tmp

void spice_death(CHAR_DATA * ch, int spice);
void send_spice_affect_msg(CHAR_DATA * ch, int spice);
char *spice_methelinoc_msg();
char *spice_melem_tuek_msg();
char *spice_krelez_msg();
char *spice_kemen_msg();
char *spice_aggross_msg();
char *spice_zharal_msg();
char *spice_thodeliv_msg();
char *spice_krentakh_msg();
char *spice_qel_msg();
char *spice_moss_msg();

/* spice_hit() is a conversion of the SPICE_HIT() macro into a regular
 * function call. - Tiernan 4/26/03 */
int
spice_hit(byte level, CHAR_DATA * ch, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj, int spice,
          int ablp)
{
    struct affected_type af, *p;
    int new_add, old_add, tmp, bonus;
    int dur, mod, detox = 1;
    bool overdose = FALSE;

    if (type != SPELL_TYPE_SPICE)
        return TRUE;

    memset(&af, 0, sizeof(af));

    if (ch->skills[spice])
        old_add = ch->skills[spice]->learned;
    else
        old_add = 0;

    dur = abs(level);
    if (!tar_obj)
        dur *= 3;

    dur = MAX(3, dur);

    mod = level + (old_add / (20 * (6 - level)));

    af.type = spice;
    af.duration = dur;
    af.modifier = mod;
    af.location = ablp;
    af.bitvector = 0;
    affect_to_char(tar_ch, &af);

    /* The affect we applied changes an attribute (e.g. endurance) or the
     * maximum amount of a stat (e.g. max moves).  For spices that have an
     * "instant bonus", we need to specify them here. - Tiernan 4/26/2006
     */
    bonus = number(5, 10) + (7 * level * (100 - old_add) / 100);
    switch (spice) {
    case (SPICE_KRENTAKH):
        set_move(tar_ch, MIN(GET_MOVE(tar_ch) + bonus, GET_MAX_MOVE(tar_ch)));
        break;
    case (SPICE_THODELIV):
        set_stun(tar_ch, MIN(GET_STUN(tar_ch) + bonus, GET_MAX_STUN(tar_ch)));
        break;
    default:
        break;
    }

    if (spice_in_system(ch, spice) > (2 * GET_END(ch))) {
        send_to_char("You transcend your body.\n\r", ch);
        act("$n's face lights up with sudden rapture as $s body collapses.", FALSE, ch, 0, 0,
            TO_ROOM);
        spice_death(ch, spice);
        return FALSE;
    }
    // If they already have the detox affect for this spice, change
    // the duration to the max of the two.
    for (p = tar_ch->affected; p; p = p->next)
        if ((p->type == spice) && (p->location == CHAR_APPLY_DETOX)) {
            p->duration = MAX(p->duration, dur * 2);
            p->expiration = p->initiation + (RT_ZAL_HOUR * p->duration) - 1;
            detox = 0;
            break;
        }

    if (detox) {
        af.type = spice;
        af.duration = dur * 2;
        af.modifier = 0;
        af.location = CHAR_APPLY_DETOX;
        af.bitvector = 0;
        affect_to_char(tar_ch, &af);
    }

    gain_skill(ch, spice, level);
    if (!(has_skill(ch, spice)))
        init_skill(ch, spice, 1);
    new_add = ch->skills[spice]->learned;

    if (spice == SPICE_KRENTAKH)
        tmp = 20;
    else
        tmp = 10;
    if (((new_add / tmp) - (old_add / tmp)) >= 1) {
        overdose = FALSE;
        switch (spice) {
        case (SPICE_METHELINOC):
            SET_WIS(ch, GET_WIS(ch) - 1);
            --ch->abilities.wis;
            if (GET_WIS(ch) < 0)
                overdose = TRUE;
            break;
        case (SPICE_MELEM_TUEK):
            SET_STR(ch, GET_STR(ch) - 1);
            --ch->abilities.str;
            if (GET_STR(ch) < 0)
                overdose = TRUE;
            break;
        case (SPICE_KRELEZ):
            SET_AGL(ch, GET_AGL(ch) - 1);
            --ch->abilities.agl;
            if (GET_AGL(ch) < 0)
                overdose = TRUE;
            break;
        case (SPICE_KEMEN):
            --GET_MAX_HIT_STAT(ch);
            ch->points.max_hit -= 4;;
            if (GET_MAX_HIT_STAT(ch) < 0)
                overdose = TRUE;
            break;
        case (SPICE_ZHARAL):
            --GET_DEF_STAT(ch);
//           commenting the next 2 lines out, because mages start with 0 def - Halaster
//            if (GET_DEF_STAT(ch) < 0)
//                overdose = TRUE;
            break;
        case (SPICE_KRENTAKH):
        case (SPICE_AGGROSS):
            SET_END(ch, GET_END(ch) - 1);
            --ch->abilities.end;
            if (GET_END(ch) < 0)
                overdose = TRUE;
            break;
        default:
            break;
        }

        if (overdose == TRUE) {
            send_to_char("You transcend your body.\n\r", ch);
            act("$n's face lights up with sudden rapture as $s body collapses.", FALSE, ch, 0, 0,
                TO_ROOM);
            spice_death(ch, spice);
            return FALSE;
        }
    }

    return TRUE;
}

int
spice_in_system(CHAR_DATA * ch, int spice)
{

    struct affected_type *af;
    int amt = 0;

    for (af = ch->affected; af; af = af->next)
        if ((af->type == spice) && (af->location != CHAR_APPLY_DETOX))
            amt += (af->duration + 1);

    return (amt / 3);
}

void
spice_death(CHAR_DATA * ch, int spice)
{

    char buf[256];
    sprintf(buf, "%s %s%s%shas died from overindulgence in %s in room #%d.",
            IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch), !IS_NPC(ch) ? "(" : "",
            !IS_NPC(ch) ? ch->account : "", !IS_NPC(ch) ? ") " : "", skill_name[spice],
            ch->in_room->number);

    if (!IS_NPC(ch)) {
        struct time_info_data playing_time =
            real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);

        if (!IS_IMMORTAL(ch)
            && (playing_time.day > 0 || playing_time.hours > 2
                || (ch->player.dead && !strcmp(ch->player.dead, "rebirth")))
            && !IS_SET(ch->specials.act, CFL_NO_DEATH)) {
            if (ch->player.info[1])
                free(ch->player.info[1]);
            ch->player.info[1] = strdup(buf);
        }

        gamelog(buf);
    } else
        shhlog(buf);

    die(ch);
}

void
tol_alcohol(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{
}
void
tol_generic(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{
}
void
tol_grishen(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{
}
void
tol_skellebain(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
               OBJ_DATA * tar_obj)
{
}
void
tol_methelinoc(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
               OBJ_DATA * tar_obj)
{
}
void
tol_terradin(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
}
void
tol_peraine(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{
}
void
tol_heramide(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
}

void
tol_plague(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
}

void
tol_pain(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
}

int
max_poison_gain(CHAR_DATA * ch)
{
    int max_gain, race_gain;

    switch (GET_RACE(ch)) {

    case RACE_HUMAN:
        race_gain = 40;
        break;
    case RACE_HALF_ELF:
        race_gain = 30;
        break;
    case RACE_ELF:
    case RACE_DESERT_ELF:
        race_gain = 10;
        break;
    case RACE_DWARF:
        race_gain = 65;
        break;
    case RACE_HALF_GIANT:
        race_gain = 45;
        break;
    case RACE_MUL:
        race_gain = 50;
        break;
    case RACE_HALFLING:
        race_gain = 60;
        break;
    default:
        race_gain = 5;
    }

    max_gain = GET_END(ch) + race_gain;

    return (max_gain);
}

void
gain_poison_tolerance(CHAR_DATA * ch, int sk, int gain)
{
    int max_gain;
    long t;

    if (!ch->skills[sk])
        init_skill(ch, sk, 0);

    if (((t = time(0)) - ch->skills[sk]->last_gain) < (SECS_PER_REAL_MIN * 10))
        return;

    /* gain the skill points */
    max_gain = max_poison_gain(ch);

    ch->skills[sk]->learned += gain;
    ch->skills[sk]->learned = MIN(ch->skills[sk]->learned, max_gain);

    ch->skills[sk]->last_gain = t;
    ch->skills[sk]->rel_lev = MAX(ch->skills[sk]->rel_lev, ch->skills[sk]->learned);
}

void
gain_tolerance_skill(CHAR_DATA * ch, int sk, int gain)
{
    long t;
    int wt, max_gain;

    if (!ch->skills[sk])
        init_skill(ch, sk, 0);

    max_gain = 20 + (get_char_size(ch) * 3);
    max_gain = MIN(max_gain, 75);

    if (IS_IMMORTAL(ch))
        max_gain = 100;

    wt = 2 * MAX(20, (get_char_size(ch) / 3) + (ch->skills[sk]->learned / 7));

    if (((t = time(0)) - ch->skills[sk]->last_gain) < (SECS_PER_REAL_MIN * wt))
        return;

    /* can't build up alcohol tolerance until drunkenness surpasses it */
    if (sk == TOL_ALCOHOL && GET_COND(ch, DRUNK) < ch->skills[sk]->learned)
        return;

    /* easier for the body to re-adapt, than to learn */
    if (ch->skills[sk]->learned < ch->skills[sk]->rel_lev)
        gain += 2;

    /* gain the skill points */
    ch->skills[sk]->learned += gain;
    ch->skills[sk]->learned = MIN(ch->skills[sk]->learned, max_gain);

    ch->skills[sk]->last_gain = t;
    ch->skills[sk]->rel_lev = MAX(ch->skills[sk]->rel_lev, ch->skills[sk]->learned);
}

void
spice_methelinoc(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{
    if (!spice_hit(level, ch, type, tar_ch, tar_obj, SPICE_METHELINOC, CHAR_APPLY_WIS))
        return;

    send_to_char("You feel a heightened awareness.\n\r", tar_ch);
    act("$n's countenance becomes a bit more solemn.", FALSE, ch, 0, 0, TO_ROOM);
}

void
spice_melem_tuek(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{
    if (!spice_hit(level, ch, type, tar_ch, tar_obj, SPICE_MELEM_TUEK, CHAR_APPLY_STR))
        return;

    send_to_char("A feeling of great confidence comes over you.\n\r", tar_ch);
    act("$n flexes unconsciously and smiles to $mself.", FALSE, ch, 0, 0, TO_ROOM);
}

void
spice_krelez(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    if (!spice_hit(level, ch, type, tar_ch, tar_obj, SPICE_KRELEZ, CHAR_APPLY_AGL))
        return;

    send_to_char("Your heart speeds up and you feel full of energy.\n\r", tar_ch);
    act("$n begins moving about excitedly.", FALSE, ch, 0, 0, TO_ROOM);

}

void
spice_kemen(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{
    if (!spice_hit(level, ch, type, tar_ch, tar_obj, SPICE_KEMEN, CHAR_APPLY_HIT))
        return;

    send_to_char("You feel flushed, and...invulnerable.\n\r", tar_ch);
    act("$n's pupils dilate and beads of sweat appear on $s brow.", FALSE, ch, 0, 0, TO_ROOM);
}

void
spice_aggross(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{
    if (!spice_hit(level, ch, type, tar_ch, tar_obj, SPICE_AGGROSS, CHAR_APPLY_END))
        return;

    send_to_char("Your heart begins to thump solidly in your chest.\n\r", tar_ch);
    act("$n's breathing slows and grows deeper.", FALSE, ch, 0, 0, TO_ROOM);

}

void
spice_zharal(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    if (!spice_hit(level, ch, type, tar_ch, tar_obj, SPICE_ZHARAL, CHAR_APPLY_OFF_STAT))
        return;

    send_to_char("You feel more aggressive.\n\r", tar_ch);
}


void
spice_thodeliv(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
               OBJ_DATA * tar_obj)
{
    if (!spice_hit(level, ch, type, tar_ch, tar_obj, SPICE_THODELIV, CHAR_APPLY_NONE))
        return;

    if (affected_by_spell(ch, AFF_MUL_RAGE))
        affect_from_char(ch, AFF_MUL_RAGE);

    send_to_char("You feel euphoric, and a numbness creeps across your " "body.\n\r", tar_ch);
    act("$n's eyes become glassy-red and half-closed.", FALSE, ch, 0, 0, TO_ROOM);
}

void
spice_krentakh(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
               OBJ_DATA * tar_obj)
{
    if (!spice_hit(level, ch, type, tar_ch, tar_obj, SPICE_KRENTAKH, CHAR_APPLY_NONE))
        return;

    send_to_char("You feel elated.\n\r", tar_ch);
    act("$n's expression becomes lighter.", FALSE, ch, 0, 0, TO_ROOM);
}

/* tweaked to be harmful for those who have gone over to defiling - Turgon */
void
spice_qel(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (type != SPELL_TYPE_SPICE)
        return;

    af.type = SPICE_QEL;
    af.duration = level;
    if (tar_ch->specials.eco >= 0)
        af.modifier = level * 2;
    else
        af.modifier = -level * 2;
    af.location = CHAR_APPLY_MANA;
    af.bitvector = 0;
    affect_to_char(tar_ch, &af);

    if (tar_ch->specials.eco >= 0)
        send_to_char("You feel more in tune with the elements.\n\r", tar_ch);
    else
        send_to_char("You feel a gut-wrenching sensation.\n\r", tar_ch);
}

void
spice_moss(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (type != SPELL_TYPE_SPICE)
        return;

    GET_COND(ch, THIRST) = 48;
    GET_COND(ch, FULL) = 48;

    af.type = SPICE_MOSS;
    af.duration = level;
    af.modifier = level;
    af.location = CHAR_APPLY_END;
    af.bitvector = 0;
    affect_to_char(tar_ch, &af);

    act("Your thirst feels quenched, and you feel more vigorous.", FALSE, ch, 0, 0, TO_CHAR);
}


/* this isn't related to spice, but it is related to cmd_sniff */
void
sniff_the_air(CHAR_DATA * ch)
{
    int dir, dist, i;
    char msgBuf[512];
    CHAR_DATA *trCh;
    ROOM_DATA *trRoom, *lstRoom;

    if ((GET_RACE(ch) == RACE_HALFLING) || (GET_RACE(ch) == RACE_GHAATI)) {
        act("You tilt your head and sniff the air.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n tilts $s head and sniffs the air.", TRUE, ch, 0, 0, TO_ROOM);

        dir = rev_dir[(int) weather_info.wind_direction];
        dist = MIN(weather_info.wind_velocity / 10, 5);

        /* not much wind indoors usually */
        if (IS_SET(ch->in_room->room_flags, RFL_INDOORS))
            dist = 0;

        lstRoom = 0;

        for (i = 0; i <= dist; i++) {
            trRoom = get_room_distance(ch, i, dir);
            if (trRoom && (trRoom != lstRoom)) {
                for (trCh = trRoom->people; trCh; trCh = trCh->next_in_room)
                    if (number(0, (1 + ((wis_app[GET_WIS(ch)].percep) / 5))) && (trCh != ch)
                        && !(IS_IMMORTAL(trCh)) && !(affected_by_spell(ch, SPELL_ETHEREAL) || IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))) {
                        if ((GET_RACE(trCh) == RACE_VAMPIRE) || (GET_RACE(trCh) == RACE_GHOUL))
                            send_to_char("You smell something unusual.\n\r", ch);
                        else {
                            sprintf(msgBuf, "You smell %s%s.\n\r",
                                    indefinite_article(race[GET_RACE(trCh)].name),
                                    race[GET_RACE(trCh)].name);
                            send_to_char(msgBuf, ch);
                        }
                    }
            } else
                break;
        }
    } else if (GET_RACE(ch) == RACE_MANTIS) {
        act("You test the air with your antennae.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n's antennae wave about, testing the air.", TRUE, ch, 0, 0, TO_ROOM);

        dir = rev_dir[(int) weather_info.wind_direction];
        dist = MIN(weather_info.wind_velocity / 10, 3);

        /* not much wind indoors */
        if (IS_SET(ch->in_room->room_flags, RFL_INDOORS))
            dist = 0;

        lstRoom = 0;

        for (i = 0; i <= dist; i++) {
            trRoom = get_room_distance(ch, i, dir);
            if (trRoom && (trRoom != lstRoom)) {
                for (trCh = trRoom->people; trCh; trCh = trCh->next_in_room)
                    if (number(0, (1 + ((wis_app[GET_WIS(ch)].percep) / 5))) && (trCh != ch)
                        && !(IS_IMMORTAL(trCh)) && !(affected_by_spell(ch, SPELL_ETHEREAL))) {
                        if ((GET_RACE(trCh) == RACE_VAMPIRE) || (GET_RACE(trCh) == RACE_GHOUL))
                            send_to_char("You sense something unusual.\n\r", ch);
                        else {
                            sprintf(msgBuf, "You sense %s%s.\n\r",
                                    indefinite_article(race[GET_RACE(trCh)].name),
                                    race[GET_RACE(trCh)].name);
                            send_to_char(msgBuf, ch);
                        }
                    }
            }
        }
    } else if ((GET_RACE(ch) == RACE_VAMPIRE) || (GET_RACE(ch) == RACE_GHOUL)) {
        act("You flare your nostrils and inhale deeply.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n's nostrils flare as $e inhales sharply.", TRUE, ch, 0, 0, TO_ROOM);

        dir = rev_dir[(int) weather_info.wind_direction];
        dist = MIN(weather_info.wind_velocity / 10, 3);

        /* not much wind indoors */
        if (IS_SET(ch->in_room->room_flags, RFL_INDOORS))
            dist = 0;

        lstRoom = 0;

        for (i = 0; i <= dist; i++) {
            trRoom = get_room_distance(ch, i, dir);
            if (trRoom && (trRoom != lstRoom)) {
                for (trCh = trRoom->people; trCh; trCh = trCh->next_in_room)
                    if (number(0, (1 + ((wis_app[GET_WIS(ch)].percep) / 5))) && (trCh != ch)
                        && !(IS_IMMORTAL(trCh)) && !(affected_by_spell(ch, SPELL_ETHEREAL))) {
                        sprintf(msgBuf, "You sense %s%s.\n\r", indefinite_article(race[GET_RACE(trCh)].name),
                                race[GET_RACE(trCh)].name);
                        send_to_char(msgBuf, ch);
                    }
            }
        }
    } else {
        act("You sniff the air.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n sniffs the air.", TRUE, ch, 0, 0, TO_ROOM);
    }
}


/* Added 3/10/2001 by Nessalin                                     */
/* This function manages some rules for adding the scent affects   */
/* to people that don't need to be applied to all affects.         */
/* 1. No scent affect should be more than 24 hours (3 days)        */
/* 2. No one should have more than one affect of the same scent at */
/*    the same time.                                               */

void
scent_to_char(CHAR_DATA * ch, int scent, int duration)
{
    struct affected_type af;
    struct affected_type *tmp_af, *next_af;
    int old_duration = 0;

    memset(&af, 0, sizeof(af));

    tmp_af = ch->affected;
    while (tmp_af) {
        if (tmp_af->next)
            next_af = tmp_af->next;
        else
            next_af = 0;

        if (tmp_af->type == scent) {
            old_duration += tmp_af->duration;
            affect_from_char(ch, tmp_af->type);
        }
        tmp_af = next_af;
    }

    af.type = scent;
    af.duration = (MIN(duration + old_duration, 24));
    af.power = 0;
    af.magick_type = 0;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return;
}

/* Added 3/10/2001 by Nessalin                                     */
/* Nothing special, just there for consistency                     */

void
scent_from_char(CHAR_DATA * ch, int scent)
{
    affect_from_char(ch, scent);

    return;
}

void
cmd_sniff(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], arg2[256], strength[50], mess[256];
    OBJ_DATA *spice;
    CHAR_DATA *target;
    int perfumed = 0, type, spiced = 0, smelly = 0;
    struct affected_type *tmp_af, *next_af;
    char *message;
    CHAR_DATA *tmp_ch = NULL;
    int old_def = 0;

    if (!(*argument)) {
        sniff_the_air(ch);
        return;
    }

    argument = one_argument(argument, arg1, sizeof(arg1));


    if ((!(spice = get_obj_in_list_vis(ch, arg1, ch->carrying)))
        && (!(spice = get_obj_in_list_vis(ch, arg1, ch->in_room->contents)))
        && (!(target = get_char_room_vis(ch, arg1))) && (stricmp(arg1, "room"))) {
        send_to_char("What, precisely, are you trying to sniff?\n\r", ch);
        return;
    }

    /* target is the room */

    if (!stricmp(arg1, "room")) {
        message = find_ex_description("[SCENT]", ch->in_room->ex_description, TRUE);
        if (message) {
            send_to_char(message, ch);
            return;
        } else {
            send_to_char("You don't smell anything special.\n\r", ch);
            return;
        }
    }

    /* target is a character */
    if ((target = get_char_room_vis(ch, arg1))) {

        message = find_ex_description("[SCENT]", target->ex_description, TRUE);
        if (message) {
            send_to_char(message, ch);
            return;
        }

        /* Look for perfumes */
        tmp_af = target->affected;
        while (tmp_af) {
            if (tmp_af->next)
                next_af = tmp_af->next;
            else
                next_af = 0;

            if ((tmp_af->type == SCENT_ROSES) || (tmp_af->type == SCENT_MUSK)
                || (tmp_af->type == SCENT_CITRON) || (tmp_af->type == SCENT_MINT)
                || (tmp_af->type == SCENT_CLOVE) || (tmp_af->type == SCENT_SHRINTAL)
                || (tmp_af->type == SCENT_LANTURIN) || (tmp_af->type == SCENT_JASMINE)
                || (tmp_af->type == SCENT_GLIMMERGRASS) || (tmp_af->type == SCENT_LAVENDER)
                || (tmp_af->type == SCENT_BELGOIKISS) || (tmp_af->type == SCENT_PYMLITHE)
                || (tmp_af->type == SCENT_EYNANA) || (tmp_af->type == SCENT_KHEE)
                || (tmp_af->type == SCENT_FILANYA) || (tmp_af->type == SCENT_LADYSMANTLE)
                || (tmp_af->type == SCENT_LIRATHUFAVOR) || (tmp_af->type == SCENT_PFAFNA)
                || (tmp_af->type == SCENT_REDHEART) || (tmp_af->type == SCENT_SWEETBREEZE)
                || (tmp_af->type == SCENT_TEMBOTOOTH) || (tmp_af->type == SCENT_THOLINOC) 
                || (tmp_af->type == SCENT_MEDICHI) || (tmp_af->type == SCENT_JOYLIT)) {
                perfumed = 1;

                if (tmp_af->type == SCENT_ROSES) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strong, ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faint, ");

                    sprintf(mess, "The %ssubtle scent of roses wafts from $S skin.", strength);
                } else if (tmp_af->type == SCENT_MUSK) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strong, sweet and musky");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "sweet, musky");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faint, sweet and musky");

                    sprintf(mess, "A %s aroma clings to $S skin.", strength);
                } else if (tmp_af->type == SCENT_CITRON) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "a strong ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "the pleasant ");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "a faint ");

                    sprintf(mess, "$E has %ssmell of citron.", strength);
                } else if (tmp_af->type == SCENT_SHRINTAL) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "A sweet, fruity scent clings %sto $S skin.", strength);
                } else if (tmp_af->type == SCENT_LANTURIN) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "A heady floral aroma wafts %sfrom $S skin", strength);
                } else if (tmp_af->type == SCENT_GLIMMERGRASS) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strong, ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faint, ");

                    sprintf(mess, "A %ssweet herbal smell surrounds $M.", strength);
                } else if (tmp_af->type == SCENT_BELGOIKISS) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "A piercingly sweet, floral scent comes %sfrom $S skin.",
                            strength);
                } else if (tmp_af->type == SCENT_LAVENDER) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strong ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faint ");

                    sprintf(mess, "The %ssmell of lavender comes from $S skin.", strength);
                } else if (tmp_af->type == SCENT_JASMINE) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "The sweet smell of jasmine wafts %sfrom $M.", strength);
                } else if (tmp_af->type == SCENT_CLOVE) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strong, ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faint, ");

                    sprintf(mess, "A %sspicy scent of clove wafts from $M.", strength);
                } else if (tmp_af->type == SCENT_MINT) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strong, ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faint, ");

                    sprintf(mess, "$E has the %spleasant smell of mint.", strength);
                } else if (tmp_af->type == SCENT_PYMLITHE) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "A dusty floral smell clings %sto $S skin.", strength);
                } else if (tmp_af->type == SCENT_EYNANA) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "The heady scent of sweet vanilla clings %sto $S skin.", strength);
                } else if (tmp_af->type == SCENT_KHEE) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "The strong, aggressive scent of licorice wafts %sfrom $S skin.", strength);
                } else if (tmp_af->type == SCENT_FILANYA) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "A heavy scent like fruit brandy clings %sto $S skin.", strength);
                } else if (tmp_af->type == SCENT_LADYSMANTLE) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "The scent of fresh growth and cloying sweetness %ssurrounds $S skin.", strength);
                } else if (tmp_af->type == SCENT_LIRATHUFAVOR) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "A sweet, fruity scent %sfills your nostrils.", strength);
                } else if (tmp_af->type == SCENT_PFAFNA) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "Citrus scents dance %sthrough the air, chased by sly hints of mint.", strength);
                } else if (tmp_af->type == SCENT_REDHEART) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "A thick smell of acrid sweetness akin to burning honey %sfills the air.", strength);
                } else if (tmp_af->type == SCENT_SWEETBREEZE) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "A faint, perfumy fragrance clings %sto $S skin.", strength);
                } else if (tmp_af->type == SCENT_TEMBOTOOTH) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "A rich, peppery scent %sfills the air, underscored by sweet anise.", strength);
                } else if (tmp_af->type == SCENT_THOLINOC) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strongly ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faintly ");

                    sprintf(mess, "The bright smell of fresh almonds %sfills the air.", strength);
                
                } else if (tmp_af->type == SCENT_MEDICHI) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strong, ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faint, ");

                    sprintf(mess, "A %sdeeply complex floral scent wafts from $S skin. Citrus, mint and blue flowers top a rich, woody scent and the calming hint of lavender.", strength);
                } else if (tmp_af->type == SCENT_JOYLIT) {
                    if ((tmp_af->duration < 25) && (tmp_af->duration > 17))
                        strcpy(strength, "strong, ");
                    else if ((tmp_af->duration < 18) && (tmp_af->duration > 6))
                        strcpy(strength, "");
                    else if ((tmp_af->duration < 7))
                        strcpy(strength, "faint, ");

                    sprintf(mess, "A %ssmell of a light, giddy blend of citron and joylit wafts from $S skin. The aroma inexplicably carrying with it a sparkling sensation of happiness.", strength);
                }
                act(mess, FALSE, ch, 0, target, TO_CHAR);
            }

            tmp_af = next_af;
        }

        if (!perfumed) {
            if (affected_by_spell(target, SPICE_METHELINOC)
                || affected_by_spell(target, SPICE_THODELIV)
                || affected_by_spell(target, SPICE_QEL)
                || affected_by_spell(target, SPICE_MELEM_TUEK)
                || affected_by_spell(target, SPICE_KRELEZ)
                || affected_by_spell(target, SPICE_KEMEN)
                || affected_by_spell(target, SPICE_AGGROSS)
                || affected_by_spell(target, SPICE_KRENTAKH)
                || affected_by_spell(target, SPICE_ZHARAL))
                spiced = 1;

            if (affected_by_spell(target, SPICE_METHELINOC))
                act("$E has a smoky, dusty aroma, redolent of the desert.", FALSE, ch, 0, target,
                    TO_CHAR);
            if (affected_by_spell(target, SPICE_THODELIV))
                act("A rich, heavy sweet scent wafts from $S skin.", FALSE, ch, 0, target, TO_CHAR);
            if (affected_by_spell(target, SPICE_QEL))
                act("A curious, mossy smell clings to $S skin.", FALSE, ch, 0, target, TO_CHAR);
            if (affected_by_spell(target, SPICE_MELEM_TUEK))
                act("A sweet, musky smell clings to $M.", FALSE, ch, 0, target, TO_CHAR);
            if (affected_by_spell(target, SPICE_KRELEZ))
                act("A heavy, bitter odor wafts from $S skin.", FALSE, ch, 0, target, TO_CHAR);
            if (affected_by_spell(target, SPICE_KRENTAKH))
                act("A sweet, floral tinge clings faintly to $S skin.", FALSE, ch, 0, target,
                    TO_CHAR);
            if (affected_by_spell(target, SPICE_KEMEN))
                act("A sharp, almost bitter scent clings to $S skin.", FALSE, ch, 0, target,
                    TO_CHAR);
            if (affected_by_spell(target, SPICE_AGGROSS))
                act("A bitter tang comes from $S skin.", FALSE, ch, 0, target, TO_CHAR);
            if (affected_by_spell(target, SPICE_ZHARAL))
                act("A sweet, dusty smell clings to $S skin.", FALSE, ch, 0, target, TO_CHAR);
        }

        if (!perfumed && !spiced) {
            if (affected_by_spell(target, SCENT_DOODIE)) {
                smelly = 1;
                act("$E is surrounded by a horrible, pungent odor.", FALSE, ch, 0, target, TO_CHAR);
            }
        }

        if (!perfumed && !spiced && !smelly) {
            if (is_char_drunk(target) > DRUNK_MEDIUM) {
                act("$E reeks of alcohol.", FALSE, ch, 0, target, TO_CHAR);
            } else if (IS_SET(target->specials.act, CFL_UNDEAD)) {
                act("$E reeks of death and decay.", FALSE, ch, 0, target, TO_CHAR);
            } else
                act("$E smells like dust and sweat.", FALSE, ch, 0, target, TO_CHAR);
        }
        return;
    }

    if (!spice) {
        errorlog("SPICE NOT FOUND.");
        return;
    }

    if (spice->obj_flags.type != ITEM_SPICE) {
        message = find_ex_description("[SCENT]", spice->ex_description, TRUE);
        if (message) {
            send_to_char(message, ch);
            return;
        } else {
            send_to_char("You don't smell anything special.\n\r", ch);
            return;
        }
    }

    if (spice->obj_flags.value[2]) {
        send_to_char("You have to smoke that.\n\r", ch);
        return;
    }

    type = spice->obj_flags.value[1];

    if ((type < 0) || (type > MAX_SKILLS) || (skill[type].sk_class != CLASS_SPICE)) {
        send_to_char("You sniff the spice but nothing seems to happen.\n\r", ch);
        gamelogf("Spice has bad spice num of %d", type);
        return;
    }

    argument = one_argument(argument, arg2, sizeof(arg2));
    if (!strcmp(arg2, "unaddict") && IS_IMMORTAL(ch))
        if (ch->skills[type])
            remove_skill_taught(ch, type);


    /*  If trying to sniff a knot etc...   */
    if (spice->obj_flags.value[0] > 5) {
        send_to_char("Suicidal?  Try shaving off a smaller pinch.\n\r", ch);
        return;
    }
    // Shamelessly cut & pasted from cmd_get() with a few modifications
    if (GET_POS(ch) == POSITION_FIGHTING) {
        for (tmp_ch = ch->in_room->people; tmp_ch; tmp_ch = tmp_ch->next)
            /* ch might be null if the player has been killed */
            if ((ch) && (GET_POS(tmp_ch) == POSITION_FIGHTING)
                && !trying_to_disengage(tmp_ch)
                && (tmp_ch->specials.fighting == ch || tmp_ch->specials.alt_fighting == ch)) {
                stop_fighting(tmp_ch, ch);

                act("$n attacks as you stop fighting to inhale $p.", FALSE, tmp_ch, spice, ch,
                    TO_VICT);
                act("You attack $N as $E stops fighting to inhale $p.", FALSE, tmp_ch, spice, ch,
                    TO_CHAR);
                act("$n attacks $N as $E stops fighting to inhale $p.", FALSE, tmp_ch, spice, ch,
                    TO_NOTVICT);
                old_def = (int) ch->abilities.def;
                ch->abilities.def -= 25;
                ch->abilities.def = (sbyte) MAX(0, ch->abilities.def - 25);

                if (!hit(tmp_ch, ch, TYPE_UNDEFINED)) { /* they bees dying, deadlike */
                    return;
                }
                hit(tmp_ch, ch, TYPE_UNDEFINED);
                if (ch)
                    ch->abilities.def = (sbyte) old_def;
            }
    } else {
        act("$n brings $p up to $s nose and inhales deeply.", TRUE, ch, spice, 0, TO_ROOM);
        act("You bring $p up to your nose and inhale deeply.", TRUE, ch, spice, 0, TO_CHAR);
    }

    /* check for lacing on the spice */
    if (spice->obj_flags.value[3] && (spice->obj_flags.value[3] <= MAX_POISON)) {
        poison_char(ch, spice->obj_flags.value[3], number(1, 2), 0);
        gamelog("Poisoning char from spice.");
    }

    ((*skill[spice->obj_flags.value[1]].function_pointer)
     (spice->obj_flags.value[0], ch, "", SPELL_TYPE_SPICE, ch, 0));

    extract_obj(spice);
}

void
cmd_smoke(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char msg[256];
    OBJ_DATA *smoke;
    int type;

    if ((!ch->equipment[ES]) || (ch->equipment[ES]->obj_flags.type != ITEM_SPICE)) {
        send_to_char("You have to hold a cigarette or pipe to smoke.\n\r", ch);
        return;
    } else
        smoke = ch->equipment[ES];

    if ((smoke->obj_flags.type != ITEM_SPICE) || (!smoke->obj_flags.value[2])) {
        cprintf(ch, "You can't smoke the %s.\n\r", OSTR(smoke, short_descr));
        return;
    }

    type = smoke->obj_flags.value[1];

    if ((type < MIN_SPICE) || (type > MAX_SPICE)) {
        send_to_char("You puff at the spice but nothing seems to happen.\n\r", ch);
        gamelogf("Smoke has bad spice num of %d", type);
        return;
    }

    sprintf(msg, "A thin trail of %s smoke trickles from $n's mouth as $e " "smokes $p.",
            spice_odor[type - MIN_SPICE]);
    act(msg, TRUE, ch, smoke, 0, TO_ROOM);
    act("You bring $p up to your mouth and inhale deeply.", TRUE, ch, smoke, 0, TO_CHAR);

    if (smoke->obj_flags.value[3] && (smoke->obj_flags.value[3] <= MAX_POISON))
        poison_char(ch, smoke->obj_flags.value[3], number(1, 2), 0);

    ((*skill[smoke->obj_flags.value[1]].function_pointer)
     (1, ch, "", SPELL_TYPE_SPICE, ch, smoke));
    if (smoke->obj_flags.value[0] <= 1) {
        cprintf(ch, "The last spark of %s dies as you take your last puff.\n\r",
                OSTR(smoke, short_descr));
        extract_obj(smoke);
    } else
        smoke->obj_flags.value[0]--;

    return;
}

void
remove_page(OBJ_DATA * note)
{
    char buf[16];

    sprintf(buf, "page_%d", note->obj_flags.value[0]);

    if (!(note->obj_flags.value[0] -= 1)) {
        extract_obj(note);
        return;
    }

    note->obj_flags.cost =
        (note->obj_flags.cost * note->obj_flags.value[0]) / (note->obj_flags.value[0] + 1);

    rem_obj_extra_desc_value(note, buf);
    return;
}

void
make_smoke(CHAR_DATA * ch, const char *argument, int cmd)
{
    char paper[80], spice[80];

    OBJ_DATA *p_obj, *sp_obj, *sm_obj;

    argument = two_arguments(argument, paper, sizeof(paper), spice, sizeof(spice));
    if (!*paper || !*spice) {
        send_to_char("Usage: 'make smoke <paper> <spice>'.\n\r", ch);
        return;
    }

    if (!(p_obj = get_obj_in_list(ch, paper, ch->carrying))) {
        send_to_char("You are not carrying any paper of that sort.\n\r", ch);
        return;
    }
    if (!(sp_obj = get_obj_in_list(ch, spice, ch->carrying))) {
        send_to_char("You are not carrying any spice of that sort.\n\r", ch);
        return;
    }

    if (p_obj->obj_flags.type != ITEM_NOTE) {
        cprintf(ch, "You cannot roll the spice into %s.\n\r", OSTR(p_obj, short_descr));
        return;
    }
    if (p_obj->obj_flags.value[0] <= 0) {
        cprintf(ch, "There aren't any pages left in %s.\n\r", OSTR(p_obj, short_descr));
        return;
    }

    if ((sp_obj->obj_flags.type != ITEM_SPICE) || (sp_obj->obj_flags.value[1] == SPICE_MOSS)
        || (sp_obj->obj_flags.value[2])) {
        cprintf(ch, "You cannot roll %s into the paper.", OSTR(sp_obj, short_descr));
        return;
    }
    if (sp_obj->obj_flags.value[0] > 5) {
        send_to_char("There is too much spice to roll into the paper.\n\r"
                     "You'll have to shave off a bit first.\n\r", ch);
        return;
    }


    sm_obj = read_object(64150 + sp_obj->obj_flags.value[1], VIRTUAL);
    if (!sm_obj)
        return;

    sm_obj->obj_flags.value[0] = sp_obj->obj_flags.value[0];
    sm_obj->obj_flags.cost = (sp_obj->obj_flags.cost * 3) / 2;
    sm_obj->obj_flags.value[3] = sp_obj->obj_flags.value[3];

    act("You carefully roll a pinch of spice with $p.", FALSE, ch, p_obj, 0, TO_CHAR);
    act("$n carefully rolls a pinch of spice with $p.", FALSE, ch, p_obj, 0, TO_ROOM);

    remove_page(p_obj);
    extract_obj(sp_obj);

    obj_to_char(sm_obj, ch);

    return;
}

void
cmd_shave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *brick;
    OBJ_DATA *pinch;

    int pcost = 0;              /* cost of pinch */
    int spice_type = 0;         /* meth, melem, etc.  */

    if ((!ch->equipment[ES]) || (ch->equipment[ES]->obj_flags.type != ITEM_SPICE)) {
        act("Hold the spice from which you wish to shave a pinch.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    } else
        brick = ch->equipment[ES];

    if (brick->obj_flags.value[0] < 5) {
        act("That's too small to break up any further.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /*  Pinches will start at 20201  */
    spice_type = (brick->obj_flags.value[1] + 20200);
    if (spice_type < MINPINCH || spice_type > MAXPINCH) {
        send_to_char("Error in spice code, contact Kelvik.\n\r", ch);
        gamelog("Bad pinch number generated in cmd_shave.");
        return;
    }

    pinch = read_object(spice_type, VIRTUAL);

    pcost = ((brick->obj_flags.cost / brick->obj_flags.value[0]) * 3);
    pinch->obj_flags.cost = pcost;

    brick->obj_flags.cost -= pcost;
    brick->obj_flags.value[0] -= 3;

    act("You shave a bit off of $p.", FALSE, ch, brick, 0, TO_CHAR);
    act("$n carefully shaves a pinch from $p.", FALSE, ch, brick, 0, TO_ROOM);

    obj_to_char(pinch, ch);

}

void
update_detox(CHAR_DATA * ch, struct affected_type *af)
{

    if (af->duration >= 1) {
        return;
    }

    if ((af->type < MIN_SPICE) || (af->type > MAX_SPICE)) {
        gamelogf("Detox has illegal spice type of number %d", af->type);
        affect_remove(ch, af);
        return;
    }

    if (!ch->skills[af->type]) {
        gamelogf("Detoxing character does not have addiction %s", skill_name[af->type]);
        affect_remove(ch, af);
        return;
    }

    /* decrease addiction by one */
    --(ch->skills[af->type]->learned);

    /* if addiction is over, remove the affect */
    if (ch->skills[af->type]->learned <= 1) {
        affect_remove(ch, af);
        return;
    }
    // Another day's worth of detox, reset the timestamps
    af->duration = 9;
    af->initiation = time(NULL);
    af->expiration = af->initiation + (RT_ZAL_HOUR * af->duration) - 1;

    if (((ch->skills[af->type]->rel_lev - 74) <= (75 - ch->skills[af->type]->learned)))
        remove_skill_taught(ch, af->type);

    /* if we have gone below a mutlitple of ten, notch the appropriate
     * stat(s) */
    if ((ch->skills[af->type]->learned % 10) == 9)
        switch (af->type) {
        case (SPICE_METHELINOC):
            SET_WIS(ch, GET_WIS(ch) + 1);
            ++ch->abilities.wis;
            break;
        case (SPICE_MELEM_TUEK):
            SET_WIS(ch, GET_STR(ch) + 1);
            ++ch->abilities.str;
            break;
        case (SPICE_KRELEZ):
            SET_AGL(ch, GET_AGL(ch) + 1);
            ++ch->abilities.agl;
            break;
        case (SPICE_KEMEN):
            ++GET_MAX_HIT_STAT(ch);
            ch->points.max_hit += 4;;
            break;
        case (SPICE_AGGROSS):
            SET_END(ch, GET_END(ch) + 1);
            ++ch->abilities.end;
            break;
        case (SPICE_ZHARAL):
            ++GET_DEF_STAT(ch);
            break;
        case (SPICE_KRENTAKH):
            /* I _THINK_ this is the cause of the krentakh endurance boosting
             * bug.  We only decrease the PC's END stuff when their learned level
             * in the skill crosses the threshold of a multiple of 20.  (ie. If
             * their previous level was 18 and it goes to 21, we drop END).  But,
             * here we were boosting their endurance whenever they crossed a
             * threshold of a multiple of 10.  The problem is the level % 20 == 9.
             * A PC can achieve a learned level between 10 and 20 (say 14) and then
             * drop back down to 9.  Thus, level % 20 == 9, and we boost the stat.
             * However, the PC never crossed the 20 threshold to begin with, so
             * we never reduced the END at all.  Therefore, we just gave them a 
             * permanent increase.  If we change the modulo to 19, we can avoid
             * this problem as we're guaranteed to only boost when we know we just
             * crossed a threshold of 20. -- 11/20/00 Tiernan */
            /* if ((ch->skills[af->type]->learned % 20) == 9) */
            if ((ch->skills[af->type]->learned % 20) == 19) {
                SET_END(ch, GET_END(ch) + 1);
                ++ch->abilities.end;
            }
            break;
        default:
            break;
        }

    return;
}

void
update_spice(CHAR_DATA * ch, struct affected_type *af)
{
    int sk = af->type;

    if (af->location == CHAR_APPLY_DETOX) {
        update_detox(ch, af);
        return;
    }

    if (af->duration >= 1) {
        if (!(af->duration % 3)) {
            // Out with the old
            affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);
            af->modifier--;
            // In with the new
            affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
        }
        // A good place to add an echo to the player about what they're on
        send_spice_affect_msg(ch, sk);
    } else {
        affect_remove(ch, af);
        if (!spice_in_system(ch, sk)) {
            send_to_char(spell_wear_off_msg[sk], ch);
        }
    }

    return;
}

int
in_spice_wd(CHAR_DATA * ch)
{

    int i;

    if (IS_IMMORTAL(ch) && !IS_SET(ch->specials.act, CFL_UNIQUE))
        return 0;

    for (i = MIN_SPICE; i <= MAX_SPICE; i++)
        if ((ch->skills[i]) && ((is_skill_taught(ch, i)) &&
                                /* ((ch->skills[i]->taught) && */
                                (!spice_in_system(ch, i))))
            return 1;

    return 0;
}

double
spice_factor(CHAR_DATA * ch)
{

    int i;
    int power = 0;
    double fac = 1;

    if (IS_IMMORTAL(ch) && !IS_SET(ch->specials.act, CFL_UNIQUE))
        return 1;

    for (i = MIN_SPICE; i <= MAX_SPICE; i++) {
        if (ch->skills[i])
            if (is_skill_taught(ch, i))
                /* if (ch->skills[i]->taught) */
                if (!spice_in_system(ch, i))
                    power += ((ch->skills[i]->rel_lev - 74) - abs(ch->skills[i]->learned - 75));
    }

    for (i = power; i > 0; i--) {
        fac *= .85;
    }

    return fac;
}

void
gain_spice_skill(CHAR_DATA * ch, int sk, int gain)
{

    if (!ch->skills[sk])
        init_skill(ch, sk, 0);

    /* if the character has been this addicted before, double the
     * addiction affect */
    if (ch->skills[sk]->learned < ch->skills[sk]->rel_lev)
        gain *= 2;

    /* increase the addiction by gain points */
    ch->skills[sk]->learned += gain;
    ch->skills[sk]->learned = MIN(ch->skills[sk]->learned, 100);

    if (ch->skills[sk]->learned > 75)
        set_skill_taught(ch, sk);
    /* ch->skills[sk]->taught = 1; */

    ch->skills[sk]->last_gain = time(0);
    ch->skills[sk]->rel_lev = MAX(ch->skills[sk]->rel_lev, ch->skills[sk]->learned);

    return;
}

/* Return if a char is affected by a spice (SPICE_XXX)
 * returns the affect if found, otherwise NULL indicates not affected 
 */
struct affected_type *
affected_by_spice(CHAR_DATA * ch, int spice)
{
    struct affected_type *aff_node = NULL;

    if (ch->affected)
        for (aff_node = ch->affected; aff_node; aff_node = aff_node->next)
            if ((aff_node->type == spice) && (aff_node->location != CHAR_APPLY_DETOX))
                break;

    return (aff_node);
}

/* send_spice_affect_msg()
 *
 * Sends a spice affect message
 */
void
send_spice_affect_msg(CHAR_DATA * ch, int spice)
{
    char *msg = NULL;

    switch (spice) {
    case SPICE_METHELINOC:
        msg = spice_methelinoc_msg();
        break;
    case SPICE_MELEM_TUEK:
        msg = spice_melem_tuek_msg();
        break;
    case SPICE_KRELEZ:
        msg = spice_krelez_msg();
        break;
    case SPICE_KEMEN:
        msg = spice_kemen_msg();
        break;
    case SPICE_AGGROSS:
        msg = spice_aggross_msg();
        break;
    case SPICE_ZHARAL:
        msg = spice_zharal_msg();
        break;
    case SPICE_THODELIV:
        msg = spice_thodeliv_msg();
        break;
    case SPICE_KRENTAKH:
        msg = spice_krentakh_msg();
        break;
    case SPICE_QEL:
        msg = spice_qel_msg();
        break;
    case SPICE_MOSS:
        msg = spice_moss_msg();
        break;
    default:
        break;
    }
    if (msg && strlen(msg))
        cprintf(ch, "%s\n\r", msg);
}

/* spice_methelinoc_msg()
 *
 * Return a msg for the affects of methelinoc
 */
char *
spice_methelinoc_msg()
{
    static char *messages[] = {
        ""
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

/* spice_melem_tuek_msg()
 * 
 * Return a msg for the affects of melem tuek
 */
char *
spice_melem_tuek_msg()
{
    static char *messages[] = {
        ""
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

/* spice_krelez_msg()
 *
 * Return a msg for the affects of krelez
 */
char *
spice_krelez_msg()
{
    static char *messages[] = {
        "You glimpse a shadow of movement out of the corners of your eyes.",
        "Alert wariness floods your senses.",
        "It becomes nearly unbearable to stay still for more than a few seconds.",
        "You feel a rush of jittery energy and a strong urge to keep your body in motion.",
        "You feel nimble and clear-headed."
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

/* spice_kemen_msg()
 *
 * Return a msg for the affects of kemen
 */
char *
spice_kemen_msg()
{
    static char *messages[] = {
        ""
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

/* spice_aggross_msg()
 *
 * Return a msg for the affects of aggross
 */
char *
spice_aggross_msg()
{
    static char *messages[] = {
        "You feel your heart pounding solidly in your chest.",
        "A tireless sensation lingers in your body as if you could run for miles without stopping for breath.",
        "A brief shiver runs up your spine as your body is flooded with a surge of energy.",
        "Your jawline tenses as you feel an energetic rush.",
        "A burst of energy runs through you."
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

/* spice_zharal_msg()
 *
 * Return a msg for the affects of zharal
 */
char *
spice_zharal_msg()
{
    static char *messages[] = {
        "Your muscles flex sporadically and your skin feels hot.",
        "With each pulse of your blood through your veins you feel a continuing surge of aggressive confidence.",
        "A stalking, predatory feeling overtakes you as if you are among tantalizing prey.",
        "You feel a surge of aggression and the need to release it on someone or something.",
        "A visceral sensation of power and physical prowess tingles through your body.",
        "Your skin prickles and you feel a surge of paranoid anger."
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

/* spice_thodeliv_msg()
 *
 * Return a msg for the affects of thodeliv
 */
char *
spice_thodeliv_msg()
{
    static char *messages[] = {
        ""
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

/* spice_krentakh_msg()
 *
 * Return a msg for the affects of krentakh
 */
char *
spice_krentakh_msg()
{
    static char *messages[] = {
        ""
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

/* spice_qel_msg()
 *
 * Return a msg for the affects of qel
 */
char *
spice_qel_msg()
{
    static char *messages[] = {
        ""
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

/* spice_moss_msg()
 *
 * Return a msg for the affects of moss
 */
char *
spice_moss_msg()
{
    static char *messages[] = {
        ""
    };
    return messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)];
}

