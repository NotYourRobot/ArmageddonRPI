/*
 * File: UTILITY.C
 * Usage: Utility functions.
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

/*
 * Changes:
 *   06/06/2000 - removed assert in dice(), set to use default to 1 if < 1
 *                and gamelog the error -Morg
 *   03/11/2002 - added trim_string() function -Morg
 *   03/12/2002 - aded generate_keywords() and add_keywords()
 */

//#include <execinfo.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>

#include "constants.h"
#include "structs.h"
#include "handler.h"
#include "info.h"
#include "modify.h"
#include "comm.h"
#include "creation.h"
#include "cities.h"
#include "utility.h"
#include "utils.h"
#include "guilds.h"
#include "clan.h"
#include "skills.h"
#include "db.h"
#include "object_list.h"
#include "psionics.h"
#include "limits.h"
#include "watch.h"
#include "pather.h"
#include "wagon_save.h"

extern bool ch_starts_falling_check(CHAR_DATA *ch);

int
get_dir_from_name(char *dir_name) {
  int dir = DIR_UNKNOWN;
  
  if (is_abbrev(dir_name, "north")) {
    dir = DIR_NORTH;
  } else if (is_abbrev(dir_name, "south")) {
    dir = DIR_SOUTH;
  } else if (is_abbrev(dir_name, "east")) {
    dir = DIR_EAST;
  } else if (is_abbrev(dir_name, "west")) {
    dir = DIR_WEST;
  } else if (is_abbrev(dir_name, "up")) {
    dir = DIR_UP;
  } else if (is_abbrev(dir_name, "down")) {
    dir = DIR_DOWN;
  }
  
  return dir;
}

/* to_lower converts a string into all lowercase characters */
void
to_lower(char *arg)
{
    int len;
    int i;

    len = strlen(arg);
    for (i = 0; i < len; i++)
        arg[i] = tolower(arg[i]);

    return;
}


int
intcmp(const void *v1, const void *v2)
{
    return (*(int *) v1 - *(int *) v2);
}

int
revintcmp(const void *v1, const void *v2)
{
    return (*(int *) v2 - *(int *) v1);
}

int
lawful_races_in_allanak(CHAR_DATA * ch)
{
    int ok;

    switch (GET_RACE(ch)) {
    case RACE_HUMAN:
    case RACE_ELF:
    case RACE_DESERT_ELF:
    case RACE_DWARF:
    case RACE_MUL:
    case RACE_HALF_ELF:
    case RACE_HALF_GIANT:
    case RACE_VAMPIRE:
    case RACE_SUB_ELEMENTAL:
        ok = 1;
        break;
    default:
        ok = 0;
        break;
    }

    return (ok);
}

bool
get_char_mood(CHAR_DATA *ch, char *buf, int sizeofbuf ) {
    return get_char_extra_desc_value(ch, "[js_mood]", buf, sizeofbuf);
}

void
change_mood( CHAR_DATA *ch, char *mood ) {
    if( mood == NULL || (mood != NULL && *mood == '\0')) {
       rem_char_extra_desc_value(ch, "[js_mood]");
       return;
    }
    set_char_extra_desc_value(ch, "[js_mood]", mood);
}

void
set_char_position(CHAR_DATA * ch, int pos)
{
    if (!ch)
        return;

    if (ch->on_obj && !IS_SET(ch->on_obj->obj_flags.value[1], FURN_SKIMMER)
     && !IS_SET(ch->on_obj->obj_flags.value[1], FURN_WAGON))
        remove_occupant(ch->on_obj, ch);

    if (is_char_watching_any_dir(ch))
        stop_watching(ch, 0);

    // sleeping stops observation skills
    if (pos < POSITION_RESTING) {
       if (IS_AFFECTED(ch, CHAR_AFF_LISTEN)) 
           affect_from_char(ch, SKILL_LISTEN);

       if (IS_AFFECTED(ch, CHAR_AFF_SCAN))
           affect_from_char(ch, SKILL_SCAN);

       if (is_char_watching(ch))
           stop_watching(ch, 0);

       if (ch->specials.guarding)
           stop_guarding(ch);
    }

    ch->specials.position = pos;

    if( pos <= POSITION_STUNNED ) {
       change_mood(ch, NULL);
    }

    // Psionicists can keep their contacts in combat
    if (pos == POSITION_FIGHTING && ch->specials.contact
        && (GET_GUILD(ch) != GUILD_PSIONICIST && GET_GUILD(ch) != GUILD_LIRATHU_TEMPLAR)) {
        send_to_char("You can't maintain your contact...\n\r", ch);
        cease_psionic_skill(ch, PSI_CONTACT, FALSE);
    }

    // Psionicists can keep their barriers in combat
    if (pos == POSITION_FIGHTING && affected_by_spell(ch, PSI_BARRIER)
        && (GET_GUILD(ch) != GUILD_PSIONICIST && GET_GUILD(ch) != GUILD_LIRATHU_TEMPLAR)) {
        send_to_char("You can't maintain your barrier...\n\r", ch);
        cease_psionic_skill(ch, PSI_BARRIER, FALSE);
    }

    if (pos <= POSITION_STANDING) {
      ch_starts_falling_check(ch);
    }
}

void
draw_all_weapons(CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *belt1_obj = NULL;
    OBJ_DATA *belt2_obj = NULL;
    OBJ_DATA *back_obj = NULL;

    /* Both hands are full */
    if ((ch->equipment[ETWO]) || (ch->equipment[EP] && ch->equipment[ES]))
        return;

    if (ch->equipment[WEAR_ON_BACK]) {
        back_obj = ch->equipment[WEAR_ON_BACK];
        one_argument(OSTR(back_obj, name), buf, sizeof(buf));
        if (back_obj->obj_flags.type == ITEM_WEAPON) {
            cmd_draw(ch, buf, CMD_DRAW, 0);
        }
    }
    if (ch->equipment[WEAR_ON_BACK]) {
        back_obj = ch->equipment[WEAR_ON_BACK];
        one_argument(OSTR(back_obj, name), buf, sizeof(buf));
        if (back_obj->obj_flags.type == ITEM_WEAPON) {
            cmd_draw(ch, buf, CMD_DRAW, 0);
        }
    }

    if (ch->equipment[WEAR_ON_BELT_1]) {
        belt1_obj = ch->equipment[WEAR_ON_BELT_1];
        one_argument(OSTR(belt1_obj, name), buf, sizeof(buf));
        if (belt1_obj->obj_flags.type == ITEM_WEAPON) {
            cmd_draw(ch, buf, CMD_DRAW, 0);
        }
    }

    if (ch->equipment[WEAR_ON_BELT_2]) {
        belt2_obj = ch->equipment[WEAR_ON_BELT_2];
        one_argument(OSTR(belt2_obj, name), buf, sizeof(buf));
        if (belt2_obj->obj_flags.type == ITEM_WEAPON) {
            cmd_draw(ch, buf, CMD_DRAW, 0);
        }
    }
}

void
sheath_all_weapons(CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *ep_obj = NULL;
    OBJ_DATA *es_obj = NULL;
    OBJ_DATA *etwo_obj = NULL;

    if (ch->equipment[EP])
        ep_obj = ch->equipment[EP];
    if (ch->equipment[ES])
        es_obj = ch->equipment[ES];
    if (ch->equipment[ETWO])
        etwo_obj = ch->equipment[ETWO];

    /* Everything is sheathed already */
    if (!ep_obj && !es_obj && !etwo_obj)
        return;

    if (ep_obj) {
        one_argument(OSTR(ep_obj, name), buf, sizeof(buf));
        if ((ep_obj->obj_flags.type != ITEM_WEAPON)
            || IS_SET(ep_obj->obj_flags.extra_flags, OFL_NO_SHEATHE)) {
            cmd_remove(ch, buf, CMD_REMOVE, 0);
        } else {
            if (!ch->equipment[WEAR_ON_BACK] && CAN_WEAR(ep_obj, ITEM_WEAR_BACK)) {
                strcat(buf, " back");
            }

            cmd_sheathe(ch, buf, CMD_SHEATHE, 0);
        }
    }

    if (es_obj) {
        one_argument(OSTR(es_obj, name), buf, sizeof(buf));
        if ((es_obj->obj_flags.type != ITEM_WEAPON)
            || IS_SET(es_obj->obj_flags.extra_flags, OFL_NO_SHEATHE)) {
            cmd_remove(ch, buf, CMD_REMOVE, 0);
        } else {
            if (!ch->equipment[WEAR_ON_BACK] && CAN_WEAR(es_obj, ITEM_WEAR_BACK)) {
                strcat(buf, " back");
            }

            cmd_sheathe(ch, buf, CMD_SHEATHE, 0);
        }
    }

    if (etwo_obj) {
        one_argument(OSTR(etwo_obj, name), buf, sizeof(buf));
        if ((etwo_obj->obj_flags.type != ITEM_WEAPON)
            || IS_SET(etwo_obj->obj_flags.extra_flags, OFL_NO_SHEATHE)) {
            cmd_remove(ch, buf, CMD_REMOVE, 0);
        } else {
            if (!ch->equipment[WEAR_ON_BACK] && CAN_WEAR(etwo_obj, ITEM_WEAR_BACK)) {
                strcat(buf, " back");
            }

            cmd_sheathe(ch, buf, CMD_SHEATHE, 0);
        }
    }
}

int
has_sight_spells_resident(CHAR_DATA * ch)
{
    if (affected_by_spell(ch, SPELL_DETECT_POISON))
        return TRUE;
    if (affected_by_spell(ch, SPELL_DETECT_MAGICK))
        return TRUE;
    if (affected_by_spell(ch, SPELL_DETECT_INVISIBLE))
        return TRUE;
    if (affected_by_spell(ch, SPELL_DETECT_ETHEREAL))
        return TRUE;
    if (affected_by_spell(ch, SPELL_INFRAVISION))
        return TRUE;
    if (affected_by_spell(ch, SPELL_IDENTIFY))
        return TRUE;

    return FALSE;
}

int
can_use_psionics(CHAR_DATA * ch)
{
    if (affected_by_spell(ch, SPELL_PSI_SUPPRESSION))
        return 0;
    if (affected_by_spell(ch, SPELL_SHIELD_OF_NILAZ))
        return 0;
    if (affected_by_spell(ch, PSI_BARRIER))
        return 0;
    if (ch->in_room && IS_SET(ch->in_room->room_flags, RFL_NO_PSIONICS))
        return 0;
    return 1;
}

int
can_cast_magick(CHAR_DATA * ch)
{
    int i;

    if (!ch) {
        gamelog("No char sent to can_cast_magick.  Erroring out.");
        return 0;
    }

    for (i = 0; i < MAX_SKILLS; i++) {
        if (!ch->skills[i])
            continue;
        if ((i >= 1) && (i <= 76))
            return 1;
        if ((i >= 83) && (i <= 87))
            return 1;
        if (i == 90)
            return 1;
        if ((i >= 170) && (i <= 182))
            return 1;
        if ((i >= 319) && (i <= 320))
            return 1;
        if (i == 247)
            return 1;
    }
    return 0;
}


int
get_land_distance_helper(ROOM_DATA *room, int depth, void *data)
{

    OBJ_DATA *tmpob = NULL;

    if ((room->sector_type == SECT_AIR)
       || (room->sector_type == SECT_NILAZ_PLANE))
        return FALSE;

    if (room->wagon_exit) {
        tmpob = find_wagon_for_room(room);
        if ((tmpob) && (obj_index[tmpob->nr].vnum == 73001)){
            if (tmpob->in_room)
                return FALSE;
        }
    }

    return TRUE;

}


/* Determines the actual land distance from point A to B */
int
get_land_distance(ROOM_DATA * start_room, ROOM_DATA * end_room)
{

    ROOM_DATA *tmp_room;

    if (start_room == end_room)
        return 0;

    while (start_room->direction[DIR_DOWN] && start_room->direction[DIR_DOWN]->to_room) {
        tmp_room = start_room->direction[DIR_DOWN]->to_room;
        if ((start_room->sector_type != SECT_AIR)
           && (start_room->sector_type != SECT_NILAZ_PLANE))
            break;
        start_room = tmp_room;
    };

    while (end_room->direction[DIR_DOWN] && end_room->direction[DIR_DOWN]->to_room) {
        tmp_room = end_room->direction[DIR_DOWN]->to_room;
        if ((end_room->sector_type != SECT_AIR)
           && (end_room->sector_type != SECT_NILAZ_PLANE))
            break;
        end_room = tmp_room;
    };

    return generic_astar(start_room, end_room, 50, 30.0f, 0, 0, 0, 0, 0, get_land_distance_helper, 0, 0);

}


/* The following function is primarily for Ruk-based spells to determine
   if there is enough sand in the room for the magick.  Function returns
   0 if there is not enough, returns 1 if there is enough because of the
   sector, and returns 2 if there is enough because of sandstorms. */
int
is_enough_sand(ROOM_DATA * rm)
{

    struct weather_node *wn;
    double condition = 0.0;

    if (!rm) {
        gamelogf("null room passed to is_enough_sand");
        return FALSE;
    }

    wn = find_weather_node(rm);
    if (wn)
        condition = wn->condition;

    if ((rm->sector_type == SECT_DESERT)
           || (rm->sector_type == SECT_EARTH_PLANE)
           || (rm->sector_type == SECT_SILT)
           || (rm->sector_type == SECT_SHALLOWS)
           || (rm->sector_type == SECT_SALT_FLATS))
        return 1;

    if (IS_SET(rm->room_flags, RFL_SANDSTORM)
           || condition >= 2.5)
        return 2;

    return 0;

}


int
is_spell_offensive(int i)
{
    return 0;
}

CHAR_DATA *
is_char_criminal_in_city(CHAR_DATA * ch, int ct)
{
    int ex;
    CHAR_DATA *crim;

    for (crim = ch->in_room->people; crim; crim = crim->next_in_room)
        if ((IS_SET(crim->specials.act, city[ct].c_flag)
             || (ct == CITY_ALLANAK && GET_RACE(ch) == RACE_GITH)
             || IS_SET(crim->specials.act, CFL_AGGRESSIVE) || (GET_RACE(crim) == RACE_MANTIS
                                                               && ct == CITY_BW)
             || (GET_RACE(crim) == RACE_ELF && ct == CITY_CAI_SHYZN)) && CAN_SEE(ch, crim)
            && !IS_IN_SAME_TRIBE(crim, ch))
            break;

    if (!crim) {
        for (ex = 0; (ex < 6) && !crim; ex++)
            if (ch->in_room->direction[ex] && ch->in_room->direction[ex]->to_room)
                for (crim = ch->in_room->direction[ex]->to_room->people; crim;
                     crim = crim->next_in_room)
                    if ((IS_SET(crim->specials.act, city[ct].c_flag)
                         || IS_SET(crim->specials.act, CFL_AGGRESSIVE)
                         || (GET_RACE(crim) == RACE_MANTIS && ct == CITY_BW)
                         || (GET_RACE(crim) == RACE_ELF && ct == CITY_CAI_SHYZN))
                        && CAN_SEE(ch, crim) && !IS_IN_SAME_TRIBE(crim, ch))
                        break;

        if (!crim)
            return (CHAR_DATA *) 0;

        ch->specials.speed = SPEED_RUNNING;
        cmd_move(ch, "", ex, 0);
        ch->specials.speed = SPEED_WALKING;
    }

    if (!crim)
        return (CHAR_DATA *) 0;

    return crim;

}

bool
is_magicker(CHAR_DATA * ch)
{
    bool result = FALSE;

    switch (GET_GUILD(ch)) {
    case GUILD_DEFILER:
    case GUILD_TEMPLAR:
    case GUILD_FIRE_ELEMENTALIST:
    case GUILD_WATER_ELEMENTALIST:
    case GUILD_STONE_ELEMENTALIST:
    case GUILD_WIND_ELEMENTALIST:
    case GUILD_SHADOW_ELEMENTALIST:
    case GUILD_LIGHTNING_ELEMENTALIST:
    case GUILD_VOID_ELEMENTALIST:
        return TRUE;
    default:
	result = FALSE;
    }

    switch (GET_SUB_GUILD(ch)) {
    case SUB_GUILD_PATH_OF_ENCHANTMENT:
    case SUB_GUILD_PATH_OF_KNOWLEDGE:
    case SUB_GUILD_PATH_OF_MOVEMENT:
    case SUB_GUILD_PATH_OF_WAR:
    case SUB_GUILD_ENCHANTMENT_MAGICK:
    case SUB_GUILD_COMBAT_MAGICK:
    case SUB_GUILD_MOVEMENT_MAGICK:
    case SUB_GUILD_ENLIGHTENMENT_MAGICK:
        return TRUE;
    default:
	result = FALSE;
    }

    return result;
}


/* Following function will return true if both ch and victim are on
 * the same Plane, false if they are not.  Same plane means it checks
 * their sector for things like earth_plane, lightning_plane. */
bool
is_on_same_plane(ROOM_DATA *src, ROOM_DATA *dst)
{

    if (!src) {
        gamelogf("No source room in is_on_same_plane");
	    return FALSE;
    }

    if (!dst) {
        gamelogf("No destination room is_on_same_plane");
	    return FALSE;
    }

    if (((dst->sector_type == SECT_FIRE_PLANE) && (src->sector_type != SECT_FIRE_PLANE))
        || ((dst->sector_type == SECT_WATER_PLANE) && (src->sector_type != SECT_WATER_PLANE))
        || ((dst->sector_type == SECT_AIR_PLANE) && (src->sector_type != SECT_AIR_PLANE))
        || ((dst->sector_type == SECT_EARTH_PLANE) && (src->sector_type != SECT_EARTH_PLANE))
        || ((dst->sector_type == SECT_SHADOW_PLANE) && (src->sector_type != SECT_SHADOW_PLANE))
        || ((dst->sector_type == SECT_LIGHTNING_PLANE) && (src->sector_type != SECT_LIGHTNING_PLANE))
        || ((dst->sector_type == SECT_NILAZ_PLANE) && (src->sector_type != SECT_NILAZ_PLANE)))
        return FALSE;

    if (((src->sector_type == SECT_FIRE_PLANE) && (dst->sector_type != SECT_FIRE_PLANE))
        || ((src->sector_type == SECT_WATER_PLANE) && (dst->sector_type != SECT_WATER_PLANE))
        || ((src->sector_type == SECT_AIR_PLANE) && (dst->sector_type != SECT_AIR_PLANE))
        || ((src->sector_type == SECT_EARTH_PLANE) && (dst->sector_type != SECT_EARTH_PLANE))
        || ((src->sector_type == SECT_SHADOW_PLANE) && (dst->sector_type != SECT_SHADOW_PLANE))
        || ((src->sector_type == SECT_LIGHTNING_PLANE) && (dst->sector_type != SECT_LIGHTNING_PLANE))
        || ((src->sector_type == SECT_NILAZ_PLANE) && (dst->sector_type != SECT_NILAZ_PLANE)))
        return FALSE;

    return TRUE;
}


int
is_soldier(CHAR_DATA * ch)
{
    int ct;
    char buf[200];
    int debug;

    if (!ch)
        return 0;

    debug = FALSE;

    if (debug) {
        snprintf(buf, sizeof(buf), "checking if %s [%d] is soldier", GET_NAME(ch),
                 npc_index[ch->nr].vnum);
        shhlog(buf);
    }

    if (!IS_NPC(ch))
        return 0;

    //  if ( debug) 
    //    shhlog ("DEBUG: for is_soldier() in special.c: is npc");

    // For now, this is allanak only 
    //  if( !IS_TRIBE( ch, TRIBE_ALLANAK_MIL ) )
    //    return 0;

    //  if ( debug) 
    //   {
    //   shhlog ("DEBUG: for is_soldier() in special.c: is tribe_allanak_mil");
    //   };

    if (!(ct = room_in_city(ch->in_room)))
        return 0;

    if (debug)
        shhlog("is in city");

    if (!IS_TRIBE(ch, city[ct].tribe))
        if ((ct == CITY_TULUK) && (!IS_TRIBE(ch, 12)))
            if ((ct == CITY_LUIR) && (!IS_TRIBE(ch, 66)))
                return 0;

    if (debug) {
        shhlog("is right tribe");
    };


    if (GET_GUILD(ch) == GUILD_TEMPLAR)
        return 0;

    if (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
        return 0;

    if (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)
        return 0;

    if (debug) {
        shhlog("is a templar");
    };

    if (npc_index[ch->nr].vnum == 50055)
        return 0;


    if (debug) {
        shhlog("isnt vnum 50055");
    };

    if (npc_index[ch->nr].vnum == 50607)
        return 0;

    if (debug) {
        shhlog("isnt vnum 50607");
    };

    if (npc_index[ch->nr].vnum == 50028)
        return 0;

    if (debug) {
        shhlog("isnt vnum 50028, good to go");
    };

    return 1;
}

int
find_max_offense_for_char(CHAR_DATA * ch)
{
    int offense;
    int i, count = 0;

    offense = (GET_AGL(ch) * 2);
    offense += (GET_STR(ch));
    offense += (GET_WIS(ch) / 2);

    for (i = 0; i < MAX_SKILLS; i++) {
        if (ch->skills[i] && (skill[i].sk_class == CLASS_COMBAT))
            count += 1;
    }
    if (count > 5)
        offense += (count * 2);

    return MIN(offense, 95);
}

int
is_spiced_char(CHAR_DATA * ch)
{
/*
skills.h:173:#define SPICE_METHELINOC            141
skills.h:174:#define SPICE_MELEM_TUEK            142
skills.h:175:#define SPICE_KRELEZ                143
skills.h:176:#define SPICE_KEMEN                 144
skills.h:177:#define SPICE_AGGROSS               145
skills.h:178:#define SPICE_ZHARAL                146
skills.h:179:#define SPICE_THODELIV              147
skills.h:180:#define SPICE_KRENTAKH              148
skills.h:181:#define SPICE_QEL                   149
skills.h:182:#define SPICE_MOSS                  150
*/
    if (affected_by_spice(ch, SPICE_METHELINOC))
        return SPICE_METHELINOC;
    if (affected_by_spice(ch, SPICE_MELEM_TUEK))
        return SPICE_MELEM_TUEK;
    if (affected_by_spice(ch, SPICE_KRELEZ))
        return SPICE_KRELEZ;
    if (affected_by_spice(ch, SPICE_KEMEN))
        return SPICE_KEMEN;
    if (affected_by_spice(ch, SPICE_AGGROSS))
        return SPICE_AGGROSS;
    if (affected_by_spice(ch, SPICE_ZHARAL))
        return SPICE_ZHARAL;
    if (affected_by_spice(ch, SPICE_THODELIV))
        return SPICE_THODELIV;
    if (affected_by_spice(ch, SPICE_KRENTAKH))
        return SPICE_KRENTAKH;
    if (affected_by_spice(ch, SPICE_QEL))
        return SPICE_QEL;
    if (affected_by_spice(ch, SPICE_MOSS))
        return SPICE_MOSS;

    return 0;
}

/* This code makes it so people 'know' when they are getting drunk.
   The start_level int is used so that people arent spammed with
   messages everytime they drink something alcoholic.               */
void
make_char_drunk(CHAR_DATA * ch, int gain)
{
    int start_level, drunk_level;
    char drunkmsg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    start_level = is_char_drunk(ch);
    gain_condition(ch, DRUNK, gain);
    gain_skill(ch, TOL_ALCOHOL, 1);
    strcpy(drunkmsg, "");

    drunk_level = is_char_drunk(ch);
    switch (drunk_level) {
    case 0:
        break;
    case DRUNK_LIGHT:
        if (start_level < DRUNK_LIGHT)
            send_to_char("You feel a slight buzz from the alcohol.\n\r", ch);
        strcpy(drunkmsg, "slightly ");
        break;
    case DRUNK_MEDIUM:
        if (start_level < DRUNK_MEDIUM)
            send_to_char("Your head is starting to feel very light.\n\r", ch);
        strcpy(drunkmsg, "");
        break;
    case DRUNK_HEAVY:
        if (start_level < DRUNK_HEAVY)
            send_to_char("You are feeling very intoxicated.\n\r", ch);
        strcpy(drunkmsg, "very ");
        break;
    case DRUNK_SMASHED:
        if (start_level < DRUNK_SMASHED)
            send_to_char("Your body begins to tingle in intoxicated" " numbness.\n\r", ch);
        strcpy(drunkmsg, "extremely ");
        break;
    case DRUNK_DEAD:
        if (start_level < DRUNK_DEAD)
            send_to_char("You are completely drunk.\n\r", ch);
        strcpy(drunkmsg, "completely ");
        if ((GET_COND(ch, DRUNK) > 42) && !number(0, 2)) {
            /* Ok, they are so drunk, they are gonna pass out */
            knock_out(ch, "from intoxication", number(1, 2));
            drop_weapons(ch);
            if (!change_ldesc("is lying here, passed out and smelling of" " alcohol.", ch))
                shhlog("ERROR in ldesc changing in intox/drinking.");
        }
        break;
    default:
        errorlog("cmd_drink, drunkeness code.  Defaulted out of main.");
        break;
    }
    if (drunk_level && (start_level != drunk_level) && AWAKE(ch)) {
        snprintf(buf, sizeof(buf), "%s feels %sintoxicated.", MSTR(ch, short_descr), drunkmsg);
        send_to_empaths(ch, buf);
    }
}

int
is_poisoned_char(CHAR_DATA * ch)
{
    if (affected_by_spell(ch, POISON_SKELLEBAIN))
        return 3;
    if (affected_by_spell(ch, POISON_SKELLEBAIN_2))
        return 3;
    if (affected_by_spell(ch, POISON_SKELLEBAIN_3))
        return 3;
    if (affected_by_spell(ch, POISON_GRISHEN))
        return 2;
    if (affected_by_spell(ch, POISON_TERRADIN))
        return 5;
    if (affected_by_spell(ch, POISON_GENERIC))
        return 1;
    if (affected_by_spell(ch, POISON_METHELINOC))
        return 4;
    if (affected_by_spell(ch, POISON_PERAINE))
        return 6;
    if (affected_by_spell(ch, POISON_HERAMIDE))
        return 7;
    if (affected_by_spell(ch, SPELL_POISON))
        return 1;
    if (affected_by_spell(ch, POISON_PLAGUE))
        return 8;
    return FALSE;

}

int
poison_stop_hps(CHAR_DATA * ch)
{
    // affected by generic poison but not the damaging piece of it
    if (affected_by_spell(ch, POISON_GENERIC) 
     && suffering_from_poison(ch, POISON_GENERIC))
        return TRUE;
    if (affected_by_spell(ch, POISON_METHELINOC))
        return TRUE;
    if (affected_by_spell(ch, SPELL_POISON)
     && suffering_from_poison(ch, SPELL_POISON))
        return TRUE;
    if (affected_by_spell(ch, SPELL_IMMOLATE))
        return TRUE;
    return FALSE;
}

    /* This function sets poison affect on a player.  Send who it is, the */
    /* poison type as a value 1-(x many poisons), which is computer by its */
    /* #definition in skills.h - 219.  Then send a duration for how long you */
    /* want the affect for.  If you define a new poison, you can set an */
    /* optional duration, which if set, will over-ride any 0 duration sent */
void
poison_char(CHAR_DATA * ch, int poison, int duration, int silent)
{
    int i, m, l, b, real_poison;
    CHAR_DATA *temp;
    struct affected_type af;

    struct poison_values
    {
        int name;
        int mod;
        int loc;
        int dur;
        int bit;
    };

    struct poison_values pvals[] = {
        {POISON_GENERIC, 0, 0, 0, CHAR_AFF_DAMAGE},
        {POISON_GRISHEN, 0, 0, 0, 0},
        {POISON_SKELLEBAIN, 0, 0, 0, 0},
        {POISON_METHELINOC, 0, 0, 50, 0},
        {POISON_TERRADIN, 0, 0, 0, 0},
        {POISON_PERAINE, CFL_FROZEN, CHAR_APPLY_CFLAGS, 0, 0},
        {POISON_HERAMIDE, 0, 0, 0, 0},
        {POISON_PLAGUE, -4, CHAR_APPLY_AGL, 50, 0},
        {POISON_SKELLEBAIN_2, 0, 0, 0, 0},
        {POISON_SKELLEBAIN_3, 0, 0, 0, 0},
        {0, 0, 0, 0, 0}
    };


    memset(&af, 0, sizeof(af));

    real_poison = poison + 219;
    for (i = 0; pvals[i].name != 0; i++) {
        if (real_poison == pvals[i].name)
            break;
    }

    if (pvals[i].name == 0) {
        errorlogf("Unknown poison val(%d) sent to poison_char in utility.c.", poison);
        gamelog("Defaulting to generic poison.");
        real_poison = POISON_GENERIC;
        i = 0;
    }

    if (real_poison == POISON_SKELLEBAIN) {
        if (GET_RACE(ch) == RACE_HALF_GIANT)
            return;
    } else if (real_poison == POISON_SKELLEBAIN) {
        if (GET_RACE(ch) == RACE_HALF_GIANT)
            return;
    } else if (real_poison == POISON_SKELLEBAIN) {
        if (GET_RACE(ch) == RACE_HALF_GIANT)
            return;
    }

/* Have they built an immunity through tolerance? */
    if (ch->skills[real_poison - 19]) {
        gamelog("They have tolerance to this poison.");
        if (ch->skills[real_poison - 19]->learned > number(0, 101))
            return;
        if (!affected_by_spell(ch, real_poison))
            gain_poison_tolerance(ch, (real_poison - 19), 1);
    }

    /* If they've already got the plague, don't give it to them again */
    if (real_poison == POISON_PLAGUE && affected_by_spell(ch, POISON_PLAGUE))
        return;

    m = pvals[i].mod;
    l = pvals[i].loc;
    b = pvals[i].bit;

    af.type = (real_poison);
    af.duration = ((duration) ? duration : pvals[i].dur);
    // cut peraine duration in half
    if (real_poison == POISON_PERAINE) {
        af.duration /= 2;
    }
    if (real_poison == POISON_PLAGUE)   /* Plague always lasts the default */
        af.duration = pvals[i].dur;
    af.modifier = m;
    af.location = l;
    af.bitvector = b;
    af.power = POWER_PAV;
    affect_join(ch, &af, FALSE, FALSE);

    /* Notify staff if they have skellebain */
    if (real_poison == POISON_SKELLEBAIN) { 
        if (!IS_NPC(ch)) {qgamelogf(QUIET_LOG, "%s (%s) is tripping on skellebain in r%d!",
                     GET_NAME(ch), ch->account,
                     ch->in_room->number);}
    } else if (real_poison == POISON_SKELLEBAIN_2) {
        if (!IS_NPC(ch)) {qgamelogf(QUIET_LOG, "%s (%s) is tripping on skellebain in r%d!",
                     GET_NAME(ch), ch->account,
                     ch->in_room->number);}
    } else if (real_poison == POISON_SKELLEBAIN_3) {
        if (!IS_NPC(ch)) {qgamelogf(QUIET_LOG, "%s (%s) is tripping on skellebain in r%d!",
                     GET_NAME(ch), ch->account,
                     ch->in_room->number);}
    }

    /* Send poisoned message to char and to room */

    /* Extra messages */
    if (real_poison == POISON_SKELLEBAIN) {
        send_to_char("At the corners of your vision, the world around you begins to warp slightly.\n\r", ch);
    } else if (real_poison == POISON_SKELLEBAIN_2) {
        send_to_char("At the corners of your vision, the world around you begins to warp slightly.\n\r", ch);
    } else if (real_poison == POISON_SKELLEBAIN_3) {
        send_to_char("At the corners of your vision, the world around you begins to warp slightly.\n\r", ch);
    }

    if (real_poison == POISON_PERAINE) {
        send_to_char("You feel your your muscles contract, and your body goes" " rigid.\n\r", ch);
        act("$n's muscles contract, and $s body goes rigid.", FALSE, ch, 0, 0, TO_ROOM);
        for (temp = ch->in_room->people; temp; temp = temp->next)
            stop_fighting(temp, ch);

        stop_all_fighting(ch);
    }

    /* Small chance your wisdom goes down from this shit */
    if (real_poison == POISON_SKELLEBAIN) {
        if (!number(0, 999))
            SET_WIS(ch, MAX(1, (GET_WIS(ch) - 1)));
    }

}

/* Returns a poison type, based on race */
int
racial_poison(int race)
{
    switch (race) {
    case RACE_SCORPION:
        return (POISON_SKELLEBAIN - 219);
    case RACE_WEZER:
        return (POISON_PERAINE - 219);
    case RACE_SNAKE1:
        return (POISON_GRISHEN - 219);
    case RACE_FIREANT:
        return (POISON_GRISHEN - 219);
    case RACE_TARANTULA:
        return (POISON_TERRADIN - 219);
    case RACE_RODENT:
        return (POISON_PLAGUE - 219);
    case RACE_WYVERN:
        return (POISON_PERAINE - 219);
    case RACE_GHOUL:
        return (POISON_PLAGUE - 219);
    default:
        return (POISON_GENERIC - 219);
    }
}

int
write_mount_info_to_ticket(CHAR_DATA * mount, OBJ_DATA * ticket)
{
/*  char bug[MAX_STRING_LENGTH]; */
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH] = "";
    int i, sk = 0;

    /* Find mount stats and write them on the ticket as [MOUNT_STATS] */
    snprintf(buf, sizeof(buf),
             "STR %d AGL %d WIS %d END %d OFF %d DEF %d HGT %d WGT %d " "CHP %d CMV %d TIME %d",
             GET_STR(mount), GET_AGL(mount), GET_WIS(mount), GET_END(mount),
             (int) (mount->abilities.off), (int) (mount->abilities.def), GET_HEIGHT(mount),
             GET_WEIGHT(mount), GET_HIT(mount), GET_MOVE(mount), current_hour());

    set_obj_extra_desc_value(ticket, "[MOUNT_STATS]", buf);

    /* Find mount skills and write them on the ticket as [MOUNT_SKILLS] */
    for (i = 0; i <= MAX_SKILLS; i++) {
        sk = 0;
        if (has_skill(mount, i)) {
            sk = get_skill_percentage(mount, i);
            if ((sk > 0) && (sk <= 100)) {
                /*
                 * sprintf(bug, "Skill found: %s with %d percent.", skill_name[i],
                 * sk);
                 * gamelog(bug);
                 */
                snprintf(buf1, sizeof(buf1), "%d %d ", i, sk);
                strcat(buf2, buf1);
            }
        }
    }                           /* End if statement */
    strcat(buf2, "xxx");

    set_obj_extra_desc_value(ticket, "[MOUNT_SKILLS]", buf2);

    /* Find what mount is carrying and write on ticket as [MOUNT_CARRYING] */

    return FALSE;
}


int
read_mount_info_from_ticket(CHAR_DATA * mount, OBJ_DATA * ticket)
{
    char buf[MAX_STRING_LENGTH];
    char msg[MAX_STRING_LENGTH], msg1[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
    const char *bufptr;
    int i;
    int skill = 0, perc = 0;
    int errorcode = 7;

    char *stat_values[] = {
        "STR",
        "AGL",
        "WIS",
        "END",
        "OFF",
        "DEF",
        "HGT",
        "WGT",
        "CHP",
        "CMV",
        "TIME",
        "\n"
    };

    bufptr = buf;
    /* If [MOUNT_STATS] edesc exists, pull off values and assign to mount */
    if (get_obj_extra_desc_value(ticket, "[MOUNT_STATS]", buf, MAX_STRING_LENGTH)) {
        for (i = 0; *stat_values[i] != '\n'; i++) {
            bufptr = two_arguments(bufptr, arg1, sizeof(arg1), arg2, sizeof(arg2));
            if (STRINGS_SAME(arg1, stat_values[i])) {
                switch (i) {
                case 0:        /* strength */
                    if (is_number(arg2))
                        SET_STR(mount, atoi(arg2));
                    snprintf(msg, sizeof(msg), "Str: %d ", atoi(arg2));
                    break;
                case 1:        /* agility */
                    if (is_number(arg2))
                        SET_AGL(mount, atoi(arg2));
                    snprintf(msg1, sizeof(msg1), "Agl: %d ", atoi(arg2));
                    strcat(msg, msg1);
                    break;
                case 2:        /* wisdom */
                    if (is_number(arg2))
                        SET_WIS(mount, atoi(arg2));
                    snprintf(msg1, sizeof(msg1), "Wis: %d ", atoi(arg2));
                    strcat(msg, msg1);
                    break;
                case 3:        /* endurance */
                    if (is_number(arg2))
                        SET_END(mount, atoi(arg2));
                    snprintf(msg1, sizeof(msg1), "End: %d ", atoi(arg2));
                    strcat(msg, msg1);
                    break;
                case 4:        /* offense */
                    if (is_number(arg2))
                        mount->abilities.off = atoi(arg2);
                    snprintf(msg1, sizeof(msg1), "Off: %d ", atoi(arg2));
                    strcat(msg, msg1);
                    break;
                case 5:        /* defense */
                    if (is_number(arg2))
                        mount->abilities.def = atoi(arg2);
                    snprintf(msg1, sizeof(msg1), "Def: %d ", atoi(arg2));
                    strcat(msg, msg1);
                    break;
                case 6:        /* height */
                    if (is_number(arg2))
                        GET_HEIGHT(mount) = atoi(arg2);
                    snprintf(msg1, sizeof(msg1), "Hgt: %d ", atoi(arg2));
                    strcat(msg, msg1);
                    break;
                case 7:        /* weight */
                    if (is_number(arg2))
                        GET_WEIGHT(mount) = atoi(arg2);
                    snprintf(msg1, sizeof(msg1), "Wgt: %d", atoi(arg2));
                    strcat(msg, msg1);
                    break;
                case 8:        /* current hps */
                    snprintf(msg1, sizeof(msg1), "Chp: %d", atoi(arg2));
                    strcat(msg, msg1);
                    break;
                case 9:        /* current moves */
                    snprintf(msg1, sizeof(msg1), "Cmv: %d", atoi(arg2));
                    strcat(msg, msg1);
                    break;
                case 10:       /* time stamp */
                    snprintf(msg1, sizeof(msg1), "Time: %d", atoi(arg2));
                    strcat(msg, msg1);
                    break;

                default:
                    gamelog("Error in read_mount_info_from_ticket, hit default in" " MOUNT_STATS");
                    break;
                }
            }
        }
        /* gamelog message, remove when not needed */
        strcat(msg, "\n\r");
/*
      gamelog(msg); 
*/
        /* remove the first bit */
        errorcode -= 1;
    }


    /* read skills from ticket and write to mount */
    bufptr = buf;
    if (get_obj_extra_desc_value(ticket, "[MOUNT_SKILLS]", buf, MAX_STRING_LENGTH)) {
        bufptr = two_arguments(bufptr, arg1, sizeof(arg1), arg2, sizeof(arg2));
        for (; !STRINGS_SAME(arg1, "xxx");) {
            if (is_number(arg1))
                skill = atoi(arg1);
            if (is_number(arg2))
                perc = atoi(arg2);
            if (skill && perc && (skill <= MAX_SKILLS) && (skill > 0)) {
                skill = atoi(arg1);
                perc = atoi(arg2);
                if (has_skill(mount, skill))
                    mount->skills[skill]->learned = perc;
                else
                    init_skill(mount, skill, perc);
            } else {
                break;
            }

            bufptr = two_arguments(bufptr, arg1, sizeof(arg1), arg2, sizeof(arg2));
        }
        errorcode -= 2;
    }

    return (errorcode);
}

OBJ_DATA *
find_corpse_in_room_from_edesc(ROOM_DATA * rm, char *name, char *ainfo)
{
    int match = FALSE;
    OBJ_DATA *corpse;
/*  char bug[MAX_STRING_LENGTH]; */
    char obj_ename[MAX_STRING_LENGTH];
    char obj_ainfo[MAX_STRING_LENGTH];
    char obj_edesc[MAX_STRING_LENGTH];

    strcpy(obj_ename, "[PC_info]");
    strcpy(obj_ainfo, "[ACCOUNT_info]");

/*
  sprintf(bug, "Name: %s, Ainfo: %s.", name, ainfo);
  gamelog(bug);
*/

    for (corpse = rm->contents; corpse; corpse = corpse->next_content) {
        if (IS_CORPSE(corpse)) {
            if (get_obj_extra_desc_value(corpse, obj_ename, obj_edesc, MAX_STRING_LENGTH)) {
/*
              sprintf(bug, "Obj_edesc: %s", obj_edesc);
              gamelog(bug);
*/
                if (STRINGS_SAME(name, obj_edesc)) {
                    if (!STRINGS_SAME(ainfo, "\n")) {   /* Corpse is from a PC */
                        if (get_obj_extra_desc_value
                            (corpse, obj_ainfo, obj_edesc, MAX_STRING_LENGTH)) {
/*
                          sprintf(bug, "obj_edesc: %s", obj_edesc);
                          gamelog(bug);
*/
                            if (STRINGS_SAME(ainfo, obj_edesc)) {
/*
                              gamelog("Strings same: ainfo, obj_edesc");
*/
                                match = TRUE;
                                break;
                            }   /* End STRINGS_SAME(ainfo, obj_edesc) */
                        }
                        /* End get_obj_extra_desc_value */
                    } /* End if ainfo */
                    else {      /* corpse is from an npc */

                        match = TRUE;
                        break;
                    }
                }               /* End STRINGS_SAME(name, obj_edesc) */
            }                   /* End get_obj_extra_desc_value */
        }                       /* End IS_CORPSE */
    }                           /* End for loop */

    if (!match)
        return (0);

/*
  sprintf(bug, "Corpse found:  %s.", OSTR(corpse, short_descr));
  gamelog(bug);
*/
    return (corpse);
}                               /* End find_corpse_in_room_from_edesc */

OBJ_DATA *
find_head_in_room_from_edesc(ROOM_DATA * rm, char *name, char *ainfo)
{
    int match = FALSE;
    OBJ_DATA *corpse;
/*  char bug[MAX_STRING_LENGTH]; */
    char obj_ename[MAX_STRING_LENGTH];
    char obj_ainfo[MAX_STRING_LENGTH];
    char obj_edesc[MAX_STRING_LENGTH];

    strcpy(obj_ename, "[HEAD_PC_info]");
    strcpy(obj_ainfo, "[HEAD_ACCOUNT_info]");

/*
  sprintf(bug, "Name: %s, Ainfo: %s.", name, ainfo);
  gamelog(bug);
*/

    for (corpse = rm->contents; corpse; corpse = corpse->next_content) {
        if (IS_CORPSE(corpse)) {
            if (get_obj_extra_desc_value(corpse, obj_ename, obj_edesc, MAX_STRING_LENGTH)) {
/*
              sprintf(bug, "Obj_edesc: %s", obj_edesc);
              gamelog(bug);
*/
                if (STRINGS_SAME(name, obj_edesc)) {
                    if (!STRINGS_SAME(ainfo, "\n")) {   /* Corpse is from a PC */
                        if (get_obj_extra_desc_value
                            (corpse, obj_ainfo, obj_edesc, MAX_STRING_LENGTH)) {
/*
                          sprintf(bug, "obj_edesc: %s", obj_edesc);
                          gamelog(bug);
*/
                            if (STRINGS_SAME(ainfo, obj_edesc)) {
/*
                              gamelog("Strings same: ainfo, obj_edesc");
*/
                                match = TRUE;
                                break;
                            }   /* End STRINGS_SAME(ainfo, obj_edesc) */
                        }
                        /* End get_obj_extra_desc_value */
                    } /* End if ainfo */
                    else {      /* corpse is from an npc */

                        match = TRUE;
                        break;
                    }
                }               /* End STRINGS_SAME(name, obj_edesc) */
            }                   /* End get_obj_extra_desc_value */
        }                       /* End IS_CORPSE */
    }                           /* End for loop */

    if (!match)
        return (0);

/*
  sprintf(bug, "Corpse found:  %s.", OSTR(corpse, short_descr));
  gamelog(bug);
*/
    return (corpse);
}                               /* End find_corpse_in_room_from_edesc */


int
launch_obj(OBJ_DATA * obj, int direction, int distance, const char *called_from)
{
    int counter = 0, which;
    char buf[MAX_STRING_LENGTH], calling[MAX_INPUT_LENGTH];
    char room_exit_msg[MAX_STRING_LENGTH];
    char room_enter_msg[MAX_STRING_LENGTH], room_land_msg[MAX_STRING_LENGTH];
    const char * const launch_msg[] = {
        "tail_thwap",
        "\n"
    };

    ROOM_DATA *to_room;

    called_from = one_argument(called_from, calling, sizeof(calling));

    if (!*calling) {
        errorlog("No 'calling' in program: launch_obj in utility.c");
        return TRUE;
    }

    which = search_block(calling, launch_msg, 0);
    switch (which) {
    case 0:                    /* tail_thwap */
        snprintf(room_exit_msg, sizeof(room_exit_msg), "%s tumbles in the air to the %s.",
                 OSTR(obj, short_descr), dirs[direction]);
        snprintf(room_enter_msg, sizeof(room_enter_msg),
                 "%s flies in tumbling through the air from the" " %s.", OSTR(obj, short_descr),
                 rev_dirs[direction]);
        snprintf(room_land_msg, sizeof(room_land_msg), "%s hits the ground hard.",
                 OSTR(obj, short_descr));
        break;
    default:
        snprintf(room_exit_msg, sizeof(room_exit_msg), "%s tumbles in the air to the %s.",
                 OSTR(obj, short_descr), dirs[direction]);
        snprintf(room_enter_msg, sizeof(room_enter_msg),
                 "%s flies in tumbling through the air from the" " %s.", OSTR(obj, short_descr),
                 rev_dirs[direction]);
        snprintf(room_land_msg, sizeof(room_land_msg), "%s hits the ground hard.",
                 OSTR(obj, short_descr));
        break;
    }

    if (!obj) {
        errorlog("No char in program: launch_obj in utility.c");
        return TRUE;
    }
    if (!distance) {
        errorlog("No distance in program: launch_obj in utility.c");
        return TRUE;
    }
    if ((direction < 0) || (direction > 5)) {
        errorlog("Direction out of bounds in program: launch_obj" " in utility.c");
        return TRUE;
    }


    for (counter = 1; counter <= distance; counter++) {
        if (obj->in_room->direction[direction]) {
            if (!(to_room = obj->in_room->direction[direction]->to_room)) {
                errorlog("in launch_obj:  to_room does not exist.");
                return TRUE;
            }
            /* obj leaves room he's in */
            /* First instance, no message given, as program calling this */
            /* function will supply the initial launch string */

            if (counter != 1) {
                send_to_room(room_exit_msg, obj->in_room);
            }

            obj_from_room(obj);

            /* Obj arrives in destination room */
            obj_to_room(obj, to_room);

            send_to_room(room_enter_msg, obj->in_room);
            counter++;
        } else
            break;
    }


    if (counter < distance) {
        snprintf(buf, sizeof(buf), "Obj: %s did not go the distance in program: launch_obj.",
                 OSTR(obj, short_descr));
/*      gamelog(buf);  */
    }


    send_to_room(room_land_msg, obj->in_room);

    return TRUE;
}                               /* End launch_obj */

int
launch_char(CHAR_DATA * ch, int direction, int distance, const char *called_from)
{
    int counter = 0, which;
    char buf[MAX_STRING_LENGTH], calling[MAX_INPUT_LENGTH];
    char char_exit_msg[MAX_STRING_LENGTH], room_exit_msg[MAX_STRING_LENGTH];
    char room_enter_msg[MAX_STRING_LENGTH], room_land_msg[MAX_STRING_LENGTH];
    char char_land_msg[MAX_STRING_LENGTH];
    const char * const launch_msg[] = {
        "tail_thwap",
        "\n"
    };

    ROOM_DATA *to_room;

    called_from = one_argument(called_from, calling, sizeof(calling));

    if (!*calling) {
        errorlog("No 'calling' in program: launch_char in utility.c");
        return TRUE;
    }

    which = search_block(calling, launch_msg, 0);
    switch (which) {
    case 0:                    /* tail_thwap */
        snprintf(room_exit_msg, sizeof(room_exit_msg),
                 "$n's momentum carries $m head-over-heels" " to the %s.", dirs[direction]);
        snprintf(char_exit_msg, sizeof(char_exit_msg), "Your momentum carries you to the %s.\n\r",
                 dirs[direction]);
        snprintf(room_enter_msg, sizeof(room_enter_msg), "$n flies in head over heels from the %s.",
                 rev_dirs[direction]);
        snprintf(room_land_msg, sizeof(room_land_msg), "$n hits the ground hard.");
        snprintf(char_land_msg, sizeof(char_land_msg), "You land hard on the ground in a heap.");
        break;
    default:
        snprintf(room_exit_msg, sizeof(room_exit_msg),
                 "$n's momentum carries $m head-over-heels" " to the %s.", dirs[direction]);
        snprintf(char_exit_msg, sizeof(char_exit_msg), "Your momentum carries you to the %s.\n\r",
                 dirs[direction]);
        snprintf(room_enter_msg, sizeof(room_enter_msg), "$n flies in head over heels from the %s.",
                 rev_dirs[direction]);
        snprintf(room_land_msg, sizeof(room_land_msg), "$n hits the ground hard.");
        snprintf(char_land_msg, sizeof(char_land_msg), "You land hard on the ground in a heap.");
        break;
    }

    if (!ch) {
        errorlog("No char in program: launch_char in utility.c");
        return TRUE;
    }
    if (!distance) {
        errorlog("No distance in program: launch_char in utility.c");
        return TRUE;
    }
    if ((direction < 0) || (direction > 5)) {
        errorlog("Direction out of bounds in program: launch_char" " in utility.c");
        return TRUE;
    }


    for (counter = 1; counter <= distance; counter++) {
        if (ch->in_room->direction[direction]) {
            if (!(to_room = ch->in_room->direction[direction]->to_room)) {
                errorlog("in launch_char:  to_room does not exist.");
                return TRUE;
            }
            /* char leaves room he's in */
            /* First instance, no message given, as program calling this */
            /* function will supply the initial launch string */

            if (counter != 1) {
                act(room_exit_msg, FALSE, ch, 0, 0, TO_ROOM);
            }
            send_to_char(char_exit_msg, ch);
            char_from_room(ch);

            /* Ch arrives in destination room */
            char_to_room(ch, to_room);
            act(room_enter_msg, FALSE, ch, 0, 0, TO_ROOM);
            cmd_look(ch, "", 0, 0);
            counter++;
        } else
            break;
    }


    if (counter < distance) {
        snprintf(buf, sizeof(buf), "Char: %s did not go the distance in program: launch_char.",
                 MSTR(ch, name));
/*      gamelog(buf); */
    }

    act(room_land_msg, FALSE, ch, 0, 0, TO_ROOM);
    send_to_char(char_land_msg, ch);
    set_char_position(ch, POSITION_SITTING);

    return TRUE;
}


int
calc_wagon_max_damage(OBJ_DATA * wagon)
{
    int wt = 0;                 /* weight of the wagon without contents */
    int dam = 0;                /* max damage for the wagon, based on weight */

    extern void add_room_weight_no_people(struct room_data *room, void *arg);

    map_wagon_rooms(wagon, (void *) add_room_weight_no_people, (void *) &wt);

    /* wt now equals the weight of the wagon.. should be between 500-20,000+ */
    /* stones in weight */

    /* 10 will be the minimum for a wagon, while behemoths will have 30+ */
    dam = ((wt / 1000) + 10);

    return (dam);
}

int
is_char_in_arena(CHAR_DATA * ch)
{
    if ((ch->in_room->number >= 50660) && (ch->in_room->number <= 50668))
        return TRUE;

    else
        return FALSE;
}

ROOM_DATA *
find_stands_room_pref_with_people()
{
    char buf[MAX_STRING_LENGTH];
    int i;
    int counter = 0;
    int correct_room;
    CHAR_DATA *people;
    ROOM_DATA *rm;

    for (i = 50673; i <= 50679; i++) {
        rm = get_room_num(i);

        if (rm) {
/*
          for (people = rm->people; people; people = people->next_in_room)
            {
             
            }
*/
            people = rm->people;
            if (people) {
                counter++;
                snprintf(buf, sizeof(buf), "Room: %d has people in it.", i);
/*              gamelog(buf);  */
            }
        }
    }

    if (!counter) {
        rm = get_room_num((50672 + number(1, 7)));
        return (rm);
    }

    correct_room = counter;
    counter = 0;

    for (i = 50673; i <= 50679; i++) {
        rm = get_room_num(i);
        if (rm) {
/*
          for (people = rm->people; people; people = people->next_in_room)
            {
            }
*/
            people = rm->people;
            if (people) {
                counter++;
                if (counter == correct_room)
                    return (rm);
            }
        }
    }

    if (rm)
        return (rm);
    else
        return 0;
}


int
random_room_dir(CHAR_DATA * ch, int updown)
{
    int dir = 0;
    int retdir = 0, counter = 0, max_room = 0;
    char bug[MAX_STRING_LENGTH];
    char bug2[MAX_STRING_LENGTH];

    if (updown)
        max_room = DIR_DOWN;
    else
        max_room = DIR_WEST;
/*
structs.h:730:#define DIR_NORTH 0
structs.h:731:#define DIR_EAST  1
structs.h:732:#define DIR_SOUTH 2
structs.h:733:#define DIR_WEST  3
structs.h:734:#define DIR_UP    4
structs.h:735:#define DIR_DOWN  5
*/

    /* Find how many exits the room has */
    snprintf(bug, sizeof(bug), "Exits available: ");
    for (dir = 0; dir <= max_room; dir++) {
        if (!ch->in_room->direction[dir]) {
            /*         sprintf(bug, "No Exit: %s.\n\r", dirs[dir]);
             * gamelog(bug); */
        } else {
            counter++;
            snprintf(bug2, sizeof(bug2), "%s, ", dirs[dir]);
            strcat(bug, bug2);
        }
    }

/*  gamelog(bug);
 
  sprintf(bug, "There are %d exists available.\n\r", counter);
  gamelog(bug);
*/

    /* No exits */
    if (!counter)
        return (-1);

    retdir = number(1, counter);
    counter = 1;
/*
  sprintf(bug, "Exit chosen: #%d.\n\r", retdir);
  gamelog(bug);
*/

    for (dir = 0; dir <= max_room; dir++) {
        if (!ch->in_room->direction[dir]) {
        } else {
            if (retdir == counter)
                break;
            else
                counter++;
        }
    }
/*
  sprintf(bug, "Exit returned to function: %s.\n\r", dirs[(dir)]);
  gamelog(bug);
*/
    if ((dir >= 0) && (dir <= max_room))
        return (dir);
    else
        return (-1);
}

    /* Adds an edesc to an object to determine if reins are in a */
    /* wagon or land position */
void
add_reins_edesc(OBJ_DATA * wagon, char *edesc)
{
    char ename[MAX_STRING_LENGTH];

    strcpy(ename, "WAGON_REINS_CODE");

    if (STRINGS_SAME(edesc, "wagon") || STRINGS_SAME(edesc, "land"))
        set_obj_extra_desc_value(wagon, ename, edesc);
    else
        errorlog("in add_reins_edesc:  Tried to set invalid edesc");

    return;
}

    /* Find the position of the reins for a wagon object, by finding its */
    /* value[3] bit for wagon reins.  If it does not have one, add one */
bool
get_reins_pos(OBJ_DATA * wagon)
{
    if (IS_SET(wagon->obj_flags.value[3], WAGON_REINS_LAND))
        return (REINS_LAND);
    else
        return (REINS_WAGON);
}

    /* Set the position of the wagon's reins */
void
set_reins_pos(OBJ_DATA * wagon, int pos)
{
    /* if pos == 1, then it is REINS_WAGON */
    /* if pos == 0, then it is REINS_LAND  */

    if (!pos) {
        if (!IS_SET(wagon->obj_flags.value[3], WAGON_REINS_LAND))
            MUD_SET_BIT(wagon->obj_flags.value[3], WAGON_REINS_LAND);

        return;
    } else {
        if (IS_SET(wagon->obj_flags.value[3], WAGON_REINS_LAND))
            REMOVE_BIT(wagon->obj_flags.value[3], WAGON_REINS_LAND);
        return;
    }
}

    /* Find the position of the reins for a wagon object, by finding its */
    /* edesc for wagon reins.  If it does not have one, add one */
#ifdef AZROEN_GET_REINS_CODE
bool
get_reins_pos(OBJ_DATA * wagon)
{
    char ename[MAX_STRING_LENGTH];
    char edesc[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    strcpy(ename, "WAGON_REINS_CODE");

    if (get_obj_extra_desc_value(wagon, ename, edesc, MAX_STRING_LENGTH)) {
        if (STRINGS_SAME(edesc, "wagon"))
            return 1;
        else {
            if (STRINGS_SAME(edesc, "land"))
                return 0;
            else {
                errorlogf("in get_reins_pos: edesc on %s is %s",
                          OSTR(wagon, short_descr), edesc);
                gamelog("Adding edesc in position: land");
                add_reins_edesc(wagon, "land");
                return 0;
            }
        }
    } else {
        errorlogf("in get_reins_pos: %s does not have an edesc" " for %s",
                 OSTR(wagon, short_descr), ename);
        gamelog("Adding edesc in position: land");
        add_reins_edesc(wagon, "land");
        return 0;
    }

    return 0;
}
#endif

    /* This is a characters normal maximum that they can eat or drink */
int
get_max_condition(CHAR_DATA * ch, int cond)
{
    int max = 44;

/* return (44); */

    switch (cond) {
    case DRUNK:
        max = 100;
        break;
    case THIRST:
    case FULL:
    default:
        max = (5 * get_char_size(ch));
        break;
    }

    /* condition is an sbyte, so it has to be in the range of -128 - 127 */
/*
  max = MIN(max, 126);
*/
    /* 10 should be the minimum */
    max = MAX(10, max);

    return (max);
}

    /* This is what is called when a person tries to eat or drink.  It */
    /* is the same as above, except there are modifies which can allow */
    /* the player to eat more than their normal max, or stop them      */
    /* short of their normal maximum.                                  */
int
get_normal_condition(CHAR_DATA * ch, int cond)
{
    int max = 44;
    int mod = 0;

    switch (cond) {
    case DRUNK:
        max = 100;
        break;
    case THIRST:
        max = (5 * get_char_size(ch));
        break;
    case FULL:
        max = (5 * get_char_size(ch));
        break;
    default:
        max = (5 * get_char_size(ch));
        break;
    }

    /* condition is an sbyte, so it has to be in the range of -128 - 127 */
/*
  max = MIN(max, 126);
*/
    /* 10 should be the minimum */
    max = MAX(10, max);
    max += mod;

    /* 0 with modifier */
    max = MAX(0, max);

    return (max);
}

int
shadow_walk_into_dome_room(struct room_data *tar_room)
{
    int dome = FALSE;
    struct char_data *psion;

    for (psion = tar_room->people; psion; psion = psion->next_in_room) {
        if (affected_by_spell(psion, PSI_DOME)) {
            dome = TRUE;
            break;
        }
    }

    if (IS_SET(tar_room->room_flags, RFL_NO_PSIONICS))
        dome = TRUE;

    return (dome);
}

int
is_guild_elementalist(int guild)
{
    int is_elem;

    switch (guild) {
    case GUILD_FIRE_ELEMENTALIST:
        is_elem = 1;
        break;
    case GUILD_WATER_ELEMENTALIST:
        is_elem = 2;
        break;
    case GUILD_WIND_ELEMENTALIST:
        is_elem = 3;
        break;
    case GUILD_STONE_ELEMENTALIST:
        is_elem = 4;
        break;
    case GUILD_SHADOW_ELEMENTALIST:
        is_elem = 5;
        break;
    case GUILD_LIGHTNING_ELEMENTALIST:
        is_elem = 6;
        break;
    case GUILD_VOID_ELEMENTALIST:
        is_elem = 7;
        break;
    default:
        is_elem = FALSE;
        break;
    }
    return is_elem;
}

char *get_guild_element[8] = {
    "none",
    "fire",
    "water",
    "wind",
    "stone",
    "shadow",
    "lightning",
    "void",
};

int
compute_stat_rating(int for_race, int stat, int minmax)
{
    int dice, size, plus, min, max;

    dice = find_att_die_roll(for_race, stat, DICE_NUM);
    size = find_att_die_roll(for_race, stat, DICE_SIZE);
    plus = find_att_die_roll(for_race, stat, DICE_PLUS);

    /* Find min and max rolls */
    min = dice + plus;
    max = ((dice * size) + plus);

    if (minmax)
        return (max);
    else
        return (min);
}

int
find_att_die_roll(int for_race, int att, int type)
{
    int val = 0;

    switch (type) {
    case DICE_NUM:
        switch (att) {
        case ATT_STR:
            val = race[for_race].strdice;
            break;
        case ATT_WIS:
            val = race[for_race].wisdice;
            break;
        case ATT_AGL:
            val = race[for_race].agldice;
            break;
        case ATT_END:
            val = race[for_race].enddice;
            break;
        };
        break;
    case DICE_SIZE:
        switch (att) {
        case ATT_STR:
            val = race[for_race].strsize;
            break;
        case ATT_WIS:
            val = race[for_race].wissize;
            break;
        case ATT_AGL:
            val = race[for_race].aglsize;
            break;
        case ATT_END:
            val = race[for_race].endsize;
            break;
        };
        break;
    case DICE_PLUS:
        switch (att) {
        case ATT_STR:
            val = race[for_race].strplus;
            break;
        case ATT_WIS:
            val = race[for_race].wisplus;
            break;
        case ATT_AGL:
            val = race[for_race].aglplus;
            break;
        case ATT_END:
            val = race[for_race].endplus;
            break;
        };
        break;
    };

    return val;
}

int
is_room_indoors(struct room_data *room)
{
    if (IS_SET(room->room_flags, RFL_INDOORS))
        return TRUE;
    if (room->sector_type == SECT_INSIDE)
        return TRUE;

    return FALSE;
}

int
is_char_ethereal(CHAR_DATA * ch)
{
    if (IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))
        return 1;
    if (affected_by_spell(ch, SPELL_ETHEREAL))
        return 1;
    if (GET_RACE(ch) == RACE_SHADOW)
        return 1;

    return 0;
}

int
is_char_noble(CHAR_DATA * ch, int city)
{
    int is_noble = 0;

    if (!ch || !city)
        return 0;

    switch (city) {
    case CITY_ALLANAK:
        if (IS_TRIBE(ch, TRIBE_HOUSE_TOR)
            ||IS_TRIBE(ch, TRIBE_HOUSE_FALE)
            ||IS_TRIBE(ch, TRIBE_HOUSE_BORSAIL)
            ||IS_TRIBE(ch, TRIBE_HOUSE_OASH))
            is_noble = 1;
        break;
    case CITY_TULUK:
        if (IS_TRIBE(ch, 1)
            || IS_TRIBE(ch, 15)
            || IS_TRIBE(ch, 18)
            || IS_TRIBE(ch, 47))
            is_noble = 1;
        break;
    default:
        break;
    }
    return is_noble;
}

int
char_can_fight(CHAR_DATA * ch)
{
    /* no fighting back if frozen (poison perraine) */
    if (IS_SET(ch->specials.act, CFL_FROZEN))
        return (0);

    /* the only other time char can't fight is when both hands are
     * holding non weapons */

    if (ch->equipment[ETWO] && (ch->equipment[ETWO]->obj_flags.type != ITEM_WEAPON))
        return (0);

    if ((ch->equipment[EP] && (ch->equipment[EP]->obj_flags.type != ITEM_WEAPON))
        && (ch->equipment[ES] && (ch->equipment[ES]->obj_flags.type != ITEM_WEAPON)))
        return (0);

    return (1);

/* old def is here.... 

  (!(ch->equipment[EP] && \
  (ch->equipment[EP]->obj_flags.type != ITEM_WEAPON) && \
  (!ch->equipment[ES] || \
  (ch->equipment[ES]->obj_flags.type != ITEM_WEAPON)) && \
  (!ch->equipment[ETWO] || \
  (ch->equipment[ETWO]->obj_flags.type != ITEM_WEAPON))))

 ... which says the same thing but i didn't want to have to
     mess around with this funny logic.
*/
}


char *real_sdesc_names[] = {
    "someone",
    "a %sshadow",
    "a %sblur",
    "a %sghostly image",
    "a %sred shape",
    "a %sfaint shape",
    "a %sshimmering shape",
};

int
obj_is_tuluki_templar_robe(OBJ_DATA *robe) {
    if( obj_index[robe->nr].vnum == 20001
     || obj_index[robe->nr].vnum == 20351
     || obj_index[robe->nr].vnum == 20002
     || obj_index[robe->nr].vnum == 29513
     || obj_index[robe->nr].vnum == 29514
     || obj_index[robe->nr].vnum == 29577
     || obj_index[robe->nr].vnum == 29578
     || obj_index[robe->nr].vnum == 29593
     || obj_index[robe->nr].vnum == 59902
     || obj_index[robe->nr].vnum == 59900
     || obj_index[robe->nr].vnum == 59908
     || obj_index[robe->nr].vnum == 59909
     || obj_index[robe->nr].vnum == 59910) 
        return TRUE;
    return FALSE;
}

int
obj_is_allanaki_templar_robe(OBJ_DATA *robe) {

    if( obj_index[robe->nr].vnum == 50600 
     || obj_index[robe->nr].vnum == 50601
     || obj_index[robe->nr].vnum == 50604
     || obj_index[robe->nr].vnum == 50610) 
        return TRUE;
    return FALSE;
}

const char *
real_sdesc(CHAR_DATA * ch, CHAR_DATA * looker, int vis)
{
    OBJ_DATA *mask;
    OBJ_DATA *hood;
    OBJ_DATA *robe;
    int pos, templar_item;
    static char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    switch (vis) {
    case 0:                    /* can't see */
        return (real_sdesc_names[0]);
    case 2:                    /* hidden */
    case 3:                    /* invis */
    case 4:                    /* ethereal */
    case 5:                    /* infravision */
    case 6:                    /* bad weather */
    case 7:                    /* detect magick */
        snprintf(buf, sizeof(buf), real_sdesc_names[vis - 1], height_weight_descr(looker, ch));
        return buf;
    case 8:                    /* true sdesc */
        return MSTR(ch, short_descr);
    default:                   /* see normally */
        buf[0] = '\0';

        if (!char_can_see_char(looker, ch)) {
            strcat(buf, "someone");
            return buf;
        } else {
            char title[256];
            char *det_mag_sdesc = NULL;
            char *det_eth_sdesc = NULL;
            char *det_invis_sdesc = NULL;

            if (IS_IMMORTAL(looker) 
             && !IS_IMMORTAL(ch)
             && !IS_NPC(ch)
             && IS_SET(looker->specials.brief, BRIEF_STAFF_ONLY_NAMES) ) {
                sprintf( buf, "%s (%s)", ch->name, ch->account );
                return buf;
            }

            if( GET_SEX(ch) == SEX_FEMALE ) {
                strcpy( title, "female" );
            } else {
                strcpy( title, "male" );
            }

            /* Added hallucination code here - Morg */
            if (looker != ch && affected_by_spell(looker, POISON_SKELLEBAIN_2) 
                && number(0, 8)) {
                    return (hallucinations[number(0, MAX_HAL)]);
            } else if (looker != ch && affected_by_spell(looker, POISON_SKELLEBAIN_3)
                && number(0, 4)) {
                    return (hallucinations[number(0, MAX_HAL)]);
            }

            /* For special cases where the sdesc is altered based on perception
             * we want to figure out what the sdesc should be instead of the
             * default one on the character.
             */

            // Detect magick sdesc
            if (affected_by_spell(looker, SPELL_DETECT_MAGICK)
                    || IS_AFFECTED(looker, CHAR_AFF_DETECT_MAGICK)
                    || (affected_by_spell(looker, PSI_MAGICKSENSE) && can_use_psionics(ch))) {
                det_mag_sdesc = find_ex_description("[DET_MAG_SDESC]", ch->ex_description, TRUE);
                if (det_mag_sdesc)
                    strcpy(buf, det_mag_sdesc);
            }

            // Detect ethereal sdesc
            if (!strlen(buf) && (affected_by_spell(looker, SPELL_DETECT_ETHEREAL)
                    || IS_AFFECTED(looker, CHAR_AFF_DETECT_ETHEREAL))) {
                det_eth_sdesc = find_ex_description("[DET_ETH_SDESC]", ch->ex_description, TRUE);
                if (det_eth_sdesc)
                    strcpy(buf, det_eth_sdesc);
            }

            // Detect invisible sdesc
            if (!strlen(buf) && (affected_by_spell(looker, SPELL_DETECT_INVISIBLE)
                    || IS_AFFECTED(looker, CHAR_AFF_DETECT_INVISIBLE))) {
                det_invis_sdesc = find_ex_description("[DET_INVIS_SDESC]", ch->ex_description, TRUE);
                if (det_invis_sdesc)
                    strcpy(buf, det_invis_sdesc);
            }

            // Empty buffer, use the character's short_descr
            if (!strlen(buf))
                strcpy(buf, MSTR(ch, short_descr));

            templar_item = 0;
            if ((ch->equipment[WEAR_ABOUT]) != NULL) {
               robe = ch->equipment[WEAR_ABOUT];
               if (char_can_see_obj(looker, robe)
                   && (obj_is_tuluki_templar_robe(robe)
                   || obj_is_allanaki_templar_robe(robe)))  {
                 templar_item = 1;
               }
            } else if ((ch->equipment[WEAR_NECK]) != NULL) {
               robe = ch->equipment[WEAR_NECK];
               if (char_can_see_obj(looker, robe)
                   && (obj_is_tuluki_templar_robe(robe)
                   || obj_is_allanaki_templar_robe(robe)))  {
                  templar_item = 1;
               }
            } else if ((ch->equipment[WEAR_HEAD]) != NULL) {
               robe = ch->equipment[WEAR_HEAD];
               if (char_can_see_obj(looker, robe)
                   && (obj_is_tuluki_templar_robe(robe)
                   || obj_is_allanaki_templar_robe(robe)))  {
                  templar_item = 1;
              }
            } 

            // change sdesc to reflect templar
            if ((templar_item)) {            
                // human sized
                if(get_char_size(ch) >= 8 && get_char_size(ch) <= 12) {
                    int len;
                    char *tmpptr;
    
                    strcpy(title, "templar");
    
                    strcpy(buf, MSTR(ch, short_descr));
                    tmpptr = rindex(buf, ' ');
                    if( tmpptr != NULL ) {
                        // move past the space
                        tmpptr++;

                        // get the length of the end
                        len = strlen(tmpptr);
                        pos = strlen(buf) - len;

                        // one last check to make sure it's in a valid place
                        if (pos >= 0) {
                            // remove the last word
                            delete_substr(buf, pos, len);
                            // add the title
                            insert_substr(title, buf, pos, sizeof(buf));
                        } 
                    } else {
                        strcat(buf, " ");
                        strcat(buf, title);
                    }
                }
                else {
                    // Non-humans are easiliy identified by race
                    snprintf(buf, sizeof(buf), "%s wearing %s",
                     MSTR(ch, short_descr), OBJS(robe, looker));
                }
            }

            // HOOD ITEM WORN ABOUT
            if ((hood = ch->equipment[WEAR_ABOUT]) != NULL 
             && char_can_see_obj(looker, hood)
             && (hood->obj_flags.type == ITEM_WORN
              || hood->obj_flags.type == ITEM_CONTAINER)
             && IS_SET(hood->obj_flags.value[1], CONT_HOODED)
             && IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
                if( strstr( title, "male" ) ) strcpy(title, "figure");
                snprintf(buf, sizeof(buf), "the %s%s in %s",
                 height_weight_descr(looker, ch), title, OBJS(hood, looker));
            }
            // HOOD ITEM WORN ON BODY
            else if ((hood = ch->equipment[WEAR_BODY]) != NULL 
             && char_can_see_obj(looker, hood)
             && (hood->obj_flags.type == ITEM_WORN
              || hood->obj_flags.type == ITEM_CONTAINER)
             && IS_SET(hood->obj_flags.value[1], CONT_HOODED)
             && IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
                if( strstr( title, "male" ) ) strcpy(title, "figure");
                snprintf(buf, sizeof(buf), "the %s%s in %s",
                 height_weight_descr(looker, ch), title, OBJS(hood, looker));
            }
            // HOOD ITEM WORN ON HEAD
            else if ((hood = ch->equipment[WEAR_HEAD]) != NULL 
             && char_can_see_obj(looker, hood)
             && (hood->obj_flags.type == ITEM_WORN
              || hood->obj_flags.type == ITEM_CONTAINER)
             && IS_SET(hood->obj_flags.value[1], CONT_HOODED)
             && IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
                if( strstr( title, "male" ) ) strcpy(title, "figure");
                snprintf(buf, sizeof(buf), "the %s%s in %s",
                 height_weight_descr(looker, ch), title, OBJS(hood, looker));
            } 
            else if ((mask = ch->equipment[WEAR_HEAD]) != NULL
             && char_can_see_obj(looker, mask)
             && mask->obj_flags.type == ITEM_MASK) {
                snprintf(buf, sizeof(buf), "the %s%s wearing %s",
                 height_weight_descr(looker, ch), title, OBJS(mask, looker));
            } 
            else if ((mask = ch->equipment[WEAR_FACE]) != NULL
             && char_can_see_obj(looker, mask)
             && mask->obj_flags.type == ITEM_MASK) {
                snprintf(buf, sizeof(buf), "the %s%s wearing %s",
                 height_weight_descr(looker, ch), title, OBJS(mask, looker));
            } 

            if (IS_SWITCHED_IMMORTAL(looker) && !IS_IMMORTAL(ch)) {
                CHAR_DATA *viewed = NULL;

                if( IS_SWITCHED(ch)) {
                    viewed = SWITCHED_PLAYER(ch);
                } else if( !IS_NPC(ch) ) {
                    viewed = ch;
                }

                // We already know our name, no need to show us again
                if (ch == looker)
                    viewed = NULL;

                if( viewed != NULL ) {
                    snprintf(buf2, sizeof(buf2), " (%s)", MSTR(viewed, name));
                    strcat(buf, buf2);
                }
            }
            return buf;
        }
    }
}

/* 1/12/03 - Nessalin */
/* Blatantly copying tiernan's code from real_sdesc_keywords */
char *
real_sdesc_keywords_obj(OBJ_DATA * object, CHAR_DATA *ch)
{
    static char buf[MAX_STRING_LENGTH];
    char tmp[MAX_STRING_LENGTH];
    int color_done = 0, sflag = 0, adverb_done = 0;

    buf[0] = '\0';              /* Clean out the old stuff */

    /* Add words based on adverbial flags */
    if ((IS_SET(object->obj_flags.color_flags, OAF_DYED))
        && (adverb_done < 1)) {
        strcat(buf, "dyed ");
        color_done++;
    }
    if ((IS_SET(object->obj_flags.color_flags, OAF_STRIPED))
        && (adverb_done < 1)) {
        strcat(buf, "striped ");
        color_done++;
    }
    if ((IS_SET(object->obj_flags.color_flags, OAF_BATIKED))
        && (adverb_done < 1)) {
        strcat(buf, "batiked ");
        color_done++;
    }
    if ((IS_SET(object->obj_flags.color_flags, OAF_PATTERNED))
        && (adverb_done < 1)) {
        strcat(buf, "patterned ");
        color_done++;
    }
    if ((IS_SET(object->obj_flags.color_flags, OAF_CHECKERED))
        && (adverb_done < 1)) {
        strcat(buf, "checkered ");
        color_done++;
    }


    /* Add words based on color flags */
    if (IS_SET(object->obj_flags.color_flags, OCF_RED)) {
        strcat(buf, "red ");
        color_done++;
    }
    if (IS_SET(object->obj_flags.color_flags, OCF_BLUE)) {
        strcat(buf, "blue ");
        color_done++;
    }
    if ((IS_SET(object->obj_flags.color_flags, OCF_YELLOW)) && (color_done < 2)) {
        color_done++;
        strcat(buf, "yellow ");
    }
    if ((IS_SET(object->obj_flags.color_flags, OCF_GREEN)) && (color_done < 2)) {
        color_done++;
        strcat(buf, "green ");
    }
    if ((IS_SET(object->obj_flags.color_flags, OCF_PURPLE)) && (color_done < 2)) {
        color_done++;
        strcat(buf, "purple ");
    }
    if ((IS_SET(object->obj_flags.color_flags, OCF_ORANGE)) && (color_done < 2)) {
        color_done++;
        strcat(buf, "orange ");
    }
    if ((IS_SET(object->obj_flags.color_flags, OCF_BLACK)) && (color_done < 2)) {
        color_done++;
        strcat(buf, "black ");
    }
    if ((IS_SET(object->obj_flags.color_flags, OCF_WHITE)) && (color_done < 2)) {
        color_done++;
        strcat(buf, "white ");
    }
    if ((IS_SET(object->obj_flags.color_flags, OCF_BROWN)) && (color_done < 2)) {
        color_done++;
        strcat(buf, "brown ");
    }
    if ((IS_SET(object->obj_flags.color_flags, OCF_GREY)) && (color_done < 2)) {
        color_done++;
        strcat(buf, "grey ");
    }
    if ((IS_SET(object->obj_flags.color_flags, OCF_SILVERY)) && (color_done < 2)) {
        color_done++;
        strcat(buf, "silvery ");
    }

    /* Add words based on state flags */
    if ((IS_SET(object->obj_flags.state_flags, OST_DUSTY)) && sflag == 0) {
        sflag++;
        strcat(buf, "dusty ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_BLOODY)) && sflag == 0) {
        sflag++;
        strcat(buf, "bloodied ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_SWEATY)) && sflag == 0) {
        sflag++;
        strcat(buf, "sweat-stained ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_TORN)) && sflag == 0) {
        sflag++;
        strcat(buf, "torn ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_TATTERED)) && sflag == 0) {
        sflag++;
        strcat(buf, "tattered ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_STAINED)) && sflag == 0) {
        sflag++;
        strcat(buf, "stained ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_MUDCAKED)) && sflag == 0) {
        sflag++;
        strcat(buf, "mud-caked ");
    }


    if ((IS_SET(object->obj_flags.state_flags, OST_OLD)) && sflag == 0) {
        sflag++;
        strcat(buf, "old ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_REPAIRED)) && sflag == 0) {
        sflag++;
        strcat(buf, "patched ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_SEWER)) && sflag == 0) {
        sflag++;
        strcat(buf, "smelly ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_LACED)) && sflag == 0) {
        sflag++;
        strcat(buf, "lace-edged ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_EMBROIDERED)) && sflag == 0) {
        sflag++;
        strcat(buf, "embroidered ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_FRINGED)) && sflag == 0) {
        sflag++;
        strcat(buf, "fringed ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_PATCHED)) && sflag == 0) {
        sflag++;
        strcat(buf, "patched ");
    }
    if ((IS_SET(object->obj_flags.state_flags, OST_BURNED)) && sflag == 0) {
        sflag++;
        switch (object->obj_flags.material) {
        case MATERIAL_METAL:
        case MATERIAL_STONE:
        case MATERIAL_CHITIN:
        case MATERIAL_OBSIDIAN:
        case MATERIAL_GEM:
        case MATERIAL_GLASS:
        case MATERIAL_BONE:
        case MATERIAL_CERAMIC:
        case MATERIAL_HORN:
        case MATERIAL_GRAFT:
        case MATERIAL_IVORY:
        case MATERIAL_DUSKHORN:
        case MATERIAL_TORTOISESHELL:
            strcat(buf, "blackened ");
            break;
        default:
            strcat(buf, "burned ");
            break;
        }
    }


    if (IS_SET(object->obj_flags.extra_flags, OFL_ETHEREAL_OK)) {
        strcat(buf, "shadowy ");
    }

    switch (object->obj_flags.type) {
    case ITEM_LIGHT:
        {
            int remaining = object->obj_flags.value[0];

            if (object->obj_flags.value[2] == LIGHT_TYPE_MAGICK
                && (object->obj_flags.value[0] < 0 || object->obj_flags.value[1] < 0)) {
                break;
            }

            if (object->obj_flags.value[1] == -1 && IS_LIT(object)) {
                remaining = 10;
            }

            if (!object->obj_flags.value[0] && IS_REFILLABLE(object)) {
                strcat(buf, "empty ");
            } else if (!object->obj_flags.value[0]
                       && !IS_LIT(object)
                       && (IS_CAMPFIRE(object) || IS_TORCH(object))) {
                strcat(buf, "ashen ");
            } else {
                switch (object->obj_flags.value[2]) {
                case LIGHT_TYPE_ANIMAL:
                    strcat(buf,
                           !IS_LIT(object) ? "" : remaining <= 1 ? "very dim " : remaining <=
                           5 ? "dim " : "shining ");
                    break;
                case LIGHT_TYPE_FLAME:
                    strcat(buf,
                           !IS_LIT(object) ? "unlit " : remaining <= 1 ? "very dim " : remaining <=
                           5 ? "dim " : IS_CAMPFIRE(object) | IS_TORCH(object) ? "burning " :
                           "lit ");
                    break;
                case LIGHT_TYPE_MAGICK:
                case LIGHT_TYPE_MINERAL:
                case LIGHT_TYPE_PLANT:
                    strcat(buf,
                           !IS_LIT(object) ? "" : remaining <= 1 ? "very dim " : remaining <=
                           5 ? "dim " : "glowing ");
                    break;
                default:
                    strcat(buf,
                           !IS_LIT(object) ? "" : remaining <= 1 ? "very dim " : remaining <=
                           5 ? "dim " : "lit ");
                }
            }

            if (IS_CAMPFIRE(object)) {
                strcat(buf,
                       object->obj_flags.value[1] > 40 ? "bonfire " : object->obj_flags.value[1] >
                       30 ? "huge " : object->obj_flags.value[1] >
                       20 ? "large " : object->obj_flags.value[1] >
                       10 ? "medium-sized " : object->obj_flags.value[1] <= 4 ? "small " : "");
            }

            break;
        }
    case ITEM_ARMOR:
        if ((object->obj_flags.value[0] != object->obj_flags.value[3])
            && object->obj_flags.value[3] > 0) {
            int temp = get_condition(object->obj_flags.value[0], object->obj_flags.value[3]);
            if ((object->obj_flags.material == MATERIAL_SKIN)
                || (object->obj_flags.material == MATERIAL_CLOTH)
                || (object->obj_flags.material == MATERIAL_SILK))
                sprintf(tmp, "%s ", soft_condition[temp]);
            else
                sprintf(tmp, "%s ", hard_condition[temp]);
            strcat(buf, tmp);
        }
        break;
    case ITEM_FOOD:
        if (object->obj_flags.value[0]
            && obj_default[object->nr].value[0]) {
            int percent = PERCENTAGE(object->obj_flags.value[0],
                                     obj_default[object->nr].value[0]);

            if (obj_default[object->nr].value[0] < object->obj_flags.value[0])
                percent = 100;

            strcat(buf,
                   percent > 90 ? "" : percent > 70 ? "partially eaten " : percent >
                   40 ? "half eaten " : "small portion ");
        }
        break;
    case ITEM_DRINKCON:
        if ((!object->obj_flags.value[1]))
            strcat(buf, "empty ");
        break;
    case ITEM_NOTE:
        // must have a character to use title keywords
        if( ch != NULL ) {
            char * title;
            /* if they can read the language */
            /* and if it has a title */
            if (has_skill(ch, object->obj_flags.value[2])
                && ch->skills[object->obj_flags.value[2]]->learned >= 40
                && (title =
                    find_ex_description("[BOOK_TITLE]", object->ex_description, TRUE)) != NULL) {
                strcat(buf, title);
            }
            break;
        }
    }

    return buf;
}

/* 4/23/98 - Tiernan */
char *
real_sdesc_keywords(CHAR_DATA * ch)
{
    static char buf[MAX_STRING_LENGTH];
    OBJ_DATA *mask;
    OBJ_DATA *hood;
    OBJ_DATA *robe;

    buf[0] = '\0';              /* Clean out the old stuff */

    /* Check to see if there is a condition where the sdesc
     * is something that we've modified via hoods and/or masks */

    if (((hood = ch->equipment[WEAR_ABOUT]) != NULL)
        && (hood->obj_flags.type == ITEM_WORN || hood->obj_flags.type == ITEM_CONTAINER)
        && IS_SET(hood->obj_flags.value[1], CONT_HOODED)
        && IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
        strcat(buf, "figure ");
        strcat(buf, OSTR(hood, name));
    }

    if (((hood = ch->equipment[WEAR_HEAD]) != NULL)
        && (hood->obj_flags.type == ITEM_WORN || hood->obj_flags.type == ITEM_CONTAINER)
        && IS_SET(hood->obj_flags.value[1], CONT_HOODED)
        && IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
        strcat(buf, "figure ");
        strcat(buf, OSTR(hood, name));
    }

    if (((hood = ch->equipment[WEAR_BODY]) != NULL)
        && (hood->obj_flags.type == ITEM_WORN || hood->obj_flags.type == ITEM_CONTAINER)
        && IS_SET(hood->obj_flags.value[1], CONT_HOODED)
        && IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
        strcat(buf, "figure ");
        strcat(buf, OSTR(hood, name));
    }
    if ((robe = ch->equipment[WEAR_ABOUT]) != NULL
        && (robe->obj_flags.type == ITEM_WORN || robe->obj_flags.type == ITEM_CONTAINER)
        && (obj_index[robe->nr].vnum == 50056 || obj_index[robe->nr].vnum == 50600
            || obj_index[robe->nr].vnum == 50601 || obj_index[robe->nr].vnum == 50604
            || obj_index[robe->nr].vnum == 50610)) {
        strcat(buf, "templar ");
        strcat(buf, MSTR(ch, name));
        if (!IS_NPC(ch))
            strcat(buf, ch->player.extkwds);
    }

    if (((mask = ch->equipment[WEAR_HEAD]) != NULL)
        && mask->obj_flags.type == ITEM_MASK)
        strcat(buf, OSTR(mask, name));

    return buf;
}

#ifdef USE_OLD_REAL_SDESC

/*  bad -Tenebrius */

char *
real_sdesc(CHAR_DATA * ch, int vis)
{
    char *buf = (char *) malloc(256 * (sizeof *buf));

    strcpy(buf, "");
    switch (vis) {
    case 0:                    /* can't see */
        strcpy(buf, "someone");
        break;
    case 2:                    /* hidden */
        strcpy(buf, "a shadow");
        break;
    case 3:                    /* invis */
        strcpy(buf, "a blur");
        break;
    case 4:                    /* ethereal */
        strcpy(buf, "a ghostly image");
        break;
    case 5:                    /* infravision */
        strcpy(buf, "a red shape");
        break;
    case 6:                    /* bad weather */
        strcpy(buf, "a faint shape");
        break;
    case 7:                    /* detect magick */
        strcpy(buf, "a shimmering shape");
        break;
    default:                   /* see normally */
        strcpy(buf, MSTR(ch, short_descr));
        break;
    }
    return (buf);
}
#endif

int
get_condition(int current, int max)
{
    int n, percent;

    if ((current == 0) || ((max == 0) || (max == 1))) {
        n = 0;
    } else {
        percent = ((100 * current) / max);
        if (percent >= 90)
            n = 6;
        else if (percent >= 75)
            n = 5;
        else if (percent >= 50)
            n = 4;
        else if (percent >= 25)
            n = 3;
        else if (percent >= 10)
            n = 2;
        else
            n = 1;
    }
    return (n);
}


int
hands_free(CHAR_DATA * ch)
{
    int h = 2;

    if (GET_RACE(ch) == RACE_MANTIS)
        h = 4;
    if (ch->equipment[ES])
        h--;
    if (ch->equipment[EP])
        h--;
    if (ch->equipment[ETWO])
        h -= 2;

    return (h);
}


int
current_hour(void)
{
    return time(0);
/*
    return (time_info.hours + (time_info.day * 9) + (time_info.month * 2079) +
            ((time_info.year - 1000) * 6237));
*/
}


char *
height_weight_descr(CHAR_DATA * viewer, CHAR_DATA * target)
{
    int weight_diff, height_diff;
    char hstr[MAX_STRING_LENGTH], wstr[MAX_STRING_LENGTH];
    static char return_str[MAX_STRING_LENGTH];

    wstr[0] = '\0';
    hstr[0] = '\0';
    return_str[0] = '\0';

    if (viewer == target)
        return "";

    height_diff = (GET_HEIGHT(viewer) - GET_HEIGHT(target));
    weight_diff = (GET_WEIGHT(viewer) - GET_WEIGHT(target));

    if (height_diff > 2) {
        if (height_diff < 15)
            snprintf(hstr, sizeof(hstr), "short ");
        if ((height_diff >= 15) && (height_diff < 25))
            snprintf(hstr, sizeof(hstr), "very short ");
        if ((height_diff >= 25) && (height_diff < 50))
            snprintf(hstr, sizeof(hstr), "extremely short ");
        if (height_diff >= 50)
            snprintf(hstr, sizeof(hstr), "tiny ");
    } else {
        if (height_diff < -2) {
            height_diff *= -1;
            if (height_diff < 15)
                snprintf(hstr, sizeof(hstr), "tall ");
            if ((height_diff >= 15) && (height_diff < 25))
                snprintf(hstr, sizeof(hstr), "very tall ");
            if ((height_diff >= 25) && (height_diff < 50))
                snprintf(hstr, sizeof(hstr), "huge ");
            if (height_diff >= 50)
                snprintf(hstr, sizeof(hstr), "gigantic ");
        } else {
            strcpy(hstr, "");
        }
    }

    if (weight_diff > 2) {
        if (weight_diff < 15)
            snprintf(wstr, sizeof(wstr), "thin ");
        if ((weight_diff >= 15) && (weight_diff < 25))
            snprintf(wstr, sizeof(wstr), "very thin ");
        if ((weight_diff >= 25) && (weight_diff < 50))
            snprintf(wstr, sizeof(wstr), "extremely thin ");
        if (weight_diff >= 50)
            snprintf(wstr, sizeof(wstr), "gaunt ");
    } else {
        if (weight_diff < -2) {
            weight_diff *= -1;
            if (weight_diff < 15)
                snprintf(wstr, sizeof(wstr), "thick ");
            if ((weight_diff >= 15) && (height_diff < 25))
                snprintf(wstr, sizeof(wstr), "stout ");
            if ((weight_diff >= 25) && (height_diff < 50))
                snprintf(wstr, sizeof(wstr), "heavy ");
            if (weight_diff >= 50)
                snprintf(wstr, sizeof(wstr), "obese ");
        } else {
            strcpy(wstr, "");
        }
    }

    /* 4/15/98 Ensure return_str is empty before we rebuild it. - Tiernan */
    return_str[0] = '\0';

    if (hstr[0] != '\0')
        strcpy(return_str, hstr);
    if (wstr[0] != '\0') {
        if (return_str[0] == '\0')
            strcpy(return_str, wstr);
        else {
            strcat(return_str, "and ");
            strcat(return_str, wstr);
        }
    }
    if (return_str[0] == '\0')
        return "";
    return return_str;
}

int
get_char_size(CHAR_DATA * ch)
{
    int sz;
    float tmp;

    tmp = ch->player.weight + ch->player.height;
    tmp = tmp / 7.4;

    sz = (int) tmp;
    return sz;
}

int
char_init_height(int race)
{
    int ht = 72;

/* Racial heights updated by Savak, 2/3/00.
   In order from smallest to largest; alphabetical within each subgroup */

    switch (race) {
    case RACE_COCHRA:
    case RACE_AGOUTI:
    case RACE_GIMPKA_RAT:
    case RACE_KANK_FLY:
    case RACE_SILTHOPPER:
    case RACE_SLUG:
        ht = (10 + dice(1, 4));
        break;
    case RACE_GRETH:
    case RACE_GURTH:
    case RACE_KOLORI:
    case RACE_RODENT:
    case RACE_RITIKKI:
    case RACE_SAND_FLIER:
    case RACE_SHIK:
    case RACE_SILTSTRIDER:
    case RACE_BAT:
    case RACE_TOAD:
    case RACE_TREGIL:
    case RACE_TURAAL:
    case RACE_TUSK_RAT:
        ht = (22 + dice(1, 10));
        break;
    case RACE_CHALTON:
    case RACE_ESCRU:
    case RACE_JOHZA:
    case RACE_JOZHAL:
    case RACE_VESTRIC:
    case RACE_RAVEN:
    case RACE_SCORPION:
    case RACE_SNAKE1:
    case RACE_SNAKE2:
    case RACE_ARACH:
    case RACE_SPIDER:
    case RACE_TANDU:
    case RACE_TARANTULA:
    case RACE_VERRIN:
    case RACE_VISHITH:
    case RACE_VULTURE:
        ht = (30 + dice(1, 6));
        break;
    case RACE_MEPHIT:
        ht = (29 + dice(1, 12));
        break;
    case RACE_XILL:
        ht = (82 + dice(1, 4));
        break;
    case RACE_BAMUK:
    case RACE_CURR:
    case RACE_EAGLE:
    case RACE_KALICH:
    case RACE_QUIRRI:
    case RACE_SKEET:
    case RACE_GNU:
    case RACE_GORMI:
    case RACE_WOLF:
    case RACE_YOMPAR_LIZARD:
        ht = (33 + dice(1, 5));
        break;
    case RACE_HALFLING:
        ht = (36 + dice(1, 8));
        break;
    case RACE_FIREANT:
    case RACE_GORTOK:
    case RACE_GOUDRA:
    case RACE_JAKHAL:
    case RACE_RANTARRI:
    case RACE_KIYET_LION:
    case RACE_KRYL:
    case RACE_WEZER:
        ht = (44 + dice(1, 8));
        break;
    case RACE_DWARF:
    case RACE_GETH:
    case RACE_MERCH_BAT:
    case RACE_SHADOW:
        ht = (50 + dice(2, 4));
        break;
    case RACE_ANAKORE:
    case RACE_BALTHA:
    case RACE_BELGOI:
    case RACE_DUSKHORN:
    case RACE_GHAATI:
    case RACE_GHYRRAK:
    case RACE_KYLORI:
    case RACE_MUL:
    case RACE_SAND_RAPTOR:
    case RACE_TARI:
    case RACE_TEMBO:
        ht = (60 + dice(2, 6));
        break;
    case RACE_CHEOTAN:
    case RACE_HUMAN:
    case RACE_MELEP:
    case RACE_VAMPIRE:
    case RACE_GHOUL:
    case RACE_ZOMBIE:
    case RACE_SUB_ELEMENTAL:
        ht = (66 + dice(2, 8));
        break;
    case RACE_BATMOUNT:
    case RACE_GWOSHI:
    case RACE_HALF_ELF:
    case RACE_PLAINSOX:
    case RACE_THRYZN:
        ht = (70 + dice(2, 6));
        break;
    case RACE_GALITH:
    case RACE_GITH:
        ht = (78 + dice(2, 4));
        break;
    case RACE_BANSHEE:
    case RACE_CHARIOT:
    case RACE_DESERT_ELF:
    case RACE_DURRIT:
    case RACE_ERDLU:
    case RACE_ELF:
    case RACE_MAGERA:
        ht = (78 + dice(2, 8));
        break;
    case RACE_CILOPS:
        ht = (80 + dice(1, 30));
        break;
    case RACE_MANTIS:
        ht = (82 + dice(1, 4));
        break;
    case RACE_SILT_TENTACLE:
    case RACE_SCRAB:
    case RACE_BIG_INSECT:
    case RACE_CARRU:
    case RACE_GIANT_SPI:
    case RACE_HORSE:
    case RACE_KANK:
    case RACE_RATLON:
    case RACE_SUNLON:
    case RACE_SUNBACK:
    case RACE_THLON:
    case RACE_WAR_BEETLE:
        ht = (84 + dice(2, 12));
        break;
    case RACE_ALATAL:
    case RACE_BEAST:
    case RACE_FIEND:
    case RACE_THORNWALKER:
    case RACE_THYGUN:
    case RACE_USELLIAM:
        ht = (100 + dice(1, 15));
        break;
    case RACE_AVANGION:
    case RACE_AUROCH:
    case RACE_BRAXAT:
    case RACE_DEMON:
    case RACE_DJINN:
    case RACE_ELEMENTAL:
    case RACE_GOLEM:
    case RACE_HALF_GIANT:
        ht = (125 + dice(3, 10));
        break;
    case RACE_BAHAMET:
        ht = (140 + dice(2, 10));
        break;
    case RACE_ANKHEG:
    case RACE_DUJAT_WORM:
    case RACE_GAJ:
    case RACE_INIX:
    case RACE_SILTFLYER:
    case RACE_WYVERN:
        ht = (150 + dice(4, 40));
        break;
    case RACE_ROC:
        ht = (450 + dice(4, 20));
        break;
    case RACE_DRAGON:
    case RACE_GIANT:
    case RACE_MEKILLOT:
    case RACE_SILT:
    case RACE_WORM:
        ht = (1100 + dice(6, 50));
        break;
    case RACE_GIZHAT:
    case RACE_SAURIAN:
    default:
        ht = (66 + dice(2, 8));
        break;
    }
    return ht;
}

/* Racial weights updated by Savak 2/5/00.
   In order of increasing weight; alphabetical within each subgroup. */

int
char_init_weight(int race)
{
    int wt = 7;

    switch (race) {
    case RACE_COCHRA:
    case RACE_BAT:
    case RACE_AGOUTI:
    case RACE_GIMPKA_RAT:
    case RACE_KANK_FLY:
    case RACE_SHADOW:
    case RACE_SILTHOPPER:
    case RACE_SLUG:
        wt = (17 + dice(3, 4)) / 20;
        break;
    case RACE_CHALTON:
    case RACE_ESCRU:
    case RACE_GRETH:
    case RACE_GURTH:
    case RACE_JOZHAL:
    case RACE_JOHZA:
    case RACE_RAVEN:
    case RACE_RITIKKI:
    case RACE_RODENT:
    case RACE_SAND_FLIER:
    case RACE_SCORPION:
    case RACE_SHIK:
    case RACE_SILTSTRIDER:
    case RACE_SNAKE1:
    case RACE_SNAKE2:
    case RACE_ARACH:
    case RACE_SPIDER:
    case RACE_TANDU:
    case RACE_TARANTULA:
    case RACE_TURAAL:
    case RACE_TUSK_RAT:
    case RACE_VERRIN:
    case RACE_VESTRIC:
        wt = (40 + dice(5, 5)) / 20;
        break;
    case RACE_XILL:
        wt = (450 + dice(1, 20)) / 20;
        break;
    case RACE_EAGLE:
    case RACE_HALFLING:
    case RACE_KOLORI:
    case RACE_TOAD:
    case RACE_TREGIL:
    case RACE_VISHITH:
    case RACE_VULTURE:
        wt = (48 + dice(5, 4)) / 20;
        break;
    case RACE_GETH:
    case RACE_KALICH:
    case RACE_KYLORI:
    case RACE_MERCH_BAT:
    case RACE_QUIRRI:
        wt = (55 + dice(4, 4)) / 20;
        break;
    case RACE_CURR:
    case RACE_GORMI:
    case RACE_GORTOK:
    case RACE_GOUDRA:
    case RACE_JAKHAL:
    case RACE_WOLF:
        wt = (70 + dice(4, 5)) / 20;
        break;
    case RACE_CHEOTAN:
    case RACE_DUSKHORN:
    case RACE_GHAATI:
    case RACE_HALF_ELF:
    case RACE_MELEP:
    case RACE_SAND_RAPTOR:
    case RACE_SKEET:
    case RACE_TEMBO:
    case RACE_ZOMBIE:
        wt = (115 + dice(3, 12)) / 20;
        break;
    case RACE_MEPHIT:
        wt = (19 + dice(1, 11));
        break;
    case RACE_BANSHEE:
    case RACE_HUMAN:
    case RACE_RANTARRI:
    case RACE_KIYET_LION:
    case RACE_THRYZN:
    case RACE_VAMPIRE:
    case RACE_GHOUL:
        wt = (130 + dice(6, 10)) / 20;
        break;
    case RACE_GALITH:
    case RACE_GITH:
        wt = (150 + dice(3, 9)) / 20;
        break;
    case RACE_DESERT_ELF:
    case RACE_ELF:
        wt = (150 + dice(3, 10)) / 20;
        break;
    case RACE_BALTHA:
    case RACE_BAMUK:
    case RACE_DWARF:
    case RACE_DURRIT:
    case RACE_ERDLU:
    case RACE_YOMPAR_LIZARD:
        wt = (170 + dice(4, 10)) / 20;
        break;
    case RACE_ANAKORE:
    case RACE_BELGOI:
    case RACE_GHYRRAK:
    case RACE_KRYL:
    case RACE_MAGERA:
    case RACE_MUL:
    case RACE_SUB_ELEMENTAL:
    case RACE_TARI:
    case RACE_THORNWALKER:
        wt = (210 + dice(5, 20)) / 20;
        break;
    case RACE_SILT_TENTACLE:
    case RACE_GNU:
    case RACE_ASLOK:
    case RACE_BIG_INSECT:
    case RACE_SCRAB:
    case RACE_CILOPS:
    case RACE_FIREANT:
    case RACE_MANTIS:
    case RACE_GIANT_SPI:
        wt = (450 + dice(1, 20)) / 20;
        break;
    case RACE_ALATAL:
    case RACE_BATMOUNT:
    case RACE_BEAST:
    case RACE_CARRU:
    case RACE_CHARIOT:
    case RACE_FIEND:
    case RACE_GWOSHI:
    case RACE_HORSE:
    case RACE_KANK:
    case RACE_PLAINSOX:
    case RACE_RATLON:
    case RACE_SUNBACK:
    case RACE_SUNLON:
    case RACE_THLON:
    case RACE_WAR_BEETLE:
        wt = (700 + dice(2, 100)) / 20;
        break;
    case RACE_AUROCH:
    case RACE_AVANGION:
    case RACE_BRAXAT:
    case RACE_DEMON:
    case RACE_DJINN:
    case RACE_ELEMENTAL:
    case RACE_GOLEM:
    case RACE_HALF_GIANT:
    case RACE_THYGUN:
    case RACE_USELLIAM:
        wt = (1500 + dice(3, 100)) / 20;
        break;
    case RACE_ANKHEG:
    case RACE_DUJAT_WORM:
    case RACE_GAJ:
    case RACE_INIX:
    case RACE_SILTFLYER:
    case RACE_WYVERN:
        wt = (1800 + dice(4, 100)) / 20;
        break;
    case RACE_BAHAMET:
        wt = (2000 + dice(4, 100)) / 20;
        break;
    case RACE_DRAGON:
    case RACE_GIANT:
    case RACE_MEKILLOT:
    case RACE_ROC:
    case RACE_SILT:
    case RACE_WORM:
        wt = (30000 + dice(4, 400)) / 20;
        break;
    case RACE_GIZHAT:
    case RACE_SAURIAN:
    default:
        wt = (180 + dice(6, 10)) / 20;
        break;
    }
    return wt;
}

// Counts the number of lights in a room and returns FALSE if none
//   are found, TRUE if at least one is found. -Nessalin 9/24/2000
int
room_lit(struct room_data *room)
{
    //  OBJ_DATA *temp_obj;
    //  CHAR_DATA *temp_ch;

    if (!room)
        return TRUE;

    return (room->light > 0);
}

/* definition of darkness in a room */
int
IS_DARK(struct room_data *room)
{
    int jihae, lirathu, vis = 0;
    CHAR_DATA *victim;
    CHAR_DATA *temp;

    jihae = weather_info.moon[JIHAE];
    lirathu = weather_info.moon[LIRATHU];

    if (!room)
        return FALSE;

    victim = room->people;
    while (victim) {
        temp = victim->next_in_room;
        if ((affected_by_spell(victim, SPELL_IMMOLATE))
            || (affected_by_spell(victim, SPELL_FIRE_ARMOR))
            || (affected_by_spell(victim, SPELL_SANCTUARY)))
            return FALSE;
        victim = temp;
    }

    if (IS_SET(room->room_flags, RFL_DARK) && (!room_lit(room)))
        return TRUE;
    if (IS_SET(room->room_flags, RFL_INDOORS))
        return FALSE;
    if (IS_SET(room->room_flags, RFL_LIT))
        return FALSE;
    if (room_lit(room))
        return FALSE;

    if (use_global_darkness)
        return TRUE;

    if (weather_info.sunlight == SUN_DARK) {
        vis = -1;
        if ((jihae >= JIHAE_RISE) && (jihae <= JIHAE_SET))
            vis += 1;
        if ((lirathu >= LIRATHU_RISE) && (lirathu <= LIRATHU_SET))
            vis += 1;
        if (room->light)
            vis = MAX(vis, 0);
    }

    if (vis < 0)
        return TRUE;

    return FALSE;
}

int
IS_LIGHT(struct room_data *room)
{
    return (!IS_DARK(room));
}

char *save_catagory_names[9] = {
    "almost immune",
    "extremely high",
    "very high",
    "high",
    "moderate",
    "low",
    "very low",
    "extremely low",
    "none"
};

char *
save_category(int num)
{
    int catagory;

    if (num >= 100)
        catagory = 0;
    else if (num >= 90)
        catagory = 1;
    else if (num >= 75)
        catagory = 2;
    else if (num >= 55)
        catagory = 3;
    else if (num >= 45)
        catagory = 4;
    else if (num >= 30)
        catagory = 5;
    else if (num >= 15)
        catagory = 6;
    else if (num >= 1)
        catagory = 7;
    else
        catagory = 8;

    return (save_catagory_names[catagory]);
}

char *number_suffix_names[4] = {
    "st",
    "nd",
    "rd",
    "th",
};

char *
number_suffix(int num)
{
    int suffix;

    if (num == 1)
        suffix = 0;
    else if (num == 2)
        suffix = 1;
    else if (num == 3)
        suffix = 2;
    /* changed this so 211th wouldn't show up as 211st -Morg */
    else if ((num % 100) < 20 && (num % 100) > 10)
        suffix = 3;
    else if ((num % 10) == 1)
        suffix = 0;
    else if ((num % 10) == 2)
        suffix = 1;
    else if ((num % 10) == 3)
        suffix = 2;
    else
        suffix = 3;

    return (number_suffix_names[suffix]);
};

/*
char *number_suffix(int num) 
{
  char suf[256];
  char *sf;
  
  if (num == 1)
    strcpy(suf, "st");
  else if (num == 2)
    strcpy(suf, "nd");
  else if (num == 3)
    strcpy(suf, "rd");
  else if (num < 20)
    strcpy(suf, "th");
  else if ((num % 10) == 1)
    strcpy(suf, "st");
  else if ((num % 10) == 2)
    strcpy(suf, "nd");
  else if ((num % 10) == 3)
    strcpy(suf, "rd");
  else strcpy(suf, "th");
  
  CREATE(sf, char, strlen(suf) + 1);
  sprintf(sf, "%s", suf);
  return sf;
}
*/


/* return a number from 0..10 (maybe more) which is encumberance class */
int
get_encumberance(CHAR_DATA * ch)
{
    int enc = 0;


    enc = IS_CARRYING_W(ch);

    if (IS_SET(ch->specials.act, CFL_MOUNT))
        enc /= 3;

    if (str_app[GET_STR(ch)].carry_w)
        enc = (10 * enc) / str_app[GET_STR(ch)].carry_w;
    else
        enc *= 10;

    return (enc);
}

int
get_obj_encumberance(OBJ_DATA * obj, CHAR_DATA * ch)
{
    int enc = 0;

    enc = GET_OBJ_WEIGHT(obj);

    if (IS_SET(ch->specials.act, CFL_MOUNT))
        enc /= 3;

    if (str_app[GET_STR(ch)].carry_w)
        enc = (10 * enc) / str_app[GET_STR(ch)].carry_w;
    else
        enc *= 10;

    return (enc);
}


char *encumb_class_names[11] = {
    "very light",
    "no problem",
    "light",
    "easily manageable",
    "manageable",
    "heavy, but manageable",
    "very heavy",
    "VERY heavy",
    "extremely heavy",
    "unbelievably heavy",
    "terribly unusual"
};

char *
encumb_class(int enc)
{

    if (enc >= 10)
        enc = 9;
    if (enc < 0)
        enc = 11;

    return (encumb_class_names[enc]);
}


/* creates a random number in interval [from;to] */
int
number(int from, int to)
{
    float n;

    if (to < from) return from;
    n = (to - from + 1.0);      // Size of the interval
    return (from + (int) (n * random() / (RAND_MAX + 1.0)));

    /* Guidance from the manpage on rand() says to use this format
     * for generating random numbers because it uses high-order bits:
     * 
     * j=1+(int) (10.0*rand()/(RAND_MAX+1.0));
     *
     * instead of this format which uses low-order bits:
     *
     * j=1+(rand() % 10);
     */
}


/* simulates dice roll */
int
dice(int num, int size)
{
    int r;
    int sum = 0;

    if (size < 1) {
        gamelogf("dice: size %d < 1", size);
        size = 1;
    }

    for (r = 1; r <= num; r++)
        sum += number(1, size);
    return (sum);
}


/* gets a random character from a room, non-immortal */
CHAR_DATA *
get_random_non_immortal_in_room(struct room_data * rm)
{
    int count, rnd;
    CHAR_DATA *temp;

    count = 0;
    if (!rm) {
        gamelog("No room value passed to get_random_non_immortal_in_room.");
        return ((CHAR_DATA *) 0);
    }

    for (temp = rm->people; temp; temp = temp->next_in_room) {
        if (!IS_IMMORTAL(temp))
            count++;
    };

    if (count == 0)
        return ((CHAR_DATA *) 0);

    rnd = number(0, count);

    for (temp = rm->people; temp; temp = temp->next_in_room) {
        if (!IS_IMMORTAL(temp))
            rnd--;
        if ((rnd <= 0) && !IS_IMMORTAL(temp))
            break;
    };

    return (temp);

}

#ifdef OLD_RANDOM
/* gets a random character from a room, non-immortal */
CHAR_DATA *
get_random_non_immortal_in_room(struct room_data * rm)
{
    int count, rnd;
    CHAR_DATA *temp;

    count = 0;
    for (temp = rm->people; temp; temp = temp->next_in_room) {
        if (!IS_IMMORTAL(temp))
            count++;
    };

    if (count == 0)
        return ((CHAR_DATA *) 0);

    rnd = number(0, count);

    for (temp = rm->people; temp && (rnd > 0); temp = temp->next_in_room) {
        if (!IS_IMMORTAL(temp))
            rnd--;
    };

    return (temp);

}
#endif

int silence_log = 0;

/* writes a string to the gamelog */
void
log_to_file(const char *str, char *fn)
{

    long ct;
    char tmstr[128];
    FILE *fd;
    char outbuf[1024];

    if (silence_log)
        return;

    if (!fn)
        fd = stderr;
    else if (!(fd = fopen(fn, "a"))) {
        gamelog("Unable to open %s gamelog file");
        return;
    }

    strncpy(outbuf, str, sizeof(outbuf));
    size_t i = strlen(outbuf) - 1;
    for (; i > 0 && isspace(outbuf[i]); i--)
        outbuf[i] = '\0';

    ct = time(0);
    asctime_r(localtime(&ct), tmstr);

    i = strlen(tmstr) - 1;
    for (; i > 0 && isspace(tmstr[i]); i--)
        tmstr[i] = '\0';

    fprintf(fd, "%s: %s\n", tmstr, outbuf);
    fflush(fd);

    if (fn)
        fclose(fd);

    return;
}

void
warning_log(const char *str)
{
    char msg[512];

    log_to_file(str, WARNING_LOG_FILE);
    snprintf(msg, sizeof(msg), "/* WARNING LOG */\n\r/* %s */\n\r/* WARNING LOG */\n\r", str);
    send_to_overlords(msg);

    return;
}

void
errorlog(const char *str)
{
    gamelogf("ERROR: %s", str);
  //  shhlog_backtrace();
}

void
errorlogf(const char *fmt, ...)
{
    char buf[1024];
    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    errorlog(buf);
}

void
hack_log(const char *str)
{
    log_to_file(str, "hack_log");
    return;
}

void
stat_log(char *str)
{
    log_to_file(str, "stat_log");
    return;
}

void
who_log(char *str)
{
    log_to_file(str, "who_log");
    return;
}

void
who_log_line(char *str)
{
    FILE *fd;
    char fn[] = "who_log";

    if (!(fd = fopen(fn, "a"))) {
        gamelog("Unable to open %s gamelog file");
        return;
    }

    fprintf(fd, "%s\n", str);
    fflush(fd);
    fclose(fd);

    return;
}

void
qgamelog(int quiet_type, const char *str)
{
    char buf[3000], buf2[3000];
    struct descriptor_data *i;
    CHAR_DATA *to;
    int index;

    strncpy(buf2, str, sizeof(buf2));
    for (index = strlen(buf2) - 1; index >= 0 && isspace(buf2[index]); index--) // Remove trailing whitespace
        buf2[index] = 0;

    log_to_file(buf2, NULL);

    if (descriptor_list) {
        if (strlen(str) < 250)
            snprintf(buf, sizeof(buf), "/* %s */\n\r", buf2);
        else {
            snprintf(buf, sizeof(buf), "/* %.200s ... (truncated, too long) */\n\r", buf2);
        }

        for (i = descriptor_list; i; i = i->next) {
            if (!i->character) continue;

            if( i->connected && IS_SWITCHED_IMMORTAL(i->character) ) {
                to = SWITCHED_PLAYER(i->character);
            } else {
                to = i->character;
            }

            if (!i->connected
             && !IS_NPC(to) 
             && GET_LEVEL(to) > BUILDER
             && !IS_SET(to->specials.quiet_level, QUIET_LOG)
             && !IS_SET(to->specials.quiet_level, quiet_type))
                cprintf(i->character, "%s", buf);
        }
    }
}

void
roomlog(ROOM_DATA *rm, const char *str)
{
   qroomlog( QUIET_LOG, rm, str );
}

void
qroomlog(int quiet_type, ROOM_DATA *rm, const char *str ) {
    CHAR_DATA *tch = NULL;

    if (!rm)
        return;

    for (tch = rm->people; tch; tch = tch->next_in_room) {
        if (IS_IMMORTAL(tch) 
         && !IS_SET(tch->specials.quiet_level, quiet_type) 
         && !IS_SET(tch->specials.quiet_level, QUIET_LOG)) 
            cprintf(tch, "/* %s */\n\r", str);
    }
}

void
gamelog(const char *str)
{
    qgamelog(0, str);
}

void
connect_log(const char *str)
{
   qgamelog(QUIET_CONNECT, str);
}

void
death_log(const char *str)
{
    qgamelog(QUIET_DEATH, str);
    log_to_file(str, "deathlog");
}

/* writes a string to the gamelog */
void
shhlog(const char *str)
{
    log_to_file(str, NULL);
}

void
shhlog_backtrace()
{
/*    void *btbuf[20];
    int btsize = sizeof(btbuf)/sizeof(btbuf[0]);
    char **output;
    int i;

    btsize = backtrace(btbuf, btsize);
    output = backtrace_symbols(btbuf, btsize);

    for (i = 0; i < btsize; i++) {
        if (strstr(output[i], __FUNCTION__) ||
            strstr(output[i], "errorlog"))
            continue;
        
        shhlog(output[i]);
    }
    free(output); */
}

void
warning_logf(const char *fmt, ...)
{

    char buf[1024];

    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    warning_log((char *) buf);
}

void
hack_logf(const char *fmt, ...)
{

    char buf[1024];
    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    hack_log((char *) buf);
}

void
roomlogf(ROOM_DATA *rm, const char *fmt, ...)
{
    char buf[1024];
    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    qroomlog(QUIET_LOG, rm, buf);
}

void
qroomlogf( int quiet_type, ROOM_DATA *rm, const char *fmt, ...)
{
    char buf[1024];
    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    qroomlog(quiet_type, rm, buf);
}


struct timeval
timediff(struct timeval *a, struct timeval *b)
{
    struct timeval rslt, tmp;

    tmp = *a;

    if ((rslt.tv_usec = tmp.tv_usec - b->tv_usec) < 0) {
        rslt.tv_usec += 1000000;
        tmp.tv_sec--;
    }

    if ((rslt.tv_sec = tmp.tv_sec - b->tv_sec) < 0) {
        rslt.tv_usec = 0;
        rslt.tv_sec = 0;
    }

    return rslt;
}

bool should_do_perf_logging = FALSE;

void
perf_enter(struct timeval *enter_time, const char *method)
{
   perflogf("Entering '%s'\n\r", method);
   gettimeofday(enter_time, NULL);
}

void
perf_exit(const char *method, struct timeval enter_time)
{
   struct timeval diff, now;

   gettimeofday(&now, NULL);
   diff = timediff(&now, &enter_time);

   perflogf("Exiting  '%s': %ld secs %ld usecs\n\r",
    method, diff.tv_sec, diff.tv_usec);
}

void
perflogf(const char *fmt, ...)
{
    char buf[1024];
    va_list argp;

    if (!should_do_perf_logging) return;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    log_to_file(buf, "/armag/last-crash/perf.log");
}

void
gamelogf(const char *fmt, ...)
{
    char buf[1024];
    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    gamelog(buf);
}

void
qgamelogf(int quiet_type, const char *fmt, ...)
{
    char buf[1024];
    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    qgamelog(quiet_type, buf);
}

void
shhlogf(const char *fmt, ...)
{

    char buf[1024];
    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    shhlog(buf);
}

void
sprintbit(long vektor, char *names[], char *result)
{
    long nr;
    bool begin;

    *result = '\0';
    begin = TRUE;

    for (nr = 0; vektor; vektor >>= 1) {
        if (IS_SET(1, vektor)) {
            if (!begin) {
                strcat(result, " ");
            } else {
                begin = FALSE;
            }
            if (*names[nr] != '\n') {
                strcat(result, names[nr]);
            } else {
                strcat(result, "UNDEFINED");
            }
        }
        if (*names[nr] != '\n')
            nr++;
    }

    if (!*result)
        strcat(result, "None");
}


void
sprint_flag(long vektor, struct flag_data *flag, char *result)
{
    long nr;
    bool begin;

    *result = '\0';
    begin = TRUE;

    for (nr = 0; *flag[nr].name; nr++) {
        if (IS_SET(vektor, flag[nr].bit)) {
            if (!begin)
                strcat(result, " ");
            else
                begin = FALSE;
            strcat(result, flag[nr].name);
        }
    }

    if (!*result)
        strcat(result, "None");
}



void
sprinttype(int type, char *names[], char *result)
{
    int nr;

    for (nr = 0; (*names[nr] != '\n'); nr++);

    if (type < nr)
        strcpy(result, names[type]);
    else
        strcpy(result, "UNDEFINED");
}


void
sprint_attrib(int val, struct attribute_data *att, char *result)
{
    int nr;

    for (nr = 0; *att[nr].name; nr++)
        if (att[nr].val == val)
            break;

    if (*att[nr].name)
        strcpy(result, att[nr].name);
    else
        strcpy(result, "unknown attribute");
}


struct time_info_data
age(CHAR_DATA * ch)
{
    /*  long secs; */
    struct time_info_data player_age;

    player_age = mud_time_passed(ch->player.time.starting_time, time(0));
    player_age.year += ch->player.time.starting_age;

    return player_age;
}

/* begin stat aging routines */


/* adjust stat according to new age.  age_stat uses the age_stat_table 
   to determine the chance that a given stat will go up or down when 
   the character grows older or younger by one year.*/
void
age_stat(CHAR_DATA * ch, int old_age, int new_age, char stat)
{
    struct race_data ch_race = race[(int) GET_RACE(ch)];
    int max_age = ch_race.max_age;
    int old_age_group, new_age_group;
    int old_mod, new_mod;
    int orig_value;

    /* if the race is immortal, its stats don't change */
    if (max_age < 1)
        return;

    old_age_group = (old_age * 20) / max_age;
    new_age_group = (new_age * 20) / max_age;

    if( old_age_group == new_age_group ) return;

    old_age_group = MAX(old_age_group, 0);
    old_age_group = MIN(old_age_group, 20);
    new_age_group = MAX(new_age_group, 0);
    new_age_group = MIN(new_age_group, 20);

    switch (stat) {
    case ('s'):                /* str */
        old_mod = age_stat_table.str[old_age_group];
        new_mod = age_stat_table.str[new_age_group];
        orig_value = ch->abilities.str;
        ch->abilities.str = lroundf((float)ch->abilities.str * new_mod / old_mod); 
        //qgamelogf(QUIET_REROLL, "str: %d = %d orig * %d new / %d old",
         //ch->abilities.str, orig_value, new_mod, old_mod);
        break;
    case ('e'):                /* end */
        old_mod = age_stat_table.end[old_age_group];
        new_mod = age_stat_table.end[new_age_group];
        orig_value = ch->abilities.end;
        ch->abilities.end = lroundf((float)ch->abilities.end * new_mod / old_mod); 
        //qgamelogf(QUIET_REROLL, "end: %d = %d orig * %d new / %d old",
         //ch->abilities.end, orig_value, new_mod, old_mod);
        break;
    case ('a'):                /* agl */
        old_mod = age_stat_table.agl[old_age_group];
        new_mod = age_stat_table.agl[new_age_group];
        orig_value = ch->abilities.agl;
        ch->abilities.agl = lroundf((float)ch->abilities.agl * new_mod / old_mod); 
        //qgamelogf(QUIET_REROLL, "agl: %d = %d orig * %d new / %d old",
         //ch->abilities.agl, orig_value, new_mod, old_mod);
        break;
    case ('w'):                /* wis */
        old_mod = age_stat_table.wis[old_age_group];
        new_mod = age_stat_table.wis[new_age_group];
        orig_value = ch->abilities.wis;
         ch->abilities.wis = lroundf((float)ch->abilities.wis * new_mod / old_mod); 
        //qgamelogf(QUIET_REROLL, "wis: %d = %d orig * %d new / %d old",
         //ch->abilities.wis, orig_value, new_mod, old_mod);
        break;
    default:
        gamelog("Unable to find stat type in age_stat");
        break;
    }

    ch->tmpabilities = ch->abilities;

    return;
}

/* adjust stats according to new age.  Second argument can either be the 
   character's old age or -1.  In the latter case, age_stats will use the
   age of the character at the time of his last logon */
void
age_all_stats(CHAR_DATA * ch, int old_age)
{
    int new_age;
    struct time_info_data new_age_data;
    char edesc[256];
    bool init_age = FALSE;

    if (old_age < 0) {
        old_age =
            (mud_time_passed(ch->player.time.starting_time, ch->player.time.logon).year +
             ch->player.time.starting_age);
    }

// age stats if older than stat aging code
    // 1217989140 is the # of seconds since 1970 when this change went in
    if( !get_char_extra_desc_value(ch, "[AGE_STAT_INIT]",edesc,sizeof(edesc))){
       old_age = ch->player.time.starting_age;
       init_age = TRUE;
       set_char_extra_desc_value(ch, "[AGE_STAT_INIT]", "1");
    }

    new_age_data = age(ch);
    new_age = new_age_data.year;
    if (old_age != new_age) {
        struct race_data ch_race = race[(int) GET_RACE(ch)];
        int max_age = ch_race.max_age;
        int old_age_group, new_age_group;
        int old_str, old_end, old_agl, old_wis;
        char buf[MAX_STRING_LENGTH];
        long ct;
        char *tmstr;

        old_str = ch->abilities.str;
        old_end = ch->abilities.end;
        old_agl = ch->abilities.agl;
        old_wis = ch->abilities.wis;

        if( new_age_data.month == 0 && new_age_data.day == 0 ) 
            cprintf(ch, "Happy Birthday! You are now %d.\n", new_age);
        else
            cprintf(ch, "Happy Belated Birthday! You are now %d.\n", new_age);

        if( !init_age ) {
            cprintf( ch, "You have aged %d years since your last visit.\n",
             new_age - old_age );
        } 

        age_stat(ch, old_age, new_age, 's');
        age_stat(ch, old_age, new_age, 'e');
        age_stat(ch, old_age, new_age, 'a');
        age_stat(ch, old_age, new_age, 'w');

        old_age_group = (old_age * 20) / max_age;
        new_age_group = (new_age * 20) / max_age;
        if( old_age_group != new_age_group ) {
            ct = time(0);
            tmstr = asctime(localtime(&ct));
            sprintf( buf, "Aged from %d (%d) to %d (%d) on %s"
             "  Old Stats:\n"
             "    Str: %2d  Agl: %2d  Wis: %2d  End %2d\n"
             "  New Stats:\n"
             "    Str: %2d  Agl: %2d  Wis: %2d  End %2d\n\n",
             old_age, old_age_group, new_age, new_age_group, tmstr, 
             old_str, old_agl, old_wis, old_end, 
             ch->abilities.str, ch->abilities.agl, ch->abilities.wis,
             ch->abilities.end);
            set_char_extra_desc_value(ch, "[STAT_AGING]", buf);            

            if( !IS_NPC(ch)) {
                qgamelogf( QUIET_REROLL, "%s (%s) %s", ch->name, ch->account,
                 buf);
            }
        }
    }

    return;
}

int
set_age(CHAR_DATA * ch, int new_age)
{
    struct time_data *ch_age = &ch->player.time;
    struct time_info_data old_age = age(ch);

    ch_age->starting_age = -1 * (old_age.year - new_age - ch_age->starting_age);
    age_all_stats(ch, old_age.year);

    return 1;
}

int
set_age_raw(CHAR_DATA * ch, int new_age)
{
    struct time_data *ch_age = &ch->player.time;
    struct time_info_data old_age = age(ch);

    ch_age->starting_age = -1 * (old_age.year - new_age - ch_age->starting_age);
    return 1;
}

int
get_char_str(CHAR_DATA * ch)
{
    return ch->tmpabilities.str;
}

void
set_char_str(CHAR_DATA * ch, int val)
{
    ch->tmpabilities.str = val;
}

int
get_char_agl(CHAR_DATA * ch)
{
    return ch->tmpabilities.agl;
}

void
set_char_agl(CHAR_DATA * ch, int val)
{
    ch->tmpabilities.agl = val;
}

int
get_char_wis(CHAR_DATA * ch)
{
    return ch->tmpabilities.wis;
}

void
set_char_wis(CHAR_DATA * ch, int val)
{
    ch->tmpabilities.wis = val;
}

int
get_char_end(CHAR_DATA * ch)
{
    return ch->tmpabilities.end;
}

void
set_char_end(CHAR_DATA * ch, int val)
{
    ch->tmpabilities.end = val;
}

int
get_char_off(CHAR_DATA * ch)
{
    return ch->tmpabilities.off;
}

void
set_char_off(CHAR_DATA * ch, int val)
{
    ch->tmpabilities.off = val;
}

int
get_char_def(CHAR_DATA * ch)
{
    return ch->tmpabilities.def;
}

void
set_char_def(CHAR_DATA * ch, int val)
{
    ch->tmpabilities.def = val;
}

int
get_char_armor(CHAR_DATA * ch)
{
    return ch->tmpabilities.armor;
}

void
set_char_armor(CHAR_DATA * ch, int val)
{
    ch->tmpabilities.armor = val;
}

int
get_char_damroll(CHAR_DATA * ch)
{
    return ch->tmpabilities.damroll;
}

void
set_char_damroll(CHAR_DATA * ch, int val)
{
    ch->tmpabilities.damroll = val;
}

int
char_occupies_space(CHAR_DATA *ch)
{
    // Immortals can always fit
    if (IS_IMMORTAL(ch))
        return 0;

    // If the newcomer is ethereal, he's good to go
    if (is_char_ethereal(ch))
        return 0;

    // Half-giants are double-sized
    if (GET_RACE(ch) == RACE_HALF_GIANT)
        return 2;

    return 1;
}

int
count_solid_occupants(ROOM_DATA *rm)
{
    int result = 0;

    for (CHAR_DATA *ch = rm->people; ch; ch = ch->next_in_room) {
        result += char_occupies_space(ch);
    }

    return result;
}

int
room_can_contain_char(ROOM_DATA *rm, CHAR_DATA *ch)
{
    if (IS_IMMORTAL(ch))
        return 1;

    // If it's not a tunnel room we're done.
    // TODO: Change tunnel concept to support multiple occupants
    if (!IS_SET(rm->room_flags, RFL_TUNNEL))
        return 1;

    // Otherwise, is the room empty?
    if (count_solid_occupants(rm) + char_occupies_space(ch) <= 1)
        return 1;

    // Must not fit
    return 0;
}

// Can this character and his victims/mounts enter the room?
// If not, handle messaging the character and return false.
int
handle_tunnel(ROOM_DATA *room, CHAR_DATA *ch)
{
    if (IS_SET(room->room_flags, RFL_TUNNEL)) {
        CHAR_DATA *mt = ch->specials.riding;
        CHAR_DATA *sub = ch->specials.subduing;

        if (mt && ((char_occupies_space(ch) + char_occupies_space(mt)) > 1)) {
            send_to_char("There's not enough room in there for you and your mount!\n\r", ch);
            return 0;
        }

        if (sub && ((char_occupies_space(ch) + char_occupies_space(sub)) > 1)) {
            cprintf(ch, "You and your victim can't both fit in there!  Nice try...\n\r");
            return 0;
        }

        if (!room_can_contain_char(room, ch)) {
            send_to_char("There isn't enough room!\n\r", ch);

            // This is the original logic: if you are trying to enter
            // a tunnel and fail, you drop whatever you're dragging
            if (ch->lifting) {
                char buf[1024];
                sprintf(buf, "%s is pulled from your hands.\n\r",
                        capitalize(format_obj_to_char(ch->lifting, ch, 1)));
                send_to_char(buf, ch);

                drop_lifting(ch, ch->lifting);
            }
            return 0;
        }
    }

    // I think the "drop whatever you're dragging" stuff should
    // actually happen HERE.  -- Xygax
    return 1;
}


/* returns:
    0 if sub cannot see obj at all
    1 if sub can see obj clearly
    2 if sub detects obj as a hidden form
    3 if sub detects obj as an invis form
    4 if sub detects obj as an ethereal form
    5 if sub detects obj as a heat shape (infra)
    6 if sub can barely see obj (weather)
    7 if sub detects obj as a magick aura (detect magick)
    8 if sub can see obj through psionic means
 */

int
char_can_see_char(CHAR_DATA *sub, CHAR_DATA *obj)
{
    int j = 1;
    int chance = 0;
    struct affected_type *af;
    struct affected_type *vanished = NULL;

    if (sub == 0) {
        errorlog("Passed null value in sub in char_can_see_char.");
        return (0);
    }

    if (obj == 0) {
        errorlogf("Passed null value in obj in char_can_see_char.  Subject was: %s in room %d",
                  GET_NAME(sub), sub->in_room->number);
        return (0);
    }

    if (sub == obj)
        return (1);

    if ((GET_LEVEL(sub) < obj->specials.il) || (obj->specials.il && IS_NPC(sub)))
        return (0);

    if (IS_IMMORTAL(sub))
        return (1);

    if (IS_SET(obj->specials.act, CFL_SILT_GUY) && obj->in_room
        && (obj->in_room->sector_type == SECT_SILT)) {
        if (IS_AFFECTED(sub, CHAR_AFF_SCAN) && !sub->specials.fighting && 
            !obj->specials.fighting) {
            if (sub->skills[SKILL_SCAN])
                chance = sub->skills[SKILL_SCAN]->learned;
            else
                chance = 0;
            chance += wis_app[GET_WIS(sub)].percep;

            switch(GET_POS(sub)){
                case POSITION_STANDING:
                    // no change
                    break;
                case POSITION_SITTING: /* 50% of skill */
                    chance /= 2;
                    break;
                case POSITION_RESTING: /* 25% of skill */
                    chance /= 4;
                    break;
            }

            chance -= agl_app[GET_AGL(obj)].stealth_manip /2;
            // At lower tide the chance is increased to spot them
            if (weather_info.tides < 30)
                chance += 10; 
            
            // At higher tide the chanve is descreased to spot them
            if (weather_info.tides > 70)
                chance -= 10;

            // Still a slight, random change.
            chance += (number(-5, 5));

            if (chance >= 70) {
                return (6);
            } else {
                return (0);
                gain_skill(sub, SKILL_SCAN, 1);
            }
        } else {
            if (obj->specials.fighting || (obj->specials.position < POSITION_RESTING)) {
                return (1);
            } else {
                return (0);
            }
        }
    }

    if (IS_AFFECTED(sub, CHAR_AFF_BLIND))
        return (0);

    if (!AWAKE(sub))
        return (0);

    if (affected_by_spell(sub, SPELL_ELEMENTAL_FOG) && has_sight_spells_resident(sub))
        return (0);

    if (affected_by_spell(obj, SPELL_ELEMENTAL_FOG) && has_sight_spells_resident(sub))
        return (0);

    if (!obj)
        return (0);

    if (!obj->in_room)
        return (0);

    if (obj->in_room && IS_DARK(obj->in_room)) {
        if ((GET_RACE(sub) == RACE_HALFLING) || (GET_RACE(sub) == RACE_MANTIS))
            j = 1;
        else if ((IS_AFFECTED(sub, CHAR_AFF_INFRAVISION)) || (GET_RACE(sub) == RACE_VAMPIRE))
            j = 5;
        else
            j = 0;
    }

    if (IS_AFFECTED(obj, CHAR_AFF_INVISIBLE)) {
        if (IS_AFFECTED(sub, CHAR_AFF_DETECT_INVISIBLE))
            j = 1;
        else if (is_char_watching_char(sub, obj))
            j = 3;
        else if (IS_AFFECTED(sub, CHAR_AFF_SCAN) && !sub->specials.fighting) {
            if (sub->skills[SKILL_SCAN])
                chance = sub->skills[SKILL_SCAN]->learned;
            else
                chance = 0;

            chance += wis_app[GET_WIS(sub)].percep;

            switch(GET_POS(sub)){
                case POSITION_STANDING:
                    // no change
                    break;
                case POSITION_SITTING: /* 50% of skill */
                    chance /= 2;
                    break;
                case POSITION_RESTING: /* 25% of skill */
                    chance /= 4;
                    break;
            }

            // if affected by the invisibility spell
            if ((af = affected_by_spell(obj, SPELL_INVISIBLE)) != NULL) {
                chance -= af->power * 10;
            } else {
                // no spell, so assume an 'average' casting
                chance -= 35;
            }

            if (number(1, 100) <= chance)
                j = 3;
            else {
                j = 0;
                gain_skill(sub, SKILL_SCAN, 1);
            }
	} else if (IS_AFFECTED(sub, CHAR_AFF_DETECT_MAGICK)) {
            // Detect magickal auras
            chance = wis_app[GET_WIS(sub)].percep;
            // if affected by the invisibility spell
            if ((af = affected_by_spell(obj, SPELL_INVISIBLE)) != NULL) {
                chance -= af->power * 10;
            } else {
                // no spell, so assume an 'average' casting
                chance -= 35;
            }

            // if affected by the detect magick spell
            if ((af = affected_by_spell(sub, SPELL_DETECT_MAGICK)) != NULL) {
                chance += af->power * 10;
            } else {
                // no spell, so assume an 'average' casting
                chance += 35;
            }

            if (number(1, 100) <= chance)
                j = 7;
            else
                j = 0;
        } else
            j = 0;
    }

    if (is_char_ethereal(obj)
        && (!IS_AFFECTED(sub, CHAR_AFF_DETECT_ETHEREAL) && !is_char_ethereal(sub)))
        j = 0;

    if (IS_AFFECTED(obj, CHAR_AFF_HIDE)) {
        /* Hiding in a no-hide room makes them visible, always. */
        if (obj->in_room && IS_SET(obj->in_room->room_flags, RFL_NO_HIDE))
            return 1;

        if (is_char_watching_char(sub, obj)) {
            j = 2;
        } else if (IS_AFFECTED(sub, CHAR_AFF_SCAN) && !sub->specials.fighting) {
            if (sub->skills[SKILL_SCAN])
                chance = sub->skills[SKILL_SCAN]->learned;
            else
                chance = 0;
            chance += wis_app[GET_WIS(sub)].percep;

            switch(GET_POS(sub)){
                case POSITION_STANDING:
                    // no change
                    break;
                case POSITION_SITTING: /* 50% of skill */
                    chance /= 2;
                    break;
                case POSITION_RESTING: /* 25% of skill */
                    chance /= 4;
                    break;
            }

            chance -= agl_app[GET_AGL(obj)].stealth_manip;
            chance += (number(-20, 20));

            if (obj->skills[SKILL_HIDE]) {
                if (chance > obj->skills[SKILL_HIDE]->learned) {
                    j = 2;
                } else {
                    j = 0;
                    gain_skill(sub, SKILL_SCAN, 1);
                }
            } else {
               j = 0;
            }
        } else {
            j = 0;
        }

        /* check */
        if (IS_AFFECTED(sub, CHAR_AFF_INFRAVISION)) {
            if (obj->skills[SKILL_HIDE]) {
                chance = obj->skills[SKILL_HIDE]->learned;
                if (number(1, 100) >= chance)
                    /* MAX checks to make sure we don't blow over SCAN results */
                    j = MAX(5, j);
                else
                    j = MAX(0, j);
            } else
                j = MAX(5, j);  /* no hide skill */
        } else
            j = MAX(j, 0);      /*  end else if infra  */
    }
    /*  end if hidden  */
    if (j && sub->in_room && (visibility(sub) < 0)) {
        if ((IS_AFFECTED(sub, CHAR_AFF_INFRAVISION)) || (GET_RACE(sub) == RACE_VAMPIRE))
            j = 5;
        else
            j = 6;
    }

    /* Some psionics checking stuff.  If the two participants are of the
     * same race-type then we can evaluate any psionic abilities.  This
     * is the same check used in contact to make sure the two minds are
     * compatible for psionics.
     */
    if (GET_RACE_TYPE(sub) == GET_RACE_TYPE(obj)) {
        /* Vanish check */
        vanished = affected_by_spell(obj, PSI_VANISH);
	if (vanished && !IS_IMMORTAL(obj)
		&& is_in_psionic_range(obj, sub, vanished->power)) {
            if (can_use_psionics(sub)) {
                if (has_skill(sub, PSI_VANISH) && !psi_skill_success(obj, sub, PSI_VANISH, 0)) {
                    /* Psi didn't beat them, they were seen in whatever 
                     * manner they normally would be seen.  Let the psionicist
                     * gain for their failure.
                     */
                    gain_skill(obj, PSI_VANISH, 1);
                } else
                    j = 0;      // Psionicist beat the vanish contest
            } // Without the ability to use psionics, the psi will not prevail
        }

        /* Sense presence check : Note that this comes after the vanish check
         * on purpose, so that psis can detect other psis.
         */
        if (affected_by_spell(sub, PSI_SENSE_PRESENCE)
            && obj->in_room == sub->in_room
            && can_use_psionics(obj)
            && !affected_by_spell(obj, PSI_CONCEAL)
            && !IS_IMMORTAL(obj)) {
            j = 8;
        }
    }

    return (j);
}


int
char_can_see_obj(CHAR_DATA * sub, OBJ_DATA * obj)
{
    char edesc[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char name_string[MAX_INPUT_LENGTH];
    char account_string[MAX_INPUT_LENGTH];

    /* sanity check the viewer */
    if (!sub) {
        gamelog("Sub not present in char_can_see_obj");
        return FALSE;
    }

    /* sanity check the object */
    if (!obj) {
        gamelogf("Obj not present in char_can_see_obj, char: %s in room %d", MSTR(sub, name),
                 (sub->in_room ? sub->in_room->number : -1));
        return FALSE;
    }

    /* immortals can always see everything */
    if (IS_IMMORTAL(sub))
        return TRUE;

    /* anti-mortal flag means mortals will never see it */
    if (IS_SET(obj->obj_flags.extra_flags, OFL_ANTI_MORT))
        return FALSE;

    /* in the silt? */
    if (sub->in_room && sub->in_room->sector_type == SECT_SILT
        && (obj->obj_flags.type != ITEM_FURNITURE
            || (obj->obj_flags.type == ITEM_FURNITURE
                && !IS_SET(obj->obj_flags.value[1], FURN_SKIMMER)))
        && !IS_SET(sub->specials.act, CFL_SILT_GUY)
        && obj->in_room == sub->in_room)
        return FALSE;

    /* if you're not holding it */
    if (obj->equipped_by != sub) {
        /* invisible objects (and don't have detect invis on) */
        if (IS_SET(obj->obj_flags.extra_flags, OFL_INVISIBLE)
            && !IS_AFFECTED(sub, CHAR_AFF_DETECT_INVISIBLE))
            return FALSE;

        /* Inverse logic: If you are not ethereal and are not affected by
         * detect ethereal, you will fail to see an ethereal object.  If
         * you are affected by either spell, you would be able to see an
         * ethereal object. - Tiernan 2/16/2005 (for Halaster) */
        if (IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL)
            && !IS_AFFECTED(sub, CHAR_AFF_DETECT_ETHEREAL)
            && !IS_AFFECTED(sub, CHAR_AFF_ETHEREAL))
            return FALSE;

        /* simplified teleport object logic -Morg 3/26/06 */
        if (obj->obj_flags.type == ITEM_TELEPORT) {

            if (IS_AFFECTED(sub, CHAR_AFF_DETECT_MAGICK))
                return TRUE;

            /* Grab the player info edesc on the object */
            if (get_obj_extra_desc_value(obj, "[PC_info]", edesc, sizeof(edesc))) {
                sprintf(name_string, "%s", edesc);
            }

            /* Handle NPCs differently */
            if (IS_NPC(sub)) {
                sprintf(buf, "%d", npc_index[sub->nr].vnum);

                if (!strcmp(name_string, buf))
                    return TRUE;
            } else {            /* players */
                /* Grab the account info edesc on the object */
                if (get_obj_extra_desc_value(obj, "[ACCOUNT_info]", edesc, sizeof(edesc))) {
                    sprintf(account_string, "%s", edesc);
                }

                /* if they cast it then yes they can see it */
                if (!strcmp(name_string, sub->name)
                    && !strcmp(account_string, sub->account))
                    return TRUE;
            }

            /* otherwise no */
            return FALSE;
        }

        if (obj->carried_by != sub) {
            if (IS_AFFECTED(sub, CHAR_AFF_BLIND) || (affected_by_spell(sub, SPELL_ELEMENTAL_FOG)
                                                     && has_sight_spells_resident(sub))
                || (!IS_LIGHT(sub->in_room)
                    && (!((IS_AFFECTED(sub, CHAR_AFF_INFRAVISION))
                          || (GET_RACE(sub) == RACE_HALFLING) || (GET_RACE(sub) == RACE_MANTIS)
                          || (GET_RACE(sub) == RACE_VAMPIRE)))))
                return FALSE;
        }
    }
    return TRUE;
}

int
will_char_fall_in_room(CHAR_DATA * ch, struct room_data *room)
{
    /* check if its a falling room and not immortal */
    if (IS_IMMORTAL(ch))
        return (0);
    if (!ch->in_room)
        return (0);
    if (!IS_SET(ch->in_room->room_flags, RFL_FALLING))
        return (0);
    if (!room->direction[DIR_DOWN])
        return (0);

    /* if they are mounted, then they either fly or fall w/ the mount
     * so just check the mount from now on 
     */

    if (ch->specials.riding)
        ch = ch->specials.riding;

    /*  if its a SILT sector, then SILT_GUY's will not fall */

    if (room->sector_type == SECT_SILT) {
        if (IS_SET(ch->specials.act, CFL_SILT_GUY)) {
            return (0);
        } else if (ch->on_obj && ch->on_obj->obj_flags.type == ITEM_FURNITURE
                   && IS_SET(ch->on_obj->obj_flags.value[1], FURN_SKIMMER)) {
            return (0);
        }
    }

    /* if it is NO_CLIMB and they are not flying, they will fall */
    /* don't forget levitation */
    if (IS_SET(room->room_flags, RFL_NO_CLIMB)) {
        if (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
            return (1);
    }

    /* if they are not flying and are not climbing 
     * and are not levitating, they will fall */
    /* feather fall should fall, just at a reduced rate,
     * which isn't handled here -Morg */
    if (!IS_AFFECTED(ch, CHAR_AFF_CLIMBING)
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL))))
        return (1);

    /* if it is a NO_FLYING room and
     * they are not CLIMBING they will fall */

    if (IS_SET(room->room_flags, RFL_NO_FLYING) && !IS_AFFECTED(ch, CHAR_AFF_CLIMBING)) {
        return (1);
    };

    /* ok...passed all inpection, they must be not be falling */
    return (0);
}

int
will_obj_fall_in_room(OBJ_DATA * obj, struct room_data *room)
{
    return (FALSE);

}

int
stat_num(CHAR_DATA * ch, int stat)
{
    int cur;
    /*  char bug[MAX_STRING_LENGTH]; */

    switch (stat) {
    case ATT_STR:
        cur = GET_STR(ch);
        break;
    case ATT_AGL:
        cur = GET_AGL(ch);
        break;
    case ATT_WIS:
        cur = GET_WIS(ch);
        break;
    case ATT_END:
        cur = GET_END(ch);
        break;
    default:
        return (0);
    }
    return stat_num_raw((int) GET_RACE(ch), stat, cur);
}

int
stat_num_raw(int for_race, int stat, int cur)
{
    float perc, min, max;

    min = compute_stat_rating(for_race, stat, 0);
    max = compute_stat_rating(for_race, stat, 1);

    if (cur < min)
        return 0;
    if (cur > max)
        return 8;

    perc = (max - min) / 7.0;

    if (cur >= (max - perc))
        return 7;
    else if (cur >= (max - (2.0 * perc)))
        return 6;
    else if (cur >= (max - (3.0 * perc)))
        return 5;
    else if (cur >= (max - (4.0 * perc)))
        return 4;
    else if (cur >= (max - (5.0 * perc)))
        return 3;
    else if (cur >= (max - (6.0 * perc)))
        return 2;
    else
        return 1;
}

/*
  int consumption()
  {
  struct mallinfo mem;
  
  mem = mallinfo();
  return mem.arena;
  }
  */

void
all_cap(char *inp, char *out)
{
    int i;

    for (i = 0; i <= strlen(inp); i++)
        out[i] = UPPER(inp[i]);
}



/* flag and attribute-related functions */

void
send_attribs_to_char(struct attribute_data *att, CHAR_DATA * ch)
{
    int col, i;
    char tmp[256], buf[4096];
    CHAR_DATA *orig = SWITCHED_PLAYER(ch);

    strcpy(buf, "------------------------------------------------------------\n\r");
    for (i = 0, col = 0; *att[i].name; i++) {
        if (att[i].priv <= GET_LEVEL(orig)) {
            snprintf(tmp, sizeof(tmp), "%s%-15s", (!col && i) ? "\n\r" : "", att[i].name);
            strcat(buf, tmp);
            col++;
        }

        if (col > 3)
            col = 0;
    }
    strcat(buf, "\n\r");

    send_to_char(buf, ch);
}

void
send_list_to_char_f(CHAR_DATA * ch, const char * const list[], int (*fun) (int i))
{
  shhlog("starting: send_list_to_char_f");

  char tmp[256];
     int i, t = 0, total = 0, strl, width = 0, wide, n = 0, num = 0;
#define MAXWIDE 80
    char numbuf[MAXWIDE], buf[MAX_STRING_LENGTH];
    /*
     * int nw;
     */

    for (i = 0; *list[i] != '\n'; ++i) {
        ++total;                /* keep track of total number of elements */
        strl = strlen(list[i]);
        if (strl && (!fun || ((*fun) (i))))
            ++num;              /* the number, since will not show emptys */
        if (strl > width)
            width = strl;       /* store the maximum length               */
    };

    sprintf(tmp, "strl: %d", strl);
    shhlog(tmp);

    ++width;                    /* make sure there is one space after each */

    sprintf(tmp, "width: %d", width);
    shhlog(tmp);

    if (width > 75)
        send_to_char("an element is wider than 75, this is bad\n\r", ch);

    wide = MAXWIDE / width;     /* wide now is number of columns */

    sprintf(tmp, "wide: %d", wide);
    shhlog(tmp);

    if ((n / wide + 1) > 60)
        send_to_char("List to big, make send_list_to_char bigger\n\r", ch);
    strcpy(buf, "");
    for (i = 0; *list[i] != '\n'; ++i) {
      shhlog("Starting 2nd list loop.");
        strl = strlen(list[i]);

        sprintf(tmp, "i: %d, length: %d", i, strl);
        shhlog(tmp);

        sprintf(tmp, "list[i]: '%s'", list[i]);
        shhlog(tmp);

        if (strl && (!fun || ((*fun) (i)))) {
          shhlog("Checkpoint A");
            if ((++n == num) || (n % wide == 0)) {
              shhlog("checkpoint B");
                snprintf(numbuf, sizeof(numbuf), "%s\n\r", list[i]);
            } else {
              shhlog("checkpoint C");
                snprintf(numbuf, sizeof(numbuf), "%*s", 0 - width, list[i]);
            }
            shhlog("Checkpoint D");
            strncat(buf, numbuf, sizeof(buf));
        }
        shhlog ("chekcpoint E");
        t++;
        shhlog ("chekcpoint F");

    };
    shhlog("checkpoint G");
    page_string(ch->desc, buf, 1);
  shhlog("ending: send_list_to_char_f");
}

void
send_flags_to_char(struct flag_data *flag, CHAR_DATA * ch)
{
    int col, i;
    char tmp[256], buf[4096];
    CHAR_DATA *orig = SWITCHED_PLAYER(ch);

    strcpy(buf, "------------------------------------------------------------\n\r");
    for (i = 0, col = 0; *flag[i].name; i++) {
        if (flag[i].priv <= GET_LEVEL(orig)) {
            snprintf(tmp, sizeof(tmp), "%s%-15s", (!col && i) ? "\n\r" : "", flag[i].name);
            strcat(buf, tmp);
            col++;
        }

        if (col > 3)
            col = 0;
    }
    strcat(buf, "\n\r");

    send_to_char(buf, ch);
}

int
get_attrib_num(struct attribute_data *att, const char *arg)
{
    int i;

    for (i = 0; *att[i].name; i++)
        if (!strncasecmp(arg, att[i].name, strlen(arg)))
            return i;
    return -1;
}

int
get_attrib_val(struct attribute_data *att, char *arg)
{
    int i;

    for (i = 0; *att[i].name; i++)
        if (!strncasecmp(arg, att[i].name, strlen(arg)))
            return att[i].val;
    return -1;
}

const char *
get_attrib_name(struct attribute_data *att, int val)
{
    int i;
    for (i = 0; *att[i].name; i++)
        if (att[i].val == val)
            return att[i].name;

    return "(bogus)";
}


int
get_flag_num(struct flag_data *flag, const char *arg)
{
    int i;

    for (i = 0; *flag[i].name; i++)
        if (!strncasecmp(arg, flag[i].name, strlen(arg)))
            return i;
    return -1;
}

long
get_flag_bit(struct flag_data *flag, const char *arg)
{
    int i;

    for (i = 0; *flag[i].name; i++)
        if (!strncasecmp(arg, flag[i].name, strlen(arg)))
            return flag[i].bit;

    return -1;
}


/* returns TRUE if char has priveleges */

#define UID_ALL                 0
#define RESERVED_ZONE           1

bool
has_privs_in_zone(CHAR_DATA * ch, int zone, int mode)
{
    int g;
    CHAR_DATA *orig = SWITCHED_PLAYER(ch);

    struct zone_priv_data *temp;

    if ((zone < 0) || (zone > top_of_zone_table))
        return FALSE;

    if (orig->player.level == OVERLORD)
        return TRUE;

    if (orig->player.level == HIGHLORD)
        if (zone_table[zone].is_open)
            return TRUE;

    if (orig->specials.uid == zone_table[zone].uid)
        return TRUE;

    if (zone == RESERVED_ZONE) {
        if (mode == MODE_SEARCH)
            return TRUE;
        else if (mode == MODE_CONTROL)
            return TRUE;
        else if (mode == MODE_CREATE)
            return TRUE;
    }

    if (zone_table[zone].uid == UID_ALL)
        if (IS_SET(zone_table[zone].flags, mode))
            return TRUE;

    for (temp = zone_table[zone].privs; temp; temp = temp->next) {
        if (temp->uid != 0)
            if (temp->uid == orig->specials.uid)
                if (IS_SET(temp->type, mode))
                    return (TRUE);

    };

    for (g = 0; g < MAX_GROUPS; g++)
        if (IS_SET(zone_table[zone].groups, grp[g].bitv)
            && IS_SET(orig->specials.group, grp[g].bitv))
            break;

    if (g == MAX_GROUPS)
        return FALSE;

    if (!IS_SET(zone_table[zone].flags, mode))
        return FALSE;

    return TRUE;
}

bool
check_tunnel_size(CHAR_DATA * ch, int door)
{                               /*  Kel  */
    int sz, opening = 0;
    char buf[256];

    if (is_char_ethereal(ch))
        return TRUE;

    if (IS_SET(EXIT(ch, door)->exit_info, EX_TUNN_1))
        opening = opening + 1;
    if (IS_SET(EXIT(ch, door)->exit_info, EX_TUNN_2))
        opening = opening + 2;
    if (IS_SET(EXIT(ch, door)->exit_info, EX_TUNN_4))
        opening = opening + 4;

    switch (opening) {          /*  Begining of switch for cases of opening */
    case 1:                    /*  No pcs allowed at all  */
        opening = 4;
        snprintf(buf, sizeof(buf), "Even the smallest halfling would not be able ");
        strcat(buf, "to wriggle in there.\n\r");
        break;
    case 2:                    /*  Only halflings  */
        opening = 6;
        snprintf(buf, sizeof(buf), "Nothing larger than a halfling would be able ");
        strcat(buf, "to squeeze in there.\n\r");
        break;
    case 3:                    /*  Dwarves, small muls  */
        opening = 9;
        snprintf(buf, sizeof(buf), "The opening is too narrow for anything larger ");
        strcat(buf, "than the puniest of muls.\n\r");
        break;
    case 4:                    /*  Small humans, average muls  */
        opening = 10;
        snprintf(buf, sizeof(buf), "That way is too narrow for you to enter.\n\r");
        break;
    case 5:                    /*  Small elves, average humans  */
        opening = 11;
        snprintf(buf, sizeof(buf), "You would need to be a very thin elf to squeeze ");
        strcat(buf, "into that passage.\n\r");
        break;
    case 6:                    /*  All but mantis and half-giants  */
        opening = 13;
        snprintf(buf, sizeof(buf), "Try as you might, you just cannot squeeze your");
        strcat(buf, "self in there.\n\r");
        break;
    case 7:                    /*  All but half-giants  */
        opening = 20;
        snprintf(buf, sizeof(buf), "Your body is too large to fit in there.\n\r");
        break;
    default:
        snprintf(buf, sizeof(buf), "ERROR: Problem with EX_TUNN_* flags.\n\r");
    }                           /*  End of switch for cases of opening  */

    if (ch->specials.riding) {
        sz = get_char_size(ch->specials.riding);
        snprintf(buf, sizeof(buf), "%s is too large to ride through there.",
                 MSTR(ch->specials.riding, short_descr));
    } else
        sz = get_char_size(ch);

    if (opening < sz) {
        send_to_char(buf, ch);
        return FALSE;
    }
    return TRUE;
}

int
validate_room_in_roomlist(struct room_data *room)
{
    struct room_data *tmp;

    for (tmp = room_list; tmp != 0; tmp = tmp->next)
        if (tmp == room)
            return (TRUE);
    return (FALSE);
}

int
validate_char_in_charlist(CHAR_DATA * ch)
{
    CHAR_DATA *tmp;
    char message[256];

    if (ch->nr == 0) {          /* this is ok */
    } else if (ch->nr < top_of_npc_t) {        /* I think less than or equal is ok *//* this is ok */
    } else {
        snprintf(message, sizeof(message),
                 "Found a character with real number %d, top_of_npc_t == %d", ch->nr, top_of_npc_t);
        gamelog(message);
    }

    for (tmp = character_list; tmp != 0; tmp = tmp->next)
        if (tmp == ch)
            return (TRUE);
    return (FALSE);
}

int
validate_obj_in_objlist(OBJ_DATA * obj)
{
    OBJ_DATA *tmp;
    char message[256];

    if (obj->nr == 0) {         /* this is ok */
    } else if (obj->nr < top_of_obj_t) {        /* this is ok */
    } else {
        snprintf(message, sizeof(message),
                 "Found an object with real number %d, top_of_obj_t == %d", obj->nr, top_of_obj_t);
        gamelog(message);
    }



    for (tmp = object_list; tmp != 0; tmp = tmp->next)
        if (tmp == obj)
            return (TRUE);
    return (FALSE);
}

char real_name_buf[2000];
int
validate_char_list_in_rooms_list(void)
{
    CHAR_DATA *tmp;
    int ret;

    ret = TRUE;
    for (tmp = character_list; tmp != 0; tmp = tmp->next)
        if (FALSE == validate_room_in_roomlist(tmp->in_room)) {
            errorlogf("character was in the character list, the room they are in is not in the room list");
            ret = FALSE;
        }
    return (ret);
}

int
validate_chars_in_rooms_in_charlist(void)
{
    struct room_data *tmp;
    CHAR_DATA *ch;
    int ret;

    ret = TRUE;
    for (tmp = room_list; tmp != 0; tmp = tmp->next)
        for (ch = tmp->people; ch; ch = ch->next_in_room)
            if (FALSE == validate_char_in_charlist(ch)) {
                errorlogf("char was in room, but not in the character list");
                ret = FALSE;
            }
    return (ret);
}

int
validate_obj_in_appropriate_list(OBJ_DATA * obj)
{
    OBJ_DATA *t;
    int i, count, rval;

    rval = TRUE;
    count = 0;

    if (obj->in_room) {
        count++;
        for (t = obj->in_room->contents; t && (t != obj); t = t->next_content) {        /* do nothing, march through list till found */
        };
        if (t == 0) {
            errorlog("an item was in room, but wasnt in the rooms list.");
            rval = FALSE;
        }
    }
    if (obj->in_obj) {
        count++;
        for (t = obj->in_obj->contains; t && (t != obj); t = t->next_content) { /* do nothing, march through list till found */
        };
        if (t == 0) {
            errorlog("an Item in obj, but wasnt in its contents list.");
            rval = FALSE;
        }
    }

    if (obj->carried_by) {
        count++;
        for (t = obj->carried_by->carrying; t && (t != obj); t = t->next_content) {     /* do nothing, march through list till found */
        };
        if (t == 0) {
            errorlog("An Item in char, but wasnt in its carrying list.");
            rval = FALSE;
        }

    }
    if (obj->equipped_by) {
        count++;
        for (i = 0; (i < MAX_WEAR) && (obj->equipped_by->equipment[i] != obj); i++) {   /* do nothing, march through list till found */
        };
        if (i == MAX_WEAR) {
            errorlog("An object was equiped and not in equipped_by");
            rval = FALSE;
        }
    }
    if (count == 0) {
        rval = FALSE;
    }
    if (count > 1) {
        errorlog("An Item is in more than one list.");
        rval = FALSE;
    }
    return (rval);
}

int
validate_objects_in_lists(void)
{
    OBJ_DATA *obj;
    CHAR_DATA *ch;
    char message[1000];
    int i, rval;
    /*
     * OBJ_DATA *test;
     * struct room_data *room;
     */

    rval = TRUE;
    for (obj = object_list; obj; obj = obj->next)
        if (!validate_obj_in_appropriate_list(obj))
            rval = FALSE;

    for (ch = character_list; ch != 0; ch = ch->next) {
        /* check there equipment */
        for (i = 0; (i < MAX_WEAR); i++) {
            if (ch->equipment[i]) {
                if (ch->equipment[i]->equipped_by != ch) {
                    snprintf(message, sizeof(message), "(ch->equipment[i]->equipped_by != ch )");
                    gamelog(message);
                    rval = FALSE;
                }
            }
        }
    }
    return (rval);
}

int
check_db(void)
{
    int rval;
    rval = TRUE;
    if (!validate_char_list_in_rooms_list())
        rval = FALSE;
    if (!validate_chars_in_rooms_in_charlist())
        rval = FALSE;
    if (!validate_objects_in_lists())
        rval = FALSE;

    return (rval);
}

const char *
xformat_string(const char *str, char *fmted, size_t fmtedlen)
{
    char buf[256];
    const char *nexttok;
    size_t linelen, toklen;

    assert(fmtedlen > 0);
    *fmted = '\0';
    linelen = 0;
    nexttok = str;
    do {
        nexttok = one_argument(nexttok, buf, ARRAY_LENGTH(buf));
        if (!*buf)
            break;
        toklen = strlen(buf);

        if (linelen + toklen > 77) {
            strncat(fmted, "\n\r", fmtedlen);
            linelen = 0;
        }

        strncat(fmted, buf, fmtedlen);
        strncat(fmted, " ", fmtedlen);
        linelen += toklen + 1;

        // English spacing
        if (linelen && (buf[toklen - 1] == '?' || buf[toklen - 1] == '.' ||
                        buf[toklen - 1] == '!')) {
            strncat(fmted, "  ", fmtedlen);
            linelen += 2;
        }
    } while (nexttok);
    return fmted;
}

char *
format_string(char *oldstring)
{
    char xbuf[MAX_STRING_LENGTH];
    char xbuf2[MAX_STRING_LENGTH];
    char *rdesc;
    int i = 0;
    bool cap = TRUE;

    if (oldstring == NULL)
        return NULL;

    xbuf[0] = xbuf2[0] = 0;

    i = 0;

    for (rdesc = oldstring; *rdesc; rdesc++) {
        if (*rdesc == '\n') {
            if (i > 0 && xbuf[i - 1] != ' ') {
                xbuf[i] = ' ';
                i++;
            }
        } else if (*rdesc == '\r')
            ;
        else if (*rdesc == ')') {
            if (i > 0 && xbuf[i - 1] == ' ' && xbuf[i - 2] == ' '
                && (xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!')) {
                xbuf[i - 2] = *rdesc;
                xbuf[i - 1] = ' ';
                xbuf[i] = ' ';
                i++;
            } else {
                xbuf[i] = *rdesc;
                i++;
            }
        } else if (*rdesc == '.' || *rdesc == '?' || *rdesc == '!') {
            if (i > 0 && xbuf[i - 1] == ' ' && xbuf[i - 2] == ' '
                && (xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!')) {
                xbuf[i - 2] = *rdesc;
                if (i > 0 && *(rdesc + 1) != '\"') {
                    xbuf[i - 1] = ' ';
                    xbuf[i] = ' ';
                    i++;
                } else if (i > 0) {
                    xbuf[i - 1] = '\"';
                    xbuf[i] = ' ';
                    xbuf[i + 1] = ' ';
                    i += 2;
                    rdesc++;
                }
            } else if (*(rdesc + 1) == ' ' && *(rdesc + 2) == ' ') {
                xbuf[i] = *rdesc;
                i++;
            } else if (*(rdesc + 1) == ' ' && *(rdesc + 2) != ' ') {
                xbuf[i] = *rdesc;
                xbuf[i + 1] = ' ';
                i += 2;
            } else {
                xbuf[i] = *rdesc;
                if (*(rdesc + 1) != '\"') {
                    xbuf[i + 1] = ' ';
                    xbuf[i + 2] = ' ';
                    i += 3;
                } else {
                    xbuf[i + 1] = '\"';
                    xbuf[i + 2] = ' ';
                    xbuf[i + 3] = ' ';
                    i += 4;
                    rdesc++;
                }
            }
            cap = TRUE;
        } else {
            xbuf[i] = *rdesc;
            if (cap && *rdesc != ' ') {
                cap = FALSE;
                xbuf[i] = UPPER(xbuf[i]);
            }
            i++;
        }
    }
    xbuf[i] = 0;
    strcpy(xbuf2, xbuf);

    rdesc = xbuf2;

    xbuf[0] = 0;

    for (;;) {
        for (i = 0; i < 77; i++) {
            if (!*(rdesc + i))
                break;
        }
        if (i < 77) {
            break;
        }
        for (i = (xbuf[0] ? 76 : 73); i; i--) {
            if (*(rdesc + i) == ' ')
                break;
        }
        if (i) {
            *(rdesc + i) = 0;
            strcat(xbuf, rdesc);
            strcat(xbuf, "\n\r");
            rdesc += i + 1;
            while (*rdesc == ' ')
                rdesc++;
        } else {
            gamelog("format_string:: No spaces");
            *(rdesc + 75) = 0;
            strcat(xbuf, rdesc);
            strcat(xbuf, "-\n\r");
            rdesc += 76;
        }
    }

    while (*(rdesc + i)
           && (*(rdesc + i) == ' ' || *(rdesc + i) == '\n' || *(rdesc + i) == '\r'))
        i--;

    *(rdesc + i + 1) = 0;
    strcat(xbuf, rdesc);

    size_t len = strlen(xbuf);
    if (len > 1 && xbuf[len - 2] != '\n')
        strcat(xbuf, "\n\r");

    free(oldstring);
    return (strdup(xbuf));
}

/* new indented format string */
char *
indented_format_string(char *oldstring /*, bool fSpace */ )
{
    char xbuf[MAX_STRING_LENGTH];
    char xbuf2[MAX_STRING_LENGTH];
    char *rdesc;
    char *origstring = oldstring;
    int i = 0;
    bool cap = TRUE;

    if (oldstring == NULL)
        return NULL;

    xbuf[0] = xbuf2[0] = 0;

    xbuf[0] = xbuf[1] = xbuf[2] = ' ';
    i = 3;

    for (; *oldstring == ' ' || *oldstring == '\t'; oldstring++);

    for (rdesc = oldstring; *rdesc; rdesc++) {
        if (*rdesc == '\n') {
            if (xbuf[i - 1] != ' ') {
                xbuf[i] = ' ';
                i++;
            }

            /* user intended to start a new paragraph */
            if (isspace(*(rdesc + 2))) {
                cap = TRUE;
                xbuf[i] = '\n';
                xbuf[i + 1] = ' ';
                xbuf[i + 2] = ' ';
                xbuf[i + 3] = ' ';
                i += 4;
                while (*rdesc && isspace(*rdesc))
                    rdesc++;
                rdesc--;
            }
        } else if (*rdesc == '\r');
        else if (*rdesc == ')') {
            if (xbuf[i - 1] == ' ' && xbuf[i - 2] == ' '
                && (xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!')) {
                xbuf[i - 2] = *rdesc;
                xbuf[i - 1] = ' ';
                xbuf[i] = ' ';
                i++;
            } else {
                xbuf[i] = *rdesc;
                i++;
            }
        } else if (*rdesc == '.' || *rdesc == '?' || *rdesc == '!') {
            if (xbuf[i - 1] == ' ' && xbuf[i - 2] == ' '
                && (xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!')) {
                xbuf[i - 2] = *rdesc;
                if (*(rdesc + 1) != '\"') {
                    xbuf[i - 1] = ' ';
                    xbuf[i] = ' ';
                    i++;
                } else {
                    xbuf[i - 1] = '\"';
                    xbuf[i] = ' ';
                    xbuf[i + 1] = ' ';
                    i += 2;
                    rdesc++;
                }
            } else if (*(rdesc + 1) == ' ' && *(rdesc + 2) == ' ') {
                xbuf[i] = *rdesc;
                i++;
            } else if (*(rdesc + 1) == ' ' && *(rdesc + 2) != ' ') {
                xbuf[i] = *rdesc;
                xbuf[i + 1] = ' ';
                i += 2;
            } else {
                xbuf[i] = *rdesc;
                if (*(rdesc + 1) != '\"') {
                    xbuf[i + 1] = ' ';
                    xbuf[i + 2] = ' ';
                    i += 3;
                } else {
                    xbuf[i + 1] = '\"';
                    xbuf[i + 2] = ' ';
                    xbuf[i + 3] = ' ';
                    i += 4;
                    rdesc++;
                }
            }
            cap = TRUE;
        } else {
            xbuf[i] = *rdesc;
            if (cap && *rdesc != ' ') {
                cap = FALSE;
                xbuf[i] = UPPER(xbuf[i]);
            }
            i++;
        }
    }
    xbuf[i] = 0;
    strcpy(xbuf2, xbuf);

    rdesc = xbuf2;

    xbuf[0] = 0;

    for (;;) {
        char *newline = NULL;

        for (i = 0; i < 77; i++) {
            if (!*(rdesc + i))
                break;
            if (*(rdesc + i) == '\n') {
                /* if newline is first character, skip over it and move on */
                if (i == 0) {
                    rdesc++;
                    i--;
                    continue;
                }
                newline = rdesc + i;
            }
        }
        if (i < 77) {
            break;
        }

        /* new paragraph */
        if (newline != NULL) {
            i = newline - rdesc;
        } else {
            for (i = (xbuf[0] ? 76 : 73); i; i--) {
                if (isspace(*(rdesc + i)))
                    break;
            }
        }

        if (i) {
            char oldSpace = *(rdesc + i);
            *(rdesc + i) = 0;
            strcat(xbuf, rdesc);
            strcat(xbuf, "\n\r");
            rdesc += i + 1;

            if (oldSpace != '\n')
                while (*rdesc == ' ')
                    rdesc++;
        } else {
            gamelog("format_string:: No spaces");
            *(rdesc + 75) = 0;
            strcat(xbuf, rdesc);
            strcat(xbuf, "-\n\r");
            rdesc += 76;
        }
    }

    while (*(rdesc + i)
           && (*(rdesc + i) == ' ' || *(rdesc + i) == '\n' || *(rdesc + i) == '\r'))
        i--;

    *(rdesc + i + 1) = 0;
    strcat(xbuf, rdesc);

    if (xbuf[strlen(xbuf) - 2] != '\n')
        strcat(xbuf, "\n\r");

    free(origstring);
    return (strdup(xbuf));
}


/*
 * Returns an initial-lower string if the first is an article, otherwise it
 * leaves it alone.
 * 3/18/98 -Morg
 */
char *
lower_article_first(const char *str)
{
    static char strlower[MAX_STRING_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    strcpy(strlower, str);
    str = one_argument(str, arg2, sizeof(arg2));

    if (!strcasecmp(arg2, "a") || !strcasecmp(arg2, "an")
        || !strcasecmp(arg2, "the"))
        strlower[0] = LOWER(strlower[0]);

    return strlower;
}


/*
 * Sees if last char is 's' and returns 'is' or 'are' pending.
 */
char *
is_are(char *text)
{
    static char buf[MAX_STRING_LENGTH];
    char *v;

    strcpy(buf, text);
    v = strstr(buf, " of ");
    if (v == NULL) {
        if (((buf[strlen(buf) - 1] == 's' || buf[strlen(buf) - 1] == 'S')
             && buf[strlen(buf) - 2] != 's' && buf[strlen(buf) - 2] != 'S')
            || strstr(buf, "pair"))
            return "are";
        else
            return "is";
    } else {
        buf[strlen(buf) - strlen(v)] = '\0';
        if (((buf[strlen(buf) - 1] == 's' || buf[strlen(buf) - 1] == 'S')
             && buf[strlen(buf) - 2] != 's' && buf[strlen(buf) - 2] != 'S')
            || strstr(buf, "pair"))
            return "are";
        else
            return "is";
    }
}

/*
 * Trim the whitespace from the end of a string
 */
char *
trim_string(char *text)
{
    char *v;

    if (text == NULL)
        return (text);

    for (v = text + strlen(text); isspace(*v) && v >= text; v--);

    *v = '\0';

    return (text);
}


bool
is_lawful_humanoid(CHAR_DATA * ch)
{
    switch (GET_RACE(ch)) {
    case RACE_HUMAN:
    case RACE_HALF_ELF:
    case RACE_ELF:
    case RACE_DESERT_ELF:
    case RACE_MUL:
    case RACE_DWARF:
    case RACE_HALF_GIANT:
        return TRUE;
    }
    return FALSE;
}


char *
generate_keywords(CHAR_DATA * ch)
{
    return generate_keywords_from_string(ch->player.extkwds);
}

char *
generate_keywords_from_string(const char * str)
{
    char *ptr;
    char *ptr2;

    if (!str)
        return NULL;

    /* initialize to provided str */
    ptr = strdup(str);

    /* get rid of the article */
    ptr2 = strdup(smash_article(ptr));
    free(ptr);
    ptr = ptr2;

    /* get rid of 'and' */
    ptr2 = strdup(smash_word(ptr, "and"));
    free(ptr);
    ptr = ptr2;

    /* get rid of commas */
    ptr = smash_char(ptr, ',');
    return (ptr);
}

void
add_keywords(CHAR_DATA * ch, char *words)
{
    static char buf[MAX_STRING_LENGTH];
    char word[MAX_STRING_LENGTH];
    char *ptr;
    const char *w;

    // generate keywords based on sdesc
    ptr = generate_keywords(ch);

    // store new keywords on buf
    snprintf(buf, sizeof(buf), "%s", ptr);

    // free up memory from the generated keywords
    free(ptr);

    // need a pointer into the submited keywords
    w = words;
    do {
        // grab the next keyword
        w = one_argument(w, word, sizeof(word));
        // make sure it's not an existing keyword, and not their name
        if (*word && !isname(word, buf) && stricmp(word, ch->name)) {
            // add a blank space
            strncat(buf, " ", sizeof(buf));
            // it's ok, add it
            strncat(buf, word, sizeof(buf));
        }
        // while we still have words
    } while (*word);

    // free up the memory that was sent to us
    free(words);

    // if they had keywords, free them
    if (ch->player.extkwds != NULL)
        free(ch->player.extkwds);

    // duplicate the new keywords
    ch->player.extkwds = strdup(buf);
    return;
}

bool
is_charmed(CHAR_DATA *ch) {
  if (IS_SET(ch->specials.affected_by, CHAR_AFF_CHARM) &&
      ch->master) {
    return TRUE;
  }
  
  return FALSE;
}

bool
is_paralyzed(CHAR_DATA * ch)
{
    /* peraine poison or paralyze */
    if (affected_by_spell(ch, POISON_PERAINE)
        || affected_by_spell(ch, SPELL_PARALYZE))
        return 1;

    /* npc & frozen - sparring dummies */
    if (IS_NPC(ch) && IS_SET(ch->specials.act, CFL_FROZEN))
        return 1;

    return 0;
}

int
is_ch_in_clan_list( CHAR_DATA *ch, const char *clans ) {
    char arg[MAX_INPUT_LENGTH];
    int i = 0;
    int argi = 0;
    int len = strlen(clans);
    int clan;

    /* while there's still text in the edesc */
    while (clans[i] != '\0') {
        /* reset the current arg */
        argi = 0;
        arg[0] = '\0';

        for (; i < len; i++) {
            /* watch for white space at the start of a new arg */
            if (argi == 0 && clans[i] == ' ') {
                continue;
            }

            /* if it's a space, see if arg is a number */
            if (isspace(clans[i]) && is_number(arg)) {
                /* convert it to an int */
                clan = atoi(arg);

                /* make sure the int is in range & they are that clan */
                if (clan > 0 && clan < MAX_CLAN && IS_TRIBE(ch, clan)) {
                    return TRUE;
                }
                i++;
                break;
            }

            /* watch for the separator */
            if (clans[i] == ',' || clans[i] == '\n' || clans[i] == '\r') {
                if (argi > 0) {
                    if( is_number(arg) ) {
                        /* convert it to an int */
                        clan = atoi(arg);

                        /* make sure the int is in range & they are that clan */
                        if (clan > 0 && clan < MAX_CLAN && IS_TRIBE(ch, clan)) {
                            return TRUE;
                        }
                    }
                    else {
                        /* find the matching clan */
                        clan = lookup_clan_by_name(arg);

                        /* if the name matched, and in that tribe, no crim */
                        if (clan != -1 && IS_TRIBE(ch, clan)) {
                            return TRUE;
                        }
                    }
                }

                /* skip ahead to the next character, and exit the for loop */
                i++;
                break;
            }
            /* otherwise, good, save it to arg */
            else {
                arg[argi] = clans[i];
                argi++;
                /* keep it null terminated */
                arg[argi] = '\0';
            }
        }
    }

    // was there something left?
    if (argi > 0) {
        if( is_number( arg ) ) {
            clan = atoi(arg);

            /* make sure the int is in range & they are that clan */
            if (clan > 0 && clan < MAX_CLAN && IS_TRIBE(ch, clan)) {
                return TRUE;
            }
        } else {
            /* find the matching clan and see if ch is in that clan */
            clan = lookup_clan_by_name(arg);

            /* if the name matched, and they are in that tribe, ok */
            if (clan != -1 && IS_TRIBE(ch, clan)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void 
set_hit(CHAR_DATA *ch, int newval) {
    newval = MAX(-11, MIN(GET_MAX_HIT(ch), newval));

    if( ch->points.hit == newval )
       return;

    ch->points.hit = newval;
    adjust_ch_infobar(ch, GINFOBAR_HP);
}

void 
adjust_hit(CHAR_DATA *ch, int amount) {
    set_hit(ch, ch->points.hit + amount);
}

void 
set_mana(CHAR_DATA *ch, int newval) {
    newval = MAX(0, MIN(GET_MAX_MANA(ch), newval));

    if( ch->points.mana == newval )
       return;

    ch->points.mana = newval;
    adjust_ch_infobar(ch, GINFOBAR_MANA);
}

void 
adjust_mana(CHAR_DATA *ch, int amount) {
    set_mana(ch, ch->points.mana + amount);
}

void
set_move(CHAR_DATA *ch, int newval) {
    newval = MAX(0, MIN(GET_MAX_MOVE(ch), newval));

    if( ch->points.move == newval )
       return;

    ch->points.move = newval;
    adjust_ch_infobar(ch, GINFOBAR_STAM);
}

void
adjust_move(CHAR_DATA *ch, int amount) {
    set_move(ch, ch->points.move + amount);
}

void 
set_stun(CHAR_DATA *ch, int newval) {
    newval = MAX(0, MIN(GET_MAX_STUN(ch), newval));

    if( ch->points.stun == newval )
       return;

    ch->points.stun = newval;
    adjust_ch_infobar(ch, GINFOBAR_STUN);
}

void 
adjust_stun(CHAR_DATA *ch, int amount) {
    set_stun(ch, ch->points.stun + amount);
}

void
touchfile(const char *filename, const char *content)
{
    FILE *outfile;

    outfile = fopen(filename, "w");
    if (outfile) {
        fprintf(outfile, "%s", content);
        fclose(outfile);
    }
}

void
appendfile(const char *filename, const char *content)
{
    FILE *outfile;

    outfile = fopen(filename, "a");
    if (outfile) {
        fprintf(outfile, "%s", content);
        fclose(outfile);
    }
}

void
remfile(const char *filename)
{
    unlink(filename);
}

bool
is_following(CHAR_DATA *follower, CHAR_DATA *leader)
{
    struct follow_type *f = leader->followers;

    while (f) {
        if (f->follower == follower)
            return TRUE;
        f = f->next;
    }

    return FALSE;
}

int
is_cloak(CHAR_DATA *ch, OBJ_DATA *obj) {
   if (ch == NULL || obj == NULL)
       return 0;

   if (ch->equipment[WEAR_ABOUT] != obj)
       return 0;

   if ((obj->obj_flags.type == ITEM_WORN
    || obj->obj_flags.type == ITEM_CONTAINER)
    /* with cloak set */
    && IS_SET(obj->obj_flags.value[1], CONT_CLOAK))
      return 1;

   return 0;
}

int
is_cloak_open(CHAR_DATA *ch, OBJ_DATA *obj) {
   if (obj == NULL)
       return 0;

   // make sure it's a cloak
   if (!is_cloak(obj->equipped_by, obj)) return 0;

   // if they can't see the object must be invis, so open
   if (ch != NULL && !CAN_SEE_OBJ(ch, obj)) return 1;

   // if the open bit is set, the cloak is open
   return !IS_SET(obj->obj_flags.value[1], CONT_CLOSED);
}

int
is_closed_container(CHAR_DATA *ch, OBJ_DATA *obj) {
    if (ch == NULL || obj == NULL)
        return 0;

    if (GET_ITEM_TYPE(obj) != ITEM_CONTAINER) 
        return 0;

    if (IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
        return 1;

    return 0;
}

void
show_drink_container_fullness(CHAR_DATA *ch, OBJ_DATA *obj) {
   int temp, tempcolor;
   if (obj->obj_flags.value[1] <= 0) {
      cprintf(ch, "It is empty.\n\r");
   } else {
       if (obj->obj_flags.value[0] != 0)
           temp =
               (((obj->obj_flags.value[1]) * 3) /
                (obj->obj_flags.value[0]));
       else
           temp = 3;
       temp = MAX(0, MIN(3, temp));
       tempcolor = obj->obj_flags.value[2];
       tempcolor = MAX(0, MIN((MAX_LIQ - 1), tempcolor));

       cprintf(ch, "It's %sfull of %s%s liquid.\n\r", fullness[temp],
        indefinite_article(color_liquid[tempcolor]), color_liquid[tempcolor]);
   }
}

