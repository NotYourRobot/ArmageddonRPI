/*
 * File: NPC.C
 * Usage: Routines for NPC intelligence.
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
 * 05/16/2000: Fiddled things to try to make sure NPC elementalists
 *             will cast in combat.  -Sanvean
 * 05/17/2000: Added check for POSITION_STANDING in cmd_flee.  -San
 */

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "limits.h"
#include "db.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "guilds.h"
#include "clan.h"
#include "skills.h"
#include "cities.h"
#include "utility.h"
#include "memory.h"
#include "modify.h"
#include "psionics.h"

extern int lawful_races_in_allanak(CHAR_DATA * ch);
extern void send_comm_to_reciters(CHAR_DATA * ch, const char *output);


/* local variables */
int npc_act_soldier(CHAR_DATA * ch);
void go_to_jail(CHAR_DATA * soldier);
void drop_dung(CHAR_DATA * ch);

bool
lawful_avoid_combat(CHAR_DATA * ch, CHAR_DATA * target)
{
    int ct, dir;
    CHAR_DATA *tch;
    ROOM_DATA *rm;

    /* if you're not a lawful humanoid, don't avoid combat */
    if (!is_lawful_humanoid(ch))
        return FALSE;

    /* if your target isn't in a city, don't avoid combat */
    if (!(ct = room_in_city(target->in_room)))
        return FALSE;

    /* if you're in the tribe for the city the person's in, don't avoid combat */
    if (IS_TRIBE(ch, city[ct].tribe))
        return FALSE;

    /* if target isn't in a populated room, and there aren't any cops around,
     * don't avoid combat */
    if (!IS_SET(target->in_room->room_flags, RFL_POPULATED)) {
        for (tch = target->in_room->people; tch; tch = tch->next_in_room)
            if (IS_TRIBE(tch, city[ct].tribe) && !IS_IMMORTAL(tch)
                && AWAKE(tch))
                break;

        for (dir = 0; (dir < 6) && !tch; dir++)
            if (target->in_room->direction[dir]
                && (rm = target->in_room->direction[dir]->to_room) != NULL)
                for (tch = rm->people; tch; tch = tch->next_in_room)
                    if (IS_TRIBE(tch, city[ct].tribe) && !IS_IMMORTAL(tch)
                        && AWAKE(tch))
                        break;

        if (!tch)
            return FALSE;
    }

    return TRUE;
}

int
is_target(CHAR_DATA * ch, CHAR_DATA * victim)
{
    int i;

    if (!CAN_SEE(ch, victim))
        return FALSE;

    if ((GET_RACE(ch) == RACE_JAKHAL)
        && (IS_SET(victim->specials.act, CFL_MOUNT))) {
        if (!does_hate(ch, victim))
            add_to_hates(ch, victim);
        return TRUE;
    }

    if (GET_RACE(ch) == RACE_WEZER
        && (knocked_out(victim) || IS_SET(victim->specials.act, CFL_FROZEN)))
        return FALSE;

    if ((GET_RACE(ch) == RACE_HALFLING) && (GET_RACE(victim) == RACE_HALFLING))
        return 0;

    if (IS_IN_SAME_TRIBE(ch, victim))
        return 0;

    if (victim->master && IS_IN_SAME_TRIBE(ch, victim->master))
        return 0;

    if (IS_SET(ch->specials.act, CFL_SCAVENGER)
        && ch->skills[SKILL_PEEK]
        && is_lawful_humanoid(ch)
        && !AWAKE(victim))
        return FALSE;

    /* humanoid scavenger aggresives look for high priced items before aggro */
    if (IS_SET(ch->specials.act, CFL_SCAVENGER)
        && ch->skills[SKILL_PEEK]
        && is_lawful_humanoid(ch)
        && AWAKE(victim)) {
        /* look for anything over 60 coins */
        /* changed from 100 to 60 at Zagren's request -Morg */
        for (i = 0; i < MAX_WEAR; i++) {
            if (victim->equipment[i] != NULL && !is_covered(ch, victim, i)
                && (victim->equipment[i]->obj_flags.cost > 60)) {
                return TRUE;
            }
        }
        /* If we get here, then they're not worth attacking
         * ... unless we hate them. */
        return does_hate(ch, victim);
    }

    if (IS_TRIBE(ch, 34)        /* ALA */
        &&IS_TRIBE(victim, TRIBE_ALLANAK_MIL)) /* Allanaki Militia */
        return 1;

    if (IS_TRIBE(ch, 34) && !IS_TRIBE(victim, TRIBE_ALLANAK_MIL))
        return 0;

/* Added to keep ethereal baddies from attacking non-ethereals */
#define ETHEREAL_SMART_TARGETTING
#ifdef ETHEREAL_SMART_TARGETTING
    if (is_char_ethereal(ch) != is_char_ethereal(victim))
        return 0;
#endif

    if (CAN_SEE(ch, victim)
        && !IS_IN_SAME_TRIBE(ch, victim)
        && (!IS_SET(victim->specials.act, CFL_MOUNT))
        && (!IS_IMMORTAL(victim))
        && (GET_POS(ch) == POSITION_STANDING)
        && (!ch->specials.fighting))
        return 1;

    return does_hate(ch, victim);
}

int
is_thief_target(CHAR_DATA * ch, CHAR_DATA * victim)
{
    int ct;
    ct = room_in_city(ch->in_room);
    if ((ct) && IS_TRIBE(victim, city[ct].tribe))
        return FALSE;

    if (!CAN_SEE(ch, victim))
        return FALSE;

    if ((is_merchant(victim)) || (IS_SET(victim->specials.act, CFL_MOUNT))
        || IS_IN_SAME_TRIBE(victim, ch))
        return FALSE;

    return TRUE;
}

int
max_npc_spell_power(CHAR_DATA * ch, int spell)
{
    int lev, n;

    if (!(ch->skills[spell]))
        return -1;

    n = 0;

    if (GET_MANA(ch) >= 50)
        n = 1;
    else if (GET_MANA(ch) >= 33)
        n = 2;
    else if (GET_MANA(ch) >= 20)
        n = 3;
    else if (GET_MANA(ch) >= 10)
        n = 4;
    else
        return -1;

    lev = (ch->skills[spell]->rel_lev) - n;
    lev = MAX(lev, 0);
    lev = MIN(lev, 6);

    return lev;

}

/* Note from Sanvean - this (below) doesn't seem to be getting called at all.  */

int
wind_cleric_act_combat(CHAR_DATA * ch, CHAR_DATA * fighting)
{
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];

    if ((ch->skills[SPELL_BANISHMENT]) && (!(IS_SET(ch->in_room->room_flags, RFL_INDOORS)))
        && (max_npc_spell_power(ch, SPELL_BANISHMENT) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un whira locror hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_BANISHMENT)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_REPEL]) && (max_npc_spell_power(ch, SPELL_REPEL) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un whira locror chran' %s",
                    Power[max_npc_spell_power(ch, SPELL_REPEL)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_TELEPORT]) && (max_npc_spell_power(ch, SPELL_TELEPORT) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un whira locror viod' %s",
                    Power[max_npc_spell_power(ch, SPELL_TELEPORT)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    return 0;
}

int
water_cleric_act_combat(CHAR_DATA * ch, /* caster */
                        CHAR_DATA * fighting)
{                               /* hapless victim */
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];

    /* cast at less than 50% HitPoints */
    if ((GET_HIT(ch) < (GET_MAX_HIT(ch) / 2)) && (ch->skills[SPELL_HEAL])
        && (max_npc_spell_power(ch, SPELL_HEAL) > -1)) {
        /* Vivaduan, Heal Thyself */
        sprintf(buf, "cast '%s un vivadu viqrol wril' me",
                Power[max_npc_spell_power(ch, SPELL_HEAL)]);
        parse_command(ch, buf);
        qgamelogf(QUIET_NPC_AI, "%s", buf);
        return 1;
    }

    if ((!IS_CALMED(fighting)) && (ch->skills[SPELL_CALM])
        && (max_npc_spell_power(ch, SPELL_CALM) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un vivadu psiak chran' %s",
                    Power[max_npc_spell_power(ch, SPELL_CALM)], name);
            parse_command(ch, buf);
            qgamelogf(QUIET_NPC_AI, "%s", buf);
            return 1;
        }
        return 0;
    }

    if (!(affected_by_spell(fighting, SPELL_POISON)) && (ch->skills[SPELL_POISON])
        && (max_npc_spell_power(ch, SPELL_POISON) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un vivadu viqrol hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_POISON)], name);
            parse_command(ch, buf);
            qgamelogf(QUIET_NPC_AI, "%s", buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_HEALTH_DRAIN]) && (max_npc_spell_power(ch, SPELL_HEALTH_DRAIN) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un vivadu divan hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_HEALTH_DRAIN)], name);
            parse_command(ch, buf);
            qgamelogf(QUIET_NPC_AI, "%s", buf);
            return 1;
        }
        return 0;
    }

    /*  if ((!IS_AFFECTED (ch, CHAR_AFF_INVULNERABILITY)) &&
     * (ch->skills[SPELL_INVULNERABILITY]) &&
     * (max_npc_spell_power (ch, SPELL_INVULNERABILITY) > -1))
     * {
     * sprintf (buf, "cast '%s un vivadu pret grol' me",
     * Power[max_npc_spell_power (ch, SPELL_INVULNERABILITY)]);
     * parse_command (ch, buf);
     * return 1;
     * }
     */

    if (!IS_AFFECTED(fighting, CHAR_AFF_SILENCE) && (ch->skills[SPELL_SILENCE])) {
        switch (GET_GUILD(fighting)) {
        case GUILD_TEMPLAR:
        case GUILD_LIRATHU_TEMPLAR:
        case GUILD_JIHAE_TEMPLAR:
        case GUILD_LIGHTNING_ELEMENTALIST:
        case GUILD_VOID_ELEMENTALIST:
        case GUILD_WATER_ELEMENTALIST:
        case GUILD_WIND_ELEMENTALIST:
        case GUILD_STONE_ELEMENTALIST:
        case GUILD_FIRE_ELEMENTALIST:
        case GUILD_DEFILER:
            if (max_npc_spell_power(ch, SPELL_SILENCE) > -1) {
                get_arg_char_room_vis(ch, fighting, name);
                if (*name) {
                    sprintf(buf, "cast '%s un vivadu psiak hurn' %s",
                            Power[max_npc_spell_power(ch, SPELL_SILENCE)], name);
                    parse_command(ch, buf);
                    qgamelogf(QUIET_NPC_AI, "%s", buf);
                    return 1;
                }
                return 0;
            }
            break;
        default:
            break;
        }
        return 0;
    }

    return 0;
}

int
stone_cleric_act_combat(CHAR_DATA * ch, CHAR_DATA * fighting)
{
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];
    char nothing[MAX_INPUT_LENGTH] = "";

    if ((IS_AFFECTED(fighting, CHAR_AFF_CALM)) && (GET_HIT(ch) <= (GET_MAX_HIT(ch) / 2))) {
        cmd_flee(ch, nothing, 0, 0);
        return 1;
    }

    /* commenting out, because Ruks don't get this spell anymore.
     * if ((!IS_AFFECTED (fighting, CHAR_AFF_CALM)) &&
     * (GET_HIT (ch) <= (GET_MAX_HIT (ch) / 2)) &&
     * (ch->skills[SPELL_CALM]) &&
     * (max_npc_spell_power (ch, SPELL_CALM) > -1))
     * {
     * get_arg_char_room_vis (ch, fighting, name);
     * if (*name)
     * {
     * sprintf (buf, "'%s un vivadu psiak chran' %s",
     * Power[max_npc_spell_power (ch, SPELL_CALM)], name);
     * cmd_cast (ch, buf, 83);
     * return 1;
     * }
     * return 0;
     * }
     */

    if ((!affected_by_spell(ch, SPELL_STONE_SKIN)) && (ch->skills[SPELL_STONE_SKIN])
        && (max_npc_spell_power(ch, SPELL_STONE_SKIN) > -1)) {
        sprintf(buf, "cast '%s un krok pret grol' me",
                Power[max_npc_spell_power(ch, SPELL_STONE_SKIN)]);
        parse_command(ch, buf);
        return 1;
    }

    if ((!IS_AFFECTED(fighting, CHAR_AFF_FEEBLEMIND)) && (ch->skills[SPELL_FEEBLEMIND])) {

        switch (GET_GUILD(fighting)) {
        case GUILD_TEMPLAR:
        case GUILD_LIRATHU_TEMPLAR:
        case GUILD_JIHAE_TEMPLAR:
        case GUILD_WATER_ELEMENTALIST:
        case GUILD_WIND_ELEMENTALIST:
        case GUILD_STONE_ELEMENTALIST:
        case GUILD_FIRE_ELEMENTALIST:
        case GUILD_DEFILER:
            if (max_npc_spell_power(ch, SPELL_FEEBLEMIND) > -1) {
                get_arg_char_room_vis(ch, fighting, name);
                if (*name) {
                    sprintf(buf, "cast '%s un suk-krath mutur hurn' %s",
                            Power[max_npc_spell_power(ch, SPELL_FEEBLEMIND)], name);
                    parse_command(ch, buf);
                    return 1;
                }
                return 0;
            }
            break;
        default:
            break;
        }
        return 0;
    }

    if ((!affected_by_spell(ch, SPELL_STRENGTH)) && (ch->skills[SPELL_STRENGTH])
        && (max_npc_spell_power(ch, SPELL_STRENGTH) > -1)) {
        sprintf(buf, "cast '%s un ruk mutur wril'", Power[max_npc_spell_power(ch, SPELL_STRENGTH)]);
        parse_command(ch, buf);
        return 1;
    }


    if ((!affected_by_spell(fighting, SPELL_WEAKEN)) && (ch->skills[SPELL_WEAKEN])
        && (max_npc_spell_power(ch, SPELL_WEAKEN) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un ruk morz hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_WEAKEN)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_STAMINA_DRAIN]) && (max_npc_spell_power(ch, SPELL_STAMINA_DRAIN) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un ruk divan hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_STAMINA_DRAIN)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_BANISHMENT]) && (max_npc_spell_power(ch, SPELL_BANISHMENT) > 1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "'%s un whira locror hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_BANISHMENT)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_REPEL]) && (GET_HIT(ch) <= (GET_MAX_HIT(ch) / 2))
        && (GET_HIT(ch) < GET_HIT(fighting)) && (max_npc_spell_power(ch, SPELL_REPEL) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un whira locror chran' %s",
                    Power[max_npc_spell_power(ch, SPELL_REPEL)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_BLIND]) && (max_npc_spell_power(ch, SPELL_BLIND) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un drov mutur hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_BLIND)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 1;
    }
    return 0;
}

int
sun_cleric_act_combat(CHAR_DATA * ch, CHAR_DATA * fighting)
{
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];

    if (((IS_AFFECTED(fighting, CHAR_AFF_SANCTUARY))
         || (affected_by_spell(fighting, SPELL_STONE_SKIN))
         || (affected_by_spell(fighting, SPELL_STRENGTH))
         || (affected_by_spell(fighting, SPELL_ARMOR))) && (ch->skills[SPELL_DISPEL_MAGICK])
        && (max_npc_spell_power(ch, SPELL_DISPEL_MAGICK) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un suk-krath psiak echri' %s",
                    Power[max_npc_spell_power(ch, SPELL_DISPEL_MAGICK)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((!IS_AFFECTED(fighting, CHAR_AFF_BLIND)) && (ch->skills[SPELL_BLIND])
        && (max_npc_spell_power(ch, SPELL_BLIND) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un drov mutur hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_BLIND)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((GET_MANA(fighting) >= (GET_MAX_MANA(fighting) / 4)) && (ch->skills[SPELL_AURA_DRAIN])) {
        switch (GET_GUILD(fighting)) {
        case GUILD_TEMPLAR:
        case GUILD_LIRATHU_TEMPLAR:
        case GUILD_JIHAE_TEMPLAR:
        case GUILD_WATER_ELEMENTALIST:
        case GUILD_WIND_ELEMENTALIST:
        case GUILD_STONE_ELEMENTALIST:
        case GUILD_FIRE_ELEMENTALIST:
        case GUILD_DEFILER:
            if (max_npc_spell_power(ch, SPELL_AURA_DRAIN) > -1) {
                get_arg_char_room_vis(ch, fighting, name);
                if (*name) {
                    sprintf(buf, "cast '%s un suk-krath divan hurn' %s",
                            Power[max_npc_spell_power(ch, SPELL_AURA_DRAIN)], name);
                    parse_command(ch, buf);
                    return 1;
                }
                return 0;
            }
            break;
        default:
            break;
        }
        return 0;
    }

    if ((ch->skills[SPELL_DEMONFIRE]) && (max_npc_spell_power(ch, SPELL_DEMONFIRE) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un suk-krath morz hekro' %s",
                    Power[max_npc_spell_power(ch, SPELL_DEMONFIRE)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_FIREBALL]) && (max_npc_spell_power(ch, SPELL_FIREBALL) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un suk-krath divan hekro' %s",
                    Power[max_npc_spell_power(ch, SPELL_FIREBALL)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_FLAMESTRIKE]) && (max_npc_spell_power(ch, SPELL_FLAMESTRIKE) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un suk-krath viqrol hekro' %s",
                    Power[max_npc_spell_power(ch, SPELL_FLAMESTRIKE)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }
    return 0;
}


int
drov_cleric_act_combat(CHAR_DATA * ch, CHAR_DATA * fighting)
{
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];

    if ((!IS_AFFECTED(fighting, CHAR_AFF_BLIND)) && (ch->skills[SPELL_BLIND])
        && (max_npc_spell_power(ch, SPELL_BLIND) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un drov mutur hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_BLIND)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((!affected_by_spell(fighting, SPELL_CURSE)) && (ch->skills[SPELL_BLIND])
        && (max_npc_spell_power(ch, SPELL_CURSE) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un drov psiak chran' %s",
                    Power[max_npc_spell_power(ch, SPELL_CURSE)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((!affected_by_spell(fighting, SPELL_FEAR)) && (ch->skills[SPELL_FEAR])
        && (max_npc_spell_power(ch, SPELL_FEAR) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un drov iluth hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_FEAR)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    return 0;
}

int
nilaz_cleric_act_combat(CHAR_DATA * ch, CHAR_DATA * fighting)
{
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];

    if ((!affected_by_spell(fighting, SPELL_PSI_SUPPRESSION))
        && (ch->skills[SPELL_PSI_SUPPRESSION])
        && (max_npc_spell_power(ch, SPELL_PSI_SUPPRESSION) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un nilaz psiak chran' %s",
                    Power[max_npc_spell_power(ch, SPELL_PSI_SUPPRESSION)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_AURA_DRAIN]) && (max_npc_spell_power(ch, SPELL_AURA_DRAIN) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un nilaz divan hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_AURA_DRAIN)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }
    return 0;
}

int
lightning_cleric_act_combat(CHAR_DATA * ch, CHAR_DATA * fighting)
{
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];

    if ((!affected_by_spell(fighting, SPELL_PARALYZE)) && (ch->skills[SPELL_PARALYZE])
        && (max_npc_spell_power(ch, SPELL_PARALYZE) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un elkros mutur hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_PARALYZE)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((!affected_by_spell(fighting, SPELL_SLOW)) && (ch->skills[SPELL_SLOW])
        && (max_npc_spell_power(ch, SPELL_SLOW) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un elkros mutur chran' %s",
                    Power[max_npc_spell_power(ch, SPELL_SLOW)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_LIGHTNING_BOLT]) && (max_npc_spell_power(ch, SPELL_LIGHTNING_BOLT) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un elkros divan hekro' %s",
                    Power[max_npc_spell_power(ch, SPELL_LIGHTNING_BOLT)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    return 0;
}


int
templar_act_combat(CHAR_DATA * ch, CHAR_DATA * fighting)
{
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];

    if ((GET_HIT(ch) < (GET_MAX_HIT(ch) / 2)) && (ch->skills[SPELL_HEAL])
        && (max_npc_spell_power(ch, SPELL_HEAL) > -1)) {
        sprintf(buf, "'%s un vivadu viqrol wril' me", Power[max_npc_spell_power(ch, SPELL_HEAL)]);
        cmd_cast(ch, buf, 83, 0);
        return 1;
    }

    if (((IS_AFFECTED(fighting, CHAR_AFF_SANCTUARY))
         || (affected_by_spell(fighting, SPELL_STONE_SKIN))
         || (affected_by_spell(fighting, SPELL_STRENGTH))
         || (affected_by_spell(fighting, SPELL_ARMOR))) && (ch->skills[SPELL_DISPEL_MAGICK])
        && (max_npc_spell_power(ch, SPELL_DISPEL_MAGICK) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "'%s un suk-krath psiak echri' %s",
                    Power[max_npc_spell_power(ch, SPELL_DISPEL_MAGICK)], name);
            cmd_cast(ch, buf, 83, 0);
            return 1;
        }
        return 0;
    }

    if (!IS_AFFECTED(fighting, CHAR_AFF_SILENCE) && (ch->skills[SPELL_SILENCE])) {

        switch (GET_GUILD(fighting)) {
        case GUILD_TEMPLAR:
        case GUILD_LIRATHU_TEMPLAR:
        case GUILD_JIHAE_TEMPLAR:
        case GUILD_WATER_ELEMENTALIST:
        case GUILD_WIND_ELEMENTALIST:
        case GUILD_STONE_ELEMENTALIST:
        case GUILD_FIRE_ELEMENTALIST:
        case GUILD_DEFILER:
            if (max_npc_spell_power(ch, SPELL_SILENCE) > -1) {
                get_arg_char_room_vis(ch, fighting, name);
                if (*name) {
                    sprintf(buf, "cast '%s un vivadu psiak hurn' %s",
                            Power[max_npc_spell_power(ch, SPELL_SILENCE)], name);
                    parse_command(ch, buf);
                    return 1;
                }
                return 0;
            }
            break;
        default:
            break;
        }
        return 0;
    }

    if ((!IS_AFFECTED(fighting, CHAR_AFF_BLIND)) && (ch->skills[SPELL_BLIND])
        && (max_npc_spell_power(ch, SPELL_BLIND) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un drov mutur hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_BLIND)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_BANISHMENT]) && (max_npc_spell_power(ch, SPELL_BANISHMENT) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un whira locror hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_BANISHMENT)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_FIREBALL]) && (max_npc_spell_power(ch, SPELL_FIREBALL) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un suk-krath divan hekro' %s",
                    Power[max_npc_spell_power(ch, SPELL_FIREBALL)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_HEALTH_DRAIN]) && (max_npc_spell_power(ch, SPELL_HEALTH_DRAIN) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un vivadu divan hurn' %s",
                    Power[max_npc_spell_power(ch, SPELL_HEALTH_DRAIN)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }
    return 0;
}

int
defiler_act_combat(CHAR_DATA * ch, CHAR_DATA * fighting)
{
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];


    if ((ch->skills[SPELL_LIGHTNING_BOLT]) && (max_npc_spell_power(ch, SPELL_LIGHTNING_BOLT) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un elkros divan hekro' %s",
                    Power[max_npc_spell_power(ch, SPELL_LIGHTNING_BOLT)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    if ((ch->skills[SPELL_FIREBALL]) && (max_npc_spell_power(ch, SPELL_FIREBALL) > -1)) {
        get_arg_char_room_vis(ch, fighting, name);
        if (*name) {
            sprintf(buf, "cast '%s un suk-krath divan hekro' %s",
                    Power[max_npc_spell_power(ch, SPELL_FIREBALL)], name);
            parse_command(ch, buf);
            return 1;
        }
        return 0;
    }

    return 0;
}

int
non_spell_act_combat(CHAR_DATA * ch, CHAR_DATA * fighting)
{
    char name[MAX_STRING_LENGTH];

    get_arg_char_room_vis(ch, fighting, name);
    if (!*name)
        return 0;

    /* wait on delays like the pcs have to */
    if (ch->specials.act_wait > 0)
       return 0;

    if ((ch->skills[SKILL_DISARM]) && (!(number(0, 4)))) {
        cmd_skill(ch, name, 215, 0);
        return 1;
    } else if ((ch->skills[SKILL_KICK]) && (!(number(0, 3)))) {
        cmd_skill(ch, name, 158, 0);
        return 1;
    } else if ((ch->skills[SKILL_BASH]) && (!(number(0, 3)))) {
        cmd_skill(ch, name, 156, 0);
        return 1;
    }
    return 0;
}

int
will_chase(CHAR_DATA * ch, CHAR_DATA * hates)
{
    if( ch == NULL || hates == NULL )
        return FALSE;

    // sentinels never move
    if (IS_SET(ch->specials.act, CFL_SENTINEL))
        return FALSE;

    // if they aren't aggressive, or don't hate them
    if (!IS_SET(ch->specials.act, CFL_AGGRESSIVE) && !does_hate(ch, hates))
        return FALSE;

    if (CAN_SEE(ch, hates)
        && GET_RACE(ch) != RACE_JAKHAL && GET_RACE(ch) != RACE_GESRA && !ch->specials.fighting
        && !IS_IMMORTAL(hates)
        && GET_POS(ch) == POSITION_STANDING && !is_merchant(ch)
        && !lawful_avoid_combat(ch, hates))
        return TRUE;

    return FALSE;
}

ROOM_DATA *
get_jail(CHAR_DATA * soldier)
{
    ROOM_DATA *jail = NULL;

    if (IS_TRIBE(soldier, TRIBE_ALLANAK_MIL))
        jail = get_room_num(5505);

    if (IS_TRIBE(soldier, 8))
        jail = get_room_num(58728);

    return (jail);
}

  /* Do not pass GO, do not collect $100 */
void
go_to_jail(CHAR_DATA * soldier)
{
    int ex;
    char ch_command[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char real_name_buf[MAX_STRING_LENGTH];
    char real_name_buf2[MAX_STRING_LENGTH];
    ROOM_DATA *jail;
    CHAR_DATA *jailer;
    /* CHAR_DATA *criminal; */

    if (!soldier || !soldier->specials.subduing)
        return;

    jail = get_jail(soldier);

    if (!jail) {
        gamelog("No jail available.  Damn.");
        return;
    }

    if (jail != soldier->in_room) {
        /* Slow up the walking a bit */
        if (!number(0, 3))
            return;

        ex = choose_exit_name_for(soldier->in_room, jail, ch_command, 100, soldier);
        if (ex != -1) {
            parse_command(soldier, ch_command);
            if (!(number(0, 3))) {      /* 25% chance, I think */
                switch (number(0, 2)) {
                case 0:
                    parse_command(soldier, "emote pushes through the crowd.");
                    break;
                case 1:
                    parse_command(soldier, "emote mutters in disgust while passing a beggar.");
                    break;
                case 2:
                    parse_command(soldier,
                                  "say Wish they'd move the jail closer to the criminals.");
                    break;
                }
            }
        }
        return;
    }

    if (IS_TRIBE(soldier, TRIBE_ALLANAK_MIL)) {        /* In jail room */
        for (jailer = soldier->in_room->people; jailer; jailer = jailer->next_in_room) {
            if (npc_index[jailer->nr].vnum == 5826)
                break;
        }
        if (!jailer || (npc_index[jailer->nr].vnum != 5826))
            return;

        if (jailer->specials.subduing)
            return;

        // FIXME:  This has trouble with NPCs for some reason
        sprintf(buf, "release %s %s",
                ((IS_NPC(soldier->specials.subduing)) ?
                 first_name(GET_NAME(soldier->specials.subduing),
                            real_name_buf) : GET_NAME(soldier->specials.subduing)),
                first_name(GET_NAME(jailer), real_name_buf2));
        qgamelogf(QUIET_NPC_AI, "%s", buf);
        parse_command(soldier, buf);
    }
    if (IS_TRIBE(soldier, 8)) {
        for (jailer = soldier->in_room->people; jailer; jailer = jailer->next_in_room) {
            if (npc_index[jailer->nr].vnum == 59015)
                break;
        }
        if (!jailer || (npc_index[jailer->nr].vnum != 59015))
            return;

        if (jailer->specials.subduing)
            return;
        sprintf(buf, "release %s %s",
                ((IS_NPC(soldier->specials.subduing)) ?
                 first_name(GET_NAME(soldier->specials.subduing),
                            real_name_buf) : GET_NAME(soldier->specials.subduing)),
                first_name(GET_NAME(jailer), real_name_buf2));
        qgamelogf(QUIET_NPC_AI, "%s", buf);
        parse_command(soldier, buf);
    }

}

int
npc_act_soldier(CHAR_DATA * soldier)
{
    int ct, ex;
    char buf[MAX_STRING_LENGTH];
    //  char buf2[256];
    CHAR_DATA *criminal;
    CHAR_DATA *subduer;
    int debug = FALSE;

    if (debug)
        shhlog("npc_act_soldier: Starting");

    if (GET_POS(soldier) == POSITION_FIGHTING)
        return 0;

    if (soldier->specials.fighting)
        return 0;

    if (debug)
        shhlog("npc_act_soldier: Wasn't fighting");

    if (!(ct = room_in_city(soldier->in_room)))
        //  if (!(ct = room_in_city( soldier->in_room ))) 
        return 0;

    if (debug) {
        sprintf(buf, "npc_act_soldier: city=%d, is tribe=%d, city's tribe=%d", ct,
                IS_TRIBE(soldier, city[ct].tribe), city[ct].tribe);
        shhlog(buf);
    }

    if (!IS_TRIBE(soldier, city[ct].tribe))
        return 0;

    if (debug)
        shhlog("npc_act_soldier: Soldier is in own city");

    if (is_charmed(soldier)) {
      return 0;
    }

    if (debug)
        shhlog("npc_act_soldier: Wasn't charmed (called)");

    /* If a soldier is subduing someone who isn't a criminal
     * and they weren't called release them -Morg 9/11/02
     */
    if (soldier->specials.subduing
        && !IS_SET(soldier->specials.subduing->specials.act, city[ct].c_flag)) {
        cmd_release(soldier, NULL, CMD_RELEASE, 0);
        return 1;
    }

/* #define SOLDIER_RETURN_TO_LAST_ROOM */
#ifdef SOLDIER_RETURN_TO_LAST_ROOM
    /* If the soldier is in the jail, but not subduing, move on */
    if (soldier->in_room == get_jail(soldier)
        && !soldier->specials.subduing) {
        // create & call soldier_go_back( soldier ) or something
    }
#endif


    if (debug)
        shhlog("npc_act_soldier: Found criminal");

    if (criminal->in_room != soldier->in_room && ex >= CMD_NORTH && ex <= CMD_DOWN) {

        soldier->specials.speed = SPEED_RUNNING;
        cmd_move(soldier, "", ex, 0);
        soldier->specials.speed = SPEED_WALKING;
        return 1;
    }

    if (debug)
        shhlog("npc_act_soldier: Criminal in same room");

    if (IS_AFFECTED(criminal, CHAR_AFF_SUBDUED)) {
        for (subduer = criminal->in_room->people; subduer; subduer = subduer->next_in_room) {
            if (subduer->specials.subduing == criminal)
                break;
        }

        if (!subduer) {
            shhlog("Error in npc_act_soldier: Char subdued but no subduer.");
            REMOVE_BIT(criminal->specials.affected_by, CHAR_AFF_SUBDUED);
            return FALSE;
        }

        /* Another soldier has the criminal subdued.  Leave 'em alone */
        if (subduer != soldier && IS_IN_SAME_TRIBE(subduer, soldier))
            return FALSE;

        /* Soldier is subduing the criminal.  Take 'em to jail */
        if (subduer == soldier) {
            go_to_jail(soldier);
            return 1;
        }

        /* Need a jail bringing func */
        /* Someone, not a soldier has the crim subdued.  */
        if (number(0, 10)) {
            if (IS_TRIBE(soldier, TRIBE_ALLANAK_MIL))
                cmd_say(soldier, "Release that criminal to me, or suffer the Highlord's Fury!", 0,
                        0);
            if (IS_TRIBE(soldier, 65))  // RSV
                cmd_say(soldier, "Release that criminal to me, or suffer the Sandlord's Fury!", 0,
                        0);
            if (IS_TRIBE(soldier, 66))  // LUIR
                cmd_say(soldier, "Release that criminal to me, or suffer the fury of Kurac!", 0, 0);
            if (IS_TRIBE(soldier, 8))   // LUIR
                cmd_say(soldier, "Release that criminal to me, or suffer the fury of the Sun King!",
                        0, 0);
        } else {
            if (IS_TRIBE(soldier, TRIBE_ALLANAK_MIL)) 
                switch (number(0, 2)) {
                case 0:
                    cmd_shout(soldier, "By Tektolnes!", 0, 0);
                    break;
                case 1:
                    cmd_shout(soldier, "For the Dragon!", 0, 0);
                    break;
                case 2:
                    cmd_shout(soldier, "Death to the law-breakers!", 0, 0);
                    break;
                default:
                    cmd_shout(soldier, "For the glory of Allanak!", 0, 0);
                }

            if (IS_TRIBE(soldier, 66))  // LUIR'S
                cmd_shout(soldier, "Die, enemy of Kurac!", 0, 0);

            hit(soldier, subduer, TYPE_UNDEFINED);
            return 1;
        }
    }
    /* End if criminal is subdued */
    if (debug)
        shhlog("npc_at_soldier: Criminal wasn't subdued");

    /* criminal not subdued */
    get_arg_char_room_vis(soldier, criminal, buf);
    if (!*buf) {
        gamelog("npc_act_soldier: No argument in get_arg_char_room_vis, exiting.");
        return 0;
    }

    if (!lawful_races_in_allanak(criminal)) {
        if (IS_TRIBE(soldier, TRIBE_ALLANAK_MIL))
            switch (number(0, 2)) {
            case 0:
                cmd_shout(soldier, "Suffer the Highlord's Fury!", 0, 0);
                break;
            case 1:
                cmd_shout(soldier, "Militia, over here!", 0, 0);
                break;
            case 2:
                cmd_shout(soldier, "Away from Allanak, scum!", 0, 0);
                break;
            default:
                cmd_shout(soldier, "Suffer the Highlord's Fury!", 0, 0);
                break;
            }
        if (IS_TRIBE(soldier, 65))      // RSV
            cmd_shout(soldier, "Die, fool!", 0, 0);
        hit(soldier, criminal, TYPE_UNDEFINED);
        return 1;
    }

    if (debug)
        shhlog("npc_act_soldier: Going to subdue");

    // Allanak is the only place they should subdue since that
    // is the only place with a jail 
    // -Nesslain 10/14/2000

    if ((ct = room_in_city(soldier->in_room)) == CITY_ALLANAK) {
        sheath_all_weapons(soldier);    /* Tiernan 1/14/03 */
        cmd_skill(soldier, buf, 311, 0);
    }

    if ((ct = room_in_city(soldier->in_room)) == CITY_TULUK) {
        sheath_all_weapons(soldier);    /* Tiernan 1/14/03 */
        cmd_skill(soldier, buf, 311, 0);
    }


    if (IS_AFFECTED(criminal, CHAR_AFF_SUBDUED)) {
#ifdef SOLDIER_RETURN_TO_LAST_ROOM
        /* store the room the soldier subdued them in, so we can return later */
        /* use an edesc or something */
#endif

        if (IS_TRIBE(soldier, TRIBE_ALLANAK_MIL)) {
            switch (number(0, 3)) {
            case 0:
                cmd_shout(soldier, "Yield, criminal, or suffer the consequences!", 0, 0);
                break;
            case 1:
                cmd_shout(soldier, "Submit, or there will be bloodshed!", 0, 0);
                break;
            case 2:
                cmd_shout(soldier, "Militia! A criminal, here!", 0, 0);
                break;
            case 3:
                cmd_shout(soldier, "Surrender, now, or it will mean your death!", 0, 0);
                break;
            default:
                cmd_shout(soldier, "Yield, criminal, or suffer the consequences!", 0, 0);
                break;
            }
        }
        if (IS_TRIBE(soldier, 65))
            cmd_say(soldier, "Die, fool!", 0, 0);
    } else {
        if (number(0, 3)) {
            if (IS_TRIBE(soldier, TRIBE_ALLANAK_MIL))  // Allanak
                switch (number(0, 3)) {
                case 0:
                    cmd_shout(soldier, "Militia! A criminal, here!", 0, 0);
                    break;
                case 1:
                    cmd_shout(soldier, "To the Highlord's Glory!", 0, 0);
                    break;
                case 2:
                    cmd_shout(soldier, "In the name of the Highlord!", 0, 0);
                    break;
                case 3:
                    cmd_shout(soldier, "Templars! A criminal!", 0, 0);
                    break;
                default:
                    cmd_shout(soldier, "Give yourself up criminal, or suffer the Highlord's wrath!",
                              0, 0);
                    break;
                }
            if (IS_TRIBE(soldier, 65))  // RSV
                cmd_say(soldier, "Die, fool!", 0, 0);
            draw_all_weapons(soldier);
            hit(soldier, criminal, TYPE_UNDEFINED);
            return 1;
        }

        draw_all_weapons(soldier);
        hit(soldier, criminal, TYPE_UNDEFINED);
    }
    return 1;
}


int
wezer_count_food_in_room(CHAR_DATA * wezer, int roomNr)
{
    int count = 0;
    ROOM_DATA *choice;
    CHAR_DATA *food;

    if ((choice = get_room_num(roomNr)) == NULL)
        return -1;

    for (food = choice->people; food; food = food->next_in_room)
        if (GET_RACE(food) != RACE_WEZER && (knocked_out(food)
                                             || is_paralyzed(food)))
            count++;

    return count;
}

int wezer_chambers[8] = { 71305, 71308, 71312, 71315, 71319, 71322, 71326, 71330 };

bool
is_in_wezer_chamber(CHAR_DATA * wezer)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (wezer_chambers[i] == wezer->in_room->number) {
            return TRUE;
        }
    }

    return FALSE;
}


void
go_to_wezer_feeding_chamber(CHAR_DATA * wezer)
{
    int ex;
    int min_food = 999;
    int i;
    int count;
    int choice = 0;
    char ch_command[MAX_INPUT_LENGTH];
    ROOM_DATA *chamber = NULL;
    CHAR_DATA *food;

    if (!wezer || !wezer->specials.subduing)
        return;

    if (GET_RACE(wezer) != RACE_WEZER)
        return;

    if (npc_index[wezer->nr].vnum == 70304)
        return;

    if (!is_in_wezer_chamber(wezer)) {
        for (i = 0; i < 8; i++) {
            count = wezer_count_food_in_room(wezer, wezer_chambers[i]);
            if (count != -1 && count < min_food) {
                min_food = count;
                choice = i;
            }
        }

        chamber = get_room_num(wezer_chambers[choice]);
    } else {
        chamber = wezer->in_room;
    }

    if (!chamber) {
        gamelog("No chamber available.  Damn.");
        return;
    }

    if (chamber != wezer->in_room) {
        /* Slow up the walking a bit */
        if (!number(0, 2))
            return;

        ex = choose_exit_name_for(wezer->in_room, chamber, ch_command, 100, wezer);

        if (ex != -1)
            parse_command(wezer, ch_command);

        return;
    }

    /* In feeding chamber room */
    food = wezer->specials.subduing;
    act("$n drops $N.", FALSE, wezer, NULL, food, TO_ROOM);
    act("$n drops you.", FALSE, wezer, NULL, food, TO_VICT);
    act("You drop $N.", FALSE, wezer, NULL, food, TO_CHAR);
    REMOVE_BIT(food->specials.affected_by, CHAR_AFF_SUBDUED);
    wezer->specials.subduing = NULL;
}

int act_aggressive(CHAR_DATA * ch, CHAR_DATA * tar_ch);

int
npc_act_wezer(CHAR_DATA * wezer)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *food;
    CHAR_DATA *subduer;

    if (GET_POS(wezer) == POSITION_FIGHTING)
        return 0;

    if (GET_RACE(wezer) != RACE_WEZER)
        return 0;
    
    if (is_charmed(wezer)) {
      return 0;
    }

    for (food = wezer->in_room->people; food; food = food->next_in_room) {
        if (!CAN_SEE(wezer, food) || IS_IMMORTAL(food))
            continue;

        if (GET_RACE(food) != RACE_WEZER && (!AWAKE(food) || is_paralyzed(food)))
            break;
        else if (GET_RACE(food) != RACE_WEZER)
            return (act_aggressive(wezer, food));
    }

    if (!food)
        return FALSE;

    if (IS_AFFECTED(food, CHAR_AFF_SUBDUED)) {
        for (subduer = food->in_room->people; subduer; subduer = subduer->next_in_room)
            if (subduer->specials.subduing == food)
                break;

        if (!subduer) {
            shhlog("Error in npc_act_wezer: Char subdued but no subduer.");
            REMOVE_BIT(food->specials.affected_by, CHAR_AFF_SUBDUED);
            return FALSE;
        }
        /* Another wezer has the food subdued.  Leave 'em alone */
        if ((subduer != wezer) && (GET_RACE(subduer) == GET_RACE(wezer)))
            return FALSE;

        /* Wezer is subduing the food.  Take 'em to feeding chamber */
        if (subduer == wezer) {
            go_to_wezer_feeding_chamber(wezer);
            return 1;
        }

        /* Someone, not a wezer has the food subdued.  */
        hit(wezer, subduer, TYPE_UNDEFINED);
        return 1;
    }

    /* End if food is subdued */
    /* already in feeding chamber, leave them alone then */
    if (is_in_wezer_chamber(wezer))
        return FALSE;

    /* food not subdued */
    get_arg_char_room_vis(wezer, food, buf);

    if (!*buf) {
        gamelog("No argument to npc_act_wezer in get_arg_char_room_vis.");
        return 0;
    }

    cmd_skill(wezer, buf, 311, 0);
    return 1;
}

int
npc_act_fear(CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];

    if ((ch->skills[SPELL_BURROW]) && (max_npc_spell_power(ch, SPELL_BURROW) > -1)) {
        sprintf(buf, "'%s un ruk locror grol'", Power[max_npc_spell_power(ch, SPELL_BURROW)]);
        cmd_cast(ch, buf, 83, 0);
        return TRUE;
    }

    if ((!IS_AFFECTED(ch, CHAR_AFF_INVISIBLE)) && (ch->skills[SPELL_INVISIBLE])
        && (max_npc_spell_power(ch, SPELL_INVISIBLE) > -1)) {
        sprintf(buf, "'%s un whira iluth grol' me",
                Power[max_npc_spell_power(ch, SPELL_INVISIBLE)]);
        cmd_cast(ch, buf, 83, 0);
        return TRUE;
    }

    return FALSE;
}

void
npc_act_combat(CHAR_DATA * ch)
{
    int j, acted;
    char buf[256], buf2[256];
    char nothing[MAX_INPUT_LENGTH] = "";
    static int drawloc[3] = { WEAR_ON_BELT_1, WEAR_ON_BELT_2, WEAR_ON_BACK };
    OBJ_DATA *temp_obj;

    /*   char bug[MAX_STRING_LENGTH];
     * CHAR_DATA *bug_ch;              */

    if (!(ch->in_room) || (ch->desc) || (!ch->specials.fighting))
        return;

    /*
     * if ((bug_ch = get_char_room_vis(ch, "Azroen")))
     * {
     * send_to_char("Got here.\n\r", bug_ch);
     * sprintf(bug, "Ch is: %s.\n\r", MSTR(ch, short_descr));
     * send_to_char(bug, bug_ch);
     * }
     */

    /* if frozen, no actions possible
     * this should catch spell_paralyze
     * and peraine poison - Tiernan 12/05/01
     */
    if (IS_SET(ch->specials.act, CFL_FROZEN))
        return;

    if (is_in_command_q(ch))
        return;

    /*
     * if (bug_ch)
     * send_to_char("Not in command_q.\n\r", bug_ch);
     */

    /* Nessalin tinkering 1/99
     * if (GET_RACE(ch) == RACE_GITH)
     * {
     * parse_command(ch, "change language heshrak");
     * parse_command(ch, "shout die, pinky!");
     * };  */

    switch (GET_GUILD(ch)) {
    case GUILD_SHADOW_ELEMENTALIST:
        acted = drov_cleric_act_combat(ch, ch->specials.fighting);
        break;
    case GUILD_VOID_ELEMENTALIST:
        acted = nilaz_cleric_act_combat(ch, ch->specials.fighting);
        break;
    case GUILD_LIGHTNING_ELEMENTALIST:
        acted = lightning_cleric_act_combat(ch, ch->specials.fighting);
        break;
    case GUILD_FIRE_ELEMENTALIST:
        acted = sun_cleric_act_combat(ch, ch->specials.fighting);
        break;
    case GUILD_WATER_ELEMENTALIST:
        acted = water_cleric_act_combat(ch, ch->specials.fighting);
        break;
    case GUILD_WIND_ELEMENTALIST:
        acted = wind_cleric_act_combat(ch, ch->specials.fighting);
        break;
    case GUILD_STONE_ELEMENTALIST:
        acted = stone_cleric_act_combat(ch, ch->specials.fighting);
        break;
    case GUILD_TEMPLAR:
        acted = templar_act_combat(ch, ch->specials.fighting);
        break;
    case GUILD_DEFILER:
        acted = defiler_act_combat(ch, ch->specials.fighting);
        break;
    case GUILD_RANGER:
    case GUILD_WARRIOR:
    case GUILD_BURGLAR:
    case GUILD_PICKPOCKET:
    case GUILD_ASSASSIN:
    case GUILD_MERCHANT:
        acted = non_spell_act_combat(ch, ch->specials.fighting);
        break;
    default:
        acted = non_spell_act_combat(ch, ch->specials.fighting);
        break;
    }

    /*
     * if (bug_ch)
     * send_to_char("After switch guilds.\n\r", bug_ch);
     */

    if (acted)
        return;

    /*
     * if (bug_ch)
     * send_to_char("Not acted.\n\r", bug_ch);
     */

    for (j = 0; j < 3; j++)
        if (ch->equipment[drawloc[j]] && (ch->equipment[drawloc[j]]->obj_flags.type == ITEM_WEAPON)) {
            sprintf(buf, "%s", first_name(OSTR(ch->equipment[drawloc[j]], name), buf2));
            cmd_draw(ch, buf, 0, 0);
            return;
        }

    if (!ch->equipment[ETWO] && (!ch->equipment[ES] || !ch->equipment[EP])) {
        for (temp_obj = ch->carrying; temp_obj; temp_obj = temp_obj->next_content) {
            if ((temp_obj->obj_flags.type == ITEM_WEAPON)
                && (!IS_SET(temp_obj->obj_flags.extra_flags, OFL_ARROW))) {
                sprintf(buf, "%s", first_name(OSTR(temp_obj, name), buf2));
                if (!ch->equipment[EP])
                    cmd_wield(ch, buf, 0, 0);
                else if (!ch->equipment[ES])
                    cmd_grab(ch, buf, 0, 0);
            }
        }
    }

    if (IS_SET(ch->specials.act, CFL_WIMPY)) {
	char edesc[MAX_STRING_LENGTH];
	int wimpy_perc = 20; // Default is 20% of max HP
	if (get_char_extra_desc_value(ch, "[WIMPY_PERCENT]", edesc, sizeof(edesc))) {
		wimpy_perc = atoi(edesc);
		wimpy_perc = MAX(10, MIN(90, wimpy_perc)); // 10 - 90% range
	}
	if ((GET_HIT(ch) < (GET_MAX_HIT(ch) * wimpy_perc / 100)) && (GET_POS(ch) >= POSITION_FIGHTING)) {
		cmd_think(ch, "(deathly afraid) I don't want to die!", 0, 0);
		cmd_flee(ch, nothing, 0, 0);
	}
    }

    /*
     * if (bug_ch)
     * send_to_char("Exiting npc_act_combat.\n\r", bug_ch);
     */
}

int
act_helpful(CHAR_DATA * ch, CHAR_DATA * tar_ch)
{
    int dir;
    char name[256], buf[512];

    char ch_command[MAX_INPUT_LENGTH];

    if (IS_SET(ch->specials.act, CFL_NOHELP))
        return (FALSE);

    if (IS_IN_SAME_TRIBE(ch, tar_ch)
        /* This will make gypsies assist conclave members, and vice versa */
        || (IS_TRIBE(ch, 14) && IS_TRIBE(tar_ch, 24))
        || (IS_TRIBE(ch, 24) && IS_TRIBE(tar_ch, 14))
        /* This will make the kurac militia tribe help normal kurac, but not
         * the other way around */
        || (IS_TRIBE(ch, 66) && IS_TRIBE(tar_ch, 4))) {
        if (tar_ch->specials.fighting) {
            if ((ch->in_room != tar_ch->in_room) && (!ch->specials.fighting)
                && !IS_SET(ch->specials.act, CFL_SENTINEL)) {
                dir = choose_exit_name_for(ch->in_room, tar_ch->in_room, ch_command, 2, ch);
                if (dir != -1) {
                    parse_command(ch, ch_command);
                    return (TRUE);
                } else
                    return (FALSE);
            }
            if (GET_HIT(tar_ch) < (GET_MAX_HIT(tar_ch) / 2)) {  /*  Kel  */
                if (ch->skills[SKILL_RESCUE]) { /*  should work now  */
                    get_arg_char_room_vis(ch, tar_ch, name);
                    cmd_skill(ch, name, 157, 0);
                    return (TRUE);
                }
                if (ch->skills[SPELL_HEAL] && (max_npc_spell_power(ch, SPELL_HEAL) > -1)) {
                    get_arg_char_room_vis(ch, tar_ch, name);
                    sprintf(buf, "'%s un vivadu viqrol wril' %s",
                            Power[max_npc_spell_power(ch, SPELL_HEAL)], name);
                    cmd_cast(ch, buf, 83, 0);
                    return (TRUE);
                }
            }
            if (!ch->specials.fighting) {
                get_arg_char_room_vis(ch, tar_ch, name);
                cmd_assist(ch, name, 0, 0);
                return (TRUE);
            }
        }
    }
    return FALSE;
}

int
act_aggressive(CHAR_DATA * ch, CHAR_DATA * tar_ch)
{
    int j, dir, check, wpos = -1, ex = 0;
    char name[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];
    char logbuf[MAX_INPUT_LENGTH];
    OBJ_DATA *wielded;
    OBJ_DATA *missile;
    OBJ_DATA *temp_obj, *temp_obj2;
    char ch_command[MAX_INPUT_LENGTH];
    char exit_name[MAX_INPUT_LENGTH];
    char real_name_buf[MAX_STRING_LENGTH];
    static int drawloc[3] = { WEAR_ON_BELT_1, WEAR_ON_BELT_2, WEAR_ON_BACK };

    if (IS_CALMED(ch) || ch->specials.fighting)
        return (TRUE);

    if ((GET_RACE(ch) == RACE_ELEMENTAL)
        && (GET_GUILD(ch) == GET_GUILD(tar_ch))
        && !does_hate(ch, tar_ch))
        return (FALSE);

    if (IS_TRIBE(ch, TRIBE_ALLANAK_MIL)
        && IS_SET(tar_ch->specials.act, CFL_CRIM_ALLANAK)
        && IS_AFFECTED(tar_ch, CHAR_AFF_SUBDUED))
        return (FALSE);

    /* prevent soldiers from autoattacking when under templar order -Morg */
    if (IS_TRIBE(ch, TRIBE_ALLANAK_MIL)
        && IS_SET(tar_ch->specials.act, CFL_CRIM_ALLANAK)
        && IS_AFFECTED(ch, CHAR_AFF_CHARM))
        return (FALSE);

    if (IS_TRIBE(ch, TRIBE_CAI_SHYZN) && IS_TRIBE(tar_ch, 24999)
        && !does_hate(ch, tar_ch))
        return (FALSE);

    if (IS_TRIBE(ch, TRIBE_ALLANAKI_LIBERATION_ALLIANCE)
        && IS_TRIBE(tar_ch, TRIBE_ALLANAKI_LIBERATION_ALLIANCE)
        && !IS_TRIBE(tar_ch, TRIBE_ALLANAK_MIL) && !does_hate(ch, tar_ch))
        return (FALSE);

    /* Temporary patch to unite the gith tribes for an Allanaki plotline.
     * Remove when the plotline has concluded. - Tiernan 9/23/2007
     */
    if ((GET_RACE(ch) == RACE_GITH) && (GET_RACE(tar_ch) == RACE_GITH))
        return (FALSE);

    /* Don't attack if you like the person */
    if (does_like(ch, tar_ch))
        return (FALSE);

    if (is_merchant(tar_ch)) 
        return (FALSE);

    if (is_target(ch, tar_ch)) {
        if ((ch->in_room != tar_ch->in_room) && !IS_SET(ch->specials.act, CFL_SENTINEL)) {
            dir = choose_exit_name_for(ch->in_room, tar_ch->in_room, ch_command, 2, ch);
            if (dir >= 0) {
                /* BEGIN SHOOTER */

                if ((missile = ch->equipment[EP]) != NULL) {
                    if (IS_SET(missile->obj_flags.extra_flags, OFL_MISSILE_L)) {

                        ex = choose_exit_name_for(ch->in_room,  /*start room  */
                                                  tar_ch->in_room,      /* target room */
                                                  exit_name,    /*dir command */
                                                  5,    /* max dist   */
                                                  ch);  /* walker     */

                        /* If the exit is valid */
                        if (ex != -1) {
                            sprintf(buf, "throw %s %s %s", first_name(OSTR(missile, name), buf2),
                                    first_name(GET_NAME(tar_ch), real_name_buf), exit_name);
                            parse_command(ch, buf);
#define DEBUG_SHOOTER TRUE
#ifdef DEBUG_SHOOTER
                            sprintf(logbuf, "SHOOTER NPC CODE:1:%s", buf);
                            shhlog(logbuf);
#endif

                            return (TRUE);
                        }
                    }
                }
                if ((missile = ch->equipment[ES]) != NULL)
                    if (missile->obj_flags.type == ITEM_BOW)
                        if (weather_info.wind_velocity < 50) {
                            /* Look in inventory for arrow */
                            if (!(missile = ch->equipment[EP]))
                                for (temp_obj = ch->carrying; temp_obj;
                                     temp_obj = temp_obj->next_content)
                                    if (IS_SET(temp_obj->obj_flags.extra_flags, OFL_ARROW)) {
                                        sprintf(buf, "ep %s",
                                                first_name(OSTR(temp_obj, name), buf2));
                                        parse_command(ch, buf);
#ifdef DEBUG_SHOOTER
                                        sprintf(logbuf, "SHOOTER NPC CODE:2:%s", buf);
                                        shhlog(logbuf);
#endif

                                    }
                            /* Look in containers in inventory for arrow */
                            if (!(missile = ch->equipment[EP]))
                                for (temp_obj = ch->carrying; temp_obj;
                                     temp_obj = temp_obj->next_content)
                                    if (temp_obj->obj_flags.type == ITEM_CONTAINER)
                                        for (temp_obj2 = temp_obj->contains; temp_obj2;
                                             temp_obj2 = temp_obj2->next_content)
                                            if (IS_SET(temp_obj2->obj_flags.extra_flags, OFL_ARROW)) {
                                                sprintf(buf, "get %s %s",
                                                        first_name(OSTR(temp_obj2, name), buf2),
                                                        first_name(OSTR(temp_obj, name), buf2));
                                                parse_command(ch, buf);

#ifdef DEBUG_SHOOTER
                                                sprintf(logbuf, "SHOOTER NPC CODE:3:%s", buf);
                                                shhlog(logbuf);
#endif
                                                sprintf(buf, "ep %s",
                                                        first_name(OSTR(temp_obj2, name), buf2));
                                                parse_command(ch, buf);

#ifdef DEBUG_SHOOTER
                                                sprintf(logbuf, "SHOOTER NPC CODE:4:%s", buf);
                                                shhlog(logbuf);
#endif



                                                break;
                                            }

                            if ((missile = ch->equipment[EP]) != NULL) {
                                if (IS_SET(missile->obj_flags.extra_flags, OFL_ARROW)) {
                                    ex = choose_exit_name_for(ch->in_room, tar_ch->in_room, exit_name, 3,       /* max dist   */
                                                              ch);
                                    /* If the exit is valid */
                                    if (ex != -1) {
                                        sprintf(buf, "shoot %s %s",
                                                first_name(GET_NAME(tar_ch), real_name_buf),
                                                exit_name);
                                        parse_command(ch, buf);
#ifdef DEBUG_SHOOTER
                                        sprintf(logbuf, "SHOOTER NPC CODE:5:%s", buf);
                                        shhlog(logbuf);
#endif

                                        return (TRUE);
                                    }
                                }
                            } else {
                                /* They're holding a bow, but can't find any arrows */
                                sprintf(buf, "remove %s",
                                        first_name(OSTR(ch->equipment[ES], name), buf2));
                                parse_command(ch, buf);
#ifdef DEBUG_SHOOTER
                                sprintf(logbuf, "SHOOTER NPC CODE:6:%s", buf);
                                shhlog(logbuf);
#endif

                                for (j = 0; j < 3; j++)
                                    if (ch->equipment[drawloc[j]]
                                        && (ch->equipment[drawloc[j]]->obj_flags.type ==
                                            ITEM_WEAPON)) {
                                        sprintf(buf, "%s",
                                                first_name(OSTR(ch->equipment[drawloc[j]], name),
                                                           buf2));
                                        cmd_draw(ch, buf, 0, 0);
#ifdef DEBUG_SHOOTER
                                        sprintf(logbuf, "SHOOTER NPC CODE:7:draw %s", buf);
                                        shhlog(logbuf);
#endif

                                    }

                            }
                        }
                /* END SHOOTER */

                if (does_fear(ch, tar_ch))
                    return FALSE;

                /* if they're of the 'lawful' races and not flagged aggressive,
                 * check criminal status before chasing after someone */
                if (lawful_avoid_combat(ch, tar_ch))
                    return FALSE;

                parse_command(ch, ch_command);
#ifdef DEBUG_SHOOTER
                /*
                 * sprintf (logbuf, "SHOOTER NPC CODE:8:%s", ch_command);
                 * shhlog (logbuf); */
#endif

                return (TRUE);
            } else
                return (FALSE);
        }

        check = number(0, 3);

        if (IS_AFFECTED(tar_ch, CHAR_AFF_SNEAK))
            check = number(0, 12);

        if (!check) {
            /* if they're of the 'lawful' races and not flagged aggressive,
             * check criminal status before attacking someone */
            if (lawful_avoid_combat(ch, tar_ch)
                && !IS_SET(ch->specials.act, CFL_AGGRESSIVE))
                return FALSE;

            get_arg_char_room_vis(ch, tar_ch, name);
            if (ch->skills[SKILL_BACKSTAB]) {
                if ((wielded = get_weapon(ch, &wpos))) {
                    if (wielded->obj_flags.value[3] == (TYPE_STAB - 300))
                        cmd_skill(ch, name, CMD_BACKSTAB, 0);
                    else
                        cmd_kill(ch, name, 0, 0);
                }
            } else if (ch->skills[SKILL_SAP]) {
                if ((wielded = get_weapon(ch, &wpos))) {
                    if (wielded->obj_flags.value[3] == (TYPE_BLUDGEON - 300))
                        cmd_skill(ch, name, CMD_SAP, 0);
                    else
                        cmd_kill(ch, name, 0, 0);
                }
            } else
                cmd_kill(ch, name, 0, 0);

            return (TRUE);
        }
    }
    return FALSE;
}

/* This is how kanks poop. If you want to make it so other   *
 * mount races do, you will need to rearrange things and add *
 * a case statement for race, or something funky like that.  *
 *                                   -Sanvean                */

void
drop_dung(CHAR_DATA * ch)
{
/*
    int tmp = 0;
    OBJ_DATA *poopobject;
    struct descriptor_data *d;
*/
    /* Place marker - should be able to set edesc on kank and    *
     * specify poopobject that way. Coming soon to a pooping     *
     * kank near you.                                            *
     * if (find_ex_description("[SPECIAL_POOP]", ch->ex_description, TRUE))
     * blahblahblah code goes here */

    return;
/*
    if (!(d && d->character && d->character->in_room && d->character->in_room->zone))
        return;

    tmp = d->character->in_room->zone;
    switch (tmp) {
    case 14:
    case 20:
    case 24:
    case 28:
    case 39:
    case 45:
    case 55:
    case 56:
    case 58:
    case 59:
        poopobject = read_object(1063, VIRTUAL);
        break;
    default:
        poopobject = read_object(1059, VIRTUAL);
        break;
    }
    parse_command(ch,
                  "emote With a placid expression, me waddles its hind legs apart and deposits excrement onto the ground.");
    obj_to_room(poopobject, ch->in_room);
    return;
*/
}

int
npc_actions(CHAR_DATA * ch) {
    CHAR_DATA *tmp, *next_in_room;
    ROOM_DATA *near_room, *tent_room;
    OBJ_DATA *best_obj = NULL, *obj, *tmp_obj;
    char buf[MAX_STRING_LENGTH], name[MAX_STRING_LENGTH];
    char nothing[MAX_INPUT_LENGTH] = "";
    int dir, max, best_wear_loc = 0;
    bool acted = FALSE;
    int roomCount = 0;
    int startCount = 0;
    int counter = 0;
    CHAR_DATA *startPerson = NULL;

    /* switched people are booted out of the routine */
    if (!(ch->in_room) || (ch->desc) || is_in_command_q(ch))
        return TRUE;

    /* if frozen, no actions possible
     * this should catch spell_paralyze
     * and peraine poison
     */
    if (IS_SET(ch->specials.act, CFL_FROZEN))
        return TRUE;

    /* if you're subdued ... */
    if (IS_AFFECTED(ch, CHAR_AFF_SUBDUED)) {
        /* awake and a 1 in 6 chance ... */
        if (AWAKE(ch) && !number(0, 5))
            /* flee */
            cmd_flee(ch, nothing, 0, 0);
        return TRUE;
    }

    if (find_ex_description("[UNDYING]", ch->ex_description, TRUE))
        if (IS_SET(ch->specials.act, CFL_UNDEAD))
            if (GET_POS(ch) < POSITION_RESTING) {

                // This function is called once per three seconds
                // Going once/50 calls means once/150 seconds or once/2.5 minutes
                if (!number(0, 50))
                    switch (number(1, 6)) {
                    case 1:
                        parse_command(ch, "emote lets out a low moan.");
                        break;
                    case 2:
                        parse_command(ch, "emote @'s eyes flutter open briefly.");
                        break;
                    case 3:
                        parse_command(ch,
                                      "emote makes a coughing noise as green fluid drips from ^me mouth.");
                        break;
                    case 4:
                        parse_command(ch, "emote @'s limbs begin twitching.");
                        break;
                    case 5:
                        parse_command(ch, "emote makes a feeble attempt to crawl away.");
                        break;
                    case 6:    /* chance of healing faster than normal */
                        parse_command(ch, "pemote eyes snap open!");
                        parse_command(ch, "emote slowly gets up and stands to ^me feet.");
                        set_hit(ch, 1);
                        update_pos(ch);
                    default:
                        break;
                    }
            }

    /* If sleeping, don't emote -Gleden */
    if (GET_POS(ch) < POSITION_RESTING)
        return TRUE;

    /* if you're subduing someone with fire armor ... */
    if (ch->specials.subduing && affected_by_spell(ch->specials.subduing, SPELL_FIRE_ARMOR)) {
        /* react */
        parse_command(ch, "emote cries out in pain.");
        parse_command(ch, "release");
        return TRUE;
    }

    if (AWAKE(ch) // awake
     && !number(0, 5) // 1 in 6 chance
     && !has_skill(ch, SKILL_BLIND_FIGHT) // no blind fighting
     && GET_HIT(ch) < GET_MAX_HIT(ch) // wounded
     // and fighting someone you can't see
     && ch->specials.fighting && !CAN_SEE(ch, ch->specials.fighting)){ 
        cmd_flee(ch, nothing, 0, 0);
        return TRUE;
    }


    /* mounts don't do anything */
    if (IS_SET(ch->specials.act, CFL_MOUNT) && !ch->specials.fighting) {
        if (!(number(0, 225))) {
/* messages for erdlus */

            if ((GET_RACE(ch) == RACE_ERDLU) && (GET_MOVE(ch) < 100)) {
                parse_command(ch,
                              "emote wearily twists its head about, scanning the surroundings.");
            } else if (GET_RACE(ch) == RACE_ERDLU) {
                parse_command(ch, "emote twists its head about, scanning the surroundings.");
            }

/* messages for horses */

            if ((GET_RACE(ch) == RACE_HORSE) && (GET_MOVE(ch) < 100)) {
                switch (number(1, 3)) {
                case 1:
                    parse_command(ch, "emote stamps at the ground with a tired hoof.");
                    break;
                case 2:
                    parse_command(ch, "emote tosses its head, whinnying wearily.");
                    break;
                case 3:
                    parse_command(ch, "emote snorts, nosing tiredly at the ground.");
                    break;
                default:
                    break;
                }
            } else if (GET_RACE(ch) == RACE_HORSE) {
                switch (number(1, 3)) {
                case 1:
                    parse_command(ch, "emote stamps at the ground with a hoof.");
                    break;
                case 2:
                    parse_command(ch, "emote tosses its head, whinnying.");
                    break;
                case 3:
                    parse_command(ch, "emote snorts, nosing at the ground.");
                    break;
                default:
                    break;
                }
            }
/* messages for inix */
            if ((GET_RACE(ch) == RACE_INIX) && (GET_MOVE(ch) < 100)) {
                switch (number(1, 4)) {
                case 1:
                    parse_command(ch,
                                  "emote opens its jaws, letting its tongue dart out as it yawns.");
                    break;
                case 2:
                    parse_command(ch,
                                  "emote puffs through its nostrils and bobs its head tiredly.");
                    break;
                case 3:
                    parse_command(ch, "emote wearily stretches its hind legs, arching its back.");
                    break;
                case 4:
                    parse_command(ch, "emote yawns, opening its gaping mouth.");
                    break;
                }
            } else if (GET_RACE(ch) == RACE_INIX) {
                switch (number(1, 3)) {
                case 1:
                    parse_command(ch,
                                  "emote opens its jaws, letting its tongue dart out to lick its nostrils.");
                    break;
                case 2:
                    parse_command(ch, "emote puffs through its nostrils and bobs its head.");
                    break;
                case 3:
                    parse_command(ch, "emote stretches its hind legs, arching its back.");
                    break;
                }
            }

            if (GET_RACE(ch) == RACE_KANK) {
                switch (number(1, 4)) {
                case 1:
                    parse_command(ch,
                                  "emote makes a bleating noise and moves its head from side to side.");
                    break;
                case 2:
                    parse_command(ch, "emote rubs its mandibles together.");
                    break;
                case 3:
                    parse_command(ch, "emote leisurely waves its antennae about.");
                    break;
                case 4:
                    counter = number(1, 100);
                    if (counter == 1) {
                        drop_dung(ch);
                    }
                    break;
                default:
                    break;
                }
            }

/* messages for oxen */
            if ((GET_RACE(ch) == RACE_PLAINSOX) && (GET_MOVE(ch) < 100)) {
                switch (number(1, 3)) {
                case 1:
                    parse_command(ch, "emote chews its cud wearily.");
                    break;
                case 2:
                    parse_command(ch,
                                  "emote stamps a heavy hoof on the ground, pawing at it sleepily.");
                    break;
                case 3:
                    parse_command(ch, "emote lets out a low, weary moo.");
                    break;
                default:
                    break;
                }
            } else if (GET_RACE(ch) == RACE_PLAINSOX) {
                switch (number(1, 3)) {
                case 1:
                    parse_command(ch, "emote chews its cud ruminatively.");
                    break;
                case 2:
                    parse_command(ch, "emote stamps a heavy hoof on the ground.");
                    break;
                case 3:
                    parse_command(ch, "emote lets out a low, wavering moo.");
                    break;
                default:
                    break;
                }
            }
/* messages for ratlon */
            if ((GET_RACE(ch) == RACE_RATLON) && (GET_MOVE(ch) < 100)) {
                switch (number(1, 3)) {
                case 1:
                    parse_command(ch, "emote paws wearily at the ground with a scaley foot.");
                    break;
                case 2:
                    parse_command(ch, "emote tosses its head wearily.");
                    break;
                case 3:
                    parse_command(ch,
                                  "emote lets out a tired huff through its nostrils, looking around.");
                    break;
                default:
                    break;
                }
            }

            else if (GET_RACE(ch) == RACE_RATLON) {
                switch (number(1, 3)) {
                case 1:
                    parse_command(ch, "emote paws at the ground with a scaley foot.");
                    break;
                case 2:
                    parse_command(ch, "emote tosses its head restlessly.");
                    break;
                case 3:
                    parse_command(ch,
                                  "emote lets out a soft huff through its nostrils, looking around.");
                    break;
                default:
                    break;
                }
            }

/* messages for sunback */
            if ((GET_RACE(ch) == RACE_SUNBACK) && (GET_MOVE(ch) < 100)) {
                parse_command(ch, "emote lashes its tail, looking irritable.");
            } else if (GET_RACE(ch) == RACE_SUNBACK) {
                parse_command(ch, "emote moves its feet about, stretching.");
            }
        }
        return TRUE;
    }

    /* in fact, neither do charmed people */
    if (is_charmed(ch)) {
      return TRUE;
    }

    /* if you're supposed to be standing, but aren't ... */
    if (GET_POS(ch) < POSITION_STANDING) {
        char edesc[256];
        // 33% chance, if fighting, to stand up.
        if (ch->specials.fighting && !number(0, 2)) {
            parse_command(ch, "stand");
            return TRUE;
        }

        if ( GET_HIT(ch) == GET_MAX_HIT(ch)
         && GET_MOVE(ch) == GET_MAX_MOVE(ch)
         && GET_STUN(ch) == GET_MAX_STUN(ch)
         && GET_MANA(ch) == GET_MAX_MANA(ch)
         && !get_char_extra_desc_value(ch, "[NO_STAND]", edesc, sizeof(edesc))){
            parse_command(ch, "stand");
            return TRUE;
        }
    }

    /* act out of fear, before aggress, so you don't run back when fleeing */
    for (tmp = ch->in_room->people; tmp; tmp = tmp->next_in_room) {
        if (ch != tmp && (does_fear(ch, tmp)) && (CAN_SEE(ch, tmp))
            && !IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
            sprintf(buf, "%s is here! Flee!\n\r", PERS(ch, tmp));
            send_to_char(buf, ch);
            sprintf(buf, "%s sees you and cringes in terror.\n\r", PERS(tmp, ch));
            send_to_char(buf, tmp);
            act("$n sees $N and cringes in terror.", FALSE, ch, 0, tmp, TO_NOTVICT);
            parse_command(ch, "flee");
            return TRUE;
        }
    }


#define RANDOM_AGGRESS_START
#ifdef RANDOM_AGGRESS_START
    /* Moved these out of the targetting loop as they don't use it */
    if (is_soldier(ch)) {
        acted = npc_act_soldier(ch);
    } else if (GET_RACE(ch) == RACE_WEZER)
        acted = npc_act_wezer(ch);

    if (acted)
        return TRUE;

    startCount = 0;
    roomCount = 0;

    /* Count the number of people in the room */
    for (tmp = ch->in_room->people; tmp; tmp = next_in_room) {
        next_in_room = tmp->next_in_room;
        roomCount++;
    }

    /* Pick a random start point */
    startCount = number(0, roomCount);

    roomCount = 0;
    for (startPerson = ch->in_room->people; startPerson && roomCount < startCount;
         roomCount++, startPerson = next_in_room) {
        next_in_room = startPerson->next_in_room;
    }

    /* search through all possible people and react */
    for (tmp = startPerson; tmp && !acted; tmp = next_in_room) {
        /* stop when we get back to the start person */
        if (tmp->next_in_room == startPerson) {
            next_in_room = NULL;
        }
        /* else if not null, keep looping */
        else if (tmp->next_in_room != NULL) {
            next_in_room = tmp->next_in_room;
        }
        /* at this point, don't restart if we're still at the head of the list
         * and the head of the list is the first person in the room
         */
        else if (tmp == startPerson && startPerson == ch->in_room->people) {
            next_in_room = NULL;
        }
        /* else start at the head of the room list */
        else {
            next_in_room = ch->in_room->people;

            /* if first person in room is start, stop after this one */
            if (next_in_room == startPerson)
                next_in_room = NULL;
        }

        if ((tmp) && (ch)) {
            if ((tmp != ch) && CAN_SEE(ch, tmp)) {
                if (IS_SET(ch->specials.act, CFL_AGGRESSIVE)
                    || does_hate(ch, tmp))
                    acted = act_aggressive(ch, tmp);
                else
                    acted = act_helpful(ch, tmp);
            }
        }
    }
#else
    /* search through all possible people and react */
    for (tmp = ch->in_room->people; tmp && !acted; tmp = next_in_room) {
        next_in_room = tmp->next_in_room;

        if ((tmp) && (ch))
            if ((tmp != ch) && CAN_SEE(ch, tmp)) {

                if (is_soldier(ch)) {
                    acted = npc_act_soldier(ch);
                } else if (GET_RACE(ch) == RACE_WEZER)
                    acted = npc_act_wezer(ch);
                else if (!acted) {
                    if (IS_SET(ch->specials.act, CFL_AGGRESSIVE)
                        || does_hate(ch, tmp))
                        acted = act_aggressive(ch, tmp);
                    else
                        acted = act_helpful(ch, tmp);
                }
            }
    }
#endif

    if (acted)
        return TRUE;

    for (dir = 0; dir < 6; dir++) {
        if (ch && ch->in_room && ch->in_room->direction[dir]) {
            near_room = ch->in_room->direction[dir]->to_room;
#ifdef RANDOM_AGGRESS_START
            startCount = 0;
            roomCount = 0;

            /* Count the number of people in the room */
            for (tmp = near_room->people; tmp; tmp = next_in_room) {
                next_in_room = tmp->next_in_room;
                roomCount++;
            }

            /* Pick a random start point */
            startCount = number(0, roomCount);

            roomCount = 0;
            for (startPerson = near_room->people; startPerson && roomCount < startCount;
                 roomCount++, startPerson = next_in_room) {
                next_in_room = startPerson->next_in_room;
            }

            /* search through all possible people and react */
            for (tmp = startPerson; tmp && !acted; tmp = next_in_room) {
                /* stop when we get back to the start person */
                if (tmp->next_in_room == startPerson)
                    next_in_room = NULL;
                /* else if not null, keep looping */
                else if (tmp->next_in_room != NULL)
                    next_in_room = tmp->next_in_room;
                /* at this point, don't restart if we're still at the head of the list */
                else if (tmp == startPerson && startPerson == ch->in_room->people)
                    next_in_room = NULL;
                /* else start at the head of the room list */
                else {
                    next_in_room = near_room->people;

                    /* if first person in room is start, stop after this one */
                    if (next_in_room == startPerson)
                        next_in_room = NULL;
                }

                if ((tmp != ch) && CAN_SEE(ch, tmp)) {
                    if ((IS_SET(ch->specials.act, CFL_AGGRESSIVE)
                         || does_hate(ch, tmp)) && will_chase(ch, tmp)
                         && !does_fear(ch, tmp))
                        acted = act_aggressive(ch, tmp);
                    else if (!does_fear(ch, tmp))
                        acted = act_helpful(ch, tmp);
                }

            }
#else
            for (tmp = near_room->people; tmp && !acted; tmp = next_in_room) {
                next_in_room = tmp->next_in_room;
                if ((tmp != ch) && CAN_SEE(ch, tmp)) {
                    if ((IS_SET(ch->specials.act, CFL_AGGRESSIVE)
                         || does_hate(ch, tmp)) && will_chase(ch, tmp)
                         && !does_fear(ch, tmp))
                        acted = act_aggressive(ch, tmp);
                    else if (!does_fear(ch, tmp))
                        acted = act_helpful(ch, tmp);
                }
            }
#endif
        }
    }

    if (acted)
        return TRUE;

    /* nobody nearby to abuse or help... */

    /* scavenge the fallen */
    if (IS_SET(ch->specials.act, CFL_SCAVENGER)) {
        if (AWAKE(ch) && !(ch->specials.fighting)) {
            if (ch->skills[SKILL_PEEK] && !number(0, 10)) {
                for (tmp = ch->in_room->people; tmp && !acted; tmp = next_in_room) {
                    next_in_room = tmp->next_in_room;

                    if (!AWAKE(tmp)) {
                        int wear_loc = 0;

                        max = 1;
                        for (wear_loc = 0; wear_loc < MAX_WEAR; wear_loc++) {
                            if ((obj = tmp->equipment[wear_loc]) != NULL
                                && obj->obj_flags.cost > max && CAN_GET_OBJ(ch, obj)) {
                                best_obj = obj;
                                best_wear_loc = wear_loc;
                                max = obj->obj_flags.cost;
                            }
                        }

                        for (obj = tmp->carrying; obj; obj = obj->next_content)
                            if (obj->obj_flags.cost > max && CAN_GET_OBJ(ch, obj)) {
                                best_obj = obj;
                                max = obj->obj_flags.cost;
                            }

                        if (best_obj) {
                            if (best_obj->equipped_by)
                                unequip_char(tmp, best_wear_loc);
                            else
                                obj_from_char(best_obj);
                            obj_to_char(best_obj, ch);
                            act("$n takes $p from $N.", TRUE, ch, best_obj, tmp, TO_ROOM);

                            return TRUE;
                        }
                    }
                }
            }

            if (ch->in_room->contents && !number(0, 10)) {
                for (max = 1, best_obj = 0, obj = ch->in_room->contents; obj;
                     obj = obj->next_content) {
                    if (CAN_GET_OBJ(ch, obj) 
                     && !obj->lifted_by && !obj->occupants) {
                        if (obj->obj_flags.cost > max) {
                            best_obj = obj;
                            max = obj->obj_flags.cost;
                        }
                    }
                }
                if (best_obj) {
                    obj_from_room(best_obj);
                    obj_to_char(best_obj, ch);
                    act("$n picks up $p.", FALSE, ch, best_obj, 0, TO_ROOM);
                    return TRUE;
                }
            }
        }
    }
    /* end of scavenging */

    /* hunt the hated */

    if ((ch->skills[SKILL_HUNT]) && ch->specials.hates) {
        tmp = find_closest_hates(ch, MAX(5, ch->skills[SKILL_HUNT]->learned / 10));
        if (tmp && will_chase(ch, tmp)) {
            get_arg_char_world_vis(ch, tmp, name);
            cmd_skill(ch, name, 220, 0);
            return TRUE;
        }
    }
#define TENTS                   /* -- Uncomment as necessary (Tiernan) */
#ifdef TENTS
    /* tear into any tents pitched here */
    if (IS_SET(ch->specials.act, CFL_AGGRESSIVE)) {
        if (AWAKE(ch) && !(ch->specials.fighting)) {
            if (ch->in_room->contents && !number(0, 10)) {
                for (best_obj = 0, obj = ch->in_room->contents; obj; obj = obj->next_content) {
                    /* Make sure it's a tent by examining the entrance. If
                     * the entrance is to a "reserved room" (73000-73999)
                     * then it _SHOULD_ be a tent.  -- Might want to look 
                     * into a more definitive means of testing for tents. */
                    if (obj->obj_flags.type == ITEM_WAGON
                        && !IS_SET(obj->obj_flags.extra_flags, OFL_MAGIC)
                        && obj->obj_flags.value[0] >= 73000 && obj->obj_flags.value[0] <= 73999)
                        best_obj = obj;
                }
                if (best_obj && !obj_guarded(ch, best_obj, CMD_BREAK)) {
                    act("$n starts destroying $p.", FALSE, ch, best_obj, 0, TO_ROOM);
                    /* If anyone or anything is in it, move them to the
                     * current room, before tearing down the tent */
                    tent_room = get_room_num(best_obj->obj_flags.value[0]);
                    for (tmp = tent_room->people; tmp; tmp = tent_room->people) {
                        char_from_room(tmp);
                        char_to_room(tmp, ch->in_room);
                        send_to_char("Suddenly, the tent collapses around you!\n\r", tmp);
                    }
                    for (tmp_obj = tent_room->contents; tmp_obj; tmp_obj = tent_room->contents) {
                        obj_from_room(tmp_obj);
                        obj_to_room(tmp_obj, ch->in_room);
                    }
                    /* Remove the tent, and put in the remains of a tent obj */
                    /* free_reserved_room( tent_room ); handled by extract_obj */
                    /*  extract_room(tent_room); */
                    extract_obj(best_obj);
                    tmp_obj = read_object(64618, VIRTUAL);
                    obj_to_room(tmp_obj, ch->in_room);
                    return TRUE;
                }
            }
        }
    }
#endif

    /* steal if you can */
    /* Sanvean requested this be changed back to on a flagged basis,
     * not just skill -Morg
     */
#define SCAVENGER_REQUIRED_TO_STEAL
#ifdef SCAVENGER_REQUIRED_TO_STEAL
    /* disabled for now */
    if (IS_SET(ch->specials.act, CFL_SCAVENGER) && FALSE) {
#endif
        if (ch->skills[SKILL_STEAL]) {
            if (ch->skills[SKILL_STEAL]->learned > 50) {
                if (AWAKE(ch) && !number(0, 10)) {
                    for (tmp = ch->in_room->people; tmp; tmp = tmp->next_in_room)
                        if (is_thief_target(tmp, ch))
                            break;

                    if (tmp) {
                        get_arg_char_room_vis(ch, tmp, name);
                        sprintf(buf, "coins %s", name);
                        cmd_skill(ch, buf, 155, 0);
                        return TRUE;
                    }
                }
            }
        }
#ifdef SCAVENGER_REQUIRED_TO_STEAL
    }                           /* end of thievery */
#endif

    return TRUE;
}


void
flag_mul_rage(CHAR_DATA * ch)
{
    struct affected_type af;
    char logbuf[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    if (GET_RACE(ch) != RACE_MUL)
        return;


    if (!ch->specials.fighting || (GET_POS(ch) != POSITION_FIGHTING))
        return;


    if (affected_by_spice(ch, SPICE_THODELIV))
        return;

    if (affected_by_spell(ch, AFF_MUL_RAGE))
        return;

    /* Instead of the flat 1:2000 chance, lower the odds at the same ratio
     * as their hitpoints are to their maximum hitpoints.
     *
     * 100% HP = 1:2000 chance
     * 75% HP  = 1:1500 chance
     * 50% HP  = 1:1000 chance
     * 25% HP  = 1:500 chance
     */
    if (number(0, MAX(0, ((GET_HIT(ch) * 2000) / GET_MAX_HIT(ch)))))
        return;

    /* They're about to rage, are they able to control themselve enough
     * to prevent it? */
    if (skill_success(ch, NULL, ch->skills[SKILL_WILLPOWER]->learned))
        return;
    else
        gain_skill(ch, SKILL_WILLPOWER, 1);

    /* Added gamelog message 6/7/2002. -Sanvean */
    sprintf(logbuf, "Mul rage overtakes %s in room %d.", GET_NAME(ch), ch->in_room->number);
    gamelog(logbuf);

    send_to_char("You feel a deep rage from within boil to the surface, and a "
                 "blood-lust\n\rtakes control.", ch);

    af.type = AFF_MUL_RAGE;
    af.duration = 1;
    af.modifier = CFL_AGGRESSIVE;
    af.location = CHAR_APPLY_CFLAGS;
    af.bitvector = 0;

    affect_join(ch, &af, FALSE, FALSE);
    change_mood(ch, NULL);
}

void
ch_act_agg(CHAR_DATA * ch)
{
    /*
     * char buf[MAX_STRING_LENGTH];
     */
    int found = 0;
    CHAR_DATA *target;
    ROOM_DATA *rm;


    /* Take this out when you want to play with it */

    if (!(rm = ch->in_room)) {
        gamelog("Error in ch_act_agg: ch not in a room.");
        return;
    }

    if (GET_POS(ch) <= POSITION_SLEEPING)
        return;
    if (GET_POS(ch) == POSITION_FIGHTING)
        return;
    if (IS_AFFECTED(ch, CHAR_AFF_SUBDUED))
        return;

    for (target = rm->people; target; target = target->next_in_room) {
/*
   sprintf(buf, "Searching for target.  Found %s.\n\r",
   MSTR(target, short_descr));
   send_to_room(buf, rm);
 */

        if (ch == target)
            continue;
/*
   send_to_room("Ch and target are not the same.\n\r", rm);
 */
        if (!char_can_see_char(ch, target))
            continue;
/*
   send_to_room("Ch can see target.\n\r", rm);
   sprintf(buf, "Targets level is %d.\n\r", GET_LEVEL(target));
   send_to_room(buf, rm);
 */
        if (!IS_NPC(target) && (GET_LEVEL(target) > MORTAL))
            continue;
/*
   send_to_room("Target is mortal.\n\r", rm);
 */

        /* Ok, we have a target.  25% chance of attacking this one */
        if (number(0, 3))
            continue;
/*
   send_to_room("Target found.\n\r", rm);
 */
        /* Ok, we got the lucky winner */
        found = 1;
        break;
    }
    if (!found)
        return;

    /* Ok, we have a target.. lets do some killing! */
    act("$n's eyes turn red, and charges towards $N.", FALSE, ch, 0, target, TO_NOTVICT);
    act("$n's eyes turn red, and charges towards you.", FALSE, ch, 0, target, TO_VICT);
    act("Your battle-rage overcomes you, and you charge at $N", FALSE, ch, 0, target, TO_CHAR);
    hit(ch, target, TYPE_UNDEFINED);
    return;
}

int
npc_intelligence(CHAR_DATA *ch) {
    int in_command_q = is_in_command_q(ch);

    if (!IS_NPC(ch))
        return TRUE;

    if (!ch->desc && ch->specials.act_wait > 0)
        ch->specials.act_wait = MAX(ch->specials.act_wait - WAIT_SEC, 0);

    if (ch->queued_commands && ch->specials.act_wait <= 0 && !in_command_q ) {
        parse_command(ch, pop_queued_command(ch));
        return FALSE;
    }

    if (ch->specials.act_wait == 0 && !in_command_q) {
        return npc_actions(ch);
    }
    return TRUE;
}

void
cmd_talk(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vch, *lis;
    ROOM_DATA *room;
    char buf[MAX_STRING_LENGTH];
    char name[MAX_STRING_LENGTH];
    char how[MAX_STRING_LENGTH];
    char language[MAX_STRING_LENGTH];
    int k;
    OBJ_DATA *table = NULL;
    struct watching_room_list *tmp_wlist;

    if (!ch->on_obj || (ch->on_obj && (!ch->on_obj->table && ch->on_obj->obj_flags.value[2] <= 1))) {
        send_to_char("You're not sitting at a table!\n\r", ch);
        return;
    }

    if (ch->on_obj->table)
        table = ch->on_obj->table;
    else
        table = ch->on_obj;

    for (; *(argument) == ' '; argument++);

    if (argument[0] == '\0') {
        send_to_char("Say what at your table?\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, CHAR_AFF_SILENCE)) {
        act("$n tries to mumble something.", TRUE, ch, 0, 0, TO_ROOM);
        act("You can't seem to say anything!", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (affected_by_spell(ch, PSI_BABBLE)) {
        gprintf(NULL, ch->in_room, "$csc babble$cy incoherently.\n", ch, ch);
        return;
    }

    if (argument[0] == '-' || argument[0] == '[' || argument[0] == '(')
        argument = one_argument_delim(argument, how, MAX_STRING_LENGTH, argument[0]);
    else
        how[0] = '\0';

    sprintf(buf, "talks at their %s", ch->on_obj->table ? "table" : "seat");
    send_comm_to_monitor(ch, how, buf, buf, buf, argument);
    send_comm_to_reciters(ch, argument);
    send_comm_to_clairsentient(ch, ch, how, "says", "exclaims", "asks", argument, TRUE);

    sprintf(buf, "At your %s, you say in %s%s%s:\n\r", ch->on_obj
            && ch->on_obj->table ? "table" : "seat", skill_name[GET_SPOKEN_LANGUAGE(ch)],
            how[0] != '\0' ? ", " : "", how[0] != '\0' ? how : "");

    if (send_to_char_parsed(ch, ch, buf, FALSE)) {
        sprintf(buf, "     \"%s\"\n\r", argument);
        send_to_char(buf, ch);
    } else
        return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room) {
        if (vch != ch && AWAKE(vch)
            && !IS_AFFECTED(vch, CHAR_AFF_DEAFNESS)) {
            bool heard = FALSE;
            bool nearby = FALSE;
            CHAR_DATA *gch = vch->specials.guarding;
            CHAR_DATA *fch = vch->master;

            strcpy(name, VOICE(vch, ch));

            k = apply_accent(language, ch, vch);

            // guarding someone at the table
            if( gch && (gch->on_obj == table 
             || (gch->on_obj 
              && gch->on_obj->table && gch->on_obj->table == table))) {
                nearby = TRUE;
            }

            // followers who aren't on something else
            if( !vch->on_obj && fch && (fch->on_obj == table 
             || (fch->on_obj && fch->on_obj->table
              && fch->on_obj->table == table))) {
                nearby = TRUE;
            }

            if (vch->on_obj == table
                || (vch->on_obj && vch->on_obj->table && vch->on_obj->table == table)) {
                sprintf(buf, "At your %s, %s says in %s%s%s:\n\r", vch->on_obj
                        && vch->on_obj->table ? "table" : "seat", name, language,
                        how[0] != '\0' ? ", " : "", how[0] != '\0' ? how : "");

                if (send_to_char_parsed(ch, vch, buf, FALSE))
                    send_translated(ch, vch, argument);
                send_comm_to_clairsentient(vch, ch, how, "says", "exclaims", "asks", argument,
                                           TRUE);
                /* Give 'em a chance to learn it */
                learn_language(ch, vch, k);
                heard = TRUE;
            }
            /* check for eavesdroppers */
            else if (nearby || IS_AFFECTED(vch, CHAR_AFF_LISTEN)) {
                int percent = 0;
                int learned = has_skill(vch, SKILL_LISTEN)
                 ? vch->skills[SKILL_LISTEN]->learned : 0;

                learned += wis_app[GET_WIS(vch)].percep;
                if( nearby ) learned += 75;

                if (IS_IMMORTAL(vch) || (GET_RACE(vch) == RACE_VAMPIRE))
                    percent = 0;
                else {
                    percent += number(1, 101);
                }

                if (percent <= learned) {
                    sprintf(buf, "At %s, you overhear %s say in %s%s%s:\n\r",
                        ch->on_obj->table ? format_obj_to_char(ch->on_obj->table, vch, 1)
                        : format_obj_to_char(ch->on_obj, vch, 1), name, language,
                        how[0] != '\0' ? ", " : "", how[0] != '\0' ? how : "");

                    if (send_to_char_parsed(ch, vch, buf, FALSE))
                        send_translated(ch, vch, argument);
                    send_comm_to_clairsentient(vch, ch, how, "says", "exclaims", "asks", argument,
                                           TRUE);
                    /* Give 'em a chance to learn it */
                    learn_language(ch, vch, k);
                    heard = TRUE;
                } else if( !nearby && has_skill(vch, SKILL_LISTEN)) {
                    gain_skill(vch, SKILL_LISTEN, 1);
                }
            }
#ifdef TALK_EMOTE_NONLISTENER
            if(!heard) {
                if (how[0] != '\0') {
                    sprintf(buf, "At %s, %s speaks, %s.\n\r",
                            (ch->on_obj->table ? format_obj_to_char(ch->on_obj->table, vch, 1)
                             : format_obj_to_char(ch->on_obj, vch, 1)), name, how);
                    send_to_char_parsed(ch, vch, buf, FALSE);
                }
            }
#endif
        }
    }
        /* start listening rooms  - Tenebrius 2005/03/27  */
        for (tmp_wlist = ch->in_room->watched_by; tmp_wlist; tmp_wlist = tmp_wlist->next_watched_by) {
            room = tmp_wlist->room_watching;
            if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                        sprintf(buf, "%sat %s, %s speaks%s%s.\n\r",
                            tmp_wlist->view_msg_string,
                            (ch->on_obj->table ? format_obj_to_char(ch->on_obj->table, lis, 1)
                             : format_obj_to_char(ch->on_obj, lis, 1)), PERS(lis, ch),
                            how[0] != '\0' ? ", " : "", how[0] != '\0' ? how : "");
			send_to_char_parsed(ch, lis, buf, FALSE);
		    }
	    }
        }
        /* end listening rooms */

    return;
}

