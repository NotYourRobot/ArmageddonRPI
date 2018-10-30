/*
 * File: SKILLS.C
 * Usage: Routines and commands for skill use.
 *
 * Copyright (C) 1993, 1994 by Dan Brumleve, Brian Takle, and the creators
 * of DikuMUD.  May Vivadu bring them good health and many children!
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 * ---------------------------------------------------------------------


 * Revision history.  Please update as changes made.
 * 10/15/02 Fixed bug which subtracted bonuses for HIDE, SNEAK & LISTEN instead
 *          of adding them. -Tiernan
 *          Updated SNEAK to allow SILK & LEATHER as materials which won't
 *          cause sneak to fail. -Tiernan
 * 12/13/01 Added PICKS_BREAK code, based on quality of pick
 *          and picker's skill vs. percent rolled. -Morg
 *          Made pick quality add directly to the % learned for picking. -Morg
 * December 9-present working on new brew.  grep for SANBREW
 *          to find, currently disabled.  -Sanvean
 * 01/16/00 added check so calmed ppl can't subdue  -Sanvean
 * 05/06/00 Added foraging for mushrooms/roots. Also added some
 *          new stones and artifacts. -Sanvean
 * 05/15/00 Made it so you can't forage in really bad weather. -San
 * 05/17/00 When you're subdued, hide affect is removed.  -San
 * 05/18/00 Successful forage now costs 1 mv point.  -San
 * 05/26/00 Fixed some typos in skill_kick. -San
 * 05/17/00 Fixed bad forage item. -San
 * 07/01/00 Added more stone foraging items. -san
 * 12/01/00 Fixed typos in track messages in skill_hunt -Savak
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "delay.h"
#include "db.h"
#include "event.h"
#include "limits.h"
#include "parser.h"
#include "skills.h"
#include "handler.h"
#include "guilds.h"
#include "cities.h"
#include "modify.h"
#include "object.h"
#include "object_list.h"
#include "watch.h"
#include "info.h"

#define AZBREW TRUE

extern int count_opponents();

extern int missile_wall_check(CHAR_DATA * ch, OBJ_DATA * arrow, int dir, int cmd);
extern bool ch_starts_falling_check(CHAR_DATA *ch);

/* Local funcs */
int new_skill_success(CHAR_DATA * ch);


int
guard_check(CHAR_DATA * ch, CHAR_DATA * victim)
{                               /* begin guard_check */
    CHAR_DATA *guard;
    struct guard_type *curr_guard;
    int skill, ct;

    guard = (CHAR_DATA *) 0;

    if (victim->guards) {
        for (curr_guard = victim->guards; curr_guard; curr_guard = curr_guard->next) {
            guard = curr_guard->guard;

            if ((guard != ch) && (GET_POS(guard) >= POSITION_STANDING)
                && (CAN_SEE(guard, ch)) && (guard != victim)
                && (guard->in_room == victim->in_room)) {

                ct = room_in_city(ch->in_room);
                // nosave arrest is on the guard
                if (IS_SET(guard->specials.nosave, NOSAVE_ARREST)
                // and the aggressor is in the city militia
                 && IS_SET(ch->specials.act, city[ct].c_flag)) {
                    // step aside
                    act("$n steps aside for you.", 
                     FALSE, guard, 0, ch, TO_VICT);
                    act("$n steps aside for $N!",
                     FALSE, guard, 0, ch, TO_NOTVICT);
                    act("You step aside for $N!", FALSE, guard, 0, ch, TO_CHAR);
                } else {
                    skill = MAX(15, get_skill_percentage(guard, SKILL_GUARD));

                    /* if the guard is watching someone, but not ch */
                    if (is_char_watching(guard)
                        && !is_char_actively_watching_char(guard, ch)) {
                        /* their own skill in watching serves as a penalty */
                        skill -= get_watched_penalty(guard);
                    }
                    /* if they are watching the aggressor */
                    else if( is_char_watching(guard)
                      && is_char_actively_watching_char(guard, ch)) {
                        /* their own skill helps */
                        skill += get_watched_penalty(guard);
                    }
    
                    if (skill_success(guard, ch, skill)) {
                        /* send messages */
                        act("$n protects you from harm!", 
                         FALSE, guard, 0, victim, TO_VICT);
                        act("$n leaps in front of $N, protecting $M.", 
                         FALSE, guard, 0, victim, TO_ROOM);
                        act("You leap in front of $N, protecting $M.", 
                         FALSE, guard, 0, victim, TO_CHAR);
    
                        /* apply crim flags where needed */
                        check_criminal_flag(ch, guard);
    
                            /* redirect the attackers blow */
                        hit(guard, ch, TYPE_UNDEFINED);
    
                        /* exit the loop since the blow was diverted */
                        return 1;
                    } else {
                        /* send the messages */
                        act("$n tries to protect you but fails!",
                         FALSE, guard, 0, victim, TO_VICT);
                        act("$n tries to protect $N but fails!", 
                         FALSE, guard, 0, victim, TO_NOTVICT);
                        act("You try to protect $N but fail!",
                         FALSE, guard, 0, victim, TO_CHAR);
    
                        if (!(number(0, 3))) {
                            if (GET_GUILD(ch) == GUILD_WARRIOR)
                                gain_skill(ch, SKILL_GUARD, number(2, 3));
                            else
                                gain_skill(ch, SKILL_GUARD, number(1, 2));
                        }
                    }
                }
            }
        }
    }
    return 0;
}                               /* end of guard_check */



/* I'm going to send most skills through this check, which is going to
   modify their ability to succeed if they are intoxicated.  This function
   will return a number, which will be multiplied by their learned % of the
   skill, and that number divided by 100.  This will knock their success
   down by a set number.  */
int
new_skill_success(CHAR_DATA * ch)
{
    int reduction = 0;

    switch (is_char_drunk(ch)) {
    case 0:
        reduction = 100;
        break;
    case DRUNK_LIGHT:
        reduction = 85;
        break;
    case DRUNK_MEDIUM:
        reduction = 70;
        break;
    case DRUNK_HEAVY:
        reduction = 50;
        break;
    case DRUNK_SMASHED:
        reduction = 30;
        break;
    case DRUNK_DEAD:
        reduction = 10;
        break;
    }
    return reduction;
}                               /* end new_skill_success() */

/* return a tool if possible */
OBJ_DATA *
get_tool(CHAR_DATA * ch, int tool_type)
{
  OBJ_DATA *tool;
  
  if ((tool = ch->equipment[EP])) {
    if (tool->obj_flags.type == ITEM_TOOL && tool->obj_flags.value[0] == tool_type) {
      return tool;
    }
  }
  
  if ((tool = ch->equipment[ES])) {
    if (tool->obj_flags.type == ITEM_TOOL && tool->obj_flags.value[0] == tool_type) {
      return tool;
    }
  }

  return (OBJ_DATA *) 0;
}


bool
check_str(CHAR_DATA * ch, int bonus)
{
    int check = number(1, 25);

    return (((GET_STR(ch) + bonus) > check) ? TRUE : FALSE);
}

bool
check_agl(CHAR_DATA * ch, int bonus)
{
    int check = number(1, 25);

    return (((GET_AGL(ch) + bonus) > check) ? TRUE : FALSE);
}

bool
check_wis(CHAR_DATA * ch, int bonus)
{
    int check = number(1, 25);

    return (((GET_WIS(ch) + bonus) > check) ? TRUE : FALSE);
}

bool
check_end(CHAR_DATA * ch, int bonus)
{
    int check = number(1, 25);

    return (((GET_END(ch) + bonus) > check) ? TRUE : FALSE);
}

void
skill_bandage(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
              OBJ_DATA * tar_obj)
{                               /* begin Skill_Bandage */
    OBJ_DATA *tool;
    int heal;

    assert(tar_ch);

    if (!ch->skills[SKILL_BANDAGE]) {
        send_to_char("You have no idea how to bandage anyone.\n\r", ch);
        /* no wait state */
        ch->specials.act_wait = 0;
        return;
    }

    if (!(tool = get_tool(ch, TOOL_BANDAGE))) {
        send_to_char("You aren't holding a bandage.\n\r", ch);
        /* no wait state */
        ch->specials.act_wait = 0;
        return;
    }

    if ((tool->obj_flags.value[1] >= 8)
        && affected_by_spell(tar_ch, SPELL_POISON)
        && (ch->skills[SKILL_BANDAGE]->learned >= 50)
        && (skill_success(ch, 0, ch->skills[SKILL_BANDAGE]->learned))) {
        affect_from_char(tar_ch, SPELL_POISON);
        act("You skillfully try to draw out $S poison, and they begin to look better.", FALSE, ch,
            0, tar_ch, TO_CHAR);
        act("$n skillfully tries to draw out your poison, and your head clears a bit.", FALSE, ch,
            0, tar_ch, TO_VICT);
        act("$n skillfully tries to draw out $N's poison, and they begin to look better.", FALSE,
            ch, 0, tar_ch, TO_NOTVICT);
        extract_obj(tool);
        return;
    }
    if ((tool->obj_flags.value[1] >= 8)
        && affected_by_spell(tar_ch, POISON_GENERIC)
        && (ch->skills[SKILL_BANDAGE]->learned >= 50)
        && (skill_success(ch, 0, ch->skills[SKILL_BANDAGE]->learned))) {
        affect_from_char(tar_ch, POISON_GENERIC);
        act("You skillfully try to draw out $S poison, and they begin to look better.", FALSE, ch,
            0, tar_ch, TO_CHAR);
        act("$n skillfully tries to draw out your poison, and your head clears a bit.", FALSE, ch,
            0, tar_ch, TO_VICT);
        act("$n skillfully tries to draw out $N's poison, and they begin to look better.", FALSE,
            ch, 0, tar_ch, TO_NOTVICT);
        extract_obj(tool);
        return;
    }

    if (GET_HIT(tar_ch) > ((GET_MAX_HIT(tar_ch) / 2) + 10)) {
        if (tar_ch == ch)
            send_to_char("You don't need any bandaging.\n\r", ch);
        else
            act("$N doesn't need to be bandaged.", FALSE, ch, 0, tar_ch, TO_CHAR);
        return;
    }

    if ((GET_HIT(tar_ch) < ((GET_MAX_HIT(tar_ch) / 2) - ch->skills[SKILL_BANDAGE]->learned))
        || (GET_HIT(tar_ch) < -6)) {
        if (tar_ch == ch)
            act("You don't have enough skill to bandage these wounds.", FALSE, ch, 0, tar_ch,
                TO_CHAR);
        else
            act("You don't have enough skill to bandage $N's wounds.", FALSE, ch, 0, tar_ch,
                TO_CHAR);
        return;
    }

    if (skill_success(ch, 0, ch->skills[SKILL_BANDAGE]->learned)) {
        if (tar_ch == ch) {
            act("You manage to bind your wounds.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n wraps $s wounds with bandages.", TRUE, ch, 0, 0, TO_ROOM);
        } else {
            act("You skillfully bandage $S wounds.", FALSE, ch, 0, tar_ch, TO_CHAR);
            act("$n skillfully bandages $N's wounds.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
            act("Your head clears a little as $n bandages your wounds.", FALSE, ch, 0, tar_ch, TO_VICT);
        }

        if (tool->obj_flags.value[1])
            heal = number(tool->obj_flags.value[1], ch->skills[SKILL_BANDAGE]->learned / 3);
        else
            heal = number(-5, 5);
        heal = MAX(1, heal);
        adjust_hit(tar_ch, heal);
        update_pos(tar_ch);
    } else {
        if (tar_ch == ch) {
            act("$n tries to bandage $mself.", TRUE, ch, 0, 0, TO_ROOM);
            act("You try to bandage yourself but fail.", FALSE, ch, 0, 0, TO_CHAR);
        } else {
            act("You try to bandage $S wounds, but fail.", FALSE, ch, 0, tar_ch, TO_CHAR);
            act("$n tries to bandage your wounds but fails.", FALSE, ch, 0, tar_ch, TO_VICT);
            act("$n tries to bandage $N's wounds, but fails.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        }
        act("$n groans loudly as $s wounds tear.", FALSE, tar_ch, 0, 0, TO_ROOM);

        adjust_hit(tar_ch, -number(1,12));
        update_pos(tar_ch);
        if (GET_POS(tar_ch) == POSITION_DEAD)
            die(tar_ch);

        gain_skill(ch, SKILL_BANDAGE, 4);
    }
    extract_obj(tool);
}

void
skill_parry(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
            OBJ_DATA * tar_obj)
{

}

int
backstab_check(CHAR_DATA * ch, CHAR_DATA * tar_ch)
{
    int chance;

    chance = (ch->skills[SKILL_BACKSTAB] ? ch->skills[SKILL_BACKSTAB]->learned : 1);
    chance += ((GET_AGL(ch) - GET_AGL(tar_ch)) * 3);

    if (IS_AFFECTED(tar_ch, CHAR_AFF_SUBDUED))
        chance += 75;

    if (is_paralyzed(tar_ch))
        chance += 75;

    if ((GET_POS(tar_ch) > POSITION_SLEEPING)
        && (GET_POS(tar_ch) < POSITION_FIGHTING))
        chance += (15 * (POSITION_STANDING - GET_POS(tar_ch)));

    if (GET_POS(tar_ch) == POSITION_FIGHTING)
        chance = chance / 5;

    if (affected_by_spell(ch, PSI_MINDWIPE))
        chance -= 25;

    // if the target cant' see them
    if (!CAN_SEE(tar_ch, ch)) {
        // If the target is actively listening
        if (IS_AFFECTED(tar_ch, CHAR_AFF_LISTEN)
          && !tar_ch->specials.fighting
          && AWAKE(tar_ch)
          && !IS_AFFECTED(tar_ch, CHAR_AFF_DEAFNESS)
          && has_skill(tar_ch, SKILL_LISTEN)) {
            // opposed skill check
            if (number(1, get_skill_percentage(tar_ch, SKILL_LISTEN))
                > number(1, chance)) {
                chance -= 25;
            } else {
                // listener failed 
                gain_skill(tar_ch, SKILL_LISTEN, 1);
                chance += 15;
            }
        }
        else {
            chance += 25;
        }
    }
    /* skill penalty if target is watching ch */
    else if (is_char_actively_watching_char(tar_ch, ch)) {
        chance -= get_watched_penalty(tar_ch);
    }
    // can see but not watching, give listen a chance
    else if (IS_AFFECTED(tar_ch, CHAR_AFF_LISTEN)
      && !tar_ch->specials.fighting
      && AWAKE(tar_ch)
      && !IS_AFFECTED(tar_ch, CHAR_AFF_DEAFNESS)
      && has_skill(tar_ch, SKILL_LISTEN)) {
        if (number(1, get_skill_percentage(tar_ch, SKILL_LISTEN))
            > number(1, chance)) {
            chance -= 25;
        } else {
            gain_skill(tar_ch, SKILL_LISTEN, 1);
        }
    }

    chance = MAX(chance, 1);
    chance = MIN(chance, 100);

    if (GET_POS(tar_ch) <= POSITION_SLEEPING)   /* never fail */
        chance = 101;

    return chance;
}

int
backstab_damage(CHAR_DATA * ch, CHAR_DATA * tar_ch, OBJ_DATA * wielded, int wpos)
{
    int dam;

    dam =
        dice(wielded->obj_flags.value[1],
             wielded->obj_flags.value[2]) + wielded->obj_flags.value[0];

    dam = MAX(1, dam);

    if (ch->skills[SKILL_BACKSTAB])
        dam *= (2 + (ch->skills[SKILL_BACKSTAB]->learned / 7));
    else
        dam *= 2;

    dam += str_app[GET_STR(ch)].todam;

    switch (wpos) {
    case ES:
        dam = MAX(1, (dam / 2));
        break;
    case ETWO:
        dam += number(3, MAX(str_app[GET_STR(ch)].todam, 4));
        break;
    case EP:
    default:
        break;
    }

/* damage = maxhps of target(backstab% -10) +/- 1 per size pt difference */

    dam += race[(int) GET_RACE(ch)].attack->damplus;
/* dam = MIN (dam, 100); */

    return dam;
}

void
skill_backstab(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
               OBJ_DATA * tar_obj)
{
    int dam, chance = 0, wpos = 0;
    OBJ_DATA *wielded = NULL;
    OBJ_DATA *tmp1, *tmp2, *tmp3;
    char buf[MAX_STRING_LENGTH];

    /* Too tired to do it. */
    /* Morgenes 4/19/2006 */
    if (GET_MOVE(ch) < 10) {
        cprintf(ch, "You are too tired!\n\r");
        return;
    }

    if (IS_CALMED(ch)) {
        send_to_char("You feel too at ease right now to attack anyone.\n\r", ch);
        return;
    }

    if (ch->specials.riding) {
        send_to_char("You can't possibly do this while mounted.\n\r", ch);
        return;
    }

    if (ch->specials.fighting) {
        clear_disengage_request(ch);
        send_to_char("You are fighting for your life!\n\r", ch);
        return;
    }

    wielded = get_weapon(ch, &wpos);
    if (!wielded) {
        send_to_char("You need to wield a weapon to make it a success.\n\r", ch);
        return;
    }

    if (wielded->obj_flags.value[3] != (TYPE_STAB - 300)) {
        send_to_char("Only stabbing weapons can be used for " "backstabbing.\n\r", ch);
        return;
    }

    if (is_char_ethereal(ch) != is_char_ethereal(tar_ch)) {
        act("Your strike passes right through $M!", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n strikes at you, but it passes harmlessly through you.", FALSE, ch, 0, tar_ch,
            TO_VICT);
        act("$n strikes at $N, but ends up swinging at air.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        return;
    }

    if (ch_starts_falling_check(ch)) {
      return;
    }


    /*  Can't BS someone who is flying unless you are flying as well or 
     * they can't see u  -  Kel  */
    if ((IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
         && !affected_by_spell(tar_ch, SPELL_FEATHER_FALL))
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
        && CAN_SEE(tar_ch, ch)) {
        act("You try, but $E rises overhead and evades you.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to sneak up behind you, but you easily rise overhead.", FALSE, ch, 0, tar_ch,
            TO_VICT);
        return;
    }

    if (affected_by_spell(tar_ch, SPELL_INVULNERABILITY)) {
        act("In a flare of energy, your blow bounces off $N, the light surrounding $M dying away.",
            FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to backstab you, but your shield deflects $m before dying away.", FALSE, ch,
            0, tar_ch, TO_VICT);
        act("$n stabs $N from behind, but is deflected in a flare of light.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
    }

    /* have to fake out ws_damage() with the weapon */
    tmp1 = ch->equipment[EP];
    tmp2 = ch->equipment[ES];
    tmp3 = ch->equipment[ETWO];
    if (tmp1)
        obj_to_char(unequip_char(ch, EP), ch);
    if (tmp2)
        obj_to_char(unequip_char(ch, ES), ch);
    if (tmp3)
        obj_to_char(unequip_char(ch, ETWO), ch);

    /* one of them = wielded */
    obj_from_char(wielded);
    equip_char(ch, wielded, EP);

/*
   if ( !skill_success(ch, tar_ch, ch->skills[SKILL_BACKSTAB] ?
   ch->skills[SKILL_BACKSTAB]->learned : 0) )
   {
   ws_damage(ch, tar_ch, 0, 0, 0, 0, SKILL_BACKSTAB, 0);
   gain_skill(ch, SKILL_BACKSTAB, 1);
   } 
 */

    /* Stamina drain. */
    adjust_move(ch, -number(2, (100 - get_skill_percentage(ch, SKILL_BACKSTAB)) / 22 + 3));

    chance = backstab_check(ch, tar_ch);

    /* send the message to the monitors */
    sprintf(buf, "%s backstabs %s.\n\r", MSTR(ch, name), MSTR(tar_ch, name));
    send_to_monitors(ch, tar_ch, buf, MONITOR_FIGHT);
    shhlogf("%s: %s", __FUNCTION__, buf);

    if (number(1, 101) > chance) {
        ws_damage(ch, tar_ch, 0, 0, 0, 0, SKILL_BACKSTAB, 0);
        // can only gain on non-paralyzed, awake, humanoid creatures
        if (!is_paralyzed(tar_ch) && AWAKE(tar_ch)
            && get_char_size(tar_ch) > 4 && GET_RACE(tar_ch) != RACE_VESTRIC
            && GET_RACE(tar_ch) != RACE_QUIRRI
            && GET_RACE(tar_ch) != RACE_RODENT)
            gain_skill(ch, SKILL_BACKSTAB, number(1, 3));
    } else {
        int prior_hitpoints = GET_HIT(ch);
        int poison_type = -1;

        if (guard_check(ch, tar_ch))
            return;

         if (wielded->obj_flags.value[5])
            poison_type = wielded->obj_flags.value[5];
        else if (IS_SET(wielded->obj_flags.extra_flags, OFL_POISON))
            poison_type = POISON_GENERIC;

        dam = backstab_damage(ch, tar_ch, wielded, wpos);

        /* Debug message -Tiernan 10/23/02 
         * sprintf(buf,"%s backstabbed %s for %d damage",
         * MSTR( ch, name ), MSTR( tar_ch, name ), dam);
         * gamelog( buf );
         * end debug */
        ws_damage(ch, tar_ch, dam, 0, 0, dam, SKILL_BACKSTAB, 0);

        if( prior_hitpoints > GET_HIT(ch) ) {
            if (poison_type != -1
             && !does_save(tar_ch, SAVING_PARA, 0)) {
                poison_char(tar_ch, poison_type, dice(2, 4), 0);
                send_to_char("You feel very sick.\n\r", tar_ch);
            }
        }
    }

    /* now to put it all back */
    obj_to_char(unequip_char(ch, EP), ch);
    if (tmp1) {
        obj_from_char(tmp1);
        equip_char(ch, tmp1, EP);
    }
    if (tmp2) {
        obj_from_char(tmp2);
        equip_char(ch, tmp2, ES);
    }
    if (tmp3) {
        obj_from_char(tmp3);
        equip_char(ch, tmp3, ETWO);
    }
    check_criminal_flag(ch, tar_ch);
    WAIT_STATE(ch, number(2, 3) * PULSE_VIOLENCE);
}

void
skill_scan(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{
}

void
cmd_scan(CHAR_DATA *ch, const char *arg, Command cmd, int count) {
    struct affected_type af;
    bool is_on = 0, turn_on = 0;

    memset(&af, 0, sizeof(af));

    for (; arg != NULL && *arg == ' ' && *arg != '\0'; arg++);

    if (IS_AFFECTED(ch, CHAR_AFF_SCAN)) {
        is_on = 1;
    }

    if (arg != NULL && *arg != '\0') {
        if (!str_prefix(arg, "status")) {
            if (!is_on)
                send_to_char("You aren't currently scanning.\n\r", ch);
            else
                send_to_char("You are currently scanning.\n\r", ch);
            return;
        }

        if (!str_prefix(arg, "on")) {
            if (is_on) {
                send_to_char("You are already scanning.\n\r", ch);
                return;
            }
            turn_on = 1;
        }
        else if (!str_prefix(arg, "off")) {
            if (!is_on) {
                send_to_char("You weren't scanning.\n\r", ch);
                return;
            }
            turn_on = 0;
        } else {
            send_to_char("Syntax:  scan [on|off|status]\n\r", ch);
            return;
        }
    }

    if (turn_on == 0 && is_on) {         /* means turn off instead */
        affect_from_char(ch, SKILL_SCAN);
        send_to_char("You stop scanning.\n\r", ch);
        return;
    }

    if (is_char_watching(ch)) {
        stop_watching(ch, FALSE);
    }

    af.type = SKILL_SCAN;
    if (ch->skills[SKILL_SCAN])
        af.duration = (ch->skills[SKILL_SCAN]->learned / 10) + 1;
    else
        af.duration = 1;
    af.location = CHAR_APPLY_STUN;
    af.modifier = -10 + round(7.0 * get_skill_percentage(ch, SKILL_SCAN) / 100.0);
    af.bitvector = CHAR_AFF_SCAN;
    affect_to_char(ch, &af);

    act("You intently scan the area.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n intently scans the area.", TRUE, ch, 0, 0, TO_ROOM);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

void
skill_search(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    int door, found_one = 0;
    char buf[256];
    char *message;
    OBJ_DATA * artifact_cache = NULL;

    message = find_ex_description("[SEARCH_MESSAGE]", ch->in_room->ex_description, TRUE);
    if (message) {
        send_to_char(message, ch);
    }

    // Check for secret / hidden exits
    for (door = 0; door < 6; door++) {
        if (EXIT(ch, door) && IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)
            && IS_SET(EXIT(ch, door)->exit_info, EX_SECRET)) {
            if (skill_success
                (ch, 0,
                 ch->skills[SKILL_SEARCH] ? ch->skills[SKILL_SEARCH]->learned : number(1, 5))) {
                sprintf(buf, "You discovered %s%s %s.\n\r", indefinite_article(EXIT(ch, door)->keyword),
                        EXIT(ch, door)->keyword, dir_name[door]);
                send_to_char(buf, ch);
                found_one = 1;
            } else {
                gain_skill(ch, SKILL_SEARCH, 1);
            }
        }
    }

    // Check for an artifact cache to forage from
    artifact_cache = find_artifact_cache(ch->in_room);
    if (artifact_cache) {
        if (skill_success (ch, 0,
		ch->skills[SKILL_SEARCH] ? ch->skills[SKILL_SEARCH]->learned : number(1, 5))) {
	    send_to_char("You identify that something has been buried here.\n\r", ch);
	    qgamelogf(QUIET_DEBUG, "skill_search: %s discovered an artifact cache in room %d.\n\r", MSTR(ch, name), ch->in_room->number);
	    found_one = 1;
	} else {
	    gain_skill(ch, SKILL_SEARCH, 1);
	}
    }

    if (!found_one)
        send_to_char("You didn't find anything unusual.\n\r", ch);
}

void
repair_error(CHAR_DATA * ch)
{
    send_to_char("Usage: repair <damaged item> <material item>\n\r", ch);
}

bool
wagon_tool_ok(OBJ_DATA * tool)
{
    if (tool->obj_flags.type != ITEM_TOOL)
        return FALSE;

    if (tool->obj_flags.value[0] == TOOL_CHISEL)
        return TRUE;
    if (tool->obj_flags.value[0] == TOOL_HAMMER)
        return TRUE;

    return FALSE;
}

void
wagon_repair_parsed(CHAR_DATA * ch, OBJ_DATA * wagon, OBJ_DATA * tool, const char *arg)
{
    int dam, maxdam, lag;

    if (!has_skill(ch, SKILL_WAGON_REPAIR)) {
        send_to_char("You do not know how to repair that.\n\r", ch);
        return;
    }

    gamelog("Wagon repairing");

    dam = wagon->obj_flags.value[4];
    maxdam = calc_wagon_max_damage(wagon);

    if (!dam) {
        act("Repair it?  But $p is in perfect condition!", FALSE, ch, wagon, 0, TO_CHAR);
        return;
    }

    if (dam <= (maxdam / 5)) {
        act("$p is repaired to the extent of your ability.", FALSE, ch, wagon, 0, TO_CHAR);
        return;
    }

    if (((maxdam * 70) / 100) < dam) {
        act("$p is damaged beyond your ability.", FALSE, ch, wagon, 0, TO_CHAR);
        return;
    }

    lag = 40;
    lag +=
        (has_skill(ch, SKILL_WAGON_REPAIR) ? get_skill_percentage(ch, SKILL_WAGON_REPAIR) :
         MAX_DELAY);
    lag = MIN(lag, MAX_DELAY);
    lag = MAX(40, lag);

    lag = 1;                    /* Remove after testing */

    if (!ch->specials.to_process) {
        act("You begin repairing $p.", FALSE, ch, wagon, 0, TO_CHAR);
        act("$n begins repairing $p.", FALSE, ch, wagon, 0, TO_ROOM);
        add_to_command_q(ch, arg, CMD_REPAIR, lag, 0);
        return;
    }
}


void
wagon_repair_function(CHAR_DATA * ch, OBJ_DATA * wagon, OBJ_DATA * tool, const char *arg)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *tool1, *tool2;
    CHAR_DATA *person, *next_guy;
    int dam;
    int maxdam;
    int success = 0;
    int bonus = 0;
    int lag = 0;
    int counter = 0;

    extern bool is_in_command_q_with_cmd(CHAR_DATA * ch, int cmd);
/*
  gamelog("Broken");
  return;
*/
    if (!has_skill(ch, SKILL_WAGON_REPAIR)) {
        send_to_char("You do not know how to repair that.\n\r", ch);
        return;
    }

    gamelog("Wagon repairing");

    dam = wagon->obj_flags.value[4];
    maxdam = calc_wagon_max_damage(wagon);

    if (!dam) {
        act("Repair it?  But $p is in perfect condition!", FALSE, ch, wagon, 0, TO_CHAR);
        return;
    }

    if (dam <= (maxdam / 5)) {
        act("$p is repaired to the extent of your ability.", FALSE, ch, wagon, 0, TO_CHAR);
        return;
    }

    if (((maxdam * 70) / 100) < dam) {
        act("$p is damaged beyond your ability.", FALSE, ch, wagon, 0, TO_CHAR);
        return;
    }

    lag = 40;
    lag +=
        (has_skill(ch, SKILL_WAGON_REPAIR) ? get_skill_percentage(ch, SKILL_WAGON_REPAIR) :
         MAX_DELAY);
    lag = MIN(lag, MAX_DELAY);
    lag = MAX(40, lag);

    lag = 1;                    /* Remove after testing */

    if (!ch->specials.to_process) {
        act("You begin repairing $p.", FALSE, ch, wagon, 0, TO_CHAR);
        act("$n begins repairing $p.", FALSE, ch, wagon, 0, TO_ROOM);
        add_to_command_q(ch, arg, CMD_REPAIR, lag, 0);
        return;
    }

/* base percentage for repairing */
    if (has_skill(ch, SKILL_WAGON_REPAIR))
        success = get_skill_percentage(ch, SKILL_WAGON_REPAIR);
    else
        success = 0;

#ifdef WAGON_REPAIR_LOG
    sprintf(buf, "After percentage, success is %d", success);
    gamelog(buf);
#endif

/* If they have a proper tool equipped, add a bonus */
    if ((tool1 = ch->equipment[EP]) && wagon_tool_ok(tool1)) {
        success += 10;
    } else
        tool1 = (struct obj_data *) 0;
    if ((tool2 = ch->equipment[ES]) && wagon_tool_ok(tool2)) {
        if (tool1 && wagon_tool_ok(tool1)) {
            if ((tool1->obj_flags.value[0] != tool2->obj_flags.value[0]))
                success += 5;
        } else
            success += 10;
    } else
        tool2 = (struct obj_data *) 0;

#ifdef WAGON_REPAIR_LOG
    sprintf(buf, "After tool check, success is %d", success);
    gamelog(buf);
#endif

    if (IS_IMMORTAL(ch)) {
        if (tool1 && wagon_tool_ok(tool1))
            act("Tool1 found: $p", FALSE, ch, tool1, 0, TO_CHAR);
        if (tool2 && wagon_tool_ok(tool2))
            act("Tool2 found: $p", FALSE, ch, tool2, 0, TO_CHAR);
    }

/* Bonus for groups */
    for (counter = 0, person = ch->in_room->people; person; person = next_guy) {
        next_guy = person->next;
/* problem with htis is that we find a person in the room who is repairing */
/* a wagon, but we can't tell if it is the same wagon.  A solution could */
/* be to set the person to following the wagon as he repairs it. */
        if ((person != ch) && is_in_command_q_with_cmd(person, CMD_REPAIR))
            counter++;
    }
    bonus += (counter * 5);

#ifdef WAGON_REPAIR_LOG
    sprintf(buf, "Counter is %d, Bonus %d, and Success %d", counter, bonus, success);
    gamelog(buf);
    sprintf(buf, "After people check, success is %d", (bonus + success));
    gamelog(buf);
#endif

/* Throw in some random emotes so people know they are still repairing, and */
/* to add some local flavor.  Also keeps things interesting */
    switch (number(0, 100)) {
    case 0:
        if (tool1) {
            sprintf(buf, "$n %s $p with %s.",
                    (tool1->obj_flags.value[0] ==
                     TOOL_HAMMER) ? "pounds on" : "chisels away at irregularities on", OSTR(tool1,
                                                                                            short_descr));
            act(buf, FALSE, ch, wagon, 0, TO_ROOM);

            sprintf(buf, "You %s $p with %s.",
                    (tool1->obj_flags.value[0] ==
                     TOOL_HAMMER) ? "pound on" : "chisel away at irregularities on", OSTR(tool1,
                                                                                          short_descr));
            act(buf, FALSE, ch, wagon, 0, TO_CHAR);

            if (tool1->obj_flags.value[0] == TOOL_HAMMER)
                act("You hear some pounding to the $D.", FALSE, ch, wagon, 0, TO_NEARBY_ROOMS);
        } else {
            if (tool2) {
                sprintf(buf, "$n %s $p with %s.",
                        (tool2->obj_flags.value[0] ==
                         TOOL_HAMMER) ? "pounds on" : "chisels away at irregularities on",
                        OSTR(tool2, short_descr));
                act(buf, FALSE, ch, wagon, 0, TO_ROOM);

                sprintf(buf, "You %s $p with %s.",
                        (tool2->obj_flags.value[0] ==
                         TOOL_HAMMER) ? "pound on" : "chisel away at irregularities on", OSTR(tool2,
                                                                                              short_descr));
                act(buf, FALSE, ch, wagon, 0, TO_CHAR);
                if (tool2->obj_flags.value[0] == TOOL_HAMMER)
                    act("You hear some pounding to the $D.", FALSE, ch, wagon, 0, TO_NEARBY_ROOMS);
            }
        }
        break;
    case 1:                    /* Flip it in case they have 2 tools, one in each hand */
        if (tool2) {
            sprintf(buf, "$n %s $p with %s.",
                    (tool2->obj_flags.value[0] ==
                     TOOL_HAMMER) ? "pounds on" : "chisels away at irregularities on", OSTR(tool2,
                                                                                            short_descr));
            act(buf, FALSE, ch, wagon, 0, TO_ROOM);

            sprintf(buf, "You %s $p with %s.",
                    (tool2->obj_flags.value[0] ==
                     TOOL_HAMMER) ? "pound on" : "chisel away at irregularities on", OSTR(tool2,
                                                                                          short_descr));
            act(buf, FALSE, ch, wagon, 0, TO_CHAR);

            if (tool2->obj_flags.value[0] == TOOL_HAMMER)
                act("You hear some pounding to the $D.", FALSE, ch, wagon, 0, TO_NEARBY_ROOMS);
        } else {
            if (tool1) {
                sprintf(buf, "$n %s $p with %s.",
                        (tool1->obj_flags.value[0] ==
                         TOOL_HAMMER) ? "pounds on" : "chisels away at irregularities on",
                        OSTR(tool1, short_descr));
                act(buf, FALSE, ch, wagon, 0, TO_ROOM);

                sprintf(buf, "You %s $p with %s.",
                        (tool1->obj_flags.value[0] ==
                         TOOL_HAMMER) ? "pound on" : "chisel away at irregularities on", OSTR(tool1,
                                                                                              short_descr));
                act(buf, FALSE, ch, wagon, 0, TO_CHAR);

                if (tool1->obj_flags.value[0] == TOOL_HAMMER)
                    act("You hear some pounding to the $D.", FALSE, ch, wagon, 0, TO_NEARBY_ROOMS);
            }
        }
        break;
    case 2:
        act("$p creaks as you work on it.", FALSE, ch, wagon, 0, TO_CHAR);
        act("$p creaks as $n repairs it.", FALSE, ch, wagon, 0, TO_ROOM);
        act("You hear creaking sounds to the $D.", FALSE, ch, wagon, 0, TO_NEARBY_ROOMS);
        break;
    case 3:
        if (tool1 && (success < 35)) {
            if (tool1->obj_flags.value[0] == TOOL_HAMMER) {
                sprintf(buf, "As $n repairs $p, $e strikes $s thumb with %s!",
                        OSTR(tool1, short_descr));
                act(buf, FALSE, ch, wagon, 0, TO_ROOM);
                act("$p slips, striking your thumb!  OUCH!", FALSE, ch, tool1, 0, TO_CHAR);
            } else {
                sprintf(buf, "As $n repairs $p, %s slips in $s grip, cutting " "him!",
                        OSTR(tool1, short_descr));
                act(buf, FALSE, ch, wagon, 0, TO_ROOM);
                act("$p slips in your grip, slicing your hand!  OUCH!", FALSE, ch, tool1, 0,
                    TO_CHAR);
            }
            generic_damage(ch, 1, 0, 0, 1);
            cmd_shout(ch, "Ow!", 0, 0);
            return;
        }
    case 4:
        if (tool2 && (success < 35)) {
            if (tool2->obj_flags.value[0] == TOOL_HAMMER) {
                sprintf(buf, "As $n repairs $p, $e strikes $s thumb with %s!",
                        OSTR(tool2, short_descr));
                act(buf, FALSE, ch, wagon, 0, TO_ROOM);
                act("$p slips, striking your thumb!  OUCH!", FALSE, ch, tool2, 0, TO_CHAR);
            } else {
                sprintf(buf, "As $n repairs $p, %s slips in $s grip, cutting " "him!",
                        OSTR(tool2, short_descr));
                act(buf, FALSE, ch, wagon, 0, TO_ROOM);
                act("$p slips in your grip, slicing your hand!  OUCH!", FALSE, ch, tool2, 0,
                    TO_CHAR);
            }
            generic_damage(ch, 1, 0, 0, 1);
            cmd_shout(ch, "Ow!", 0, 0);
            return;
        }
    default:
        break;
    }

#ifdef AZROEN_REQUEUE_CODE
    if (((success + bonus) * 2) < number(1, 500)) {
        gamelog("Failed success time check.  Re-entering queue.");
        ch->specials.to_process = FALSE;
        rem_from_command_q(ch);
        add_to_command_q(ch, arg, CMD_REPAIR, lag);
        return;
    }
#endif

#ifdef TENEBRIUS_REQUEUE_CODE
    if (((success + bonus) * 2) < number(1, 500)) {
        gamelog("Failed success time check.  Re-entering queue.");
        add_to_command_q(ch, arg, CMD_REPAIR, lag);
        return;
    }
#endif


    if (success >= number(0, 101)) {
        gamelog("Successful repairing!  Remove 1 point of damage from wagon");
        wagon->obj_flags.value[4] = MAX(0, (wagon->obj_flags.value[4] - 1));
    } else {
        gamelog("Not successful.. but maybe gain some skill knowledge!");
        gain_skill(ch, SKILL_WAGON_REPAIR, 1);
    }

#ifdef AZROEN_REQUEUE_CODE
    ch->specials.to_process = FALSE;
    rem_from_command_q(ch);
    add_to_command_q(ch, arg, CMD_REPAIR, lag);
#endif

#ifdef TENEBRIUS_REQUEUE_CODE
    add_to_command_q(ch, arg, CMD_REPAIR, lag);
#endif


    return;
}

/* Started 5/18/2000 -Nessalin */
void
cmd_repair(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
  OBJ_DATA *broke_item, *repair_material;
  char arg1[256], arg2[256], buf[256];
  char oldarg[MAX_INPUT_LENGTH];
  int max_repair = 0;
  int sk_level = 0;
  int modifiers = 0;
  bool broke_in_inventory = FALSE;
  bool broke_in_room = FALSE;
  bool material_in_inventory = FALSE;
  
  strcpy(oldarg, argument);
  
  argument = one_argument(argument, arg1, sizeof(arg1));
  argument = one_argument(argument, arg2, sizeof(arg2));
  
  if ((!strlen(arg1)) || (!strlen(arg2))) {
    repair_error(ch);
    return;
  } else {
    if ((broke_item = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      broke_in_inventory = TRUE;
    } else {
      if ((broke_item = get_obj_room_vis(ch, arg1))) {
        broke_in_room = TRUE;
      }
    }
    if ((repair_material = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
      material_in_inventory = TRUE;
    }
  }

  if (broke_in_room && (broke_item->obj_flags.type == ITEM_WAGON)) {
    wagon_repair_function(ch, broke_item, repair_material, oldarg);
    return;
  }


  if (!(broke_in_inventory && material_in_inventory)) {
    repair_error(ch);
    return;
  }

  if (broke_item == repair_material) {
    act("You cannot repair $p with itself!", FALSE, ch, broke_item, 0, TO_CHAR);
    return;
  }
  
  // Taken out since there's no such thing as weapon repair.
  //  if (broke_item->obj_flags.type != ITEM_WEAPON &&
  //      broke_item->obj_flags.type != ITEM_ARMOR) {

  if (broke_item->obj_flags.type != ITEM_ARMOR) {
    act("You cannot repair $p, it is not a piece of armor.", FALSE, ch, broke_item, 0, TO_CHAR);
    return;
  }
  
  if (broke_item->obj_flags.type == ITEM_ARMOR &&
      has_skill(ch, SKILL_ARMOR_REPAIR)) {
    modifiers = get_room_skill_bonus(ch->in_room, SKILL_ARMOR_REPAIR);
    sk_level = get_skill_percentage(ch, SKILL_ARMOR_REPAIR);
  }

  // There's nothing in here that supports weapon repair, taking this out
  // 'til there is something to do with it.
  // else if (broke_item->obj_flags.type == ITEM_WEAPON  &&
  //             has_skill(ch, SKILL_WEAPON_REPAIR)) {
  //    modifiers = get_room_skill_bonus(ch->in_room, SKILL_WEAPON_REPAIR);
  //    sk_level = get_skill_percentage(ch, SKILL_WEAPON_REPAIR);
  //  }
  
  if (sk_level == 0) {
    send_to_char("You know nothing of repairing such things.\n\r", ch);
    return;
  }

  int curr_points = broke_item->obj_flags.value[0];
  int max_curr_points = broke_item->obj_flags.value[1];

  if (curr_points == max_curr_points) {
    act("$p is not damaged.", FALSE, ch, broke_item, 0, TO_CHAR);
    return;
  }
  
  if (broke_item->obj_flags.material == MATERIAL_CERAMIC ||
      broke_item->obj_flags.material == MATERIAL_CLAY ||
      broke_item->obj_flags.material == MATERIAL_DUNG ||
      broke_item->obj_flags.material == MATERIAL_FEATHER ||
      broke_item->obj_flags.material == MATERIAL_FOOD ||
      broke_item->obj_flags.material == MATERIAL_FUNGUS ||
      broke_item->obj_flags.material == MATERIAL_GEM ||
      broke_item->obj_flags.material == MATERIAL_GLASS ||
      broke_item->obj_flags.material == MATERIAL_GRAFT ||
      broke_item->obj_flags.material == MATERIAL_METAL ||
      broke_item->obj_flags.material == MATERIAL_OBSIDIAN ||
      broke_item->obj_flags.material == MATERIAL_PAPER ||
      broke_item->obj_flags.material == MATERIAL_PLANT ||
      broke_item->obj_flags.material == MATERIAL_SALT ||
      broke_item->obj_flags.material == MATERIAL_SILK ||
      broke_item->obj_flags.material == MATERIAL_STONE ||
      broke_item->obj_flags.material == MATERIAL_TISSUE) {
    sprintf(buf, 
            "You cannot repair things made of %s.\n\r", 
            obj_material[(int) broke_item->obj_flags.material].name);
    send_to_char(buf, ch);
    return;
  }
  
  if (broke_item->obj_flags.material == MATERIAL_WOOD) {
    act("You cannot repair wood $o, a new one will have to be made.", FALSE, ch, broke_item, 0, TO_CHAR);
    return;
  }
  
  //  Review if this is something we should do - how long have we been building not keeping this in mind
  if ((broke_item->obj_flags.material == MATERIAL_BONE ||
       broke_item->obj_flags.material == MATERIAL_HORN ||
       broke_item->obj_flags.material == MATERIAL_ISILT ||
       broke_item->obj_flags.material == MATERIAL_TORTOISESHELL ||
       broke_item->obj_flags.material == MATERIAL_SKIN ||
       broke_item->obj_flags.material == MATERIAL_GWOSHI ||
       broke_item->obj_flags.material == MATERIAL_LEATHER ||
       broke_item->obj_flags.material == MATERIAL_CHITIN) &&
      (repair_material->obj_flags.type != ITEM_SKIN && 
       repair_material->obj_flags.type != ITEM_ARMOR)) {
    act("$p can only be repaired with unfinished goods or scavenging from other pieces of armor, $P is neither.",
        FALSE, ch, broke_item, repair_material, TO_CHAR);
    return;
  }
  
  if (broke_item->obj_flags.material == MATERIAL_CLOTH &&
      repair_material->obj_flags.type != ITEM_TOOL) {
    act("You would need an unworked bolt of cloth to repair $p.", FALSE, ch, broke_item, 0, TO_CHAR);
    return;
  }
  
  if (broke_item->obj_flags.material == MATERIAL_CHITIN &&
      repair_material->obj_flags.material != MATERIAL_CHITIN) {
    act("$P is not made of chitin like $p.", FALSE, ch, broke_item, repair_material, TO_CHAR);
    return;
  }
  
  if ((broke_item->obj_flags.material == MATERIAL_LEATHER ||
       broke_item->obj_flags.material == MATERIAL_GWOSHI ||
       broke_item->obj_flags.material == MATERIAL_SKIN) &&
      !(repair_material->obj_flags.material == MATERIAL_LEATHER ||
        repair_material->obj_flags.material == MATERIAL_GWOSHI ||
        repair_material->obj_flags.material == MATERIAL_SKIN)) {
    act("$P must be made of some kind of hide or leather to repair $p.", FALSE, ch, broke_item, repair_material, TO_CHAR);
    return;
  }
  else if ((broke_item->obj_flags.material == MATERIAL_BONE ||
            broke_item->obj_flags.material == MATERIAL_ISILT ||
            broke_item->obj_flags.material == MATERIAL_DUSKHORN ||
            broke_item->obj_flags.material == MATERIAL_HORN) &&
           !(repair_material->obj_flags.material == MATERIAL_HORN ||
             repair_material->obj_flags.material == MATERIAL_DUSKHORN ||
             repair_material->obj_flags.material == MATERIAL_ISILT ||
             repair_material->obj_flags.material == MATERIAL_BONE)) {
    act("$P must be made of some kind of horn or bone to repair $p.", FALSE, ch, broke_item, repair_material, TO_CHAR);
    return;
  } else if (broke_item->obj_flags.material != repair_material->obj_flags.material) {
    sprintf(buf, 
            "%s%s $o cannot be repaired using %s%s $O.",
            indefinite_article(obj_material[broke_item->obj_flags.material].name),
            obj_material[broke_item->obj_flags.material].name,
            indefinite_article(obj_material[repair_material->obj_flags.material].name),
            obj_material[repair_material->obj_flags.material].name);
    act(buf, FALSE, ch, broke_item, repair_material, TO_CHAR);
    return;
  }
  
  if (!ch->specials.to_process) {
    act("You begin making some repairs.", FALSE, ch, 0, 0, TO_CHAR);
    add_to_command_q(ch, oldarg, CMD_REPAIR, 60, 0);
    return;
  }
  
  //  int curr_max_points = broke_item->obj_flags.value[1];
  //  int curr_points = broke_item->obj_flags.value[0];
  int pts_to_repair = 0;
  
  if (max_curr_points <= 0) {
    cprintf(ch, "It is damaged beyond repair.\n\r");
    return;
  }
  
  
  if (sk_level < 20) {
    max_repair = MAX(1, max_curr_points - 4);
    pts_to_repair = 1;
  } else if (sk_level < 40) {
    max_repair = MAX(1, max_curr_points - 3);
    pts_to_repair = 2;
  } else if (sk_level < 60) {
    max_repair = MAX(1, max_curr_points - 2);
    pts_to_repair = 3;
  } else if (sk_level < 80) {
    max_repair = MAX(1, max_curr_points - 1);
    pts_to_repair = 4;
  } else { // 80+
    max_repair = max_curr_points;
    pts_to_repair = 5;
  }
  
  if (curr_points >= max_repair) {
    cprintf(ch, "It is as good as you can make it.\n\r");
    return;
  }
  
  if (repair_material->obj_flags.type == ITEM_ARMOR) {
    if (repair_material->obj_flags.value[0] < 1) {
      act("$P isn't in good enough condition to be used in repairing $p.", FALSE, ch, broke_item, repair_material, TO_CHAR);
      return;
    }
  } else if (repair_material->obj_flags.weight < 100) {
    act("To repair $p you'll need something bigger than $P.", FALSE, ch, broke_item, repair_material, TO_CHAR);
    return;
  }
  
  if (repair_material->obj_flags.type == ITEM_ARMOR) {
    pts_to_repair = MIN(pts_to_repair, repair_material->obj_flags.value[0]);
  } else {
    pts_to_repair = MIN(pts_to_repair, GET_OBJ_WEIGHT(repair_material));
  }
  
  if (max_repair - curr_points < pts_to_repair) {
    modifiers += (pts_to_repair - (max_curr_points - curr_points)) * 5;
    pts_to_repair = max_repair - curr_points;
  }
  
  if (affected_by_spell(ch, PSI_MINDWIPE)) {
    modifiers -= 25;
  }
  
  if (number(0, 101) <= sk_level + modifiers ) {
    modifiers -= ((max_curr_points - curr_points) - pts_to_repair) * 5;
    
    if (number(1, 80) > sk_level + modifiers) {
      MUD_SET_BIT(broke_item->obj_flags.state_flags, OST_REPAIRED); 
      
      gain_skill(ch, SKILL_ARMOR_REPAIR, 1);
    }
    
    curr_points = MIN(curr_points + pts_to_repair, max_repair);
    broke_item->obj_flags.value[0] = curr_points;
    
    if (curr_points == max_curr_points) {
      if (repair_material->obj_flags.type == ITEM_ARMOR) {
        act("You completely repair $p by salvaging bits off of $P.", FALSE, ch, broke_item, repair_material, TO_CHAR);
        act("$n completely repairs $p by salvaging bits off of $P.", FALSE, ch, broke_item, repair_material, TO_ROOM);
      } else {
        act("You completely repair $p with $P.", FALSE, ch, broke_item, repair_material, TO_CHAR);
        act("$n completely repairs $p with $P.", FALSE, ch, broke_item, repair_material, TO_ROOM);
      }
    } else {
      if (repair_material->obj_flags.type == ITEM_ARMOR) {
        act("You partially repair $p by salvaging bits off of $P.", FALSE, ch, broke_item, repair_material, TO_CHAR);
        act("$n partially repairs $p by salvaging bits off of $P.", FALSE, ch, broke_item, repair_material, TO_ROOM);
      } else {
        act("You partially repair $p with $P.", FALSE, ch, broke_item, repair_material, TO_CHAR);
        act("$n partially repairs $p with $P.", FALSE, ch, broke_item, repair_material, TO_ROOM);
      }
    }
  } else {
    act("You fail to repair $p with $P.", FALSE, ch, broke_item, repair_material, TO_CHAR);
    act("$n fails to repair $p with $P.", FALSE, ch, broke_item, repair_material, TO_ROOM);
    
    modifiers -= ((max_repair - curr_points) - pts_to_repair) * 5;
    
    if (number(1, 45) > sk_level + modifiers) {
      curr_points = MAX(0, (curr_points - number(1, pts_to_repair)));
      broke_item->obj_flags.value[0] = curr_points;
      
      act("You slip and do further damage to $p.", FALSE, ch, broke_item, repair_material, TO_CHAR);
      act("$n slips and does further damage to $p.", FALSE, ch, broke_item, repair_material, TO_ROOM);
    }
    
    gain_skill(ch, SKILL_ARMOR_REPAIR, number(1,3));
  }
  
  if (repair_material->obj_flags.type == ITEM_ARMOR) {
    repair_material->obj_flags.value[0] -= pts_to_repair;
    if (repair_material->obj_flags.value[0] < 1) {
      cprintf(ch, "The repairs leave %s useless as armor.\n\r", format_obj_to_char(repair_material, ch, 1));
    }
  } else {
    repair_material->obj_flags.weight -= pts_to_repair * 100; 
    if (repair_material->obj_flags.weight < 100) {
      cprintf(ch, "The repairs use the last of %s.\n\r", format_obj_to_char(repair_material, ch, 1));
      extract_obj(repair_material);
    }
  }
  

  return;
}


void
skill_value(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
            OBJ_DATA * tar_obj)
{
    char obj_name[256];

    arg = one_argument(arg, obj_name, sizeof(obj_name));

    if (!(tar_obj = get_obj_in_list_vis(ch, obj_name, ch->carrying))) {
        send_to_char("You do not seem to have anything like that.\n\r", ch);
        return;
    }

    if (!tar_obj) {
        send_to_char("Value what?\n\r", ch);
        return;
    }

    value_object(ch, tar_obj, TRUE, TRUE, TRUE);
    return;
}

void
value_object(CHAR_DATA *ch, OBJ_DATA *tar_obj, bool showValue, bool showWeight, bool showMaker) {
    char cost_edesc[256];
    char weight_edesc[256];
    char cost_edesc_name[256];
    char weight_edesc_name[256];

    int percent, price, weight, skill;
    float temp, price_variation, weight_variation;
    char buf[256];

    sprintf(cost_edesc_name, "[%s(%s)_PERCEIVED_COST]", ch->name, ch->account);
    sprintf(weight_edesc_name, "[%s(%s)_PERCEIVED_WEIGHT]", ch->name, ch->account);

    // They've valued this before, tell them the same thing again
    if (get_obj_extra_desc_value(tar_obj, weight_edesc_name, weight_edesc, MAX_STRING_LENGTH) &&
        get_obj_extra_desc_value(tar_obj, cost_edesc_name, cost_edesc, MAX_STRING_LENGTH)) {

        weight = atoi(weight_edesc);
        price = atoi(cost_edesc);

    }
    // They haven't valued this before, find out what they think
    else {
        weight = GET_OBJ_WEIGHT(tar_obj);
        price = tar_obj->obj_flags.cost;

        // You have the skill
        if (ch->skills[SKILL_VALUE]) {
            skill = ch->skills[SKILL_VALUE]->learned;
 
            // Your skill roll
            percent = number(1, 101);

            // Bonuses and penalties to your roll
            if (affected_by_spell(ch, PSI_MINDWIPE))
                percent += 25;

            // What happens if you make / don't make your skill check
            if (percent <= skill) {
                price_variation = 25;
                weight_variation = 25;
            }
            else {
                weight_variation = 35;
                price_variation = 35;
                gain_skill(ch, SKILL_VALUE, 1);
            }

            // Take the skill level into account
            temp = (100 - skill);
            weight_variation *= (temp / 100);
            price_variation *= (temp / 100);

        }

        // You don't have the skill, damn!
        else {
            weight_variation = 50;
            price_variation = 50;
        }

        // Apply entropy
        weight_variation = number(0, weight_variation); 
        price_variation = number(0, price_variation);

        // Get the price and weight that the player discerns
        if (number(0, 1))
            price += (price * (price_variation / 100));
        else
            price -= (price * (price_variation / 100));
        if (number(0, 1))
            weight += (weight * (weight_variation / 100));
        else
            weight -= (weight * (weight_variation / 100));

        // Minimum result is 1
        price = MAX(1, price);
        weight = MAX(1, weight);

        // Remember their perceived weight/cost
        sprintf(weight_edesc, "%d", weight);
        sprintf(cost_edesc, "%d", price);
        set_obj_extra_desc_value(tar_obj, weight_edesc_name, weight_edesc);
        set_obj_extra_desc_value(tar_obj, cost_edesc_name, cost_edesc);
    }

    if( showValue ) {
        if (price == 1) {
            sprintf(buf, "%s would seem to cost about %d obsidian piece\n\r",
                capitalize(format_obj_to_char(tar_obj, ch, 1)), price);
            send_to_char(buf, ch);
        } else {
            sprintf(buf, "%s would seem to cost about %d obsidian pieces.\n\r",
                capitalize(format_obj_to_char(tar_obj, ch, 1)), price);
            send_to_char(buf, ch);
        }
    }

    if( showWeight ) {
        if (weight == 1) {
            sprintf(buf, "%s would seem to weigh %d stone.\n\r",
                capitalize(format_obj_to_char(tar_obj, ch, 1)), weight);
            send_to_char(buf, ch);
        } else {
            sprintf(buf, "%s would seem to weigh %d stones.\n\r",
                capitalize(format_obj_to_char(tar_obj, ch, 1)), weight);
            send_to_char(buf, ch);
        }
    }

    if (!showMaker || !(ch->skills[SKILL_VALUE]))
        return;

    if (ch->skills[SKILL_VALUE]->learned > 5) {
        if (isnamebracketsok("[allanak_make]", OSTR(tar_obj, name)))
            send_to_char("This item shows the signs of Allanaki craftmanship." "\n\r", ch);
        if (isnamebracketsok("[allanaki_village_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted in one of the"
                         " Allanaki villages.\n\r", ch);
        if (isnamebracketsok("[cenyr_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted in the city" " of Cenyr.\n\r", ch);
        if (isnamebracketsok("[halfling_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by the hands"
                         " of a halfling.\n\r", ch);
        if (isnamebracketsok("[kadian_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by the"
                         " Merchant House of Kadius.\n\r", ch);
        if (isnamebracketsok("[kurac_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by the"
                         " Merchant House of Kurac.\n\r", ch);
        if (isnamebracketsok("[muark_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by the" " Tan Muark.\n\r", ch);
        if (isnamebracketsok("[northern_kadian_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by the"
                         " northern branch of Kadius.\n\r", ch);
        if (isnamebracketsok("[northlands_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted in " "the Northlands.\n\r", ch);
        if (isnamebracketsok("[northern_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted in " "the Northlands.\n\r", ch);
        if (isnamebracketsok("[salarr_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of" " House Salarr.\n\r", ch);
        if (isnamebracketsok("[salarr_make_north]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by the"
                         " northern branch of Salarr.\n\r", ch);
        if (isnamebracketsok("[salarr_make_south]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by the"
                         " southern branch of Salarr.\n\r", ch);
        if (isnamebracketsok("[southern_kadian_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by the "
                         "southern branch of Kadius.\n\r", ch);
        if (isnamebracketsok("[southlands_gith_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crudely crafted by " "githish hands.\n\r",
                         ch);
        if (isnamebracketsok("[mesa_gith_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crudely crafted by " "githish hands.\n\r",
                         ch);
        if (isnamebracketsok("[tribal_make]", OSTR(tar_obj, name)))
            send_to_char("This item looks as though it were crafted by " "an unknown tribe.\n\r",
                         ch);
        if (isnamebracketsok("[tuluk_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by " "Tuluki artisans.\n\r", ch);
        if (isnamebracketsok("[voryek_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of House Voryek." "\n\r", ch);
        if (isnamebracketsok("[red_storm_east_make]", OSTR(tar_obj, name)))
            send_to_char("This is crafted in a style known to be used in the "
                         "village of Red Storm East.\n\r", ch);
        if (isnamebracketsok("[rennik_make]", OSTR(tar_obj, name)))
            send_to_char("This item looks to have been crafted under the auspices "
                         "of House Rennik.\n\r", ch);
    }
    if (ch->skills[SKILL_VALUE]->learned >= 41) {
        if (isnamebracketsok("[akei_ta_var_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the elvish "
                         "tribe of the Akei Ta Var.\n\r", ch);
        if (isnamebracketsok("[arabet_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the "
                         "nomadic Arabet tribe.\n\r", ch);
        if (isnamebracketsok("[benjari_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the nomadic "
                         "Benjari tribe.\n\r", ch);
        if (isnamebracketsok("[blackwing_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the elvish "
                         "Blackwing tribe.\n\r", ch);
        if (isnamebracketsok("[conclave_make]", OSTR(tar_obj, name)))
            send_to_char("This item has a distinctive, almost mystical " "style.\n\r", ch);
        if (isnamebracketsok("[delann_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been crafted by "
                         "the Merchant House Delann.\n\r", ch);
        if (isnamebracketsok("[fale_make]", OSTR(tar_obj, name)))
            send_to_char("This item has a whimsical, flamboyant style "
                         "characteristic of House Fale.\n\r", ch);
        if (isnamebracketsok("[jul_tavan_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the nomadic "
                         "Jul Tavan tribe.\n\r", ch);
        if (isnamebracketsok("[kidanyali_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the northern "
                         "Kidanyali tribe.\n\r", ch);
        if (isnamebracketsok("[mantis_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been carved by " "a mantis.\n\r", ch);
        if (isnamebracketsok("[red_fang_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the "
                         "elvish Red Fang tribe.\n\r", ch);
        if (isnamebracketsok("[reynolte_make]", OSTR(tar_obj, name)))
            send_to_char("This item looks as though it might have been crafted "
                         "for the fallen House of Reynolte.\n\r", ch);
        if (isnamebracketsok("[seik_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the nomadic "
                         "Al Seik tribe.\n\r", ch);
        if (isnamebracketsok("[seven_spears_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the elvish "
                         "Seven Spears tribe.\n\r", ch);
        if (isnamebracketsok("[silt_wind_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the elvish "
                         "Silt Wind tribe.\n\r", ch);
        if (isnamebracketsok("[sand_jakhal_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the elvish "
                         "Sand Jackals tribe.\n\r", ch);
        if (isnamebracketsok("[sun_runner_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the "
                         "elvish Sun Runner tribe.\n\r", ch);
        if (isnamebracketsok("[sunrunner_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the "
                         "elvish Sun Runner tribe.\n\r", ch);
        if (isnamebracketsok("[tesdanyali_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the "
                         "northern Tesdanyali tribe.\n\r", ch);
/* added 12/31/02 -Sanvean */
        if (isnamebracketsok("[tenneshi_make]", OSTR(tar_obj, name)))
            send_to_char("This item looks as though it were crafted for "
                         "the northern noble house of Tenneshi.\n\r", ch);
        if (isnamebracketsok("[winrothol_make]", OSTR(tar_obj, name)))
            send_to_char("This item looks as though it were crafted for "
                         "the northern noble house of Winrothol.\n\r", ch);
        if (isnamebracketsok("[poet_make]", OSTR(tar_obj, name)))
            send_to_char("This object was made by a bard of Poets' " "Circle.\n\r", ch);
    }
    if (ch->skills[SKILL_VALUE]->learned > 60) {
        if (isnamebracketsok("[athali_make]", OSTR(tar_obj, name)))
            send_to_char("This item bears the distinctive style of the "
                         "northern Athali tribe.\n\r", ch);
        if (isnamebracketsok("[galith_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been made by " "galith.\n\r", ch);
        if (isnamebracketsok("[ghaati_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been made " "by ghaati.\n\r", ch);
        if (isnamebracketsok("[relic_make]", OSTR(tar_obj, name)))
            send_to_char("This item seems of ancient, " "unknown origin.\n\r", ch);
        if (isnamebracketsok("[mal_krian_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been made in the "
                         "fallen city of Mal Krian.\n\r", ch);
/* added 12/31/02 -Sanvean */
        if (isnamebracketsok("[folotran_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been made by the "
                         "northern Folotran tribe.\n\r", ch);
        if (isnamebracketsok("[zugara_make]", OSTR(tar_obj, name)))
            send_to_char("This item appears to have been made by the "
                         "northern Master-Bard, Zugara.\n\r", ch);
    }

    return;
}

void
wall_of_fire_subdue_throw_messages(struct char_data *ch, int n)
{
    char tmp[MAX_STRING_LENGTH];

    sprintf(tmp, "$n roughly shoves $N %s, through a wall of fire.", dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_NOTVICT);

    sprintf(tmp, "$n roughly shoves you %s, through a wall of fire.", dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_VICT);

    sprintf(tmp, "You release $N, and roughly shove $M %s, through a wall of " "fire.", dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_CHAR);

    return;
}

void
wall_of_thorn_subdue_throw_messages(struct char_data *ch, int n)
{
    char tmp[MAX_STRING_LENGTH];

    sprintf(tmp, "$n roughly shoves $N %s, through a wall of fire.", dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_NOTVICT);

    sprintf(tmp, "$n roughly shoves you %s, through a wall of fire.", dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_VICT);

    sprintf(tmp, "You release $N, and roughly shove $M %s, through a wall of " "fire.", dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_CHAR);

    return;
}

void
wall_of_wind_subdue_throw_messages(struct char_data *ch, int n)
{
    char tmp[MAX_STRING_LENGTH];

    sprintf(tmp, "$n roughly shoves $N %s, into a wall of wind, which " "blows $N back.", dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_NOTVICT);

    sprintf(tmp, "$n roughly shoves you %s, into a wall of wind, which " "blows you back.",
            dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_VICT);

    sprintf(tmp,
            "You release $N, and roughly shove $M %s, into a wall of " "wind, which blows $S back.",
            dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_CHAR);

    return;
}

void
blade_barrier_subdue_throw_messages(struct char_data *ch, int n)
{
    char tmp[MAX_STRING_LENGTH];

    sprintf(tmp, "$n roughly shoves $N %s, through a wall of whirling " "blades.", dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_NOTVICT);

    sprintf(tmp, "$n roughly shoves you %s, through a wall of whirling " "blades.", dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_VICT);

    sprintf(tmp, "You release $N, and roughly shove $M %s, through a " "wall of whirling blades.",
            dirs[n]);
    act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_CHAR);

    return;
}

void
skill_throw(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
            OBJ_DATA * tar_obj)
{                               /* begin skill_throw */
    int offense, defense, dam, n = 0, alive, prior_hitpoints;
    char tmp[256], buf[256], buf2[256], nbuf[256];
    char arg1[256], arg2[256], arg3[256], arg4[256];
    OBJ_DATA *dart, *wagon, *wagobj = 0;
    CHAR_DATA *victim = NULL;
    ROOM_DATA *tar_room, *was_in;
    bool is_far;
    struct weather_node *wn;
    float max_cond = 0.0;

#define CHECK_SUBDUED_WEIGHT
#ifdef CHECK_SUBDUED_WEIGHT
    int subdued_weight;
#endif

    is_far = FALSE;

    if (IS_CALMED(ch)) {
        send_to_char("You feel too at ease right now to attack anyone.\n\r", ch);
        return;
    }
#ifdef RANGED_STAMINA_DRAIN
    /* Too tired to do it. */
    /* Morgenes 4/19/2006 */
    if (GET_MOVE(ch) < 10) {
        cprintf(ch, "You are too tired!\n\r");
        return;
    }
#endif

    if (IS_AFFECTED(ch, CHAR_AFF_SUBDUED)) {
        send_to_char("You're held tightly and cannot throw anything.\n\r", ch);
        return;
    }

    arg = one_argument(arg, arg1, sizeof(arg1));
    arg = one_argument(arg, arg2, sizeof(arg2));
    arg = one_argument(arg, arg3, sizeof(arg3));

    // If a person isn't the first argument then they're not doing a subdue-throw
    // assume they're throwing an object

    if (!(victim = get_char_room_vis(ch, arg1))) {
      if (!((dart = ch->equipment[EP]) || (dart = ch->equipment[ES]))
          || !isname(arg1, OSTR(dart, name))) {
        send_to_char("You don't have that item equipped.\n\r", ch);
        return;
      }
      
      if (dart == ch->equipment[ES]
          && (dart->obj_flags.type == ITEM_WEAPON || dart->obj_flags.type == ITEM_DART)) {
        send_to_char("Weapons must be thrown from the primary hand.\n\r", ch);
        return;
      }
      
      if (ch_starts_falling_check(ch)) {
        return;
      }

      /* Need to see if we're throwing far first - Tiernan 2/19/99 */
      
      n = get_direction(ch, arg3);
      if ((n == -1) && (!stricmp(arg3, "out"))) {
        n = DIR_OUT;
      }
      
      if (n != -1) {
        if ((!IS_SET(dart->obj_flags.extra_flags, OFL_MISSILE_L))
            && ((dart->obj_flags.type == ITEM_WEAPON)
                || (dart->obj_flags.type == ITEM_DART))) {
          send_to_char("You can't throw it that far.\n\r", ch);
          return;
        }
        
        /* Get target at a distance. */
        tar_room = get_room_distance(ch, 1, n);
        if (!tar_room) {
          send_to_char("You can't see anyone in that direction.\n\r", ch);
          return;
        }
        victim = get_char_other_room_vis(ch, tar_room, arg2);
        if ((!victim)
            || (CAN_SEE(ch, victim) != 1 && CAN_SEE(ch, victim) != 7)) {
          if (n < 5) {
            sprintf(arg4, "to the %s", dir_name[n]);
          } else {
            sprintf(arg4, "%s", dir_name[n]);
          }
          
          snprintf(buf, sizeof(buf), "No one like that %s.\n\r", arg4);
          send_to_char(buf, ch);
          
          return;
        }
        
        is_far = TRUE;
      }
      
      
      if ((tar_obj) && (tar_obj->obj_flags.type == ITEM_WAGON)) {
        arg = one_argument(arg, arg4, sizeof(arg4));
        wagobj = tar_obj;
        if (!wagobj) {
          snprintf(buf, sizeof(buf), "You don't see %s%s to throw %s into.\n\r",
                   indefinite_article(arg4), arg4, arg1);
          send_to_char(buf, ch);
          return;
            }
            if (wagobj->obj_flags.type != ITEM_WAGON) {
                snprintf(buf, sizeof(buf), "You can't throw things into %s.\n\r",
                         OSTR(wagobj, short_descr));
                send_to_char(buf, ch);
                return;
            }
            if (wagobj->obj_flags.value[0] == NOWHERE) {        // Entrance Room
                snprintf(buf, sizeof(buf), "You can't throw things into %s.\n\r",
                         OSTR(wagobj, short_descr));
                send_to_char(buf, ch);
                return;
            }
            tar_room = get_room_num(wagobj->obj_flags.value[0]);
            victim = get_char_other_room_vis(ch, tar_room, arg2);
            if (obj_index[wagobj->nr].vnum == 73001) {  // vortex items
                snprintf(buf, sizeof(buf),
                         "You hurl %s at %s, but as it gets close an unknown force causes it to fall to the ground.\n\r",
                         OSTR(dart, short_descr), OSTR(wagobj, short_descr));
                send_to_char(buf, ch);
                snprintf(buf, sizeof(buf),
                         "$n hurls %s at %s, but as it gets close an unknown force causes it to fall to the ground.\n\r",
                         OSTR(dart, short_descr), OSTR(wagobj, short_descr));
                act(buf, FALSE, ch, 0, 0, TO_NOTVICT);

                if ((dart == ch->equipment[EP]))
                    obj_to_room(unequip_char(ch, EP), tar_room);
                if ((dart == ch->equipment[ES]))
                    obj_to_room(unequip_char(ch, ES), tar_room);
                return;
            }

            if (!victim) {
                send_to_char("You don't see anyone like that in there.\n\r", ch);
                return;
            }
            if (!char_can_see_char(ch, victim)) {
                send_to_char("You don't see anyone like that in there.\n\r", ch);
                return;
            }
        }

        if ((!victim) && !(victim = get_char_room_vis(ch, arg2))) {
            send_to_char("You don't see anyone like that here.\n\r", ch);
            return;
        }

        if (ch == victim) {
            send_to_char("Throw it at yourself?  " "That could be dangerous.\n\r", ch);
            return;
        }

        tar_room = victim->in_room;

        if (has_skill(ch, SKILL_THROW))
            offense = number(1, 50) + ch->skills[SKILL_THROW]->learned;
        else
            offense = number(1, 25);

        if (max_cond >= 2.5) {
            // look at weather condition
            for (wn = wn_list; wn; wn = wn->next) {
               // handle ch's room
               if (wn->zone == ch->in_room->zone) {
                  max_cond = MAX(max_cond, wn->condition);
                  offense -= MAX(0, (wn->condition - 1.5) * 10);
               }
        
               // if not in same room, other room
               if (ch->in_room != tar_room && wn->zone == tar_room->zone) {
                  max_cond = MAX(max_cond, wn->condition);
                  offense -= MAX(0, (wn->condition - 1.5) * 10);
               }
            }
        } else {

           // if worst condition is stormy, don't also apply wind
           offense -= (weather_info.wind_velocity / 5) * 2;
        }
    
        offense += agl_app[GET_AGL(ch)].missile;
    
        // let's give the defender a bit more than defense skill
        defense = number(1, 50);

        // but only if they're not paralyzed
        if (AWAKE(victim) && !is_paralyzed(victim))
           defense += victim->abilities.def + agl_app[GET_AGL(victim)].reaction;

        // mantis have amazing powers of deflection
        if (GET_RACE(victim) == RACE_MANTIS)
            defense *= 2;

        // distances influences skill
        if (is_far)
            offense -= 10;

        // can't throw at gesra's more than a room away
        if (GET_RACE(victim) == RACE_GESRA && n > 0)
            offense = 0;

        // no throwing at sentinels a room away
        if (IS_NPC(victim) && IS_SET(victim->specials.act, CFL_SENTINEL)
         && is_far)
            offense = 0;

        prior_hitpoints = GET_HIT(victim);

        if (n > -1)
            if (is_far && missile_wall_check(ch, dart, n, CMD_THROW))
                return;

        if (ch->specials.fighting && ch->specials.fighting == victim)
            clear_disengage_request(ch);

        if (!(dart->obj_flags.type == ITEM_WEAPON)
            && !(dart->obj_flags.type == ITEM_DART)) {  // Non-weapon throwing 
            if (n != -1) {      /* Throwing in direction */
                sprintf(nbuf, "$n throws $p %s.", dir_name[n]);
                act(nbuf, FALSE, ch, dart, tar_ch, TO_NOTVICT);
                sprintf(nbuf, "You throw $p %s, aiming for $N.", dir_name[n]);
                act(nbuf, FALSE, ch, dart, tar_ch, TO_CHAR);

                sprintf(nbuf, "$p flies in from the %s and lands on the " "ground near $n.",
                        rev_dirs[n]);
                act(nbuf, FALSE, ch, dart, tar_ch, TO_ROOM);
                sprintf(nbuf, "$p flies in from the %s and lands on the " "ground near you.",
                        rev_dirs[n]);
                act(nbuf, FALSE, tar_ch, dart, tar_ch, TO_VICT);
            }

            else if (!stricmp(arg3, "in")) {
                act("$p flies in from outside and lands on the ground near $n.", FALSE, ch, dart,
                    tar_ch, TO_NOTVICT);
                act("$p flies in from outside and lands on the ground near you.", FALSE, ch, dart,
                    tar_ch, TO_VICT);
                sprintf(nbuf, "You throw %s into %s at %s.", OSTR(dart, short_descr),
                        OSTR(wagobj, short_descr), MSTR(victim, short_descr));
                send_to_char(nbuf, ch);
            } else {
                act("$n throws $p at $N.", FALSE, ch, dart, tar_ch, TO_NOTVICT);
                act("You throw $p at $N.", FALSE, ch, dart, tar_ch, TO_CHAR);
                act("$n throws $p at you.", FALSE, ch, dart, tar_ch, TO_VICT);
            }
            if ((dart == ch->equipment[EP]))
                obj_to_room(unequip_char(ch, EP), tar_room);
            if ((dart == ch->equipment[ES]))
                obj_to_room(unequip_char(ch, ES), tar_room);
            return;
        }
#ifdef RANGED_STAMINA_DRAIN
        /* Stamina drain. */
        adjust_move(ch, -(number(1,
                   (100 - get_skill_percentage(ch, SKILL_THROW)) / 22 + 2) + (is_far ? 1 : 0) +
            (GET_OBJ_WEIGHT(dart) > 2 ? 1 : 0)));
#endif

        if (offense < defense) {
            if (!stricmp(arg3, "in"))
                alive = missile_damage(ch, victim, dart, DIR_IN, 0, SKILL_THROW);
            else if (is_far)
                alive = missile_damage(ch, victim, dart, n, 0, SKILL_THROW);
            else
                alive = ws_damage(ch, victim, 0, 0, 0, 0, SKILL_THROW, 0);

            /* high winds without skill, or offense = 0, no gain */
            if (has_skill(ch, SKILL_THROW) && offense > 0
             && max_cond - 1.5 * 10 < ch->skills[SKILL_THROW]->learned) {
                /* if target is asleep or paralyzed,
                 * decrease chance of learning
                 */
                if ((!AWAKE(victim) || is_paralyzed(victim)) && !number(0, 3)){
                    /* can only learn up to 40% on helpless (sparring dummies)*/
                    if (get_skill_percentage(ch, SKILL_THROW) <= 40)
                        gain_skill(ch, SKILL_THROW, 1);
                } else {
                    gain_skill(ch, SKILL_THROW, 3);
                }
            }
        } else {
            int poison_type = -1;

            // Changed this to allow new poison types - Tiernan 4/22
            if (dart->obj_flags.type == ITEM_DART
             && dart->obj_flags.value[3])
                poison_type = dart->obj_flags.value[3];
            else if (dart->obj_flags.type == ITEM_WEAPON
             && dart->obj_flags.value[5])
                poison_type = dart->obj_flags.value[5];
            else if (IS_SET(dart->obj_flags.extra_flags, OFL_POISON)) 
                poison_type = POISON_GENERIC;

            // figure out damage done
            if (dart->obj_flags.type == ITEM_WEAPON)
                dam =
                    dice(dart->obj_flags.value[1],
                         dart->obj_flags.value[2]) + dart->obj_flags.value[0];
            else
                dam = dice(dart->obj_flags.value[0], dart->obj_flags.value[1]);

            // strength modifier
            dam += str_app[GET_STR(ch)].todam;
            // + race modifier
            dam += race[(int) GET_RACE(ch)].attack->damplus;

            // if extra throw skill check, get a bonus based on str
            offense = number(1, 100);
            if (ch->skills[SKILL_THROW])
                if (offense < (ch->skills[SKILL_THROW]->learned))
                    dam += number(1, GET_STR(ch));

            // if (!stricmp(arg3, "in")) 
            if (tar_obj)
                alive = missile_damage(ch, victim, dart, DIR_IN, dam, SKILL_THROW);
            else if (is_far)
                alive = missile_damage(ch, victim, dart, n, dam, SKILL_THROW);
            else
                alive = ws_damage(ch, victim, dam, 0, 0, dam, SKILL_THROW, 0);

            // if they're still alive
            if (alive ) {
                // did it do any damage?
                if( prior_hitpoints != GET_HIT(victim)) {
                    // saving throw for poison
                    if (poison_type != -1 
                     && !does_save(victim, SAVING_PARA, 0)) {
                        poison_char(victim, poison_type,
                         number(1, (27 - GET_END(ch))), 0);
                        send_to_char("You feel very sick.\n\r", victim);
                    }

                    // remove poison after sucessfully doing damage
                    REMOVE_BIT(dart->obj_flags.extra_flags, OFL_POISON);
                    
                    if (dart->obj_flags.type == ITEM_DART)
                        dart->obj_flags.value[3] = 0;
                    else if (dart->obj_flags.type == ITEM_WEAPON)
                        dart->obj_flags.value[5] = 0;

                }

                // 1/4 of remaining health done & not sitting
                if ( dam > (GET_HIT(victim) / 4)
                 && (GET_POS(victim) > POSITION_SITTING)) {
                    if( victim->specials.riding ) {
                        // riding check
                        if( number(0, 100) 
                         > (get_skill_percentage(ch, SKILL_RIDE) - dam)) {
                            cprintf(victim, "You tumble to the ground.\n\r");
                            act("$n tumbles to the ground.", 
                             TRUE, victim, 0, 0, TO_NOTVICT);
                            set_char_position(victim, POSITION_SITTING);
                            victim->specials.riding = NULL; 
                            gain_skill(victim, SKILL_RIDE, number(1,2));
                            generic_damage(ch, number(1, 10), 0, 0, 
                             number(5, 20));
                        } else { // nearly fell
                            act("You nearly fall off $N, but recover.", FALSE,
                             victim, 0, victim->specials.riding, TO_CHAR);
                            act("$n nearly falls off $N.", TRUE, victim, 0, 
                             victim->specials.riding, TO_NOTVICT);
                        }
                        // always delayed
                        WAIT_STATE(victim, PULSE_VIOLENCE * 2);
                    } else {
                        cprintf(victim, "You stumble to the ground.\n\r");
                        act("$n stumbles to the ground.", TRUE, victim, 0, 0, 
                         TO_NOTVICT);
                        set_char_position(victim, POSITION_SITTING);
                        WAIT_STATE(victim, PULSE_VIOLENCE * 2);
                    }
                }
            }
        }

        if ((dart = ch->equipment[EP]))
            obj_to_room(unequip_char(ch, EP), tar_room);
        check_criminal_flag(ch, victim);
        return;
    }

    /*  Begin Kelvik addition to throw  */

    if (!(ch->specials.subduing == victim)) {
        act("You must get a hold on them first.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    subdued_weight = calc_subdued_weight(ch);

    n = (old_search_block(arg2, 0, strlen(arg2), dirs, 0));

    if ((n == -1) || (n == 0)) {
        if ((wagon = get_obj_room_vis(ch, arg2)) == NULL) {
            if (!strncmp(arg2, "outside", strlen(arg2))) {
                for (wagon = object_list; wagon; wagon = wagon->next)
                    if (wagon->obj_flags.type == ITEM_WAGON
                        && wagon->obj_flags.value[0] == ch->in_room->number)
                        break;

                if (wagon && wagon->in_room) {
                    if (IS_SET(wagon->obj_flags.value[3], WAGON_NO_LEAVE)) {
                        send_to_char("You cannot throw them out!\n\r", ch);
                        return;
                    }

                    if (IS_SET(wagon->in_room->room_flags, RFL_TUNNEL)
                        && (!room_can_contain_char(wagon->in_room, victim))) {
                        send_to_char("There's no more room that way.\n\r", ch);
                        return;
                    }

                    if ((IS_SET(wagon->in_room->room_flags, RFL_NO_MOUNT))
                        && (IS_SET(victim->specials.act, CFL_MOUNT))) {
                        send_to_char("There's not enough room that way.\n\r", ch);
                        return;
                    }
#ifdef RANGED_STAMINA_DRAIN
                    /* Stamina drain. */
                    adjust_move(ch, -number(5,10));
#endif

                    act("$n roughly shoves $N out of $p.", TRUE, ch, wagon, victim, TO_NOTVICT);

                    act("$n roughly shoves you out of $p.", TRUE, ch, wagon, victim, TO_VICT);

                    act("You release $N, and roughly shove $M out of $p.", TRUE, ch, wagon, victim,
                        TO_CHAR);

                    REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_SUBDUED);
                    ch->specials.subduing = (CHAR_DATA *) 0;

                    was_in = ch->in_room;
                    char_from_room_move(victim);
                    char_to_room(victim, wagon->in_room);
                    cmd_look(victim, "", -1, 0);
                    return;
                }
            }
            send_to_char("What direction is that?\n\r", ch);
            return;
        } else {

            tar_room = get_room_num(wagon->obj_flags.value[0]);
            if (tar_room == NULL) {
                gamelog("Error: Throwing person into wagon will NULL as room");
                return;
            }

            if (IS_SET(tar_room->room_flags, RFL_TUNNEL)
                && (!room_can_contain_char(wagon->in_room, victim))) {
                send_to_char("There's no more room that way.\n\r", ch);
                return;
            }

            if ((IS_SET(tar_room->room_flags, RFL_NO_MOUNT))
                && (IS_SET(victim->specials.act, CFL_MOUNT))) {
                send_to_char("There's not enough room that way.\n\r", ch);
                return;
            }
#ifdef RANGED_STAMINA_DRAIN
            adjust_move(ch, -number(5,10));
#endif

            act("$n roughly shoves $N into $p.", TRUE, ch, wagon, victim, TO_NOTVICT);

            act("$n roughly shoves you into $p.", TRUE, ch, wagon, victim, TO_VICT);

            act("You release $N, and roughly shove $M into $p.", TRUE, ch, wagon, victim, TO_CHAR);

            REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_SUBDUED);
            ch->specials.subduing = (CHAR_DATA *) 0;


            if (wagon->obj_flags.type == ITEM_WAGON && wagon->obj_flags.value[0] != 0
                && (tar_room = get_room_num(wagon->obj_flags.value[0])) != NULL) {
                was_in = ch->in_room;
                char_from_room_move(victim);
                char_to_room(victim, tar_room);
                cmd_look(victim, "", -1, 0);

                act("$n flies into the room from outside, landing on $s " "backside.", TRUE, victim,
                    0, 0, TO_ROOM);

                /* so mortally wounded don't end up sitting up */
                if (GET_POS(victim) > POSITION_SITTING)
                    set_char_position(victim, POSITION_SITTING);
            } else
                generic_damage(victim, number(1, 4), 0, 0, number(1, 4));
            return;
        }
    }
    --n;

    /* no throwing them up in the air and watching for them to fall */
    /* -Morg */
    if (n == DIR_UP) {
#ifdef CHECK_SUBDUED_WEIGHT
/*
      sprintf( buf, "%s just tried to throw %s up [%d vs %d]",
       MSTR( ch, name ), MSTR( victim, name ),
       subdued_weight, str_app[GET_STR(ch)].carry_w );
      gamelog( buf );
*/
        if (subdued_weight > str_app[GET_STR(ch)].carry_w) {
            send_to_char("You can't throw them up.\n\r", ch);
            return;
        }
#else
        sprintf(buf, "%s just tried to throw %s up", MSTR(ch, name), MSTR(victim, name));
        send_to_monitor(ch, buf, MONITOR_OTHER);

        /* possibly add a check here to allow extreme str vs size */
        send_to_char("You can't throw them up.\n\r", ch);
        return;
#endif
    }
#ifdef CHECK_SUBDUED_WEIGHT
    else {
/*
      sprintf( buf, "%s just tried to throw %s %s [%d vs %d]",
       MSTR( ch, name ), MSTR( victim, name ),
       dirs[n],
       subdued_weight, str_app[GET_STR(ch)].carry_w * 4 );
      gamelog( buf );
*/
        if (subdued_weight > str_app[GET_STR(ch)].carry_w * 4) {
            send_to_char("You can't throw them, they're too heavy.\n\r", ch);
            return;
        }
    }
#endif

    if (!ch->in_room->direction[n]) {
        send_to_char("But you see no exit that way.\n\r", ch);
        return;
    } else {                    /* Direction is possible */
        if (IS_SET(EXIT(ch, n)->exit_info, EX_SPL_SAND_WALL)) {
            send_to_char("A large wall of sand blocks the way.\n\r", ch);
            return;
        }
        if (IS_SET(EXIT(ch, n)->exit_info, EX_CLOSED)) {
            if (!IS_SET(EXIT(ch, n)->exit_info, EX_SECRET))
                if (EXIT(ch, n)->keyword) {
                    sprintf(tmp, "The %s seems to be closed.\n\r",
                            first_name(EXIT(ch, n)->keyword, buf2));
                    send_to_char(tmp, ch);
                } else
                    send_to_char("It seems to be closed.\n\r", ch);
            else                /*  IS secret (and closed)  */
                send_to_char("But you see no exit that way.\n\r", ch);
            return;
        } else if (!EXIT(ch, n)->to_room) {
            send_to_char("But you see no exit that way.\n\r", ch);
            return;
        }
    }


    if (exit_guarded(ch, n, CMD_THROW))
        return;                 /* messages all in exit_guarded_messages */

    tar_room = ch->in_room->direction[n]->to_room;
    if (IS_SET(tar_room->room_flags, RFL_TUNNEL)
        && (!room_can_contain_char(tar_room, victim))) {
        send_to_char("There's no more room that way.\n\r", ch);
        return;
    }

    if ((IS_SET(ch->in_room->direction[n]->to_room->room_flags, RFL_NO_MOUNT))
        && (IS_SET(victim->specials.act, CFL_MOUNT))) {
        send_to_char("There's not enough room that way.\n\r", ch);
        return;
    }

    if (IS_SET(EXIT(ch, n)->exit_info, EX_SPL_FIRE_WALL)) {
        wall_of_fire_subdue_throw_messages(ch, n);
        was_in = ch->in_room;
        char_from_room_move(victim);
        char_to_room(victim, was_in->direction[n]->to_room);
        cmd_look(victim, "", -1, 0);

        sprintf(buf,
                "$n flies into the room from %s, through a wall of fire, "
                "landing on $s backside.", rev_dir_name[n]);
        act(buf, FALSE, ch->specials.subduing, 0, 0, TO_ROOM);

        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
        ch->specials.subduing = (CHAR_DATA *) 0;

        /* so mortally wounded don't end up sitting up */
        if (GET_POS(victim) > POSITION_SITTING)
            set_char_position(victim, POSITION_SITTING);
        return;
    }
    if (IS_SET(EXIT(ch, n)->exit_info, EX_SPL_BLADE_BARRIER)) {
        blade_barrier_subdue_throw_messages(ch, n);
        was_in = ch->in_room;
        char_from_room_move(victim);
        char_to_room(victim, was_in->direction[n]->to_room);
        cmd_look(victim, "", -1, 0);

        sprintf(buf,
                "$n flies into the room from %s, through a wall of "
                "whirling blades, landing on $s backside.", rev_dir_name[n]);
        act(buf, FALSE, ch->specials.subduing, 0, 0, TO_ROOM);

        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
        ch->specials.subduing = (CHAR_DATA *) 0;

        /* so mortally wounded don't end up sitting up */
        if (GET_POS(victim) > POSITION_SITTING)
            set_char_position(victim, POSITION_SITTING);
        return;
    }
    if (IS_SET(EXIT(ch, n)->exit_info, EX_SPL_WIND_WALL)) {
        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
        ch->specials.subduing = (CHAR_DATA *) 0;

        /* so mortally wounded don't end up sitting up */
        if (GET_POS(victim) > POSITION_SITTING)
            set_char_position(victim, POSITION_SITTING);
    } else {
#ifdef RANGED_STAMINA_DRAIN
        adjust_move(ch, -number(5,10));
#endif

        sprintf(tmp, "$n roughly shoves $N %s.", dirs[n]);
        act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_NOTVICT);

        sprintf(tmp, "$n roughly shoves you %s.", dirs[n]);
        act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_VICT);

        sprintf(tmp, "You release $N, and roughly shove $M %s.", dirs[n]);
        act(tmp, FALSE, ch, 0, ch->specials.subduing, TO_CHAR);

        was_in = ch->in_room;
        char_from_room_move(victim);
        char_to_room(victim, was_in->direction[n]->to_room);
        cmd_look(victim, "", -1, 0);

        sprintf(buf, "$n flies into the room from %s, landing on $s backside.", rev_dir_name[n]);
        act(buf, FALSE, ch->specials.subduing, 0, 0, TO_ROOM);

        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
        /* so mortally wounded don't end up sitting up */
        if (GET_POS(victim) > POSITION_SITTING)
            set_char_position(victim, POSITION_SITTING);

        ch->specials.subduing = (CHAR_DATA *) 0;
    }
}


void
cmd_release(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *person;
    CHAR_DATA *victim;
    CHAR_DATA *new_subduer;

    if (!ch->specials.subduing) {
        act("You aren't subduing anyone else.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (argument != NULL) {
        argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
    } else {
        arg2[0] = '\0';
    }

    if (!*arg2) {
        act("$n releases $N, who immediately moves away.", FALSE, ch, 0, ch->specials.subduing,
            TO_NOTVICT);
        act("$n releases you, and you immediately move away.", FALSE, ch, 0, ch->specials.subduing,
            TO_VICT);
        act("You release $N, and $E immediately moves away.", FALSE, ch, 0, ch->specials.subduing,
            TO_CHAR);

        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
        ch->specials.subduing = (CHAR_DATA *) 0;
        return;
    };

    if (!(victim = get_char_room_vis(ch, arg1))) {
        gamelog("No victim in cmd_release.");
        return;
    };

    if (victim != ch->specials.subduing) {
        send_to_char("You aren't subduing that person.\n\r", ch);
        return;
    };

    if (!(new_subduer = get_char_room_vis(ch, arg2))) {
        send_to_char("You don't see that person here.\n\r", ch);
        return;
    };

    if (new_subduer == victim) {
        send_to_char("You can't release a person to themself!\n\r", ch);
        return;
    };

    if (new_subduer->specials.subduing) {
        sprintf(buf, "%s hands are full with %s.\n\r", MSTR(new_subduer, short_descr),
                MSTR(victim, short_descr));
        send_to_char(buf, ch);
    };

    if (!IS_IN_SAME_TRIBE(ch, new_subduer)) {
        if (!is_soldier(new_subduer) && !IS_NPC(new_subduer)) {
            sprintf(buf, "$N refuses to take %s from you.", MSTR(victim, short_descr));
            act(buf, FALSE, ch, 0, new_subduer, TO_CHAR);
            return;
        }
    };

    /* Check and make it size dependent - Dakurus */
    if (get_char_size(victim) - get_char_size(new_subduer) > 7) {
        send_to_char("Your victim is much too large for them! There's no way!\n\r", ch);
        return;
    }

    sprintf(buf, "You release $N and shove $M roughly into %s's arms.",
            MSTR(new_subduer, short_descr));
    act(buf, FALSE, ch, 0, victim, TO_CHAR);

    sprintf(buf, "$n releases you, shoving you roughly into %s's arms.",
            MSTR(new_subduer, short_descr));
    act(buf, FALSE, ch, 0, victim, TO_VICT);

    sprintf(buf, "%s releases $N, shoving $M roughly into your arms.",
            capitalize(PERS(ch, new_subduer)));
    act(buf, FALSE, new_subduer, 0, victim, TO_CHAR);


    for (person = ch->in_room->people; person; person = person->next_in_room) {
        if ((person != ch) && (person != new_subduer) && (person != victim)) {
            sprintf(buf, "%s releases %s, shoving %s roughly into %s's arms.\n\r",
                    capitalize(PERS(ch, person)), PERS(victim, person),
                    HMHR(victim), PERS(new_subduer, person));
            send_to_char(buf, person);
        }
    }

    REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_SUBDUED);
    ch->specials.subduing = (CHAR_DATA *) 0;

    if (is_charmed(victim)) {
      REMOVE_BIT(victim->specials.act, CHAR_AFF_CHARM);
    }

    new_subduer->specials.subduing = victim;
    MUD_SET_BIT(victim->specials.affected_by, CHAR_AFF_SUBDUED);
    stop_follower(victim);
    stop_guard(victim, 0);
    stop_watching(victim, 0);
    stop_guard(new_subduer, 0);
    stop_watching(new_subduer, 0);
}

void
skill_subdue(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    int learned = 5, min_learned = 0;
    int dir = number(0, 3);
    int ct = 0;
    char tmp[256];
    CHAR_DATA *tch;
    OBJ_DATA *obj;
    ROOM_DATA *room;
    char buf[MAX_STRING_LENGTH];

    /* added check so calmed people can't subdue */
    /* 1/16/00 -Sanvean                          */

    if (IS_CALMED(ch)) {
        send_to_char("You feel too at ease right now to attack anyone.\n\r", ch);
        return;
    }

    /* watch for too tired */
    if (GET_MOVE(ch) < 10) {
        cprintf(ch, "You are too tired!\n\r");
        return;
    }

    if (!CAN_SEE(ch, tar_ch)) { /* Check added [geez Dan!] - Kelvik */
        send_to_char("Subdue whom?\n\r", ch);
        return;
    }

    if (affected_by_spell(ch, PSI_DISORIENT)) {
        act("You attempt to grab $N, but completely miss.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n attempts to grab you, but completely misses.", FALSE, ch, 0, tar_ch, TO_VICT);
        act("$n attempts to grab $N, but completely misses.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        return;
    }

    if (ch->specials.subduing) {
        act("You already have $N subdued.", FALSE, ch, 0, ch->specials.subduing, TO_CHAR);
        return;
    }

    /* can't subdue trees and stuff, cause it's dumb  --Ur */
    if ((GET_RACE(tar_ch) == RACE_GESRA)) {
        send_to_char("That's ridiculous.\n\r", ch);
        return;
    }

    /* Removed hard HG/Giant check and made it size dependent - Dakurus */
    if (get_char_size(tar_ch) - get_char_size(ch) > 7) {
        send_to_char("Your target is much too large for you. There's no way!\n\r", ch);
        return;
    }
/*  NOT taken out because there is no way a mul will subdue a h-g - Kel */

    if (is_char_ethereal(ch) != is_char_ethereal(tar_ch)) {
        act("Your arms pass right through $M!", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n grabs at you, but $s arms pass right through you.", FALSE, ch, 0, tar_ch, TO_VICT);
        act("$n grabs at $N, but ends up hugging air.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        return;
    }

    /*  Can't subdue someone who is flying unless
     *  you are flying as well or they can't see u  -  Kel  */
    /* except featherfall */
    if ((IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
         && !affected_by_spell(tar_ch, SPELL_FEATHER_FALL))
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
        && CAN_SEE(tar_ch, ch)) {
        act("$n tries to grab $N out of the air, but cannot reach $M.", FALSE, ch, 0, tar_ch,
            TO_NOTVICT);
        act("You fruitlessly attempt to nab $M out of the air.", TRUE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to grab you out of the air, but you avoid $m.", TRUE, ch, 0, tar_ch, TO_VICT);
        return;
    }

    if (ch->skills[SKILL_SUBDUE]) {
        learned += (ch->skills[SKILL_SUBDUE]->learned / 2);
        min_learned = learned;
    }

    learned += (3 * (GET_STR(ch) - GET_STR(tar_ch)));
    learned += (2 * (GET_AGL(ch) - GET_AGL(tar_ch)));
    learned += (get_char_size(ch) - get_char_size(tar_ch));

    learned = MAX(learned, min_learned);        // Don't go lower than min_learned
    learned = MIN(learned, 90); // Or higher than 90 (unless sleeping, see below)

    if (GET_POS(tar_ch) < POSITION_STANDING && GET_POS(tar_ch) > POSITION_SLEEPING) {
        learned += 10 * (POSITION_STANDING - GET_POS(tar_ch));
    }


    if (ch->specials.riding) {
        act("That would be pretty difficult while riding $r.", FALSE, ch, 0, ch->specials.riding,
            TO_CHAR);
        return;
    }
    if (ch->lifting) {
        sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
        send_to_char(buf, ch);
        if (!drop_lifting(ch, ch->lifting))
            return;
    }

    for (tch = ch->in_room->people; tch; tch = tch->next_in_room)
        if (tch->specials.subduing == tar_ch)
            break;

    if (tch) {
        act("$N is already subdued by someone else.", FALSE, ch, 0, tar_ch, TO_CHAR);
        return;
    }

    /*  Check added for obvious reasons here.  - Kelvik  */
    if (IS_IMMORTAL(tar_ch)) {
        send_to_char("That would only prove futile, and foolish.\n\r", ch);
        return;
    }

    if ((ch->specials.fighting) || (tar_ch->specials.fighting)) {
        act("You cannot get a hold while the fight is going on.", FALSE, ch, 0, tar_ch, TO_CHAR);
        return;
    }

    /* can't subdue and stay invis *//*  Kel  */
    if (IS_AFFECTED(ch, CHAR_AFF_INVISIBLE)) {
        act("$n becomes visible as $e grabs at you.", FALSE, ch, 0, tar_ch, TO_VICT);
        act("$n suddenly snaps into visibility as $e begins grappling with $N.", FALSE, ch, 0,
            tar_ch, TO_NOTVICT);
        act("You see yourself fade back into existence as you grab at $M.", TRUE, ch, 0, tar_ch,
            TO_CHAR);
        if (affected_by_spell(ch, SPELL_INVISIBLE))
            affect_from_char(ch, SPELL_INVISIBLE);
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_INVISIBLE);
    }

    if (!IS_IN_SAME_TRIBE(ch, tar_ch))
        if (guard_check(ch, tar_ch))
            return;

    /*  Everything's ok.  Let's subdue em.  */
    if (IS_IMMORTAL(ch))
        act("About to do check for ES.", FALSE, ch, 0, 0, TO_CHAR);

    if (ch->equipment[ES]) {
        obj = ch->equipment[ES];
        obj_to_room(unequip_char(ch, ES), ch->in_room);

        act("[ You stop using $p. ]", FALSE, ch, obj, 0, TO_CHAR);
        act("You drop $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n hastily drops $p.", TRUE, ch, obj, 0, TO_ROOM);

        drop_light(obj, ch->in_room);
    }

    if (IS_IMMORTAL(ch))
        act("About to do check for EP.", FALSE, ch, 0, 0, TO_CHAR);

    if (ch->equipment[EP]) {
        obj = ch->equipment[EP];
        obj_to_room(unequip_char(ch, EP), ch->in_room);

        act("[ You stop using $p. ]", FALSE, ch, obj, 0, TO_CHAR);
        act("You drop $p in the struggle.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n hastily drops $p.", TRUE, ch, obj, 0, TO_ROOM);

        drop_light(obj, ch->in_room);
    }

    if (IS_IMMORTAL(ch))
        act("About to do check for ETWO.", FALSE, ch, 0, 0, TO_CHAR);

    if (ch->equipment[ETWO]) {
        obj = ch->equipment[ETWO];
        obj_to_room(unequip_char(ch, ETWO), ch->in_room);

        act("[ You stop using $p. ]", FALSE, ch, obj, 0, TO_CHAR);
        act("You drop $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n hastily drops $p.", TRUE, ch, obj, 0, TO_ROOM);

        if (obj->obj_flags.type == ITEM_LIGHT && obj->obj_flags.value[0])
            ch->in_room->light--;
    }

    if (IS_IMMORTAL(ch))
        act("About to do check for sleeping.", FALSE, ch, 0, 0, TO_CHAR);

    if (!AWAKE(tar_ch))
        learned = 101;

    if (GET_POS(tar_ch) == POSITION_SLEEPING && !affected_by_spell(tar_ch, SPELL_SLEEP)) {
        set_char_position(tar_ch, POSITION_STANDING);
        act("You wake up as you feel your arms being twisted behind your back.", FALSE, ch, 0,
            tar_ch, TO_VICT);
    }

/* Chars who are sitting/resting when subdued now re-set */
/* to standing.    -Sanvean 9/9/00                       */

    if (GET_POS(tar_ch) == POSITION_RESTING || GET_POS(tar_ch) == POSITION_SITTING) {
        set_char_position(tar_ch, POSITION_STANDING);
        act("You are hauled to your feet roughly.", FALSE, ch, 0, tar_ch, TO_VICT);
    }



    if (IS_IMMORTAL(ch))
        act("About to do if(skill_success(ch, tar_ch, learned)).", FALSE, ch, 0, 0, TO_CHAR);

    /* Stamina drain. */
    adjust_move(ch, -number(2, (100 - get_skill_percentage(ch, SKILL_SUBDUE)) / 22 + 3));

    ct = room_in_city(ch->in_room);

    if (skill_success(ch, tar_ch, learned)
        || IS_SET(tar_ch->specials.nosave, NOSAVE_SUBDUE)
        || (ct && IS_TRIBE(ch, city[ct].tribe) && IS_SET(tar_ch->specials.nosave, NOSAVE_ARREST))
        || !AWAKE(tar_ch)
        || is_paralyzed(tar_ch)) {
        if (affected_by_spell(tar_ch, SPELL_WIND_ARMOR)) {
            if ((number(1, 10)) < 10) {
                act("A sudden guest of wind hurls you away from $N!", FALSE, ch, 0, tar_ch,
                    TO_CHAR);
                act("$n is hurled away from you by a sudden gust of wind!", FALSE, ch, 0, tar_ch,
                    TO_VICT);
                act("$n is hurled away from $N by a sudden guest of wind!", FALSE, ch, 0, tar_ch,
                    TO_NOTVICT);

                if (!ch->in_room->direction[dir])
                    return;

                if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)
                    && !is_char_ethereal(ch)) {
                    if (EXIT(ch, dir)->keyword) {
                        sprintf(tmp, "A violent wind slams $n against the " "closed %s.",
                                EXIT(ch, dir)->keyword);
                        act(tmp, FALSE, ch, 0, 0, TO_ROOM);
                        sprintf(tmp, "A violent wind slams you against the " "closed %s.",
                                EXIT(ch, dir)->keyword);
                        act(tmp, FALSE, ch, 0, 0, TO_CHAR);

                        /* Then damage them for colliding against it */
                        do_damage(ch, 10, 10);
                        return;
                    }
                }

                act("$n is hurled out of the room by a violent wind!", FALSE, ch, 0, 0, TO_ROOM);
                act("You are hurled out of the room by a violent wind!", FALSE, ch, 0, 0, TO_CHAR);

                room = ch->in_room;
                char_from_room(ch);
                char_to_room(ch, room->direction[dir]->to_room);

                act("$n is hurled into the room by a violent wind!", FALSE, ch, 0, 0, TO_ROOM);

                cmd_look(ch, "", 0, 0);

                return;
            } else {
                act("You subdue $N, despite a sudden gust of wind that almost "
                    "hurls you away from $M.", FALSE, ch, 0, tar_ch, TO_CHAR);
                act("$n subdues you, despite a sudden gust of wind that almost"
                    " hurls $m away from you.", FALSE, ch, 0, tar_ch, TO_VICT);
                act("$n subdues $N despite a sudden guest of wind that almost"
                    " hurls $m away from $N.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
            }
        } else if (!AWAKE(tar_ch) || is_paralyzed(tar_ch)
                   || (ct && IS_TRIBE(ch, city[ct].tribe)
                       && IS_SET(tar_ch->specials.nosave, NOSAVE_ARREST))
                   || IS_SET(tar_ch->specials.nosave, NOSAVE_SUBDUE)) {
            act("You subdue $N.", FALSE, ch, 0, tar_ch, TO_CHAR);
            act("$n subdues you.", FALSE, ch, 0, tar_ch, TO_VICT);
            act("$n subdues $N.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        } else {
            act("You subdue $N, despite $S attempts to struggle away.", FALSE, ch, 0, tar_ch,
                TO_CHAR);
            act("$n subdues you, despite your attempts to struggle away.", FALSE, ch, 0, tar_ch,
                TO_VICT);
            act("$n subdues $N, despite $S attempts to struggle away.", FALSE, ch, 0, tar_ch,
                TO_NOTVICT);
        }

        if (affected_by_spell(tar_ch, SPELL_FIRE_ARMOR)) {
            act("You are burned harshly by the flames surrounding $N.", FALSE, ch, 0, tar_ch,
                TO_CHAR);
            act("$n is burned harshly by the flames surrounding you.", FALSE, ch, 0, tar_ch,
                TO_VICT);
            act("$n is burned harshly by the flames surrounding $N.", FALSE, ch, 0, tar_ch,
                TO_NOTVICT);
            do_damage(ch, 10, 0);
        }
        
        if (is_charmed(tar_ch)) {
          REMOVE_BIT(tar_ch->specials.act, CHAR_AFF_CHARM);
        }

        if (GET_POS(tar_ch) < POSITION_STANDING && GET_POS(tar_ch) > POSITION_SLEEPING)
            set_char_position(tar_ch, POSITION_STANDING);

        if (ch->on_obj)
            remove_occupant(ch->on_obj, ch);

        /* Stuff to yank the target off of whatever they're on */
        if (tar_ch->on_obj)
            remove_occupant(tar_ch->on_obj, tar_ch);


        if (tar_ch->specials.riding)
            tar_ch->specials.riding = NULL;

        /* Added so if you're subdued, you're unhidden.  -San     */
        if (IS_SET(tar_ch->specials.affected_by, CHAR_AFF_HIDE))
            REMOVE_BIT(tar_ch->specials.affected_by, CHAR_AFF_HIDE);


        ch->specials.subduing = tar_ch;
        MUD_SET_BIT(tar_ch->specials.affected_by, CHAR_AFF_SUBDUED);
        stop_follower(tar_ch);
        stop_guard(tar_ch, 0);
        stop_watching(tar_ch, 0);
        stop_guard(ch, 0);
        stop_watching(ch, 0);
    } else {
        if (affected_by_spell(tar_ch, SPELL_FIRE_ARMOR)) {
            act("You are burned harshly by the flames surrounding $N and" " let $M go.", FALSE, ch,
                0, tar_ch, TO_CHAR);
            act("$n is burned harshly by the flames surrounding you, and" "lets you go.", FALSE, ch,
                0, tar_ch, TO_VICT);
            act("$n is burned harshly by the flames surrounding $N, and" " lets $M go.", FALSE, ch,
                0, tar_ch, TO_NOTVICT);
            do_damage(ch, 10, 0);
        } else {
            act("You attempt to grab $N, but $E wrestles away.", FALSE, ch, 0, tar_ch, TO_CHAR);
            act("$n attempts to grab you, but you wrestle away.", FALSE, ch, 0, tar_ch, TO_VICT);
            act("$n attempts to grab $N, but $E wrestles away.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        }

        /* has to be here, otherwise no crim cause the next hit by the victim
         * will cause them to be crimmed */
        if (!IS_SET(tar_ch->specials.nosave, NOSAVE_SUBDUE)
            && !(ct && IS_TRIBE(ch, city[ct].tribe)
                 && IS_SET(tar_ch->specials.nosave, NOSAVE_ARREST)))
            check_criminal_flag(ch, tar_ch);

        if ((IS_NPC(tar_ch)) && (GET_POS(tar_ch) > POSITION_STUNNED)
            && (!tar_ch->desc))
            hit(tar_ch, ch, TYPE_UNDEFINED);

        gain_skill(ch, SKILL_SUBDUE, 2);
        return;
    }

    if (IS_IMMORTAL(ch))
        act("Done with if skill_success.", FALSE, ch, 0, 0, TO_CHAR);

    if (GET_POS(tar_ch) >= POSITION_SLEEPING && GET_POS(tar_ch) <= POSITION_FIGHTING
        && !affected_by_spell(tar_ch, SPELL_SLEEP))
        set_char_position(tar_ch, POSITION_STANDING);

    if (AWAKE(tar_ch) && !is_paralyzed(tar_ch)
        && !IS_SET(tar_ch->specials.nosave, NOSAVE_SUBDUE)
        && !(ct && IS_TRIBE(ch, city[ct].tribe) && IS_SET(tar_ch->specials.nosave, NOSAVE_ARREST)))
        check_criminal_flag(ch, tar_ch);
    return;
}


void
skill_bash(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{
    /*  New bash.  - Kel  */
    int chance = 0;
    int dodge = 0;

    if (IS_CALMED(ch)) {
        send_to_char("You feel too at ease right now to attack anyone.\n\r", ch);
        return;
    }

    /* Too tired to do it. */
    /* Raesanos 4/19/2006 */
    if (GET_MOVE(ch) < 10) {
        send_to_char("You are too tired!\n\r", ch);
        return;
    }

    if (GET_POS(tar_ch) < POSITION_FIGHTING) {
        act("But $E is already on the ground.", FALSE, ch, 0, tar_ch, TO_CHAR);
        /* no wait state */
        ch->specials.act_wait = 0;
        return;
    }

    if (ch->specials.riding) {
        send_to_char("That would be hard while riding.\n\r", ch);
        return;
    }

    if (tar_ch->specials.riding) {
        send_to_char("You can't bash them while they are mounted.\n\r", ch);
        return;
    }

    if (ch->specials.fighting && tar_ch == ch->specials.fighting)
        clear_disengage_request(ch);

    /* Stamina drain. */
    adjust_move(ch, -number(2, (100 - get_skill_percentage(ch, SKILL_BASH)) / 22 + 3));

    if (is_char_ethereal(ch) != is_char_ethereal(tar_ch)) {
        act("You stumble right through $M, falling over as you pass.", FALSE, ch, 0, tar_ch,
            TO_CHAR);
        act("$n charges at you, passing through your form harmlessly.", FALSE, ch, 0, tar_ch,
            TO_VICT);
        act("$n charges at $N, but stumbles to the ground.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        set_char_position(ch, POSITION_SITTING);
        return;
    }

    /*  Can't bash someone who is flying unless you are flying as well
     * or they can't see you                                           */
    if ((IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
         && !affected_by_spell(tar_ch, SPELL_FEATHER_FALL))
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
        && CAN_SEE(tar_ch, ch)) {
        act("You try, but $E rises overhead and evades you.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to knock you over, but you rise overhead.", FALSE, ch, 0, tar_ch, TO_VICT);
        return;
    }

    /* Adding check for energy shield */
    if (affected_by_spell(tar_ch, SPELL_ENERGY_SHIELD)) {
        act("In a spectacular shower of sparks, your blow bounces off $N.", FALSE, ch, 0, tar_ch,
            TO_CHAR);
        act("$n tries to bash you, but your energy shield deflects $m.", FALSE, ch, 0, tar_ch,
            TO_VICT);
        act("$n bashes at $N, but is deflected in a shower of sparks.", FALSE, ch, 0, tar_ch,
            TO_NOTVICT);
        return;
    }

    if (guard_check(ch, tar_ch))
        return;

    if (ch->in_room != tar_ch->in_room) {
        send_to_char("You don't see that person here.\n\r", ch);
        return;
    }

    if (ch->skills[SKILL_BASH] || is_paralyzed(tar_ch)) {
        int csize, tsize;

        chance = has_skill(ch, SKILL_BASH) ? ch->skills[SKILL_BASH]->learned : 0;

        /* I added this to use size instead of blind race -Morg */
        csize = get_char_size(ch);
        tsize = get_char_size(tar_ch);

        chance -= 4 * (tsize - csize);

        /* the above corrects this, if desired, this can be replaced
         * if ((GET_RACE (tar_ch) == RACE_HALF_GIANT) &&
         * (GET_RACE (ch) != RACE_HALF_GIANT))
         * chance -= 40;          */

        /*  Assume no shield */
        chance -= 10;           /* this was -25, but seemed a little 
                                 * unreasonable to have a 
                                 * -25 for not using a shield, 
                                 * +5 for having a shield as primary weapon, 
                                 * -5 for havingg a shield as secondary weapon
                                 * -Tenebrius
                                 */

        if (ch->equipment[ETWO]
            && (ch->equipment[ETWO]->obj_flags.type == ITEM_ARMOR))
            chance += 40;
        else if (ch->equipment[EP]
            && (ch->equipment[EP]->obj_flags.type == ITEM_ARMOR))
            chance += 30;
        else if (ch->equipment[ES]
                 && (ch->equipment[ES]->obj_flags.type == ITEM_ARMOR))
            chance += 20;

        chance += agl_app[GET_AGL(ch)].reaction;

        if (!is_paralyzed(tar_ch))
            chance -= agl_app[GET_AGL(tar_ch)].reaction;

        /*  victim has bash  */
        if (has_skill(tar_ch, SKILL_BASH)) {
            dodge = number(1, tar_ch->skills[SKILL_BASH]->learned);
            if (affected_by_spell(ch, PSI_MINDWIPE))
                dodge -= 25;
            if ((dodge - chance) > 25) {
                act("$N meets your charge and knocks YOU down!", FALSE, ch, 0, tar_ch, TO_CHAR);
                act("$n rams into you, but you knock $m down.", FALSE, ch, 0, tar_ch, TO_VICT);
                act("$n slams into $N but is sent sprawling.", FALSE, ch, 0, tar_ch, TO_ROOM);

                set_char_position(ch, POSITION_SITTING);
                generic_damage(ch, MAX(str_app[GET_STR(ch)].todam, str_app[GET_STR(tar_ch)].todam), 0, 0,
                               MAX(str_app[GET_STR(ch)].todam, str_app[GET_STR(tar_ch)].todam));

                if (!number(0, 4))
                    gain_skill(ch, SKILL_BASH, 2);
                return;
            }
        }

        /* Rolled a successful bash that's higher than opp's bash roll */
        if (chance > number(1, 100) || is_paralyzed(tar_ch)) {
            /* todam {1,9}  */
            if (!IS_NPC(tar_ch) || (IS_NPC(tar_ch)
                                    && !isname("dummy", MSTR(tar_ch, name))))
                set_char_position(tar_ch, POSITION_SITTING);

            ws_damage(ch, tar_ch, MAX(number(1, str_app[GET_STR(ch)].todam), 2),
                    0, 0, MAX(number(1, str_app[GET_STR(ch)].todam), 2), SKILL_BASH, 0);
            WAIT_STATE(tar_ch, PULSE_VIOLENCE * 3);
            return;
        }
    }

    /*  end ch has bash skill  */
    set_char_position(ch, POSITION_SITTING);
    ws_damage(ch, tar_ch, 0, 0, 0, 0, SKILL_BASH, 0);
    if (!number(0, 9))
        gain_skill(ch, SKILL_BASH, 1);
}

/* New riding skill that allows for a rider to use their mount offensively.
 * Using bash as a role model.   -Tiernan 5/24/2003 */
void
skill_charge(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    int chance = 0;
    int random = 0;
    int dodge = 0;
    CHAR_DATA *mount = 0;

    if (IS_CALMED(ch)) {
        send_to_char("You feel too at ease right now to attack anyone.\n\r", ch);
        return;
    }

    /* Get their mount */
    mount = ch->specials.riding;
    if (!mount) {
        send_to_char("You aren't riding an animal.\n\r", ch);
        return;
    }

    if (tar_ch->specials.riding) {
        send_to_char("You can't charge them while they are mounted.\n\r", ch);
        return;
    }

    if ((ch->specials.fighting && tar_ch == ch->specials.fighting)
     || (tar_ch->specials.fighting && tar_ch->specials.fighting == ch)) {
        send_to_char("You can't charge them from this close, disengage and try again.\n\r", ch);
        return;
    }

    /* Can't trample`someone who is flying unless you are flying as well or they can't see you */
    if ((IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
         && !affected_by_spell(tar_ch, SPELL_FEATHER_FALL))
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
        && CAN_SEE(tar_ch, ch)) {
        act("You try, but $E rises overhead and evades you.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to knock you over, but you rise overhead.", FALSE, ch, 0, tar_ch, TO_VICT);
        return;
    }

    /* Adding check for energy shield */
    if (affected_by_spell(tar_ch, SPELL_ENERGY_SHIELD)) {
        act("In a spectacular shower of sparks, your charge at $N is deflected.", FALSE, ch, 0,
            tar_ch, TO_CHAR);
        act("$n tries to topple you, but your energy shield deflects $m.", FALSE, ch, 0, tar_ch,
            TO_VICT);
        act("$n charges at $N, but is deflected in a shower of sparks.", FALSE, ch, 0, tar_ch,
            TO_NOTVICT);
        return;
    }

    if (guard_check(ch, tar_ch))
        return;

    if (ch->in_room != tar_ch->in_room) {
        send_to_char("You don't see that person here.\n\r", ch);
        return;
    }

    if( GET_MOVE(mount) <= 10 ) {
        send_to_char("Your mount is too tired to charge.\n\r", ch);
        return;
    }

    /* Stamina drain. */
    adjust_move(mount, -number(4, 8));

    if (ch->skills[SKILL_CHARGE]) {
        int csize, tsize;

        chance = ch->skills[SKILL_CHARGE]->learned;

        csize = get_char_size(mount);
        tsize = get_char_size(tar_ch);

        chance -= tsize - csize;

        chance += agl_app[GET_AGL(ch)].reaction;
        chance -= agl_app[GET_AGL(tar_ch)].reaction;
       
        if (is_char_actively_watching_char(tar_ch, ch) 
         || is_char_actively_watching_char(tar_ch, mount)) {
           chance -= 20;
        }

        /*  victim has knowledge of how to trample, they can try to
         *  anticipate it and pull off the rider trampling them */
        if (tar_ch->skills[SKILL_CHARGE]) {
            dodge = number(1, tar_ch->skills[SKILL_CHARGE]->learned);
            if (affected_by_spell(ch, PSI_MINDWIPE))
                dodge -= 25;
            if ((dodge - chance) > 25) {
                act("$N avoids your charge and pulls you off of $r!", FALSE, ch, 0, tar_ch,
                    TO_CHAR);
                act("$n charges at you, but you pull $m off of $r instead!", FALSE, ch, 0, tar_ch,
                    TO_VICT);
                act("$n charges at $N but is pulled off of $r instead.", FALSE, ch, 0, tar_ch,
                    TO_ROOM);

                set_char_position(ch, POSITION_SITTING);
                ch->specials.riding = (CHAR_DATA *) 0;
                generic_damage(ch, MAX(str_app[GET_STR(ch)].todam, str_app[GET_STR(tar_ch)].todam), 0, 0,
                               MAX(str_app[GET_STR(ch)].todam, str_app[GET_STR(tar_ch)].todam));

                if (!number(0, 4))
                    gain_skill(ch, SKILL_CHARGE, 2);
                return;
            }
        }

        /* Super easy to charge sleeping losers */
        if (GET_POS(tar_ch) <= POSITION_SLEEPING) {
            dodge = 0;
            random = 0;
        } else {
            random = number(1, 100);
        }

        /* Rolled a successful charge that's higher than opp's charge roll */
        if ((chance > dodge) && (chance > random)) {    
            int dambonus = str_app[GET_STR(mount)].todam 
             * ch->skills[SKILL_CHARGE]->learned / 100;
            int dam = dice(race[(int) GET_RACE(mount)].attack->damdice,
                           race[(int) GET_RACE(mount)].attack->damsize)
                      + str_app[GET_STR(mount)].todam;

            set_char_position(tar_ch, POSITION_SITTING);
            ws_damage(ch, tar_ch, dam + dambonus, 0, 0, dam + dambonus,
             SKILL_CHARGE, 0);
            WAIT_STATE(tar_ch, PULSE_VIOLENCE * 2);
            return;
        }
    }
    /*  end ch has trample skill  */
    ws_damage(ch, tar_ch, 0, 0, 0, 0, SKILL_CHARGE, 0);
    gain_skill(ch, SKILL_CHARGE, 1);

    if (ch->specials.riding) {
        if (number(0, 80) > get_skill_percentage(ch, SKILL_RIDE)) {
            gain_skill(ch, SKILL_RIDE, 1);
            act("You lose your balance and fall off $r!", FALSE, ch, 0, 0, TO_CHAR);
            act("$n loses $s balance and falls off of you.", FALSE, ch, 0, ch->specials.riding,
                TO_VICT);
            act("$n loses $s balance and falls off $r.", FALSE, ch, 0, 0, TO_ROOM);
            set_char_position(ch, POSITION_SITTING);
            ch->specials.riding = (CHAR_DATA *) 0;
            generic_damage(ch, number(1, 10), 0, 0, number(5, 20));
        }
    }
}

/* New riding skill that allows for a rider to use their mount offensively
 * while in combat.
 * Using charge as a role model.   -Morgenes 6/29/2009 */
void
skill_trample(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    int chance = 0;
    int random = 0;
    int dodge = 0;
    int num_fighting = 0;
    CHAR_DATA *mount = 0;
    CHAR_DATA *rch;

    if (IS_CALMED(ch)) {
        send_to_char("You feel too at ease right now to attack anyone.\n\r", ch);
        return;
    }

    /* Get their mount */
    mount = ch->specials.riding;
    if (!mount) {
        send_to_char("You aren't riding an animal.\n\r", ch);
        return;
    }

    if (tar_ch->specials.riding) {
        send_to_char("You can't trample them while they are mounted.\n\r", ch);
        return;
    }

    if (get_char_size(mount) <= get_char_size(tar_ch)) {
        send_to_char("You can't trample something that big.\n\r", ch);
        return;
    }

    if (ch->specials.fighting && tar_ch != ch->specials.fighting) {
        send_to_char("You can only trample the person you are fighting.\n\r", ch);
        return;
    }

    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
       if( rch != tar_ch && rch->specials.fighting == tar_ch ) {
          num_fighting++;
       }
    }

    if (tar_ch->specials.fighting != ch && (!has_skill(ch, SKILL_TRAMPLE) || num_fighting * 20 > ch->skills[SKILL_TRAMPLE]->learned)) {
       send_to_char("You can't maneuver in to trample them.\n\r", ch);
       return;
    }


    /* Can't trample`someone who is flying unless you are flying as well or they can't see you */
    if ((IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
         && !affected_by_spell(tar_ch, SPELL_FEATHER_FALL))
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
        && CAN_SEE(tar_ch, ch)) {
        act("You try, but $E rises overhead and evades you.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to trample you, but you rise overhead.", FALSE, ch, 0, tar_ch, TO_VICT);
        return;
    }

    /* Adding check for energy shield */
    if (affected_by_spell(tar_ch, SPELL_ENERGY_SHIELD)) {
        act("In a spectacular shower of sparks, your trample at $N is deflected.", FALSE, ch, 0,
            tar_ch, TO_CHAR);
        act("$n tries to trample you, but your energy shield deflects $m.", FALSE, ch, 0, tar_ch,
            TO_VICT);
        act("$n tries to trample $N, but is deflected in a shower of sparks.", FALSE, ch, 0, tar_ch,
            TO_NOTVICT);
        return;
    }

    if (guard_check(ch, tar_ch))
        return;

    if (ch->in_room != tar_ch->in_room) {
        send_to_char("You don't see that person here.\n\r", ch);
        return;
    }

    if( GET_MOVE(mount) <= 10 ) {
        send_to_char("Your mount is too tired to trample.\n\r", ch);
        return;
    }

    /* Stamina drain. */
    adjust_move(mount, -number(3, 7));

    if (ch->skills[SKILL_TRAMPLE]) {
        int csize, tsize;

        chance = ch->skills[SKILL_TRAMPLE]->learned;

        csize = get_char_size(mount);
        tsize = get_char_size(tar_ch);

        chance += csize - tsize;

        chance += agl_app[GET_AGL(ch)].reaction;
        chance -= agl_app[GET_AGL(tar_ch)].reaction;

        if( GET_POS(tar_ch) > POSITION_SITTING ) {
           chance -= 20;
        }

        if (is_char_actively_watching_char(tar_ch, ch) 
         || is_char_actively_watching_char(tar_ch, mount)) {
           chance -= 20;
        }

        if (num_fighting > 1)
            chance -= (num_fighting - 1) * 20;

        /*  victim has knowledge of how to trample, they can try to
         *  anticipate it and pull off the rider charging them */
        if (tar_ch->skills[SKILL_TRAMPLE] 
         && GET_POS(tar_ch) >= POSITION_FIGHTING) {
            dodge = number(1, tar_ch->skills[SKILL_TRAMPLE]->learned);
            if (affected_by_spell(ch, PSI_MINDWIPE))
                dodge -= 25;
            if ((dodge - chance) > 25) {
                act("$N avoids your trample and pulls you off of $r!", FALSE, ch, 0, tar_ch,
                    TO_CHAR);
                act("$n trample at you, but you pull $m off of $r instead!", FALSE, ch, 0, tar_ch,
                    TO_VICT);
                act("$n trample at $N but is pulled off of $r instead.", FALSE, ch, 0, tar_ch,
                    TO_ROOM);

                set_char_position(ch, POSITION_SITTING);
                ch->specials.riding = (CHAR_DATA *) 0;
                generic_damage(ch, MAX(str_app[GET_STR(ch)].todam, str_app[GET_STR(tar_ch)].todam), 0, 0,
                               MAX(str_app[GET_STR(ch)].todam, str_app[GET_STR(tar_ch)].todam));

                if (!number(0, 4))
                    gain_skill(ch, SKILL_TRAMPLE, 2);
                return;
            }
        }

        /* Super easy to trample sleeping losers */
        if (GET_POS(tar_ch) <= POSITION_SLEEPING) {
            dodge = 0;
            random = 0;
            chance = 100;
        } else {
            random = number(1, 100);
        }

        /* Rolled a successful trample that's higher than opp's trample roll */
        if ((chance > dodge) && (chance > random)) { 
            int dam = dice(race[(int) GET_RACE(mount)].attack->damdice,
                           race[(int) GET_RACE(mount)].attack->damsize)
                      + str_app[GET_STR(mount)].todam;
            bool alive = TRUE;

            dam = (dam * ch->skills[SKILL_TRAMPLE]->learned / 100);

            if( GET_POS(tar_ch) > POSITION_SITTING ) {
                if( number(1, get_skill_percentage(ch, SKILL_TRAMPLE)/4) > GET_AGL(tar_ch)) {
                   set_char_position(tar_ch, POSITION_SITTING);
                   WAIT_STATE(tar_ch, PULSE_VIOLENCE);
                   alive = ws_damage(ch, tar_ch, dam, 0, 0, dam, SKILL_TRAMPLE, 0 );
                   if( alive ) {
                       act( "$n falls under you.", FALSE, tar_ch, NULL, mount, TO_VICT);
                       act( "$n falls under $N.", FALSE, tar_ch, NULL, mount, TO_ROOM);
                       act( "You fall under $N.", FALSE, tar_ch, NULL, mount, TO_CHAR);
                   }
                } else {
                   dam = MAX(0, dam - number(1, GET_AGL(tar_ch)));
                   alive = ws_damage(ch, tar_ch, dam, 0, 0, dam, SKILL_TRAMPLE, 0 );
                   if( alive && dam > 0 ) {
                       act( "$n avoids falling under you.", FALSE, tar_ch, NULL, mount, TO_VICT);
                       act( "$n avoids falling under $N.", FALSE, tar_ch, NULL, mount, TO_ROOM);
                       act( "You avoid falling under $N.", FALSE, tar_ch, NULL, mount, TO_CHAR);
                   }
                   WAIT_STATE(tar_ch, PULSE_VIOLENCE - 1);
                }
            } else if (GET_POS(tar_ch) == POSITION_SITTING) { // sitting 
               // they were sitting/resting 
               dam = number(dam, dam * 2);

               // tough agil check to negate some of the damage
               if (number(1, get_skill_percentage(ch, SKILL_TRAMPLE)) > 
                GET_AGL(tar_ch) - (5*(POSITION_STANDING - GET_POS(tar_ch)))) {
                  dam = MAX(0, dam - number(1, GET_AGL(tar_ch) / 2));
               }

               ws_damage(ch, tar_ch, dam, 0, 0, dam, SKILL_TRAMPLE, 0 );
            } else { /* sleeping or worse */
               dam *= 2;
               ws_damage(ch, tar_ch, dam, 0, 0, dam, SKILL_TRAMPLE, 0 );
            }

            return;
        }
    }
    /*  end ch has trample skill  */
    ws_damage(ch, tar_ch, 0, 0, 0, 0, SKILL_TRAMPLE, 0);
    gain_skill(ch, SKILL_TRAMPLE, 1);
}

void
skill_guard(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * vict,
            OBJ_DATA * tar_obj)
{
}

void
skill_rescue(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    CHAR_DATA *tmp_ch;
    int percent;
    char from_name[MAX_INPUT_LENGTH];
    char target_name[MAX_INPUT_LENGTH];
    int skill = has_skill(ch, SKILL_RESCUE) ? ch->skills[SKILL_RESCUE]->learned : 0;

    if (ch->specials.fighting == tar_ch || ch->specials.alt_fighting == tar_ch) {
        cprintf(ch, "How can you rescue someone you are trying to kill?\n\r");
        return;
    }

    if (IS_CALMED(ch)) {
        cprintf(ch, "You feel too at ease right now to attack anyone.\n\r");
        return;
    }

    /* pop off the target */
    arg = one_argument(arg, target_name, sizeof(target_name));

    /* grab who they want to rescue from */
    arg = one_argument(arg, from_name, sizeof(from_name));

    if (affected_by_spell(ch, PSI_MINDWIPE))
        skill -= 25;

    /* if you have less than 50% skill or more than 50% but didn't specify who */
    if (skill < 50 || from_name[0] == '\0') {
        for (tmp_ch = ch->in_room->people;
             tmp_ch && tmp_ch->specials.fighting != tar_ch
             && tmp_ch->specials.alt_fighting != tar_ch; tmp_ch = tmp_ch->next_in_room);
    } else if (from_name[0] != '\0') {
        /* look for someone in the room matching the argument */
        tmp_ch = get_char_room_vis(ch, from_name);

        /* noone found */
        if (!tmp_ch) {
            cprintf(ch, "You don't see a '%s' here.\n\r", from_name);
            return;
        }

        /* make sure they're fighting the person */
        if (tmp_ch->specials.fighting != tar_ch && tmp_ch->specials.alt_fighting != tar_ch) {
            cprintf(ch, "But %s isn't fighting %s?", PERS(ch, tmp_ch), HMHR(tar_ch));
            return;
        }
    }

    if (!tmp_ch) {
        act("But nobody is fighting $M?", FALSE, ch, 0, tar_ch, TO_CHAR);
        return;
    }

    percent = number(1, 101);
    if (percent > skill) {
        send_to_char("You fail the rescue.\n\r", ch);
        gain_skill(ch, SKILL_RESCUE, 1);
        act("$n tries unsuccessfully to dart in front of $N.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        return;
    }

    cprintf(ch, "You charge into the fight!\n\r");
    act("$N darts in front of you, and knocks you out of the way!", FALSE, tar_ch, 0, ch, TO_CHAR);
    act("$n rescues $N.", FALSE, ch, 0, tar_ch, TO_NOTVICT);

    sprintf(from_name, "%s rescues %s from %s", GET_NAME(ch), GET_NAME(tar_ch), GET_NAME(tmp_ch));
    send_to_monitors(ch, tar_ch, from_name, MONITOR_FIGHT);

    stop_fighting(tar_ch, tmp_ch);
    stop_fighting(tmp_ch, tar_ch);
    stop_all_fighting(ch);

    set_fighting(ch, tmp_ch);
    set_fighting(tmp_ch, ch);

    WAIT_STATE(tar_ch, number(0, 2));
}

void
skill_peek(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{

}

void
skill_flee(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{
}

void
skill_nesskick(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
               OBJ_DATA * tar_obj)
{
    return;
}

void
skill_kick(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{
    int damage = 0, skill_damage_mod = 0, chance = 0, dodge = 0, csize, tsize = 0, loc =
        0, yes_shield = 0, yes_armor = 0, shield_loc = 0;
    OBJ_DATA *shield = NULL, *armor = NULL;
    /*  char mess[256]; */

    /* Sanity check */
    if ((tar_ch->in_room) != (ch->in_room)) {
        send_to_char("Kick who?\n\r", ch);
        return;
    }

    /* Too tired to do it. */
    /* Raesanos 4/19/2006 */
    if (GET_MOVE(ch) < 10) {
        send_to_char("You are too tired!\n\r", ch);
        return;
    }

    /* Magick Checks */
    if (IS_CALMED(ch)) {
        send_to_char("You feel too at ease right now to attack anyone.\n\r", ch);
        return;
    }

    if (ch->specials.fighting && tar_ch == ch->specials.fighting)
        clear_disengage_request(ch);

    if (is_char_ethereal(ch) != is_char_ethereal(tar_ch)) {
        act("Your kick passes right through $M and you fall over.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n kicks at you, but it passes harmlessly through you.", FALSE, ch, 0, tar_ch,
            TO_VICT);
        act("$n kicks at $N, but ends up on $S behind.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        set_char_position(ch, POSITION_SITTING);
        return;
    }

    /* Only flying people can kick flying people */
    if ((IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
         && !affected_by_spell(tar_ch, SPELL_FEATHER_FALL))
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
        && CAN_SEE(tar_ch, ch)) {
        act("You jump at $M, but $E rises overhead and evades you.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to kick you, but you rise overhead.", FALSE, ch, 0, tar_ch, TO_VICT);
        return;
    }

    /* Riding Checks */
    if (ch->specials.riding) {
        send_to_char("You can't possibly do this while mounted.\n\r", ch);
        return;
    }

    if (tar_ch->specials.riding) {
        send_to_char("You can't kick that high, they are mounted.\n\r", ch);
        return;
    }

    /* Stamina drain. */
    adjust_move(ch, -number(2, (100 - get_skill_percentage(ch, SKILL_KICK)) / 22 + 3));

    /* Guard check */

    if (guard_check(ch, tar_ch))
        return;


    /* I added this to use size instead of blind race 
     * -Nessalin (copying Morg's code in bash) */
    csize = get_char_size(ch);
    tsize = get_char_size(tar_ch);


    //  sprintf (mess, "Current chance: %d.", chance); 
    //  gamelog (mess);
    //  sprintf (mess, "Char size is %d, Target size is %d.", csize, tsize);
    //  gamelog (mess);

    chance -= 4 * (tsize - csize);

    //  sprintf (mess, "Current chance: %d.", chance); 
    //  gamelog (mess);


    /* Adding in checks for armor, shield use and natural armor */
    /* -Nessalin 6/16/2001                                      */

    if (!(loc = get_random_hit_loc(tar_ch, SKILL_KICK)))
        loc = WEAR_BODY;

    shield_loc = get_shield_loc(tar_ch);
    /* Looking for shields */
    if (shield_loc != -1) {
        shield = tar_ch->equipment[shield_loc];
        int chance_to_block = get_skill_percentage(tar_ch, SKILL_SHIELD);

        switch( shield_loc ) {
            case ETWO:
               chance_to_block += 10;
               break;
            case EP:
               chance_to_block += 5;
               break;
            default:
               chance_to_block += 0;
               break;
        }

        // if target can't see kicker
        if( !CAN_SEE(tar_ch, ch) ) {
           // if they can't blind fight, or fail a skill check
           if( !has_skill(tar_ch, SKILL_BLIND_FIGHT)
            || number(0, 100) > get_skill_percentage(tar_ch, SKILL_BLIND_FIGHT)) {
               chance_to_block = 0;
               if( has_skill(tar_ch, SKILL_BLIND_FIGHT) ) {
                   gain_skill(tar_ch, SKILL_BLIND_FIGHT, 1);
               }
           } 
        } else if( is_char_watching_char( tar_ch, ch )
          || tar_ch->specials.fighting == ch ) {
            chance_to_block += 10;
        }

        chance_to_block /= MAX(1, count_opponents(tar_ch));

        if (shield && number(0, 100) < chance_to_block) {
            yes_shield = 1;
        } else if( has_skill(tar_ch, SKILL_SHIELD ) ) {
           gain_skill(tar_ch, SKILL_SHIELD, 1 );
        }
    }

    else if ((armor = tar_ch->equipment[loc]) != NULL)
        if (armor->obj_flags.type == ITEM_ARMOR)
            yes_armor = 1;

    /* You either need the skill, or your opponent must be sleeping */
    if (ch->skills[SKILL_KICK] || !AWAKE(tar_ch) || is_paralyzed(tar_ch)) {

        /* concious opponents get a chance to dodge */
        if (AWAKE(tar_ch) && !is_paralyzed(tar_ch)) {
            dodge = agl_app[GET_AGL(tar_ch)].reaction;
            if (tar_ch->skills[SKILL_KICK])
                dodge += number(1, tar_ch->skills[SKILL_KICK]->learned);
        } else
            dodge = 0;

        chance = has_skill(ch, SKILL_KICK) ? ch->skills[SKILL_KICK]->learned : 1;
        chance += agl_app[GET_AGL(ch)].reaction;

        if (tar_ch->skills[SKILL_KICK] && AWAKE(tar_ch)
            && !is_paralyzed(tar_ch)) {
            if ((dodge - chance) > 25) {
                if ((get_char_size(ch) - get_char_size(tar_ch)) > 4) {
                    act("$N ducks under your kick and trips you!", FALSE, ch, 0, tar_ch, TO_CHAR);
                    act("$n kicks at you, but you duck it and trip $m.", FALSE, ch, 0, tar_ch,
                        TO_VICT);
                    act("$N ducks a slow kick from $n and trips $m.", FALSE, ch, 0, tar_ch,
                        TO_NOTVICT);
                } else {
                    act("$N grabs your leg and knocks you down!", FALSE, ch, 0, tar_ch, TO_CHAR);
                    act("$n kicks at you, but you grabs $s leg and drop $m.", FALSE, ch, 0, tar_ch,
                        TO_VICT);
                    act("$n's kick is caught by $N, and $e is knocked over.", FALSE, ch, 0, tar_ch,
                        TO_NOTVICT);
                }

                set_char_position(ch, POSITION_SITTING);
                if (!number(0, 4))
                    gain_skill(ch, SKILL_KICK, 2);
                return;
            }                   /*  end vict gets better roll  */
        }

        /*  end vict has kick & is awake */
        if (chance > dodge) {
            if ((number(1, 100) <= chance) || !AWAKE(tar_ch)
                || is_paralyzed(tar_ch)) {
                /* As of coding this only templars & warriors get kick, and
                 * they both max out at 80.  That's why there's a sudden jump
                 * after 80 - so we can reserve skills that high for quests
                 * or special characters.  -Nessalin 10-23-1999 */
                if (ch->skills[SKILL_KICK]) {
                    if (ch->skills[SKILL_KICK]->learned < 30)
                        skill_damage_mod = number(1, 8);
                    else if (ch->skills[SKILL_KICK]->learned < 40)
                        skill_damage_mod = number(2, 10);
                    else if (ch->skills[SKILL_KICK]->learned < 50)
                        skill_damage_mod = number(3, 15);
                    else if (ch->skills[SKILL_KICK]->learned < 60)
                        skill_damage_mod = number(5, 15);
                    else if (ch->skills[SKILL_KICK]->learned < 70)
                        skill_damage_mod = number(6, 18);
                    else if (ch->skills[SKILL_KICK]->learned < 81)
                        skill_damage_mod = number(9, 21);
                    else        /* 81 or higher */
                        skill_damage_mod = number(10, 25);
                } else
                    skill_damage_mod = number(1, 2);

                //         sprintf(mess, "Raw damage based on skill: %d", skill_damage_mod);
                //         gamelog (mess);


                damage = number(1, str_app[GET_STR(ch)].todam) + skill_damage_mod;

                //         sprintf(mess, "Damage with STR bonus: %d", damage);
                //         gamelog (mess);

                //         sprintf(mess, "Natural armor: %d", tar_ch->abilities.armor);
                //         gamelog (mess);

                //         sprintf(mess, "Natural armor modifier: %d", (tar_ch->abilities.armor) * (2/3));
                //         gamelog (mess);


                damage -= tar_ch->abilities.armor;

                //         sprintf(mess, "Damage with nat armor minus: %d", damage);
                //         gamelog (mess);


                if ((damage < 1) && (tar_ch->abilities.armor > 0)) {
                    if (damage < 0) {
                        //               gamelog ("Kick damage is less than 0, changing to 0.");
                        damage = 0;
                    }
                    act("$n's kick bounces off $N's tough skin.", FALSE, ch, armor, tar_ch,
                        TO_NOTVICT);
                    act("$n's kick at you bounces off your tough skin.", FALSE, ch, armor, tar_ch,
                        TO_VICT);
                    act("Your kick at $N bounces off $S tough skin.", FALSE, ch, armor, tar_ch,
                        TO_CHAR);
                    if (!(ch->specials.fighting))
                        hit(ch, tar_ch, 0);
                    return;
                }
                // Changing to full armor value -Nessalin 7/4/2003
                //              damage -= tar_ch->abilities.armor / 10;

                if (yes_shield) {
                    int orig_dam = damage;
                    damage -= absorbtion(shield->obj_flags.value[0]);
                    damage = MAX(0, damage);

//                sprintf(mess, "Damage lessed by shield: %d", damage);
//                gamelog (mess);


                    if (damage < 1) {
                        act("$N blocks $n's kick with $p.", FALSE, ch, shield, tar_ch, TO_NOTVICT);
                        act("You block $n's kick with $p.", FALSE, ch, shield, tar_ch, TO_VICT);
                        act("$N blocks your kick with $p.", FALSE, ch, shield, tar_ch, TO_CHAR);
                        if (!(ch->specials.fighting))
                            hit(ch, tar_ch, 0);
                        return;
                    } else {
                        act("$N partially blocks $n's kick with $p.", FALSE, ch, shield, tar_ch,
                            TO_NOTVICT);
                        act("You partially block $n's kick with $p.", FALSE, ch, shield, tar_ch,
                            TO_VICT);
                        act("$N partially blocks your kick with $p.", FALSE, ch, shield, tar_ch,
                            TO_CHAR);
                    }
                    // damage armor after giving messages
                    damage_armor(shield, orig_dam, SKILL_KICK);
                }
                if (yes_armor) {
                    // not sure why we're not using absorbtion here???
                    damage -= armor->obj_flags.value[0];
                    damage = MAX(0, damage);
//                sprintf(mess, "Damage lessed by worn armor: %d", damage);
//                gamelog (mess);


                    if (damage < 1) {
                        act("$n's kick at $N is absorbed by $S $o.", FALSE, ch, armor, tar_ch,
                            TO_NOTVICT);
                        act("$n's kick at you is absorbed by your $o.", FALSE, ch, armor, tar_ch,
                            TO_VICT);
                        act("Your kick at $N is absorbed by $S $o.", FALSE, ch, armor, tar_ch,
                            TO_CHAR);
                        if (!(ch->specials.fighting))
                            hit(ch, tar_ch, 0);
                        return;
                    } else {
                        act("$n's kick at $N is partially absorbed by $S $o.", FALSE, ch, armor,
                            tar_ch, TO_NOTVICT);
                        act("$n's kick at you is partially absorbed by your $o.", FALSE, ch, armor,
                            tar_ch, TO_VICT);
                        act("Your kick at $N is partially absorbed by $S $o.", FALSE, ch, armor,
                            tar_ch, TO_CHAR);
                    }
                    armor = damage_armor(armor, damage, SKILL_KICK);
                }
                //                 sprintf(mess, "Damage applied: %d", damage);
                //                 gamelog (mess);

                if (damage < 0) {
//                gamelog ("Kick damage is less than 0, changing to 0.");
                    damage = 0;
                }

                ws_damage(ch, tar_ch, damage, 0, 0, damage, SKILL_KICK, 0);
                return;
            }
        }
    }

    /* If target is sleeping, do some damage even if you fail */
    /* -Nessalin 6/16/2001                                    */
    if (!AWAKE(tar_ch) || is_paralyzed(tar_ch))
        ws_damage(ch, tar_ch, number(1, 2), 0, 0, number(2,3), SKILL_KICK, 0);
    else
        ws_damage(ch, tar_ch, 0, 0, 0, 0, SKILL_KICK, 0);

    /* if your target is asleep or paralyzed, decrease chance of learning */
    if ((!AWAKE(tar_ch) || is_paralyzed(tar_ch)) && !number(0, 11)) {
        /* can only learn up to 25% on sparring dummies */
        if (has_skill(ch, SKILL_KICK) && ch->skills[SKILL_KICK]->learned <= 25)
            gain_skill(ch, SKILL_KICK, 1);
    } else if (!number(0, 9)) {
        gain_skill(ch, SKILL_KICK, 1);
    }

}

void
skill_split_opponents(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
                      OBJ_DATA * tar_obj)
{
    int chance = 0;
    int dodge = 0;
    int old_def;
    char buf[MAX_STRING_LENGTH];

#define USE_SPLIT_OPPONENTS
#ifndef USE_SPLIT_OPPONENTS
    /* Don't use this skill yet */
    send_to_char("Under development.\n\r", ch);
    ch->specials.act_wait = 0;
    return;
#endif

    /* TBD - Tiernan 8/6/2003 */
    if (!ch->specials.fighting) {
        send_to_char("But you are not fighting anyone.\n\r", ch);
        return;
    }
    if (ch->specials.riding) {
        send_to_char("That would be hard while riding.\n\r", ch);
        return;
    }
    if (!ch->equipment[EP] || !ch->equipment[ES]) {
        send_to_char("You can't do that without wielding two weapons.\n\r", ch);
        return;
    }
    /* Magick Checks */
    if (IS_CALMED(ch)) {
        send_to_char("You feel too at ease right now to attack anyone.\n\r", ch);
        return;
    }
    /* Sanity check */
    if ((tar_ch->in_room) != (ch->in_room)) {
        send_to_char("Fight who?\n\r", ch);
        return;
    }
    /* Are they attacking us? */
    if (tar_ch->specials.fighting != ch) {
        send_to_char("But they are not fighting you.\n\r", ch);
        return;
    }

    if (ch->specials.fighting == tar_ch) {
        if (ch->specials.alt_fighting) {
            send_to_char("You refocus your effort on your main opponent.\n\r", ch);
            ch->specials.alt_fighting = 0;
        } else {
            send_to_char("Who else do you want to focus your effort on?\n\r", ch);
        }
        return;
    }

    /* Are we already fighting them? */
    if (ch->specials.alt_fighting == tar_ch) {
        send_to_char("But you are already fighting them.\n\r", ch);
        return;
    }
    if (ch->specials.alt_fighting) {
        send_to_char("To fight more than two would be pure chaos.\n\r", ch);
        return;
    }

    if (ch->skills[SKILL_SPLIT_OPPONENTS]) {
        dodge = agl_app[GET_AGL(tar_ch)].reaction;
        if (tar_ch->skills[SKILL_SPLIT_OPPONENTS])
            dodge += number(1, tar_ch->skills[SKILL_SPLIT_OPPONENTS]->learned);
        else
            dodge = 0;

        chance = ch->skills[SKILL_SPLIT_OPPONENTS] ? ch->skills[SKILL_SPLIT_OPPONENTS]->learned : 1;
        chance += agl_app[GET_AGL(ch)].reaction;

        if ((dodge - chance) > 25) {
            act("$N deftly avoids your attempt to strike $M.", FALSE, ch, 0, tar_ch, TO_CHAR);
            act("$n attempts to attack you, but you deftly avoid $m.", FALSE, ch, 0, tar_ch,
                TO_VICT);
            act("$n attempts to attack $N, but $E deftly avoids $m.", FALSE, ch, 0, tar_ch,
                TO_ROOM);

            old_def = (int) ch->abilities.def;
            ch->abilities.def -= 25;
            ch->abilities.def = (sbyte) MAX(0, ch->abilities.def - 25);

            if (!hit(tar_ch, ch, TYPE_UNDEFINED)) {     /* they bees dying, deadlike */
                return;
            }
            if (ch)
                ch->abilities.def = (sbyte) old_def;
            if (!number(0, 4))
                gain_skill(ch, SKILL_SPLIT_OPPONENTS, 2);
            return;
        }
        /*  end vict gets better roll  */
        if ((chance > dodge) && chance >= (number(1, 100))) {
            act("You divide your attention, focussing on $N.", FALSE, ch, 0, tar_ch, TO_CHAR);
            sprintf(buf, "%s successfully splits %s opponents.\n\r", MSTR(ch, name), HSHR(ch));
            send_to_monitor(ch, buf, MONITOR_FIGHT);
            ch->specials.alt_fighting = tar_ch;
        } else {
            send_to_char("You fail to separate your opponents.\n\r", ch);
            if (!number(0, 4))
                gain_skill(ch, SKILL_SPLIT_OPPONENTS, 1);
        }
    } else
        send_to_char("You fail to separate your opponents.\n\r", ch);
}

int
succeed_sneak(CHAR_DATA * ch, CHAR_DATA * tch, int roll)
{
    int j, enc, percent = 0;
    int base_percent = 0;
    OBJ_DATA *obj;

    // Can never sneak up on an immortal
    if (IS_IMMORTAL(tch))
        return 0;

    // no sneaking while riding 
    if (ch->specials.riding)
        return 0;

    if (ch->lifting)
        return 0;

    // You must be ethereal, or see ethereal, to catch sneaking ethereals
    if (is_char_ethereal(ch) && !is_char_ethereal(tch) && !IS_AFFECTED(tch, CHAR_AFF_DETECT_ETHEREAL))
        return 1;

    // give the stealth manip if they have the sneak skill
    if (has_skill(ch, SKILL_SNEAK))
        percent = get_skill_percentage(ch, SKILL_SNEAK) + agl_app[GET_AGL(ch)].stealth_manip;
    else
        percent = agl_app[GET_AGL(ch)].stealth_manip;

    // make the penalty for not sneaking where you should be like hide.
    if (!IS_IMMORTAL(ch) && !SNEAK_OK(ch))
        percent /= 5;

    // If not sneaking, no benefit from sneak skill or agility
    if (GET_SPEED(ch) != SPEED_SNEAKING && !IS_AFFECTED(ch, CHAR_AFF_HIDE))
        percent = 0;

    // standard mindwipe penalty
    if (affected_by_spell(ch, PSI_MINDWIPE))
        percent -= 25;

    // look for armor
    for (j = 0; j < MAX_WEAR; j++) {
        if (j == WEAR_BODY || j == WEAR_FEET || j == WEAR_LEGS || j == WEAR_ARMS) {
            obj = ch->equipment[j];

            // if they're wearing armor
            if (obj && (obj->obj_flags.type == ITEM_ARMOR)) {

                // based on material type, apply a penalty
                switch (obj->obj_flags.material) {
                    // 'Hard' materials
                case MATERIAL_ISILT:
                case MATERIAL_TORTOISESHELL:
                case MATERIAL_GRAFT:
                case MATERIAL_HORN:
                case MATERIAL_CERAMIC:
                case MATERIAL_BONE:
                case MATERIAL_WOOD:
                case MATERIAL_OBSIDIAN:
                case MATERIAL_STONE:
                case MATERIAL_CHITIN:
                case MATERIAL_METAL:
                    /* loose sneak based on how much armor it provides */
                    percent -= obj->obj_flags.value[3];

                    /* if it's damaged, make it even more so */
                    percent -= obj->obj_flags.value[1] - obj->obj_flags.value[0];
                    break;
                    // 'Soft' materials
                case MATERIAL_NONE:
                case MATERIAL_CLOTH:
                case MATERIAL_LEATHER:
                case MATERIAL_SILK:
                case MATERIAL_SKIN:
                    // 1/5 the maximum armor value is applied as a penalty
                    percent -= obj->obj_flags.value[3] / 5;
                    break;
                    // otherwise...stuff like fungus...fungus armor? ewww...
                default:
                    percent -= obj->obj_flags.value[3];
                    break;
                }
            }
        }
    }

    // encumberance penalty
    enc = get_encumberance(ch);
    if( enc > 3 ) { /* more than easily manageable */
        percent -= (enc - 3) * 10;
    }

    // Bulky items impede chance of sneaking
    percent -= get_char_bulk_penalty(ch);

    // Take off for the victim's wisdom perception
    percent -= wis_app[GET_WIS(tch)].percep;

    // If they're somehow actively watching the person
    if (is_char_actively_watching_char(tch, ch))
        percent -= get_watched_penalty(tch);

    // bonus to hide if they're in a populated room
    if (CITY_SNEAKER(ch)
        && IS_SET(tch->in_room->room_flags, RFL_POPULATED)) {
        percent += 25;
    }
    // Room is not populated
    else if (!IS_SET(tch->in_room->room_flags, RFL_POPULATED)) {
        // If the target is actively listening, and not in a populated room
        if (IS_AFFECTED(tch, CHAR_AFF_LISTEN) 
            && has_skill(tch, SKILL_LISTEN)) {
            if (number(1, tch->skills[SKILL_LISTEN]->learned) > number(1, percent)) {
                percent -= 25;
            } else {
                gain_skill(tch, SKILL_LISTEN, 1);
            }
        }
    }

    // If they're hidden
    if (IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
        // Speed penalties only applied if ch isn't following tch
        if (ch->master != tch) {
            // hard to run & stay hidden while following
            if (GET_SPEED(ch) == SPEED_RUNNING)
                percent -= 25;
        }
    } else if (GET_SPEED(ch) != SPEED_SNEAKING && CAN_SEE(tch, ch)) {
        // if you're not hidden and not sneaking, you cannot sneak up on people who can see you
        return 0;
    }

    base_percent = MAX(1, percent);

    //  Hard to sneak up on someone you don't see
    if (!CAN_SEE(ch, tch))
        percent -= 20;

    // Take off for the sense presence
    if (affected_by_spell(tch, PSI_SENSE_PRESENCE)
        && has_skill(tch, PSI_SENSE_PRESENCE)
        && can_use_psionics(ch)) {
        percent -= tch->skills[PSI_SENSE_PRESENCE]->learned;
    }
    // easier if they can't see you
    if (!CAN_SEE(tch, ch))
        percent += 40;

    // always 1% chance of success
    percent = MAX(1, percent);

    // NPCs set with the sneak affect almost always succeed.
    if (IS_AFFECTED(ch, CHAR_AFF_SNEAK))
        percent = 101;

    // if over 100% chance
    if (percent > 100) {
        // make it a 1 in 500 chance
        if (!number(0, 499)) {
            // if before vision/sense presence checks, you would have failed
            if (base_percent <= roll)
                // remove hide
                REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
            roomlogf(ch->in_room, "1/500 sneak failure!  BAD LUCK");
            return 0;
        }
        // otherwise success
        roomlogf(ch->in_room, "sneak success");
        return 1;
    }

    // do the skill check
    roomlogf(ch->in_room, "percent: %d, roll: %d = %s %s them", percent, roll,
             MSTR(tch, name), percent <= roll ? "saw" : "did not see");
    if (percent > roll)
        return 1;

    /* A lot of sneak checks happen, so only a 5% chance per check of attempting to gain. */
    if (!number(0,19) && SNEAK_OK(ch) && has_skill(ch, SKILL_SNEAK))
        gain_skill(ch, SKILL_SNEAK, number(1, 2));

    // failed the check, remove the hide affect 
    // if before vision/sense presence checks, you would have failed
    if (base_percent <= roll && IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
        roomlogf(ch->in_room, "base_percent: %d, roll: %d = %s", base_percent, roll, base_percent <= roll ? "removed hide" : "remain hidden");
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE); // remove hide
    }
    return 0;
}


void
skill_hide(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{
    byte roll = 0;
    int success_chance = 0;
    int j;
    char buf[256];
    CHAR_DATA *rch;

    /* Must be standing to hide --Morg 9/11/02 */
    if (GET_POS(ch) != POSITION_STANDING) {
        send_to_char("You must be standing to hide!\n\r", ch);
        return;
    }

    if (ch->specials.riding) {
        send_to_char("That would be difficult while mounted.\n\r", ch);
        return;
    }

    for (j = 0; j < MAX_WEAR; j++) {
        if ((j == WEAR_BODY) || (j == WEAR_ARMS) || (j == WEAR_LEGS)
            || (j == WEAR_FEET))
            if (ch->equipment[j]
                && (ch->equipment[j]->obj_flags.type == ITEM_ARMOR)
                && (ch->equipment[j]->obj_flags.material == MATERIAL_METAL)) {
                sprintf(buf, "That would be difficult wearing %s.\n\r",
                        OSTR(ch->equipment[j], short_descr));
                send_to_char(buf, ch);
                return;
            }
    }

    /* Everyone has an 8% chance */
    if (ch->skills[SKILL_HIDE] && (roll < ch->skills[SKILL_HIDE]->learned))
        success_chance = (ch->skills[SKILL_HIDE]->learned);
    else
        success_chance = 8;

    /* check to see if they're in their element, if not... */
    if (!HIDE_OK(ch)) {
        /* if you're a wild hider, let them know */
        if (WILD_HIDER(ch) && !HIDE_WILD_AREA(ch->in_room)) {
            send_to_char("It's not the wilderness, but you try anyway.\n\r", ch);
        } else if (CITY_HIDER(ch) && !HIDE_CITY_AREA(ch->in_room)) {
            send_to_char("It's not the city, but you try anyway.\n\r", ch);
        }
        success_chance /= 5;
    } else {
        if (WILD_HIDER(ch)) {
            /* Area should only be sparse OR heavy, don't let both add up */
            if (IS_SET(ch->in_room->room_flags, RFL_PLANT_SPARSE)) {
                send_to_char("You use foliage in the area to hide.\n\r", ch);
                success_chance += number(1, 5);
            } else if (IS_SET(ch->in_room->room_flags, RFL_PLANT_HEAVY)) {
                send_to_char("You use foliage in the area to hide.\n\r", ch);
                success_chance += number(6, 10);
            }
        }

        if (CITY_HIDER(ch)) {
            if (IS_SET(ch->in_room->room_flags, RFL_POPULATED)) {
                send_to_char("You try and blend into the crowd.\n\r", ch);
                success_chance += 25;
            }
        }
    }

    if (affected_by_spell(ch, PSI_MINDWIPE))
        success_chance -= 25;

    send_to_char("You attempt to hide yourself.\n\r", ch);

    roll += number(1, 101);

    /* sneak changed in 98 to a speed mode, updating it here in 2006 -Morg */
    if (GET_SPEED(ch) == SPEED_SNEAKING)
        success_chance += 15;

    if (IS_AFFECTED(ch, CHAR_AFF_HIDE))
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);

    if (roll > success_chance) {
        gain_skill(ch, SKILL_HIDE, number(1, 3));

        for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
            if (rch != ch && watch_success(rch, ch, NULL, roll - success_chance, SKILL_HIDE)) {
                cprintf(rch, "You notice %s trying to slip into the shadows.\n\r", PERS(rch, ch));
            }
        }
        return;
    }

    /* allow watchers a chance to still keep watching them */
    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if (is_char_actively_watching_char(rch, ch)) {
            int penalty = roll - (success_chance - get_watched_penalty(rch));

            if (!watch_success(rch, ch, NULL, penalty, SKILL_HIDE)) {
                cprintf(rch, "%s slips out of your sight.\n\r", capitalize(PERS(rch, ch)));
                stop_watching(rch, FALSE);
            } else {
                cprintf(rch, "You notice %s trying to slip into the shadows.\n\r", PERS(rch, ch));
            }
        }
    }

    for (rch = character_list; rch; rch = rch->next) {
        if (is_char_watching_char(rch, ch) && rch->in_room != ch->in_room) {
            stop_watching_raw(rch);
        }
    }

    MUD_SET_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
}

void
check_theft_flag(CHAR_DATA * ch, CHAR_DATA * victim)
{                               /* Start Check_Theft_Flag */
    int ct, dir;
    CHAR_DATA *tch = 0;
    struct room_data *rm;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!(ct = room_in_city(ch->in_room))
        || (victim && IS_SET(victim->specials.act, city[ct].c_flag))
        || (victim && IS_SET(victim->specials.act, CFL_AGGRESSIVE))
        || IS_SET(ch->specials.act, city[ct].c_flag)
        || IS_TRIBE(ch, city[ct].tribe))
        return;

    if (victim) {
        if (ct == CITY_CAI_SHYZN) {
            if (GET_RACE(victim) != RACE_MANTIS)
                return;
        } else if (ct == CITY_BW) {
            if (GET_RACE(victim) != RACE_ELF)
                return;
        } else if (GET_RACE_TYPE(victim) != RTYPE_HUMANOID)
            return;
    }

    if (!IS_SET(ch->in_room->room_flags, RFL_POPULATED)
     || room_visibility(ch->in_room) < 0
     || (IS_SET(ch->in_room->room_flags, RFL_POPULATED)
      && !IS_SET(ch->in_room->room_flags, RFL_INDOORS)
      && (time_info.hours == NIGHT_EARLY || time_info.hours == NIGHT_LATE))) {
        for (tch = ch->in_room->people; tch; tch = tch->next_in_room)
            if (IS_TRIBE(tch, city[ct].tribe) && IS_NPC(tch) && CAN_SEE(tch, ch))
                break;
        for (dir = 0; (dir < 6) && !tch; dir++)
            if (ch->in_room->direction[dir]
                && (rm = ch->in_room->direction[dir]->to_room))
                for (tch = rm->people; tch; tch = tch->next_in_room)
                    if (IS_TRIBE(tch, city[ct].tribe) && IS_NPC(tch) && CAN_SEE(tch, ch))
                        break;
        if (!tch)
            return;
    } else if(IS_SET(ch->specials.affected_by, CHAR_AFF_HIDE)) {
        // room is populated, but the player was hidden
        return;
    }

    /* some people just don't care... */
    if (number(1, 100) < 30) {
        if (tch) {
            if (tch->in_room == ch->in_room)
                act("$N watches you closely, then shrugs $S " "shoulders.", FALSE, ch, 0, tch,
                    TO_CHAR);
            else
                act("$N observes you from afar, but does nothing.", FALSE, ch, 0, tch, TO_CHAR);
        } else
            send_to_char("Several sets of eyes notice your " "behavior, but then turn away.\n\r",
                         ch);
        drop_clan_status(GET_TRIBE(ch), 10);
        return;
    }

    if (city[ct].c_flag) {
        af.type = TYPE_CRIMINAL;
        af.location = CHAR_APPLY_CFLAGS;
        af.duration = number(1, 10);
        af.modifier = city[ct].c_flag;
        af.bitvector = 0;
        af.power = CRIME_THEFT;
        affect_to_char(ch, &af);
    }

    drop_clan_status(GET_TRIBE(ch), 20);
}


/* calculate a skill for unlatching the specified obj from the specified victim */
bool
get_sleight_of_hand_skill(CHAR_DATA * ch, OBJ_DATA * obj, CHAR_DATA * victim)
{
    int skill;

    /* 101% for imms baby! */
    if (IS_IMMORTAL(ch)) {
        send_to_char("Immortals always succeed.\n\r", ch);
        return 101;
    }

    /* get a SoH skill if they have it, otherwise a number from 1 to agil */
    skill = has_skill(ch, SKILL_SLEIGHT_OF_HAND) ? ch->skills[SKILL_SLEIGHT_OF_HAND]->learned : number(1, GET_AGL(ch));

    /* if we're unlatching a container on someone */
    if (victim) {
        /* no chance of affecting a merchant */
        if (is_merchant(victim))
            return 0;

        /* if you're subdued, always succeed */
        if (is_paralyzed(victim) || IS_AFFECTED(victim, CHAR_AFF_SUBDUED)) {
            skill = 100;
        }
        /* if they're not awake */
        else if (!AWAKE(victim)) {
            /* If seriously unconscious (stun or hp loss) */
            if (knocked_out(victim) || GET_POS(victim) < POSITION_SLEEPING) {
                /* automatic start at 100% skill */
                skill = 100;
            } else {
                /* 30% bonus for them being asleep */
                skill += 30;
            }
        } else {                /* they are awake */
            /* bonus if they are awake and can't see you */
            if (!CAN_SEE(victim, ch))
                skill += 25;

            /* if the victim is watching the character give a penalty
             * Morg - 3/18/2006
             */
            if (is_char_actively_watching_char(victim, ch))
                skill -= get_watched_penalty(victim);

            /* penalty if victim is sitting or resting */
            skill -= (5 * (POSITION_STANDING - GET_POS(victim)));

            if (IS_AFFECTED(victim, CHAR_AFF_DEAFNESS))
                skill += 15;
        }
    }

    /* mindwiped by a psionicist */
    if (affected_by_spell(ch, PSI_MINDWIPE))
        skill -= 25;

    /* Hallucinatory drug that affects the brain */
    if (affected_by_spell(ch, POISON_SKELLEBAIN))
        skill -= 40;

    /* Hallucinatory drug that affects the brain phase 2*/
    if (affected_by_spell(ch, POISON_SKELLEBAIN_2))
        skill -= 50;

    /* Hallucinatory drug that affects the brain phase 3*/
    if (affected_by_spell(ch, POISON_SKELLEBAIN_3))
        skill -= 60;

    /* bonus if there are crowds */
    if (IS_SET(ch->in_room->room_flags, RFL_POPULATED))
        skill += 30;

    /* but if it's too enclosed of a space...penalty */
    if (IS_SET(ch->in_room->room_flags, RFL_INDOORS))
        skill = (skill * 3 / 4);

    /* always a 1% chance */
    skill = MAX(skill, 1);

    /* no more than a 100% chance */
    skill = MIN(skill, 100);

    /* give the skill % back for comparison */
    return (skill);
}

/* calculate a skill for stealing the specified obj from the specified victim */
bool
get_steal_skill(CHAR_DATA * ch, OBJ_DATA * obj, CHAR_DATA * victim)
{
    int skill;
    int loc;
    int locmod = 0;

    /* 101% for imms baby! */
    if (IS_IMMORTAL(ch)) {
        send_to_char("Immortals always succeed.\n\r", ch);
        return 101;
    }

    /* get a steal skill if they have it, otherwise a number from 1 to agil */
    skill = has_skill(ch, SKILL_STEAL) ? ch->skills[SKILL_STEAL]->learned : number(1, GET_AGL(ch));

    /* if we're stealing from someone */
    if (victim) {
        /* no chance of stealing from a merchant */
        if (is_merchant(victim))
            return 0;

        /* if you're subdued, always succeed */
        if (is_paralyzed(victim) || IS_AFFECTED(victim, CHAR_AFF_SUBDUED)) {
            skill = 100;
            //gamelogf( "steal target is incapacitated: %d", skill );
        }
        /* if they're not awake */
        else if (!AWAKE(victim)) {
            /* If seriously unconscious (stun or hp loss) */
            if (knocked_out(victim) || GET_POS(victim) < POSITION_SLEEPING) {
                /* automatic start at 100% skill */
                skill = 100;
                //gamelogf( "steal target is unconscious: %d", skill );
            } else {
                /* 30% bonus for them being asleep */
                skill += 30;
                //gamelogf( "steal target is asleep: %d", skill );
            }
        } else {                /* they are awake */
            /* bonus if they are awake and can't see you */
            if (!CAN_SEE(victim, ch))
                skill += 25;

            /* if the victim is watching the character give a penalty
             * Morg - 3/18/2006
             */
            if (is_char_actively_watching_char(victim, ch))
                skill -= get_watched_penalty(victim);

            /* penalty if victim is sitting or resting */
            skill -= (5 * (POSITION_STANDING - GET_POS(victim)));

            if (IS_AFFECTED(victim, CHAR_AFF_DEAFNESS))
                skill += 15;
        }

        /* this really should be reworked, as it is potentially crippling */
        if (IS_SET(victim->specials.nosave, NOSAVE_THEFT))
            return 101;
    }

    /* if we are stealing an object */
    if (obj) {
        /* get the weight of the object for future calculations */
        int weight = GET_OBJ_WEIGHT(obj);

        /* can't steal something weighing more than 22!!!! lbs from a vict */
        if ((victim && AWAKE(victim)) && weight > 10) {
            return 0;
        }

        /* this really should be looked at, 3 stones is 6.6 lbs
         * we have the ability to look at factional weight, this
         * should be more like 10% per half stone above 1.
         * -Morg 3/18/06 
         *
         * Leaving at 3 for now due to the fact that currently objects
         * are overly weighted. -Morg 4/12/06
         */
        skill -= (weight - 3) * 15;
        //gamelogf( "after weight modifier: %d", skill );

        /* if it's in a container */
        if (obj->in_obj) {
            if (victim && obj->in_obj->equipped_by == victim) {
                locmod = 10;
            }
        } else if (victim && obj->equipped_by == victim) {
            /* this is really for people who are asleep */
            for (loc = 0; loc < MAX_WEAR; loc++)
                if (victim->equipment[loc] == obj)
                    break;

            switch (loc) {
            case WEAR_ON_BELT_1:
            case WEAR_ON_BELT_2:
            case WEAR_IN_HAIR:
                /* easy no mod */
                break;
            case WEAR_FINGER_R:
            case WEAR_FINGER_L:
            case WEAR_FINGER_R2:
            case WEAR_FINGER_L2:
            case WEAR_FINGER_R3:
            case WEAR_FINGER_L3:
            case WEAR_FINGER_R4:
            case WEAR_FINGER_L4:
            case WEAR_FINGER_R5:
            case WEAR_FINGER_L5:
            case WEAR_NECK:
            case WEAR_ABOUT_THROAT:
            case WEAR_ANKLE:
            case WEAR_ANKLE_L:
            case WEAR_WRIST_R:
            case WEAR_WRIST_L:
            case WEAR_FACE:
            case WEAR_LEFT_EAR:
            case WEAR_RIGHT_EAR:
            case WEAR_ABOUT_HEAD:
            case WEAR_SHOULDER_R:
            case WEAR_SHOULDER_L:
                /* somewhat difficult */
                locmod = 20;
                break;
            case EP:
            case ES:
                /* difficult */
                locmod = 60;
                break;
            case ETWO:
            case WEAR_BELT:
            case WEAR_BACK:
            case WEAR_BODY:
            case WEAR_HEAD:
            case WEAR_LEGS:
            case WEAR_FEET:
            case WEAR_HANDS:
            case WEAR_ARMS:
            case WEAR_ABOUT:
            case WEAR_WAIST:
            case WEAR_FOREARMS:
            case WEAR_ON_BACK:
            case WEAR_OVER_SHOULDER_R:
            case WEAR_OVER_SHOULDER_L:
                /* very difficult */
                locmod = 80;
                break;
            default:
                break;
            }
        }
        /* subtract the modifier based on location */
        skill -= locmod;
        //gamelogf( "after location modifier: %d", skill );

        // if it's covered
        if( is_covered(ch, victim, loc)) {
            switch (loc) {
            // gloves over fingers
            case WEAR_FINGER_R:
            case WEAR_FINGER_L:
            case WEAR_FINGER_R2:
            case WEAR_FINGER_L2:
            case WEAR_FINGER_R3:
            case WEAR_FINGER_L3:
            case WEAR_FINGER_R4:
            case WEAR_FINGER_L4:
            case WEAR_FINGER_R5:
            case WEAR_FINGER_L5:
                // covered fingers very very very very hard
                skill -= 80;
                break;
            // closed cloak over body
            case WEAR_BODY:
            case WEAR_BELT:
            case WEAR_ON_BELT_1:
            case WEAR_ON_BELT_2:
            case WEAR_WAIST: 
                skill -= 30;
                break;
            }
        }
    }

    /* mindwiped by a psionicist */
    if (affected_by_spell(ch, PSI_MINDWIPE))
        skill -= 25;

    /* Hallucinatory drug that affects the brain */
    if (affected_by_spell(ch, POISON_SKELLEBAIN))
        skill -= 40;

    /* Hallucinatory drug that affects the brain phase 2*/
    if (affected_by_spell(ch, POISON_SKELLEBAIN_2))
        skill -= 50;

    /* Hallucinatory drug that affects the brain phase 3*/
    if (affected_by_spell(ch, POISON_SKELLEBAIN_3))
        skill -= 60;

    /* bonus if there are crowds */
    if (IS_SET(ch->in_room->room_flags, RFL_POPULATED))
        skill += 30;

    /* but if it's too enclosed of a space...penalty */
    if (IS_SET(ch->in_room->room_flags, RFL_INDOORS))
        skill = (skill * 3 / 4);

    //gamelogf( "steal before max/min: %d", skill );

    /* always a 1% chance */
    skill = MAX(skill, 1);

    /* no more than a 100% chance */
    skill = MIN(skill, 100);

    //gamelogf( "final steal skill: %d", skill );

    /* give the skill % back for comparison */
    return (skill);
}


void
skill_steal(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
            OBJ_DATA * tar_obj)
{
    CHAR_DATA *guard = NULL;
    CHAR_DATA *victim = NULL;
    CHAR_DATA *rch = NULL;
    struct guard_type *curr_guard;
    OBJ_DATA *obj = NULL, *item = NULL;
    char victim_name[256], obj_name[256];
    char buf[256];
    int eq_pos = -1, ct, obsidian = 0, vnum;
    bool ohoh = FALSE;
    char item_name[256] = "";
    int roll = number(1, 101);
    int skill = 0;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char real_name_buf[MAX_STRING_LENGTH];

    if (hands_free(ch) < 1) {
        send_to_char("You'll need at least one free hand to do that.\n\r", ch);
        return;
    }

    if (ch->specials.riding) {
        send_to_char("You can't do that while riding.\n\r", ch);
        return;
    }

    extract_emote_arguments(arg, preHow, sizeof(preHow), args,
                            sizeof(args), postHow, sizeof(postHow));

    arg = two_arguments(arg, obj_name, sizeof(obj_name), victim_name, sizeof(victim_name));

    victim = get_char_room_vis(ch, victim_name);

    /* Need to check for person's item here */
    if (!victim && (strlen(victim_name) > 2)
        && (((victim_name[strlen(victim_name) - 2] == '\'')
             && (victim_name[strlen(victim_name) - 1] == 's'))
            || (((victim_name[strlen(victim_name) - 1] == '\'')
                 && (victim_name[strlen(victim_name) - 2] == 's'))))) {
        /* crop off the possessives */
        if (victim_name[strlen(victim_name) - 1] == '\'')
            victim_name[strlen(victim_name) - 1] = 0;   /* nul terminate */
        else
            victim_name[strlen(victim_name) - 2] = 0;   /* nul terminate */

        victim = get_char_room_vis(ch, victim_name);
        arg = one_argument(arg, item_name, sizeof(item_name));
    }

    if (!victim) {
        if ((obj = get_obj_in_list_vis(ch, obj_name, ch->in_room->contents))) {
            /* check for skill success */
            if ((skill = get_steal_skill(ch, obj, NULL)) < roll) {
                if ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)) {
                    if (!IS_SET(obj->obj_flags.wear_flags, ITEM_TAKE)) {
                        send_to_char("You cannot take that.\n\r", ch);
                        return;
                    }
/* added so you can't steal glyphed stuff  -sv */
                    if ((IS_SET(obj->obj_flags.extra_flags, OFL_GLYPH))
                        && (!IS_IMMORTAL(ch))) {
                        act("$p sparks as $n tries take it, burning $s skin.", FALSE, ch, obj, 0,
                            TO_ROOM);
                        act("$p sparks as you try to take it, burning your skin.", FALSE, ch, obj,
                            0, TO_CHAR);
                        generic_damage(ch, dice(obj->obj_flags.temp, 4), 0, 0,
                                       dice(obj->obj_flags.temp, 6));
                        REMOVE_BIT(obj->obj_flags.extra_flags, OFL_GLYPH);
                        REMOVE_BIT(obj->obj_flags.extra_flags, OFL_MAGIC);
                        return;
                    }
                    if ((isnamebracketsok("[no_steal]", OSTR(obj, name)))
                        && (!IS_IMMORTAL(ch))) {
                        act("You couldn't steal that without being noticed.", FALSE, ch, 0, 0,
                            TO_CHAR);
                        return;
                    }
		    snprintf(buf, sizeof(buf), "%s gets %s from room r%d. (steal failure)\n\r", GET_NAME(ch), OSTR(obj, short_descr), ch->in_room->number);
		    send_to_monitors(ch, NULL, buf, MONITOR_ROGUE);
		    shhlogf("%s: %s", __FUNCTION__, buf);

		    cmd_get(ch, obj_name, 0, 0);
		    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
			    if (watch_success(rch, ch, NULL, roll - skill, SKILL_STEAL)) {
				    cprintf(rch, "You notice %s was trying to be furtive about it.\n\r", PERS(rch, ch));
			    }
		    }
		    return;
                } else {
                    send_to_char("You're carrying too many things.\n\r", ch);
                    return;
                }
            } else {
                if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
			snprintf(buf, sizeof(buf), "%s steals %s from room r%d. (success)\n\r",
					GET_NAME(ch), OSTR(obj, short_descr), ch->in_room->number);
			send_to_monitors(ch, NULL, buf, MONITOR_ROGUE);
			shhlogf("%s: %s", __FUNCTION__, buf);
 
                        sprintf(to_char, "you deftly pick up %s", OSTR(obj, short_descr));
		        sprintf(to_room, "@ deftly picks up %s", OSTR(obj, short_descr));
		        if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow, to_char,
			        to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
                            send_to_char("Got it!\n\r", ch);

                        obj_from_room(obj);
                        obj_to_char(obj, ch);

                        // if it's a light and it's lit
                        if (GET_ITEM_TYPE(obj) == ITEM_LIGHT) {
                            extinguish_object(ch, obj);
                        }
			return;
		} else {
			send_to_char("It's too heavy.\n\r", ch);
			return;
		}
	    }
	}
        if (ch)
            send_to_char("Steal what?\n\r", ch);
        return;
    } else if (victim == ch) {
        send_to_char("Come on now, that's rather stupid.\n\r", ch);
        return;
    } else if (GET_LEVEL(ch) < GET_LEVEL(victim) && !IS_NPC(victim)) {
        send_to_char("You really don't want to do that.\n\r", ch);
        return;
    }

    if ((!stricmp(obj_name, "iron") || !stricmp(obj_name, "steel")
         || !stricmp(obj_name, "copper") || !stricmp(obj_name, "bronze")
         || !stricmp(obj_name, "wood") || !stricmp(obj_name, "silver")
         || !stricmp(obj_name, "gold") || !stricmp(obj_name, "chitin")
         || !stricmp(obj_name, "bone")) && (!IS_IMMORTAL(ch))) {
        sprintf(buf, "Be more specific, steal the %s what?\n\r", obj_name);
        send_to_char(buf, ch);
        return;
    }

    if (victim->guards) {
        for (curr_guard = victim->guards; curr_guard; curr_guard = curr_guard->next) {
            guard = curr_guard->guard;

            if ((guard != ch) && (GET_POS(guard) >= POSITION_STANDING)
                && (CAN_SEE(guard, ch)) && (guard != victim)
                && (guard->in_room == victim->in_room)) {
                if (skill_success
                    (guard, ch,
                     MAX(15,
                         guard->skills[SKILL_GUARD] ? guard->skills[SKILL_GUARD]->learned : 0))) {
                    act("$n steps forward, and you flee to avoid being caught.\n\r", FALSE, guard,
                        0, ch, TO_VICT);
                    act("You notice someone approaching, but they flee as $n steps forward.\n\r",
                        FALSE, guard, 0, victim, TO_VICT);
                    act("You notice someone approaching, but they flee as you step in front of $n.\n\r", FALSE, victim, 0, guard, TO_VICT);

                    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
                        /* chance to 'watch' and see guard movement. */
                        if (watch_success(rch, ch, guard, 0, SKILL_STEAL)) {
                            if (rch == guard || rch == victim) {
                                cprintf(rch, "You notice %s running away.\n\r", PERS(rch, ch));
                            } else {
                                strcpy( buf, PERS(rch, guard));
                                cprintf(rch, "You notice %s approach %s then run away.\n\r",
                                        PERS(rch, ch), buf);
                            }
                        }
                    }
                    snprintf(buf, sizeof(buf),
                             "%s is blocked from stealing by %s. (steal failure)\n\r", GET_NAME(ch),
                             GET_NAME(guard));
                    send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
                    shhlogf("%s: %s", __FUNCTION__, buf);

                    return;
                }
            }
        }
    }

    /* A good place to do the steal from worn-obj stuff */
    if (*item_name) {
        if (!(item = get_object_in_equip_vis(ch, item_name, victim->equipment, &eq_pos))) {
            if (!(item = get_obj_in_list_vis(ch, item_name, victim->carrying)))  {
                act("$E doesn't have anything like that.", FALSE, ch, 0, victim, TO_CHAR);
                return;
	    }
        }

        if (is_covered(ch, victim, eq_pos) && !IS_IMMORTAL(ch)) {
            act("Surely, $E would notice you stealing that.\n\r", FALSE, ch, 0, victim, TO_CHAR);
            return;
        }

        if (item->obj_flags.type != ITEM_CONTAINER) {
            act("You can't steal anything from $p.", FALSE, ch, item, victim, TO_CHAR);
            return;
        } else if (is_closed_container(ch, item) && !IS_IMMORTAL(ch)) {
            act("$p is closed.", FALSE, ch, item, victim, TO_CHAR);
            return;
        }

        if (!(obj = get_obj_in_list_vis(victim, obj_name, item->contains))) {
            act("You can't find that in $p.", FALSE, ch, item, victim, TO_CHAR);
            snprintf(buf, sizeof(buf), "%s fails to steal %s from %s's container. (no item)\n\r",
                     GET_NAME(ch), obj_name, GET_NAME(victim));
            send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
            shhlogf("%s: %s", __FUNCTION__, buf);

            /* So you won't use "steal" to find stuff (ie. peek into a pack ) */
            if ((skill = get_steal_skill(ch, NULL, victim)) < roll)
                ohoh = TRUE;
        } else {                /* The target is real, try to swipe it */
            if ((GET_POS(victim) > POSITION_SLEEPING && GET_OBJ_WEIGHT(obj) >= 10
                 && (!IS_IMMORTAL(ch)))) {
                send_to_char("It is way too heavy to steal, they would surely notice.", ch);
                snprintf(buf, sizeof(buf),
                         "%s fails to steal %s from %s's container. (too heavy)\n\r", GET_NAME(ch),
                         OSTR(obj, short_descr), GET_NAME(victim));
                send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
                shhlogf("%s: %s", __FUNCTION__, buf);
                return;
            }

            if ((isnamebracketsok("[no_steal]", OSTR(obj, name)))
                && (!IS_IMMORTAL(ch))) {
                act("You can't seem to unfasten it from $N.", FALSE, ch, 0, victim, TO_CHAR);
                return;
            }

            if ((obj->obj_flags.type == ITEM_MONEY)
                && (obj->obj_flags.value[0] > 2)) {
                if ((skill = get_steal_skill(ch, obj, victim)) >= roll) {
                    obsidian = (number(0, obj->obj_flags.value[0] - 2) % 100);
                    if (obsidian) {
                        obj->obj_flags.value[0] -= obsidian;
                        GET_OBSIDIAN(ch) += obsidian;

                        snprintf(buf, sizeof(buf), "%s steals %d coins from %s's container.\n\r",
                                 GET_NAME(ch), obsidian, GET_NAME(victim));
                        shhlogf("%s: %s", __FUNCTION__, buf);
                    } else {
                        snprintf(buf, sizeof(buf),
                                 "%s steals 0 coins from %s's container. (success)\n\r",
                                 GET_NAME(ch), GET_NAME(victim));
                        shhlogf("%s: %s", __FUNCTION__, buf);
                    }

                    sprintf(to_char, "you steal %d coins from %%%s ~%s", obsidian, FORCE_NAME(victim), OSTR(item, short_descr));
		    sprintf(to_room, "@ steals something from ~%s", FORCE_NAME(victim));
		    if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow, to_char, NULL,
			to_room, to_room, MONITOR_ROGUE, 0, SKILL_SLEIGHT_OF_HAND))
                        send_to_char("Got it!\n\r", ch);
                } else
                    ohoh = TRUE;
            } else if ((skill = get_steal_skill(ch, obj, victim)) >= roll) {
                if (obj->obj_flags.type == ITEM_MONEY) {
                    obsidian = obj->obj_flags.value[0];
                    snprintf(buf, sizeof(buf),
                             "%s steals %d coin from %s's container. (<2 coins?)\n\r", GET_NAME(ch),
                             obj->obj_flags.value[0], GET_NAME(victim));
                    shhlogf("%s: %s", __FUNCTION__, buf);

                    sprintf(to_char, "you steal %d coins from %%%s ~%s", obsidian, FORCE_NAME(victim), OSTR(item, short_descr));
		    sprintf(to_room, "@ steals something from ~%s", FORCE_NAME(victim));
		    if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow, to_char, NULL,
			to_room, to_room, MONITOR_ROGUE, 0, SKILL_SLEIGHT_OF_HAND))
			    return;
                } else {
                    sprintf(to_char, "you steal %s from %%%s ~%s", OSTR(obj, short_descr), FORCE_NAME(victim), OSTR(item, short_descr));
                    sprintf(to_room, "@ steals something from ~%s", FORCE_NAME(victim));

                    if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow, to_char, NULL,
                     to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
                        send_to_char("Got it!\n\r", ch);
                    snprintf(buf, sizeof(buf), "%s steals %s from %s's container.\n\r",
                             GET_NAME(ch), OSTR(obj, short_descr), GET_NAME(victim));
                    shhlogf("%s: %s", __FUNCTION__, buf);
                }

                obj_from_obj(obj);
                obj_to_char(obj, ch);
            } else {
                if (is_paralyzed(victim)
                    || IS_AFFECTED(victim, CHAR_AFF_SUBDUED)
                    || !AWAKE(victim)) {
                    sprintf(to_char, "you steal %s from %%%s ~%s as they sleep", OSTR(obj, short_descr), FORCE_NAME(victim), OSTR(item, short_descr));
                    sprintf(to_room, "@ steals something from ~%s as they sleep", FORCE_NAME(victim));

                    if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow, to_char, NULL,
                     to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
                        send_to_char("Got it!\n\r", ch);
                    snprintf(buf, sizeof(buf),
                             "%s steals %s from %s's container as they sleep.\n\r", GET_NAME(ch),
                             OSTR(obj, short_descr), GET_NAME(victim));
                    shhlogf("%s: %s", __FUNCTION__, buf);

                    obj_from_obj(obj);
                    obj_to_char(obj, ch);

                    if (!AWAKE(victim) && !knocked_out(victim)) {
                        cprintf(victim, "You feel a faint tugging.\n\r");
                    }
                }
                ohoh = TRUE;
            }
        }
    } else if (stricmp(obj_name, "coins") && stricmp(obj_name, "obsidian")) {
        if (!(obj = get_obj_in_list_vis(victim, obj_name, victim->carrying))) {
            if (!(obj = get_object_in_equip_vis(ch, obj_name, victim->equipment, &eq_pos))) {
                act("$E doesn't have anything like that.", FALSE, ch, 0, victim, TO_CHAR);
                return;
            }
        }

        if (eq_pos > -1) {
            if ((eq_pos != WEAR_ON_BELT_1) && (eq_pos != WEAR_ON_BELT_2)) {
                if (is_covered(ch, victim, eq_pos)) {
                    act("$E doesn't have anything like that.", FALSE, ch, 0, victim, TO_CHAR);
                    return;
                } else if (AWAKE(victim)) {
                    send_to_char("That would be impossible.\n\r", ch);
                    return;
                }
            }
        }

        /* I don't want people to be able to steal the allanaki elementalist
         * gems from one another to get them off */
        vnum = obj_index[obj->nr].vnum;
        if ((vnum == 50606) || (vnum == 50607)) {
            cprintf(ch,
                    "A dull pain pounds into the back of your head, causing "
                    "you to abort your stealthy maneuver.\n");
            generic_damage(ch, 5, 0, 0, 10);
            return;
        }

        if ((isnamebracketsok("[no_steal]", OSTR(obj, name)))
            && (!IS_IMMORTAL(ch))) {
            act("You can't seem to unfasten it from $N.", FALSE, ch, 0, victim, TO_CHAR);
            return;
        }

        if ((GET_POS(victim) > POSITION_SLEEPING && GET_OBJ_WEIGHT(obj) >= 10
             && (!IS_IMMORTAL(ch)))) {
            send_to_char("It is way too heavy to steal, they would surely notice.", ch);
            return;
        }

        if ((skill = get_steal_skill(ch, obj, victim)) >= roll) {
            /* added so you can't steal glyphed stuff  -sv */
            if ((IS_SET(obj->obj_flags.extra_flags, OFL_GLYPH))
                && (!IS_IMMORTAL(ch))) {
                act("$p sparks as $n tries take it, burning $s skin.", FALSE, ch, obj, 0, TO_ROOM);
                act("$p sparks as you try to take it, burning your skin.", FALSE, ch, obj, 0,
                    TO_CHAR);
                generic_damage(ch, dice(obj->obj_flags.temp, 4), 0, 0, dice(obj->obj_flags.temp, 6));
                REMOVE_BIT(obj->obj_flags.extra_flags, OFL_GLYPH);
                REMOVE_BIT(obj->obj_flags.extra_flags, OFL_MAGIC);
                return;
            } else {
                snprintf(buf, sizeof(buf), "%s steals %s from %s's inventory.\n\r", GET_NAME(ch),
                         OSTR(obj, short_descr), GET_NAME(victim));
                shhlogf("%s: %s", __FUNCTION__, buf);

                sprintf(to_char, "you steal %s from ~%s", OSTR(obj, short_descr), FORCE_NAME(victim));
                sprintf(to_room, "@ steals something from ~%s", FORCE_NAME(victim));
                if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow,
			to_char, NULL, to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
		    send_to_char("Got it!\n\r", ch);
            }
            if (eq_pos > -1) {
                if (eq_pos == WEAR_BELT) {
                    /* The thief wants to steal their belt.  If there's
                     * anything on the belt, remove it and place it into the
                     * victim's inventory first.  7/15/2005 -Tiernan */
                    if (victim->equipment[WEAR_ON_BELT_1])
                        obj_to_char(unequip_char(victim, WEAR_ON_BELT_1), victim);
                    if (victim->equipment[WEAR_ON_BELT_2])
                        obj_to_char(unequip_char(victim, WEAR_ON_BELT_2), victim);
                }
                obj_to_char(unequip_char(victim, eq_pos), ch);
            } else {
                obj_from_char(obj);
                obj_to_char(obj, ch);
            }
            return;
        } else {
            if (is_paralyzed(victim)
                || IS_AFFECTED(victim, CHAR_AFF_SUBDUED)
                || !AWAKE(victim)) {
                sprintf(to_char, "you steal %s from ~%s, who is helpless", OSTR(obj, short_descr), FORCE_NAME(victim));
                sprintf(to_room, "@ steals something from ~%s, who is helpless", FORCE_NAME(victim));
                if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow,
			to_char, NULL, to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
		    send_to_char("Got it!\n\r", ch);
                snprintf(buf, sizeof(buf), "%s steals %s from %s, who is helpless.\n\r",
                         GET_NAME(ch), OSTR(obj, short_descr), GET_NAME(victim));
                shhlogf("%s: %s", __FUNCTION__, buf);

                if (eq_pos > -1) {
                    if (eq_pos == WEAR_BELT) {
                        /* The victim is asleep and the thief wants to steal
                         * their belt.  If there's anything on the belt, remove
                         * it and place it into the victim's inventory first.
                         * 7/15/2005 -Tiernan */
                        if (victim->equipment[WEAR_ON_BELT_1])
                            obj_to_char(unequip_char(victim, WEAR_ON_BELT_1), victim);
                        if (victim->equipment[WEAR_ON_BELT_2])
                            obj_to_char(unequip_char(victim, WEAR_ON_BELT_2), victim);
                    }
                    obj_to_char(unequip_char(victim, eq_pos), ch);
                } else {
                    obj_from_char(obj);
                    obj_to_char(obj, ch);
                }

                /* if you're not awake, but not knocked out */
                if (!AWAKE(victim) && !knocked_out(victim)) {
                    cprintf(victim, "You feel a faint tugging.\n\r");
                }
            }

            ohoh = TRUE;
        }
    } else {
        if ((skill = get_steal_skill(ch, NULL, victim)) >= roll) {
            obsidian = (number(0, GET_OBSIDIAN(victim)) % 100);
            if (obsidian && money_mgr(MM_CH2CH, obsidian, (void *) victim, (void *) ch)) {
                snprintf(buf, sizeof(buf), "%s steals %d coins from %s.\n\r", GET_NAME(ch),
                         obsidian, GET_NAME(victim));
                send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
                shhlogf("%s: %s", __FUNCTION__, buf);
                sprintf(to_char, "you steal %d coins from ~%s", obsidian, FORCE_NAME(victim));
                sprintf(to_room, "@ steals something from ~%s", FORCE_NAME(victim));
                if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow,
			to_char, NULL, to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
		    cprintf(ch, "You manage to steal %d obsidian coins.\n\r", obsidian);
            } else {
                snprintf(buf, sizeof(buf), "%s steals 0 coins from %s. (success)\n\r", GET_NAME(ch),
                         GET_NAME(victim));
                send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
                shhlogf("%s: %s", __FUNCTION__, buf);
                sprintf(to_char, "you steal %d coins from ~%s", obsidian, FORCE_NAME(victim));
                sprintf(to_room, "@ steals something from ~%s", FORCE_NAME(victim));
                if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow,
			to_char, NULL, to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
		    send_to_char("You couldn't get any coins at all.\n\r", ch);
            }
        } else {
            if (is_paralyzed(victim)
                || IS_AFFECTED(victim, CHAR_AFF_SUBDUED)
                || !AWAKE(victim)) {
                obsidian = (number(0, GET_OBSIDIAN(victim)) % 100);
                if (obsidian && money_mgr(MM_CH2CH, obsidian, (void *) victim, (void *) ch)) {
                    snprintf(buf, sizeof(buf), "%s steals %d coins from %s, who is helpless.\n\r",
                             GET_NAME(ch), obsidian, GET_NAME(victim));
                    send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
                    shhlogf("%s: %s", __FUNCTION__, buf);

                    cprintf(ch, "You manage to steal %d obsidian coins.\n\r", obsidian);

                    act("$n takes some coins from $N.", TRUE, ch, obj, victim, TO_ROOM);

                    /* if you're not awake, but not knocked out */
                    if (!AWAKE(victim) && !knocked_out(victim)) {
                        cprintf(victim, "You feel a faint tugging.\n\r");
                    }
                }
            }

            ohoh = TRUE;
        }
    }

    if (ohoh) {
        if (AWAKE(victim)) {
            /* Char was seen */
            roll = number(1, 101);
            /* skill without taking into account the object */
            skill = get_steal_skill(ch, NULL, victim);
            if (skill < roll) {
                REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);

                snprintf(buf, sizeof(buf), "%s got caught stealing from from %s.\n\r", GET_NAME(ch),
                         GET_NAME(victim));
                send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
                shhlogf("%s: %s", __FUNCTION__, buf);
                act("$E discovers you at the last instant!", FALSE, ch, 0, victim, TO_CHAR);
                act("$n tries to steal something from you.", FALSE, ch, 0, victim, TO_VICT);
                act("$n tries to steal something from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
                check_theft_flag(ch, victim);
                if (IS_NPC(victim)) {
                    if (GET_RACE_TYPE(victim) == RTYPE_HUMANOID) {
                        strcpy(buf, "Thief! Thief!");
                        cmd_shout(victim, buf, 0, 0);
                    };
                    if ((ct = room_in_city(ch->in_room)) == CITY_NONE)
                        hit(victim, ch, TYPE_UNDEFINED);
                    else if (!IS_SET(victim->specials.act, CFL_SENTINEL))
                        cmd_flee(victim, "", 0, 0);
                }
            } /* End char got seen */
            else {              /* Char wasn't seen */
                snprintf(buf, sizeof(buf),
                         "%s bungles an attempt to steal from %s, but is not caught.\n\r",
                         GET_NAME(ch), GET_NAME(victim));
                send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
                shhlogf("%s: %s", __FUNCTION__, buf);

                sprintf(to_char, "you fail to steal from ~%s", FORCE_NAME(victim));
                sprintf(to_room, "@ tries to steal from ~%s", FORCE_NAME(victim));
                if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow,
			to_char, NULL, to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
		    return;

                act("You cover your mistake before $E discovers you.", FALSE, ch, 0, victim,
                    TO_CHAR);
                act("You feel a hand in your belongings, but are unable to catch the culprit.",
                    FALSE, ch, 0, victim, TO_VICT);

                if (IS_NPC(victim)) {
                    if (GET_RACE_TYPE(victim) == RTYPE_HUMANOID)

/* My thanks to whoever made it so kanks don't shout thief thief anymore. */

                    {
                        strcpy(buf, "Thief! Thief!");
                        cmd_shout(victim, buf, 0, 0);
                    } else if (!IS_SET(victim->specials.act, CFL_SENTINEL))
                        cmd_flee(victim, "", 0, 0);
                }
            }                   /* Char isn't seen */
        }

        /* If the char is awake */
        /* only gain if ! paralyzed or low skill */
        if (!is_paralyzed(victim) || get_skill_percentage(ch, SKILL_STEAL) < 40)
            gain_skill(ch, SKILL_STEAL, 1);
    }
}


void
skill_pick(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{                               /* begin Skill_Pick */
    byte percent;
    char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], arg3[MAX_STRING_LENGTH];
    int difficulty = 0;
    int learned;
    int door;
    struct room_data *other_room;
    struct room_direction_data *back;
    OBJ_DATA *obj;
    OBJ_DATA *picks;
    CHAR_DATA *victim;
    bool relocking = FALSE;

    arg = two_arguments(arg, type, sizeof(type), dir, sizeof(dir));
    arg = one_argument(arg, arg3, sizeof(arg3));

    if (!stricmp(arg3, "relock"))
      relocking = TRUE;

    // relock could be the 2nd argument if they're picking an item or 
    // don't specify a direction
    if (!stricmp(dir, "relock")) {
      relocking = TRUE;
      // voiding this out so the code doesn't try & find a valid direction later
      dir[0] = '\0';
    }
    
    if (!(picks = get_tool(ch, TOOL_LOCKPICK))) {
      if (!relocking)
        send_to_char("Do you plan to pick it with your fingers?  "
                     "You'll need some lock picks.\n\r", ch);
      else
        send_to_char("Do you plan to relock it with your fingers?  "
                     "You'll need some lock picks.\n\r", ch);
      return;
    }
    
    percent = number(1, 101);
    if (affected_by_spell(ch, PSI_MINDWIPE))
      percent += 25;
    
    learned = ch->skills[SKILL_PICK_LOCK]
      ? ch->skills[SKILL_PICK_LOCK]->learned : 0;
    
    /* Let the pick do some of the work -Morg 12/13/01 */
    learned += picks->obj_flags.value[1];
    
    if (!*type)
        send_to_char("Pick what?\n\r", ch);
    else if (generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj)) {
        if (obj->obj_flags.type != ITEM_CONTAINER)
            send_to_char("That's not a container.\n\r", ch);
        else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED))
            send_to_char("But it isn't even closed!\n\r", ch);
        else if (obj->obj_flags.value[2] <= 0)
            send_to_char("Odd - you can't seem to find a keyhole.\n\r", ch);
        else if (!IS_SET(obj->obj_flags.value[1], CONT_LOCKED)) {
            if (!relocking)
                send_to_char("Oho! This thing is NOT locked!\n\r", ch);
            else {
	            if (IS_SET(obj->obj_flags.value[1], CONT_PICKPROOF))
	                send_to_char("You failed to relock it.\n\r", ch);
	            else {
                    if (learned < 71)
                        send_to_char("You aren't skilled enough to relock that.\n\r", ch);
                    else {
		                if (percent > learned) {
                            gain_skill(ch, SKILL_PICK_LOCK, 1);
                            send_to_char("You failed to relock it.\n\r", ch);
                            if (percent - learned /5 > picks->obj_flags.value[1]) {
                                if (--picks->obj_flags.value[1] <=0) {
                                    act ("$p breaks in the attempt.", FALSE, ch, picks, NULL, TO_CHAR);
                                    extract_obj(picks);
                                }
                            }
                            return;
                        }
                        MUD_SET_BIT(obj->obj_flags.value[1], CONT_LOCKED);
                        send_to_char("*Click*\n\r", ch);
                        act ("$n fiddles with $p.", FALSE, ch, obj, 0, TO_ROOM);
                    }
                }
            }
        }
        else if (IS_SET(obj->obj_flags.value[1], CONT_PICKPROOF)) {
            if (!relocking)
                send_to_char("You failed to pick it.\n\r", ch);
            else 
                send_to_char("Oho! This thing is ALREADY locked!\n\r", ch);
            }
        else {
            if (relocking)
                send_to_char("Oho! This thing is ALREADY locked!\n\r", ch);
            else {
                if (percent > learned) {
                    gain_skill(ch, SKILL_PICK_LOCK, 1);
                    send_to_char("You failed to pick the lock.\n\r", ch);
                    if ((percent - learned) / 5 > picks->obj_flags.value[1]) {
                        if (--picks->obj_flags.value[1] <= 0) {
                            act("$p breaks in the attempt.", FALSE, ch, picks, NULL, TO_CHAR);
                            extract_obj(picks);
                        }
                    }
                    return;
                }
                REMOVE_BIT(obj->obj_flags.value[1], CONT_LOCKED);
                send_to_char("*Click*\n\r", ch);
                act("$n fiddles with $p.", FALSE, ch, obj, 0, TO_ROOM);
            }
        }
    } else if ((door = find_door(ch, type, dir)) >= 0) {
        if (IS_SET(EXIT(ch, door)->exit_info, EX_SPL_FIRE_WALL)) {
            send_to_char("A wall of fire keeps you from getting close to the " "lock.\n\r", ch);
            return;
	}
        if (IS_SET(EXIT(ch, door)->exit_info, EX_SPL_WIND_WALL)) {
            send_to_char("Strong winds blow you away the lock.\n\r", ch);
            return;
        }
        if (IS_SET(EXIT(ch, door)->exit_info, EX_SPL_BLADE_BARRIER)) {
            send_to_char("A whirling wall of blades keep you from getting "
                         "close to the lock.\n\r", ch);
            return;
        } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
            send_to_char("That's absurd.\n\r", ch);
        else if (exit_guarded(ch, door, CMD_PICK))
            return;
        else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
	  if (!relocking)
            send_to_char("You realize that the door is already open.\n\r", ch);
	  else
            send_to_char("You'll have to close the door before you can relock it.\n\r", ch);
	}
        else if (EXIT(ch, door)->key < 0)
            send_to_char("You can't seem to spot any lock to pick.\n\r", ch);
        else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) { /* bobo */
	  if (!relocking)
	    send_to_char("Oh.. it wasn't locked at all.\n\r", ch);
	  else {  // Relocking non-locked valid exit
            if (IS_SET(EXIT(ch, door)->exit_info, EX_DIFF_1))
	      difficulty = difficulty + 1;
            if (IS_SET(EXIT(ch, door)->exit_info, EX_DIFF_2))
	      difficulty = difficulty + 2;
            if (IS_SET(EXIT(ch, door)->exit_info, EX_DIFF_4))
	      difficulty = difficulty + 4;
	    
            switch (difficulty) {       /* Begining of switch for cases of difficulty */
            case 0:            /* Instant success, no skill needed, just tools. */
	      difficulty = 0;
	      sprintf(buf, "Frighteningly simple, you barely needed the pick.\n\r");
	      break;
            case 1:            /* 15%, starting chance for burglars, etc.. */
	      difficulty = 15;
	      sprintf(buf, "Although fairly primitive, you find this lock impossible to relock.\n\r");
	      break;
            case 2:            /* 30% */
	      difficulty = 30;
	      sprintf(buf, "Below average in quality, this lock is still beyond your ability to relock.\n\r");
	      break;
            case 3:            /* 45% */
	      difficulty = 45;
	      sprintf(buf, "Despite being fairly standard, it is unlikely you will be able to relock this exit.\n\r");
	      break;
            case 4:            /* 60% */
	      difficulty = 60;
	      sprintf(buf, "A cut above average, you find this lock too difficult to relock.\n\r");
	      break;
            case 5:            /* 75% */
	      difficulty = 75;
	      sprintf(buf, "A masterpiece in itself, this lock will require more skill than you possess.\n\r");
	      break;
            case 6:            /* 100%, will require a pretty good tool AND high skill. */
	      difficulty = 100;
	      sprintf(buf, "Representing a pinnacle in the art of such devices, this lock will require both great skill and superior tools to relock.\n\r");
	      break;
            case 7:            /* Impossible, in time this will replace the pickproof flag */
	      
	      /* changed to 1 + learned to make it truly impossible -Morg */
	      difficulty = 1 + learned;
	      sprintf(buf, "You find this lock impossible to relock.\n\r");
	      break;
            default:
	      sprintf(buf, "ERROR: Problem with EX_DIFF_* flags.\n\r");
            } /* end of switch for cases of relock difficulty */


            if (difficulty > learned) {
	      if ((difficulty - learned) / 5 > picks->obj_flags.value[1]) {
		if (--picks->obj_flags.value[1] <= 0) {
		  act("$p breaks in the attempt.", FALSE, ch, picks, NULL, TO_CHAR);
		  extract_obj(picks);
		}
	      }
	      send_to_char(buf, ch);
	      return;
            }
	    
            if (difficulty > 0 && percent > learned) {
	      gain_skill(ch, SKILL_PICK_LOCK, 1);
	      send_to_char("You failed to relock the exit.\n\r", ch);
	      
	      if ((percent - learned) / 5 > picks->obj_flags.value[1]) {
		if (--picks->obj_flags.value[1] <= 0) {
		  act("$p breaks in the attempt.", FALSE, ch, picks, NULL, TO_CHAR);
		  extract_obj(picks);
		}
	      }
	      return;
            }
	    MUD_SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
            if (EXIT(ch, door)->keyword)
	      act("$n skillfully relocks the lock of the $F.", 0, ch, 0, EXIT(ch, door)->keyword,
		  TO_ROOM);
            else
	      act("$n relocks the lock on the $F.", TRUE, ch, 0, (void*)dirs[door], TO_ROOM);
	    
            send_to_char("The twists closed with a soft click.\n\r", ch);
	    
            if ((other_room = EXIT(ch, door)->to_room))
	      if ((back = other_room->direction[(int) rev_dir[door]]))
		if (back->to_room == ch->in_room)
		  MUD_SET_BIT(back->exit_info, EX_LOCKED);
	  }
	}
        else {                  /* Begining of valid doors */
	  if (relocking) {
	    send_to_char("It is already locked.\n\r", ch);
	    return;
	  }
            if (IS_SET(EXIT(ch, door)->exit_info, EX_DIFF_1))
                difficulty = difficulty + 1;
            if (IS_SET(EXIT(ch, door)->exit_info, EX_DIFF_2))
                difficulty = difficulty + 2;
            if (IS_SET(EXIT(ch, door)->exit_info, EX_DIFF_4))
                difficulty = difficulty + 4;
            switch (difficulty) {       /* Begining of switch for cases of difficulty */
            case 0:            /* Instant success, no skill needed, just tools. */
                difficulty = 0;
                sprintf(buf, "Frighteningly simple, odd that it was locked at all.\n\r");
                break;
            case 1:            /* 15%, starting chance for burglars, etc.. */
                difficulty = 15;
                sprintf(buf, "Although fairly primitive, you find this lock impossible.\n\r");
                break;
            case 2:            /* 30% */
                difficulty = 30;
                sprintf(buf,
                        "Below average in quality, this lock is still beyond your ability.\n\r");
                break;
            case 3:            /* 45% */
                difficulty = 45;
                sprintf(buf,
                        "Despite being fairly standard, it is unlikely this lock will yield to you.\n\r");
                break;
            case 4:            /* 60% */
                difficulty = 60;
                sprintf(buf,
                        "A cut above average, you find this lock too difficult to spring.\n\r");
                break;
            case 5:            /* 75% */
                difficulty = 75;
                sprintf(buf,
                        "A masterpiece in itself, this lock will require more skill than you possess.\n\r");
                break;
            case 6:            /* 100%, will require a pretty good tool AND high skill. */
                difficulty = 100;
                sprintf(buf,
                        "Representing a pinnacle in the art of such devices, this lock will require both great skill and superior tools.\n\r");
                break;
            case 7:            /* Impossible, in time this will replace the pickproof flag */

                /* changed to 1 + learned to make it truly impossible -Morg */
                difficulty = 1 + learned;
                sprintf(buf, "You find this lock impossible to pick.\n\r");
                break;
            default:
                sprintf(buf, "ERROR: Problem with EX_DIFF_* flags.\n\r");
            }                   /* end of switch for cases of difficulty */


            if (difficulty > learned) {
                if ((difficulty - learned) / 5 > picks->obj_flags.value[1]) {
                    if (--picks->obj_flags.value[1] <= 0) {
                        act("$p breaks in the attempt.", FALSE, ch, picks, NULL, TO_CHAR);
                        extract_obj(picks);
                    }
                }
                send_to_char(buf, ch);
                return;
            }

            if (difficulty > 0 && percent > learned) {
                gain_skill(ch, SKILL_PICK_LOCK, 1);
                send_to_char("You failed to pick the lock.\n\r", ch);

                if ((percent - learned) / 5 > picks->obj_flags.value[1]) {
                    if (--picks->obj_flags.value[1] <= 0) {
                        act("$p breaks in the attempt.", FALSE, ch, picks, NULL, TO_CHAR);
                        extract_obj(picks);
                    }
                }
                return;
            }
            REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
            if (EXIT(ch, door)->keyword)
                act("$n skillfully picks the lock of the $F.", 0, ch, 0, EXIT(ch, door)->keyword,
                    TO_ROOM);
            else
                act("$n picks the lock on the $F.", TRUE, ch, 0, (void*)dirs[door], TO_ROOM);

            send_to_char("The lock springs open with a soft click.\n\r", ch);

            if ((other_room = EXIT(ch, door)->to_room))
                if ((back = other_room->direction[(int) rev_dir[door]]))
                    if (back->to_room == ch->in_room)
                        REMOVE_BIT(back->exit_info, EX_LOCKED);
        }                       /* end of valid doors */
    } else {
        send_to_char("You don't see that here.\n\r", ch);
        return;
    }
}


void
skill_disarm(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    char tmp[256];
    int percent;
    int old_skill;
    OBJ_DATA *obj;
    OBJ_DATA *cobj;
    ROOM_DATA *drop_room = 0;
    int c_wtype, v_wtype;
    int pos = 0;
    int cpos = 0;
    int dir = 0;

#if defined(DISARM_TRAPS)
    arg = one_argument(arg, arg1);
    obj = (get_obj_in_list_vis(ch, arg1, ch->is_carrying));
    if (obj && (obj->obj_flags.type == ITEM_CONTAINER)) {
        act("You begin disarming $p.", TRUE, ch, obj, 0, TO_CHAR);
        act("$n begins disarming $p.", TRUE, ch, obj, 0, TO_ROOM);

        percent = (ch->skills[SKILL_TRAP]
                   && ch->skills[SKILL_TRAP]->learned) ? ch->skills[SKILL_TRAP]->learned : number(1,
                                                                                                  10);

        if ((percent >= number(1, 100)) &&      /* They succeed */
            (IS_SET(obj->obj_flags.value[1], CONT_TRAP))) {
            act("You disarm $p!", TRUE, ch, obj, 0, TO_CHAR);
            act("$n finishes attempting to disarm $p.", TRUE, ch, obj, 0, TO_ROOM);
            REMOVE_BIT(obj->obj_flags.value[1], CONT_TRAPPED);
            return TRUE;
        } else
            /* They biff, or no trap... how bad though? */
        {
            if (!number(0, 5) || !IS_SET(ch->skills.value[1], CONT_TRAP)) {


            } else {

            }
        }
        return TRUE;
    }
#endif

    /* #define DISARM_EP_OR_ES 1 */
#if defined(DISARM_EP_OR_ES)
    gamelog("I am using DISARM_EP_OR_ES.");
    arg = one_argument(arg, arg1);

    if (ch->in_room != tar_ch->in_room) {
        act("Disarm who?", TRUE, ch, 0, 0, TO_CHAR);
        return;
    }


    obj = (get_obj_in_list_vis(ch, arg1, tar_ch->is_carrying));

    obj = (OBJ_DATA *) 0;       /* re-initialize it */
    obj = get_ep_or_es(tar_ch, &pos);
    if (!obj) {
        act("$N isn't wielding a weapon.", TRUE, ch, 0, tar_ch, TO_CHAR);
        return;
    }

    if (is_char_ethereal(ch) != is_char_ethereal(tar_ch)) {
        act("But $E is on another plane entirely!", FALSE, ch, 0, tar_ch, TO_CHAR);
        return;
    }

    /* Too tired to do it. */
    /* Raesanos 4/19/2006 */
    if (GET_MOVE(ch) < 10) {
        send_to_char("You are too tired!\n\r", ch);
        return;
    }

    if (ch->specials.fighting && tar_ch == ch->specials.fighting)
        clear_disengage_request(ch);

    /* Stamina drain. */
    adjust_move(ch, -number(2, (100 - get_skill_percentage(ch, SKILL_DISARM)) / 22 + 3));

    /*  Can't disarm someone who is flying unless
     * you are flying as well or they can't see you  -  Kel  */
    if ((IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
         && !affected_by_spell(tar_ch, SPELL_FEATHER_FALL))
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
        && CAN_SEE(tar_ch, ch)) {
        act("You try, but $E rises overhead and evades you.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to disarm you, but you float overhead.", FALSE, ch, 0, tar_ch, TO_VICT);
        return;
    }

    percent = 0;

    /* Minus 30% if you aren't using a weapon */
    cobj = get_weapon(ch, &cpos);
    if (!cobj)
        percent -= 30;

    /* Plus 5% if they are wearing gloves for old times sake */
    if (tar_ch->equipment[WEAR_HANDS])
        percent += 5;

    /*  Bonus/penalty for knowledge of tar_ch's wtype  */
    v_wtype = get_weapon_type(obj->obj_flags.value[3]);
    percent += proficiency_bonus(ch, v_wtype) - proficiency_bonus(tar_ch, v_wtype);

    /*  Bonus/penalty for knowledge of ch's wtype  */
    if (cobj) {
        c_wtype = get_weapon_type(cobj->obj_flags.value[3]);
        if (v_wtype != c_wtype)
            percent += proficiency_bonus(ch, c_wtype) - proficiency_bonus(tar_ch, c_wtype);

    }

    if (pos == ETWO) {
        //detriment for victim using two hands 
        percent -= 10;

        // Detriment if opponent is using two-hands and have two-handed skill
        if( has_skill(tar_ch, SKILL_TWO_HANDED)) {
            percent -= get_skill_percentage(tar_ch, SKILL_TWO_HANDED) / 5;
        }
    }

    if (cobj && cpos == ETWO) {
        // bonus for disarmer using two hands 
        percent += 10;

        // Bonus if they are using two-hands and have two-handed skill
        if(has_skill(ch, SKILL_TWO_HANDED)) {
            percent += get_skill_percentage(ch, SKILL_TWO_HANDED) / 5;
        }
    }

    /* disarm skills compared */
    if (has_skill(ch, SKILL_DISARM))
        percent += get_skill_percentage(ch, SKILL_DISARM);
    if (has_skill(tar_ch, SKILL_DISARM))
        percent -= get_skill_percentage(tar_ch, SKILL_DISARM);

    /* agil reaction bonuses compared */
    percent += agl_app[GET_AGL(tar_ch)].reaction;
    percent -= agl_app[GET_AGL(ch)].reaction;

    /* look for mindwipe */
    if (affected_by_spell(ch, PSI_MINDWIPE))
        percent -= 25;
    if (affected_by_spell(tar_ch, PSI_MINDWIPE))
        percent += 25;

    /* You always have a 3% chance */
    percent = MAX(3, percent);

    /* If you don't have the skill you never have more than 10% chance */
    if (!ch->skills[SKILL_DISARM])
        percent = MAX(10, percent);

    /* Roll under your chance on a d100 */
    if (number(1, 100) < percent) {
        act("You knock $p from $N's hands.", TRUE, ch, obj, tar_ch, TO_CHAR);
        act("$n knocks $p from $N's hands.", FALSE, ch, obj, tar_ch, TO_NOTVICT);
        act("$N deftly knocks $p from your hands.", TRUE, ch, obj, tar_ch, TO_VICT);

        if ((GET_RACE(tar_ch) == RACE_MANTIS) &&        /*  Kel  */
            (ch->skills[SKILL_DISARM]->learned)
            && ((percent + 25) > ch->skills[SKILL_DISARM]->learned)) {
            act("$E snatches it out of the air with another arm!", TRUE, ch, obj, tar_ch, TO_CHAR);
            act("$N snatches it out of the air with another arm!", FALSE, ch, obj, tar_ch,
                TO_NOTVICT);
            send_to_char("You quickly snatch it out of the air.\n\r", ch);
            return;
        }

        obj_to_room(unequip_char(tar_ch, pos), tar_ch->in_room);
    } else {
        act("You attempt to knock the $o from $N's hands, but $E deftly avoids " "you.", TRUE, ch,
            obj, tar_ch, TO_CHAR);
        act("$n attempts to knock the $o from $N's hands, but $E deftly " "avoids $m.", FALSE, ch,
            obj, tar_ch, TO_NOTVICT);
        act("$n attempts to knock $p from your hands, but you deftly avoid $m.", TRUE, ch, obj,
            tar_ch, TO_VICT);
        /* 50/50 chance you'll improve if you have the skill */
        if (!number(0, 1))
            if (ch->skills[SKILL_DISARM])
                gain_skill(ch, SKILL_DISARM, 1);
    }

    if (!ch->specials.fighting)
        set_fighting(ch, tar_ch);
    if (!tar_ch->specials.fighting)
        set_fighting(tar_ch, ch);

#else
    obj = (OBJ_DATA *) 0;       /* re-initialize it */
    obj = get_weapon(tar_ch, &pos);
    if (!obj) {
        act("$N isn't wielding a weapon.", TRUE, ch, 0, tar_ch, TO_CHAR);
        return;
    }

    if (is_char_ethereal(ch) != is_char_ethereal(tar_ch)) {
        act("But $E is on another plane entirely!", FALSE, ch, 0, tar_ch, TO_CHAR);
        return;
    }

    /* Too tired to do it. */
    /* Raesanos 4/19/2006 */
    if (GET_MOVE(ch) < 10) {
        send_to_char("You are too tired!\n\r", ch);
        return;
    }

    /* Stamina drain. */
    adjust_move(ch, -number(2, (100 - get_skill_percentage(ch, SKILL_DISARM)) / 22 + 3));

    /*  Can't disarm someone who is flying unless
     * you are flying as well or they can't see you  -  Kel  */
    if ((IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
         && !affected_by_spell(tar_ch, SPELL_FEATHER_FALL))
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
        && CAN_SEE(tar_ch, ch)) {
        act("You try, but $E rises overhead and evades you.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to disarm you, but you float overhead.", FALSE, ch, 0, tar_ch, TO_VICT);
        return;
    }

    percent = 0;

    /* Minus 30% if you aren't using a weapon */
    cobj = get_weapon(ch, &cpos);
    if (!cobj)
        percent -= 30;

    /* Plus 5% if they are wearing gloves for old times sake */
    if (tar_ch->equipment[WEAR_HANDS])
        percent += 5;

    if (pos == ETWO) {
        //detriment for victim using two hands 
        percent -= 10;

        // Detriment if opponent is using two-hands and have two-handed skill
        if( has_skill(tar_ch, SKILL_TWO_HANDED)) {
            percent -= get_skill_percentage(tar_ch, SKILL_TWO_HANDED) / 5;
        }
    }

    if (cobj && cpos == ETWO) {
        // bonus for disarmer using two hands 
        percent += 10;

        // Bonus if they are using two-hands and have two-handed skill
        if(has_skill(ch, SKILL_TWO_HANDED)) {
            percent += get_skill_percentage(ch, SKILL_TWO_HANDED) / 5;
        }
    }

    /*  Bonus/penalty for knowledge of tar_ch's wtype  */
    v_wtype = get_weapon_type(obj->obj_flags.value[3]);
    percent += proficiency_bonus(ch, v_wtype) - proficiency_bonus(tar_ch, v_wtype);

    /*  Bonus/penalty for knowledge of ch's wtype  */
    if (cobj) {
        c_wtype = get_weapon_type(cobj->obj_flags.value[3]);
        if (v_wtype != c_wtype)
            percent += proficiency_bonus(ch, c_wtype) - proficiency_bonus(tar_ch, c_wtype);
    }

    /* disarm skills compared */
    if (ch->skills[SKILL_DISARM])
        percent += (ch->skills[SKILL_DISARM]->learned);
    if (tar_ch->skills[SKILL_DISARM])
        percent -= tar_ch->skills[SKILL_DISARM]->learned;

    /* agil reaction bonuses compared */
    percent += agl_app[GET_AGL(tar_ch)].reaction;
    percent -= agl_app[GET_AGL(ch)].reaction;

    /* look for mindwipe */
    if (affected_by_spell(ch, PSI_MINDWIPE))
        percent -= 25;
    if (affected_by_spell(tar_ch, PSI_MINDWIPE))
        percent += 25;

    /* You always have a 3% chance */
    percent = MAX(3, percent);

    /* If you don't have the skill you never have more than 10% chance */
    /* Changed from MAX to MIN so the above will actually be true. -Savak */
    if (!ch->skills[SKILL_DISARM])
        percent = MIN(10, percent);

#define NESS_DISARM_FLING
#ifdef NESS_DISARM_FLING
    /* Roll under your chance on a d100 */
    if (number(1, 100) < percent) {
        if (!((number(1, 100) < (percent / 2)) && (percent > 50))) {
            act("You knock $p from $N's hands.", TRUE, ch, obj, tar_ch, TO_CHAR);
            act("$n knocks $p from $N's hands.", FALSE, ch, obj, tar_ch, TO_NOTVICT);
            act("$n deftly knocks $p from your hands.", TRUE, ch, obj, tar_ch, TO_VICT);

            /*  Kel original. Small attempts to fix crashbug by Savak */
            if ((GET_RACE(tar_ch) == RACE_MANTIS)
                && (tar_ch->skills[SKILL_DISARM])
                && ((percent + 25) < tar_ch->skills[SKILL_DISARM]->learned)) {
                act("$E snatches it out of the air with another arm!", TRUE, ch, obj, tar_ch,
                    TO_CHAR);
                act("$N snatches it out of the air with another arm!", FALSE, ch, obj, tar_ch,
                    TO_NOTVICT);
                send_to_char("You quickly snatch it out of the air.\n\r", tar_ch);
            } else
                obj_to_room(unequip_char(tar_ch, pos), ch->in_room);

            if (!ch->specials.fighting)
                set_fighting(ch, tar_ch);
            if (!tar_ch->specials.fighting)
                set_fighting(tar_ch, ch);
            return;
        }

        dir = number(0, 5);
        if (ch->in_room->direction[dir]) {      /* Direction is possible */
            if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
                if ((!IS_SET(EXIT(ch, dir)->exit_info, EX_SECRET))
                    && (EXIT(ch, dir)->keyword)) {
                    sprintf(tmp,
                            "$n knocks $p from $N's hands and sends "
                            "it flying %s, bouncing off the closed %s.", dirs[dir], EXIT(ch,
                                                                                         dir)->
                            keyword);
                    act(tmp, FALSE, ch, obj, tar_ch, TO_NOTVICT);
                    sprintf(tmp,
                            "You knock $p from $N's hands and send "
                            " it flying %s, bouncing off the closed %s.", dirs[dir], EXIT(ch,
                                                                                          dir)->
                            keyword);
                    act(tmp, FALSE, ch, obj, tar_ch, TO_CHAR);
                    sprintf(tmp,
                            "$n deftly knocks $p from your hands and "
                            "sends it flying %s, bouncing off the closed %s.", dirs[dir], EXIT(ch,
                                                                                               dir)->
                            keyword);
                    act(tmp, FALSE, ch, obj, tar_ch, TO_VICT);
                }
            } else if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_FIRE_WALL)) {
                sprintf(tmp,
                        "$n knocks $p from $N's hands and sends "
                        "it flying %s through a wall of flames.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_NOTVICT);
                sprintf(tmp,
                        "You knock $p from $N's hands and send "
                        "it flying %s through a wall of flames.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_CHAR);
                sprintf(tmp,
                        "$n deftly knocks $p from your hands and sends "
                        "it flying %s through a wall of flames.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_VICT);
                drop_room = ch->in_room->direction[dir]->to_room;
            } else if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_SAND_WALL)) {
                sprintf(tmp,
                        "$n knocks $p from $N's hands and sends it "
                        "flying %s, bouncing off a wall of sand.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_NOTVICT);
                sprintf(tmp,
                        "You knock $p from $N's hands and send it"
                        " flying %s, bouncing off a wall of sand.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_CHAR);
                sprintf(tmp,
                        "$n deftly knocks $p from your hands and sends it "
                        "it flying %s, bouncing off a wall of sand.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_VICT);
            } else if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL)) {
                sprintf(tmp,
                        "$n knocks $p from $N's hands and sends it flying"
                        " %s before a sudden gust of wind blows it back.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_NOTVICT);
                sprintf(tmp,
                        "You knock $p from $N's hands and send it flying"
                        " %s before a sudden gust of wind blows it back.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_CHAR);
                sprintf(tmp,
                        "$n deftly knocks $p from your hands and sends"
                        "it flying %s before a sudden gust of wind blows " "it back.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_VICT);
            } else if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER)) {
                sprintf(tmp,
                        "$n knocks $p from $N's hands and sends it flying"
                        " %s through a mass of spinning blades.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_NOTVICT);
                sprintf(tmp,
                        "You knock $p from $N's hands and send it flying"
                        " %s through a mass of spinning blades.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_CHAR);
                sprintf(tmp,
                        "$n deftly knocks $p from your hands and sends "
                        "it flying %s through a mass of spinning blades.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_VICT);
                drop_room = ch->in_room->direction[dir]->to_room;
            } else if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_THORN_WALL)) {
                sprintf(tmp,
                        "$n knocks $p from $N's hands and sends it flying"
                        " %s, bouncing off a wall of thorns.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_NOTVICT);
                sprintf(tmp,
                        "You knock $p from $N's hands and send it flying"
                        " %s, bouncing off a wall of thorns.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_CHAR);
                sprintf(tmp,
                        "$n deftly knocks $p from your hands and "
                        "sends it flying %s, bouncing off a wall of thorns.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_VICT);
            } else {
                sprintf(tmp, "$n knocks $p from $N's hands and sends it flying" " %s.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_NOTVICT);
                sprintf(tmp, "You knock $p from $N's hands and send it flying %s.", dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_CHAR);
                sprintf(tmp, "$n deftly knocks $p from your hands and sends " "it flying %s.",
                        dirs[dir]);
                act(tmp, FALSE, ch, obj, tar_ch, TO_VICT);
                drop_room = ch->in_room->direction[dir]->to_room;
            }
        } else {
            act("You knock $p from $N's hands.", TRUE, ch, obj, tar_ch, TO_CHAR);
            act("$n knocks $p from $N's hands.", FALSE, ch, obj, tar_ch, TO_NOTVICT);
            act("$n deftly knocks $p from your hands.", TRUE, ch, obj, tar_ch, TO_VICT);
        }
#endif
        if (!drop_room)
            drop_room = tar_ch->in_room;
        obj_to_room(unequip_char(tar_ch, pos), drop_room);
    } else {
        if (tar_ch->skills[SKILL_DISARM] && GET_MOVE(tar_ch) >= 10) {
            if (((tar_ch->skills[SKILL_DISARM]->learned) - (percent)) > 25) {
                act("$N leads your attack and reverses your disarm attempt!", FALSE, ch, 0, tar_ch,
                    TO_CHAR);
                act("You lead $n's attempt to disarm you and reverse it.", FALSE, ch, 0, tar_ch,
                    TO_VICT);
                act("$n attempts to disarm $N, but finds $s attack reversed!", FALSE, ch, 0, tar_ch,
                    TO_NOTVICT);
                if (ch->skills[SKILL_DISARM]) {
                    old_skill = ch->skills[SKILL_DISARM]->learned;
                    ch->skills[SKILL_DISARM]->learned = 5;
                    skill_disarm(0, tar_ch, "", 0, ch, 0);
                    ch->skills[SKILL_DISARM]->learned = old_skill;
                } else
                    skill_disarm(0, tar_ch, "", 0, ch, 0);

                if (!ch->specials.fighting)
                    set_fighting(ch, tar_ch);
                if (!tar_ch->specials.fighting)
                    set_fighting(tar_ch, ch);
                return;
            }
        }

        if ((number(1, 10) < 2) && (cobj)) {
            act("You attempt to knock the $o from $N's hands, but fumble and "
                "drop your own weapon instead!", TRUE, ch, cobj, tar_ch, TO_CHAR);
            act("$n attempts to knock the $o from $N's hands, but fumbles and " "drops $s weapon!",
                TRUE, ch, cobj, tar_ch, TO_NOTVICT);
            act("$n attempts to knock $p from your hands, but $e fumbles and "
                "drops $s own weapon instead!", TRUE, ch, cobj, tar_ch, TO_VICT);
            obj_to_room(unequip_char(ch, cpos), ch->in_room);
        } else {
            act("You attempt to knock the $o from $N's hands, but $E " "deftly avoids you.", TRUE,
                ch, obj, tar_ch, TO_CHAR);
            act("$n attempts to knock the $o from $N's hands, but $E deftly " "avoids $m.", FALSE,
                ch, obj, tar_ch, TO_NOTVICT);
            act("$n attempts to knock $p from your hands, but you deftly avoid $m.", TRUE, ch, obj,
                tar_ch, TO_VICT);
        }
        /* 50/50 chance you'll improve if you have the skill */
        if (!number(0, 1))
            if (ch->skills[SKILL_DISARM])
                gain_skill(ch, SKILL_DISARM, 1);
    }

    if (!ch->specials.fighting)
        set_fighting(ch, tar_ch);
    if (!tar_ch->specials.fighting)
        set_fighting(tar_ch, ch);

#endif

}


void
skill_listen(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
}

void
cmd_listen(CHAR_DATA *ch, const char *arg, Command cmd, int count) {
    bool is_on = 0, turn_on = 0;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    for (; arg != NULL && *arg == ' ' && *arg != '\0'; arg++);

    if (IS_AFFECTED(ch, CHAR_AFF_LISTEN)) {
        is_on = 1;
    }

    if (arg != NULL && *arg != '\0') {
        if (!str_prefix(arg, "status")) {
            if (!is_on)
                send_to_char("You aren't currently listening.\n\r", ch);
            else
                send_to_char("You are currently listening.\n\r", ch);
            return;
        }
    
        if (!str_prefix(arg, "on")) {
            if (is_on) {
                send_to_char("You are already listening.\n\r", ch);
                return;
            }
            turn_on = 1;
        }
        else if (!str_prefix(arg, "off")) {
            if (!is_on) {
                send_to_char("You weren't listening.\n\r", ch);
                return;
            }
            turn_on = 0;
        } else {
            send_to_char("Syntax:  listen [on|off|status]\n\r", ch);
            return;
        }
    }

    if (turn_on == 0 && is_on) {         /* means turn off instead */
        affect_from_char(ch, SKILL_LISTEN);
        send_to_char("You stop listening.\n\r", ch);
        return;
    }

    af.type = SKILL_LISTEN;
    af.duration = (has_skill(ch, SKILL_LISTEN) ? ch->skills[SKILL_LISTEN]->learned / 10 + 1 : 1);
    af.magick_type = MAGICK_NONE;
    af.power = 0;
    af.location = CHAR_APPLY_STUN;
    af.modifier = -10 + round(7.0 * get_skill_percentage(ch, SKILL_LISTEN) / 100.0);
    af.bitvector = CHAR_AFF_LISTEN;
    affect_to_char(ch, &af);

    send_to_char("You start trying to listen.\n\r", ch);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}


void
skill_trap(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{
    OBJ_DATA *cont, *trap;
    byte percent, mode = 0;
    char direction[256], name[256], buf[256], buf1[256];
    int n = 0;
    OBJ_DATA *tmp_obj;
    int damage = 0, difficulty = 0;
    struct room_data *other_room;
    struct room_direction_data *back;
    int chance;
    bool breakLock = FALSE;



    /* 4/29/98 - Would be a good place to insert ranger traps/snares
     *           There is a tool_type of TOOL_SNARE, which would be good
     *           for ranger traps
     */

    if (!(trap = get_tool(ch, TOOL_TRAP)))
        if (!(trap = get_tool(ch, TOOL_FUSE))) {
            send_to_char("You'll need to hold a trap if you want to set one.\n\r", ch);
            /* no wait state */
            ch->specials.act_wait = 0;
            return;
        }
    /* Based on the type of tool it is, construct either a trap or a fuse */
    switch (trap->obj_flags.value[0]) {
    case TOOL_FUSE:
        arg = one_argument(arg, name, sizeof(name));

        if ((cont = get_obj_in_list_vis(ch, name, ch->carrying))) {
            percent = number(1, 101);
            if (trap->obj_flags.value[1])
                percent -= trap->obj_flags.value[1];
            if (affected_by_spell(ch, PSI_MINDWIPE))
                percent += 25;

            act("You set the fuse in $p, and light it.", FALSE, ch, cont, 0, TO_CHAR);
            act("$n sets a fuse in $p, and lights it.", TRUE, ch, cont, 0, TO_ROOM);

            /* They had the skill & were successful, set the thing to go off */
            if ((ch->skills[SKILL_TRAP])
                && (percent <= ch->skills[SKILL_TRAP]->learned)
                && (IS_SET(cont->obj_flags.value[1], CONT_TRAPPED))) {
                new_event(EVNT_DETONATE, (trap->obj_flags.value[1] * 60), 0, 0, cont, 0, 0);
            }
            extract_obj(trap);
        } else {
            arg = one_argument(arg, direction, sizeof(direction));
            if ((n = find_door(ch, name, direction)) >= 0) {
                /* found a door to trap */
                if (!IS_SET(EXIT(ch, n)->exit_info, EX_ISDOOR)) {
                    send_to_char("There's no such thing around to trap.\n\r", ch);
                    return;
                }
                if (exit_guarded(ch, n, CMD_TRAP))
                    return;
                if (!IS_SET(EXIT(ch, n)->exit_info, EX_CLOSED)) {
                    send_to_char("You'll have to close it first.\n\r", ch);
                    return;
                }
                if (EXIT(ch, n)->keyword) {
                    first_name(EXIT(ch, n)->keyword, buf);
                } else
                    sprintf(buf, "door");

                sprintf(buf1, "$n sets a fuse on the %s, and lights it.", buf);
                act(buf1, TRUE, ch, 0, 0, TO_ROOM);
                sprintf(buf1, "You set a fuse on the %s, and light it.", buf);
                act(buf1, FALSE, ch, 0, 0, TO_CHAR);

                percent = number(1, 101);
                if (trap->obj_flags.value[1])
                    percent -= trap->obj_flags.value[1];
                if (affected_by_spell(ch, PSI_MINDWIPE))
                    percent += 25;

                /* They had the skill, were successful... blow the lock */
                if ((ch->skills[SKILL_TRAP])
                    && (percent <= ch->skills[SKILL_TRAP]->learned)
                    && (IS_SET(EXIT(ch, n)->exit_info, EX_TRAPPED))) {
                    act("You step back from $F just before it explodes.", 0, ch, 0,
                        EXIT(ch, n)->keyword, TO_CHAR);

                    sprintf(buf1, "Smoke rises up from the %s after a small explosion.\n\r", buf);
                    send_to_room(buf1, ch->in_room);

                    REMOVE_BIT(EXIT(ch, n)->exit_info, EX_TRAPPED);

                    /* See if the lock blows open */
                    if (IS_SET(EXIT(ch, n)->exit_info, EX_DIFF_1))
                        difficulty = difficulty + 1;
                    if (IS_SET(EXIT(ch, n)->exit_info, EX_DIFF_2))
                        difficulty = difficulty + 2;
                    if (IS_SET(EXIT(ch, n)->exit_info, EX_DIFF_4))
                        difficulty = difficulty + 4;

                    chance = number(0, 100);
                    switch (difficulty) {
                    case 0:
                        breakLock = TRUE;
                        break;
                    case 1:    /* 15% */
                        if (chance >= 15)
                            breakLock = TRUE;
                        break;
                    case 2:    /* 30% */
                        if (chance >= 30)
                            breakLock = TRUE;
                        break;
                    case 3:    /* 45% */
                        if (chance >= 45)
                            breakLock = TRUE;
                        break;
                    case 4:    /* 60% */
                        if (chance >= 60)
                            breakLock = TRUE;
                        break;
                    case 5:    /* 75% */
                        if (chance >= 75)
                            breakLock = TRUE;
                        break;
                    case 6:    /* 90% */
                        if (chance >= 90)
                            breakLock = TRUE;
                        break;
                    case 7:
                    default:   /* Impossible or error */
                        breakLock = FALSE;
                        break;
                    }

                    if (breakLock)
                        REMOVE_BIT(EXIT(ch, n)->exit_info, EX_LOCKED);
                    /* now for trapping the other side, too */
                    if ((other_room = EXIT(ch, n)->to_room))
                        if ((back = other_room->direction[rev_dir[n]]))
                            if (back->to_room == ch->in_room) {
                                REMOVE_BIT(back->exit_info, EX_TRAPPED);
                                if (breakLock)
                                    REMOVE_BIT(back->exit_info, EX_LOCKED);
                                send_to_room(buf1, other_room);
                            }
                }
                extract_obj(trap);
            } else {
                send_to_char("Set a fuse where?\n\r", ch);
            }
        }
        return;

    case TOOL_TRAP:
        arg = one_argument(arg, name, sizeof(name));

        if ((cont = get_obj_in_list_vis(ch, name, ch->carrying))) {
            /* found an object to trap */
            if (cont->obj_flags.type != ITEM_CONTAINER) {
                send_to_char("You can only set a trap on a container.\n\r", ch);
                return;
            }
            if (!IS_SET(cont->obj_flags.value[1], CONT_CLOSEABLE)) {
                send_to_char("You have to be able to close the container.\n\r", ch);
                return;
            }
            if (!IS_SET(cont->obj_flags.value[1], CONT_CLOSED)) {
                act("[ Closing $p first. ]", FALSE, ch, cont, 0, TO_CHAR);
                MUD_SET_BIT(cont->obj_flags.value[1], CONT_CLOSED);
            }

            act("You set a trap in $p.", FALSE, ch, cont, 0, TO_CHAR);
            act("$n sets a trap in $p.", TRUE, ch, cont, 0, TO_ROOM);
            mode = 1;
        } else {
            arg = one_argument(arg, direction, sizeof(name));
            if ((n = find_door(ch, name, direction)) >= 0) {
                /* found a door to trap */
                if (!IS_SET(EXIT(ch, n)->exit_info, EX_ISDOOR)) {
                    send_to_char("There's no such thing around to trap.\n\r", ch);
                    return;
                }
                if (exit_guarded(ch, n, CMD_TRAP))
                    return;
                if (!IS_SET(EXIT(ch, n)->exit_info, EX_CLOSED)) {
                    send_to_char("You'll have to close it first.\n\r", ch);
                    return;
                }
                if (EXIT(ch, n)->keyword) {
                    first_name(EXIT(ch, n)->keyword, buf);
                    strcat(buf, ".");
                } else
                    sprintf(buf, "door.");
                sprintf(buf1, "$n sets a trap on the %s", buf);
                act(buf1, TRUE, ch, 0, 0, TO_ROOM);
                sprintf(buf1, "You set a trap on the %s", buf);
                act(buf1, FALSE, ch, 0, 0, TO_CHAR);
                mode = 2;
            } else {
                send_to_char("Set a trap where?\n\r", ch);
                return;
            }
        }
        if (!mode) {
            send_to_char("Trap what, where?\n\r", ch);
            return;
        }
        percent = number(1, 101);
        if (trap->obj_flags.value[1])
            percent -= trap->obj_flags.value[1];
        if (affected_by_spell(ch, PSI_MINDWIPE))
            percent += 25;

        if (!ch->skills[SKILL_TRAP]
            || (percent > ch->skills[SKILL_TRAP]->learned)) {
            act("The trap backfires, and explodes in your face!", FALSE, ch, tar_obj, 0, TO_CHAR);
            act("The trap backfires, and explodes in $n's face!", FALSE, ch, tar_obj, 0, TO_ROOM);

            damage = number(10, 20) + number(10, 20);
            /* add damage done by stuff in the obj */
            for (tmp_obj = trap->contains; tmp_obj; tmp_obj = tmp_obj->next_content) {
                /* More explosive stuff inside, set it off */
                if (tmp_obj->obj_flags.type == ITEM_TOOL)
                    damage += number(10, 20);
            }
            extract_obj(trap);

            /* destroy item sometimes */
            gain_skill(ch, SKILL_TRAP, 3);

/*    replaced with generic damage below... better way of doing it
   GET_HIT(ch) -= ((number(10, 20)) + (number(10, 20)));
   update_pos(ch);
 */
            generic_damage(ch, damage, 0, 0, number(10, 25) * number(1, 4));
            if (GET_POS(ch) == POSITION_DEAD) {
                act("$n falls headlong, and dies before hitting the ground.", FALSE, ch, 0, 0,
                    TO_ROOM);
                die(ch);
            }
            return;
        }
        if (mode == 1)
            MUD_SET_BIT(cont->obj_flags.value[1], CONT_TRAPPED);
        else {
            MUD_SET_BIT(EXIT(ch, n)->exit_info, EX_TRAPPED);
            /* now for trapping the other side, too */
            if ((other_room = EXIT(ch, n)->to_room))
                if ((back = other_room->direction[rev_dir[n]]))
                    if (back->to_room == ch->in_room)
                        MUD_SET_BIT(back->exit_info, EX_TRAPPED);
        }

        extract_obj(trap);
        break;
    }
}

void
skill_hunt(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{
    char buf[512], ch_command[MAX_INPUT_LENGTH];
    char buf2[256];
    int dir, percent, skill_level;
    struct room_data *to, *from;
    struct room_data *rm;
    int i, hours, counter = 0;
    int bloody = 0, track_perc, hour_check;
    char *message;
    struct time_info_data ellapsed_time;

    char *hour_message[] = {
        "Less than an hour",
        "A few hours",
        "Several hours",
        "Many hours",
        "\n"
    };

    if (!IS_NPC(ch) || (ch->desc)) {
        percent = number(1, 101);
        buf[0] = '\n';

        skill_level = ch->skills[SKILL_HUNT]
            ? ch->skills[SKILL_HUNT]->learned : 10;

        if (GET_RACE(ch) == RACE_DESERT_ELF && ch->in_room->sector_type != SECT_CITY)
            skill_level += 30;

        if (affected_by_spell(ch, PSI_MINDWIPE))
            skill_level -= 25;

        if (ch->specials.riding) {
            act("But you can hardly see the ground, mounted up on $r.", FALSE, ch, 0,
                ch->specials.riding, TO_CHAR);
            return;
        }

        rm = ch->in_room;

        message = find_ex_description("[HUNT_MESSAGE]", ch->in_room->ex_description, TRUE);
        if (message) {
            send_to_char(message, ch);
        }

        for (i = 0; i < MAX_TRACKS; i++) {
            if (rm->tracks[i].type == TRACK_NONE)
                continue;

            ellapsed_time = mud_time_passed(rm->tracks[i].time, time(0));
            hours = ellapsed_time.hours;

            hours += ellapsed_time.day * ZAL_HOURS_PER_DAY;
            hours += ellapsed_time.month * ZAL_HOURS_PER_DAY * ZAL_DAYS_PER_MONTH;
            hours +=
                ellapsed_time.year * ZAL_HOURS_PER_DAY * ZAL_DAYS_PER_MONTH * ZAL_MONTHS_PER_YEAR;

            sprint_attrib(rm->tracks[i].type, track_types, buf2);

            hour_check = MIN(hours, 3);
            hour_check = MAX(hour_check, 0);

            sprintf(buf, "%s ago a ", hour_message[hour_check]);

            if (rm->tracks[i].type == TRACK_WAGON) {
                sprintf(buf, "WAGON_TRACK: %s ago a WAGON dir= %d, " "weight=%d\n\r",
                        hour_message[hour_check], rm->tracks[i].track.wagon.dir,
                        rm->tracks[i].track.wagon.weight);
                /* Commenting out to reduce spam 
                 * gamelog(buf); */
                sprintf(buf, "There are wagon tracks leading %s.\n\r",
                        dirs[(int) rm->tracks[i].track.wagon.dir]);
                send_to_char(buf, ch);
            } else if (rm->tracks[i].type == TRACK_TENT) {
                sprintf(buf, "TENT TRACK: %s\n\r", hour_message[hour_check]);
                /* Commenting out to reduce spam 
                 * gamelog(buf); */
                sprintf(buf, "Someone recently used a tent here.\n\r");
                send_to_char(buf, ch);
            } else if (rm->tracks[i].type == TRACK_BATTLE) {
                sprintf(buf, "WAGON_TRACK: %s ago a BATTLE number = %d, " "\r\r",
                        hour_message[hour_check], rm->tracks[i].track.battle.number);
                /* commenting out to reduce spam
                 * gamelog(buf); */
                sprintf(buf, "There was a battle here recently.\n\r");
                send_to_char(buf, ch);
            } else if (rm->tracks[i].type == TRACK_FIRE) {
                sprintf(buf, "FIRE_TRACK: %s ago a FIRE number = %d, " "\r\r",
                        hour_message[hour_check], rm->tracks[i].track.fire.number);
                sprintf(buf, "Flames have raged across the ground here recently.\n\r");
                send_to_char(buf, ch);
            }
            /* Placeholder, eventually add pour and storm tracks 
             * end placeholder */

            else if ((rm->tracks[i].type == TRACK_SLEEP)
                     || (rm->tracks[i].type == TRACK_SLEEP_BLOODY)
                     || (rm->tracks[i].type == TRACK_SNEAK_BLOODY)
                     || (rm->tracks[i].type == TRACK_SNEAK_CLEAN)
                     || (rm->tracks[i].type == TRACK_SNEAK_GLOW)
                     || (rm->tracks[i].type == TRACK_RUN_BLOODY)
                     || (rm->tracks[i].type == TRACK_RUN_CLEAN)
                     || (rm->tracks[i].type == TRACK_RUN_GLOW)
                     || (rm->tracks[i].type == TRACK_WALK_BLOODY)
                     || (rm->tracks[i].type == TRACK_WALK_CLEAN)
                     || (rm->tracks[i].type == TRACK_WALK_GLOW)) {

                switch (rm->tracks[i].track.ch.race) {
                case RACE_AVANGION:
                case RACE_HUMAN:
                case RACE_MUL:
                case RACE_THRYZN:
                case RACE_VAMPIRE:
                case RACE_GHOUL:
                    strcat(buf, "humanoid");
                    break;
                case RACE_MEPHIT:
                case RACE_SUB_ELEMENTAL:
                    if (strcmp(GET_NAME(ch), "Gerrol") != 0)
                        strcat(buf, "short-strided humanoid");
                    else
                        strcat(buf, "humanoid");
                    break;
                case RACE_DWARF:
                    strcat(buf, "short-strided humanoid");
                    break;
                case RACE_GALITH:
                case RACE_HALF_GIANT:
                    strcat(buf, "large, long-strided humanoid");
                    break;
                    /* 
                     * Begin overhaul addition of greater & more varied track
                     * messages. Please keep additions in alphabetical order,
                     * for ease & convenience.  -Savak
                     */

                case RACE_AGOUTI:
                    strcat(buf, "small creature with clawed feet");
                    break;
                case RACE_ALATAL:
                    strcat(buf, "large, four-legged, four-toed creature");
                    break;
                case RACE_ANAKORE:
                    strcat(buf, "three-toed, thick-clawed creature");
                    break;
                case RACE_ANKHEG:
                    strcat(buf, "huge, insectoid-like worm");
                    break;
                case RACE_ARACH:
                    strcat(buf, "insectoid with eight sticky legprints");
                    break;
                case RACE_ASLOK:
                    strcat(buf, "massive worm with several spiked legs");
                    break;
                case RACE_AUROCH:
                    strcat(buf, "very large, four-legged hooved creature");
                    break;
                case RACE_BAHAMET:
                    strcat(buf, "very large, heavy four-legged creature");
                    break;
                case RACE_BALTHA:
                    strcat(buf, "splay-toed creature, with finger drags");
                    break;
                case RACE_BAT:
                    strcat(buf, "small, winged creature");
                    break;
                case RACE_BATMOUNT:
                    strcat(buf, "enormous, winged creature");
                    break;
                case RACE_BAMUK:
                    strcat(buf, "six-legged, pawed creature");
                    break;
                case RACE_SCRAB:
                case RACE_BIG_INSECT:
                case RACE_FIREANT:
                    strcat(buf, "medium-large insectoid");
                    break;
                case RACE_BRAXAT:
                    strcat(buf, "large claw-footed humanoid");
                    break;
                case RACE_CARRU:
                    strcat(buf, "large, four-legged claw-hooved creature");
                    break;
                case RACE_CHALTON:
                    strcat(buf, "six-legged, hooved creature");
                    break;
                case RACE_CHARIOT:
                    strcat(buf, "small wheeled vehicle");
                    break;
                case RACE_CHEOTAN:
                    strcat(buf, "medium-sized four-legged lizard");
                    break;
                case RACE_CILOPS:
                    strcat(buf, "medium-sized, many-legged insectoid worm");
                    break;
                case RACE_COCHRA:
                    strcat(buf, "tiny insectoid");
                    break;
                case RACE_GORMI:
                    strcat(buf, "small creature with a skittered stride");
                    break;
                case RACE_GNU:
                    strcat(buf, "heavy-gaited creature");
                    break;
                case RACE_CURR:
                case RACE_GORTOK:
                case RACE_WOLF:
                    strcat(buf, "four-legged, pawed creature");
                    break;
                case RACE_DEMON:
                    strcat(buf, "large, wickedly-clawed creature");
                    break;
                case RACE_DESERT_ELF:
                case RACE_ELF:
                case RACE_HALF_ELF:
                    strcat(buf, "long-strided humanoid");
                    break;
                    /* Added half-elves in with the elves..can't win for losing.   -Savak
                     */
                case RACE_DUJAT_WORM:
                    strcat(buf, "great worm-like creature");
                    break;
                case RACE_DURRIT:
                    strcat(buf, "two-legged, hooved creature");
                    break;
                case RACE_DUSKHORN:
                    strcat(buf, "medium-sized four-legged hooved creature");
                    break;
                case RACE_ERDLU:
                    strcat(buf, "medium-sized, two-legged taloned creature");
                    break;
                case RACE_ESCRU:
                    strcat(buf, "short-strided, four-legged hooved creature");
                    break;
                case RACE_GETH:
                    strcat(buf, "several-legged, small belly-dragging lizard");
                    break;
                case RACE_GHAATI:
                    strcat(buf, "two-legged, pawed humanoid");
                    break;
                case RACE_GHYRRAK:
                    strcat(buf, "medium-large creature with eight cane-like legs");
                    break;
                case RACE_GIANT:
                    strcat(buf, "gargantuan, long-strided humanoid");
                    break;
                case RACE_GIANT_SPI:
                    strcat(buf, "medium-large, eight-legged insectoid");
                    break;
                case RACE_GIMPKA_RAT:
                    strcat(buf, "small, long-tailed rodent");
                    break;
                case RACE_GITH:
                    strcat(buf, "claw-footed humanoid");
                    break;
                case RACE_GIZHAT:
                    strcat(buf, "heavy, short-strided creature");
                    break;
                case RACE_GOUDRA:
                    strcat(buf, "medium-small, four-legged clawed creature");
                    break;
                case RACE_GURTH:
                    strcat(buf, "compact, short-strided creature");
                    break;
                case RACE_GWOSHI:
                    strcat(buf, "medium-large, four-legged wide-pawed creature");
                    break;
                case RACE_HALFLING:
                    strcat(buf, "small, short-strided humanoid");
                    break;
                case RACE_HORSE:
                    strcat(buf, "large, four-legged hooved creature");
                    break;
                case RACE_INIX:
                    strcat(buf, "very large, four-legged lizard");
                    break;
                case RACE_JAKHAL:
                    strcat(buf, "short-strided, four-legged clawed lizard");
                    break;
                case RACE_JOZHAL:
                    strcat(buf, "small, three-clawed lizard");
                    break;
                case RACE_KALICH:
                    strcat(buf, "medium-sized, slender-pawed creature");
                    break;
                case RACE_KANK:
                case RACE_WAR_BEETLE:
                    strcat(buf, "large, several-legged insectoid");
                    break;
                case RACE_KIYET_LION:
                    strcat(buf, "large creature with clawed paws");
                    break;
                case RACE_RANTARRI:
                    strcat(buf, "large creature with clawed paws");
                    break;
                case RACE_KOLORI:
                    strcat(buf, "medium-small, two-legged & narrow-footed creature");
                    break;
                case RACE_KRYL:
                    strcat(buf, "medium-sized, heavy insectoid");
                    break;
                case RACE_MAGERA:
                    strcat(buf, "shambling humanoid");
                    break;
                case RACE_MANTIS:
                    strcat(buf, "two-legged insectoid");
                    break;
                case RACE_MEKILLOT:
                    strcat(buf, "gargantuan, heavy four-legged reptile");
                    break;
                case RACE_PLAINSOX:
                    strcat(buf, "medium-heavy hooved creature");
                    break;
                case RACE_QUIRRI:
                    strcat(buf, "medium-small, slender-pawed creature");
                    break;
                case RACE_RATLON:
                    strcat(buf, "heavy, two-legged taloned creature");
                    break;
                case RACE_RAVEN:
                    strcat(buf, "small bird");
                    break;
                case RACE_RITIKKI:
                    strcat(buf, "very small insectoid");
                    break;
                case RACE_RODENT:
                case RACE_TUSK_RAT:
                    strcat(buf, "small, four-legged narrow-footed creature");
                    break;
                case RACE_SAND_FLIER:
                    strcat(buf, "medium-sized, flat creature");
                    break;
                case RACE_SAND_RAPTOR:
                    strcat(buf, "medium-sized, two-legged reptile");
                    break;
                case RACE_SCORPION:
                    strcat(buf, "small, belly-dragging insectoid");
                    break;
                case RACE_SHIK:
                    strcat(buf, "small insectoid");
                    break;
                case RACE_SILTSTRIDER:
                    strcat(buf, "medium sized, many-legged insect");
                    break;
                case RACE_SILT_TENTACLE:
                    strcat(buf, "a large slithering thing");
                    break;
                case RACE_SKEET:
                    strcat(buf, "six-legged, belly-dragging clawed lizard");
                    break;
                case RACE_SLUG:
                    strcat(buf, "small, slime-trail depositing worm");
                    break;
                case RACE_SNAKE1:
                case RACE_SNAKE2:
                case RACE_GRETH:
                    strcat(buf, "very slender, slithering creature");
                    break;
                case RACE_SPIDER:
                    strcat(buf, "medium-small, eight-legged insectoid");
                    break;
                case RACE_SUNLON:
                case RACE_SUNBACK:
                    strcat(buf, "medium-large, two-legged lizard");
                    break;
                case RACE_TANDU:
                    strcat(buf, "small, four-legged hooved creature");
                    break;
                case RACE_TARANTULA:
                    strcat(buf, "large, eight-legged insectoid");
                    break;
                case RACE_TEMBO:
                    strcat(buf, "large, short-strided four-legged reptile");
                    break;
                case RACE_THLON:
                    strcat(buf, "heavy-set, clawed feline creature");
                    break;
                case RACE_THORNWALKER:
                    strcat(buf, "unidentifiable, rooted creature");
                    break;
                case RACE_TOAD:
                    strcat(buf, "medium-small web-footed creature");
                    break;
                case RACE_TREGIL:
                    strcat(buf, "small, light four-legged creature");
                    break;
                case RACE_TURAAL:
                    strcat(buf, "long, tiny four-legged creature");
                    break;
                case RACE_USELLIAM:
                    // Replaced at Naephet's request.
                    // -Nessalin Nov 01, 2003
                    //   strcat (buf, "perhaps ten sets of humanoid tracks");
                    strcat(buf, "large number of tracks");
                    break;
                case RACE_VESTRIC:
                    strcat(buf, "medium-small birdlike creature");
                    break;
                case RACE_WEZER:
                    strcat(buf, "large six-legged insectoid");
                    break;
                case RACE_WORM:
                    strcat(buf, "gargantuan worm-like creature");
                    break;
                case RACE_YOMPAR_LIZARD:
                    strcat(buf, "medium-large, short-strided tail-dragging lizard");
                    break;
                case RACE_ZOMBIE:
                    strcat(buf, "awkwardly shambling creature");
                    break;
                    /* End overhaul addition of greater & more varied track messages. -Savak
                     */
                default:
                    /* Took out using the name of the race as these are not
                     * as descriptive as an actual message.  Odds are if the
                     * code has reached this case, the track most likely is
                     * corrupt.  Better to be vague than to mislead players.
                     * -Tiernan 7/29/05 */
                    strcat(buf, "creature whose tracks are too vague to identify");
                    break;
                }

                track_perc = 0;

                switch (rm->tracks[i].type) {
                case TRACK_SLEEP:
                    track_perc = 85;
                    strcpy(buf2, " had been sleeping ");
                    break;
                case TRACK_SLEEP_BLOODY:
                    bloody = 1;
                    track_perc = 100;
                    strcpy(buf2, " had been sleeping ");
                    break;
                case TRACK_SNEAK_BLOODY:
                    bloody = 1;
                    track_perc = 50;
                    strcpy(buf2, " moved lightly ");
                    break;
                case TRACK_SNEAK_CLEAN:
                    track_perc = 30;
                    strcpy(buf2, " moved lightly ");
                    break;
                case TRACK_RUN_BLOODY:
                    bloody = 1;
                    track_perc = 100;
                    strcpy(buf2, " moved swiftly ");
                    break;
                case TRACK_RUN_CLEAN:
                    track_perc = 80;
                    strcpy(buf2, " moved swiftly ");
                    break;
                case TRACK_WALK_BLOODY:
                    bloody = 1;
                    track_perc = 80;
                    strcpy(buf2, " moved ");
                    break;
                case TRACK_WALK_CLEAN:
                    track_perc = 60;
                    strcpy(buf2, " moved ");
                    break;
                case TRACK_SNEAK_GLOW:
                    bloody = 2;
                    track_perc = 1000;
                    strcpy(buf2, " moved lightly ");
                    break;
                case TRACK_RUN_GLOW:
                    bloody = 2;
                    track_perc = 1000;
                    strcpy(buf2, " moved swiftly ");
                    break;
                case TRACK_WALK_GLOW:
                    bloody = 2;
                    track_perc = 1000;
                    strcpy(buf2, " moved ");
                    break;
                default:
                    track_perc = 60;
                    strcpy(buf2, " moved ");
                    break;
                }


                if ((rm->tracks[i].type == TRACK_SLEEP)
                    || (rm->tracks[i].type == TRACK_SLEEP_BLOODY)) {
                    strcat(buf, buf2);
                    strcat(buf, "here.\n\r");
                } else {        /* Normal tracks */
                    strcat(buf, buf2);
                    if (rm->tracks[i].track.ch.into)
                        strcat(buf, "in from the ");
                    else
                        strcat(buf, "to the ");

                    if ((rm->tracks[i].track.ch.dir >= 0)
                        && (rm->tracks[i].track.ch.dir < 6))
                        strcat(buf, dir_name[(int) rm->tracks[i].track.ch.dir]);
                    else
                        strcat(buf, "somewhere");

                    strcat(buf, ".\n\r");
                }               /* End else */

                track_perc += (ch->skills[SKILL_HUNT]) ? ch->skills[SKILL_HUNT]->learned : 0;

                if ((GET_RACE(ch) == RACE_DESERT_ELF)
                    && (ch->in_room->sector_type != SECT_CITY))
                    track_perc += 45;

                if (affected_by_spell(ch, PSI_MINDWIPE))
                    track_perc -= 25;

                percent = number(41, 141);      /* Do they find any tracks? */
                percent = (percent + (hours * 5));      /* Harder to find as they age. */

                /* Assassins & rangers need to be in their element */
                if (!TRACK_OK(ch)) {
                    track_perc /= 2;
                }

                if (percent > track_perc) {     /* They didnt see the track */
                    /* give them a chance every failed track, otherwise when they
                     * succeed on their own tracks, they'll hardly ever fail, 
                     * especially for desert elf rangers */
                    if (ch->skills[SKILL_HUNT])
                        gain_skill(ch, SKILL_HUNT, 1);
                    continue;
                }

                /* Tracks are too old, they have 'disappeared.' */
                if (hours > 5)
                    continue;

                /* finally, sending the message */
                send_to_char(buf, ch);

                if (bloody == 1) {
                    send_to_char("Blood rivulets line the tracks.\n\r", ch);
                    bloody = 0;
                }
                if (bloody == 2) {
                    send_to_char("A yellow glow illuminates the tracks.\n\r", ch);
                    bloody = 0;
                }
                counter = counter + 1;  /* We have at least one track now */
            }
        }

        if (counter == 0)       /* No tracks FOUND */
            send_to_char("You can't seem to find any tracks.\n\r", ch);
        else if (rm->tracks[0].type == TRACK_NONE)      /* There ARE no tracks */
            send_to_char("You can't seem to find any tracks.\n\r", ch);

        return;
    }




    if (!tar_ch) {              /*  Kel  */
        send_to_char("You can't seem to pick up the trail.\n\r", ch);
        return;
    }

    /* no fair hunting flyers... Ur */

    if (IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
        && (affected_by_spell(tar_ch, SPELL_FLY)
            || affected_by_spell(tar_ch, SPELL_LEVITATE))) {
        send_to_char("You can't seem to pick up the trail.\n\r", ch);
        return;
    }

    from = ch->in_room;
    to = tar_ch->in_room;

    if (ch->specials.riding) {
        act("But you can hardly see the ground, mounted up on $r.", FALSE, ch, 0,
            ch->specials.riding, TO_CHAR);
        return;
    }

    if (to == from) {
        send_to_char("But your quarry is already here!\n\r", ch);
        return;
    }

    percent = number(1, 101);

    if (ch->skills[SKILL_HUNT])
        skill_level = MAX(ch->skills[SKILL_HUNT]->learned / 10, 5);
    else
        skill_level = 5;

    /* ony NPCS's should get this far
     * if ( IS_NPC(ch))
     * else
     * doing this cause players should get direction info, mobiles get
     * the next command 
     * dir = choose_exit_name_for(from, to, ch_command, skill_level, 0);
     */

    dir = choose_exit_name_for(from, to, ch_command, skill_level, ch);

    if (dir == -1) {
        send_to_char("You can't seem to pick up the trail.\n\r", ch);
        return;
    }

    if (!ch->skills[SKILL_HUNT] || (percent > ch->skills[SKILL_HUNT]->learned)) {
        send_to_char("You can't seem to pick up the trail.\n\r", ch);
        if (ch->skills[SKILL_HUNT])
            gain_skill(ch, SKILL_HUNT, 1);
        return;
    }

    sprintf(buf, "It looks like they must have gone %s.\n\r", ch_command);
    send_to_char(buf, ch);

    if (ch->in_room == to)
        send_to_char("You have found your quarry!\n\r", ch);
    else
        parse_command(ch, ch_command);
    /*
     * only npc's should get this far 
     * if (IS_NPC(ch))
     * parse_command(ch, ch_command);
     */
    /* Taken out because people were using hunt to get past DMPL    */
    /* guards. If there's something wrong with making them actually */
    /* type the direction themselves, change it back.   -ness       */



}


/* This is such a big kludge, heh.  But, it works.  */
#ifdef AZBREW
void
skill_brew(byte level, CHAR_DATA * ch, const char *argument, int si, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{
    int i, obj_num, color_var, new, which, tmparray[3], herbarray[3];
    int colorarray[3];
    int herb1, herb2, herb3;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
    char arg4[MAX_INPUT_LENGTH];
    char oldarg[MAX_INPUT_LENGTH];
    char monitormsg[256];
    char msg[MAX_STRING_LENGTH];
    const char * const brew_what[] = {
        "tablet",
        "vial",
        "alcohol",
        "\n"
    };

    struct antidote_struct
    {
        int name;
        int first;
        int second;
        int third;
    };

    struct antidote_struct cure_recipes[] = {
        {POISON_GENERIC, HERB_COLD, HERB_BLAND, -1},
        {POISON_GRISHEN, HERB_CALMING, HERB_COLD, HERB_SWEET},
        {POISON_SKELLEBAIN, HERB_REFRESHING, HERB_CALMING, HERB_ACIDIC},
        {POISON_TERRADIN, HERB_REFRESHING, HERB_BLAND, HERB_SOUR},
        {POISON_PERAINE, HERB_HOT, HERB_SOUR, HERB_ACIDIC},
        {POISON_HERAMIDE, HERB_HOT, HERB_SOUR, HERB_SWEET},
        {-1, -1, -1, -1}
    };

    struct antidote_struct poison_recipes[] = {
        {POISON_GENERIC, HERB_HOT, HERB_ACIDIC, -1},
        {POISON_GRISHEN, HERB_CALMING, HERB_COLD, HERB_HOT},
        {POISON_SKELLEBAIN, HERB_BLAND, HERB_SOUR, HERB_SWEET},
        {POISON_TERRADIN, HERB_SOUR, HERB_SWEET, HERB_ACIDIC},
        {POISON_HERAMIDE, HERB_REFRESHING, HERB_CALMING, HERB_BLAND},
        {-1, -1, -1, -1}
    };


    OBJ_DATA *first_comp, *second_comp, *third_comp = NULL;
    OBJ_DATA *base_obj = NULL, *cure_obj;

    if (!ch->skills[SKILL_BREW]) {
        send_to_char("What?\n\r", ch);
        return;
    }

    strcpy(oldarg, argument);   /* Save a copy of argument */

    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
    argument = two_arguments(argument, arg3, sizeof(arg3), arg4, sizeof(arg4));

    if (!*arg1) {
        send_to_char("You can use \"brew\" with the following:\n\r", ch);
        for (new = 0; *brew_what[new] != '\n'; new++) {
            sprintf(msg, "     %s\n\r", brew_what[new]);
            send_to_char(msg, ch);
        }
        return;
    }

    /* First check if they are using a supported type of brew */
    which = search_block(arg1, brew_what, 0);
    switch (which) {
    case 0:                    /* tablet */
    case 1:                    /* vial */
        break;
    case 2:                    /* alcohol */
        send_to_char("Not implemented yet.  Sorry.\n\r", ch);
        return;
    default:
        send_to_char("That is not a valid brewing type.\n\r", ch);
        send_to_char("Type brew without any arguments for a list of valid" " types.\n\r", ch);
        return;
    }


    if (!*arg3 || !*arg2) {
        send_to_char("You need at least 2 items to brew together.\n\r", ch);
        return;
    }

    /* Find 3 objects that are herbs */
    if (!(first_comp = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
        send_to_char("You do not have that!\n\r", ch);
        return;
    }
    if (first_comp->obj_flags.type != ITEM_HERB) {
        send_to_char("That cannot be brewed.\n\r", ch);
        return;
    }
    /* Quick hack to ensure non-duplication of items */
    obj_from_char(first_comp);
    if (!(second_comp = get_obj_in_list_vis(ch, arg3, ch->carrying))) {
        send_to_char("You do not have that!\n\r", ch);
        obj_to_char(first_comp, ch);
        return;
    }
    if (second_comp->obj_flags.type != ITEM_HERB) {
        send_to_char("That cannot be brewed.\n\r", ch);
        obj_to_char(first_comp, ch);
        return;
    }
    obj_from_char(second_comp);
    if (*arg4) {
        if (!(third_comp = get_obj_in_list_vis(ch, arg4, ch->carrying))) {
            send_to_char("You do not have that!\n\r", ch);
            obj_to_char(first_comp, ch);
            obj_to_char(second_comp, ch);
            return;
        }
        if (third_comp->obj_flags.type != ITEM_HERB) {
            send_to_char("That cannot be brewed.\n\r", ch);
            obj_to_char(first_comp, ch);
            obj_to_char(second_comp, ch);
            return;
        }
    }
    obj_to_char(first_comp, ch);
    obj_to_char(second_comp, ch);


/* meat of the program.  Figure out what they are brewing, and brew it */
    switch (which) {
    case 0:                    /* tablet */
    case 1:                    /* vial */
        if (which == 1) {       /* vial - Need an empty vial to brew */
            for (base_obj = ch->carrying; base_obj; base_obj = base_obj->next_content)
                if (obj_index[base_obj->nr].vnum == 13248)
                    break;

            if (!base_obj) {
                send_to_char("You need an empty vial to brew it into.\n\r", ch);
                return;
            }
            if (base_obj->obj_flags.type != ITEM_DRINKCON) {
                gamelog("Error in skill_brew: Item 13248 is not a drink container.");
                send_to_char("Error in brew.  It has been logged.\n\r", ch);
                return;
            }
            if (base_obj->obj_flags.value[1]) {
                send_to_char("The vial is not empty!\n\r", ch);
                return;
            }
        }

        if (!ch->specials.to_process) {
            if (*arg4)
                sprintf(msg, "You begin to brew %s, %s, and %s into a %s.\n\r",
                        OSTR(first_comp, short_descr), OSTR(second_comp, short_descr),
                        OSTR(third_comp, short_descr), (which) ? "vial" : "tablet");
            else
                sprintf(msg, "You begin to brew %s and %s into a %s.\n\r",
                        OSTR(first_comp, short_descr), OSTR(second_comp, short_descr),
                        (which) ? "vial" : "tablet");
            send_to_char(msg, ch);
            if (*arg4)
                sprintf(msg, "$n begins to brew %s, %s, and %s into a %s.",
                        OSTR(first_comp, short_descr), OSTR(second_comp, short_descr),
                        OSTR(third_comp, short_descr), (which) ? "vial" : "tablet");
            else
                sprintf(msg, "$n begins to brew %s and %s into a %s.",
                        OSTR(first_comp, short_descr), OSTR(second_comp, short_descr),
                        (which) ? "vial" : "tablet");
            act(msg, FALSE, ch, 0, 0, TO_ROOM);
            add_to_command_q(ch, oldarg, CMD_BREW, 0, 0);
            return;
        }

        /* Ok, everything is in their inventory, and they have the skill.. lets */
        /* brew some anti-dotes! */

        herb1 = first_comp->obj_flags.value[0];
        herb2 = second_comp->obj_flags.value[0];
        if (*arg4)
            herb3 = third_comp->obj_flags.value[0];
        else
            herb3 = -1;

/* Load up the herb array with the herb_type values */
        herbarray[0] = herb1;
        herbarray[1] = herb2;
        herbarray[2] = herb3;

/* Load up the color array with the color_type values */
        colorarray[0] = first_comp->obj_flags.value[2];
        colorarray[1] = second_comp->obj_flags.value[2];
        if (*arg4)
            colorarray[2] = third_comp->obj_flags.value[2];
        else
            colorarray[2] = -1;
/*
   sprintf(msg, "Before sort:\n\r\tHerb1: %d, Herb2: %d; Herb3: %d.\n\r",
   herbarray[0], herbarray[1], herbarray[2]);
   send_to_char(msg, ch);
 */

        qsort(herbarray, 3, sizeof(herbarray[0]), intcmp);
        for (i = 0; i < 3; i++)
            tmparray[i] = herbarray[2 - i];
        for (i = 0; i < 3; i++)
            herbarray[i] = tmparray[i];

        /* Load up the object */

        if (*arg4)
            i = number(0, 2);
        else
            i = number(0, 1);

        color_var = colorarray[i];
/*      sprintf(msg, "Color_var is: %d.\n\r", color_var);
   send_to_char(msg, ch);  */
        obj_num = ((which) ? (13270 + color_var) : (13250 + color_var));

        cure_obj = read_object(obj_num, VIRTUAL);
        if (!cure_obj) {
            sprintf(msg, "Error in skill_brew.  Obj#%d not found.", obj_num);
            gamelog(msg);
            send_to_char("Error in brew skill.  It has been logged.\n\r", ch);
            return;
        }

        if (cure_obj->obj_flags.type != ITEM_CURE) {
            send_to_char("You produce only a useless smear of paste.\n\r", ch);
            extract_obj(cure_obj);
            return;
        }

        /* Do funky conversion to find sort the herbs, and find recipe */
        for (i = 0; cure_recipes[i].name != -1; i++) {
            if ((herbarray[0] == cure_recipes[i].first)
                && (herbarray[1] == cure_recipes[i].second)
                && ((herbarray[2] == cure_recipes[i].third)
                    || (cure_recipes[i].third == -1)))
                break;
        }

        if (cure_recipes[i].name == -1) {       /* No match */
            if (IS_IMMORTAL(ch))        /* Missed this one Az -Morg */
                send_to_char("No cure was found.\n\r", ch);
            for (i = 0; poison_recipes[i].name != -1; i++) {
                if ((herbarray[0] == poison_recipes[i].first)
                    && (herbarray[1] == poison_recipes[i].second)
                    && ((herbarray[2] == poison_recipes[i].third)
                        || (poison_recipes[i].third == -1)))
                    break;
            }
            if (poison_recipes[i].name == -1) { /* No match */
                if (IS_IMMORTAL(ch))
                    send_to_char("No poison match.\n\r", ch);
            } else {
                sprintf(msg, "Poison recipe found!  Poisons: %d.\n\r", poison_recipes[i].name);
                if (IS_IMMORTAL(ch))
                    send_to_char(msg, ch);
                cure_obj->obj_flags.value[2] = (poison_recipes[i].name - 219);
            }
        }

        else
            /* Cure found */
        {
            sprintf(msg, "Cure recipe found!  Cures: %d.\n\r", cure_recipes[i].name);
            if (IS_IMMORTAL(ch))
                send_to_char(msg, ch);

            cure_obj->obj_flags.value[0] = (cure_recipes[i].name - 219);
        }

        obj_to_char(cure_obj, ch);
        if (*arg4)
            sprintf(msg, "You mix %s, %s, and %s together.\n\r", OSTR(first_comp, short_descr),
                    OSTR(second_comp, short_descr), OSTR(third_comp, short_descr));
        else
            sprintf(msg, "You mix %s and %s together.\n\r", OSTR(first_comp, short_descr),
                    OSTR(second_comp, short_descr));
        send_to_char(msg, ch);
        if (*arg4)
            sprintf(msg, "$n mixes %s, %s, and %s together.", OSTR(first_comp, short_descr),
                    OSTR(second_comp, short_descr), OSTR(third_comp, short_descr));
        else
            sprintf(msg, "$n mixes %s and %s together.", OSTR(first_comp, short_descr),
                    OSTR(second_comp, short_descr));
        act(msg, FALSE, ch, 0, 0, TO_ROOM);
        act("You make $p.", FALSE, ch, cure_obj, 0, TO_CHAR);
        act("$n makes $p.", FALSE, ch, cure_obj, 0, TO_ROOM);

        if (cure_obj) {
            sprintf(monitormsg, "%s brews %s.\n\r", MSTR(ch, name), OSTR(cure_obj, short_descr));
            send_to_monitor(ch, monitormsg, MONITOR_CRAFTING);
        }

        if (which == 1)

            extract_obj(base_obj);
        extract_obj(first_comp);
        extract_obj(second_comp);
        if (*arg4)
            extract_obj(third_comp);
        break;
    case 2:                    /* alcohol */
        send_to_char("Currently not working yet.  Sorry.\n\r", ch);
        return;
    default:
        send_to_char("That is not a valid brewing type.\n\r", ch);
        send_to_char("Type brew without any arguments for a list of valid" " types.\n\r", ch);
        return;
    }

    return;
}
#endif /* AZBREW */

/* This is the revised brewing skill, put in by Sanvean.       */
/* They can brew the following object types: tablet, bandage,  */
/* alcohol, vial and powder.  To do alcohol, they need fruit,  */
/* a container, and sometimes spices/honey.  To do anything    */
/* else, they need herbs and in some cases, the basic bandage  */
/* item (if doing bandage) or a vial (if doing vial).  The     */
/* results are calculated from the herb values: color and      */
/* affect.                                                     */

#ifdef SANBREW
void
skill_brew(byte level, CHAR_DATA * ch, const char *argument, int j, CHAR_DATA * tar_ch,
           OBJ_DATA * tar_obj)
{
    int i;
    int obj_num;
    int firstcolor, secondcolor, thirdcolor, finalcolor;
/* used in calculating herb/product color */
    int firstaffect, secondaffect, thirdaffect, finalaffect, new;
/* used in calculating herb/product affect */
    int which;
    int brewproduct[10][13];    /* yeah, yeah, it's a 2D array. Sorry. */
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
    char arg4[MAX_INPUT_LENGTH];
    char oldarg[MAX_INPUT_LENGTH];
    char msg[MAX_STRING_LENGTH];
    const char * const brew_what[] = {
        "alcohol",
        "tablet",
        "vial",
        "bandage",
        "powder",
        "paint",
        "/n"
    };

/*  Check to make sure they actually have the skill.  */

    if (!ch->skills[SKILL_BREW]) {
        send_to_char("What?  You seem to lack that knowledge.\n\r", ch);
        return;
    }

/*  Save a copy of the argument.                       */

    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
    argument = two_arguments(argument, arg3, sizeof(arg3), arg4, sizeof(arg4));

/*  Let them know what sort of stuff they can brew, if */
/*  they've just typed brew by itself.                 */

    if (!*arg1) {
        send_to_char("You can use \"brew\" with the following:\n\r", ch);
        for (new = 0; *brew_what[new] != '\n'; new++) {
            sprintf(msg, "     %s\n\r", brew_what[new]);
            send_to_char(msg, ch);
        }
        return;
    }

/* First check if they are using a supported type of brew  */
/* and tell them how to find out what they can brew.       */

    which = search_block(arg1, brew_what, 0);
    switch (which) {
    case 0:                    /* alcohol */
        send_to_char("Not implemented yet.  Sorry.\n\r", ch);
        return;
        break;
    case 1:                    /*tablet */
        send_to_char("Not implemented yet.  Sorry.\n\r", ch);
        return;
        break;
    case 2:                    /* vial */
        send_to_char("Not implemented yet.  Sorry.\n\r", ch);
        return;
        break;
    case 3:                    /* bandage */
        send_to_char("Not implemented yet.  Sorry.\n\r", ch);
        return;
        break;
    case 4:                    /* powder */
        send_to_char("Not implemented yet.  Sorry.\n\r", ch);
        return;
        break;
    case 5:                    /* paint */
    default:
        send_to_char("That seems like an unlikely concoction.\n\r", ch);
        send_to_char("Type the word brew by itself for a list of the"
                     " sort of things you can brew.\n\r", ch);
        return;
    }

    /* Find 2 or 3 objects that are herbs */

    if (!(first_comp = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
        send_to_char("You do not have that, and hence cannot use it. \n\r", ch);
        return;
    }

    /* Note1: they'll need to be able to use minerals and        */
    /* components here later on.       -Sanvean                  */
    /* Note2: Also powders.                                      */
    /* Note3: ethereal people should not be able to brew.        */

    if ((first_comp->obj_flags.type != ITEM_HERB)
        && (first_comp->obj_flags.type != ITEM_FOOD)
        && (first_comp->obj_flags.type != ITEM_DRINK)
        && (first_comp->obj_flags.type != ITEM_SPICE)) {
        send_to_char("That cannot be brewed.\n\r", ch);
        return;
    }

    /* Quick hack to ensure non-duplication of items             */
    /* Simply yanked from Az's code.                             */

    obj_from_char(first_comp);
    if (!(second_comp = get_obj_in_list_vis(ch, arg3, ch->carrying))) {
        send_to_char("You do not have that, and hence cannot use it. \n\r", ch);
        obj_to_char(first_comp, ch);
        return;
    }
    if ((second_comp->obj_flags.type != ITEM_HERB)
        && (second_comp->obj_flags.type != ITEM_FOOD)
        && (second_comp->obj_flags.type != ITEM_DRINK)
        && (second_comp->obj_flags.type != ITEM_SPICE)) {
        send_to_char("That cannot be brewed.\n\r", ch);
        obj_to_char(first_comp, ch);
        return;
    }
    obj_from_char(second_comp);
    if (*arg4) {
        if (!(third_comp = get_obj_in_list_vis(ch, arg4, ch->carrying))) {
            send_to_char("You do not have that, and hence cannot use it.\n\r", ch);
            obj_to_char(first_comp, ch);
            obj_to_char(second_comp, ch);
            return;
        }
        if ((third_comp->obj_flags.type != ITEM_HERB)
            && (third_comp->obj_flags.type != ITEM_FOOD)
            && (third_comp->obj_flags.type != ITEM_DRINK)
            && (third_comp->obj_flags.type != ITEM_SPICE)) {
            send_to_char("That cannot be brewed.\n\r", ch);
            obj_to_char(first_comp, ch);
            obj_to_char(second_comp, ch);
            return;
        }
    }
    obj_to_char(first_comp, ch);
    obj_to_char(second_comp, ch);

    /* Okay, now we get to the point where we separate  */
    /* according to what sort of thing they wanna brew. */

    switch (which) {
    case 0:                    /* alcohol */
        if (which == 1) {       /* Need a drink container to brew */
            for (base_obj = ch->carrying; base_obj; base_obj = base_obj->next_content)
                if (!base_obj) {
                    send_to_char("You need a container to brew it into.\n\r", ch);
                    return;
                }
            if (base_obj->obj_flags.type != ITEM_DRINKCON) {
                send_to_char("You need a container to produce your alcohol.\n\r", ch);
                return;
            }
            if (base_obj->obj_flags.value[1]) {
                send_to_char("The container is already full!\n\r", ch);
                return;
            }
        }
    case 1:                    /* tablet */
        return;
    case 2:                    /* vial */
        if (which == 1) {       /* Need an empty vial to brew */
            for (base_obj = ch->carrying; base_obj; base_obj = base_obj->next_content)
                if (obj_index[base_obj->nr].vnum == 13248)
                    break;

            if (!base_obj) {
                send_to_char("You need an empty vial to brew it into.\n\r", ch);
                return;
            }
            if (base_obj->obj_flags.type != ITEM_DRINKCON) {
                gamelog("Error in skill_brew: Item is not a drink container.");
                send_to_char("Error in brew.  It has been logged.\n\r", ch);
                return;
            }
            if (base_obj->obj_flags.value[1]) {
                send_to_char("The vial is not empty!\n\r", ch);
                return;
            }
        }
    case 3:                    /* bandage */
        if ((base_obj->obj_flags.type != ITEM_TOOL)
            && (base_obj->obj_flags.value[0] != 5))
            send_to_char("You need some cloth to create a bandage.\n\r", ch);
        else
            return;
    case 4:                    /* powder */
        return;
    case 5:                    /* paint */
        if (which == 1) {       /* Need a drink container to brew */
            for (base_obj = ch->carrying; base_obj; base_obj = base_obj->next_content)
                if (!base_obj) {
                    send_to_char("You need a container to brew it into.\n\r", ch);
                    return;
                }
            if (base_obj->obj_flags.type != ITEM_DRINKCON) {
                send_to_char("You need a container to produce your paint.\n\r", ch);
                return;
            }
            if ((base_obj->obj_flags.value[1])
                && (base_obj->obj_flags.value[3] != 0)) {
                send_to_char("What sort of mess are you trying to produce?  Use water.\n\r", ch);
                return;
            }
        }
    default:
        gamelog("Error in skill_brew: attempting to brew invalid type.");
        send_to_char("Error in brew.  It has been logged.\n\r", ch);
        return;
    }
}

/* Now then.  Start brewing.                                  */
/* If they're trying to produce alcohol, they can be doing    */
/* any of the following: fruit into wine, bread or roots into */
/* ale, wine into brandy, honey into mead, ale into spiced or */
/* honied ale, wine into spiced or honied wine, brandy into   */
/* spiced or honied brandy.                                   */
/* If the itemtype is food			              */
/* switch on ftype					      */
/* case fruit					              */
/* case tuber						      */
/* case grain 						      */
/* case honey                                                 */
/* default					              */
/* store newliquidtype					      */
/* if drinkcontainer != empty                                 */
/* check oldliquidtype against newliquidtype		      */
/* if oldliquidtype != newliquidtype			      */
/*    newliquidtype = crapwater                               */
/* sendtochar "Your futile efforts produce a noisome liquid." */
/* else                                                       */
/* add liquidtype to drink container  			      */
/* sendtochar "You concoct a small amount of" newliquidtype   */
/* Note: remember to check delay.			      */

/* If the item is drinkcontainer		              */
/* switch on liquidtype					      */
/* case ale                                                   */
/*     if adding honey                                        */
/*     if adding spice                                        */
/* case wine (all kinds) (note for later, wine varieties)     */
/*     if adding honey                                        */
/*     if adding spice                                        */
/* case brandy						      */
/*     if adding honey                                        */
/*     if adding spice                                        */
/* Note for later, spiced whiskey                             */
/* default					              */
/* if drinkcontainer != empty                                 */
/* check oldliquidtype against newliquidtype		      */
/* if oldliquidtype != newliquidtype			      */
/*    newliquidtype = crapwater                               */
/* sendtochar "Your futile efforts produce a noisome liquid." */
/* else                                                       */
/* add liquidtype to drink container  			      */
/* store new liquidtype				  	      */

/* If they're trying to create a tablet                       */
/* check to make sure they have two or more herbs             */
/* get the firstcolor and firstaffect                         */
/* get the secondcolor and second affect                      */
/* calculate finalcolor and finalaffect                       */
/* if three, get thirdcolor and thirdaffect                   */
/* calculate finalcolor and finalaffect again                 */

/* If they're trying to create a vial                         */
/* check to make sure they have two or more herbs             */
/* get the firstcolor and firstaffect                         */
/* get the secondcolor and second affect                      */
/* calculate finalcolor and finalaffect                       */
/* if three, get thirdcolor and thirdaffect                   */
/* calculate finalcolor and finalaffect again                 */

/* If they're trying to create a bandage                      */
/* check to make sure they have two or more herbs             */
/* get the firstcolor and firstaffect                         */
/* get the secondcolor and second affect                      */
/* calculate finalcolor and finalaffect                       */
/* if three, get thirdcolor and thirdaffect                   */
/* calculate finalcolor and finalaffect again                 */

/* If they're trying to create a powder                       */
/* check to make sure they have two or more herbs             */
/* get the firstcolor and firstaffect                         */
/* get the secondcolor and second affect                      */
/* calculate finalcolor and finalaffect                       */
/* if three, get thirdcolor and thirdaffect                   */
/* calculate finalcolor and finalaffect again                 */

#endif /* SANBREW */

void
skill_poisoning(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
                OBJ_DATA * tar_obj)
{
    bool failed = FALSE;
    OBJ_DATA *p_obj;

    if (!ch->skills[SKILL_POISON]) {
        send_to_char("You have no knowledge of poisons.\n\r", ch);
        return;
    }

    if (!(p_obj = ch->equipment[ES])) {
        send_to_char("You aren't holding a poison.\n\r", ch);
        return;
    }

    if (p_obj->obj_flags.type != ITEM_POISON) {
        act("$p is not a poison.", FALSE, ch, p_obj, 0, TO_CHAR);
        return;
    }


    if (obj_guarded(ch, tar_obj, CMD_POISONING))
        return;

    if ((tar_obj->obj_flags.type != ITEM_WEAPON)
        && (tar_obj->obj_flags.type != ITEM_DART)
        && (tar_obj->obj_flags.type != ITEM_FOOD)
        && (tar_obj->obj_flags.type != ITEM_SPICE)
        && (tar_obj->obj_flags.type != ITEM_DRINKCON)) {
        act("You cannot poison that.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (tar_obj->obj_flags.type == ITEM_WEAPON) {
        if (((tar_obj->obj_flags.value[3] + 300) == TYPE_CHOP)
            || ((tar_obj->obj_flags.value[3] + 300) == TYPE_BLUDGEON)) {
            act("You cannot poison that type of weapon.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
/* use SIZE here!!! */
        if (tar_obj->obj_flags.weight > 600) {
            act("That weapon is too large to poison.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (IS_SET(tar_obj->obj_flags.extra_flags, OFL_POISON)) {
            act("That weapon is already poisoned.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        if (affected_by_spell(ch, PSI_MINDWIPE))
            failed = (number(25, 125) > ch->skills[SKILL_POISON]->learned);
        else
            failed = (number(1, 100) > ch->skills[SKILL_POISON]->learned);

        if (failed) {
            if (!does_save(ch, SAVING_PARA, number(0, GET_AGL(ch)))) {
                poison_char(ch, p_obj->obj_flags.value[0], 4, 0);
                act("You slip and cut yourself!", FALSE, ch, 0, 0, TO_CHAR);
                act("$n tries to poison $p but cuts $mself.", FALSE, ch, tar_obj, 0, TO_ROOM);
            } else {
                act("You slip and nearly cut yourself!", FALSE, ch, 0, 0, TO_CHAR);
                act("$n tries to poison $p and nearly cuts $mself.", FALSE, ch, tar_obj, 0,
                    TO_ROOM);
            }

            gain_skill(ch, SKILL_POISON, 3);

        } else {
            act("You coat $p with poison.", FALSE, ch, tar_obj, 0, TO_CHAR);
            act("$n coats $p with poison.", FALSE, ch, tar_obj, 0, TO_ROOM);
            MUD_SET_BIT(tar_obj->obj_flags.extra_flags, OFL_POISON);
            tar_obj->obj_flags.value[5] = p_obj->obj_flags.value[0];
        }
    } else {
        act("Feeling vicious, you poison $p.", FALSE, ch, tar_obj, 0, TO_CHAR);
        act("$n does something to $p.", FALSE, ch, tar_obj, 0, TO_ROOM);
        if (tar_obj->obj_flags.type == ITEM_DRINKCON) {
            tar_obj->obj_flags.value[5] = p_obj->obj_flags.value[0];
            //        MUD_SET_BIT( tar_obj->obj_flags.value[3], LIQFL_POISONED );
        } else
            tar_obj->obj_flags.value[3] = p_obj->obj_flags.value[0];
    }

    extract_obj(unequip_char(ch, ES));
}


void
skill_sap(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
          OBJ_DATA * tar_obj)
{
    int dam, chance = 0, wpos = 0;
    OBJ_DATA *wielded;
    OBJ_DATA *tmp1, *tmp2, *tmp3;
    char buf[MAX_STRING_LENGTH];

    if (IS_CALMED(ch)) {
        send_to_char("You feel too at ease right now to attack anyone.\n\r", ch);
        return;
    }

    /* Too tired to do it. */
    /* Morgenes 4/19/2006 */
    if (GET_MOVE(ch) < 10) {
        send_to_char("You are too tired!\n\r", ch);
        return;
    }

    if (ch->specials.riding) {
        send_to_char("You can't possibly do this while mounted.\n\r", ch);
        return;
    }

    if (ch->specials.fighting) {
        clear_disengage_request(ch);
        send_to_char("You are fighting for your life!\n\r", ch);
        return;
    }

    wielded = get_weapon(ch, &wpos);
    if (!wielded) {
        send_to_char("You need to wield a weapon to make it a success.\n\r", ch);
        return;
    }

    if (wielded->obj_flags.value[3] != (TYPE_BLUDGEON - 300)) {
        send_to_char("Only bludgeoning weapons can be used for " "sapping.\n\r", ch);
        return;
    }

    if (is_char_ethereal(ch) != is_char_ethereal(tar_ch)) {
        act("Your strike passes right through $M!", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n strikes at you, but it passes harmlessly through you.", FALSE, ch, 0, tar_ch,
            TO_VICT);
        act("$n strikes at $N, but ends up swinging at air.", FALSE, ch, 0, tar_ch, TO_NOTVICT);
        return;
    }

    /*  Can't BS someone who is flying unless you are flying as well or 
     * they can't see u  -  Kel  */
    if ((IS_AFFECTED(tar_ch, CHAR_AFF_FLYING)
         && !affected_by_spell(tar_ch, SPELL_FEATHER_FALL))
        && (!IS_AFFECTED(ch, CHAR_AFF_FLYING)
            || (IS_AFFECTED(ch, CHAR_AFF_FLYING)
                && affected_by_spell(ch, SPELL_FEATHER_FALL)))
        && CAN_SEE(tar_ch, ch)) {
        act("You try, but $E rises overhead and evades you.", FALSE, ch, 0, tar_ch, TO_CHAR);
        act("$n tries to sneak up behind you, but you easily rise overhead.", FALSE, ch, 0, tar_ch,
            TO_VICT);
        return;
    }

    /* have to fake out ws_damage() with the weapon */
    tmp1 = ch->equipment[EP];
    tmp2 = ch->equipment[ES];
    tmp3 = ch->equipment[ETWO];
    if (tmp1)
        obj_to_char(unequip_char(ch, EP), ch);
    if (tmp2)
        obj_to_char(unequip_char(ch, ES), ch);
    if (tmp3)
        obj_to_char(unequip_char(ch, ETWO), ch);

    /* one of them = wielded */
    obj_from_char(wielded);
    equip_char(ch, wielded, EP);

    chance = (ch->skills[SKILL_SAP] ? ch->skills[SKILL_SAP]->learned : 1);

    chance += ((GET_AGL(ch) - GET_AGL(tar_ch)) * 3);

    if ((GET_POS(tar_ch) > POSITION_SLEEPING)
        && (GET_POS(tar_ch) < POSITION_FIGHTING))
        chance += (15 * (POSITION_STANDING - GET_POS(tar_ch)));

    if (GET_POS(tar_ch) == POSITION_FIGHTING)
        chance = chance / 5;

    if (affected_by_spell(ch, PSI_MINDWIPE))
        chance -= 25;

    // if the target cant' see them
    if (!CAN_SEE(tar_ch, ch)) {
        // If the target is actively listening
        if (IS_AFFECTED(tar_ch, CHAR_AFF_LISTEN)
          && !tar_ch->specials.fighting
          && AWAKE(tar_ch)
          && !IS_AFFECTED(tar_ch, CHAR_AFF_DEAFNESS)
          && has_skill(tar_ch, SKILL_LISTEN)) {
            // opposed skill check
            if (number(1, get_skill_percentage(tar_ch, SKILL_LISTEN))
                > number(1, chance)) {
                chance -= 25;
            } else {
                // listener failed
                gain_skill(tar_ch, SKILL_LISTEN, 1);
                chance += 15;
            }
        }
        else {
            chance += 25;
        }
    }
    /* skill penalty if target is watching ch */
    else if (is_char_actively_watching_char(tar_ch, ch)) {
        chance -= get_watched_penalty(tar_ch);
    }
    // can see but not watching, give listen a chance
    else if (IS_AFFECTED(tar_ch, CHAR_AFF_LISTEN)
      && !tar_ch->specials.fighting
      && AWAKE(tar_ch)
      && !IS_AFFECTED(tar_ch, CHAR_AFF_DEAFNESS)
      && has_skill(tar_ch, SKILL_LISTEN)) {
        if (number(1, get_skill_percentage(tar_ch, SKILL_LISTEN))
            > number(1, chance)) {
            chance -= 25;
        } else {
            gain_skill(tar_ch, SKILL_LISTEN, 1);
        }
    }

    chance = MIN(chance, 100);

    if (IS_AFFECTED(tar_ch, CHAR_AFF_SUBDUED))  /* NEVER EVER fail */
        chance = 101;

    if (GET_POS(tar_ch) <= POSITION_SLEEPING)   /* never fail */
        chance = 101;

    /* Stamina drain. */
    adjust_move(ch, -number(2, (100 - get_skill_percentage(ch, SKILL_SAP)) / 22 + 3));

    /* send the message to the monitors */
    sprintf(buf, "%s saps %s.\n\r", MSTR(ch, name), MSTR(tar_ch, name));
    send_to_monitor(ch, buf, MONITOR_FIGHT);
    send_to_monitor(tar_ch, buf, MONITOR_FIGHT);
    shhlogf("%s: %s", __FUNCTION__, buf);

    if (number(1, 101) > chance) {
        ws_damage(ch, tar_ch, 0, 0, 0, 0, SKILL_SAP, 0);
        gain_skill(ch, SKILL_SAP, number(1, 3));
    } else {
        if (guard_check(ch, tar_ch))
            return;

        dam =
            dice(wielded->obj_flags.value[1],
                 wielded->obj_flags.value[2]) + wielded->obj_flags.value[0];

        if (ch->skills[SKILL_SAP])
            dam *= (1 + (ch->skills[SKILL_SAP]->learned / 10));
        else
            dam *= 2;

        dam += str_app[GET_STR(ch)].todam;

        if (wpos == ES)
            dam = MAX(1, (dam / 2));
        else if (wpos == ETWO)
            dam += number(3, MAX(str_app[GET_STR(ch)].todam, 4));

        dam += race[(int) GET_RACE(ch)].attack->damplus;
        dam = MIN(dam, 100);
        /* Debug message -Tiernan 1/19/02
         * sprintf(buf,"%s sapped %s for %d damage before bonuses", MSTR( ch, name ),
         * MSTR( tar_ch, name ), dam);
         * gamelog( buf );
         */
        ws_damage(ch, tar_ch, dam, 0, 0, dam, SKILL_SAP, 0);
    }

    /* now to put it all back */
    obj_to_char(unequip_char(ch, EP), ch);
    if (tmp1) {
        obj_from_char(tmp1);
        equip_char(ch, tmp1, EP);
    }
    if (tmp2) {
        obj_from_char(tmp2);
        equip_char(ch, tmp2, ES);
    }
    if (tmp3) {
        obj_from_char(tmp3);
        equip_char(ch, tmp3, ETWO);
    }
    check_criminal_flag(ch, tar_ch);
    WAIT_STATE(ch, number(2, 3) * PULSE_VIOLENCE);
}

int
get_max_skill(CHAR_DATA * ch, int nSkill)
{
    int guild_max = guild[(int) GET_GUILD(ch)].skill_max_learned[nSkill];
    int subguild_max = 0;

    if (GET_SUB_GUILD(ch)) {
        subguild_max = sub_guild[(int) GET_SUB_GUILD(ch)].skill_max_learned[nSkill];
    }

    return MAX(guild_max, subguild_max);
}

int
get_innate_max_skill(CHAR_DATA * ch, int sk)
{
    int max = 0;

    switch (skill[sk].sk_class) {
    case CLASS_MANIP:
    case CLASS_STEALTH:
        max = 40 + agl_app[GET_AGL(ch)].stealth_manip;
        break;
    case CLASS_MAGICK:
        max = 30 + wis_app[GET_WIS(ch)].wis_saves / 2;
        break;
    case CLASS_PERCEP:
        max = 40 + wis_app[GET_WIS(ch)].percep;
        break;
    case CLASS_COMBAT:
    case CLASS_WEAPON:
        max = 25 + end_app[GET_END(ch)].end_saves;
        // allow trample to go higher than other combat/weapon skills
        if (sk == SKILL_TRAMPLE) max += 25;
        break;
    case CLASS_BARTER:
        max = 30 + wis_app[GET_WIS(ch)].percep;
        break;
    case CLASS_PSIONICS:
        /* added a max default in here for Psi skills as 35 was the ceiling
         * previously, going against the code below, I left it alone -Morg */
        max = MIN(90, 35 + wis_app[GET_WIS(ch)].learn);
        break;
    case CLASS_KNOWLEDGE:
    case CLASS_LANG:
    case CLASS_REACH:
	max = 100;
	break;
    case CLASS_CRAFT:
    default:
        max = 35;
        break;
    }

    /* Innate skill everyone has */
    if (sk == SKILL_WILLPOWER)
        max = 100;

    // Return the guild/subguild tree or their natural talent if not in a tree
    return (get_max_skill(ch, sk) ? get_max_skill(ch, sk) : max);
}


/* these return 0 if they didnt have the skill to begin with
   or if it was already set to taught ,
   return 1 if they have skill, and it wasnt taught, and got set to taught
*/

int
set_skill_taught(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (set_skill_taught_raw((ch->skills[skilno])));
}

int
set_skill_taught_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    if (IS_SET(skill->taught_flags, SKILL_FLAG_LEARNED)) {
        return (0);
    } else {
        MUD_SET_BIT(skill->taught_flags, SKILL_FLAG_LEARNED);
        return (1);
    };
}

/* these return 0 if they didnt have the skill to begin with
   or if it was already set to taught ,
   return 1 if they have skill, and it wasnt taught, and got set to taught
*/

int
remove_skill_taught(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (remove_skill_taught_raw((ch->skills[skilno])));

}

int
remove_skill_taught_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    if (IS_SET(skill->taught_flags, SKILL_FLAG_LEARNED)) {
        REMOVE_BIT(skill->taught_flags, SKILL_FLAG_LEARNED);
        return (1);
    } else {
        return (0);
    };
}


/* these return 0 if they didnt have the skill to begin with,
   or the skill was not taught.
   return 1 if they had the skill, and it was taught
*/
int
is_skill_taught(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (is_skill_taught_raw((ch->skills[skilno])));
}

int
is_skill_taught_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    if (IS_SET(skill->taught_flags, SKILL_FLAG_LEARNED))
        return (1);
    else
        return (0);
}


/* returns 0 if they dont have the skill, and 1 if they do */
int
has_skill(CHAR_DATA * ch, int skill_number)
{
  if (!ch || 
      (skill_number < 0) || 
      (skill_number >= MAX_SKILLS)) {
    return (0);
  }
  
  if (ch->skills[skill_number]) {
    return (1);
  } else {
    return (0);
  }
}
 
/* returns percentage if they have the skill and 0 if they dont. */
int
get_skill_percentage(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (get_skill_percentage_raw((ch->skills[skilno])));
}

int
get_skill_percentage_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    return (skill->learned);
}

/* returns percentage if they have the skill and 0 if they dont, sets skill */
int
set_skill_percentage(CHAR_DATA * ch, int skilno, int perc)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (set_skill_percentage_raw((ch->skills[skilno]), perc));
}

int
set_skill_percentage_raw(struct char_skill_data *skill, int perc)
{
    if (!skill)
        return (0);

    skill->learned = perc;

    return (skill->learned);
}

/* these return 0 if they didnt have the skill to begin with
   or if it was already set to nogain ,
   return 1 if they have skill, and it wasnt nogain, and got set to nogain
*/

int
set_skill_nogain(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (set_skill_nogain_raw((ch->skills[skilno])));
}

int
set_skill_nogain_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    if (IS_SET(skill->taught_flags, SKILL_FLAG_NOGAIN)) {
        return (0);
    } else {
        MUD_SET_BIT(skill->taught_flags, SKILL_FLAG_NOGAIN);
        return (1);
    }
}

/* these return 0 if they didnt have the skill to begin with
   or if it was already set to nogain ,
   return 1 if they have skill, and it wasnt nogain, and got set to nogain
*/

int
remove_skill_nogain(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (remove_skill_nogain_raw((ch->skills[skilno])));

}

int
remove_skill_nogain_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    if (IS_SET(skill->taught_flags, SKILL_FLAG_NOGAIN)) {
        REMOVE_BIT(skill->taught_flags, SKILL_FLAG_NOGAIN);
        return (1);
    } else {
        return (0);
    }
}


/* these return 0 if they didnt have the skill to begin with,
   or the skill was not nogain.
   return 1 if they had the skill, and it was nogain
*/
int
is_skill_nogain(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (is_skill_nogain_raw((ch->skills[skilno])));
};

int
is_skill_nogain_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    if (IS_SET(skill->taught_flags, SKILL_FLAG_NOGAIN))
        return (1);
    else
        return (0);
};

/* these return 0 if they didnt have the skill to begin with
   or if it was already set to hidden ,
   return 1 if they have skill, and it wasnt hidden, and got set to hidden
*/

int
set_skill_hidden(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (set_skill_hidden_raw((ch->skills[skilno])));
};

int
set_skill_hidden_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    if (IS_SET(skill->taught_flags, SKILL_FLAG_HIDDEN)) {
        return (0);
    } else {
        MUD_SET_BIT(skill->taught_flags, SKILL_FLAG_HIDDEN);
        return (1);
    };
};

/* these return 0 if they didnt have the skill to begin with
   or if it was already set to hidden ,
   return 1 if they have skill, and it wasnt hidden, and got set to hidden
*/

int
remove_skill_hidden(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (remove_skill_hidden_raw((ch->skills[skilno])));

};

int
remove_skill_hidden_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    if (IS_SET(skill->taught_flags, SKILL_FLAG_HIDDEN)) {
        REMOVE_BIT(skill->taught_flags, SKILL_FLAG_HIDDEN);
        return (1);
    } else {
        return (0);
    };
};


/* these return 0 if they didnt have the skill to begin with,
   or the skill was not hidden.
   return 1 if they had the skill, and it was hidden
*/
int
is_skill_hidden(CHAR_DATA * ch, int skilno)
{
    if (!ch || (skilno < 0) || (skilno >= MAX_SKILLS))
        return (0);

    return (is_skill_hidden_raw((ch->skills[skilno])));
};

int
is_skill_hidden_raw(struct char_skill_data *skill)
{
    if (!skill)
        return (0);

    if (IS_SET(skill->taught_flags, SKILL_FLAG_HIDDEN))
        return (1);
    else
        return (0);
};

/*
 * increase_or_give_char_skill()
 *
 * This is largely a utility function intended for use by the subguild
 * code.  Subguilds should ever only _increase_ your skills, never
 * lower them.  This function will do just that, or give you a new
 * skill and set it learned, if needed.  We always return the old value
 * of the skill, to that the caller can calculate deltas, as needed.
 */
int
increase_or_give_char_skill(CHAR_DATA * ch, int skillno, int skill_perc)
{
    int retval = get_skill_percentage(ch, skillno);
    // They already have the skill, and it's better than what
    // we're trying to give them.
    if (retval > skill_perc) {
        return retval;
    }

    if (has_skill(ch, skillno)) {
        set_skill_percentage(ch, skillno, skill_perc);
        return retval;
    }
    // If we got here, they need the skill to be initialized,
    // and set as a "taught" skill
    init_skill(ch, skillno, skill_perc);
    set_skill_taught(ch, skillno);

    return retval;
}

