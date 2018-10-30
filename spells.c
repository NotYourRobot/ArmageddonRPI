/*
 * File: SPELLS.C
 * Usage: Spell functions.
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
 * REVISION HISTORY
 * 11/05/2000:    Reformatted file.  Made the following changes:
 *                - Fixed a lot of typos and erroneous gamelog error msgs.
 *                - Things are alphabetized, because I am compulsive.
 *                - All of the reaches were looked at, scroll and room
 *                added to many.
 *                - Potion effects: if you drink a create darkness potion
 *                you go blind.  Many potions still do not work, but the
 *                message has been made more interesting than "nothing
 *                happens".
 *                - Room reach with create water and create wine fill all
 *                the drink containers in the room.
 *                - If you attempt to empower something with the magick
 *                or glyph flag on it, it is destroyed.  -Sanvean
 * 12/02/2000     Added spells delusion, illuminant, mirage, phantasm.  -San
 */

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "parser.h"
#include "skills.h"
#include "handler.h"
#include "modify.h"
#include "guilds.h"
#include "event.h"
#include "utility.h"
#include "spells.h"

/* Global data */

/* Extern procedures */
void clone_char(CHAR_DATA * ch);
void set_creator_edesc(CHAR_DATA * ch, OBJ_DATA * obj); 

/* I know this one is not template...but it doesn't need to be, it's
     not a generic call, I am getting sloppy...   -Tenebrius    */
extern void spell_divert_storm(byte level, CHAR_DATA * ch, int dir);
extern int ethereal_ch_compare(CHAR_DATA * ch, CHAR_DATA * victim);
extern int get_dir_from_name(char *arg);

sh_int
get_magick_type(CHAR_DATA * ch)
{
    switch (GET_GUILD(ch)) {
    case GUILD_FIRE_ELEMENTALIST:
    case GUILD_WATER_ELEMENTALIST:
    case GUILD_WIND_ELEMENTALIST:
    case GUILD_STONE_ELEMENTALIST:
    case GUILD_SHADOW_ELEMENTALIST:
    case GUILD_LIGHTNING_ELEMENTALIST:
    case GUILD_VOID_ELEMENTALIST:
        return MAGICK_ELEMENTAL;
    default:
        return MAGICK_SORCEROR;
    }
}

sh_int
get_spell_magick_type(int type, CHAR_DATA * ch)
{
    sh_int magick_type;

    switch (type) {
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_STAFF:
        magick_type = MAGICK_OBJECT;
        break;
    default:
        magick_type = get_magick_type(ch);
    }

    return magick_type;
}


/* Begin Spells */

void
cast_alarm(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Alarm */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_alarm(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A shiver runs across your skin.\n\r", ch);
        break;
    default:
        break;
    }
}                               /* end Cast_Alarm */


void
cast_animate_dead(byte level, CHAR_DATA * ch, const char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Animate_Dead */
    OBJ_DATA *i, *temp_obj;
    char buf[256];
    int find_targ = 1;
    sh_int magick_type = get_spell_magick_type(type, ch);

    arg = one_argument(arg, buf, sizeof(buf));

    switch (type) {
    case SPELL_TYPE_POTION:
        send_to_char("You gag on the foul taste, as a deadly chill runs through you.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
        if (!tar_obj) {
            send_to_char("They're not dead yet!\n\r", ch);
            return;
        }
        find_targ = 1;
    case SPELL_TYPE_SPELL:
        if (find_targ)
            if (!(tar_obj = get_obj_in_list_vis(ch, buf, ch->in_room->contents))) {
                send_to_char("You don't see that here.\n\r", ch);
                return;
            }
        spell_animate_dead(level, magick_type, ch, 0, tar_obj);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        i = ch->in_room->contents;
        while (i) {
            temp_obj = i->next_content;
            if (IS_CORPSE(i))
                spell_animate_dead(level, magick_type, ch, 0, i);
            i = temp_obj;
        }
        break;
    default:
        gamelog("Serious screw-up in animate dead spell!");
        break;
    }
}                               /* end Cast_Animate_Dead */

void
cast_armor(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_armor(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_armor(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch) 
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (tar_obj) {
	act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
	return;
      }
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
      spell_armor(level, magick_type, ch, tar_ch, 0);
      return;
      
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_armor(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in armor spell!");
        break;
    }
}                               /* end Spell_Armor */


void
cast_aura_drain(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_aura_drain(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!victim)
	victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_aura_drain(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_aura_drain(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_aura_drain(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Screw-up in aura drain spell!");
        break;
    }
}                               /* end Cast_Aura_Drain */


void
cast_light(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Light */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
      tar_ch = ch;
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
      // So they can use it in the dark
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_SPELL:
      if (tar_obj) {
	act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
	return;
      }
      if (!tar_ch) {
	send_to_char ("Cast on who?\n\r", ch);
	return;
      }

        spell_ball_of_light(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_ball_of_light(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in ball of light spell!");
        break;
    }
}                               /* end Cast_Light */

void
cast_banishment(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{                               /* begin Cast_Banishment */
    CHAR_DATA *temp;
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        tar_ch = ch;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        //        if (tar_obj) {
        //            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
        //            return;
        //        }
        /* spell is target self nono, hence this makes no sense */
        // if (!tar_ch)
        //    tar_ch = ch; 
        spell_banishment(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        tar_ch = ch->in_room->people;
        while (tar_ch) {
            temp = tar_ch->next_in_room;
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_banishment(level, magick_type, ch, tar_ch, tar_obj);
            tar_ch = temp;
        }
        break;
    default:
        gamelog("Serious screw-up in banishment spell!");
        break;
    }
}                               /* end Cast_Banishment */

extern int rev_dir[];
extern char *dirs[];

/* Added 05/16/2000 -nessalin */
void
cast_blade_barrier(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_Blade_Barrier */
    char buf[256];
    int dir;
    int duration;

    struct room_data *other_room;
    struct room_direction_data *back;

    // sh_int magick_type = get_spell_magick_type( type, ch );

    switch (type) {             /* begin type-checking with switch (type) */
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SPELL:
        //    gamelog ("Logging argument in cast_blade_barrier() in spells.c");
        //    gamelog (arg);
        for (; *arg && (*arg == ' '); arg++);

        dir = get_dir_from_name(arg);
        if (DIR_UNKNOWN == dir) {
          act("The spell must be cast in a specific direction.", FALSE, ch, 0, 0, TO_CHAR);
          return;
        }

        if (!ch->in_room->direction[dir]) {
            act("There is no exit from here in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER)) {
            act("There is already a blade barrier in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        duration = WAIT_SEC * (30 + level * 30);
        MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER);
        sprintf(buf,
                "$n points $s finger and a mass of whirling blades spins into existence, blocking the %s exit.",
                dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        sprintf(buf,
                "You point your finger and a mass of whirling blades spins into existance, blocking the %s exit.",
                dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        new_event(EVNT_REMOVE_BLADEBARRIER, duration, 0, 0, 0, ch->in_room, dir);

        if ((other_room = EXIT(ch, dir)->to_room))
            if ((back = other_room->direction[rev_dir[dir]]))
                if (back->to_room == ch->in_room) {
                    MUD_SET_BIT(back->exit_info, EX_SPL_BLADE_BARRIER);
                    sprintf(buf,
                            "A mass of whirling blades spins into existence, "
                            "blocking the %s exit.\n\r", dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                    new_event(EVNT_REMOVE_BLADEBARRIER, duration, 0, 0, 0, EXIT(ch, dir)->to_room,
                              rev_dir[dir]);
                }
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        duration = WAIT_SEC * (30 + level * 30);
        for (dir = DIR_NORTH; dir <= 5; dir++) {
            if (ch->in_room->direction[dir]) {
                if (!IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER)) {
                    MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER);
                    sprintf(buf,
                            "$n points $s finger and a mass of whirling blades spins into "
                            "existence, blocking the %s exit.", dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_ROOM);
                    sprintf(buf,
                            "You point your finger and a mass of whirling blades spins into "
                            "existance, blocking the %s exit.", dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_CHAR);
                    new_event(EVNT_REMOVE_BLADEBARRIER, duration, 0, 0, 0, ch->in_room, dir);
                    if ((other_room = EXIT(ch, dir)->to_room))
                        if ((back = other_room->direction[rev_dir[dir]]))
                            if (back->to_room == ch->in_room) {
                                MUD_SET_BIT(back->exit_info, EX_SPL_BLADE_BARRIER);
                                sprintf(buf,
                                        "A mass of whirling blades spins into "
                                        "existence, blocking the %s exit.\n\r", dirs[rev_dir[dir]]);
                                send_to_room(buf, EXIT(ch, dir)->to_room);
                                new_event(EVNT_REMOVE_BLADEBARRIER, duration, 0, 0, 0,
                                          EXIT(ch, dir)->to_room, rev_dir[dir]);
                            }
                }
            }
        }
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A sour, metallic taste fills your mouth.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in blade barrier!");
    }                           /* end type-checking with switch (type) */
}                               /* end Cast_Blade_Barrier */


void
cast_blind(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Blind */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_blind(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch; 
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
	if (!tar_ch) {
	  send_to_char ("Cast on who?\n\r", ch);
	  return;
	}
        spell_blind(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_blind(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in blindness spell!");
        break;
    }
}                               /* end Cast_Blind */


void
cast_breathe_water(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_Blind */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_breathe_water(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch) 
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) // Allowing this because it is beneficial
            tar_ch = ch;
        spell_breathe_water(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_breathe_water(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in blindness spell!");
        break;
    }
}                               /* end Cast_Breathe_Water */


void
cast_burrow(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Burrow */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_POTION:
        spell_burrow(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (tar_obj) {
	act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
	return;
      }
      
        if (tar_ch != ch) {
            send_to_char("Nothing seems to happen.\n\r", ch);
            return;
        }
        spell_burrow(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_burrow(level, magick_type, ch, ch, 0);
        break;
    default:
        gamelog("Serious screw-up with the Burrow spell!");
        break;
    }
}                               /* end Cast_Burrow */


void
cast_calm(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Calm */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_calm(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
	if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }    

    case SPELL_TYPE_SPELL:
      spell_calm(level, magick_type, ch, tar_ch, 0);
      break;
      
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_calm(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in Calm spell!");
        break;
    }
}                               /* end Cast_Calm */


void
cast_chain_lightning(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                     OBJ_DATA * tar_obj)
{                               /* begin Cast_Chain_Lightning */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_chain_lightning(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Serious screw-up in chain lightning spell!");
        break;
    }
}                               /* end Cast_Chain_Lightning */


void
cast_charm_person(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Charm_Person */

    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_SPELL:
        if (!tar_ch)
            send_to_char("A wave of dizziness washes over you.\n\r", ch);
        else
            spell_charm_person(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A wave of dizziness washes over you.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up with Charm Person spell.");
        break;
    }
}                               /* end Cast_Charm_Person */


void
cast_create_darkness(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                     OBJ_DATA * tar_obj)
{                               /* begin Cast_Create_Darkness */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_create_darkness(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A blackness settles itself across your vision.\n\r", ch);
        spell_blind(level, magick_type, ch, ch, 0);
        break;
    default:
        gamelog("Serious screw-up in create darkness spell!");
        break;
    }
}                               /* end Cast_Create_Darkness */


void
cast_create_food(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{                               /* begin Cast_Create_Food */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        send_to_char("A pleasant, satiated feeling comes over you.\n\r", ch);
        GET_COND(ch, FULL) += (level * (level * number(1, 2)));
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
      if (tar_ch != ch) {
	send_to_char("A pleasant, satiated feeling comes over you.\n\r", tar_ch);
	GET_COND(tar_ch, FULL) += (level * (level * number(1, 2)));
	return;
      }
      spell_create_food(level, magick_type, ch, 0, 0);
      break;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        act("$n gestures, summoning food from the air itself.", FALSE, ch, 0, 0, TO_ROOM);
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
        spell_create_food(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Serious screw-up in create food spell!");
        break;
    }
}


void
cast_create_rune(byte level, CHAR_DATA * ch, const char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{
//    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *tmp_obj;
    char buf[MAX_STRING_LENGTH];
    char runename[256];

    arg = one_argument(arg, runename, sizeof(runename));

    if (!*runename) {
        send_to_char("You must supply a name for the rune to be created.\n\r", ch);
        return;
    }

    if ((ch->in_room->sector_type == SECT_NILAZ_PLANE)
        || (ch->in_room->sector_type == SECT_EARTH_PLANE)
        || (ch->in_room->sector_type == SECT_SILT)
        || (ch->in_room->sector_type == SECT_WATER_PLANE)) {
        send_to_char("Your spell fizzles and dies.\n\r", ch);
        return;
    }

    if (!get_componentB
        (ch, SPHERE_CREATION, level, "$p quickly unravels and fades into a wisp of air.", "",
         "You must have the proper component to create a rune."))
        return;

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SPELL:

        tmp_obj = read_object(1124, VIRTUAL);
        if (!tmp_obj) {
            errorlog("spell_create_rune() in spells.c, object #1124 not found.");
            return;
        }

        obj_to_room(tmp_obj, ch->in_room);

        sprintf(buf, "the runes, \"%s\"", runename);
        tmp_obj->short_descr = (char *) strdup(buf);

        if (ch->in_room->sector_type == SECT_AIR)
            sprintf(buf, "Some runes, \"%s\", hover in the air here.", runename);
        else if ((ch->in_room->sector_type == SECT_CAVERN)
                 || (ch->in_room->sector_type == SECT_SOUTHERN_CAVERN))
            sprintf(buf, "Some runes, \"%s\", have been scribed onto the cavern wall.", runename);
        else if (ch->in_room->sector_type == SECT_DESERT)
            sprintf(buf, "Some runes, \"%s\", are formed in the sands here.", runename);
        else if (ch->in_room->sector_type == SECT_SALT_FLATS)
            sprintf(buf,
                    "Salt crystals have been magickally formed into the shape of some runes, \"%s\".",
                    runename);
        else if (ch->in_room->sector_type == SECT_THORNLANDS)
            sprintf(buf, "Thorns have been burned into the shape of Some runes, \"%s\".", runename);
        else if (ch->in_room->sector_type == SECT_ROAD)
            sprintf(buf, "Some runes, \"%s\", have been scribed on the ground near the road.",
                    runename);
        else if (ch->in_room->sector_type == SECT_SEWER)
            sprintf(buf, "Floating in the muck is a set of runes, \"%s\".", runename);
        else if (ch->in_room->sector_type == SECT_GRASSLANDS)
            sprintf(buf, "Burned into a patch of grass is some runes, \"%s\".", runename);
        else
            sprintf(buf, "Some runes, \"%s\", have been scribed onto ground here.", runename);

        tmp_obj->long_descr = (char *) strdup(buf);
        sprintf(buf, "%s [teleport_rune_spell]", runename);
        tmp_obj->name = (char *) strdup(buf);

        if (IS_NPC(ch)) {
            sprintf(buf, "%d", npc_index[ch->nr].vnum);
            set_obj_extra_desc_value(tmp_obj, "[PC_info]", buf);
        } else {
            set_obj_extra_desc_value(tmp_obj, "[PC_info]", MSTR(ch, name));
            set_obj_extra_desc_value(tmp_obj, "[ACCOUNT_info]", ch->account);
        }

        act("You gently guide the winds to coelesce into $p here.", FALSE, ch, tmp_obj, 0, TO_CHAR);
        act("$n gently guides the winds to coelesce into a magickal rune here.", FALSE, ch, 0, 0,
            TO_ROOM);

        MUD_SET_BIT(tmp_obj->obj_flags.extra_flags, OFL_NOSAVE);
        new_event(EVNT_REMOVE_OBJECT, ((level + 1) * 24000), 0, 0, tmp_obj, 0, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_ROOM:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in create rune!");
        break;
    }
}


void
cast_create_water(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Create_Water */
    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *i, *temp_obj;

    switch (type) {
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_room("Mist fills the air for a moment.\n\r", ch->in_room);
        i = ch->in_room->contents;
        while (i) {
            temp_obj = i->next_content;
            if (i->obj_flags.type == ITEM_DRINKCON)
                spell_create_water(level, magick_type, ch, 0, i);
            i = temp_obj;
        }
        break;

    case SPELL_TYPE_POTION:
        tar_ch = ch;
        send_to_char("A refreshing wetness fills your mouth.\n\r", tar_ch);
        GET_COND(tar_ch, THIRST) += (level * (level * number(1, 2)));
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if ((!tar_ch) && (!tar_obj)) {
	send_to_char("Use on who or what?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        if ((!tar_ch) && (!tar_obj))
            tar_ch = ch;
        if (tar_ch) {
            send_to_char("A refreshing wetness fills your mouth.\n\r", tar_ch);
            GET_COND(tar_ch, THIRST) += (level * (level * number(1, 2)));
            return;
        }
        if (tar_obj->obj_flags.type != ITEM_DRINKCON) {
            act("$p is unable to hold water.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        spell_create_water(level, magick_type, ch, 0, tar_obj);
        break;
    default:
        gamelog("Serious screw-up in create water!");
        break;
    }
}


void
cast_create_wine(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{                               /* begin Cast_Create_Wine */
    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *i, *temp_obj;

    switch (type) {
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
      send_to_room("A dark mist fills the air for a moment.\n\r", ch->in_room);
      i = ch->in_room->contents;
      while (i) {
	temp_obj = i->next_content;
	if ((temp_obj) && (temp_obj->obj_flags.type != ITEM_DRINKCON))
	  spell_create_wine(level, magick_type, ch, 0, i);
	i = temp_obj;
      }
      break;
    case SPELL_TYPE_POTION:
      spell_intoxication(level, magick_type, ch, ch, 0);
      break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if ((!tar_ch) && (!tar_obj)) {
	send_to_char("Use on who or what?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
      if ((!tar_ch) && (!tar_obj))
	tar_ch = ch;
      if (tar_ch) {
	spell_intoxication(level, magick_type, ch, tar_ch, 0);
	return;
      }
      if (tar_obj->obj_flags.type != ITEM_DRINKCON) {
	send_to_char("It is unable to hold liquid.\n\r", ch);
	return;
      }
      spell_create_wine(level, magick_type, ch, 0, tar_obj);
      break;
    default:
        gamelog("Serious screw-up in create wine!");
        break;
    }
}


void
cast_cure_blindness(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                    OBJ_DATA * tar_obj)
{                               /* begin Cast_Cure_Blind */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_cure_blindness(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_SPELL:
        spell_cure_blindness(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_cure_blindness(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in cure blind spell!");
        break;
    }
}                               /* end Cast_Cure_Blind */


void
cast_cure_poison(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{                               /* begin Cast_Cure_Poison */
    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *i, *temp_obj;

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_cure_poison(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if ((!tar_obj) && (!tar_ch))
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if ((!tar_obj) && (!tar_ch)) {
	send_to_char ("Use on who or what?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
      if ((!tar_obj) && (!tar_ch)) {
	send_to_char ("Cast on who or what?\n\r", ch);
	return;
      }
        spell_cure_poison(level, magick_type, ch, tar_ch, tar_obj);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        /* do the peoples */
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_cure_poison(level, magick_type, ch, tar_ch, 0);
        /* then do the objects */
        i = ch->in_room->contents;
        while (i) {
            temp_obj = i->next_content;
            spell_cure_poison(level, magick_type, ch, 0, i);
            i = temp_obj;
        }
        break;
    default:
        gamelog("Serious screw-up in remove poison spell!");
        break;
    }
}                               /*  end Cast_Cure_Poison */


void
cast_curse(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        tar_ch = ch;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
      act("The words on the scroll smoulder, tendrils of smoke curling towards you.", FALSE, ch,
	  0, 0, TO_CHAR);
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch)
            tar_ch = ch;
        spell_curse(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_curse(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in curse spell!");
        break;
    }
}                               /* end Spell_Curse */


void
cast_dead_speak(byte level, CHAR_DATA * ch, const char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{                               /* begin Cast_Dead_Speak */

    OBJ_DATA *i, *temp_obj;
    char buf[256];
    int find_targ = 1;
    sh_int magick_type = get_spell_magick_type(type, ch);

    arg = one_argument(arg, buf, sizeof(buf));

    switch (type) {
    case SPELL_TYPE_POTION:
        send_to_char("You gag on the foul taste, as a deadly chill runs through you.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
        if (!tar_obj) {
            send_to_char("They're not dead yet!\n\r", ch);
            return;
        }
        find_targ = 1;
    case SPELL_TYPE_SPELL:
        if (find_targ)
            if (!(tar_obj = get_obj_in_list_vis(ch, buf, ch->in_room->contents))) {
                send_to_char("You don't see that here.\n\r", ch);
                return;
            }
        spell_dead_speak(level, magick_type, ch, 0, tar_obj);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        i = ch->in_room->contents;
        while (i) {
            temp_obj = i->next_content;
            if (IS_CORPSE(i))
                spell_dead_speak(level, magick_type, ch, 0, i);
            i = temp_obj;
        }
        break;
    default:
        gamelog("Serious screw-up in dead speak spell in spells.c!");
        break;
    }
}                               /* end Cast_Dead_Speak */


void
cast_deafness(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Deafness */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_deafness(level, magick_type, ch, ch, 0);
    case SPELL_TYPE_SPELL:
        if (!IS_AFFECTED(tar_ch, CHAR_AFF_DEAFNESS))
            spell_deafness(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("Your ears throb numbly.\n\r", ch);
        /* Note: deafness should be cumulative, imo.  -San */
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
        if (!IS_AFFECTED(ch, CHAR_AFF_DEAFNESS))
            spell_deafness(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("Your ears throb numbly.\n\r", ch);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_AFFECTED(tar_ch, CHAR_AFF_DEAFNESS))
                    if (!IS_IMMORTAL(tar_ch))
                        if (ethereal_ch_compare(ch, tar_ch))
                            spell_deafness(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in deafness spell!");
        break;
    }
}                               /* end Cast_Deafness */


void
cast_delusion(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Delusion */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_delusion(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Something is wrong with Delusion.  Blame Daigon.");
        break;
    }
}                               /* end Cast_Delusion */


void
cast_demonfire(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
               OBJ_DATA * tar_obj)
{                               /* begin Cast_Demonfire */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_demonfire(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_demonfire(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_SCROLL:
      if (!victim)
	victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}

	spell_demonfire(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_demonfire(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Serious screw-up in demonfire spell!");
        break;
    }
}                               /* end Cast_Demonfire */


void
cast_detect_ethereal(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                     OBJ_DATA * tar_obj)
{                               /* begin Cast_Detect_Ethereal */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_detect_ethereal(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        if (tar_obj) {
            send_to_char("It seems unaffected.\n\r", ch);
            return;
        }
        if (!tar_ch)
            tar_ch = ch;
        spell_detect_ethereal(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    if (!IS_AFFECTED(tar_ch, CHAR_AFF_DETECT_ETHEREAL))
                        spell_detect_ethereal(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in detect ethereal spell!");
        break;
    }
}                               /* end Cast_Detect_Ethereal */


void
cast_detect_invisibility(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                         OBJ_DATA * tar_obj)
{                               /* begin Cast_Detect_invis */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_detect_invisibility(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_detect_invisibility(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char ("Use on who?\n\r", ch);
	  return;
	}

	spell_detect_invisibility(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_detect_invisibility(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in detect invisibility spell!");
        break;
    }
}                               /* end Cast_Detect_Invis */

void
cast_detect_magick(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_Detect_Magick */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_detect_magick(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_detect_magick(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
        spell_detect_magick(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_detect_magick(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in detect magick spell!");
        break;
    }
}                               /* end Cast_Detect_Magick */


void
cast_detect_poison(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* cast_Detect_Poison */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_detect_poison(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch && !tar_obj)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch && !tar_obj) {
	send_to_char("Use on who or what ?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        if (tar_obj)
            spell_detect_poison(level, magick_type, ch, 0, tar_obj);
        else {
            if (!tar_ch)
                tar_ch = ch;
            spell_detect_poison(level, magick_type, ch, tar_ch, 0);
        }
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_detect_poison(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in detect poison spell!");
        break;
    }
}                               /* cast_Detect_Poison */


void
cast_determine_relationship(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                            OBJ_DATA * tar_obj)
{                               /* begin Cast_Determine_Relationship */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_determine_relationship(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_determine_relationship(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}

        spell_determine_relationship(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_determine_relationship(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up with determine relationship spell!");
        break;
    }
}                               /* end Cast_Determine_Relationship */



void
cast_disembody(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
               OBJ_DATA * tar_obj)
{                               /* begin Cast_Disembody */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_disembody(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_disembody(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
      
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
        spell_disembody(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_disembody(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up with determine relationship spell!");
        break;
    }
}                               /* end Cast_Disembody */


void
cast_dispel_magick(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_Dispel_Magick */
    struct char_data *temp;
    sh_int magick_type = get_spell_magick_type(type, ch);
    char buf[MAX_STRING_LENGTH];
    int dir = -1;

    sprintf (buf, "%s", arg);

    struct room_data *other_room;
    struct room_direction_data *back;

    for (; *arg && (*arg == ' '); arg++);

    dir = get_dir_from_name(arg);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_dispel_magick(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch && !tar_obj)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch && !tar_obj) {
	send_to_char("Use on who or what?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        if (dir > -1) {
            if (!ch->in_room->direction[dir]) {
                act("There is no exit from here in that direction.", FALSE, ch, 0, 0, TO_CHAR);
                return;
            }

            if (IS_SET(ch->in_room->room_flags, RFL_SPL_ALARM)) {
		remove_alarm_spell(ch->in_room);
                send_to_room("A faint ringing is briefly audible before fading " "away.\n\r",
                             ch->in_room);
            }

            /* WALL OF SAND CHECK */
            if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL)) {
                REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL);
                sprintf(buf, "A giant wall of sand descends, opening the %s " "exit.\n\r",
                        dirs[dir]);
                send_to_room(buf, ch->in_room);
                if (((other_room = EXIT(ch, dir)->to_room))
                    && ((back = other_room->direction[rev_dir[dir]]))
                    && (back->to_room == ch->in_room)) {
                    REMOVE_BIT(back->exit_info, EX_SPL_SAND_WALL);
                    sprintf(buf, "A giant wall of sand descends, opening the " "%s exit.\n\r",
                            dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                }
            }
            /* WALL OF FIRE CHECK */
            if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_FIRE_WALL)) {
                REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_FIRE_WALL);
                sprintf(buf, "A giant wall of fire dies away, opening the %s " "exit.\n\r",
                        dirs[dir]);
                send_to_room(buf, ch->in_room);
                if (((other_room = EXIT(ch, dir)->to_room))
                    && ((back = other_room->direction[rev_dir[dir]]))
                    && (back->to_room == ch->in_room)) {
                    REMOVE_BIT(back->exit_info, EX_SPL_FIRE_WALL);
                    sprintf(buf, "A giant wall of fire dies away, opening the " "%s exit.\n\r",
                            dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                }
            }
            /* WALL OF WIND CHECK */
            if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL)) {
                REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL);
                sprintf(buf, "A giant wall of wind blows out, opening the %s " "exit.\n\r",
                        dirs[dir]);
                send_to_room(buf, ch->in_room);
                if (((other_room = EXIT(ch, dir)->to_room))
                    && ((back = other_room->direction[rev_dir[dir]]))
                    && (back->to_room == ch->in_room)) {
                    REMOVE_BIT(back->exit_info, EX_SPL_WIND_WALL);
                    sprintf(buf, "A giant wall of wind blows out, opening the " "%s exit.\n\r",
                            dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                }
            }
            /* WALL OF THORN CHECK */
            if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_THORN_WALL)) {
                REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_THORN_WALL);
                sprintf(buf,
                        "A giant wall of thorns shrinks into the ground"
                        ", opening the %s exit.\n\r", dirs[dir]);
                send_to_room(buf, ch->in_room);
                if (((other_room = EXIT(ch, dir)->to_room))
                    && ((back = other_room->direction[rev_dir[dir]]))
                    && (back->to_room == ch->in_room)) {
                    REMOVE_BIT(back->exit_info, EX_SPL_THORN_WALL);
                    sprintf(buf,
                            "A giant wall of thorns shrinks into the "
                            "ground, opening the %s exit.\n\r", dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                }
            }
            /* BLADE BARRIER  CHECK */
            if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER)) {
                REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER);
                sprintf(buf, "A mass of spinning blades fades away, opening " "the %s exit.\n\r",
                        dirs[dir]);
                send_to_room(buf, ch->in_room);
                if (((other_room = EXIT(ch, dir)->to_room))
                    && ((back = other_room->direction[rev_dir[dir]]))
                    && (back->to_room == ch->in_room)) {
                    REMOVE_BIT(back->exit_info, EX_SPL_BLADE_BARRIER);
                    sprintf(buf,
                            "A mass of spinning blades fades away, " "opening the %s exit.\n\r",
                            dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                }
            }
            return;
	}

	tar_ch = get_char_room_vis(ch, arg);
	if (tar_ch) {
	  spell_dispel_magick(level, magick_type, ch, tar_ch, 0);
	  return;
	}

	tar_obj = get_obj_room_vis(ch, arg);
	if (tar_obj) {
	  spell_dispel_magick(level, magick_type, ch, 0, tar_obj);
	  return;
	}

        // At this point arg is not a direction, character, or object 
	send_to_char("Nobody here by that name.\n\r", ch);
	
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_dispel_magick(level, magick_type, ch, 0, 0);
        tar_ch = ch->in_room->people;
        while (tar_ch) {
            temp = tar_ch->next_in_room;
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    spell_dispel_magick(level, magick_type, ch, tar_ch, tar_obj);
            tar_ch = temp;
        }

        if (IS_SET(ch->in_room->room_flags, RFL_SPL_ALARM)) {
            remove_alarm_spell(ch->in_room);
            send_to_room("A faint ringing is briefly audible before fading " "away.\n\r",
                         ch->in_room);
        }

        for (dir = 0; dir < 6; dir++)
            if (ch->in_room->direction[dir]) {
                /* WALL OF SAND CHECK */
                if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL)) {
                    REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL);
                    sprintf(buf, "A giant wall of sand descends, opening the %s " "exit.\n\r",
                            dirs[dir]);
                    send_to_room(buf, ch->in_room);
                    if (((other_room = EXIT(ch, dir)->to_room))
                        && ((back = other_room->direction[rev_dir[dir]]))
                        && (back->to_room == ch->in_room)) {
                        REMOVE_BIT(back->exit_info, EX_SPL_SAND_WALL);
                        sprintf(buf, "A giant wall of sand descends, opening the " "%s exit.\n\r",
                                dirs[rev_dir[dir]]);
                        send_to_room(buf, EXIT(ch, dir)->to_room);
                    }
                }
                /* WALL OF FIRE CHECK */
                if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_FIRE_WALL)) {
                    REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_FIRE_WALL);
                    sprintf(buf, "A giant wall of fire dies away, opening the %s " "exit.\n\r",
                            dirs[dir]);
                    send_to_room(buf, ch->in_room);
                    if (((other_room = EXIT(ch, dir)->to_room))
                        && ((back = other_room->direction[rev_dir[dir]]))
                        && (back->to_room == ch->in_room)) {
                        REMOVE_BIT(back->exit_info, EX_SPL_FIRE_WALL);
                        sprintf(buf, "A giant wall of fire dies away, opening the " "%s exit.\n\r",
                                dirs[rev_dir[dir]]);
                        send_to_room(buf, EXIT(ch, dir)->to_room);
                    }
                }
                /* WALL OF WIND CHECK */
                if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL)) {
                    REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL);
                    sprintf(buf, "A giant wall of wind blows out, opening the %s " "exit.\n\r",
                            dirs[dir]);
                    send_to_room(buf, ch->in_room);
                    if (((other_room = EXIT(ch, dir)->to_room))
                        && ((back = other_room->direction[rev_dir[dir]]))
                        && (back->to_room == ch->in_room)) {
                        REMOVE_BIT(back->exit_info, EX_SPL_WIND_WALL);
                        sprintf(buf, "A giant wall of wind blows out, opening the " "%s exit.\n\r",
                                dirs[rev_dir[dir]]);
                        send_to_room(buf, EXIT(ch, dir)->to_room);
                    }
                }
                /* WALL OF THORN CHECK */
                if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_THORN_WALL)) {
                    REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_THORN_WALL);
                    sprintf(buf,
                            "A giant wall of thorns shrinks into the ground"
                            ", opening the %s exit.\n\r", dirs[dir]);
                    send_to_room(buf, ch->in_room);
                    if (((other_room = EXIT(ch, dir)->to_room))
                        && ((back = other_room->direction[rev_dir[dir]]))
                        && (back->to_room == ch->in_room)) {
                        REMOVE_BIT(back->exit_info, EX_SPL_THORN_WALL);
                        sprintf(buf,
                                "A giant wall of thorns shrinks into the "
                                "ground, opening the %s exit.\n\r", dirs[rev_dir[dir]]);
                        send_to_room(buf, EXIT(ch, dir)->to_room);
                    }
                }
                /* BLADE BARRIER  CHECK */
                if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER)) {
                    REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER);
                    sprintf(buf,
                            "A mass of spinning blades fades away, opening " "the %s exit.\n\r",
                            dirs[dir]);
                    send_to_room(buf, ch->in_room);
                    if (((other_room = EXIT(ch, dir)->to_room))
                        && ((back = other_room->direction[rev_dir[dir]]))
                        && (back->to_room == ch->in_room)) {
                        REMOVE_BIT(back->exit_info, EX_SPL_BLADE_BARRIER);
                        sprintf(buf,
                                "A mass of spinning blades fades away, " "opening the %s exit.\n\r",
                                dirs[rev_dir[dir]]);
                        send_to_room(buf, EXIT(ch, dir)->to_room);
                    }
                }
            }
        break;
    default:
        gamelog("Serious screw-up in dispel magick spell!");
        break;
    }
}                               /* end Cast_Dispel_Magick */


void
cast_dragon_bane(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                 OBJ_DATA * tar_obj)
{                               /* begin Cast_Dragon_Bane */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {

    case SPELL_TYPE_POTION:
        send_to_char("A spasm twists through your stomach.\n\r", ch);
        spell_dragon_bane(level, magick_type, ch, ch, 0);
        return;

    case SPELL_TYPE_SPELL:
        spell_dragon_bane(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!victim)
	victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
	if (!victim) {
	  send_to_char ("Use on who?\n\r", ch);
	  return;
	}
        spell_dragon_bane(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_dragon_bane(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Screw-up in dragon bane spell!");
        break;
    }
}                               /* end Cast_Dragon_Bane */


void
cast_dragon_drain(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                  OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        send_to_char("A racking pain tears through you.\n\r", ch);
        return;

    case SPELL_TYPE_SCROLL:
      if (!victim)
	victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
        }

    case SPELL_TYPE_SPELL:
        spell_dragon_drain(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_dragon_drain(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Screw-up with dragon drain spell!");
        break;
    }
}                               /* end Cast_Dragon_Drain */


void
cast_earthquake(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                OBJ_DATA * tar_obj)
{                               /* begin Cast_Earthquake */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_earthquake(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Serious screw-up in earthquake!");
        break;
    }
}                               /* end Cast_Earthquake */


void
cast_elemental_fog(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                   OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_elemental_fog(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
	  send_to_char ("Use on who?\n\r", ch);
	  return;
	}

    case SPELL_TYPE_SPELL:
        if (!victim)
            victim = ch;
        spell_elemental_fog(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (!IS_IMMORTAL(victim))
                if (ethereal_ch_compare(ch, victim))
                    spell_elemental_fog(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Serious screw-up in elemental fog!");
        break;
    }
}                               /* end Spell_Elemental_Fog */


void
cast_empower(byte level, CHAR_DATA * ch, const char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{                               /* Begin cast_empower */

    struct empower_data
    {                           /* empower_data structure */
        byte mana;
        byte health;
        byte stun;
        byte stamina;
        byte str;
        byte agl;
        byte wis;
        byte end;
        bool glow;
        bool hum;
    };                          /* empower_data structure */

    struct empower_data bonus[7] = {    /* empower_data_bonus structure */
        /* M   H  Stu Sta Str Agl Wis End  Glo    Hum */
        {1, 1, 1, 1, 0, 0, 0, 0, TRUE, FALSE},     /* wek */ // just to make it
        {2, 1, 1, 2, 0, 0, 0, 0, TRUE, FALSE},     /* yuqa */// easier to read.
        {4, 2, 2, 4, 0, 0, 0, 1, TRUE, FALSE},     /* kral */// -Nessalin
        {6, 2, 2, 8, 1, 1, 1, 2, FALSE, TRUE},     /* een */
        {8, 3, 3, 14, 2, 2, 1, 3, FALSE, TRUE},    /* pav */
        {10, 6, 6, 18, 3, 3, 2, 4, FALSE, TRUE},   /* sul */
        {12, 8, 8, 20, 4, 4, 3, 5, TRUE, TRUE}     /* mon */
    };                          /* empower_data_bonus structure */

    const char * const bonus_name[] = {      /* bonus_name structure */
        "mana",
        "health",
        "stun",
        "stamina",
        "strength",
        "agility",
        "wisdom",
        "endurance",
        "casting",
        "\n"
    };                          /* bonus_name structure */

    int amount, location;
    char arg1[256], arg2[256];
    OBJ_DATA *psiak, *mutur, *item;

    //sh_int magick_type = get_spell_magick_type( type, ch );

    switch (type) {             /* begin case (type) for spell types */
    case SPELL_TYPE_SPELL:

        arg = two_arguments(arg, arg1, sizeof(arg1), arg2, sizeof(arg2));

        if ((!*arg1) || (!*arg2)) {
            send_to_char("You cannot change the item in that manner.\n\r", ch);
            return;
        }

        if (!(item = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
            act("You do not carry that item.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        /* only mon is permanent with a component */
        if ((!(IS_IMMORTAL(ch))) && (level >= 7)) {     /* only mortals need components */
            psiak = get_component(ch, SPHERE_ENCHANTMENT, level);
            mutur = get_component(ch, SPHERE_ALTERATION, level);

            if (!psiak || !mutur) {
                send_to_char("You do not have the proper components for an" " empowerment.\n\r",
                             ch);
                return;
            }
            act("$p glows white and vanishes.", FALSE, ch, psiak, 0, TO_CHAR);
            act("$p scatters into fragments and disappears.", FALSE, ch, mutur, 0, TO_CHAR);
            extract_obj(psiak);
            extract_obj(mutur);
        }
        /* only mortals need components */
        if ((IS_SET(item->obj_flags.extra_flags, OFL_MAGIC))
            || ((IS_SET(item->obj_flags.extra_flags, OFL_GLYPH))
                || (!number(0, 6)))) {
            /* destroy item sometimes or automatically if it has magic or glyph flag */
            act("$p is overpowered with magick and explodes!", TRUE, ch, item, 0, TO_CHAR);
            act("$n reels backward from a brilliantly white explosion.", TRUE, ch, 0, 0, TO_ROOM);
            if (GET_HIT(ch) > 15)
                adjust_hit(ch, -10);
            else
                adjust_hit(ch, -5);
            extract_obj(item);

            return;
        }
        /* destroy item sometimes */
        type = old_search_block(arg2, 0, strlen(arg2), bonus_name, 0);
        if (type == -1) {
            act("You may not change the item in that manner.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        type--;
        level--;                /* Need to decrement to index properly - Tiernan */
        switch (type) {         /* switch statement to determine type of enchantment */
        case 0:
            amount = bonus[(int) level].mana;
            location = CHAR_APPLY_MANA;
            break;
        case 1:
            amount = bonus[(int) level].health;
            location = CHAR_APPLY_HIT;
            break;
        case 2:
            amount = bonus[(int) level].stun;
            location = CHAR_APPLY_STUN;
            break;
        case 3:
            amount = bonus[(int) level].stamina;
            location = CHAR_APPLY_MOVE;
            break;
        case 4:
            amount = bonus[(int) level].str;
            location = CHAR_APPLY_STR;
            break;
        case 5:
            amount = bonus[(int) level].agl;
            location = CHAR_APPLY_AGL;
            break;
        case 6:
            amount = bonus[(int) level].wis;
            location = CHAR_APPLY_WIS;
            break;
        case 7:
            amount = bonus[(int) level].end;
            location = CHAR_APPLY_END;
            break;
        case 8:
            if (!IS_SET(item->obj_flags.extra_flags, OFL_CAST_OK))
                MUD_SET_BIT(item->obj_flags.extra_flags, OFL_CAST_OK);
            MUD_SET_BIT(item->obj_flags.extra_flags, OFL_MAGIC);
            if (level < 6) {
                if (!IS_SET(item->obj_flags.extra_flags, OFL_NOSAVE))
                    MUD_SET_BIT(item->obj_flags.extra_flags, OFL_NOSAVE);
                new_event(EVNT_REMOVE_OBJECT, (level + 1) * 3000, 0, 0, item, 0, 0);
            }
	    set_creator_edesc(ch, item);
            act("You have enchanted the item.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        default:
            gamelog("unknown modifier type in spell_empower");
            return;
        }                       /* switch statemnt to determine enchantment type */

        if (amount == 0) {
            act("You may not change the item in that manner.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (level < 6) {        /* Was 5 (sul), now only 6 (mon) is exempt. - Tiernan */
            /* These are temporarily empowered, don't save them - Tiernan */
            if (!IS_SET(item->obj_flags.extra_flags, OFL_NOSAVE))
                MUD_SET_BIT(item->obj_flags.extra_flags, OFL_NOSAVE);
            new_event(EVNT_REMOVE_OBJECT, (level + 1) * 3500, 0, 0, item, 0, 0);
        }
        item->affected[0].modifier = amount;
        item->affected[0].location = location;
        if (bonus[(int) level].glow && !IS_SET(item->obj_flags.extra_flags, OFL_GLOW)) {
            MUD_SET_BIT(item->obj_flags.extra_flags, OFL_GLOW);
            act("It begins to glow.", FALSE, ch, 0, 0, TO_CHAR);
        }
        if (bonus[(int) level].hum && !IS_SET(item->obj_flags.extra_flags, OFL_HUM)) {
            MUD_SET_BIT(item->obj_flags.extra_flags, OFL_HUM);
            act("It begins to hum.", FALSE, ch, 0, 0, TO_CHAR);
        }
        MUD_SET_BIT(item->obj_flags.extra_flags, OFL_MAGIC);
	set_creator_edesc(ch, item);
        act("You have enchanted the item.", FALSE, ch, 0, 0, TO_CHAR);
        break;                  /* for spell_type_spell */
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_POTION:
        send_to_char("A surge of pleasure spasms through your body.\n\r", ch);
        break;
    }                           /* end case (type) for spell types */
}                               /* end Cast_Empower */


void
cast_energy_shield(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                   OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_energy_shield(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
      if (!victim)
	victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
	  send_to_char ("Use on who?\n\r", ch);
	  return;
	}
    case SPELL_TYPE_SPELL:
        spell_energy_shield(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (!IS_IMMORTAL(victim))
                if (ethereal_ch_compare(ch, victim))
                    spell_energy_shield(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Screw-up in energy shield spell!");
        break;
    }
}                               /* end Spell_Energy_Shield */


void
cast_ethereal(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Ethereal */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_ethereal(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch && !tar_obj)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (!tar_ch && !tar_obj) {
            send_to_char("Use it on what or who?\n\r", ch);
            return;
        }
        if (tar_ch != ch) {
            send_to_char("You can only use this on yourself.\n\r", ch);
            return;
        }
    case SPELL_TYPE_SPELL:
        if (((tar_ch) && (!(is_char_ethereal(tar_ch))))
            || ((tar_obj) && (!IS_SET(tar_obj->obj_flags.extra_flags, OFL_ETHEREAL))))
            spell_ethereal(level, magick_type, ch, tar_ch, tar_obj);
        else
            send_to_char("They are already on the Plane of Shadow.\n\r", ch);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!is_char_ethereal(tar_ch))
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_ethereal(level, magick_type, ch, tar_ch, tar_obj);
        break;
    default:
        gamelog("Screw-up with ethereal spell!");
        break;
    }
}                               /* end Cast_Ethereal */


void
cast_fear(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Fear */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_fear(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch){
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
    case SPELL_TYPE_SPELL:
        spell_fear(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_fear(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in Fear Spell!\n\r");
        break;
    }
}                               /* end Cast_Fear */


void
cast_feather_fall(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_feather_fall(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        if (!IS_AFFECTED(tar_ch, CHAR_AFF_FLYING))
            /* Needs to check for levitate and featherfall as well */
            spell_feather_fall(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("You feel a giddy surge of weightlessness.\n\r", ch);
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
        if (!IS_AFFECTED(ch, CHAR_AFF_FLYING))
            spell_feather_fall(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("You feel a giddy surge of weightlessness.\n\r", ch);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    if (!IS_AFFECTED(tar_ch, CHAR_AFF_FLYING))
                        spell_feather_fall(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in feather fall spell!");
        break;
    }
}                               /* end Cast_Feather_Fall */


void
cast_feeblemind(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{                               /* begin Cast_Feeblemind */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_feeblemind(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_feeblemind(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
        spell_feeblemind(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_feeblemind(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in feeblemind spell!");
        break;
    }
}                               /* end Cast_Feeblemind */


void
cast_fire_armor(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_fire_armor(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_fire_armor(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
        spell_fire_armor(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_fire_armor(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in fire armor spell!");
        break;
    }
}                               /* end Spell_Fire_Armor */

void
cast_shadow_armor(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_shadow_armor(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_shadow_armor(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
        spell_shadow_armor(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_shadow_armor(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in shadow armor spell!");
        break;
    }
}                               /* end Spell_Shadow_Armor */


void
cast_fire_jambiya(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Fire_Jambiya */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        act("A searing pain flashes through your hands.", FALSE, ch, 0, 0, TO_CHAR);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
    case SPELL_TYPE_SPELL:
        spell_fire_jambiya(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_fire_jambiya(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Something is wrong with Fire Jambiya!");
        break;
    }
}                               /* end Cast_Fire_Jambiya */


void
cast_fireball(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Fireball */
    sh_int magick_type = get_spell_magick_type(type, ch);
    CHAR_DATA *temp;

    switch (type) {

    case SPELL_TYPE_POTION:
        spell_fireball(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_fireball(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
        spell_fireball(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        victim = ch->in_room->people;
        while (victim) {
            temp = victim->next_in_room;
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_fireball(level, magick_type, ch, victim, 0);
            victim = temp;
        }
        break;

    default:
        gamelog("Screw-up in fireball spell!");
        break;
    }
}                               /* end Cast_Fireball */


void
cast_firebreather(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Firebreather */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_firebreather(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_firebreather(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char ("Use on who?\n\r", ch);
	  return;
	}
        spell_firebreather(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_firebreather(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in Firebreather spell!");
        break;
    }
}                               /* End Cast_Firebreather */


void
cast_flamestrike(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{                               /* begin Cast_Flamestrike */
    sh_int magick_type = get_spell_magick_type(type, ch);
    CHAR_DATA *temp;

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_flamestrike(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_flamestrike(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	  send_to_char("Use on who?\n\r", ch);
	  return;
	}
        spell_flamestrike(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        tar_ch = ch->in_room->people;
        while (tar_ch) {
            temp = tar_ch->next_in_room;
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_flamestrike(level, magick_type, ch, tar_ch, 0);
            tar_ch = temp;
        }
        break;
    default:
        gamelog("Serious screw-up in flamestrike!");
        break;
    }
}                               /* end Cast_Flamestrike */


void
cast_fluorescent_footsteps(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                           OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        tar_ch = ch;
        act("A crackling surge of energy surrounds your feet.", FALSE, ch, 0, 0, TO_CHAR);
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_fluorescent_footsteps(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_fluorescent_footsteps(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in fluorescent footprints!");
        break;
    }
}                               /* end Spell_Flourescent_Footprints */


void
cast_fly(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Fly */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_fly(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_fly(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_fly(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in Fly!");
        break;
    }
}                               /* end Cast_Fly */


void
cast_forbid_elements(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                     OBJ_DATA * tar_obj)
{                               /* begin Cast_Forbid_Elements */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        send_to_char("Dizziness washes over you.\n\r", ch);
        break;

    case SPELL_TYPE_SPELL:
        spell_forbid_elements(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (tar_ch) {
            act("$N seems to be unaffected.", FALSE, ch, 0, tar_ch, TO_CHAR);
            return;
        }
        spell_forbid_elements(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_forbid_elements(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Screw-up in forbid elements spell!");
        break;
    }
}                               /* end Cast_Forbid_Elements */


void
cast_fury(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Fury */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_fury(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_fury(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_fury(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in fury spell!");
        break;
    }
}                               /* end Cast_Fury */

void
cast_gate(byte level, CHAR_DATA * ch, const char *arg, int type, CHAR_DATA * tar_ch,
          OBJ_DATA * tar_obj)
{                               /* cast_Gate */
    char name[256], bug[MAX_STRING_LENGTH];
    int i, too_many = 0;

    OBJ_DATA *component;
    CHAR_DATA *victim;
    CHAR_DATA *eatme;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    sh_int magick_type = get_spell_magick_type(type, ch);

    struct demon_type
    {
        char *name;
        int comp1;
        int comp2;
        int comp3;
    };

    /* Structure that holds all the named demons. */
    /* Demon name, followed by up to 3 components.  Only 1 is required to */
    /* cast the spell.  */
    struct demon_type demon_name[] = {
        {"rubicant", 701, 702, -1},
        {"milon", 708, 709, -1},
        {"vapus", 704, 707, -1},
        {"allo-ver", 721, 722, 703},
        {"gygagnos", 705, 706, 0},
        {"daegodola", 711, 712, -1},
        {"mespodeum", 713, 714, -1},
        {"antanen", 715, 716, -1},
        {"balbador", 717, 718, -1},
        {"hadon", 719, 720, -1},
        {"dyrinis", 37120, -1, -1},
        {"\n", 0, 0, 0}
    };

    victim = tar_ch;

    arg = one_argument(arg, name, sizeof(name));

    if (*name) {
        for (i = 0; demon_name[i].comp1 != 0; i++) {
            if (STRINGS_I_SAME(demon_name[i].name, name))
                break;
        }

        /* See if the demon is already summoned elsewhere.. don't want to */
        /* duplicate him anywhere.  */
        eatme = get_char_world(ch, name);
        if (eatme) {
            gamelog("enabling too_many because person found in world.");
            too_many = 1;
        }
        switch (i) {
        case 0:
            victim = read_mobile(1099, VIRTUAL);
            break;
        case 1:
            victim = read_mobile(1096, VIRTUAL);
            break;
        case 2:
            victim = read_mobile(1098, VIRTUAL);
            break;
        case 3:
            victim = read_mobile(1095, VIRTUAL);
            break;
        case 4:
            victim = read_mobile(1097, VIRTUAL);
            break;
        case 5:
            victim = read_mobile(1093, VIRTUAL);
            break;
        case 6:
            victim = read_mobile(1092, VIRTUAL);
            break;
        case 7:
            victim = read_mobile(1090, VIRTUAL);
            break;
        case 8:
            victim = read_mobile(1089, VIRTUAL);
            break;
        case 9:
            victim = read_mobile(1088, VIRTUAL);
            break;
        case 10:
            if (!(victim = get_char_world(ch, name))) {
                send_to_char("You try to summon him, but he's out wanking off " "somewhere.", ch);
                return;
            }
            break;
        default:
            gamelog("enabling too_many because NPC not found in case statement.");
            too_many = 1;
            break;
        }

        if (victim) {
            /* See if they have the normal gate component */
            if (!get_componentB
                (ch, SPHERE_CONJURATION, 6,
                 "$p folds in upon itself with an audible grinding noise.",
                 "$p folds in upon itself in $n's hands.",
                 "You lack the correct component to lure otherwordly creatures" " here.")) {
                if (victim && (i != 10))
                    extract_char(victim);
                return;
            }

            /* See if they have the demons component */
            if (!(component = get_component_nr(ch, demon_name[i].comp1))) {
                if (!(component = get_component_nr(ch, demon_name[i].comp2))) {
                    if (!(component = get_component_nr(ch, demon_name[i].comp3))) {
                        act("You do not seem to have the correct component for " "this summoning.",
                            FALSE, ch, 0, 0, TO_CHAR);
                        if (i != 10)
                            extract_char(victim);
                        return;
                    }
                }
            }

            /* They have both components.  */
            act("As you invoke your spell, $p glows brightly.", FALSE, ch, component, victim,
                TO_CHAR);
            act("$p glows brightly in $s hand.", FALSE, ch, component, victim, TO_ROOM);

            sprintf(bug, "The gate spell has successfully found a target in" " room: %d.\n\r",
                    ch->in_room->number);
            gamelog(bug);

            if (i == 10) {      /* Dyrinis summoning */
                act("You feel that someone is holding $p, which is yours.", FALSE, ch, component,
                    victim, TO_VICT);
                parse_command(victim, "shout Someone has stolen my dildo!");
                sprintf(bug, "goto %d", ch->in_room->number);
                parse_command(victim, bug);
                parse_command(victim, "say Who has my dildo?");
                act("$n wiggles $p in the air.", FALSE, ch, component, victim, TO_ROOM);
                act("You wiggle $p in the air.", FALSE, ch, component, victim, TO_CHAR);
                act("You feel the chains of slavery, as your will becomes $n's.", FALSE, ch,
                    component, victim, TO_VICT);
                parse_command(victim, "say Oh shit.");

                add_follower(victim, ch);

                af.type = SPELL_CHARM_PERSON;
                af.duration = (level / 2) + dice(1, 3);
                af.power = level;
                af.magick_type = magick_type;
                af.modifier = 0;
                af.location = 0;
                af.bitvector = CHAR_AFF_CHARM;
                affect_to_char(victim, &af);

                return;
            }
        }
        /* Whups.  Demon already exists, so he will not come */
        if (too_many) {
            act("You invoke the power of a true name, but there is no" " response.", FALSE, ch, 0,
                0, TO_CHAR);
            if (victim)
                extract_char(victim);
            return;
        }
    }

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_gate(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_WAND:
        send_to_char("A wave of vertigo makes you sway.\n\r", ch);
        break;
    default:
        gamelog("Screw up in gate spell.");
        break;
    }
}                               /* end Cast_Gate */


void
cast_glyph(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Glyph */
    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *i, *temp_obj;

    switch (type) {
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
        i = ch->in_room->contents;
        while (i) {
            temp_obj = i->next_content;
            if ((!IS_SET(i->obj_flags.extra_flags, OFL_GLYPH)) && (char_can_see_obj(ch, i)))
                spell_glyph(level, magick_type, ch, 0, i);
            i = temp_obj;
        }

        send_to_room("A brilliant glow suffuses the air momentarily.\n\r", ch->in_room);
        break;

    case SPELL_TYPE_POTION:
        send_to_char("Your hands burn and tingle.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
        if ((!tar_obj) || (tar_ch)) {
            send_to_char("Only objects may be glyphed.\n\r", ch);
            return;
        }
    case SPELL_TYPE_SPELL:
        if (!IS_SET(tar_obj->obj_flags.extra_flags, OFL_GLYPH))
            spell_glyph(level, magick_type, ch, 0, tar_obj);
        else
            send_to_char("It is already enchanted with a glyph.\n\r", ch);
        break;
    default:
        gamelog("Screw-up in glyph spell!");
        break;
    }
}                               /* end Cast_Glyph */


void
cast_godspeed(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Godspeed */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        if (!affected_by_spell(tar_ch, SPELL_GODSPEED))
            spell_godspeed(level, magick_type, ch, ch, 0);
        else
            send_to_char("Your heart races.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        if (!affected_by_spell(tar_ch, SPELL_GODSPEED))
            spell_godspeed(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("Your heart races.\n\r", ch);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!affected_by_spell(tar_ch, SPELL_GODSPEED))
                    if (!IS_IMMORTAL(tar_ch))
                        if (ethereal_ch_compare(ch, tar_ch))
                            spell_godspeed(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in godspeed!");
        break;
    }
}                               /* end Cast_Godspeed */


void
cast_golem(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Golem */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_STAFF:
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_golem(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Something is wrong with Golem!");
        break;
    }
}                               /* end Cast_Golem */


void
cast_guardian(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Guardian */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_guardian(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_WAND:
        send_to_char("The air ripples and shimmers.\n\r", ch);
        break;
    default:
        gamelog("Screw up in gate spell!");
        break;
    }
}                               /* end Cast_Guardian */


void
cast_hands_of_wind(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_Hands_Of_Wind */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
        if ((!tar_ch) || (tar_obj))
            tar_ch = ch;
        level = MIN(1, level - 1);
    case SPELL_TYPE_SPELL:
        if (is_merchant(tar_ch))
            send_to_char("Your attempt failed.\n\r", ch);
        else
            spell_hands_of_wind(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_WAND:
        send_to_char("Fitful breezes swirl around you.\n\r", ch);
        break;
    default:
        gamelog("Something is wrong with the Hands of Wind spell.");
        break;
    }
}                               /* end Cast_Hands_Of_Winds */


void
cast_haunt(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_haunt */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SPELL:
        spell_haunt(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_char("The shadows darken.\n\r", ch);
        break;
    default:
        gamelog("Screw up in Haunt spell.");
        break;
    }
}


void
cast_heal(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Heal */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_heal(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        if (ch->in_room->sector_type != SECT_NILAZ_PLANE) {
            if (ch == tar_ch) {
                act("$n heals $mself.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
                act("You heal yourself.", FALSE, ch, 0, tar_ch, TO_CHAR);
            } else {
                act("$n heals $N.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
                act("You heal $N.", FALSE, ch, 0, tar_ch, TO_CHAR);
            }
        }
        spell_heal(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_heal(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch)) {
                    spell_heal(level, magick_type, ch, tar_ch, 0);
                    act("A glowing aura briefly forms around $n as $e heals $N.", FALSE, ch, 0,
                        tar_ch, TO_NOTVICT);
                    act("A glowing aura briefly forms around $n as $e heals you.", FALSE, ch, 0,
                        tar_ch, TO_VICT);
                    act("A glowing aura briefly forms around you as you send forth"
                        " your healing energies.", FALSE, ch, 0, tar_ch, TO_CHAR);
                }
        break;
    default:
        gamelog("Serious screw-up in heal!");
        break;
    }
}                               /* end Cast_Heal */


void
cast_health_drain(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Health_Drain */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_health_drain(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_health_drain(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_health_drain(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_health_drain(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Something is wrong with health drain.");
        break;
    }
}                               /* end Cast_Health_Drain */


void
cast_hero_sword(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Hero_Sword */
    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *i, *temp_obj;

    switch (type) {
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
        i = ch->in_room->contents;
        while (i) {
            temp_obj = i->next_content;
            if (char_can_see_obj(ch, i))
	      spell_hero_sword(level, magick_type, ch, 0, i);
            i = temp_obj;
        }

        send_to_room("Hero Sword room message.\n\r", ch->in_room);
        break;

    case SPELL_TYPE_POTION:
        send_to_char("Your vision wavers with the memories of others.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
        if ((!tar_obj) || (tar_ch)) {
            send_to_char("This spell can only be used on objects.\n\r", ch);
            return;
        }
    case SPELL_TYPE_SPELL:
      spell_hero_sword(level, magick_type, ch, 0, tar_obj);
      break;
    default:
        gamelog("Screw-up in hero sword spell!");
        break;
    }
}                               /* end Cast_Hero_Sword */


void
cast_identify(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Identify */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_identify(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_identify(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_identify(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_identify(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in identify spell!");
        break;
    }
}                               /* end Cast_Identify */


void
cast_illuminant(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{                               /* begin Cast_Illuminant */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SPELL:
        spell_illuminant(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
        spell_illuminant(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_illuminant(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Screwup with the illuminant spell.");
        break;
    }
}                               /* end Cast_Illuminant */


void
cast_immolate(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Immolate */
    sh_int magick_type = get_spell_magick_type(type, ch);
    CHAR_DATA *temp;

    switch (type) {

    case SPELL_TYPE_POTION:
        spell_immolate(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_immolate(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_immolate(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        victim = ch->in_room->people;
        while (victim) {
            temp = victim->next_in_room;
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_immolate(level, magick_type, ch, victim, 0);
            victim = temp;
        }
        break;

    default:
        gamelog("Screw-up in immolate spell in spells.c!");
        break;
    }
}                               /* end spell immolate */


void
cast_infravision(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{                               /* begin Cast_Infravision */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        if (!affected_by_spell(ch, SPELL_INFRAVISION))
            spell_infravision(level, magick_type, ch, ch, 0);
        else
            send_to_char("Your vision blurs.\n\r", ch);
        break;

    case SPELL_TYPE_SPELL:
        spell_infravision(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_infravision(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_infravision(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in infravision!");
        break;
    }
}                               /* end Cast_Infravision */


void
cast_insomnia(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Insomnia */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_insomnia(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_insomnia(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_insomnia(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_insomnia(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in insomnia!");
        break;
    }
}                               /* End Cast_Insomnia */



void
cast_intoxication(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Intoxication */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_intoxication(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_intoxication(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_intoxication(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_intoxication(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Something is wrong with that nifty Intoxication spell!");
        break;
    }
}


void
cast_invisibility(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Invisibility */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_invisibility(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        if (!tar_ch)
            tar_ch = ch;
        spell_invisibility(level, magick_type, ch, tar_ch, tar_obj);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch && !tar_obj)
	  tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (!tar_ch && !tar_obj) {
            send_to_char("Use it on who or what?\n\r", ch);
            return;
        }
        spell_invisibility(level, magick_type, ch, tar_ch, tar_obj);
        break;

    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_invisibility(level, magick_type, ch, tar_ch, 0);
        for (tar_obj = ch->in_room->contents; tar_obj; tar_obj = tar_obj->next_content)
            if (char_can_see_obj(ch, tar_obj))
                spell_invisibility(level, magick_type, ch, 0, tar_obj);
        break;
    default:
        gamelog("Serious screw-up in invisibility!");
        break;
    }
}                               /* end Cast_Invisibility */


void
cast_invulnerability(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                     struct obj_data *tar_obj)
{                               /* begin Cast_Invulnerability */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_invulnerability(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        if (!affected_by_spell(tar_ch, SPELL_INVULNERABILITY))
            spell_invulnerability(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("Nothing seems to happen.\n\r", ch);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_invulnerability(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Something is wrong with invulnerability");
        break;
    }
}                               /* end Cast_Invulnerability */


void
cast_levitate(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Levitate */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_levitate(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_levitate(level, magick_type, ch, tar_ch, tar_obj);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    if (!affected_by_spell(tar_ch, SPELL_LEVITATE))
                        spell_levitate(level, magick_type, ch, tar_ch, 0);
        for (tar_obj = ch->in_room->contents; tar_obj; tar_obj = tar_obj->next_content)
            if (char_can_see_obj(ch, tar_obj))
                spell_levitate(level, magick_type, ch, 0, tar_obj);
        break;
    default:
        gamelog("Serious screw-up in levitate!");
        break;
    }
}                               /* end Cast_Levitate */


void
cast_lightning_bolt(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                    OBJ_DATA * tar_obj)
{                               /* begin Cast_Lightning_Bolt */
    CHAR_DATA *temp;
    CHAR_DATA *tar_ch;
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_lightning_bolt(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        if (ch->in_room->sector_type == SECT_WATER_PLANE) {
            act("The waters here cause the lightning to arc out of control.", TRUE, ch, 0, 0,
                TO_ROOM);
            send_to_char("The waters here cause the lightning to arc out of control.\n\r", ch);
            tar_ch = ch->in_room->people;
            while (tar_ch) {
                temp = tar_ch->next_in_room;
                if (tar_ch != ch)
                    spell_lightning_bolt(level, magick_type, ch, tar_ch, 0);
                tar_ch = temp;
            }
        } else
            spell_lightning_bolt(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_lightning_bolt(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        tar_ch = ch->in_room->people;
        while (tar_ch) {
            temp = tar_ch->next_in_room;
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_lightning_bolt(level, magick_type, ch, tar_ch, 0);
            tar_ch = temp;
        }
        break;
    default:
        gamelog("Screw-up in lightning bolt!");
        break;

    }
}                               /* end Cast_Lightning_Bolt */


/* Added 4/06/2005 - Halaster */
void
cast_lightning_spear(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                     OBJ_DATA * tar_obj)
{                               /* begin Cast_Lightning_Spear */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
      spell_lightning_spear(level, magick_type, ch, ch, 0);

    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
      spell_lightning_spear(level, magick_type, ch, tar_ch, 0);
      break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_lightning_spear(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in Lightning Spear in spells.c!");
    }
}                               /* end Cast_Lightning_Spear */


void
cast_lightning_storm(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                     OBJ_DATA * tar_obj)
{                               /* begin Cast_Lightning_Storm */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_lightning_storm(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_WAND:
        send_to_char("A slow shiver runs through your body.\n\r", ch);
        break;
    default:
        gamelog("Screw-up in lightning storm!");
        break;
    }
}                               /* end Cast_Lightning_Storm */


void
cast_lightning_whip(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                    OBJ_DATA * tar_obj)
{                               /* begin Cast_Lightning_Whip */

    CHAR_DATA *tmp;
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
        tmp = ch->in_room->people;
        while (tmp) {
            if (!IS_IMMORTAL(tmp))
                if (ethereal_ch_compare(ch, tmp))
                    spell_lightning_whip(level, magick_type, ch, tmp, 0);
            tmp = tmp->next_in_room;
        }
        break;
    case SPELL_TYPE_POTION:
      spell_lightning_whip(level, magick_type, ch, ch, 0);
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
      spell_lightning_whip(level, magick_type, ch, tar_ch, 0);
      break;
    default:
        gamelog("Something is wrong with Lightning Whip!");
        break;
    }
}                               /* end Cast_Lightning_Whip */


void
cast_mark(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Mark */
    sh_int magick_type = get_spell_magick_type(type, ch);

    if (!tar_obj || tar_ch)
        return;

    switch (type) {
    case SPELL_TYPE_POTION:
        send_to_char("The world tilts sideways for a dizzy moment.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
        if (!tar_obj) {
            send_to_char("Only objects may be marked!.\n\r", ch);
            return;
        }
    case SPELL_TYPE_SPELL:
        if (!IS_SET(tar_obj->obj_flags.extra_flags, OFL_MAGIC))
            spell_mark(level, magick_type, ch, 0, tar_obj);
        else
            send_to_char("It cannot hold your enchantment.\n\r", ch);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_room("A brilliant glow suffuses the air momentarily.\n\r", ch->in_room);
        break;
    default:
        gamelog("Serious screw-up in mark!");
        break;
    }
}                               /* end Cast_Mark */


void
cast_messenger(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
               OBJ_DATA * tar_obj)
{                               /* begin Cast_Messenger */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_messenger(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_char("Reaching into the plane of Air, you find no messenger there.\n\r", ch);
        break;
    default:
        gamelog("Screw up in Messenger spell.");
        break;
    }
}                               /* end Cast_Messenger */


void
cast_mirage(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Mirage */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SPELL:
        spell_mirage(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Screw up with the mirage spell.");
        break;
    }
}                               /* end Cast_Mirage */


void
cast_mount(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Mount */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_mount(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A queasy, uneasy feeling fills your stomach.\n\r", ch);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_mount(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Screw up in mount spell.");
        break;
    }
}                               /* end Cast_Mount */


void
cast_oasis(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim, OBJ_DATA * tar_obj)
{                               /* begin Cast_Oasis */
    sh_int magick_type = get_spell_magick_type(type, ch);

    if (IS_SET(ch->in_room->room_flags, RFL_INDOORS)) {
        send_to_char("There's not enough room here to create an oasis.\n\r", ch);
        return;
    }

    switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_oasis(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A faint shimmer of moisture spreads across the ground.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in oasis!");
        break;
    }
}                               /* end Cast_Oasis */


void
cast_paralyze(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_paralyze(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_paralyze(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_paralyze(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_paralyze(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in paralyze!");
        break;
    }
}                               /* end Spell_Paralyze */


void
cast_parch(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Parch */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_parch(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SPELL:
        spell_parch(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_parch(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_parch(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in parch!");
        break;
    }
}                               /* End Cast_Parch */


void
cast_phantasm(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Phantasm */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_phantasm(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Screwup with the phantasm spell.");
        break;
    }
}                               /* end Cast_Phantasm */


void
cast_poison(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Poison */
    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *i;

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_poison(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_ch && !tar_obj) 
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch && !tar_obj) {
	send_to_char("Use it on who or what?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        spell_poison(level, magick_type, ch, tar_ch, tar_obj);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        i = ch->in_room->contents;
        while (i) {
            tar_obj = i->next_content;
            if ((i->obj_flags.type == ITEM_DRINKCON) || (i->obj_flags.type == ITEM_FOOD))
                spell_poison(level, magick_type, ch, 0, i);
            i = tar_obj;
        }
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_poison(level, magick_type, ch, tar_ch, 0);
        send_to_room("A dark mist fills the air for a moment.\n\r", ch->in_room);
        break;
    default:
        gamelog("Serious screw-up in poison!");
        break;
    }
}                               /* end Cast_Poison */


void
cast_portable_hole(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_Portable_Hole */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SPELL:
        spell_portable_hole(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (tar_ch) {
            act("$N seems to be unaffected.", FALSE, ch, 0, tar_ch, TO_CHAR);
            return;
        }
        spell_portable_hole(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_portable_hole(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_WAND:
        send_to_char("A wave of vertigo overcomes you.\n\r", ch);
        /* Note for later: should reset their position to resting */
        break;
    default:
        gamelog("Serious screw-up in portable hole!");
        break;
    }
}                               /* end Cast_Portable_hole */


void
cast_portal(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Portal */
    sh_int magick_type = get_spell_magick_type(type, ch);
    char buf[MAX_STRING_LENGTH];

    if (tar_obj && !find_ex_description("[SPELL_MARK]", tar_obj->ex_description, TRUE)) {
        sprintf(buf, "There is no mark on %s.\n\r", format_obj_to_char(tar_obj, ch, 1));
        send_to_char(buf, ch);
        return;
    }

    switch (type) {
    case SPELL_TYPE_SPELL:
        spell_portal(level, magick_type, ch, tar_ch, tar_obj);
        break;
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_ROOM:
        send_to_char("A wave of vertigo overcomes you.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in portal!");
        break;
    }
}


void
cast_possess_corpse(byte level, CHAR_DATA * ch, const char *arg, int type, CHAR_DATA * tar_ch,
                    OBJ_DATA * tar_obj)
{
    /* begin Cast_Possess_corpse */

    char buf[256];
    sh_int magick_type = get_spell_magick_type(type, ch);

    arg = one_argument(arg, buf, sizeof(buf));

    switch (type) {
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_char("A hint of chill enters the air.\n\r", ch);
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A putrid taste fills your mouth, making you gag.\n\r", ch);
        break;
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
        if (tar_ch) {
            send_to_char("They're not dead yet!\n\r", ch);
            return;
        }
        if (!tar_obj) {
            send_to_char("A hint of chill enters the air.\n\r", ch);
            return;
        }
    case SPELL_TYPE_SPELL:
        spell_possess_corpse(level, magick_type, ch, 0, tar_obj);
        break;
    default:
        gamelog("Serious screw-up in possess corpse!");
        break;
    }
}                               /* end Cast_possess_corpse */


void
cast_pseudo_death(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Pseudo_death */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        if ((!tar_obj) || (!tar_ch))
            return;
        spell_pseudo_death(level, magick_type, ch, tar_ch, tar_obj);
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A deadly chill sweeps through you.\n\r", ch);
        break;
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_char("The air takes on a clammy, unsettling feel.\n\r", ch);
        break;
    default:
        gamelog("Screw up in pseudo_death spell.");
        break;
    }
}                               /* end Cast_Pseudo_death */


void
cast_psionic_drain(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_psionic_drain */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_psionic_drain(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_psionic_drain(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_SCROLL:
      if (!victim)
	victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_psionic_drain(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_psionic_drain(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Screw-up in psionic_drain spell!");
        break;
    }
}                               /* end Cast_psionic_drain */


void
cast_psionic_suppression(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                         OBJ_DATA * tar_obj)
{                               /* begin Cast_Psionic_Suppression */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_psionic_suppression(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            send_to_char("It seems unaffected by your magicks.\n\r", ch);
            return;
        }
        if (!tar_ch) {
	  send_to_char ("Use on who?\n\r", ch);
	  return;
	}
    case SPELL_TYPE_SPELL:
        spell_psionic_suppression(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (tar_ch != ch)
                if (!(IS_AFFECTED(tar_ch, CHAR_AFF_PSI)))
                    if (!IS_IMMORTAL(tar_ch))
                        if (ethereal_ch_compare(ch, tar_ch))
                            spell_psionic_suppression(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Psionic suppression is fucked up!");
        break;
    }
}                               /* end Cast_Psionic_Supression */


void
cast_pyrotechnics(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Pyrotechnics */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_pyrotechnics(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Something is wrong with pyrotechnics.");
        break;
    }
}                               /* end Cast_Pyrotechnics */


void
cast_quickening(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{                               /* begin Cast_Quickening */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        if (!affected_by_spell(ch, SPELL_QUICKENING))
            spell_quickening(level, magick_type, ch, ch, 0);
        else
            send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        if (!affected_by_spell(tar_ch, SPELL_QUICKENING))
            spell_quickening(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("Nothing seems to happen.\n\r", ch);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!affected_by_spell(tar_ch, SPELL_QUICKENING))
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_quickening(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in quickening!");
        break;
    }
}                               /* end Cast_Quickening */


void
cast_rain_of_fire(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Rain_Of_Fire */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        if (IS_SET(ch->in_room->room_flags, RFL_INDOORS)) {
            send_to_char("Full view of the sky is needed to bring the fires.\n\r", ch);
            break;
        }
        spell_rain_of_fire(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A giddy rush of warmth fills your stomach.\n\r", ch);
        spell_firebreather(level, magick_type, ch, ch, 0);
        break;
    default:
        gamelog("Something is wrong with Rain of Fire.");
        break;
    }
}                               /* end Cast_Rain_Of_Fire */


void
cast_refresh(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{                               /* begin Cast_Refresh */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_refresh(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_refresh(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_refresh(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in Refresh!");
        break;
    }
}                               /* end Cast_Refresh */


void
cast_regenerate(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_regenerate(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_regenerate(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_regenerate(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in regenerate!");
        break;
    }
}                               /* end Spell_Regenerate */


void
cast_relocate(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Relocate */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_relocate(level, magick_type, ch, tar_ch, tar_obj);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_char("Your surroundings shimmer and ripple momentarily.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up with Relocate spell!");
        break;
    }
}                               /* end Cast_Relocate */


void
cast_remove_curse(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Remove_Curse          */
    /* Do we even need this spell?      */
    /* Yes, to remove curses on people. */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_remove_curse(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_remove_curse(level, magick_type, ch, tar_ch, tar_obj);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_remove_curse(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in remove curse!");
        break;
    }
}                               /* end Cast_Remove_Curse */


void
cast_repair_item(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{                               /* begin cast_repair_item */
    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *i, *temp_obj;

    switch (type) {
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
        i = ch->in_room->contents;
        while (i) {
            temp_obj = i->next_content;
            if (char_can_see_obj(ch, i))
                spell_repair_item(level, magick_type, ch, 0, i);
            i = temp_obj;
        }

        send_to_room("A dull, brown glow fills the area momentarily.\n\r", ch->in_room);
        break;

    case SPELL_TYPE_POTION:
        send_to_char("Your hands itch and tingle.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
      if (!tar_obj && !tar_ch) {
        send_to_char("Your hands itch and tingle.\n\r", ch);
	break;
      }
    case SPELL_TYPE_WAND:
        if ((!tar_obj) || (tar_ch)) {
            send_to_char("Only objects may be repaired.\n\r", ch);
            return;
        }
    case SPELL_TYPE_SPELL:
        if (char_can_see_obj(ch, tar_obj))
            spell_repair_item(level, magick_type, ch, 0, tar_obj);
        break;
    default:
        gamelog("Screw-up in repair item spell in spells.c!");
        break;
    }
}                               /* end Cast_Repair_item */


void
cast_repel(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim, OBJ_DATA * tar_obj)
{                               /* begin Cast_Repel */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_repel(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_repel(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Something is wrong with repel.");
        break;
    }
}                               /* end Cast_Repel */


void
cast_restful_shade(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_Restful_Shade */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_WAND:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;

    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_restful_shade(level, magick_type, ch, 0, 0);
        break;

    default:
        gamelog("Serious screw-up in restful_shade!");
        break;
    }
}                               /* end Cast_Restful_Shade */


void
cast_rewind(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Rewind */
    sh_int magick_type = get_magick_type(ch);
    send_to_char("A sigil burns brightly in the air, then vanishes.\n", ch);
    spell_rewind(level, magick_type, ch, 0, 0);
}

void
cast_rocky_terrain(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_rocky_terrain */
    sh_int magick_type = get_spell_magick_type(type, ch);
    spell_rocky_terrain(level, magick_type, ch, 0, 0);
}


void
cast_rot_items(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
               OBJ_DATA * tar_obj)
{                               /* begin Cast_Rot_Items */
    sh_int magick_type = get_spell_magick_type(type, ch);
    CHAR_DATA *temp;

    switch (type) {

    case SPELL_TYPE_POTION:
        spell_rot_items(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SPELL:
        spell_rot_items(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
        spell_rot_items(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        victim = ch->in_room->people;
        while (victim) {
            temp = victim->next_in_room;
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_rot_items(level, magick_type, ch, victim, 0);
            victim = temp;
        }
        break;

    default:
        gamelog("Screw-up in rot items spell!");
        break;
    }
}                               /* end Cast_Rot_Items */


void
cast_sanctuary(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
               OBJ_DATA * tar_obj)
{                               /* begin Cast_Sanctuary */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_sanctuary(level, magick_type, ch, ch, 0);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_sanctuary(level, magick_type, ch, tar_ch, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_sanctuary(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in sanctuary!");
        break;
    }
}                               /* end Cast_Sanctuary */


void
cast_sand_jambiya(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Sand_Jambiya */
    CHAR_DATA *tmp;
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        if (ch->in_room->sector_type <= SECT_CITY) {
            act("There's not enough sand here to call a weapon.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        tmp = ch->in_room->people;
        while (tmp) {
            if (!IS_IMMORTAL(tmp))
                if (ethereal_ch_compare(ch, tmp))
                    spell_sand_jambiya(level, magick_type, ch, tmp, 0);
            tmp = tmp->next_in_room;
        }
        break;
    case SPELL_TYPE_POTION:
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        if (ch->in_room->sector_type <= SECT_CITY) {
            act("There's not enough sand here to call a weapon.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        spell_sand_jambiya(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Something is wrong with Sand Jambiya!");
        break;
    }
}                               /* end Cast_Sand_Jambiya */


void
cast_sand_shelter(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SPELL:
        spell_sand_shelter(level, magick_type, ch, NULL, NULL);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_ROOM:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in sand shelter!");
        break;
    }
}

void
cast_sandstatue(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                OBJ_DATA * tar_obj)
{                               /* begin Cast_Sandstatue */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_sandstatue(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_sandstatue(level, magick_type, ch, victim, tar_obj);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_sandstatue(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Screw-up in sandstatue!");
        break;
    }
}                               /* end Cast_Sandstatue */

void
cast_sandstorm(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
               OBJ_DATA * tar_obj)
{                               /* begin Cast_Sandstorm */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_sandstorm(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_sandstorm(level, magick_type, ch, victim, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_sandstorm(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Screw-up in sandstorm!");
        break;
    }
}                               /* end Cast_Sandstorm */


void
cast_send_shadow(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{                               /* begin Cast_Send_Shadow */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (tar_ch && (tar_ch != ch)) {
            act("$N seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SPELL:
        spell_send_shadow(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_char("The world dims to shades of grey momentarily.\n\r", ch);
        break;
    default:
        gamelog("Screw-up in send shadow!");
        break;
    }
}                               /* end Cast_Send_Shadow */


void
cast_shadow_sword(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Shadow_Sword */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        spell_shadow_sword(level, magick_type, ch, 0, 0);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_shadow_sword(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Something is wrong with that nifty Shadow Sword spell!");
        break;
    }
}


void
cast_shadowplay(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{                               /* begin Cast_Shadowplay */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_shadowplay(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Screwup with the shadowplay spell.");
        break;
    }
}                               /* end Cast_Shadowplay */


void
cast_shield_of_mist(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                    OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_shield_of_mist(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_shield_of_mist(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_shield_of_mist(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in shield of mist!");
        break;
    }
}                               /* end Spell_Shield_Of_Mist */


void
cast_shield_of_wind(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                    OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_shield_of_wind(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_shield_of_wind(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_shield_of_wind(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in shield of wind!");
        break;
    }
}                               /* end Spell_Shield_Of_Wind */


void
cast_healing_mud(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_healing_mud(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
	    send_to_char ("Use on who?\n\r", ch);
	    return;
	  }
    case SPELL_TYPE_SPELL:
        spell_healing_mud(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_healing_mud(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in healing mud!");
        break;
    }
}                               /* end Spell_Healing_Mud */


void
cast_shield_of_nilaz(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                     OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_shield_of_nilaz(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_shield_of_nilaz(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_shield_of_nilaz(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in shield of nilaz!");
        break;
    }
}                               /* end Spell_Shield_Of_Nilaz */


void
cast_show_the_path(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_Show_The_Path */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_show_the_path(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_show_the_path(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in show the path!");
        break;
    }
}                               /* end Cast_Show_The_Path */


void
cast_silence(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{                               /* begin Cast_Silence */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        if (!IS_AFFECTED(tar_ch, CHAR_AFF_SILENCE))
            spell_silence(level, magick_type, ch, ch, 0);
        else
            send_to_char("Nothing happens.\n\r", ch);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        if (!IS_AFFECTED(tar_ch, CHAR_AFF_SILENCE))
            spell_silence(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("Nothing happens.\n\r", ch);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        if (ch->in_room)
            for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
                if (tar_ch != ch)
                    if (!IS_AFFECTED(tar_ch, CHAR_AFF_SILENCE))
                        if (!IS_IMMORTAL(tar_ch))
                            if (ethereal_ch_compare(ch, tar_ch))
                                spell_silence(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in silence!");
        break;
    }
}                               /* end Cast_Silence */


void
cast_sleep(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Sleep */
    CHAR_DATA *temp;
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_sleep(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_sleep(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        tar_ch = ch->in_room->people;
        while (tar_ch) {
            temp = tar_ch->next_in_room;
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_sleep(level, magick_type, ch, tar_ch, 0);
            tar_ch = temp;
        }
        break;
    default:
        gamelog("Screw-up in sleep!");
        break;
    }
}                               /* end Cast_Sleep */


void
cast_slow(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim, OBJ_DATA * tar_obj)
{                               /* begin Cast_Slow */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        if (!(IS_AFFECTED(victim, CHAR_AFF_SLOW)))
            spell_slow(level, magick_type, ch, ch, 0);
        else
            send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        if (!(IS_AFFECTED(victim, CHAR_AFF_SLOW)))
            spell_slow(level, magick_type, ch, victim, 0);
        else
            send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!(IS_AFFECTED(victim, CHAR_AFF_SLOW)))
                    if (!IS_IMMORTAL(victim))
                        if (ethereal_ch_compare(ch, victim))
                            spell_slow(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Screwup in the slow spell!");
        break;
    }
}                               /* end Cast_Slow */


void
cast_sober(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Sober */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_sober(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_sober(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_sober(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelogf("Bogus spell-type in Sober spell: %d", type);
        break;
    }
}


void
cast_solace(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Solace */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_SCROLL:
        spell_solace(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_POTION:
        send_to_char("The taste of grit and dust fills your mouth.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in solace!");
        break;
    }
}                               /* end Cast_Solace */


void
cast_stalker(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{                               /* begin Cast_Stalker */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_stalker(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        send_to_char("Reaching into the plane of Air, you" " find no stalker there.\n\r", ch);
        break;
    default:
        gamelog("Screw up in stalker spell.");
        break;
    }
}                               /* end Cast_Stalker */


void
cast_stamina_drain(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
                   OBJ_DATA * tar_obj)
{                               /* begin Cast_Stamina_Drain */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_stamina_drain(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_stamina_drain(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_stamina_drain(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Something is wrong with stamina drain.");
        break;
    }
}                               /* end Cast_Stamina_Drain */


void
cast_stone_skin(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{                               /* begin Cast_Stone_Skin */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        if (!affected_by_spell(ch, SPELL_STONE_SKIN))
            spell_stone_skin(level, magick_type, ch, ch, 0);
        else
            send_to_char("The earth's energies elude you.\n\r", ch);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        if (!affected_by_spell(tar_ch, SPELL_STONE_SKIN))
            spell_stone_skin(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("The earth's energies elude you.\n\r", ch);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!affected_by_spell(tar_ch, SPELL_STONE_SKIN))
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_stone_skin(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw up in a stone skin spell.");
        break;
    }
}                               /* end Cast_Stone_Skin */


void
cast_strength(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Strength */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_strength(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_strength(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_strength(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in strength!");
        break;
    }
}                               /* end Cast_Strength */


void
cast_summon(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Summon */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_summon(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_WAND:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in summon!");
        break;
    }
}                               /* end Cast_Summon */


void
cast_teleport(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    if ((tar_obj) && (tar_obj->in_room) && (tar_obj->obj_flags.type == ITEM_TELEPORT)) {
        if ((tar_obj->obj_flags.type == ITEM_TELEPORT) && (tar_obj->in_room)) {
            act("$p was found as a target.", TRUE, ch, tar_obj, 0, TO_CHAR);
        }
    }

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_teleport(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        if (!tar_obj) {
            cprintf(ch, "Winds surge around you, but nothing else happens.\n\r");
            return;
        }

        {
        struct follow_type *follower, *nextfol;
        for (follower = ch->followers; follower; follower = nextfol) {
            nextfol = follower->next;

            CHAR_DATA *tar_ch = follower->follower;
            if (ethereal_ch_compare(ch, tar_ch) && ch->in_room == tar_ch->in_room)
                spell_teleport(level, magick_type, ch, tar_ch, tar_obj);
        }
        }
        // Finally the caster
        spell_teleport(level, magick_type, ch, ch, tar_obj);
        break;
    case SPELL_TYPE_WAND:
      if (tar_ch && (tar_ch != ch)) {
	send_to_char("Nothing seems to happen.\n\r", ch);
	return;
      }
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_teleport(level, magick_type, ch, ch, tar_obj);
        break;
    default:
        gamelogf("Invalid type in cast_teleport switch: %d", type);
        break;
    }
}                               /* end Spell_Teleport */


void
cast_thunder(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{                               /* begin Cast_Thunder */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_thunder(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
    case SPELL_TYPE_WAND:
        send_to_char("A throbbing pain echoes through your head.\n\r", ch);
        break;
    default:
        gamelog("Screwup with thunder spell!");
        break;
    }
}                               /* end Cast_Thunder */


void
cast_tongues(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{                               /* begin Cast_tongues */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        if (!affected_by_spell(tar_ch, SPELL_TONGUES))
            spell_tongues(level, magick_type, ch, ch, 0);
        else
            send_to_char("Nothing seems to happen.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        if (!affected_by_spell(tar_ch, SPELL_TONGUES))
            spell_tongues(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("Nothing seems to happen.\n\r", ch);
        break;

    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!affected_by_spell(tar_ch, SPELL_TONGUES))
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_tongues(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in tounges!");
        break;
    }
}                               /* end Cast_tongues */


void
cast_transference(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Transference */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SPELL:
        spell_transference(level, magick_type, ch, tar_ch, tar_obj);
        break;
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_POTION:
        send_to_char("Onrushing winds threaten to tear you apart.\n\r", ch);
        break;
    default:
        gamelog("Transference is fucked up!");
        break;
    }
}                               /* end Cast_Transference */


void
cast_travel_gate(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                 OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        send_to_char("Nothing seems to happen.\n\r", ch);
        return;
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_SPELL:
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
        spell_travel_gate(level, magick_type, ch, tar_ch, tar_obj);
        break;
    default:
        gamelog("Screw-up in travel_gate!");
        break;
    }
}                               /* end Spell_Travel_Gate */


void
cast_turn_element(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_POTION:
        tar_ch = ch;
    case SPELL_TYPE_SPELL:
        spell_turn_element(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_turn_element(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Screw-up in turn_element!");
        break;
    }
}                               /* end Spell_Turn_Element */


void
cast_vampiric_blade(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                    OBJ_DATA * tar_obj)
{                               /* begin cast_vampiric_blade */
    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *i, *temp_obj;

    switch (type) {
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
        i = ch->in_room->contents;
        while (i) {
            temp_obj = i->next_content;
            if (char_can_see_obj(ch, i))
                spell_vampiric_blade(level, magick_type, ch, 0, i);
            i = temp_obj;
        }

        send_to_room("A cold, eerie chill fills the area momentarily.\n\r", ch->in_room);
        break;

    case SPELL_TYPE_POTION:
        send_to_char("Your hands itch and tingle.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
        if ((!tar_obj) || (tar_ch)) {
            send_to_char("Only objects may be affected by this spell.\n\r", ch);
            return;
        }
    case SPELL_TYPE_SPELL:
        if (char_can_see_obj(ch, tar_obj))
            spell_vampiric_blade(level, magick_type, ch, 0, tar_obj);
        break;
    default:
        gamelog("Screw-up in vampiric blade spell in spells.c!");
        break;
    }
}                               /* end Cast_Vampiric_Blade */


void
cast_planeshift(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Vortex */
    CHAR_DATA *temp;
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_planeshift(level, magick_type, ch, ch, NULL);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        // Everyone in the room but the caster
        tar_ch = ch->in_room->people;
        while (tar_ch) {
            temp = tar_ch->next_in_room;
            if (tar_ch != ch)
                if (!IS_IMMORTAL(tar_ch))
                    if (ethereal_ch_compare(ch, tar_ch))
                        spell_planeshift(level, magick_type, ch, tar_ch, NULL);
            tar_ch = temp;
        }
        // Finally the caster
        spell_planeshift(level, magick_type, ch, ch, tar_obj);
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        if (!tar_ch)
            tar_ch = ch;
        spell_planeshift(level, magick_type, ch, tar_ch, tar_obj);
        break;
    default:
        gamelog("Serious screw-up in planeshift!");
        break;
    }
}                               /* end Cast_Vortex */


/* Added 8/14/2004 -nessalin */
void
cast_fireseed(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Cast_Fireseed */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
      tar_ch = ch;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
      spell_fireseed(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_fireseed(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in fireseed!");
    }                           /* end type-checking with switch (type) */
}                               /* end Cast_Fire_Seed */


/* Added 12/12/2005 -Tenebrius */
/*
>>Acct: Cuusardo
>>Char: Narayan
>>
>>After a conversation with another PC, my Rukkian has
>>been thinking of a way that she could do something
>>like dropping rocks on people, or making stones
>>explode.  So, I had the idea of a Rukkian spell called
>>rockburst, that would basically detonate stone and
>>cause some shrapnel damage.  It would require that
>>there is a coded piece of stone in the room, or on a
>>person and also has a chance of hitting the caster if
>>they don't make a save.
>>
>>Example Scenerio 1: Jane the Rukkian has a head-sized
>>chunk of obsidian that she throws onto the ground in
>>front of Bill the Raider.  (codewise, dropping the
>>obsidian into the room)  She casts a spell tha causes
>>the chunk of obsidian to explode, sending shards of
>>rock flying in all directions.
>>
>>Example Scenerio 2: John the Rukkian is engaged in a
>>fight with Angry Byn Trouper, who happens to be using
>>an axe with a stone head.  John casts a spell, and the
>>axe head explodes, thus destroying the weapon and
>>doing some damage to the wielder.
>>
>>I'm thinking the damage wouldn't be terribly huge in
>>either scenerio, but it would be really neat if damage
>>could vary depending on the size of the stone that was
>>targeted.
>>
>>As far as for the incantation, it would be channeling
>>the energies of Krok in order to cast the spell. 
>>Sphere would either be divan, morz, or mutur, and mood
>>would be echri.
*/
void
cast_shatter(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{                               /* begin Cast_Shatter */
    sh_int magick_type = get_spell_magick_type(type, ch);
    OBJ_DATA *i, *temp_obj;

    switch (type) {
    case SPELL_TYPE_POTION:
      send_to_char("You feel a painful vibration in your bones, that passes almost immediately.\n\r", ch);
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
      if (!tar_obj) {
	send_to_char ("Use on what?\n\r", ch);
	return;
      }
      spell_shatter(level, magick_type, ch, tar_ch, tar_obj);
    case SPELL_TYPE_SPELL:
      spell_shatter(level, magick_type, ch, tar_ch, tar_obj);
      break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        /* Hit all the objects in the room */
        i = ch->in_room->contents;
        while (i) {
            temp_obj = i->next_content;
            spell_shatter(level, magick_type, ch, 0, i);
            i = temp_obj;
        }
        break;
    default:
        gamelog("Serious screw-up in shatter!");
    }                           /* end type-checking with switch (type) */
}                               /* end Cast_Shatter */


/* Added 05/06/2000 -nessalin */
void
cast_wall_of_fire(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Wall_Of_Fire */
    char buf[256];
    int dir;
    int duration;

    struct room_data *other_room;
    struct room_direction_data *back;

    // sh_int magick_type = get_spell_magick_type( type, ch );

    if ((ch->in_room->sector_type == SECT_WATER_PLANE)
        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)
        || (ch->in_room->sector_type == SECT_NILAZ_PLANE)) {
        send_to_char("You cannot reach Suk-Krath to create a wall of fire here.\n\r", ch);
        return;
    }

    switch (type) {             /* begin type-checking with switch (type) */
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SPELL:
        for (; *arg && (*arg == ' '); arg++);

        dir = get_dir_from_name(arg);
        if (DIR_UNKNOWN == dir) {
            act("The spell must be cast in a specific direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (!ch->in_room->direction[dir]) {
            act("There is no exit from here in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_FIRE_WALL)) {
            act("There is already a fire wall in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        duration = WAIT_SEC * (30 + level * 30);
        MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_FIRE_WALL);
        sprintf(buf,
                "Fire spews from $n hands and a giant wall of fire erupts, blocking"
                " the %s exit.", dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        sprintf(buf,
                "Fire spews from your hands and a giant wall of fire erupts, blocking the %s exit.",
                dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        new_event(EVNT_REMOVE_FIREWALL, duration, 0, 0, 0, ch->in_room, dir);

        if ((other_room = EXIT(ch, dir)->to_room))
            if ((back = other_room->direction[rev_dir[dir]]))
                if (back->to_room == ch->in_room) {
                    MUD_SET_BIT(back->exit_info, EX_SPL_FIRE_WALL);
                    sprintf(buf, "A giant wall of fire erupts, blocking " "the %s exit.\n\r",
                            dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                    new_event(EVNT_REMOVE_FIREWALL, duration, 0, 0, 0, EXIT(ch, dir)->to_room,
                              rev_dir[dir]);
                }
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:     /*quote here */
        duration = WAIT_SEC * (30 + level * 30);
        for (dir = DIR_NORTH; dir <= 5; dir++) {
            if (ch->in_room->direction[dir]) {
                if (!IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_FIRE_WALL)) {
                    MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_FIRE_WALL);
                    sprintf(buf,
                            "Fire spews from $n hands and a giant wall of fire erupts, blocking"
                            " the %s exit.", dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_ROOM);
                    sprintf(buf,
                            "Fire spews from your hands and a giant wall of fire erupts, blocking the %s exit.",
                            dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_CHAR);
                    new_event(EVNT_REMOVE_FIREWALL, duration, 0, 0, 0, ch->in_room, dir);
                    if ((other_room = EXIT(ch, dir)->to_room))
                        if ((back = other_room->direction[rev_dir[dir]]))
                            if (back->to_room == ch->in_room) {
                                MUD_SET_BIT(back->exit_info, EX_SPL_FIRE_WALL);
                                sprintf(buf,
                                        "A giant wall of fire erupts up, "
                                        "blocking the %s exit.\n\r", dirs[rev_dir[dir]]);
                                send_to_room(buf, EXIT(ch, dir)->to_room);
                                new_event(EVNT_REMOVE_FIREWALL, duration, 0, 0, 0,
                                          EXIT(ch, dir)->to_room, rev_dir[dir]);
                            }
                }
            }
        }
        break;
    case SPELL_TYPE_POTION:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in wall of fire!");
    }                           /* end type-checking with switch (type) */
}                               /* end Cast_Wall_Of_Fire */


void
cast_wall_of_sand(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Wall_Of_Sand */
    char buf[256];
    int dir;
    int duration;

    struct room_data *other_room;
    struct room_direction_data *back;

    // sh_int magick_type = get_spell_magick_type( type, ch );

    if ((ch->in_room->sector_type == SECT_AIR_PLANE)
        || (ch->in_room->sector_type == SECT_NILAZ_PLANE)) {
        send_to_char("You cannot reach into Ruk from here to create a wall of sand.\n\r", ch);
        return;
    }

    /* gamelogf("Logging for wall of sand: %s", arg); */
    switch (type) {             /* begin type-checking with switch (type) */
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SPELL:
        for (; *arg && (*arg == ' '); arg++);

        dir = get_dir_from_name(arg);
        if (DIR_UNKNOWN == dir) {
            act("The spell must be cast in a specific direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (!ch->in_room->direction[dir]) {
            act("There is no exit from here in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL)) {
            act("There is already a sandwall in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        duration = WAIT_SEC * (30 + level * 30);
        MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL);
        sprintf(buf, "Sand erupts from $n hands and a giant wall ascends, blocking" " the %s exit.",
                dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        sprintf(buf, "Sand erupts from your hands and a giant wall ascends, blocking the %s exit.",
                dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        new_event(EVNT_REMOVE_SANDWALL, duration, 0, 0, 0, ch->in_room, dir);

        if ((other_room = EXIT(ch, dir)->to_room))
            if ((back = other_room->direction[rev_dir[dir]]))
                if (back->to_room == ch->in_room) {
                    MUD_SET_BIT(back->exit_info, EX_SPL_SAND_WALL);
                    sprintf(buf, "A giant wall of sand surges up, blocking " "the %s exit.\n\r",
                            dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                    new_event(EVNT_REMOVE_SANDWALL, duration, 0, 0, 0, EXIT(ch, dir)->to_room,
                              rev_dir[dir]);
                }
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:     /*quote here */
        duration = WAIT_SEC * (30 + level * 30);
        for (dir = DIR_NORTH; dir <= 5; dir++) {
            if (ch->in_room->direction[dir]) {
                if (!IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL)) {
                    MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL);
                    sprintf(buf,
                            "Sand erupts from $n hands and a giant wall ascends, blocking"
                            " the %s exit.", dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_ROOM);
                    sprintf(buf,
                            "Sand erupts from your hands and a giant wall ascends, blocking the %s exit.",
                            dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_CHAR);
                    new_event(EVNT_REMOVE_SANDWALL, duration, 0, 0, 0, ch->in_room, dir);
                    if ((other_room = EXIT(ch, dir)->to_room))
                        if ((back = other_room->direction[rev_dir[dir]]))
                            if (back->to_room == ch->in_room) {
                                MUD_SET_BIT(back->exit_info, EX_SPL_SAND_WALL);
                                sprintf(buf,
                                        "A giant wall of sand surges up, blocking "
                                        "the %s exit.\n\r", dirs[rev_dir[dir]]);
                                send_to_room(buf, EXIT(ch, dir)->to_room);
                                new_event(EVNT_REMOVE_SANDWALL, duration, 0, 0, 0,
                                          EXIT(ch, dir)->to_room, rev_dir[dir]);
                            }
                }
            }
        }
        break;
    case SPELL_TYPE_POTION:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in wall of sand!");
    }                           /* end type-checking with switch (type) */
}                               /* end Cast_Wall_Of_Sand */


/* Added 05/16/2000 -nessalin */
void
cast_wall_of_thorns(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                    OBJ_DATA * tar_obj)
{                               /* begin Cast_Wall_of_Thorns */
    char buf[256];
    int dir;
    int duration;

    struct room_data *other_room;
    struct room_direction_data *back;

    // sh_int magick_type = get_spell_magick_type( type, ch );

    if ((ch->in_room->sector_type == SECT_FIRE_PLANE)
        || (ch->in_room->sector_type == SECT_NILAZ_PLANE)) {
        send_to_char("You cannot reach Vivadu from here to create a wall of thorns.\n\r", ch);
        return;
    }

    switch (type) {             /* begin type-checking with switch (type) */
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SPELL:
        for (; *arg && (*arg == ' '); arg++);
        dir = get_dir_from_name(arg);
        if (DIR_UNKNOWN == dir) {
            act("The spell must be cast in a specific direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (!ch->in_room->direction[dir]) {
            act("There is no exit from here in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_THORN_WALL)) {
            act("There is already a wall of thorns in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        duration = WAIT_SEC * (30 + level * 30);
        MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_THORN_WALL);
        sprintf(buf,
                "$n makes a lifting motion and a giant wall of thorns bursts from the ground, blocking the %s exit.",
                dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        sprintf(buf,
                "You make a lifting motion and a giant wall of thorns bursts from the ground, blocking the %s exit.",
                dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        new_event(EVNT_REMOVE_THORNWALL, duration, 0, 0, 0, ch->in_room, dir);

        if ((other_room = EXIT(ch, dir)->to_room))
            if ((back = other_room->direction[rev_dir[dir]]))
                if (back->to_room == ch->in_room) {
                    MUD_SET_BIT(back->exit_info, EX_SPL_THORN_WALL);
                    sprintf(buf,
                            "A giant wall of thorns burst from the ground, "
                            "blocking the %s exit.\n\r", dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                    new_event(EVNT_REMOVE_THORNWALL, duration, 0, 0, 0, EXIT(ch, dir)->to_room,
                              rev_dir[dir]);
                }
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:     /*quote here */
        duration = WAIT_SEC * (30 + level * 30);
        for (dir = DIR_NORTH; dir <= 5; dir++) {
            if (ch->in_room->direction[dir]) {
                if (!IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_THORN_WALL)) {
                    MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_THORN_WALL);
                    sprintf(buf,
                            "$n makes a lifting motion and a giant wall of thorns bursts from "
                            "the ground, blocking the %s exit.", dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_ROOM);
                    sprintf(buf,
                            "You make a lifting motion and a giant wall of thorns bursts from "
                            "the ground, blocking the %s exit.", dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_CHAR);
                    new_event(EVNT_REMOVE_THORNWALL, duration, 0, 0, 0, ch->in_room, dir);
                    if ((other_room = EXIT(ch, dir)->to_room))
                        if ((back = other_room->direction[rev_dir[dir]]))
                            if (back->to_room == ch->in_room) {
                                MUD_SET_BIT(back->exit_info, EX_SPL_THORN_WALL);
                                sprintf(buf,
                                        "A giant wall of thorns bursts from "
                                        "the ground, blocking the %s exit.\n\r",
                                        dirs[rev_dir[dir]]);
                                send_to_room(buf, EXIT(ch, dir)->to_room);
                                new_event(EVNT_REMOVE_THORNWALL, duration, 0, 0, 0,
                                          EXIT(ch, dir)->to_room, rev_dir[dir]);
                            }
                }
            }
        }
        break;
    case SPELL_TYPE_POTION:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in wall of thorns!");
    }                           /* end type-checking with switch (type) */
}                               /* end Cast_Wall_Of_Thorns */


/* Added 05/16/2000 -nessalin */
void
cast_wall_of_wind(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                  OBJ_DATA * tar_obj)
{                               /* begin Cast_Wall_Of_Wind */
    char buf[256];
    int dir;
    int duration;

    struct room_data *other_room;
    struct room_direction_data *back;

    // sh_int magick_type = get_spell_magick_type( type, ch );

    if ((ch->in_room->sector_type == SECT_EARTH_PLANE)
        || (ch->in_room->sector_type == SECT_NILAZ_PLANE)) {
        send_to_char("You cannot reach Whira to create a wall of wind here.\n\r", ch);
        return;
    }

    switch (type) {             /* begin type-checking with switch (type) */
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_SPELL:
        for (; *arg && (*arg == ' '); arg++);

        dir = get_dir_from_name(arg);
        if (DIR_UNKNOWN == dir) {
            act("The spell must be cast in a specific direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (!ch->in_room->direction[dir]) {
            act("There is no exit from here in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL)) {
            act("There is already a wind wall in that direction.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        duration = WAIT_SEC * (30 + level * 30);
        MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL);
        sprintf(buf,
                "$n exhales hard, and a giant wall of wind gusts into being, blocking the %s exit.",
                dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        sprintf(buf,
                "You exhale hard, and a giant wall of wind gusts into being, blocking the %s exit.",
                dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        new_event(EVNT_REMOVE_WINDWALL, duration, 0, 0, 0, ch->in_room, dir);

        if ((other_room = EXIT(ch, dir)->to_room))
            if ((back = other_room->direction[rev_dir[dir]]))
                if (back->to_room == ch->in_room) {
                    MUD_SET_BIT(back->exit_info, EX_SPL_WIND_WALL);
                    sprintf(buf,
                            "A giant wall of wind gusts into being, blocking " "the %s exit.\n\r",
                            dirs[rev_dir[dir]]);
                    send_to_room(buf, EXIT(ch, dir)->to_room);
                    new_event(EVNT_REMOVE_WINDWALL, duration, 0, 0, 0, EXIT(ch, dir)->to_room,
                              rev_dir[dir]);
                }
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:     /*quote here */
        duration = WAIT_SEC * (30 + level * 30);
        for (dir = DIR_NORTH; dir <= 5; dir++) {
            if (ch->in_room->direction[dir]) {
                if (!IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL)) {
                    MUD_SET_BIT(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL);
                    sprintf(buf,
                            "$n exhales hard, and a giant wall of wind gusts into being, blocking the %s exit.",
                            dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_ROOM);
                    sprintf(buf,
                            "You exhale hard, and a giant wall of wind gusts into being, blocking the %s exit.",
                            dirs[dir]);
                    act(buf, FALSE, ch, 0, 0, TO_CHAR);
                    new_event(EVNT_REMOVE_WINDWALL, duration, 0, 0, 0, ch->in_room, dir);
                    if ((other_room = EXIT(ch, dir)->to_room))
                        if ((back = other_room->direction[rev_dir[dir]]))
                            if (back->to_room == ch->in_room) {
                                MUD_SET_BIT(back->exit_info, EX_SPL_WIND_WALL);
                                sprintf(buf,
                                        "A giant wall of wind gusts into "
                                        "being, blocking the %s exit.\n\r", dirs[rev_dir[dir]]);
                                send_to_room(buf, EXIT(ch, dir)->to_room);
                                new_event(EVNT_REMOVE_WINDWALL, duration, 0, 0, 0,
                                          EXIT(ch, dir)->to_room, rev_dir[dir]);
                            }
                }
            }
        }
        break;
    case SPELL_TYPE_POTION:
        send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    default:
        gamelog("Serious screw-up in wall of wind!");
    }                           /* end type-checking with switch (type) */
}                               /* end Cast_Wall_Of_Wind */


void
cast_weaken(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Weaken */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        if (!affected_by_spell(ch, SPELL_WEAKEN))
            spell_weaken(level, magick_type, ch, ch, 0);
        else
            send_to_char("Nothing seems to happen.\n\r", ch);
        break;

    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        if (!affected_by_spell(tar_ch, SPELL_WEAKEN))
            spell_weaken(level, magick_type, ch, tar_ch, 0);
        else
            send_to_char("Nothing seems to happen.\n\r", ch);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    if (!affected_by_spell(tar_ch, SPELL_WEAKEN)) {
                        if (tar_ch != ch)
                            spell_weaken(level, magick_type, ch, tar_ch, 0);
                        else
                            send_to_char("Nothing seems to happen.\n\r", ch);
                    }
        break;
    default:
        gamelog("Screw up in a weaken spell.");
        break;
    }
}                               /* end Cast_Weaken */



void
cast_wind_armor(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_wind_armor(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!tar_ch)
            tar_ch = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!tar_ch) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_wind_armor(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_wind_armor(level, magick_type, ch, tar_ch, 0);
        break;
    default:
        gamelog("Serious screw-up in wind armor!");
        break;
    }
}                               /* end Spell_Wind_Armor */


void
cast_wind_fist(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * victim,
               OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_wind_fist(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
        if (!victim)
            victim = ch;
    case SPELL_TYPE_WAND:
        if (tar_obj) {
            act("$p seems to be unaffected.", FALSE, ch, tar_obj, 0, TO_CHAR);
            return;
        }
        if (!victim) {
          send_to_char ("Use on who?\n\r", ch);
          return;
        }
    case SPELL_TYPE_SPELL:
        spell_wind_fist(level, magick_type, ch, victim, 0);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
            if (victim != ch)
                if (!IS_IMMORTAL(victim))
                    if (ethereal_ch_compare(ch, victim))
                        spell_wind_fist(level, magick_type, ch, victim, 0);
        break;
    default:
        gamelog("Something is wrong with that spiffy wind fist spell.");
        break;
    }
}

void
cast_wither(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch, OBJ_DATA * tar_obj)
{                               /* begin Cast_Wither */
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_ROOM:
        spell_wither(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_WAND:
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SPELL_TYPE_POTION:
        act("Nothing seems to happen.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_wither(level, magick_type, ch, 0, 0);
        break;
    default:
        gamelog("Something is wrong with that nifty Wither spell!");
        break;
    }
}

void
cast_daylight(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_STAFF:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_SPELL:
        spell_daylight(level, magick_type, ch, 0, 0);
        break;
    case SPELL_TYPE_POTION:
        if (affected_by_spell(ch, SPELL_BLIND)) {
            spell_cure_blindness(level, magick_type, ch, ch, 0);
        } else {
            cprintf(ch, "Nothing seems to happen.\n\r");
        }
        break;
    default:
        gamelog("Serious screw-up in daylight spell!");
        break;
    }
}

void
cast_dispel_invisibility(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                         OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_dispel_invisibility(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        spell_dispel_invisibility(level, magick_type, ch, tar_ch, tar_obj);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        tar_ch = ch->in_room->people;
        while (tar_ch) {
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_dispel_invisibility(level, magick_type, ch, tar_ch, NULL);
            tar_ch = tar_ch->next_in_room;
        }

        tar_obj = ch->in_room->contents;
        while (tar_obj) {
            spell_dispel_invisibility(level, magick_type, ch, NULL, tar_obj);
            tar_obj = tar_obj->next_content;
        }
        break;
    default:
        gamelog("Serious screw-up in dispel invisibility!");
        break;
    }
}

void
cast_dispel_ethereal(byte level, CHAR_DATA * ch, char *arg, int type, CHAR_DATA * tar_ch,
                     OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);

    switch (type) {
    case SPELL_TYPE_POTION:
        spell_dispel_ethereal(level, magick_type, ch, ch, 0);
        break;
    case SPELL_TYPE_SCROLL:
      if (!tar_ch)
	tar_ch = ch;
    case SPELL_TYPE_WAND:
      if (!tar_ch) {
	send_to_char ("Use on who?\n\r", ch);
	return;
      }
    case SPELL_TYPE_SPELL:
        spell_dispel_ethereal(level, magick_type, ch, tar_ch, tar_obj);
        break;
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        tar_ch = ch->in_room->people;
        while (tar_ch) {
            if (!IS_IMMORTAL(tar_ch))
                if (ethereal_ch_compare(ch, tar_ch))
                    spell_dispel_ethereal(level, magick_type, ch, tar_ch, NULL);
            tar_ch = tar_ch->next_in_room;
        }

        tar_obj = ch->in_room->contents;
        while (tar_obj) {
            spell_dispel_ethereal(level, magick_type, ch, NULL, tar_obj);
            tar_obj = tar_obj->next_content;
        }
        break;
    default:
        gamelog("Serious screw-up in dispel ethereal!");
        break;
    }
}

void
cast_recite(byte level, CHAR_DATA * ch, const char *arg, int type, CHAR_DATA * tar_ch,
            OBJ_DATA * tar_obj)
{
    sh_int magick_type = get_spell_magick_type(type, ch);
    
    switch (type) {
    case SPELL_TYPE_SPELL:      
    case SPELL_TYPE_SCROLL:
    case SPELL_TYPE_WAND:
    case SPELL_TYPE_ROOM:
    case SPELL_TYPE_STAFF:
        spell_recite(level, magick_type, ch, tar_ch, 0);
        break;
    case SPELL_TYPE_POTION:
        send_to_char("A shiver runs across your skin.\n\r", ch);
        break;
    default:
        break;
    }
}


