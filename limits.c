/*
 * File: LIMITS.C
 * Usage: Routines for periodic game updates, etc.
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

/* Revision History:
 * 05/16/2000 Added more messages to skellebaine.  -San
 * 10/08/2000 Made death from terradin give gamelog mesg & store in 'objective' field -Savak
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "constants.h"
#include "structs.h"
#include "limits.h"
#include "utils.h"
#include "skills.h"
#include "comm.h"
#include "guilds.h"
#include "vt100c.h"
#include "parser.h"
#include "utility.h"
#include "cities.h"
#include "handler.h"
#include "creation.h"
#include "barter.h"
#include "memory.h"
#include "object_list.h"
#include "psionics.h"
#include "watch.h"
#include "spice.h"
 
#define MAX_VAGE_CLASS  7
#define NESS_HUNGER 1

extern void sflag_to_char(CHAR_DATA * ch, long sflag);
void sflag_to_obj(OBJ_DATA * obj, long sflag);

 
/* local variables */
int year_limit[] = { 0, 19, 29, 40, 60, 80, 120 };


/* initialize a 'learned' skill, like ride, gather or watch 
 *
 * ch - Character to initialize
 * skill - skill # to give them
 * initialSkill - what to set the starting learned value as
 */
bool
init_learned_skill(CHAR_DATA * ch, int skill, int initialSkill)
{
    /* if they don't have the skill */
    if (!has_skill(ch, skill)) {
        if (number(0, 100) <= wis_app[GET_WIS(ch)].learn) {
            init_skill(ch, skill, initialSkill);

            /* if they don't normally get the skill, set it as taught */
            if (get_max_skill(ch, skill) <= 0)
                set_skill_taught(ch, skill);
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

void
delete_skill(CHAR_DATA * ch, int sk)
{
    if (!ch)
        return;
    if ((sk < 0) || (sk > MAX_SKILLS))
        return;
    if (has_skill(ch, sk))
        free((char *) ch->skills[sk]);
    ch->skills[sk] = (struct char_skill_data *) 0;
}

void
init_skill(CHAR_DATA * ch, int sk, int perc)
{
    if (perc < 0)
        return;

    if ((sk < 0) || (sk > MAX_SKILLS))
        return;

    CREATE(ch->skills[sk], struct char_skill_data, 1);
    ch_mem += sizeof(struct char_skill_data);
    ch->skills[sk]->learned = perc;
    ch->skills[sk]->taught_flags = 0;
    ch->skills[sk]->last_gain = 0;
    ch->skills[sk]->rel_lev = 0;

    switch (sk) {
    case TOL_ALCOHOL:
    case TOL_PAIN:
    case SKILL_WILLPOWER:
    case PSI_WILD_CONTACT:
    case PSI_WILD_BARRIER:
    case LANG_RINTH_ACCENT:
    case LANG_NORTH_ACCENT:
    case LANG_SOUTH_ACCENT:
    case LANG_ANYAR_ACCENT:
    case LANG_BENDUNE_ACCENT:
    case LANG_WATER_ACCENT:
    case LANG_FIRE_ACCENT:
    case LANG_WIND_ACCENT:
    case LANG_EARTH_ACCENT:
    case LANG_SHADOW_ACCENT:
    case LANG_LIGHTNING_ACCENT:
    case LANG_NILAZ_ACCENT:
    case LANG_DESERT_ACCENT:
    case LANG_EASTERN_ACCENT:
    case LANG_WESTERN_ACCENT:
        set_skill_hidden(ch, sk);
        break;
    default:
        break;
    }
}

void
init_psi_skills(CHAR_DATA * ch)
{
    if (!has_skill(ch, SKILL_WILLPOWER))
        /* if (!ch->skills[SKILL_WILLPOWER]) */
    {
        init_skill(ch, SKILL_WILLPOWER, 5);

        set_skill_taught(ch, SKILL_WILLPOWER);
        /* ch->skills[SKILL_WILLPOWER]->taught = TRUE; */
    }

    if (!has_skill(ch, PSI_CONTACT))
        /* if (!ch->skills[PSI_CONTACT]) */
    {
        init_skill(ch, PSI_CONTACT, 80);
        set_skill_taught(ch, PSI_CONTACT);
        /*      ch->skills[PSI_CONTACT]->taught = TRUE; */
    }
    if (!has_skill(ch, PSI_BARRIER))
        /* if (!ch->skills[PSI_BARRIER]) */
    {
        init_skill(ch, PSI_BARRIER, 20);
        set_skill_taught(ch, PSI_BARRIER);
        /* ch->skills[PSI_BARRIER]->taught = TRUE; */
    }
    // NPCs get their max levels for these
    if (IS_NPC(ch)) {
        increase_or_give_char_skill(ch, SKILL_WILLPOWER, wis_app[GET_WIS(ch)].learn);
        increase_or_give_char_skill(ch, PSI_CONTACT, 80);
        increase_or_give_char_skill(ch, PSI_BARRIER, 80);
    }
}

void
init_racial_skills(CHAR_DATA * ch)
{
    int new_skill[3] = { 0, 0, 0 };
    int i;

    for (i = 0; i < 3; i++)
        new_skill[i] = 0;

    switch (GET_RACE(ch)) {
    case RACE_ELF:
        new_skill[0] = SKILL_STEAL;
        new_skill[1] = SKILL_HAGGLE;
        break;
    case RACE_HALFLING:
        new_skill[0] = SKILL_SNEAK;
        new_skill[1] = SKILL_HIDE;
        new_skill[2] = SKILL_CLIMB;
        break;
    case RACE_MANTIS:
        new_skill[0] = SKILL_HIDE;
        break;
    case RACE_DESERT_ELF:
        new_skill[0] = SKILL_SNEAK;
        new_skill[1] = SKILL_HIDE;
    default:
        break;
    }

    for (i = 0; i < 3; i++) {
        if (new_skill[i]) {
            if (!has_skill(ch, new_skill[i]))
                /* if (!ch->skills[new_skill[i]]) */
            {
                init_skill(ch, new_skill[i], 20);
                set_skill_taught(ch, new_skill[i]);
                /* ch->skills[new_skill[i]]->taught = TRUE; */
            } else
                ch->skills[new_skill[i]]->learned += 10;
        }
    }
};

int
get_skill_wait_time(CHAR_DATA *ch, int sk ) {
    int wt = 0;

    wt = 2 * MAX(8, 60 - wis_app[GET_WIS(ch)].learn 
     + (ch->skills[sk]->learned / 7));

    if ((skill[sk].sk_class == CLASS_COMBAT) 
     || (skill[sk].sk_class == CLASS_WEAPON))
        wt *= 2;

     if (skill[sk].sk_class == CLASS_PSIONICS)
        wt *= 3;

    return wt;
}

void
gain_skill(CHAR_DATA * ch, int sk, int gain)
{
    long t;
    /*  char msg[MAX_STRING_LENGTH]; */
    int max, raw, diff, i, wt, chance;

    if (IS_NPC(ch))
        return;

    if ((sk < 0) || (sk >= MAX_SKILLS)) {
        gamelogf("Attempted to change skill not between 0 and %d", MAX_SKILLS - 1);
        return;
    }

    if (has_skill(ch, sk) && is_skill_nogain(ch, sk)) {
		
        return;
    }

    if (skill[sk].sk_class == CLASS_SPICE) {
        gain_spice_skill(ch, sk, gain);
        return;
    }

    if (skill[sk].sk_class == CLASS_TOLERANCE) {
        gain_tolerance_skill(ch, sk, gain);
        return;
    }

    if (!has_skill(ch, sk))
        return;

    if (skill[sk].sk_class == CLASS_LANG)
        return;

    /* Can't learn if you're mindwiped, or hallucinating */
    if (affected_by_spell(ch, PSI_MINDWIPE))
        return;
    if (affected_by_spell(ch, POISON_SKELLEBAIN))
        return;
    if (affected_by_spell(ch, POISON_SKELLEBAIN_2))
        return;
    if (affected_by_spell(ch, POISON_SKELLEBAIN_3))
        return;


    chance = 100;
    switch (is_char_drunk(ch)) {
    case 0:
        break;
    case DRUNK_LIGHT:
        chance = 80;
        break;
    case DRUNK_MEDIUM:
        chance = 70;
        break;
    case DRUNK_HEAVY:
        chance = 50;
        break;
    case DRUNK_SMASHED:
        chance = 30;
        break;
    case DRUNK_DEAD:
        chance = 5;
        break;
    }
    if (chance < number(1, 100))
        return;

    // Has the character waited long enough?
    wt = get_skill_wait_time(ch, sk);

    if (((t = time(0)) - ch->skills[sk]->last_gain) < (SECS_PER_REAL_MIN * wt))
        return;

    ch->skills[sk]->last_gain = t;
    int oldsk = ch->skills[sk]->learned;

    if ((GET_GUILD(ch) == GUILD_TEMPLAR || GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR
         || GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)
        && skill[sk].sk_class != CLASS_PSIONICS && skill[sk].sk_class != CLASS_REACH) {
        ch->skills[sk]->learned = MIN(ch->skills[sk]->learned + gain, 90);
        // I think this is causing the 3 templar guilds not to proceed to
        // a point where their skills can branch. - Tiernan 11/07/2003
        // return;
    }

    max = get_max_skill(ch, sk);
    raw = get_raw_skill(ch, sk);
    diff = get_skill_percentage(ch, sk) - raw;

    /* all magickers can get to 100% in all reach skills */
    if (skill[sk].sk_class == CLASS_REACH && is_magicker(ch)) {
        max = 100;
    }

    if (max <= 0 && is_skill_taught(ch, sk)) 
        max = get_innate_max_skill(ch, sk);

    raw = MIN(raw + gain, max);
    ch->skills[sk]->learned = raw + diff;

    if (max <= 0 && raw <= 0) {
        free((char *) ch->skills[sk]);
        ch->skills[sk] = (struct char_skill_data *) 0;
        return;
    }

 //   if (oldsk != ch->skills[sk]->learned)
        gamelogf("gain_skill: %s gained %d points of skill in '%s' (%d => %d), max is %d", MSTR(ch, name), gain, skill_name[sk],
                oldsk, ch->skills[sk]->learned, max);

    // See if the branched any new skills from the gain.
    if (raw >= guild[(int) ch->player.guild].skill_max_learned[sk] - 10) {
        for (i = 0; i < MAX_SKILLS; i++)
            if (guild[(int) ch->player.guild].skill_prev[i] == sk)
                if (!has_skill(ch, i))
                    init_skill(ch, i, guild[(int) ch->player.guild].skill_percent[i]);
    }

    // And now look for subguild branches. -- Tiernan 12/22/2004
    if (raw >= sub_guild[(int) ch->player.sub_guild].skill_max_learned[sk] - 10) {
        for (i = 0; i < MAX_SKILLS; i++)
            if (sub_guild[(int) ch->player.sub_guild].skill_prev[i] == sk)
                if (!has_skill(ch, i))
                    init_skill(ch, i, sub_guild[(int) ch->player.sub_guild].skill_percent[i]);
    }

    // Added for charge skill and half-elves & rangers -- Tiernan 6/6/2003
    if (sk == SKILL_RIDE) {
        if((GET_RACE(ch) == RACE_HALF_ELF || GET_GUILD(ch) == GUILD_RANGER)) {
            if (raw >= 55 && !has_skill(ch, SKILL_CHARGE)) {
                init_skill(ch, SKILL_CHARGE, 15);
                set_skill_taught(ch, SKILL_CHARGE);
            }
        }

        if (raw >= 50 && !has_skill(ch, SKILL_TRAMPLE)) {
            init_skill(ch, SKILL_TRAMPLE, 10);
            set_skill_taught(ch, SKILL_TRAMPLE);
        }
    }
}                               /* end gain_skill */

int
is_psi_skill(int sk)
{
    if (skill[sk].sk_class == CLASS_PSIONICS)
        return 1;

    return 0;
}

void
init_saves(CHAR_DATA * ch)
{
    int n, j;

    switch (GET_RACE(ch)) {
    case RACE_HUMAN:
        j = 1;
        break;
    case RACE_ELF:
    case RACE_DESERT_ELF:
        j = 2;
        break;
    case RACE_DWARF:
        j = 3;
        break;
    case RACE_HALFLING:
        j = 4;
        break;
    case RACE_HALF_GIANT:
        j = 5;
        break;
    case RACE_HALF_ELF:
        j = 6;
        break;
    case RACE_MUL:
        j = 7;
        break;
    case RACE_MANTIS:
        j = 8;
        break;
    case RACE_GITH:
        j = 9;
        break;
    case RACE_DRAGON:
    case RACE_SUB_ELEMENTAL:
    case RACE_ELEMENTAL:
    case RACE_AVANGION:
        j = 10;
        break;
    default:
        j = 0;
        break;
    }

    for (n = 0; n < 5; n++) {
        ch->specials.apply_saving_throw[n] = saving_throw[j][n];
        if ((n == SAVING_PARA) || (n == SAVING_PETRI))
            ch->specials.apply_saving_throw[n] += end_app[GET_END(ch)].end_saves;
        if (n == SAVING_ROD)
            ch->specials.apply_saving_throw[n] += agl_app[GET_AGL(ch)].reaction;
    }
}


int
get_native_language(CHAR_DATA * ch)
{
    int n, lang, max;

    max = 0;
    lang = 0;
    for (n = 301; n < MAX_SKILLS; n++) {
        if ((skill[n].sk_class == CLASS_LANG) && has_skill(ch, n))
            /*        (ch->skills[n])) */
        {
            if ((ch->skills[n]->learned) > max) {
                max = ch->skills[n]->learned;
                lang = n;
            }
        }
    }
    return lang;
}

void
init_languages(CHAR_DATA * ch)
{
    int check, primary = 0;

    for (check = 0; check < MAX_TONGUES; check++)
        if (has_skill(ch, language_table[check].spoken))
            return;

    switch (GET_RACE(ch)) {
    case RACE_HALF_ELF:
	primary = LANG_COMMON;
        init_skill(ch, LANG_ELVISH, 60);
        break;
    case RACE_MUL:
	primary = LANG_COMMON;
	init_skill(ch, LANG_DWARVISH, 100);
        break;
    case RACE_ELF:
	primary = LANG_ELVISH;
	init_skill(ch, LANG_COMMON, 100);
        break;
    case RACE_DESERT_ELF:
	primary = LANG_ELVISH;
        break;
    case RACE_DWARF:
	primary = LANG_DWARVISH;
	init_skill(ch, LANG_COMMON, 100);
	break;
    case RACE_HALFLING:
	primary = LANG_HALFLING;
        break;
    case RACE_GITH:
	primary = LANG_HESHRAK;
        break;
    case RACE_GALITH:
	primary = LANG_GALITH;
        break;
    case RACE_MANTIS:
	primary = LANG_MANTIS;
        break;
    default:
	primary = LANG_COMMON;
        break;
    }
    
    // Primary language set here
    if (primary)
        init_skill(ch, primary, 100);

    switch (GET_GUILD(ch)) {
    case GUILD_MERCHANT:
	init_skill(ch, LANG_MERCHANT, 100);
	break;
    case GUILD_TEMPLAR:
    case GUILD_LIRATHU_TEMPLAR:
    case GUILD_JIHAE_TEMPLAR:
	init_skill(ch, LANG_ANCIENT, 100);
	break;
    default:
	break;
    }

    /* set their spoken language to their primary */
    for (check = 0; check < MAX_TONGUES; check++)
        if (language_table[check].spoken == primary)
            ch->specials.language = check;
}

void
gain_accent(CHAR_DATA * ch,     /* person gaining a language skill */
            int accent,         /* accent being increased        */
            int amount)
{                               /* How much the language gains     */
    long t;
    int i, wait;

    /* If they don't have the accent, it won't increase. */
    if (!has_skill(ch, accent))
        return;

    if (IS_SET(ch->skills[accent]->taught_flags, SKILL_FLAG_NOGAIN))
        return;

    wait = MAX(5, (60 - wis_app[GET_WIS(ch)].learn) + (ch->skills[accent]->learned / 9));

    if (((t = time(0)) - ch->skills[accent]->last_gain) < (SECS_PER_REAL_MIN * wait))
        return;

    ch->skills[accent]->last_gain = t;

    if (amount < 0)
        return;

    amount = (amount / 10) + 1;

    i = number(1, 50);
    if (i < (GET_WIS(ch) * 2)) {
        ch->skills[accent]->learned += amount;
        // Can't go over 100
        ch->skills[accent]->learned = MIN(ch->skills[accent]->learned, 100);
    }
}

void
gain_language(CHAR_DATA * ch,   /* person gaining a language skill */
              int lang,         /* language being increased        */
              int amount)
{                               /* How much the language gains     */
    long t;
    int i, wait;

    lang = language_table[lang].spoken;

    /* If they don't have the language, it won't increase.  Gaining
     * a new language is handled elsewhere */

    if (!has_skill(ch, lang))
        /* if (!ch->skills[lang]) */
        return;

    if (IS_SET(ch->skills[lang]->taught_flags, SKILL_FLAG_NOGAIN))
        return;

    wait = MAX(5, (60 - wis_app[GET_WIS(ch)].learn) + (ch->skills[lang]->learned / 9));

    if (((t = time(0)) - ch->skills[lang]->last_gain) < (SECS_PER_REAL_MIN * wait))
        return;

    ch->skills[lang]->last_gain = t;

    if (amount < 0)
        return;

    amount = (amount / 10) + 1;

    i = number(1, 50);
    if (i < (GET_WIS(ch) * 2)) {
        ch->skills[lang]->learned += amount;
        // Can't go over 100
        ch->skills[lang]->learned = MIN(ch->skills[lang]->learned, 100);
    }
}


void
skill_effects_of_death(CHAR_DATA * ch)
{
    int sk, tot = 0;

    ch->abilities.off /= 2;
    ch->abilities.def /= 2;

    for (sk = 0; sk < MAX_SKILLS; sk++)
        if (has_skill(ch, sk))
            /* if (ch->skills[sk]) */
            if (ch->skills[sk]->rel_lev > tot)
                tot = ch->skills[sk]->rel_lev;

    if (tot <= 0)
        return;

    for (sk = 0; sk < MAX_SKILLS; sk++) {
        if (has_skill(ch, sk))
            /* if (ch->skills[sk]) */
        {
            if (ch->skills[sk]->rel_lev <= 0) {
                free((char *) ch->skills[sk]);
                ch->skills[sk] = 0;
            } else
                ch->skills[sk]->rel_lev--;
        }
    }
}


/* Gain maximum in various points */
void
advance_level(CHAR_DATA * ch)
{
    int i;

    if (GET_LEVEL(ch) > MORTAL)
        for (i = 0; i < 3; i++)
            ch->specials.conditions[i] = -1;
}



void
gain_condition(CHAR_DATA * ch, int condition, int value)
{
    bool intoxicated;
    int old_cond, percent, old_percent;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    extern int get_normal_condition(CHAR_DATA * ch, int cond);

    // People over 1hr idle do not need lose condition points, they're considered linkdead
    // Nessalin 11/27/2008 
    if (ch->desc)
      if ( (ch->desc->idle / (WAIT_SEC *20)) > 60 ) 
	return;

    if (IS_IMMORTAL(ch))
      return;

    if (IS_NPC(ch) && condition != DRUNK) {
        GET_COND(ch, condition) = 48;
        return;
    }

    old_cond = GET_COND(ch, condition);
    intoxicated = (GET_COND(ch, DRUNK) > 5);

    /* random hack for the hell of it - hal */
    if ((condition == DRUNK) && (GET_RACE(ch) == RACE_GALITH))
        value /= 5;

    GET_COND(ch, condition) += value;

    /* make sure it doesn't go below 0 */
    GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));

    /* watch for max levels */
    if (condition == DRUNK) {
        /* 100 is the new max for drinking */
        GET_COND(ch, condition) = MIN(100, GET_COND(ch, condition));
    } else if (condition == THIRST) {
        GET_COND(ch, condition) = MIN(get_normal_condition(ch, THIRST), GET_COND(ch, condition));
    } else {
        GET_COND(ch, condition) = MIN(get_normal_condition(ch, FULL), GET_COND(ch, condition));
    }

    if ((GET_RACE(ch) == RACE_MUL) && affected_by_spell(ch, AFF_MUL_RAGE))
        return;

    percent = PERCENTAGE(GET_COND(ch, condition), 
     get_normal_condition(ch, condition));
    old_percent = PERCENTAGE(old_cond, get_normal_condition(ch, condition));

    switch (condition) {
    case FULL:
        if (GET_RACE(ch) != RACE_VAMPIRE) {
            if (percent >= 40 && old_percent < 40)
                send_to_char("You are no longer hungry.\n\r", ch);
            if (percent < 40 && percent >= 22)
                send_to_char("You are a little hungry.\n\r", ch);
            if (percent < 22 && percent >= 16)
                send_to_char("You are hungry.\n\r", ch);
            if (percent < 16 && percent >= 8)
                send_to_char("You are very hungry.\n\r", ch);
            if (percent < 8 && percent >= 4)
                send_to_char("You are famished.\n\r", ch);
            if (percent < 4)
                send_to_char("You are starving.\n\r", ch);
            /* A little something for all the people ignoring food
             * Nessalin 9-12-1999 */
            if (percent < 2) {
                // send_to_char ("Ho, ho! No food!\n\r", ch);
                af.type = TYPE_HUNGER_THIRST;
                af.duration = 9;    /* One day */
                af.modifier = -1;
                af.location = CHAR_APPLY_END;
                af.bitvector = 0;
                af.power = 0;
                af.magick_type = 0;
                affect_to_char(ch, &af);
                send_to_char("Your health worsens from lack of food.\n\r", ch);
            }
        } else {
            if (GET_COND(ch, condition) >= 24)
                send_to_char("You feel unbelievably potent.\n\r", ch);
            if (GET_COND(ch, condition) == 21)
                send_to_char("You feel fairly potent.\n\r", ch);
            if (GET_COND(ch, condition) == 18)
                send_to_char("You feel some of your potency fade.\n\r", ch);
            if (GET_COND(ch, condition) == 15)
                send_to_char("You feel vaguely tired, nothing serious.\n\r", ch);
            if (GET_COND(ch, condition) == 12)
                send_to_char("You feel a bit sapped.\n\r", ch);
            if (GET_COND(ch, condition) == 9)
                send_to_char("You feel sharp hunger pangs.\n\r", ch);
            if (GET_COND(ch, condition) == 6)
                send_to_char("You you vision comes and goes along with"
                             " sharp hunger pangs.\n\r", ch);
            if (GET_COND(ch, condition) == 3)
                send_to_char("You feel a burning desire to feed.\n\r", ch);
            if (GET_COND(ch, condition) == 2)
                send_to_char("If you don't feed soon, you might" " not make it.\n\r", ch);
            if (GET_COND(ch, condition) == 1)
                send_to_char("The desire in you to feed is so strong,"
                             " you can barely think.\n\r", ch);
            if (GET_COND(ch, condition) == 0)
                send_to_char("Your mind is aflame with desire, you are near" " collapse.\n\r",
                             ch);
        }
        return;
    case THIRST:
        if (percent >= 40 && old_percent < 40)
            send_to_char("You are no longer thirsty.\n\r", ch);
        if (percent < 40 && percent > 22)
            send_to_char("You are a little thirsty.\n\r", ch);
        if (percent < 22 && percent >= 16)
            send_to_char("You are thirsty.\n\r", ch);
        if (percent < 16 && percent >= 8)
            send_to_char("You are very thirsty.\n\r", ch);
        if (percent < 8 && percent >= 4)
            send_to_char("You are parched.\n\r", ch);
        if (percent < 4)
            send_to_char("You are dehydrated.\n\r", ch);
        /* A little something for all the people ignoring water
         * Nessalin 9-12-9999 */
        if (percent < 2) {
            send_to_char("You begin to suffer from thirst!\n\r", ch);
            af.type = TYPE_HUNGER_THIRST;
            af.duration = 9;        /* One day */
            af.modifier = -10;
            af.location = CHAR_APPLY_MOVE;
            af.bitvector = 0;
            af.power = 0;
            af.magick_type = 0;
            affect_to_char(ch, &af);
        }
        return;
    case DRUNK:
        if (intoxicated && GET_COND(ch, DRUNK) == 0)
            send_to_char("You are now sober.\n\r", ch);
        return;
    default:
        break;
    }
}


void
check_idling(CHAR_DATA * ch)
{
/*
   if (++ch->specials.timer > 8)
   if (!IS_IMMORTAL(ch) && !ch->specials.was_in_room && ch->in_room) 
   {
   ch->specials.was_in_room = ch->in_room;
   if (ch->specials.fighting) {
   stop_fighting(ch->specials.fighting);
   stop_fighting(ch);
   }
   act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char("You have been idle, and are pulled into a void.\n\r",
   ch);
   char_from_room(ch);
   char_to_room(ch, get_room_num(0));
   } 
 */
}


/* Update both PC's & NPC's and objects */
void
point_update(void)
{
    int pts;
    int vnum;

    CHAR_DATA *i;
    OBJ_DATA *j, *next_thing;
    struct descriptor_data *point, *next_point;
    struct timeval start;

    perf_enter(&start, "point_update()");
    /* characters  */
    for (point = descriptor_list; point; point = next_point) {
        next_point = point->next;

#ifdef SMELLS
        if (!point->connected && (i = point->character))
            char_smell_update(i);
#endif

        if (!point->connected && (i = point->character) && !IS_NPC(i)) {
            update_char_objects(i);
            /* increase galith eco once a day */
            if ((GET_RACE(i) == RACE_GALITH) && (!(time_info.hours % 3) || !(time_info.hours % 6))
                && !(number(0, 2)) && (i->specials.eco < 100))
                i->specials.eco++;
            if (!is_char_ethereal(i)) { /* non etheral */
                if (GET_RACE(i) == RACE_VAMPIRE) {      /* only lose a hunger point at dawn..so they last 8 days */
                    if (time_info.hours == 3)
                        gain_condition(i, FULL, -1);
                    gain_condition(i, THIRST, -1);
                }
                /* immortal races do not get hungry */
                else if (number(0, 1) && (race[(int) GET_RACE(i)].max_age > -1)) {      /* non vampire stuff */

                    // NESS_THIRST_9_13_2003
                    // doubled cost for desert, salt flags, thornlands
                    // doubled cost for field, hisll,e tc...
                    // halved benefit for inside and indoors

                    pts = -1;
                    if (time_info.hours == HIGH_SUN)
                        pts = pts - 2;
                    if ((time_info.hours == MORNING) || (time_info.hours == AFTERNOON))
                        pts = pts - 1;
                    if ((i->in_room->sector_type == SECT_DESERT)
                        || (i->in_room->sector_type == SECT_SALT_FLATS)
                        || (i->in_room->sector_type == SECT_THORNLANDS)) {
                        if (GET_RACE(i) == RACE_DESERT_ELF)
                            pts = pts - 2;
                        else
                            pts = pts - 4;
                    }
                    if ((i->in_room->sector_type == SECT_FIELD)
                        || (i->in_room->sector_type == SECT_GRASSLANDS)
                        || (i->in_room->sector_type == SECT_COTTONFIELD)
                        || (i->in_room->sector_type == SECT_HILLS)
                        || (i->in_room->sector_type == SECT_MOUNTAIN)
                        || (i->in_room->sector_type == SECT_SILT)
                        || (i->in_room->sector_type == SECT_LIGHT_FOREST)
                        || (i->in_room->sector_type == SECT_HEAVY_FOREST)
                        || (i->in_room->sector_type == SECT_RUINS)
                        || (i->in_room->sector_type == SECT_SHALLOWS)) {
                        if (GET_RACE(i) == RACE_DESERT_ELF)
                            pts = pts - 1;
                        else
                            pts = pts - 2;
                    }
                    if ((i->in_room->sector_type == SECT_INSIDE)
                        || (i->in_room->sector_type == SECT_SEWER)) {
                        if (GET_RACE(i) == RACE_DESERT_ELF)
                            pts = pts + 2;
                        else
                            pts = pts + 1;
                    }
                    if (IS_SET(i->in_room->room_flags, RFL_INDOORS)) {
                        if (GET_RACE(i) == RACE_DESERT_ELF)
                            pts = pts + 2;
                        else
                            pts = pts + 1;
                    }
                    if (GET_POS(i) < POSITION_STANDING)
                        pts = pts + 1;
                    pts = MIN(pts, 0);

                    if (GET_RACE(i) == RACE_DESERT_ELF)
                        pts = MAX(-5, pts);
                    else
                        pts = MAX(-10, pts);

                    if ((GET_RACE(i) == RACE_MUL) && affected_by_spell(i, AFF_MUL_RAGE)) {
                        pts *= 2;
                        pts = MIN(pts, -1);
                        pts = MAX(pts, -10);
                    }

                    if (!is_char_ethereal(i)) {
                        if ((GET_RACE(i) == RACE_MUL) && affected_by_spell(i, AFF_MUL_RAGE)) {
                            if (affected_by_spell(i, SPELL_REGENERATE))
                                gain_condition(i, FULL, -8);
                            else
                                gain_condition(i, FULL, -4);
                        } else {
                            if (affected_by_spell(i, SPELL_REGENERATE))
                                gain_condition(i, FULL, -4);
                            else
                                gain_condition(i, FULL, -1);
                        }
                        if (!affected_by_spell(i, SPELL_FIREBREATHER)) {
                            if (affected_by_spell(i, SPELL_REGENERATE))
                                gain_condition(i, THIRST, (2 * pts));
                            else
                                gain_condition(i, THIRST, pts);
                        }
                    }

                    if (((GET_COND(i, FULL) < 8) || (GET_COND(i, THIRST) < 8))
                        && (GET_POS(i) == POSITION_SLEEPING)) {
                        if (!(IS_AFFECTED(i, CHAR_AFF_SLEEP))) {
                            send_to_char("You can no longer sleep without " "nourishment.\n\r", i);
                            set_char_position(i, POSITION_RESTING);
                        }
                    }
                }
            }
            if (GET_POS(i) == POSITION_SLEEPING)
                gain_condition(i, DRUNK, -4);
            else
                gain_condition(i, DRUNK, -1);
        }
    }


    /* objects */
    for (j = object_list; j; j = next_thing) {
        next_thing = j->next;   /* Next in object list */

        if (GET_ITEM_TYPE(j) == ITEM_LIGHT && j->equipped_by == NULL && IS_LIT(j)) {
            if (!j->in_room) {
                REMOVE_BIT(j->obj_flags.value[5], LIGHT_FLAG_LIT);
                continue;
            }
            if (update_light(j) == NULL)
                continue;
        }
        if (IS_CORPSE(j)) {
            /* timer count down */
            if (j->obj_flags.timer > 0)
                j->obj_flags.timer--;

            /* It's finished, let's get it out of here */
            if (!j->obj_flags.timer) {
                /* Do any echoes to people as necessary */
                if (j->carried_by) {
                    act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
                } else if (j->in_room && j->in_room->people) {
                    if (j->in_room->sector_type == SECT_DESERT) {
                        act("$p slowly sinks into the sand.", TRUE, j->in_room->people, j, 0,
                            TO_ROOM);
                        act("$p slowly sinks into the sand.", TRUE, j->in_room->people, j, 0,
                            TO_CHAR);
                    } else {
                        vnum = obj_index[j->nr].vnum;
                        if ((vnum == 857) || (vnum == 48009)) {
                            act("$p is swept away in the wind.", TRUE, j->in_room->people, j, 0,
                                TO_ROOM);
                            act("$p is swept away in the wind.", TRUE, j->in_room->people, j, 0,
                                TO_CHAR);
                        } else {
                            act("$p decays into morbid dust and blows away.", TRUE,
                                j->in_room->people, j, 0, TO_ROOM);
                            act("$p decays into morbid dust and blows away.", TRUE,
                                j->in_room->people, j, 0, TO_CHAR);
                        }
                    }
                }

                /* Bodies and their gear get "consumed" by the desert */
                if (j->in_room) {
		    switch (j->in_room->sector_type) {
			case SECT_DESERT:
			case SECT_FIELD:
			case SECT_SEWER:
			case SECT_SHALLOWS:
			case SECT_SILT:
			case SECT_RUINS:
			    if (j->contains)
				    gamelogf("Sending '%s' to artifact cache in room #%d\n\r", OSTR(j, short_descr), j->in_room->number);
			    while (j->contains) {
				OBJ_DATA *tmp_obj = j->contains;
				obj_from_obj(tmp_obj);
				obj_to_artifact_cache(tmp_obj, j->in_room);
			    }
			    break;
			default:
			    break;
		    }
                }

                /* Empty whatever's left inside and remove the corpse */
                dump_obj_contents(j);
                extract_obj(j);

            }                   /* End if timer == 0 */
        }                       /* End if corpse */
    }                           /* End for() */

    update_no_barter();
    perf_exit("point_update()", start);
}

/* Find the weather node associated with the given room */
struct weather_node *
find_weather_node(ROOM_DATA * room)
{
    struct weather_node *wn;

    if (!room)
        return NULL;

    for (wn = wn_list; wn; wn = wn->next) {
        if (wn->zone == room->zone)
            break;
    }

    return wn;
}


/* used with update_stats() to account for weather conditions */
int
weather_effects(CHAR_DATA * ch)
{
    int ht, eff = 0;
    double w;

    if (!ch || !ch->in_room)
        return 0;

    struct weather_node *wn = find_weather_node(ch->in_room);

    if (wn) {
        ht = wn->temp;
        if (GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST) {
            if (ht > 90 && !affected_by_spell(ch, SPELL_SHADOW_ARMOR)) {
                eff -= (ht - 90);
            } else if (ht < 50 && !affected_by_spell(ch, SPELL_FIRE_ARMOR)) {
                eff += (50 - ht);
            }
        } else {
            if ((ht > 90) && (!affected_by_spell(ch, SPELL_SHADOW_ARMOR)))
                eff += (ht - 90);
            else if (ht < 50 && !affected_by_spell(ch, SPELL_FIRE_ARMOR))
                eff += (50 - ht);
        }

        if (IS_SET(ch->in_room->room_flags, RFL_RESTFUL_SHADE))
            eff = eff / 2;

        w = wn->condition;
        /* Wind Armor blocks the harmful effects of harsh conditions */
        if (!affected_by_spell(ch, SPELL_WIND_ARMOR)) {
            if (w >= 2.50)
                eff += (int) w *5;
            else if (w >= 1.00)
                eff += (int) w *6;
        }

        w = wn->life;
        if (w > 1000)
            eff -= ((w - 1000) / 100);
    }

    if (IS_SET(ch->in_room->room_flags, RFL_INDOORS))
        eff = 0;

    if (GET_RACE_TYPE(ch) == RTYPE_INSECTINE)
        eff = eff / 2;

    if (GET_RACE_TYPE(ch) == RTYPE_REPTILIAN)
        eff -= (eff / 3);

    return eff;
}

void
adjust_ch_infobar( CHAR_DATA *ch, int type ) {
    char buf[MAX_STRING_LENGTH];

    if ( ch->desc && ch->desc->term
     && IS_SET(ch->specials.act, CFL_INFOBAR)) {
        switch( type ) {
        case GINFOBAR_HP:
            sprintf(buf, "%d(%d)", GET_HIT(ch), GET_MAX_HIT(ch));
            break;
        case GINFOBAR_MANA:
            sprintf(buf, "%d(%d)", GET_MANA(ch), GET_MAX_MANA(ch));
            break;
        case GINFOBAR_STAM:
            sprintf(buf, "%d(%d)", GET_MOVE(ch), GET_MAX_MOVE(ch));
            break;
        case GINFOBAR_STUN:
            sprintf(buf, "%d(%d)", GET_STUN(ch), GET_MAX_STUN(ch));
            break;
        default:
            return;
        }
        adjust_infobar(ch->desc, type, buf);
    }
}

bool
suffering_from_poison(CHAR_DATA *ch, int type ) {
    struct affected_type *af;

    switch( type ) {
    case SPELL_POISON:
    case POISON_GENERIC:
    {
        int active = FALSE;
        int lingering = FALSE;

        for( af = ch->affected; af; af = af->next ) 
            if( af->type == type )  {
                if( !IS_SET(af->bitvector, CHAR_AFF_DAMAGE))
                   lingering = TRUE;
                else
                   active = TRUE;
            }

        if( active && lingering )
            return TRUE;

        break;
    }
    default:
        if(affected_by_spell(ch, type)) return TRUE;
        break;
    }
    return FALSE;
}

int
update_poison(CHAR_DATA *ch, struct affected_type *af, int duration_changed ) {
    struct affected_type skell_af;
    
    int chance;
    char buf[MAX_STRING_LENGTH];

    OBJ_DATA *obj;

    if( !ch || !af )
        return FALSE;

    switch( af->type ) {
    case SPELL_POISON:
    case POISON_GENERIC:
        if (IS_SET(af->bitvector, CHAR_AFF_DAMAGE)
         && GET_RACE(ch) != RACE_VAMPIRE) {
            if (!number(0, 5 + GET_END(ch))) {
                struct affected_type afnew; 

                memset(&afnew, 0, sizeof(afnew));

                bool alive;
                int damage = number(1, 5);
                //if (!ws_damage(ch, ch, number(1, 5), 0, 0, 0, SPELL_POISON, 0))
                //    return FALSE;
                afnew.type = af->type;
                afnew.duration = 9;        /* One day */
                afnew.modifier = -1 * damage;
                afnew.location = CHAR_APPLY_HIT;
                afnew.bitvector = 0;
                afnew.power = 0;
                afnew.magick_type = 0;
                affect_to_char(ch, &afnew);

                alive = generic_damage(ch, damage, 0, 0, 0);
                
                // send appropriate messages
                send_fight_messages(ch, ch, SPELL_POISON, 0);

                if( !alive ) {
                    sprintf(buf, "%s %s%s%shas died from poison in room #%d.",
                     IS_NPC(ch) ? MSTR(ch, short_descr)
                     : GET_NAME(ch), !IS_NPC(ch) ? "(" : "",
                     !IS_NPC(ch) ? ch->account : "", !IS_NPC(ch) ? ") " : "",
                     ch->in_room->number);
                    die_with_message(ch, ch, buf);
                }
                return alive;
            }
        }
	return TRUE;

#ifndef TIERNAN_PLAGUE
    case POISON_PLAGUE: {
        CHAR_DATA *rch;

        /* Let's tell them how they're suffering */
        chance = GET_MOVE(ch);
        if (chance < 0)
            chance *= -1;
        // at worst 1 in 10 clicks
        chance = MIN(chance, 10);
        if (!number(0, chance)) {
            /* Display how bad it really is */
            /* The plague causes fever and brain swelling, and lasts
             * for a while, after which it causes -1 WIS as a result
             * of brain damage  */
            switch (number(1, 3)) {
            case 1:
                send_to_char("Sweat rolls down your face, "
                             "and your head throbs incessantly.\n\r", ch);
                act("$n breaks out into a sweat, and looks slightly pale.", FALSE, ch,
                    0, 0, TO_ROOM);
                break;
            case 2:
                send_to_char
                    ("Your nose tickles and you have an overwhelming need to sneeze.\n\r",
                     ch);
                parse_command(ch, "sneeze");
                break;
            case 3:
            default:
                send_to_char("You have an uncontrollable urge to cough, and do so.\n\r",
                             ch);
                parse_command(ch, "cough");
                break;
            }
#ifndef TIERNAN_PLAGUE_NO_SPREAD
            /* Very slim chance to pass it on */
            for (rch = ch->in_room->people; rch; rch = rch->next_in_room)
                if (number(1, 10000) == 1)  /* 0.001% chance to infect */
                    if (!affected_by_spell(rch, POISON_PLAGUE))
                        poison_char(rch, (POISON_PLAGUE - 219), 1, 0);
#endif
        }
        return TRUE;
    }
#endif

    case POISON_GRISHEN:
        if (!number(0, GET_END(ch))) {
            if (GET_MOVE(ch) <= 0) {
                if (GET_POS(ch) != POSITION_SLEEPING) {
                    send_to_char("You cease to sweat, and you collapse" " in exhaustion.\n\r",
                                 ch);
                    act("$n's eyes roll up in $s head, and $e collapses.", FALSE, ch, 0, 0,
                        TO_ROOM);
                    set_move(ch, 0);
                    GET_POS(ch) = POSITION_SLEEPING;
                }
            } else {
                adjust_move(ch, -5);
                send_to_char("Sweat rolls down your face as your temperature"
                             " rises, and you feel tired.\n\r", ch);
                act("$n breaks out into a sweat, and looks slightly pale.", FALSE, ch, 0, 0,
                    TO_ROOM);
            }
        }
        return TRUE;

    case POISON_METHELINOC:
        if (!number(0, GET_END(ch))) {
            // if you try and sleep it off to recover, you die 
            if (GET_POS(ch) == POSITION_SLEEPING) {
                send_to_char
                    ("A terrible coldness consumes you, and you die!\n\r", ch);
                act("$n shivers one last time, then goes limp!", TRUE,
                    ch, 0, 0, TO_ROOM);
                // Ch is dead, so let's leave a death message
                if( !IS_NPC(ch) ) {
                    free(ch->player.info[1]);
                    sprintf(buf, "%s has died from methelinoc poison in room %d.",
                     GET_NAME(ch), ch->in_room->number);
                    ch->player.info[1] = strdup(buf);
                    gamelog(buf);
                }
                die(ch);
                return FALSE;
            }
            // Not asleep, so let's damage them
            send_to_char("You feel a terrible coldness in your veins,"
                         " and your body is wracked with pain.\n\r", ch);
            act("$n shivers uncontrollably.", FALSE, ch, 0, 0, TO_ROOM);
            generic_damage(ch, number(5, GET_END(ch)), 0, 0, 0);
        }
        return TRUE;

    /* Heramide  --drops stun to enduce unconsciousness */
    case POISON_HERAMIDE: 
        if( !knocked_out(ch)) {
            if (!number(0, GET_END(ch) / 5)) {
                switch (number(1, 3)) {
                case 1:
                    send_to_char("Your surroundings blur slightly.\n\r", ch);
                    act("$n's eyes seem to lose focus for a second.", FALSE, ch, 0, 0, TO_ROOM);
                    break;
                case 2:
                    send_to_char("A darkness creeps at the edge of your vision.\n\r", ch);
                    act("$n's eyes seem to lose focus for a second.", FALSE, ch, 0, 0, TO_ROOM);
                    break;
                default:
                    send_to_char("You feel slightly dizzy and the room seems to spin.\n\r", ch);
                    act("$n stumbles slightly and grasps for nearby objects " "to steady $mself.",
                        FALSE, ch, 0, 0, TO_ROOM);
                    break;
                }

                if( !generic_damage(ch, 0, 0, 0, number(1, 4) * number(20, 25)) ) {
                    return FALSE;
                }
            }
        }
        return TRUE;

    // Hallucinogenic 
    case POISON_SKELLEBAIN:    
        // Half-giants are immune.. shouldnt even get here
        if (GET_RACE(ch) == RACE_HALF_GIANT) {
            affect_from_char(ch, POISON_SKELLEBAIN);
        }
        else if (GET_POS(ch) > POSITION_SLEEPING) {
           
            // Chance to progress to SKELLEBAIN stage 2
            // for a 12 end, this would be about 1 min avg
            if (number(0, GET_END(ch) + 28) == GET_END(ch) + 28) {
                send_to_char("You feel your head reel and feel sick to your stomach as the"
                             " infection inside of you grows.\n\r", ch); 
                af->type = POISON_SKELLEBAIN_2;
                af->power = 6; //set to SUL
            }

        } // End if position is greater than sleeping
        return TRUE;       
        // end SKELLEBAIN
   
    case POISON_SKELLEBAIN_2:
        // Half-giants are immune.. shouldnt even get here
        if (GET_RACE(ch) == RACE_HALF_GIANT) {
            affect_from_char(ch, POISON_SKELLEBAIN_2);
        }
        else if (GET_POS(ch) > POSITION_SLEEPING) {
           
            // Chance to progress to SKELLEBAIN stage 3 or back to 1
            if (number(0, GET_END(ch) + 20) == GET_END(ch) + 20) {
                if ((GET_END(ch) + number(1, 100)) > 60) { // with 10 end, 50% of the time it will go back to phase 1
                    send_to_char("Your head clears a little as some of the visions subside.\n\r", ch);
                    af->type = POISON_SKELLEBAIN;
                    af->power = 5; //set to pav
                } else {
                    send_to_char("You feel your body shake for a moment and feel very "
                                 "sick to your stomach.\n\r", ch);
                    af->type = POISON_SKELLEBAIN_3;
                    af->power = 7; //set to mon
                }
            }

            if (number(0, 22) == 0) { 
                switch (number(0, 50)) {
                case 0:
                    send_to_char("Tiny flying ratlons dance in front of your eyes.\n\r", ch);
                    break;
                case 1:
                    if (GET_POS(ch) > POSITION_SITTING) {
                        send_to_char("The ground rumbles underfoot, sending everyone lurching."
                                     "\n\r", ch);
                        act("$n stumbles and falls to the ground.", FALSE, ch, 0, 0, TO_ROOM);
                        set_char_position(ch, POSITION_SITTING);
                    } else
                        send_to_char("You feel the world lurch, and you grasp the"
                                     " ground for support.\n\r", ch);
                    break;
                case 2:
                    if ((room_in_city(ch->in_room) == CITY_ALLANAK)
                        || (room_in_city(ch->in_room) == CITY_TULUK))
                        send_to_char("The red-robed templar points a finger at you, "
                                     "and gestures for nearby guards.\n\r", ch);
                    break;
                case 3:
                    send_to_char("Magickal currents begin to swirl around someone.\n\r", ch);
                    break;
                case 4:
                    send_to_char("You see a small road materialize from your eyeballs, and stretch "
                                 "out into the room. Small wagons and travelers appear to travel along it.\n\r", ch);
                    break;
                case 5:
                    send_to_char("You hear an irritating song in your head, one that you cannot " 
                                 "remember ever hearing, and that you cannot forget.\n\r", ch);
                    break;
                case 6:
                    send_to_char("You sense defiling in the area.\n\r", ch);
                    break;
                case 7:
                    if (ch->in_room->sector_type == SECT_CITY)
                        act("You feel a hand in your belongings, but are unable"
                            " to catch the culprit.", FALSE, ch, 0, 0, TO_CHAR);
                    break;
                case 8:
                    send_to_char("You feel the eyes of someone watching you.\n\r", ch);
                    break;
                case 9:
                    send_to_char("Your hand feels incredibly hot, and when you"
                                 " look at it, it's on fire!\n\r", ch);
                    act("$n looks at $s hand and yelps in surprise.", FALSE, ch, 0, 0, TO_ROOM);
                    break;
                case 10:
                    send_to_char("You notice: A tiny kank-fly land on your nose, wave and flit off.\n\r", ch);
                    break;
                case 11:
                    send_to_char("A hideous dragon materializes in front of you.\n\r", ch);
                    send_to_char("A hideous dragon opens its maw and reaches out for you.\n\r",
                                 ch);
                    break;
                case 12:
                    send_to_char("You feel hungry.\n\r", ch);
                    break;
                case 13:
                    send_to_char("The lean, wild-eyed gith draws an obsidian longsword!\n\r",
                                 ch);
                    break;
                case 14:
                    send_to_char("Your body lightens, and you start floating.\n\r", ch);
                    break;
                case 15:
                    send_to_char("A drop of water falls from above, landing on your head.\n\r",
                                 ch);
                    break;
                case 16:
                    send_to_char("A wavering howl sounds in the distance.\n\r", ch);
                    break;
                case 17:
                    send_to_char("You feel burning poison in your veins, and suffer.\n\r", ch);
                    break;
                case 18:
                    send_to_char("The world goes black.\n\r", ch);
                    break;
                case 19:
                    send_to_char("The wind picks up, howling fiercely.\n\r", ch);
                    break;
                case 20:
                    send_to_char("You glimpse a dark shadow out of the corner of your eye.\n\r",
                                 ch);
                    break;
                case 21:
                    send_to_char("Your stomach roils and burns.\n\r", ch);
                    break;
                case 22:
                    send_to_char("A soft tingling at your neck feels like fingers brushing along it.\n\r", ch);
                    break;
                case 23:
                    send_to_char("Colors flicker along your surroundings, impossible "
                                 "purples and greys and whites that baffle your mind.\n\r", ch);
                    break;
                case 24:
                    send_to_char("A dwarf with blue skin, wearing all white silk, materializes "
                                 "through the floor and offers you a tray of delicacies. Upon " 
                                 "closer inspection, they all seem to be human ears.\n\r", ch);
                    break;
                case 25:
                    send_to_char("Your legs feel restless, like you need to burn off extra energy and run.\n\r", ch);
                    break;
                case 26:
                    send_to_char("Everything around you takes on a purple, pink and red hues for a "
                                 "moment before fading.\n\r", ch);
                    break;
                case 27:
                    send_to_char("A long, multi-colored stream of light trails behind everything that moves.\n\r", ch);
                    break;
                case 28:
                    send_to_char("All movement around you stops for a moment, then speeds back up to normal.\n\r", ch);
                    break;
                case 29:
                    send_to_char("You Notice: A shadow with pink glowing eyes slowly creeping towards you.\n\r", ch);
                    break;
                case 30:
                    send_to_char("Suddenly, it feels like everything is falling upward.\n\r", ch);
                    break;
                case 31:
                    send_to_char("Your skin begins to itch badly, moving when you try and scratch it.\n\r", ch);
                    break;
                case 32:
                    send_to_char("You see what might be tiny beetles crawling just under the "
                                 "surface of your skin.\n\r", ch);
                    break;
                case 33:
                    send_to_char("Everything you touch seems to be covered in blood after you remove your hand.\n\r", ch);
                    break;
                case 34:
                    send_to_char("You see a rotting, pallid corpse sprawled out at your feet, covered in kank-flies.\n\r", ch);
                    break;
                case 35:
                    send_to_char("The surfaces of all the objects around you suddenly grow thick, coarse black hair.\n\r", ch);
                    break;
                case 36:
                    send_to_char("As you reach to touch anything it warps away from your hand, always remaining out of reach.\n\r", ch);
                    break;
                case 37:
                    send_to_char("You feel like you are being followed, catching only a glimpse of your stalker out of "
                                 "the corner of your eyes.\n\r", ch);
                    break;
                case 38:
                    send_to_char("You feel like touching everything as the items around you takes on a soft silk-like texture.\n\r", ch);
                    break;
                case 39:
                    send_to_char("With every light touch on your skin, you hear the touch, ringing clearly in your ears.\n\r", ch);
                    break;
                case 40:
                    send_to_char("You a loud ringing in the distance.\n\r", ch);
                    break;
                case 41:
                    send_to_char("You feel your eyelids become heavy as a wave of exhaustion racks your body.\n\r", ch);
                    break;
                case 42:
                    send_to_char("Suddenly your fingers turn into tiny, brown snakes writhing about and curling onto one another.\n\r", ch);
                    break;
                case 43:
                    send_to_char("The ground beneath your feet feels soft and squishy, for a moment it seems as though you might sink into it.", ch);
                    break;
                case 44:
                    send_to_char("All movement around you speeds up, a number of distracting things happening in a sudden "
                                 "collision of time before returning to normal.", ch);
                    break;
                case 45:
                    send_to_char("Iridescent threads of web surround you, sparkling in the light. Any movement is followed "
                                 "by a new thread, appearing to be attached to your skin.", ch);
                    break;
                case 46:
                    send_to_char("In every shadow hides a face, gaunt, stretched features melding into the darkness. "
                                 "Hollow eyes peering, menacing teeth bared.", ch);
                    break;
                case 47:
                    send_to_char("Every solid object you focus on seems to shimmer and shake, appearing somewhat translucent "
                                 "at the edges as if they were shuddering extremely fast.", ch);
                    break;
                case 48:
                    send_to_char("Thick black tendrils ooze upwards out of the ground, slithering over your feet and "
                                 "beginning to entangle your legs in their tight clutch.", ch);
                    break;
                case 49:
                    send_to_char("You feel your ears filled with maniacal laughter: louder... quieter... distant... close... "
                                 "echoed... you can't tell where it is coming from.", ch);
                    break;
                case 50:
                    send_to_char("You are blinded by a searing, white light.", ch);
                    break;
                default:
                    break;
                } // End switch()
            }
        }
        return TRUE;
        // end SKELLEBAIN_2

    case POISON_SKELLEBAIN_3:
        // Half-giants are immune.. shouldnt even get here
        if (GET_RACE(ch) == RACE_HALF_GIANT) {
            affect_from_char(ch, POISON_SKELLEBAIN_3);
        }
        else if (GET_POS(ch) > POSITION_SLEEPING) {

            // Chance to progress to SKELLEBAIN stage 2
            if (number(0, GET_END(ch) + 25) == GET_END(ch) + 25) {
                send_to_char("Your head clears a little, and you feel a cold sweat cover your skin.\n\r", ch);
                af->type = POISON_SKELLEBAIN_2;
                af->power--; //set back to sul
            }
           
           if (number(0, 22) == 0) { // Should be about every 30 seconds on average.
                switch (number(0, 38)) {
                case 0:
                    send_to_char("Your stomach feels as if it is going to retch.\n\r", ch);
                    break;
                case 1:
                    if (GET_POS(ch) > POSITION_SITTING) {
                        send_to_char("Your head spins and you feel an acute sense of vertigo as you fall down."
                                     "\n\r", ch);
                        act("$n stumbles and falls to the ground.", FALSE, ch, 0, 0, TO_ROOM);
                        set_char_position(ch, POSITION_SITTING);
                    } 
                    break;
                case 2:
                    send_to_char("Your limbs feel tingly and numb, distant and unusable.\n\r", ch);
                    break;
                case 3:
                    send_to_char("You vomit uncontrollably over your chest.\n\r", ch);
                    act("$n makes a gagging sound and vomits loudly.", FALSE, ch, 0, 0, TO_ROOM);
                    break;
                case 4:
                    send_to_char("You look down at your chest, and your legs seem to be where your arms are.\n\r", ch);
                    break;
                case 5:
                    send_to_char("Your blood seems to boil, sweat dripping from all of your pores.\n\r", ch);
                    break;
                case 6:
                    send_to_char("You lose control of your bowels making a nasty mess.\n\r", ch);
                    if (ch->equipment[WEAR_LEGS]) {      // leggings smelly
                        obj = ch->equipment[WEAR_LEGS];
                        sflag_to_obj(obj, OST_SEWER);
                    }
                    scent_to_char(ch, SCENT_DOODIE, 24);  // Add poo smell to them
                    break;
                case 7:
                    send_to_char("Your tongue feels numb and fat in your mouth.\n\r", ch);
                    // Make 'em babble 
                    if (!affected_by_spell(ch, PSI_BABBLE)) {    
                         skell_af.type = PSI_BABBLE;
                         skell_af.location = 0;
                         skell_af.modifier = 0;
                         skell_af.duration = 1;
                         skell_af.bitvector = CHAR_AFF_PSI;
                         affect_to_char(ch, &skell_af);
                    }
                    break;
                case 8:
                    send_to_char("Your hands seem to melt before your very eyes into little piles of goo.\n\r", ch);
                    // Drop their EP, ES, or ETWO equipment immediately
                    if (ch->equipment[ES]) {
                        obj = ch->equipment[ES];
                        obj_to_room(unequip_char(ch, ES), ch->in_room);

                        act("You lose your grip and drop $p.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n loses $s grip and drops $p.", TRUE, ch, obj, 0, TO_ROOM);

                        drop_light(obj, ch->in_room);
                    }
                    if (ch->equipment[EP]) {
                        obj = ch->equipment[EP];
                        obj_to_room(unequip_char(ch, EP), ch->in_room);

                        act("You lose your grip and drop $p.", FALSE, ch, obj, 0, TO_CHAR);
                        act("You lose your grip and drop $p.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n loses $s grip and drops $p.", TRUE, ch, obj, 0, TO_ROOM);

                        drop_light(obj, ch->in_room);
                    }
                    if (ch->equipment[ETWO]) {
                        obj = ch->equipment[ETWO];
                        obj_to_room(unequip_char(ch, ETWO), ch->in_room);

                        act("You lose your grip and drop $p.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n loses $s grip and drops $p.", TRUE, ch, obj, 0, TO_ROOM);

                        drop_light(obj, ch->in_room);
                    }
                    // Stop subduing whoever they are subduing
                    if (ch->specials.subduing)
                        parse_psionic_command(ch, "release");

                    break;
                case 9:
                    send_to_char("Your head pounds, feeling like something is trying to escape through your forehead.\n\r", ch);
                    break;
                case 10:
                    send_to_char("You feel a uncontrollably sense of fear wrack through your body.\n\r", ch);
                    // flee self?
                    break;
                case 11:
                    send_to_char("Shaking uncontrollably, your hands seem to twist and crack.\n\r", ch);
                    break;
                case 12:
                    send_to_char("Your skin tingles as you feel thousands of insects crawl over you.\n\r", ch);
                    break;
                case 13:
                    send_to_char("Your heart begins racing, looking down you notices it is no longer in your chest.\n\r", ch);
                    break;
                case 14:
                    send_to_char("Just as it looks away, you catch a glimpse of your shadow smiling at you.\n\r", ch);
                    break;
                case 15:
                    send_to_char("An unexpected chorus fills your ears as you hear every color.\n\r", ch);
                    break;
                case 16:
                    send_to_char("Your vision blurs, making the images of everything around you smear together.\n\r", ch);
                    break;
                case 17:
                    send_to_char("As you attempt to take a deep breath, it feel like you are breathing silt.\n\r", ch);
                    break;
                case 18:
                    send_to_char("After a whiff of an erotic scent, your eyes dilate and your mouth waters.\n\r", ch);
                    break;
                case 19:
                    send_to_char("Feeling severely parched, you notice sand coming from your mouth as you"
                                 " exhale and you feel the urge to quench your thirst.\n\r", ch);
                    break;
                case 20:
                    send_to_char("From a distance, you hear the sounds of a pair of mating mekillots.\n\r", ch);
                    break;
                case 21:
                    send_to_char("After a tingle within your skull, your eyes blink and twitch uncontrollably.\n\r", ch);
                    break;
                case 22:
                    send_to_char("Without warning, you feel yourself suddenly vanish.\n\r", ch);
                    break;
                case 23:
                    send_to_char("The room suddenly feels sweltering hot and the air stagnant.\n\r", ch);
                    break;
                case 24:
                    send_to_char("A shiver passes through you as you feel a chill.\n\r", ch);
                    break;
                case 25:
                    send_to_char("An unseen voice whispers horrible things into your ear.\n\r", ch);
                    break;
                case 26:
                    send_to_char("As your mouth waters, you cannot escape the taste of blood in your mouth.\n\r", ch);
                    break;
                case 27:
                    send_to_char("You feel as if everyone around you wants you dead.\n\r", ch);
                    break;
                case 28:
                    send_to_char("Someone takes a swing at you from behind, missing narrowly and nearly decapitating you!\n\r", ch);
                    break;
                case 29:
                    send_to_char("A sharp stabbing behind your eyes causes your vision to blur for a moment.\n\r", ch);
                    break;
                case 30:
                    send_to_char("You hear an intense thumping sound in your ears, making it difficult to hear anything else.\n\r", ch);
                    break;
                case 31:
                    send_to_char("You feel a wave of intense depression, knowing for certain that your death is moments away.\n\r", ch);
                    break;
                case 32:
                    send_to_char("You feel like you're in a bubble and everything is flat.", ch);
                    break;
                case 33:
                    send_to_char("Your eyes begin to itch incessantly, followed by random patches of powerful itching sensations "
                                 "across your body. If you scratch it, it moves somewhere else.", ch);
                    break;
                case 34:
                    send_to_char("Your legs feel like heavy stumps and your feet like solid stone, you can't seem to move them.", ch);
                    break;
                case 35:
                    send_to_char("A chill runs down your spine, filling you with a very cold sensation and causing you to shiver.", ch);
                    break;
                case 36:
                    send_to_char("You feel incredibly comfortable, your limbs are light and your movements are graceful and airy.", ch);
                    break;
                case 37:
                    send_to_char("Your knees feel weak and begin to tremble as if though they can no longer hold up your weight.", ch);
                    break;
                case 38:
                    send_to_char("You begin to feel dizzy as your head spins, the view around you blurred in motion and you can't focus.", ch);
                    break;

                default:
                    break;
                } // End switch()
            }  
        }
        return TRUE;
        // end SKELLEBAIN_3

    case POISON_TERRADIN:
        if (!number(0, GET_COND(ch, FULL))) {
            if (GET_COND(ch, FULL) > 0) {
                send_to_char("You grasp your stomach, as it begins to tremble" " queasily.\n\r",
                             ch);
                send_to_char("You double over in pain, as your insides are"
                             " forcefully vomited out.\n\r", ch);
                act("$n grasps $s stomach, and turns a palish color.", FALSE, ch, 0, 0,
                    TO_ROOM);
                act("$n doubles over and throws up.", FALSE, ch, 0, 0, TO_ROOM);
                GET_COND(ch, FULL) = MAX(0, GET_COND(ch, FULL) - 5);
            } else {
                /* This will happen every 'tick' due to hunger being 0 */
                /* so we're gonna throw another chance based on end in */
                if (!number(0, (GET_END(ch) / 2))) {
                    send_to_char("You feel a terrible burning in your"
                                 " stomach, and you cry out in pain.\n\r", ch);
                    send_to_char("Your cries are muffled as you dry heave,"
                                 " your insides feeling like they tear\n\rwith each heave."
                                 "\n\r", ch);
                    act("$n grasps $s stomach, and cries out in agonized" " pain.", FALSE, ch,
                        0, 0, TO_ROOM);
                    act("$n doubles over and dry heaves.", FALSE, ch, 0, 0, TO_ROOM);
                    if (!generic_damage(ch, number(1, (5 + (MAX_ABILITY - GET_END(ch)))), 0, 0, 0)) {
                        send_to_char("As you heave you feel something inside"
                                     " snap, and the world goes dark.\n\r", ch);
                        /* Ch is dead, so let's leave a death message */
                        if( !IS_NPC(ch) ) {
                            free(ch->player.info[1]);

                            sprintf(buf, "%s has perished from terradin in room %d. Yuck.",
                                GET_NAME(ch), ch->in_room->number);
                            ch->player.info[1] = strdup(buf);
                            gamelog(buf);
                        }
                        /* End death message segment */
                        die(ch);
                        return FALSE;
                    }
                }
            }
        }
        return TRUE;

    default: /* unknown poison */
        break;
    }
    return TRUE;
}


/* This function is called every RL second */
int
update_stats(CHAR_DATA *ch) {
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    int chance, hps, gain, stamina;
    struct weather_node *wn;

    d = ch->desc;

    // if there's a valid character, using infobar
    if (d && !d->connected && d->term && IS_SET(ch->specials.act, CFL_INFOBAR)){
        ch->old.hp = GET_HIT(ch);
        ch->old.max_hp = GET_MAX_HIT(ch);
        ch->old.mana = GET_MANA(ch);
        ch->old.max_mana = GET_MAX_MANA(ch);
        ch->old.move = GET_MOVE(ch);
        ch->old.max_move = GET_MAX_MOVE(ch);
        ch->old.stun = GET_STUN(ch);
        ch->old.max_stun = GET_MAX_STUN(ch);
    }

    /* Health/Stamina draining if thirst == 0 - Moved out of stun block -Morg */
    if (!IS_NPC(ch) && GET_COND(ch, THIRST) == 0) {
        hps = 0;
        stamina = 0;

        /* chance being higher makes it less likely */
        chance = GET_END(ch);
        /* old code, caused lower endurance to be better
         * at resisting effects */
        /* chance = REGEN_MAX(ch) - GET_END(ch)) / 2); */

        /* if it's evening reduces chances */
        if ((time_info.hours < MORNING)
         || (time_info.hours > AFTERNOON))
            chance += 4;

        /* room is sector inside reduces chances */
        switch( ch->in_room->sector_type ) {
        case SECT_INSIDE:
            chance += 4;
            break;
        case SECT_CITY:
            chance += 3;
            break;
        case SECT_DESERT:
            chance -= 4;
            break;
        default:
            break;
        }

        /* room is set indoors reduces chances */
        if (IS_SET(ch->in_room->room_flags, RFL_INDOORS))
            chance += 4;

        /* restful shade reduces chances */
        if (IS_SET(ch->in_room->room_flags, RFL_RESTFUL_SHADE))
            chance += 4;

        /* standing or worse improves chances */
        if (GET_POS(ch) >= POSITION_STANDING)
            chance -= 4;

        /* high sun improves chances */
        if (time_info.hours == HIGH_SUN)
            chance -= 3;

        chance = MAX(1, chance);
        chance = MIN(chance, 30);

        if (affected_by_spell(ch, SPELL_REGENERATE))
            chance *= 2;

        if (GET_MOVE(ch) > 0) {
            stamina = 2;
            if (!number(0, chance)) {
                adjust_move(ch, -stamina);
                send_to_char("You suffer from dehydration.\n\r", ch);
            }
        }

        if (GET_MOVE(ch) <= 0) {
            hps = 5;
            if (!number(0, chance)) {
                if (!generic_damage(ch, hps, 0, 0, 0)) {
                    send_to_char("You die from dehydration.\n\r", ch);
                    sprintf(buf, "%s %s%s%shas died from dehydration in room #%d.",
                     GET_NAME(ch), !IS_NPC(ch) ? "(" : "",
                     !IS_NPC(ch) ? ch->account : "", 
                     !IS_NPC(ch) ? ") " : "",
                     ch->in_room->number);

                    if (!IS_NPC(ch)) {
                        struct time_info_data playing_time =
                            real_time_passed((time(0) - ch->player.time.logon) +
                             ch->player.time.played, 0);

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
                    return FALSE;
                }

                send_to_char("You suffer from dehydration.\n\r", ch);
                if (!affected_by_spell(ch, POISON_GRISHEN)
                    && number(1, 20) < 6) {
                    adjust_move(ch, 15);
                    send_to_char("You feel a burst of energy.\n\r", ch);
                }
            }
            update_pos(ch);
        }
    }

    // stunned block 
    if (GET_POS(ch) > POSITION_STUNNED 
     || (GET_POS(ch) > POSITION_MORTALLYW 
      && (IS_NPC(ch) || GET_COND(ch, FULL) == 0 || GET_COND(ch, THIRST) == 0))){
        /* Update hitpoints/stamina/mana if needed, poison/fighting block */
        if ( GET_POS(ch) != POSITION_FIGHTING && !poison_stop_hps(ch)) {     
            if ((GET_RACE(ch) == RACE_VAMPIRE)
                && (((time_info.hours) != 8) && ((time_info.hours) != 0))
                && (!IS_SET(ch->in_room->room_flags, RFL_INDOORS))) {
                /* send_to_char( "The sun stings your eyes, making it"
                 * " difficult to concentrate.\n\r",ch ); */
            } else {        /* vampire block */
                /* gain back some health */
                hps = GET_MAX_HIT(ch) * (.55);

                // if they're hurt, and not more than half hurt
                // or sleeping, mantis or elemental
                // and an npc or not thirsty/hungry
                if (GET_HIT(ch) < GET_MAX_HIT(ch)
                 && (GET_HIT(ch) > hps
                   || GET_POS(ch) == POSITION_SLEEPING 
                   || GET_RACE(ch) == RACE_MANTIS
                   || GET_RACE(ch) == RACE_ELEMENTAL)
                 && (IS_NPC(ch) || GET_COND(ch, THIRST) != 0
                  || GET_COND(ch, FULL) != 0)) {

                    switch(GET_GUILD(ch)) {
                    case GUILD_WARRIOR:
                        gain = 3;
                        break;
                    case GUILD_RANGER:
                    case GUILD_TEMPLAR:
                        gain = 2;
                        break;
                    default:
                        gain = 1;
                        break;
                    }

                    chance = ((REGEN_MAX(ch) - GET_END(ch)) / 3) - 1;
                    switch( GET_POS(ch) ) {
                    case POSITION_SITTING:
                        chance -= (chance / 5);
                        break;
                    case POSITION_RESTING:
                        chance -= (chance / 3);
                        break;
                    case POSITION_SLEEPING:
                        chance /= 2;
                        gain++;
                        break;
                    default:
                        break;
                    }

                    if ( GET_RACE(ch) == RACE_MANTIS
                     || GET_RACE(ch) == RACE_ELEMENTAL)
                        gain++;

                    // expensive calculation, skip it for npcs
                    if( !IS_NPC(ch) ) 
                        chance += weather_effects(ch);

                    if (affected_by_spell(ch, SPELL_REGENERATE)) {
                        chance = chance / 2;
                        gain *= 2;
                    }

                    if (IS_NPC(ch) || (GET_COND(ch, THIRST) != 0
                     && GET_COND(ch, FULL) != 0 )) {
                        /* NESSALIN_REGEN_CHANGE_8_24_2002 */
                        gain = gain / 2;
                        if( gain < 1 ) gain = 1;
                        chance = MAX(chance, 1);
                        if (!number(0, chance) && !ch->specials.fighting)
                            adjust_hit(ch, gain);
                    }
                }

                /* gain back some mana */
                if ( GET_MANA(ch) < GET_MAX_MANA(ch)
                 && is_guild_elementalist(GET_GUILD(ch))
                 && !has_skill(ch, SKILL_GATHER)) {
                    chance = ((45 - GET_WIS(ch)) / 2) - 1;

                    if (IS_SET(ch->in_room->room_flags, RFL_INDOORS)
                     && ((GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST)
                      || (GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST)
                      || (GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST))) {
                        chance *= 3;
                    } else if (GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST) {
                        struct weather_node *wn;

                        if (weather_info.wind_velocity >= 15)
                            chance -= (weather_info.wind_velocity / 5);
                        else
                            chance += (15 - weather_info.wind_velocity);

                        /* this is from weather_effects()
                         * need to counteract it, so we're applying the
                         * reverse
                         */
                        if (!affected_by_spell(ch, SPELL_WIND_ARMOR)) {
                            double w = 0.0;

                            wn = find_weather_node(ch->in_room);
                            if (wn)
                                w = wn->condition;

                            if (w >= 2.50)
                                chance -= (int) w *5;
                            else if (w >= 1.00)
                                chance -= (int) w *6;
                        }
                    } else if (GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST) {
                        if (weather_info.sunlight == SUN_DARK || use_global_darkness)
                            chance = chance * 2;
                        else if (weather_info.sunlight == SUN_ZENITH)
                            chance = chance / 2;
                    } else if (GET_GUILD(ch) == GUILD_VOID_ELEMENTALIST) {
                        if (ch->in_room
                            && (ch->in_room->sector_type == SECT_NILAZ_PLANE
                                || IS_SET(ch->in_room->room_flags, RFL_NO_ELEM_MAGICK)))
                            chance = chance / 2;
                    } else if (GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST) {
                        if (ch->in_room && IS_DARK(ch->in_room))
                            chance = chance / 2;
                    }

                    switch (GET_POS(ch) ) {
                    case POSITION_SITTING:
                        chance -= (chance / 5);
                        break;
                    case POSITION_RESTING:
                        chance -= (chance / 3);
                        break;
                    case POSITION_SLEEPING:
                        chance = (chance / 2);
                        break;
                    default:
                        break;
                    }

                    if (IS_SET(ch->in_room->room_flags, RFL_DMANAREGEN))
                        chance = chance / 2;

                    chance += weather_effects(ch);

                    // Alter Lightning Elementalist regen rates based
                    // on storminess.
                    for (wn = wn_list; wn; wn = wn->next) {
                        if (wn->zone == ch->in_room->zone)
                            break;
                    }       // End search for weather node

                    // If we have weather here, see if we apply a bonus
                    if (wn && GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST
                        && !IS_SET(ch->in_room->room_flags, RFL_INDOORS))
                        chance -= (int) wn->condition * 6;
                    /* This is a *6 bonus because the call to
                     * weather_effects() is applied at
                     * least a *5 if not *6 penalty based on
                     * the storminess of the area. */

                    chance = MAX(chance, 1);
                    /* If the room does not prevent elemental magick, or
                     * the caster is a void elementalst, allow the regen
                     * to occur.
                     */
                    if (!IS_SET(ch->in_room->room_flags, RFL_NO_ELEM_MAGICK)
                     || (GET_GUILD(ch) == GUILD_VOID_ELEMENTALIST)) {
                        if (!number(0, chance))
                            adjust_mana(ch, 5);
                    }
                }

                /* gain back some stamina */
                if (GET_MOVE(ch) < GET_MAX_MOVE(ch)
                 && (IS_SET(ch->specials.act, CFL_MOUNT)
                  || (GET_POS(ch) < POSITION_FIGHTING))) {
                    if (GET_MOVE(ch) < GET_MAX_MOVE(ch) * .9 
                     || GET_POS(ch) == POSITION_SLEEPING
                     || GET_RACE(ch) == RACE_MANTIS) {
                        chance = ((REGEN_MAX(ch) - GET_END(ch)) / 2) - 1;

                        if (GET_POS(ch) == POSITION_SITTING)
                            chance -= chance / 5;
                        else if (GET_POS(ch) == POSITION_RESTING)
                            chance -= chance / 3;
                        else if (GET_POS(ch) == POSITION_SLEEPING)
                            chance /= 2;

                        if (ch->in_room->sector_type == SECT_DESERT)
                            chance += 2;
                        else if (ch->in_room->sector_type == SECT_INSIDE)
                            chance -= 2;

                        /* If you're thirsty, you wont recooperate as quickly */
                        if (GET_COND(ch, THIRST) <= 2)
                            chance += 7;
                        else if (GET_COND(ch, THIRST) < 4)
                            chance += 3;
                        else if (GET_COND(ch, THIRST) < 6)
                            chance += 2;
                        else if (GET_COND(ch, THIRST) < 8)
                            chance++;

                        if( !IS_NPC(ch) ) 
                            chance += weather_effects(ch);

                        chance = MAX(chance, 1);

                        if ((GET_RACE(ch) == RACE_MUL)
                         && affected_by_spell(ch, AFF_MUL_RAGE))
                            chance /= 2;

                        gain = 8;
                        if (GET_GUILD(ch) == GUILD_RANGER)
                            gain = 12;

                        if (affected_by_spell(ch, SPELL_REGENERATE)) {
                            gain += 6;
                            chance /= 2;
                        }

                        if (affected_by_spell(ch, POISON_GRISHEN))
                            gain = 0;

                        if (GET_COND(ch, THIRST) != 0 && GET_COND(ch, FULL) != 0
                         && GET_RACE(ch) != RACE_SHADOW && !number(0, chance)
                         && !ch->specials.fighting) {
                            adjust_move(ch, gain);
                        }
                    }
                }
            }
        }

        // regain stun points, poison, fighting doesn't block 
        if (GET_STUN(ch) < GET_MAX_STUN(ch)) {
            chance = ((REGEN_MAX(ch) - GET_END(ch)) / 4) - 1;

            switch( GET_GUILD(ch) ) {
            case GUILD_WARRIOR:
            case GUILD_PSIONICIST:
                gain = 3;
                break;
            case GUILD_RANGER:
            case GUILD_TEMPLAR:
            case GUILD_LIRATHU_TEMPLAR:
            case GUILD_JIHAE_TEMPLAR:
                gain = 2;
                break;
            default:
                gain = 1;
                break;
            }

            switch( GET_POS(ch) ) {
            case POSITION_SITTING:
                chance -= (chance / 5);
                gain++;
                break;
            case POSITION_RESTING:
                chance -= (chance / 3);
                gain += 2;
                break;
            case POSITION_SLEEPING:
                chance /= 2;
                gain = 5;
                break;
            default:
                break;
            }

            if (knocked_out(ch))
                gain = 0;

            if( !IS_NPC(ch) )
                chance += weather_effects(ch) / 4;

            chance = MAX(chance, 1);
            chance = MIN(chance, 3); /* fairly high chance of improvement */

            if (affected_by_spell(ch, SPELL_REGENERATE)) {
                chance = chance / 2;
                gain *= 2;
            }

            if (!number(0, chance))
                adjust_stun(ch, gain);
        }
    } else if (GET_POS(ch) > POSITION_MORTALLYW) {
        if ((30 + GET_END(ch)) > number(1, 100)) {
            chance = ((REGEN_MAX(ch) - GET_END(ch)) / 3) - 1;
            chance *= 2;

            if (!IS_NPC(ch)
             && (GET_COND(ch, THIRST) == 0 || GET_COND(ch, FULL) == 0))
                chance *= 5;

            if( !IS_NPC(ch) ) 
               chance += weather_effects(ch);

            chance = MAX(chance, 1);
            if (affected_by_spell(ch, SPELL_REGENERATE))
                chance /= 2;
            if (!number(0, chance)
             && !affected_by_spell(ch, POISON_METHELINOC)) {
                adjust_hit(ch, 1);
                update_pos(ch);
            }
        } else {
            chance = REGEN_MAX(ch) + GET_END(ch) - 1;
            chance *= 2;

            if ((GET_COND(ch, THIRST) == 0)
                || (GET_COND(ch, FULL) == 0))
                chance /= 5;

            if( !IS_NPC(ch) ) 
                chance -= weather_effects(ch);

            chance = MAX(chance, 1);
            if (!number(0, chance)
             && !affected_by_spell(ch, SPELL_REGENERATE))
                if (!ws_damage(ch, ch, 1, 0, 0, 1, TYPE_SUFFERING, 0))
                    return FALSE;
        }
    } else if (GET_POS(ch) > POSITION_DEAD) {
        if ((10 + GET_END(ch)) > number(1, 100)) {
            chance = REGEN_MAX(ch) - GET_END(ch) - 1;
            chance *= 2;
            if ((GET_COND(ch, THIRST) == 0)
                || (GET_COND(ch, FULL) == 0))
                chance *= 5;

            if( !IS_NPC(ch) ) 
                chance += weather_effects(ch);

            chance = MAX(chance, 1);
            gain = ((affected_by_spell(ch, POISON_METHELINOC)) ? 0 : 1);
            if (!number(0, chance)) {
                adjust_hit(ch, gain);
                update_pos(ch);
            }
        } else {
            chance = REGEN_MAX(ch) + GET_END(ch) - 1;
            chance *= 2;

            if ((GET_COND(ch, THIRST) == 0)
                || (GET_COND(ch, FULL) == 0))
                chance /= 5;

            if( !IS_NPC(ch) ) 
                chance -= weather_effects(ch);

            chance = MAX(chance, 1);
            if (!number(0, chance)
             && !affected_by_spell(ch, SPELL_REGENERATE))
                if (!ws_damage(ch, ch, 1, 0, 0, 1, TYPE_SUFFERING, 0))
                    return FALSE;
        }
    }

    if (GET_RACE(ch) == RACE_MUL && GET_POS(ch) == POSITION_FIGHTING)
        flag_mul_rage(ch);

    if (!IS_NPC(ch) && IS_SET(ch->specials.act, CFL_AGGRESSIVE))
        ch_act_agg(ch);
    return TRUE;
}

/* Age effects for stats, and limits for mana and hits and moves */

int
age_class(int virt_age)
{
    int n;

    for (n = 1; year_limit[n] != 120; n++) {
        if ((virt_age > year_limit[n - 1]) && (virt_age <= year_limit[n]))
            return (n - 1);
    }
    return (6);
}

int
get_virtual_age(int years, int race)
{
    int vage;

    switch (race) {
    case RACE_DWARF:
        vage = (years * 100) / 220;
        break;
    case RACE_ELF:
    case RACE_DESERT_ELF:
        vage = (years * 100) / 116;
        break;
    case RACE_HALF_ELF:
        vage = (years * 100) / 108;
        break;
    case RACE_HALF_GIANT:
        vage = (years * 100) / 180;
        break;
    case RACE_HALFLING:
        vage = (years * 100) / 244;
        break;
    case RACE_MUL:
        vage = (years * 100) / 75;
        break;
    case RACE_MANTIS:
        vage = (years * 100) / 29;
        break;
    case RACE_GITH:
        vage = (years * 100) / 90;
        break;
    case RACE_SUB_ELEMENTAL:
    case RACE_DRAGON:
    case RACE_AVANGION:
    case RACE_DEMON:
        vage = (years * 100) / 1000;
        break;
    default:
        vage = years;
        break;
    }
    return (vage);
}


void
age_effect(int years, int *stat, int race, int eco_bonus, int type)
{
    int n, alt, virtual_age, ac = 0;
    int base_stat;
    float alt_stat;

    int mod_hits[] = { 0, 0, 2, 0, -1, -2, -3 };
    int mod_mana[] = { 0, 0, 0, 1, 2, 4, 8 };
    int mod_stam[] = { 0, 1, 2, 1, -1, -3, -5 };

    base_stat = *stat;
    virtual_age = get_virtual_age(years, race);

    virtual_age -= eco_bonus;   /* minus to make "younger" */
    alt = 0;

    for (n = 1; n <= virtual_age; n++) {
        ac = age_class(n);
        if (type == 1)          /* health */
            alt += mod_hits[ac];
        else if (type == 2)     /* mana */
            alt += mod_mana[ac];
        else                    /* stamina */
            alt += mod_stam[ac];
    }
    alt_stat = (alt / 100);
    *stat += (int) (base_stat * alt_stat);
}


int
hit_limit(CHAR_DATA * ch)
{
    int pts;

    pts = ch->points.max_hit;
    /* if (!IS_NPC (ch)) */
    pts += end_app[GET_END(ch)].end_saves;

    /* I took this out in lieu of the new aging system -hal
     * if (ch->specials.eco) 
     * alt = (ch->specials.eco / 3);
     * if (!IS_NPC(ch))
     * age_effect(age(ch).year, &pts, GET_RACE(ch), alt, 1); */

    /* if (GET_RACE(ch) == RACE_DESERT_ELF)
     * {
     * pts = (2 * end_app[GET_END(ch)].end_saves);
     * pts += ch->points.max_hit;
     * } */

    /* Can put a case statement here for races, if it gets necessary */
    /*
     * switch (GET_RACE(ch)) {
     * case RACE_HALF_GIANT:
     * pts += 50;
     * case RACE_MEKILLOT:
     * pts += 150;
     * case RACE_KANK:
     * pts += 75;
     * case RACE_INIX:
     * pts += 50;
     * }
     */

    if (get_char_size(ch) > 20) {
        if (get_char_size(ch) < 50)
            pts += (get_char_size(ch) * 2);
        else
            pts += (get_char_size(ch) * 3);
    }

    // removed min cap so generic poison can kill
    // pts = MAX(pts, GET_END(ch) * 2);
    pts = MIN(pts, GET_END(ch) * 10);   /* maximum possible hits = 1000 */

    pts *= spice_factor(ch);

    return (pts);
}

extern int psionic_affects[];

int
stun_limit(CHAR_DATA * ch)
{
    int pts, end_mod, i, adj_end;
    struct affected_type *af = NULL;

    end_mod = 0;
    pts = ch->points.max_stun;

    // Check for psionics that drain stunpoints
    if (IS_AFFECTED(ch, CHAR_AFF_PSI)) {
        for (i = 0; psionic_affects[i] > 0; i++) {
            af = affected_by_spell(ch, psionic_affects[i]);
            if (af) {
		switch (psionic_affects[i]) {
		case PSI_MINDWIPE:
		case PSI_BARRIER:
		    end_mod += 4;
		    break;
		case PSI_HEAR:
		case PSI_VANISH:
		    end_mod += (4 + af->power); // 0-based
		    break;
		default:
		    end_mod += 2;
		    break;
		}
	    }
	}
    }

    adj_end = MAX(0, GET_END(ch) - end_mod);
    adj_end = MIN(100, adj_end);

    pts += end_app[adj_end].end_saves;

    // -5 to max stun if actively watching
    if( is_char_watching(ch) )
        pts -= 5 - ceil(3.0 * (get_skill_percentage(ch, SKILL_WATCH)) / 100.0);

    if(ch->specials.guarding 
     || ch->specials.dir_guarding > -1 
     ||  ch->specials.obj_guarding) 
        pts -= 10 - ceil(6.0 * get_skill_percentage(ch, SKILL_GUARD) / 100.0);

    /* Can put a case statement here for races, if it gets necessary */
/*
  switch (GET_RACE(ch)) {
    case RACE_MEKILLOT:
      pts += 150;
    case RACE_KANK:
      pts += 75;
    case RACE_INIX:
      pts += 50;
    case RACE_ASLOK:
      pts += 100;
  }
*/

    if (get_char_size(ch) > 20) {
        if (get_char_size(ch) < 50)
            pts += (get_char_size(ch) * 2);
        else
            pts += (get_char_size(ch) * 3);
    }

    pts = MAX(pts, (GET_END(ch) - end_mod) * 2);
    pts = MIN(pts, (GET_END(ch) - end_mod) * 10);

    pts *= spice_factor(ch);

    return (pts);
}


int
mana_limit(CHAR_DATA * ch)
{
    int pts = 100;

    /* Add some variance to mana depending on wisdom.. */

    pts += ((GET_WIS(ch) - 13) * 3);

    pts = MAX(pts, GET_WIS(ch) * 2);
    pts = MIN(pts, GET_WIS(ch) * 12);   /* max possible mana = 300 */
    if (GET_RACE(ch) == RACE_HALF_GIANT)
        pts -= 40;
    if (GET_RACE(ch) == RACE_MUL)
        pts -= 15;

    /* This is so that any elementalist always has the minimum of 50
     * mana in order to cast a spell.  Spice affects can increase/decrease
     * this value. -Tiernan 11/23/04 */
    /* Changed to any mage, including sorcerers and paths of magick subguilds
     * -Tiernan 12/07/06 */
    if (is_magicker(ch))
        pts = MAX(pts, 50);

    pts *= spice_factor(ch);

    // Add all mana bonuses after the wisdom and spice affects
    pts += ch->points.mana_bonus;

    return (pts);
}

int
move_limit(CHAR_DATA * ch)
{
    int pts, bon = 0;
//	int alt = 0;
//    int stam;
    int stam_mod = 0;

//    stam = GET_END(ch) - stam_mod;

    pts = 100 + end_app[GET_END(ch)].move_bonus + ch->points.move_bonus;

//    if (ch->specials.eco != 0)
//        alt = (ch->specials.eco / 3);
    switch (GET_RACE(ch)) {     /* case statements for race to determine movement bonus */
    case RACE_VAMPIRE:
        bon += ((GET_COND(ch, FULL) - 6) * 3);
        break;
    case RACE_ERDLU:
        bon += 75;
        break;
    case RACE_THLON:
        bon += 75;
        break;
    case RACE_GWOSHI:
    case RACE_PLAINSOX:
    case RACE_RATLON:
        bon += 200;
        break;
    case RACE_CHARIOT:
    case RACE_WAR_BEETLE:
    case RACE_KANK:
        bon += 300;
        break;
    case RACE_HORSE:
        bon += 75;
        break;
    case RACE_SUNBACK:
    case RACE_SUNLON:
    case RACE_CHEOTAN:
    case RACE_INIX:
        bon += 225;
        break;
    case RACE_MEKILLOT:
        bon += 500;
        break;

        /* They already get a discount on movement cost, and stamina
         * is used for other things.  Double dipping. -Nessalin 3/29/2003 */
        //    case RACE_ELF:
        //      bon += 60;
        //      break;

        /* Leaving them with a small bonus over their city coutnerparts */
        /* -Nessalin 3/29/2003 */

    case RACE_DESERT_ELF:
        bon += 60;
        break;

    case RACE_GITH:
        bon += 185;
        break;
    case RACE_MANTIS:
        bon += 35;
        break;
    case RACE_HALF_GIANT:
        bon += 20;
        break;
    case RACE_MUL:
        bon += 20;
        break;
    default:
        bon += 0;
        break;
    }                           /* case statements for race to determine movement bonus */

    /* taken out in lieu of new aging system -hal
     * if (!IS_NPC(ch))
     * age_effect(age(ch).year, &pts, GET_RACE(ch), alt, 3); */

    /* why bother with this code, just use what you calculated
     * pts = MAX( pts + bon, bon + end_app[GET_END(ch)].move_bonus * 3 ); */

    pts += bon;
    pts = MIN(pts, 1000);       /* max possible moves = 1000 */

    pts *= spice_factor(ch);

    return (pts);
}

int
is_char_drunk(CHAR_DATA * ch)
{
    int drunk_val, tol;

    if (has_skill(ch, TOL_ALCOHOL))
        /*  if (ch->skills[TOL_ALCOHOL]) */
        tol = ch->skills[TOL_ALCOHOL]->learned;
    else
        tol = 0;

    drunk_val = (GET_COND(ch, DRUNK) - tol);
    if (drunk_val < 1)
        return 0;
    if (drunk_val < 5)
        return DRUNK_LIGHT;
    if (drunk_val < 10)
        return DRUNK_MEDIUM;
    if (drunk_val < 20)
        return DRUNK_HEAVY;
    if (drunk_val < 25)
        return DRUNK_SMASHED;
    else
        return DRUNK_DEAD;
}

