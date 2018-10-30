/*
 * File: MAGICK.C
 * Usage: Routines for magickal spells.
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
 * 05/15/2000 Paralyze spell no longer affects immortals.  -San
 * 05/15/2000 Added check in Hands of Wind to wake characters that
 *            are sleeping when they are victims of the spell. -Gleden
 * 05/05/2000 Commented out debug stuff in solace spell, since it was
 *            appearing to mortals.  -San
 * 04/20/2000 Can now cast levitate on objects.  -San
 * 08/13/2000 Can no longer repel ppl through closed doors.  -San
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "constants.h"
#include "structs.h"
#include "creation.h"
#include "parser.h"
#include "cities.h"
#include "db.h"
#include "utils.h"
#include "comm.h"
#include "skills.h"
#include "handler.h"
#include "limits.h"
#include "guilds.h"
#include "event.h"
#include "modify.h"
#include "psionics.h"
#include "utility.h"
#include "object_list.h"
#include "player_accounts.h"
#include "memory.h"
#include "watch.h"
#include "pather.h"
#include "reporting.h"
#include "utility.h"
#include "info.h"
#include "special.h"
#include "wagon_save.h"
#include "clan.h"

extern void sflag_to_char(CHAR_DATA * ch, long sflag);
extern void sflag_to_obj(OBJ_DATA * obj, long sflag);
extern void spell_repel(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                        OBJ_DATA * obj);
extern void spell_slow(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                       OBJ_DATA * obj);
extern int is_enough_sand(ROOM_DATA * rm);
extern int blade_barrier_check(CHAR_DATA * mover, int dir);
extern int thorn_check(CHAR_DATA * mover, int dir);
extern int wall_of_fire_check(CHAR_DATA * mover, int dir);
extern OBJ_DATA *find_wagon_for_room( ROOM_DATA *room );


/* Local variables */
int is_soldier_in_city(CHAR_DATA * ch);
int find_nilaz_chest(CHAR_DATA * ch);
int templar_protected(CHAR_DATA * caster, CHAR_DATA * templar, int spell, int level);
int allanak_magic_barrier(CHAR_DATA * caster, ROOM_DATA * target_room);
int tuluk_magic_barrier(CHAR_DATA * caster, ROOM_DATA * target_room);

#define PORTAL_OBJ 73001
#define SHELTER_OBJ 73002
#define ARROW(dir)  real_object(2801+(dir))

void
stack_spell_affect(CHAR_DATA *ch, struct affected_type *af, int max)
{
    struct affected_type *tmp_af;

    for (tmp_af = ch->affected; tmp_af; tmp_af = tmp_af->next) {
        if (tmp_af->type == af->type) {
            af->duration += tmp_af->duration;
            af->power = MAX(tmp_af->power, af->power);
        }
    }

    af->duration = MIN(max, af->duration);

    while (affected_by_spell(ch, af->type))
        affect_from_char(ch, af->type);

    affect_to_char(ch, af);
}

bool
wearing_allanak_black_gem(const CHAR_DATA * ch)
{
    if (ch->equipment[WEAR_NECK]
        && !strcmp(OSTR(ch->equipment[WEAR_NECK], short_descr), "a dull black gem"))
        return TRUE;

    if (ch->equipment[WEAR_ABOUT_THROAT]
        && !strcmp(OSTR(ch->equipment[WEAR_ABOUT_THROAT], short_descr), "a dull black gem"))
        return TRUE;

    return FALSE;
}

/* use for all non-offensive spells */
/* This is checking to see if people casting non-offensive spells
   within Allanak are wearing "a dull black gem".  These gems are
   produced by the templarate to enslave the elementalists */
/* For aggressive spells, use check_criminal_flag directly
 * -Raesanos 4/5/2005
 */
void
check_crim_cast(CHAR_DATA * ch)
{
    /* If not in a 'magick ok' room */
    if (!(IS_SET(ch->in_room->room_flags, RFL_MAGICK_OK))) {
        /* if you're in Allanak */
        if (ch->in_room->city == CITY_ALLANAK) {
            /* but not wearing the gem */
            if (!wearing_allanak_black_gem(ch)) {
                check_criminal_flag(ch, NULL);
            }
        } else {                /* not in Allanak */
            check_criminal_flag(ch, NULL);
        }
    }

    return;
}

/* This function will return 1 if the character is a soldier and
 * is in the city-state of their highlord.  Returns 0 if not.
 * Added 4/8/15 by Raesanos
 */

int
is_soldier_in_city(CHAR_DATA * ch)
{
    int ct;
    for (ct = 0; ct < MAX_CITIES; ct++)
        if (IS_TRIBE(ch, city[ct].tribe))
            break;
    if (ct == MAX_CITIES)
        return 0;               /* Not a soldier for any city */
    if (room_in_city(ch->in_room) != ct)
        return 0;               /* Outside their city */
    return 1;                   /* They are a soldier in their city-state. */
}


/* Following function will return 1 if the character is a templar and is */
/* within the city-state of their highlord.  Returns 0 if not. */
int
is_templar_in_city(CHAR_DATA * ch)
{
    int ct;
    /*char bug[MAX_STRING_LENGTH]; */

    if ((GET_GUILD(ch) != GUILD_TEMPLAR) && (GET_GUILD(ch) != GUILD_LIRATHU_TEMPLAR)
        && (GET_GUILD(ch) != GUILD_JIHAE_TEMPLAR))
        return FALSE;

    for (ct = 0; ct < MAX_CITIES; ct++)
        if (IS_TRIBE(ch, city[ct].tribe))
            //      if (city[ct].tribe == ch->player.tribe)
            break;

    if (ct == MAX_CITIES)
        return 0;               /* No sorcerer-king to protect them */

    if (room_in_city(ch->in_room) != ct)
        return 0;               /* Outside the protection of their sorcerer-king */

    return 1;                   /* They are a templar in their city-state.  */
}


void
set_creator_edesc(CHAR_DATA *ch, OBJ_DATA *obj) {
  char creator_name[MAX_STRING_LENGTH];

  sprintf(creator_name, "[CREATED_BY_%s_(%s)]", ch->name, ch->account);

  if (!IS_NPC(ch))
    sprintf(creator_name, "[CREATED_BY_%s_(%s)]", ch->name, ch->account);
  else {
    if (!ch->desc)
      sprintf(creator_name, "[CREATED_BY__m%d]", ch->nr);
    else
      if (ch->desc->original)
        sprintf(creator_name, "[CREATED_BY_%s_(%s)_while_playing_m%d]", ch->desc->original->name, ch->desc->original->account, ch->nr);
  }

  set_obj_extra_desc_value (obj, creator_name,  "");
}

void
remove_alarm_spell(ROOM_DATA *room) {
   char buf[MAX_STRING_LENGTH];

   if (IS_SET(room->room_flags, RFL_SPL_ALARM))
      REMOVE_BIT(room->room_flags, RFL_SPL_ALARM);
   if (TRUE == get_room_extra_desc_value(room, "[ALARM_SPL_USES]", buf, MAX_STRING_LENGTH))
      rem_room_extra_desc_value(room, "[ALARM_SPL_USES]");
   if (TRUE == get_room_extra_desc_value(room, "[ALARM_SPL_PC_info]", buf, MAX_STRING_LENGTH))
      rem_room_extra_desc_value(room, "[ALARM_SPL_PC_info]");
   if (TRUE == get_room_extra_desc_value(room, "[ALARM_SPL_ACCOUNT_info]", buf, MAX_STRING_LENGTH))
      rem_room_extra_desc_value(room, "[ALARM_SPL_ACCOUNT_info]");
}
/* This is a template, for using when adding new spells.  */

/* COMMENT BLOCK: Should contain a brief description of the 
 * spell, any restrictions, any notes about duration or affect,
 * if a component is required, which guilds get it, and the
 * spell words.
 */
void
spell_template(byte level,      /* level cast at       */
               sh_int magick_type,      /* type of magick used */
               CHAR_DATA * ch,  /* caster              */
               CHAR_DATA * victim,      /* target character    */
               OBJ_DATA * obj)
{                               /* target object       *//* begin Spell_Name_Of_Spell */

/* int duration;   If you want duration to vary according to      */
/*                 various conditions.                            */
/* int sector;     If you want to use sector to determine affects */
/*                 or messages.                                   */

/* struct affected_type af;        If affects will be applied.        */
/* struct affected_type *tmp_af;   If you want to make it cumulative. */

/* If a component is required */
    if (!get_componentB(ch, SPHERE_ALTERATION,  /* Sphere required     */
                        level,  /* level cast at       */
                        "Message to caster when $p is consumed.",       /* $p is the component */
                        "Message to room when $n uses $p.",     /* $n is the caster    */
                        "Message to caster if component not sufficient."))
        return;

    check_crim_cast(ch);        /* this is what flags people for casting when they shouldn't */

    /* various spell checks    */
    /* various spell functions */

/* Note: if you want to make it cumulative, and for the message to vary
 * according to whether or no they have the spell already cast on them,
 * put this check before the affect is applied.  The structure I have 
 * been following is this: checks for other spells that affect/negate
 * the spell; checks for sectors or room flags that affect/negate the
 * spell; check for whether they already have the spell on them and any
 * specific message as a result of that, and then a switch (sector) that
 * differentiates between messages and effects.  For good examples of
 * these, spell fire_armor, spell_guardian, and spell god_speed will 
 * all serve.   -Sanvean
 */
    act("Message to room when spell is cast by $n.",    /* $n is the caster */
        FALSE, ch, 0, 0, TO_ROOM);
    act("Message to caster when spell is cast.", FALSE, ch, 0, 0, TO_CHAR);
    act("To the $D, message to nearby rooms when the spell is cast.",   /* $D is direction */
        FALSE, ch, 0, 0, TO_NEARBY_ROOMS);

    /* various spell affects and events */

}                               /* end Spell_Name_Of_Spell */

/* Begin actual spells.  */

/* ALARM: Sets a spell on the room that is triggered when someone
 * (even the caster) enters it.  Alarms can be dispelled with dispel
 * magick; otherwise they vanish only when triggered (or, of course,
 * the game resetting).  Cannot be cast in rooms sectored NILAZ_PLANE
 * or flagged NO_ELEM_MAGICK.  Specialized templar casting message.
 * Messages for casters vary according to caster.  Rukkian spell. 
 * Rukkian spell.  Ruk mutur inrof.
 * Note: need to add special effects for rooms flagged sandstorm and
 * ash.
 */
void
spell_alarm(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Alarm */

    int sector;
    char buf[MAX_STRING_LENGTH];
    char ename[MAX_STRING_LENGTH], edesc[MAX_STRING_LENGTH];

    /* Cannot be cast if there is already an alarm set on room */
    if (IS_SET(ch->in_room->room_flags, RFL_SPL_ALARM)) {
        act("An alarm is already cast upon this area.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* Cannot be cast in rooms on Plane of Nilaz or in the air */
    if (((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))
            || (ch->in_room->sector_type == SECT_AIR)) {
        act("You cannot touch Ruk in this place.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* Ash in the room interferes with the spell. */
    if (IS_SET(ch->in_room->room_flags, RFL_ASH)) {
        act("Swirling ash clouds your vision, hampering the spell.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    check_crim_cast(ch);

    if (IS_NPC(ch)) {
        sprintf(buf, "%d", npc_index[ch->nr].vnum);
        set_room_extra_desc_value(ch->in_room, "[ALARM_SPL_PC_info]", buf);
    } else {
        set_room_extra_desc_value(ch->in_room, "[ALARM_SPL_PC_info]", MSTR(ch, name));
        set_room_extra_desc_value(ch->in_room, "[ALARM_SPL_ACCOUNT_info]", ch->account);
    }

    MUD_SET_BIT(ch->in_room->room_flags, RFL_SPL_ALARM);

    strcpy(ename, "[ALARM_SPL_USES]");
    sprintf(edesc, "%d", level);
    set_room_extra_desc_value(ch->in_room, ename, edesc);

    /* Specialized message for the templars */
    if ((GET_GUILD(ch) == GUILD_TEMPLAR) || (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
        || (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)) {
        act("Drawing on arcane energies, you ward this place "
            "with your Highlord's watchful eyes.", FALSE, ch, 0, 0, TO_CHAR);
        act("Raising $s fists upwards, $n chants softly as the " "ground shudders underfoot.",
            FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    /* Adding a message in for room flag SANDSTORM. */

    if (IS_SET(ch->in_room->room_flags, RFL_SANDSTORM)) {
        act("The sand in the air swirls violently, then settles "
            "onto the ground in arcane patterns as you enchant the " "area with an alarm.", FALSE,
            ch, 0, 0, TO_CHAR);
        act("The sand in the air swirls violently, then settles "
            "onto the ground in arcane patterns as $n gestures.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    /* Switch so casting messages vary according to sector */
    sector = ch->in_room->sector_type;

    switch (sector) {
    case SECT_CAVERN:
        act("The stony walls of the cavern groan aloud " "as you enchant the area with an alarm.",
            FALSE, ch, 0, 0, TO_CHAR);
        act("The stony walls of the cavern groan aloud " "as $n gestures.", FALSE, ch, 0, 0,
            TO_ROOM);
        break;
    case SECT_DESERT:
        act("The sands shift underfoot, whispering against "
            "each other as you enchant the area with an alarm.", FALSE, ch, 0, 0, TO_CHAR);
        act("The sands shift underfoot, whispering against " "each other as $n gestures.", FALSE,
            ch, 0, 0, TO_ROOM);
        break;
    case SECT_EARTH_PLANE:
        act("The ground underfoot shudders violently as $n gestures, "
            "a glowing sigil appearing for a moment on it before fading.", FALSE, ch, 0, 0,
            TO_ROOM);
        act("The ground shudders violently and there is a flash of light to the $D", FALSE, ch, 0,
            0, TO_NEARBY_ROOMS);
        break;

    case SECT_FIELD:
        act("The stony ground beneath your feet shifts as " "you enchant the area with an alarm.",
            FALSE, ch, 0, 0, TO_CHAR);
        act("The stony ground beneath your feet shifts as " "$n gestures.", FALSE, ch, 0, 0,
            TO_ROOM);
        break;
    case SECT_HILLS:
        act("The stones underfoot shift and clatter against each "
            "other as you enchant the area with an alarm.", FALSE, ch, 0, 0, TO_CHAR);
        act("The stones underfoot shift and clatter against each " "other as $n gestures.", FALSE,
            ch, 0, 0, TO_ROOM);
        break;
    case SECT_LIGHT_FOREST:
    case SECT_HEAVY_FOREST:
        act("The vegetation rustles as the ground underfoot shifts "
            "and groans as you enchant the area with an alarm.", FALSE, ch, 0, 0, TO_CHAR);
        act("The vegetation rustles, the ground beneath your feet "
            "shifting and groaning as $n gestures.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case SECT_MOUNTAIN:
        act("The stony mountains reverberate as you enchant " "the area with an alarm.", FALSE, ch,
            0, 0, TO_CHAR);
        act("A deep rumbling comes from the roots of the " "mountains as $n gestures.", FALSE, ch,
            0, 0, TO_ROOM);
        break;
    case SECT_ROAD:
        act("The road underfoot groans aloud as you enchant " "its surface with an alarm.", FALSE,
            ch, 0, 0, TO_CHAR);
        act("The road underfoot groans aloud as $n gestures.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case SECT_RUINS:
        act("The rubble underfoot rustles and shifts as you enchant " "the area with an alarm.",
            FALSE, ch, 0, 0, TO_CHAR);
        act("The rubble underfoot rustles and shifts " "as $n gestures.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case SECT_SALT_FLATS:
        act("The gritty, salty ground underfoot shifts, sending up "
            "glints of light as you enchant the area with an alarm.", FALSE, ch, 0, 0, TO_CHAR);
        act("The gritty, salty ground underfoot shifts, sending up "
            "glints of light as $n gestures.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case SECT_SHALLOWS:
    case SECT_SILT:
        act("The silt underfoot roils and bubbles " "as you enchant the area with an alarm.", FALSE,
            ch, 0, 0, TO_CHAR);
        act("The silt underfoot roils and bubbles " "as $n gestures.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    case SECT_THORNLANDS:
        act("The ground beneath the thorny bushes shifts and "
            "murmurs as you enchant the area with an alarm.", FALSE, ch, 0, 0, TO_CHAR);
        act("The ground beneath the thorny bushes shifts and " "murmurs as $n gestures.", FALSE, ch,
            0, 0, TO_ROOM);
        break;
    default:
        act("You can hear Ruk groan as you enchant the area " "with an alarm.", FALSE, ch, 0, 0,
            TO_CHAR);
        act("A low groan comes from the ground itself as " "$n gestures.", FALSE, ch, 0, 0,
            TO_ROOM);
        break;
    }
}                               /* end Spell_Alarm */


CHAR_DATA *
raise_corpse_helper(OBJ_DATA *corpse, CHAR_DATA *ch, int magick_type, int level)
{
    int npcnumber = 100, r_num;
    CHAR_DATA *mob;
    char e_desc[512], e_account[512];
    struct affected_type af;
    memset(&af, 0, sizeof(af));

    ROOM_DATA *trm = ch->in_room;

    if (get_obj_extra_desc_value(corpse, "[PC_info]", e_desc, sizeof(e_desc))) {
        if (isdigit(e_desc[0])) {
            /* It's an NPC, go fetch from the DB */
            npcnumber = atoi(e_desc);
            if ((r_num = real_mobile(npcnumber)) < 0) {
                gamelogf("raising corpse: n%d not found.", npcnumber);
                return 0;
            }
            mob = read_mobile(r_num, REAL);
        } else {
            /* It's a PC, go read it in. */
            if (get_obj_extra_desc_value(corpse, "[ACCOUNT_info]", e_account, sizeof(e_account))) {
                global_game_stats.npcs++;
                CREATE(mob, CHAR_DATA, 1);
                clear_char(mob);
                mob->player.info[0] = e_account;
                load_char(e_desc, mob);
                MUD_SET_BIT(mob->specials.act, CFL_ISNPC);
                REMOVE_BIT(mob->specials.act, CFL_DEAD);
                add_to_character_list(mob);
                mob->specials.was_in_room = NULL;
            } else {
                /* Bogus player, get the default mobile */
                if ((r_num = real_mobile(npcnumber)) < 0) {
                    gamelogf("raising corpse: n%d not found.", npcnumber);
                    return 0;
                }
                mob = read_mobile(r_num, REAL);
            }
        }
    } else {
        if ((r_num = real_mobile(npcnumber)) < 0) {
            gamelogf("raising corpse: n%d not found.", npcnumber);
            return 0;
        }
        mob = read_mobile(r_num, REAL);
    }

    free_specials(mob->specials.programs);
    mob->specials.programs = NULL;
    char_to_room(mob, trm);

    IS_CARRYING_N(mob) = 0;

    /* smash_article added by Morg to shorten short description */
    char buf[512];
    sprintf(buf, "the undead %s", smash_article(OSTR(corpse, short_descr)));
    mob->short_descr = (char *) strdup(buf);

    if (!IS_SET(corpse->obj_flags.value[1], CONT_CORPSE_HEAD)) {
        sprintf(buf, "%s body undead", OSTR(corpse, name));
        mob->long_descr = (char *) strdup("slowly staggers around.\n\r");
        parse_command(mob, "emote starts to move and rises to its feet.");
    } else {
        sprintf(buf, "%s head undead", OSTR(corpse, name));
        mob->long_descr = (char *) strdup("slowly floats around.\n\r");
        parse_command(mob, "emote starts to move and slowly floats up from the ground.");
        af.type = SPELL_LEVITATE;
        af.duration = level + 10;
        af.modifier = 0;
        af.location = CHAR_APPLY_NONE;
        af.bitvector = CHAR_AFF_FLYING;
        affect_join(mob, &af, FALSE, FALSE);
    }
    mob->name = (char *) strdup(buf);

    set_char_extra_desc_value(mob, "[UNDYING]", "LUMP");

    OBJ_DATA *tmp, *next;
    for (tmp = corpse->contains; tmp; tmp = next) {
        next = tmp->next_content;
        obj_from_obj(tmp);
        obj_to_char(tmp, mob);
    }
    parse_command(mob, "wear all");

    mob->points.max_hit = dice((level + 1), 12);
    set_hit(mob, (int) (GET_MAX_HIT(mob) / 1.75));
    set_move(mob, GET_MAX_MOVE(mob));
    set_stun(mob, GET_MAX_STUN(mob));
    mob->player.sex = 0;

    MUD_SET_BIT(mob->specials.act, CFL_UNDEAD);
    MUD_SET_BIT(mob->specials.act, CFL_AGGRESSIVE);
    GET_RACE(mob) = corpse->obj_flags.value[3];

    delete_skill(mob, LANG_COMMON);
    delete_skill(mob, LANG_ELVISH);
    delete_skill(mob, LANG_NOMAD);
    delete_skill(mob, LANG_HESHRAK);
    delete_skill(mob, LANG_DWARVISH);
    delete_skill(mob, LANG_HALFLING);
    delete_skill(mob, LANG_MERCHANT);
    delete_skill(mob, LANG_ANCIENT);
    delete_skill(mob, LANG_GALITH);
    delete_skill(mob, LANG_MANTIS);
    delete_skill(mob, LANG_ANYAR);

    if (!has_skill(mob, LANG_VLORAN))
        init_skill(mob, LANG_VLORAN, 100);

    if (!has_skill(mob, LANG_NILAZ_ACCENT))
        init_skill(mob, LANG_NILAZ_ACCENT, 100);

    parse_command(mob, "change language vloran");
    parse_command(mob, "change accent nilazi");

    check_criminal_flag(mob, NULL);
    update_pos(mob);

    /* If they are in SECT_SILT, it messes with the magick. -San */
    if (mob->in_room->sector_type == SECT_SHALLOWS || mob->in_room->sector_type == SECT_SILT) {
        parse_command(mob, "emote stares blankly with silt-clouded eyes.");
    } else {
        af.type = SPELL_CHARM_PERSON;
        af.duration = level + 6;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = 0;
        af.location = 0;
        af.bitvector = CHAR_AFF_CHARM;
        affect_to_char(mob, &af);
    }

    af.type = SPELL_ANIMATE_DEAD;
    af.duration = level + 10;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_SUMMONED;
    affect_join(mob, &af, TRUE, FALSE);
    add_follower(mob, ch);

    if (!number(0,6))
        ch->specials.eco = MAX(-100, (ch->specials.eco - 1));
    return mob;
}

/* ANIMATE DEAD: When cast on a body, it will rise from the ground, 
 * alive again (or rather, undead).  The undead body will be similar 
 * to its living form, except not quite as strong and without a mind 
 * in any normal sense of the word.  The corpse will follow the 
 * caster's every command, without regard for its own well-being.
 * Nilazi and sorcerer spell.  Has a negative affect on the caster's
 * relationship to the Land.  Note that heads can be animated too.
 * Echoes a feeling of unease to surrounding rooms. Nilaz morz viod.
 */
void
spell_animate_dead(byte level, sh_int magick_type,      /* level of spell */
                   CHAR_DATA * ch,      /* the caster     */
                   CHAR_DATA * victim,  /* target char    */
                   OBJ_DATA * corpse)
{                               /* corpse item    *//* begin Spell_Animate_Dead */
    check_criminal_flag(ch, NULL);

    /*  This was coming up true all the time, even when cast on a corpse */
    if (!corpse) {
        gamelogf("A NULL corpse object got passed to %s", __FUNCTION__);
        return;
    }

    if ((GET_ITEM_TYPE(corpse) != ITEM_CONTAINER)
        || (!(corpse->obj_flags.value[3]))
        || (strstr(OSTR(corpse, short_descr), "the undead "))) {
        send_to_char("Your magick seems to not affect it.\n\r", ch);
        return;
    }

    act("You lean over $p and grow drenched with perspiration.", TRUE, ch, corpse, 0, TO_CHAR);
    act("$n becomes drenched with the sweat of effort as $e leans over $p.", TRUE, ch, corpse, 0, TO_ROOM);
    act("Your skin crawls for no apparent reason.", TRUE, ch, 0, 0, TO_NEARBY_ROOMS);

    raise_corpse_helper(corpse, ch, magick_type, level);

    extract_obj(corpse);
}                               /* end Spell_Animate_Dead */


/* ARMOR: Endows the caster with a layer of protection, whose
 * duration depends on power level.  Non-cumulative.  Visible
 * with detect magick.  Spell affects and message differ in
 * accordance with room sector. Rukkian and sorcerer spell.
 * Cumulative.  Ruk pret grol.
 */
void
spell_armor(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* Begin Spell_Armor */
    int sector;
    int duration;

    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in armor");
        return;
    }

    check_crim_cast(ch);

    if (affected_by_spell(victim, SPELL_ENERGY_SHIELD)) {
        send_to_char("The cloud of sparks around you flare, burning "
                     "away the dust trying to settle on your skin.\n\r", victim);
        act("The cloud of sparks around $N flare, burning away "
            "the dust and sand trying to settle on $S skin.", FALSE, ch, 0, victim, TO_NOTVICT);
        return;
    }
    if (affected_by_spell(victim, SPELL_SHIELD_OF_NILAZ)) {
        send_to_char("The magic sinks into the stillness around you, " "but refuses to adhere.\n\r",
                     victim);
        return;
    }

/* If cast in rooms sectored CAVERN, CITY, DESERT, LIGHT/HEAVY_FOREST,
 * MOUNTAIN, SALT_FLATS, SILT, OR THORNLANDS. it affects both the 
 * duration and spell messages received unless you are a templar.
 * The spell is cumulative, up to a max of 24 hours.  -Sanvean
 */

    sector = ch->in_room->sector_type;

    if (victim != ch)
        act("You empower $N with the protection of Ruk.", FALSE, ch, 0, victim, TO_CHAR);

    if ((GET_GUILD(ch) == GUILD_TEMPLAR) || (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
        || (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)) {
        send_to_char("Fine flakes of obsidian coalesce around you, "
                     "forming a layer of armor.\n\r", victim);
        act("Fine flakes of obsidian coalesce from the air around $N " "forming a layer of armor.",
            FALSE, ch, 0, victim, TO_NOTVICT);
        duration = level;
    } else
        switch (sector) {
        case SECT_CAVERN:
            send_to_char("The stony walls around you infuse you with their " "strength.\n\r",
                         victim);
            act("$N's skin begins to take on a greyish cast, reflecting "
                "the pallid color of the walls around $M.", FALSE, ch, 0, victim, TO_NOTVICT);
            duration = level * 2;
            break;
        case SECT_DESERT:
            send_to_char("The sand underfoot spirals up to fuse in a layer " "around you.\n\r",
                         victim);
            act("The sand under $N spirals up to fuse into " "a layer of armor.", FALSE, ch, 0,
                victim, TO_NOTVICT);
            duration = level * 2;
            break;
        case SECT_LIGHT_FOREST:
        case SECT_HEAVY_FOREST:
            send_to_char("The vegetation rustles, and earth from underfoot "
                         "rises to sift onto your skin in a thick layer.\n\r", victim);
            act("The vegetation rustles, and earth from underfoot rises to "
                "sift onto $N's skin in a protective layer.", FALSE, ch, 0, victim, TO_NOTVICT);
            duration = (level * 3) / 2;
            break;
        case SECT_MOUNTAIN:
            send_to_char("The mountains infuse you with their strength, "
                         "and a greyish cast creeps over your skin.\n\r", victim);
            act("A greyish cast creeps across $N's skin, as dust settles "
                "on it in a protective layer.", FALSE, ch, 0, victim, TO_NOTVICT);
            duration = level * 2;
            break;
        case SECT_SALT_FLATS:
            send_to_char("A fine layer of salty dust settles on your skin, "
                         "forming a protective shield.\n\r", victim);
            act("A fine layer of salty dust settles on $N's skin, " "forming a protective shield.",
                FALSE, ch, 0, victim, TO_NOTVICT);
            duration = (level * 3) / 2;
            break;
        case SECT_THORNLANDS:
            send_to_char("The thorny bushes rustle as the dust in the air "
                         "settles on your skin in a protective layer.\n\r", victim);
            act("The thorny bushes rustle as the dust in the air settles "
                "on $N's skin in a protective layer.", FALSE, ch, 0, victim, TO_NOTVICT);
            duration = level;
            break;
        case SECT_SILT:
        case SECT_SHALLOWS:
            send_to_char("Silt and ash whirl in the air around you, settling "
                         "in a protective layer on your skin.\n\r", victim);
            act("Silt and ash whirl in the air around $N, settling in a "
                "protective layer on $S skin.", FALSE, ch, 0, victim, TO_NOTVICT);
            duration = level / 2;
            break;
        default:
            send_to_char("The dust and sand in the air fuse into a layer "
                         "of armor around you.\n\r", victim);
            act("The dust and sand in the air fuse into a layer " "of armor around $N.", FALSE, ch,
                0, victim, TO_NOTVICT);
            duration = level;
            break;
        }

    duration = MAX(1, duration);        // No 0 durations

    af.type = SPELL_ARMOR;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = level;
    af.location = CHAR_APPLY_AC;
    af.bitvector = 0;
    stack_spell_affect(victim, &af, 24);
}                               /* end Spell_Armor */


/* AURA DRAIN: Damages a player's mana, in an amount proportional
 * to the power level used.  Suk-Krathian and sorcerer spell.
 * Nilaz divan hurn.
 */
void
spell_aura_drain(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* Begin Spell_Aura_Drain */

    if (!ch) {
        gamelog("No ch in aura drain");
        return;
    }

    if (!victim) {
        gamelog("No victim in aura drain");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (ch->in_room->sector_type == SECT_SHALLOWS || ch->in_room->sector_type == SECT_SILT) {
        adjust_mana(victim, -number((level * level), (2 * (level * level))));
    } else {
        adjust_mana(victim, -number((level * level), (3 * (level * level))));
    }

    if (ch != victim) {
        act("You point your finger at $N, draining $S magickal aura.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("$n points $s finger at you, pulling magick away from you.", FALSE, ch, 0, victim,
            TO_VICT);
        act("$n points $s finger at $N, who writhes, shuddering in pain.", TRUE, ch, 0, victim,
            TO_NOTVICT);
    } else {
        act("You shudder in pain as you drain your own aura.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n shudders in pain!", FALSE, ch, 0, 0, TO_ROOM);
    }
}                               /* end Spell_Aura_Drain */


/* BALL OF LIGHT: Summons a light object, which automatically takes
 * up the AROUND_HEAD wear location.  The object varies according
 * to the guild of the caster, and its duration is determined by
 * level.  It can be cast on another person, in which case the object
 * appears around their head instead.  Currently a Krathi and sorcerer
 * spell only, but provisions have been made for other elementalists
 * (Ruk, Whira, and Vivadu) casting it.  Suk-Krath wilith viod.
 */
void
spell_ball_of_light(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* Begin Spell_Ball_Of_Light */
    OBJ_DATA *tmp_obj;
    char *customLightObjVnum;

    if (!ch) {
        gamelog("No ch in ball of light");
        return;
    }

    if (!victim) {
        gamelog("No victim in ball of light");
        return;
    }

    check_crim_cast(ch);

    /* edesc values for light objects */
    customLightObjVnum = find_ex_description("[CUSTOM_BALL_OF_LIGHT_VNUM]", ch->ex_description, TRUE);
    if (customLightObjVnum) {
        tmp_obj = read_object(atoi(customLightObjVnum), VIRTUAL);
    } else if (IS_TRIBE(ch, 14)) { /* specialized Conclave light objects */
        switch (GET_GUILD(ch)) {
        case GUILD_FIRE_ELEMENTALIST:
            tmp_obj = read_object(910, VIRTUAL);
            break;
        case GUILD_STONE_ELEMENTALIST:
            tmp_obj = read_object(911, VIRTUAL);
            break;
        case GUILD_WATER_ELEMENTALIST:
            tmp_obj = read_object(912, VIRTUAL);
            break;
        case GUILD_LIGHTNING_ELEMENTALIST:
            tmp_obj = read_object(913, VIRTUAL);
            break;
        case GUILD_WIND_ELEMENTALIST:
            tmp_obj = read_object(914, VIRTUAL);
            break;
        default:
            tmp_obj = read_object(910, VIRTUAL);
            break;
        }
    } else if (IS_TRIBE(ch, 26))
        tmp_obj = read_object(27, VIRTUAL);
    else if (IS_TRIBE(ch, 24))
        tmp_obj = read_object(29, VIRTUAL);
    else if (IS_TRIBE(ch, 45))
        tmp_obj = read_object(31, VIRTUAL);
    else if ((IS_TRIBE(ch, 64)) || ((IS_TRIBE(ch, 73)) || (IS_TRIBE(ch, 11))))
        tmp_obj = read_object(28, VIRTUAL);
    else
        switch (GET_GUILD(ch)) {
        case GUILD_TEMPLAR:    /* green */
            tmp_obj = read_object(1894, VIRTUAL);
            break;
        case GUILD_WIND_ELEMENTALIST:  /* white */
            tmp_obj = read_object(1895, VIRTUAL);
            break;
        case GUILD_STONE_ELEMENTALIST: /* brown */
            tmp_obj = read_object(1896, VIRTUAL);
            break;
        case GUILD_WATER_ELEMENTALIST: /* blue */
            if (ch->in_room->sector_type == SECT_WATER_PLANE) {
                tmp_obj = read_object(1118, VIRTUAL);
            } else {
                tmp_obj = read_object(1897, VIRTUAL);
            }
            break;
        case GUILD_FIRE_ELEMENTALIST:  /* red  */
            tmp_obj = read_object(1898, VIRTUAL);
            break;
        case GUILD_LIGHTNING_ELEMENTALIST:     /* yellow sparks */
            tmp_obj = read_object(1893, VIRTUAL);
            break;
        case GUILD_DEFILER:
            if (ch->specials.eco >= -5)
                tmp_obj = read_object((1893 + number(0, 6)), VIRTUAL);
            else
                tmp_obj = read_object((1889 + number(0, 3)), VIRTUAL);
            break;
        default:
            tmp_obj = read_object(1899, VIRTUAL);
            break;
        }
    obj_to_char(tmp_obj, ch);

    act("$n opens $s hand and conjures $p.", TRUE, ch, tmp_obj, 0, TO_ROOM);
    act("You open your hand and conjure $p.", TRUE, ch, tmp_obj, 0, TO_CHAR);

    if (CAN_WEAR(tmp_obj, ITEM_WEAR_ABOUT_HEAD)) {
        if (victim->equipment[WEAR_ABOUT_HEAD]) {
            if (ch != victim) {
                act("$n tosses $p into the air, where it collides with something"
                    " orbiting $N's head, and falls to the ground.", FALSE, ch, tmp_obj, victim,
                    TO_NOTVICT);
                act("$n tosses $p into the air, where it collides with something"
                    " orbiting about your head and falls to the ground.", FALSE, ch, tmp_obj,
                    victim, TO_VICT);
                act("You toss $p into the air, where it collides with something"
                    " orbiting about $N's head and falls to the ground.", FALSE, ch, tmp_obj,
                    victim, TO_CHAR);
            } else {
                act("$n tosses $p into the air, where it collides with something"
                    " orbiting about $s head and falls to the ground.", FALSE, ch, tmp_obj, 0,
                    TO_ROOM);
                act("You toss $p into the air, where it collides with something"
                    " orbiting your head and falls to the ground.", FALSE, ch, tmp_obj, 0, TO_CHAR);
            }
            obj_from_char(tmp_obj);
            obj_to_room(tmp_obj, ch->in_room);
            light_a_light(tmp_obj, ch->in_room);
        } else {
            if (victim != ch) {
                act("$n tosses $p into the air, where it assumes orbit around " "$N's head.", TRUE,
                    ch, tmp_obj, victim, TO_NOTVICT);
                act("$n tosses $p into the air, where it assumes orbit around " "your head.", TRUE,
                    ch, tmp_obj, victim, TO_VICT);
                act("You toss $p into the air, where it assumes orbit around " "$N's head.", TRUE,
                    ch, tmp_obj, victim, TO_CHAR);
            } else {
                act("$n tosses $p into the air, where it assumes orbit around " "$N's head.", FALSE,
                    ch, tmp_obj, victim, TO_ROOM);
                act("You toss $p into the air, where it assumes orbit around " "your head.", FALSE,
                    ch, tmp_obj, victim, TO_CHAR);
            }
            obj_from_char(tmp_obj);
            equip_char(victim, tmp_obj, WEAR_ABOUT_HEAD);

            light_a_light(tmp_obj, ch->in_room);
        }
    } else                      // can't wear on about head
    {
        act("$n tosses $p into the air, where it assumes an orbit around" " the room.", FALSE, ch,
            tmp_obj, 0, TO_ROOM);
        act("You toss $p into the air, where it assumes an orbit around" " the room.", FALSE, ch,
            tmp_obj, 0, TO_CHAR);
        obj_from_char(tmp_obj);
        obj_to_room(tmp_obj, ch->in_room);
        light_a_light(tmp_obj, ch->in_room);
    }
    // set the hour timer on it to 3 * level game hours
    tmp_obj->obj_flags.value[0] = tmp_obj->obj_flags.value[1] = level * 3;

    // make it so that removing or extinguishing it extracts it from the game.
    MUD_SET_BIT(tmp_obj->obj_flags.value[5], LIGHT_FLAG_CONSUMES);
}                               /* end Spell_Ball_Of_Light */


/* Helper function used in BANISH and TELEPORT. */
void
create_teleport_rune(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                     OBJ_DATA * obj)
{
    check_crim_cast(ch);
}


ROOM_DATA *
pick_allanak_barrier_sewer_room()
{
  ROOM_DATA *rm = (ROOM_DATA *) NULL;
  int rooms[19] = {11020, 11021, 11022, 11023, 11024, 11025, 11026, 11030, 11031, 11032, 
                   11033, 11034, 11035, 11039, 11040, 11041, 11042, 11043, 11044};
  int try = 0;

  while (!rm && try < 20) {
    rm = get_room_num(rooms[number(0, 18)]);
    if (rm && (IS_SET(rm->room_flags, RFL_IMMORTAL_ROOM)
               || IS_SET(rm->room_flags, RFL_PRIVATE)
               || IS_SET(rm->room_flags, RFL_FALLING)
               || IS_SET(rm->room_flags, RFL_NO_MAGICK)
               || IS_SET(rm->room_flags, RFL_DEATH))) {
      rm = (ROOM_DATA *) NULL;
      try++;
    }
  }

  return rm;
}

ROOM_DATA *
pick_teleport_room()
{
    int num = 0, i = 0;
    ROOM_DATA *rm = (ROOM_DATA *) NULL;
#ifdef PICKZONE_USE
    byte pickzone = 0;
#else
    int pickzone = 0;
#endif


    switch (number(1, 10)) {    /* number 1-10 */
    case 1:                    /* 10% is crap like cities */
        switch (number(1, 8)) {
        case 1:                /* Villages */
            pickzone = (byte) 4;
            break;
        case 2:                /* Allanak */
            pickzone = (byte) 5;
            break;
        case 3:                /* Gol Krathu */
            pickzone = (byte) 58;
            break;
        case 4:                /* Labyrinth */
            pickzone = (byte) 77;
            break;
        case 5:                /* Sewers */
            pickzone = (byte) 11;
            break;
        case 6:                /* Red Storm */
            pickzone = (byte) 10;
            break;
        case 7:                /* Estates, etc. */
            pickzone = (byte) 35;
            break;
        case 8:                /* Under Tuluk */
            pickzone = (byte) 46;
            break;
        default:
            pickzone = (byte) 56;
            break;
        }
        break;
    default:                   /* the other 90% */
        switch (number(1, 24)) {
        case 1:                /* Mantis Valley */
            pickzone = (byte) 24;
            break;
        case 2:                /* Grey Forest */
            pickzone = (byte) 39;
            break;
        case 3:                /* Southern Desert */
            pickzone = (byte) 48;
            break;
        case 4:                /* Desert Around Allanak */
            pickzone = (byte) 49;
            break;
        case 5:                /* Hal's Scary Chasm */
            pickzone = (byte) 14;
            break;
        case 6:                /* Table Lands */
            pickzone = (byte) 68;
            break;
        case 7:                /* Muark lands */
            pickzone = (byte) 6;
            break;
        case 8:                /* Red Desert South */
            pickzone = (byte) 60;
            break;
        case 9:                /* Red Desert North */
            pickzone = (byte) 61;
            break;
        case 10:               /* More Red Desert */
            pickzone = (byte) 67;
            break;
        case 11:               /* Lanthil */
            pickzone = (byte) 45;
            break;
        case 12:               /* Northern Plains */
            pickzone = (byte) 56;
            break;
        case 13:               /* Blackwing territory */
            pickzone = (byte) 21;
            break;
        case 14:               /* Canyons of Waste */
            pickzone = (byte) 23;
            break;
        case 15:               /* Southern Gol Krathu) */
            pickzone = (byte) 38;
            break;
        case 16:               /* Dyrinis' mountains */
            pickzone = (byte) 25;
            break;
        case 17:               /* Air zone */
            pickzone = (byte) 26;
            break;
        case 18:               /* Air zone */
            pickzone = (byte) 27;
            break;
        case 19:               /* SE Gol Krathu */
            pickzone = (byte) 38;
            break;
        case 20:               /* Salt Flats */
            pickzone = (byte) 52;
            break;
        case 21:               /* More Grey Forest */
            pickzone = (byte) 55;
            break;
        case 22:               /* Western desert */
            pickzone = (byte) 66;
            break;
        case 23:               /* Thornwalker Mountains */
            pickzone = (byte) 72;
            break;
        case 24:               /* Shalindral Mountains */
            pickzone = (byte) 74;
            break;
        default:
            pickzone = (byte) 1;
            break;
        }
        break;
    }

    num = number(1, zone_table[pickzone].rcount);

    for (rm = zone_table[pickzone].rooms, i = 1; rm && (i < num); rm = rm->next_in_zone, i++);

    return rm;
}                               /* end pick_teleport_room */


/* BANISHMENT: The banishment spell flings the target to a random
 * room, as determined by the helper function, pick_teleport_room.
 * People cannot be banished to rooms flagged NO_MAGICK, PRIVATE,
 * IMMORTAL_ROOM, DEATH, or INDOORS.  Templars automatically banish
 * people to room 50664 (the arena in Allanak).  Power level has no
 * affect.  Whiran and sorcerer spell.  Whira locror hurn. */
void
spell_banishment(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Banishment */
  ROOM_DATA *dest_room = (ROOM_DATA *) NULL;
  ROOM_DATA *was_in;
  int roomnum, i = 0, sector, basenum;
  bool found_good_room = FALSE;
  
  if (!ch) {
    gamelog("No ch in banishment");
    return;
  }
  
  if (!victim) {
    gamelog("No victim in banishment");
    return;
  }
  
  check_criminal_flag(ch, NULL);

  if (obj && 
      obj->obj_flags.type == ITEM_TELEPORT && 
      obj->in_room &&
      IS_SET(victim->specials.nosave, NOSAVE_MAGICK)) {
    dest_room = obj->in_room;
  } 

  if (!dest_room) { // Didn't get dest room from obj, keep looking

    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
      act("You gesture at $N but nothing happens.", 
          FALSE, ch, 0, victim, TO_CHAR);
      act("$n gestures at you but nothing happens.", 
          FALSE, ch, 0, victim, TO_VICT);
      act("$n gestures at $N and $E but nothing happens.", 
          TRUE, ch, 0, victim, TO_NOTVICT);
      return;
    }
    
    /* If victim is a templar in their own city-state, they are protected */
    if (templar_protected(ch, victim, SPELL_BANISHMENT, level)) {
      return;
    }

    /* if caster is in Tuluk, and not a templar, spell dies */
    if (tuluk_magic_barrier(ch, 0)) {
      act("You gesture at $N and $E is briefly surrounded by a cloud of grey, billowing dust.", 
          FALSE, ch, 0, victim, TO_CHAR);
      act("$n gestures at you, briefly enveloping you in a cloud of grey, billowing dust.", 
          FALSE, ch, 0, victim, TO_VICT);
      act("$n gestures at $N and $E is briefly surrounded by a cloud of grey, billowing dust.", 
          TRUE, ch, 0, victim, TO_NOTVICT);

      return;
    }
    
    /* if the caster is in Allanak, and not a templar, move them to arena and exit */
    if (allanak_magic_barrier(ch, 0)) {
      if (!no_specials && special(victim, CMD_FROM_ROOM, "banish")) {
        return;
      }

      if (!IS_SET(ch->specials.act, CFL_CRIM_ALLANAK)) {
        qgamelogf(QUIET_MISC, 
                  "allanak_magic_barrier: %s tried to cast banish in #%d, blocked.",
                  MSTR(ch, name), 
                  ch->in_room->number);

        send_to_char("Nothing happens.\n\r", ch);
        return;
      }

      qgamelogf(QUIET_MISC, 
                "allanak_magic_barrier: %s tried to cast banish in #%d, sent to arena.",
                MSTR(ch, name), 
                ch->in_room->number);
      
      dest_room = get_room_num(50664);

      act("You gesture at $N and your surroundings being to fade away, and then come back again...", 
          FALSE, ch, 0, victim, TO_CHAR);
      act("$n gestures at you and then is enveloped in a cloud of grey, billowing dust.",
          FALSE, ch, 0, victim, TO_VICT);
      act("$n gestures at $N before disappearing in a cloud of grey, billowing dust.", 
          TRUE, ch, 0, victim, TO_NOTVICT);
      
      act("Your surroundings begin to fade away, and then come back again...", 
          FALSE, victim, 0, 0, TO_CHAR);
      
      was_in = ch->in_room;
      char_from_room(ch);
      char_to_room(ch, dest_room);
      
      if (!no_specials && special(ch, CMD_TO_ROOM, "banish")) {
        char_from_room(ch);
        char_to_room(ch, was_in);
        return;
      }
      
      act("$n appears in a cloud of grey, billowing dust, looking surprised.", 
          FALSE, ch, 0, 0, TO_ROOM);
      cmd_look(ch, "", 0, 0);

      return;
    }
    
    /* Templars automatically banish peoples to the arena. -San */
    if (GET_GUILD(ch) == GUILD_TEMPLAR) {
      dest_room = get_room_num(50664);
      found_good_room = (dest_room != 0);
    } else { // Not a templar
      if (ch->in_room->zone == 34) {      /* Elemental Planes zone 34 */
        sector = ch->in_room->sector_type;
        switch (sector) {
        case SECT_EARTH_PLANE:
          basenum = 34100;
          break;
        case SECT_WATER_PLANE:
          basenum = 34200;
          break;
        case SECT_LIGHTNING_PLANE:
          basenum = 34300;
          break;
        case SECT_NILAZ_PLANE:
          basenum = 34400;
          break;
        case SECT_SHADOW_PLANE:
          basenum = 34500;
          break;
        case SECT_AIR_PLANE:
          basenum = 34600;
          break;
        case SECT_FIRE_PLANE:
          basenum = 34700;
          break;
        default:
          basenum = 34400;
          break;
        }
        while ((!dest_room) || (i <= 4)) {
          roomnum = basenum + number(1, 99);
          dest_room = get_room_num(roomnum);
          i++;
        }
        if (dest_room)
          found_good_room = TRUE;
      } else {              /* Not on Elemental Planes */
        for (i = 1; i <= MAX(1, (level / 2)); i++) {
          dest_room = pick_teleport_room();
          if ((dest_room) && (!IS_SET(dest_room->room_flags, RFL_IMMORTAL_ROOM)
                              && !IS_SET(dest_room->room_flags, RFL_PRIVATE)
                              && !IS_SET(dest_room->room_flags, RFL_NO_MAGICK)
                              && !IS_SET(dest_room->room_flags, RFL_DEATH)
                              && !IS_SET(dest_room->room_flags, RFL_INDOORS))) {
            found_good_room = TRUE;
            break;
          } else
            found_good_room = FALSE;
        }     /* for */
      }         /* else not on elemental plane */
    }             /* else not a templar */
  }                 /* else not an obj */
  
  if ((!found_good_room) || (!dest_room)){
    act("The spell fizzles and dies.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
  /* Messages to source room */
  if (!no_specials && special(victim, CMD_FROM_ROOM, "banish")) {
    return;
  }
  
  act("You gesture at $N and $E disappears in a cloud of grey, billowing dust.", 
      FALSE, ch, 0, victim, TO_CHAR);
  act("$n gestures at you, enveloping you in a cloud of grey, billowing dust.", 
      FALSE, ch, 0, victim, TO_VICT);
  act("$n gestures at $N and $E disappears in a cloud of grey, billowing dust.", 
      TRUE, ch, 0, victim, TO_NOTVICT);
  
  act("Your surroundings begin to fade away, and then come back again...", 
      FALSE, victim, 0, 0, TO_CHAR);
  
  was_in = victim->in_room;
  char_from_room(victim);
  char_to_room(victim, dest_room);
  
  if (!no_specials && special(victim, CMD_TO_ROOM, "banish")) {
    char_from_room(victim);
    char_to_room(victim, was_in);
    return;
  }
  
  act("$n appears in a cloud of grey, billowing dust, looking surprised.", FALSE, victim, 0, 0,
      TO_ROOM);
  cmd_look(victim, "", 0, 0);
  
  return;
}                               /* end Spell_Banishment */


/* BLIND: Blinds the target completely for a duration proportional 
 * to power level.  Cumulative. Beside affecting their ability
 * to see, reduces their offense and defense. Drovian and sorcerer spell. 
 * Drov mutur hurn.  No component required.
 */
void
spell_blind(byte level,         /* level cast at */
            sh_int magick_type, /* type of magick used */
            CHAR_DATA * ch,     /* caster */
            CHAR_DATA * victim, /* target character */
            OBJ_DATA * obj)
{                               /* target object */
    /* Begin Spell_Blind */
    int duration;
    struct affected_type af;
    char buf[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in blind");
        return;
    }

    if (!victim) {
        gamelog("No victim in blind");
        return;
    }

    check_criminal_flag(ch, NULL);

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        act("Your magicks seem ineffectual in this place.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* Don't blind someone who is permanently blinded -Morg 2/13/06 */
    if (IS_AFFECTED(victim, CHAR_AFF_BLIND)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
        act("The shadows around $N solidify briefly, blocking your spell.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("The shadows around you solidify briefly.", FALSE, ch, 0, victim, TO_VICT);
        act("The shadows around $N solidify briefly.", FALSE, ch, 0, victim, TO_NOTVICT);
        return;
    }
    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
        act("You point at $N and their eyes glaze over momentarily, then return to normal.",
            FALSE, ch, 0, victim, TO_CHAR);
        act("$n points at you and your eyes tingle slightly.", TRUE, ch, 0, victim, TO_VICT);
        act("$n points at $N and $S eyes glaze over momentarily, then return to normal.", TRUE, ch,
            0, victim, TO_NOTVICT);
        return;
    }

    duration = number(1, level);

    if (GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST) {
        act("Your surroundings darken, growing black and blacker, " "until you cannot see at all.",
            TRUE, victim, 0, 0, TO_CHAR);
    } else {
        act("The world splinters, falling into shards of light, and "
            "then darkness claims your vision.", TRUE, victim, 0, 0, TO_CHAR);
    }

    act("You point at $N, and $E staggers in shock, blind.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n points at $N, and $E staggers in shock, blind.", FALSE, ch, 0, victim, TO_NOTVICT);

    af.type = SPELL_BLIND;
    af.location = CHAR_APPLY_NONE;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.duration = duration;
    af.bitvector = CHAR_AFF_BLIND;
    stack_spell_affect(victim, &af, 24);

    /* Stop fighting if it's been cast in combat. */
    /* they get a blind fight skill check to stay in combat */
    if (victim->specials.fighting 
        && !CAN_SEE(victim, victim->specials.fighting)
        && number(1, 100) > get_skill_percentage(victim, SKILL_BLIND_FIGHT)) {
        stop_all_fighting(victim);
    } else if (victim->specials.fighting 
        && !CAN_SEE(victim, victim->specials.fighting)) {
        gain_skill(ch, SKILL_BLIND_FIGHT, 1);
    }

    /* if you're blind you can't see */
    stop_watching_raw(victim);

    if ((IS_NPC(victim)) && (!victim->specials.fighting)) {
        find_ch_keyword(ch, victim, buf, sizeof(buf));
        cmd_kill(victim, buf, CMD_KILL, 0);
    }

}                               /* end Spell_Blind */


/* BREATHE WATER: 
 * Let's people breathe underwater/sector_water_plane
 * vivadu wril pret.  No component required.
 */
void
spell_breathe_water(byte level, /* level cast at */
                    sh_int magick_type, /* type of magick used */
                    CHAR_DATA * ch,     /* caster */
                    CHAR_DATA * victim, /* target character */
                    OBJ_DATA * obj)
{                               /* target object */
    /* Begin Spell_Breathe_Water */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in breathe water");
        return;
    }

    if (!victim) {
        gamelog("No victim in breathe water");
        return;
    }

    check_crim_cast(ch);

    // Plane specific checks
    //  if (ch->in_room->sector_type == SECT_NILAZ_PLANE) {
    //    act("Your magicks seem ineffectual in this place.", FALSE, ch, 0, 0, TO_CHAR);
    //    return;
    //  }

    duration = MAX(1, number(level / 2, level * 3));

    if (victim == ch) {
        act("A blue light envelopes your mouth and nose briefly.", FALSE, ch, 0, 0, TO_CHAR);
        act("A blue light envelops $n's mouth and nose briefly.", TRUE, ch, 0, 0, TO_ROOM);
    } else {
        act("A blue light comes from your hands, enveloping $N's mouth and nose briefly.", FALSE,
            ch, 0, victim, TO_CHAR);
        act("A blue light comes from $n's hands, enveloping your mouth and nose briefly.", FALSE,
            ch, 0, victim, TO_VICT);
        act("A blue light comes from $n's hands, enveloping $N's mouth and nose briefly.", TRUE, ch,
            0, victim, TO_NOTVICT);
    }

    if (!affected_by_spell(victim, SPELL_BREATHE_WATER))
        act("Your mouth, nose, and lungs feel strange - you can now breathe water!", TRUE, victim,
            0, 0, TO_CHAR);
    else
        act("Your mouth, nose, and lungs tingle, increasing your ability to breathe water.", TRUE,
            victim, 0, 0, TO_CHAR);

    af.type = SPELL_BREATHE_WATER;
    af.location = 0;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.duration = duration;
    af.bitvector = 0;
    stack_spell_affect(victim, &af, 36);
}                               /* end Spell_Breathe_Water */


/* BURROW: Tunnels a hole into the ground, no matter where the caster is
 * standing, which swallows up the caster and promptly closes again.
 * The burrow is large enough only to crouch within, and leaving the burrow
 * will negate the spell and place the caster back where the spell was cast. 
 * The caster is capable of looking up into the room above him/her. 
 * Rukkian and sorcerer spell.  Ruk divan grol.
 */
void
spell_burrow(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Burrow */
    int sector;
    ROOM_DATA *burrow;
    int duration;
    struct affected_type af;

    check_crim_cast(ch);

    /* cannot be cast in sector SILT */
    if (ch->in_room->sector_type == SECT_SHALLOWS && ch->in_room->sector_type == SECT_SILT) {
        act("You call upon Ruk for safety, but the silty ground refuses to yield itself to your will.\n\r",
            FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* cannot be cast in sector AIR */
    if (ch->in_room->sector_type == SECT_AIR) {
        act("You call upon Ruk for safety, but find no earth to shelter you.\n\r", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* cannot be case in the elemental planes except Ruk */
    if ((ch->in_room->sector_type == SECT_FIRE_PLANE)
        || (ch->in_room->sector_type == SECT_WATER_PLANE)
        || ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)
        || (ch->in_room->sector_type == SECT_AIR_PLANE)
        || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)) {
        act("You call upon Ruk for safety, but the spell seems unable to function here.\n\r", FALSE,
            ch, 0, 0, TO_CHAR);
        return;
    }

    duration = ((level * 5) - get_char_size(ch));
    if (duration < 0) {
	act("You call upon Ruk for safety, but the spell is not powerful enough to yield to your girth.", FALSE, ch, 0, 0, TO_CHAR);
	return;
    }

    if (!(burrow = find_reserved_room())) {
        cprintf(ch, "Error in spell, please see a Coder.\n");
        return;
    }
    if (burrow->name)
        free(burrow->name);
    burrow->name = strdup("A Small Underground Burrow");
    if (burrow->description)
        free(burrow->description);
    burrow->description =
        strdup("   You are resting in a tiny burrow, apparently underground\n\r"
               "somewhere.  The walls surround your body, restricting your movement\n\r"
               "and blocking your vision, but are cool and comfortable to the\n\r"
               "touch.  Above your head is a flickering portal, through which\n\r"
               "you can see the place from which you entered this magickal\n\rburrow.\n\r");

    CREATE(burrow->direction[DIR_UP], struct room_direction_data, 1);
    burrow->direction[DIR_UP]->general_description = NULL;
    burrow->direction[DIR_UP]->keyword = NULL;
    burrow->direction[DIR_UP]->exit_info = 0;
    burrow->direction[DIR_UP]->key = -1;
    burrow->direction[DIR_UP]->to_room_num = NOWHERE;
    burrow->direction[DIR_UP]->to_room = ch->in_room;
    burrow->sector_type = SECT_CAVERN;
    MUD_SET_BIT(burrow->room_flags, RFL_INDOORS);
    MUD_SET_BIT(burrow->room_flags, RFL_NO_MAGICK);
    new_event(EVNT_REMOVE_ROOM, (int) duration * 60, 0, 0, 0, burrow, 0);

    save_rooms(&zone_table[RESERVED_Z], RESERVED_Z);

    /* Different messages for different sectors.  */
    sector = ch->in_room->sector_type;

    switch (sector) {
    case SECT_CAVERN:
        act("$n suddenly dives to the stony ground, and disappears.", FALSE, ch, 0, 0, TO_ROOM);
        act("You call upon Ruk for safety, and the stony ground swallows you.\n\r"
            "You find yourself in...", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_DESERT:
        act("The sands reach up, enveloping $n as $e disappears.", FALSE, ch, 0, 0, TO_ROOM);
        act("You call upon Ruk for safety, and the sands reach up to swallow you.\n\r"
            "You find yourself in...", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_ROAD:
        act("$n suddenly dives towards the road's dusty surface and disappears.", FALSE, ch, 0, 0,
            TO_ROOM);
        act("You call upon Ruk for safety, and the ground reaches up to swallow you.\n\r"
            "You find yourself in...", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_RUINS:
        act("$n suddenly dives towards the rubble-strewn ground and disappears.", FALSE, ch, 0,
            0, TO_ROOM);
        act("You call upon Ruk for safety, and the rubble-strewn ground "
            "reaches up to swallow you.\n\rYou find yourself in...", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_SALT_FLATS:
        act("$n suddenly dives to the salty ground, and disappears.", FALSE, ch, 0, 0, TO_ROOM);
        act("You call upon Ruk for safety, and the salty ground swallows you.\n\r"
            "You find yourself in...", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_THORNLANDS:
        act("The twisted roots of the thornbushes part for $n as $e "
            "dives to the ground and disappears.", FALSE, ch, 0, 0, TO_ROOM);
        act("You call upon Ruk for safety, and the thornbush roots "
            "part to allow you shelter.\n\r" "You find yourself in...", FALSE, ch, 0, 0, TO_CHAR);
        break;
    default:
        act("$n suddenly dives to the ground, and disappears.", FALSE, ch, 0, 0, TO_ROOM);
        act("You call upon Ruk for safety, and dive to the ground.\n\rYou find yourself in...",
            FALSE, ch, 0, 0, TO_CHAR);
        break;
    }
    /* people burrowing while in combat end up standing.. let's remove fighting */
    stop_all_fighting(ch);

    char_from_room(ch);
    char_to_room(ch, burrow);
    cmd_look(ch, "", 0, 0);

    act("$n suddenly dives in from the portal in the ceiling.", FALSE, ch, 0, 0, TO_ROOM);

    set_char_position(ch, POSITION_RESTING);
    update_pos(ch);

    ch->specials.riding = NULL;
    
    // Adding timeout affect - Tiernan 3/14/2011
    af.type = SPELL_BURROW;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_BURROW;
    affect_join(victim, &af, FALSE, FALSE);

}                               /* end Spell_Burrow */


/* CALM: The victim of this spell will feel an overwhelming sense of serenity
 * and peacefulness for the duration of the spell, or until someone attacks
 * him/her.  While affected, the victim will be unable to attack anyone or
 * otherwise cause harm. Cumulative.  Vivaduan and sorcerer spell. 
 * Vivadu psiak chran. 
 */
void
spell_calm(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Calm */
    int duration, didsomething = 0;
    struct affected_type af;
    struct affected_type *tmp_af;
    char buf[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in calm");
        return;
    }

    if (!victim) {
        gamelog("No victim in calm");
        return;
    }

    check_crim_cast(ch);

    if ((IS_SET(victim->specials.act, CFL_UNDEAD))
        || (affected_by_spell(victim, SPELL_QUICKENING))
        || (affected_by_spell(victim, SPELL_INSOMNIA))) {
        if (affected_by_spell(victim, SPELL_QUICKENING)) {
            affect_from_char(victim, SPELL_QUICKENING);
        }
        if (affected_by_spell(victim, SPELL_INSOMNIA)) {
            affect_from_char(victim, SPELL_INSOMNIA);
        }
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    // Fury check
    if ((tmp_af = affected_by_spell(victim, SPELL_FURY))) {
        act("You wave your hand at $N and $S eyes glaze over as $E appears calmed.", FALSE, ch, 0,
            victim, TO_CHAR);
        act("$n waves $s hand at you and you feel much calmer than before.", FALSE, ch, 0, victim,
            TO_VICT);
        act("$n waves $s hand at $N and $S eyes glaze over as $E appears calmed.", TRUE, ch, 0,
            victim, TO_NOTVICT);

        if (tmp_af->power <= level) {   // Not strong enough
            level -= tmp_af->power;
            affect_from_char(victim, SPELL_FURY);
        } else {                // Weaken it
            tmp_af->power -= level;
            return;
        }
        if (level <= 0)
            return;
    }

    duration = 2 * level;
    if (!does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
        didsomething = 1;
        af.type = SPELL_CALM;
        af.duration = duration;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = 0;
        af.location = CHAR_APPLY_NONE;
        af.bitvector = CHAR_AFF_CALM;
        stack_spell_affect(victim, &af, 24);

        if (affected_by_spell(victim, SPELL_FURY))
            affect_from_char(victim, SPELL_FURY);

        act("You wave your hand at $N and $E appears calmed.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n waves $s hand at you and you feel suddenly calm.", FALSE, ch, 0, victim, TO_VICT);
        act("$n waves $s hand at $N and $E appears calmed.", TRUE, ch, 0, victim, TO_NOTVICT);

        snprintf(buf, sizeof(buf), "%s feels calmer.", MSTR(victim, short_descr));
        send_to_empaths(victim, buf);
        change_mood(victim, NULL);
    }

    /*  Stop fighting - Kelvik  */
    if (victim->specials.fighting) {
        stop_all_fighting(victim);
        didsomething = 1;
    }
    if (ch->specials.fighting == victim || ch->specials.alt_fighting == victim) {
        stop_fighting(ch, victim);
        didsomething = 1;
    }

    if (affected_by_spell(victim, AFF_MUL_RAGE)) {
        affect_from_char(victim, AFF_MUL_RAGE);
        didsomething = 1;
    }

    if (didsomething == 0)
        act("Your victim resists your spell.", FALSE, ch, 0, 0, TO_CHAR);

}                               /* end Spell_Calm */


/* CHAIN LIGHTNING: Allows the caster to conjure up a storm cloud, from 
 * whose heart lightning will strike out at those around him or her.  The 
 * duration of the cloud depends on the strength at which the spell is cast. 
 * Elkros and sorcerer spell.  Elkros dreth echri.
 */
void
spell_chain_lightning(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                      OBJ_DATA * obj)
{                               /* begin Spell_Chain_Lightning */
    int i, first_cast = 1;
    int dam;

    CHAR_DATA *tar_ch, *prev_ch;


    check_criminal_flag(ch, NULL);
    dam = (number((level * 3), (level * 5)));

    prev_ch = ch;

    send_to_char("You conjure lightning, and fling it into the air.\n\r", ch);
    act("$n conjures lightning, and hurls it into the air.", FALSE, ch, 0, 0, TO_ROOM);

/* Sticks around twice as long when cast in sector SILT, SHALLOWS, OR LIGHTNING_PLANE */
    if ((ch->in_room->sector_type == SECT_SILT)
        || (ch->in_room->sector_type == SECT_SHALLOWS)
        || (ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)) {
        level = level * 2;
        act("The lightning crackles and hisses with a greater than expected fury!", FALSE, ch, 0,
            ch, TO_CHAR);
    }
    if (IS_AFFECTED(ch, CHAR_AFF_INVISIBLE))
        appear(ch);             /* make caster visible */

    for (i = 0; i <= level; i++) {
        tar_ch = get_random_non_immortal_in_room(ch->in_room);
        if (!tar_ch) {
            send_to_room("The lightning streaks around angrily, and then " "dissipates.\n\r",
                         ch->in_room);
            gamelog("Error in spell chain_lightning.  0 return on get_random func.");
            return;
        }
        if (first_cast == 1) {
            act("The bolt of lightning twists sharply in the air, and strikes" " $N.", FALSE, ch, 0,
                tar_ch, TO_ROOM);
            if (tar_ch != ch) {
                act("The bolt of lightning twists sharply in the air, and strikes" " $N.", FALSE,
                    ch, 0, tar_ch, TO_CHAR);
                act("The bolt of lightning twists sharply in the air, and strikes" " you.", FALSE,
                    ch, 0, tar_ch, TO_VICT);
                if (IS_NPC(tar_ch))
                    if (!does_hate(tar_ch, ch))
                        add_to_hates_raw(tar_ch, ch);
            } else
                act("The bolt of lightning twists sharply in the air, and strikes" " you.", FALSE,
                    ch, 0, tar_ch, TO_CHAR);

            first_cast = 0;
        } else {
            if (prev_ch) {
                act("Lightning exits $n's body and strikes $N in the chest.", FALSE, prev_ch, 0,
                    tar_ch, TO_NOTVICT);
                act("Lightning exits $n's body and strikes you in the chest.", FALSE, prev_ch, 0,
                    tar_ch, TO_VICT);
                act("Lightning exits your body and strikes $N in the chest.", FALSE, prev_ch, 0,
                    tar_ch, TO_CHAR);
                if ((IS_NPC(prev_ch)) && (prev_ch != ch))
                    if (!does_hate(prev_ch, ch))
                        add_to_hates_raw(prev_ch, ch);
            } else {
                act("Lightning swirls in the air, then strikes $N in the chest.", FALSE, tar_ch, 0,
                    0, TO_ROOM);
                act("Lightning swirls in the air, then strikes you in the chest.", FALSE, tar_ch, 0,
                    0, TO_CHAR);
                if ((IS_NPC(tar_ch)) && (tar_ch != ch))
                    if (!does_hate(tar_ch, ch))
                        add_to_hates_raw(tar_ch, ch);
            }
        }
        if (affected_by_spell(tar_ch, SPELL_ENERGY_SHIELD)) {
            if (tar_ch != ch) {
                act("With a flash, the sparks of light surrounding $N absorb the bolt.", FALSE,
                    tar_ch, 0, tar_ch, TO_CHAR);
                act("With a flash, the sparks of light surrounding you absorb the bolt.", FALSE,
                    tar_ch, 0, tar_ch, TO_VICT);
                act("With a flash, the sparks of light surrounding $N absorb the bolt.", FALSE,
                    tar_ch, 0, tar_ch, TO_NOTVICT);
            } else {
                act("With a flash, the sparks of light surrounding you absorb the bolt.", FALSE,
                    tar_ch, 0, tar_ch, TO_CHAR);
                act("With a flash, the sparks of light surrounding $N absorb the bolt.", FALSE,
                    tar_ch, 0, tar_ch, TO_NOTVICT);
            }
        }
        if (tar_ch == ch) {
            if (!(affected_by_spell(tar_ch, SPELL_ENERGY_SHIELD))) {
                sflag_to_char(tar_ch, OST_BURNED);
                generic_damage(tar_ch, (dam / 4), 0, 0, (dam / 4));
            }
        } else {
            if (!(affected_by_spell(tar_ch, SPELL_ENERGY_SHIELD))) {
                sflag_to_char(tar_ch, OST_BURNED);
                generic_damage(tar_ch, dam, 0, 0, dam);
                WAIT_STATE(tar_ch, 1);
            }
        }
        prev_ch = tar_ch;
        if (GET_POS(tar_ch) == POSITION_DEAD) {
            if (!IS_NPC(tar_ch))
                gamelogf("%s has been killed by Chain Lighting cast by %s in room %d.",
                         GET_NAME(tar_ch), GET_NAME(ch), ch->in_room->number);
            die(tar_ch);
            prev_ch = ((CHAR_DATA *) NULL);
        }
        if (i == level) {
            if (!prev_ch)
                send_to_room("The lightning swirls in the air, then dissipates.\n\r", ch->in_room);
            else {
                act("Lightning exits $n's body, then dissipates.", FALSE, prev_ch, 0, tar_ch,
                    TO_NOTVICT);
                act("Lightning exits your body, then dissipates.", FALSE, prev_ch, 0, tar_ch,
                    TO_VICT);
            }
        }
    }
}                               /* end Spell_Chain_Lightning */


/* CHARM PERSON: Allows the caster to control the target. 
 * Requires a component.  Nilaz spell.  Nilaz mutur nikiz.
 */
void
spell_charm_person(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Charm_Person */
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in charm person");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if ((affected_by_spell(victim, SPELL_CHARM_PERSON)) || IS_IMMORTAL(victim)) {
        act("You cannot seem to gain control of their mind.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!get_componentB
        (ch, SPHERE_ALTERATION, level, "$p is consumed in a greenish flame.",
         "$p is consumed in a greenish flame in $n's hands",
         "Your hands heat up then grow instantly cool - something is missing."))
        return;

    if (number(0, 6) > level) {
        act("You feel a struggle for the control of your mind, but you" " maintain your will.",
            FALSE, ch, 0, 0, TO_VICT);
        act("You cannot seem to gain control of their mind.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    act("You intently stare at $N and $E is forced to meet your gaze.", FALSE, ch, 0, victim,
        TO_CHAR);
    act("$n intently stares at you and you are forced to meet $s gaze.", FALSE, ch, 0, victim,
        TO_VICT);
    act("$n intently stares at $N who is forced to meet $s gaze.", TRUE, ch, 0, victim, TO_NOTVICT);

    stop_follower(victim);
    add_follower(victim, ch);

    af.type = SPELL_CHARM_PERSON;
    af.duration = (level / 2) + dice(1, 3);
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = CHAR_AFF_CHARM;
    affect_to_char(victim, &af);
}                               /* end Spell_Charm_Person */


/* CREATE DARKNESS: Creates a cloud of darkness over the room,
 * whose duration is dependent on the power level.  Non cumulative.
 * Drovian and sorcerer spell.  Drov wilith echri.
 */
void
spell_create_darkness(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                      OBJ_DATA * obj)
{                               /* begin Spell_Create_Darkness */
    int sector;

    check_crim_cast(ch);

    if (IS_SET(ch->in_room->room_flags, RFL_DARK)) {
        act("There is already ample darkness here.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    MUD_SET_BIT(ch->in_room->room_flags, RFL_DARK);

    /* Send to nearby_rooms -Sanvean */
    act("A cloud of darkness descends upon the area to the $D", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);

    sector = ch->in_room->sector_type;

    switch (sector) {

    case SECT_FIRE_PLANE:
        act("You call upon the darkness, but it cannot take hold here.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    case SECT_AIR:
        act("$n closes $s fist and the air around you darkens, coiling shadows unfolding within it.", FALSE, ch, 0, 0, TO_ROOM);
        act("You call upon the darkness, and shadows begin to coil and writhe "
            "in the air around you.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_DESERT:
        act("$n closes $s fist and shadows seep upwards from the sands, slowly covering the area "
            "in darkness.", FALSE, ch, 0, 0, TO_ROOM);
        act("You close your fist and shadows seep upwards from the sands, slowly covering the area "
            "in darkness.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_HEAVY_FOREST:
        act("$n closes $s fist and the shadows between the trees swell, slowly covering the area "
            "in darkness.", FALSE, ch, 0, 0, TO_ROOM);
        act("You close your fist and the shadows between the trees swell, slowly covering the area "
            "in darkness.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_LIGHT_FOREST:
        act("$n closes $s fist and darkness creeps upward from the shadows beneath the vegetation, "
            "covering the area in darkness.", FALSE, ch, 0, 0, TO_ROOM);
        act("You close your fist and darkness creeps upward from the shadows beneath the vegetation, " "covering the area in darkness.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_ROAD:
        act("$n closes $s fist and shadows seep upward from the road's surface, lengthening until they " "cover the ground like a dark blanket.", FALSE, ch, 0, 0, TO_ROOM);
        act("You close your fist and shadows seep upward from the road's surface, lengthening until they " "cover the ground like a dark blanket.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_RUINS:
        act("$n closes $s fist and and shadows slowly seep upward from the rubble, covering the area in " "darkness.", FALSE, ch, 0, 0, TO_ROOM);
        act("You call upon the shadows, which seep upward from the rubble, "
            "covering the area in darkness.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_SILT:
    case SECT_SHALLOWS:
        act("$n closes $s fist and the silt and ash in the air whirl violently, and slowly the area " "darkens.", FALSE, ch, 0, 0, TO_ROOM);
        act("You close your fist and the silt and ash in the air whirl violently, and slowly the area " "darkens.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    default:
        act("$n closes $s fist and a cloud of darkness descends upon the area.", FALSE, ch, 0, 0,
            TO_ROOM);
        act("You close your fist and summon a cloud of darkness that descends upon the area.",
            FALSE, ch, 0, 0, TO_CHAR);
        break;
    }

    new_event(EVNT_REMOVE_DARK, level * 30 * 60, 0, 0, 0, ch->in_room, 0);
/*    act("The cloud of darknes to the $D slowly dissipates.", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);*/

}                               /* End Spell_Create_Darkness */


/* CREATE FOOD: Allows the caster to summon food.  The type of food
 * varies depending on guild, race or tribe.  Cases exist for RACE
 * PLAINSOX, GUILD_STONE_ELEMENTALIST, GUILD WATER_ELEMENTALIST, 
 * GUILD_TEMPLAR, GUILD_DEFILER, and TRIBE 24.  Amount of food
 * created depends on power level.  Rukkian, Vivaduan, and sorcerer
 * spell.  Ruk wilith wril.
 * Note: sometime in the future, it would be nifty to add a case
 * where mon creates a particular delicacy for each.
 */
void
spell_create_food(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Create_Food */

    struct food_data
    {
        char *name;
        char *short_descr;
        char *long_descr;
    };

    OBJ_DATA *tmp_obj;
    int i, j;

    if (!ch) {
        gamelog("No ch in create food");
        return;
    }

    check_crim_cast(ch);

    /*act("You point a finger at the ground and it shimmers slightly.", FALSE, ch, 0, 0, TO_CHAR);
     * act("$n points a finger at the ground and it shimmers slightly.", TRUE, ch, 0, 0, TO_ROOM); */

    for (j = 0; j < level; j++) {
        i = number(0, 2);

        switch (i) {
        case (0):
            if (ch->in_room->sector_type == SECT_SILT)
                tmp_obj = read_object(644, VIRTUAL);
            else if (GET_RACE(ch) == RACE_PLAINSOX)
                tmp_obj = read_object(1304, VIRTUAL);
            else if (GET_RACE(ch) == RACE_HALFLING)
                tmp_obj = read_object(155, VIRTUAL);
            else if (GET_GUILD(ch) == GUILD_TEMPLAR)
                tmp_obj = read_object(156, VIRTUAL);
            else if (GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)
                tmp_obj = read_object(45466, VIRTUAL);
            else if (IS_TRIBE(ch, 24))
                tmp_obj = read_object(157, VIRTUAL);
            else if (GET_GUILD(ch) == GUILD_DEFILER)
                tmp_obj = read_object(39103, VIRTUAL);
            else
                tmp_obj = read_object(151, VIRTUAL);
            break;
        case (1):
            if (ch->in_room->sector_type == SECT_SILT)
                tmp_obj = read_object(20940, VIRTUAL);
            else if (GET_RACE(ch) == RACE_PLAINSOX)
                tmp_obj = read_object(1305, VIRTUAL);
            else if (GET_RACE(ch) == RACE_HALFLING)
                tmp_obj = read_object(55162, VIRTUAL);
            else if (GET_GUILD(ch) == GUILD_TEMPLAR)
                tmp_obj = read_object(50321, VIRTUAL);
            else if (GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)
                tmp_obj = read_object(45509, VIRTUAL);
            else if (IS_TRIBE(ch, 24))
                tmp_obj = read_object(64150, VIRTUAL);
            else if (GET_GUILD(ch) == GUILD_DEFILER)
                tmp_obj = read_object(20908, VIRTUAL);
            else
                tmp_obj = read_object(49039, VIRTUAL);
            break;
        default:
            if (ch->in_room->sector_type == SECT_SILT)
                tmp_obj = read_object(72197, VIRTUAL);
            else if (GET_RACE(ch) == RACE_PLAINSOX)
                tmp_obj = read_object(1307, VIRTUAL);
            else if (GET_RACE(ch) == RACE_HALFLING)
                tmp_obj = read_object(55160, VIRTUAL);
            else if (GET_GUILD(ch) == GUILD_TEMPLAR)
                tmp_obj = read_object(50071, VIRTUAL);
            else if (GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)
                tmp_obj = read_object(37452, VIRTUAL);
            else if (IS_TRIBE(ch, 24))
                tmp_obj = read_object(6398, VIRTUAL);
            else if (GET_GUILD(ch) == GUILD_DEFILER)
                tmp_obj = read_object(72255, VIRTUAL);
            else
                tmp_obj = read_object(126, VIRTUAL);
            break;
        }

        if (!tmp_obj) {
            gamelogf("Unable to create object for create food");
            return;
        }

        obj_to_room(tmp_obj, ch->in_room);
        MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);
        act("$p suddenly appears.", TRUE, ch, tmp_obj, 0, TO_ROOM);
        act("$p suddenly appears.", TRUE, ch, tmp_obj, 0, TO_CHAR);
        new_event(EVNT_REMOVE_OBJECT, level * 30 * 60, 0, 0, tmp_obj, 0, 0);
    }
}                               /* end Spell_Create_Food */


/* CREATE RUNE: Creates a magickal rune whos name is determined by
 * the caster at the time of casting and places it in the room.  This rune
 * can then be teleported to with the teleportation spell.
 * Whira and Sorcerer spell.
 * whira wilith viod.
 */
void
spell_create_rune(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Crete_Rune */

}                               /* end Spell_Create_Rune */

/* CREATE WATER: Allows the caster to fill a target container
 * with water, the amount dependent on the power level.  If cast
 * on a container holding a liquid other than water, it creates
 * undrinkable slime in it.  Vivaduan and sorcerer spell.   
 * Vivadu wilith wril.
 */
void
spell_create_water(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Create_Water */
    int water;

    if (!ch) {
        gamelog("No ch in create water");
        return;
    }

    check_crim_cast(ch);

    if (ch->in_room->sector_type == SECT_FIRE_PLANE) {
        send_to_char("The flames in this area prevent you from creating any water.\n\r", ch);
        return;
    }

    if (!obj) {
        act("You open your palm and water pours from your skin onto the ground.", FALSE, ch, 0, 0,
            TO_CHAR);
        act("$n opens $s palm and water pours from $s skin onto the ground.", TRUE, ch, 0, 0,
            TO_ROOM);
        return;
    }

    if (GET_ITEM_TYPE(obj) == ITEM_DRINKCON) {
        if (((obj->obj_flags.value[2] != LIQ_WATER)
             && (obj->obj_flags.value[2] != LIQ_WATER_VIVADU))
            && (obj->obj_flags.value[1] != 0))
            obj->obj_flags.value[2] = LIQ_SLIME;
        else {
            water = 5 * level;
            if (ch->in_room->sector_type == SECT_WATER_PLANE) {
                water = 10 * level;
                send_to_char
                    ("The magick surges here, and you make much more water than expected!\n\r", ch);
            }
            water = MIN(obj->obj_flags.value[0] - obj->obj_flags.value[1], water);
            if (water > 0) {
                if (ch->in_room->sector_type == SECT_SILT)
                    obj->obj_flags.value[2] = LIQ_SLIME;
                else
                    obj->obj_flags.value[2] = LIQ_WATER_VIVADU;
                obj->obj_flags.value[1] += water;
                act("You open your palm and water flows from your skin into $p.", FALSE, ch, obj, 0,
                    TO_CHAR);
                act("$n opens $s palm and water flows from $s skin into $p.", TRUE, ch, obj, 0,
                    TO_ROOM);
            }
        }
    }
}                               /* end Spell_Create_Water */

/* CREATE WINE: Allows the caster to fill a target container
 * with wine, the amount dependent on the power level.  If cast
 * on a container holding a liquid other than water, it creates
 * undrinkable slime in it.  Vivaduan spell. Vivadu wilith chran.
 */
void
spell_create_wine(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Create_Wine */
    int wine;

    if (!ch) {
        gamelog("No ch in create wine");
        return;
    }

    check_crim_cast(ch);

    if (ch->in_room->sector_type == SECT_FIRE_PLANE) {
        send_to_char("The flames in this area prevent you from creating any wine.\n\r", ch);
        return;
    }

    if (!obj) {
        act("You open your palm and wine pours from your skin onto the ground.", FALSE, ch, 0, 0,
            TO_CHAR);
        act("$n opens $s palm and wine pours from $s skin onto the ground.", TRUE, ch, 0, 0,
            TO_ROOM);
        return;
    }

    if (GET_ITEM_TYPE(obj) == ITEM_DRINKCON) {
        if ((obj->obj_flags.value[2] != LIQ_WINE)
            && (obj->obj_flags.value[1] != 0))
            obj->obj_flags.value[2] = LIQ_SLIME;
        else {
            wine = 5 * level;
            if (ch->in_room->sector_type == SECT_WATER_PLANE) {
                wine = 10 * level;
                send_to_char
                    ("The magick surges here, and you make much more wine than expected!\n\r", ch);
            }
            wine = MIN(obj->obj_flags.value[0] - obj->obj_flags.value[1], wine);

            if (wine > 0) {
                if (ch->in_room->sector_type == SECT_SILT)
                    obj->obj_flags.value[2] = LIQ_SLIME;
                // Muark
                else if (IS_TRIBE(ch, 24))
                    obj->obj_flags.value[2] = LIQ_HORTA_WINE;
                // Conclave
                else if (IS_TRIBE(ch, 14))
                    obj->obj_flags.value[2] = LIQ_GLOTH;
                // Southern Templars
                else if (GET_GUILD(ch) == GUILD_TEMPLAR)
                    obj->obj_flags.value[2] = LIQ_OCOTILLO_WINE;
                // Fales
                else if (IS_TRIBE(ch, 53))
                    obj->obj_flags.value[2] = LIQ_SPICE_BRANDY;
                // Halflings
                else if (IS_TRIBE(ch, 9))
                    obj->obj_flags.value[2] = LIQ_BADU;
                // Plainsfolk
                else if (IS_TRIBE(ch, 45))
                    obj->obj_flags.value[2] = LIQ_JIK;
                // Kuraci Archives
                else if (IS_TRIBE(ch, 62))
                    obj->obj_flags.value[2] = LIQ_SPICED_GINKA_WINE;
                else
                    obj->obj_flags.value[2] = LIQ_WINE;
                obj->obj_flags.value[1] += wine;
                act("You open your palm and a liquid flows from your skin into $p.", FALSE, ch, obj,
                    0, TO_CHAR);
                act("$n opens $s palm and a liquid flows from $s skin into $p.", TRUE, ch, obj, 0,
                    TO_ROOM);
            }
        }
    }
}                               /* end Spell_Create_Wine */


/* CURE BLINDNESS: Allows the caster to remove the blind spell
 * from the target, which can include him or herself.  Drovian
 * and sorcerer spell.  Drov viqrol wril.
 */
void
spell_cure_blindness(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                     OBJ_DATA * obj)
{                               /* begin Spell_Cure_Blindness */

    if (!victim) {
        gamelog("No victim in cure blindness");
        return;
    }

    check_crim_cast(ch);

    if (affected_by_spell(victim, SPELL_BLIND)) {
        affect_from_char(victim, SPELL_BLIND);
        if (victim != ch) {
            act("Color returns to $N's eyes as you wave a hand over them.", FALSE, ch, 0, victim,
                TO_CHAR);
            act("Color returns to your eyes as $n waves a hand over them.", FALSE, ch, 0, victim,
                TO_VICT);
            act("Color returns to $N's eyes as $n waves a hand over them.", TRUE, ch, 0, victim,
                TO_NOTVICT);
        } else {
            act("Color returns to your eyes as you wave a hand over them.", FALSE, ch, 0, 0,
                TO_CHAR);
            act("Color returns to $n's eyes as $e waves a hand over them.", TRUE, ch, 0, 0,
                TO_ROOM);
        }
        return;
    } else
        act("$N seems unaffected.", FALSE, ch, 0, victim, TO_CHAR);
}                               /* end Spell_Cure_Blindness */


/* CURE POISON: Allows the caster to cure poison on a person
 * affected by it, or to remove the poison flag from a drink,
 * food, dart, or weapon item.  Vivaduan and sorcerer spell.
 * Vivadu nathro wril.
 */
void
spell_cure_poison(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Cure_Poison */
    if (!ch) {
        gamelog("No ch in cure poison");
        return;
    }

    if (!victim && !obj) {
        gamelog("No victim or obj in cure poison");
        return;
    }

    check_crim_cast(ch);

    if (victim) {

        act("A cool mist flows from your hands onto $N.", FALSE, ch, 0, victim, TO_CHAR);
        act("A cool mist flows from $n's hands onto you.", FALSE, ch, 0, victim, TO_VICT);
        act("A cool mist flows from $n's hands onto $N.", TRUE, ch, 0, victim, TO_NOTVICT);

        if (affected_by_spell(victim, SPELL_POISON) || affected_by_spell(victim, POISON_GENERIC)) {
            if (affected_by_spell(victim, SPELL_POISON))
                affect_from_char(victim, SPELL_POISON);
            else
                affect_from_char(victim, POISON_GENERIC);
            act("A warm feeling runs through your body and you stop shaking.", FALSE, victim, 0, 0,
                TO_CHAR);
            act("$n stops shaking and starts to get $s color back.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
        if (affected_by_spell(victim, POISON_GRISHEN)) {
            affect_from_char(victim, POISON_GRISHEN);
            act("A soothing feeling runs through your body, and you stop" " sweating.", FALSE,
                victim, 0, 0, TO_CHAR);
            act("$n stops sweating and starts to get $s color back.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
        if (affected_by_spell(victim, POISON_SKELLEBAIN)) {
            affect_from_char(victim, POISON_SKELLEBAIN);
            act("A cool feeling tingles through your body, and your mind" " calms down.", FALSE,
                victim, 0, 0, TO_CHAR);
            act("$n blinks several times, as if clearing $s head.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
        if (affected_by_spell(victim, POISON_SKELLEBAIN_2)) {
            affect_from_char(victim, POISON_SKELLEBAIN);
            act("A cool feeling tingles through your body, and your mind" " calms down.", FALSE,
                victim, 0, 0, TO_CHAR);
            act("$n blinks several times, as if clearing $s head.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
        if (affected_by_spell(victim, POISON_SKELLEBAIN_3)) {
            affect_from_char(victim, POISON_SKELLEBAIN);
            act("A cool feeling tingles through your body, and your mind" " calms down.", FALSE,
                victim, 0, 0, TO_CHAR);
            act("$n blinks several times, as if clearing $s head.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
        if (affected_by_spell(victim, POISON_METHELINOC)) {
            affect_from_char(victim, POISON_METHELINOC);
            act("A feeling of warmth enters your body, and " "soothes your aches.", FALSE, victim,
                0, 0, TO_CHAR);
            act("$n stops shivering and begins to relax.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
        if (affected_by_spell(victim, POISON_TERRADIN)) {
            affect_from_char(victim, POISON_TERRADIN);
            act("A feeling of warmth enters your body, and " "soothes your stomach.", FALSE, victim,
                0, 0, TO_CHAR);
            act("$n stops shaking and begins to regain $s color.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
        if (affected_by_spell(victim, POISON_PERAINE)) {
            affect_from_char(victim, POISON_PERAINE);
            act("A relaxing warmth soothes your contracted muscles.", FALSE, victim, 0, 0, TO_CHAR);
            act("$n twitches, and begins to move again.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
        if (affected_by_spell(victim, POISON_HERAMIDE)) {
            affect_from_char(victim, POISON_HERAMIDE);
            act("A jolt of energy fills you, removing the numbness within your" " head.", FALSE,
                victim, 0, 0, TO_CHAR);
            act("$s's sleep appears to lighten.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
        if (affected_by_spell(victim, POISON_PLAGUE)) {
            affect_from_char(victim, POISON_PLAGUE);
            act("A soothing wave of tranquility overcomes you.", FALSE, victim, 0, 0, TO_CHAR);
            act("$n stops sweating and begins to regain $s color.", FALSE, victim, 0, 0, TO_ROOM);
            return;
        }
    } else if (obj) {
        act("$n places $s hand over $p extracting a dark aura from it.", FALSE, ch, obj, 0,
            TO_ROOM);
        act("You place your hand over $p extracting a dark aura from it.", FALSE, ch, obj, 0,
            TO_CHAR);

        if (obj->obj_flags.type == ITEM_DRINKCON) {
            if (obj->obj_flags.value[5] > 0) {
                obj->obj_flags.value[5] = 0;
                return;
            }
        }

        if ((obj->obj_flags.type == ITEM_FOOD) || (obj->obj_flags.type == ITEM_DART)) {
            if (obj->obj_flags.value[3] > 0) {
                obj->obj_flags.value[3] = 0;
                return;
            }
        }

        if (obj->obj_flags.type == ITEM_WEAPON) {
            if (obj->obj_flags.value[5] > 0) {
                obj->obj_flags.value[5] = 0;
                return;
            }
        }
    }
}                               /* end Spell_Cure_Poison */


/* CURSE: Weakens the target, affecting his or her offense
 * and defense.  Not cumulative. Drovian and sorcerer spell.
 * Drov psiak chran.
 */
void
spell_curse(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Curse */
    int duration;
    int dam;

    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in curse");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
        act("The shadows around $N solidify briefly, blocking your spell.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("The shadows around you solidify briefly.", FALSE, ch, 0, victim, TO_VICT);
        act("The shadows around $N solidify briefly.", FALSE, ch, 0, victim, TO_NOTVICT);
        return;
    }

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (IS_IMMORTAL(victim))
        return;

    if (affected_by_spell(victim, SPELL_CURSE)) {
        act("$N is already cursed - nothing happens.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    duration = number(level, ((level + 1) * 2));

    if (ch != victim) {
        if (!does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
            act("You cast a powerful curse upon $N.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n gestures towards you, and you feel clumsy.", FALSE, ch, 0, victim, TO_VICT);
            act("$n gestures at $N and a dark aura suddenly descends on $M.", FALSE, ch, 0, victim,
                TO_NOTVICT);
        } else {
            act("You gesture at $N, but nothing happens.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n gestures towards you, but nothing happens.", FALSE, ch, 0, victim, TO_VICT);
            act("$n gestures at $N, but nothing happens.", FALSE, ch, 0, victim, TO_NOTVICT);
            return;
        }
    } else
        act("You suddenly feel very clumsy.", FALSE, ch, 0, victim, TO_VICT);

    dam = (GET_OFFENSE(victim) > (level * 5)) ? (level * 5) : GET_OFFENSE(victim);

    af.type = SPELL_CURSE;
    af.power = level;
    af.magick_type = magick_type;
    af.duration = duration;
    af.modifier = -dam;
    af.location = CHAR_APPLY_OFFENSE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    dam = (GET_DEFENSE(victim) > (level * 5)) ? (level * 5) : GET_DEFENSE(victim);

    af.type = SPELL_CURSE;
    af.power = level;
    af.magick_type = magick_type;
    af.duration = duration;
    af.modifier = -dam;
    af.location = CHAR_APPLY_DEFENSE;
    af.bitvector = 0;
    affect_to_char(victim, &af);
}                               /* end Spell_Curse */

void spell_recite(byte level, sh_int magick_type, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj);
/* DEAD SPEAK: dead speak
 * Void Elementalist spell.  Nilaz morz chran.
 */
void
spell_dead_speak(byte level, sh_int magick_type,        /* level of spell */
                 CHAR_DATA * ch,        /* the caster     */
                 CHAR_DATA * victim,    /* target char    */
                 OBJ_DATA * corpse)
{                               /* corpse item    *//* begin Spell_Dead_Speak */
    CHAR_DATA *mob;
    CHAR_DATA *targch;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    check_criminal_flag(ch, NULL);

    /*  This was coming up true all the time, even when cast on a corpse */
    if (!corpse) {
        gamelog("A non object got passed to spell_dead_speak");
        return;
    }

    targch = ch->specials.contact;
    if (!targch) {
        cprintf(ch, "You must be in contact with someone to cause the dead to speak for them.\n\r");
        return;
    }

    if ((GET_ITEM_TYPE(corpse) != ITEM_CONTAINER)
        || (!(corpse->obj_flags.value[3]))
        || (strstr(OSTR(corpse, short_descr), "the undead "))) {
        send_to_char("Your magick seems to not affect it.\n\r", ch);
        return;
    }

    act("You lean over $p and grow drenched with perspiration.", TRUE, ch, corpse, 0, TO_CHAR);
    act("$n becomes drenched with the sweat of effort as $e leans over $p.", TRUE, ch, corpse, 0, TO_ROOM);
    act("Your skin crawls for no apparent reason.", TRUE, ch, 0, 0, TO_NEARBY_ROOMS);

    mob = raise_corpse_helper(corpse, ch, magick_type, level);
    if (!mob) {
        gamelogf("raise_corpse_helper failed in %s!", __FUNCTION__);
        return;
    }

    extract_obj(corpse);

    return spell_recite(level, magick_type, ch, mob, NULL);
}                               /* end dead_speak */


/* DEAFNESS: Makes the target unable to hear, for a duration
 * dependent on power level.  Cumulative.  Vivaduan spell.
 * Vivadu iluth hurn.
 */
void
spell_deafness(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Deafness */
    int duration;
    int sector;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    check_criminal_flag(ch, NULL);

    if IS_IMMORTAL
        (victim)
            return;

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* Switch so durations can vary according to sector */
    sector = ch->in_room->sector_type;

    switch (sector) {
    case SECT_CAVERN:
    case SECT_SILT:
        duration = level * 2;
        break;
    default:
        duration = level;
        break;
    }

    af.type = SPELL_DEAFNESS;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_AGL;
    af.modifier = -3;
    af.bitvector = 0;
    af.bitvector = CHAR_AFF_DEAFNESS;

    /*  af.bitvector = CHAR_AFF_DEAFNESS;  -checking how necessary this is */
    /*  Whoever took this out...it's very necessary, the spell hasn't been
     * working in a long time, when I finally looked at it I found this.
     * When you're testing something, please place your name next to it if
     * you don't plan to keep an eye on it.  -Nessalin                   */

    stack_spell_affect(victim, &af, 24);

    act("You gesture towards $N and $E gazes around quizzically, seeming unable to hear.", FALSE,
        ch, 0, victim, TO_CHAR);
    act("$n gestures towards $N and $E gazes around quizzically, seeming unable to hear.", TRUE, ch,
        0, victim, TO_NOTVICT);

    /* Switch so casting messages vary according to sector */
    sector = ch->in_room->sector_type;

    switch (sector) {
    case SECT_CAVERN:
        act("The echoes of sounds against the rocky walls grow "
            "louder and louder, deafening you as $n gestures at you.", FALSE, ch, 0, victim,
            TO_VICT);
        break;
    case SECT_CITY:
        act("The noises of the crowded city grow louder and louder, "
            "echoing inside your head painfully, deafening you as $n gestures.", FALSE, ch, 0,
            victim, TO_VICT);
        break;
    case SECT_DESERT:
        act("The sound of the winds whispering over the sand gradually "
            "grows louder and louder, deafening you as $n gestures.", FALSE, ch, 0, victim,
            TO_VICT);
        break;
    case SECT_THORNLANDS:
        act("The sound of the winds whispering through the thorny bushes "
            "grows louder and louder, deafening you as $n gestures.", FALSE, ch, 0, victim,
            TO_VICT);
        break;
    default:
        act("The sounds around you slowly fade away as $n gestures.", FALSE, ch, 0, victim,
            TO_VICT);
        break;
    }
}                               /* end Spell_Deafness */


/* DELUSION: Allows the caster to create an amusing and
 * sometimes alarming illusion.  Mon and sul level turn the caster
 * invisible as well.  Effect is amplified if cast in SECT_AIR;
 * diminished if cast in SECT_SILT.  Cannot be cast while already
 * invisible.  Sanvean and Daigon. 12/2/2000  Basically, a whiran 
 * version of pyrotechnics.  Whira iluth nikiz.  
 */
void
spell_delusion(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Delusion */
    int effect = number(1, 6);  /* random message according to power level */
    char msg1[256], msg2[256];  /* message to room and caster              */
    struct affected_type af;    /* for the invisibility effect             */

    memset(&af, 0, sizeof(af));

    check_crim_cast(ch);

    /* Make it so spell level diminished  while invisible.            */
    /* (Because many of the effects at mon/sul depend on fading away. */
    if (affected_by_spell(ch, SPELL_INVISIBLE))
        level = level - 2;

    /* Effect heightened if cast in SECT_AIR */
    if (ch->in_room->sector_type == SECT_AIR) {
        level = level + 2;
        /* since there is no case statement at 8 or 9, although 
         * this would be a cool addition later on               */
        if (level >= 7)
            level = 7;
    }

    /* Effect diminished if cast in SECT_SILT */
    if (ch->in_room->sector_type == SECT_SILT)
        level = level - 2;

    if (level == 7) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "$n fades, till all that is left is $s grin, hovering in the air, which vanishes in turn.");
            strcpy(msg2, "You fade slowly away.");
            break;
        case 2:
            strcpy(msg1, "Winds rush inward towards $n, carrying $m away in a swirl of sand.");
            strcpy(msg2, "Winds rush inward towards you, sand swirling as you fade from view.");
            break;
        case 3:
            strcpy(msg1,
                   "Winds swirl around $n at an increasing rate, a column of wind enveloping $m.");
            strcpy(msg2,
                   "Winds swirl around you at an increasing rate, a column of wind enveloping you.");
            break;
        case 4:
            strcpy(msg1, "An immense tornado forms before $m, spinning upward.");
            strcpy(msg2, "An immense tornado forms before you, spinning upward.");
            break;
        case 5:
            strcpy(msg1,
                   "Glassy serpents writhe in the air, coiling about $n till $e is no longer visible.");
            strcpy(msg2,
                   "Glassy serpents writhe in the air, coiling about you until you are hidden.");
            break;
        case 6:
            strcpy(msg1, "$n vanishes, and winds rush in to fill the vacuum left by $s form.");
            strcpy(msg2, "You vanish, winds swirling around you.");
            break;
        default:
            strcpy(msg1,
                   "$n fades, till all that is left is $s grin, hovering in the air, which vanishes in turn.");
            strcpy(msg2, "You fade slowly away.");
            break;
        }
    } else if (level == 6) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "Particles of dust and sand whirl through the air around $n as $e vanishes.");
            strcpy(msg2,
                   "Particles of dust and sand whirl through the air around you as you vanish.");
            break;
        case 2:
            strcpy(msg1,
                   "$n seems to waver, then blows away like smoke with a sudden, fierce gust of wind.");
            strcpy(msg2, "You waver, then vanish in the wind.");
            break;
        case 3:
            strcpy(msg1,
                   "A whistle sounds on the wind, an odd medley of three notes, as $n vanishes.");
            strcpy(msg2,
                   "A whistle sounds on the wind, an odd medley of three notes, as you vanish.");
            break;
        case 4:
            strcpy(msg1, "$n fades from sight, wind driven sand obscuring $s form.");
            strcpy(msg2, "You fade from sight, wind driven sand obscuring your form.");
            break;
        case 5:
            strcpy(msg1, "$n vanishes, fading from $s feet up towards $s head.");
            strcpy(msg2, "You vanish, fading from your feet up towards your head.");
            break;
        case 6:
            strcpy(msg1, "In a whirl of dust and wind, $n vanishes.");
            strcpy(msg2, "You vanish in a swirl of wind.");
            break;
        default:
            strcpy(msg1,
                   "Particles of dust and sand whirl through the air around $n as $e vanishes.");
            strcpy(msg2,
                   "Particles of dust and sand whirl through the air around you as you vanish.");
            break;
        }
    } else if (level == 5) {
        switch (effect) {
        case 1:
            strcpy(msg1, "All around $n, winds swirl, stinging sand in their wake.");
            strcpy(msg2, "All around you, winds swirl, stinging sand in their wake.");
            break;
        case 2:
            strcpy(msg1, "Fierce gusts of wind blow up debris around $n, obscuring the air.");
            strcpy(msg2, "Fierce gusts of wind blow up debris around you, obscuring the air.");
            break;
        case 3:
            strcpy(msg1, "A tornado forms before $m, spinning upward.");
            strcpy(msg2, "A tornado forms before you, spinning upward.");
            break;
        case 4:
            strcpy(msg1, "Pelting grains of sand, driven by the wind, strike you harshly.");
            strcpy(msg2, "Pelting grains of sand, driven by the wind, strike you harshly.");
            break;
        case 5:
            strcpy(msg1,
                   "Winds swirl around $n, lifting $m up a few inches above the ground briefly.");
            strcpy(msg2,
                   "Winds swirl around you, lifting you up a few inches above the ground briefly.");
            break;
        case 6:
            strcpy(msg1, "Winds spin and dance around $n, throwing out dust and sand.");
            strcpy(msg2, "Winds spin and dance around you, throwing out dust and sand.");
            break;
        default:
            strcpy(msg1, "All around $n, winds swirl, stinging sand in their wake.");
            strcpy(msg2, "All around you, winds swirl, stinging sand in their wake.");
            break;
        }
    } else if (level == 4) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Shrill voices rise on the wind that swirls around $n, crying out.");
            strcpy(msg2, "Shrill voices rise on the wind that swirls around you, crying out.");
            break;
        case 2:
            strcpy(msg1, "Parts of $n begin to phase in and out of existence.");
            strcpy(msg2, "Parts of you begin to phase in and out of existence.");
            break;
        case 3:
            strcpy(msg1, "A whirling dervish of wind spins around $n.");
            strcpy(msg2, "A whirling dervish of wind spins around you.");
            break;
        case 4:
            strcpy(msg1, "A haze of sand fills the air, created by the winds spinning around $n.");
            strcpy(msg2, "A haze of sand fills the air, created by the winds spinning around you.");
            break;
        case 5:
            strcpy(msg1, "All around $n, dustdevils rise, spinning up out of the ground.");
            strcpy(msg2, "All around you, dustdevils rise, spinning up out of the ground.");
            break;
        case 6:
            strcpy(msg1, "A clamor of voices babbles through the air, deafening you.");
            strcpy(msg2, "A tumultuous clamor of voices babbles through the air, deafening you.");
            break;
        default:
            strcpy(msg1, "Shrill voices rise on the wind that swirls around $n, crying out.");
            strcpy(msg2, "Shrill voices rise on the wind that swirls around you, crying out.");
            break;
        }
    } else if (level == 3) {
        switch (effect) {
        case 1:
            strcpy(msg1, "The wind whispers, a soft and almost understandable sentence.");
            strcpy(msg2, "The wind whispers, a soft and almost understandable sentence.");
            break;
        case 2:
            strcpy(msg1, "A gust of wind sends sand pelting across the area.");
            strcpy(msg2, "A gust of wind sends sand pelting across the area.");
            break;
        case 3:
            strcpy(msg1, "Particles of dust and sand whirl through the air around $n.");
            strcpy(msg2, "Particles of dust and sand whirl through the air around you.");
            break;
        case 4:
            strcpy(msg1, "Sand obscures your vision as winds dance around $n.");
            strcpy(msg2, "Winds dance around you, sand filling the air.");
            break;
        case 5:
            strcpy(msg1, "A flurry of sand pelts across the surroundings, hurled by the wind.");
            strcpy(msg2, "A flurry of sand pelts across the surroundings, hurled by the wind.");
            break;
        case 6:
            strcpy(msg1, "The wind whistles, a sad and lonely song, as it stirs around $n.");
            strcpy(msg2, "The wind whistles, a sad and lonely song, as it stirs around you.");
            break;
        default:
            strcpy(msg1, "A gust of wind sends sand pelting across the area.");
            strcpy(msg2, "A gust of wind sends sand pelting across the area.");
            break;
        }
    } else if (level == 2) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Dust devils careen through the air in a dizzying dance around $n.");
            strcpy(msg2, "Dust devils careen through the air in a dizzying dance around you.");
            break;
        case 2:
            strcpy(msg1, "$n's form becomes translucent, wavering in and out of view.");
            strcpy(msg2, "Your form becomes translucent, wavering in and out of view.");
            break;
        case 3:
            strcpy(msg1,
                   "Color seeps slowly from $n's form, rendering $m a pallid white before it returns.");
            strcpy(msg2,
                   "Color seeps slowly from your form, rendering it a pallid white before it returns.");
            break;
        case 4:
            strcpy(msg1, "A babble of voices arises on the wind, confusing and delirious.");
            strcpy(msg2, "A babble of voices arises on the wind, confusing and delirious.");
            break;
        case 5:
            strcpy(msg1, "Fierce breezes blow around $n, whirling and ebbing.");
            strcpy(msg2, "Fierce breezes blow around you, whirling and ebbing.");
            break;
        case 6:
            strcpy(msg1, "Winds swirl outward from $n in a fierce gust.");
            strcpy(msg2, "Winds swirl outward from you in a fierce gust.");
            break;
        default:
            strcpy(msg1, "Dust devils careen through the air in a dizzying dance around $n.");
            strcpy(msg2, "Dust devils careen through the air in a dizzying dance around you.");
            break;
        }
    } else {
        switch (effect) {
        case 1:
            strcpy(msg1, "A slight breeze stirs $n's hair.");
            strcpy(msg2, "A slight breeze stirs your hair.");
            break;
        case 2:
            strcpy(msg1, "A whisper of wind dances around $n.");
            strcpy(msg2, "A whisper of wind dances around you.");
            break;
        case 3:
            strcpy(msg1, "Wind eddies near $n, stirring dust up around $m.");
            strcpy(msg2, "Wind eddies near you, stirring dust up.");
            break;
        case 4:
            strcpy(msg1, "The wind whispers something intelligible, its tone soft but urgent.");
            strcpy(msg2, "The wind whispers something intelligible, its tone soft but urgent.");
            break;
        case 5:
            strcpy(msg1, "Sand stirs and sifts across the ground, stirred by the wind.");
            strcpy(msg2, "Sand stirs and sifts across the ground, stirred by the wind.");
            break;
        case 6:
            strcpy(msg1, "Breezes dance in swirls around $n.");
            strcpy(msg2, "Breezes dance in swirls around you.");
            break;
        default:
            strcpy(msg1, "A slight breeze stirs $n's hair.");
            strcpy(msg2, "A slight breeze stirs your hair.");
            break;
        }
    }

    act(msg1, FALSE, ch, 0, 0, TO_ROOM);
    act(msg2, FALSE, ch, 0, 0, TO_CHAR);

/* If cast at mon or sul, caster turns invisible as well.  */
    if (level >= 6) {
        af.type = SPELL_INVISIBLE;
        af.duration = level;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = 0;
        af.location = CHAR_APPLY_NONE;
        af.bitvector = CHAR_AFF_INVISIBLE;
        affect_to_char(ch, &af);
    }
}                               /* end Spell_Delusion */


/* DEMONFIRE: Directs fire at target.  Requires a component
 * Krathi spell. Suk-Krath morz hekro.
 */
void
spell_demonfire(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Demonfire */
    int dam;

    if (level < 1)
        level = 1;

    if (!ch) {
        gamelog("No ch in demonfire");
        return;
    }

    if (!victim) {
        gamelog("No victim in demonfire");
        return;
    }

    if (level <= 0) {
        gamelogf("demonfire level was <= 0 (%d)", level);
        return;
    }

    check_criminal_flag(ch, NULL);

    if (!get_componentB
        (ch, SPHERE_NECROMANCY, level, "$p sparks and bursts into flames.",
         "$p sparks and bursts into flames in $n's hands",
         "Your hands heat up, then grow instantly cool - something is missing."))
        return;

    dam = dice(level * 3, 8);
    qroomlogf(QUIET_COMBAT, ch->in_room, "demonfire base dam:  %d", dam);

    if (!(affected_by_spell(victim, SPELL_FIRE_ARMOR)))
        sflag_to_char(victim, OST_BURNED);

    if (ch->in_room->sector_type == SECT_WATER_PLANE) {
        send_to_char("The waters in this area seems to dampen the affects of your spell.\n\r", ch);
        dam = dam / 2;
    }

    if (affected_by_spell(victim, SPELL_FIRE_ARMOR))
        dam = dam / 2;

    qroomlogf(QUIET_COMBAT, ch->in_room, "demonfire final dam: %d", dam);
    if (victim == ch) {
        act("$n is engulfed in a sheet of white flame.", FALSE, ch, 0, 0, TO_ROOM);
        act("You are engulfed in a sheet of white flame!", FALSE, ch, 0, 0, TO_CHAR);
        generic_damage(victim, dam, 0, 0, 0);
    } else
        ws_damage(ch, victim, dam, 0, 0, dam, SPELL_DEMONFIRE, TYPE_ELEM_FIRE);

    if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
        send_to_char("The flames protecting you flare briefly, absoring some of the attack..\n\r",
                     victim);
        act("The flames surrounding $n flare briefly, absorbing some of the attack.", FALSE, victim,
            0, 0, TO_ROOM);
    }

    act("You see a blinding flash of white light to the $D.", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);
}                               /* end Spell_Demonfire */


/* DETECT ETHEREAL: Allows the caster to see things on the
 * ethereal plane for a duration dependent on power level. 
 * Non-cumulative.  Requires a component.  Drovian and 
 * sorcerer spell.  Drov threl inrof.
 */
void
spell_detect_ethereal(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                      OBJ_DATA * obj)
{                               /* begin Spell_Detect_Ethereal */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in detect ethereal");
        return;
    }

    check_crim_cast(ch);

    if (ch->in_room->sector_type == SECT_SILT) {
        send_to_char("Particles of swirling silt darken your vision.\n\r", ch);
        return;
    }

    if (ch->in_room->sector_type != SECT_SHADOW_PLANE) {
        if (!get_componentB
            (ch, SPHERE_DIVINATION, level - 2, "$p unravels into infinite particles.",
             "$p seemingly unravels in $n's hands.",
             "You are lacking the component necessary for extraplanar vision."))
            return;
    }

    duration = level * 3;

    af.type = SPELL_DETECT_ETHEREAL;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_DETECT_ETHEREAL;
    stack_spell_affect(victim, &af, 36);

    send_to_char("You feel more perceptive of the shadow plane.\n\r", victim);
}                               /* end Spell_Detect_Ethereal */


/* DETECT INVISIBILITY: Allows the target to see the invisible.
 * Cumulative.  Whiran and sorcerer spell.  Whira threl inrof.
 */
void
spell_detect_invisibility(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                          OBJ_DATA * obj)
{                               /* begin Spell_Detect_Invisible */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim)
        return;

    check_crim_cast(ch);

    if (ch->in_room->sector_type == SECT_SILT) {
        send_to_char("Particles of swirling silt darken your vision.\n\r", ch);
        return;
    }

    duration = level * 5;

    af.type = SPELL_DETECT_INVISIBLE;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_DETECT_INVISIBLE;
    stack_spell_affect(victim, &af, 48);

    send_to_char("Your eyes tingle.\n\r", victim);
}                               /* end Spell_Detect_Invisible */


/* DETECT MAGICK: Allows the recipient to see magickal affects.
 * Cumulative. Suk-Krathi and sorcerer spell.
 * Suk-Krath threl inrof.
 */
void
spell_detect_magick(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Detect_Magick */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        errorlog("spell_detect_magick in magick.c was passed a null value in victim.");
        return;
    }

    check_crim_cast(ch);

    if (ch->in_room->sector_type == SECT_SILT) {
        send_to_char("Particles of swirling silt darken your vision.\n\r", ch);
        return;
    }

    duration = level * 5;

    af.type = SPELL_DETECT_MAGICK;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_DETECT_MAGICK;
    stack_spell_affect(victim, &af, 48);

    send_to_char("You feel in tune with the elements.\n\r", victim);
}                               /* end Spell_Detect_Magick */


/* DETECT POISON: Cast on a target person, it will confirm whether
 * or not they are poisoned and the color of the aura will indicate
 * what sort of poison it is.  Will also detect poison on drink
 * containers, food, or weapons and indicate if the target item is
 * itself poisonous in nature.  Vivaduan and sorcerer spell.
 * Vivadu threl inrof. 
 */
void
spell_detect_poison(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Detect_Poison */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        errorlog("No victim in detect poison");
        return;
    }

    check_crim_cast(ch);

    if (ch->in_room->sector_type == SECT_SILT) {
        send_to_char("Particles of swirling silt darken your vision.\n\r", ch);
        return;
    }

    duration = level * 5;

    af.type = SPELL_DETECT_POISON;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_DETECT_POISON;
    stack_spell_affect(victim, &af, 48);

    send_to_char("Your eyes glow brown for a moment.\n\r", victim);
}                               /* end Spell_Detect_Poison */

/* DETERMINE RELATIONSHIP: Allows the caster to determine the
 * target's relationship to the Land.  Cast on vampires or targets
 * that have the UNDEAD flag, it reports that they seem to have no
 * relationship.  Vivaduan spell.  Vivadu nathro inrof.
 */

void
spell_determine_relationship(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                             OBJ_DATA * obj)
{                               /* begin Spell_Determine_Relationship */
    char buf[256];
    char bufs[256];

    if (!victim) {
        gamelog("No victim in determine relationship");
        return;
    }

    check_crim_cast(ch);

    if ((GET_RACE(victim) == RACE_VAMPIRE) || (IS_SET(victim->specials.act, CFL_UNDEAD)))
        act("$N has no discernable relationship, as " "though $E were a stone or a piece of wood.",
            FALSE, ch, 0, victim, TO_CHAR);
    else {
        sprintf(buf, "$N's relationship to the land is ");

        if ((victim->specials.eco < 10) && (victim->specials.eco > -10))
            sprintf(bufs, "neutral");
        else if ((victim->specials.eco < 50) && (victim->specials.eco >= 10))
            sprintf(bufs, "good");
        else if ((victim->specials.eco < 90) && (victim->specials.eco >= 50))
            sprintf(bufs, "potent");
        else if ((victim->specials.eco < 100) && (victim->specials.eco >= 90))
            sprintf(bufs, "nearly a oneness.");
        else if (victim->specials.eco == 100)
            sprintf(bufs, "as one.");
        else if ((victim->specials.eco > -50) && (victim->specials.eco <= -10))
            sprintf(bufs, "harmful");
        else if ((victim->specials.eco > -90) && (victim->specials.eco <= -50))
            sprintf(bufs, "destructive");
        else if ((victim->specials.eco > -100) && (victim->specials.eco <= -90))
            sprintf(bufs, "almost malign");
        else if (victim->specials.eco == -100)
            sprintf(bufs, "malign");

        strcat(buf, bufs);

        act(buf, FALSE, ch, 0, victim, TO_CHAR);
    }
}                               /* begin Spell_Determine_Relationship */




/* DISEMBODY: Forces the target out of their body.  They are then bound
 * to the room and unable to leave it until the spell wears off.  Doesn't
 * affect golems or the undead for what are, hopefully, obvious, reasons.
 * Nilazi spell.  Vivadu nathro inrof.
 */

void
spell_disembody(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Disembody */
    struct affected_type af;
    CHAR_DATA *shadow;
    int i;
    char *f;
    char shadow_name[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in disembody");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (!victim->desc)
        return;

    if (victim->desc->snoop.snooping || victim->desc->snoop.snoop_by) {
        send_to_char("The magick fizzles and dies.\n\r", ch);
        return;
    }

    if (GET_RACE(victim) == RACE_VAMPIRE || GET_RACE(victim) == RACE_GOLEM
        || IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("You experience a tingling sensation which quickly passes.", FALSE, victim, 0, 0,
            TO_CHAR);
        if (victim != ch)
            send_to_char("Nothing seems to happen.\n\r", ch);
    }


    shadow = read_mobile(1206, VIRTUAL);

    if (!shadow) {
        send_to_char("The magick fizzles and dies. (no mobile loaded).\n\r", ch);
        errorlog("spell_disembody() in magick.c unable to load NPC");
        return;
    }

    char_to_room(shadow, victim->in_room);

    act("$N gasps suddenly and collapses.", FALSE, ch, 0, victim, TO_ROOM);
    act("$N gasps suddenly and collapses.", FALSE, ch, 0, victim, TO_CHAR);
    act("You gasp as a sudden coldness fills your body.", FALSE, ch, 0, victim, TO_VICT);

    if (!victim->desc->original)
        victim->desc->original = victim;
    victim->desc->character = shadow;

    victim->desc->original = victim;

    shadow->desc = victim->desc;
    victim->desc = 0;

    /* Give the shadow the extra hidden keyword of the occupant */
#define ADD_NAME_TO_SHADOW
#ifdef ADD_NAME_TO_SHADOW
    sprintf(shadow_name, "%s [%s]", MSTR(shadow, name), MSTR(victim, name));

    f = strdup(shadow_name);
    free((char *) shadow->name);
    shadow->name = f;

    /* Also a good spot to move this "shadow" char onto the lists of those who
     * are monitoring the char */

#endif

    set_hit(shadow, GET_MAX_HIT(shadow));
    set_move(shadow, GET_MAX_MOVE(shadow));
    set_stun(shadow, GET_MAX_STUN(shadow));

    /* Give psionics & languages to shadow */
    for (i = 0; i < MAX_SKILLS; i++) {
        if ((victim->skills[i] && skill[i].sk_class == CLASS_PSIONICS)
            || (victim->skills[i] && skill[i].sk_class == CLASS_LANG)) {
            /* so they can't shadow walk in send-shadow form */
            if (i == PSI_SHADOW_WALK)
                continue;
            if (!shadow->skills[i])
                init_skill(shadow, i, victim->skills[i]->learned);
            else
                shadow->skills[i]->learned = victim->skills[i]->learned;
        }
    }

    shadow->specials.language = victim->specials.language;

    af.type = SPELL_SEND_SHADOW;
    af.power = level;
    af.duration = level * 3;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_SUMMONED;
    af.modifier = 0;
    affect_to_char(shadow, &af);

    affect_join(shadow, &af, TRUE, FALSE);

    af.type = SPELL_INFRAVISION;
    af.power = level;
    af.duration = level * 3 + 1;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_INFRAVISION;
    af.modifier = 0;

    affect_to_char(shadow, &af);

    af.type = SPELL_PARALYZE;
    af.power = level;
    af.duration = level * 3 + 1;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_CFLAGS;
    af.modifier = CFL_FROZEN;
    af.bitvector = 0;

    gamelog("Adding freeze bit to player in spell_disembody");
    affect_to_char(shadow, &af);


}                               /* begin Spell_Disembody */


/* DISPEL MAGICK: Cast on an item or person, it removes all
 * magickal affects.  Cast on undead or summoned creatures, it
 * completely removes them.  
 * Note: currently the help file indicates
 * that higher power levels are necessary to dispel stronger
 * spells, although this is not the case.  It would be cool to
 * put a check for duration in and have the effectiveness of
 * the spell dependent on level, or (more complicated) to have
 * a weaker dispel magick lessen the duration if not able to 
 * completely negate the spell.  Suk Krathi and sorcerer spell.
 * Suk-Krath psiak echri.
 */
bool dispel_ethereal_obj(CHAR_DATA * ch, OBJ_DATA * obj);
bool dispel_ethereal_ch(CHAR_DATA * ch, CHAR_DATA * victim, struct affected_type *af, byte level);
bool dispel_invisibility_obj(CHAR_DATA * ch, OBJ_DATA * obj);
bool dispel_invisibility_ch(CHAR_DATA * ch, CHAR_DATA * victim, struct affected_type *af,
                            byte level);

void
spell_dispel_magick(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Dispel_Magick */
    int yes = 0, i;
    int counter = 0;
    int dir, alive;
    char buf[256];
    struct affected_type *tmp_af = 0;
    struct affected_type *next_af = 0;

    ROOM_DATA *other_room;
    OBJ_DATA *next_obj;
    struct room_direction_data *back;
    char logbuf[MAX_STRING_LENGTH];
    char e_desc[MAX_STRING_LENGTH];

    if (obj) {                  /* if the target is an object */

        act("A fiery light sparks from your hands into $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("A fiery light sparks from $n's hands into $p.", TRUE, ch, obj, 0, TO_ROOM);

        check_crim_cast(ch);    /* dispelling an object is not considered aggressive */

        if (isnamebracketsok("[fire_jambiya]", OSTR(obj, name))) {
            act("$p disappears in a puff of smoke.", FALSE, ch, obj, 0, TO_ROOM);
            act("$p disappears in a puff of smoke.", FALSE, ch, obj, 0, TO_CHAR);
            extract_obj(obj);
            return;
        }

        if (isnamebracketsok("[sand_jambiya]", OSTR(obj, name))) {
            act("$p comes undone and falls into a small pile of sand.", FALSE, ch, obj, 0, TO_ROOM);
            act("$p comes undone and falls into a small pile of sand.", FALSE, ch, obj, 0, TO_CHAR);
            extract_obj(obj);
            return;
        }

        if (isnamebracketsok("[lightning_whip]", OSTR(obj, name))) {
            act("$p explodes in a shower of sparks.", FALSE, ch, obj, 0, TO_ROOM);
            act("$p explodes in a shower of sparks.", FALSE, ch, obj, 0, TO_CHAR);
            extract_obj(obj);
            return;
        }

        if (isnamebracketsok("[shadow_sword]", OSTR(obj, name))) {
            act("$p winks out of existence.", FALSE, ch, obj, 0, TO_ROOM);
            act("$p winks out of existence.", FALSE, ch, obj, 0, TO_CHAR);
            extract_obj(obj);
            return;
        }

        if (isnamebracketsok("[shield_of_mist]", OSTR(obj, name))) {
            act("$p suddenly dissolves in a loud hiss.", FALSE, ch, obj, 0, TO_ROOM);
            act("$p suddenly dissolves in a loud hiss.", FALSE, ch, obj, 0, TO_CHAR);
            extract_obj(obj);
            return;
        }

        if (isnamebracketsok("[shield_of_wind]", OSTR(obj, name))) {
            act("$p quickly stops swirling and winks out of existance.", FALSE, ch, obj, 0,
                TO_ROOM);
            act("$p quickly stops swirling and winks out of existence.", FALSE, ch, obj, 0,
                TO_CHAR);
            extract_obj(obj);
            return;
        }

        if (find_ex_description("[SPELL_VAMPIRIC_BLADE]", obj->ex_description, TRUE)) {
            act("$p quickly dissolves into a fine, grey ash.", FALSE, ch, obj, 0, TO_ROOM);
            act("$p quickly dissolves into a fine, grey ash.", FALSE, ch, obj, 0, TO_CHAR);
            extract_obj(obj);
            return;
        }

        if (isnamebracketsok("oasis_spellobj", OSTR(obj, name))) {
            act("$p seeps into the ground, vanishing from sight.", FALSE, ch, obj, 0, TO_ROOM);
            act("$p seeps into the ground, vanishing from sight.", FALSE, ch, obj, 0, TO_CHAR);

            extract_obj(obj);
            return;
        }

	// Deal with player-created runes
        if (isnamebracketsok("[teleport_rune_spell]", OSTR(obj, name))) {
            act("$p dissolve into thin air.", FALSE, ch, obj, 0, TO_ROOM);
            act("$p dissolve into thin air.", FALSE, ch, obj, 0, TO_CHAR);
            extract_obj(obj);
            return;
        }

	// Protect the immortal-created runes
        if (obj->obj_flags.type == ITEM_TELEPORT) {
            send_to_char("Your magick was not powerful enough to remove the runes.\n\r", ch);
            return;
	}

        if (IS_SET(obj->obj_flags.extra_flags, OFL_INVISIBLE)) {
            REMOVE_BIT(obj->obj_flags.extra_flags, OFL_INVISIBLE);
            send_to_char("You remove its invisibility.\n\r", ch);
            counter += 1;
        }
        if (IS_SET(obj->obj_flags.extra_flags, OFL_GLYPH)) {
            if (level <= 2) {
                send_to_char("Your magick was not powerful enough to remove the glyph.\n\r", ch);
                counter += 1;
                return;
            } else {
                REMOVE_BIT(obj->obj_flags.extra_flags, OFL_GLYPH);
                send_to_char("You remove the glyph.\n\r", ch);
                counter += 1;
            }
        }
        if (IS_SET(obj->obj_flags.extra_flags, OFL_HUM)) {
            REMOVE_BIT(obj->obj_flags.extra_flags, OFL_HUM);
            send_to_char("It stops humming.\n\r", ch);
            counter += 1;
        }
        if (IS_SET(obj->obj_flags.extra_flags, OFL_GLOW)) {
            REMOVE_BIT(obj->obj_flags.extra_flags, OFL_GLOW);
            send_to_char("It stops glowing.\n\r", ch);
            counter += 1;
        }
        if (IS_SET(obj->obj_flags.extra_flags, OFL_NO_FALL)) {
            // Not going to worry about a message here
            REMOVE_BIT(obj->obj_flags.extra_flags, OFL_NO_FALL);
            if (!obj->nr)
                obj->obj_flags.weight = obj_default[obj->nr].weight;
        }

        if (dispel_ethereal_obj(ch, obj)) {
            counter += 1;
        }
        if (IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL_OK)) {
            send_to_char("You remove its shadowy hue.\n\r", ch);
            REMOVE_BIT(obj->obj_flags.extra_flags, OFL_ETHEREAL_OK);
            counter += 1;
        }
        if (IS_SET(obj->obj_flags.extra_flags, OFL_MAGIC)) {
            /* sand shelter */
            if (obj_index[obj->nr].vnum == 73002 || obj_index[obj->nr].vnum == 73003
                || obj_index[obj->nr].vnum == 73004) {
                obj->obj_flags.cost = 1;
                act("$p shudders slightly.", FALSE, ch, obj, NULL, TO_CHAR);
                act("$p shudders slightly.", FALSE, ch, obj, NULL, TO_ROOM);
                counter += 1;
            } else {
                REMOVE_BIT(obj->obj_flags.extra_flags, OFL_MAGIC);
                send_to_char("You remove its magickal blue aura.\n\r", ch);
                counter += 1;
                for (i = 0; i < MAX_OBJ_AFFECT; i++) {  /* to make sure people don't dispel off/deff for weapons */
                    obj->affected[i].location = 0;
                    obj->affected[i].modifier = 0;
                }
            }
        }

        if (obj->obj_flags.type == ITEM_LIGHT && obj->obj_flags.value[2] == LIGHT_TYPE_MAGICK) {
            act("$p shrinks until it is no longer visible.", FALSE, ch, obj, NULL, TO_ROOM);
            act("$p shrinks until it is no longer visible.", FALSE, ch, obj, NULL, TO_CHAR);
            extract_obj(obj);
            counter += 1;
        }

        if (counter == 0)
            send_to_char("There's nothing magickal about that object.\n\r", ch);
        return;
    }
    /* if target is an object: if (obj) */
    else if (victim) {          /* begin if (victim) */
        /* Immortals can always dispel immortals, changed from what we
         * have below so that imms can dispel other imms.  There are some
         * spells you cannot dispel off yourself because you cannot see
         * yourself. Nessalin -11/25/2000 */

        act("A fiery light sparks from your hands towards $N.", FALSE, ch, 0, victim, TO_CHAR);
        act("A fiery light sparks from $n's hands towards you.", FALSE, ch, 0, victim, TO_VICT);
        act("A fiery light sparks from $n's hands towards $N.", TRUE, ch, 0, victim, TO_NOTVICT);

        /* Aggressive against soldiers/templars, not so against other characters */
        if (is_soldier_in_city(victim) || is_templar_in_city(victim))
            check_criminal_flag(ch, NULL);
        else
            check_crim_cast(ch);

        /* Check their inventory */
        //      for( obj = ch->carrying; obj; obj = obj->next_content )
        obj = victim->carrying;
        while (obj) {
            next_obj = obj->next_content;
            if (obj->obj_flags.type == ITEM_LIGHT && obj->obj_flags.value[2] == LIGHT_TYPE_MAGICK) {
                act("$N's $o shrinks until it is no longer visible.", FALSE, ch, obj, victim,
                    TO_ROOM);
                act("$N's $o shrinks until it is no longer visible.", FALSE, ch, obj, victim,
                    TO_CHAR);
                if (extinguish_light(obj, victim->in_room))
                    extract_obj(obj);
            }

            else if (isnamebracketsok("[fire_jambiya]", OSTR(obj, name))) {
                act("$N's $o disappears in a puff of smoke.", FALSE, ch, obj, victim, TO_ROOM);
                act("$N's $o disappears in a puff of smoke.", FALSE, ch, obj, victim, TO_CHAR);
                extract_obj(obj);
            } else if (isnamebracketsok("[sand_jambiya]", OSTR(obj, name))) {
                act("$N's $o comes undone and falls into a small pile of sand.", FALSE, ch, obj,
                    victim, TO_ROOM);
                act("$N's $o comes undone and falls into a small pile of sand.", FALSE, ch, obj,
                    victim, TO_CHAR);
                extract_obj(obj);
            } else if (isnamebracketsok("[lightning_whip]", OSTR(obj, name))) {
                act("$N's $o explodes in a shower of sparks.", FALSE, ch, obj, victim, TO_ROOM);
                act("$N's $o explodes in a shower of sparks.", FALSE, ch, obj, victim, TO_CHAR);
                extract_obj(obj);
            } else if (isnamebracketsok("[shadow_sword]", OSTR(obj, name))) {
                act("$N's $o flickers then fades out of existance.", FALSE, ch, obj, victim,
                    TO_ROOM);
                act("$N's $o flickers then fades out of existance.", FALSE, ch, obj, victim,
                    TO_CHAR);
                extract_obj(obj);
            } else if (isnamebracketsok("[shield_of_mist]", OSTR(obj, name))) {
                act("$N's $o suddenly dissolves in a loud hiss.", FALSE, ch, obj, victim, TO_ROOM);
                act("$N's $o suddenly dissolves in a loud hiss.", FALSE, ch, obj, victim, TO_CHAR);
                extract_obj(obj);
            } else if (isnamebracketsok("[shield_of_wind]", OSTR(obj, name))) {
                act("$N's $o stop swirling and blinks from existance.", FALSE, ch, obj, victim,
                    TO_ROOM);
                act("$N's $o stop swirling and blinks from existance.", FALSE, ch, obj, victim,
                    TO_CHAR);
                extract_obj(obj);
            } else if (find_ex_description("[SPELL_VAMPIRIC_BLADE]", obj->ex_description, TRUE)) {
                act("$N's $o quickly dissolves into a fine, grey ash.", FALSE, ch, obj, victim,
                    TO_ROOM);
                act("$N's $o quickly dissolves into a fine, grey ash.", FALSE, ch, obj, victim,
                    TO_CHAR);
                extract_obj(obj);
            }


            obj = next_obj;
        }


        /* check their equipment */
        for (i = 0; i < MAX_WEAR; i++) {
            if (victim->equipment[i] != NULL) {
                obj = victim->equipment[i];

                if (obj->obj_flags.type == ITEM_LIGHT
                    && obj->obj_flags.value[2] == LIGHT_TYPE_MAGICK) {
                    act("$N's $o shrinks until it is no longer visible.", FALSE, ch, obj, victim,
                        TO_ROOM);
                    act("$N's $o shrinks until it is no longer visible.", FALSE, ch, obj, victim,
                        TO_CHAR);
                    unequip_char(victim, i);
                    if (extinguish_light(obj, victim->in_room))
                        extract_obj(obj);
                }

                else if (isnamebracketsok("[fire_jambiya]", OSTR(obj, name))) {
                    act("$N's $o disappears in a puff of smoke.", FALSE, ch, obj, victim, TO_ROOM);
                    act("$N's $o disappears in a puff of smoke.", FALSE, ch, obj, victim, TO_CHAR);
                    unequip_char(victim, i);
                    extract_obj(obj);
                }

                else if (isnamebracketsok("[sand_jambiya]", OSTR(obj, name))) {
                    act("$N's $o comes undone and falls into a small pile of sand.", FALSE, ch, obj,
                        victim, TO_ROOM);
                    act("$N's $o comes undone and falls into a small pile of sand.", FALSE, ch, obj,
                        victim, TO_CHAR);
                    unequip_char(victim, i);
                    extract_obj(obj);
                }

                else if (isnamebracketsok("[lightning_whip]", OSTR(obj, name))) {
                    act("$N's $o explodes in a shower of sparks.", FALSE, ch, obj, victim, TO_ROOM);
                    act("$N's $o explodes in a shower of sparks.", FALSE, ch, obj, victim, TO_CHAR);
                    unequip_char(victim, i);
                    extract_obj(obj);
                }

                else if (isnamebracketsok("[shadow_sword]", OSTR(obj, name))) {
                    act("$N's $o flickers then fades out of existance.", FALSE, ch, obj, victim,
                        TO_ROOM);
                    act("$N's $o flickers then fades out of existance.", FALSE, ch, obj, victim,
                        TO_CHAR);
                    unequip_char(victim, i);
                    extract_obj(obj);
                }

                else if (isnamebracketsok("[shield_of_mist]", OSTR(obj, name))) {
                    act("$N's $o suddenly dissolves in a loud hiss.", FALSE, ch, obj, victim,
                        TO_ROOM);
                    act("$N's $o suddenly dissolves in a loud hiss.", FALSE, ch, obj, victim,
                        TO_CHAR);
                    unequip_char(victim, i);
                    extract_obj(obj);
                }

                else if (isnamebracketsok("[shield_of_wind]", OSTR(obj, name))) {
                    act("$N's $o stop swirling and blinks from existance.", FALSE, ch, obj, victim,
                        TO_ROOM);
                    act("$N's $o stop swirling and blinks from existance.", FALSE, ch, obj, victim,
                        TO_CHAR);
                    unequip_char(victim, i);
                    extract_obj(obj);
                }

            }
        }

        if (IS_IMMORTAL(victim) && IS_IMMORTAL(ch))
            yes = 1;

        tmp_af = victim->affected;
        while (tmp_af) {
            if (tmp_af->next)
                next_af = tmp_af->next;
            else
                next_af = 0;

            switch (tmp_af->type) {
            case SPELL_SEND_SHADOW:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    act("Your vision goes momentarily black...\n\r", FALSE, victim, 0, 0, TO_CHAR);
                    act("$N quickly dissipates.", FALSE, ch, 0, victim, TO_CHAR);
                    act("$N quickly dissipates.", FALSE, ch, 0, victim, TO_ROOM);
                    parse_command(victim, "return");
                    return;     /* the victim no longer exists, so exit here  */
                } else {
                    send_to_char("You feel your shadow form weaken.\n\r", victim);
                    act("$n's form wavers.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;
            case SPELL_INVISIBLE:
                dispel_invisibility_ch(ch, victim, tmp_af, level);
                break;
            case SPELL_DETECT_INVISIBLE:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_DETECT_INVISIBLE);
                    send_to_char("You feel less perceptive.\n\r", victim);
                } else
                    send_to_char("Your vision dims for a moment.\n\r", victim);
                break;

            case SPELL_DETECT_POISON:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_DETECT_POISON);
                    send_to_char("You stop focussing on poisonous auras.\n\r", victim);
                } else
                    send_to_char("Your perception of poisonous auras dims for a moment.\n\r", victim);
                break;

            case SPELL_DETECT_MAGICK:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_DETECT_MAGICK);
                    send_to_char("You stop noticing magickal auras.\n\r", victim);
                } else
                    send_to_char("Your perception of magickal auras dims for" " a moment.\n\r",
                                 victim);
                break;

            case SPELL_ELEMENTAL_FOG:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_ELEMENTAL_FOG);
                    send_to_char("You feel the fog dissipate from your eyes.\n\r", victim);
                } else
                    send_to_char("You feel the fog lessen slightly about your " "eyes.\n\r",
                                 victim);
                break;

            case SPELL_FIREBREATHER:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_FIREBREATHER);
                    act("Your body cools down to a normal temperature.\n\r", FALSE, victim, 0, 0,
                        TO_CHAR);
                    act("$n's body cools to a normal temperature.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    act("Your body cools down slightly.\n\r", FALSE, victim, 0, 0, TO_CHAR);
                    act("$n's body cools down slightly.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;


            case SPELL_INSOMNIA:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_INSOMNIA);
                    act("You don't feel so energized anymore.\n\r", FALSE, victim, 0, 0, TO_CHAR);
                    act("$n's body seems less excited.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    act("You feel less energized.", FALSE, victim, 0, 0, TO_CHAR);
                    act("$n's body seems less excited.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_FLOURESCENT_FOOTSTEPS:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_FLOURESCENT_FOOTSTEPS);
                    act("Your feet cease to glow.\n\r", FALSE, victim, 0, 0, TO_CHAR);
                    act("$n's feet cease to glow.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    act("The glow about your feet dims.\n\r", FALSE, victim, 0, 0, TO_CHAR);
                    act("The glow about $n's feet dims.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_FEAR:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_FEAR);
                    if (IS_SET(victim->specials.act, CFL_FLEE))
                        REMOVE_BIT(victim->specials.act, CFL_FLEE);
                    act("You don't feel so frightened anymore.\n\r", FALSE, victim, 0, 0, TO_CHAR);
                    act("$n shakes $s head as if to clear it.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    act("You feel less frightened.\n\r", FALSE, victim, 0, 0, TO_CHAR);
                    act("$n shakes $s head as if to clear it, then looks" " around fearfully.",
                        FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_SANCTUARY:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_SANCTUARY);
                    send_to_char("You don't feel protected anymore.\n\r", victim);
                    act("The white glow around $n's body fades.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("The protection surrounding you weakens.\n\r", victim);
                    act("The white glow around $n's body weakens.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_INFRAVISION:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_INFRAVISION);
                    send_to_char("Your sight grows dimmer.\n\r", victim);
                    act("The red glow from $n's eyes fades.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your sight grows dimmer.\n\r", victim);
                    act("The red glow from $n's eyes fades slightly.", FALSE, victim, 0, 0,
                        TO_ROOM);
                }
                break;

            case SPELL_REGENERATE:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_REGENERATE);
                    send_to_char("You feel your energy ebb a little.\n\r", victim);
                    act("$n slumps slightly, paling.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You feel your energy ebb.\n\r", victim);
                    act("$n slumps slightly, looking pale.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_SLEEP:
                if (!knocked_out(victim)) {
                    tmp_af->expiration -= (level * RT_ZAL_HOUR);
                    if (tmp_af->expiration < time(NULL)) {
                        affect_from_char(victim, SPELL_SLEEP);
                        send_to_char("You don't feel so tired.\n\r", victim);
                        act("$n's sleep lightens.", FALSE, victim, 0, 0, TO_ROOM);
                    } else {
                        send_to_char("You don't feel so tired.\n\r", victim);
                        act("$n's sleep lightens.", FALSE, victim, 0, 0, TO_ROOM);
                    }
                }
                break;

            case SPELL_CHARM_PERSON:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_CHARM_PERSON);
                    send_to_char("You feel less enthused about your master.\n\r", victim);
                    act("$n regains mental control.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You feel less enthused about your master, but "
                                 "they're still your master.\n\r", victim);
                    act("$n blinks rapidly a few times.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_STRENGTH:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_STRENGTH);
                    send_to_char("You don't feel so strong.\n\r", victim);
                    act("$n's muscles return to their normal size.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You don't feel so strong.\n\r", victim);
                    act("$n's muscles shrink slightly.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_WEAKEN:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_WEAKEN);
                    send_to_char("You don't feel so weak.\n\r", victim);
                    act("$n's muscles return to their normal size.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You don't feel so weak.\n\r", victim);
                    act("$n's muscles swell slightly.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_ARMOR:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_ARMOR);
                    send_to_char("You don't feel so well protected.\n\r", victim);
                    act("The diffused air around $n dissipates.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your magickal armor wavers momentarily.\n\r", victim);
                    act("The diffused air around $n wavers momentarily.\n\r", FALSE, victim, 0, 0,
                        TO_ROOM);
                }
                break;

            case SPELL_STONE_SKIN:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_STONE_SKIN);
                    send_to_char("Your skin becomes soft again.\n\r", victim);
                    act("$n's skin becomes soft again.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your skin becomes softer.\n\r", victim);
                    act("$n's skin becomes softer.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_LEVITATE:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_LEVITATE);
                    send_to_char("You don't feel lighter than air anymore.\n\r", victim);
                    act("$n floats gently to the ground.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You float closer to the ground, but remain aloft.\n\r", victim);
                    act("$n floats closer to the ground, but remains aloft.", FALSE, victim, 0, 0,
                        TO_ROOM);
                }
                break;

            case SPELL_FLY:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_FLY);
                    send_to_char("Your feet slowly descend to the ground.\n\r", victim);
                    act("$n's feet slowly descend to the ground.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You float closer to the ground, but remain aloft.\n\r", victim);
                    act("$n floats closer to the ground, but remains aloft.", FALSE, victim, 0, 0,
                        TO_ROOM);
                }
                break;

            case SPELL_TONGUES:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_TONGUES);
                    send_to_char("Your tongue feels heavier.\n\r", victim);
                    act("$n appears dazed momentarily.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your tongue feels a bit heavier.\n\r", victim);
                    act("$n appears dazed momentarily, but regains $s composure.", FALSE, victim, 0,
                        0, TO_ROOM);
                }
                break;

            case SPELL_ETHEREAL:
                dispel_ethereal_ch(ch, victim, tmp_af, level);
                break;

            case SPELL_DETECT_ETHEREAL:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_DETECT_ETHEREAL);
                    send_to_char("You feel less in touch with the shadow plane.\n\r", victim);
                    act("$n's eyes lose their shadowy aura.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You feel less in touch with the shadow plane.\n\r", victim);
                    act("$n's briefly eyes lose their shadowy aura.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_DEAFNESS:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_DEAFNESS);
                    send_to_char("You rub your ears, as your hearing returns.\n\r", victim);
                    act("$n rubs $s ears.", FALSE, victim, 0, 0, TO_ROOM);
                } else
                    send_to_char("You hear a drumming noise which slips away, leaving "
                                 "you once again deaf.\n\r", victim);
                break;

            case SPELL_SILENCE:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_SILENCE);
                    send_to_char("You are able to speak again.\n\r", victim);
                    act("$n regains the ability to speak.", FALSE, victim, 0, 0, TO_ROOM);
                } else
                    send_to_char("Your tongue tingles for a moment.\n\r", victim);
                break;

            case SPELL_FEATHER_FALL:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_FEATHER_FALL);
                    send_to_char("You don't feel as light anymore.\n\r", victim);
                    act("$n's body regains its weight.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your body regains some of its weight.\n\r", victim);
                    act("$n's body regains some its weight.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_INVULNERABILITY:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_INVULNERABILITY);
                    send_to_char("The creamy shell around you dissipates.\n\r", victim);
                    act("The creamy shell around $n collapses.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("The creamy shell around you wavers.\n\r", victim);
                    act("The creamy shell around $n wavers.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_PSI_SUPPRESSION:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_PSI_SUPPRESSION);
                    send_to_char("You feel your mental barrier crumble.\n\r", victim);
                    act("$n winces and grabs $s head.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You feel your mental barrier waver.\n\r", victim);
                    act("$n winces and grabs $s head.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_CALM:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_CALM);
                    send_to_char("You feel less calm.\n\r", victim);
                    act("$n appears less calm.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You feel less calm.\n\r", victim);
                    act("$n appears less calm.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_GODSPEED:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_GODSPEED);
                    send_to_char("Your body slows down.\n\r", victim);
                    act("$n's body slows down.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your body slows down slightly.\n\r", victim);
                    act("$n's body slows down a bit.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_SLOW:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_SLOW);
                    send_to_char("Your body does not feel so sluggish.\n\r", victim);
                    act("$n does not look as sluggish.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your body does not feel so sluggish.\n\r", victim);
                    act("$n does not look as sluggish.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_FURY:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_FURY);
                    send_to_char("You feel more calm and in control.\n\r", victim);
                    act("$n becomes less angry.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You feel more calm and in control.\n\r", victim);
                    act("$n becomes less angry.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_ENERGY_SHIELD:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_ENERGY_SHIELD);
                    send_to_char("The electrical energy around you disappears.\n\r", victim);
                    act("The electrical energy around $n disappears.", FALSE, victim, 0, 0,
                        TO_ROOM);
                } else {
                    send_to_char("The electrical energy around you crackles.\n\r", victim);
                    act("The electrical energy around $n crackles.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_QUICKENING:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_QUICKENING);
                    send_to_char("Your body slows down to normal.\n\r", victim);
                    act("$n's body slows down to a normal speed.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your body slows down some.\n\r", victim);
                    act("$n's body slows down some.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_BREATHE_WATER:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_BREATHE_WATER);
                    if (victim->in_room->sector_type == SECT_WATER_PLANE) {
                        send_to_char("You can no longer breathe water.\n\r", victim);
                        act("$n's begins gasping for air.", FALSE, victim, 0, 0, TO_ROOM);
                    }
                } else {
                    if (victim->in_room->sector_type == SECT_WATER_PLANE)
                        send_to_char("You find it a bit harder to breathe underwater.\n\r", victim);
                }
                break;


            case SPELL_WIND_ARMOR:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_WIND_ARMOR);
                    send_to_char("The gusting winds around you scatter in all " "directions.\n\r",
                                 victim);
                    act("The gusting winds surrounding $n's body scatter in all " "directions.",
                        FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("The gusting winds surrounding your body slow down" " a bit.\n\r",
                                 victim);
                    act("The gusting winds surrounding $n's body slow down a bit.", FALSE, victim,
                        0, 0, TO_ROOM);
                }
                break;

            case SPELL_FIRE_ARMOR:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_FIRE_ARMOR);
                    send_to_char("The flames surrounding you abruptly sputter away "
                                 "into little wisps of smoke.\n\r", victim);
                    act("The flames surrounding $n's body abruptly sputter away "
                        "into little wisps of smoke.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("The flames surrounding your body sputter briefly.\n\r", victim);
                    act("The flames surrounding $n's body sputter briefly.", FALSE, victim, 0, 0,
                        TO_ROOM);
                }
                break;

            case SPELL_SHADOW_ARMOR:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_SHADOW_ARMOR);
                    send_to_char("The shadows surrounding you abruptly disperse "
                                 "into fading wisps of nothing.\n\r", victim);
                    act("The shadows surrounding $n's body abruptly disperse into "
                        "fading wisps of nothing.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("The shadows surrounding your body brighten a little "
                                 "as their magick weakens.\n\r", victim);
                    act("The shadows surrounding $n's body brighten a little as their "
                        "magick weakens.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_IMMOLATE:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_IMMOLATE);
                    send_to_char("The ravaging fires burning you suddenly "
                                 "die out with a wisp of smoke..\n\r", victim);
                    act("The ravaging fires burning $n suddenly die out with " "a wisp of smoke.",
                        FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("The ravaging flames around you weaken a little.\n\r", victim);
                    act("The ravaging flames around $n weaken a little.", FALSE, victim, 0, 0,
                        TO_ROOM);
                }
                break;


            case SPELL_PARALYZE:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_PARALYZE);
                    send_to_char("Your body relaxes, and you can move again.\n\r", victim);
                    act("$n begins to move again.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your body relaxes, but you are still paralyzed.\n\r", victim);
                    act("$n begins to show signs of movement.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;

            case SPELL_BLIND:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_BLIND);
                    send_to_char("Your vision returns to normal as you can see again.\n\r", victim);
                    act("$n blinks rapidly, the color completely returning to $s eyes.", FALSE,
                        victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("Your vision lightens a little, but you are still blind.\n\r",
                                 victim);
                    act("$n blinks rapidly, a little color returning to $s eyes.", FALSE, victim, 0,
                        0, TO_ROOM);
                }
                break;

            case SPELL_FEEBLEMIND:
                tmp_af->expiration -= (level * RT_ZAL_HOUR);
                if (tmp_af->expiration < time(NULL)) {
                    affect_from_char(victim, SPELL_FEEBLEMIND);
                    send_to_char("You feel your magickal powers returning.\n\r", victim);
                    act("$n shakes $s head as if to clear it.", FALSE, victim, 0, 0, TO_ROOM);
                } else {
                    send_to_char("You feel your magickal powers returning.\n\r", victim);
                    act("$n shakes $s head as if to clear it.", FALSE, victim, 0, 0, TO_ROOM);
                }
                break;
            default:
                break;
            }

            if (next_af) {
                struct affected_type *check_af = 0;
                // Walk the list fresh and see if we can find our "next" pointer
                // again.  If we can't, it probably got removed by an
                // affect_from_char() above, and we should start at the
                // beginning of the list
                for (check_af = victim->affected; check_af && check_af != next_af;
                     check_af = check_af->next);

                if (next_af == check_af)
                    tmp_af = next_af;
                else
                    tmp_af = victim->affected;
            } else
                tmp_af = 0;
        }

        /* Begin animate dead modification  -Az */
        if (IS_SET(victim->specials.act, CFL_UNDEAD)) { /* begin if(undead) */
            if (yes || !does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
                if (get_char_extra_desc_value(victim, "[ACTION_DISPEL_MAGICK]", e_desc, sizeof(e_desc))) {
                    send_to_char("Your magickal existance is weakened.\n\r", victim);
                    act("$n's body flickers slightly, appearing weakened.",
                        FALSE, victim, 0, 0, TO_ROOM);
                    alive = ws_damage(ch, victim, (number(5,10) + level), 0, 0,
                                      (number(5,10) + level), SPELL_DISPEL_MAGICK, TYPE_ELEM_FIRE);
                } else {
                    send_to_char("Your magickal existence is dispersed.\n\r", victim);
                    act("$n explodes into a million pieces.", FALSE, victim, 0, 0, TO_ROOM);
                    if (affected_by_spell(victim, SPELL_POSSESS_CORPSE)
                        || GET_RACE(victim) == RACE_VAMPIRE) {
                        sprintf(logbuf, "%s was killed by dispel magick cast by %s in room #%d",
                                GET_NAME(victim), (victim == ch) ? "themself" : GET_NAME(ch),
                                victim->in_room->number);
                        gamelog(logbuf);
                        die(victim);
                    } else
                        extract_char(victim);
                    /* need to return here, cause victim is not longer
                     * good */
                return;
                }
            } else {
                send_to_char("You resist the magick that attempts to "
                             "unravel your magickal existence.\n\r", victim);
                act("$n's body flickers slightly, but remains stable.", FALSE, victim, 0, 0,
                    TO_ROOM);
            }
        }
        /* end  if (undead) */
        if (IS_AFFECTED(victim, CHAR_AFF_SUMMONED)) {
            if ((affected_by_spell(victim, SPELL_MOUNT))
                && (yes || !does_save(victim, SAVING_SPELL, ((level - 4) * (-5))))) {
                send_to_char("You feel your magickal bindings " "released.\n\r", victim);

                if ((npc_index[victim->nr].vnum >= 101) &&      /* Krathi */
                    (npc_index[victim->nr].vnum <= 107))
                    act("$n explodes in a shower of hot embers.", FALSE, victim, 0, 0, TO_ROOM);
                else if ((npc_index[victim->nr].vnum >= 108) && /* Viv */
                         (npc_index[victim->nr].vnum <= 114))
                    act("$n's body turns to a fine mist, and dissipates.", FALSE, victim, 0, 0,
                        TO_ROOM);
                else if ((npc_index[victim->nr].vnum >= 115) && /* Whira */
                         (npc_index[victim->nr].vnum <= 121))
                    act("$n slowly turns to dust and blows away.", FALSE, victim, 0, 0, TO_ROOM);
                else if ((npc_index[victim->nr].vnum >= 122) && /* drov */
                         (npc_index[victim->nr].vnum <= 128))
                    act("$n falls into its own shadow, which shrinks from view.", FALSE, victim, 0,
                        0, TO_ROOM);
                else if ((npc_index[victim->nr].vnum >= 129) && /* elkros */
                         (npc_index[victim->nr].vnum <= 135))
                    act("$n explodes in a shower of sparks.", FALSE, victim, 0, 0, TO_ROOM);
                else if ((npc_index[victim->nr].vnum >= 136) && /* nilaz */
                         (npc_index[victim->nr].vnum <= 142))
                    act("$n fades from view as a silence falls upon the area.", FALSE, victim, 0, 0,
                        TO_ROOM);
                else if ((npc_index[victim->nr].vnum >= 1302) &&        /* Ruk */
                         (npc_index[victim->nr].vnum <= 1308))
                    act("$n explodes in a shower of sand.", FALSE, victim, 0, 0, TO_ROOM);
                else if ((npc_index[victim->nr].vnum >= 1309) &&        /* Templar */
                         (npc_index[victim->nr].vnum <= 1315))
                    act("$n shatters, scattering the area with obsidian flakes.", FALSE, victim, 0,
                        0, TO_ROOM);
                else
                    act("$n explodes in a shower of sand.", FALSE, victim, 0, 0, TO_ROOM);


                extract_char(victim);
                /* victim is no longer good, return */
                return;
            } else {            /* Non-mount summoned creatures */
                if (yes || !does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
                    send_to_char("You feel yourself drawn back to the plane " "of Nilaz.\n\r",
                                 victim);
                    act("A stillness descends around $n and $e slowly fades " "away.", FALSE,
                        victim, 0, 0, TO_ROOM);
                    extract_char(victim);
                    /* victim is no longer good, return */
                    return;
                }
            }
        }

        if (level >= 4) {
            if (affected_by_spell(victim, SPELL_BLIND)) {
                if (yes || !does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
                    affect_from_char(victim, SPELL_BLIND);
                    send_to_char("Your vision returns.\n\r", victim);
                    act("Color returns to $n's eyes.", FALSE, victim, 0, 0, TO_ROOM);
                }
            }
            if (affected_by_spell(victim, SPELL_POISON)) {
                if (yes || !does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
                    affect_from_char(victim, SPELL_POISON);
                    send_to_char("There is a sudden end to the burning in your " "veins.\n\r",
                                 victim);
                    act("$n doesn't look as sick anymore.", FALSE, victim, 0, 0, TO_ROOM);
                }
            }
        }
    } /* end if (victim) */
    else {                      /* begin else if that determines victim and obj aren't valid */
        if (IS_SET(ch->in_room->room_flags, RFL_RESTFUL_SHADE)) {
            REMOVE_BIT(ch->in_room->room_flags, RFL_RESTFUL_SHADE);
            act("The restful shade here dissipates.", FALSE, ch, 0, 0, TO_ROOM);
            act("You dissipate the magickal shade here.\n\r", FALSE, ch, 0, 0, TO_CHAR);
        }
        if (IS_SET(ch->in_room->room_flags, RFL_SPL_ALARM)) {
	    remove_alarm_spell(ch->in_room);
            act("A faint pinging noise can be heard.", FALSE, ch, 0, 0, TO_ROOM);
            act("As you complete your spell, a faint pinging can be heard.\n\r", FALSE, ch, 0, 0,
                TO_CHAR);
        }
        for (dir = DIR_NORTH; dir <= 5; dir++) {        /*for loop that goes through exits */
            if (ch->in_room->direction[dir]) {  /* is there an exit that way? */
                if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL)) { /* Does exit have wall of sand on it? */
                    REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL);
                    sprintf(buf, "The %s exit is opened as a giant wall of" " sand dissolves.\n\r",
                            dirs[dir]);
                    send_to_room(buf, ch->in_room);
                    /*  Now for that way into the room - Kelvik  */
                    if ((other_room = EXIT(ch, dir)->to_room))
                        if ((back = other_room->direction[rev_dir[dir]]))
                            if (back->to_room == ch->in_room) {
                                REMOVE_BIT(back->exit_info, EX_SPL_SAND_WALL);
                                sprintf(buf,
                                        "A giant wall of sand to the %s" "suddenly dissolves.\n\r",
                                        dirs[rev_dir[dir]]);
                                send_to_room(buf, EXIT(ch, dir)->to_room);
                            }
                }               /* does exit have wall of sand on it */
            }                   /* is there an exit that way? */
        }                       /* for loop that goes through exits */
    }                           /* end of else if else that dtermins victim and obj aren't valid */
}                               /* end Spell_Dispel_Magick */


/* DRAGON BANE: Can only be cast by caster with a positive
 * relationship to the Land.  Damages a target if they have
 * a negative relationship to the land.  Quest only spell.
 * Nilaz divan hekro.
 */
void
spell_dragon_bane(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Dragon_Bane */
    int dam = 0;

    if (!ch) {
        gamelog("No ch in dragon bane");
        return;
    }

    if (!victim) {
        gamelog("No victim in dragon bane");
        return;
    }

    if ((ch->in_room->sector_type == SECT_INSIDE) || (ch->in_room->sector_type == SECT_CITY)) {
        act("You are unable to reach the land to call on it.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (ch->specials.eco < 0) {
        act("The land refuses to be wielded by a defiler, and harms you in" " retaliation.", FALSE,
            ch, 0, 0, TO_CHAR);
        act("Coiling arcs of magick swirl angrily, leaving $n wounded.", FALSE, ch, 0, 0, TO_ROOM);
        dam = (level * number(1, level));
        generic_damage(ch, dam, 0, 0, dam);
        if (GET_POS(ch) == POSITION_DEAD)
            die(ch);
        return;
    }

    if (ch->specials.eco == 0) {
        act("The land resists your call.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (victim->specials.eco > -1)
        dam = 0;

    if (victim->specials.eco < 0)
        dam = (level * ((victim->specials.eco * (-1)) / 5));

    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5))))
        dam = (dam / 2);

    ws_damage(ch, victim, dam, 0, 0, dam, SPELL_DRAGON_BANE, 0);
}                               /* end Spell_Dragon_Bane */


/* DRAGON DRAIN: Hits target for hit points, mana and stamina,
 * and transfers some of the energy to the caster.  Casting has
 * a negative effect on the caster's relationship to the Land.
 * Quest only spell. Note: should be updated at some point to
 * remove stun as well.  Nilaz morz hurn.
 */
void
spell_dragon_drain(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Dragon_Drain */
    int hps, mana, stna;

    if (!ch) {
        gamelog("No ch in dragon drain");
        return;
    }

    if (!victim) {
        gamelog("No victim in dragon drain");
        return;
    }

    check_criminal_flag(ch, NULL);

    hps = dice(level, level + 2) * 5;
    mana = dice(level, level + 1) * 5;
    stna = dice(level, level + 3) * 5;

    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
        hps = (hps / 3);
        mana = (mana / 2);
        stna = (stna / 4);
    }

    /* Can't drain more life than the victim has */
    if (hps > (GET_HIT(victim) + 10))   /* You can drain them to death */
        hps = (GET_HIT(victim) + 10);
    if (mana > GET_MANA(victim))
        mana = GET_MANA(victim);
    if (stna > GET_MOVE(victim))
        stna = GET_MOVE(victim);

    act("You invoke the symbols of the Dragon, and " "drain the life from $N.", FALSE, ch, 0,
        victim, TO_CHAR);
    act("You see the image of a frightening beast, and " "your body feels drained by $n.", FALSE,
        ch, 0, victim, TO_VICT);
    act("$n invokes the symbols of the Dragon, and drains $N's energy.", FALSE, ch, 0, victim,
        TO_NOTVICT);

    /* Take energy away from victim */
    generic_damage(victim, hps, 0, 0, 0);
    if (GET_POS(victim) == POSITION_DEAD)
        die(victim);

    adjust_mana(victim, -mana);
    adjust_move(victim, -stna);
    update_pos(victim);

    /* Return part of life to defiler */

    act("You revel in ecstasy as $N's energy flows into you.", FALSE, ch, 0, victim, TO_CHAR);

    hps = ((hps * (level / 7)) - number(level, (level * 2)));
    if (hps < 0)
        hps = number(1, level);
    adjust_hit(ch, hps);

    mana = ((mana * (level / 7)) - number(0, level));
    if (mana < 0)
        mana = number(1, level);
    adjust_mana(ch, mana);

    stna = ((stna * (level / 7)) - number(0, level));
    if (stna < 0)
        stna = number(1, level);
    adjust_move(ch, stna);

    /* This _is_ an anti-land spell */
    ch->specials.eco = MAX(-100, ch->specials.eco - level);
}                               /* end Spell_Dragon_Drain */

/*
 * This function guides the astar algorithm, and additionally
 * emits messages as it visits rooms.  For cases where it returns
 * FALSE, it acts as a pruning tool for the pather, as well,
 * and the pather will not generate "child" nodes from this room.
 */
int
earthquake_helper(ROOM_DATA *room, int depth, void *data)
{
    CHAR_DATA *ch = (CHAR_DATA*)data, *tch;

    if (!ch) return FALSE;                 // Should never happen -- crash prevention

    if (room == ch->in_room) return TRUE;  // No messages, but continue pathing

    if (room) {
        switch (room->sector_type) {
        case SECT_AIR:         // No earthquakes here
        case SECT_SILT:        // Quakes don't make sense, and you have enough probs
        case SECT_NILAZ_PLANE:
        case SECT_WATER_PLANE:
        case SECT_AIR_PLANE:
            return FALSE;
        default:
            break;
        }
    }

    for (tch = room->people; tch; tch = tch->next_in_room) {
        switch (room->sector_type) {
        case SECT_CAVERN:
            act("All around you, the stony walls of the cavern shake as the ground moves beneath your feet.", FALSE, ch, 0, tch, TO_VICT);
            break;
        case SECT_DESERT:
            act("The sands shift and groan, dunes sliding as the ground moves beneath your feet.", FALSE, ch, 0, tch, TO_VICT);
            break;
        default:
            act("The ground shakes violently beneath your feet.", FALSE, ch, 0, tch, TO_VICT);
            break;
        }

        if (check_agl(tch, 0) || IS_IMMORTAL(tch) ||
            IS_AFFECTED(tch, CHAR_AFF_FLYING) ||
            affected_by_spell(tch, SPELL_ETHEREAL) ||
            affected_by_spell(tch, SPELL_STONE_SKIN)) {
            continue;
        }

        cprintf(tch, "You stumble and fall to the ground.\n\r");
        set_char_position(tch, POSITION_RESTING);
    }

    return TRUE;
}

/* EARTHQUAKE: Area effect attack, damaging everyone except
 * the caster or flying/ethereal creatures.  Does more damage
 * in the following sectors: INSIDE, CAVERN, CITY, and RUINS. 
 * Echoes to the entire zone, with message to nearby rooms as
 * well. Rukkian and sorcerer spell.  Ruk nathro echri. 
 * Note: it would be cool to make the room echoes dependent on
 * sector.
 */
void
spell_earthquake(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Earthquake */
    int dam, alive, sector;
    CHAR_DATA *tch, *next_tch;

    if (!ch) {
        gamelog("No ch in earthquake");
        return;
    }

    if (!ch->in_room) {
        gamelog("No room in earthquake");
        return;
    }

    if (ch->in_room->sector_type == SECT_AIR_PLANE) {
        send_to_char("There is not enough ground here to cause an earthquake.\n\r", ch);
        return;
    }

    if ((ch->in_room->sector_type == SECT_INSIDE) || (ch->in_room->sector_type == SECT_CAVERN))
        dam = dice(3, 8) + (level * level);
    else if ((ch->in_room->sector_type == SECT_CITY) || (ch->in_room->sector_type == SECT_RUINS))
        dam = dice(3, 7) + (4 * level);
    else
        dam = dice(2, 8) + (3 * level);

    check_criminal_flag(ch, NULL);

    sector = ch->in_room->sector_type;

    generic_astar(ch->in_room, 0, level - 1, (float)level - 1.0, 0, RFL_NO_ELEM_MAGICK | RFL_NO_MAGICK,
                  0, 0, 0, earthquake_helper, ch, 0);

    act("$n makes some arcane gestures, and the ground shakes violently.", FALSE, ch, 0, 0,
        TO_ROOM);
    act("You make some arcane gestures, and the ground shakes violently.", FALSE, ch, 0, ch,
        TO_CHAR);
    act("To the $D, the ground rumbles violently.", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);

    for (tch = ch->in_room->people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;

        if (ch != tch) {
            if ((!IS_IMMORTAL(tch))
                && (!IS_AFFECTED(tch, CHAR_AFF_FLYING))
                && (!affected_by_spell(tch, SPELL_ETHEREAL))
                && (!affected_by_spell(tch, SPELL_STONE_SKIN))
                && (GET_RACE(tch) != RACE_SHADOW)) {
                stop_all_fighting(tch);
                stop_all_fighting(ch);
                if (IS_NPC(tch))
                    if (!does_hate(tch, ch))
                        add_to_hates_raw(tch, ch);
                WAIT_STATE(tch, number(1, 2));
                alive = ws_damage(ch, tch, dam, 0, 0, dam, SPELL_EARTHQUAKE, TYPE_ELEM_EARTH);
            }
        } else {
            if ((!IS_IMMORTAL(tch))
                && (!IS_AFFECTED(tch, CHAR_AFF_FLYING))
                && (!affected_by_spell(tch, SPELL_ETHEREAL))
                && (!affected_by_spell(tch, SPELL_STONE_SKIN))
                && (GET_RACE(tch) != RACE_SHADOW)) {
                WAIT_STATE(tch, number(1, 2));
                alive = generic_damage(tch, (dam / 3), 0, 0, (dam / 3));
            }
        }

    }
}                               /* end Spell_Earthquake */


/* ELEMENTAL FOG: Creates a field around the recipient that reflects 
 * the gaze of those wearing sight-enhanced spells, and which extends
 * to the caster as well, if they have such spells upon them.  Nilazi
 * and sorcerer spell. Nilaz iluth viod.
 */
void
spell_elemental_fog(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Elemental_Fog */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in elemental fog");
        return;
    }

    check_criminal_flag(ch, NULL);

    duration = number(level, ((level + 1) * 2));

    if (victim == ch) {
        act("A misty fog escapes your mouth, engulfing you briefly, then dissipates.", FALSE,
            victim, 0, 0, TO_CHAR);
        act("A misty fog escapes $n's mouth, engulfing $m briefly, then disappears.", FALSE, victim,
            0, 0, TO_ROOM);
    } else {
        act("A misty fog escapes your mouth, engulfing $N briefly before dissipating.", FALSE, ch,
            0, victim, TO_CHAR);
        act("A misty fog escapes $n's mouth, engulfing you briefly before dissipating.", FALSE, ch,
            0, victim, TO_VICT);
        act("A misty fog escapes $n's mouth, engulfing $N briefly before dissipating.", TRUE, ch, 0,
            victim, TO_NOTVICT);
    }

    af.type = SPELL_ELEMENTAL_FOG;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;

    stack_spell_affect(victim, &af, 36);
}                               /* end Spell_Elemental_Fog */


/* ENERGY SHIELD: Creates a protective shield around the caster, its
 * duration dependent on level.  Non-cumulative.  The shield is visible
 * to the naked eye.  Elkros and sorcerer spell.  Elkros wilith grol.
 */

void
spell_energy_shield(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Energy_Shield */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in energy shield");
        return;
    }

    check_crim_cast(ch);

    duration = number(level, ((level + 1) * 2));

    if (affected_by_spell(victim, SPELL_WIND_ARMOR)) {
        send_to_char("The winds swirling around your form prevent the energy shield.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_STONE_SKIN)) {
        send_to_char("The energy shield is unable to take hold against your stony skin.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
        send_to_char("The flames surrounding you prevent the energy shield from taking hold.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
        send_to_char("The shadows surrounding you prevent the energy shield from taking hold.\n\r",
                     victim);
        return;
    }

    if (ch->in_room->sector_type == SECT_WATER_PLANE) {
        send_to_char
            ("The waters here hinder your ability to call on Elkros, weakening the spell.\n\r", ch);
        duration = duration / 2;
    }

    if (affected_by_spell(victim, SPELL_ENERGY_SHIELD)) {
        send_to_char("Nothing happens.\n\r", ch);
/* gamelog ("Awooga.  Cast Energy Shield while inside an energy shield."); */
        return;
    }

    act("A crackling shield of energy surrounds your body, " "fading into sparkles of light.",
        FALSE, victim, 0, 0, TO_CHAR);
    act("A crackling shield of energy forms around $n, " "fading into sparkles of light.", FALSE,
        victim, 0, 0, TO_ROOM);

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_ENERGY_SHIELD;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

}                               /* end Spell_Energy_Shield */

/* Helper function for spell_ethereal */
void
dump_nonethereal_objects(OBJ_DATA * container, ROOM_DATA * room)
{
    OBJ_DATA *obj, *tmp_obj;

    for (obj = container->contains; obj; obj = tmp_obj) {
        // Recursion into containers to clear everything out
        if (GET_ITEM_TYPE(obj) == ITEM_CONTAINER)
            dump_nonethereal_objects(obj, room);


        if (GET_ITEM_TYPE(obj) == ITEM_FURNITURE)
            dump_nonethereal_objects(obj, room);

        // List-keeping
        tmp_obj = obj->next_content;

        // Move it to the room if it can't go ethereal, except money since
        // you can't set the 'money object' to ethereal_ok due to how
        // money is managed between inventory and containers
        if (!IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL_OK)) {
            obj_from_obj(obj);
            obj_to_room(obj, room);
        }
    }
}

/* ETHEREAL: Moves the caster (but not his or her equipment and
 * inventory!) to the plane of Drov for a duration dependent on
 * power level.  Can be cast on an object as well. If the target
 * is riding, it removes them from their mount.  Requires a 
 * component.  Drovian and sorcerer spell.  Drov mutur nikiz.
 * Note: in the future, the objects removed should be checked and
 * left on the caster, if they are a component of SPHERE_ALTERATION.
 * (This will allow kizn ethereals to be cast.
 */
void
spell_ethereal(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Ethereal */
    struct affected_type af;
    OBJ_DATA *tmp_object, *next_obj;
    int duration, pos = 0;

    memset(&af, 0, sizeof(af));

    assert((ch && obj) || victim);

    check_crim_cast(ch);

    if (obj && (obj == get_component(ch, SPHERE_ALTERATION, level))) {
        send_to_char("You cannot cast this spell on the component being used for it.\n\r", ch);
        return;
    }

    if (!get_componentB
        (ch, SPHERE_ALTERATION, level, "$p blurs and fades into a shadowy wisp.", "",
         "You must have the proper component to shift planes."))
        return;

    if (obj) {
        act("Shadows creep from your hands, surrounding $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("Shadows creep from $n's hands, surrounding $p.", FALSE, ch, obj, 0, TO_ROOM);

        if (!IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL)) {
            if (!IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL_OK)) {
                if (obj->obj_flags.weight >= (level * 300)) {
                    act("$p is too heavy to be imbued with the shadows.", FALSE, ch, obj, 0,
                        TO_CHAR);
                    return;
                } else {
                    act("$p takes on a barely noticeable shadowy hue.", FALSE, ch, obj, 0, TO_CHAR);
                    act("$p takes on a barely noticeable shadowy hue.", TRUE, ch, obj, 0, TO_ROOM);
                    MUD_SET_BIT(obj->obj_flags.extra_flags, OFL_ETHEREAL_OK);
                    if (number(1, 100) <= level) {
                        act("$p shimmers then fades deeply into the shadow plane, lost forever.",
                            FALSE, ch, obj, 0, TO_CHAR);
                        act("$p shimmers then fades deeply into the shadow plane, lost forever.",
                            TRUE, ch, obj, 0, TO_ROOM);
                        extract_obj(obj);
                    }
                }
            } else {
                if (obj->obj_flags.weight >= (level * 300)) {
                    act("$p is too heavy to merge with the shadow plane.", FALSE, ch, obj, 0,
                        TO_CHAR);
                    return;
                } else {
                    act("$p fades away into the shadow plane.", FALSE, ch, obj, 0, TO_CHAR);
                    act("$p fades away into the shadow plane.", TRUE, ch, obj, 0, TO_ROOM);
                    MUD_SET_BIT(obj->obj_flags.extra_flags, OFL_ETHEREAL);
                    if (number(1, 100) <= level) {
                        act("$p shimmers then fades deeply into the shadow plane, lost forever.",
                            FALSE, ch, obj, 0, TO_CHAR);
                        act("$p shimmers then fades deeply into the shadow plane, lost forever.",
                            TRUE, ch, obj, 0, TO_ROOM);
                        extract_obj(obj);
                    }
                }
            }
        } else {
            act("$p is already a part of the shadow plane.", FALSE, ch, obj, 0, TO_CHAR);
            return;
        }

    } else {
        if (!affected_by_spell(victim, SPELL_ETHEREAL)) {

            if (victim == ch) {
                act("You point your palms toward your body, causing shadows to coelesce around you.", FALSE, victim, 0, 0, TO_CHAR);
                act("$n points $s palms towards $s body, causing shadows to coelesce around $mself.", FALSE, victim, 0, 0, TO_ROOM);
            } else {
                act("You point your palms toward $N, causing shadows to coelesce around $M.", FALSE,
                    ch, 0, victim, TO_CHAR);
                act("$n points $s palms towards you, causing shadows to coelesce around you.",
                    FALSE, ch, 0, victim, TO_VICT);
                act("$n points $s palms towards $N, causing shadows to coelesce around $M.", TRUE,
                    ch, 0, victim, TO_NOTVICT);
            }

            /* New stuff by Nessalin 9/30/97 */
            act("As $n slowly fades away into the shadow plane, all of "
                "$s possessions fall to the ground.", TRUE, victim, 0, 0, TO_ROOM);
            send_to_char("You feel yourself fading away into imperceptible " "darkness.\n\r",
                         victim);
            send_to_char("All of your possessions fall through your " "noncorporeal form.\n\r",
                         victim);

            /* Changed ch in the following to victim -- cast with room reach,     */
            /* it was making all the caster's eq fall off, but not the target's.  */

            for (tmp_object = victim->carrying; tmp_object; tmp_object = next_obj) {    /* Go through the loop and drop each item */
                next_obj = tmp_object->next_content;
                if (IS_SET(tmp_object->obj_flags.extra_flags, OFL_DISINTIGRATE))
                    extract_obj(tmp_object);
                //                else {
                else if (!IS_SET(tmp_object->obj_flags.extra_flags, OFL_ETHEREAL_OK)) {
                    obj_from_char(tmp_object);
                    obj_to_room(tmp_object, victim->in_room);
                } else {
                    act("You bring $p into the shadow plane with you.", TRUE, victim, tmp_object, 0,
                        TO_CHAR);
                    MUD_SET_BIT(tmp_object->obj_flags.extra_flags, OFL_ETHEREAL);
                }
                if (GET_ITEM_TYPE(tmp_object) == ITEM_CONTAINER)
                    dump_nonethereal_objects(tmp_object, victim->in_room);
                if (GET_ITEM_TYPE(tmp_object) == ITEM_FURNITURE)
                    dump_nonethereal_objects(tmp_object, victim->in_room);
            }                   /* end drop all loop */

            if (GET_OBSIDIAN(victim))
                money_mgr(MM_CH2OBJ, GET_OBSIDIAN(victim), (void *) victim, 0);

            /* added this so non-nulls
             * will not get passed into
             * unequp char  -Tene 10/1/97 */
            for (pos = 0; pos < MAX_WEAR; pos++)
                if (victim->equipment && victim->equipment[pos]) {
                    if (GET_ITEM_TYPE(victim->equipment[pos]) == ITEM_CONTAINER)
                        dump_nonethereal_objects(victim->equipment[pos], victim->in_room);
                    if (IS_SET(victim->equipment[pos]->obj_flags.extra_flags, OFL_ETHEREAL_OK)) {
                        act("You bring $p into the shadow plane with you.", TRUE, victim,
                            victim->equipment[pos], 0, TO_CHAR);
                        MUD_SET_BIT(victim->equipment[pos]->obj_flags.extra_flags, OFL_ETHEREAL);
                    } else if (NULL != (tmp_object = unequip_char(victim, pos)))
                        obj_to_room(tmp_object, victim->in_room);
                }
            /* End new stuff by Nessalin */

            /*      else
             * {
             * act ("$n slowly fades away into the shadow plane.",
             * TRUE, victim, 0, 0, TO_ROOM);
             * send_to_char ("You feel yourself fading away into imperceptible "
             * "darkness.\n\r", victim);
             * } */

            duration = level;

            if (ch->in_room->sector_type == SECT_FIRE_PLANE) {
                send_to_char
                    ("The flames here hinder your ability to call on Drov, weakening your spell.\n\r",
                     ch);
                duration = duration / 2;
            }

            duration = MAX(1, duration);        // No 0 durations
            af.type = SPELL_ETHEREAL;
            af.duration = duration;
            af.power = level;
            af.magick_type = magick_type;
            af.modifier = 0;
            af.location = CHAR_APPLY_NONE;
            af.bitvector = CHAR_AFF_ETHEREAL;
            affect_to_char(victim, &af);

            /* handle visibility watch checks */
            watch_vis_check(victim);

            if (ch->specials.riding) {
                send_to_char("You pass right through your mount.\n\r", ch);
                ch->specials.riding = 0;
            }
        }
    }
}                               /* end Spell_Ethereal */


/* FEAR: Forces the target to flee in terror. Note: some races
 * should be unaffected by this.  Drovian and sorcerer spell.
 * Drov iluth hurn. 
 */
void
spell_fear(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Fear */
    char buf[MAX_STRING_LENGTH];
    int x, duration, sector;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in fear");
        return;
    }

    if (!victim) {
        gamelog("No victim in fear");
        return;
    }

    duration = level + 1;
    check_criminal_flag(ch, NULL);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
        act("The shadows around $N solidify briefly, blocking your spell.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("The shadows around you solidify briefly.", FALSE, ch, 0, victim, TO_VICT);
        act("The shadows around $N solidify briefly.", FALSE, ch, 0, victim, TO_NOTVICT);
        return;
    }

    if (IS_SET(victim->specials.act, CFL_FLEE)) {
        /* Target has cflag flee, but isn't affected by the fear spell */
        /* It would be bad if the fear spell is placed on them, because */
        /* when it wears off, the cflag would be removed as well.  */
        if (!affected_by_spell(ch, SPELL_FEAR)) {
            send_to_char("Nothing happens.\n\r", ch);
            return;
        } else
            duration = (level + 1) / 2;
    }

    if (!number(0, level)) {
        send_to_char("For a moment you feel compelled to run away, but you "
                     "fight back the urge.\n\r", victim);
        act("$N resists your terrifying illusion, standing firm.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    if (victim == ch) {
        act("You fill your mind with frightening images.", FALSE, ch, 0, 0, TO_CHAR);
    } else {
        act("You glare ominously at $N, sending frightening images into $S mind.", FALSE, ch, 0,
            victim, TO_CHAR);
        act("$n glares ominously at you, sending frightening images into your mind.", FALSE, ch, 0,
            victim, TO_VICT);
        act("$n glares ominously at $N, sending frightening images into $S mind.", TRUE, ch, 0,
            victim, TO_NOTVICT);
    }

    af.type = SPELL_FEAR;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = CFL_FLEE;
    af.location = CHAR_APPLY_CFLAGS;
    af.bitvector = 0;
    affect_join(victim, &af, FALSE, FALSE);

    snprintf(buf, sizeof(buf), "%s feels terrified.", MSTR(victim, short_descr));
    send_to_empaths(victim, buf);
    change_mood(victim, NULL);

    sector = ch->in_room->sector_type;
    x = number(1, (level + 1));
    for (; x; x--) {
        switch (sector) {
        case SECT_AIR:
            send_to_char("Vertigo seizes you, as you see the long fall that " "awaits you.\n\r",
                         victim);
            break;
        case SECT_CAVERN:
        case SECT_CITY:
        case SECT_INSIDE:
            send_to_char("The walls press in on you, closer and closer, until "
                         "you scream aloud in terror and flee.\n\r", victim);
            break;
        case SECT_DESERT:
            send_to_char("The sand parts, long tendrils of darkness reaching up "
                         "to grasp hungrily at you.\n\r", victim);
            break;
        case SECT_LIGHT_FOREST:
        case SECT_HEAVY_FOREST:
            send_to_char("The trees advance on you, reaching to seize you with "
                         "long, menacing limbs.\n\r", victim);
            break;
        case SECT_MOUNTAIN:
            send_to_char("All around you, the mountains cast wavering shadows, "
                         "fanged maws visible in their depths.\n\r", victim);
            break;
        case SECT_ROAD:
        case SECT_RUINS:
        case SECT_SALT_FLATS:
            send_to_char("Skulls glare up at you from the salty, sun-parched "
                         "ground, urging you to join them.\n\r", victim);
            break;
        case SECT_SILT:
            send_to_char("In the depths of the silt, you glimpse your death, "
                         "and flee in terror.\n\r", victim);
            break;
        case SECT_THORNLANDS:
            send_to_char("From the bushes, black thorns reach out, ripping and "
                         "tearing hungrily at you.\n\r", victim);
            break;
        default:
            send_to_char("A frightening image appears before your eyes, and you "
                         "are compelled to run!\n\r", victim);
            break;
        }

        strcpy(buf, "flee self");
        parse_command(victim, buf);
    }
}                               /* end Spell_Fear */


/* FEATHER FALL:  Caster receives no falling damage, for a
 * duration dependent on power level.  Cumulative with cap
 * of 36 hours.  Cast in sector AIR, doubles duration.
 * Whiran spell.  Whira mutur grol.
 */
void
spell_feather_fall(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Feather_Fall */
    struct affected_type af;
    int duration;

    memset(&af, 0, sizeof(af));

    check_crim_cast(ch);

    if (IS_AFFECTED(victim, CHAR_AFF_FLYING)) {
        act("Nothing happens.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    if (ch->in_room->sector_type == SECT_AIR)
        duration = level * 10;
    else if (ch->in_room->sector_type == SECT_SILT)
        duration = level / 2;
    else
        duration = level * 3;

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_FEATHER_FALL;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_NONE;
    af.modifier = 0;
    af.bitvector = CHAR_AFF_FLYING;

    affect_to_char(victim, &af);

    if (victim == ch) {
        act("You begin to walk with a lighter step.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n begins to walk with a lighter step.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("You send a quick breeze towards $N and $E begins to walk with a lighter step.", FALSE,
            ch, 0, victim, TO_CHAR);
        act("$n sends a quick breeze towards you, and you begin to walk with a lighter step.",
            FALSE, ch, 0, victim, TO_VICT);
        act("$n sends a quick breeze towards $N and $E begins to walk with a lighter step.", TRUE,
            ch, 0, victim, TO_NOTVICT);
    }

    return;
}                               /* end Spell_Feather_Fall */


/* FEEBLEMIND: Affects the target's wisdom.  If they have a
 * psionic barrier up, they will be unaffected.  Non cumulative.
 * Duration depends on power level. Rukkian spell. Ruk mutur hurn.
 */
void
spell_feeblemind(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Feeblemind */
    struct affected_type af;
    int magick;
    int duration;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in feeblemind");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(victim, CHAR_AFF_PSI) && affected_by_spell(victim, PSI_BARRIER)) {
        act("You feel a tingling in your mental barrier.", FALSE, victim, 0, 0, TO_CHAR);
        send_to_char("Your magick encounters some obstacle.\n\r", ch);
        return;
    }

    if (ch != victim) {
        act("You make a sharp gesture at $N with your balled fist.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n makes a sharp gesture at you with a balled fist.", FALSE, ch, 0, victim, TO_VICT);
        act("$n makes a sharp gesture at $N with a balled fist.", TRUE, ch, 0, victim, TO_NOTVICT);
        if (!does_save(victim, SAVING_SPELL, ((level - 2) * (-5)))) {
            act("You feel less magickally potent.", FALSE, victim, 0, 0, TO_CHAR);
            send_to_char("You cloud their mind.\n\r", ch);
        } else
            return;
    }

    if (ch->in_room->sector_type == SECT_SILT)
        duration = level * 3;
    else
        duration = level * 2;

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_FEEBLEMIND;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = -2;
    af.location = CHAR_APPLY_WIS;
    af.bitvector = CHAR_AFF_FEEBLEMIND;
    stack_spell_affect(victim, &af, 24);

    magick = ((level / 7.0) * GET_MAX_MANA(victim));
    adjust_mana(victim, -magick);

/*
   af.type = SPELL_FEEBLEMIND;
   af.duration = level * 2;
   af.modifier = (-magick);
   af.location = CHAR_APPLY_MANA;
   af.bitvector = 0;

  affect_to_char (victim, &af); */

}                               /* end Spell_Feeblemind */


/* FIRE ARMOR: Creates a shield of protective flames around the
 * caster, which is visible to the naked eye. Cumulative;
 * duration is dependent on power level. Suk Krathi spell.
 * Sul-krath pret grol.  Casting messages vary in some sectors;
 * SECT_SILT reduces duration to half.
 */
void
spell_fire_armor(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* Begin Spell_Fire_Armor */
    struct affected_type af;
    int duration;
    int sector;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in fire armor");
        return;
    }

    check_crim_cast(ch);

    if (affected_by_spell(victim, SPELL_ENERGY_SHIELD)) {
        send_to_char("The sparks already surrounding your form " "disperse the flames.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_STONE_SKIN)) {
        send_to_char("The flames sink into your stony skin and " "disappear.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_WIND_ARMOR)) {
        send_to_char("The gusting winds already surrounding your form " "disperse the flames.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
        send_to_char("The shadows surrounding you prevent the fires from taking hold.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
        send_to_char("The flames surrounding your form leap up as though " "renewed.\n\r", victim);
        act("The flames surrounding $N's form leap up with new intensity.", FALSE, ch, 0, victim,
            TO_NOTVICT);
    } else {
        sector = ch->in_room->sector_type;
        switch (sector) {
        case SECT_AIR:
            act("Coalescing from the surrounding air, flames engulf your body.", FALSE, victim, 0,
                0, TO_CHAR);
            act("Coalescing from the surrounding air, flames engulf $N's body.", FALSE, victim, 0,
                victim, TO_NOTVICT);
            break;
        case SECT_CAVERN:
            act("A ruddy light plays across the surrounding stony walls as flames engulf your body.", FALSE, victim, 0, 0, TO_CHAR);
            act("A ruddy light plays across the surrounding stony walls as flames engulf $N's body.", FALSE, victim, 0, victim, TO_NOTVICT);
            break;
        case SECT_LIGHT_FOREST:
        case SECT_HEAVY_FOREST:
        case SECT_THORNLANDS:
            act("The surrounding vegetation begins to smoulder as flames engulf your body.", FALSE,
                victim, 0, 0, TO_CHAR);
            act("The surrounding vegetation begins to smoulder as flames engulf $N's body.", FALSE,
                victim, 0, victim, TO_NOTVICT);
            break;
        case SECT_SILT:
            act("Silt and ash smoulder and spark in the air as feeble flames engulf your body.",
                FALSE, victim, 0, 0, TO_CHAR);
            act("Silt and ash smoulder and spark in the air as feeble flames engulf $N's body.",
                FALSE, victim, 0, victim, TO_NOTVICT);
            break;
        default:
            act("With a hissing sound, flames engulf your body.", FALSE, victim, 0, 0, TO_CHAR);
            act("With a hissing sound, flames engulf $N's body.", FALSE, victim, 0, victim,
                TO_NOTVICT);
            break;
        }
    }

    if (affected_by_spell(victim, SPELL_FURY))
        duration = level * 2;
    else if (ch->in_room->sector_type == SECT_SILT)
        duration = level / 2;
    else
        duration = level;

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_FIRE_ARMOR;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = level * 2;
    af.bitvector = 0;
    af.location = CHAR_APPLY_AC;
    stack_spell_affect(victim, &af, 24);
}                               /* end of Spell_Fire_Armor */

void
spell_shadow_armor(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* Begin Spell_Shadow_Armor */
    struct affected_type af;
    int duration;
    int sector;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in shadow armor");
        return;
    }

    check_crim_cast(ch);

    if (affected_by_spell(victim, SPELL_ENERGY_SHIELD)) {
        send_to_char("The sparks already surrounding your form " "disperse the shadows.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_STONE_SKIN)) {
        send_to_char("The shadows sink into your stony skin and " "disappear.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_WIND_ARMOR)) {
        send_to_char("The gusting winds already surrounding your form " "disperse the shadows.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
        send_to_char("The fires surrounding your form disperse the shadows.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
        act("Nearby shadows are drawn to you, deepening those already surrounding you.", FALSE,
            victim, 0, 0, TO_CHAR);
        act("Nearby shadows flit towards $N's form, deepening those already surrounding $M.", FALSE,
            victim, 0, victim, TO_NOTVICT);

    } else {
        sector = ch->in_room->sector_type;
        switch (sector) {
        case SECT_AIR:
        case SECT_AIR_PLANE:
            act("Nearby shadows darken and flit toward you, surrounding you completely.", FALSE,
                victim, 0, 0, TO_CHAR);
            act("Nearby shadows darken and flit towards $N, surrounding $M completely.", FALSE,
                victim, 0, victim, TO_NOTVICT);
            break;
        case SECT_CAVERN:
            act("A dark shadow plays across the surrounding stony walls and then surrounds your body.", FALSE, victim, 0, 0, TO_CHAR);
            act("A dark shadow plays across the surrounding stony walls and then surrounds $N's body.", FALSE, victim, 0, victim, TO_NOTVICT);
            break;
        case SECT_LIGHT_FOREST:
        case SECT_HEAVY_FOREST:
        case SECT_THORNLANDS:
            act("The shadows beneath the surrounding vegetation lengthen and then surround your body.", FALSE, victim, 0, 0, TO_CHAR);
            act("The shadows beneath the surrounding vegetation lengthen and then surround $N's body.", FALSE, victim, 0, victim, TO_NOTVICT);
            break;
        default:
            act("Nearby shadows darken and flit toward you, surrounding you completely.", FALSE,
                victim, 0, 0, TO_CHAR);
            act("Nearby shadows darken and flit towards $N, surrounding $M completely.", FALSE,
                victim, 0, victim, TO_NOTVICT);
            break;
        }
    }

    if (ch->in_room->sector_type == SECT_SHADOW_PLANE)
        duration = level * 2;
    else if (ch->in_room->sector_type == SECT_SILT)
        duration = level / 2;
    else
        duration = level;

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_SHADOW_ARMOR;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = level * 2;
    af.location = CHAR_APPLY_AC;
    af.bitvector = 0;
    stack_spell_affect(victim, &af, 24);
}                               /* end of Spell_Shadow_Armor */



/* FIRE JAMBIYA: Creates a weapon made of pure flame for the 
 * caster.  Based on the sand jambiya spell.  At mon, with the
 * appropriate component, it creates a permanent staff item.
 * Suk Krathi and sorcerer spell.  Suk-Krath dreth hekro.
 * Started by Nessalin 11/8/1999, copied from sand_jambiya 
 */

void
spell_fire_jambiya(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Fire_Jambiya */

    int jambiya_num = 0;
    OBJ_DATA *tmp_obj;

    if (!ch) {
        gamelog("No ch in fire jambiya");
        return;
    }

    check_crim_cast(ch);

    if (ch->in_room->sector_type == SECT_WATER_PLANE) {
        send_to_char("The waters here block your ability to create a weapon of flames.\n\r", ch);
        return;
    }

    switch (level) {
    case 1:
        jambiya_num = 462;
        break;
    case 2:
        jambiya_num = 463;
        break;
    case 3:
        jambiya_num = 464;
        break;
    case 4:
        jambiya_num = 465;
        break;
    case 5:
        jambiya_num = 466;
        break;
    case 6:
        jambiya_num = 467;
        break;
    case 7:
        if (!get_componentB
            (ch, SPHERE_CONJURATION, level, "$p catches fire in your hands and turns to ash.",
             "$p catches fire in $n's hands and turns to ash.",
             "You do not have a powerful enough component to do that."))
            return;

        jambiya_num = 1386 + number(0, 7);      /* random object 1386 to 1393 */
        tmp_obj = read_object(jambiya_num, VIRTUAL);

        obj_to_char(tmp_obj, ch);

	set_creator_edesc(ch, tmp_obj);

        act("Flames roll down $n's arms, forming $p.", TRUE, ch, tmp_obj, 0, TO_ROOM);
        act("Flames roll down your arms, forming $p.", TRUE, ch, tmp_obj, 0, TO_CHAR);

        return;
    default:
        gamelog("UNKNOWN POWER LEVEL IN SPELL_FIRE_JAMBIYA()");
    }

    tmp_obj = read_object(jambiya_num, VIRTUAL);

    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);

    if (victim && (victim != ch)) {
        act("Flames roll down $n's arms, forming $p, which $s drops at $N's " "feet.", TRUE, ch,
            tmp_obj, victim, TO_NOTVICT);
        act("Flames roll down your arms, forming $p, which you drop at $N's " "feet.", TRUE, ch,
            tmp_obj, victim, TO_CHAR);
        act("Flames roll down $n's arms, forming $p, which $s drops at your " "feet.", TRUE, ch,
            tmp_obj, victim, TO_VICT);
        obj_to_room(tmp_obj, ch->in_room);
    } else {
        act("Flames roll down $n's arms, forming $p.", TRUE, ch, tmp_obj, 0, TO_ROOM);
        act("Flames roll down your arms, forming $p.", TRUE, ch, tmp_obj, 0, TO_CHAR);
        obj_to_char(tmp_obj, ch);
    }

    if (ch->in_room->sector_type == SECT_SHADOW_PLANE) {
        new_event(EVNT_FIREJAMBIYA_REMOVE, (((level + 1) * 1500) / 2), 0, 0, tmp_obj, 0, 0);
        act("The surrounding shadows weaken $p's strength.", TRUE, ch, tmp_obj, 0, TO_CHAR);
    } else
        new_event(EVNT_FIREJAMBIYA_REMOVE, (((level + 1) * 3000) / 2), 0, 0, tmp_obj, 0, 0);

    return;

}                               /* end Spell_Fire_Jambiya */


/* FIREBALL: Casts a flame attack at the target, which will not
 * affect fire elementals or Suk-Krathians.  Damage is depedent
 * on level.  Echoes to nearby rooms.  Suk-Krathi and sorcerer 
 * spell.  Suk-Krath divan hekro.
 */
void
spell_fireball(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Fireball */
    int dam;

    if (!ch) {
        gamelog("No ch in fireball");
        return;
    }

    if (!victim) {
        gamelog("No victim in fireball");
        return;
    }
    dam = number(10, 15) * level;
    qroomlogf(QUIET_COMBAT, ch->in_room, "fireball base dam:  %d", dam);

    if (!((GET_RACE(victim) == RACE_ELEMENTAL) && (GET_GUILD(victim) == GUILD_FIRE_ELEMENTALIST))) {
        if (does_save(victim, SAVING_SPELL, agl_app[GET_AGL(ch)].reaction))
            dam >>= 1;

        if (!(affected_by_spell(victim, SPELL_FIRE_ARMOR)))
            sflag_to_char(victim, OST_BURNED);

        if ((ch->in_room->sector_type == SECT_WATER_PLANE)
            || (ch->in_room->sector_type == SECT_SHADOW_PLANE))
            dam = dam / 2;

        if (affected_by_spell(victim, SPELL_FIRE_ARMOR))
            dam = dam / 2;

        if (ch->in_room->sector_type == SECT_WATER_PLANE) {
            act("The water in the area weakens your spell with a hiss of steam.", FALSE, ch, 0, 0,
                TO_CHAR);
            act("The water in the area weakens $n's spell with a hiss of steam.", FALSE, ch, 0, ch,
                TO_ROOM);
        }

        if (ch->in_room->sector_type == SECT_SHADOW_PLANE) {
            act("The shadows in the area coalesce near you, weakening your spell.", FALSE, ch, 0, 0,
                TO_CHAR);
            act("The shadows in the area coalesce near $n, weakening the spell.", FALSE, ch, 0, ch,
                TO_ROOM);
        }

        if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
            send_to_char
                ("The flames protecting you flare briefly, absoring some of the attack..\n\r",
                 victim);
            act("The flames surrounding $n flare briefly, absorbing some of the attack.", FALSE,
                victim, 0, 0, TO_ROOM);
        }

        qroomlogf(QUIET_COMBAT, ch->in_room, "fireball final dam: %d", dam);
        ws_damage(ch, victim, dam, 0, 0, dam, SPELL_FIREBALL, TYPE_ELEM_FIRE);

    } else {
        adjust_hit(victim, dam);
        update_pos(victim);

        act("An intense flame passes over you, and you feel rejuvenated.", FALSE, ch, 0, victim,
            TO_VICT);
        act("You throw a ball of fire towards $N.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n throws a ball of fire towards $N.", FALSE, ch, 0, victim, TO_ROOM);
        act("The fire is absorbed into $N's body.", FALSE, ch, 0, victim, TO_NOTVICT);
        act("The fire is absorbed into $N's body.", FALSE, ch, 0, victim, TO_CHAR);
        act("You feel a sudden wash of heat from the $D.", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);
    }
}                               /* end Spell_Fireball */

/* FIREBREATHER: (Taken from the helpfile) This spell calls upon 
* the element of Suk-Krath to invert the life-giving effects of precious 
* water. Anything drunk while under the affect of this spell will cause 
* damage to the target as the very liquids burn within them, making it 
* the nightmare of desert wanderers and tavern regulars alike.
* Note: creatures with affect FIREBREATHER will be unharmed by the
* PARCH spell.  Suk Krathi spell.  Suk-Krath nathro viod.
*/
void
spell_firebreather(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Firebreather */
    struct affected_type af;
    int duration;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in firebreather");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (affected_by_spell(victim, SPELL_BREATHE_WATER)) {
        send_to_char("Nothing seems to happen as your spell fizzles uselessly.\n\r", ch);
        return;
    }

    duration = ((level * 5) - get_char_size(victim));

    if (duration < 0) {
        act("You gesture at $N but your spell dissipates around $M.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("You feel a slight thirsting sensation as $n gestures at you.", FALSE, ch, 0, victim,
            TO_VICT);
        act("A reddish glow surrounds $N then dissipates as $n gestures at $M.", FALSE, ch, 0,
            victim, TO_NOTVICT);
        GET_COND(victim, THIRST) = MAX(0, GET_COND(victim, THIRST) - 5);
        return;
    }

    act("You gesture at $N and $E begins to twitch and sweat.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n gestures at you and you feel a burning sensation within your veins.", FALSE, ch, 0,
        victim, TO_VICT);
    act("$n gestures at $N and $E begins to twitch and sweat.", TRUE, ch, 0, victim, TO_NOTVICT);

    if (affected_by_spell(victim, SPELL_FIREBREATHER))
        duration = (duration + number(1, (level + 1)));

    af.type = SPELL_FIREBREATHER;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = 0;

    affect_join(victim, &af, TRUE, FALSE);

}                               /* end Spell_Firebreather */


/* FLAMESTRIKE: Hits target with a flame attack, which will
 * not affect fire elementals and Suk-Krathians.  Echoes to
 * surrounding rooms.  Suk Krathi spell.  Suk-Krath viqrol hekro.
 */
void
spell_flamestrike(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Flamestrike */
    int dam;

    if (!ch) {
        gamelog("No ch in flamestrike");
        return;
    }

    if (!victim) {
        gamelog("No victim in flamestrike");
        return;
    }

    dam = dice(level + 3, 8);

    if (!((GET_RACE(victim) == RACE_ELEMENTAL) && (GET_GUILD(victim) == GUILD_FIRE_ELEMENTALIST))) {
        if (does_save(victim, SAVING_SPELL, agl_app[GET_AGL(ch)].reaction))
            dam >>= 1;

        if ((ch->in_room->sector_type == SECT_WATER_PLANE)
            || (ch->in_room->sector_type == SECT_SHADOW_PLANE))
            dam = dam / 2;

        if (affected_by_spell(victim, SPELL_FIRE_ARMOR))
            dam = dam / 2;

        if (ch->in_room->sector_type == SECT_WATER_PLANE) {
            act("The water in the area weakens your spell with a hiss of steam.", FALSE, ch, 0, 0,
                TO_CHAR);
            act("The water in the area weakens $n's spell with a hiss of steam.", FALSE, ch, 0, ch,
                TO_ROOM);
        }

        if (ch->in_room->sector_type == SECT_SHADOW_PLANE) {
            act("The shadows coalesce near you, weakening your spell.", FALSE, ch, 0, 0, TO_CHAR);
            act("The shadows coalesce near $n, weakening the spell.", FALSE, ch, 0, ch, TO_ROOM);
        }

        if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
            send_to_char
                ("The flames protecting you flare briefly, absoring some of the attack..\n\r",
                 victim);
            act("The flames surrounding $n flare briefly, absorbing some of the attack.", FALSE,
                victim, 0, 0, TO_ROOM);
        }

        ws_damage(ch, victim, dam, 0, 0, dam, SPELL_FLAMESTRIKE, TYPE_ELEM_FIRE);

    } else {
        adjust_hit(victim, dam);
        update_pos(victim);

        act("You send a column of searing flames towards $N.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n hurls a column of searing flames towards you.", FALSE, ch, 0, victim, TO_VICT);
        act("$n sends a column of searing flames towards $N.", FALSE, ch, 0, victim, TO_ROOM);
        act("You feel a sudden wash of heat from the $D.", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);

        act("You absorb the flames into your body, and revel in ecstasy.", FALSE, ch, 0, victim,
            TO_VICT);
        act("The flames are absorbed into $N's body", FALSE, ch, 0, victim, TO_ROOM);
        act("The flames are absorbed into $N's body", FALSE, ch, 0, victim, TO_CHAR);
        act("You feel a sudden wash of heat from the $D.", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);
    }
}                               /* end Spell_Flamestrike */


/* FLOURESCENT FOOTSTEPS: Causes the recipient to leave glowing
 * footprints.  Duration dependent on power level.  Elkros spell.
 * Elkros mutur inrof.
 * Note: this hideous misspelling (should be fluorescent) needs
 * to be fixed at some point.
 */
void
spell_fluorescent_footsteps(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                            OBJ_DATA * obj)
{                               /* begin Spell_Flourescent_Footsteps */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in fluorescent footsteps");
        return;
    }

    check_crim_cast(ch);

    if (ch->in_room->sector_type == SECT_ROAD)
        duration = number(1, level + 5);
    else
        duration = number(1, level + 1);

    if (victim == ch) {
        act("A crackling surge of energy swirls around your feet, then fades from view.", FALSE, ch,
            0, 0, TO_CHAR);
        act("A crackling surge of energy swirls around $n's feet, then fades from view.", FALSE, ch,
            0, 0, TO_ROOM);
    } else {
        act("Crackling energy surges from your hands into $N's feet, then fades from view.", FALSE,
            ch, 0, victim, TO_CHAR);
        act("Crackling energy surges from $n's hands into your feet, then fades from view.", FALSE,
            ch, 0, victim, TO_VICT);
        act("Crackling energy surges from $n's hands into $N's feet, then fades from view.", TRUE,
            ch, 0, victim, TO_NOTVICT);
    }

    af.type = SPELL_FLOURESCENT_FOOTSTEPS;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;
    stack_spell_affect(victim, &af, 24);
}                               /* end Spell_Flourescent_Footsteps */


/* FLY: Allows the target to fly, for a duration dependent on
 * power level. Whiran and sorcerer spell.  Whira divan chran.
 */
void
spell_fly(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Fly */
    struct affected_type af;
    int duration;

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in fly");
        return;
    }

    if (!victim) {
        gamelog("No victim in fly");
        return;
    }

    check_crim_cast(ch);

    if (ch->in_room->sector_type == SECT_AIR)
        duration = ((level * 5) - (get_char_size(victim) / 2));
    else if (ch->in_room->sector_type == SECT_AIR_PLANE) {
        duration = ((level * 7) - (get_char_size(victim) / 2));
        send_to_char("You feel a surge in your magick here, causing a greater affect.\n\r", ch);
    } else
        duration = ((level * 4) - (get_char_size(victim) / 2));

    if (duration < 0) {
        send_to_char("Your feet rise off the ground momentarily, then float back down.\n\r",
                     victim);
        act("$N's feet rise off the ground momentarily, then float back down.", FALSE, ch, 0,
            victim, TO_ROOM);
        return;
    }

    if ((IS_AFFECTED(victim, CHAR_AFF_FLYING)) && (!affected_by_spell(victim, SPELL_FLY))
        && (!affected_by_spell(victim, SPELL_LEVITATE))
        && (!affected_by_spell(victim, SPELL_FEATHER_FALL))) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    if (!IS_AFFECTED(victim, CHAR_AFF_FLYING)) {
        send_to_char("You feel light as your feet rise " "off the ground.\n\r", victim);
        act("$N's feet rise off the ground.", FALSE, ch, 0, victim, TO_ROOM);
    } else {
        if (affected_by_spell(victim, SPELL_FLY)) {
            send_to_char("You feel a little lighter.\n\r", victim);
            act("$N's feet rise a little higher.", FALSE, ch, 0, victim, TO_ROOM);
        }
        if ((affected_by_spell(victim, SPELL_LEVITATE))
            || (affected_by_spell(victim, SPELL_FEATHER_FALL))) {
            send_to_char("You feel lighter and in more control.\n\r", victim);
            act("$N's movements appear to stabilize.", FALSE, ch, 0, victim, TO_ROOM);

            if (affected_by_spell(victim, SPELL_LEVITATE))
                affect_from_char(victim, SPELL_LEVITATE);
            if (affected_by_spell(victim, SPELL_FEATHER_FALL))
                affect_from_char(victim, SPELL_FEATHER_FALL);
            REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_FLYING);
        }
    }

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_FLY;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = CHAR_AFF_FLYING;
    stack_spell_affect(victim, &af, 36);
}                               /* end Spell_Fly */


/* FORBID ELEMENTS: Sets the NO_ELEMENTAL_MAGICK flag on the room
 * for a duration dependent on level.  Also dispels any shadows within it
 * and exposes anyone ethereal or invisible.  Nilazi spell.
 * Nilaz nathro echri. 
 */
void
spell_forbid_elements(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                      OBJ_DATA * obj)
{                               /* begin Spell_Forbid_Elements */
    CHAR_DATA *tch, *next_tch;

    check_criminal_flag(ch, NULL);

    if ((ch->in_room->sector_type == SECT_AIR_PLANE)
        || (ch->in_room->sector_type == SECT_FIRE_PLANE)
        || (ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_EARTH_PLANE)
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)
        || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)) {
        act("Your spell has no affect here, and you reel from the attempt.", FALSE, ch, 0, 0,
            TO_CHAR);
        generic_damage(ch, level, 0, 0, level);
        return;
    }

    if (IS_SET(ch->in_room->room_flags, RFL_NO_ELEM_MAGICK)) {
        act("You feel the elements are already forbidden here.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    MUD_SET_BIT(ch->in_room->room_flags, RFL_NO_ELEM_MAGICK);

    act("$n slowly closes $s hand into a fist and an eerie stillness settles over the area.", FALSE,
        ch, 0, 0, TO_ROOM);
    act("You feel the elements flee the area, as your barrier descends.", FALSE, ch, 0, 0, TO_CHAR);

    /* Should send magick shadows back to their true selves here */
    for (tch = ch->in_room->people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;

        if (ch != tch) {
            if ((!IS_IMMORTAL(tch)) && (affected_by_spell(tch, SPELL_SEND_SHADOW))) {
                /* No magick shadows allowed here anymore */
                act("Your vision goes momentarily black...", FALSE, tch, 0, 0, TO_CHAR);
                act("$N quickly dissipates.", FALSE, ch, 0, tch, TO_CHAR);
                act("$N quickly dissipates.", FALSE, ch, 0, tch, TO_ROOM);
                parse_command(tch, "return");
                /* the victim (tch) no longer exists */
            }
            if ((!IS_IMMORTAL(tch)) && (affected_by_spell(tch, SPELL_ETHEREAL))) {
                affect_from_char(tch, SPELL_ETHEREAL);
                send_to_char("You fade back from the realm of shadows.\n\r", tch);
                act("$n's body returns from the plane of Drov.", FALSE, tch, 0, 0, TO_ROOM);
            }

            if ((!IS_IMMORTAL(tch)) && (affected_by_spell(tch, SPELL_INVISIBLE))) {
                affect_from_char(tch, SPELL_INVISIBLE);
                send_to_char("You feel exposed.\n\r", tch);
                act("$n fades into existence.", FALSE, tch, 0, 0, TO_ROOM);
            }
        }
    }

    new_event(EVNT_REMOVE_NOELEMMAGICK, level * 30 * 60, 0, 0, 0, ch->in_room, 0);

}                               /* end Spell_Forbid_Elements */


/* FURY: Sets the target with the courage and endurance to fight,
 * for a duration dependent on power level.  Non-cumulative.
 * Rukkian and sorcerer spell.  Ruk psiak hurn.
 */
void
spell_fury(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Fury */
    int duration, end_boost;
    int sector;
    struct affected_type af;
    struct affected_type *tmp_af;
    char logbuf[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int wasfurious = FALSE;

    memset(&af, 0, sizeof(af));

    check_crim_cast(ch);

    if (victim != ch) {
        act("You make a slicing motion in the air towards $N.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n makes a slicing motion in the air towards you.", FALSE, ch, 0, victim, TO_VICT);
        act("$n makes a slicing motion in the air towards $N.", TRUE, ch, 0, victim, TO_NOTVICT);
    }
    // Calm check
    if ((tmp_af = affected_by_spell(victim, SPELL_CALM))) {
        send_to_char("You feel less calm.\n\r", victim);
        act("$N's eyes glaze over briefly and $S composure appears less calm.", FALSE, ch, 0,
            victim, TO_NOTVICT);
        if (tmp_af->power <= level) {   // Not strong enough
            level -= tmp_af->power;
            affect_from_char(victim, SPELL_CALM);
        } else {                // Weaken it
            tmp_af->power -= level;
            return;
        }
        if (level <= 0)
            return;
    }

    sector = ch->in_room->sector_type;

    end_boost = number(level, ((level * level) / 2));

    switch (sector) {
    case SECT_AIR:
        duration = level * 2;
        break;
    case SECT_CITY:
    case SECT_INSIDE:
        duration = (level * 5) / 2;
        break;
    case SECT_HILLS:
    case SECT_MOUNTAIN:
        duration = level * 3.5;
        break;
    case SECT_THORNLANDS:
        duration = level * 3.3;
        break;
    case SECT_EARTH_PLANE:
        duration = level * 4;
        end_boost = end_boost + level;
        send_to_char("You feel a surge in your magick here, causing a greater affect.\n\r", ch);
        break;
    default:
        duration = level * 3;
        break;
    }

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_FURY;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = end_boost;
    af.location = CHAR_APPLY_END;
    af.bitvector = 0;

    if (affected_by_spell(victim, SPELL_FURY)) {
        wasfurious = TRUE;
        for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next) {
            if (tmp_af->type == SPELL_FURY) {
                duration = MIN((duration + tmp_af->duration), 36);
                memcpy(&af, tmp_af, sizeof(af));
                af.duration = duration;
                break;
            }
        }
        affect_from_char(victim, SPELL_FURY);
        cprintf(ch, "You feel renewed rage surge through you.\n\r");
    }

    affect_to_char(victim, &af);

    snprintf(buf, sizeof(buf), "%s feels enraged.", MSTR(victim, short_descr));
    send_to_empaths(victim, buf);

    change_mood(victim, NULL);

    /* Added instigating mul rage. - Tiernan 5/9/05 */
    if (GET_RACE(victim) == RACE_MUL) {
	if(!affected_by_spice(victim, SPICE_THODELIV)) {
            sprintf(logbuf, "%s cast fury upon %s, instigating mul rage in room %d.", GET_NAME(ch), (ch == victim ? "themself" : GET_NAME(victim)), ch->in_room->number);
	    gamelog(logbuf);

	    send_to_char("You feel a deep rage from within boil to the surface, and a blood-lust\n\rtakes control.", victim);

	    af.type = AFF_MUL_RAGE;
	    af.duration = duration;
	    af.modifier = CFL_AGGRESSIVE;
	    af.location = CHAR_APPLY_CFLAGS;
	    af.bitvector = 0;

	    affect_join(victim, &af, FALSE, FALSE);
	} else
            send_to_char("Your eyes glaze over and you feel quite irritable.\n\r", victim);
    } else if (!wasfurious)
        send_to_char("Your eyes glaze over as you enter a fighting rage.\n\r", victim);

    act("$N's eyes glaze over briefly.", FALSE, ch, 0, victim, TO_NOTVICT);
}                               /* end Spell_Fury */


/* GATE: This spell will call forth a powerful denizen of another plane,
 * typically an elemental of the same planar affinity of an elementalist,
 * or a demon for those without such a connection. Summoning forth such a
 * creature is difficult, at best, and controlling them even harder. Often,
 * such creatures are angry at having been bothered, though if the caster can
 * control it, the being will do anything and everything the caster bids. 
 * Requires a component.  Note: needs updating to make sure all guilds have
 * summonable creatures.  Nilaz dreth chran.
 */
void
spell_gate(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Gate */
    struct affected_type af;
    int vnum;
    int sp_pow, gated_followers;
    char buf[MAX_STRING_LENGTH];
    struct follow_type *follower;

    memset(&af, 0, sizeof(af));

    if (victim) {
      sp_pow = 6;
    } else {
      sp_pow = level;
    }
    
    if (!victim &&
        !get_componentB(ch, SPHERE_CONJURATION, sp_pow,
                        "$p folds in upon itself with an audible grinding noise.",
                        "$p folds in upon itself in $n's hands.",
                        "You lack the correct component to lure otherwordly creatures here.")) {
      if (victim) {
        extract_char(victim);
      }

      return;
    }

    check_criminal_flag(ch, NULL);

    /* Adding special case for Egamid, for Brixius plot 7/8/2002 -Sanvean */
    if (!strcasecmp(MSTR(ch, name), "egamid")) {
        vnum = 1111;
    } /* fiery ghost */
    else if (has_skill(ch, SKILL_GATHER)) {
            /*Preservers get random creature from other elements */
            if (ch->specials.eco >= 0) {
                switch (number(1, 6)) {
                case 1:
                    vnum = 1224 + level - 1;
                    break;
                case 2:
                    vnum = 1235 + level - 1;
                    break;
                case 3:
                    vnum = 1210 + level - 1;
                    break;
                case 4:
                    vnum = 1028 + level - 1;
                    break;
                case 5:
                    vnum = 1217 + level - 1;
                    break;
                case 6:
                    vnum = 1018 + level - 1;
                    break;
                default:
                    vnum = 1100 + level - 1;
                    break;
                }
            }
            /*Defilers get demons */
            else
                vnum = 1100 + level - 1;
	    // End sorcerer settings
    } else {
        switch (GET_GUILD(ch)) {
        case GUILD_FIRE_ELEMENTALIST:
            vnum = 1224 + level - 1;
            break;
        case GUILD_STONE_ELEMENTALIST:
            vnum = 1235 + level - 1;
            break;
        case GUILD_WATER_ELEMENTALIST:
            vnum = 1210 + level - 1;
            break;
        case GUILD_WIND_ELEMENTALIST:
            vnum = 1028 + level - 1;
            break;
        case GUILD_SHADOW_ELEMENTALIST:
            vnum = 1217 + level - 1;
            break;
        case GUILD_VOID_ELEMENTALIST:
            vnum = 1245 + level - 1;
            break;
        case GUILD_LIGHTNING_ELEMENTALIST:
            vnum = 1018 + level - 1;
            break;
        default:
            vnum = 1100 + level - 1;
            break;
        }
    }

    if (victim) {
        if (GET_RACE(victim) != RACE_DEMON && GET_RACE(victim) != RACE_FIEND
            && GET_RACE(victim) != RACE_THYGUN && GET_RACE(victim) != RACE_USELLIAM
            && GET_RACE(victim) != RACE_BEAST) {
            act("You are unable to invoke $N's presence with this spell.", TRUE, ch, 0, victim,
                TO_CHAR);
            return;
        }

        /* Named demons need sul or mon power to summon */
        if (level < 6) {
            act("Your spell lacks the power to bring $M here, and $E refuses.", FALSE, ch, 0,
                victim, TO_CHAR);
            extract_char(victim);
            return;
        }

        /* Same as messages below, but should be changed in the future. */
        act("$n speaks a word of power and $N appears in a shower of sparks.", TRUE, ch, 0, victim,
            TO_ROOM);
        act("You speak its true name and $N appears in a shower of sparks.", TRUE, ch, 0, victim,
            TO_CHAR);

        char_from_room(victim);
        char_to_room(victim, ch->in_room);
    } else {
        victim = read_mobile(vnum, VIRTUAL);

        sprintf(buf, "%s has gated in creature %d in room %d.", GET_NAME(ch), vnum,
                ch->in_room->number);

        gamelog(buf);

        // Commenting out this next short block - no longer needed.  Summoned creatures
        // hate the 'gate' affect that expires. - Halaster 01/03/2005
        // if (vnum > 1109) {      /* so it affects elementals only */
        //     victim->points.max_hit = 50 + (level * 10);
        //     victim->points.hit = 50 + (level * 10);
        //     new_event(EVNT_REMOVE_MOBILE, level * 4 * 60, victim, 0, 0, 0, 0);
        // }

        char_to_room(victim, ch->in_room);

        act("$n concentrates visibly as $N fades into existence.", TRUE, ch, 0, victim, TO_ROOM);
        act("You focus your will as $N is brought to this plane.", TRUE, ch, 0, victim, TO_CHAR);
    }

    int dur = level + number(2, 4);
    af.type = SPELL_GATE;
    af.duration = dur;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = CHAR_AFF_SUMMONED;
    affect_join(victim, &af, TRUE, FALSE);

    /* Count the gated follows this guy already has */
    gated_followers = 0;
    for (follower = ch->followers; follower; follower = follower->next) {
        CHAR_DATA *tmp_ch = follower->follower;
        if (affected_by_spell(tmp_ch, SPELL_GATE))
            gated_followers++;
    }

    int summon_danger = 3 + (gated_followers * 40);
    int roll = number(1, 100);
    qroomlogf(QUIET_LOG, ch->in_room,
              "%s summons with summon_danger of %d%% and roll of %d",
              MSTR(ch, name), summon_danger, roll);

    if (roll < summon_danger) {
        act("You struggle to control $N, but $E breaks free from your control.", FALSE, ch, 0,
            victim, TO_CHAR);
        act("$n and $N stare at each other intensely.", FALSE, ch, 0, victim, TO_ROOM);
        act("$N throws $S head back in exultation, and lunges towards $n.", FALSE, ch, 0, victim,
            TO_ROOM);
        if ((IS_NPC(victim)) && (!victim->specials.fighting)) {
            find_ch_keyword(ch, victim, buf, sizeof(buf));
            cmd_kill(victim, buf, CMD_KILL, 0);
        }
        MUD_SET_BIT(victim->specials.act, CFL_AGGRESSIVE);
    } else {
        add_follower(victim, ch);

        af.type = SPELL_CHARM_PERSON;
        af.duration = dur;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = 0;
        af.location = 0;
        af.bitvector = CHAR_AFF_CHARM;
        affect_to_char(victim, &af);
    }

    // Added next 5 lines to make sure NPC is at full hps/mana/stun/move when loading 12/29/2004 Halaster
    set_mana(victim, GET_MAX_MANA(victim));
    set_hit(victim, GET_MAX_HIT(victim));
    set_move(victim, GET_MAX_MOVE(victim));
    set_stun(victim, GET_MAX_STUN(victim));
    update_pos(victim);

    if (!number(0, 9) && vnum <= 1109 && vnum >= 1100
        && !IS_SET(victim->specials.act, CFL_AGGRESSIVE))
        MUD_SET_BIT(victim->specials.act, CFL_AGGRESSIVE);

    check_criminal_flag(ch, NULL);
    check_criminal_flag(victim, NULL);
    parse_command(victim, "pray");
}                               /* end Spell_Gate */


/* GLYPH: Cast on an object, it sets a glyph on it that will
 * protect it from being stolen or taken.  Krathi spell.
 * Suk-Krath psiak hurn.
 */
void
spell_glyph(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Glyph */

    if (!obj) {
        gamelog("No obj in glyph.");
        return;
    }

    check_crim_cast(ch);

    act("$n casts a glyph upon $p.", FALSE, ch, obj, 0, TO_ROOM);
    act("You cast a glyph upon $p.", FALSE, ch, obj, 0, TO_CHAR);

    MUD_SET_BIT(obj->obj_flags.extra_flags, OFL_GLYPH);

    if (!IS_SET(obj->obj_flags.extra_flags, OFL_MAGIC))
        MUD_SET_BIT(obj->obj_flags.extra_flags, OFL_MAGIC);
#ifdef USE_OBJ_FLAG_TEMP
    obj->obj_flags.temp = level;
#endif
}                               /* end Spell_Glyph */


/* GODSPEED: Allows the target to run faster than normal beings,
 * for a duration dependent on power level.  Negates the effects
 * of slow and levitate.  Rukkian and sorcerer spell.  The duration
 * is doubled if cast in SECT_ROAD, halved in SECT_SILT.  Cannot be
 * cast on someone with stoneskin on them.
 * Krok mutur wril.
 */
void
spell_godspeed(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Godspeed */
    struct affected_type af;
    struct affected_type *tmp_af;

    int duration = 0;
    int sector;

    memset(&af, 0, sizeof(af));

    check_crim_cast(ch);

    if (affected_by_spell(victim, SPELL_SLOW)) {
        if (victim == ch) {
            act("You bring your hands together and your body speeds back up to normal.", FALSE, ch,
                0, 0, TO_CHAR);
            act("$n brings $s hands togther and $s body speeds back up to normal.", FALSE, ch, 0, 0,
                TO_ROOM);
        } else {
            act("You bring your hands together and $N's body speeds back up to normal.", FALSE, ch,
                0, victim, TO_CHAR);
            act("$n brings $s hands together and your body speeds back up to normal.", FALSE, ch, 0,
                victim, TO_VICT);
            act("$n brings $s hands together and $N's body speeds back up to normal.", TRUE, ch, 0,
                victim, TO_NOTVICT);
        }
        affect_from_char(victim, SPELL_SLOW);
        return;
    }

    if (affected_by_spell(victim, SPELL_STONE_SKIN)) {
        send_to_char("Your limbs, weighted with stone, resist the spell.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_GODSPEED)) {
        send_to_char("Your limbs feel even more energetic.\n\r", victim);
    }

    else {
        sector = ch->in_room->sector_type;
        switch (sector) {
        case SECT_CITY:
        case SECT_INSIDE:
            send_to_char("Your legs feel fleet.\n\r", victim);
            act("$n's body begins to speed up.", FALSE, victim, 0, 0, TO_ROOM);
            duration = level * 2;
            break;
        case SECT_DESERT:
        case SECT_FIELD:
            send_to_char("Your legs feel fleet.\n\r", victim);
            act("$n's body begins to speed up.", FALSE, victim, 0, 0, TO_ROOM);
            duration = (level * 3) / 2;
            break;
        case SECT_ROAD:
            send_to_char("You feel the road beneath your feet, filling you with " "momentum.\n\r",
                         victim);
            act("$n's body begins to speed up.", FALSE, victim, 0, 0, TO_ROOM);
            duration = level * 3;
            break;
        case SECT_SILT:
            send_to_char("Despite the drag of the silt underfoot, your legs feel "
                         "a little more fleet.\n\r", victim);
            act("$n's body begins to speed up.", FALSE, victim, 0, 0, TO_ROOM);
            duration = level;
            break;
        case SECT_WATER_PLANE:
            send_to_char("There is too much water in the way for this spell to be effective.\n\r",
                         ch);
            return;
        case SECT_AIR_PLANE:
            send_to_char("The spell fizzles and dies, unable to function here.\n\r", ch);
            return;
        case SECT_EARTH_PLANE:
            send_to_char("Your legs feel fleet, and especially energetic here.\n\r", victim);
            act("$n's body begins to speed up with extra energy.", FALSE, victim, 0, 0, TO_ROOM);
            duration = level * 4;
            break;
        default:
            send_to_char("Your legs feel fleet.\n\r", victim);
            act("$n's body begins to speed up.", FALSE, victim, 0, 0, TO_ROOM);
            duration = level * 2;
            break;
        }
    }

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_GODSPEED;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;

    /* The next two lines put back in on March 20th, 2000 by Nessalin
     * If there's a reason they were removed, please comment here why  before
     * taking them out again.  Thanks. */
    af.modifier = dice(1, MAX(1, (level + 1) / 2));
    af.location = CHAR_APPLY_AGL;

    af.bitvector = CHAR_AFF_GODSPEED;

    if (affected_by_spell(victim, SPELL_GODSPEED)) {
        for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next) {
            if (tmp_af->type == SPELL_GODSPEED) {
                duration = MIN((duration + tmp_af->duration), 24);
                affect_from_char(victim, SPELL_GODSPEED);
                break;
            }
        }
        af.duration = duration;
    }

    affect_to_char(victim, &af);

    if (affected_by_spell(victim, SPELL_LEVITATE)) {
        affect_from_char(victim, SPELL_LEVITATE);
        send_to_char("Your body floats to the ground.\n\r", victim);
        act("$n's body floats to the ground.", FALSE, victim, 0, 0, TO_ROOM);
    }
}                               /* end Spell_Godspeed */


/* GOLEM */
/* Started by Nessalin 11/8/1999  */
void
spell_golem(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{
    OBJ_DATA *tmp_obj;
    int totem_vnum = 0;
    int STR = 0, END = 0, AGIL = 0;
    int height = 0, weight = 0;
    char pre_to_char[512], pre_to_room[512], post_to_char[512], post_to_room[512];

    if (!ch)
        return;
    if (!ch->in_room)
        return;

    // Removing for the time being, too many abuse issues.
    // -Nessalin 11/2/2002

    if (!IS_IMMORTAL(ch)) {
        send_to_char("Disabled.  Removing skill.", ch);
        if (has_skill(ch, SPELL_GOLEM))
            ch->skills[SPELL_GOLEM]->learned = 0;
        return;
    }
    // End remove skill code.

    switch (GET_GUILD(ch)) {
    case GUILD_FIRE_ELEMENTALIST:
        if ((IS_SET(ch->in_room->room_flags, RFL_INDOORS))
            || (time_info.hours < 2 || time_info.hours > 5)) {
            send_to_char("There is not enough light from Suk-Krath to perform" " this spell.\n\r",
                         ch);
            return;
        }
        sprintf(pre_to_char, "%s",
                "Spreading your arms wide you gaze upward, when your hands"
                " begin to warm up you bring them together over your head.");
        sprintf(pre_to_room, "%s",
                "$n spreads $s arms wide and gazes upwards then slowly brings"
                " $s hands together over $s head.");
        sprintf(post_to_char, "%s", "Bringing your hands down you open them to find $p.");
        sprintf(post_to_room, "%s", "$n brings $s hands down and opens them to reveal $p.");
        totem_vnum = 923;
        break;
    case GUILD_WATER_ELEMENTALIST:
        sprintf(pre_to_char, "%s",
                "Water begins to drip from your hands as you place them" " together.");
        sprintf(pre_to_room, "%s",
                "Water begins to drip from $n's hands as $e places them" " together.");
        sprintf(post_to_char, "%s", "You open your hands to reveal $p.");
        sprintf(post_to_room, "%s", "$n opens $s hands to reveal $p.");
        totem_vnum = 919;
        height = 95;
        weight = 60;
        break;
    case GUILD_STONE_ELEMENTALIST:
        if (ch->in_room->sector_type != SECT_DESERT) {
            send_to_char("There is not enough sand here.\n\r", ch);
            return;
        }
        sprintf(pre_to_char, "%s",
                "You kneel and scoop sand into your hands and quickly mold it as "
                "it heats up momentarily becoming hot and clay-like.");
        sprintf(pre_to_room, "%s",
                "$n kneels and scoops some sand up into $s hands which becomes "
                "maleable judging by the way $e molds it quickly.");
        sprintf(post_to_char, "%s", "You open your hands to find $p, which cools quickly.");
        sprintf(post_to_room, "%s", "$n opens $s hands to reveal $p.");
        totem_vnum = 915;
        STR = 40;
        AGIL = 6;
        height = 75;
        break;
    case GUILD_LIGHTNING_ELEMENTALIST:
        sprintf(pre_to_char, "%s",
                "Sparks begin to flash between your hands as you bring them"
                " together in front of you.");
        sprintf(pre_to_room, "%s",
                "Sparks begin to flash between $n's hands as $m brings them" " together.");
        sprintf(post_to_char, "%s",
                "There is a sudden flash from between your closed hands, when"
                " you open them $p is there.");
        sprintf(post_to_room, "%s",
                "There is a sudden flash from between $n's closed hands, when"
                "$e opens them $p is there.");
        totem_vnum = 917;
        STR = 25;
        AGIL = 30;
        height = 55;
        weight = 50;
        break;
    case GUILD_WIND_ELEMENTALIST:
        if (IS_SET(ch->in_room->room_flags, RFL_INDOORS)) {
            send_to_char("The walls around you hamper your spell.\n\r", ch);
            return;
        }
        sprintf(pre_to_char, "%s",
                "You cup your hands and exhale into them as sudden gusts of"
                "wind begin to gather around you.");
        sprintf(pre_to_room, "%s",
                "$n cups $s hands and exhaules into them as sudden gusts of"
                "wind being to gather around $m.");
        sprintf(post_to_char, "%s", "You open your hands to find $p.");
        sprintf(post_to_room, "%s", "$n opens $s hands to reveal $p");
        totem_vnum = 921;
        STR = 25;
        AGIL = 15;
        height = 88;
        weight = 40;
        break;
    case GUILD_VOID_ELEMENTALIST:
        sprintf(pre_to_char, "%s", "You put your hands together and a silence fills the area.");
        sprintf(pre_to_room, "%s",
                "The world momentarily goes silent as $n puts $s hands " "together.");
        sprintf(post_to_char, "%s", "Opening your hands you see $p.");
        sprintf(post_to_room, "%s", "$n opens $s hands to reveal $p.");
        totem_vnum = 925;
        END = 40;
        AGIL = 8;
        break;
    case GUILD_SHADOW_ELEMENTALIST:
        if ((time_info.hours > 1 || time_info.hours > 7)
            && (!(IS_SET(ch->in_room->room_flags, RFL_INDOORS)))) {
            send_to_char("There is too much light from Suk-Krath to perform" " this spell.\n\r",
                         ch);
            return;
        }
        sprintf(pre_to_char, "%s",
                "Your hands are enveloped in sudden darkness as you bring them" " together.");
        sprintf(pre_to_room, "%s",
                "$n's hands are enveloped in sudden darkness as $e brings them" " together.");
        sprintf(post_to_char, "%s",
                "As the darkness fades away your hands again become visible."
                " Opening them you find $p.");
        sprintf(post_to_room, "%s",
                "The darkness around $m's hands fades away, when $e opens them"
                " you see $e is holding $p.");
        totem_vnum = 927;
        break;
    case GUILD_NOBLE:
    case GUILD_DEFILER:
    case GUILD_BURGLAR:
    case GUILD_PICKPOCKET:
    case GUILD_WARRIOR:
    case GUILD_TEMPLAR:
    case GUILD_RANGER:
    case GUILD_MERCHANT:
    case GUILD_ASSASSIN:
    case GUILD_PSIONICIST:
    case GUILD_LIRATHU_TEMPLAR:
    case GUILD_JIHAE_TEMPLAR:
    case GUILD_NONE:
        send_to_char("Not done yet.\n\r", ch);
        return;
        //  case default:
        //    return;
    }

    check_crim_cast(ch);

    act(pre_to_char, FALSE, ch, 0, 0, TO_CHAR);
    act(pre_to_room, FALSE, ch, 0, 0, TO_ROOM);

    tmp_obj = read_object(totem_vnum, VIRTUAL);

    if (!tmp_obj)
        return;

    obj_to_char(tmp_obj, ch);

    new_event(EVNT_GOLEM_REMOVE, level * 3 * 60, 0, 0, tmp_obj, 0, 0);

    act(post_to_char, FALSE, ch, tmp_obj, 0, TO_CHAR);
    act(post_to_room, FALSE, ch, tmp_obj, 0, TO_ROOM);



    return;
}


/* GUARDIAN: Summons 1-7 creatures, depending on the power level.
 * Casting at any power greater than wek requires a component.
 * Summoned creatures vary according to whether caster is GUILD
 * WIND_ELEMENTALIST, GUILD_DEFILER, GUILD_TEMPLAR or tribe 24.
 * Whiran and sorcerer spell.  Whira dreth hekro.
 */
void
spell_guardian(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Guardian */
    char buf[256];
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    /* OBJ_DATA *dreth; */
    struct affected_type af;
    /* Experimenting with making different types, according to */
    /* guild, and using the Muark as guinea pigs.    -San      */
    int i, mobnum;
    int tribe = number(1900, 1999);

    memset(&af, 0, sizeof(af));

    if ((ch->in_room->sector_type == SECT_EARTH_PLANE)
        || ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))) {
        act("You are unable to reach Whira for a guardian here.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    act("A wind howls near you as you call to Whira.", FALSE, ch, 0, 0, TO_CHAR);
    act("A wind howls near $n as $e calls to Whira.", FALSE, ch, 0, 0, TO_ROOM);

    if ((level > 1)
        &&
        (!get_componentB
         (ch, SPHERE_CONJURATION, level,
          "$p lets off an emerald green light before crumbling to dust.",
          "$p crumbles to dust in $n's hand.",
          "You lack the correct component to call that many creatures.")))
        return;

    check_criminal_flag(ch, NULL);

    switch (level) {
    case 1:
        if (IS_TRIBE(ch, 24))
            sprintf(buf, "You entreat a djinn into taking form.");
        else if (IS_TRIBE(ch, 14))
            sprintf(buf, "You entreat the Land for a guardian.");
        else
            sprintf(buf, "You summon a guardian from the Plane of Wind.");
        break;
    case 2:
        if (IS_TRIBE(ch, 24))
            sprintf(buf, "You entreat two djinni into taking form.");
        else if (IS_TRIBE(ch, 14))
            sprintf(buf, "You entreat the Land for a guardian.");
        sprintf(buf, "You summon two guardians from the Plane of Wind.");
        break;
    case 3:
        if (IS_TRIBE(ch, 24))
            sprintf(buf, "You entreat three djinni into taking form.");
        else if (IS_TRIBE(ch, 14))
            sprintf(buf, "You entreat the Land for a guardian.");
        else
            sprintf(buf, "You summon three guardians from the Plane of Wind.");
        break;
    case 4:
        if (IS_TRIBE(ch, 24))
            sprintf(buf, "You entreat four djinni into taking form.");
        else if (IS_TRIBE(ch, 14))
            sprintf(buf, "You entreat the Land for a guardian.");
        else
            sprintf(buf, "You summon four guardians from the Plane of Wind.");
        break;
    case 5:
        if (IS_TRIBE(ch, 24))
            sprintf(buf, "You entreat five djinni into taking form.");
        else if (IS_TRIBE(ch, 14))
            sprintf(buf, "You entreat the Land for a guardian.");
        else
            sprintf(buf, "You summon five guardians from the Plane of Wind.");
        break;
    case 6:
        if (IS_TRIBE(ch, 24))
            sprintf(buf, "You entreat six djinni into taking form.");
        else if (IS_TRIBE(ch, 14))
            sprintf(buf, "You entreat the Land for a guardian.");
        else
            sprintf(buf, "You summon six guardians from the Plane of Wind.");
        break;
    case 7:
        if (IS_TRIBE(ch, 24))
            sprintf(buf, "You entreat seven djinni into taking form.");
        else if (IS_TRIBE(ch, 14))
            sprintf(buf, "You entreat the Land for a guardian.");
        else
            sprintf(buf, "You summon seven guardians from the Plane of Wind.");
        break;
    default:
        break;
    }

    act(buf, FALSE, ch, 0, 0, TO_CHAR);

    if ((!IS_TRIBE(ch, 14)) || (!IS_TRIBE(ch, 24))) {
        act("An eerie howl resounds from the $D.", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);
    }

    for (i = 0; i < level; i++) {

/* Set so different guilds/tribes summon diff creatures -San */
        if (IS_TRIBE(ch, 24))
            mob = read_mobile(6033, VIRTUAL);
        else if ((IS_TRIBE(ch, 14)) && (GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST))
            mob = read_mobile(144, VIRTUAL);
        else if ((IS_TRIBE(ch, 14)) && (GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST))
            mob = read_mobile(146, VIRTUAL);
        else if ((IS_TRIBE(ch, 14)) && (GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST))
            mob = read_mobile(143, VIRTUAL);
        else if ((IS_TRIBE(ch, 14)) && (GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST))
            mob = read_mobile(148, VIRTUAL);
        else if ((IS_TRIBE(ch, 14)) && (GET_GUILD(ch) == GUILD_VOID_ELEMENTALIST))
            mob = read_mobile(149, VIRTUAL);
        else if ((IS_TRIBE(ch, 14)) && (GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST))
            mob = read_mobile(147, VIRTUAL);
        else if ((IS_TRIBE(ch, 14)) && (GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST))
            mob = read_mobile(145, VIRTUAL);
        else if (GET_GUILD(ch) == GUILD_TEMPLAR)
            mob = read_mobile(1151, VIRTUAL);
        else {

            if (TRUE ==
                get_char_extra_desc_value(ch, "[GUARDIAN_SPELL_VNUM]", buf1, MAX_STRING_LENGTH))
                mobnum = atoi(buf1);
            else {
                switch (number(1, 9)) {
                case 1:
                    mobnum = 1316;
                    break;
                case 2:
                    mobnum = 1317;
                    break;
                case 3:
                    mobnum = 1318;
                    break;
                case 4:
                    mobnum = 1319;
                    break;
                case 5:
                    mobnum = 1320;
                    break;
                case 6:
                    mobnum = 1321;
                    break;
                case 7:
                    mobnum = 1322;
                    break;
                case 8:
                    mobnum = 1323;
                    break;
                case 9:
                    mobnum = 1301;
                    break;
                default:
                    mobnum = 1301;
                    break;
                }
                act("You reach out to Whira, and make a bond with a guardian.", FALSE, ch, 0, 0,
                    TO_CHAR);
                gamelogf("%s has cast spell_guardian for the first time - setting edesc.",
                         GET_NAME(ch));
                sprintf(buf2, "%d", mobnum);
                set_char_extra_desc_value(ch, "[GUARDIAN_SPELL_VNUM]", buf2);
            }

            if (mobnum <= 0) {
                act("Your call goes unanswered.", FALSE, ch, 0, 0, TO_CHAR);
                gamelogf("%s has an edesc with a bad vnum set in spell_guardian.", GET_NAME(ch));
                return;
            }
            mob = read_mobile(mobnum, VIRTUAL);
            if (!mob) {
                act("Your call goes unanswered.", FALSE, ch, 0, 0, TO_CHAR);
                gamelogf("%s has an edesc with a bad vnum set in spell_guardian.", GET_NAME(ch));
                return;
            }

        }
        mob->specials.state = level + 1;
        mob->player.tribe = tribe;

        char_to_room(mob, ch->in_room);

        if ((IS_TRIBE(ch, 24)) || (IS_TRIBE(ch, 14)))
            act("The air shimmers as $n appears.", FALSE, mob, 0, 0, TO_ROOM);
        else
            act("$n appears in a cloud of grey dust, snapping and snarling.", FALSE, mob, 0, 0,
                TO_ROOM);

        // Make 'em like the caster
        add_to_likes(mob, ch);
        add_guard(mob, ch);

        af.type = SPELL_GUARDIAN;
        af.duration = 5 + number(0,level);
        /* af.duration = ((8 - level) * 5); */
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = 0;
        af.location = CHAR_APPLY_NONE;
        af.bitvector = CHAR_AFF_SUMMONED;

        affect_join(mob, &af, TRUE, FALSE);
    }
}                               /* End Spell_Guardian */



/* Function used in HANDS OF WIND and SUMMON spells */
void
templar_protected_damage(CHAR_DATA * caster, CHAR_DATA * templar, int level)
{
    int dam = 0;

    /* Sorcerer king now steps in to save his minion */
    if (IS_TRIBE(templar, TRIBE_ALLANAK_MIL)) {
        act("The image of an immense, scaled beast suddenly forms before" " you.", TRUE, caster, 0,
            0, TO_CHAR);
        act("The beast turns its gaze upon you, its eyes glowing a baleful" " red.", TRUE, caster,
            0, 0, TO_CHAR);
        act("The beast opens its maw, and a jet of fire shoots out," " engulfing you in flame.",
            TRUE, caster, 0, 0, TO_CHAR);
    } else if (IS_TRIBE(templar, 8)) {  /* Tuluk */
        act("The image of the white moon appears before you, shining intensely.", TRUE, caster, 0,
            0, TO_CHAR);
        act("The light of the moon pierces into your very being.", TRUE, caster, 0, 0, TO_CHAR);
    } else {
        act("A black cloud suddenly obscures your vision.", TRUE, caster, 0, 0, TO_CHAR);
        act("An enormous presence suddenly weighs upon you, and you lose" " your concentration.",
            TRUE, caster, 0, 0, TO_CHAR);
        act("You feel a crushing weight upon your mind.", TRUE, caster, 0, 0, TO_CHAR);
    }

    /* Damage the caster */
    level = MAX(level, 1);
    dam = number(level, level * 8);

    /* messages to caster about damage */
    act("$n grabs $s head in pain.", TRUE, caster, 0, 0, TO_ROOM);
    act("You grab your head in pain.", TRUE, caster, 0, 0, TO_CHAR);

    /* book keeping */
    generic_damage(caster, dam, 0, 0, dam);
    if (GET_POS(caster) == POSITION_DEAD) {
        act("The pain overwhelms you, and everything fades to black.", TRUE, caster, 0, 0, TO_CHAR);
        die(caster);
    }
}

// Represents the barrier that exists around Allanak preventing magick from getting in/out.
// This only applies to rooms flagged Allanak, we've purposely left the 'rinth and the sewers
// without protection. -Nessalin April, 2014

int
allanak_magic_barrier(CHAR_DATA * caster, ROOM_DATA * target_room)
{
  shhlog ("allanak_magic_barrier: starting.");
  
  if (IS_IMMORTAL(caster)) {
    return FALSE;
  }

  if (!caster || !caster->in_room) {
    shhlog("error in allanak_magic_barrier, either no caster provided or caster is not in room.");
    return FALSE;
  }

  if (caster->player.guild == GUILD_TEMPLAR && IS_TRIBE(caster, 12)) {
    shhlog("allanak_magic_barrier: caster is a tribe 12 templar, returning FALSE.");
    return FALSE;
  }

  bool caster_in_allanak = room_in_city(caster->in_room) == CITY_ALLANAK;
  bool target_room_in_allanak = target_room && room_in_city(target_room) == CITY_ALLANAK;

  if (target_room_in_allanak || caster_in_allanak) {
    shhlog("target_room_in_allanak || caster_in_allanak was true, returning TRUE.");
    return TRUE;
  }

  shhlog("allanak_magic_barrier: ending, returning FALSE.");

  return FALSE;
}

// Represents the barrier that exists around Tuluk preventing magick from getting in/out.
// This only applies to rooms flagged Tuluk, if possible the barrens should be left
// without protection. -Nessalin May, 2014

int
tuluk_magic_barrier(CHAR_DATA * caster, ROOM_DATA * target_room)
{
  shhlog ("tuluk_magic_barrier: starting.");
  if (IS_IMMORTAL(caster)) {
    return FALSE;
  }

  if (!caster || !caster->in_room) {
    shhlog("error in tuluk_magick_barrier, either no caster provided or caster is not in room.");
    return FALSE;
  }

  if ((caster->player.guild == GUILD_JIHAE_TEMPLAR || caster->player.guild == GUILD_LIRATHU_TEMPLAR)
      && IS_TRIBE(caster, 8)) {
    shhlog("tuluk_magic_barrier: caster is a tribe 8 templar, returning FALSE.");
    return FALSE;
  }

  bool caster_in_tuluk = room_in_city(caster->in_room) == CITY_TULUK;
  bool target_room_in_tuluk = target_room && room_in_city(target_room) == CITY_TULUK;

  if (target_room_in_tuluk || caster_in_tuluk) {
    shhlog("target_room_in_tuluk || caster_in_tuluk was true, returning TRUE.");
    return TRUE;
  }

  shhlog("tuluk_magic_barrier: ending, returning FALSE.");

  return FALSE;
}

int
templar_protected(CHAR_DATA * caster, CHAR_DATA * templar, int spell, int level)
{
    ROOM_DATA *target, *orig_room;

    /* If victim is a templar in their own city-state, they are protected */
    if (!(is_templar_in_city(templar)))
        return 0;

    switch (spell) {
    case SPELL_SUMMON:
        /* messages to source room */
        act("$n fades slowly from view.", TRUE, templar, 0, 0, TO_ROOM);

        orig_room = templar->in_room;
        target = caster->in_room;

        /* Templar is moved to the new room, and has a look around */
        char_from_room(templar);
        char_to_room(templar, target);

        /* messages to destination room */
        act("$N's body fades slowly into view.", TRUE, caster, 0, templar, TO_ROOM);
        act("$N's body fades slowly into view.", TRUE, caster, 0, templar, TO_CHAR);

        /* Messages to templar */
        act("Your surroundings fade away, and you feel yourself being summoned.", TRUE, templar, 0,
            0, TO_CHAR);
        act("Your surroundings slowly fade back into view.", TRUE, templar, 0, 0, TO_CHAR);
        cmd_look(templar, "", 15, 0);

        templar_protected_damage(caster, templar, level);

        act("You feel a force drawing you back to your city-state.", TRUE, templar, 0, 0, TO_CHAR);
        act("Your surroundings fade away, and then return.", TRUE, templar, 0, 0, TO_CHAR);
        act("$n disappears in a flash of light.", TRUE, templar, 0, 0, TO_ROOM);

        char_from_room(templar);
        char_to_room(templar, orig_room);
        act("$n's body fades back into view.", TRUE, templar, 0, 0, TO_ROOM);
        break;
    case SPELL_HANDS_OF_WIND:
    case SPELL_BANISHMENT:
        act("You feel the sudden tug of wind on your body, " "which abruptly stops.", TRUE, templar,
            0, 0, TO_CHAR);
        act("A sudden wind tugs at $N, then abruptly stops.", TRUE, templar, 0, 0, TO_ROOM);

        templar_protected_damage(caster, templar, level);
        break;
    default:
        break;
    }
    return 1;
}


/* HANDS OF WIND: Pulls the target closer to the caster by a
 * distance dependent on the power level employed. Cannot be
 * cast on templars.  Whiran and sorcerer spell.
 * Whira nathro hekro.
 */
void
spell_hands_of_wind(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Hands_of_Wind */
    int i;
    int cmd = -1;
    char ch_command[MAX_INPUT_LENGTH];
    CHAR_DATA *tch;
    ROOM_DATA *dest_room = NULL;
    OBJ_DATA *tmpob = NULL;
    char buf[MAX_STRING_LENGTH];

    check_criminal_flag(ch, NULL);

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        act("You are unable to summon any winds here for this spell.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    act("You quickly pull your arms into your body in an exaggerated motion.", 
        FALSE, ch, 0, 0, TO_CHAR);
    act("$n quickly pulls $s arms into $s body in an exaggerated motion.", 
        FALSE, ch, 0, 0, TO_ROOM);

    /* If you are casting it in a SECT_SILT room, it backfires */
    /* by kicking up the silt into the air and flagging it     */
    /* SANDSTORM.  -San                                        */
    if ((ch->in_room->sector_type == SECT_SILT)
        && (!IS_SET(ch->in_room->room_flags, RFL_SANDSTORM))) {
        act("Silt dances up at the touch of your winds, obscuring your vision totally.", FALSE,
            ch, 0, 0, TO_CHAR);
        act("As winds surround $N, the silt on the ground dances "
            "up at their touch, obscuring your vision totally.", FALSE, ch, 0, 0, TO_ROOM);
        MUD_SET_BIT(ch->in_room->room_flags, RFL_SANDSTORM);
        return;
    }

//    gamelogf ("distance is %d", get_land_distance (ch->in_room, victim->in_room));

    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
        send_to_char("They resist your winds.\n\r", ch);
        return;
    }

    if (choose_exit_name_for(victim->in_room, ch->in_room, ch_command, level * 10, 0) == -1) {
        send_to_char("Your winds are unable to reach them.\n\r", ch);
        return;
    }

    if (templar_protected(ch, victim, SPELL_HANDS_OF_WIND, level)) {
        return;
    }

    if (allanak_magic_barrier(ch, victim->in_room)) {
      qgamelogf(QUIET_MISC,
                "allanak_magic_barrier: %s tried to cast hands of wind in #%d, blocked.",
                MSTR(ch, name),
                ch->in_room->number);
      act("Your winds are unable to reach them.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    
    if (tuluk_magic_barrier(ch, victim->in_room)) {
      qgamelogf(QUIET_MISC,
                "tuluk_magic_barrier: %s tried to cast hands of wind in #%d, blocked.",
                MSTR(ch, name),
                ch->in_room->number);
      act("Your winds are unable to reach them.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }

    /* If victim is affected by the Rooted ability, they are protected */
    if (find_ex_description("[ROOTED]", victim->ex_description, TRUE)) {
        act("Your winds are unable to reach them.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (victim->specials.riding) {
        act("You are violently yanked from $N's back by the winds!",
              FALSE, victim, 0, victim->specials.riding, TO_CHAR);
        act("$n is violently yanked from your back by the winds!",
              FALSE, victim, 0, victim->specials.riding, TO_VICT);
        act("$n is violently yanked from $N's back by the winds!",
              FALSE, victim, 0, victim->specials.riding, TO_NOTVICT);
        victim->specials.riding = 0;
        WIPE_LDESC(victim);
    }

    if (victim->specials.subduing) {
        for (tch = victim->in_room->people; tch; tch = tch->next_in_room)
            if (tch == victim->specials.subduing)
                break;
        if (tch) {
            act("You can't hold on to $N.", FALSE, victim, 0, tch, TO_CHAR);
            act("$n can't seem to hold on to you.", FALSE, victim, 0, tch, TO_VICT);
            act("$n can't seem to hold on to $N.", FALSE, victim, 0, tch, TO_NOTVICT);
            victim->specials.subduing = NULL;
            REMOVE_BIT(tch->specials.affected_by, CHAR_AFF_SUBDUED);
        }
    }

    for (tch = victim->in_room->people; tch; tch = tch->next_in_room)
        if ((tch != victim) && (victim == tch->master))
            stop_follower(tch);

    if ((choose_exit_name_for(victim->in_room, ch->in_room, ch_command, level * 10, 0) != -1)
        && (victim->in_room != ch->in_room))
        act("You sense $S aura drawing closer.", FALSE, ch, 0, victim, TO_CHAR);

    /* Cast while standing in an AIR room, spell is more powerful.  -san */
    if (ch->in_room->sector_type == SECT_AIR)
        level = level + 3;

    if (ch->in_room->sector_type == SECT_AIR_PLANE)
        level = level + 5;

    for (i = 0; i < level; i++) {
        if (!victim)
            return;
        if (choose_exit_name_for(victim->in_room, ch->in_room, ch_command, level * 10, 0) != -1) {

            one_argument(ch_command, buf, sizeof(buf));

            if (!strcmp(ch_command, "leave"))    cmd = 102;
            if (!strcmp(ch_command, "north"))    cmd = 0;
            if (!strcmp(ch_command, "east"))     cmd = 1;
            if (!strcmp(ch_command, "south"))    cmd = 2;
            if (!strcmp(ch_command, "west"))     cmd = 3;
            if (!strcmp(ch_command, "up"))       cmd = 4;
            if (!strcmp(ch_command, "down"))     cmd = 5;
            if (!strcmp(buf, "in"))              cmd = 6;

            if (cmd == -1) {
                gamelogf("hands of winds, couldn't find good exit value for %s", ch_command);
                return;
            }

            if ((cmd <= 5) && IS_SET(EXIT(victim, cmd)->exit_info, EX_CLOSED)
                    && !is_char_ethereal(victim)) {
                if (EXIT(victim, cmd)->keyword) {
                    sprintf(buf, "A violent wind slams $2$n$1 against the closed %s.",
                            EXIT(victim, cmd)->keyword);
                    act(buf, FALSE, victim, 0, 0, TO_ROOM);
                    sprintf(buf, "A violent wind slams $2you$1 against the closed %s.",
                            EXIT(victim, cmd)->keyword);
                    act(buf, FALSE, victim, 0, 0, TO_CHAR);
                    do_damage(victim, number(5,10), 10);

                    if (!IS_SET(EXIT(victim, cmd)->exit_info, EX_LOCKED)) {
                        if (number (1,100) <= get_char_size(victim)) {
                            sprintf(buf, "The %s flings wide open from the impact of $s weight!",
                                    EXIT(victim, cmd)->keyword);
                            act(buf, FALSE, victim, 0, 0, TO_ROOM);
                            sprintf(buf, "The %s flings wide open from the impact your weight!",
                                    EXIT(victim, cmd)->keyword);
                            act(buf, FALSE, victim, 0, 0, TO_CHAR);
                            REMOVE_BIT(EXIT(victim, cmd)->exit_info, EX_CLOSED);
                            do_damage(victim, number(5,10), 10);
                        } else
                            break;
                    } else
                        break;
                }
            }


            if ((cmd <= 5) && IS_SET(EXIT(victim, cmd)->exit_info, EX_SPL_SAND_WALL)
                    && !is_char_ethereal(victim)) {
                act("A violent wind slams $2$n$1 against the wall of sand.",
                        FALSE, victim, 0, 0, TO_ROOM);
                act("A violent wind slams $2you$1 against the wall of sand.",
                        FALSE, victim, 0, 0, TO_CHAR);
                do_damage(victim, number(5,10), 10);
                break;
            }


            if ((cmd <= 5) && IS_SET(EXIT(victim, cmd)->exit_info, EX_SPL_WIND_WALL)
                    && !is_char_ethereal(victim)) {
                act("A violent wind slams $2$n$1 against the wall of wind.",
                        FALSE, victim, 0, 0, TO_ROOM);
                act("A violent wind slams $2you$1 against the wall of wind.",
                        FALSE, victim, 0, 0, TO_CHAR);
                do_damage(victim, number(5,10), 10);
                break;
            }


            if (cmd == 102) {
                tmpob = find_wagon_for_room(victim->in_room);
                if ((tmpob) && (obj_index[tmpob->nr].vnum != 73001)) {
                    if (tmpob->in_room)
                        dest_room = tmpob->in_room;
                } else {
                    gamelogf ("Error in Hands of Wind - victim in object not in room");
                    act("Your winds are unable to reach them.", FALSE, ch, 0, 0, TO_CHAR);
                    return;
                }
            } else if (cmd == 6) {
                tmpob = find_wagon_for_room(ch->in_room);
                if ((tmpob) && (obj_index[tmpob->nr].vnum != 73001)){
                    if (tmpob->in_room)
                        dest_room = ch->in_room;
                } else {
                    gamelogf ("Error in Hands of Wind - ch in obj not in room, or obj is vortex");
                    act("Your winds are unable to reach them.", FALSE, ch, 0, 0, TO_CHAR);
                    return;
                }
            } else
                dest_room = victim->in_room->direction[cmd]->to_room;


            if (cmd == 102)
                sprintf(buf, "$2$n$1 is hurled out of %s by a violent wind!", OSTR(tmpob, short_descr));
            else if (cmd == 6)
                sprintf(buf, "$2$n$1 is hurled into %s by a violent wind!", OSTR(tmpob, short_descr));
            else
                sprintf(buf, "$2$n$1 is hurled out of the area by a violent wind to the %s!", dirs[cmd]);
            act(buf, FALSE, victim, 0, 0, TO_ROOM);
            if (GET_POS(victim) > POSITION_SLEEPING) {
                if (cmd == 102)
                    sprintf(buf, "$2You$1 are hurled out of %s by a violent wind!", OSTR(tmpob, short_descr));
                else if (cmd == 6)
                    sprintf(buf, "$2You$1 are hurled into %s by a violent wind!", OSTR(tmpob, short_descr));
                else
                    sprintf(buf, "$2You$1 are hurled out of the area by a violent wind to the %s!", dirs[cmd]);
                act(buf, FALSE, victim, 0, 0, TO_CHAR);
            }


            if (cmd <= 5) {
                if (!blade_barrier_check(victim, cmd))
                    return;
                if (!thorn_check(victim, cmd))
                    return;
                if (!wall_of_fire_check(victim, cmd))
                    return;
            }

            char_from_room(victim);
            char_to_room(victim, dest_room);

            if (cmd == 102)
                sprintf(buf, "$2$n$1 is dragged into the area by a violent wind from inside %s!",
                        OSTR(tmpob, short_descr));
            else if (cmd == 6)
                sprintf(buf, "$2$n$1 is dragged into the area by a violent wind from outside %s!",
                        OSTR(tmpob, short_descr));
            else
                sprintf(buf, "$2$n$1 is dragged into the area by a violent wind from the %s!",
                         rev_dirs[cmd]);
            act(buf, FALSE, victim, 0, 0, TO_ROOM);

            cmd_look(victim, "", 0, 0);

        }
        if (victim->in_room == ch->in_room)
            break;
    }

    WAIT_STATE(victim, level / 2);

}                               /* end Spell_Hands_of_Wind */


/* HAUNT: Allows the caster to summon a creature from the plane
 * of Drov, npc #476, which can be dispatched against a specific
 * target.  The npc has a dmpl, drov_haunt, on it that will allow
 * them to track their target down.  Note: this spell has not been
 * fully tested yet, and still needs to be added to the appropriate
 * spell tree when complete.  Requires a component.  Drovian spell.  
 * Drov dreth chran.
 */
void
spell_haunt(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)

/* First thing is to get the critter there, copied from the gate spell.  -San */
{                               /* begin Spell_Haunt */
    struct affected_type af;
    int vnum;

    memset(&af, 0, sizeof(af));

    if ((ch->in_room->sector_type == SECT_FIRE_PLANE)
        || ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))) {
        send_to_char("You are unable to reach Drov and summon a haunt here.\n\r", ch);
        return;
    }

    if (!get_componentB
        (ch, SPHERE_CONJURATION, level, "$p folds in upon itself with an audible grinding noise.",
         "$p folds in upon itself in $n's hands.",
         "You lack the correct component to lure otherwordly creatures here.")) {
        return;
    }

    check_criminal_flag(ch, NULL);

    switch (GET_GUILD(ch)) {
    case GUILD_SHADOW_ELEMENTALIST:
        vnum = 476;
        break;
    default:
        vnum = 476;
        break;
    }

    victim = read_mobile(vnum, VIRTUAL);

    if (!victim) {
        errorlogf("spell_haunt() in magick.c unable to load NPC# %d", vnum);
        return;
    }

    char_to_room(victim, ch->in_room);

    act("$n concentrates visibly as $N slowly forms from the shadows.", TRUE, ch, 0, victim,
        TO_ROOM);
    act("You focus your will as $N is brought to this plane.", TRUE, ch, 0, victim, TO_CHAR);

    add_follower(victim, ch);

/* Note: charm affect cannot be put on creature, else it */
/* will be unable to leave the caster's side.            */

    af.type = SPELL_GATE;
    af.duration = 5 + (2 * level);
    af.modifier = 0;
    af.location = 0;
    af.bitvector = CHAR_AFF_SUMMONED;
    affect_join(victim, &af, TRUE, FALSE);

}                               /* end Spell_Haunt */


/* HEAL: Your basic curative spell.  Vivaduan and sorcerer
 * spell.  Vivadu viqrol wril.
 */
void
spell_heal(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Heal */
    if (!victim) {
        gamelog("No victim in heal");
        return;
    }

    check_crim_cast(ch);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        act("You are unable to call upon Vivadu's aid for this spell here.", FALSE, ch, 0, 0,
            TO_CHAR);
        return;
    }

    if (ch->in_room->sector_type == SECT_FIRE_PLANE) {
        act("The fires in the area flare, weakening your spell.", FALSE, ch, 0, 0, TO_CHAR);
        act("The fires in the area flare, weakening the spell.", FALSE, ch, 0, 0, TO_ROOM);
        level = MAX(1, level - 1);
    }

    if (ch->in_room->sector_type == SECT_WATER_PLANE) {
        act("Your magick surges with power here, having a greater affect.", FALSE, ch, 0, 0,
            TO_CHAR);
        level = level + 1;
    }

    adjust_hit(victim, (level * (level * number(1, 3))));
    update_pos(victim);

    send_to_char("A warm feeling fills you, as wounds close all over your " "body.\n\r", victim);
    if (victim != ch)
        act("$N wounds heal as your magick does its work.", FALSE, ch, 0, victim, TO_CHAR);

}                               /* end Spell_Heal */


/* HEALTH DRAIN: Damages a player's hit points - generic damage spell 
 * Vivaduan and sorcerer spell.  Vivadu divan hurn.
 */
void
spell_health_drain(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Health_Drain */
    int dam;                    /* stores amount of damage applied to the target's hit points */

    if (!ch) {
        gamelog("No ch in health drain");
        return;
    }

    if (!victim) {
        gamelog("No victim in health drain");
        return;
    }

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        act("You are unable to call upon Vivadu's draining here.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    dam = dice(level + 2, 8);   /* determine how much damage is done */

    if (ch->in_room->sector_type == SECT_FIRE_PLANE) {
        act("The fires in the area flare, weakening your spell.", FALSE, ch, 0, 0, TO_CHAR);
        act("The fires in the area flare, weakening the spell.", FALSE, ch, 0, 0, TO_ROOM);
        dam = dam / 2;
    }

    if (ch->in_room->sector_type == SECT_WATER_PLANE) {
        act("Your magick surges here, increasing the effectiveness of your spell.", FALSE, ch, 0, 0,
            TO_CHAR);
        dam = dam * 2;
    }

    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5))))  /* check for saving throw */
        dam >>= 1;              /* if made, reduce damage to 1 */

    dam = MIN(dam, (GET_HIT(victim) - level));
    dam = MAX(dam, level);

    ws_damage(ch, victim, dam, 0, 0, 0, SPELL_HEALTH_DRAIN, TYPE_ELEM_WATER);     /* call the damage proc */
}                               /* end Spell_Health_Drain */


/* HERO SWORD: Cast on an object, it checks for edescs that are stored off when
 * someone dies holding a weapon.  The edesc, if found, is used to re-create the
 * dead hero and give them the weapon to wield.
 * drov dreth grol
 */
void
spell_hero_sword(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Hero_Sword */
  struct affected_type af;
  char chEdesc[MAX_STRING_LENGTH], 
    eqEdesc [MAX_STRING_LENGTH], 
    arg1[MAX_STRING_LENGTH], 
    arg2[MAX_STRING_LENGTH];
  
  CHAR_DATA *mob;
  OBJ_DATA *newObj;
  
  const char *eqEdescPtr;
  const char *chEdescPtr;
  int locationVal=0, itemNum=0, chNum;
  int duration = 5 + number(0,level);
  
  if (!obj)
    return;

  check_crim_cast(ch);
  
  if (!get_obj_extra_desc_value(obj, "[HERO_SWORD_EQ_INFO]",eqEdesc, MAX_STRING_LENGTH) ||
      !get_obj_extra_desc_value(obj, "[HERO_SWORD_CH_INFO]",chEdesc, MAX_STRING_LENGTH))
    {
      act("$p glows briefly, but nothing seems to happen.", FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
  
  eqEdescPtr = eqEdesc;
  chEdescPtr = chEdesc;
  
  chEdescPtr = one_argument(chEdescPtr, arg1, sizeof(arg1)); 

  if (!is_number(arg1)) {
    gamelog ("[HERO_SWORD_CH_INFO] does not contain a number.");
    return;
  }
  if (is_number(arg1)) {
    chNum = atoi(arg1);
    mob = read_mobile(chNum, VIRTUAL);
    if (!mob) {
      gamelogf("Hero Sword trying to create NPC#%d, which does not exist.", chNum);
      act("$p glows briefly, but nothing seems to happen.", FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    char_to_room(mob, ch->in_room);
    if (obj->in_room)
      obj_from_room(obj);
    if (obj->carried_by)
      obj_from_char(obj);
    if (obj->equipped_by)
      obj_from_char(obj);
    
  }
  
  if (get_obj_extra_desc_value(obj, "[HERO_SWORD_EQ_INFO]", eqEdesc, MAX_STRING_LENGTH)) {
    eqEdescPtr = two_arguments(eqEdescPtr, arg1, sizeof(arg1), arg2, sizeof(arg2));
    for (; !STRINGS_SAME(arg1, "xxx");) {
      if (is_number(arg1))
    	locationVal = atoi(arg1);
      if (is_number(arg2))
    	itemNum = atoi(arg2);
      if (locationVal && itemNum) {
    	newObj = read_object(itemNum, VIRTUAL);
	if (!newObj) 
	  gamelogf("Obj #%d not found in DB.", itemNum);
	else {
	  new_event(EVNT_REMOVE_OBJECT, duration*60, 0, 0, newObj, 0, 0);
	  equip_char(mob, newObj, locationVal);
	}
      }
      eqEdescPtr = two_arguments(eqEdescPtr, arg1, sizeof(arg1), arg2, sizeof(arg2));
    }
  }

  af.type = SPELL_HERO_SWORD;
  af.duration = duration;
  af.power = level;
  af.magick_type = magick_type;
  af.modifier = 0;
  af.location = CHAR_APPLY_NONE;
  af.bitvector = CHAR_AFF_SUMMONED;

  if (!IS_SET(mob->specials.act, CFL_SENTINEL))
    MUD_SET_BIT(mob->specials.act, CFL_SENTINEL);

  affect_join(mob, &af, TRUE, FALSE);

  act("$n swings $p through the air twice before dropping it to the ground.", 
      FALSE, ch, obj, 0, TO_ROOM);
  act("You swing $p through the air twice before dropping it on the ground.",
      FALSE, ch, obj, 0, TO_CHAR);

  act("$n's form gathers from the shadows and picks up $p.", 
      FALSE, mob, obj, 0, TO_ROOM);

  // Make 'em like the caster
  add_to_likes(mob, ch);
  add_guard(mob, ch);
}                               /* end Spell_Hero_Sword */


/* IDENTIFY: Allows the caster to identify fellow elementalists of
 * the same guild via the look command.  Each elementalist gets a 
 * version of this.  Nilaz threl inrof. Cumulative.
 */
void
spell_identify(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{
    struct affected_type af;
    struct affected_type *tmp_af;
    int duration;

    if (!victim) {              /* begin Spell_Identify */
        errorlog("spell_identify in magick.c was passed a null value in victim.");
        return;
    }

    memset(&af, 0, sizeof(af));

    check_crim_cast(ch);

    /* Only Nilazi can cast in SECT_SILT */

    if ((ch->in_room->sector_type == SECT_SILT) && (GET_GUILD(ch) != GUILD_VOID_ELEMENTALIST)) {
        send_to_char("Particles of swirling silt darken your vision.\n\r", ch);
        return;
    }

    /* Duration is affected by a combination of allied element and   */
    /* the sector.  This holds true for everyone except Krathi,      */
    /* because I could not think of a sector that would affect them. */

    if (((ch->in_room->sector_type == SECT_CAVERN) && (GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST))
        || ((IS_SET(ch->in_room->room_flags, RFL_DARK))
            && (GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST)))
        duration = level * 6;
    else if (((ch->in_room->sector_type == SECT_AIR) && (GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST))
             || (((ch->in_room->sector_type == SECT_NILAZ_PLANE)
                  && (GET_GUILD(ch) == GUILD_VOID_ELEMENTALIST))
                 || ((ch->in_room->sector_type == SECT_ROAD)
                     && (GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST))))
        duration = level * 7;
    else if (((IS_SET(ch->in_room->room_flags, RFL_PLANT_HEAVY))
              || (IS_SET(ch->in_room->room_flags, RFL_PLANT_SPARSE)))
             && (GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST))
        duration = level * 5;
    else
        duration = level * 4;

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_IDENTIFY;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_NOTHING;
/* af.bitvector = CHAR_AFF_IDENTIFY; */

    if (affected_by_spell(victim, SPELL_IDENTIFY)) {
        for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next) {
            if (tmp_af->type == SPELL_IDENTIFY) {
                duration = MIN((duration + tmp_af->duration), 48);
                affect_from_char(victim, SPELL_IDENTIFY);
                break;
            }
        }
        af.duration = duration;
    }

    if (victim != ch)
        act("You have heightened $N's awareness of the elements.", FALSE, ch, 0, victim, TO_CHAR);
    if (affected_by_spell(victim, SPELL_IDENTIFY))
        send_to_char("Your heightened awareness of the elements increases.\n\r", victim);
    else
        send_to_char("You feel a heightened awareness with the elements.\n\r", victim);
    affect_to_char(victim, &af);

}                               /* end Spell_Identify */


/* IMMOLATE: Catches the victim on fire for a duration dependent
 * on power level.  Will not harm fire elementals or anyone with
 * fire armor on them.  Suk-Krathi and sorcerer
 * spell.  Suk-Krath psiak hekro.
 */
void
spell_immolate(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Immolate */

    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in immolate");
        return;
    }

    if (!victim) {
        gamelog("No victim in immolate");
        return;
    }

    check_criminal_flag(ch, NULL);

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        act("Your magicks seem ineffectual in this place.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* Make caster visible */
    if (IS_AFFECTED(ch, CHAR_AFF_INVISIBLE))
        appear(ch);

    /* If victim is an NPC, make them hate the caster */
    if (IS_NPC(victim))
        if (!does_hate(victim, ch))
            add_to_hates_raw(victim, ch);

    /* Won't harm anyone with Fire Armor, Immolate, or fire elementals */
    if (affected_by_spell(victim, SPELL_IMMOLATE)
        || affected_by_spell(victim, SPELL_FIRE_ARMOR)
        || ((GET_RACE(victim) == RACE_ELEMENTAL) && (GET_GUILD(victim) == GUILD_FIRE_ELEMENTALIST))
        || (does_save(victim, SAVING_SPELL, ((level - 4) * -5) - 10))
        || IS_IMMORTAL(victim)) {
        act("You breathe orange flames at $N, but they die out harmlessly.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("$n breathes orange flames at you, but they die out harmlessly.", FALSE, ch, 0, victim,
            TO_VICT);
        act("$n breathes orange flames at $N, but they die out harmlessly.", FALSE, ch, 0, victim,
            TO_NOTVICT);
        return;
    }

    /* Do the invulnerability boogy */
    if (affected_by_spell(victim, SPELL_INVULNERABILITY)) {
        act("A cream-colored shell around $N flashes, blocking your spell.", FALSE, ch, 0, victim,
           TO_CHAR);
        act("The cream-colored shell around you flashes blocking a spell from $n.", FALSE, ch, 0,
            victim, TO_VICT);
        act("A cream-colored shell around $N flashes, blocking a spell from $n.", FALSE, ch, 0,
            victim, TO_NOTVICT);
        affect_from_char(victim, SPELL_INVULNERABILITY);
        send_to_char("The creamy shell around you fades away.\n\r", victim);
        act("The creamy shell around $n fades away.", FALSE, victim, 0, 0, TO_ROOM);
        if (IS_AFFECTED(victim, CHAR_AFF_INVULNERABILITY))
            REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_INVULNERABILITY);
        return;
    }

    act("You spew a bright orange fire from your mouth at $N, and $S body ignites!", FALSE, ch, 0,
        victim, TO_CHAR);
    act("$n spews a bright orange fire from $s mouth at you, and your body ignites!.", FALSE, ch, 0,
        victim, TO_VICT);
    act("$n spews a bright orange fire from $s mouth at $N, and $S body ignites!.", FALSE, ch, 0,
        victim, TO_NOTVICT);

    af.type = SPELL_IMMOLATE;
    af.duration = level;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_DAMAGE;

    affect_join(victim, &af, FALSE, FALSE);


}                               /* end Spell_Immolate */


/* ILLUMINANT: Allows the caster to create an amusing and sometimes alarming 
 * illusion.  Elkrosian version of pyrotechnics.  Cannot be cast indoors or in
 * caverns.  SECT_SILT diminishes the effects.  Mon and sul scares all mounts.
 * Elkros iluth nikiz.  
 */
void
spell_illuminant(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Illuminant */
    int effect = number(1, 6);
    char msg1[256], msg2[256];
    char fleemessage[MAX_STRING_LENGTH];
    CHAR_DATA *curr_ch, *next_ch;

    check_crim_cast(ch);

    /* Make it so you cannot cast while enclosed by walls. */
    if ((IS_SET(ch->in_room->room_flags, RFL_INDOORS))
        || (ch->in_room->sector_type == SECT_CAVERN)) {
        send_to_char("The walls around you hamper your spell.\n\r", ch);
        return;
    }

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        send_to_char("You are unable to reach Elkros for your spell here.\n\r", ch);
        return;
    }

    /* Effect diminished if cast in SECT_SILT */
    if (ch->in_room->sector_type == SECT_SILT)
        level = level - 2;

    if (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)
        level = MIN(7, level + 1);

    if (level == 7) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "Intense, blinding lightning crackles out in a sphere around $n, illuminating the area with a harsh radiance.");
            strcpy(msg2,
                   "Intense, blinding lightning crackles out in a sphere around you, illuminating the area with a harsh radiance.");
            break;
        case 2:
            strcpy(msg1,
                   "$n gestures, and an army of fat, luminescent glow-worms crawl from the ground, throwing off sparks as they inch along.");
            strcpy(msg2,
                   "You gesture, and an army of fat, luminescent glow-worms crawl from the ground, throwing off sparks as they inch along.");
            break;
        case 3:
            strcpy(msg1,
                   "$n raises $s arms, and a searing lightning bolt strikes from above, turning $m into a living torch before crackling away.");
            strcpy(msg2,
                   "You raise your arms, and a searing lightning bolt strikes from above, turning you into a living torch before crackling away.");
            break;
        case 4:
            strcpy(msg1,
                   "$n form diverges, splitting into several identical mirrors which blur in and out.");
            strcpy(msg2,
                   "Your form diverges, splitting into several identical mirrors which blur in and out.");
            break;
        case 5:
            strcpy(msg1, "Balls of electricity roll and tumble outward from $n's hands.");
            strcpy(msg2, "Balls of electricity roll and tumble outward from your hands.");
            break;
        case 6:
            strcpy(msg1,
                   "Streaks of lightning strike in a circle around $n, filling the air with their hiss.");
            strcpy(msg2,
                   "Streaks of lightning strike in a circle around you, filling the air with their hiss.");
            break;
        default:
            strcpy(msg1,
                   "Intense, blinding lightning crackles out in a sphere around $n, illuminating the area with a harsh radiance.");
            strcpy(msg2,
                   "Intense, blinding lightning crackles out in a sphere around you, illuminating the area with a harsh radiance.");
            break;
        }
    } else if (level == 6) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Lightning slashes up out of the ground behind $n, clawing at the sky.");
            strcpy(msg2, "Lightning slashes up out of the ground behind you, clawing at the sky.");
            break;
        case 2:
            strcpy(msg1, "Drifting dust in the air, unseen moments ago, begins to glow, each mote giving off a painful jolt as it brushes across exposed skin.");
            strcpy(msg2, "Drifting dust in the air, unseen moments ago, begins to glow, each mote giving off a painful jolt as it brushes across exposed skin.");
            break;
        case 3:
            strcpy(msg1, "$n holds $s fist out, then opens it palm up, releasing an expanding, skeletal falcon of blinding energy to arrow up into the sky.");
            strcpy(msg2, "You hold your fist out, then open it palm up, releasing an expanding, skeletal falcon of blinding energy to arrow up into the sky.");
            break;
        case 4:
            strcpy(msg1, "A circle of sizzling white lightnings forms around $n.");
            strcpy(msg2, "A circle of sizzling white lightnings forms around you.");
            break;
        case 5:
            strcpy(msg1, "A rainbow cascade of lightning, one color after another, streams outward from $n.");
            strcpy(msg2, "A rainbow cascade of lightning, one color after another, streams outward from you.");
            break;
        case 6:
            strcpy(msg1, "A column of white-hot lightning forms around $n, before arcing into the sky.");
            strcpy(msg2, "A column of white-hot lightning forms around you, before arcing into the sky.");
            break;
        default:
            strcpy(msg1, "Lightning slashes up out of the ground behind $n, clawing at the sky.");
            strcpy(msg2, "Lightning slashes up out of the ground behind you, clawing at the sky.");
            break;
        }
    } else if (level == 5) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "Flickering horns of lightning branch out from $n's head as $e grins fiendishly.");
            strcpy(msg2,
                   "Flickering horns of lightning branch out from your head as you grin fiendishly.");
            break;
        case 2:
            strcpy(msg1,
                   "Streams of multi-colored light burst out from a point before $n's hands, streaking outwards before whirling to form a spinning wheel.");
            strcpy(msg2,
                   "Streams of multi-colored light burst out from a point before $n's hands, streaking outwards before whirling to form a spinning wheel.");
            break;
        case 3:
            strcpy(msg1, "Flashes of light play over $n's form, blinding in their intensity.");
            strcpy(msg2, "Flashes of light play over your form, blinding in their intensity.");
            break;
        case 4:
            strcpy(msg1,
                   "Thunder rumbles as a searing flash of energy erupts behind $n, throwing $s form into sharp-edged shadow.");
            strcpy(msg2,
                   "Thunder rumbles as a searing flash of energy erupts behind you, throwing your form into sharp-edged shadow.");
            break;
        case 5:
            strcpy(msg1, "For a moment, $n's form is outlined in vivid flashes of electricity.");
            strcpy(msg2, "For a moment, your form is outlined in vivid flashes of electricity.");
            break;
        case 6:
            strcpy(msg1,
                   "Brightly dangerous lightnings play in dazzling white around $n's body for an instant.");
            strcpy(msg2,
                   "Brightly dangerous lightnings play in dazzling white around your body for an instant.");
            break;
        default:
            strcpy(msg1,
                   "Flickering horns of lightning branch out from $n's head as $e grins fiendishly.");
            strcpy(msg2,
                   "Flickering horns of lightning branch out from your head as you grin fiendishly.");
            break;
        }
    } else if (level == 4) {
        switch (effect) {
        case 1:
            strcpy(msg1, "A cloud of lightnings arcs and sizzles around $n.");
            strcpy(msg2, "A cloud of lightnings arcs and sizzles around you.");
            break;
        case 2:
            strcpy(msg1,
                   "Coils of shifting lightning loop around $n, crawling up and down $s body.");
            strcpy(msg2,
                   "Coils of shifting lightning loop around you, crawling up and down your body.");
            break;
        case 3:
            strcpy(msg1,
                   "A disc of spinning energy rises from $n's fingertips before exploding into a shower of incandescent sparks.");
            strcpy(msg2,
                   "A disc of spinning energy rises from your fingertips before exploding into a shower of incandescent sparks.");
            break;
        case 4:
            strcpy(msg1, "Sparks fly from $n's hands, setting the ground smouldering.");
            strcpy(msg2, "Sparks fly from your hands, setting the ground smouldering.");
            break;
        case 5:
            strcpy(msg1, "A vivid blue sheet of lightning cracks across the sky.");
            strcpy(msg2, "A vivid blue sheet of lightning cracks across the sky.");
            break;
        case 6:
            strcpy(msg1, "A whirlwind of sparks envelops $n, hiding $s face momentarily.");
            strcpy(msg2, "A whirlwind of sparks envelops you, hiding your face momentarily.");
            break;
        default:
            strcpy(msg1, "A cloud of lightnings arcs and sizzles around $n.");
            strcpy(msg2, "A cloud of lightnings arcs and sizzles around you.");
            break;
        }
    } else if (level == 3) {
        switch (effect) {
        case 1:
            strcpy(msg1, "A single bolt of lightning strikes beside $n.");
            strcpy(msg2, "A single bolt of lightning strikes beside $n.");
            break;
        case 2:
            strcpy(msg1, "Lightning arcs from $n's feet to $s hands, throwing off wild shadows.");
            strcpy(msg2, "Lightning arcs from your feet to your hands, throwing off wild shadows.");
            break;
        case 3:
            strcpy(msg1,
                   "The air sizzles and crackles as $n's eyes blaze brilliantly white for an instant.");
            strcpy(msg2,
                   "The air sizzles and crackles as your eyes blaze brilliantly white for an instant.");
            break;
        case 4:
            strcpy(msg1, "Short, jagged bolts of lightning arc around $n's hands.");
            strcpy(msg2, "Short, jagged bolts of lightning arc around your hands.");
            break;
        case 5:
            strcpy(msg1,
                   "A spray of lightning dances in the air, filling it with the sharp tang of ozone.");
            strcpy(msg2,
                   "A spray of lightning dances in the air, filling it with the sharp tang of ozone.");
            break;
        case 6:
            strcpy(msg1,
                   "An arc of lightning fills the sky, stretching from one horizon to the other.");
            strcpy(msg2,
                   "An arc of lightning fills the sky, stretching from one horizon to the other.");
            break;
        default:
            strcpy(msg1, "A single bolt of lightning strikes beside $n.");
            strcpy(msg2, "A single bolt of lightning strikes beside $n.");
            break;
        }
    } else if (level == 2) {
        switch (effect) {
        case 1:
            strcpy(msg1, "A tiny storm cloud coalesces from the air, lightnings crackling.");
            strcpy(msg2, "A tiny storm cloud coalesces from the air, lightnings crackling.");
            break;
        case 2:
            strcpy(msg1, "A scintillation of sparks ignites around $n.");
            strcpy(msg2, "A scintillation of sparks ignites around you.");
            break;
        case 3:
            strcpy(msg1, "Energy spits from $n's fingertips, giving off harsh pops and cracks.");
            strcpy(msg2, "Energy spits from your fingertips, giving off harsh pops and cracks.");
            break;
        case 4:
            strcpy(msg1,
                   "$n's hands glow softly, and then the light flows from $m into the air, forming a face.");
            strcpy(msg2,
                   "Your hands glow softly, and then the light flows from them into the air, forming a face.");
            break;
        case 5:
            strcpy(msg1,
                   "A low hum and sizzle of sparks emanates from $n as $e gestures mysteriously.");
            strcpy(msg2, "A low hum and sizzle of sparks emanates from you as you gesture.");
            break;
        case 6:
            strcpy(msg1,
                   "All the hair on $n's body suddenly stands straight out, the longer strands writhing eerily.");
            strcpy(msg2,
                   "All the hair on your body suddenly stands straight out, the longer strands writhing eerily.");
            break;
        default:
            strcpy(msg1, "A tiny storm cloud coalesces from the air, lightnings crackling.");
            strcpy(msg2, "A tiny storm cloud coalesces from the air, lightnings crackling.");
            break;
        }
    } else {
        switch (effect) {
        case 1:
            strcpy(msg1, "A circle of sparks dances around $n's head.");
            strcpy(msg2, "A circle of sparks dance around your head.");
            break;
        case 2:
            strcpy(msg1, "The air sizzles, filled with the smell of ozone.");
            strcpy(msg2, "The air sizzles, filled with the smell of ozone.");
            break;
        case 3:
            strcpy(msg1, "Sparkles of light dance around $n's fingers.");
            strcpy(msg2, "Sparkles of light dance around your fingers.");
            break;
        case 4:
            strcpy(msg1, "$n's eyes glow a brilliant silver for a few moments.");
            strcpy(msg2, "Your eyes glow a brilliant silver for a few moments.");
            break;
        case 5:
            strcpy(msg1, "Sparks play over $s skin in scintillating cascades.");
            strcpy(msg2, "Sparks play over your skin in scintillating cascades.");
            break;
        case 6:
            strcpy(msg1, "Bright sparks fly around $n's face, illuminating it.");
            strcpy(msg2, "Bright sparks fly around your face, illuminating it.");
            break;
        default:
            strcpy(msg1, "A circle of sparks dances around $n's head.");
            strcpy(msg2, "A circle of sparks dance around your head.");
            break;
        }
    }

    act(msg1, FALSE, ch, 0, 0, TO_ROOM);
    act(msg2, FALSE, ch, 0, 0, TO_CHAR);

    if (level >= 6) {
        for (curr_ch = ch->in_room->people; curr_ch; curr_ch = next_ch) {
            next_ch = curr_ch->next_in_room;
            if (IS_SET(curr_ch->specials.act, CFL_MOUNT)) {
                if (number(0, 1))
                    parse_command(curr_ch, fleemessage);
            }
        }
    }
}                               /* end Spell_Illuminant */


/* INFRAVISION: Sets affect on target that allows them to see
 * in the dark as well as through sandstorms, for a duration
 * dependent on power level.  Cumulative.  Drovian and
 * sorcerer spell.  Cannot be cast in SECT_SILT. Drov mutur wril.
 */
void
spell_infravision(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Infravision */
    struct affected_type af;
    struct affected_type *tmp_af;
    int duration;

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in infravision");
        return;
    }

    if (!victim) {
        gamelog("No victim in infravision");
        return;
    }

    check_crim_cast(ch);

    /* Cannot be cast in SECT_SILT */
    if (ch->in_room->sector_type == SECT_SILT) {
        send_to_char("Particles of swirling silt darken your vision.\n\r", ch);
        return;
    }

    /* Blinded if cast in sector FIRE_PLANE */
    if (ch->in_room->sector_type == SECT_FIRE_PLANE) {
        send_to_char("Your are blinded by all the surrouding flames and heat.\n\r", victim);
        act("$n staggers in shock, blinded by the surrounding flames and heat.", TRUE, victim, 0, 0,
            TO_ROOM);
        duration = level;
        af.type = SPELL_BLIND;
        af.location = CHAR_APPLY_OFFENSE;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = -(level * 5);
        af.duration = 2;
        af.bitvector = CHAR_AFF_BLIND;

        if (affected_by_spell(victim, SPELL_BLIND)) {
            for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next) {
                if (tmp_af->type == SPELL_BLIND) {
                    duration = MIN((duration + tmp_af->duration), 24);
                    affect_from_char(victim, SPELL_BLIND);
                    break;
                }
            }
            af.duration = duration;
        }

        affect_to_char(victim, &af);

        return;
    }

    if (ch == victim)
        send_to_char("Your eyes glow red for a moment.\n\r", ch);
    else
        act("Your eyes glow red for a moment.", FALSE, ch, 0, victim, TO_VICT);
    act("$n's eyes take on a red hue.", TRUE, victim, 0, 0, TO_ROOM);

/*    if (ch != victim) {
        send_to_char("Your eyes glow red for a moment.\n\r", victim);
        act("$n's eyes take on a red hue.", FALSE, victim, 0, 0, TO_ROOM);
    } else {
        send_to_char("Your eyes glow red.\n\r", ch);
        act("$n's eyes glow red.", FALSE, ch, 0, 0, TO_ROOM);
    } */

    duration = 2 * level;

    af.type = SPELL_INFRAVISION;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_INFRAVISION;

    if (affected_by_spell(victim, SPELL_INFRAVISION)) {
        for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next) {
            if (tmp_af->type == SPELL_INFRAVISION) {
                duration = MIN((duration + tmp_af->duration), 24);
                affect_from_char(victim, SPELL_INFRAVISION);
                break;
            }
        }
        af.duration = duration;
    }

    affect_to_char(victim, &af);
}                               /* end Spell_Infravision */


/* INSOMNIA: Recipient of spell is unaffected by the sleep spell,
 * which is removed if they already have that affect on them and
 * will not be able to sleep of their own accord.  Cumulative.  
 * Elkros spell.  Elkros psiak echri.
 */
void
spell_insomnia(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Insomnia */
    struct affected_type af;
    int duration;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in insomnia");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    duration = ((level * 5) - get_char_size(victim));

    if (affected_by_spell(victim, SPELL_SLEEP)) {
        act("Your spell disappears around $N with a loud popping noise.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("$n makes a gesture are your head and you don't feel so tired anymore.", FALSE, ch, 0,
            victim, TO_VICT);
        act("$n gestures at $N's head and an aura around $M suddenly disperses with a loud pop.",
            FALSE, ch, 0, victim, TO_NOTVICT);
        affect_from_char(victim, SPELL_SLEEP);
        return;
    }

    if (duration < 0) {
        act("Your spell dissipates around $N uselessly.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n gestures at your head and you feel a small amount of energy that passes quickly.",
            FALSE, ch, 0, victim, TO_VICT);
        act("$n gstures at $N's head and energy crackles around $M, then dissipates quickly.",
            FALSE, ch, 0, victim, TO_NOTVICT);
        return;
    }

    if (affected_by_spell(victim, SPELL_INSOMNIA)) {
        if (victim == ch) {
            act("You begin to twitch with energy.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n begins to twitch with energy.", TRUE, ch, 0, 0, TO_ROOM);
        } else {
            act("You gesture at $N's head, sending an aura around $M.", FALSE, ch, 0, victim,
                TO_CHAR);
            act("$n gestures are your head, and your energy level increases.", FALSE, ch, 0, victim,
                TO_VICT);
            act("$n gestures at $N's head, and $E twitches with energy.", TRUE, ch, 0, victim,
                TO_NOTVICT);
        }
        duration = (duration + number(1, (level + 1)));
    } else {
        if (victim == ch) {
            act("You twitch with even more energy.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n twitches with more energy.", TRUE, ch, 0, 0, TO_ROOM);
        } else {
            act("You gesture at $N's head, sending an aura around $M.", FALSE, ch, 0, victim,
                TO_CHAR);
            act("$n gestures are your head, and your energy level increases even more.", FALSE, ch,
                0, victim, TO_VICT);
            act("$n gestures at $N's head, and $E twitches with more energy.", TRUE, ch, 0, victim,
                TO_NOTVICT);
        }
    }

    af.type = SPELL_INSOMNIA;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = 0;

    affect_join(victim, &af, TRUE, FALSE);
}                               /* end Spell_Insomnia */

int get_normal_condition(CHAR_DATA * ch, int cond);


/* INTOXICATION: Raises the target's drunkeness level, to the
 * point where they can pass out.  Cannot be cast on the undead 
 * or in the desert.  Vivaduan spell.  Vivadu mutur hurn.
 */
void
spell_intoxication(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * tar_ch,
                   OBJ_DATA * obj)
{                               /* begin Spell_Intoxication */
    check_criminal_flag(ch, NULL);

    if (!tar_ch) {
        gamelog("Warning! Null tar_ch passed to spell_intoxication! Exiting.");
        return;
    }

    if ((GET_RACE(tar_ch) == RACE_VAMPIRE) || (IS_SET(tar_ch->specials.act, CFL_UNDEAD))) {
        act("$N seems to be unaffected.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tried some sort of spell on you, how amusing.", FALSE, ch, 0, tar_ch, TO_VICT);
        return;
    }

    if (ch->in_room->sector_type == SECT_DESERT) {
        send_to_char("It is too dry here, the air swallows up your spell.\n\r", ch);
        return;
    }

    if (affected_by_spell(tar_ch, SPELL_TURN_ELEMENT)) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    if (tar_ch != ch) {
        if (!does_save(tar_ch, SAVING_SPELL, ((level - 4) * (-5)))) {
            act("You point your finger at $N, and a mist briefly surrounds them.", FALSE, ch, 0,
                tar_ch, TO_CHAR);
            act("A fine mist surrounds you as $n points $s finger at you.", FALSE, ch, 0, tar_ch,
                TO_VICT);
            act("$n points $s finger at $N, as a fine mist surrounds them.", TRUE, ch, 0, tar_ch,
                TO_NOTVICT);
        } else {
            act("You point your finger at $N, but nothing happens.", FALSE, ch, 0, tar_ch, TO_CHAR);
            act("$n points $s finger at you, but nothing happens.", FALSE, ch, 0, tar_ch, TO_VICT);
            act("$n points $s finger at $N, but nothing happens.", TRUE, ch, 0, tar_ch, TO_NOTVICT);
            return;
        }
    } else {
        act("A fine mist surrounds you briefly.", FALSE, ch, 0, tar_ch, TO_VICT);
        act("A fine mist surrounds $N briefly.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
    }
    send_to_char("The taste of wine fills your mouth, and you feel an intoxicating rush.\n\r",
                 tar_ch);

    make_char_drunk(tar_ch, level * number(4, 9));
}                               /* end Spell_Intoxication */


/* INVISIBILITY: Makes someone or something invisible.  Success
 * is affected by the size of the target.  Duration depends on
 * power level.  Cumulative.  Whiran and sorcerer spell.
 * Whira iluth grol.
 */
void
spell_invisibility(byte level, sh_int magick_type,      /* power level cast at  */
                   CHAR_DATA * ch,      /* the caster           */
                   CHAR_DATA * victim,  /* the target character */
                   OBJ_DATA * obj)
{                               /* the target object    *//* begin Spell_Invisibility */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    check_crim_cast(ch);

    if (!((ch && obj) || victim))
        return;

    assert((ch && obj) || victim);

    if (obj) {
        if (obj->obj_flags.type == ITEM_WAGON)
            act("$p is too large for your magick.", FALSE, ch, obj, 0, TO_CHAR);
        else if (!IS_SET(obj->obj_flags.extra_flags, OFL_INVISIBLE)) {
            act("You wave a hand over $p and it turns invisible.", FALSE, ch, obj, 0, TO_CHAR);
            act("$n waves a hand over $p and it suddenly disappears.", TRUE, ch, obj, 0, TO_ROOM);
            MUD_SET_BIT(obj->obj_flags.extra_flags, OFL_INVISIBLE);
        } else
            act("$p is already invisible.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
        duration = ((level * 5) - get_char_size(victim));
        if (duration < 0) {
            act("Your magick was not powerful enough to conceal $N.", FALSE, ch, 0, victim,
                TO_CHAR);
            act("$n waves a hand towards you and you briefly fade from view, then return to normal.", FALSE, ch, 0, victim, TO_VICT);
            act("$n waves a hand at $N who briefly fades from view, then returns to normal.", FALSE,
                ch, 0, victim, TO_NOTVICT);
            return;
        }
        if (!affected_by_spell(victim, SPELL_INVISIBLE)) {
            if (victim == ch) {
                act("You vanish.", FALSE, ch, 0, 0, TO_CHAR);
                act("$n slowly fades from view.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                act("You wave a hand over $N, and $E slowly fades from view.", FALSE, ch, 0, victim,
                    TO_CHAR);
                act("$n waves a hand over you, and you slowly fade from view.", FALSE, ch, 0,
                    victim, TO_VICT);
                act("$n waves a hand over $N, and $E slowly fades from view.", TRUE, ch, 0, victim,
                    TO_NOTVICT);
            }
        } else {
            if (victim == ch) {
                act("Your concealment becomes more potent.", FALSE, ch, 0, 0, TO_CHAR);
                act("$n's concealment becomes more potent.", TRUE, ch, 0, 0, TO_ROOM);
            } else {
                act("You wave a hand over $N and $S concealment becomes more potent.", FALSE, ch, 0,
                    victim, TO_CHAR);
                act("$n waves a hand over you and your concealment becomes more potent.", FALSE, ch,
                    0, victim, TO_VICT);
                act("$n waves a hand over $N and $S concealment becomes more potent.", TRUE, ch, 0,
                    victim, TO_NOTVICT);
            }
        }

        if (((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))
            || (ch->in_room->sector_type == SECT_EARTH_PLANE)) {
            send_to_char
                ("You have trouble reaching Whira here, and your spell is weakened somewhat.\n\r",
                 ch);
            duration = duration / 2;
        }

        af.type = SPELL_INVISIBLE;
        af.duration = duration;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = 0;
        af.location = CHAR_APPLY_NONE;
        af.bitvector = CHAR_AFF_INVISIBLE;
        affect_join(victim, &af, FALSE, FALSE);

        /* handle removing watching */
        watch_vis_check(victim);
    }
}                               /* end Spell_Invisibility */


/* INVULNERABILITY: Puts a protective shield around the caster, for
 * a duration dependent on power level, which deflects the next attack
 * made on the caster.  The shell is dispersed at the moment of the
 * attack.  Non-cumulative.  Vivaduan and sorcerer spell. 
 * Vivadu pret grol.
 */
void
spell_invulnerability(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                      OBJ_DATA * obj)
{                               /* begin Spell_Invulnerability */
    struct affected_type af;
    int duration;

    memset(&af, 0, sizeof(af));

    check_crim_cast(ch);

    if (IS_AFFECTED(victim, CHAR_AFF_INVULNERABILITY)) {
        act("A shell of invulnerability already exists.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    duration = level * 2;

    if (ch->in_room->sector_type == SECT_WATER_PLANE) {
        send_to_char("The magick surges powerfully here, causing greater affect.\n\r", victim);
        duration = duration * 2;
    }

    af.type = SPELL_INVULNERABILITY;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_NONE;
    af.modifier = 0;
    af.bitvector = CHAR_AFF_INVULNERABILITY;

    affect_to_char(victim, &af);

    if (victim == ch) {
        act("A bright, creamy-colored sphere appears and encloses you.", FALSE, ch, 0, 0, TO_CHAR);
        act("A bright, creamy-colored sphere appears and encloses $n.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("You send a bright, creamy-colored sphere that encloses $N .", FALSE, ch, 0, victim,
            TO_CHAR);
        act("$n sends a bright, creamy-colored sphere that encloses you.", FALSE, ch, 0, victim,
            TO_VICT);
        act("$n sends a bright, creamy-colored sphere that encloses $N.", TRUE, ch, 0, victim,
            TO_NOTVICT);
    }
}                               /* end Spell_Invulnerability */


/* LEVITATE: Reduces movement costs and falling damage if cast on
 * a person.  If cast on an object, reduces its weight to 1 (cannot
 * be cast on WAGON objects.  Whiran and sorcerer spell.
 * Whira mutur wril.
 */
void
spell_levitate(byte level, sh_int magick_type,  /* power level cast at  */
               CHAR_DATA * ch,  /* the caster           */
               CHAR_DATA * victim,      /* the target character */
               OBJ_DATA * obj)
{                               /* the target object    *//* begin Spell_Levitate */
    /* Note from Sanvean: tinkered this 4/30/00 so you can cast on objects */
    struct affected_type af;
    int duration;
    static bool lev_nesting = 0;
    OBJ_DATA *innerobj;

    memset(&af, 0, sizeof(af));

    check_crim_cast(ch);

    if (!ch) {
        gamelog("No ch in levitate");
        return;
    }

    if (!victim && !obj) {
        gamelog("No victim/obj in levitate");
        return;
    }

    if (obj) {
        if (obj->obj_flags.type == ITEM_WAGON)
            act("$p is too large for your magick.", FALSE, ch, obj, 0, TO_CHAR);
        else {
            if (!obj->nr) { // corpses?
                if (lev_nesting == 0) {
                    act("You wave a hand over $p and it begins to float gently above the ground.", FALSE,
                        ch, obj, 0, TO_CHAR);
                    act("$n waves a hand over $p and it begins to float gently above the ground.", TRUE, ch,
                        obj, 0, TO_ROOM);
                }
                obj->obj_flags.weight = 100;
            } else if (obj->nr && obj->obj_flags.weight > obj_default[obj->nr].weight / level) {
                if (lev_nesting == 0) {
                    act("You wave a hand over $p and it begins to float gently above the ground.", FALSE,
                        ch, obj, 0, TO_CHAR);
                    act("$n waves a hand over $p and it begins to float gently above the ground.", TRUE, ch,
                        obj, 0, TO_ROOM);
                }

                obj->obj_flags.weight = obj_default[obj->nr].weight / level;

                if (obj->obj_flags.weight < 1)
                    obj->obj_flags.weight = 1;
            }
            MUD_SET_BIT(obj->obj_flags.extra_flags, OFL_NO_FALL);

            if (level >= 5 && IS_CONTAINER(obj)) {
                for (innerobj = obj->contains; innerobj; innerobj = innerobj->next_content) {
                    lev_nesting++;
                    spell_levitate(level, magick_type, ch, victim, innerobj);
                    lev_nesting--;
                }
            }
        }
    } else if (victim) {
        duration = ((level * 10) - (get_char_size(victim)));
        if (duration < 0) {
            act("You rise from the ground momentarily, then float back down as $n gestures at you.",
                FALSE, ch, 0, victim, TO_VICT);
            act("$N rises from the ground momentarily, then floats back down as $n gestures at $M.",
                FALSE, ch, 0, victim, TO_NOTVICT);
            act("$N rises from the ground momentarily, then floats back down as you gesture at $M.",
                FALSE, ch, 0, victim, TO_CHAR);
            return;
        }

        /*  You can't cast fly on things that can fly naturally, else they'll */
        /*  lose the fly ability at the end of the duration. */
        if ((IS_AFFECTED(victim, CHAR_AFF_FLYING)) && (!affected_by_spell(victim, SPELL_LEVITATE))
            && (!affected_by_spell(victim, SPELL_FEATHER_FALL))) {
            send_to_char("They are already flying.\n\r", ch);
            return;
        }
        if (affected_by_spell(victim, SPELL_GODSPEED)) {
            act("You rise from the ground momentarily, then float back down as $n gestures at you.",
                FALSE, ch, 0, victim, TO_VICT);
            act("$N rises from the ground momentarily, then floats back down as $n gestures at $M.",
                FALSE, ch, 0, victim, TO_NOTVICT);
            act("$N rises from the ground momentarily, then floats back down as you gesture at $M.",
                FALSE, ch, 0, victim, TO_CHAR);
            return;
        }

        if (affected_by_spell(victim, SPELL_FLY)) {
            send_to_char("They are already flying.\n\r", ch);
            return;
        }

        if (!affected_by_spell(victim, SPELL_LEVITATE)) {
            if (victim == ch) {
                act("Your feet rise a little off the ground.", FALSE, ch, 0, 0, TO_CHAR);
                act("$n's feet rise a little off the ground.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                act("You send a brisk wind at $N causing $S feet to rise a little off the ground.",
                    FALSE, ch, 0, victim, TO_CHAR);
                act("$n sends a brisk wind at you, causing your feet to rise a little off the ground.", FALSE, ch, 0, victim, TO_VICT);
                act("$n sends a brisk wind at $N causing $S feet to rise a little off the ground.",
                    TRUE, ch, 0, victim, TO_NOTVICT);
            }
        } else {
            if (victim == ch) {
                act("Your feet rise higher off the ground.", FALSE, ch, 0, 0, TO_CHAR);
                act("$n's feet rise higher off the ground.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                act("You send a brisk wind at $N causing $S feet to rise higher off the ground.",
                    FALSE, ch, 0, victim, TO_CHAR);
                act("$n sends a brisk wind at you, causing your feet to rise higher off the ground.", FALSE, ch, 0, victim, TO_VICT);
                act("$n sends a brisk wind at $N causing $S feet to rise higher off the ground.",
                    TRUE, ch, 0, victim, TO_NOTVICT);
            }
        }

        if (affected_by_spell(victim, SPELL_FEATHER_FALL)) {
            affect_from_char(victim, SPELL_FEATHER_FALL);
            REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_FLYING);
        }

        if (GET_SPEED(victim) == SPEED_RUNNING)
            GET_SPEED(victim) = SPEED_WALKING;

        af.type = SPELL_LEVITATE;
        af.duration = duration;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = 0;
        af.location = 0;
        af.bitvector = CHAR_AFF_FLYING;
        affect_join(victim, &af, FALSE, FALSE);
    }
}                               /* end Spell_Levitate */


/* LIGHTNING BOLT: Flings a bolt of lightning at the target.
 * Damage dependent on power level.  Echoes to nearby rooms.
 * Elkros and sorcerer spell.  Elkros divan hekro.  
 */
void
spell_lightning_bolt(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                     OBJ_DATA * obj)
{                               /* begin Spell_Lightning_Bolt */
    int dam;

    if (!ch) {
        gamelog("No ch in lightning bolt");
        return;
    }

    if (!victim) {
        gamelog("No victim in lightning bolt");
        return;
    }

    if (ch->in_room->sector_type == SECT_SILT) {
        dam = number(1, 6) * level;
        send_to_char("Your spell seems weakend by the silt.\n\r", ch);
    } else if (ch->in_room->sector_type == SECT_LIGHTNING_PLANE) {
        dam = number(7, 12) * (level + 1);
        send_to_char("Your magicks surges powerfully and your spell has greater affect.\n\r", ch);
    } else
        dam = number(7, 12) * level;

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        dam = dam / 2;
        send_to_char("Your spell seems weakend by the Void.\n\r", ch);
    }

    /* energy shield defends against this  */
    if (affected_by_spell(victim, SPELL_ENERGY_SHIELD)) {
        act("A bolt of lightning jolts from your fingers, striking $N with a sizzle.", FALSE, ch, 0,
            victim, TO_CHAR);
        act("A bolt of lightning jolts from $n's fingers, striking you with a sizzle.", FALSE, ch,
            0, victim, TO_VICT);
        act("A bolt of lightning jolts from $n's fingers, striking $N with a sizzle.", FALSE, ch, 0,
            victim, TO_NOTVICT);
        act("With a flash, the sparks of light surrounding $N absorb the bolt.", FALSE, ch, 0,
            victim, TO_CHAR);
        act("With a flash, the sparks of light surrounding you absorb the bolt.", FALSE, ch, 0,
            victim, TO_VICT);
        act("With a flash, the sparks of light surrounding $N absorb the bolt.", FALSE, ch, 0,
            victim, TO_NOTVICT);
        return;
    }

    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5))))
        dam >>= 1;

    act("The crackling hiss of striking lightning comes from the $D.", FALSE, ch, 0, 0,
        TO_NEARBY_ROOMS);

    ws_damage(ch, victim, dam, 0, 0, dam, SPELL_LIGHTNING_BOLT, TYPE_ELEM_LIGHTNING);
}                               /* end Spell_Lightning_Bolt */


/* LIGHTNING_SPEAR:  Creates a magickal spear of elkros, object 488, in the target's
 * primary hand.  Sets the object as NOSAVE, and adds an edesc with the powerlevel
 * to the object (edesc is used by a javascript attached to the object).  Lightning
 * Elementalist and Sorcerer spell.  Elkros wilith hekro
 */

void
spell_lightning_spear(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                      OBJ_DATA * obj)
{                               /* begin Spell_Lightning_Spear */
    int spear_num = 488;
    OBJ_DATA *tmp_obj;
    char ename[MAX_STRING_LENGTH], edesc[MAX_STRING_LENGTH];

    if (!ch) {
        gamelog("No ch in lightning spear");
        return;
    }

    if (!get_componentB
        (ch, SPHERE_CREATION, level - 2, "$p takes on a charge and dissolves into energy.", "",
         "You must have the proper component to create a spear of lightning."))
        return;

    check_crim_cast(ch);

    tmp_obj = read_object(spear_num, VIRTUAL);

    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);

    strcpy(ename, "[LIGHTNINGSPEAR_POWERLEVEL]");
    sprintf(edesc, "%d", level);
    set_obj_extra_desc_value(tmp_obj, ename, edesc);

    if (victim && (victim != ch)) {
        if (!(victim->equipment[EP])) {
            act("A crackling light flashes between $n's hands, revealing $p, which $e tosses to $N.", TRUE, ch, tmp_obj, victim, TO_NOTVICT);
            act("A crackling light flashes between your hands, revealing $p, which you toss to $N.",
                TRUE, ch, tmp_obj, victim, TO_CHAR);
            act("A crackling light flashes beween $n's  hands, revealing $p, which $e tosses to you.", TRUE, ch, tmp_obj, victim, TO_VICT);
            equip_char(victim, tmp_obj, EP);
        } else {
            act("$n tries to catch it but $s hand already holds $p.", TRUE, victim,
                victim->equipment[EP], 0, TO_NOTVICT);
            act("You try to catch it but you're are already holding $p.", TRUE, victim,
                victim->equipment[EP], 0, TO_CHAR);
            extract_obj(tmp_obj);
            return;
        }
    } else {
        // This could happen if they're a mantis
        if (ch->equipment[EP]) {
            act("A crackling light flashes between your hands, but $p blocks the magick.", TRUE, ch,
                ch->equipment[EP], 0, TO_CHAR);
            return;
        }
        act("A crackling light flashes between $n's hands, revealing $p.", TRUE, ch, tmp_obj, 0,
            TO_ROOM);
        act("A crackling light flashes between your hands, revealing $p.", TRUE, ch, tmp_obj, 0,
            TO_CHAR);
        equip_char(ch, tmp_obj, EP);
    }

    //  obj_to_char(tmp_obj, ch);

    new_event(EVNT_SANDJAMBIYA_REMOVE, (24 + (number(1, 6))) * 90 * level, 0, 0, tmp_obj, 0, 0);

    return;
}                               /* end Spell_Lightning_Spear */


/* LIGHTNING STORM: Conjures a lightning storm, object #950
 * which moves around randomly, doing damage to characters 
 * in the room. Echoes to nearby rooms.
 * Cannot be cast in rooms flagged INDOORS, or sectored
 * INSIDE or CAVERNS.  Note: it would be cool to make this
 * check for weather conditions, and its effects lessened or
 * augmented accordingly.  Elkros spell.  Elkros nathro hurn.
 */
void
spell_lightning_storm(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                      OBJ_DATA * obj)
{                               /* begin Spell_Lightning_Storm */
    //    int duration;
    OBJ_DATA *temp, *next_obj;
    OBJ_DATA *storm;

    check_criminal_flag(ch, NULL);

    if (((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))
        || (ch->in_room->sector_type == SECT_WATER_PLANE)) {
        send_to_char("You are unable to reach Elkros to summon a storm here.\n\r", ch);
        return;
    }

    storm = read_object(950, VIRTUAL);
    if (!storm) {
        send_to_char("Nothing happened.\n\r", ch);
        gamelog("No storm object #950, for spell lightning storm.");
        return;
    }

    if (IS_SET(ch->in_room->room_flags, RFL_INDOORS)
        || ((ch->in_room->sector_type == SECT_INSIDE)
            || (ch->in_room->sector_type == SECT_CAVERN))) {
        send_to_char("You are too far from the elements to summon a storm.\n\r", ch);
        extract_obj(storm);
        return;
    }

    if (!get_componentB
        (ch, SPHERE_NATURE, level, "$p explodes in a shower of sparks in your hands.",
         "$p explodes in a shower of sparks in $n's hands.",
         "You do not have a powerful enough component to do that."))
        return;

    /* Search room to see if a storm object is present */
    for (temp = ch->in_room->contents; temp; temp = next_obj) {
        next_obj = temp->next;

        if (obj_index[temp->nr].vnum == 950) {
            send_to_char("Nothing happened.\n\r", ch);
            extract_obj(storm);
            return;
        }
    }

    storm->obj_flags.value[0] = level;
    storm->obj_flags.value[1] = level * 4;

    obj_to_room(storm, ch->in_room);
    act("You summon $p from the plane of Elkros.", FALSE, ch, storm, 0, TO_CHAR);
    act("$n conjures $p with a crackling of energy.", FALSE, ch, storm, 0, TO_ROOM);
    act("The crackling of energy and flashes of light come from the $D.", FALSE, ch, 0, 0,
        TO_NEARBY_ROOMS);

    // Handled now by JS halaster wrote -nessalin 11/27/2004
    //    duration = (level * 2 * level * 60);
    //    new_event(EVNT_REMOVE_OBJECT, duration, 0, 0, storm, 0, 0);
}                               /* end Spell_Lightning_Storm */


/* LIGHTNING WHIP: Yet another of the many spells based on SAND
 * JAMBIYA, this one creating a weapon for Elkrosians.  At levels
 * wek-sul, it creates the same whip item, but at level mon with
 * the appropriate component, it creates a permanent staff item,
 * object #1028.  Elkros spell.  Elkros wilith viod.
 */
void
spell_lightning_whip(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                     OBJ_DATA * obj)
{                               /* begin Spell_Lightning_Whip */

    int jambiya_num = 0;
    OBJ_DATA *tmp_obj;

    if (!ch) {
        gamelog("No ch in lightning whip");
        return;
    }

    check_crim_cast(ch);

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        send_to_char("You are unable to reach Elkros to create a whip here.\n\r", ch);
        return;
    }

    switch (level) {
    case 1:
        jambiya_num = 468;
        break;
    case 2:
        jambiya_num = 469;
        break;
    case 3:
        jambiya_num = 470;
        break;
    case 4:
        jambiya_num = 471;
        break;
    case 5:
        jambiya_num = 472;
        break;
    case 6:
        jambiya_num = 473;
        break;
    case 7:
        if (!get_componentB
            (ch, SPHERE_CREATION, level, "$p explodes in a shower of sparks in your hands.",
             "$p explodes in a shower of sparks in $n's hands.",
             "You do not have a powerful enough component to do that."))
            return;

        jambiya_num = 1476 + number(0, 7);      /* random object 1476 to 1483 */
        tmp_obj = read_object(jambiya_num, VIRTUAL);

	set_creator_edesc(ch, tmp_obj);

        obj_to_char(tmp_obj, ch);

        act("Brilliant blue sparks dance around $n's hands, forming into $p.", TRUE, ch, tmp_obj, 0,
            TO_ROOM);
        act("Brilliant blue sparks dance around your hands, forming into $p.", TRUE, ch, tmp_obj, 0,
            TO_CHAR);

        return;
    default:
        gamelog("UNKNOWN POWER LEVEL IN SPELL_LIGHTNING_WHIP()");
        break;
    };

    tmp_obj = read_object(jambiya_num, VIRTUAL);

    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);

    if (victim && (victim != ch)) {
        act("Brilliant blue sparks dance around $n's hands, forming into $p, which $s drops at $N's feet.", TRUE, ch, tmp_obj, victim, TO_NOTVICT);
        act("Brilliant blue sparks dance around your hands, forming into $p, which you drop at $N's feet.", TRUE, ch, tmp_obj, victim, TO_CHAR);
        act("Brilliant blue sparks dance around $n's hands, forming into $p, which $s drops at your feet.", TRUE, ch, tmp_obj, victim, TO_VICT);
        obj_to_room(tmp_obj, ch->in_room);
    } else {
        act("Brilliant blue sparks dance around $n's hands, forming into $p.", TRUE, ch, tmp_obj, 0,
            TO_ROOM);
        act("Brilliant blue sparks dance around your hands, forming into $p.", TRUE, ch, tmp_obj, 0,
            TO_CHAR);
        obj_to_char(tmp_obj, ch);
    }
    new_event(EVNT_LIGHTNING_REMOVE, (((level + 1) * 3000) / 2), 0, 0, tmp_obj, 0, 0);

    return;

}                               /* end Spell_Lightning_Whip */



/* Helper function for mark spell 
 *
 * Decrements the usage counter and removes the mark when it reaches 0
 */
void
mark_spell_counter(OBJ_DATA *obj, CHAR_DATA *ch)
{
    char *tmp = NULL;
    int uses = 0;
    char edesc[MAX_STRING_LENGTH];

    // Make sure we've got an object
    if (!obj) 
        return;

    // Time to check if that was the last casting allowed
    tmp = find_ex_description("[MARK_SPL_USES]", obj->ex_description, TRUE);
    if (tmp)
        uses = atoi(tmp);
    // Remove the mark if appropriate
    if (--uses <= 0) {
        act("$p glows briefly in $n's hands.", FALSE, ch, obj, 0, TO_ROOM);
        act("The mark upon $p glows briefly before fading away.", FALSE, ch, obj, 0, TO_CHAR);
        if (IS_SET(obj->obj_flags.extra_flags, OFL_MAGIC))
            REMOVE_BIT(obj->obj_flags.extra_flags, OFL_MAGIC);
	rem_obj_extra_desc_value(obj, "[SPELL_MARK]");
	rem_obj_extra_desc_value(obj, "[MARK_SPL_USES]");
    } else {
        act("The mark upon $p glows briefly, yet remains.", FALSE, ch, obj, 0, TO_CHAR);
	rem_obj_extra_desc_value(obj, "[MARK_SPL_USES]");
	sprintf(edesc, "%d", uses);
	set_obj_extra_desc_value(obj, "[MARK_SPL_USES]", edesc);
    }
}

/* MARK: Necessary to the TRAVEL_GATE and PORTAL spells.  It sets
 * a magickal mark on an object, which is visible to the naked eye.
 * Writes the description of the room to the object.  Cannot be
 * cast on code-generated rooms, such as burrows or the insides of
 * tents, or on objects already marked or magickal.  Also canno be
 * cast on the elemental planes.  Nilazi spell.
 * Nilaz psiak viod.
 */
void
spell_mark(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Mark */
    char ename[MAX_STRING_LENGTH], edesc[MAX_STRING_LENGTH];
    /*char tmp1[MAX_STRING_LENGTH]; */
    OBJ_DATA *tmp_obj;
    int room_num;

    check_crim_cast(ch);

    for (tmp_obj = ch->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
        if (tmp_obj == obj)
            break;
    }

    if (IS_SET(obj->obj_flags.extra_flags, OFL_MAGIC)) {
        act("You attempt to mark $p, but it seems unaffected.", FALSE, ch, obj, 0, TO_ROOM);
        return;
    }

    if (!ch->in_room) {
        gamelog("Uh, there's a character casting spell_mark who is not in a room.");
        return;
    }

    /* Doesn't work on the elemental planes */
    if ((ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_FIRE_PLANE)
        || (ch->in_room->sector_type == SECT_EARTH_PLANE)
        || (ch->in_room->sector_type == SECT_AIR_PLANE)
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)
        || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)
        || (ch->in_room->sector_type == SECT_NILAZ_PLANE)) {
        send_to_char("Your magicks fail, unable to function here.\n\r", ch);
        return;
    }

    /* Can't let them mark code generated / removed rooms */
    if (ch->in_room->zone == RESERVED_Z) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    room_num = ch->in_room->number;

    /* Ok, spell is cast. Store room number on edesc */

    strcpy(ename, "[SPELL_MARK]");
    sprintf(edesc, "%d", room_num);
    set_obj_extra_desc_value(obj, ename, edesc);

    /* Add object edesc that shows the room that was marked */
    sprintf(edesc, "You look at the image, and you see:\n\r%s%s",
            bdesc_list[ch->in_room->zone][ch->in_room->bdesc_index].desc, ch->in_room->description ? ch->in_room->description : "");
    strcpy(ename, "image");
    set_obj_extra_desc_value(obj, ename, edesc);

    act("$p glows briefly in $n's hands.", FALSE, ch, obj, 0, TO_ROOM);
    act("You cast a mark upon $p, and it glows briefly.", FALSE, ch, obj, 0, TO_CHAR);

    if (!IS_SET(obj->obj_flags.extra_flags, OFL_MAGIC))
        MUD_SET_BIT(obj->obj_flags.extra_flags, OFL_MAGIC);

    strcpy(ename, "[MARK_SPL_USES]");
    sprintf(edesc, "%d", level);
    set_obj_extra_desc_value(obj, ename, edesc);

#ifdef USE_OBJ_FLAG_TEMP
    obj->obj_flags.temp = level;
#endif
}                               /* end Spell_Mark */


/* MESSENGER: Summons a creature from the elemental plane of Whira,
 * mob #478, which the caster can then dispatch to send a message.
 * Depends on a javascript placed upon the mob.  Requires a component.
 * Whiran spell.  Whira dreth viod.
 */
void
spell_messenger(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{
    struct affected_type af;
    int vnum;
    char buf[80];

    memset(&af, 0, sizeof(af));

    if (((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))
        || (ch->in_room->sector_type == SECT_EARTH_PLANE)) {
        send_to_char("You are unable to reach into Whira from here to summon a messenger.\n\r", ch);
        return;
    }

    if (!get_componentB
        (ch, SPHERE_CONJURATION, level, "$p folds in upon itself with an audible grinding noise.",
         "$p folds in upon itself in $n's hands.",
         "You lack the correct component to lure otherwordly creatures here.")) {
        return;
    }

    check_crim_cast(ch);

    switch (GET_GUILD(ch)) {
    case GUILD_WIND_ELEMENTALIST:
        vnum = 478;
        break;
    default:
        vnum = 478;
        break;
    }

    victim = read_mobile(vnum, VIRTUAL);
    if (!victim) {
        errorlogf("spell_messenger() in magick.c unable to load NPC# %d", vnum);
        return;
    }
    sprintf(buf, "spell_messenger() in magick.c loaded NPC# %d correctly.", vnum);

    char_to_room(victim, ch->in_room);

    act("$n concentrates visibly as $N slowly shimmers into shape.", TRUE, ch, 0, victim, TO_ROOM);
    act("You focus your will as $N is brought to this plane.", TRUE, ch, 0, victim, TO_CHAR);

    add_follower(victim, ch);

    af.type = SPELL_GATE;
    af.duration = 5 + (2 * level);
    af.modifier = 0;
    af.location = 0;
    af.bitvector = CHAR_AFF_SUMMONED;
    affect_join(victim, &af, TRUE, FALSE);

}                               /* end Spell_Messenger */


/* MIRAGE: Allows the caster to create an amusing and sometimes alarming 
 * illusion.  Vivaduan version of pyrotechnics.  Mon and sul level raise
 * the thirst of everyone in the room.  Blame the snow message on Daigon.
 * Cannot be cast INDOORS or in SECT_CAVERN.  SECT_SILT lessens the effects.
 * Also cannot be cast in SECT_FIRE_PLANE, but works at one level higher
 * in SECT_WATER_PLANE.
 * Vivadu iluth nikiz.  
 */
void
spell_mirage(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Mirage */
    int effect = number(1, 6);
    char msg1[256], msg2[256];
    CHAR_DATA *curr_ch, *next_ch;

    check_crim_cast(ch);

    /* Make it so you cannot cast while enclosed by walls. */
    if ((IS_SET(ch->in_room->room_flags, RFL_INDOORS))
        || (ch->in_room->sector_type == SECT_CAVERN)) {
        send_to_char("The walls around you hamper your spell.\n\r", ch);
        return;
    }

    if (ch->in_room->sector_type == SECT_FIRE_PLANE) {
        send_to_char("The flames and heat here block your spell.\n\r", ch);
        return;
    }

    /* Effect diminished if cast in SECT_SILT */
    if (ch->in_room->sector_type == SECT_SILT)
        level = level - 2;

    if (ch->in_room->sector_type == SECT_WATER_PLANE)
        level = level + 1;

    if (level >= 7) {
        switch (effect) {
        case 1:
            strcpy(msg1, "$n stretches out a hand, and around $m, the ground bursts into blossom.");
            strcpy(msg2, "You stretch out a hand, and the ground bursts into blossom.");
            break;
        case 2:
            strcpy(msg1,
                   "A flurry of cold white flakes blows in from above, sticking to whatever they touch before melting away.");
            strcpy(msg2,
                   "A flurry of cold white flakes blows in from above, sticking to whatever they touch before melting away.");
            break;
        case 3:
            strcpy(msg1,
                   "Horta vines sprout and wriggle through the ground, carpeting it in green.");
            strcpy(msg2,
                   "Horta vines sprout and wriggle through the ground, carpeting it in green.");
            break;
        case 4:
            strcpy(msg1,
                   "Before $n, a massive baobab tree begins to form, spreading shade across the area.");
            strcpy(msg2,
                   "Before you, a massive baobab tree begins to form, spreading shade across the area.");
            break;
        case 5:
            strcpy(msg1, "Rain showers from the sky, drenching the ground with water.");
            strcpy(msg2, "Rain showers from the sky, drenching the ground with water.");
            break;
        case 6:
            strcpy(msg1, "Water spreads out around $n, till $e stands on the surface of a pool.");
            strcpy(msg2, "Water spreads out around you, till you stand on the surface of a pool.");
            break;
        default:
            strcpy(msg1, "$n stretches out a hand, and around $m, the ground bursts into blossom.");
            strcpy(msg2, "You stretch out a hand, and the ground bursts into blossom.");
            break;
        }
    } else if (level == 6) {
        switch (effect) {
        case 1:
            strcpy(msg1, "A vivid carpet of green surrounds $n, spreading outward.");
            strcpy(msg2, "A vivid carpet of green surrounds you, spreading outward.");
            break;
        case 2:
            strcpy(msg1, "A pool of water appears before $n, its surface glistening.");
            strcpy(msg2, "A pool of water appears before you, its surface glistening.");
            break;
        case 3:
            strcpy(msg1, "Fog curls and writhes around $n, cloaking all in white mist.");
            strcpy(msg2, "Fog curls and writhes around you, cloaking all in white mist.");
            break;
        case 4:
            strcpy(msg1,
                   "A small agafari tree forms before $n, casting wavering shade across the ground.");
            strcpy(msg2,
                   "A small agafari tree forms before you, casting wavering shade across the ground.");
            break;
        case 5:
            strcpy(msg1, "Tendrils of thick white fog coil around $n before reaching outward.");
            strcpy(msg2, "Tendrils of thick white fog coil around you before reaching outward.");
            break;
        case 6:
            strcpy(msg1, "Falling from the sky above, drops of water spatter against $n.");
            strcpy(msg2, "Falling from the sky above, drops of water spatter against you.");
            break;
        default:
            strcpy(msg1, "A vivid carpet of green surrounds $n, spreading outward.");
            strcpy(msg2, "A vivid carpet of green surrounds you, spreading outward.");
            break;
        }
    } else if (level == 5) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Mist coalesces around $n's form, leaving $m only a wavering outline.");
            strcpy(msg2, "Mist coalesces around your form.");
            break;
        case 2:
            strcpy(msg1, "Thunder rumbles sullenly in the sky above.");
            strcpy(msg2, "Thunder rumbles sullenly in the sky above.");
            break;
        case 3:
            strcpy(msg1, "A myriad of vines cover the ground, enfolding all in green.");
            strcpy(msg2, "A myriad of vines cover the ground, enfolding all in green.");
            break;
        case 4:
            strcpy(msg1, "A shimmer of water spreads across the dry ground.");
            strcpy(msg2, "A shimmer of water spreads across the dry ground.");
            break;
        case 5:
            strcpy(msg1, "As mist envelops $n, the air fills with damp moisture.");
            strcpy(msg2, "As mist envelops $n, the air fills with damp moisture.");
            break;
        case 6:
            strcpy(msg1, "Vines reach up from the ground, twining around $n's form.");
            strcpy(msg2, "Vines reach up from the ground, twining around your form.");
            break;
        default:
            strcpy(msg1, "Mist coalesces around $n's form, leaving $m only a wavering outline.");
            strcpy(msg2, "Mist coalesces around your form.");
            break;
        }
    } else if (level == 4) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "$n's body glints with an oily opalescence, a few drops of water falling from $s fingertips.");
            strcpy(msg2,
                   "Your body glints with an oily opalescence, a few drops of water falling from your fingertips.");
            break;
        case 2:
            strcpy(msg1, "$n's skin shimmers with moisture, drops of water beading across it.");
            strcpy(msg2, "Your skin shimmers with moisture, drops of water beading across it.");
            break;
        case 3:
            strcpy(msg1,
                   "Before $n, a carpet of leafy green plants sprouts, opening their leaves.");
            strcpy(msg2,
                   "Before you, a carpet of leafy green plants sprouts, opening their leaves.");
            break;
        case 4:
            strcpy(msg1, "All around $n, green vines begin to writhe forth from the ground.");
            strcpy(msg2, "All around you, green vines begin to writhe forth from the ground.");
            break;
        case 5:
            strcpy(msg1,
                   "Vines uncoil from the ground, putting forth blossoms to fill the air with a sweet scent.");
            strcpy(msg2,
                   "Vines uncoil from the ground, putting forth blossoms to fill the air with a sweet scent.");
            break;
        case 6:
            strcpy(msg1, "A faint rumble of thunder sounds from far above in the sky.");
            strcpy(msg2, "A faint rumble of thunder sounds from far above in the sky.");
            break;
        default:
            strcpy(msg1, "$n's skin shimmers with moisture, drops of water beading across it.");
            strcpy(msg2, "Your skin shimmers with moisture, drops of water beading across it.");
            break;
        }
    } else if (level == 3) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "A faint haze of green creeps across the ground, plants putting forth leaves.");
            strcpy(msg2,
                   "A faint haze of green creeps across the ground, plants putting forth leaves.");
            break;
        case 2:
            strcpy(msg1, "An oasis bubbles forth from the ground.");
            strcpy(msg2, "An oasis bubbles forth from the ground.");
            break;
        case 3:
            strcpy(msg1,
                   "Moss begins to sprout on the surroundings, covering all with shaggy growth.");
            strcpy(msg2,
                   "Moss begins to sprout on the surroundings, covering all with shaggy growth.");
            break;
        case 4:
            strcpy(msg1, "$n's skin flushes green before deepening to a barklike texture.");
            strcpy(msg2, "Your skin flushes green before deepening to a barklike texture.");
            break;
        case 5:
            strcpy(msg1, "The air grows cool and damp with moisture.");
            strcpy(msg2, "The air grows cool and damp with moisture.");
            break;
        case 6:
            strcpy(msg1,
                   "Lichens begin to spread across the ground, covering it with dusty green patterns.");
            strcpy(msg2,
                   "Lichens begin to spread across the ground, covering it with dusty green patterns.");
            break;
        default:
            strcpy(msg1,
                   "A faint haze of green creeps across the ground, plants putting forth leaves.");
            strcpy(msg2,
                   "A faint haze of green creeps across the ground, plants putting forth leaves.");
            break;
        }
    } else if (level == 2) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Vivid hues of green suffuse $n's skin.");
            strcpy(msg2, "Vivid hues of green suffuse your skin.");
            break;
        case 2:
            strcpy(msg1, "Cool mist eddies and whirls on the ground at ankle depth.");
            strcpy(msg2, "Cool mist eddies and whirls on the ground at ankle depth.");
            break;
        case 3:
            strcpy(msg1, "$n's eyes deepen to a lucid emerald, leaves sprouting on $s head.");
            strcpy(msg2, "Your eyes deepen to a lucid emerald, leaves sprouting on your head.");
            break;
        case 4:
            strcpy(msg1, "In front of $n, a plant unfolds from the ground, stretching upward.");
            strcpy(msg2, "In front of $n, a plant unfolds from the ground, stretching upward.");
            break;
        case 5:
            strcpy(msg1, "The air cools, the smell of growing plants pervading it.");
            strcpy(msg2, "The air cools, the smell of growing plants pervading it.");
            break;
        case 6:
            strcpy(msg1, "Dappled hues of green dance across the ground in lichenous patterns.");
            strcpy(msg2, "Dappled hues of green dance across the ground in lichenous patterns.");
            break;
        default:
            strcpy(msg1, "Vivid hues of green suffuse $n's skin.");
            strcpy(msg2, "Vivid hues of green suffuse your skin.");
            break;
        }
    } else {
        switch (effect) {
        case 1:
            strcpy(msg1, "Mist seeps upward from the ground.");
            strcpy(msg2, "Mist seeps upward from the ground.");
            break;
        case 2:
            strcpy(msg1, "$n stretches out $s hand, tendrils of mist coiling about it.");
            strcpy(msg2, "You stretch out your hand, tendrils of mist coiling about it.");
            break;
        case 3:
            strcpy(msg1, "The smell of fresh, green growth pervades the air.");
            strcpy(msg2, "The smell of fresh, green growth pervades the air.");
            break;
        case 4:
            strcpy(msg1, "In $n's palm, a single flower begins to open.");
            strcpy(msg2, "In your palm, a single flower begins to open.");
            break;
        case 5:
            strcpy(msg1, "$n purses $s lips, exhaling a stream of bubbles.");
            strcpy(msg2, "You purse your lips, exhaling a stream of bubbles.");
            break;
        case 6:
            strcpy(msg1, "In front of $n, a tiny plant bursts from the ground.");
            strcpy(msg2, "In front of you, a tiny plant bursts from the ground.");
            break;
        default:
            strcpy(msg1, "A misty sheen covers $n as $e presses $s palms together.");
            strcpy(msg2, "A misty sheen covers you as you press your palms together.");
            break;
        }
    }


    act(msg1, FALSE, ch, 0, 0, TO_ROOM);
    act(msg2, FALSE, ch, 0, 0, TO_CHAR);

    if (level >= 6) {
        for (curr_ch = ch->in_room->people; curr_ch; curr_ch = next_ch) {
            next_ch = curr_ch->next_in_room;
            GET_COND(curr_ch, THIRST) += (level * (level * number(1, 2)));
        }
    }
}                               /* end Spell_Mirage */


/* MOUNT: Creates a summoned mount, which varies according to the
 * guild of the caster and the power level.  The mounts produced 
 * by sorcerers are randomly drawn from the pool of elementalist 
 * mounts.  For all guilds except Rukkians, a component is required
 * in order to cast the spell.  Ruk wilith chran.
 */
void
spell_mount(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Mount */
    struct affected_type af;
    CHAR_DATA *mob;
    int vnum;
    char room_msg[256];
    char mage_msg[256];
    char mount_edesc[128];
    char buf1[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

/* I moved this over from spells.c   -San    */

    if (IS_SET(ch->in_room->room_flags, RFL_NO_MOUNT)) {
        send_to_char("There's not enough room to create a mount.\n\r", ch);
        return;
    }
    if ((ch->in_room->sector_type <= SECT_CITY) && (GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST)) {
        send_to_char("There's not enough sand here to do such a thing.\n\r", ch);
        return;
    }

/* Complex little if statement dictated by Tlaloc's */
/* screwy way of doings things.   -San              */

    if ((IS_TRIBE(ch, 14)) && (level >= 6)) {
        sprintf(room_msg, "$N emerges into being next to $n.");
        sprintf(mage_msg, "$N emerges into being.");
        vnum = 154 + level;
        gamelog("Those wacky Conclavers are summoning a sul or mon mount.");
    } else {
        switch (GET_GUILD(ch)) {
        case GUILD_TEMPLAR:
            sprintf(room_msg, "$N forms from bits of obsidian falling from $n's hands.");
            sprintf(mage_msg, "$N forms from bits of obsidian falling from your hands.");
            vnum = 1308 + level;
            break;
        case GUILD_FIRE_ELEMENTALIST:
            sprintf(room_msg, "$N gathers itself into being from the flames around $n.");
            sprintf(mage_msg, "$N gathers itself into being from the flames around you.");
            vnum = 100 + level;
            break;
        case GUILD_WATER_ELEMENTALIST:
            sprintf(room_msg, "The mists around $n come together, forming $N.");
            sprintf(mage_msg, "The mists around you come together, forming $N.");
            vnum = 107 + level;
            break;
        case GUILD_WIND_ELEMENTALIST:
            sprintf(room_msg, "$N forms from the winds gathered about $n.");
            sprintf(mage_msg, "$N forms from the winds gathered about you.");
            vnum = 114 + level;
            break;
        case GUILD_SHADOW_ELEMENTALIST:
            sprintf(room_msg, "$N emerges from $n's shadow.");
            sprintf(mage_msg, "$N emerges from your shadow.");
            vnum = 121 + level;
            break;
        case GUILD_LIGHTNING_ELEMENTALIST:
            sprintf(room_msg, "$N appears in a shower of sparks near $n.");
            sprintf(mage_msg, "$N appears near you in a shower of sparks.");
            vnum = 128 + level;
            break;
        case GUILD_VOID_ELEMENTALIST:
            sprintf(room_msg, "$N silently comes into being beside $n.");
            sprintf(mage_msg, "$N silently comes into being beside you.");
            vnum = 135 + level;
            break;
        case GUILD_STONE_ELEMENTALIST:
            sprintf(room_msg, "$N forms out of the desert sands before $n.");
            sprintf(mage_msg, "$N forms out of the desert sands before you.");
            vnum = 1301 + level;
            break;
        case GUILD_DEFILER:
            sprintf(room_msg, "$N slowly forms near $n's outstretched hands.");
            sprintf(mage_msg,
                    "$N forms before you as you stretch out your hands " "and concentrate.");

            switch (number(1, 7)) {
            case 1:            /* Water Elementalist */
                vnum = 107 + level;
                break;
            case 2:            /* Wind Elementalist */
                vnum = 114 + level;
                break;
            case 3:            /* Fire Elementalist */
                vnum = 100 + level;
                break;
            case 4:            /* Shadow Elementlaist */
                vnum = 121 + level;
                break;
            case 5:            /* Lightning Elementalist */
                vnum = 128 + level;
                break;
            case 6:            /* Void Elementalist */
                vnum = 135 + level;
                break;
            case 7:            /* Stone Elementalist */
                vnum = 1301 + level;
                break;
            default:
                vnum = 1301 + level;
                break;
            }
            break;
        default:
            sprintf(room_msg, "$N forms from bits of obsidian falling from $n's hands.");
            sprintf(mage_msg, "$N forms from bits of obsidian falling from your hands.");
            vnum = 1301 + level;
            break;
        }
    }

    check_crim_cast(ch);

    switch (level) {
    case POWER_WEK: sprintf(mount_edesc, "[MOUNT_SPELL_WEK_VNUM]"); break;
    case POWER_YUQA: sprintf(mount_edesc, "[MOUNT_SPELL_YUQA_VNUM]"); break;
    case POWER_KRAL: sprintf(mount_edesc, "[MOUNT_SPELL_KRAL_VNUM]"); break;
    case POWER_EEN: sprintf(mount_edesc, "[MOUNT_SPELL_EEN_VNUM]"); break;
    case POWER_PAV: sprintf(mount_edesc, "[MOUNT_SPELL_PAV_VNUM]"); break;
    case POWER_SUL: sprintf(mount_edesc, "[MOUNT_SPELL_SUL_VNUM]"); break;
    case POWER_MON: sprintf(mount_edesc, "[MOUNT_SPELL_MON_VNUM]"); break;
    default: sprintf(mount_edesc, "[MOUNT_SPELL_WEK_VNUM]"); break;
    }

    if (TRUE ==
	get_char_extra_desc_value(ch, mount_edesc, buf1, MAX_STRING_LENGTH))
      vnum = atoi(buf1);

    if (!(mob = read_mobile(vnum, VIRTUAL))) {
        errorlog("in spell_mount(), attempting to load non-existent NPC.");
        return;
    }

    if (!(GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST))
        if (!get_componentB
            (ch, SPHERE_CREATION, level,
             "$p lets off an emerald green light before crumbling to dust.",
             "$p crumbles to dust in $n's hand.",
             "You lack the correct component to create mounts."))
            return;

    char_to_room(mob, ch->in_room);

    act(room_msg, FALSE, ch, 0, mob, TO_ROOM);
    act(mage_msg, FALSE, ch, 0, mob, TO_CHAR);

    af.type = SPELL_MOUNT;
    af.duration = (level * 9);  /* 1 day per level */
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_SUMMONED;

    affect_join(mob, &af, TRUE, FALSE);

}                               /* end Spell_Mount */


/* OASIS: creates a non-takeable liquid container filled with water
 * in the room.  Amount and duration is dependent on the power level.
 * Flags the room with the RESTFUL SHADE flag at the same time.
 * Cannot be cast in rooms sectored CITY or CAVERN.  Also cannot be
 * cast in Fire, Water, or Nilaz planes.  Vivaduan spell.
 * Vivadu divan wril.
 */
void
spell_oasis(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* Begin Spell_Oasis Done by Ness, 12/94 */

    struct oasis_data
    {
        char *name;
        char *short_descr;
        char *long_descr;
    };

    struct oasis_data oasis_object[1] = {
        {
         "oasis oasis_spellobj",        /* name of obj  */
         "an oasis",            /* short_descr of obj */
         "An oasis is here, freshly formed in the sand."        /* long_descr of obj */
         }
    };

    OBJ_DATA *tmp_obj;
    OBJ_DATA *test_obj;

    if (!ch) {
        gamelog("No ch in oasis");
        return;
    }

    if ((ch->in_room->sector_type == SECT_CITY)
        || ((ch->in_room->sector_type == SECT_CAVERN)
            || (ch->in_room->sector_type == SECT_INSIDE))) {
        act("You attempt to create an oasis, but the water cannot"
            " break up through the ground here.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (ch->in_room->sector_type == SECT_FIRE_PLANE) {
        send_to_char("The heat and flames here prevent the creation of an oasis.\n\r", ch);
        return;
    }

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        send_to_char
            ("You attempt to create an oasis, but the magick sputters and dies uselessly.\n\r", ch);
        return;
    }

    if (ch->in_room->sector_type == SECT_WATER_PLANE) {
        send_to_char("There is already so much water here than an oasis is impossible.\n\r", ch);
        return;
    }

    test_obj = get_obj_in_list(ch, "oasis_spellobj", ch->in_room->contents);
    if (test_obj) {
        act("There is already an oasis here.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    check_crim_cast(ch);

    CREATE(tmp_obj, OBJ_DATA, 1);
    clear_object(tmp_obj);

    tmp_obj->name = (char *) strdup(oasis_object[0].name);
    tmp_obj->short_descr = (char *) strdup(oasis_object[0].short_descr);
    tmp_obj->long_descr = (char *) strdup(oasis_object[0].long_descr);

    tmp_obj->obj_flags.type = ITEM_DRINKCON;
    tmp_obj->obj_flags.value[0] = 40 * level;   /* MAX_AMOUNT   */
    tmp_obj->obj_flags.value[1] = 10 * level;   /* CURR_AMOUNT  */
    tmp_obj->obj_flags.value[2] = 0;    /* LIQUID       */
    tmp_obj->obj_flags.value[3] = 0;    /* POISONED     */
    tmp_obj->obj_flags.weight = 60000;  /* so they can be seen from afar */
    tmp_obj->obj_flags.cost = 1;        /* to keep people from selling'em */

    tmp_obj->prev = NULL;
    tmp_obj->next = object_list;

    object_list = tmp_obj;
    if (tmp_obj->next)
        tmp_obj->next->prev = tmp_obj;

    obj_to_room(tmp_obj, ch->in_room);

    tmp_obj->nr = 0;            /* This is bad, but necessary */
    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);

    act("$p suddenly bubbles from the ground at $n's feet.", TRUE, ch, tmp_obj, 0, TO_ROOM);
    act("$p suddenly bubbles from the ground at your feet.", TRUE, ch, tmp_obj, 0, TO_CHAR);
    new_event(EVNT_REMOVE_OBJECT, level * 3 * 60, 0, 0, tmp_obj, 0, 0);

    if (!(IS_SET(ch->in_room->room_flags, RFL_RESTFUL_SHADE))) {
        MUD_SET_BIT(ch->in_room->room_flags, RFL_RESTFUL_SHADE);
        send_to_room("A restful shade descends upon you.\n\r", ch->in_room);
        new_event(EVNT_REMOVE_SHADE, level * 3 * 60, 0, 0, 0, ch->in_room, 0);
    }
}                               /* End spell_oasis by Ness 12/94 */


/* PARALYZE: Paralyzes the target, duration dependent on power
 * level.  Non cumulative.  Elkros and sorcerer spell.
 * Elkros mutur hurn.
 */
void
spell_paralyze(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* Begin Spell_Paralyze */
    int duration;
    struct affected_type af;
    char buf[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in paralyze");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if ((affected_by_spell(victim, SPELL_TURN_ELEMENT))
        || (affected_by_spell(victim, SPELL_SHIELD_OF_NILAZ))) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (affected_by_spell(victim, SPELL_INVULNERABILITY)) {
        act("A cream-colored shell around $N flashes, blocking your spell.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("The cream-colored shell around you flashes blocking a spell from $n.", FALSE, ch, 0,
            victim, TO_VICT);
        act("A cream-colored shell around $N flashes, blocking a spell from $n.", FALSE, ch, 0,
            victim, TO_NOTVICT);
        affect_from_char(victim, SPELL_INVULNERABILITY);
        send_to_char("The creamy shell around you fades away.\n\r", victim);
        act("The creamy shell around $n fades away.", FALSE, victim, 0, 0, TO_ROOM);
        if (IS_AFFECTED(victim, CHAR_AFF_INVULNERABILITY))
            REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_INVULNERABILITY);
        return;
    }
    if (affected_by_spell(victim, SPELL_WIND_ARMOR)) {
        act("The winds around $N swirl violently, blocking your spell.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("The winds around you swirl violently, blocking a spell from $n.", FALSE, ch, 0, victim,
            TO_VICT);
        act("The winds around $N swirl violently, blocking a spell from $n.", FALSE, ch, 0, victim,
            TO_NOTVICT);
        if (!(IS_AFFECTED(victim, CHAR_AFF_SLOW)))
            spell_slow(level, magick_type, ch, victim, 0);
        return;
    }
    if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
        act("The flames around $N flare violently, blocking your spell.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("The flames around you flare violently, blocking a spell from $n.", FALSE, ch, 0,
            victim, TO_VICT);
        act("The flames around $N flare violently, blocking a spell from $n.", FALSE, ch, 0, victim,
            TO_NOTVICT);
        if (!(IS_AFFECTED(victim, CHAR_AFF_SLOW)))
            spell_slow(level, magick_type, ch, victim, 0);
        return;
    }
    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
        act("The shadows around $N solidify briefly, blocking your spell.", FALSE, ch, 0, victim,
            TO_CHAR);
        act("The shadows around you solidify briefly, blocking a spell from $n.", FALSE, ch, 0,
            victim, TO_VICT);
        act("The shadows around $N solidify briefly, blocking a spell from $n.", FALSE, ch, 0,
            victim, TO_NOTVICT);
        if (!(IS_AFFECTED(victim, CHAR_AFF_SLOW)))
            spell_slow(level, magick_type, ch, victim, 0);
        return;
    }
    if (affected_by_spell(victim, SPELL_STONE_SKIN)) {
        act("$N's stone-like skin grows harder for a moment, blocking your spell.", FALSE, ch, 0,
            victim, TO_CHAR);
        act("Your stone-like skin grows harder for a moment, blocking a spell from $n.", FALSE, ch,
            0, victim, TO_VICT);
        act("$N's stone-like skin grows harder for a moment, blocking a spell from $n.", FALSE, ch,
            0, victim, TO_NOTVICT);
        if (!(IS_AFFECTED(victim, CHAR_AFF_SLOW)))
            spell_slow(level, magick_type, ch, victim, 0);
        return;
    }
    if (affected_by_spell(victim, SPELL_ENERGY_SHIELD)) {
        act("The sparkles of light surrounding $N flash briefly, blocking your spell.", FALSE, ch,
            0, victim, TO_CHAR);
        act("The sparkles of light surrounding you flash briefly, blocking a spell from $n.", FALSE,
            ch, 0, victim, TO_VICT);
        act("The sparkles of light around $N flash briefly, blocking a spell from $n.", FALSE, ch,
            0, victim, TO_NOTVICT);
        if (!(IS_AFFECTED(victim, CHAR_AFF_SLOW)))
            spell_slow(level, magick_type, ch, victim, 0);
        return;
    }
    if ((!affected_by_spell(victim, SPELL_PARALYZE)) && (!IS_IMMORTAL(victim))
        && !does_save(victim, SAVING_SPELL, ((level - 4) * -5))) {
        act("$n points at you and you feel a surge of energy enter your body, causing you to jerk spastically.", FALSE, ch, 0, victim, TO_VICT);
        act("The energy drains from your body, and your body ceases to respond.", FALSE, ch, 0,
            victim, TO_VICT);
        /* This 'if' to be sure victim is resting or unconscious. -Savak */
        if (GET_POS(victim) > POSITION_RESTING)
            set_char_position(victim, POSITION_RESTING);
        /* This 'if' simply changes victim's ldesc if possible. -Savak */
        if (!change_ldesc("is lying here, rigid and unmoving.", victim))
            shhlogf("ERROR: %s's ldesc too long.  magick.c  Fnxn: void spell_paralyze.",
                    MSTR(victim, name));
        if (victim != ch) {
            act("You point at $N sending a surge of energy into $M.", FALSE, ch, 0, victim,
                TO_CHAR);
            act("$n points at $N and $E spasms for a moment, then ceases to move.", FALSE, ch, 0,
                victim, TO_NOTVICT);
        } else {
            act("You zap yourself with a surge of energy.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n zaps $mself with a surge of energy.", FALSE, ch, 0, victim, TO_NOTVICT);
        }
    }

    else {
        if (victim != ch)
            send_to_char("Your victim resists your spell.\n\r", ch);
        else
            act("You resist a spell from $n.", FALSE, ch, 0, victim, TO_VICT);

        /* Let's make it so people can't powergame this on NPCs.  -San */
        if ((IS_NPC(victim)) && (!victim->specials.fighting)) {
            find_ch_keyword(ch, victim, buf, sizeof(buf));
            cmd_kill(victim, buf, CMD_KILL, 0);
        }

        return;
    }

    duration = level / 2;

    duration = MAX(1, duration);        // No 0 durations
    af.type = SPELL_PARALYZE;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = CFL_FROZEN;
    af.location = CHAR_APPLY_CFLAGS;
    af.bitvector = 0;

    gamelogf("%s has paralyzed %s in spell_paralyze, in room %d.", GET_NAME(ch), GET_NAME(victim),
             ch->in_room->number);

    affect_to_char(victim, &af);
    snprintf(buf, sizeof(buf), "%s is paralyzed.", MSTR(victim, short_descr));
    send_to_empaths(victim, buf);
}                               /* end of Spell_Paralyze */


/* PARCH: Drops the target's thirst points, and will do damage
 * if they are already thirsty.  Will not affect fire elementals.
 * Suk Krathis, or targets with the FIREBREATHER affect on them.
 * Will not function on the plane of Water.
 * Suk Krathi spell.  Suk-Krath nathro echri.
 */
void
spell_parch(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Parch */
    int amount;
    int dam;

    if (!victim) {
        gamelog("No victim in parch");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if ((ch->in_room->sector_type == SECT_WATER_PLANE)
        || (affected_by_spell(victim, SPELL_BREATHE_WATER))) {
        send_to_char("Nothing seems to happen as your spell fizzles uselessly.\n\r", ch);
        return;
    }

    amount = MIN(GET_COND(victim, THIRST), number(level, (level * level)));
    dam = (number(1, level) * 2);
    if (victim != ch) {
        if (!does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
            act("A red aura from $n's hands surrounds $N, then dissipates.", FALSE, ch, 0, victim,
                TO_ROOM);
            act("A red aura from your hands surrounds $N, then dissipates.", FALSE, ch, 0, victim,
                TO_CHAR);
        } else {
            act("A red aura from $n's hands fails to surround $N.", FALSE, ch, 0, victim, TO_ROOM);
            act("A red aura from your hands fails to surround $N.", FALSE, ch, 0, victim, TO_CHAR);
            return;
        }
    } else {
        act("A red aura surrounds $n, then dissipates.", FALSE, ch, 0, victim, TO_ROOM);
        act("A red aura surrounds you, then dissipates.", FALSE, ch, 0, victim, TO_CHAR);
    }

    /* Ok, now the pro's/con's */
    /* If this is cast on a fire elemental, it drains water from them, which */
    /* they don't have, so it does nothing. */
    if ((GET_RACE(victim) == RACE_ELEMENTAL) && (GET_GUILD(victim) == GUILD_FIRE_ELEMENTALIST)) {
        send_to_char("You feel a warm sensation, which passes quickly\n\r.", victim);
        return;
    }

    /* If they have the spell firebreather in affect, they also have no water */
    /* in their systems */
    if (affected_by_spell(victim, SPELL_FIREBREATHER)) {
        send_to_char("You feel a warm sensation, which passes quickly\n\r.", victim);
        return;
    }
    /* Otherwise.. drain them of their water by amount */

/* Added a statement so if they are already thirsty, causes */
/* additional damage.  -San 10/8/00                         */

    if (GET_COND(victim, THIRST) <= 10) {
        generic_damage(victim, dam, 0, 0, dam);
    };

    GET_COND(victim, THIRST) = MAX(0, GET_COND(victim, THIRST) - amount);
    send_to_char("Your body spasms, as its water is slowly drained.\n\r", victim);
    return;
}                               /* end Spell_Parch */


/* PHANTASM: Allows the caster to create a frightening and
 * sometimes truly terrifying illusion.  Nilazian version of pyrotechnics.
 * Mon level makes people run in terror.  Effect is heightened if
 * cast in SECT_NILAZ_PLANE or SECT_SILT.  Effect is lessened if cast
 * in Water, Fire, Wind, Earth, Shadow, or Lightning Planes.
 * Nilaz iluth nikiz. 
 */
void
spell_phantasm(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Phantasm */
    int effect = number(1, 8);
    char msg1[256], msg2[256];
    char fleemessage[MAX_STRING_LENGTH];
    CHAR_DATA *curr_ch, *next_ch;

    check_criminal_flag(ch, NULL);

    /* Effect heightened if cast in SECT_NILAZ_PLANE or SECT_SILT */
    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) || (ch->in_room->sector_type == SECT_SILT)) {
        level = level + 2;
        /* since there is no case statement at 8 or 9, although
         * this would be a cool addition later on               */
        if (level >= 7)
            level = 7;
    }

    if ((ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_FIRE_PLANE)
        || (ch->in_room->sector_type == SECT_AIR_PLANE)
        || (ch->in_room->sector_type == SECT_EARTH_PLANE)
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)
        || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)) {
        level = level - 2;
        if (level <= 1)
            level = 1;
    }

    if (level == 7) {
        switch (effect) {
        case 1:
            strcpy(msg1, "The ground shudders and cracks, grey serpents pouring from the opening.");
            strcpy(msg2, "The ground shudders and cracks, grey serpents pouring from the opening.");
            break;
        case 2:
            strcpy(msg1,
                   "$n spreads $s hands, and the ground erupts in skeletons, clawing their way free of the earth.");
            strcpy(msg2,
                   "You spread your hands, and the ground erupts in skeletons, clawing their way free of the earth.");
            break;
        case 3:
            strcpy(msg1, "All around you, the air writhes, filled with shrieking phantasms.");
            strcpy(msg2, "All around you, the air writhes, filled with shrieking phantasms.");
            break;
        case 4:
            strcpy(msg1, "$n's form towers upwards, becoming an immense skeleton.");
            strcpy(msg2, "Your form towers upwards, becoming an immense skeleton.");
            break;
        case 5:
            strcpy(msg1,
                   "A horde of grey phantasms fills the air, clawing at the air with skeletal fingers.");
            strcpy(msg2,
                   "A horde of grey phantasms fills the air, clawing at the air with skeletal fingers.");
            break;
        case 6:
            strcpy(msg1, "Blades slash out through the air from $n's hand in a deadly rain.");
            strcpy(msg2, "Blades slash out through the air from your hand in a deadly rain.");
            break;
        default:
            strcpy(msg1, "The ground shudders and cracks, grey serpents pouring from the opening.");
            strcpy(msg2, "The ground shudders and cracks, grey serpents pouring from the opening.");
            break;
        }
    } else if (level == 6) {
        switch (effect) {
        case 1:
            strcpy(msg1, "In front of $n, an immense fanged maw opens, gaping in blackness.");
            strcpy(msg2, "In front of you, an immense fanged maw opens, gaping in blackness.");
            break;
        case 2:
            strcpy(msg1, "A demonic figure forms slowly behind $n, standing at $s shoulder.");
            strcpy(msg2, "A demonic figure forms slowly behind you, standing at your shoulder.");
            break;
        case 3:
            strcpy(msg1,
                   "$n stretches out immense grey, leathery wings and grins, showing a mouth of pointed teeth.");
            strcpy(msg2,
                   "You stretch out immense grey, leathery wings and grin, showing a mouth of pointed teeth.");
            break;
        case 4:
            strcpy(msg1,
                   "The ground parts, revealing an ancient skeleton, grinning as it begins to stand.");
            strcpy(msg2,
                   "The ground parts, revealing an ancient skeleton, grinning as it begins to stand.");
            break;
        case 5:
            strcpy(msg1, "The air darkens, filled with deafening wails and shrieks.");
            strcpy(msg2, "The air darkens, filled with deafening wails and shrieks.");
            break;
        case 6:
            strcpy(msg1,
                   "Hisses fill the air, as small grey serpents pour from cracks in the ground.");
            strcpy(msg2,
                   "Hisses fill the air, as small grey serpents pour from cracks in the ground.");
            break;
        default:
            strcpy(msg1, "In front of $n, an immense fanged maw opens, gaping in blackness.");
            strcpy(msg2, "In front of you, an immense fanged maw opens, gaping in blackness.");
            break;
        }
    } else if (level == 5) {
        switch (effect) {
        case 1:
            strcpy(msg1, "A vortex opens in the air before $n, hungry tendrils reaching forth.");
            strcpy(msg2, "A vortex opens in the air before you, hungry tendrils reaching forth.");
            break;
        case 2:
            strcpy(msg1, "$n's form blurs, becoming a grinning skeleton.");
            strcpy(msg2, "Your form blurs, becoming a grinning skeleton.");
            break;
        case 3:
            strcpy(msg1, "An eerie vortex forms around $n, air rushing inward.");
            strcpy(msg2, "An eerie vortex forms around $n, air rushing inward.");
            break;
        case 4:
            strcpy(msg1, "Wisps of phantasms arise around $n, shrieking out in agony.");
            strcpy(msg2, "Wisps of phantasms arise around you, shrieking out in agony.");
            break;
        case 5:
            strcpy(msg1, "The air grows deadly cold as swirls of grey mist surround $n.");
            strcpy(msg2, "The air grows deadly cold as swirls of grey mist surround $n.");
            break;
        case 6:
            strcpy(msg1, "$n's form writhes, falling apart into a mass of flies and maggots.");
            strcpy(msg2, "Your form writhes, falling apart into a mass of flies and maggots.");
            break;
        default:
            strcpy(msg1, "A vortex opens in the air before $n, hungry tendrils reaching forth.");
            strcpy(msg2, "A vortex opens in the air before you, hungry tendrils reaching forth.");
            break;
        }
    } else if (level == 4) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Sounds dim and colors fade as $m gestures.");
            strcpy(msg2, "Sounds dim and colors fade as you gesture.");
            break;
        case 2:
            strcpy(msg1, "The air grows still and you find yourself gasping for breath.");
            strcpy(msg2, "You gesture, and the air goes still and lifeless.");
            break;
        case 3:
            strcpy(msg1, "At $n's feet, the ground cracks, and a skeletal hand reaches forth.");
            strcpy(msg2, "At your feet, the ground cracks, and a skeletal hand reaches forth.");
            break;
        case 4:
            strcpy(msg1, "A myriad voices fill the air, screaming out warnings and despair.");
            strcpy(msg2, "A myriad voices fill the air, screaming out warnings and despair.");
            break;
        case 5:
            strcpy(msg1,
                   "The ground shudders in agony, as skeletons begin to crawl from its depths.");
            strcpy(msg2,
                   "The ground shudders in agony, as skeletons begin to crawl from its depths.");
            break;
        case 6:
            strcpy(msg1, "Grey tendrils lash forth from $n's hands, striking angrily.");
            strcpy(msg2, "Grey tendrils lash forth from your hands, striking angrily.");
            break;
        default:
            strcpy(msg1, "Sounds dim and colors fade as $n gestures.");
            strcpy(msg2, "Sounds dim and colors fade as you gesture.");
            break;
        }
    } else if (level == 3) {
        switch (effect) {
        case 1:
            strcpy(msg1, "The air grows grey and still, color leaching from the surroundings.");
            strcpy(msg2, "The air grows grey and still, color leaching from the surroundings.");
            break;
        case 2:
            strcpy(msg1, "Hallucinatory images of death and bloodshed shimmer in the air.");
            strcpy(msg2, "Hallucinatory images of death and bloodshed shimmer in the air.");
            break;
        case 3:
            strcpy(msg1, "A swarm of insects gathers in $n's palms before spinning outward.");
            strcpy(msg2, "A swarm of insects gathers in your palms before spinning outward.");
            break;
        case 4:
            strcpy(msg1, "A wave of deathly grey washes over $n's features.");
            strcpy(msg2, "A wave of deathly grey washes over your features.");
            break;
        case 5:
            strcpy(msg1, "$n opens $s hand, a tiny skeleton capering macabrely on the palm.");
            strcpy(msg2, "You opens your hand, a tiny skeleton capering macabrely on the palm.");
            break;
        case 6:
            strcpy(msg1, "$n's features writhe as $s face becomes a grinning skull.");
            strcpy(msg2, "Your features writhe as your face becomes a grinning skull.");
            break;
        default:
            strcpy(msg1, "The air grows grey and still, color leaching from the surroundings.");
            strcpy(msg2, "The air grows grey and still, color leaching from the surroundings.");
            break;
        }
    } else if (level == 2) {
        switch (effect) {
        case 1:
            strcpy(msg1, "The air grows still and lifeless, its feel clammy and unsettling.");
            strcpy(msg2, "The air grows still and lifeless, its feel clammy and unsettling.");
            break;
        case 2:
            strcpy(msg1, "The air before $n shimmers, an inchoate image slowly forming.");
            strcpy(msg2, "The air before you shimmers, an inchoate image slowly forming.");
            break;
        case 3:
            strcpy(msg1,
                   "A ruddy haze fills the air, the scent of fresh blood strong and nauseating.");
            strcpy(msg2,
                   "A ruddy haze fills the air, the scent of fresh blood strong and nauseating.");
            break;
        case 4:
            strcpy(msg1,
                   "Smoke gathers, becoming the phantasmal shapes of skulls floating in the air.");
            strcpy(msg2,
                   "Smoke gathers, becoming the phantasmal shapes of skulls floating in the air.");
            break;
        case 5:
            strcpy(msg1, "The ground writhes at $n's feet, becoming a seething mass of worms.");
            strcpy(msg2, "The ground writhes at your feet, becoming a seething mass of worms.");
            break;
        case 6:
            strcpy(msg1, "Tendrils of grey writhe outward from $n's hands.");
            strcpy(msg2, "Tendrils of grey writhe outward from $n's hands.");
            break;
        default:
            strcpy(msg1, "The air grows still and lifeless, its feel clammy and unsettling.");
            strcpy(msg2, "The air grows still and lifeless, its feel clammy and unsettling.");
            break;
        }
    } else {
        switch (effect) {
        case 1:
            strcpy(msg1, "The air grows still and lifeless.");
            strcpy(msg2, "The air grows still and lifeless.");
            break;
        case 2:
            strcpy(msg1, "A chill permeates the air, sending shivers up your spine.");
            strcpy(msg2, "A chill permeates the air.");
            break;
        case 3:
            strcpy(msg1, "The flesh of $n's face splits, revealing a mass of writhing maggots.");
            strcpy(msg2, "The flesh of $n's face splits, revealing a mass of writhing maggots.");
            break;
        case 4:
            strcpy(msg1, "$n laughs, crawling insects spewing from $s open mouth.");
            strcpy(msg2, "You laugh, crawling insects spewing from your open mouth.");
            break;
        case 5:
            strcpy(msg1, "$n's eyes become yawning, blank abysses, gazing towards you.");
            strcpy(msg2, "Your eyes hollow into voids as you stare forward.");
            break;
        case 6:
            strcpy(msg1,
                   "A mournful wail echoes through the air as $n makes a slicing gesture with $s hand.");
            strcpy(msg2,
                   "A mournful wail echoes through the air as you make a slicing gesture with your hand");
            break;
        default:
            strcpy(msg1, "The air grows still and lifeless.");
            strcpy(msg2, "The air grows still and lifeless.");
            break;
        }
    }

    act(msg1, FALSE, ch, 0, 0, TO_ROOM);
    act(msg2, FALSE, ch, 0, 0, TO_CHAR);

    strcpy(fleemessage, "flee self");

/* at mon, you pee your pants and run */
    if (level >= 7) {
        for (curr_ch = ch->in_room->people; curr_ch; curr_ch = next_ch) {
            next_ch = curr_ch->next_in_room;
            if ((number(0, 1)) && (curr_ch != ch))
                parse_command(curr_ch, fleemessage);
        }
    }

}                               /* end Spell_Phantasm */

/* POISON: Can be cast on a person, drink container, food object
 * or weapon (except for blunt weapons).  Vivadu and sorcerer
 * spell.  Vivadu viqrol hurn.
 */
void
spell_poison(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Poison */
    struct affected_type af;
    char buf[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    if (!victim && !obj) {
        gamelog("No victim/object in poison");
        return;
    }

    if (victim) {

        check_criminal_flag(ch, NULL);       /* Poisoning an npc/pc is aggressive */

        if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
            act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        if (!does_save(victim, SAVING_PARA, (level * -5))) {
            af.type = SPELL_POISON;
            af.duration = level;
            af.power = level;
            af.magick_type = magick_type;
            af.modifier = -(level / 3);
            af.location = CHAR_APPLY_STR;
            af.bitvector = CHAR_AFF_DAMAGE;

            affect_join(victim, &af, FALSE, FALSE);

            if (victim == ch) {
                act("You feel very sick as you surround yourself with a dark, malign aura.", FALSE,
                    ch, 0, 0, TO_CHAR);
                act("$n surrounds $mself with a dark, malign aura.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                act("You surround $N with a dark, malign aura.", FALSE, ch, 0, victim, TO_CHAR);
                act("You feel very sick as $n surrounds you with a dark, malign aura.", FALSE, ch,
                    0, victim, TO_VICT);
                act("$n surrounds $N with a dark, malign aura.", TRUE, ch, 0, victim, TO_NOTVICT);
            }
            if ((IS_NPC(victim)) && (!victim->specials.fighting)) {
                find_ch_keyword(ch, victim, buf, sizeof(buf));
                cmd_kill(victim, buf, CMD_KILL, 0);
            }
        }
    } else {

        check_crim_cast(ch);    /* Poisoning an object is not aggressive */

        if ((obj->obj_flags.type == ITEM_DRINKCON) || (obj->obj_flags.type == ITEM_FOOD)) {
            /* send_to_char("You surround it with the dark aura " "of poison.\n\r", ch); */
            act("You surround $p with the dark aura of poison.", FALSE, ch, obj, 0, TO_CHAR);
            act("$N surrounds $p with a dark, malign aura.", FALSE, ch, obj, ch, TO_NOTVICT);
            obj->obj_flags.value[3] = 1;
            obj->obj_flags.value[5] = 1;
        }
        if (obj->obj_flags.type == ITEM_WEAPON) {
            if (obj->obj_flags.value[3] == TYPE_BLUDGEON) {
                send_to_char("The poison refuses to cling to the " "weapon.\n\r", ch);
            } else {
                act("You surround $p with the dark aura of poison.", FALSE, ch, obj, 0, TO_CHAR);
                act("$N surrounds $p with a dark, malign aura.", FALSE, ch, obj, ch, TO_NOTVICT);
            }
            obj->obj_flags.value[5] = 1;
        }
    }
}                               /* end Spell_Poison */


/* Helper functions for the portable hole spell */

int
find_nilaz_chest(CHAR_DATA * ch)
{
    char ename[MAX_STRING_LENGTH], edesc[MAX_STRING_LENGTH];
    char tmp1[MAX_STRING_LENGTH];
    OBJ_DATA *chest, *temp, *next_obj;
    ROOM_DATA *storage_room;

    if (!ch)
        return FALSE;
    if (!(storage_room = get_room_num(73999))) {
        gamelog("Error in Portable_hole spell: Missing room 73999.");
        return FALSE;
    }

    chest = read_object(34502, VIRTUAL);
    if (!chest) {
        send_to_char("Nothing happened.\n\r", ch);
        gamelog("No chest object #34502, for spell portable hole.");
        return FALSE;
    }


    /* Set descriptions to look for/store */
/*  sprintf(ename, "[Storage_name %s]", GET_NAME(ch)); */
    strcpy(ename, "[Storage_name]");
    sprintf(edesc, "%s", GET_NAME(ch));

    /* Search room to see if a chest object (temp) is present */
    for (temp = storage_room->contents; temp; temp = next_obj) {
        next_obj = temp->next;

        if (obj_index[temp->nr].vnum == 34502) {
/*          gamelog("Chest found.");  */
            /* Loop to find chest(temp) with edesc of chars name */
            if (get_obj_extra_desc_value(temp, ename, tmp1, MAX_STRING_LENGTH)) {
                /*              if (IS_IMMORTAL(ch))
                 * {
                 * sprintf(bug, "Chest found with values:\n\r"
                 * "  %s %s", ename, tmp1);
                 * gamelog(bug);
                 * }
                 */
                if (STRINGS_SAME(tmp1, edesc)) {
                    extract_obj(chest);
                    chest = temp;
                    break;
                }
            }
            /* End if the objects edesc is characters name */
            /*          if (IS_IMMORTAL(ch))
             * gamelog("Chest found, but no tmp1 value."); */
        }                       /* End if chest object is found in the room */
    }                           /* End for loop */

    if (chest != temp) {
        obj_to_room(chest, storage_room);
        /* set edesc on chest */
        set_obj_extra_desc_value(chest, ename, edesc);
    }

    return TRUE;
}


/* PORTABLE HOLE: Creates an inter-dimensionary hole next to the caster, 
 * which stays hovering and cannot be moved.  When its duration expires,
 * it disappears. The temp object is object #34501.  Cannot be cast on
 * the Planes of Water, Fire, Air, Earth, Shadow, or Lightning.  Nilazi spell.
 * Nilaz wilith viod.
 */
void
spell_portable_hole(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Portable_Hole */
    int duration;
    char tmp1[MAX_STRING_LENGTH] /*, tmp2[MAX_STRING_LENGTH] */ ;
    /* char buf[MAX_STRING_LENGTH]; */
    char ename[MAX_STRING_LENGTH], edesc[MAX_STRING_LENGTH];
    OBJ_DATA *temp, *next_obj;
    OBJ_DATA *hole;

    check_criminal_flag(ch, NULL);

    if ((ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_FIRE_PLANE)
        || (ch->in_room->sector_type == SECT_AIR_PLANE)
        || (ch->in_room->sector_type == SECT_EARTH_PLANE)
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)
        || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)) {
        send_to_char("You cannot reach Nilaz to open a portable hole from here.\n\r", ch);
        return;
    }

    hole = read_object(34501, VIRTUAL);
    if (!hole) {
        send_to_char("Nothing happened.\n\r", ch);
        gamelog("No hole object #34501, for spell portable hole.");
        return;
    }


    /* Get edesc/ename for portable hole object */
    strcpy(ename, "[Storage_name]");
    sprintf(edesc, "%s", GET_NAME(ch));
    set_obj_extra_desc_value(hole, ename, edesc);

    /* Search room to see if a hole object is present */
    for (temp = ch->in_room->contents; temp; temp = next_obj) {
        next_obj = temp->next;

        if (obj_index[temp->nr].vnum == 34501) {
            /* Hole exists.  Is it the casters? */
            if (get_obj_extra_desc_value(temp, ename, tmp1, MAX_STRING_LENGTH)) {
                if (STRINGS_SAME(tmp1, edesc)) {
                    send_to_char("Nothing happened.\n\r", ch);
                    extract_obj(hole);
                    return;
                }
            }
        }
    }

    if (!find_nilaz_chest(ch)) {
        gamelog("Error in spell_portable_hole, returned 0 from find_nilaz_chest.");
        send_to_char("Nothing happened.", ch);
        extract_obj(hole);
        return;
    }


    obj_to_room(hole, ch->in_room);
    act("You summon $p from the void.", FALSE, ch, hole, 0, TO_CHAR);
    act("$n makes a gesture with $s hands, and $p folds into existence.", FALSE, ch, hole, 0,
        TO_ROOM);

    /* Find casters chest */
    duration = (level * 2 * level * 60);
    new_event(EVNT_REMOVE_OBJECT, duration, 0, 0, hole, 0, 0);
}                               /* end Spell_Portable_Hole */


/* PORTAL: Uses the element of Nilaz to create a two-way portal in
 * space to anyone in the known world. The portal is only temporary, and
 * usually short lived, though powerlevel affects the duration of the
 * portal. Anyone can enter the portal on either side.  Requires a
 * component.  Cannot open a portal to someone on any elementl plane as
 * they are too far away for it to work.  Nilazi and Sorcerer spell.
 * Nilaz locror nikiz.
 */
void
spell_portal(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{
    OBJ_DATA *origin_portal, *dest_portal;
    ROOM_DATA *to_room;
    char *tmp;
    int max_size_allwd;

    check_criminal_flag(ch, NULL);


    if (!get_componentB
        (ch, SPHERE_TELEPORTATION, level,
         "$p flashes with a bright light before crumbling to dust.",
         "$p crumbles to dust in $n's hand.", "You lack the correct component to portal."))
        return;

    if (obj) {
      tmp = find_ex_description("[SPELL_MARK]", obj->ex_description, TRUE);
      if (tmp)
        to_room = get_room_num(atoi(tmp));
      else {
        send_to_char("Nothing happens.\n\r", ch);
        return;
      }
    } else if (victim && victim->in_room) {
        /* Do not allow portal to be cast to a PC/NPC in an immortal room */
        if (IS_SET(victim->in_room->room_flags, RFL_IMMORTAL_ROOM)
            || IS_SET(victim->in_room->room_flags, RFL_NO_MAGICK)) {
            send_to_char("Nothing happens.\n\r", ch);
            return;

        } else if (!is_on_same_plane(ch->in_room, victim->in_room)) {
            send_to_char("You cannot open a portal to someone on another plane.\n\r", ch);
            return;
        } else
            to_room = victim->in_room;
    } else {
        send_to_char("Error in spell: contact an immortal about the problem.", ch);
        return;
    }

    // If the room already has an 'exit' don't allow portal -Morg
    if (find_exitable_wagon_for_room(to_room)) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    if (find_exitable_wagon_for_room(ch->in_room)) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    if ((origin_portal = read_object(PORTAL_OBJ, VIRTUAL)) == NULL) {
        send_to_char("Error in spell: contact an immortal about the problem.", ch);
        return;
    }

    if (tuluk_magic_barrier(ch, to_room)) {
      qgamelogf(QUIET_MISC,
                "tuluk_magic_barrier: %s tried to cast portal in #%d to #%d, blocked.",
                MSTR(ch, name),
                ch->in_room->number,
                to_room->number);
      send_to_char("Nothing happens.\n\r", ch);
      return;
    }

    if (allanak_magic_barrier(ch, to_room)) {
      if (room_in_city(ch->in_room) == CITY_ALLANAK) {
        if (!IS_SET(ch->specials.act, CFL_CRIM_ALLANAK)) {
          qgamelogf(QUIET_MISC,
                    "allanak_magic_barrier: %s tried to cast portal in #%d to #%d, blocked.",
                    MSTR(ch, name),
                    ch->in_room->number,
                    to_room->number);
          send_to_char("Nothing happens.\n\r", ch);
          return;
        }
        // Portaling out of Allanak goes to Arena
        qgamelogf(QUIET_MISC,
                  "allanak_magic_barrier: %s tried to cast portal in #%d to #%d, re-directed to arena.",
                  MSTR(ch, name),
                  ch->in_room->number,
                  to_room->number);
        to_room = get_room_num(50664);
      } else {
        // portaling into Allanak goes to Sewer
        qgamelogf(QUIET_MISC,
                  "allanak_magic_barrier: %s tried to cast portal in #%d to #%d, re-directed to sewer.",
                  MSTR(ch, name),
                  ch->in_room->number,
                  to_room->number);
        to_room = pick_allanak_barrier_sewer_room();
      }
      if (0 == to_room) {
        gamelog("Error in portal spell, game could not find either room # 50664 or # 54325.");
        send_to_char("Error in spell: contact an immortal about the problem.", ch);
      }
    }

    origin_portal->obj_flags.value[0] = to_room->number;
    origin_portal->obj_flags.value[2] = -1;
    MUD_SET_BIT(origin_portal->obj_flags.value[3], WAGON_NO_LEAVE);
    MUD_SET_BIT(origin_portal->obj_flags.value[3], WAGON_NO_PSI_PATH);
    origin_portal->obj_flags.cost = MAX(1, level / 2);
    obj_to_room(origin_portal, ch->in_room);
    act("$n makes a broad, circular motion with $s hands and $p swirls into existence.", FALSE, ch,
        origin_portal, NULL, TO_ROOM);
    act("You make a broad, circular motion with your hands and $p swirls into existence.", FALSE,
        ch, origin_portal, NULL, TO_CHAR);

    if ((dest_portal = read_object(PORTAL_OBJ, VIRTUAL)) == NULL) {
        send_to_char("Error in spell: contact an immortal about the problem.", ch);
        return;
    }

    dest_portal->obj_flags.value[0] = ch->in_room->number;
    dest_portal->obj_flags.value[2] = -1;
    MUD_SET_BIT(dest_portal->obj_flags.value[3], WAGON_NO_LEAVE);
    MUD_SET_BIT(dest_portal->obj_flags.value[3], WAGON_NO_PSI_PATH);
    dest_portal->obj_flags.cost = MAX(1, level / 2);

    if (level <= 2) {           // yuqa or lower
        MUD_SET_BIT(dest_portal->obj_flags.value[3], WAGON_NO_ENTER);
    }

    obj_to_room(dest_portal, to_room);

    max_size_allwd = 10 + (4 * (level - 1));
    origin_portal->obj_flags.value[5] = max_size_allwd;
    dest_portal->obj_flags.value[5] = max_size_allwd;

    if (to_room->people) {
        act("$p swirls into existence.", FALSE, to_room->people, dest_portal, NULL, TO_ROOM);
        act("$p swirls into existence.", FALSE, to_room->people, dest_portal, NULL, TO_CHAR);
    }
    // See if the mark remains
    mark_spell_counter(obj, ch);
}

/* Helper function for mon possess_corpse
 *
 * This function will copy all the strings (desc, sdesc, etc) from the
 * corpse's original character (NPC or PC) onto the caster.  Essentially,
 * the caster will permanently transform into the image of the person before
 * they had died.
 */
bool
permanent_possession(CHAR_DATA * caster, CHAR_DATA * host, OBJ_DATA * corpse)
{
    int pos;
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *tmp_object, *obj_object, *next_obj;
    CLAN_DATA *pClan;
    struct extra_descr_data *tmp = NULL;

    /* Don't restring NPCs or Immortals permanently */
    if (IS_NPC(caster) || IS_IMMORTAL(caster))
        return FALSE;

    /* Some echoes to start us off before we change anything */
    act("You lean over $p and exhale a cloud of grey smoke.", TRUE, caster, corpse, 0, TO_CHAR);
    act("$n exhales a cloud of grey smoke as $e leans over $p.", TRUE, caster, corpse, 0, TO_ROOM);
    act("$n's body dissolves into dust.", TRUE, caster, corpse, 0, TO_ROOM);

    /* Change appropriate caster's settings to those of the host */
    caster->player.race = host->player.race;
    caster->player.apparent_race = host->player.apparent_race;
    caster->player.sex = host->player.sex;
    caster->player.height = host->player.height;
    caster->player.weight = host->player.weight;
    caster->player.sex = host->player.sex;
    // get their birthday
    caster->player.time.starting_time = host->player.time.starting_time;
    caster->player.time.starting_age = host->player.time.starting_age;

    caster->abilities.str = host->abilities.str;
    caster->abilities.agl = host->abilities.agl;
    caster->abilities.end = host->abilities.end;
    caster->abilities.armor = host->abilities.armor;
    caster->abilities.off = host->abilities.off;
    caster->abilities.def = host->abilities.def;
    caster->abilities.damroll = host->abilities.damroll;

    caster->tmpabilities.str = host->tmpabilities.str;
    caster->tmpabilities.agl = host->tmpabilities.agl;
    caster->tmpabilities.end = host->tmpabilities.end;
    caster->tmpabilities.armor = host->tmpabilities.armor;
    caster->tmpabilities.off = host->tmpabilities.off;
    caster->tmpabilities.def = host->tmpabilities.def;
    caster->tmpabilities.damroll = host->tmpabilities.damroll;

    /* caster->points.stun = host->points.stun; */
    caster->points.stun = 1;    /* Just barely awake */
    caster->points.max_stun = host->points.max_stun;
    /* caster->points.hit = host->points.hit; */
    caster->points.hit = 1;     /* Just barely alive */
    caster->points.max_hit = host->points.max_hit;
    caster->points.move = host->points.move;
    caster->points.move_bonus = host->points.move_bonus;
    caster->points.in_bank = host->points.in_bank;

    if (caster->short_descr) {
        free(caster->short_descr);
        caster->short_descr = strdup(MSTR(host, short_descr));
    }

    WIPE_LDESC(caster);

    if (caster->description) {
        free(caster->description);
        caster->description = strdup(MSTR(host, description));
    }

    if (caster->player.extkwds) {
        free(caster->player.extkwds);
        if (host->player.extkwds) {     /* animating a former PC */
            sprintf(buf, "%s %s", host->player.extkwds, MSTR(host, name));
            caster->player.extkwds = strdup(buf);
        } else                  /* animating a former NPC */
            caster->player.extkwds = strdup(MSTR(host, name));
    }

    /* Remove their equipment & inventory, this ain't a Hulk transformation */
    for (pos = 0; pos < MAX_WEAR; pos++)
        if (caster->equipment && caster->equipment[pos])
            if (NULL != (tmp_object = unequip_char(caster, pos)))
                obj_to_room(tmp_object, caster->in_room);

    for (tmp_object = caster->carrying; tmp_object; tmp_object = next_obj) {    /* Go through the loop and drop each item */
        next_obj = tmp_object->next_content;
        if (IS_SET(tmp_object->obj_flags.extra_flags, OFL_DISINTIGRATE))
            extract_obj(tmp_object);
        else {
            obj_from_char(tmp_object);
            obj_to_room(tmp_object, caster->in_room);
        }
    }                           /* end drop all loop */

    /* Whatever the body had, they now have in inventory */
    for (obj_object = corpse->contains; obj_object; obj_object = next_obj) {
        next_obj = obj_object->next_content;
        obj_from_obj(obj_object);
        obj_to_char(obj_object, caster);
    }

    /* Remove any old clan affiliations on the caster */
    while (caster->clan && caster->clan->clan) {
        remove_clan(caster, caster->clan->clan);
    }
    caster->player.tribe = 0;

    /* Add any clan affiliations from the host to the restrung caster */
    while (host->clan && host->clan->clan) {
        pClan = new_clan_data();
        pClan->clan = host->clan->clan;
        pClan->flags = host->clan->flags;
        pClan->rank = host->clan->rank;
        add_clan_data(caster, pClan);
        remove_clan(host, host->clan->clan);
    }

    /* Now for the tattoos (2/28/03) */
    tmp = caster->ex_description;
    caster->ex_description = host->ex_description;
    host->ex_description = tmp;

    /* Echo some cool stuff */
    act("You catch a brief image of your former self dissolving into dust.", TRUE, caster, corpse,
        0, TO_CHAR);
    act("$N's eyes suddenly snap open.", FALSE, caster, 0, caster, TO_ROOM);

    /* Clean up */
    extract_char(host);
    extract_obj(corpse);

    return TRUE;

}

/* POSSESS CORPSE: Caster can temporarily (or permanently, at mon)
 * inhabit a corpse.  Requires component.  Nilazi spell.
 * Nilaz morz nikiz.
  */
void
spell_possess_corpse(byte level,        /* level of spell */
                     sh_int magick_type, CHAR_DATA * ch,        /* the caster     */
                     CHAR_DATA * victim,        /* target char    */
                     OBJ_DATA * corpse)
{                               /* corpse item    *//* begin Spell_Posess_Corpse */
    CHAR_DATA *mob = NULL;
    OBJ_DATA *obj_object, *next_obj;
    struct affected_type af;
    char buf[MAX_STRING_LENGTH];
    char e_desc[MAX_STRING_LENGTH];
    char e_account[MAX_STRING_LENGTH];
    int npcnumber = 100, r_num, drain;
    extern int open_char_file(CHAR_DATA *, char *);
    extern int read_char_file(void);

    memset(&af, 0, sizeof(af));

    check_criminal_flag(ch, NULL);

    if (!corpse) {
        gamelog("A NULL object got passed to spell_possess_corpse");
        return;
    }

    if (!ch->desc)
        return;

    if (ch->desc->snoop.snooping || ch->desc->snoop.snoop_by) {
        send_to_char("The magick fizzles and dies.\n\r", ch);
        return;
    }

    if (ch->desc->original && level < 7) {
        send_to_char("You are already switched, and cannot possess a corpse (try mon).\n\r", ch);
        return;
    }

    if ((GET_ITEM_TYPE(corpse) != ITEM_CONTAINER)
        || (!(corpse->obj_flags.value[3]))
        || (strstr(OSTR(corpse, short_descr), "the undead "))) {
        send_to_char("Your magick seems to not affect it.\n\r", ch);
        return;
    }

    sprintf(buf, "You lack the correct component to%spossess that corpse.",
            ((level == 7) ? " permanently " : " "));

    if (!get_componentB
        (ch, SPHERE_NECROMANCY, level, "$p flashes with a bright light before crumbling to dust.",
         "$p crumbles to dust in $n's hand.", buf))
        return;

    if (get_obj_extra_desc_value(corpse, "[PC_info]", e_desc, sizeof(e_desc))) {
        if (isdigit(e_desc[0])) {
            /* It's an NPC, go fetch from the DB */
            npcnumber = atoi(e_desc);
            if ((r_num = real_mobile(npcnumber)) < 0) {
                send_to_char("Mobile: Zombie not found.\n\r", ch);
                return;
            }
            if ((mob = read_mobile(r_num, REAL)) == 0) {
                gamelogf("Couldn't load mobile %d in spell_possess_corpse, in room %d",
                         npcnumber, (ch->in_room ? ch->in_room->number : 0));
                return;
            }
        } else {
            /* It's a PC, go read it in. */
            if (get_obj_extra_desc_value(corpse, "[ACCOUNT_info]", e_account, sizeof(e_account))) {
                global_game_stats.npcs++;
                CREATE(mob, CHAR_DATA, 1);
                clear_char(mob);
                mob->player.info[0] = e_account;
                load_char(e_desc, mob);
                add_to_character_list(mob);
                mob->specials.was_in_room = NULL;
            } else {
                /* Bogus player, get the default mobile */
                if ((r_num = real_mobile(npcnumber)) < 0) {
                    send_to_char("Mobile: Zombie not found.\n\r", ch);
                    return;
                }
                mob = read_mobile(r_num, REAL);
            }
        }
    } else {
        if ((r_num = real_mobile(npcnumber)) < 0) {
            send_to_char("Mobile: Zombie not found.\n\r", ch);
            return;
        }
        if ((mob = read_mobile(r_num, REAL)) == 0) {
            gamelogf("Couldn't load mobile %d in spell_possess_corpse, in room %d",
                     npcnumber, (ch->in_room ? ch->in_room->number : 0));
            return;
        }
    }

    char_to_room(mob, ch->in_room);

    /* If this is a mon-level, go make the possession permanent */
    if ((level == 7) && (!IS_SET(corpse->obj_flags.value[1], CONT_CORPSE_HEAD))) {
        /* Can't permanently possess a head */
        if (permanent_possession(ch, mob, corpse))
            return;
    }

    act("You lean over $p and exhale a cloud of grey smoke.", TRUE, ch, corpse, 0, TO_CHAR);
    act("$n exhales a cloud of grey smoke as $e leans over $p.", TRUE, ch, corpse, 0, TO_ROOM);

    act("Your vision goes dark as you pass out momentarily.", TRUE, ch, corpse, 0, TO_CHAR);
    act("$n's eyes roll up as $e passes out.", TRUE, ch, corpse, 0, TO_ROOM);

    IS_CARRYING_N(mob) = 0;

    for (obj_object = corpse->contains; obj_object; obj_object = next_obj) {
        next_obj = obj_object->next_content;
        obj_from_obj(obj_object);
        obj_to_char(obj_object, mob);
        obj_object->obj_flags.value[2] = get_char_size(mob); 
    }

    if (IS_SET(corpse->obj_flags.value[1], CONT_CORPSE_HEAD)) {
        sprintf(buf, "%s", OSTR(corpse, name));
        mob->name = (char *) strdup(buf);

        sprintf(buf, "%s", OSTR(corpse, short_descr));

        mob->short_descr = (char *) strdup(buf);
        mob->long_descr = (char *) strdup("slowly floats around.\n\r");
    } else if (IS_NPC(mob)) {
        mob->name = strdup(MSTR(mob, name));
        mob->short_descr = strdup(MSTR(mob, short_descr));
        WIPE_LDESC(mob);
        mob->description = strdup(MSTR(mob, description));
    }

    act("$N's eyes suddenly snap open.", FALSE, ch, 0, mob, TO_ROOM);

    af.type = SPELL_POSSESS_CORPSE;
    af.duration = level;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_SUMMONED;
    affect_join(mob, &af, TRUE, FALSE);

    if (IS_SET(corpse->obj_flags.value[1], CONT_CORPSE_HEAD)) {
        af.type = SPELL_LEVITATE;
        af.duration = level;
        af.modifier = 0;
        af.location = CHAR_APPLY_NONE;
        af.bitvector = CHAR_AFF_FLYING;
        affect_join(mob, &af, FALSE, FALSE);
    }

    mob->points.max_hit = dice((level + 1), 8);
    mob->points.hit = dice((level + 1), 8);
    mob->specials.eco = -100;

    // No gaining immortal powers from possessing a corpse
    GET_LEVEL(mob) = 0;

    MUD_SET_BIT(mob->specials.act, CFL_UNDEAD);
    if( !IS_NPC(ch) ) {
        REMOVE_BIT(mob->specials.act, CFL_ISNPC);
        REMOVE_BIT(mob->specials.act, CFL_AGGRESSIVE);
        REMOVE_BIT(mob->specials.act, CFL_WIMPY);
    }
    GET_RACE(mob) = corpse->obj_flags.value[3];
    GET_COND(mob, THIRST) = get_normal_condition(mob, THIRST);
    GET_COND(mob, FULL) = get_normal_condition(mob, FULL);

    drain = level * 2;
    drain = number(1, drain);
    ch->specials.eco = MAX(-100, (ch->specials.eco - drain));
    check_criminal_flag(mob, NULL);

    update_pos(mob);

    free_specials(mob->specials.programs);
    mob->specials.programs = NULL;

    set_char_position(ch, POSITION_SLEEPING);
    set_char_position(mob, POSITION_RESTING);

    extract_obj(corpse);

    if (!ch->desc->original)
        ch->desc->original = ch;
    ch->desc->character = mob;

    mob->desc = ch->desc;
    ch->desc = 0;

    // The mind is leaving the body, no psionic affects should remain on the
    // body at this point.
    cease_all_psionics(ch, FALSE);

    /* Also a good spot to move this "corpse" char onto the lists of those who
     * are monitoring the char */
    add_monitoring_for_mobile(ch, mob);
}                               /* end Spell_Possess_Corpse */


/* PSEUDO DEATH: Cast on a corpse, it imposes the caster's features
 * over the corpse's true ones.  They must be of the same race-type,
 * and roughly of the same size.  Nilaz spell.  Nilaz iluth echri. 
 */
void
spell_pseudo_death(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Pseudo_Death */
    char buf[MAX_INPUT_LENGTH];
    char *f;
    char corpse_name[MAX_STRING_LENGTH] = "body corpse ";
    int diff;

    check_criminal_flag(ch, NULL);

    if (!victim || !obj) {
        send_to_char("What should the spell be cast upon?\n\r", ch);
        send_to_char("  Syntax: cast 'spell' <target> <obj>\n\r", ch);
        return;
    }

    if (!victim) {
        gamelog("No victim in pseudo death");
        return;
    }

    if (!IS_CORPSE(obj)) {
        act("$p is not a fresh corpse.", FALSE, ch, obj, ch, TO_CHAR);
        return;
    }

    if (race[obj->obj_flags.value[3]].race_type != GET_RACE_TYPE(victim)) {
        act("Your spell fails, destroying $p in the process.", FALSE, ch, obj, victim, TO_CHAR);
        act("$n makes an arcane gesture towards $p, and it disintegrates.", FALSE, ch, obj, 0,
            TO_ROOM);
        extract_obj(obj);
        return;
    }

    diff = abs(get_char_size(ch) - obj->obj_flags.value[5]);
    if (diff > (level * 2)) {
        act("Your magick sinks into $p, but is unable to alter it.", FALSE, ch, obj, victim,
            TO_CHAR);
        return;
    }

    act("You lean over $p in concentration.", FALSE, ch, obj, victim, TO_CHAR);
    act("$n leans over $p in concentration.", FALSE, ch, obj, victim, TO_ROOM);

    if (ch != victim)
        act("The appearance of $p slowly takes on $N's characteristics.", FALSE, ch, obj, victim,
            TO_CHAR);
    else
        act("The appearance of $p slowly takes on your characteristics.", FALSE, ch, obj, victim,
            TO_CHAR);
    act("The appearance of $p slowly takes on $N's characteristics.", FALSE, ch, obj, victim,
        TO_NOTVICT);
    act("The appearance of $p slowly takes on your characteristics.", FALSE, ch, obj, victim,
        TO_VICT);
    if (IS_NPC(victim))
        strcat(corpse_name, MSTR(victim, name));
    else {
        strcat(corpse_name, victim->player.extkwds);
        strcat(corpse_name, " ");
        strcat(corpse_name, MSTR(victim, name));
    }

    if (obj->name)
       free(obj->name);
    f = strdup(corpse_name);
    obj->name = f;

    sprintf(buf, "The body of %s lies crumpled here.", MSTR(victim, short_descr));
    if (obj->long_descr)
       free(obj->long_descr);
    obj->long_descr = strdup(buf);

    sprintf(buf, "the body of %s", MSTR(victim, short_descr));
    if (obj->short_descr)
       free(obj->short_descr);
    obj->short_descr = strdup(buf);

    if (MSTR(victim, description )) {
        if (obj->description)
           free(obj->description);
        obj->description = strdup( MSTR(victim, description));
    }
}                               /* end Spell_Pseudo_Death */

/* PSIONIC_DRAIN: Casts a void sphere around the target, which will not
 * affect void elementals or Nilazi.  Stun damage is depedent on level.
 * Nilazi spell.  nilaz divan echri.
 */
void
spell_psionic_drain(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Psionic_Drain */
    int dam, survived;
    char buf[MAX_STRING_LENGTH];

    if (!ch) {
        gamelog("No ch in psionic drain");
        return;
    }

    if (!victim) {
        gamelog("No victim in psionic drain");
        return;
    }

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* Converted in ws_damage to apply dam mostly to stun and not hp */
    dam = 8 * level;

    if ((GET_GUILD(victim) == GUILD_VOID_ELEMENTALIST)
        || ((GET_RACE(victim) == RACE_ELEMENTAL)
            && (GET_GUILD(victim) == GUILD_VOID_ELEMENTALIST))) {
        /* Heal the mind */
        adjust_stun(victim, dam);

        /* And a small bit of the body (10%) */
        adjust_hit(victim, (dam / 10));
        update_pos(victim);

        act("An intense chill passes over you, and you feel rejuvenated.", FALSE, ch, 0, victim,
            TO_VICT);
        act("You summon a cold black sphere around $S head.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n summons a cold black sphere around $S head.", FALSE, ch, 0, victim, TO_ROOM);
        act("The sphere is absorbed into $S body.", FALSE, ch, 0, victim, TO_NOTVICT);
        act("The sphere is absorbed into $S body.", FALSE, ch, 0, victim, TO_CHAR);
    } else {
        if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5))))
            dam /= 2;

        if (affected_by_spell(victim, PSI_BARRIER)) {
            affect_from_char(victim, PSI_BARRIER);
            send_to_char("Your mind spins for a moment as your psionic barrier is crushed.\n\r", victim);
            dam += level;
        }

        survived = ws_damage(ch, victim, dam/10, 0, 0, dam, SPELL_PSIONIC_DRAIN, TYPE_ELEM_VOID);

        if (survived && IS_NPC(victim) && !victim->specials.fighting) {
            find_ch_keyword(ch, victim, buf, sizeof(buf));
            cmd_kill(victim, buf, CMD_KILL, 0);
        }
    }
}                               /* end Spell_Psionic_Drain */


/* PSIONIC SUPPRESSION: Removes psionic affects from the target
 * and renders them unable to use psionic abilities, for a duration
 * dependent on power level.  Non cumulative.  Provides immunity to
 * some spells, such as FEEBLEMIND and CHARM PERSON.  Nilazi spell.
 * Nilaz psiak chran.
 * Note: this will need to be looked at and updated when the wild
 * talents stuff goes in.
 */
void
spell_psionic_suppression(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                          OBJ_DATA * obj)
{                               /* begin Spell_Psionic_Suppression */
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (affected_by_spell(victim, SPELL_PSI_SUPPRESSION)) {
        act("$N is already unable to use the Way.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (victim != ch) {
        if (!does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
            act("$n glares at you and you feel less in tune with your mind.", FALSE, ch, 0, victim,
                TO_VICT);
            act("You place a block on $N's mind.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n glares at $N, sending a dark aura around $M.", TRUE, ch, 0, victim, TO_NOTVICT);
        } else {
            act("$n glares at you, but nothing seems to happen.", FALSE, ch, 0, victim, TO_VICT);
            act("You glare at $N, but nothing seems to happen.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n glares at $N, but nothing seems to happen.", TRUE, ch, 0, victim, TO_NOTVICT);
            return;
        }
    } else {
        act("You place a block on your mind.", FALSE, ch, 0, 0, TO_CHAR);
    }

    /* following stubbed out to account for new 'cease' command - Halaster 12/15/2005
     * if (IS_AFFECTED(victim, CHAR_AFF_PSI)) {
     * if (affected_by_spell(victim, PSI_COMPREHEND_LANGS)) {
     * affect_from_char(victim, PSI_COMPREHEND_LANGS);
     * send_to_char("Your psychic awareness is shattered!\n\r", victim);
     * }
     * if (affected_by_spell(victim, PSI_BARRIER)) {
     * affect_from_char(victim, PSI_BARRIER);
     * send_to_char("Your psychic barrier is shattered!\n\r", victim);
     * }
     * if (affected_by_spell(victim, PSI_CONTACT)) {
     * affect_from_char(victim, PSI_CONTACT);
     * send_to_char("Your psychic link is shattered!\n\r", victim);
     * }
     * if (affected_by_spell(victim, PSI_HEAR)) {
     * affect_from_char(victim, PSI_HEAR);
     * send_to_char("Your awareness of the Way fades.\n\r", victim);
     * remove_listener(victim);
     * }
     * remove_contact(victim);
     * } */

    parse_command(victim, "cease");
    remove_contact(victim);

    af.type = SPELL_PSI_SUPPRESSION;
    af.duration = level * 2;
    af.power = level;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_NONE;
    af.modifier = 0;
    af.bitvector = CHAR_AFF_PSI;

    affect_to_char(victim, &af);
}                               /* end Spell_Psionic_Suppression */


/* PYROTECHNICS: Allows the caster to create an amusing and
 * sometimes alarming illusion.  Will not work in SECT_NILAZ_PLANE.  Has
 * a greater effect on the plane of Fire, but a weaker effect on the planes
 * of Water and Shadow.  Krathi spell.  
 * Suk-Krath iluth nikiz.
 */
void
spell_pyrotechnics(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Pyrotechnics */
    int effect = number(1, 8);
    char msg1[256], msg2[256];
    char fleemessage[MAX_STRING_LENGTH];
    CHAR_DATA *curr_ch, *next_ch;

    check_crim_cast(ch);

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER)) {
        send_to_char("Your spell fizzles uselessly.\n\r", ch);
        return;
    }

    if (ch->in_room->sector_type == SECT_FIRE_PLANE) {
        send_to_char("Your magick flares more powerfully.\n\r", ch);
        level = level + 2;
        if (level >= 7)
            level = 7;
    }

    if ((ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)) {
        send_to_char("Your magick seems weaker than expected, here.\n\r", ch);
        level = level - 1;
        if (level <= 1)
            level = 1;
    }

    if (level == 7) {
        switch (effect) {
        case 1:
            strcpy(msg1, "A shower of fire cascades from the sky.");
            strcpy(msg2, "A shower of fire cascades from the sky.");
            break;
        case 2:
            strcpy(msg1, "A huge ball of fire roars into existence, called forth by $n.");
            strcpy(msg2, "A huge ball of fire roars into existence, as you call it forth.");
            break;
        case 3:
            strcpy(msg1, "$n's form wavers, becoming a fiery skeleton.");
            strcpy(msg2, "Your form wavers, becoming a fiery skeleton.");
            break;
        case 4:
            strcpy(msg1, "A fiery dragon streaks across the sky, roaring as it swoops downward.");
            strcpy(msg2, "A fiery dragon streaks across the sky, roaring as it swoops downward.");
            break;
        case 5:
            strcpy(msg1, "Blinding sheets of white fire rip through the air, swirling around $n.");
            strcpy(msg2, "Blinding sheets of white fire rip through the air, swirling around you.");
            break;
        case 6:
            strcpy(msg1, "An immense fiery skull appears in the air, its jaws gaping menacingly.");
            strcpy(msg2, "An immense fiery skull appears in the air, its jaws gaping menacingly.");
            break;
        case 7:
            strcpy(msg1,
                   "Tendrils of flame lash out from the ground, striking at their surroundings.");
            strcpy(msg2,
                   "Tendrils of flame lash out from the ground, striking at their surroundings.");
            break;
        case 8:
            strcpy(msg1, "Daggers of fire rain through the air in a deadly storm.");
            strcpy(msg2, "Daggers of fire rain through the air in a deadly storm.");
            break;
        default:
            strcpy(msg1, "A shower of fire cascades from the sky.");
            strcpy(msg2, "A shower of fire cascades from the sky.");
            break;
        }
    } else if (level == 6) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "A circle of fire leaps into existence around $n, surrounding $s body with writhing flames.");
            strcpy(msg2,
                   "A circle of fire leaps into existence around you, surrounding your body with writhing flames.");
            break;
        case 2:
            strcpy(msg1, "A blast of purple fire bursts from the ground in front of $n.");
            strcpy(msg2, "A blast of purple fire bursts from the ground in front of you.");
            break;
        case 3:
            strcpy(msg1, "A giant fireball jumps into the sky, coming down in a rain of ash.");
            strcpy(msg2, "A giant fireball jumps into the sky, coming down in a rain of ash.");
            break;
        case 4:
            strcpy(msg1, "A vast ring of fire surrounds $n, springing from the ground.");
            strcpy(msg2, "A vast ring of fire surrounds you, springing from the ground.");
            break;
        case 5:
            strcpy(msg1,
                   "A pillar of furious flames bursts into the air, raging for seconds then vanishing.");
            strcpy(msg2,
                   "A pillar of furious flames bursts into the air, raging for seconds then vanishing.");
            break;
        case 6:
            strcpy(msg1, "A column of lurid, flickering flames envelops $n's form.");
            strcpy(msg2, "A column of lurid, flickering flames envelops your form.");
            break;
        case 7:
            strcpy(msg1, "Immense fiery shapes erupt into existence, dancing through the air.");
            strcpy(msg2, "Immense fiery shapes erupt into existence, dancing through the air.");
            break;
        case 8:
            strcpy(msg1, "A sudden blaze of fire erupts around $n's form.");
            strcpy(msg2, "A sudden blaze of fire erupts around your form.");
            break;
        default:
            strcpy(msg1, "A giant fireball jumps into the sky, coming down in a rain of ash.");
            strcpy(msg2, "A giant fireball jumps into the sky, coming down in a rain of ash.");
            break;
        }
    } else if (level == 5) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Fiery phantasms dance through the air around $n.");
            strcpy(msg2, "Fiery phantasms dance through the air around you.");
            break;
        case 2:
            strcpy(msg1,
                   "Blazing, fiery red filaments burst from $n's eyes, filling the air with crackling tendrils of flame.");
            strcpy(msg2,
                   "Blazing, fiery red filaments burst from your eyes, filling the air with crackling tendrils of flame.");
            break;
        case 3:
            strcpy(msg1, "Giant wings of fire rise behind $n, fanning out into the air.");
            strcpy(msg2, "Giant wings of fire rise behind you, fanning out into the air.");
            break;
        case 4:
            strcpy(msg1, "Seething diamonds of fire flash and blaze in the air above $n.");
            strcpy(msg2, "Seething diamonds of fire flash and blaze in the air above you.");
            break;
        case 5:
            strcpy(msg1, "Fiery apparitions form a circle around $n, screaming silently.");
            strcpy(msg2, "Fiery apparitions form a circle around you, screaming silently.");
            break;
        case 6:
            strcpy(msg1, "Flames rush outward in a blazing circle around $n's feet.");
            strcpy(msg2, "Flames rush outward in a blazing circle around your feet.");
            break;
        case 7:
            strcpy(msg1, "$n opens $s mouth, breathing out an immense gust of flame.");
            strcpy(msg2, "You open your mouth, breathing out an immense gust of flame.");
            break;
        case 8:
            strcpy(msg1,
                   "A fiery mist surrounds $n, yellow and orange flames dancing in its depths.");
            strcpy(msg2,
                   "A fiery mist surrounds you, yellow and orange flames dancing in its depths.");
            break;
        default:
            strcpy(msg1, "Fiery phantasms dance through the air around $n.");
            strcpy(msg2, "Fiery phantasms dance through the air around you.");
            break;
        }
    } else if (level == 4) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "Shimmering weaves of incandescent flames spread out from $n's hand, blazing before $m.");
            strcpy(msg2,
                   "Shimmering weaves of incandescent flames spread out from your hand, blazing before you.");
            break;
        case 2:
            strcpy(msg1, "Flames rage forth from the earth, encircling $n.");
            strcpy(msg2, "Flames rage forth from the earth, encircling you.");
            break;
        case 3:
            strcpy(msg1, "A fiery maelstrom of blazing energy rips through the air.");
            strcpy(msg2, "A fiery maelstrom of blazing energy rips through the air.");
            break;
        case 4:
            strcpy(msg1,
                   "A lattice of crackling tendrils ignites in the air, forging a chaotic pattern of fire.");
            strcpy(msg2,
                   "A lattice of crackling tendrils ignites in the air, forging a chaotic pattern of fire.");
            break;
        case 5:
            strcpy(msg1, "Fiery sparks rage in a blazing maelstrom of reds and orange.");
            strcpy(msg2, "Fiery sparks rage in a blazing maelstrom of reds and orange.");
            break;
        case 6:
            strcpy(msg1, "A wavering chain bursts into flame before $n, taking shape as a circle.");
            strcpy(msg2,
                   "A wavering chain bursts into flame before you, taking shape as a circle.");
            break;
        case 7:
            strcpy(msg1, "Green and yellow flames slowly form around $n, obscuring $s face.");
            strcpy(msg2, "Green and yellow flames slowly form around you, obscuring your face.");
            break;
        case 8:
            strcpy(msg1, "Flames rage forth from the ground, igniting whatever they touch.");
            strcpy(msg2, "Flames rage forth from the ground, igniting whatever they touch.");
            break;
        default:
            strcpy(msg1, "A flaming bolt bursts from the ground in front of $n.");
            strcpy(msg2, "A flaming bolt bursts from the ground in front of you.");
            break;
        }
    } else if (level == 3) {
        switch (effect) {
        case 1:
            strcpy(msg1, "A brilliant flare of fire surrounds $n's form.");
            strcpy(msg2, "A brilliant flare of fire surrounds your form.");
            break;
        case 2:
            strcpy(msg1, "A fiery aura engulfs $n's body, blazing furiously before fading.");
            strcpy(msg2, "A fiery aura engulfs your body, blazing furiously before fading.");
            break;
        case 3:
            strcpy(msg1, "A gash of fire rips through the ground, blazing furiously.");
            strcpy(msg2, "A gash of fire rips through the ground, blazing furiously.");
            break;
        case 4:
            strcpy(msg1, "Balls of flame surround $n in a chaotic dance.");
            strcpy(msg2, "Balls of flame surround you in a chaotic dance.");
            break;
        case 5:
            strcpy(msg1, "A large flame rages into existence before $n, crackling with power.");
            strcpy(msg2, "A large flame rages into existence before you, crackling with power.");
            break;
        case 6:
            strcpy(msg1, "Luminous sparkles surround $n, obscuring $m in a glowing mist.");
            strcpy(msg2, "Luminous sparkles surround you in a glowing mist.");
            break;
        case 7:
            strcpy(msg1, "An incandescent pearl of flame materializes, spinning slowly.");
            strcpy(msg2, "An incandescent pearl of flame materializes, spinning slowly.");
            break;
        case 8:
            strcpy(msg1, "Sinuous weaves of fire burst into life around $n, twisting with life.");
            strcpy(msg2, "Sinuous weaves of fire burst into life around you, twisting with life.");
            break;
        default:
            strcpy(msg1, "Sparks erupt from $n's fingers and jump around $m.");
            strcpy(msg2, "Sparks erupt from your finger and jump around you.");
            break;
        }
    } else if (level == 2) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Multi-colored sparks begin to whirl around $n's body.");
            strcpy(msg2, "Multi-colored sparks begin to whirl around your body.");
            break;
        case 2:
            strcpy(msg1, "Sparks of brilliant fire streak from $n's hands into the air.");
            strcpy(msg2, "Sparks of brilliant fire streak from your hands into the air.");
            break;
        case 3:
            strcpy(msg1, "Small glowing tendrils crackle in the air, glowing brilliant red.");
            strcpy(msg2, "Small glowing tendrils crackle in the air, glowing brilliant red.");
            break;
        case 4:
            strcpy(msg1,
                   "Flickering strands of flame flit about, dancing chaotically and vanishing.");
            strcpy(msg2,
                   "Flickering strands of flame flit about, dancing chaotically and vanishing.");
            break;
        case 5:
            strcpy(msg1, "A fiery tendril of light forms in the air, glowing fiercely.");
            strcpy(msg2, "A fiery tendril of light forms in the air, glowing fiercely.");
            break;
        case 6:
            strcpy(msg1, "Glittering sparks rage in a miniature fiery storm above the ground.");
            strcpy(msg2, "Glittering sparks rage in a miniature fiery storm above the ground.");
            break;
        case 7:
            strcpy(msg1, "A burning band of fire appears around $n, illumining $s form.");
            strcpy(msg2, "A burning band of fire appears around you, illumining your form.");
            break;
        case 8:
            strcpy(msg1, "A cloud of fiery dust materializes, seeping outwards as it fades.");
            strcpy(msg2, "A cloud of fiery dust materializes, seeping outwards as it fades.");
            break;
        default:
            strcpy(msg1, "Sparks erupt from $n's fingers and jump around $m.");
            strcpy(msg2, "Sparks erupt from your finger and jump around you.");
            break;
        }
    } else {
        switch (effect) {
        case 1:
            strcpy(msg1, "Sparks erupt from $n's fingers and jump around $m.");
            strcpy(msg2, "Sparks erupt from your finger and jump around you.");
            break;
        case 2:
            strcpy(msg1, "A small fiery glow surrounds $n's finger for a moment.");
            strcpy(msg2, "A small fiery glow surrounds your finger for a moment.");
            break;
        case 3:
            strcpy(msg1, "A reddish glow begins to form around $n, outlining $s body.");
            strcpy(msg2, "A reddish glow begins to form around you, outlining your body.");
            break;
        case 4:
            strcpy(msg1, "Motes of light rise from the ground to glow dimly in the air.");
            strcpy(msg2, "Motes of light rise from the ground to glow dimly in the air.");
            break;
        case 5:
            strcpy(msg1, "A faint orb of fire rises from $n's hand, slowly dissipating.");
            strcpy(msg2, "A faint orb of fire rises from your hand, slowly dissipating.");
            break;
        case 6:
            strcpy(msg1, "A rainbow of sparks dances back and forth between $n's hands.");
            strcpy(msg2, "A rainbow of sparks dances back and forth between your hands.");
            break;
        case 7:
            strcpy(msg1, "A single flickering flame appears before $n.");
            strcpy(msg2, "A single flickering flame appears before you.");
            break;
        case 8:
            strcpy(msg1, "Shimmering flecks of light surround $n in a patchwork of fire.");
            strcpy(msg2, "Shimmering flecks of light surround you in a patchwork of fire.");
            break;
        default:
            strcpy(msg1, "Sparks erupt from $n's fingers and jump around $m.");
            strcpy(msg2, "Sparks erupt from your finger and jump around you.");
            break;
        }
    }

    act(msg1, FALSE, ch, 0, 0, TO_ROOM);
    act(msg2, FALSE, ch, 0, 0, TO_CHAR);

    strcpy(fleemessage, "flee self");

    if (level >= 6) {
        for (curr_ch = ch->in_room->people; curr_ch; curr_ch = next_ch) {
            next_ch = curr_ch->next_in_room;
            if (IS_SET(curr_ch->specials.act, CFL_MOUNT)) {
                if (number(0, 1))
                    parse_command(curr_ch, fleemessage);
            }
        }
    }
}                               /* end Spell_Pyrotechnics */


/* QUICKENING: Increases speed, agility and movement of the caster
 * for a duration dependent on power level.  Non-cumulative.
 * Nullifies the effects of the SLOW and LEVITATE spells.
 * Elkros spell.  Elkros viqrol hekro.
 */
void
spell_quickening(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Quickening */
    int sector;
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    check_crim_cast(ch);

    if (affected_by_spell(victim, SPELL_SLOW)) {
        if (victim == ch) {
            act("Your body's slow movements speed up to normal.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n's body speeds back up to normal.", FALSE, ch, 0, 0, TO_ROOM);
        } else {
            act("You make a quick gesture at $N and $S movements speed up to normal.", FALSE, ch, 0,
                victim, TO_CHAR);
            act("$n makes a quick gesture at you and your movements speed up to normal.", FALSE, ch,
                0, victim, TO_VICT);
            act("$n makes a quick gesture at $N and $S movements speed up to normal.", TRUE, ch, 0,
                victim, TO_NOTVICT);
        }
        affect_from_char(victim, SPELL_SLOW);
        return;
    }

    if (victim == ch) {
        act("With a quick gesture, your body speeds up.", FALSE, ch, 0, 0, TO_CHAR);
        act("With a quick gesture, $n's body speeds up.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("You make a quick gesture at $N and $S body speeds up.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n makes a quick gesture at you and your body speeds up.", FALSE, ch, 0, victim,
            TO_VICT);
        act("$n makes a quick gesture at $N and $S body speeds up.", TRUE, ch, 0, victim,
            TO_NOTVICT);
    }

    sector = ch->in_room->sector_type;

    switch (sector) {
    case SECT_ROAD:
        send_to_char("You feel the energies of the road moving through you.\n\r", victim);
        duration = level * 2;
        break;
    default:
        duration = level;
        break;
    }

    af.type = SPELL_QUICKENING;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    /* Added MAX() to make sure wek doesn't end up with 0 or negative modifier
     * -Morg 6/17/02
     */
    af.modifier = MAX(level, number(level, ((level * level) / 3)));
    af.location = CHAR_APPLY_AGL;
    af.bitvector = CHAR_AFF_GODSPEED;
    stack_spell_affect(victim, &af, 24);

    if (affected_by_spell(victim, SPELL_LEVITATE)) {
        affect_from_char(victim, SPELL_LEVITATE);
        send_to_char("Your body floats to the ground.\n\r", victim);
        act("$n's body floats to the ground.", FALSE, victim, 0, 0, TO_ROOM);
    }
}                               /* end Spell_Quickening */


/* Called by RAIN OF FIRE (rain_of_fire).
 * Each person damaged gets passed here.  It looks at items they're
 * wearings and burns those made of cloth.  It also looks at their
 * inventory and burns items that are flammable.
 */
void
burn_inv_eq(CHAR_DATA * ch)
{
    int i, damaged_eq, damaged_inv;
    damaged_eq = 0;
    damaged_inv = 0;

    for (i = 0; i < MAX_WEAR; i++) {
        if (ch->equipment[i]) {
            damaged_eq = 1;
            if (ch->equipment[i]->obj_flags.material == MATERIAL_CLOTH) {
                if (ch->equipment[i]->obj_flags.type == ITEM_WORN) {
                    sflag_to_obj(ch->equipment[i], OST_BURNED);
                }
                if (ch->equipment[i]->obj_flags.type == ITEM_ARMOR) {
                    /* Damage the armor slightly, 1-2 points */
                }
                if (ch->equipment[i]->obj_flags.type == ITEM_CONTAINER) {
                    /* Destroy the container, dump the contents on the ground */
                }
            }
            /* Other material types go here */
        }
    }
}

/* RAIN OF FIRE: Area attack spell. Has no affect on the planes of Nilaz, Water,
 * or Shadow.
 * Krathi and sorcerer spell.  Suk-Krath divan echri.
 */

void
spell_rain_of_fire(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Rain_Of_Fire */
    char buf[256];

    int dam, alive;
    ROOM_DATA *other_room;
    CHAR_DATA *tch, *next_tch;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in rain of fire");
        return;
    }

    if (!ch->in_room) {
        gamelog("No room in rain of fire");
        return;
    }

    if (((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))
        || (ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)) {
        send_to_char("You are uanble to touch Suk-Krath to cause a rain of fire here.\n\r", ch);
        return;
    }

    /* Initialize pointer */
    other_room = NULL;

    /* Determine damage */
    dam = dice(2, 8) + (5 * level);

    /* Apply crim flag where needed */
    check_criminal_flag(ch, NULL);

    /* Send the messages */
    act("$n makes some arcane gestures, and raises $s arms to the "
        "sky, calling down a rain of fire.", FALSE, ch, 0, 0, TO_ROOM);
    act("You raise your arms overhead, and fire rains down " "from the sky.", FALSE, ch, 0, ch,
        TO_CHAR);

    for (tch = ch->in_room->people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;

        if (ch != tch) {
            if ((!IS_IMMORTAL(tch))
                && (!affected_by_spell(tch, SPELL_ETHEREAL))
                && (!affected_by_spell(tch, SPELL_FIRE_ARMOR))
                && (GET_RACE(tch) != RACE_SHADOW)) {
                WAIT_STATE(tch, number(1, 2));
                ws_damage(ch, tch, dam, 0, 0, dam, SPELL_RAIN_OF_FIRE, TYPE_ELEM_FIRE);
                burn_inv_eq(tch);
                stop_all_fighting(tch);
                stop_all_fighting(ch);
                if (IS_NPC(tch))
                    if (!does_hate(tch, ch))
                        add_to_hates_raw(tch, ch);

		if (number(1, 100) <= ((80 * level) / (LAST_POWER + 1))) {
		    af.type = SPELL_IMMOLATE;
		    af.duration = MAX(1, level / 2);
		    af.power = MAX(1, level / 2);
		    af.magick_type = magick_type;
		    af.modifier = 0;
		    af.location = CHAR_APPLY_NONE;
		    af.bitvector = CHAR_AFF_DAMAGE;
		    affect_join(tch, &af, FALSE, FALSE);
		}
            }                   /*  End mortal  */
        } else {                /*  tch is ch  */
            if ((!IS_IMMORTAL(tch))
                && (!affected_by_spell(tch, SPELL_ETHEREAL))
                && (!affected_by_spell(tch, SPELL_FIRE_ARMOR))
                && (GET_RACE(tch) != RACE_SHADOW)) {
                alive = generic_damage(tch, (dam / 3), 0, 0, (dam / 3));
                burn_inv_eq(tch);
                WAIT_STATE(tch, number(1, 2));
            }
        }
    }                           /*  End for on room  */

    /*  Now check for down exit  */
    if CAN_GO
        (ch, DIR_DOWN) {
        if ((other_room = EXIT(ch, DIR_DOWN)->to_room)) {
            for (tch = other_room->people; tch; tch = next_tch) {
                next_tch = tch->next_in_room;

                sprintf(buf, "The sky suddenly unleashes a downpour " "of fiery rain.\n\r");
                send_to_room(buf, EXIT(ch, DIR_DOWN)->to_room);

                if (!IS_IMMORTAL(tch)) {
                    ws_damage(ch, tch, dam, 0, 0, dam, SPELL_RAIN_OF_FIRE, TYPE_ELEM_FIRE);
                    WAIT_STATE(tch, number(1, 2));
                    burn_inv_eq(tch);
                }               /*  End mortal  */
            }                   /*  End for on room  */
        }                       /*  End if other_room  */
        }

/*    for (tch = character_list; tch; tch = next_tch) {
        next_tch = tch->next;

        if (tch->in_room && (tch->in_room != ch->in_room) && (tch->in_room != other_room)
            && (tch->in_room->zone == ch->in_room->zone))
            act("Not too far away, fire can be seen raining down out of the sky.", FALSE, ch, 0,
                tch, TO_NEARBY_ROOMS);
    }*/
    act("Not too far away, fire can be seen raining down out of the sky.", FALSE, ch, 0, tch,
        TO_NEARBY_ROOMS);
}                               /* end Spell_Rain_Of_Fire */


/* REFRESH: Refreshes target's stamina, in an amount
 * porportional to the power level.  Elkros spell.
 * Note: help file says it also lessens hunger and thirst,
 * but this is not currently coded.  Elkros viqrol wril.
 */
void
spell_refresh(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Refresh */
    int dam;

    if (!ch) {
        gamelog("No ch in refresh");
        return;
    }

    if (!victim) {
        gamelog("No victim in refresh");
        return;
    }

    check_crim_cast(ch);

    dam = dice(8, 4) * level;
    dam = MAX(dam, 20);

    adjust_move(victim, dam);

    gain_condition(victim, FULL, 2 * level);
    gain_condition(victim, THIRST, 2 * level);

    if (victim == ch) {
        act("You feel less tired.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n suddenly looks less tired.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("A pale green energy flows from your hand into $N, and $E looks less tired.", FALSE, ch,
            0, victim, TO_CHAR);
        act("A pale green energy flows from $n's hand into you, and you feel less tired.", FALSE,
            ch, 0, victim, TO_VICT);
        act("A pale green energy flows from $n's hand into $N, and $E looks less tired.", TRUE, ch,
            0, victim, TO_NOTVICT);
    }
}                               /* end Spell_Refresh */


/* REGENERATE: Allows target to heal wounds faster.
 * Duration dependent on power level.  Non cumulative.
 * Elkros spell.  Elkros divan wril.
 */
void
spell_regenerate(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Regenerate */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in regenerate");
        return;
    }

    check_crim_cast(ch);

    duration = level + number(0, 2);

    if (affected_by_spell(victim, SPELL_REGENERATE)) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    if (victim == ch) {
        act("A harmless lightning swirls around you, and you feel your metabolism speed up.", FALSE,
            ch, 0, 0, TO_CHAR);
        act("A harmless lightning swirls around $n briefly.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("You send a harmless lightning swirling around $N.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n sends a harmless lightning swirling around you, and you feel your metabolism speed up.", FALSE, ch, 0, victim, TO_VICT);
        act("$n sends a harmless lightning swirling around $N.", TRUE, ch, 0, victim, TO_NOTVICT);
    }

    af.type = SPELL_REGENERATE;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

}                               /* end Spell_Regenerate */


/* RELOCATE: Allows caster to transfer themself to a target
 * character.  Requires component. Will not work if target is in a room that is
 * flagged NO_MAGICK, IMMORTAL, NO_ELEMENTAL_MAGICK, PRIVATE,
 * or INDOORS.  Cannot relocate to someone on a different Plane (i.e. if you are
 * on the Plane of Water, you can only relocate to others on the Plane of Water.
 * Conversley, if you are not on the Plane of Fire, you cannot relocate to someone
 * who is).  Whiran and Sorcerer spell.  
 * Whira locror viod. */
void
spell_relocate(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Relocate */
  ROOM_DATA *room, *was_in;
  int dist;
  
  if (!ch) {
    gamelog("No ch in relocate");
    return;
  }
  
  if (!victim) {
    gamelog("No victim in relocate");
    return;
  }
  
  check_crim_cast(ch);
  
  if ((!zone_table[victim->in_room->zone].is_open)
      || IS_SET(victim->in_room->room_flags, RFL_NO_MAGICK)
      || IS_SET(victim->in_room->room_flags, RFL_IMMORTAL_ROOM)
      || IS_SET(victim->in_room->room_flags, RFL_PRIVATE)
      || IS_SET(victim->in_room->room_flags, RFL_NO_ELEM_MAGICK)
      || IS_SET(victim->in_room->room_flags, RFL_INDOORS)) {
    send_to_char("You failed.\n\r", ch);
    return;
  }
  
  room = victim->in_room;
  was_in = ch->in_room;
  
  if (!is_on_same_plane(ch->in_room, victim->in_room)) {
    send_to_char("You cannot relocate to someone on another plane.\n\r", ch);
    return;
  }

  // Trying to relocate out of Allanak or to someone in Allanak sends you to arena
  // if you are a criminal.
  if (allanak_magic_barrier(ch, room)) {
    if (IS_SET(ch->specials.act, CFL_CRIM_ALLANAK)) {
      qgamelogf(QUIET_MISC,
                "allanak_magic_barrier: %s (criminal) tried to relocate in #%d, sent to arena.",
                MSTR(ch, name),
                ch->in_room->number);
      room = get_room_num(50664);
    } else {
      qgamelogf(QUIET_MISC,
                "allanak_magic_barrier: %s tried to relocate in #%d, blocked.",
                MSTR(ch, name),
                ch->in_room->number);
      send_to_char("Nothing happens.\n\r", ch);
      return;
    }
  }

  if (tuluk_magic_barrier(ch, room)) {
      qgamelogf(QUIET_MISC,
                "tuluk_magic_barrier: %s tried to relocate in #%d, blocked.",
                MSTR(ch, name),
                ch->in_room->number);
    send_to_char("Nothing happens.\n\r", ch);
    return;
  }
  
  if (!no_specials && special(ch, CMD_FROM_ROOM, "relocate")) {
    return;
  }
  
  dist = find_dist_for(ch->in_room, victim->in_room, ch);
  
  if ((level == 7) || ((dist != -1) && (dist < (level * 20)))) {
    
    /* Component check moved here so that it only uses it when successful - Halaster */
    if (!get_componentB
        (ch, SPHERE_TELEPORTATION, level, "$p is consumed by the magick.",
         "$p disappears from $n's hands.", "You don't have the necessary component."))
      return;
    
    send_to_char("Your stomach lurches as you are swept into the air by violent winds.\n\r",
                 ch);
    act("Violent winds encircle $n, and sweep $m away.", FALSE, ch, 0, victim, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, room);
    
    if (!no_specials && special(ch, CMD_TO_ROOM, "relocate")) {
      char_from_room(ch);
      char_to_room(ch, was_in);
      return;
    }
    
    act("$n descends on a gust of billowing wind.", TRUE, ch, 0, victim, TO_ROOM);
    cmd_look(ch, "", 15, 0);
  }
  
  else
    send_to_char("The spell fizzles and dies.\n\r", ch);
}                               /* end Spell_Relocate */


/* REMOVE CURSE: Removes the curse spell from objects or
 * people, irrespective of strength/duration (although the
 * help spell indicates differently.  Drovian spell.
 * Drov divan wril. 
 */
void
spell_remove_curse(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Remove_Curse */
    assert(ch && (victim || obj));

    if (obj) {
        if (IS_SET(obj->obj_flags.extra_flags, OFL_EVIL)
            || IS_SET(obj->obj_flags.extra_flags, OFL_NO_DROP)) {
            act("$p briefly glows blue.", TRUE, ch, obj, 0, TO_CHAR);

            REMOVE_BIT(obj->obj_flags.extra_flags, OFL_EVIL);
            REMOVE_BIT(obj->obj_flags.extra_flags, OFL_NO_DROP);
        }
        return;
    }

    if (victim && affected_by_spell(victim, SPELL_CURSE)) {
        affect_from_char(victim, SPELL_CURSE);
        if (victim == ch) {
            act("A blue light surrounds you and you feel your curse lifted.", FALSE, ch, 0, 0,
                TO_CHAR);
            act("A blue light surrounds $n briefly.", FALSE, ch, 0, 0, TO_ROOM);
        } else {
            act("You send a blue light around $N, lifting $S curse.", FALSE, ch, 0, victim,
                TO_CHAR);
            act("$n sends a blue light around you, lifting your curse.", FALSE, ch, 0, victim,
                TO_VICT);
            act("$n sends a blue light around $N that quickly fades.", TRUE, ch, 0, victim,
                TO_NOTVICT);
        }
        return;
    }
}                               /* end Spell_Remove_Curse */


/* REPAIR ITEM: Cast on an object, it repairs the item by removing certain
 * sflags and removing damage to armor.  The greater the power level, the
 * more "problems" with the object it will repair/restore.
 * Ruk and Sorcerer spell.
 * ruk mutur chran.
 */
void
spell_repair_item(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Repair Item */

    if (!obj) {
        gamelog("No obj in repair item spell.");
        return;
    }

    check_crim_cast(ch);

    act("$n infuses $p with the power of Ruk, restoring it.", FALSE, ch, obj, 0, TO_ROOM);
    act("You infuse $p with the power of Ruk, restoring it.", FALSE, ch, obj, 0, TO_CHAR);

    if (level >= 1) {
        if (IS_SET(obj->obj_flags.state_flags, OST_DUSTY)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_DUSTY);
            act("The dust on $p fades away into nothing.", TRUE, ch, obj, 0, TO_ROOM);
            act("The dust on $p fades away into nothing.", TRUE, ch, obj, 0, TO_CHAR);
        }
    }
    if (level >= 2) {
        if (IS_SET(obj->obj_flags.state_flags, OST_MUDCAKED)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_MUDCAKED);
            act("The mud on $p evaporates into nothing.", TRUE, ch, obj, 0, TO_ROOM);
            act("The mud on $p evaporates into nothing.", TRUE, ch, obj, 0, TO_CHAR);
        }
    }
    if (level >= 3) {
        if (IS_SET(obj->obj_flags.state_flags, OST_SWEATY)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_SWEATY);
            act("The sweat-stains on $p evaporate and disappear.", TRUE, ch, obj, 0, TO_ROOM);
            act("The sweat-stains on $p evaporate and disappear.", TRUE, ch, obj, 0, TO_CHAR);
        }
    }
    if (level >= 4) {
        if (IS_SET(obj->obj_flags.state_flags, OST_BLOODY)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_BLOODY);
            act("All of the blood-stains on $p dissolve and fade away.", TRUE, ch, obj, 0, TO_ROOM);
            act("All of the blood-stains on $p dissolve and fade away.", TRUE, ch, obj, 0, TO_CHAR);
        }
        if (IS_SET(obj->obj_flags.state_flags, OST_ASH)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_ASH);
            act("The ash on $p is blown away.", TRUE, ch, obj, 0, TO_ROOM);
            act("The ash on $p is blown away.", TRUE, ch, obj, 0, TO_CHAR);
        }
    }
    if (level >= 5) {
        if (IS_SET(obj->obj_flags.state_flags, OST_SEWER)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_SEWER);
            act("The awful smell on $p disappears entirely.", TRUE, ch, obj, 0, TO_ROOM);
            act("The awful smell on $p disappears entirely.", TRUE, ch, obj, 0, TO_CHAR);
        }
        if (obj->obj_flags.type == ITEM_ARMOR && obj->obj_flags.value[0] < obj->obj_flags.value[1]) {
            obj->obj_flags.value[0] = MIN(obj->obj_flags.value[1], obj->obj_flags.value[0] += 5);
            act("Some of the damage to $p is magickally mended.", TRUE, ch, obj, 0, TO_ROOM);
            act("Some of the damage to $p is magickally mended.", TRUE, ch, obj, 0, TO_CHAR);
        }
    }
    if (level >= 6) {
        if (IS_SET(obj->obj_flags.state_flags, OST_STAINED)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_STAINED);
            act("The stains that cover $p dissolve into nothing.", TRUE, ch, obj, 0, TO_ROOM);
            act("The stains that cover $p dissolve into nothing.", TRUE, ch, obj, 0, TO_CHAR);
        }
        if (IS_SET(obj->obj_flags.state_flags, OST_OLD)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_OLD);
            act("$p is restored and looks like new.", TRUE, ch, obj, 0, TO_ROOM);
            act("$p is restored and looks like new.", TRUE, ch, obj, 0, TO_CHAR);
        }
        if (obj->obj_flags.type == ITEM_ARMOR && obj->obj_flags.value[0] < obj->obj_flags.value[1]) {
            obj->obj_flags.value[0] = MIN(obj->obj_flags.value[1], obj->obj_flags.value[0] += 5);
            act("Some of the damage to $p is magickally repaired.", TRUE, ch, obj, 0, TO_ROOM);
            act("Some of the damage to $p is magickally repaired.", TRUE, ch, obj, 0, TO_CHAR);
        }
    }
    if (level >= 7) {
        if (IS_SET(obj->obj_flags.state_flags, OST_BURNED)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_BURNED);
            act("The burns to $p are repaired and removed.", TRUE, ch, obj, 0, TO_ROOM);
            act("The burns to $p are repaired and removed.", TRUE, ch, obj, 0, TO_CHAR);
        }
        if (IS_SET(obj->obj_flags.state_flags, OST_TORN)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_TORN);
            act("The rips and tears on $p are instantly mended.", TRUE, ch, obj, 0, TO_ROOM);
            act("The rips and tears on $p are instantly mended.", TRUE, ch, obj, 0, TO_CHAR);
        }
        if (IS_SET(obj->obj_flags.state_flags, OST_TATTERED)) {
            REMOVE_BIT(obj->obj_flags.state_flags, OST_TATTERED);
            act("The tatters that line $p magickally mend themselves.", TRUE, ch, obj, 0, TO_ROOM);
            act("The tatters that line $p magickally mend themselves.", TRUE, ch, obj, 0, TO_CHAR);
        }
        if (obj->obj_flags.type == ITEM_ARMOR && (obj->obj_flags.value[0] < obj->obj_flags.value[1]
                                                  || obj->obj_flags.value[1] <
                                                  obj->obj_flags.value[3])) {
            /* reset the points to max_points */
            obj->obj_flags.value[1] = obj->obj_flags.value[3];
            /* reset the armor damage to points */
            obj->obj_flags.value[0] = obj->obj_flags.value[1];
            act("All of the damage to $p is repaired making it good as new.", TRUE, ch, obj, 0,
                TO_ROOM);
            act("All of the damage to $p is repaired making it good as new.", TRUE, ch, obj, 0,
                TO_CHAR);
        }
    }
}                               /* end Spell_repair_item */


/* REPEL: Hurls the target out of the room the caster is in,
 * through an open exit.  If they are thrown against a closed
 * exit, they take damage from it.  Whiran spell.
 * Whira locror chran.
 */
void
spell_repel(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Repel */
    char tmp[256];
    int dir = -1, i, j, num_exits = 0;
    ROOM_DATA *room;
    int exitsarray[MAX_DIRECTIONS];

    for(i = 0,j = 0; i < MAX_DIRECTIONS; i++) {
        if (ch->in_room->direction[i]) {
            exitsarray[j] = i;
            j++;
            num_exits++;
        }
    }

    check_criminal_flag(ch, NULL);

    if (num_exits <= 0) {
        send_to_char("Nothing seems to happen.\n\r", ch);
        return;
    }

    dir = exitsarray[number(0, num_exits-1)];

    /* If victim is affected by the Rooted ability, they are protected */
    if (find_ex_description("[ROOTED]", victim->ex_description, TRUE)) {
        act("$N seems completely unaffected by your spell.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    /* saving throw is based 0 at Yuqa */
    if (does_save(victim, SAVING_SPELL, ((level - 2) * (-5)))) {
        send_to_char("Your victim resists your spell.\n\r", ch);
        return;
    }

    if (ch->in_room->sector_type == SECT_AIR)
        level = level + 3;

    if (ch->in_room->sector_type == SECT_AIR_PLANE) {
        send_to_char("The winds surge with extra intensity here!\n\r", ch);
        level = level + 5;
    }

    for (i = 0; i < level; i++) {
        if (!victim)
            return;

        if (!victim->in_room->direction[dir])
            return;

        if (IS_SET(EXIT(victim, dir)->exit_info, EX_CLOSED) && !is_char_ethereal(victim)) {
            if (EXIT(victim, dir)->keyword) {
                sprintf(tmp, "A violent wind slams $2$n$1 against the closed %s.",
                        EXIT(victim, dir)->keyword);
                act(tmp, FALSE, victim, 0, 0, TO_ROOM);
                sprintf(tmp, "A violent wind slams $2you$1 against the closed %s.",
                        EXIT(victim, dir)->keyword);
                act(tmp, FALSE, victim, 0, 0, TO_CHAR);
                do_damage(victim, number(5,10), 10);

                if (!IS_SET(EXIT(victim, dir)->exit_info, EX_LOCKED)) {
                    if (number (1,100) <= get_char_size(victim)) {
                        sprintf(tmp, "The %s flings wide open from the impact of $s weight!",
                                EXIT(victim, dir)->keyword);
                        act(tmp, FALSE, victim, 0, 0, TO_ROOM);
                        sprintf(tmp, "The %s flings wide open from the impact of your weight!",
                                EXIT(victim, dir)->keyword);
                        act(tmp, FALSE, victim, 0, 0, TO_CHAR);
                        REMOVE_BIT(EXIT(victim, dir)->exit_info, EX_CLOSED);
                        do_damage(victim, number(5,10), 10);
                    } else
                        break;
                } else
                    break;

            }
        }

        if (IS_SET(EXIT(victim, dir)->exit_info, EX_SPL_SAND_WALL) && !is_char_ethereal(victim)) {
            act("A violent wind slams $2$n$1 against the wall of sand.", FALSE, victim, 0, 0, TO_ROOM);
            act("A violent wind slams $2you$1 against the wall of sand.", FALSE, victim, 0, 0, TO_CHAR);
            do_damage(victim, number(5,10), 10);
            return;
        }

        if (IS_SET(EXIT(victim, dir)->exit_info, EX_SPL_WIND_WALL) && !is_char_ethereal(victim)) {
            act("A violent wind slams $2$n$1 against the wall of wind.", FALSE, victim, 0, 0, TO_ROOM);
            act("A violent wind slams $2you$1 against the wall of wind.", FALSE, victim, 0, 0, TO_CHAR);
            do_damage(victim, number(5,10), 10);
            return;
        }

        sprintf(tmp, "$2$n$1 is hurled out of the area by a violent wind to the %s!", dirs[dir]);
        act(tmp, FALSE, victim, 0, 0, TO_ROOM);
        sprintf(tmp, "$2You$1 are hurled out of the area by a violent wind to the %s!", dirs[dir]);
        act(tmp, FALSE, victim, 0, 0, TO_CHAR);

        if (!blade_barrier_check(victim, dir))
            return;
        if (!thorn_check(victim, dir))
            return;
        if (!wall_of_fire_check(victim, dir))
            return;

        room = victim->in_room;
        char_from_room(victim);
        char_to_room(victim, room->direction[dir]->to_room);
        cmd_look(victim, "", 0, 0);

        sprintf(tmp, "$2$n$1 is hurled into the area by a violent wind from the %s!", rev_dirs[dir]);
        act(tmp, FALSE, victim, 0, 0, TO_ROOM);
    }

    WAIT_STATE(victim, level / 2);

}                               /* end Spell_Repel */


/* RESTFUL SHADE: Casts a pleasant shade over the area, for a
 * duration dependent on power level.  Will not work on the Plane of Fire
 * or the Plane of Nilaz.  Drovian spell.
 * Drov nathro wril.
 */
void
spell_restful_shade(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Restful_Shade */
    int sector;

    check_crim_cast(ch);

    if (IS_SET(ch->in_room->room_flags, RFL_RESTFUL_SHADE)) {
        act("You are already under a comfortable shade.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (IS_SET(ch->in_room->room_flags, RFL_INDOORS)) {
        act("You find it difficult to create shade indoors.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (IS_SET(ch->in_room->room_flags, RFL_NO_ELEM_MAGICK)) {
        act("You cannot reach your plane to summon the necessary shade.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    sector = ch->in_room->sector_type;

    switch (sector) {

    case SECT_NILAZ_PLANE:
    case SECT_FIRE_PLANE:
        act("You call upon a restful shade, but it cannot take hold here.", FALSE, ch, 0, 0,
            TO_CHAR);
        return;
    case SECT_AIR:
        act("Shadows swirl in the air around $n as a restful shade descends.", FALSE, ch, 0, 0,
            TO_ROOM);
        act("Shadows swirl in the air around you as a restful shade descends.", FALSE, ch, 0, 0,
            TO_CHAR);
        break;
    case SECT_DESERT:
        act("The sands are stirred near $n as a restful shade descends on the area.", FALSE, ch, 0,
            0, TO_ROOM);
        act("The sands are stirred near you as a restful shade descends on the area.", FALSE, ch, 0,
            0, TO_CHAR);
        break;
    case SECT_HEAVY_FOREST:
        act("The shadows between the trees near $n expand as a restful shade descends on the area.",
            FALSE, ch, 0, 0, TO_ROOM);
        act("The shadows between the trees near you expand as a restful shade descends on the area.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_LIGHT_FOREST:
        act("Shadows swirl from beneath the vegetation near $n as a restful shade slowly descends.",
            FALSE, ch, 0, 0, TO_ROOM);
        act("Shadows swirl from beneath the vegetation near you as a restful shade slowly descends.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_RUINS:
        act("Shadows expand outward from the rubble near $n as a restful shade slowly descends.",
            FALSE, ch, 0, 0, TO_ROOM);
        act("Shadows expand outward from the rubble near you as a restful shade slowly descends.",
            FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SECT_SILT:
    case SECT_SHALLOWS:
        act("The silt here is stirred near $n as a restful shade slowly descends.", FALSE, ch, 0, 0,
            TO_ROOM);
        act("The silt here is stirred near you as a restful shade slowly descends.", FALSE, ch, 0,
            0, TO_CHAR);
        break;
    default:
        act("$n closes $s hand and a restful shade descends upon you.", FALSE, ch, 0, 0, TO_ROOM);
        act("You summon a restful shade which descends upon the area.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    }

    MUD_SET_BIT(ch->in_room->room_flags, RFL_RESTFUL_SHADE);

    new_event(EVNT_REMOVE_SHADE, level * 30 * 60, 0, 0, 0, ch->in_room, 0);

}                               /* end Spell_Restful_Shade */


/* REWIND: Creates a link to the location the spell was cast.  When the spell
 * expires the caster gets a magical feeling for the direction they were when
 * the spell was cast.
 */
void
spell_rewind(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Rewind */

    int duration, dist;
    duration = (30 + level * 30);
    dist = 10 * (level + 1);

    if (IS_SET(ch->in_room->room_flags, RFL_NO_ELEM_MAGICK)) {
        act("You cannot reach your plane to summon the necessary energy.", FALSE, ch, 0, 0,
            TO_CHAR);
        return;
    }

    new_event(EVNT_REWIND, duration, ch, 0, 0, ch->in_room, dist);
}                               /* end Spell_Rewind */


/* ROCKY TERRAIN: Adds the rocky terrain affect to targeted rooms.
 * double movement cost
 * double movement lag
 * mounts 30% harder to ride
 * ethereal immune
 * people with rocky terrain spell immune
 * flyers immune
 * levitators immune
 * ruk summoned mounts immune */
void
spell_rocky_terrain(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Rocky_Terrain */


}                               /* end Rocky_Terrain */


/* ROT ITEMS:  Rots items.
 * Void Elementalist and Sorcerer spell.  Nilaz nathro chran.
 */
void
spell_rot_items(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Rot Item */

    gamelogf("cast rot items");
    return;

}                               /* end Rot_items */


/* SANCTUARY: Puts a shield around the caster, which is visible
 * to the naked eye, and whose duration is dependent on power
 * level.  Requires a component except on the Plane of Water.
 * Water Elementalist and Sorcerer spell.  Vivadu viqrol grol.
 */
void
spell_sanctuary(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Sanctuary */
    struct affected_type af;
    struct affected_type *tmp_af;
    int duration = level * 2;

    memset(&af, 0, sizeof(af));

    /* OBJ_DATA *viqrol; */

    if (ch->in_room->sector_type != SECT_WATER_PLANE)
        if (!get_componentB
            (ch, SPHERE_CLERICAL, level, "$p dissolves and swirls around you.",
             "$p dissolves and swirls around $n.",
             "You do not have the correct component for a sanctuary spell."))
            return;

    if (!affected_by_spell(victim, SPELL_SANCTUARY)) {
        if (victim == ch) {
            act("You start glowing with a white aura.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n starts glowing with a white aura.", FALSE, ch, 0, 0, TO_ROOM);
        } else {
            act("You point at $N and $E starts glowing with a white aura.", FALSE, ch, 0, victim,
                TO_CHAR);
            act("$n points at you and you start glowing with a white aura.", FALSE, ch, 0, victim,
                TO_VICT);
            act("$n points at $N and $E starts glowing with a white aura.", TRUE, ch, 0, victim,
                TO_NOTVICT);
        }
    } else {
        if (victim == ch) {
            act("You start to glow a little more brightly.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n starts to glow a little more brightly.", FALSE, ch, 0, 0, TO_ROOM);
        } else {
            act("You point at $N and $E starts to glow a little brighter.", FALSE, ch, 0, victim,
                TO_CHAR);
            act("$n points at you and you start to glow a little brighter.", FALSE, ch, 0, victim,
                TO_VICT);
            act("$n points at $N and $E starts to glow a little brighter.", TRUE, ch, 0, victim,
                TO_NOTVICT);
        }
    }

    af.type = SPELL_SANCTUARY;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_SANCTUARY;

    if (affected_by_spell(victim, SPELL_SANCTUARY)) {
        for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next) {
            if (tmp_af->type == SPELL_SANCTUARY) {
                duration = MIN((duration + tmp_af->duration), 24);
                affect_from_char(victim, SPELL_SANCTUARY);
                break;
            }
        }
        af.duration = duration;
    }

    affect_to_char(victim, &af);

}                               /* end Spell_Sanctuary */


/* SAND JAMBIYA: Creates a weapon, whose duration and abilties depend
 * on the power level at which the spell was cast.  At mon, it requires
 * a component, and creates a permanent staff item.  Only castable in
 * sectors desert, silt, shallows, and earth_plane.  Rukkian spell. 
 * Ruk wilith viod.
 */
void
spell_sand_jambiya(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Sand_Jambiya */

    int jambiya_num = 0;
    int sandtype = 0;
    OBJ_DATA *tmp_obj;

    if (!ch) {
        gamelog("No ch in sand jambiya");
        return;
    }

    if (!ch->in_room) {
        gamelogf("Ch not in room in sand_jambiya");
        return;
    }

    sandtype = is_enough_sand(ch->in_room);

    if (sandtype == 0) {
        send_to_char("There is not enough sand here.\n\r", ch);
        return;
    }

    check_crim_cast(ch);

    switch (level) {
    case 1:
        jambiya_num = 456;
        break;
    case 2:
        jambiya_num = 457;
        break;
    case 3:
        jambiya_num = 458;
        break;
    case 4:
        jambiya_num = 459;
        break;
    case 5:
        jambiya_num = 460;
        break;
    case 6:
        jambiya_num = 461;
        break;
    case 7:
        if (!get_componentB
            (ch, SPHERE_CREATION, level,
             "$p shatters in your hands and falls to the ground, heating it up.",
             "$p shatters in $n's hands and falls to the ground.",
             "You do not have a powerful enough component to do that."))
            return;

        jambiya_num = 1378 + number(0, 7);      /* random object 1378 to 1385 */
        tmp_obj = read_object(jambiya_num, VIRTUAL);

	set_creator_edesc(ch, tmp_obj);

        obj_to_char(tmp_obj, ch);
        if (sandtype == 1) {
            act("The sand underfoot spirals up into $n's hands, melting into $p.",
                TRUE, ch, tmp_obj, 0, TO_ROOM);
            act("The sand underfoot spirals up into your hands melting into $p.",
                TRUE, ch, tmp_obj, 0, TO_CHAR);
        } else if (sandtype == 2) {
            act("The sand from the air spirals into $n's hands, melting into $p.",
                TRUE, ch, tmp_obj, 0, TO_ROOM);
            act("The sand from the air spirals into your hands melting into $p.",
                TRUE, ch, tmp_obj, 0, TO_CHAR);
        }
        return;
    default:
        gamelog("UNKNOWN POWER LEVEL IN SPELL_SAND_JAMBIYA()");
    }

    tmp_obj = read_object(jambiya_num, VIRTUAL);
    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);

    if (victim && (victim != ch)) {
        if (sandtype == 1) {
            act("The sand underfoot spirals up into $n's hands, forming $p, which $s drops at $N's feet.",
                TRUE, ch, tmp_obj, victim, TO_NOTVICT);
            act("The sand underfoot spirals up into your hands forming $p, which you drop at $N's feet.",
                TRUE, ch, tmp_obj, victim, TO_CHAR);
            act("The sand underfoot spirals up into $n's hands forming $p, which $s drops at your feet.",
                TRUE, ch, tmp_obj, victim, TO_VICT);
        } else if (sandtype == 2) {
            act("The sand from the air spirals into $n's hands, forming $p, which $s drops at $N's feet.",
                TRUE, ch, tmp_obj, victim, TO_NOTVICT);
            act("The sand from the air spirals into your hands forming $p, which you drop at $N's feet.",
                TRUE, ch, tmp_obj, victim, TO_CHAR);
            act("The sand from the air spirals into $n's hands forming $p, which $s drops at your feet.",
                TRUE, ch, tmp_obj, victim, TO_VICT);
        }
        obj_to_room(tmp_obj, ch->in_room);
    } else {
        if (sandtype == 1) {
            act("The sand underfoot spirals up into $n's hands, forming $p.",
                TRUE, ch, tmp_obj, 0, TO_ROOM);
            act("The sand underfoot spirals up into your hands forming $p.",
                TRUE, ch, tmp_obj, 0, TO_CHAR);
        } else if (sandtype == 2) {
            act("The sand from the air spirals into $n's hands, forming $p.",
                TRUE, ch, tmp_obj, 0, TO_ROOM);
            act("The sand from the air spirals into your hands forming $p.",
                TRUE, ch, tmp_obj, 0, TO_CHAR);
        }
        obj_to_char(tmp_obj, ch);
    }

    new_event(EVNT_SANDJAMBIYA_REMOVE, (((level + 1) * 3000) / 2), 0, 0, tmp_obj, 0, 0);

    return;

}                               /* end Spell_Sand_Jambiya */


/* SAND SHELTER: Spell started on 10/2/97 by Nessalin
 * Ideally it will create a sand shelter item that is a wagon, it will last a
 * number of hours related to the level cast at, when it dissolves, everyone
 * is forced to the room outside.  Like sand jambiya it can only be cast in
 * sandy areas (desert, silt, shallows, earth_plane).  Rukkian spell.  Ruk dreth grol.
 */
void
spell_sand_shelter(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Sand_Shelter */
    ROOM_DATA *to_room;
    OBJ_DATA *shelter;
    char buf[MAX_STRING_LENGTH];
    int d;
    int sandtype = 0;
    int lev = MAX(0, (level / 2) - 1);

    if (!ch) {
        gamelog("No ch in sand shelter");
        return;
    }

    if (!ch->in_room) {
        gamelogf("Ch not in a room in spell sand_shelter");
        return;
    }

    check_crim_cast(ch);

    sandtype = is_enough_sand(ch->in_room);
    if (sandtype == 0) {
        send_to_char("There is insufficient sand to create a shelter.\n\r", ch);
        return;
    }

    /* load the shelter item. You can use the same item over and over again */
    if ((shelter = read_object(SHELTER_OBJ + lev, VIRTUAL)) == NULL) {
        send_to_char("Error in spell: contact an immortal about the problem.", ch);
        return;
    }

    /* Pick an unused front room, by checking if any of the current front
     * rooms are hooked up to a shelter. */
    if (!(to_room = find_reserved_room())) {
        cprintf(ch, "Error in spell, please see an Overlord.\n");
        return;
    }

    /* set the description of the reserved room and save it */
    if (to_room->name)
        free(to_room->name);

    sprintf(buf, "Inside %s", OSTR(shelter, short_descr));
    to_room->name = strdup(buf);

    if (to_room->description)
        free(to_room->description);

    switch (lev) {
    case 0:
        to_room->description =
            strdup("The walls of this sand dwelling are"
                   " made of plain sand. Despite this, they\n\rseem to stand up without"
                   " any problem, spiting nature and providing shelter\n\rfrom the"
                   " winds outside.  The roof is cone-shaped and low, sitting"
                   " steadily\n\ras a rock over the walls, not a grain falling from its"
                   " smooth surface.  The\n\rfloor is made of sand as everything else in"
                   " this hut, and only leaves faint\n\rboot-prints from visitors. A"
                   " rectangular hole has been cut evenly into one\n\rof the walls,"
                   " opening up to the outside.\n\r");
        break;
    case 1:
        to_room->description =
            strdup("The walls of this sand dwelling are"
                   " made of plain sand. Despite this, they\n\rseem to stand up without"
                   " any problem, spiting nature and providing shelter\n\rfrom the"
                   " winds outside.  The roof is flat, sitting steadily as a rock"
                   " over\n\rthe walls, not a grain falling from its smooth surface.  "
                   "The floor is made\n\rof sand as everything else in this hut, and"
                   " only leaves faint boot-prints\n\rfrom visitors. A rectangular hole"
                   " has been cut evenly into one of the\n\rwalls, opening up to the"
                   " outside.\n\r");
        break;
    case 2:
        to_room->description =
            strdup("The sturdy walls of this sand"
                   " dwelling are made of plain sand. Despite\n\rthis, they seem"
                   " robustly built, and stand up without any problem, spiting\n\r"
                   "nature and providing shelter from the winds outside.  The roof is"
                   " slanted\n\rin a perfect angle, sitting steadily as a rock over the"
                   " walls, not a grain\n\rfalling from its smooth surface.  The floor"
                   " is made of sand as everything\n\relse in this hut, and only leaves"
                   " faint boot-prints from visitors. A\n\rrectangular hole has been"
                   " cut evenly into one of the walls, opening up to\n\rthe" " outside.\n\r");
        break;
    default:
        send_to_char("Error in spell, contact an immortal.\n\r", ch);
        return;
    }

    MUD_SET_BIT(to_room->room_flags, RFL_INDOORS);

    if (level < POWER_PAV)
        MUD_SET_BIT(to_room->room_flags, RFL_NO_MOUNT);

    MUD_SET_BIT(to_room->room_flags, RFL_NO_WAGON);
    to_room->sector_type = SECT_CITY;   /* to prevent Ranger-Quit */

    for (d = 0; d < 6; d++) {
        if (to_room->direction[d]) {
            free(to_room->direction[d]->general_description);
            free(to_room->direction[d]->keyword);
            free(to_room->direction[d]);
            to_room->direction[d] = NULL;
        }
    }

    save_rooms(&zone_table[RESERVED_Z], RESERVED_Z);
/* Set the entrance room on the shelter to be the unused room, set the
      front of the shelter to be 0 */
    shelter->obj_flags.value[0] = to_room->number;
    shelter->obj_flags.value[2] = 0;

    /* set magick so that npc's/pc's can't destroy it */
    MUD_SET_BIT(shelter->obj_flags.extra_flags, OFL_MAGIC);

    /* Put a duration on the shelter to crumble, ala mounts. Add into event.c
     * that when a shelter crumbles, all items and players in the entrance room
     * are moved to the shelter's room  */
    shelter->obj_flags.cost = level * 100;

    /* actually put our shelter in the room */
    obj_to_room(shelter, ch->in_room);

    /* messages */
    if (sandtype == 1) {
        act("The sand begins to bubble and boil near $n.", FALSE, ch, NULL, NULL, TO_ROOM);
        act("The sand begins to bubble and boil near you.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("The sand rises and expands and slowly forms into $p.", FALSE, ch, shelter, NULL, TO_ROOM);
        act("The sand rises and expands and slowly forms into $p.", FALSE, ch, shelter, NULL, TO_CHAR);
    } else if (sandtype == 2) {
        act("The sand begins to bubble and swirl in the air near $n.", FALSE, ch, NULL, NULL, TO_ROOM);
        act("The sand begins to bubble and swirl in the air near you.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("The sand coalesces and slowly forms into $p.", FALSE, ch, shelter, NULL, TO_ROOM);
        act("The sand coalesces and slowly forms into $p.", FALSE, ch, shelter, NULL, TO_CHAR);
    }
}                               /* end Spell_Sand_Shelter */


/* SAND STATUE: Can only be cast in rooms sectored DESERT, EARTH_PLANE, FIELD
 * HILLS, SALT_FLATS, SILT, and SHALLOWS.  Creates an object that is a small
 * figurine of the victim.  Then, when cast on the figurine, it creates an
 * npc of the person that cannot move.  If damaged, the npc will explode into
 * a small pile of useless sand.  At high power levels, it becomes difficult
 * to tell the npc apart from the original victim.
 * Started by Nessalin Oct 6th, 2001.
 * Finished by Halaster Oct 22nd, 2005.
 */
void
spell_sandstatue(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{
    OBJ_DATA *tmp_obj;
    CHAR_DATA *mob;
    char buf[MAX_STRING_LENGTH];
    char ename[MAX_STRING_LENGTH], edesc[MAX_STRING_LENGTH];
    char *name_string;
    char *sdesc_string;
    char *mdesc_string;
    int npcnumber = 97, r_num, sandtype = 0;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in sandstatue");
        return;
    }

    if (!ch->in_room) {
        gamelogf("Ch not in a room in spell sandstatue");
        return;
    }

    /* check crim       */
    check_crim_cast(ch);

    sandtype = is_enough_sand(ch->in_room);
    if (sandtype == 0) {
        send_to_char("There is insufficient sand to create a statue.\n\r", ch);
        return;
    }

    if (obj) {
        if ((r_num = real_mobile(npcnumber)) < 0) {
            send_to_char("Nothing happens.\n\r", ch);
            gamelog("Error in spell_sandstatue -- invalid NPC number.");
            return;
        }
        if (!get_obj_extra_desc_value(obj, "[SAND_STATUE_NAME]", edesc, 100)) {
            act("$p is not a valid target.", FALSE, ch, obj, 0, TO_CHAR);
            return;
        } else {
            sprintf(buf, "%s", edesc);
            name_string = strdup(buf);
        }
        if (!get_obj_extra_desc_value(obj, "[SAND_STATUE_SDESC]", edesc, sizeof(edesc))) {
            act("$p is not a valid target.", FALSE, ch, obj, 0, TO_CHAR);
            return;
        } else {
            sprintf(buf, "%s", edesc);
            sdesc_string = strdup(buf);
        }
        if (!get_obj_extra_desc_value(obj, "[SAND_STATUE_MDESC]", edesc, sizeof(edesc))) {
            act("$p is not a valid target.", FALSE, ch, obj, 0, TO_CHAR);
            return;
        } else {
            sprintf(buf, "%s", edesc);
            mdesc_string = strdup(buf);
        }

        mob = read_mobile(r_num, REAL);
        char_to_room(mob, ch->in_room);

        /* Setup NPC's name desc, sdesc, ldesc */
        sprintf(buf, "%s statue sand [sand_statue]", name_string);
        mob->name = (char *) strdup(buf);
        /* add "the sand-statue of" in front of sdesc if een or lower */
        if (level <= 4)
            sprintf(buf, "the sand-statue of %s", sdesc_string);
        else
            sprintf(buf, "%s", sdesc_string);
        mob->short_descr = (char *) strdup(buf);
        sprintf(buf, "%s", mdesc_string);
        mob->description = (char *) strdup(buf);
        mob->long_descr = (char *) strdup("is standing here.\n\r");

        act("You drop $p and it quickly grows into $N.", TRUE, ch, obj, mob, TO_CHAR);
        act("$n drops $p and it quickly grows into $N.", TRUE, ch, obj, mob, TO_ROOM);
        obj_from_char(obj);
        extract_obj(obj);

        strcpy(ename, "[SAND_STATUE_POWERLEVEL]");
        sprintf(edesc, "%d", level);
        set_char_extra_desc_value(mob, ename, edesc);

        af.type = SPELL_GATE;
        af.duration = (2 * level) + number(1, 5);
        af.modifier = 0;
        af.location = 0;
        af.bitvector = CHAR_AFF_SUMMONED;
        affect_join(mob, &af, TRUE, FALSE);

        return;
    } else if (victim) {
        /* Don't let them make a statue of a stuate */
        if (isnamebracketsok("[sand_statue]", MSTR(victim, name))) {
            act("You cannot create a statue of a statue.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        tmp_obj = read_object(1209, VIRTUAL);
        if (!tmp_obj) {
            errorlog("spell_sandstatue() in magick.c, object #1209 not found.");
            return;
        }
        obj_to_char(tmp_obj, ch);
        if (victim->player.extkwds) {   /* victim is a PC */
            sprintf(buf, "%s %s", victim->player.extkwds, MSTR(victim, name));
            name_string = strdup(buf);
        } else
            sprintf(buf, "%s", MSTR(victim, name));
        name_string = strdup(buf);      /* victim is an NPC */
        sdesc_string = strdup(MSTR(victim, short_descr));
        if (MSTR(victim, description))
            mdesc_string = strdup(MSTR(victim, description));
        else
            mdesc_string = NULL;
        strcpy(ename, "[SAND_STATUE_NAME]");
        sprintf(edesc, "%s", name_string);
        set_obj_extra_desc_value(tmp_obj, ename, edesc);
        strcpy(ename, "[SAND_STATUE_SDESC]");
        sprintf(edesc, "%s", sdesc_string);
        set_obj_extra_desc_value(tmp_obj, ename, edesc);
        strcpy(ename, "[SAND_STATUE_MDESC]");
        sprintf(edesc, "%s", mdesc_string);
        set_obj_extra_desc_value(tmp_obj, ename, edesc);
        strcpy(ename, "image");
        sprintf(edesc, "You look at the image, and you see:\n\r%s", mdesc_string);
        set_obj_extra_desc_value(tmp_obj, ename, edesc);

        if (victim == ch) {
            if (sandtype == 1) {
                act("Sand rises from the ground into your hands, forming $p.",
                     FALSE, ch, tmp_obj, 0, TO_CHAR);
                act("Sand rises from the ground into $n's hands, forming $p.",
                     FALSE, ch, tmp_obj, 0, TO_ROOM);
            } else if (sandtype == 2) {
                act("Sand swirls from the air into your hands, forming $p.",
                     FALSE, ch, tmp_obj, 0, TO_CHAR);
                act("Sand swirls from the air into $n's hands, forming $p.",
                     FALSE, ch, tmp_obj, 0, TO_ROOM);
            }
        } else {
            if (sandtype == 1) {
                act("You gaze at $N as sand rises from the ground into your hands, forming $p.",
                    FALSE, ch, tmp_obj, victim, TO_CHAR);
                act("$n gazes at you as sand rises from the ground into $s hands, forming $p.",
                    FALSE, ch, tmp_obj, victim, TO_VICT);
                act("$n gazes at $N as sand rises from the ground into $s hands, forming $p.",
                    TRUE, ch, tmp_obj, victim, TO_NOTVICT);
            } else if (sandtype == 2) {
                act("You gaze at $N as sand swirls from the air into your hands, forming $p.",
                    FALSE, ch, tmp_obj, victim, TO_CHAR);
                act("$n gazes at you as sand swirls from the air into $s hands, forming $p.",
                    FALSE, ch, tmp_obj, victim, TO_VICT);
                act("$n gazes at $N as sand swirls from the air into $s hands, forming $p.",
                    TRUE, ch, tmp_obj, victim, TO_NOTVICT);
            }
        }
    } else
        return;

}

/* SANDSTORM: Can only be cast in rooms sectored DESERT.  Directs an
 * attack against the target, which has a chance of hurling them out 
 * of the room.  Damage is dependent on power level.  Whiran spell.
 * Whira nathro echri.
 */
void
spell_sandstorm(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Sandstorm */
    int alive;
    int dam, chance, dir;
    int sandtype = 0;
    ROOM_DATA *room;

    if (!ch) {
        gamelogf("No ch in spell_sandstorm!");
        return;
    }

    if (!ch->in_room) {
        gamelogf("Ch not in a room in spell sandstorm");
        return;
    }

    sandtype = is_enough_sand(ch->in_room);
    if (sandtype == 0) {
        send_to_char("There is insufficient sand to create a sandstorm.\n\r", ch);
        return;
    }

    check_criminal_flag(ch, NULL);

    adjust_move(victim, -(level * 2));

    dam = dice(3, level * 3);
    if (sandtype == 2)
        dam += (level * 2);
    alive = ws_damage(ch, victim, dam, 0, 0, dam, SPELL_SANDSTORM, TYPE_ELEM_WIND);

    if (!victim || !victim->in_room || !alive)
        return;

    chance = (level * 10) - 10;
    if ((chance > number(1, 100)) && ch->in_room->direction[dir = number(0, 5)]) {
        act("Your sandstorm hurls $2$N$1 out of the room!", FALSE, ch, 0, victim, TO_CHAR);
        act("$2$N$1 is hurled out of the room!", FALSE, ch, 0, victim, TO_NOTVICT);
        act("$2You$1 are hurled out of the room!", FALSE, ch, 0, victim, TO_VICT);
        act("There is a sudden gust of sandy wind from the $D.", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);

        room = ch->in_room->direction[dir]->to_room;
        char_from_room(victim);
        char_to_room(victim, room);

        act("$2$n$1 is hurled into the room!", FALSE, victim, 0, 0, TO_ROOM);
    }
}                               /* end Spell_Sandstorm */


/* suk-krath wilith hekro */

void
spell_fireseed(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Fireseed */
    int seed_num = 0;
    OBJ_DATA *tmp_obj;

    if (!ch) {
        gamelog("No ch in fireseed");
        return;
    }

    check_criminal_flag(ch, NULL);

    if ((ch->in_room->sector_type == SECT_WATER_PLANE)
        || ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))) {
        send_to_char("You are unable to reach into Suk-Krath for a fire seed from here.\n\r", ch);
        return;
    }

    switch (level) {
    case 1:
        seed_num = 443;
        break;
    case 2:
        seed_num = 444;
        break;
    case 3:
        seed_num = 445;
        break;
    case 4:
        seed_num = 446;
        break;
    case 5:
        seed_num = 447;
        break;
    case 6:
        seed_num = 448;
        break;
    case 7:
        seed_num = 449;
        break;
    default:
        gamelog("UNKNOWN POWER LEVEL IN SPELL_FIRESEED()");
    };

    tmp_obj = read_object(seed_num, VIRTUAL);

    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);

    if (victim && (victim != ch)) {
        if (!(victim->equipment[EP])) {
            act("A bright light flashes between $n's hands, revealing $p, which $e tosses to $N.",
                TRUE, ch, tmp_obj, victim, TO_NOTVICT);
            act("A bright light flashes between your hands, revealing $p, which you toss to $N.",
                TRUE, ch, tmp_obj, victim, TO_CHAR);
            act("A bright light flashes beween $n's  hands, revealing $p, which $e tosses to you.",
                TRUE, ch, tmp_obj, victim, TO_VICT);
            equip_char(victim, tmp_obj, EP);
        } else {
            act("$n tries to catch it but $s hand already holds $p.", TRUE, victim,
                victim->equipment[EP], 0, TO_NOTVICT);
            act("You try to catch it but you're are already holding $p.", TRUE, victim,
                victim->equipment[EP], 0, TO_CHAR);
            extract_obj(tmp_obj);
            return;
        }
    } else {
        // This could happen if they're a mantis
        if (ch->equipment[EP]) {
            act("A bright light flashes between your hands, but $p blocks the magick.", TRUE, ch,
                ch->equipment[EP], 0, TO_CHAR);
            extract_obj(tmp_obj);
            return;
        }

        act("A bright light flashes between $n's hands, revealing $p.", TRUE, ch, tmp_obj, 0,
            TO_ROOM);
        act("A bright light flashes between your hands, revealing $p.", TRUE, ch, tmp_obj, 0,
            TO_CHAR);
        equip_char(ch, tmp_obj, EP);
    }


    //  obj_to_char(tmp_obj, ch);

    new_event(EVNT_SANDJAMBIYA_REMOVE, (24 + (number(1, 6))) * 60, 0, 0, tmp_obj, 0, 0);

    return;
}                               /* end Spell_Fireseed */



/* Ruk divan echr */

/* suk-krath wilith hekro */

void
spell_shatter(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Shatter */
    int dur = 0;
    /*    OBJ_DATA *tmp_obj; */

    if (!ch) {
        gamelog("No ch in shatter");
        return;
    }

    if (obj) {
        act("You set shatter on $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n sets shatter on  $p.", FALSE, ch, obj, 0, TO_ROOM);

        switch (level) {
        case 1:
            dur = 443;
            break;
        case 2:
            dur = 444;
            break;
        case 3:
            dur = 445;
            break;
        case 4:
            dur = 445;
            break;
        case 5:
            dur = 445;
            break;
        case 6:
            dur = 445;
            break;
        case 7:
            dur = 445;
            break;
        default:
            gamelog("UNKNOWN POWER LEVEL IN SPELL_shatter()");
        };


        /*
         * new_event(EVNT_SHATTER_REMOVE,
         * (24 + (number(1, 6))) * 60, 0, 0, obj, 0, 0);
         */

    }


    return;
}                               /* end Spell_Shatter */



/* SEND SHADOW: Casting this spell sends a person into a deep trance 
 * as their consciousness is projected forth into a creature made of 
 * shadows, which is able to pass through most barriers.  Their projected 
 * form is not visible to most of the physical world but also is unable
 * to interact with it.  Duration dependent on power level.  Drovian
 * spell.  Drov iluth chran.
 */
void
spell_send_shadow(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Send_Shadow */
    struct affected_type af;
    CHAR_DATA *shadow;
    int i, duration;
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];
    char mobstr[MAX_STRING_LENGTH];
    char *f;
    char shadow_name[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    if ((ch->in_room->sector_type == SECT_FIRE_PLANE)
        || ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))) {
        send_to_char("You are unable to create that kind of shadow here.\n\r", ch);
        return;
    }

    if (!ch->desc)
        return;

    if (ch->desc->original) {
        send_to_char("You cannot send shadows while switched.\n\r", ch);
        return;
    }

    if (!get_componentB
        (ch, SPHERE_ILLUSION, level, "$p fades into shadows then disappears entirely.",
         "$p fades into shadows in $n's hands then disappears entirely.",
         "You do not have the proper component for a sending a shadow."))
        return;

    if (ch->desc->snoop.snooping || ch->desc->snoop.snoop_by) {
        send_to_char("The magick fizzles and dies.\n\r", ch);
        return;
    }

    check_crim_cast(ch);

    shadow = read_mobile(1200, VIRTUAL);

    if (!shadow) {
        send_to_char("The magick fizzles and dies." " (no mobile loaded).\n\r", ch);
        errorlog("spell_send_shadow() in magick.c unable to load NPC");
        return;
    }

    char_to_room(shadow, ch->in_room);

    /* Assign a random sdesc if not already present */
    if (TRUE == get_char_extra_desc_value(ch, "[SEND_SHADOW_SDESC]", buf1, MAX_STRING_LENGTH))
        strcpy(mobstr, buf1);
    else {
        switch (number(1, 25)) {
        case 1:
            strcpy(mobstr, "a small, shadowy rat");
            break;
        case 2:
            strcpy(mobstr, "a large, shadowy spider");
            break;
        case 3:
            strcpy(mobstr, "a shapeless, dark cloud");
            break;
        case 4:
            strcpy(mobstr, "a blurry, shadowy gortok");
            break;
        case 5:
            strcpy(mobstr, "the sinuous, shadowy serpent");
            break;
        case 6:
            strcpy(mobstr, "an ethereal, midnight-hued falcon");
            break;
        case 7:
            strcpy(mobstr, "the shadowy outline of a beetle");
            break;
        case 8:
            strcpy(mobstr, "a wiry, many branched shadow");
            break;
        case 9:
            strcpy(mobstr, "an ethereal, inky black bat");
            break;
        case 10:
            strcpy(mobstr, "an ethereal, umbral serpent");
            break;
        case 11:
            strcpy(mobstr, "a gaunt, tenebrous shadowy gith");
            break;
        case 12:
            strcpy(mobstr, "the shadowy outline of a verrin hawk");
            break;
        case 13:
            strcpy(mobstr, "the shadowy outline of a rantarri");
            break;
        case 14:
            strcpy(mobstr, "a murky wash of shadow");
            break;
        case 15:
            strcpy(mobstr, "a shadowy wisp of a gurth");
            break;
        case 16:
            strcpy(mobstr, "a gloomy, shadowy humanoid");
            break;
        case 17:
            strcpy(mobstr, "a shadowy wisp of a vestric");
            break;
        case 18:
            strcpy(mobstr, "a lumbering shadow of a lizard");
            break;
        case 19:
            strcpy(mobstr, "a smoky shadow of a cochra");
            break;
        case 20:
            strcpy(mobstr, "an ethereal, black skull");
            break;
        case 21:
            strcpy(mobstr, "an inky, ethereal silt-crawler");
            break;
        case 22:
            strcpy(mobstr, "an ethereal, wispy cheotan lizard");
            break;
        case 23:
            strcpy(mobstr, "a velvety shadow of a quirri");
            break;
        case 24:
            strcpy(mobstr, "a shadow, antlered carru shape");
            break;
        default:
            strcpy(mobstr, "a shimmery, transparent shadow of a jozhal");
            break;
        }
        gamelogf("%s has cast spell_send_shadow for the first time - setting edesc.", GET_NAME(ch));
        act("You bond with a particular manifestation of shadow.", FALSE, ch, 0, 0, TO_CHAR);
        sprintf(buf2, "%s", mobstr);
        set_char_extra_desc_value(ch, "[SEND_SHADOW_SDESC]", buf2);
    }
    sprintf(buf3, "%s", mobstr);
    free(shadow->short_descr);
    shadow->short_descr = (char *) strdup(buf3);

    act("$N appears in a shadowy fog, standing in front of $n.", FALSE, ch, 0, shadow, TO_ROOM);
    act("$N appears in a shadowy fog, standing in front of you.", FALSE, ch, 0, shadow, TO_CHAR);
    act("$n and $N stare at each other momentarily, and $n " "slowly closes his eyes.", TRUE, ch, 0,
        shadow, TO_ROOM);
    act("You stare at $N's eyes, and $S face slowly melts " "into the form of yours.", FALSE, ch, 0,
        shadow, TO_CHAR);

    if (!ch->desc->original)
        ch->desc->original = ch;
    ch->desc->character = shadow;

    ch->desc->original = ch;

    shadow->desc = ch->desc;
    ch->desc = 0;

    /* Give the shadow the extra hidden keyword of the occupant */
    sprintf(shadow_name, "%s [%s]", MSTR(shadow, name), MSTR(ch, name));

    f = strdup(shadow_name);
    free((char *) shadow->name);
    shadow->name = f;

    // The mind is leaving the body, no psionic affects should remain on the
    // body at this point.
    cease_all_psionics(ch, FALSE);

    /* Also a good spot to move this "shadow" char onto the lists of those who
     * are monitoring the char */
    add_monitoring_for_mobile(ch, shadow);

    set_hit(shadow, GET_MAX_HIT(shadow));
    set_move(shadow, GET_MAX_MOVE(shadow));
    set_stun(shadow, GET_MAX_STUN(shadow));

    /* Give psionics & languages to shadow */
    for (i = 0; i < MAX_SKILLS; i++) {
        if ((ch->skills[i] && skill[i].sk_class == CLASS_PSIONICS)
            || (ch->skills[i] && skill[i].sk_class == CLASS_LANG)) {
            /* so they can't shadow walk in send-shadow form */
            if (i == PSI_SHADOW_WALK)
                continue;
            if (!shadow->skills[i])
                init_skill(shadow, i, ch->skills[i]->learned);
            else
                shadow->skills[i]->learned = ch->skills[i]->learned;
        }
    }

    shadow->specials.language = ch->specials.language;

    if (ch->in_room->sector_type == SECT_SHADOW_PLANE)
        duration = level * 6;
    else
        duration = level * 3;

    af.type = SPELL_SEND_SHADOW;
    af.power = level;
    af.duration = duration;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_SUMMONED;
    af.modifier = 0;
    affect_to_char(shadow, &af);

    affect_join(shadow, &af, TRUE, FALSE);

    af.type = SPELL_INFRAVISION;
    af.power = level;
    af.duration = duration + 1;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_INFRAVISION;
    af.modifier = 0;
    affect_to_char(shadow, &af);
}                               /* end Spell_Send_Shadow */


/* SHADOW SWORD: Yet another version of jambiya.  This time for Drovians. 
 * Like sand jambiyas, form differs with power level and at mon with the
 * correct component, creates a permanent staff item.  Drov wilith viod.
 */
void
spell_shadow_sword(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Shadow_Sword */

    int jambiya_num = 0;
    OBJ_DATA *tmp_obj;

    if (!ch) {
        gamelog("No ch in shadow sword");
        return;
    }

    check_crim_cast(ch);

    if ((ch->in_room->sector_type == SECT_FIRE_PLANE)
        || ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))) {
        send_to_char("You are unable to create a shadow weapon here.\n\r", ch);
        return;
    }


    switch (level) {
    case 1:
        jambiya_num = 450;
        break;
    case 2:
        jambiya_num = 451;
        break;
    case 3:
        jambiya_num = 452;
        break;
    case 4:
        jambiya_num = 453;
        break;
    case 5:
        jambiya_num = 454;
        break;
    case 6:
        jambiya_num = 455;
        break;
    case 7:
        if (!get_componentB
            (ch, SPHERE_CREATION, level,
             "$p shatters in your hands and falls to the ground, heating it up.",
             "$p shatters in $n's hands and falls to the ground.",
             "You do not have a powerful enough component to do that."))
            return;

        jambiya_num = 1468 + number(0, 7);      /* random object 1468 to 1475 */
        tmp_obj = read_object(jambiya_num, VIRTUAL);

	set_creator_edesc(ch, tmp_obj);

        obj_to_char(tmp_obj, ch);

        act("Wavering shadows coalesce into $n's hands, melting into $p.", TRUE, ch, tmp_obj, 0,
            TO_ROOM);
        act("Wavering shadows coalesce into your hands, melting into $p.", TRUE, ch, tmp_obj, 0,
            TO_CHAR);

        return;
    default:
        gamelog("UNKNOWN POWER LEVEL IN SPELL_SHADOW_SWORD()");
    };

    tmp_obj = read_object(jambiya_num, VIRTUAL);

    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);


    if (victim && (victim != ch)) {
        act("Wavering shadows coalesce into $n's hands, forming $p, which $s drops at $N's feet.",
            TRUE, ch, tmp_obj, victim, TO_NOTVICT);
        act("Wavering shadows coalesce into your hands forming $p, which you drop at $N's feet.",
            TRUE, ch, tmp_obj, victim, TO_CHAR);
        act("Wavering shadows coalesce into $n's hands forming $p, which $s drops at your feet.",
            TRUE, ch, tmp_obj, 0, TO_VICT);
        obj_to_room(tmp_obj, ch->in_room);
    } else {
        act("Wavering shadows coalesce into $n's hands, forming $p.", TRUE, ch, tmp_obj, 0,
            TO_ROOM);
        act("Wavering shadows coalesce into your hands forming $p.", TRUE, ch, tmp_obj, 0, TO_CHAR);
        obj_to_char(tmp_obj, ch);
    }

    if (ch->in_room->sector_type == SECT_SHADOW_PLANE)
        new_event(EVNT_SHADOWSWORD_REMOVE, ((level + 1) * 3000), 0, 0, tmp_obj, 0, 0);
    else
        new_event(EVNT_SHADOWSWORD_REMOVE, (((level + 1) * 3000) / 2), 0, 0, tmp_obj, 0, 0);

    return;

}                               /* end Spell_Shadow_Sword */


/* SHADOWPLAY: Allows the caster to create an amusing and
 * sometimes alarming illusion.  Drovian spell.
 * Drov iluth nikiz.  Mon and sul level do create darkness
 * as well.
 */
void
spell_shadowplay(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Shadowplay */
    int effect = number(1, 7);
    char msg1[256], msg2[256];

    check_crim_cast(ch);

    if (level == 7) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Shadows rush out from $n, enveloping all in darkness.");
            strcpy(msg2, "Shadows rush outward, enveloping all in darkness.");
            break;
        case 2:
            strcpy(msg1,
                   "At $n's gesture, a group of shades leap from the ground, and launch into the air, wailing an eerie cry.");
            strcpy(msg2,
                   "At your gesture, a group of shades leap from the ground, and launch into the air, wailing an eerie cry.");
            break;
        case 3:
            strcpy(msg1, "Shadows begin to petal around $n, enveloping $m in darkness.");
            strcpy(msg2, "Shadows begin to petal around $n, enveloping $m in darkness.");
            break;
        case 4:
            strcpy(msg1, "In front of $n, the shadows bubble together into a towering form.");
            strcpy(msg2, "In front of you, the shadows bubble together into a towering form.");
            break;
        case 5:
            strcpy(msg1,
                   "Shadows ripple and merge behind $n, looming above $m in a giant, shadowy and ominous humanoid shape.");
            strcpy(msg2, "Shadows ripple and merge behind you, forming a humanoid shape.");
            break;
        case 6:
            strcpy(msg1, "A pitch-black arc of shadow crosses the sky, blotting out all light.");
            strcpy(msg2, "A pitch-black arc of shadow crosses the sky, blotting out all light.");
            break;
        case 7:
            strcpy(msg1, "The shadows before $n meld into a horrific visage of a screaming face.");
            strcpy(msg2, "The shadows before you meld into a horrific visage of a screaming face.");
            break;
        default:
            strcpy(msg1, "Shadows rush towards $n, enveloping $m in darkness.");
            strcpy(msg2, "Shadows rush towards you, enveloping you in darkness.");
            break;
        }
    } else if (level == 6) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "A shadowy carru steps out of a nearby shadow, and lowers its horns, ready to charge.");
            strcpy(msg2,
                   "A shadowy carru steps out of a nearby shadow, and lowers its horns, ready to charge.");
            break;
        case 2:
            strcpy(msg1,
                   "$n raises a hand, and hundreds of rodents scurry forth from a nearby shadow.");
            strcpy(msg2,
                   "You raise a hand, and hundreds of rodents scurry forth from a nearby shadow.");
            break;
        case 3:
            strcpy(msg1, "A pitch-black arc of shadow crosses the sky, blotting out all light.");
            strcpy(msg2, "A pitch-black arc of shadow crosses the sky, blotting out all light.");
            break;
        case 4:
            strcpy(msg1, "The shadows before $n meld into a horrific visage of a screaming face.");
            strcpy(msg2, "The shadows before you meld into a horrific visage of a screaming face.");
            break;
        case 5:
            strcpy(msg1, "$n's skin blackens, and several shadows appear, mirroring $m.");
            strcpy(msg2, "Your skin blackens, and several shadows appear, mirroring you.");
            break;
        case 6:
            strcpy(msg1,
                   "$n extends $s arms, and shadows leap towards $m, whirling around in a dark circle.");
            strcpy(msg2,
                   "You extend your arms, and shadows leap towards you, whirling around in a dark circle.");
            break;
        case 7:
            strcpy(msg1, "Massive shadows of mekillot and bahamet trundle slowly across the sky.");
            strcpy(msg2, "Massive shadows of mekillot and bahamet trundle slowly across the sky.");
            break;
        default:
            strcpy(msg1, "In front of $n, the shadows bubble together into a towering form.");
            strcpy(msg2, "In front of you, the shadows bubble together into a towering form.");
            break;
        }
    } else if (level == 5) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Shadows coil and writhe like serpents on the ground.");
            strcpy(msg2, "Shadows coil and writhe like serpents on the ground.");
            break;
        case 2:
            strcpy(msg1,
                   "$n raises a hand, and hundreds of rodents scurry forth from a nearby shadow.");
            strcpy(msg2,
                   "You raise a hand, and hundreds of rodents scurry forth from a nearby shadow.");
            break;
        case 3:
            strcpy(msg1, "Around $n, the shadows open their eyes, glowing golden and menacing.");
            strcpy(msg2, "Around you, the shadows open their eyes, glowing golden and menacing.");
            break;
        case 4:
            strcpy(msg1,
                   "Shadows wrap themselves around $n, weaving about $m like a cloak of onyx.");
            strcpy(msg2,
                   "Shadows wrap themselves around you, weaving about you like a cloak of onyx.");
            break;
        case 5:
            strcpy(msg1,
                   "A shadow appears in the air in the form of a verrin hawk, before flapping silently away.");
            strcpy(msg2,
                   "A shadow appears in the air in the form of a verrin hawk, before flapping silently away.");
            break;
        case 6:
            strcpy(msg1, "Your shadow turns, and starts advancing on you ominously.");
            strcpy(msg2, "You turn shadows in the room against their owners.");
            break;
        case 7:
            strcpy(msg1, "$n opens $s hand, and black shadows stream forth.");
            strcpy(msg2, "You open your hand, and black shadows stream forth.");
            break;
        default:
            strcpy(msg1, "Shadows coil and writhe like serpents on the ground.");
            strcpy(msg2, "Shadows coil and writhe like serpents on the ground.");
            break;
        }
    } else if (level == 4) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Around $n, the shadows open their eyes, glowing golden and menacing.");
            strcpy(msg2, "Around you, the shadows open their eyes, glowing golden and menacing.");
            break;
        case 2:
            strcpy(msg1,
                   "Tendrils of shadow seep forth from the ground to cover $n's skin, enveloping $m in shadow.");
            strcpy(msg2,
                   "Tendrils of shadow seep forth from the ground to cover your skin, enveloping you in shadow.");
            break;
        case 3:
            strcpy(msg1, "$n falls backwards, and melts into a shadow.");
            strcpy(msg2, "You fall backwards, and melt into a shadow.");
            break;
        case 4:
            strcpy(msg1, "The air is filled with a swarm of shadowy insects.");
            strcpy(msg2, "The air is filled with a swarm of shadowy insects.");
            break;
        case 5:
            strcpy(msg1,
                   "A field of blackness seeps forth from around $n, and shadowy hands reach out.");
            strcpy(msg2,
                   "A field of blackness seeps forth from around you, and shadowy hands reach out.");
            break;
        case 6:
            strcpy(msg1,
                   "A shadowy form of a quirri appears, to silently stalk and consume a lesser shadow.");
            strcpy(msg2,
                   "A shadowy form of a quirri appears, to silently stalk and consume a lesser shadow.");
            break;
        case 7:
            strcpy(msg1, "A black, shadowy maw of some huge creature appears, and consumes $n.");
            strcpy(msg2,
                   "A black, shadowy maw of some huge creature appears, seeming to consume you.");
            break;
        default:
            strcpy(msg1, "Around $n, the shadows open their eyes, glowing golden and menacing.");
            strcpy(msg2, "Around you, the shadows open their eyes, glowing golden and menacing.");
            break;
        }
    } else if (level == 3) {
        switch (effect) {
        case 1:
            strcpy(msg1, "Shadows coalesce around $n, shrouding $m in darkness.");
            strcpy(msg2, "Shadows coalesce around you, shrouding you in darkness.");
            break;
        case 2:
            strcpy(msg1, "Shadows churn around $n's feet.");
            strcpy(msg2, "Shadows churn around your feet.");
            break;
        case 3:
            strcpy(msg1, "$n's shadow begins to move on its own, scuttling across the ground.");
            strcpy(msg2, "Your shadow begins to move on its own, scuttling across the ground.");
            break;
        case 4:
            strcpy(msg1,
                   "Shadows around $n begin to move as if alive, bending into ominous forms.");
            strcpy(msg2,
                   "Shadows around you begin to move as if alive, bending into ominous forms.");
            break;
        case 5:
            strcpy(msg1,
                   "$n raises $s hands, the shadows on the ground pulling off, and swirling around $m.");
            strcpy(msg2,
                   "You raise your hands, the shadows on the ground pulling off, and swirling around you.");
            break;
        case 6:
            strcpy(msg1,
                   "$n's eyes turn completely black, and black flames lick from $s mouth for a few moments.");
            strcpy(msg2,
                   "Your eyes turn completely black, and black flames lick from your mouth for a few moments.");
            break;
        case 7:
            strcpy(msg1,
                   "$n shatters into hundreds of tiny shadowy insects, and reforms a few cords away.");
            strcpy(msg2,
                   "You shatter into hundreds of tiny shadowy insects, and reform a few cords away.");
            break;
        default:
            strcpy(msg1, "Shadows coalesce around $n, shrouding $m in darkness.");
            strcpy(msg2, "Shadows coalesce around you, shrouding you in darkness.");
            break;
        }
    } else if (level == 2) {
        switch (effect) {
        case 1:
            strcpy(msg1,
                   "An obsidian serpent uncoils itself from the shadows, undulating as it hisses.");
            strcpy(msg2,
                   "An obsidian serpent uncoils itself from the shadows, undulating as it hisses.");
            break;
        case 2:
            strcpy(msg1, "Shadows slowly crawl from the ground, up over $n's body.");
            strcpy(msg2, "Shadows slowly crawl from the ground, up over your body.");
            break;
        case 3:
            strcpy(msg1, "$n makes a gesture, and the shadows around the area move to $s side.");
            strcpy(msg2, "You gesture, and the shadows around the area move to your side.");
            break;
        case 4:
            strcpy(msg1,
                   "Shadowy hands reach up from the ground and grasp $n, pulling $m into darkness.");
            strcpy(msg2,
                   "Shadowy hands reach up from the ground and grasp you, pulling you into darkness.");
            break;
        case 5:
            strcpy(msg1,
                   "Two shadows pull themselves off the ground, appear to draw swords, and engage one another in combat.");
            strcpy(msg2,
                   "Two shadows pull themselves off the ground, appear to draw swords, and engage one another in combat.");
            break;
        case 6:
            strcpy(msg1, "The surrounding light dims to almost complete darkness.");
            strcpy(msg2, "The surrounding light dims to almost complete darkness.");
            break;
        case 7:
            strcpy(msg1,
                   "Icorous black tentacles lash forth from the ground, whipping about in the air.");
            strcpy(msg2,
                   "Icorous black tentacles lash forth from the ground, whipping about in the air.");
            break;
        default:
            strcpy(msg1,
                   "An obsidian serpent uncoils itself from the shadows, undulating as it hisses.");
            strcpy(msg2,
                   "An obsidian serpent uncoils itself from the shadows, undulating as it hisses.");
            break;
        }
    } else {
        switch (effect) {
        case 1:
            strcpy(msg1, "The shadows writhe, creeping forward to envelop $n.");
            strcpy(msg2, "The shadows writhe, creeping forward to envelop you.");
            break;
        case 2:
            strcpy(msg1, "All around $n, the shadows deepen.");
            strcpy(msg2, "All around you, the shadows deepen.");
            break;
        case 3:
            strcpy(msg1, "Black tongues of shadow weave across the ground.");
            strcpy(msg2, "Black tongues of shadow weave across the ground.");
            break;
        case 4:
            strcpy(msg1, "A dark shadow wafts through the air.");
            strcpy(msg2, "A dark shadow wafts through the air.");
            break;
        case 5:
            strcpy(msg1, "Shadows dance across the ground, moving of their own accord.");
            strcpy(msg2, "Shadows dance across the ground, moving of their own accord.");
            break;
        case 6:
            strcpy(msg1, "The air grows darker, light fading.");
            strcpy(msg2, "The air grows darker, light fading.");
            break;
        case 7:
            strcpy(msg1, "The shadows in the surrounds begin whispering in soft, quiet tones.");
            strcpy(msg2, "The shadows in the surrounds begin whispering in soft, quiet tones.");
            break;
        default:
            strcpy(msg1, "The shadows writhe, creeping forward to envelop $n.");
            strcpy(msg2, "The shadows writhe, creeping forward to envelop you.");
            break;
        }
    }

    act(msg1, FALSE, ch, 0, 0, TO_ROOM);
    act(msg2, FALSE, ch, 0, 0, TO_CHAR);

    if (level >= 5) {
        if (!IS_SET(ch->in_room->room_flags, RFL_DARK)) {
            MUD_SET_BIT(ch->in_room->room_flags, RFL_DARK);
            new_event(EVNT_REMOVE_DARK, level * 30 * 60, 0, 0, 0, ch->in_room, 0);
	}
    }
}                               /* end Spell_Shadowplay */


/* SHIELD OF MIST: 
 *  vivadu dreth grol
 */
void
spell_shield_of_mist(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                     OBJ_DATA * obj)
{                               /* begin Spell_Shield_Of_Mist */
    int shield_num = 0;
    OBJ_DATA *tmp_obj;

    if (!ch) {
        gamelog("No ch in shield of mist");
        return;
    }

    check_crim_cast(ch);

    if ((ch->in_room->sector_type == SECT_FIRE_PLANE)
        || ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))) {
        send_to_char("You are unable to reach Vivadu from here to create a shield.\n\r", ch);
        return;
    }

    switch (level) {
    case 1:
        shield_num = 474;
        break;
    case 2:
        shield_num = 475;
        break;
    case 3:
        shield_num = 476;
        break;
    case 4:
        shield_num = 477;
        break;
    case 5:
        shield_num = 478;
        break;
    case 6:
        shield_num = 479;
        break;
    case 7:
        if (!get_componentB
            (ch, SPHERE_CONJURATION, level,
             "$p shatters in your hands and falls to the ground, heating it up.",
             "$p shatters in $n's hands and falls to the ground.",
             "You do not have a powerful enough component to do that."))
            return;

        shield_num = 1460 + number(0, 7);       /* random object 1460 to 1467 */
        tmp_obj = read_object(shield_num, VIRTUAL);

        tmp_obj->obj_flags.value[2] = get_char_size(victim);

	set_creator_edesc(ch, tmp_obj);

        obj_to_char(tmp_obj, ch);

        act("A sudden mist forms in the air, gathering into $n's hands to become $p.", TRUE, ch,
            tmp_obj, 0, TO_ROOM);
        act("You call up a mist from the air, gathering it into $p.", TRUE, ch, tmp_obj, 0,
            TO_CHAR);

        return;
    default:
        gamelog("UNKNOWN POWER LEVEL IN SPELL_SHIELD_OF_MIST()");
    };

    tmp_obj = read_object(shield_num, VIRTUAL);

    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);


    if (victim && (victim != ch)) {
        tmp_obj->obj_flags.value[2] = get_char_size(victim);
        act("A sudden mist forms in the air, gathering into $n's hands to form $p, which $s drops at $N's feet.", TRUE, ch, tmp_obj, victim, TO_NOTVICT);
        act("You call up a mist from the air, gathering it into $p, which you drop at $N's feet.",
            TRUE, ch, tmp_obj, victim, TO_CHAR);
        act("A sudden mist forms in the air, gathering into $n's hands to form $p, which $s drops at your feet.", TRUE, ch, tmp_obj, victim, TO_VICT);
        obj_to_room(tmp_obj, ch->in_room);
    } else {
        tmp_obj->obj_flags.value[2] = get_char_size(ch);
        act("A sudden mist forms in the air, gathering into $n's hands to become $p.", TRUE, ch,
            tmp_obj, 0, TO_ROOM);
        act("You call up a mist from the air, gathering it into $p.", TRUE, ch, tmp_obj, 0,
            TO_CHAR);
        obj_to_char(tmp_obj, ch);
    }

    new_event(EVNT_SHIELDOFMIST_REMOVE, (((level + 1) * 3000) / 2), 0, 0, tmp_obj, 0, 0);

    return;
}                               /* end Spell_Shield_Of_Mist */


/* SHIELD OF Wind: 
 *  whira dreth grol
 */
void
spell_shield_of_wind(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                     OBJ_DATA * obj)
{                               /* begin Spell_Shield_Of_Wind */
    int shield_num = 0;
    OBJ_DATA *tmp_obj;

    if (!ch) {
        gamelog("No ch in shield of wind");
        return;
    }

    check_crim_cast(ch);

    if ((ch->in_room->sector_type == SECT_EARTH_PLANE)
        || ((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))) {
        send_to_char("You are unable to reach Whira from here to create a shield.\n\r", ch);
        return;
    }

    switch (level) {
    case 1:
        shield_num = 481;
        break;
    case 2:
        shield_num = 482;
        break;
    case 3:
        shield_num = 483;
        break;
    case 4:
        shield_num = 484;
        break;
    case 5:
        shield_num = 485;
        break;
    case 6:
        shield_num = 486;
        break;
    case 7:
        if (!get_componentB
            (ch, SPHERE_CONJURATION, level,
             "$p shatters in your hands and falls to the ground, heating it up.",
             "$p shatters in $n's hands and falls to the ground.",
             "You do not have a powerful enough component to do that."))
            return;

        shield_num = 1440 + number(0, 7);       /* random object 1440 to 1447 */
        tmp_obj = read_object(shield_num, VIRTUAL);

        tmp_obj->obj_flags.value[2] = get_char_size(victim);

	set_creator_edesc(ch, tmp_obj);

        obj_to_char(tmp_obj, ch);

        act("A light breeze swirls around $n forming into $p.", TRUE, ch, tmp_obj, 0, TO_ROOM);
        act("A light breeze swirls around you forming into $p.", TRUE, ch, tmp_obj, 0, TO_CHAR);

        return;
    default:
        gamelog("UNKNOWN POWER LEVEL IN SPELL_SHIELD_OF_WIND()");
    }

    tmp_obj = read_object(shield_num, VIRTUAL);

    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);

    if (victim && (victim != ch)) {
        tmp_obj->obj_flags.value[2] = get_char_size(victim);
        act("A light breeze swirls around $n forming into $p, which $s drops at $N's feet.", TRUE,
            ch, tmp_obj, victim, TO_NOTVICT);
        act("A light breeze swirls around you forming into $p, which you drop at $N's feet.", TRUE,
            ch, tmp_obj, victim, TO_CHAR);
        act("A light breeze swirls around $n form into $p, which $s drops at your feet.", TRUE, ch,
            tmp_obj, victim, TO_VICT);
        obj_to_room(tmp_obj, ch->in_room);
    } else {
        tmp_obj->obj_flags.value[2] = get_char_size(ch);
        act("A light breeze swirls around $n's forming into $p.", TRUE, ch, tmp_obj, 0, TO_ROOM);
        act("A light breeze swirls around you forming into $p.", TRUE, ch, tmp_obj, 0, TO_CHAR);
        obj_to_char(tmp_obj, ch);
    }

    new_event(EVNT_SHIELDOFWIND_REMOVE, (((level + 1) * 3000) / 2), 0, 0, tmp_obj, 0, 0);

    return;
}                               /* end Spell_Shield_Of_Wind */


/* HEALING MUD: 
 *  vivadu nathro viod
 */
void
spell_healing_mud(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Healing_Mud */
    int mud_num = 0;
    OBJ_DATA *tmp_obj;

    if (!ch) {
        gamelog("No ch in healing mud");
        return;
    }

    check_crim_cast(ch);

    switch (level) {
    case 1:
        mud_num = 340;
        break;
    case 2:
        mud_num = 341;
        break;
    case 3:
        mud_num = 342;
        break;
    case 4:
        mud_num = 343;
        break;
    case 5:
        mud_num = 344;
        break;
    case 6:
        mud_num = 345;
        break;
    case 7:
        mud_num = 346;
        break;
    default:
        gamelog("UNKNOWN POWER LEVEL IN SPELL_HEALING_MUD()");
    };

    tmp_obj = read_object(mud_num, VIRTUAL);

    MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);

    if (victim && (victim != ch)) {
        act("$n breathes on a handful of earth turning it into $p, which $s drops at $N's feet.",
            TRUE, ch, tmp_obj, victim, TO_NOTVICT);
        act("You breath on a handful of earth turning it into $p, which you drop at $N's feet.",
            TRUE, ch, tmp_obj, 0, TO_CHAR);
        act("$n breathes on a handful of earth turning it into $p, which $s drops at your feet.",
            TRUE, ch, tmp_obj, 0, TO_VICT);
        obj_to_room(tmp_obj, ch->in_room);
    } else {
        act("$n breathes on a handful of earth turning it into $p.", TRUE, ch, tmp_obj, 0, TO_ROOM);
        act("You breath on a handful of earth turning it into $p.", TRUE, ch, tmp_obj, 0, TO_CHAR);
        obj_to_char(tmp_obj, ch);
    }

    new_event(EVNT_HEALINGMUD_REMOVE, (24 + (number(1, 6))) * 60, 0, 0, tmp_obj, 0, 0);

    return;
}                               /* end Spell_Healing_Mud */



/* SHIELD OF NILAZ: Makes the recipient immune to any spell attacks, 
 * or psionics.  Also makes them unable to cast or use psionics. 
 * Non-cumulative.  Nilazi spell.  Nilaz divan grol.
 */
void
spell_shield_of_nilaz(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                      OBJ_DATA * obj)
{                               /* begin Spell_Shield_Of_Nilaz */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in shield of nilaz");
        return;
    }

    check_criminal_flag(ch, NULL);

    if ((ch->in_room->sector_type == SECT_FIRE_PLANE)
        || (ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_AIR_PLANE)
        || (ch->in_room->sector_type == SECT_EARTH_PLANE)
        || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)) {
        send_to_char("You are unable to call upon Nilaz's shield from here.\n\r", ch);
        return;
    }

    if (ch->in_room->sector_type == SECT_NILAZ_PLANE)
        duration = number(level, ((level + 1) * 2)) * 2;
    else
        duration = number(level, ((level + 1) * 2));

    if (affected_by_spell(victim, SPELL_SHIELD_OF_NILAZ)) {
        send_to_char("Nothing happens.\n\r", ch);
/* gamelog ("Awooga.  Cast Shield of Nilaz while inside a Shield of Nilaz."); */
        return;
    }

    if (victim == ch) {
        act("A greyish bubble surrounds your body, quicking fading to transparency.", FALSE, ch, 0,
            0, TO_CHAR);
        act("A greyish bubble forms around $n, then fades from view.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("You send a greyish bubble to surround $N that quickly fades to transparency.", FALSE,
            ch, 0, victim, TO_CHAR);
        act("$n sends a greyish bubble to surround you that quickly fades to transparency.", FALSE,
            ch, 0, victim, TO_VICT);
        act("$n sends a greyish bubble to surround $N that quickly fades to transparency.", TRUE,
            ch, 0, victim, TO_NOTVICT);
    }

    af.type = SPELL_SHIELD_OF_NILAZ;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

}                               /* end Spell_Shield_Of_Nilaz */


/* SHOW THE PATH: This spell, when cast with a target, displays the path
 * between the caster and that target (if possible).  The power level of
 * the spell determines how far away from the caster the code will search
 * for the target.  Therefore, it may return no path to the caster even
 * when there is a path available, because the distance between the caster
 * and the target is greater than the maximum allowed by the power level.
 * Ruk nathro inrof.
 */
void
spell_show_the_path(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Show_The_Path */
    char buf[MAX_STRING_LENGTH];
    int max_distance;

    if (!victim || !victim->in_room || (ch && !ch->in_room)) {
        /* No victim specified or
         * the victim isn't in a room (unlikely) or
         * the caster isn't in a room (unlikely) */
        send_to_char("Nothing seems to happen.\n\r", ch);
        return;
    }

    /* Set how far the code will search around the caster for the victim */
    switch (level) {
    case 7:                    /* mon */
        max_distance = 7000;
        break;
    case 6:                    /* sul */
        max_distance = 60;
        break;
    case 5:                    /* pav */
        max_distance = 50;
        break;
    case 4:                    /* een */
        max_distance = 40;
        break;
    case 3:                    /* kral */
        max_distance = 30;
        break;
    case 2:                    /* yuqa */
        max_distance = 20;
        break;
    case 1:                    /* wek */
        max_distance = 10;
        break;
    default:
        max_distance = 1;
        break;
    }

    /* See if the victim is within a max_distance range from the caster */
    if (path_from_to_for(ch->in_room, victim->in_room, max_distance, MAX_STRING_LENGTH, buf, victim)
        == -1) {
        /* There wasn't a path found */
        send_to_char("You are unable to sense the path.\n\r", ch);
        return;
    } else {
        send_to_char("The path is:\n\r  ", ch);
        strcat(buf, "\n\r");
        send_to_char(buf, ch);
    }

    return;
}                               /* end Spell_Show_The_Path */


/* SILENCE: Renders the target unable to speak, for a
 * duration dependent on power level.  Non-cumulative.
 * Blocked by the Tongues spell, but cancels it out.
 * Undead and Immortals immune.
 * Vivaduan spell.  Vivadu psiak hurn.
 */
void
spell_silence(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Silence */
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    check_criminal_flag(ch, NULL);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (IS_IMMORTAL(victim)) {
        act("$N appears only slightly amused.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n apparently tried to silence you. You barely take notice.", FALSE, ch, 0, victim,
            TO_VICT);
        return;
    }

    if (IS_AFFECTED(victim, CHAR_AFF_SILENCE)) {
        act("$N is already unable to speak.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
        act("Your tongue tingles slightly.\n\r", TRUE, victim, 0, 0, TO_CHAR);
        act("$n's tongue twitches, then return to normal.\n\r", TRUE, victim, 0, 0, TO_ROOM);
        return;
    }

    if (affected_by_spell(victim, SPELL_TONGUES)) {
        affect_from_char(victim, SPELL_TONGUES);
        act("You try to silence $N but something blocks it.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n tries to silence you but your enchanted tongue blocks it.", FALSE, ch, 0, victim,
            TO_VICT);
        act("$n tries to silence $N but something blocks it.", TRUE, ch, 0, victim, TO_NOTVICT);
        return;
    }

    af.type = SPELL_SILENCE;
    af.duration = level;
    af.power = level;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_NONE;
    af.modifier = 0;
    af.bitvector = CHAR_AFF_SILENCE;

    affect_to_char(victim, &af);

    if (victim == ch) {
        act("You clutch your mouth, no longer able to speak.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n clutches $s mouth, no longer able to speak.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("You put your finger to your lips and $N clutches $S mouth, no longer able to speak.",
            FALSE, ch, 0, victim, TO_CHAR);
        act("$n puts $s finger to $s lips and you clutch your mouth, no longer able to speak.",
            FALSE, ch, 0, victim, TO_VICT);
        act("$n puts $s finger to $s lips and $N clutches $S mouth, no longer able to speak.", TRUE,
            ch, 0, victim, TO_NOTVICT);
    }
}                               /* end Spell_Silence */


/* SLEEP: Puts victim to sleep, duration dependent on power level.
 * Having a barrier up will save them from it, as will having the 
 * insomnia spell cast on them.  Mantis and vampire
 * are immune to this spell.  Rukkian and sorcerer spell.
 * Ruk psiak hurn.
 */
void
spell_sleep(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Sleep */
    struct affected_type af;
    CHAR_DATA *temp;
    char buf[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in sleep");
        return;
    }

    check_criminal_flag(ch, NULL);


    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(victim, CHAR_AFF_PSI) && affected_by_spell(victim, PSI_BARRIER)) {
        if (victim != ch)
            act("Your victim seems unaffected by your spell.", FALSE, ch, 0, 0, TO_CHAR);
        act("You feel a tingling in your mental barrier.", FALSE, victim, 0, 0, TO_CHAR);
        return;
    }

/* mantis and vampires unaffected by sleep spell -sv */
    if ((GET_RACE(victim) == RACE_MANTIS) || (GET_RACE(victim) == RACE_VAMPIRE)) {
        if (victim != ch)
            act("Your victim seems unaffected by your spell.", FALSE, ch, 0, 0, TO_CHAR);
        act("You feel a wave of lethargy wash over you for a moment.", FALSE, victim, 0, 0,
            TO_CHAR);
        return;
    }

    if (affected_by_spell(victim, SPELL_SLEEP)) {
        send_to_char("But they are already dead to the world.\n\r", ch);
        return;
    }

    if (affected_by_spell(victim, SPELL_INSOMNIA)) {
        if (victim != ch)
            act("Your spell fuses around $N, and a great popping sound is heard!", FALSE, ch, 0,
                victim, TO_CHAR);
        act("You feel lightheaded, as if from a struggle, and you hear a loud" " pop.", FALSE, ch,
            0, victim, TO_VICT);
        act("Magickal swirls disappear around $n with a loud pop.", FALSE, victim, 0, victim,
            TO_ROOM);

        affect_from_char(victim, SPELL_INSOMNIA);
        return;
    }

    if (affected_by_spell(victim, SPELL_SHIELD_OF_NILAZ)) {
        act("Your spell fuses around $N, and then dissipates.", FALSE, ch, 0, victim, TO_CHAR);
        act("You feel a cold sensation, which passes quickly.", FALSE, ch, 0, victim, TO_VICT);
        act("Magickal swirls disappear around $n.", FALSE, victim, 0, victim, TO_ROOM);

        return;
    }

    /*  I took this out, for some reason players were _always_ saveing with
     * a roll of 95, which no spell can overcome -Nessalin 2/3/97
     * if(!does_save(victim, SAVING_SPELL, wis_app[GET_WIS(ch)].wis_saves)) */

    /* I altered saves once again.  Currently listed below would allow someone
     * with 15 WIS to save at 50% vs a wek, 15% vs mon.  This should also make
     * creatures such as kanks (dumb animals if there ever was one) save at
     * around 38 vs a wek (they hve, at most, 2 WIS)   -Nessalin 2/18/97 */

    if ((number(1, 100)) > (55 - (7 * level) + (2 * (GET_WIS(victim) - 15)))) {
        af.type = SPELL_SLEEP;
        af.duration = level / 2;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = 0;
        af.location = CHAR_APPLY_NONE;
        af.bitvector = CHAR_AFF_SLEEP;
        affect_join(victim, &af, FALSE, FALSE);

        /*  Stop fighting - Kelvik  */
        stop_all_fighting(victim);      // They can't fight, they're asleep
        stop_fighting(ch, victim);      // They fell asleep, leave 'em be

        for (temp = ch->in_room->people; temp; temp = temp->next)
            stop_fighting(temp, victim);        // Everyone else, leave 'em be

        if (victim->specials.riding) {
            act("$n tumbles from $r with a thump.", TRUE, victim, 0, 0, TO_ROOM);
            victim->specials.riding = NULL;
        }
        if (GET_POS(victim) > POSITION_SLEEPING) {
            if (victim == ch) {
                act("Your eyelids become leaden, and you fall into a deep sleep.", FALSE, ch, 0, 0,
                    TO_CHAR);
                act("$n collapses, falling into a deep sleep.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                act("You wipe a hand down over your eyes and $N collapses, falling into a deep sleep.", FALSE, ch, 0, victim, TO_CHAR);
                act("$n wipes a hand down over $s eyes and your eyelids become leaden as you fall into a deep sleep.", FALSE, ch, 0, victim, TO_VICT);
                act("$n wipes a hand down over $s eyes and $N collapses, falling into a deep sleep.", TRUE, ch, 0, victim, TO_NOTVICT);
            }
            set_char_position(victim, POSITION_SLEEPING);
            snprintf(buf, sizeof(buf), "%s goes to sleep.", MSTR(victim, short_descr));
            send_to_empaths(victim, buf);
            drop_weapons(victim);
        }
        return;
    } 

    // give them a message to know they were targeted
    act("You feel a wave of lethargy wash over you for a moment.", 
     FALSE, victim, 0, 0, TO_CHAR);

    if ((IS_NPC(victim)) && (!victim->specials.fighting)) {
        find_ch_keyword(ch, victim, buf, sizeof(buf));
        cmd_kill(victim, buf, CMD_KILL, 0);
    }

    return;
}                               /* end Spell_Sleep */


/* SLOW: Slows the target's movements and reduces their agility
 * for a duration dependent on power level.  Non-cumulative. 
 * Nullifies godspeed.  Elkros spell. Elkros mutur chran.
 */
void
spell_slow(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Slow */
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in slow");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (affected_by_spell(victim, SPELL_SLOW)) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    if (affected_by_spell(victim, SPELL_GODSPEED)) {
        send_to_char("Your body's rapid movements slow down to normal.\n\r", victim);
        act("$n's rapid movements slow down.", FALSE, victim, 0, 0, TO_ROOM);
        affect_from_char(victim, SPELL_GODSPEED);
        return;
    }

    if (affected_by_spell(victim, SPELL_QUICKENING)) {
        if (victim == ch) {
            act("Your body's quick movements slow down to normal.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n's body slows back down to normal.", FALSE, ch, 0, 0, TO_ROOM);
        } else {
            act("You make a quick gesture at $N and $S movements slow down normal.", FALSE, ch, 0,
                victim, TO_CHAR);
            act("$n makes a quick gesture at you and your movements slow down to normal.", FALSE,
                ch, 0, victim, TO_VICT);
            act("$n makes a quick gesture at $N and $S movements slow down to normal.", TRUE, ch, 0,
                victim, TO_NOTVICT);
        }
        affect_from_char(victim, SPELL_QUICKENING);
        return;
    }

    if (ch != victim) {
        if (!does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
            act("You make a pushing motion at the ground and $N's body seems to slow down.", FALSE,
                ch, 0, victim, TO_CHAR);
            act("$n makes a pushing motion at the ground and the movement of your entire body slows down.", FALSE, ch, 0, victim, TO_VICT);
            act("$n makes a pushing motion at the ground and $N's body seems to slow down.", TRUE,
                ch, 0, victim, TO_NOTVICT);
        } else {
            act("You gesture at $N, but nothing happens.", FALSE, ch, 0, victim, TO_CHAR);
            act("$n gestures towards you, but nothing happens.", FALSE, ch, 0, victim, TO_VICT);
            act("$n gestures at $N, but nothing happens.", FALSE, ch, 0, victim, TO_NOTVICT);
            return;
        }
    } else {
        act("The movement of your entire body slows down.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n's body seems to slow down.", FALSE, ch, 0, 0, TO_ROOM);
    }

    af.type = SPELL_SLOW;
    af.duration = level;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = -(level);
    af.location = CHAR_APPLY_AGL;
    af.bitvector = CHAR_AFF_SLOW;

    affect_to_char(victim, &af);

}                               /* end Spell_Slow */


/* SOBER: Lowers the target's level of intoxication.  
 * Cannot be cast on the undead.  Vivaduan spell.
 */
void
spell_sober(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Sober */
    if (!victim) {
        gamelog("No victim in sober");
        return;
    }

    check_crim_cast(ch);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    act("You point your finger at $N, and a mist swirls away from $M.", FALSE, ch, 0, victim,
        TO_CHAR);
    act("A fine mist swirls from your body as $n points $s finger at you.", FALSE, ch, 0, victim,
        TO_VICT);
    act("$n points $s finger at $N, as a fine mist swirls away from $M.", TRUE, ch, 0, victim,
        TO_NOTVICT);
    send_to_char("A foul taste fills your mouth and you feel more sober.\n\r", victim);

    GET_COND(victim, DRUNK) -= (level * (level * number(1, 2)));

    if (GET_COND(victim, DRUNK) < 0) {
        GET_COND(victim, DRUNK) = 0;
    }
}                               /* end Spell_Sober */


/* SOLACE: Restores life force to the Land.  Cannot be cast unless
 * the caster has a positive relationship to the land.  Cannot be
 * cast in rooms sectored CITY, AIR, or INSIDE.  Quest
 * only spell.  Nilaz viqrol wril.
 */
void
spell_solace(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Solace */
    struct weather_node *wn;
    int life;
    int debug;

    debug = 1;

    wn = NULL;     /* Initialize pointer */
    if (!ch) {
        gamelog("Null pointer for CH passed into spell_solace");
        return;
    };

    if (ch->specials.eco == 0) {
        send_to_char("Nothing Happens.", ch);
        return;
    }

    if (ch->specials.eco < 0) {
        act("You feel an awful pain inside, as the land opposes you.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n doubles over in pain.", TRUE, ch, 0, 0, TO_ROOM);
        life = ((level * ch->specials.eco) / 7);
        if (life < 0)
            life = (life * -1);
        generic_damage(ch, life, 0, 0, life * 2);
        if (GET_POS(ch) == POSITION_DEAD)
            die(ch);
    }

    if ((ch->in_room->sector_type == SECT_CITY) || (ch->in_room->sector_type == SECT_AIR)
        || (ch->in_room->sector_type == SECT_INSIDE)) {
        send_to_char("Nothing happens.\n\r", ch);
/*      send_to_char ("Check 2, sectors.\n\r", ch); */
        return;
    }

    life = number(1, level);

    /* Can't give more than you have */
    if (life > ch->specials.eco)
        life = ch->specials.eco;

    if (IS_SET(ch->in_room->room_flags, RFL_PLANT_HEAVY))
        life = number(level, (level * 3));
    if (IS_SET(ch->in_room->room_flags, RFL_PLANT_SPARSE))
        life = number(level, (level * 2));

    if (life < 1)
        act("Nothing Happens.", FALSE, ch, 0, 0, TO_CHAR);
    else {
        ch->specials.eco = MAX(0, ch->specials.eco - life);
        act("You double over in pain, as you wrench the lifeforce from your " "body.", FALSE, ch, 0,
            0, TO_CHAR);
        act("$n suddenly doubles over, clutching $s stomach in pain.", TRUE, ch, 0, 0, TO_ROOM);
        send_to_room("The land around you shimmers slightly and looks " "healthier.\n",
                     ch->in_room);
        generic_damage(ch, life, 0, 0, life);
        if (GET_POS(ch) == POSITION_DEAD)
            die(ch);

        if (IS_SET(ch->in_room->room_flags, RFL_ASH))
            REMOVE_BIT(ch->in_room->room_flags, RFL_ASH);
        else {
            for (wn = wn_list; wn; wn = wn->next)
                if (wn->zone == ch->in_room->zone)
                    break;
            if (wn) {
                wn->life = MIN(wn->life + life, 10000);
            }
        }
    }
    return;
}                               /* end Spell_Solace */


/* STALKER: Summons a creature from the elemental plane of Whira,
 * mob #475, which the caster can then dispatch to torment a target.
 * Depends on a dmpl placed upon the mob.  Requires a component.
 * Whiran spell.  Whira dreth echri.
 */
void
spell_stalker(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
/* First thing is to get the critter there, taken from the gate spell.  -San */
{
    struct affected_type af;
    int vnum;
    char buf[80];

    memset(&af, 0, sizeof(af));

    if (((ch->in_room->sector_type == SECT_NILAZ_PLANE) && (GET_GUILD(ch) != GUILD_DEFILER))
        || (ch->in_room->sector_type == SECT_EARTH_PLANE)) {
        send_to_char("You are unable to reach into Whira from here to summon a stalker.\n\r", ch);
        return;
    }

    if (!get_componentB
        (ch, SPHERE_CONJURATION, level, "$p folds in upon itself with an audible grinding noise.",
         "$p folds in upon itself in $n's hands.",
         "You lack the correct component to lure otherwordly creatures here.")) {
        return;
    }

    check_criminal_flag(ch, NULL);

    switch (GET_GUILD(ch)) {
    case GUILD_WIND_ELEMENTALIST:
        vnum = 475;
        break;
    default:
        vnum = 475;
        break;
    }

    victim = read_mobile(vnum, VIRTUAL);

    if (!victim) {
        errorlogf("spell_stalker() in magick.c unable to load NPC# %d", vnum);
        return;
    }
    sprintf(buf, "spell_stalker() in magick.c loaded NPC# %d correctly.", vnum);

    char_to_room(victim, ch->in_room);

    act("$n concentrates visibly as $N slowly shimmers into shape.", TRUE, ch, 0, victim, TO_ROOM);
    act("You focus your will as $N is brought to this plane.", TRUE, ch, 0, victim, TO_CHAR);

    add_follower(victim, ch);

/* If the charm affect is placed on the mobile, it will be 
   unable to leave its master's side, rendering the spell ineffectual.
   Accordingly, do not reinsert this.  -Sanvean  */

    af.type = SPELL_GATE;
    af.duration = 5 + (2 * level);
    af.modifier = 0;
    af.location = 0;
    af.bitvector = CHAR_AFF_SUMMONED;
    affect_join(victim, &af, TRUE, FALSE);

}                               /* end Spell_Stalker */


/* STAMINA DRAIN: Damages a player's moves for an amount
 * dependent on power level.  Elkros spell.  Elkros divan hurn.
 */
void
spell_stamina_drain(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{                               /* begin Spell_Stamina_Drain */
    int dam;

    if (!ch) {
        gamelog("No ch in stamina drain");
        return;
    }

    if (!victim) {
        gamelog("No victim in stamina drain");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    dam = dice((level * 2), 30);

    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5))))
        dam >>= 1;

    adjust_move(victim, -dam);

    if (victim == ch) {
        act("You drain your own stamina.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n makes a gesture as $e drains his own stamina.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("You point your finger at $N, draining $S stamina.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n points $s finger at you, and weakness saps your limbs.", FALSE, ch, 0, victim,
            TO_VICT);
        act("$n points $s finger at $N, who gasps and sags weakly.", TRUE, ch, 0, victim,
            TO_NOTVICT);
    }
}                               /* end Spell_Stamina_Drain */


/* STONE SKIN: Puts a layer of protective armor over the caster's
 * body, which is visible to the naked eye.  Duration dependent 
 * on power level.  Non-cumulative. Affects agility.  Rukkian
 * spell.  Krok pret grol.
 */
void
spell_stone_skin(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Stone_Skin */
    struct affected_type af;
    int duration, acmodifier;

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in stone skin");
        return;
    }

    check_crim_cast(ch);

    if (affected_by_spell(victim, SPELL_ENERGY_SHIELD)) {
        send_to_char("The energy shield around you prevents your skin from becoming harder.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_WIND_ARMOR)) {
        send_to_char("The winds surrounding your form prevent your skin from becoming harder.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
        send_to_char("The fires surrounding your form prevent your skin from becoming harder.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
        send_to_char("The shadowss surrounding your form prevent your skin from becoming harder.\n\r",
                     victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_GODSPEED)) {
        send_to_char("Your body is moving too fast for your skin to harden.\n\r", victim);
        return;
    }

    if (!affected_by_spell(victim, SPELL_STONE_SKIN)) {
        if (victim == ch) {
            act("Your skin turns to a stone-like substance.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n's skin turns grey and granite-like.", FALSE, ch, 0, 0, TO_ROOM);
        } else {
            act("You tightly grip your fist and $N's skin turns grey and granite-like.", FALSE, ch,
                0, victim, TO_CHAR);
            act("$n tightly grips $s fist and your skin turns to a stone-like substance.", FALSE,
                ch, 0, victim, TO_VICT);
            act("$n tightly grips $s fist and $N's skin turns grey and granite-like.", TRUE, ch, 0,
                victim, TO_NOTVICT);
        }

        duration = ((level * 3) + number(-5, 5));
        duration = MAX(duration, level);
        acmodifier = level * 10;

        if (ch->in_room->sector_type == SECT_EARTH_PLANE) {
            duration = duration * 2;
            acmodifier = acmodifier * 2;
            acmodifier = MIN(acmodifier, 100);
            send_to_char("Your spell is infused with greater power here.\n\r", ch);
        }

        af.type = SPELL_STONE_SKIN;
        af.duration = duration + 1;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = acmodifier;
        af.location = CHAR_APPLY_AC;
        af.bitvector = 0;
        affect_to_char(victim, &af);

        af.type = SPELL_STONE_SKIN;
        af.duration = duration + 1;
        af.modifier = (-(level / 2.0));
        af.location = CHAR_APPLY_AGL;
        af.bitvector = 0;
        affect_to_char(victim, &af);
    }
    /* target has stoneskin, and is not the caster */
    else if (victim != ch) {
        act("Their skin already has a stone-like quality.", TRUE, ch, 0, 0, TO_CHAR);
    }
    /* target has stoneskin, and IS the caster     */
    else
        act("Your skin already has a stone-like quality.", TRUE, ch, 0, 0, TO_CHAR);

}                               /* end Spell_Stone_Skin */


/* STRENGTH: Adds to the target's skin.  Duration and amount
 * added dependent on power level.  Non-cumulative. Rukkian
 * spell.  Ruk mutur wril.
 */
void
spell_strength(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Strength */
    struct affected_type af;
    int duration, strmodifier;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in strength");
        return;
    }

    check_crim_cast(ch);

    if (affected_by_spell(victim, SPELL_STRENGTH)) {
        send_to_char("No more magickal strength can be applied.\n\r", ch);
        return;
    }

    /* Times two multiplier added on march 10th, 2000 by Nessalin
     * to account for the change of stat ranges from 1-25 to 1-100 */
    strmodifier = (dice(1, MAX(1, (level + 1) / 2))) * 2;
    duration = level * 2;

    if (ch->in_room->sector_type == SECT_EARTH_PLANE) {
        duration = duration * 2;
        strmodifier = strmodifier + level;
        send_to_char("Your spell is infused with greater power here.\n\r", ch);
    }

    if (victim == ch) {
        act("Muscles all over your body begin to ripple with new strength.", FALSE, ch, 0, 0,
            TO_CHAR);
        act("$n's muscles bulge with newfound strength.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("You exhale a sandy cloud towards $N, and $S muscles bulge with newfound strength.",
            FALSE, ch, 0, victim, TO_CHAR);
        act("$n exhales a sandy cloud towards you, and your muscles bulge with newfound strength.",
            FALSE, ch, 0, victim, TO_VICT);
        act("$n exhales a sandy cloud towards $N, and $S muscles bulge with newfound strength.",
            TRUE, ch, 0, victim, TO_NOTVICT);
    }

    af.type = SPELL_STRENGTH;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = strmodifier;
    af.location = CHAR_APPLY_STR;
    af.bitvector = 0;

    affect_join(victim, &af, TRUE, FALSE);
}                               /* end Spell_Strength */


/* SUMMON: Allows the caster to move the target of the spell
 * to their own room.  Cannot be cast against templars or Whirans,
 * targets who are in rooms that are INDOORS or NO_ELEM_MAGICK. Also
 * cannot summon someone from another plane as the one you're on.
 * Requires a component.  Whiran and sorcerer spell.
 * Whira dreth wril. 
 */
void
spell_summon(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Summon */

    ROOM_DATA *target;
    /* OBJ_DATA *dreth; */
    char ch_comm[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if (!ch) {
        gamelog("No ch in summon");
        return;
    }

    if (!victim) {
        gamelog("No victim in summon");
        return;
    }

    check_crim_cast(ch);

    if (!zone_table[victim->in_room->zone].is_open) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (!is_on_same_plane(ch->in_room, victim->in_room)) {
        send_to_char("You cannot summon someone from another plane.\n\r", ch);
        return;
    }

    if (does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (!(level == 7)
        && ((choose_exit_name_for(ch->in_room, victim->in_room, ch_comm, 20 * level, ch)) == -1)) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->in_room->room_flags, RFL_NO_MAGICK)
        || IS_SET(victim->in_room->room_flags, RFL_IMMORTAL_ROOM)) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    /* See if they're inside wagon object */
    OBJ_DATA *wagon = find_exitable_wagon_for_room(victim->in_room);
    if ((IS_SET(victim->in_room->room_flags, RFL_INDOORS) && !wagon)
        || (IS_SET(victim->in_room->room_flags, RFL_NO_ELEM_MAGICK))) {
        send_to_char("They are beyond the wind's grasp.\n\r", ch);
        return;
    }

    /* If victim is a templar in their own city-state, they are protected */
    if (templar_protected(ch, victim, SPELL_SUMMON, level)) {
        return;
    }

    /* component check moved here so it only consumes on success -Morg */
    if (!get_componentB
        (ch, SPHERE_CONJURATION, level, "$p dissolves into particles and blows away.",
         "$p dissolves in $n's hands and blows away.",
         "You do not have the proper component for a summoning."))
        return;


    if (tuluk_magic_barrier(ch, victim->in_room)) {
      send_to_char("Nothing happens.\n\r", ch);
      return;
    }

    if (allanak_magic_barrier(ch, victim->in_room)) {
      if (!no_specials && special(ch, CMD_FROM_ROOM, "summon")) {
        return;
      }

      if (!IS_SET(ch->specials.act, CFL_CRIM_ALLANAK)) {
        qgamelogf(QUIET_MISC,
                  "allanak_magic_barrier: %s tried to cast summon in #%d, on %s in #%d, blocked.",
                  MSTR(ch, name),
                  ch->in_room->number,
                  MSTR(victim, name),
                  victim->in_room->number);
        send_to_char("Nothing happens.\n\r", ch);
        return;
      }

      act("$n disappears in a billowing cloud of grey dust.", TRUE, ch, 0, 0, TO_ROOM);

      target = get_room_num(50664);
      ROOM_DATA *was_in = ch->in_room;
      char_from_room(ch);
      char_to_room(ch, target);

      if (!no_specials && special(ch, CMD_TO_ROOM, "summon")) {
        char_from_room(ch);
        char_to_room(ch, was_in);
        return;
      }

      act("Something goes wrong as your surroundings fade then return with a billowing cloud of grey dust.",
          FALSE, ch, 0, 0, TO_CHAR);

      cmd_look(victim, "", 15, 0);

      return;
    }

    act("$n disappears in a billowing cloud of grey dust.", TRUE, victim, 0, 0, TO_ROOM);

    target = ch->in_room;
    char_from_room(victim);
    char_to_room(victim, target);

    act("$N arrives in a billowing cloud of grey dust next to you.",
        FALSE, ch, 0, victim, TO_CHAR);
    act("You surroundings fade then return with a billowing cloud of grey dust.",
        FALSE, ch, 0, victim, TO_VICT);
    act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
    act("$N arrives in a billowing cloud of grey dust next to $n.",
        FALSE, ch, 0, victim, TO_NOTVICT);

    cmd_look(victim, "", 15, 0);

    if ((IS_NPC(victim)) && (!victim->specials.fighting)) {
        find_ch_keyword(ch, victim, buf, sizeof(buf));
        cmd_kill(victim, buf, CMD_KILL, 0);
    }

}                               /* end Spell_Summon */


/* TELEPORT: Dependent on the function create_teleport_room.
 * When cast, it teleports the caster to a random room, unless
 * they have the DETECT_MAGICK spell on and have targeted one of
 * the rune objects.  Can only teleport to rooms on your plane.
 * i.e. If on AIR_PLANE you can only telepot to other AIR_PLANE
 * rooms.  Whiran and sorcerer spell.  Whira locror viod.
 */
void
spell_teleport(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Teleport */
    ROOM_DATA *dest_room = (ROOM_DATA *) NULL;
    ROOM_DATA *was_in;
    int roomnum, i = 0, sector, basenum;
    bool found_good_room = FALSE;

    if (!ch) {
        gamelog("No ch in teleport");
        return;
    }

    if (!victim) {
        gamelog("No victim in teleport");
        return;
    }

    if ((obj) && (obj->in_room) && (obj->obj_flags.type == ITEM_TELEPORT)) {
        if ((obj->obj_flags.type == ITEM_TELEPORT) && (obj->in_room)) {
            dest_room = obj->in_room;
            found_good_room = TRUE;
        }

        if (!is_on_same_plane(obj->in_room, ch->in_room)) {
            send_to_char("You cannot teleport to another plane.\n\r", victim);
            return;
        }
    }

    if (!dest_room) {           /* if they didn't use an object as a target */
        if (ch->in_room->zone == 34) {  /* Elemental Planes zone 34 */
            sector = ch->in_room->sector_type;
            switch (sector) {
            case SECT_EARTH_PLANE:
                basenum = 34100;
                break;
            case SECT_WATER_PLANE:
                basenum = 34200;
                break;
            case SECT_LIGHTNING_PLANE:
                basenum = 34300;
                break;
            case SECT_NILAZ_PLANE:
                basenum = 34400;
                break;
            case SECT_SHADOW_PLANE:
                basenum = 34500;
                break;
            case SECT_AIR_PLANE:
                basenum = 34600;
                break;
            case SECT_FIRE_PLANE:
                basenum = 34700;
                break;
            default:
                basenum = floor(ch->in_room->number / 100) * 100;
                break;
            }
            while ((!dest_room) || (i <= 4)) {
                roomnum = basenum + number(1, 99);
                dest_room = get_room_num(roomnum);
                i++;
            }
        } else {                    /* Not on Elemental Planes */
            for (i = 1; i <= MAX(1, (level / 2)); i++) {
                dest_room = pick_teleport_room();
                if ((dest_room) && (!IS_SET(dest_room->room_flags, RFL_IMMORTAL_ROOM)
                        && !IS_SET(dest_room->room_flags, RFL_PRIVATE)
                        && !IS_SET(dest_room->room_flags, RFL_NO_MAGICK)
                        && !IS_SET(dest_room->room_flags, RFL_DEATH)
                        && !(IS_SET(dest_room->room_flags, RFL_NO_ELEM_MAGICK)
                                && (GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST))
                        && !IS_SET(dest_room->room_flags, RFL_INDOORS))) {
                    found_good_room = TRUE;
                    break;
                } else
                    found_good_room = FALSE;
            }     /* for */
        }         /* else not on elemental plane */
    }             /* if not dest_room (didn't use obj) */
 
    check_crim_cast(ch);

    if (!found_good_room) {
        act("Something blocks your teleportation.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!dest_room) {
        act("The spell fizzles and dies.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (allanak_magic_barrier(ch, dest_room)) {
      if (IS_SET(ch->specials.act, CFL_CRIM_ALLANAK)) {
        qgamelogf(QUIET_MISC,
                  "allanak_magic_barrier: %s (criminal) tried to cast teleport in #%d, sent to arena.",
                  MSTR(ch, name),
                  ch->in_room->number);
        dest_room = get_room_num(50664);
      } else {
        qgamelogf(QUIET_MISC,
                  "allanak_magic_barrier: %s tried to cast teleport in #%d, blocked.",
                  MSTR(ch, name),
                  ch->in_room->number);
        send_to_char("Nothing happens.\n\r", ch);
        return;
      }
    }

    if (tuluk_magic_barrier(ch, dest_room)) {
      qgamelogf(QUIET_MISC,
                "tuluk_magic_barrier: %s tried to cast teleport in #%d, blocked.",
                MSTR(ch, name),
                ch->in_room->number);
      send_to_char("Nothing happens.\n\r", ch);
      return;
    }

    if (!no_specials && special(ch, CMD_FROM_ROOM, "teleport"))
        return;

    act("$n disappears in a cloud of grey, billowing dust.", FALSE, victim, 0, 0, TO_ROOM);
    act("Your surroundings begin to fade away, and then come back again...", FALSE, victim, 0, 0,
        TO_CHAR);

    was_in = victim->in_room;
    char_from_room(victim);
    char_to_room(victim, dest_room);

    if (!no_specials && special(victim, CMD_TO_ROOM, "teleport")) {
        char_from_room(victim);
        char_to_room(victim, was_in);
        return;
    }

    act("$n appears in a cloud of grey, billowing dust.", TRUE, victim, 0, 0, TO_ROOM);
    cmd_look(victim, "", 0, 0);
}                               /* end Spell_Teleport */


/* THUNDER: Creates a loud thunderclap, which has a chance
 * of deafening everyone in the room, as well as making all
 * mounts flee.  Vivaduan spell.  Vivadu divan viod.
 */
void
spell_thunder(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Thunder */
    struct affected_type af;
    CHAR_DATA *curr_ch, *next_ch;

    memset(&af, 0, sizeof(af));

    check_criminal_flag(ch, NULL);

    af.type = SPELL_DEAFNESS;
    af.duration = MAX(1, level - 3);
    af.power = level;
    af.magick_type = magick_type;
    af.location = CHAR_APPLY_AGL;
    af.modifier = -3;
    af.bitvector = 0;
    af.bitvector = CHAR_AFF_DEAFNESS;

    act("You clap your hands and the deafening sound of thunder issues forth from the sky!", FALSE,
        ch, 0, 0, TO_CHAR);
    act("$n claps $s hands and the deafening sound of thunder issues forth from the sky!", FALSE,
        ch, 0, 0, TO_ROOM);

    act("The boom of thunder comes from the $D.", FALSE, ch, 0, 0, TO_NEARBY_ROOMS);

    /* Go down this list of people in the room */
    for (curr_ch = ch->in_room->people; curr_ch; curr_ch = next_ch) {
        next_ch = curr_ch->next_in_room;

        if (curr_ch != ch) {
            /* People who don't make their save are deafened */
            if (!affected_by_spell(curr_ch, SPELL_DEAFNESS)) {
/*            if (!does_save(victim, SAVING_SPELL, 0)) */
                if (!IS_IMMORTAL(curr_ch)) {
                    affect_to_char(curr_ch, &af);
                    send_to_char("You realize with a shock that you are unable " "to hear!\n\r",
                                 curr_ch);
                }
            }

            if (((GET_RACE_TYPE(curr_ch) != RTYPE_HUMANOID)
               && (GET_RACE_TYPE(curr_ch) != RTYPE_PLANT)
               && (GET_RACE_TYPE(curr_ch) != RTYPE_OTHER)
               && (!IS_SET(curr_ch->specials.act, CFL_UNDEAD))
               && (GET_WIS (curr_ch) <= 5))
               || (IS_SET(curr_ch->specials.act, CFL_MOUNT))) {
                if (number(0, 1))
                    parse_command(curr_ch, "flee self");
            }
        }
    }
}                               /* end Spell_Thunder */


/* TONGUES: This spell calls upon the energies to enlighten the caster's
 * mind, allowing him or her to understand (but not speak) languages
 * of all sorts, for a duration dependent on power level.  Cumulative. 
 * Krathi and sorcerer spell.  Suk-Krath mutur inrof.
 */
void
spell_tongues(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Tongues */
    struct affected_type af;
    int duration;
    int variance;

    memset(&af, 0, sizeof(af));

    /* Allow the time period to vary  */
    variance = number(-level, level);
    duration = ((level * ((GET_WIS(ch)) / 5)) + variance);
    duration = MAX(duration, level);

    af.type = SPELL_TONGUES;
    af.duration = duration + 1;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;
    stack_spell_affect(victim, &af, 24);

    if (victim == ch) {
        act("Your tongue feels lighter, and tingles slightly.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n's face starts to show signs of several nervous ticks.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("You gesture at $N's mouth, and $E starts to show signs of several nervous ticks.",
            FALSE, ch, 0, victim, TO_CHAR);
        act("$n gestures at your mouth, and your tongue feels lighter, and tingles slightly.",
            FALSE, ch, 0, victim, TO_VICT);
        act("$n gestures at $N's mouth, and $E starts to show signs of several nervous ticks.",
            TRUE, ch, 0, victim, TO_NOTVICT);
    }
}                               /* end Spell_Tongues */


/* TRANSFERENCE: Allows the caster to exchange places with the
 * target.  Two components are required, one of SPHERE_TELEPORTATION
 * and one of SPHERE_ALTERATION, and they must also be in psionic
 * contact with the target.  Cannot be cast if caster or target are 
 * in a room NO_MAGICK or IMMORTAL, if caster is in room flagged
 * NO_ELEM_MAGICK, or if target is in room flagged POPULATED.  Also
 * cannot transfer with someone not on your plane of existance (i.e.
 * you are on Zalanthas, they are on Plane of Fire, it won't work).
 * Whira locror nikiz.  Whiran spell.
 */
void
spell_transference(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Transference */
    ROOM_DATA *rm1 = ch->in_room, *rm2 = victim->in_room;
    ROOM_DATA *was_rm1 = ch->in_room, *was_rm2 = victim->in_room;
    OBJ_DATA *locror, *mutur;

    locror = NULL;    /* Initialize pointers */
    mutur = NULL;

    if (GET_GUILD(ch) != GUILD_TEMPLAR) {
        if (!(locror = get_component(ch, SPHERE_TELEPORTATION, level))
            || !(mutur = get_component(ch, SPHERE_ALTERATION, level))) {
            send_to_char("You don't have the proper components for " "a transference.\n\r", ch);
            return;
        }
    }

    check_crim_cast(ch);

    if (!ch->specials.contact || (ch->specials.contact && ch->specials.contact != victim)) {
        send_to_char("You cannot cause a transference unless "
                     "your mind is linked with the victim's.\n\r", ch);
        return;
    }

    if (!(GET_GUILD(ch) == GUILD_TEMPLAR)) {
        /* Yes, only one of the components is consumed by the spell. I
         * made it this way just to add some quirks to how magick works.
         * -Nessalin 12/96 */

        act("$p glows brightly for a moment.", FALSE, ch, locror, 0, TO_CHAR);
        act("$p is consumed by the spell.", FALSE, ch, mutur, 0, TO_CHAR);
        extract_obj(mutur);
    }

    /* Can only transfer with someone on your plane of existance - Halaster */
    if (!is_on_same_plane(ch->in_room, victim->in_room)) {
        send_to_char("You cannot cause a transference with someone on another plane.\n\r", ch);
        return;
    }

    /* Just like summon... */
    if (IS_SET(victim->in_room->room_flags, RFL_INDOORS)) {
        send_to_char("They are beyond the wind's grasp.\n\r", ch);
        return;
    }

    if (IS_SET(ch->in_room->room_flags, RFL_NO_MAGICK)
        || IS_SET(victim->in_room->room_flags, RFL_NO_MAGICK)
        || IS_SET(ch->in_room->room_flags, RFL_IMMORTAL_ROOM)
        || IS_SET(victim->in_room->room_flags, RFL_IMMORTAL_ROOM)
        || IS_SET(ch->in_room->room_flags, RFL_NO_ELEM_MAGICK)
        || IS_SET(victim->in_room->room_flags, RFL_NO_ELEM_MAGICK)
        || IS_SET(victim->in_room->room_flags, RFL_POPULATED) || IS_IMMORTAL(victim)
        || !number(0, (level + 1) / 2)) {
        act("The magick fizzles and dies.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (victim != ch) {
      if (allanak_magic_barrier(ch, victim->in_room)) {
        ROOM_DATA *was_in = ch->in_room;
        
        if (!no_specials && special(ch, CMD_FROM_ROOM, "transference")) {
          return;
        }
        
        if (!IS_SET(ch->specials.act, CFL_CRIM_ALLANAK)) {
          qgamelogf(QUIET_MISC,
                    "allanak_magic_barrier: %s tried to cast transference in #%d, blocked.",
                    MSTR(ch, name),
                    ch->in_room->number);
          send_to_char("Nothing happens.\n\r", ch);
          return;
        }

        qgamelogf(QUIET_MISC,
                  "allanak_magic_barrier: %s (criminal) tried to cast transference in #%d, sent to arena.",
                  MSTR(ch, name),
                  ch->in_room->number);
        
        act("$n disappears in a cloud of grey dust.", 
            FALSE, ch, 0, 0, TO_ROOM);
        act("Grey dust overwhelms you, but soon clears away, revealing new surroundings.", 
            FALSE, ch, 0, 0, TO_CHAR);
        
        char_from_room(ch);
        char_to_room(ch, get_room_num(50664));
        
        if (!no_specials && special(ch, CMD_TO_ROOM, "relocate")) {
          char_from_room(ch);
          char_to_room(ch, was_in);
        }
        return;
      }

      if (tuluk_magic_barrier(ch, victim->in_room)) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
      }



      if (!no_specials && special(ch, CMD_FROM_ROOM, "transference"))
        return;
      
      if (!no_specials && special(victim, CMD_FROM_ROOM, "transference"))
        return;
      
      char_from_room(ch);
      char_from_room(victim);
      
      char_to_room(ch, rm2);
      
      if (!no_specials && special(ch, CMD_TO_ROOM, "relocate")) {
        char_from_room(ch);
        char_to_room(ch, was_rm1);
        char_from_room(victim);
        char_to_room(victim, was_rm2);
        return;
      }
      
      char_to_room(victim, rm1);
      
      if (!no_specials && special(victim, CMD_TO_ROOM, "relocate")) {
        char_from_room(ch);
        char_to_room(ch, was_rm1);
        char_from_room(victim);
        char_to_room(victim, was_rm2);
        return;
      }
      
      act("$n disappears in a cloud of grey dust, and $N appears in $s place.", FALSE, ch, 0,
          victim, TO_NOTVICT);
      act("$N disappears in a cloud of grey dust, and $n appears in $s place.", FALSE, ch, 0,
          victim, TO_NOTVICT);
      act("Grey dust overwhelms you, but soon clears away, revealing new " "surroundings.", FALSE,
          ch, 0, victim, TO_CHAR);
      act("Grey dust overwhelms you, but soon clears away, revealing new " "surroundings.", FALSE,
          ch, 0, victim, TO_VICT);
      
      cmd_look(ch, "", 0, 0);
      cmd_look(victim, "", 0, 0);
      
    } else
      /*  ch == victim  */
      send_to_char("You can't switch places with yourself!\n\r", ch);
}                               /* end Spell_Transference */


/* TRAVEL GATE: Allows caster to cast a spell that will take them
 * to an object or room flagged with the MARK spell.  Nilazi spell.
 * Nilaz locror chran.
 */
void
spell_travel_gate(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                  OBJ_DATA * obj)
{                               /* begin Spell_Travel_Gate */
    int d, max_size_allwd;
    OBJ_DATA *origin_portal, *dest_portal;
    ROOM_DATA *start_tunnel, *end_tunnel, *to_room, *from_room;
    char *tmp = NULL;
    struct specials_t prog;
    memset(&prog, 0, sizeof(prog));
    char buf[MAX_STRING_LENGTH];
    char tunnel_name[] = "A Grey, Bleak Hallway";
    char tunnel_desc[] = "     You are standing within a"
                        " passageway that is completely bleak and\n\rdevoid of life.  The walls"
                        " of this passage seem to be made of some type\n\rof greyish rock, but"
                        " its surface feels rough and lifeless.  The air\n\rhere is stale and hard"
                        " to breathe in.  To the south the passageway\n\rcontinues, while the rock"
                        " prevents travel in any other direction.\n\rA swirling vortex of"
                        " black light is here, providing exit from the passage.\n\r";


    check_criminal_flag(ch, NULL);

    if (obj) {
      tmp = find_ex_description("[SPELL_MARK]", obj->ex_description, TRUE);
      if (tmp) {
        to_room = get_room_num(atoi(tmp));
      }
      else {
        send_to_char("Nothing happens.\n\r", ch);
        return;
      }
    } else {
      send_to_char("Error in spell: contact an immortal about the problem.", ch);
      return;
    }
    
    if (!to_room) {
      act("$p grows warm and shatters in your hands!", FALSE, ch, obj, NULL, TO_CHAR);
      act("$p shatters in $n's hands!", FALSE, ch, obj, NULL, TO_ROOM);
      extract_obj(obj);
      return;
    }
    
    if (allanak_magic_barrier(ch, to_room)) {
      qgamelogf(QUIET_MISC,
                "allanak_magic_barrier: %s tried to cast travel gate in #%d, blocked.",
                MSTR(ch, name),
                ch->in_room->number);
      send_to_char("Nothing seems to happen.\n\r", ch);
      return;
    }

    if (tuluk_magic_barrier(ch, to_room)) {
      qgamelogf(QUIET_MISC,
                "tuluk_magic_barrier: %s tried to cast travel gate in #%d, blocked.",
                MSTR(ch, name),
                ch->in_room->number);
      send_to_char("Nothing seems to happen.\n\r", ch);
      return;
    }

    from_room = ch->in_room;

    // If the to room already has an 'exit' don't allow -Morg
    if (find_exitable_wagon_for_room(to_room)) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    // If the from room already has an 'exit' don't allow -Morg
    if (find_exitable_wagon_for_room(from_room)) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    max_size_allwd = 10 + (4 * (level - 1));

    /* build the destination portal */
    if ((dest_portal = read_object(PORTAL_OBJ, VIRTUAL)) == NULL) {
        send_to_char("Error in spell: contact an immortal about the problem.", ch);
        return;
    }

    dest_portal->obj_flags.value[2] = -1;
    dest_portal->obj_flags.cost = level;
    MUD_SET_BIT(dest_portal->obj_flags.value[3], WAGON_NO_PSI_PATH);
    obj_to_room(dest_portal, to_room);
    if (to_room->people) {
        act("$p swirls into existence.", FALSE, to_room->people, dest_portal, NULL, TO_ROOM);
        act("$p swirls into existence.", FALSE, to_room->people, dest_portal, NULL, TO_CHAR);
    }

    /* Check to see if the caster is already inside a travel gate hallway */
    if (from_room->zone == 73 && from_room->sector_type == SECT_NILAZ_PLANE
        && !strcmp(from_room->name, tunnel_name)) {
	// Set the portal to the current room
        dest_portal->obj_flags.value[0] = from_room->number;
        dest_portal->obj_flags.value[5] = max_size_allwd;

	// Update the description for the exit
        if (from_room->description)
	    free(from_room->description);
	from_room->description = (char *) strdup(tunnel_desc);
	// There's air again, stop suffocating
        unset_rprog(ch, "C pulse closed_travel_gate_suffocation");
	return;
    }

    /* build the origin portal */
    if ((origin_portal = read_object(PORTAL_OBJ, VIRTUAL)) == NULL) {
        send_to_char("Error in spell: contact an immortal about the problem.", ch);
        return;
    }

    origin_portal->obj_flags.value[2] = -1;
    origin_portal->obj_flags.cost = level;
    MUD_SET_BIT(origin_portal->obj_flags.value[3], WAGON_NO_PSI_PATH);
    obj_to_room(origin_portal, ch->in_room);
    act("$n makes several quick circular gestures and $p swirls into existence.", FALSE, ch,
        origin_portal, NULL, TO_ROOM);
    act("You make several quick circular gestures and $p swirls into existence.", FALSE, ch,
        origin_portal, NULL, TO_CHAR);

    if (!(start_tunnel = find_reserved_room())) {
        cprintf(ch, "Error in spell, please see an Overlord.\n");
        gamelog("Error in travel_gate spell:  No reserved rooms available.");
        return;
    }

    origin_portal->obj_flags.value[0] = start_tunnel->number;

    if (!(end_tunnel = find_reserved_room())) {
        cprintf(ch, "Error in spell, please see an Overlord.\n");
        gamelog("Error in travel_gate spell:  No reserved rooms available.");
        return;
    }

    dest_portal->obj_flags.value[0] = end_tunnel->number;

    origin_portal->obj_flags.value[5] = max_size_allwd;
    dest_portal->obj_flags.value[5] = max_size_allwd;

    /* Create start_tunnel room */
    /* Tier, I made the room name conform to the rest of the mud */
    /* by capitalizing the words, change back if you were aiming */
    /* for some deliberate effect.  -San                         */

    if (start_tunnel->name)
        free(start_tunnel->name);
    start_tunnel->name = (char *) strdup(tunnel_name);
    if (start_tunnel->description)
        free(start_tunnel->description);
    start_tunnel->description = (char *) strdup(tunnel_desc);

    /* put the travel_gate_exit program on the exit */
    strcpy(buf, "javascript enter travel_gate_exit");
    char_from_room(ch);
    char_to_room(ch, start_tunnel);
    set_rprog(ch, buf);

    for (d = 0; d < 6; d++) {
        if (start_tunnel->direction[d]) {
            free(start_tunnel->direction[d]->general_description);
            free(start_tunnel->direction[d]->keyword);
            free((char *) start_tunnel->direction[d]);
            start_tunnel->direction[d] = NULL;
        }
    }

    for (d = 0; d < 6; d++) {
        if (end_tunnel->direction[d]) {
            free(end_tunnel->direction[d]->general_description);
            free(end_tunnel->direction[d]->keyword);
            free((char *) end_tunnel->direction[d]);
            end_tunnel->direction[d] = NULL;
        }
    }

    start_tunnel->sector_type = SECT_NILAZ_PLANE;
    MUD_SET_BIT(start_tunnel->room_flags, RFL_INDOORS);
    MUD_SET_BIT(start_tunnel->room_flags, RFL_NO_ELEM_MAGICK);

    CREATE(start_tunnel->direction[DIR_NORTH], struct room_direction_data, 1);
    start_tunnel->direction[DIR_NORTH]->general_description = NULL;
    start_tunnel->direction[DIR_NORTH]->keyword = NULL;
    start_tunnel->direction[DIR_NORTH]->exit_info = 0;
    start_tunnel->direction[DIR_NORTH]->key = -1;
    start_tunnel->direction[DIR_NORTH]->to_room_num = end_tunnel->number;
    start_tunnel->direction[DIR_NORTH]->to_room = end_tunnel;

    /* Create end_tunnel room */
    if (end_tunnel->name)
        free(end_tunnel->name);
    end_tunnel->name = (char *) strdup(tunnel_name);
    if (end_tunnel->description)
        free(end_tunnel->description);
    end_tunnel->description = (char *) strdup(tunnel_desc);
    end_tunnel->sector_type = SECT_NILAZ_PLANE;
    MUD_SET_BIT(end_tunnel->room_flags, RFL_INDOORS);
    MUD_SET_BIT(end_tunnel->room_flags, RFL_NO_ELEM_MAGICK);

    char_from_room(ch);
    char_to_room(ch, end_tunnel);
    set_rprog(ch, buf);

    char_from_room(ch);
    char_to_room(ch, from_room);

    CREATE(end_tunnel->direction[DIR_SOUTH], struct room_direction_data, 1);
    end_tunnel->direction[DIR_SOUTH]->general_description = NULL;
    end_tunnel->direction[DIR_SOUTH]->keyword = NULL;
    end_tunnel->direction[DIR_SOUTH]->exit_info = 0;
    end_tunnel->direction[DIR_SOUTH]->key = -1;
    end_tunnel->direction[DIR_SOUTH]->to_room_num = start_tunnel->number;
    end_tunnel->direction[DIR_SOUTH]->to_room = start_tunnel;

    save_rooms(&zone_table[RESERVED_Z], RESERVED_Z);

    // See if the mark remains
    mark_spell_counter(obj, ch);
}                               /* end Spell_Travel_Gate */


/* TURN ELEMENT: Creates a field around the caster that will
 * turn aside elemental magicks, for a duration dependent on
 * power level.  Non-cumulative.  Nilazi spell.  Nilaz pret
 * grol.
 * Note: I am not convinced this spell works as the helpfile
 * says it should, have asked Tlaloc to verify.  -Sanvean
 */
void
spell_turn_element(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{                               /* begin Spell_Turn_Element */
    int duration;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in turn element");
        return;
    }

    check_criminal_flag(ch, NULL);

/*    if (ch != victim) {
        send_to_char("This spell can only be cast upon yourself.\n\r", ch);
        return;
    }
*/

    if ((ch->in_room->sector_type == SECT_FIRE_PLANE)
        || (ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_AIR_PLANE)
        || (ch->in_room->sector_type == SECT_EARTH_PLANE)
        || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)) {
        send_to_char("You are unable to call upon Nilaz for this spell from here.\n\r", ch);
        return;
    }

    duration = number(level, ((level + 1) * 2));

    if (is_guild_elementalist(GET_GUILD(victim))
        && (GET_GUILD(victim) != GUILD_VOID_ELEMENTALIST)) {    /*any elementalist except nilazis */
        if (victim == ch) {
            act("A grey aura surrounds you briefly, but then fades uselessly, unable to take hold.",
                FALSE, ch, 0, 0, TO_CHAR);
            act("A grey aura appears briefly around $n, then fades uselessly, unable to take hold.",
                FALSE, ch, 0, 0, TO_ROOM);
        } else {
            act("A grey aura leaves your hands and briefly surrounds $N, but then fades uselessly.",
                FALSE, ch, 0, victim, TO_CHAR);
            act("A grey aura leaves $n's hands and briefly surrounds you, but then fades uselessly.", FALSE, ch, 0, victim, TO_VICT);
            act("A grey aura leaves $n's hands and briefly surrounds $N, but then fades uselessly.",
                TRUE, ch, 0, victim, TO_NOTVICT);
        }
        return;
    }

    if (affected_by_spell(victim, SPELL_TURN_ELEMENT)) {
        send_to_char("Nothing happens.\n\r", ch);
        return;
    }

    if (victim == ch) {
        act("A grey aura surrounds you briefly, leaving a greasy sheen on your skin.", FALSE, ch, 0,
            0, TO_CHAR);
        act("A grey aura appears briefly around $n, then fades into $s skin.", FALSE, ch, 0, 0,
            TO_ROOM);
    } else {
        act("A grey aura leaves your hands and appears around $N, then fades into $S skin.", FALSE,
            ch, 0, victim, TO_CHAR);
        act("A grey aura leaves $n's hands and appears around you, leaving a greasy sheen on your skin.", FALSE, ch, 0, victim, TO_VICT);
        act("A grey aura leaves $n's hands and appears around $N, then fades into $S skin.", TRUE,
            ch, 0, victim, TO_NOTVICT);
    }

    af.type = SPELL_TURN_ELEMENT;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

}                               /* end Spell_Turn_Element */


/* VAMPIRIC BLADE: This spell will add a javascript "vampiric_blade_spell" to
 * any non-bludgeoning weapon that is made of bone, chitin, horn, ivory, duskhorn,
 * or tortoiseshell.  It will cause the weapon to decay into nothing based on a
 * duration affected by power level.  The javascript itself will heal damage to
 * the user when it deals damage to an opponent.
 * Void Elementalist spell.  Nilaz psiak hekro.
 */
void
spell_vampiric_blade(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                     OBJ_DATA * obj)
{                               /* begin Vampiric Blade */

    char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
    char ename[MAX_STRING_LENGTH], edesc[MAX_STRING_LENGTH];
    bool had_pulse = FALSE;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (find_ex_description("[SPELL_VAMPIRIC_BLADE]", obj->ex_description, TRUE)) {
        act("$p is already affected by the decaying magick.", FALSE, ch, obj, 0, TO_CHAR);
        return;
    }

    if (isnamebracketsok("[fire_jambiya]", OSTR(obj, name))) {
        act("$p's fires extinguish in a puff of black smoke.", FALSE, ch, obj, 0, TO_ROOM);
        act("$p's fires extinguish in a puff of black smoke.", FALSE, ch, obj, 0, TO_CHAR);
        extract_obj(obj);
        return;
    }

    if (isnamebracketsok("[sand_jambiya]", OSTR(obj, name))) {
        act("$p dissolves into filthy, black sand.", FALSE, ch, obj, 0, TO_ROOM);
        act("$p dissolves into filthy, black sand.", FALSE, ch, obj, 0, TO_CHAR);
        extract_obj(obj);
        return;
    }

    if (isnamebracketsok("[lightning_whip]", OSTR(obj, name))) {
        act("$p's sparks fade into nothing.", FALSE, ch, obj, 0, TO_ROOM);
        act("$p's sparks fade into nothing.", FALSE, ch, obj, 0, TO_CHAR);
        extract_obj(obj);
        return;
    }

    if (isnamebracketsok("[shadow_sword]", OSTR(obj, name))) {
        act("$p's shadows flitter into nothing.", FALSE, ch, obj, 0, TO_ROOM);
        act("$p's shadows flitter into nothing.", FALSE, ch, obj, 0, TO_CHAR);
        extract_obj(obj);
        return;
    }

    if (obj->obj_flags.type != ITEM_WEAPON) {
        act("$p is not a weapon.", FALSE, ch, obj, 0, TO_CHAR);
        return;
    }

    if (!((obj->obj_flags.value[3] == (TYPE_PIERCE - 300))
          || (obj->obj_flags.value[3] == (TYPE_CHOP - 300))
          || (obj->obj_flags.value[3] == (TYPE_SLASH - 300))
          || (obj->obj_flags.value[3] == (TYPE_STAB - 300))
          || (obj->obj_flags.value[3] == (TYPE_KNIFE - 300))
          || (obj->obj_flags.value[3] == (TYPE_RAZOR - 300)))) {
        act("Your magicks can only be cast on a weapon with a sharp edge.", FALSE, ch, 0, 0,
            TO_CHAR);
        return;
    }

    if ((obj->obj_flags.material != MATERIAL_BONE)
        && (obj->obj_flags.material != MATERIAL_CHITIN)
        && (obj->obj_flags.material != MATERIAL_HORN)
        && (obj->obj_flags.material != MATERIAL_IVORY)
        && (obj->obj_flags.material != MATERIAL_DUSKHORN)
        && (obj->obj_flags.material != MATERIAL_TORTOISESHELL)) {
        act("Your magicks cannot be cast on that type of material.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    act("$n breathes black ash onto $p and it starts to slowly decay.", TRUE, ch, obj, 0, TO_ROOM);
    act("You breathe black ash onto $p and it starts to slowly decay.", TRUE, ch, obj, 0, TO_CHAR);

    strcpy(ename, "[SPELL_VAMPIRIC_BLADE]");
    sprintf(edesc, "%d", 1);
    set_obj_extra_desc_value(obj, ename, edesc);

    strcpy(ename, "[POWER_LEVEL]");
    sprintf(edesc, "%d", level);
    set_obj_extra_desc_value(obj, ename, edesc);

    new_event(EVNT_VAMPIRICBLADE_REMOVE, (level * 600), 0, 0, obj, 0, 0);

    strcpy(buf, "javascript on_damage vampiric_blade_spell");
    strcpy(buf1, "javascript pulse vampiric_blade_spell");
    had_pulse = has_special_on_cmd(obj->programs, NULL, CMD_PULSE);
    obj->programs = set_prog(NULL, obj->programs, buf);
    obj->programs = set_prog(NULL, obj->programs, buf1);
    if (!had_pulse && has_special_on_cmd(obj->programs, NULL, CMD_PULSE))
        new_event(EVNT_OBJ_PULSE, PULSE_OBJECT, 0, 0, obj, 0, 0);

    /* Add a 2% offense bonus per power level if available slot */
    if (obj->affected[0].location == 0) {
        obj->affected[0].location = 18;
        obj->affected[0].modifier = level * 2;
    } else {
        if (obj->affected[1].location == 0) {
            obj->affected[1].location = 18;
            obj->affected[1].modifier = level * 2;
        }
    }

    af.type = TYPE_HUNGER_THIRST;;
    af.duration = level + 3;
    af.modifier = -1;
    af.location = CHAR_APPLY_END;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return;
}                               /* vampiric blade */


/* Note about the wall spells.  All of these are stored in spells.c
 * and what exists here are simply placeholders.  See Nessalin before
 * tampering with any of them.  For easy reference, here is who gets
 * them, and the words.  -Sanvean
 * Blade barrier      Nilazi, Sorcerers         Nilaz mutur grol
 * Wall of fire       Krathi, sorcerers         Suk-Krath mutur grol
 * Wall of sand       Rukkians, sorcerers       Ruk mutur grol
 * Wall of thorns     Vivaduans                 Vivadu mutur grol
 * Wall of wind       Whirans	                 Whira mutur viod
 */

void
spell_wall_of_fire(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{
    /* Hi there.  -Nessalin */
}
void
spell_wall_of_sand(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{
    /* Hi there.  -Nessalin */
}

void
spell_wall_of_thorns(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                     OBJ_DATA * obj)
{
    /* Hi there.  -Nessalin */
}

void
spell_wall_of_wind(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{
    /* Hi there.  -Nessalin */
}

/* WEAKEN: Causes the victim to suffer a loss in strength proportional 
 * to the spell's power level. Non-cumulative.  Rukkian spell.
 * Ruk morz hurn.
 */
void
spell_weaken(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Weaken */
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!ch) {
        gamelog("No ch in weaken");
        return;
    }

    if (!victim) {
        gamelog("No victim in weaken");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (IS_SET(victim->specials.act, CFL_UNDEAD)) {
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!affected_by_spell(victim, SPELL_WEAKEN))
        if (!does_save(victim, SAVING_SPELL, ((level - 4) * (-5)))) {
            if (victim == ch) {
                act("You feel weaker.", FALSE, ch, 0, 0, TO_CHAR);
                act("$n appears suddenly weaker.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                act("You slowly relax your fist and $N appears suddenly weaker.", FALSE, ch, 0,
                    victim, TO_CHAR);
                act("$n slowly realxes $s fist and you feel suddenly weaker.", FALSE, ch, 0, victim,
                    TO_VICT);
                act("$n slowly relaxes $s fist and $N appears suddenly weaker.", TRUE, ch, 0,
                    victim, TO_NOTVICT);
            }

            af.type = SPELL_WEAKEN;
            af.duration = MAX(1, level / 2);
            af.power = level;
            af.magick_type = magick_type;
            /* Times two multiplier added on march 10th, 2000 by Nessalin
             * to account for the stat range going from 1-25, to 1-100 */
            af.modifier = (((level / -2) - 1)) * 2;
            af.location = CHAR_APPLY_STR;
            af.bitvector = 0;

            affect_to_char(victim, &af);
        }
}                               /* end Spell_Weaken */



/* WIND ARMOR: Creates a shield of wind around the caster that will
 * defend them from attacks, including missile attacks.  Duration
 * dependent on power level.  Non-cumulative.  Whiran spell.
 * Whira pret grol.
 */
void
spell_wind_armor(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* Begin Spell_Wind_Armor */
    struct affected_type af;
    struct affected_type *tmp_af;
    int duration;
    int sector;

    memset(&af, 0, sizeof(af));

    if (!victim) {
        gamelog("No victim in wind armor");
        return;
    }

    check_crim_cast(ch);

    if (affected_by_spell(victim, SPELL_ENERGY_SHIELD)) {
        send_to_char("The sparks already surrounding your form " "disperse the winds.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_STONE_SKIN)) {
        send_to_char("The winds batter against into your stony skin and " "disappear.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
        send_to_char("The shadows around you prevent the winds from taking hold.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
        send_to_char("The fires around you prevent the winds from taking hold.\n\r", victim);
        return;
    }

    if (affected_by_spell(victim, SPELL_WIND_ARMOR)) {
        act("The winds surrounding your form swirl with renewed energy.", FALSE, victim, 0, 0,
            TO_CHAR);
        act("The winds surrounding $N's form swirl with new intensity.", FALSE, victim, 0, victim,
            TO_NOTVICT);
    } else {
        sector = ch->in_room->sector_type;
        switch (sector) {
        case SECT_AIR:
            act("From the air around you, winds swirl, surrounding your body.", FALSE, victim, 0, 0,
                TO_CHAR);
            act("Around $N, winds swirl, coalescing around $S form.", FALSE, victim, 0, victim,
                TO_NOTVICT);
            break;
        case SECT_CAVERN:
            act("Breezes batter against the stony walls as winds surround your body.", FALSE,
                victim, 0, 0, TO_CHAR);
            act("Breezes batter against the stony walls as winds surround $N's form.", FALSE,
                victim, 0, victim, TO_NOTVICT);
            break;
        case SECT_DESERT:
            act("Stinging sand batters your form as winds surround your body.", FALSE, victim, 0, 0,
                TO_CHAR);
            act("Stinging sand fills the air as winds surround $N's form.", FALSE, victim, 0,
                victim, TO_NOTVICT);
            break;
        case SECT_HEAVY_FOREST:
        case SECT_LIGHT_FOREST:
        case SECT_THORNLANDS:
            act("The vegetation rustles, battered by the winds you have called to armor you.",
                FALSE, victim, 0, 0, TO_CHAR);
            act("The vegetation rustles, battered by the winds that begin to surround $N.", FALSE,
                victim, 0, victim, TO_NOTVICT);
            break;
        case SECT_SALT_FLATS:
            act("Stinging sand and salt crystals batter your form as winds surround your body.",
                FALSE, victim, 0, 0, TO_CHAR);
            act("Stinging sand and salt crystals fills the air as winds surround $N's form.", FALSE,
                victim, 0, victim, TO_NOTVICT);
            break;
        case SECT_SILT:
            act("Whirling bits of silt fill the air as winds surround your body.", FALSE, victim, 0,
                0, TO_CHAR);
            act("Whirling bits of silt fill the air as winds surround $N's form.", FALSE, victim, 0,
                victim, TO_NOTVICT);
            break;
        default:
            act("Gusting winds swirl around you at an increasing rate.", FALSE, victim, 0, 0,
                TO_CHAR);
            act("Gusting winds swirl around $N at an increasing rate until $s features blur behind them.", FALSE, victim, 0, victim, TO_NOTVICT);
            break;
        }
    }

    if ((affected_by_spell(victim, SPELL_LEVITATE))
        || ((affected_by_spell(victim, SPELL_FLY)) || (ch->in_room->sector_type == SECT_AIR)))
        duration = level * 3;

    else
        duration = level;

    af.type = SPELL_WIND_ARMOR;
    af.duration = duration;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = level * 2;
    af.location = CHAR_APPLY_AC;
    af.bitvector = 0;

    if (affected_by_spell(victim, SPELL_WIND_ARMOR)) {
        for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next) {
            if (tmp_af->type == SPELL_WIND_ARMOR) {
                duration = MIN((duration + tmp_af->duration), 36);
                affect_from_char(victim, SPELL_WIND_ARMOR);
                break;
            }
        }
        af.duration = duration;
    }

    affect_to_char(victim, &af);

}                               /* end Spell Wind Armor */


/* WIND FIST: Slams the victim for stun points only, amount
 * dependent on power level.  Whiran spell.  Whira divan hurn.
 */
void
spell_wind_fist(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Wind_Fist */

    char buf[MAX_STRING_LENGTH];

    if (!ch) {
        gamelog("No ch in wind fist");
        return;
    }

    if (!victim) {
        gamelog("No victim in wind fist");
        return;
    }

    check_criminal_flag(ch, NULL);

    if (affected_by_spell(victim, SPELL_STONE_SKIN)) {
        act("You send a wave of wind towards $N, but $s stone-like skin harmlessly absorbs it.",
            FALSE, ch, 0, victim, TO_CHAR);
        act("$n sends a wave of wind towards you, but your stone-like skin harmlessly absorbs it.",
            FALSE, ch, 0, victim, TO_VICT);
        act("$n sends a wave of wind towards $N, but $s stone-like skin harmlessly absorbs it.",
            TRUE, ch, 0, victim, TO_NOTVICT);
        return;
    }

    if (does_save(victim, SAVING_SPELL, (level * (-5))))
        ws_damage(ch, victim, 0, 0, 0, (level * 4), SPELL_WIND_FIST, TYPE_ELEM_WIND);
    else
        ws_damage(ch, victim, 0, 0, 0, (level * 8), SPELL_WIND_FIST, TYPE_ELEM_WIND);

    act("You clench your fist, sending a wave of wind towards $N.", FALSE, ch, 0, victim, TO_CHAR);
    act("An invisible force slams into your chest as $n clenches $s fist.", FALSE, ch, 0, victim,
        TO_VICT);
    act("$n clenches $s fist, and $N reels back as winds swirl towards $M.", TRUE, ch, 0, victim,
        TO_NOTVICT);

    if ((IS_NPC(victim)) && (!victim->specials.fighting)) {
        find_ch_keyword(ch, victim, buf, sizeof(buf));
        cmd_kill(victim, buf, CMD_KILL, 0);
    }

}                               /* end Spell_Wind_Fist */


/* WITHER: This spell allows the caster to take life force, not
 * from the land, but from the surrounding vegetation, in an amount
 * proportionate to the power level.  It can only be cast in rooms
 * with the PLANT_SPARSE or PLANT_HEAVY flags.  At the time it is 
 * cast, nontakeable object, with the ldesc "The vegetation here is
 * withered and drooping." is created in the room, and will remain
 * for a duration dependent on power level.  The spell cannot be cast
 * again in the room while this object is there.  Nilazi spell.
 */
void
spell_wither(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{                               /* begin Spell_Wither */
#ifdef WITHERSPELL
    OBJ_DATA *wither_obj;
    *head int suckedlife;

    check_crim_cast(ch);

    if (obj) {
        if ((GET_ITEM_TYPE(obj) != ITEM_CONTAINER) || (strstr(OSTR(obj, short_descr), "head "))) {
            send_to_char("Your magick is unsuccessful.\n\r", ch);
            return;
        } else {
            send_to_char("Okay, got this far.\n\r", ch);
            return;
        }
    }

    struct wither_data
    {
        char *name;
        char *short_descr;
        char *long_descr;
    };

    struct wither_data wither_object[1] = {
        {
         "vegetation wither_obj",       /* name of obj  */
         "the surrounding vegetation",  /* short_descr of obj */
         "The surrounding vegetation is yellowed and withered here."
         /* long_descr of obj */
         }
    };

    OBJ_DATA *tmp_obj;
    OBJ_DATA *test_obj;

    if (!ch) {
        gamelog("No ch in wither");
        return;
    }

    check_crim_cast(ch);

    if ((IS_SET(ch->in_room->room_flags, RFL_PLANT_SPARSE)
         || (IS_SET(ch->in_room->room_flags, RFL_PLANT_HEAVY)))) {
        test_obj = get_obj_in_list(ch, "wither_obj", ch->in_room->contents);
        if (test_obj) {
            act("The vegetation here is already dying.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        CREATE(tmp_obj, OBJ_DATA, 1);
        clear_object(tmp_obj);
        tmp_obj->name = (char *) strdup(wither_object[0].name);
        tmp_obj->short_descr = (char *) strdup(wither_object[0].short_descr);
        tmp_obj->long_descr = (char *) strdup(wither_object[0].long_descr);
        tmp_obj->obj_flags.type = ITEM_OTHER;
        tmp_obj->obj_flags.weight = 60000;      /* so they can be seen from afar */
        tmp_obj->obj_flags.cost = 1;    /* to keep people from selling'em */
        tmp_obj->prev = NULL;
        tmp_obj->next = object_list;
        object_list = tmp_obj;
        if (tmp_obj->next)
            tmp_obj->next->prev = tmp_obj;
        obj_to_room(tmp_obj, ch->in_room);
        tmp_obj->nr = 0;        /* This is bad, but necessary */
        MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);
        act("As you leech its energy, the surrounding vegetation begins " "to wither and die.",
            TRUE, ch, tmp_obj, 0, TO_ROOM);
        new_event(EVNT_WITHER_REMOVE, level * 3 * 60, 0, 0, tmp_obj, 0, 0);
        /* and then give some of that leeched energy to the caster */
        suckedlife = dice(level + 2, 8);
        suckedlife = MAX(suckedlife, 20);
        adjust_move(ch, suckedlife);
        adjust_hit(ch, suckedlife);

        send_to_char("You feel the life force flowing into you.\n\r", ch);
        act("Vegetation dies in a slowly spreading circle " " around $n.", FALSE, victim, 0, 0,
            TO_ROOM);
    }

    else
        act("There is not enough vegetation here.\n\r", FALSE, 0, 0, 0, TO_CHAR);
#endif
}                               /* end Spell_Wither */


/* UNUSED OR ONLY STUBBED OUT SPELLS */
void
spell_create_magick(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                    OBJ_DATA * obj)
{
    OBJ_DATA *tmp_obj;
    if (!ch) {
        gamelog("No ch in create magick");
        return;
    }

    tmp_obj = NULL;   /* Initialize pointer */
    switch (level) {
    case 0:                    /* create a potion   one use/on self   */
        tmp_obj = read_object(1899, VIRTUAL);
    case 1:                    /* create a scroll   one use/pick target */
        tmp_obj = read_object(1899, VIRTUAL);
    case 2:                    /* create a wand   multi use/pick target */
        tmp_obj = read_object(1899, VIRTUAL);
    case 3:                    /* create a staff  multi use/room affect */
        tmp_obj = read_object(1899, VIRTUAL);
    case 4:                    /* create a staff  multi use/room affect -higher levelofcasting */
        tmp_obj = read_object(1899, VIRTUAL);
    }

    obj_to_char(tmp_obj, ch);
    act("$n creates a $p.", TRUE, ch, tmp_obj, 0, TO_ROOM);
    act("You create a $p.", TRUE, ch, tmp_obj, 0, TO_CHAR);
}





void
spell_illusionary_wall(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                       OBJ_DATA * obj)
{
    /* Hi again. -Nessalin */
}

// Helper function to move someone via the planeshift spell
void
planeshift_transfer(CHAR_DATA * victim, ROOM_DATA * dest_room)
{
    ROOM_DATA *was_in;

    // Error-checking
    if (!dest_room || !victim || !victim->in_room)
	    return;

    if (!no_specials && special(victim, CMD_FROM_ROOM, "planeshift"))
	    return;

    /* Echo to everyone involved and witnesses at the source, based on
     * which plane it is they are going to.
     */
    switch ((dest_room->number - 34000) / 100) {
	    case 1: // Earth
		act("$n turns into a stone silouette, before dissipating into a cloud of fine dust. ", FALSE, victim, NULL, NULL, TO_ROOM);
		act("Your surroundings are obscured by a cloud of fine dust...", FALSE, victim, NULL, NULL, TO_CHAR);
		break;
	    case 2: // Water
		act("$n turns into a clear water silouette, before evaporating into a steamy mist.", FALSE, victim, NULL, NULL, TO_ROOM);
		act("Your surroundings wash away into a steamy mist...", FALSE, victim, NULL, NULL, TO_CHAR);
		break;
	    case 3: // Lightning
		act("$n melts into a bright, white bolt of lightning which fires up into the sky.", FALSE, victim, NULL, NULL, TO_ROOM);
		act("Your surroundings are overtaken by a blazing white flash of light...", FALSE, victim, NULL, NULL, TO_CHAR);
		break;
	    case 4: // Nilaz
		act("$n melts into a pool of blood, before evaporating into a steamy pink mist.", FALSE, victim, NULL, NULL, TO_ROOM);
		act("Your surroundings fade away into a bleak grey haze...", FALSE, victim, NULL, NULL, TO_CHAR);
		break;
	    case 5: // Shadow
		act("$n melts into an inky, black shadow upon the ground which then fades away.", FALSE, victim, NULL, NULL, TO_ROOM);
		act("Your surroundings fade into darkness as an inky, black shadow envelops you...", FALSE, victim, NULL, NULL, TO_CHAR);
		break;
	    case 6: // Air
		act("A funnel cloud appears around $n and then quickly dissipates.", FALSE, victim, NULL, NULL, TO_ROOM);
		act("Your surroundings fade away as a funnel cloud envelops you...", FALSE, victim, NULL, NULL, TO_CHAR);
		break;
	    case 7: // Fire
		act("$n turns into a bright red-orange silouette of lava, before seeping into the ground.", FALSE, victim, NULL, NULL, TO_ROOM);
		act("Your surroundings fade into a blaze of red-orange...", FALSE, victim, NULL, NULL, TO_CHAR);
		break;
	    default: // Just in case, use the canned teleport echo
		act("$n disappears in a cloud of grey, billowing dust.", FALSE, victim, NULL, NULL, TO_ROOM);
		act("Your surroundings begin to fade away, and then come back again...", FALSE, victim, NULL, NULL, TO_CHAR);
		break;
    }

    // Move them to their destination
    was_in = victim->in_room;
    char_from_room(victim);
    char_to_room(victim, dest_room);

    if (!no_specials && special(victim, CMD_TO_ROOM, "planeshift")) {
	    char_from_room(victim);
	    char_to_room(victim, was_in);
	    return;
    }

    /* Echo to everyone involved and witnesses at the destination  based
     * on which plane it was they are coming from.
     */
    switch ((was_in->number - 34000) / 100) {
	    case 1: // Earth
		act("A cloud of fine dust rises off the ground and coalesces into $n.", FALSE, victim, NULL, NULL, TO_ROOM);
		break;
	    case 2: // Water
		act("A sudden mist of steam appears in the air and coalesces into $n.", FALSE, victim, NULL, NULL, TO_ROOM);
		break;
	    case 3: // Lightning
		act("A bright white flash of light appears and leaves $n in its wake.", FALSE, victim, NULL, NULL, TO_ROOM);
		break;
	    case 4: // Nilaz
		act("A cold and soundless stillness washes over the area as $n fades into existence.", FALSE, victim, NULL, NULL, TO_ROOM);
		break;
	    case 5: // Shadow
		act("An inky black shadow rises off the ground and transforms into $n.", FALSE, victim, NULL, NULL, TO_ROOM);
		break;
	    case 6: // Air
		act("Without warning, a funnel cloud strikes the ground and leaves $n in its wake.", FALSE, victim, NULL, NULL, TO_ROOM);
		break;
	    case 7: // Fire
		act("A sudden eruption of molten lava sprays from the ground and coasleces into $n.", FALSE, victim, NULL, NULL, TO_ROOM);
		break;
	    default: // Just in case, use the canned teleport echo
                act("$n appears in a cloud of grey, billowing dust.", FALSE, victim, NULL, NULL, TO_ROOM);
		break;
    }

    // Make 'em look at where they've landed
    cmd_look(victim, "", 0, 0);

    /* handle removing watching */
    watch_vis_check(victim);
}

/* Called when the planeshift spell expires or when spell is cast from an
 * elemental plane.
 */
void
planeshift_return(CHAR_DATA * victim)
{
    ROOM_DATA *dest_room = (ROOM_DATA *) NULL;
    char e_desc[MAX_STRING_LENGTH];
    int i;

    if (!victim)
	return;

    if (get_char_extra_desc_value(victim, "[PLANESHIFT_SPELL_RNUM]", e_desc, sizeof(e_desc))) {
        rem_char_extra_desc_value(victim, "[PLANESHIFT_SPELL_RNUM]");
	dest_room = get_room_num(atoi(e_desc));
    }
    /* ERROR:  They don't have a specific return location specified,
     * so pick one at random */
    for (i = 0; !dest_room && i < 100; i++) { // 100 tries before we give up
	dest_room = pick_teleport_room();
        if (dest_room && (IS_SET(dest_room->room_flags, RFL_IMMORTAL_ROOM)
		|| IS_SET(dest_room->room_flags, RFL_PRIVATE)
		|| IS_SET(dest_room->room_flags, RFL_NO_MAGICK)
		|| IS_SET(dest_room->room_flags, RFL_DEATH)
		|| IS_SET(dest_room->room_flags, RFL_NO_ELEM_MAGICK)
		|| IS_SET(dest_room->room_flags, RFL_INDOORS)))
            dest_room = NULL;
    }
    if (!dest_room) {
        gamelogf("Unable to locate destination for %s from planeshift spell.", GET_NAME(victim));
	return;
    }

    planeshift_transfer(victim, dest_room);
}

/* PLANESHIFT: This spell will send the target to an elemental plane, based on
 * the alignment of the caster.  For elementalists, it will be their plane.
 * For preservers, it will be a random plane.  For defilers, it will the
 * nilaz plane.
 *
 * Elementalist and sorcerer spell.  Nilaz locror hurn.
 */
void
spell_planeshift(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{
    ROOM_DATA *dest_room = (ROOM_DATA *) NULL;
    int roomnum, basenum, i;
    char e_desc[MAX_STRING_LENGTH];
    int duration = level;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    /* If they're already affected by the planeshift spell, removing the affect
     * will handle returning them to the right spot
     */
    if (affected_by_spell(victim, SPELL_PLANESHIFT)) {
        affect_from_char(victim, SPELL_PLANESHIFT);
        return;
    }

    /* Elemental Planes are in zone 34.  If they're not there already, then
     * take them to an appropriate plane.
     */
    if (victim->in_room->zone != 34) { // Not on an elemental plane
        switch(GET_GUILD(ch)) {
		case GUILD_STONE_ELEMENTALIST:
                    basenum = 34100; // Earth plane
		    break;
		case GUILD_WATER_ELEMENTALIST:
                    basenum = 34200; // Water plane
		    break;
		case GUILD_LIGHTNING_ELEMENTALIST:
                    basenum = 34300; // Lightning plane
		    break;
		case GUILD_VOID_ELEMENTALIST:
                    basenum = 34400; // Nilaz plane
		    break;
		case GUILD_SHADOW_ELEMENTALIST:
                    basenum = 34500; // Shadow plane
		    break;
		case GUILD_WIND_ELEMENTALIST:
                    basenum = 34600; // Air plane
		    break;
		case GUILD_FIRE_ELEMENTALIST:
                    basenum = 34700; // Fire plane
		    break;
		case GUILD_DEFILER: 
		default:
		    // Defilers always go to Nilaz
		    if (ch->specials.eco < 0) {
                basenum = 34400; // Nilaz plane
		    } else { // Preservers go at random
                basenum = 34000 + (number(1,7) * 100);
		    }
		    break;
        }

        // Find a random location
        for (i = 0; !dest_room && i < 100; i++) { // 100 tries before we give up
            roomnum = basenum + number(1, 99);
            dest_room = get_room_num(roomnum);
            if (dest_room && (IS_SET(dest_room->room_flags, RFL_IMMORTAL_ROOM)
                || IS_SET(dest_room->room_flags, RFL_PRIVATE)
                || IS_SET(dest_room->room_flags, RFL_NO_MAGICK)
                || IS_SET(dest_room->room_flags, RFL_DEATH)
                || IS_SET(dest_room->room_flags, RFL_NO_ELEM_MAGICK)
                || IS_SET(dest_room->room_flags, RFL_INDOORS)))
                dest_room = NULL;
        }

        // No destination or the source room is transient
        if (!dest_room || victim->in_room->zone == RESERVED_Z) {
                act("The spell fizzles and dies.", FALSE, ch, NULL, NULL, TO_CHAR);
                return;
        }

        // Components boost the duration
        if (get_component(ch, SPHERE_TELEPORTATION, level)) {
            if (get_componentB(ch, SPHERE_TELEPORTATION, level,
                               "$p is consumed by the magick.",
                               "$p disappears from $n's hands.",
                               "You don't have the necessary component.")) {
		        duration *= MAX(2, level);
	        }
        } 

        // Set the room# for the return trip
        sprintf(e_desc, "%d", victim->in_room->number);
        set_char_extra_desc_value(victim, "[PLANESHIFT_SPELL_RNUM]", e_desc);

        // Add the affect
        af.type = SPELL_PLANESHIFT;
        af.duration = duration;
        af.power = level;
        af.magick_type = magick_type;
        af.modifier = 0;
        af.location = CHAR_APPLY_NONE;
        af.bitvector = 0;
        affect_to_char(victim, &af);

        // Move 'em to the plane
        planeshift_transfer(victim, dest_room);
    } else {
        /* This is a rare condition, since the only way to get here is to
         * be on an elemental plane w/o having used the planeshift spell.  It may
         * happen if people are allowed to 'wander' onto an elemental plane,
         * a crash has removed the affect from the PC while they were on a
         * plane, or some other means has brought the person onto an elemental
         * plane.  Regardless, casting this spell while on an elemental plane
         * is a return trip to the mortal plane.
         */
        planeshift_return(victim); // Send 'em back
    }
}

void
spell_focus_sun(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{
}

void
spell_chaos_shield(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{

}

void
spell_magic_shield(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                   OBJ_DATA * obj)
{

}

void
spell_wind_demon(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{

}

void
spell_daylight(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{

}

bool
dispel_invisibility_obj(CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (IS_SET(obj->obj_flags.extra_flags, OFL_INVISIBLE)) {
        REMOVE_BIT(obj->obj_flags.extra_flags, OFL_INVISIBLE);
        send_to_char("You remove its invisibility.\n\r", ch);
        return TRUE;
    }
    return FALSE;
}

bool
dispel_invisibility_ch(CHAR_DATA * ch, CHAR_DATA * victim, struct affected_type * af, byte level)
{

    /* reduce the expiration by the level of the spell in hours */
    af->expiration -= (level * RT_ZAL_HOUR);

    /* if it would make the spell expire, remove the affect */
    if (af->expiration < time(0)) {
        affect_from_char(victim, SPELL_INVISIBLE);
        send_to_char("You feel exposed.\n\r", victim);
        act("$n fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
        return TRUE;
    }
    /* send a message about the weakening of the spell */
    else {
        send_to_char("You feel your concealment weaken.\n\r", victim);
        act("$n's concealment weakens.", FALSE, victim, 0, 0, TO_ROOM);
        return FALSE;
    }
}


void
spell_dispel_invisibility(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                          OBJ_DATA * obj)
{
    struct affected_type *tmp_af = 0;
    struct affected_type *next_af = 0;

    if (obj) {                  /* if the target is an object */
        act("A gusting wind blows from your hands, surrounding $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("A gusting wind blows from $n, surrounding $p.", FALSE, ch, obj, 0, TO_ROOM);

        /* dispelling an object is not considered aggressive */
        check_crim_cast(ch);
        dispel_invisibility_obj(ch, obj);
    } else {                    /* it's a victim */
        act("A gusting wind blows from your hands towards $N.", FALSE, ch, 0, victim, TO_CHAR);
        act("A gusting wind blows from $n towards you.", FALSE, ch, 0, victim, TO_VICT);
        act("A gusting wind blows from $n towards $N.", FALSE, ch, 0, victim, TO_NOTVICT);

        /* Aggressive against soldiers/templars, not so against others */
        if (is_soldier_in_city(victim) || is_templar_in_city(victim))
            check_criminal_flag(ch, NULL);
        else
            check_crim_cast(ch);

        /* get the level (duration) the same way invis does */
        level = ((level * 5) - get_char_size(victim));
        if (level < 0) {
            send_to_char("Your magicks are unable to affect them.\n\r", ch);
            return;
        }

        for (tmp_af = victim->affected; tmp_af; tmp_af = next_af) {
            next_af = tmp_af->next;

            switch (tmp_af->type) {
            case SPELL_INVISIBLE:
                dispel_invisibility_ch(ch, victim, tmp_af, level);
                break;
            default:
                break;
            }
        }
    }
}

bool
dispel_ethereal_obj(CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL)) {
        send_to_char("You remove its etherealness.\n\r", ch);
        REMOVE_BIT(obj->obj_flags.extra_flags, OFL_ETHEREAL);
        return TRUE;
    }
    return FALSE;
}

bool
dispel_ethereal_ch(CHAR_DATA * ch, CHAR_DATA * victim, struct affected_type * af, byte level)
{
    OBJ_DATA *obj;
    int pos;

    /* reduce the expiration by the level in hours */
    af->expiration -= (level * RT_ZAL_HOUR);

    /* if it's expired, and they're not a shadow */
    if ((af->expiration < time(0)) && (GET_RACE(victim) != RACE_SHADOW)) {
        affect_from_char(victim, SPELL_ETHEREAL);

        act("You fade back from the realm of shadows.", FALSE, victim, 0, 0, TO_CHAR);
        act("$n fades into view.", FALSE, victim, 0, 0, TO_ROOM);

        for (obj = victim->carrying; obj; obj = obj->next_content) {
            act("$p comes out of the shadow plane with you.", FALSE, victim, obj, 0, TO_CHAR);
            REMOVE_BIT(obj->obj_flags.extra_flags, OFL_ETHEREAL);
        }

        for (pos = 0; pos < MAX_WEAR; pos++) {
            if (victim->equipment && victim->equipment[pos]) {
                obj = victim->equipment[pos];
                if (IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL)) {
                    act("$p comes out of the shadow plane with you.", FALSE, victim, obj, 0,
                        TO_CHAR);
                    REMOVE_BIT(obj->obj_flags.extra_flags, OFL_ETHEREAL);
                }
            }
        }
        return TRUE;
    }
    /* else if it's not expired */
    else if (GET_RACE(victim) != RACE_SHADOW) {
        send_to_char("You fade back from the realm of shadows momentarily.\n\r", victim);
        act("$n's body becomes substantial for a moment, before " "fading away into the shadows.",
            FALSE, victim, 0, 0, TO_ROOM);
        return FALSE;
    }
    return FALSE;
}

void
spell_dispel_ethereal(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                      OBJ_DATA * obj)
{
    struct affected_type *tmp_af = 0;
    struct affected_type *next_af = 0;

    if (obj) {                  /* if the target is an object */
        /* probably need to change this message */
        act("A tendril of shadow wisps from your hands enveloping $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("A tendril of shadow wisps from $n enveloping $p.", FALSE, ch, obj, 0, TO_ROOM);

        /* dispelling an object is not considered aggressive */
        check_crim_cast(ch);
        dispel_ethereal_obj(ch, obj);
    } else {                    /* it's a victim */
        /* probably need to change this message */
        act("A tendril of shadow wisps from your hands towards $N.", FALSE, ch, 0, victim, TO_CHAR);
        act("A tendril of shadow wisps from $n towards you.", FALSE, ch, 0, victim, TO_VICT);
        act("A tendril of shadow wisps from $n towards $N.", FALSE, ch, 0, victim, TO_NOTVICT);

        /* Aggressive against soldiers/templars, not so against others */
        if (is_soldier_in_city(victim) || is_templar_in_city(victim))
            check_criminal_flag(ch, NULL);
        else
            check_crim_cast(ch);

        for (tmp_af = victim->affected; tmp_af; tmp_af = next_af) {
            next_af = tmp_af->next;

            switch (tmp_af->type) {
            case SPELL_ETHEREAL:
                dispel_ethereal_ch(ch, victim, tmp_af, level);
                break;
            default:
                break;
            }
        }
    }
}


// Update magickal affects, returns true if the character is still alive.
int
update_magick(CHAR_DATA * ch, struct affected_type *af, int duration_changed )
{
    char buf[MAX_STRING_LENGTH];

    // sanity check
    if( !ch || !af )
       return TRUE;

    // based on the affect's type (spell to update)
    switch( af->type ) {
    case SPELL_IMMOLATE:
        if (!number(0, 10)) {
            send_to_char("The orange flames that have engulfed you burn you with a hiss!\n\r", ch);
            act("The orange flames surrounding $n burn $m with a hiss!", TRUE, ch, 0, 0, TO_ROOM);
            sflag_to_char(ch, OST_BURNED);

            // Stun damage, why not health?
            if( !generic_damage(ch, dice(3,8), 0, 0, dice(1,8) ) ) {
                send_to_char
                    ("The orange flames burn you to death, and the world goes dark!\n\r", ch);
                act("$n's body goes limp and is burned to death by the orange flames!", TRUE,
                    ch, 0, 0, TO_ROOM);

                if( !IS_NPC(ch) ) {
                    /* Ch is dead, so let's leave a death message */
                    free(ch->player.info[1]);
                    sprintf(buf, "%s has been burned to death by the Immolate spell room %d.",
                        GET_NAME(ch), ch->in_room->number);
                    ch->player.info[1] = strdup(buf);
                    gamelog(buf);
                }
                die(ch);
                return FALSE;
            }

            af->expiration -= RT_ZAL_HOUR;
            if( af->expiration < time(0) ) {
                affect_from_char(ch, SPELL_IMMOLATE);
                send_to_char("The ravaging fires burning you suddenly die out with a wisp of smoke.\n\r", ch);
                act("The ravaging fires burning $n suddenly die out with a wisp of smoke.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                send_to_char("You feel the flames weaken.\n\r", ch);
            }
        }
        return TRUE;
    // All other spells...do nothing for now
    default:
        break;
    }
    return TRUE;
}

void
spell_recite(byte level, sh_int magick_type, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)
{
    if (!victim) {
        gamelogf("no victim in %s", __FUNCTION__);
        return;
    }

    CHAR_DATA *target = ch->specials.contact;
    if (!target) {
        cprintf(ch, "You must be in contact with someone to cause them to recite.\n\r");
        return;
    }

    if (affected_by_spell(victim, SPELL_RECITE)) {
        act("$N is already reciting.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    if (!is_following(victim, ch) || !affected_by_spell(victim, SPELL_CHARM_PERSON)) {
        act("$N is not your minion.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    if (victim->specials.contact) {
        act("$N is already occupied in using the Way.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    if (GET_RACE_TYPE(victim) != RTYPE_HUMANOID) {
        act("You are unable to force $N to recite, because it is not humanoid.",
            FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    victim->specials.contact = target;

    // Add the affect
    struct affected_type af;
    memset(&af, 0, sizeof(af));
    af.type = SPELL_RECITE;
    af.duration = level * 2;
    af.power = level;
    af.magick_type = magick_type;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    if (MSTR(target, short_descr)) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "pemote countenance takes on the appearance of %s for a few moments.", MSTR(target, short_descr));
        parse_command(victim, buf);
    }
}

