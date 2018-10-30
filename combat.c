/*
 * File: COMBAT.C
 * Usage: Routines and commands for combat.  (Oh, how awful!)
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

/* Test comment re: setting up personal sandbox. */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#include "constants.h"
#include "structs.h"
#include "db_file.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "parser.h"
#include "db.h"
#include "skills.h"
#include "guilds.h"
#include "clan.h"
#include "vt100c.h"
#include "cities.h"
#include "event.h"
#include "limits.h"
#include "utility.h"
#include "npc.h"
#include "handler.h"
#include "modify.h"
#include "memory.h"
#include "parser.h"
#include "object_list.h"
#include "player_accounts.h"
#include "psionics.h"
#include "utility.h"
#include "wagon_save.h"
#include "watch.h"
#include "object.h"
#include "special.h"

#define OFFENSE_LAG 1
#define DEFENSE_LAG 0

/* Structures */

CHAR_DATA *combat_list = 0;
CHAR_DATA *combat_next = 0;

CHAR_DATA *on_damage_attacker;
CHAR_DATA *on_damage_defender;


int absorbtion(int armor);
void sflag_to_obj(OBJ_DATA * obj, long sflag);
void spell_repel(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                 OBJ_DATA * obj);
int count_opponents(CHAR_DATA * ch);
extern bool ch_starts_falling_check(CHAR_DATA *ch);
extern int get_dir_from_name(char *arg);


/* weapon attack texts */
struct attack_hit_type attack_hit_text[] = {
    {"hit", "hits"},            /* 300 */
    {"bludgeon", "bludgeons"},  /* 301 */
    {"pierce", "pierces"},      /* 302 */
    {"stab", "stabs"},          /* 303 */
    {"chop", "chops"},          /* 304 */
    {"slash", "slashes"},       /* 305 */
    {"claw", "claws"},          /* 306 */
    {"whip", "whips"},          /* 307 */
    {"pinch", "pinches"},       /* 308 */
    {"sting", "stings"},        /* 309  */
    {"bite", "bites"},          /* 310  */
    {"peck", "pecks"},          /* 311 */
    {"gore", "gores"},          /* 312 */
    {"poke", "pokes"},          /* 313 *//* Pike    */
    {"poke", "pokes"},          /* 314 *//* Trident */
    {"jab", "jabs"},            /* 315 *//* Polearm */
    {"slash", "slashes"},       /* 316 *//* Knife   */
    {"lacerate", "lacerates"}   /* 317 *//* Razor   */
};

#define WEAR_BELT        0
#define WEAR_FINGER_R    1
#define WEAR_FINGER_L    2
#define WEAR_NECK        3
#define WEAR_BACK        4
#define WEAR_BODY        5
#define WEAR_HEAD        6
#define WEAR_LEGS        7
#define WEAR_FEET        8
#define WEAR_HANDS       9
#define WEAR_ARMS       10
#define WEAR_ON_BELT_1  11
#define WEAR_ABOUT      12
#define WEAR_WAIST      13
#define WEAR_WRIST_R    14
#define WEAR_WRIST_L    15
#define EP              16
#define ES                17
#define WEAR_ON_BELT_2    18
#define ETWO              19
#define WEAR_IN_HAIR      20

int phys_damage_mult[] = {
    200,
    40,
    40,
    300,
    250,
    140,
    300,
    110,
    100,
    100,
    120,
    120,
    200,
    160,
    200,
    200,
    100,
    100
};

int stun_damage_mult[] = {
    140,
    1,
    1,
    200,
    300,
    150,
    400,
    10,
    10,
    20,
    20,
    1,
    200,
    160,
    10,
    10,
    20,
    20
};

bool
is_shield(OBJ_DATA * obj)
{
    return (obj && (obj->obj_flags.type == ITEM_ARMOR)
            && (obj->obj_flags.value[0] > 0));
}

/* fight related routines */
void
appear(CHAR_DATA * ch)
{
    if (affected_by_spell(ch, SPELL_INVISIBLE))
        affect_from_char(ch, SPELL_INVISIBLE);

    REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_INVISIBLE);

    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
    act("You slowly fade into existence.", TRUE, ch, 0, 0, TO_CHAR);
}


void
unload_messages(void)
{
    int i;
    struct message_type *messages, *next_mess;

    for (i = 0; i < MAX_MESSAGES; i++) {
        messages = fight_messages[i].msg;
        fight_messages[i].msg = 0;

        while (messages) {
            next_mess = messages->next;
            free(messages->die_msg.attacker_msg);
            free(messages->die_msg.victim_msg);
            free(messages->die_msg.room_msg);
            free(messages->miss_msg.attacker_msg);
            free(messages->miss_msg.victim_msg);
            free(messages->miss_msg.room_msg);
            free(messages->hit_msg.attacker_msg);
            free(messages->hit_msg.victim_msg);
            free(messages->hit_msg.room_msg);
            free(messages->god_msg.attacker_msg);
            free(messages->god_msg.victim_msg);
            free(messages->god_msg.room_msg);
            free(messages);
            messages = next_mess;
        }
    }
}

/* This is a comment I added */

void
load_messages(void)
{
    FILE *f1;
    int i, type;
    struct message_type *messages;
    char chk[100];

    if (!(f1 = fopen(MESS_FILE, "r"))) {
        gamelogf("Couldn't open %s, bailing!", MESS_FILE);
        exit(0);
    }

    for (i = 0; i < MAX_MESSAGES; i++) {
        fight_messages[i].a_type = 0;
        fight_messages[i].number_of_attacks = 0;
        fight_messages[i].msg = 0;
    }

    fscanf(f1, " %s \n", chk);

    while (*chk != '$' && !feof(f1)) {
        type = atoi(chk);

        for (i = 0;
             (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) && fight_messages[i].a_type;
             i++);

        if (i >= MAX_MESSAGES) {
            gamelog("Too many combat messages.");
            exit(0);
        }

        CREATE(messages, struct message_type, 1);
        fight_messages[i].number_of_attacks++;
        fight_messages[i].a_type = type;
        messages->next = fight_messages[i].msg;
        fight_messages[i].msg = messages;

        messages->die_msg.attacker_msg = fread_string(f1);
        messages->die_msg.victim_msg = fread_string(f1);
        messages->die_msg.room_msg = fread_string(f1);
        messages->miss_msg.attacker_msg = fread_string(f1);
        messages->miss_msg.victim_msg = fread_string(f1);
        messages->miss_msg.room_msg = fread_string(f1);
        messages->hit_msg.attacker_msg = fread_string(f1);
        messages->hit_msg.victim_msg = fread_string(f1);
        messages->hit_msg.room_msg = fread_string(f1);
        messages->god_msg.attacker_msg = fread_string(f1);
        messages->god_msg.victim_msg = fread_string(f1);
        messages->god_msg.room_msg = fread_string(f1);
        fscanf(f1, " %s \n", chk);
    }

    fclose(f1);
}


void
update_pos(CHAR_DATA * victim) {
  if (!victim) {
    return;
  }
  
  if (GET_HIT(victim) > GET_MAX_HIT(victim)) {
    set_hit(victim, GET_MAX_HIT(victim));
  }
  
  if (GET_MOVE(victim) > GET_MAX_MOVE(victim)) {
    set_move(victim, GET_MAX_MOVE(victim));
  }
  
  if (GET_MANA(victim) > GET_MAX_MANA(victim)) {
    set_mana(victim, GET_MAX_MANA(victim));
  }
  
  if (GET_STUN(victim) > GET_MAX_STUN(victim)) {
    set_stun(victim, GET_MAX_STUN(victim));
  }
  
  /* update due to stun damage */
  if (GET_STUN(victim) <= 0 && AWAKE(victim)) {
    if (!(GET_RACE(victim) == RACE_MUL && affected_by_spell(victim, AFF_MUL_RAGE))) {
      if (!IS_AFFECTED(victim, CHAR_AFF_SLEEP)) {
        knock_out(victim, NULL, 1);     /* 10 RL mins is enough. -Tiernan */
        if (GET_RACE(victim) != RACE_SHADOW) {
          if (!isnamebracketsok("[sand_statue]", MSTR(victim, name))) {
            act("$n's eyes roll back in $s head.", FALSE, victim, NULL, NULL, TO_ROOM);
          }
        }
        drop_weapons(victim);
      }
    }
  }
  
  /* update due to physical damage */
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POSITION_STUNNED)) {
    return;
  } else if (GET_HIT(victim) > 0) {
    set_char_position(victim, POSITION_STANDING);
  } else if (GET_HIT(victim) <= -11) { 
    set_char_position(victim, POSITION_DEAD);
  } else if (GET_RACE(victim) == RACE_MUL && affected_by_spell(victim, AFF_MUL_RAGE)){
    set_char_position(victim, POSITION_STANDING);
  } else if (GET_HIT(victim) <= -3) {
    set_char_position(victim, POSITION_MORTALLYW);
  } else {
    set_char_position(victim, POSITION_STUNNED);
  }
  
  if (GET_POS(victim) < POSITION_STANDING) {
    if (is_char_watching_any_dir(victim)) {
      stop_watching(victim, FALSE);
    }
  }
  
  if (GET_POS(victim) < POSITION_STUNNED) {
    die_follower(victim);
    
    if (is_char_watching_any_char(victim)) {
      stop_watching(victim, FALSE);
    }
  }
}


bool
trying_to_disengage(CHAR_DATA * ch)
{
    return find_ex_description("[DISENGAGE_REQUEST]", ch->ex_description, TRUE) != NULL;
}

void
set_disengage_request(CHAR_DATA * ch)
{
    if (!trying_to_disengage(ch))
        set_char_extra_desc_value(ch, "[DISENGAGE_REQUEST]", "true");
}

void
clear_disengage_request(CHAR_DATA * ch)
{
    if (trying_to_disengage(ch))
        rem_char_extra_desc_value(ch, "[DISENGAGE_REQUEST]");
}

bool
daze_check(CHAR_DATA *ch, int prev_health, int lost_health, int prev_stun, int lost_stun, int loc ) {
    int tol = get_skill_percentage(ch, TOL_PAIN);
    int tol_penalty = 0;
    float max_threshold = 0.1;
    float min_threshold = 0.2;
    float sp_percent_of_max = (lost_stun + 0.0) / GET_MAX_STUN(ch);
    float hp_percent_of_max = (lost_health + 0.0) / GET_MAX_HIT(ch);
    float sp_percent_of_curr = (lost_stun + 0.0) / prev_stun;
    float hp_percent_of_curr = (lost_health + 0.0) / prev_health;
    int could_daze = FALSE;

    // change thresholds based on wear location
    switch( loc ) {
    case WEAR_HEAD:
    case WEAR_NECK:
        max_threshold = 0.1;
        min_threshold = 0.2;
        break;
    case WEAR_BODY:
    case WEAR_WAIST:
        max_threshold = 0.20;
        min_threshold = 0.30;
        break;
    case WEAR_ARMS:
    case WEAR_LEGS:
        max_threshold = 0.30;
        min_threshold = 0.40;
        break;
    default:
        max_threshold = 0.40;
        min_threshold = 0.50;
        break;
    }

    if( !AWAKE(ch) ) return FALSE;
    //roomlogf( ch->in_room, "hp: %d lost of %d (%d curr): %f minthreshold, %f maxthrehold", lost_health, GET_MAX_HIT(ch), prev_health, hp_percent_of_curr, hp_percent_of_max);
    //roomlogf( ch->in_room, "stun: %d lost of %d (%d curr): %f minthreshold, %f maxthrehold", lost_stun, GET_MAX_STUN(ch), prev_stun, sp_percent_of_curr, sp_percent_of_max);

    // 1% to thresholds for every 10% of pain tolerance
    //roomlogf( ch->in_room, "tol: %d, maxthreshold: %f, minthreshold: %f", tol, max_threshold, min_threshold );
    max_threshold += tol / 1000.0;
    min_threshold += tol / 1000.0;
    //roomlogf( ch->in_room, "maxthreshold: %f, minthreshold: %f", max_threshold, min_threshold );

    // first look at health
    if( hp_percent_of_curr >= min_threshold 
     && hp_percent_of_max >= max_threshold ) {
        // reduce tolerance check by amount over threshold
        tol_penalty = MAX(tol_penalty, (hp_percent_of_max - max_threshold)*100);
        // roomlogf( ch->in_room, "health tol penalty: %d", tol_penalty);
        could_daze = TRUE;
    }

    // then look at stun damage
    if( sp_percent_of_curr >= min_threshold
     && sp_percent_of_max >= max_threshold ) {
        // reduce tolerance check by amount over threshold
        tol_penalty = MAX(tol_penalty, (sp_percent_of_max - max_threshold)*100);
        // roomlogf( ch->in_room, "stun tol penalty: %d", tol_penalty);
        could_daze = TRUE;
    }

    // if the hit could daze
    if( could_daze ) {
        // there's a chance, give a skillcheck here
        if( number( 1, 100 ) > tol - tol_penalty ) {
            gain_skill(ch, TOL_PAIN, 1 );
            return TRUE;
        }
    }
    return FALSE;
}

bool
is_char_dazed(CHAR_DATA *ch) {
    return find_ex_description("[DAZED]", ch->ex_description, TRUE) != NULL;
}

void
set_dazed(CHAR_DATA *ch, int show) {
    if (GET_RACE(ch) == RACE_MUL && affected_by_spell(ch, AFF_MUL_RAGE))
        return;

    if( show ) {
        act( "You reel from the blow.", FALSE, ch, NULL, NULL, TO_CHAR);
        act( "$n reels from the blow.", FALSE, ch, NULL, NULL, TO_ROOM);
    }

    if (!is_char_dazed(ch)) {
        set_char_extra_desc_value(ch, "[DAZED]", "true");
        // keep them from doing other commands as well
        WAIT_STATE(ch, number(1,3));
    }
}

void
clear_dazed(CHAR_DATA *ch) {
    if (is_char_dazed(ch)) {
        rem_char_extra_desc_value(ch, "[DAZED]");
    }
}


void
handle_combat(void)
{
    CHAR_DATA *ch;
    int alive;
    struct timeval start;

    perf_enter(&start, "handle_combat()");
    for (ch = combat_list; ch; ch = combat_next) {
        combat_next = ch->next_fighting;

        // if dazed no attacks, but clear the daze and add a wait timer
        if( is_char_dazed(ch)) {
           if( ch->specials.combat_wait <= 1 ) {
               clear_dazed(ch);
               ch->specials.combat_wait += number(1, 5);
           } else {
               ch->specials.combat_wait--;
               continue;
           }
        }

        /* if you're trying to disengage, no attacking */
        if (trying_to_disengage(ch))
            continue;

        if (ch->specials.fighting && ch->in_room == ch->specials.fighting->in_room) {
            if (ch->specials.combat_wait-- < 1) {
                if ((alive = hit(ch, ch->specials.fighting, TYPE_UNDEFINED)))
                    if (IS_NPC(ch))
                        npc_act_combat(ch);
            }
        } else if (ch->specials.alt_fighting && ch->in_room == ch->specials.alt_fighting->in_room) {
            ch->specials.fighting = ch->specials.alt_fighting;
            ch->specials.alt_fighting = 0;
            if (ch->specials.combat_wait-- < 1) {
                if ((alive = hit(ch, ch->specials.fighting, TYPE_UNDEFINED)))
                    if (IS_NPC(ch))
                        npc_act_combat(ch);
            }
        } else
            stop_all_fighting(ch);
    }
    perf_exit("handle_combat()", start);
}



/* start one char fighting another (yes, it is horrible, I know... )  */
void
set_fighting(CHAR_DATA * ch, CHAR_DATA * vict)
{
  char buf[MAX_STRING_LENGTH];
  
  // only stop watching if they aren't watching the person they're fighting 
  if (!is_char_watching_char(ch, vict)) {
    stop_watching(ch, FALSE);
  }
  
  // Already fighting?  Don't bother.
  if (ch->specials.fighting) {
    return;
  }
  
  // Same with paralyzed
  if (is_paralyzed(ch)) {
    struct affected_type af;
    struct affected_type *paf;
    memset(&af, 0, sizeof(af));
    
    paf = affected_by_spell(ch, SPELL_PARALYZE);
    if( paf != NULL && !number(0, paf->power)) {
      af.type = SPELL_SLOW;
      af.duration = paf->duration;
      af.power = paf->power;
      af.magick_type = paf->magick_type;
      af.modifier = -(paf->power);
      af.location = CHAR_APPLY_AGL;
      af.bitvector = CHAR_AFF_SLOW;
      affect_join(ch, &af, FALSE, TRUE);
      affect_from_char(ch, SPELL_PARALYZE);
    }
    
    paf = affected_by_spell(ch, POISON_PERAINE);
    if( paf != NULL && !number(0, paf->power)) {
      af.type = SPELL_SLOW;
      af.duration = paf->duration;
      af.power = paf->power;
      af.magick_type = MAGICK_NONE;
      af.modifier = -(paf->power);
      af.location = CHAR_APPLY_AGL;
      af.bitvector = CHAR_AFF_SLOW;
      affect_join(ch, &af, FALSE, TRUE);
      affect_from_char(ch, POISON_PERAINE);
    }
  }
  
  // Not disengaging anymore
  clear_disengage_request(ch);
  
  // Clear their current mood
  change_mood(ch, NULL );
  
  // add to the list of combatants
  ch->next_fighting = combat_list;
  combat_list = ch;
  
  // combat wakes up sleepers
  if (IS_AFFECTED(ch, CHAR_AFF_SLEEP)) {
    struct affected_type af;
    memset(&af, 0, sizeof(af));
    
    affect_from_char(ch, SPELL_SLEEP);
    af.type = SPELL_NONE;
    af.duration = number(0, 1);
    af.power = 0;
    af.magick_type = MAGICK_NONE;
    af.location = CHAR_APPLY_STUN;
    af.modifier = GET_MAX_STUN(ch) / -number(3, 4);
    af.bitvector = 0;
    affect_to_char(ch, &af);
  }
  
  // Actually set them as fighting
  ch->specials.fighting = vict;
  
  // Put them in position fighting
  if (GET_POS(ch) > POSITION_SLEEPING && 
      GET_POS(ch) != POSITION_SITTING) {
    set_char_position(ch, POSITION_FIGHTING);
  }
  
  // If they're trying to avoid combat, auto-disengage
  if( IS_SET(ch->specials.nosave, NOSAVE_COMBAT)) {
    set_disengage_request(ch);
  } else {
    // For ch
    if (AWAKE(ch)) {
      snprintf(buf, sizeof(buf), "%s acts aggressively.", MSTR(ch, short_descr));
      send_to_empaths(ch, buf);
    }
    // For victim
    if (AWAKE(vict)) {
      snprintf(buf, sizeof(buf), "%s acts aggressively.", MSTR(vict, short_descr));
      send_to_empaths(vict, buf);
    }
  }
}

void
stop_fighting(CHAR_DATA * ch, CHAR_DATA * victim)
{
    // aggressor or victim not specified
    if (!ch || !victim)
        return;

    // victim is the primary opponent
    if (ch->specials.fighting == victim) {
        if (ch->specials.alt_fighting) {        // Switch to the other opponent
            ch->specials.fighting = ch->specials.alt_fighting;
            ch->specials.alt_fighting = NULL;
        } else                  // Only opponent
            stop_all_fighting(ch);
        return;
    }
    // victim is the secondary opponent
    if (ch->specials.alt_fighting == victim) {
        ch->specials.alt_fighting = NULL;
        if (!ch->specials.fighting)
            stop_all_fighting(ch);
        return;
    }
}

/* remove a char from the list of fighting chars */
void
stop_all_fighting(CHAR_DATA * ch)
{
    CHAR_DATA *tmp;

    if (!ch)
        return;

    if (ch == combat_next)
        combat_next = ch->next_fighting;

    if (combat_list == ch)
        combat_list = ch->next_fighting;
    else {
        for (tmp = combat_list; tmp && (tmp->next_fighting != ch); tmp = tmp->next_fighting);

        if (tmp)
            tmp->next_fighting = ch->next_fighting;
    }

    /* Removed the ->specials.fighting from the next line after ch -Morgenes */
    /* only change position if they're fighting -Morgenes */
    /* if( !( affected_by_spell( ch, SPELL_SLEEP ) ) ) */
    if (GET_POS(ch) == POSITION_FIGHTING)
        set_char_position(ch, POSITION_STANDING);

    clear_disengage_request(ch);
    clear_dazed(ch);
    ch->next_fighting = 0;
    ch->specials.fighting = 0;
    ch->specials.alt_fighting = 0;
    update_pos(ch);
}

/* disengage from a fight */
void
cmd_disengage(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vict = ch->specials.fighting;

    /* need to be in a room */
    if (!ch->in_room)
        return;

    /* if they're not fighting anyone */
    if (!vict) {
        send_to_char("You aren't fighting anyone.\n\r", ch);
        return;
    }

    if (affected_by_spell(ch, AFF_MUL_RAGE)) {
        send_to_char("The blood-lust inside you is too strong to force yourself from combat.\n\r", ch);
        return;
    }

    /* if the person they're fighting is the actor */
    if (vict->specials.fighting == ch || vict->specials.alt_fighting == ch) {
        /* if the person they're fighting isn't trying to disengage */
        if (!trying_to_disengage(vict) && !is_paralyzed(vict)
         && !is_char_dazed(vict)) {
            set_disengage_request(ch);
            act("You stop attacking $N!\n\r", FALSE, ch, 0, vict, TO_CHAR);
            act("$n stops attacking you.", FALSE, ch, 0, vict, TO_VICT);
            act("$n stops attacking $N.", FALSE, ch, 0, vict, TO_NOTVICT);

            snprintf(buf, sizeof(buf), 
             "%s disengages from combat, but %s is still fighting!\n\r",
                     MSTR(ch, name), MSTR(vict, name));
            send_to_monitors(ch, vict, buf, MONITOR_FIGHT);
        } else {
            act("$n stops fighting you.", FALSE, ch, 0, vict, TO_VICT);
            act("You stop fighting $N.", FALSE, ch, 0, vict, TO_CHAR);
            act("$n stops fighting $N.", FALSE, ch, 0, vict, TO_NOTVICT);

            snprintf(buf, sizeof(buf), 
             "%s and %s disengage.\n\r", MSTR(ch, name), MSTR(vict, name));
            send_to_monitors(ch, vict, buf, MONITOR_FIGHT);

            stop_fighting(ch, vict);
            stop_fighting(vict, ch);
        }
        return;
    }

    act("$n stops fighting you.", FALSE, ch, 0, vict, TO_VICT);
    act("You stop fighting $N.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n stops fighting $N.", FALSE, ch, 0, vict, TO_NOTVICT);

    snprintf(buf, sizeof(buf), "%s and %s disengage.", MSTR(ch, name),
             MSTR(ch->specials.fighting, name));
    send_to_monitors(ch, ch->specials.fighting, buf, MONITOR_FIGHT);

    stop_all_fighting(ch);
    return;
}

void
make_corpse(CHAR_DATA * ch)
{
    OBJ_DATA *corpse, *o;
    char buf[MAX_STRING_LENGTH];
    char *custom_skin;
    char corpse_name[MAX_STRING_LENGTH] = "body corpse ";
    int i;
    int sector;                 /* added for corpse ldesc differing with sector */
    char *f;
    struct weather_node *wn = 0;

    CREATE(corpse, OBJ_DATA, 1);
    clear_object(corpse);

    corpse->nr = 0;
    corpse->in_room = (ROOM_DATA *) 0;

    if (IS_NPC(ch)) {
        strcat(corpse_name, MSTR(ch, name));
        sprintf(buf, "%d", (npc_index + ch->nr)->vnum);
        set_obj_extra_desc_value(corpse, "[PC_info]", buf);
    } else {
        if (ch->player.extkwds) {
            strcat(corpse_name, ch->player.extkwds);
            strcat(corpse_name, " ");
        }
        strcat(corpse_name, MSTR(ch, name));
        if (!IS_IMMORTAL(ch)) {
            set_obj_extra_desc_value(corpse, "[PC_info]", MSTR(ch, name));
            set_obj_extra_desc_value(corpse, "[ACCOUNT_info]", ch->account);
        }
    }

    custom_skin = find_ex_description("[SKIN_INFO]", ch->ex_description, TRUE);

    if (custom_skin)
        set_obj_extra_desc_value(corpse, "[SKIN_INFO]", custom_skin);

    custom_skin = find_ex_description("[SKIN_INFO_OVERRIDE]", ch->ex_description, TRUE);

    if (custom_skin)
        set_obj_extra_desc_value(corpse, "[SKIN_INFO_OVERRIDE]", custom_skin);


    f = strdup(corpse_name);
    corpse->name = f;

    /* Added 6/15/2003 - switch statement so corpse ldesc 
     * differs according to sector -Sanvean */
    sector = ch->in_room->sector_type;

    switch (sector) {
    case SECT_CAVERN:
    case SECT_SOUTHERN_CAVERN:
        sprintf(buf, "The body of %s lies crumpled on the cavern floor.", MSTR(ch, short_descr));
        break;
    case SECT_DESERT:
        sprintf(buf, "The body of %s lies crumpled on the sands.", MSTR(ch, short_descr));
        break;
    case SECT_FIELD:
        sprintf(buf, "The body of %s lies crumpled on the dusty ground.", MSTR(ch, short_descr));
        break;
    case SECT_HEAVY_FOREST:
        sprintf(buf, "The body of %s lies crumpled among the trees.", MSTR(ch, short_descr));
        break;
    case SECT_HILLS:
        sprintf(buf, "The body of %s lies crumpled on the stony ground.", MSTR(ch, short_descr));
        break;
    case SECT_INSIDE:
        sprintf(buf, "The body of %s lies crumpled on the floor.", MSTR(ch, short_descr));
        break;
    case SECT_LIGHT_FOREST:
        sprintf(buf, "The body of %s lies crumpled among the scrub trees.", MSTR(ch, short_descr));
        break;
    case SECT_MOUNTAIN:
        sprintf(buf, "The body of %s lies crumpled on the rocky ground.", MSTR(ch, short_descr));
        break;
    case SECT_ROAD:
        sprintf(buf, "The body of %s lies crumpled on the road.", MSTR(ch, short_descr));
        break;
    case SECT_RUINS:
        sprintf(buf, "The body of %s lies crumpled on the stony ground.", MSTR(ch, short_descr));
        break;
    case SECT_SALT_FLATS:
        sprintf(buf, "The body of %s lies crumpled on the salty soil.", MSTR(ch, short_descr));
        break;
    case SECT_SEWER:
        sprintf(buf, "The body of %s lies half hidden in murky sludge.", MSTR(ch, short_descr));
        break;
    case SECT_SILT:
        sprintf(buf, "The body of %s lies buried in the silt.", MSTR(ch, short_descr));
        break;
    case SECT_THORNLANDS:
        sprintf(buf, "The body of %s lies amid the tangles of thorns.", MSTR(ch, short_descr));
        break;
    case SECT_FIRE_PLANE:
        sprintf(buf, "The body of %s lies here, smouldering among the coals.",
                MSTR(ch, short_descr));
        break;
    case SECT_WATER_PLANE:
        sprintf(buf, "The body of %s is here, floating in the murky water.", MSTR(ch, short_descr));
        break;
    case SECT_AIR_PLANE:
        sprintf(buf, "The body of %s floats here, hovering in mid-air.", MSTR(ch, short_descr));
        break;
    case SECT_EARTH_PLANE:
        sprintf(buf, "The body of %s lies crumpled on the hard ground.", MSTR(ch, short_descr));
        break;
    default:
        sprintf(buf, "The body of %s lies crumpled here.", MSTR(ch, short_descr));
        break;
    }
    corpse->long_descr = strdup(buf);

    sprintf(buf, "the body of %s", MSTR(ch, short_descr));
    corpse->short_descr = strdup(buf);

    if( MSTR(ch, description ))
        corpse->description = strdup( MSTR(ch, description));

    corpse->contains = ch->carrying;
    for (o = corpse->contains; o; o = o->next_content) {
        o->carried_by = 0;
        o->in_obj = corpse;
    }

    money_mgr(MM_CH2POUCH, GET_OBSIDIAN(ch), (void *) ch, (void *) corpse);

    corpse->obj_flags.type = ITEM_CONTAINER;
    corpse->obj_flags.wear_flags = ITEM_TAKE;
    corpse->obj_flags.value[0] = 0;     /* You can't store stuff in a corpse */
    corpse->obj_flags.value[3] = MAX(1, GET_RACE(ch));
    corpse->obj_flags.value[5] = get_char_size(ch);
    corpse->obj_flags.weight = (GET_WEIGHT(ch) * 10) + IS_CARRYING_W(ch);
    corpse->obj_flags.weight *= 100;
    corpse->obj_flags.timer = ZAL_HOURS_PER_DAY * GET_WEIGHT(ch);

    // extra day for pc corpses
    if (!IS_NPC(ch))
        corpse->obj_flags.timer += ZAL_HOURS_PER_DAY;

    for (i = 0; i < MAX_WEAR; i++)
        if (ch->equipment[i])
            obj_to_obj(unequip_char(ch, i), corpse);

    ch->carrying = 0;
    IS_CARRYING_N(ch) = 0;

    object_list_insert(corpse);

    corpse->prev_content = 0;
    corpse->next_content = 0;

    object_list_new_owner(corpse, 0);

    if (ch->on_obj 
     && (IS_SET(ch->on_obj->obj_flags.value[1], FURN_SKIMMER)
      || IS_SET(ch->on_obj->obj_flags.value[1], FURN_WAGON))) {
        obj_to_obj(corpse, ch->on_obj);
    } else {
        obj_to_room(corpse, ch->in_room);
    }

    // Add a little life to the zone
    wn = find_weather_node(ch->in_room);
    if (wn)
        wn->life = MIN(wn->max_life, wn->life + 1);
}



void
make_head_from_corpse(OBJ_DATA * from_corpse)
{
    OBJ_DATA *head;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];
    char buf4[MAX_STRING_LENGTH];
    char obj_ename[MAX_STRING_LENGTH];
    char obj_ainfo[MAX_STRING_LENGTH];
    char obj_edesc[MAX_STRING_LENGTH];
    //    char head_buf[MAX_STRING_LENGTH];
    //    char head_buf2[MAX_STRING_LENGTH];

    char *f;

    CREATE(head, OBJ_DATA, 1);
    clear_object(head);

    head->nr = 0;

    clear_object(head);

    //    head = read_object(1119, VIRTUAL);

    head->in_room = (ROOM_DATA *) 0;

    /* remove "body corpse " */
    if (!strncmp("body corpse ", OSTR(from_corpse, name), strlen("body corpse "))) {
        sprintf(buf, "head %s", (OSTR(from_corpse, name))
                + strlen("body corpse "));
    } else {
        sprintf(buf, "head %s", OSTR(from_corpse, name));
    }

    f = strdup(buf);
    head->name = f;

    /* remove "the body of " */
    if (!strncmp("the body of ", OSTR(from_corpse, short_descr), strlen("the body of "))) {
        sprintf(buf, "the head of %s", (OSTR(from_corpse, short_descr)) + strlen("the body of "));
        sprintf(buf2, "The head of %s lies here.",
                (OSTR(from_corpse, short_descr)) + strlen("the body of "));
        sprintf(buf3, "the headless body of %s",
                (OSTR(from_corpse, short_descr)) + strlen("the body of "));
        sprintf(buf4, "The headless body of %s lies crumpled here.",
                (OSTR(from_corpse, short_descr)) + strlen("the body of "));
    } else {
        sprintf(buf, "the head of %s", OSTR(from_corpse, short_descr));
        sprintf(buf2, "The head of %s lies here", OSTR(from_corpse, short_descr));
        sprintf(buf3, "the headless body of %s", OSTR(from_corpse, short_descr));
        sprintf(buf4, "The headless body of %s lies crumpled here.",
                OSTR(from_corpse, short_descr));
    }

    head->short_descr = strdup(buf);
    head->long_descr = strdup(buf2);
    from_corpse->short_descr = strdup(buf3);
    from_corpse->long_descr = strdup(buf4);

    head->contains = NULL;

    head->obj_flags.type = ITEM_CONTAINER;
    head->obj_flags.wear_flags = ITEM_TAKE;
    head->obj_flags.wear_flags |= ITEM_ES;
    head->obj_flags.value[0] = 0;       /* You can't store stuff in a corpse */
    MUD_SET_BIT(head->obj_flags.value[1], CONT_CORPSE_HEAD);
    head->obj_flags.value[3] = from_corpse->obj_flags.value[3];

    head->obj_flags.weight = 200;
    head->obj_flags.timer = ZAL_HOURS_PER_DAY * 20;

    // FIXME:   add_object_to_list
    object_list_insert(head);

    head->prev_content = 0;
    head->next_content = 0;

    /* Find who's head it is.. a gruesome job, but someone has to do it */
    strcpy(obj_ename, "[PC_info]");
    strcpy(obj_ainfo, "[ACCOUNT_info]");

    if (get_obj_extra_desc_value(from_corpse, obj_ename, obj_edesc, MAX_STRING_LENGTH))
        set_obj_extra_desc_value(head, "[HEAD_PC_info]", obj_edesc);

    if (get_obj_extra_desc_value(from_corpse, obj_ainfo, obj_edesc, MAX_STRING_LENGTH))
        set_obj_extra_desc_value(head, "[HEAD_ACCOUNT_info]", obj_edesc);

    //    strcpy (head_buf, buf);
    //    strcpy(head_buf2, buf2);

    //    set_obj_extra_desc_value(head, "[UNIQUE_SDESC]", strdup(head_buf));
    //    set_obj_extra_desc_value(head, "[UNIQUE_LDESC]", strdup (head_buf2));

    obj_to_room(head, from_corpse->in_room);
}



void
death_cry(CHAR_DATA * ch)
{
    int door;
    char *msg;
    CHAR_DATA *rch;

    /* old death cry
     * act( "You freeze as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);
     * 
     * for (door = 0; door <= 5; door++)
     * if( CAN_GO( ch, door ) 
     * && ( ch->in_room != ch->in_room->direction[door]->to_room))
     * send_to_room ("An agonized scream sounds from nearby.\n\r",
     * ch->in_room->direction[door]->to_room);
     */
    if (isnamebracketsok("[sand_statue]", MSTR(ch, name)))
        return;

    switch (number(0, 16)) {
    default:
        msg = "$n cries out in pain.";
        break;
    case 3:
    case 2:
        msg = "You freeze as you hear $n's death cry.";
    case 1:
        msg = "$n splatters blood on you.";
        for( rch = ch->in_room->people; rch; rch = rch->next_in_room ) {
            sflag_to_char(rch, OST_BLOODY);
        }
        break;
    }

    act(msg, FALSE, ch, NULL, NULL, TO_ROOM);

    msg = "You hear someone cry out in the distance.\n\r";

    for (door = 0; door <= 5; door++)
        if (CAN_GO(ch, door)
            && (ch->in_room != ch->in_room->direction[door]->to_room))
            send_to_room(msg, ch->in_room->direction[door]->to_room);
}


void
knock_out(CHAR_DATA * ch, const char *reason, int duration)
{
    struct affected_type af;
    char buf[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    sprintf(buf, "Your vision goes black%s%s.\n\r",
            (reason && reason[0] != '\0' && str_infix("in dmpl", reason))
            ? ", and you pass out " : "", (reason && reason[0] != '\0'
                                           && str_infix("in dmpl", reason))
            ? reason : "");
    
    if (IS_SET(ch->specials.affected_by, CHAR_AFF_HIDE)) {
      REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
    }

    /* set stun to 0 */
    set_stun(ch, 0);

    af.type = SPELL_SLEEP;
    af.duration = duration;
    af.power = 0;
    af.magick_type = MAGICK_NONE;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_SLEEP;
    affect_join(ch, &af, FALSE, FALSE);
    send_to_char(buf, ch);
    set_char_position(ch, POSITION_SLEEPING);

/* Q: Why free this here?
   A: So that the ldesc changes if they're knocked out
      (Per bug report: **Sanvean: ldesc doesn't change when PC goes unconscious)
      -Morg */
    WIPE_LDESC(ch);

    /* stop them from fighting */
    stop_all_fighting(ch);

    /* no more watching either */
    stop_watching_raw(ch);

    ch_starts_falling_check(ch);

    // Let empaths know their guy just collapsed.
    snprintf(buf, sizeof(buf), "%s falls unconscious.", MSTR(ch, short_descr));
    send_to_empaths(ch, buf);
    return;
}


bool
knocked_out(CHAR_DATA * ch)
{
    struct affected_type *af;

    for (af = ch->affected; af; af = af->next)
        if (af->type == SPELL_SLEEP && af->magick_type == MAGICK_NONE && GET_STUN(ch) == 0)
            return TRUE;
    return FALSE;
}

char *rebirth_msg =
    "\n\r\n\rA bright light fills your vision.\n\r"
    "When it clears, you find yourself standing on a plane of fog.\n\r\n\r"
    "A cold voice booms in your ears,\n\r   'You have been spared from death.'" "\n\r\n\r"
    "After a seeming eternity, the cold voice continues,\n\r"
    "   'this once.  Know that when next you stand before me, it will be the last.'\n\r\n\r";

void
raw_kill(CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *tmp_object, *next_obj;
    int pos = 0;

    stop_all_fighting(ch);

    shhlog("RAW KILL");

    if (IS_AFFECTED(ch, CHAR_AFF_SUMMONED) || affected_by_spell(ch, SPELL_POSSESS_CORPSE)) {
        if (affected_by_spell(ch, SPELL_MOUNT))
            act("The body of $n slowly dissolves into sand.", FALSE, ch, 0, 0, TO_ROOM);
        else if (affected_by_spell(ch, SPELL_SANDSTATUE))
            act("The body of $n explodes in a shower of sand!", FALSE, ch, 0, 0, TO_ROOM);
        else {
            act("The body of $n dissipates slowly from view.", FALSE, ch, 0, 0, TO_ROOM);
            if (IS_SET(ch->specials.act, CFL_UNDEAD)) {
                for (tmp_object = ch->carrying; tmp_object; tmp_object = next_obj) {
                    next_obj = tmp_object->next_content;
                    if (IS_SET(tmp_object->obj_flags.extra_flags, OFL_DISINTIGRATE))
                        extract_obj(tmp_object);
                    else {
                        obj_from_char(tmp_object);
                        obj_to_room(tmp_object, ch->in_room);
                        if (!IS_SET(tmp_object->obj_flags.state_flags, OST_BLOODY))
                            sflag_to_obj(tmp_object, OST_BLOODY);
                        if (!IS_SET(tmp_object->obj_flags.state_flags, OST_SEWER))
                            sflag_to_obj(tmp_object, OST_SEWER);
                    }
                }
                if (GET_OBSIDIAN(ch))
                    money_mgr(MM_CH2OBJ, GET_OBSIDIAN(ch), (void *) ch, 0);
                for (pos = 0; pos < MAX_WEAR; pos++)
                    if (ch->equipment && ch->equipment[pos]) {
                        if (NULL != (tmp_object = unequip_char(ch, pos))) {
                            obj_to_room(tmp_object, ch->in_room);
                            if (!IS_SET(tmp_object->obj_flags.state_flags, OST_BLOODY))
                                sflag_to_obj(tmp_object, OST_BLOODY);
                            if (!IS_SET(tmp_object->obj_flags.state_flags, OST_SEWER))
                                sflag_to_obj(tmp_object, OST_SEWER);
                        }
                    }
            }
        }
        extract_char(ch);
        return;
    }

    snprintf(buf, sizeof(buf), "%s dies.", MSTR(ch, short_descr));
    send_to_empaths(ch, buf);
    remove_all_affects_from_char(ch);

    if (IS_NPC(ch) || IS_SET(ch->specials.act, CFL_DEAD)) {
        make_corpse(ch);
        logout_character(ch);
        extract_char(ch);
    } else {
        CHAR_DATA *k;

        act("$n panics, and attempts to flee.", FALSE, ch, NULL, NULL, TO_ROOM);
        /* If someone explain why raw_kill(ch) is sending people
         * to their hometown, then put this back in, otherwise, it
         * makes no sense */

        /* Answer: for < 2 hour characters & award life */

        set_hit(ch, GET_MAX_HIT(ch) / 2);
        update_pos(ch);
        char_from_room(ch);

        if (!IS_IMMORTAL(ch))
            send_to_char(rebirth_msg, ch);
        remove_all_hates_fears(ch);
        for (k = character_list; k; k = k->next) {
            if (does_hate(k, ch))
                remove_from_hates(k, ch);
            if (does_fear(k, ch))
                remove_from_fears(k, ch);
	    if (k->specials.contact == ch) {
                act("You feel your mental contact withdrawing from the mind of your target.", FALSE, k, 0, 0, TO_CHAR);
		remove_contact(k);
	    }
        }
        remove_contact(ch);

        if (ch->player.dead)
            free(ch->player.dead);
        ch->player.dead = strdup("rebirth");

        char_to_room(ch, home_room(ch));
        gamelogf("%s (%s) has been newbie repopped.", GET_NAME(ch), ch->account);
    }
}



void
die(CHAR_DATA *ch)
{
    struct time_info_data playing_time;
    CHAR_DATA *tch;

    act("$b", FALSE, ch, NULL, NULL, TO_ROOM);

    if (has_special_on_cmd(ch->specials.programs, 0, CMD_DIE)) {
        bool result = execute_npc_program(ch, 0, CMD_DIE, "");
       
        for (tch = character_list; tch && tch != ch; tch = tch->next) ;
        if (!tch)
            return;
       
        if ((result == TRUE) && GET_POS(ch) != POSITION_DEAD)
            return;
    }

    if (!IS_NPC(ch) && ch->account && !IS_IMMORTAL(ch) 
     && !affected_by_spell(ch, SPELL_POSSESS_CORPSE)) {
        PLAYER_INFO *pPInfo;
        DESCRIPTOR_DATA d;
        bool fOld = FALSE;

        if (ch->desc && ch->desc->player_info)
            pPInfo = ch->desc->player_info;
        else {          /* look them up */
            if (ch->account
             && (pPInfo = find_player_info(ch->account)) == NULL) {
                if ((fOld = load_player_info(&d, ch->account)) == FALSE) {
                    gamelogf("%s died with invalid account '%s'.", ch->name, ch->account);
                    return;
                }
                pPInfo = d.player_info;
                d.player_info = NULL;
            }
        }

        playing_time =
            real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);

        if (pPInfo->karma > 0 || playing_time.day > 0 
         || playing_time.hours >= 2 || 
          (ch->player.dead && !strcmp(ch->player.dead, "rebirth"))) {
            if (IS_SET(ch->specials.act, CFL_NO_DEATH)) {
                REMOVE_BIT(ch->specials.act, CFL_NO_DEATH);
            } else {
                ch->application_status = APPLICATION_DEATH;
                update_char_info(pPInfo, lowercase(ch->name), APPLICATION_DEATH);
                save_player_info(pPInfo);
                MUD_SET_BIT(ch->specials.act, CFL_DEAD);
            }
        }

        if (fOld)
            free_player_info(pPInfo);
    }
    raw_kill(ch);
}

void
replace_string(char *str, char *weapon, char *buf, int len, int pos, int rtype)
{
    int i;

    if (rtype > 9)
        rtype = 9;
    if (rtype < 0)
        rtype = 0;
    if (pos > 19)
        pos = 19;
    if (pos < 0)
        pos = 0;

    strcpy(buf, str);
    while ((i = string_pos("#", buf)) >= 0) {
        switch (buf[i + 1]) {
        case 'W':
            delete_substr(buf, i, 2);
            insert_substr(weapon, buf, i, len);
            break;
        case 'P':
            delete_substr(buf, i, 2);
            insert_substr(hit_loc_name[pos][rtype], buf, i, len);
            break;
        default:
            delete_substr(buf, i, 1);
            break;
        }
    }
}


void
dam_message(int dam, CHAR_DATA * ch, CHAR_DATA * victim, int w_type, int miss, int pos, int rtype)
{
    OBJ_DATA *wield;
    int mess_n;
    char buf[MAX_INPUT_LENGTH];
    int x;

    struct dam_weapon_type
    {
        char *to_room;
        char *to_char;
        char *to_victim;
    };

    /*  Kelvik's dam_weapon_type  */
    static struct dam_weapon_type dam_weapons[] = {
        {"$N swiftly dodges $n's #W.",  /* 0-0 */
         "$N swiftly dodges your #W.",
         "$n #W at you, but you dodge out of the way."},

        {"$n #W $2$N$1, barely grazing $S #P.", /* 1-2 */
         "You #W $2$N$1, barely grazing $S #P.",
         "$n #W $2you$1, barely grazing your #P."},

        {"$n #W at $2$N$1's #P, nicking $M.",   /* 3-4 */
         "You nick $2$N$1's #P with your #W.",
         "$n #W $2your$1 #P, nicking you."},

        {"$n lightly #W $2$N$1's #P.",  /* 5-6 */
         "You lightly #W $2$N$1's #P.",
         "$n lightly #W $2your$1 #P."},

        {"$n #W $2$N$1 on $S #P.",      /* 7-8 */
         "You #W $2$N$1's #P.",
         "$n #W $2your$1 #P."},

        {"$n solidly #W $2$N$1's #P.",  /* 9-11 */
         "You land a solid #W to $2$N$1's #P.",
         "$n solidly #W $2your$1 #P."},

        {"$n #W $2$N$1's #P, connecting hard.", /* 12-15 */
         "You #W $2$N$1 very hard on $S #P.",
         "$n #W $2you$1 very hard on your #P."},

        {"$n #W $2$N$1 on $S #P, wounding $M.", /* 16-19 */
         "You wound $2$N$1 on $S #P with your #W.",
         "$n #W $2your$1 #P, wounding you."},

        {"$n viciously #W $2$N$1 on $S #P.",    /* 20-24 */
         "You viciously #W $2$N$1 on $S #P.",
         "$n viciously #W $2you$1 on your #P."},

        {"$n brutally #W $2$N$1 on $S #P.",     /* 25-29 */
         "You wound $2$N$1 on $S #P with a brutal #W.",
         "$n brutally #W $2you$1 on your #P."},

        {"$n #W $2$N$1's #P, inflicting a grievous wound.",     /* 30-37 */
         "You inflict a grievous wound on $2$N$1's #P with your #W.",
         "$n #W $2your$1 #P, inflicting a grievous wound."},

        {"$n #W $2$N$1's #P, doing horrendous damage.", /* 38--- */
         "You do unspeakable damage to $2$N$1's #P with your #W.",
         "$n #W $2your$1 #P, doing frightening damage."}
    };

    w_type -= TYPE_HIT;         /* Change to base of table with text */

    wield = ch->equipment[EP];

    if (!miss && (dam == 0)) {

        if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
            act("$N's body momentarily turns to shadow, as $n's attack passes right through!",
                FALSE, ch, 0, victim, TO_NOTVICT);
            act("$N's body momentarily turns to shadow, as your attack passes right through!",
                FALSE, ch, 0, victim, TO_CHAR);
            act("Your body momentarily turns to shadow, as $n's attack passes right through!",
                FALSE, ch, 0, victim, TO_VICT);

        } else if (victim->equipment[pos]) {
            x = number(1, 3);

            switch (number(1, 3)) {
            case 1:
                act("$n lunges at $N, but $s blow is deftly deflected by $p.", FALSE, ch,
                    victim->equipment[pos], victim, TO_NOTVICT);
                act("You lunge at $N, but your blow is deftly deflected by $p.", FALSE, ch,
                    victim->equipment[pos], victim, TO_CHAR);
                act("$n lunges at you, but $s blow is deftly deflected by $p.", FALSE, ch,
                    victim->equipment[pos], victim, TO_VICT);
                break;
            case 2:
                act("$n's attack on $N is absorbed by $p.", FALSE, ch, victim->equipment[pos],
                    victim, TO_NOTVICT);
                act("Your attack on $N is absorbed by $p.", FALSE, ch, victim->equipment[pos],
                    victim, TO_CHAR);
                act("$n's attack on you is absorbed by $p.", FALSE, ch, victim->equipment[pos],
                    victim, TO_VICT);
                break;
            case 3:
                act("$n viciously leaps toward $N, but $p gets in the way.", FALSE, ch,
                    victim->equipment[pos], victim, TO_NOTVICT);
                act("You viciously leap toward $N, but $p gets in the way.", FALSE, ch,
                    victim->equipment[pos], victim, TO_CHAR);
                act("$n viciously leaps toward you, but $p gets in the way.", FALSE, ch,
                    victim->equipment[pos], victim, TO_VICT);
                break;
            }
        } else {
            if (affected_by_spell(victim, SPELL_STONE_SKIN)) {
                act("$n's blow bounces off $N's tough, stone-like skin.", FALSE, ch,
                    victim->equipment[pos], victim, TO_NOTVICT);
                act("Your blow bounces off $N's tough, stone-like skin.", FALSE, ch,
                    victim->equipment[pos], victim, TO_CHAR);
                act("$n's blow bounces off your tough, stone-like skin.", FALSE, ch,
                    victim->equipment[pos], victim, TO_VICT);
            } else {
                act("$n's blow bounces off $N's tough skin.", FALSE, ch, victim->equipment[pos],
                    victim, TO_NOTVICT);
                act("Your blow bounces off $N's tough skin.", FALSE, ch, victim->equipment[pos],
                    victim, TO_CHAR);
                act("$n's blow bounces off your tough skin.", FALSE, ch, victim->equipment[pos],
                    victim, TO_VICT);
            }
        }
    } else {
        if (miss && (dam == 0))
            mess_n = 0;
        else if (dam <= 2)
            mess_n = 1;
        else if (dam <= 4)      /*  Kel changes start here  */
            mess_n = 2;
        else if (dam <= 6)
            mess_n = 3;
        else if (dam <= 8)
            mess_n = 4;
        else if (dam <= 11)
            mess_n = 5;
        else if (dam <= 15)
            mess_n = 6;
        else if (dam <= 19)
            mess_n = 7;
        else if (dam <= 24)
            mess_n = 8;
        else if (dam <= 29)
            mess_n = 9;
        else if (dam <= 37)
            mess_n = 10;
        else
            mess_n = 11;

        replace_string(dam_weapons[mess_n].to_room, attack_hit_text[w_type].plural, buf,
                       MAX_INPUT_LENGTH, pos, rtype);

        if (miss) {
            act(buf, FALSE, ch, wield, victim, TO_NOTVICT_COMBAT);
        } else {
            act(buf, FALSE, ch, wield, victim, TO_NOTVICT);
        }

        if (!miss || !IS_SET(ch->specials.brief, BRIEF_COMBAT)) {
            replace_string(dam_weapons[mess_n].to_char, attack_hit_text[w_type].singular, buf,
                           MAX_INPUT_LENGTH, pos, rtype);
            act(buf, FALSE, ch, wield, victim, TO_CHAR);
        }

        if (!miss || !IS_SET(victim->specials.brief, BRIEF_COMBAT)) {
            replace_string(dam_weapons[mess_n].to_victim, attack_hit_text[w_type].plural, buf,
                           MAX_INPUT_LENGTH, pos, rtype);
            act(buf, FALSE, ch, wield, victim, TO_VICT);
        }

        /*
         * sprintf( buf, "w_type is: %d.\n\r", w_type );
         * send_to_char( buf, ch );
         */
    }
}


#ifdef USE_NEW_ROOM_IN_CITY

int
room_in_city(ROOM_DATA * in_room)
{
    if (!in_room)
        return CITY_NONE;

    assert(in_room->city >= 0 && in_room->city < MAX_CITIES);
    return (in_room->city);
}

int
char_in_city(CHAR_DATA * ch)
{
    if (!ch)
        return CITY_NONE;

    return (room_in_city(ch->in_room));
}

int
old_room_in_city(ROOM_DATA * in_room)
{
    if (!in_room)
        return CITY_NONE;

    if (IS_SET(in_room->room_flags, RFL_ALLANAK)) {
        REMOVE_BIT(in_room->room_flags, RFL_ALLANAK);
        return CITY_ALLANAK;
    }

    if (IS_SET(in_room->room_flags, RFL_TULUK)) {
        REMOVE_BIT(in_room->room_flags, RFL_TULUK);
        return CITY_TULUK;
    }

    if ((in_room->number >= 50000)
        && (in_room->number <= 50999)) {
        /* I removed the Arena from the city's rooms  -Ur */
        if ((in_room->number >= 50660)
            && (in_room->number <= 50668))
            return CITY_NONE;
        return CITY_ALLANAK;
    }

    if (in_room->number == 66170 || in_room->number == 66301 || in_room->number == 66302
        || in_room->number == 66167 || in_room->number == 66171 || in_room->number == 66304
        || in_room->number == 66300 || in_room->number == 66076 || in_room->number == 66303
        || in_room->number == 66077 || in_room->number == 66112 || in_room->number == 66075
        || in_room->number == 66079 || in_room->number == 66128)
        return CITY_ALLANAK;

    if (in_room->number >= 13900 && in_room->number <= 13950)
        return CITY_ALLANAK;

    if ((in_room->number >= 20001) && (in_room->number <= 20699))
        return CITY_TULUK;

    if (((in_room->number >= 21001) && (in_room->number <= 21018))
        || ((in_room->number >= 21049) && (in_room->number <= 21061)))
        return CITY_BW;

    if (in_room->number >= 4301 && in_room->number <= 4329)
        return CITY_CENYR;

    if (in_room->number >= 48001 && in_room->number <= 48100)
        return CITY_RS;

    if (in_room->number >= 4101 && in_room->number <= 4165)
        return CITY_RS;

    //   if( in_room->number >= 24000 && in_room->number <= 24999 )
    //      return CITY_CAI_SHYZN;

    if ((in_room->number >= 64000 && in_room->number <= 64046)
        || (in_room->number >= 64191 && in_room->number <= 64197)
        || (in_room->number >= 64647 && in_room->number <= 64672)
        || (in_room->number >= 64986 && in_room->number <= 64999))
        return CITY_LUIR;

    if (in_room->number >= 37701 && in_room->number <= 37800)
        return CITY_MAL_KRIAN;

    return CITY_NONE;
}

#else

int
room_in_city(ROOM_DATA * in_room)
{
    if (!in_room)
        return CITY_NONE;

    return (in_room->city);

    if (IS_SET(in_room->room_flags, RFL_ALLANAK))
        return CITY_ALLANAK;

    if (IS_SET(in_room->room_flags, RFL_TULUK))
        return CITY_TULUK;

    if ((in_room->number >= 50000)
        && (in_room->number <= 50999)) {
        /* I removed the Arena from the city's rooms  -Ur */
        if ((in_room->number >= 50660)
            && (in_room->number <= 50668))
            return CITY_NONE;
        return CITY_ALLANAK;
    }

    if (in_room->number == 66170 || in_room->number == 66301 || in_room->number == 66302
        || in_room->number == 66167 || in_room->number == 66171 || in_room->number == 66304
        || in_room->number == 66300 || in_room->number == 66076 || in_room->number == 66303
        || in_room->number == 66077 || in_room->number == 66112 || in_room->number == 66075
        || in_room->number == 66079 || in_room->number == 66128)
        return CITY_ALLANAK;

    if (in_room->number >= 13900 && in_room->number <= 13950)
        return CITY_ALLANAK;

    if ((in_room->number >= 20001) && (in_room->number <= 20699))
        return CITY_TULUK;

    if (((in_room->number >= 21001) && (in_room->number <= 21018))
        || ((in_room->number >= 21049) && (in_room->number <= 21061)))
        return CITY_BW;

    if (in_room->number >= 4301 && in_room->number <= 4329)
        return CITY_CENYR;

    if (in_room->number >= 48001 && in_room->number <= 48100)
        return CITY_RS;

    if (in_room->number >= 4101 && in_room->number <= 4165)
        return CITY_RS;

    //   if( in_room->number >= 24000 && in_room->number <= 24999 )
    //      return CITY_CAI_SHYZN;

    if ((in_room->number >= 64000 && in_room->number <= 64046)
        || (in_room->number >= 64191 && in_room->number <= 64197)
        || (in_room->number >= 64647 && in_room->number <= 64672)
        || (in_room->number >= 64986 && in_room->number <= 64999))
        return CITY_LUIR;

    if (in_room->number >= 37701 && in_room->number <= 37800)
        return CITY_MAL_KRIAN;

    return CITY_NONE;
}

int
char_in_city(CHAR_DATA * ch)
{
    if (!ch)
        return CITY_NONE;

    return room_in_city(ch->in_room);
}

#endif



void
check_criminal_flag(CHAR_DATA * ch, CHAR_DATA * victim)
{
    int ct, dir;
    CHAR_DATA *tch;
    ROOM_DATA *rm;
    struct affected_type af;
    char buf[MAX_STRING_LENGTH];

    memset(&af, 0, sizeof(af));

    if (!IS_AFFECTED(ch, CHAR_AFF_NOQUIT)
        && !IS_NPC(ch)
        && !IS_IMMORTAL(ch)
        && victim && !(IS_SET(victim->specials.act, CFL_AGGRESSIVE))) {
        af.type = TYPE_COMBAT;
        af.magick_type = MAGICK_NONE;
        af.power = 0;
        af.location = 0;
        af.duration = 1;
        af.modifier = 0;
        af.bitvector = CHAR_AFF_NOQUIT;
        affect_to_char(ch, &af);
    }

    if (!(ch->in_room))
        return;

    if (!(ct = room_in_city(ch->in_room))
        || (victim && IS_SET(victim->specials.act, city[ct].c_flag))
        || (victim && IS_SET(victim->specials.act, CFL_AGGRESSIVE))
        || IS_SET(ch->specials.act, city[ct].c_flag))
        return;

    if (IS_TRIBE(ch, city[ct].tribe))
        return;

    if (victim) {
        /* non-humanoids should be up for slaughter */
        if (GET_RACE_TYPE(victim) != RTYPE_HUMANOID)
            return;
        
        if (is_char_noble(ch, ct) && !IS_TRIBE(victim, city[ct].tribe)) {
          return;
        }
        
        if (ct == CITY_CAI_SHYZN) {
          if (GET_RACE(victim) != RACE_MANTIS) {
            return;
          }
        } else if (ct == CITY_BW) {
          /* only protect desert elves */
          if (GET_RACE(victim) != RACE_DESERT_ELF || !IS_TRIBE(victim, TRIBE_BW)) {
            return;
          }
        }
    }

    /* Commenting this out until the binary values are removed
     * -Nessalin 1/12/97 
     * else 
     * if( get_clan_rel( city[ct].tribe, GET_TRIBE( ch ) ) == CLAN_REL_VASSAL)
     * {
     * if ( victim && ( GET_TRIBE( victim ) != city[ct].tribe) &&
     * !is_greater_vassal( GET_TRIBE( victim ), ch->player.tribe,
     * city[ct].tribe) )
     * return; *//* greater vassals are never wrong */
    /*      } */

    if (!IS_SET(ch->in_room->room_flags, RFL_POPULATED)
     || room_visibility(ch->in_room) < 0
     || (IS_SET(ch->in_room->room_flags, RFL_POPULATED) 
      && !IS_SET(ch->in_room->room_flags, RFL_INDOORS)
      && (time_info.hours == NIGHT_EARLY || time_info.hours == NIGHT_LATE))) {
        for (tch = ch->in_room->people; tch; tch = tch->next_in_room)
            if (IS_TRIBE(tch, city[ct].tribe)
                && !IS_IMMORTAL(tch)
                && AWAKE(tch)
                && CAN_SEE(tch, ch))
                break;

        for (dir = 0; (dir < 6) && !tch; dir++)
            if (ch->in_room->direction[dir]
                && (rm = ch->in_room->direction[dir]->to_room) != NULL)
                for (tch = rm->people; tch; tch = tch->next_in_room)
                    if (IS_TRIBE(tch, city[ct].tribe)
                        && !IS_IMMORTAL(tch)
                        && AWAKE(tch)
                        && CAN_SEE(tch, ch))
                        break;

        if (!tch)
            return;
    }

    /* look for edesc for [NO_CRIM_CLANS] */
    if (TRUE == get_room_extra_desc_value(ch->in_room, "[NO_CRIM_CLANS]", buf, MAX_STRING_LENGTH)) {
        if( is_ch_in_clan_list( ch, buf ) ) return;
    }

    /* if the city has a criminal flag, set it */
    if (city[ct].c_flag) {
        af.type = TYPE_CRIMINAL;
        af.location = CHAR_APPLY_CFLAGS;
        af.duration = number(5, 10);
        af.modifier = city[ct].c_flag;
        af.bitvector = 0;
        if (victim)
            af.power = CRIME_ASSAULT;
        else
            af.power = CRIME_DEFILING;
        affect_to_char(ch, &af);
        send_to_char("You're now wanted!\n\r", ch);
    }
}

int
gain_weapon_racetype_proficiency(CHAR_DATA * ch, int wtype, int rtype)
{
    if ((wtype < 0) || (wtype > 7)) {
        wtype = 0;
    }
    if ((rtype > 0) && (rtype < 9)) {
        if (!has_skill(ch, skill_weap_rtype[rtype][wtype]))
            /*      if (!(ch->skills[skill_weap_rtype[ rtype][wtype]] )) */
        {
            if (number(0, 100) <= wis_app[GET_WIS(ch)].learn) {
                init_skill(ch, skill_weap_rtype[rtype][wtype], 1);

                /* if they don't normally get the skill, set it as taught */
                if (get_max_skill(ch, skill_weap_rtype[rtype][wtype]) <= 0)
                    set_skill_taught(ch, skill_weap_rtype[rtype][wtype]);
            }
        } else {
            gain_skill(ch, skill_weap_rtype[rtype][wtype], 1);
        };
        return (1);
    };
    return (0);
};

int
get_weapon_racetype_proficiency(CHAR_DATA * ch, int wtype, int rtype)
{
    if ((wtype < 0) || (wtype > 7)) {
        return (0);
    }
    if ((rtype > 0) && (rtype < 9)) {
        return (get_skill_percentage(ch, skill_weap_rtype[rtype][wtype]));
    } else {
        return (0);
    }

};

void
gain_proficiency(CHAR_DATA * ch, int attacktype)
{
    switch (attacktype) {
    case TYPE_SLASH:
    case TYPE_WHIP:
        if (has_skill(ch, PROF_SLASHING))
            gain_skill(ch, PROF_SLASHING, 1);
        break;
    case TYPE_PIERCE:
    case TYPE_STAB:
        if (has_skill(ch, PROF_PIERCING))
            gain_skill(ch, PROF_PIERCING, 1);
        break;
    case TYPE_CHOP:
        if (has_skill(ch, PROF_CHOPPING))
            gain_skill(ch, PROF_CHOPPING, 1);
        break;
    case TYPE_BLUDGEON:
        if (has_skill(ch, PROF_BLUDGEONING))
            gain_skill(ch, PROF_BLUDGEONING, 1);
        break;
    case TYPE_PIKE:
        if (has_skill(ch, PROF_PIKE))
            gain_skill(ch, PROF_PIKE, 1);
        break;
    case TYPE_POLEARM:
        if (has_skill(ch, PROF_POLEARM))
            gain_skill(ch, PROF_POLEARM, 1);
        break;
    case TYPE_TRIDENT:
        if (has_skill(ch, PROF_TRIDENT))
            gain_skill(ch, PROF_TRIDENT, 1);
        break;
    case TYPE_KNIFE:
        if (has_skill(ch, PROF_KNIFE))
            gain_skill(ch, PROF_KNIFE, 1);
        break;
    case TYPE_RAZOR:
        if (has_skill(ch, PROF_RAZOR))
            gain_skill(ch, PROF_RAZOR, 1);
        break;
    default:
        break;
    }
}

int
proficiency_bonus(CHAR_DATA * ch, int attacktype)
{

    int skill = SPELL_NONE;

    switch (attacktype) {
    case TYPE_SLASH:
    case TYPE_WHIP:
        skill = PROF_SLASHING;
        break;
    case TYPE_CHOP:
        skill = PROF_CHOPPING;
        break;
    case TYPE_PIERCE:
    case TYPE_STAB:
        skill = PROF_PIERCING;
        break;
    case TYPE_BLUDGEON:
        skill = PROF_BLUDGEONING;
        break;
    case TYPE_PIKE:
        skill = PROF_PIKE;
        break;
    case TYPE_POLEARM:
        skill = PROF_POLEARM;
        break;
    case TYPE_TRIDENT:
        skill = PROF_TRIDENT;
        break;
    case TYPE_KNIFE:
        skill = PROF_KNIFE;
        break;
    case TYPE_RAZOR:
        skill = PROF_RAZOR;
        break;
    default:
        break;
    }
    if ((skill != SPELL_NONE) && (has_skill(ch, skill)))
        return (get_skill_percentage(ch, skill));
    else
        return (0);
}


void
add_rest_to_npcs_in_need(void)
{
    CHAR_DATA *k;
    struct timeval start;

    perf_enter(&start, "add_rest_to_npcs_in_need()");
 //   shhlog("Adding rest to all NPCS in need");
    for (k = character_list; k; k = k->next) {
        if (!IS_NPC(k))
            continue;

        /*
         * maybe check if they are short of moves and such 
         * if (GET_MOVE (ev->ch) <= GET_MAX_MOVE (ev->ch))
         */

        new_event(EVNT_REST, 20, k, 0, 0, 0, 0);
    };

    perf_exit("add_rest_to_npcs_in_need()", start);
}


void
drop_weapons(CHAR_DATA * victim)
{
    OBJ_DATA *weapon;

    /* chance to drop held/wielded things */
    if ((weapon = victim->equipment[EP]) != NULL && number(0, 3) != 0) {
        obj_from_char(unequip_char(victim, EP));
        obj_to_room(weapon, victim->in_room);
        act("$p clatters to the ground as $n releases it.", FALSE, victim, weapon, NULL, TO_ROOM);

        drop_light(weapon, victim->in_room);
    }

    if ((weapon = victim->equipment[ES]) != NULL && number(0, 3) != 0) {
        obj_from_char(unequip_char(victim, ES));
        obj_to_room(weapon, victim->in_room);
        act("$p clatters to the ground as $n releases it.", FALSE, victim, weapon, NULL, TO_ROOM);

        drop_light(weapon, victim->in_room);
    }

    if ((weapon = victim->equipment[ETWO]) != NULL && number(0, 3) != 0) {
        obj_from_char(unequip_char(victim, ETWO));
        obj_to_room(weapon, victim->in_room);
        act("$p clatters to the ground as $n releases it.", FALSE, victim, weapon, NULL, TO_ROOM);

        drop_light(weapon, victim->in_room);
    }
}




/* causes damage to victim */
bool
generic_damage_immortal(CHAR_DATA * victim, int phys_dam, int mana_dam, int mv_dam, int stun_dam)
{
    bool alive = TRUE;
    char *undying = 0;

    /* damage can't be less than 0 */
    phys_dam = MAX(phys_dam, 0);
    mana_dam = MAX(mana_dam, 0);
    mv_dam = MAX(mv_dam, 0);
    stun_dam = MAX(stun_dam, 0);
    
    if (!phys_dam && !stun_dam && !mana_dam && !mv_dam)
        return alive;

    if (IS_NPC(victim)) {
        new_event(EVNT_REST, (PULSE_MOBILE + number(-10, 10)) * 4, victim, 0, 0, 0, 0);
    }

    undying = find_ex_description("[UNDYING]", victim->ex_description, TRUE);
    if (undying)
        if ((GET_HIT(victim) - phys_dam) < -4)
            set_hit(victim, phys_dam - 6);

    /* subtract damage and update position */
    adjust_hit(victim, -phys_dam);
    adjust_mana(victim, -mana_dam);
    adjust_move(victim, -mv_dam);
    adjust_stun(victim, -stun_dam);
    return update_pos_after_damage(victim);
}

bool
update_pos_after_damage(CHAR_DATA *victim) {
    int old_pos = GET_POS(victim);
    bool alive = TRUE;

    // call update position
    update_pos(victim);

    /* dead? */
    if (GET_POS(victim) == POSITION_DEAD)
        alive = FALSE;

    /* update position to char and room */
    if (!AWAKE(victim) && old_pos > POSITION_SLEEPING) {
        if (GET_POS(victim) == POSITION_DEAD)
            death_cry(victim);

        switch (GET_POS(victim)) {
        case POSITION_MORTALLYW:
            /* act ("$n lies still on the ground, mortally wounded.",
             * TRUE, victim, 0, 0, TO_ROOM); */
            act("You are mortally wounded, and will die soon if not aided.", FALSE, victim, 0, 0,
                TO_CHAR);
            break;
        case POSITION_STUNNED:
            /*act("$n lies still on the ground, stunned and unable to move.",
             * TRUE, victim, 0, 0, TO_ROOM); */
            act("You are stunned, and unable to move.", FALSE, victim, 0, 0, TO_CHAR);
            break;
        case POSITION_DEAD:
            /*if( race[(int) GET_RACE (victim)].race_type == RTYPE_HUMANOID )
             * act ("$n rolls over, blood spattering $s lifeless face.",
             * TRUE, victim, 0, 0, TO_ROOM);
             * else
             * act ("$n becomes suddenly still, apparently dead.",
             * TRUE, victim, 0, 0, TO_ROOM); */
            act("You are dead!", FALSE, victim, 0, 0, TO_CHAR);
            send_to_char("\a", victim);
            break;
        default:
            break;
        }

        drop_weapons(victim);

        if (GET_RACE_TYPE(victim) != RTYPE_OTHER && GET_RACE_TYPE(victim) != RTYPE_PLANT) {
            if (victim->specials.riding) {
                act("$n falls off $N and lies still on the ground.", FALSE, victim, NULL,
                    victim->specials.riding, TO_ROOM);
                victim->specials.riding = NULL;
            } else {

                act("$n crumples to the ground.", FALSE, victim, NULL, NULL, TO_ROOM);
            }
        } else {
            if (!isnamebracketsok("[sand_statue]", MSTR(victim, name)))
                act("$n stops moving.", FALSE, victim, NULL, NULL, TO_ROOM);
        }
    }
    return alive;
}

bool
generic_damage(CHAR_DATA * victim, int phys_dam, int mana_dam, int mv_dam, int stun_dam)
{
    /* can't hurt immortals */
    if (IS_IMMORTAL(victim))
        return 1;
    else
        return generic_damage_immortal(victim, phys_dam, mana_dam, mv_dam, stun_dam);
}


void
cmd_assist(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *friend;
    char buf[MAX_STRING_LENGTH];

    if (IS_CALMED(ch)) {
        cprintf(ch, "You don't feel much like joining fights right now.\n\r");
        return;
    }

    argument = one_argument(argument, buf, sizeof(buf));

    if (!(friend = get_char_room_vis(ch, buf))) {
        cprintf(ch, "Assist who?\n\r");
        return;
    }

    if (!friend->specials.fighting) {
        cprintf(ch, "%s is not fighting anyone.\n\r", capitalize(PERS(ch, friend)));
        return;
    }

    if (ch->specials.fighting) {
        cprintf(ch, "You're already doing the best you can!\n\r");
        return;
    }

    if (is_char_ethereal(ch) != is_char_ethereal(friend)) {
        cprintf(ch, "%s is beyond your reach.\n\r", capitalize(PERS(ch, friend)));
        return;
    }

    act("You charge into the fight!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n joins your fight!", FALSE, ch, 0, friend, TO_VICT);
    act("$n joins $N's fight!", FALSE, ch, 0, friend, TO_NOTVICT);

    sprintf(buf, "%s assists %s\n\r", GET_NAME(ch), GET_NAME(friend));
    send_to_monitors(ch, friend, buf, MONITOR_FIGHT);

    set_fighting(ch, friend->specials.fighting);
}

void
extract_merchant(CHAR_DATA * merch)
{
    OBJ_DATA *obj;
    ROOM_DATA *rm;
    char buf[MAX_STRING_LENGTH];

    if (!merch->in_room)
        return;

    if (merch->specials.fighting)
        sprintf(buf, "%s attacks %s, MERCHANT_FLEE in room %d.",
                MSTR(merch->specials.fighting, name), MSTR(merch, name), merch->in_room->number);
    else
        sprintf(buf, "%s flees from room %d. MERCHANT_FLEE", MSTR(merch, name),
                merch->in_room->number);

    gamelog(buf);

    act("$n panics, and attempts to flee.", FALSE, merch, 0, 0, TO_ROOM);
    char_from_room(merch);
    rm = get_room_num(0);
    char_to_room(merch, get_room_num(0));
    extract_char(merch);

    OBJ_DATA *next_obj;
    for (obj = rm->contents; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (!IS_WAGON(obj))
            extract_obj(obj);
    }
}

bool
missile_damage(CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * arrow, int direction, int dam,
               int attacktype)
{
    struct time_info_data playing_time;
    char buf[MAX_STRING_LENGTH], txt[MAX_STRING_LENGTH];
    bool alive = FALSE;
    int percent = 0;
    float float_dam, stun_dam, phys_dam;
    int loc = 0, shield_loc = -1;
    int chance_of_success;
    int chance_parry = 0;
    OBJ_DATA *varm, *shield = (OBJ_DATA *) 0;
    struct affected_type af;
    OBJ_DATA *wagobj;
    ROOM_DATA *tar_room;

    memset(&af, 0, sizeof(af));

    float_dam = (float) dam;
    /* can't throw missiles while invis */
    if (IS_AFFECTED(ch, CHAR_AFF_INVISIBLE))
        appear(ch);

    /* can't hurt immortals */
    if (IS_IMMORTAL(victim))
        float_dam = 0;

    if (affected_by_spell(victim, SPELL_WIND_ARMOR))
        float_dam = 0;

    if (affected_by_spell(victim, SPELL_FIRE_ARMOR))
        float_dam = 0;

    if (affected_by_spell(victim, SPELL_SHADOW_ARMOR))
        float_dam = 0;

    /* Can't hit sentinels unless they're unique (avatar/npcs) */
    if (IS_SET(victim->specials.act, CFL_SENTINEL)
        && !IS_SET(victim->specials.act, CFL_UNIQUE))
        float_dam = 0;

    if (is_merchant(victim)) {
        check_criminal_flag(ch, victim);
        extract_merchant(victim);
        return FALSE;
    }

    if (direction < DIR_OUT)
        sprintf(buf, "You %s $p %s%s", (attacktype == SKILL_THROW) ? "hurl" : "fire",
                dir_name[direction],
                (IS_SET(ch->in_room->direction[direction]->exit_info, EX_SPL_FIRE_WALL)) ?
                " through a wall of fire." : ".");
    else if (direction == DIR_OUT)
        sprintf(buf, "You %s $p out.", (attacktype == SKILL_THROW) ? "hurl" : "fire");
    else if (direction == DIR_IN) {
        tar_room = get_room_distance(victim, 1, DIR_OUT);
        if (tar_room != ch->in_room)
            sprintf(buf, "You %s $p into something.",
                    (attacktype == SKILL_THROW) ? "hurl" : "fire");
        else {
            wagobj = find_exitable_wagon_for_room(victim->in_room);
            if (!wagobj)
                sprintf(buf, "You %s $p into something.",
                        (attacktype == SKILL_THROW) ? "hurl" : "fire");
            else
                sprintf(buf, "You %s $p into %s.", (attacktype == SKILL_THROW) ? "hurl" : "fire",
                        OSTR(wagobj, short_descr));
        }
    }

    act(buf, FALSE, ch, arrow, 0, TO_CHAR);

    if (direction < DIR_OUT)
        sprintf(buf, "$n %s $p %s%s", (attacktype == SKILL_THROW) ? "hurls" : "fires",
                dir_name[direction],
                (IS_SET(ch->in_room->direction[direction]->exit_info, EX_SPL_FIRE_WALL)) ?
                " through a wall of fire." : ".");
    else if (direction == DIR_OUT)
        sprintf(buf, "$n %s $p out.", (attacktype == SKILL_THROW) ? "hurls" : "fires");
    else if (direction == DIR_IN) {
        tar_room = get_room_distance(victim, 1, DIR_OUT);
        if (tar_room != ch->in_room)
            sprintf(buf, "$n %s $p into something.",
                    (attacktype == SKILL_THROW) ? "hurls" : "fires");
        else {
            wagobj = find_exitable_wagon_for_room(victim->in_room);
            if (!wagobj)
                sprintf(buf, "$n %s $p into something.",
                        (attacktype == SKILL_THROW) ? "hurls" : "fires");
            else
                sprintf(buf, "$n %s $p into %s.", (attacktype == SKILL_THROW) ? "hurls" : "fires",
                        OSTR(wagobj, short_descr));
        }
    }

    act(buf, FALSE, ch, arrow, 0, TO_ROOM);

    if (IS_SET(arrow->obj_flags.extra_flags, OFL_GLYPH)) {
        if (direction < DIR_OUT)
            sprintf(buf, "$p flies in from %s, trailing sparks", rev_dir_name[direction]);
        else if (direction == DIR_OUT) {
            wagobj = find_wagon_for_room(ch->in_room);
            if (wagobj)
                sprintf(buf, "$p flies out of %s, trailing sparks", OSTR(wagobj, short_descr));
            else
                sprintf(buf, "$p flies into the area, trailing sparks");
        } else if (direction == DIR_IN)
            sprintf(buf, "$p flies in from outside, trailing sparks");
    } else {
        if (direction < DIR_OUT)
            sprintf(buf, "$p flies in from %s", rev_dir_name[direction]);
        else if (direction == DIR_OUT) {
            wagobj = find_wagon_for_room(ch->in_room);
            if (wagobj)
                sprintf(buf, "$p flies out of %s", OSTR(wagobj, short_descr));
            else
                sprintf(buf, "$p flies into the area");
        } else if (direction == DIR_IN)
            sprintf(buf, "$p flies in from outside");
    }

    if (float_dam <= 0) {       /* They missed */
        if (IS_SET(arrow->obj_flags.extra_flags, OFL_MISSILE_R)) {      /*  Kelvik Boomerang-Chatkcha Thang begin  */
            percent = number(1, 80);
            if (has_skill(ch, SKILL_THROW)
                && (percent < get_skill_percentage(ch, SKILL_THROW))) {
                sprintf(txt, "%s, whirls around, and heads back.", buf);
                act(txt, FALSE, victim, arrow, 0, TO_ROOM);
                act(txt, FALSE, victim, arrow, 0, TO_CHAR);
                obj_to_char(unequip_char(ch, EP), ch);
                act("$p comes spinning back to you.", FALSE, ch, arrow, 0, TO_CHAR);
                act("$p comes spinning back and lands in $s hand.", FALSE, ch, arrow, 0, TO_ROOM);
            }
        } /*  End boomerang ang ang ang  */
        else {
            if (affected_by_spell(victim, SPELL_WIND_ARMOR)) {
                sprintf(txt,
                        "%s and is headed towards $n, but gets blown away from $m by a sudden gust of wind.",
                        buf);
                act(txt, FALSE, victim, arrow, 0, TO_ROOM);
                act(txt, FALSE, victim, arrow, 0, TO_CHAR);
	    } else if (affected_by_spell(victim, SPELL_FIRE_ARMOR)) {
                sprintf(txt,
                        "%s and is headed towards $n, but is deflected by an explosion within the dancing flames surrounding $m.",
                        buf);
                act(txt, FALSE, victim, arrow, 0, TO_ROOM);
                act(txt, FALSE, victim, arrow, 0, TO_CHAR);
		MUD_SET_BIT(arrow->obj_flags.state_flags, OST_BURNED);
	    } else if (affected_by_spell(victim, SPELL_SHADOW_ARMOR)) {
                sprintf(txt,
                        "%s and is headed towards $n, but vanishes in a brief flash of darkness around $m.",
                        buf);
                act(txt, FALSE, victim, arrow, 0, TO_ROOM);
                act(txt, FALSE, victim, arrow, 0, TO_CHAR);
                MUD_SET_BIT(arrow->obj_flags.extra_flags, OFL_ETHEREAL);
            } else {            /* They missed with a normal missile weapon */
                sprintf(txt, "%s and hits the ground.", buf);
                act(txt, FALSE, victim, arrow, 0, TO_ROOM);
                act(txt, FALSE, victim, arrow, 0, TO_CHAR);
            }
        }
        /* Wether they hit or not, the target now get no_quit */
        if (victim && !IS_AFFECTED(victim, CHAR_AFF_NOQUIT) && !IS_IMMORTAL(victim)) {
            af.type = TYPE_COMBAT;
            af.magick_type = MAGICK_NONE;
            af.power = 0;
            af.location = 0;
            af.duration = 1;
            af.modifier = 0;
            af.bitvector = CHAR_AFF_NOQUIT;
            affect_to_char(victim, &af);
        }
    } /* End, they missed */
    else {                      /* They hit */
        if (AWAKE(victim) && has_skill(victim, SKILL_PARRY) 
         && (get_skill_percentage(victim, SKILL_PARRY) > 60)
         && visibility(victim) > 0) {      /* they can try and parry */
            chance_parry = get_skill_percentage(victim, SKILL_PARRY);   /* Max is 65 */
            chance_parry -= 55; // 5-10% base chance

            // bonus for being 'alert'
            if ((is_char_watching_char(victim, ch) && CAN_SEE(victim, ch))
             || (direction != -1 && is_char_watching_dir(victim, direction))) {
                chance_parry += 10;  // max is 20%
            } 

            // if you're fighting someone other than the attacker
            if (victim->specials.fighting && victim->specials.fighting != ch ) {
               chance_parry -= 20;  
            }

            // etwo weapon bonus to parry
            if ((victim->equipment[ETWO])
             && (victim->equipment[ETWO]->obj_flags.type == ITEM_WEAPON)) {
                shield = victim->equipment[ETWO];
                chance_parry += 5;        // 5 - 25%
            } else if ((victim->equipment[EP])
             && (victim->equipment[EP]->obj_flags.type == ITEM_WEAPON)) {
                shield = victim->equipment[EP];
                chance_parry -= 5;       // 0 - 15%
            } else if ((victim->equipment[ES])
             && (victim->equipment[ES]->obj_flags.type == ITEM_WEAPON)) {
                shield = victim->equipment[ES];
                chance_parry -= 10;       // 0 - 10%
            } else if(hands_free(victim) > 0 ) // barehanded
                chance_parry -= 19;       // 0 - 1%
            else // no hands free, no weapons
                chance_parry = 0;         // 0%

            if (number(1, 100) < chance_parry) {        /* They do parry */
                if (shield) {
                    sprintf(txt, "%s but is knocked out of the air by $n using %s.", buf,
                            OSTR(shield, short_descr));
                    act(txt, FALSE, victim, arrow, 0, TO_ROOM);
                    sprintf(txt, "%s but you knock it out of the air using %s.", buf,
                            OSTR(shield, short_descr));
                    act(txt, FALSE, victim, arrow, 0, TO_CHAR);
                    act("$N knocks it out of the air using $p.", FALSE, ch, shield, victim,
                        TO_CHAR);
                } else {
                    sprintf(txt, "%s but is knocked out of the air by $n using $s " "hands.", buf);
                    act(txt, FALSE, victim, arrow, 0, TO_ROOM);
                    sprintf(txt, "%s but you knock it out of the air using your " "hands.", buf);
                    act(txt, FALSE, victim, arrow, 0, TO_CHAR);
                    act("$N knocks it out of the air using $S hands.", FALSE, ch, 0, victim,
                        TO_CHAR);
                }
                float_dam = 0;
                return alive;
            } else {            /* Whoops, they tried to parry and failed */
                if (shield) {
                    sprintf(txt, "%s and is swiped at by $n using %s, but $e " "misses.", buf,
                            OSTR(shield, short_descr));
                    act(txt, FALSE, victim, arrow, 0, TO_ROOM);
                    sprintf(txt, "%s. You swipe at it using %s, but miss.", buf,
                            OSTR(shield, short_descr));
                    act(txt, FALSE, victim, arrow, 0, TO_CHAR);
                    act("$N swipes at it using $p, but $E misses.", FALSE, ch, shield, victim,
                        TO_CHAR);
                } else {
                    sprintf(txt, "%s and is swiped at by $n using $s " "hands, but $e misses.",
                            buf);
                    act(txt, FALSE, victim, arrow, 0, TO_ROOM);
                    sprintf(txt, "%s. You swipe at it using your hands, but miss.", buf);
                    act(txt, FALSE, victim, arrow, 0, TO_CHAR);
                    act("$N swipes at it using $S hands, but $E misses", FALSE, ch, 0, victim,
                        TO_CHAR);
                }
            }
        }

        /* Wether they hit or not, the target now get no_quit */
        if (victim && !IS_AFFECTED(victim, CHAR_AFF_NOQUIT) && !IS_IMMORTAL(victim)) {
            af.type = TYPE_COMBAT;
            af.magick_type = MAGICK_NONE;
            af.power = 0;
            af.location = 0;
            af.duration = 1;
            af.modifier = 0;
            af.bitvector = CHAR_AFF_NOQUIT;
            affect_to_char(victim, &af);
        }

        if (float_dam) {        /* indicates they couldn't, or didn't parry the missile */
            /* Get a random body location. If that fails, hit them on the body */
            if (!(loc = get_random_hit_loc(victim, attacktype)))
                loc = WEAR_BODY;

            shield_loc = get_shield_loc(victim);

            if (shield_loc != -1 && has_skill(victim, SKILL_SHIELD)) {
                chance_of_success = get_skill_percentage(victim, SKILL_SHIELD);
            } else
                chance_of_success = 0;

            // alert, or watching the attacker, or the direction
            if ((is_char_watching_char(victim, ch) && CAN_SEE(victim, ch))
             || (direction != -1 && is_char_watching_dir(victim, direction))) {
               chance_of_success += 10;
            }

            // not paying attention in the right direction
            if (direction != -1 && !is_char_watching_dir(victim, direction)) {
               chance_of_success -= 20;
            }

            // if actively fighting someone else, reduce chance by 20%
            if( victim->specials.fighting && victim->specials.fighting != ch ) {
               chance_of_success -= 20;
            }

            // mods for shield location
            switch( shield_loc ) {
               case ETWO:
                  chance_of_success += 20;
                  break;
               case EP:
                  chance_of_success += 10;
                  break;
               // base shield use skill if es
               default:
                  break;
            }

            // can they see?
            if( visibility(victim) <= 0 ) {
               chance_of_success = 0;
            }

            if (number(1, 100) < chance_of_success) {
                sprintf(txt, "%s and deflects off $n's shield.", buf);
                act(txt, FALSE, victim, arrow, 0, TO_ROOM);

                sprintf(txt, "%s and deflects off your shield!", buf);
                act(txt, FALSE, victim, arrow, 0, TO_CHAR);

                sprintf(txt, "You see $p deflect off $N's shield.");
                act(txt, FALSE, ch, arrow, victim, TO_CHAR);
                float_dam = 0;
            } else {
                sprintf(txt, "%s and strikes $n's %s.", buf,
                        hit_loc_name[loc][GET_RACE_TYPE(victim)]);
                act(txt, FALSE, victim, arrow, 0, TO_ROOM);

                sprintf(txt, "%s and strikes your %s!", buf,
                        hit_loc_name[loc][GET_RACE_TYPE(victim)]);
                act(txt, FALSE, victim, arrow, 0, TO_CHAR);

                sprintf(txt, "You see $p strike $N's %s.",
                        hit_loc_name[loc][GET_RACE_TYPE(victim)]);
                act(txt, FALSE, ch, arrow, victim, TO_CHAR);

                if( chance_of_success > 0 && has_skill(victim, SKILL_SHIELD)) {
                   gain_skill(victim, SKILL_SHIELD, 1);
                }
            }
        }

        if (float_dam) {        /* no damage for invulnerability */
            if (IS_AFFECTED(victim, CHAR_AFF_INVULNERABILITY)) {
                sprintf(buf, "You see a creamy shell around %s fade away.\n\r", PERS(ch, victim));
                send_to_char(buf, ch);
                act("The creamy shell around $n fades away.", FALSE, victim, 0, 0, TO_ROOM);
                act("The creamy shell around you fades away.", TRUE, victim, 0, 0, TO_CHAR);
                REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_INVULNERABILITY);
                float_dam = 0;
            }
        }

        if (float_dam)
            if (affected_by_spell(victim, SPELL_INVULNERABILITY)) {     /*  Kel  */
                affect_from_char(victim, SPELL_INVULNERABILITY);
                send_to_char("The creamy shell around you fades away.\n\r", victim);
                act("The creamy shell around $n fades away.", FALSE, victim, 0, 0, TO_ROOM);
                float_dam = 0;
            }

        if (float_dam)
            if (affected_by_spell(victim, SPELL_LEVITATE)) {
                act("Your concentration is broken and you fall to the ground!", FALSE, ch, 0,
                    victim, TO_VICT);
                act("$N cries out and freefalls to the ground.", FALSE, ch, 0, victim, TO_ROOM);
                affect_from_char(victim, SPELL_LEVITATE);
            }

        if (float_dam)          /* reduced damage for sanctuary */
            if (IS_AFFECTED(victim, CHAR_AFF_SANCTUARY))
                float_dam /= 2;

        stun_dam = phys_dam = float_dam;

        /* vary stun_dam based on attack type */
        switch (attacktype) {
        case TYPE_BOW:
        case SKILL_THROW:
            stun_dam *= 50;
            break;
        default:
            stun_dam *= 100;
            break;
        }
        if (stun_dam)           /* to avoid that division by 0 thing */
            stun_dam /= 100;

        /* look to see if they're wearing armor */
        if ((varm = victim->equipment[loc]))
            if (varm->obj_flags.type == ITEM_ARMOR) {
                phys_dam -= absorbtion(varm->obj_flags.value[0]);
                stun_dam -= absorbtion(varm->obj_flags.value[0]);
            }

        /* handle natural armor */
        phys_dam -= absorbtion(GET_ARMOR(victim));
        stun_dam -= absorbtion(GET_ARMOR(victim));

        /* modify damaged based on location */
        phys_dam = phys_dam * phys_damage_mult[loc];
        if (phys_dam)
            phys_dam = phys_dam / 100;

        stun_dam = stun_dam * stun_damage_mult[loc];
        if (stun_dam)
            stun_dam = stun_dam / 100;

        if ((alive = generic_damage(victim, phys_dam, 0, 0, stun_dam)) != FALSE) {
            // If victim is paralyzed, have damage give a chance to shake affect
            if (is_paralyzed(victim)) {
                struct affected_type af;
                struct affected_type *paf;
                memset(&af, 0, sizeof(af));

                paf = affected_by_spell(victim, SPELL_PARALYZE);
                if( paf != NULL && !number(0, paf->power)) {
                    af.type = SPELL_SLOW;
                    af.duration = paf->duration;
                    af.power = paf->power;
                    af.magick_type = paf->magick_type;
                    af.modifier = -(paf->power);
                    af.location = CHAR_APPLY_AGL;
                    af.bitvector = CHAR_AFF_SLOW;
                    affect_join(victim, &af, FALSE, TRUE);
                    affect_from_char(victim, SPELL_PARALYZE);
                }
         
                paf = affected_by_spell(victim, POISON_PERAINE);
                if( paf != NULL && !number(0, paf->power)) {
                    af.type = SPELL_SLOW;
                    af.duration = paf->duration;
                    af.power = paf->power;
                    af.magick_type = MAGICK_NONE;
                    af.modifier = -(paf->power);
                    af.location = CHAR_APPLY_AGL;
                    af.bitvector = CHAR_AFF_SLOW;
                    affect_join(victim, &af, FALSE, TRUE);
                    affect_from_char(victim, POISON_PERAINE);
                }
            }

            
            if (IS_IN_SAME_TRIBE(victim, ch)
                && IS_NPC(victim)
                && !does_hate(victim, ch))
                add_to_hates(victim, ch);

            /* here is where we might want to add a random chance to move
             * toward the attacker, or flee
             */
        }
/* At this point they are DEAD, but need to do a couple of things 
   before going to "die( victim );" */
        else {
            if (ch == victim)
                switch (attacktype) {
                default:
                    sprintf(buf,
                            "%s %s%s%shas died from something in missile_damage in room #%d,"
                            " have a look.", IS_NPC(victim) ? MSTR(victim, short_descr)
                            : GET_NAME(victim), !IS_NPC(victim) ? "(" : "",
                            !IS_NPC(victim) ? victim->account : "", !IS_NPC(victim) ? ") " : "",
                            victim->in_room->number);
                    break;
            } else
                switch (attacktype) {
                case TYPE_BOW:
                    sprintf(buf,
                            "%s %s%s%shas been killed by something shot by %s %s%s%sin room #%d.",
                            IS_NPC(victim) ? MSTR(victim, short_descr)
                            : GET_NAME(victim), !IS_NPC(victim) ? "(" : "",
                            !IS_NPC(victim) ? victim->account : "", !IS_NPC(victim) ? ") " : "",
                            IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch),
                            !IS_NPC(ch) ? "(" : "", !IS_NPC(ch) ? ch->account : "",
                            !IS_NPC(ch) ? ") " : "", ch->in_room->number);
                    break;
                case SKILL_THROW:
                    sprintf(buf,
                            "%s %s%s%shas been killed by something thrown by %s %s%s%sin room #%d.",
                            IS_NPC(victim) ? MSTR(victim, short_descr)
                            : GET_NAME(victim), !IS_NPC(victim) ? "(" : "",
                            !IS_NPC(victim) ? victim->account : "", !IS_NPC(victim) ? ") " : "",
                            IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch),
                            !IS_NPC(ch) ? "(" : "", !IS_NPC(ch) ? ch->account : "",
                            !IS_NPC(ch) ? ") " : "", ch->in_room->number);
                    break;
                default:
                    sprintf(buf,
                            "%s %s%s%shas been killed by missile_damage %s %s%s%sin room #%d, amen.",
                            IS_NPC(victim) ? MSTR(victim, short_descr)
                            : GET_NAME(victim), !IS_NPC(victim) ? "(" : "",
                            !IS_NPC(victim) ? victim->account : "", !IS_NPC(victim) ? ") " : "",
                            IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch),
                            !IS_NPC(ch) ? "(" : "", !IS_NPC(ch) ? ch->account : "",
                            !IS_NPC(ch) ? ") " : "", ch->in_room->number);
                    break;
                }

            if (IS_NPC(victim))
                shhlog(buf);
            else {
                //                gamelog(buf);
                // Added by Nessalin 1/22/2005 to work with new quiet flags
                death_log(buf);

                /* Added by Morg to allow us to see how they died */
                playing_time =
                    real_time_passed((time(0) - victim->player.time.logon) +
                                     victim->player.time.played, 0);

                if (!IS_IMMORTAL(victim)
                    && (playing_time.day > 0 || playing_time.hours > 2
                        || (victim->player.dead && !strcmp(victim->player.dead, "rebirth")))
                    && !IS_SET(victim->specials.act, CFL_NO_DEATH)) {
                    if (victim->player.info[1])
                        free(victim->player.info[1]);
                    victim->player.info[1] = strdup(buf);
                }
            }

            /* And now I think we're ready to finish the job. They die. */
            die(victim);
        }
    }

    /* check for flag */
    check_criminal_flag(ch, victim);

    return alive;
}


void
show_init_combat(CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (!ch || !victim)
        return;

    cprintf(ch, "You attack %s.\n\r", PERS(ch, victim));

    cprintf(victim, "%s attacks you.\n\r", capitalize(PERS(victim, ch)));

    act("$n attacks $N.", FALSE, ch, NULL, victim, TO_NOTVICT);
}


bool
process_energy_shield(CHAR_DATA * ch, CHAR_DATA * victim, int phys_dam, int stun_dam)
{

    char buf[MAX_STRING_LENGTH];
    struct time_info_data playing_time;
    struct affected_type *tmp_af;
    float float_phys_dam = (float) phys_dam;
    float float_stun_dam = (float) stun_dam;

    act("$n shudders as a jolt of energy from $N enters $s body.", FALSE, ch, 0, victim,
        TO_ROOM);
    act("A bolt of energy arcs from $N, striking you!", TRUE, ch, 0, victim, TO_CHAR);
    act("The energy shield around you arcs into $n, and $e shudders.", TRUE, ch, 0, victim,
        TO_VICT);

    for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next)
        if (tmp_af->type == SPELL_ENERGY_SHIELD) {
            float_phys_dam *= (float)(((tmp_af->power * 10) + 30) / 100.0);
            float_stun_dam *= (float)(((tmp_af->power * 10) + 30) / 100.0);
        }
    phys_dam = (int) float_phys_dam;
    stun_dam = (int) float_stun_dam;

    if (!generic_damage(ch, phys_dam, 0, 0, stun_dam)) {
        stop_all_fighting(victim);
        snprintf(buf, sizeof(buf),
                 "%s has died to %s's energy shield in room %d Bzzrt!\n\r", GET_NAME(ch),
                 GET_NAME(victim), victim->in_room->number);
        send_to_monitors(ch, victim, buf, MONITOR_FIGHT);
        if (IS_NPC(ch))
            shhlog(buf);
        else {
            death_log(buf);

            /* Added by Morg to allow us to see how they died */
            playing_time =
                real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played,
                                 0);

            if (!IS_IMMORTAL(ch)
                && (playing_time.day > 0 || playing_time.hours > 2
                    || (ch->player.dead && !strcmp(ch->player.dead, "rebirth")))
                && !IS_SET(ch->specials.act, CFL_NO_DEATH)) {
                if (ch->player.info[1])
                    free(ch->player.info[1]);

                ch->player.info[1] = strdup(buf);
            }
        }

        die(ch);
        return FALSE;
    }

    if ((GET_POS(ch) <= POSITION_SLEEPING))
        stop_all_fighting(ch);

    return TRUE;
}

void
send_fight_messages(CHAR_DATA *ch, CHAR_DATA *victim, int attacktype, int miss){
    int i, j, nr;
    struct message_type *messages;

    for (i = 0; i < MAX_MESSAGES; i++) {
        if (fight_messages[i].a_type == attacktype) {
            nr = dice(1, fight_messages[i].number_of_attacks);
            for (j = 1, messages = fight_messages[i].msg; (j < nr) && messages; j++)
                messages = messages->next;

            if (IS_IMMORTAL(victim)) {
                act(messages->god_msg.attacker_msg, FALSE, ch,
                 ch->equipment[EP], victim, TO_CHAR);
                act(messages->god_msg.victim_msg, FALSE, ch,
                 ch->equipment[EP], victim, TO_VICT);
                act(messages->god_msg.room_msg, FALSE, ch, ch->equipment[EP],
                 victim, TO_NOTVICT);
            } else if (!miss) {
                if (GET_HIT(victim) < -10) {
                    act(messages->die_msg.attacker_msg, FALSE, ch,
                     ch->equipment[EP], victim, TO_CHAR);
                    act(messages->die_msg.victim_msg, FALSE, ch,
                     ch->equipment[EP], victim, TO_VICT);
                    act(messages->die_msg.room_msg, FALSE, ch, 
                     ch->equipment[EP], victim, TO_NOTVICT);
                } else {
                    act(messages->hit_msg.attacker_msg, FALSE, ch, 
                     ch->equipment[EP], victim, TO_CHAR);
                    act(messages->hit_msg.victim_msg, FALSE, ch,
                     ch->equipment[EP], victim, TO_VICT);
                    act(messages->hit_msg.room_msg, FALSE, ch,
                     ch->equipment[EP], victim, TO_NOTVICT);
                }
            } /*  End damage != 0 ... Kel */
            else {
                act(messages->miss_msg.attacker_msg, FALSE, ch,
                 ch->equipment[EP], victim, TO_CHAR);
                act(messages->miss_msg.victim_msg, FALSE, ch, 
                 ch->equipment[EP], victim, TO_VICT);
                act(messages->miss_msg.room_msg, FALSE, ch, 
                 ch->equipment[EP], victim, TO_NOTVICT);
            }               /* end miss message */
        }                   /* end found match */
    }                       /* end for messages */
}

bool
check_combat_gain_wait(CHAR_DATA *ch, int curr_skill, long last_gain) {
    int wt = 4 * MAX(8, 60 - wis_app[GET_WIS(ch)].learn + (curr_skill / 7));

    if (time(0) - last_gain < SECS_PER_REAL_MIN * wt)
        return FALSE;

    return TRUE;
}


/* weapon/spell damage to victims in room */
bool
ws_damage(CHAR_DATA * ch, CHAR_DATA * victim, int hpdam, int manadam,
          int mvdam, int stundam, int attacktype, int elementtype)
{
    struct affected_type *tmp_af;

    char buf[MAX_STRING_LENGTH];
    int miss, loc;
    int percent;
    int merciful;
    float phys_dam = 0, stun_dam = 0, mv_dam = 0, mana_dam = 0;
    float float_hpdam = (float) hpdam;
    float float_stundam = (float) stundam;
    float float_manadam = (float) manadam;
    float float_mvdam = (float) mvdam;
    char *undying = 0;
    bool alive = TRUE;

    CHAR_DATA *tch;
    OBJ_DATA *varm;
    OBJ_DATA *burnit = 0;

    /* no damage for immortals */
    if (IS_IMMORTAL(victim)) {
        float_hpdam = 0;
        float_stundam = 0;
        float_manadam = 0;
        float_mvdam = 0;
    }

    /* can't kill your mount */
    if (ch->specials.riding == victim) {
        act("You should get off of $N before trying to kill $M.", FALSE, ch, 0, victim, TO_CHAR);
        return alive;
    }

    /* mounted attack? */
    if (ch->specials.riding) {
        if (!has_skill(ch, SKILL_RIDE)
            /*      if( !ch->skills[SKILL_RIDE]  */
            || (has_skill(ch, SKILL_RIDE)
                /*       || ( ch->skills[SKILL_RIDE] */
                && ((number(1, 50) > get_skill_percentage(ch, SKILL_RIDE))
                    /* && ( ( number( 1, 50 ) > ch->skills[SKILL_RIDE]->learned ) */
                    || ((GET_GUILD(ch) == GUILD_RANGER)
                        && (number(1, 60) > ch->skills[SKILL_RIDE]->learned))))) {
            act("$N throws you from $S back!", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
            act("You throw $n from your back.", FALSE, ch, 0, ch->specials.riding, TO_VICT);
            act("$N throws $n from $S back.", FALSE, ch, 0, ch->specials.riding, TO_NOTVICT);
            ch->specials.riding = (CHAR_DATA *) 0;

            set_char_position(ch, POSITION_SITTING);

            if (!generic_damage(ch, number(0, 3), 0, 0, number(5, 20)))
                return FALSE;

            if (is_in_command_q(ch))
                rem_from_command_q(ch);

            gain_skill(ch, SKILL_RIDE, 2);
            snprintf(buf, sizeof(buf), "%s has been thrown from his mount\n\r", GET_NAME(ch));
            send_to_monitor(ch, buf, MONITOR_FIGHT);
        } 
    }

    /* can't guard if you are being assulted */
    if (ch->specials.guarding && (GET_GUILD(ch) != GUILD_WARRIOR))
        stop_guarding(ch);

    /* can't fight while invisible */
    if (IS_AFFECTED(ch, CHAR_AFF_INVISIBLE))
        appear(ch);

    /* can't keep certain psionics active when being aggressive */
    aggressive_psionics(ch);

    /* make sure its all clean n spiffy */
    if (victim != ch) {
        if (GET_POS(victim) > POSITION_SLEEPING) {
            if (!victim->specials.fighting) {
                if (CAN_SEE(victim, ch)) {
                    set_fighting(victim, ch);
                } else {
                    /* check blind fighting here */
                    if (number(1, 100) < get_skill_percentage(victim, SKILL_BLIND_FIGHT)) {
                        set_fighting(victim, ch);
                    } else {
                        gain_skill(victim, SKILL_BLIND_FIGHT, 1);
                    }
                }
            }
        }

        if (GET_POS(ch) > POSITION_SLEEPING) {
            if (!ch->specials.fighting && !is_char_ethereal(ch)
                && !is_char_ethereal(victim)) {
                set_fighting(ch, victim);
            }
        }
    }

    if (affected_by_spell(ch, SPELL_LEVITATE)) {
        act("You stop floating and quickly drop to the ground.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n stops floating and quickly drops to the ground.", FALSE, ch, 0, 0, TO_ROOM);
        if (affected_by_spell(ch, SPELL_LEVITATE))
            affect_from_char(ch, SPELL_LEVITATE);
    }

    /* don't follow people who are killing you */
    if (victim->master == ch)
        stop_follower(victim);

    /* or guard */
    if (victim->specials.guarding == ch)
        stop_guard(victim, 0);

    /* nor while hiding */
    if (IS_AFFECTED(ch, CHAR_AFF_HIDE))
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);

    /* can't kill merchants for now */
    if (is_merchant(victim)) {
        snprintf(buf, sizeof(buf), "%s has attacked %s, a merchant!\n\r", GET_NAME(ch),
                 GET_NAME(victim));
        send_to_monitors(ch, victim, buf, MONITOR_FIGHT);
        check_criminal_flag(ch, victim);
        extract_merchant(victim);
        return FALSE;
    }

    /* subdued? */
    if (IS_AFFECTED(victim, CHAR_AFF_SUBDUED)) {
        for (tch = victim->in_room->people; tch; tch = tch->next_in_room)
            if (tch->specials.subduing == victim)
                break;

        /*  if hitter is also subduer  */
        if (tch == ch) {
            REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_SUBDUED);
            ch->specials.subduing = (CHAR_DATA *) 0;
            float_hpdam *= 3;
            float_stundam *= 3;
        } else if (tch) {       /*  someone else is holding victim  */
            if (!check_str(tch, -float_hpdam)) {  /* added penalty - Kel */
                REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_SUBDUED);
                tch->specials.subduing = (CHAR_DATA *) 0;
            }
            float_hpdam *= 4;
            float_stundam *= 4;
        }

        float_hpdam = MIN(float_hpdam, 50); /*  Kel  quick fix  */
        float_stundam = MIN(float_stundam, 50);
    }

    if ((victim != ch) && ch->in_room && victim->in_room) {
        for (tch = victim->in_room->people; tch; tch = tch->next_in_room)
            if (IS_IN_SAME_TRIBE(tch, victim)
                && IS_NPC(tch) && !does_hate(tch, ch))
                add_to_hates(tch, ch);

        if (!check_agl(ch, 0)
            || (attacktype != SKILL_THROW && attacktype != SKILL_BACKSTAB))
            check_criminal_flag(ch, victim);
    }

    miss = ((float_hpdam == 0) && (float_stundam == 0) && (float_mvdam == 0) && (float_manadam == 0));

    if (IS_ATTACK(attacktype)) {
        if (miss && GET_GUILD(ch)) {
            /* offense slow hack - hal */
            percent = 4 * number(1, 1000);
            // loading your self down to fail won't let you gain
            percent += MAX(get_encumberance(ch) - 4, 0) * 10;

            // prevent using riding as a quick-gain
            if (ch->specials.riding) {
               if (has_skill(ch, SKILL_RIDE))
                   // subtract riding skill - 50, or offense, whichever is less
                   percent -= MIN(0, get_skill_percentage(ch, SKILL_RIDE) - 50);
               else
                   // no riding skill, 50% penalty
                   percent += 50;
            }

            if ((percent <= guild[(int) GET_GUILD(ch)].off_kpc)
                && check_combat_gain_wait(ch, ch->abilities.off, ch->abilities.off_last_gain)) {
                int oldsk = ch->abilities.off;

                /* increment the base ability, and current */
                ch->abilities.off += 1;
                ch->abilities.off_last_gain = time(0);
                ch->abilities.off = MIN(ch->abilities.off, find_max_offense_for_char(ch));
                if (!IS_NPC(ch))
                   gamelogf("gain_skill: %s (%s) gained 1 points of skill in 'offense' (%d => %d)", MSTR(ch, name), ch->account, oldsk, ch->abilities.off);

                ch->tmpabilities.off += 1;
                ch->tmpabilities.off_last_gain = time(0);
                ch->tmpabilities.off = MIN(ch->tmpabilities.off, find_max_offense_for_char(ch));
            }
        } else if (!miss && GET_GUILD(victim)) {
            /* defense slow hack - hal */
            percent = 4 * number(1, 1000);
            // loading your self down to fail won't let you gain
            percent += MAX(get_encumberance(victim) - 4, 0) * 10;

            // prevent using riding as a quick-gain
            if (victim->specials.riding) {
               if (has_skill(victim, SKILL_RIDE))
                   // subtract riding skill - 50, or defense, whichever is less
                   percent -= MIN(0, get_skill_percentage(victim, SKILL_RIDE) - 50);
               else
                   // no riding skill, 50% penalty
                   percent += 50;
            }

            if ((percent <= guild[(int) GET_GUILD(victim)].def_kpc)
                && check_combat_gain_wait(victim, victim->abilities.def,
                     victim->abilities.def_last_gain)) {
                /* increment the base ability, and current */
                int oldsk = victim->abilities.def;
                victim->abilities.def += 1;
                victim->abilities.def_last_gain = time(0);
                victim->abilities.def = MIN(victim->abilities.def, 95);
                if (!IS_NPC(victim))
                   gamelogf("gain_skill: %s (%s) gained 1 points of skill in 'defense' (%d => %d)", MSTR(victim, name), victim->account, oldsk, victim->abilities.def);

                victim->tmpabilities.def += 1;
                victim->tmpabilities.def_last_gain = time(0);
                victim->tmpabilities.def = MIN(victim->tmpabilities.def, 95);
            }
        }
    }

    merciful = IS_SET(ch->specials.mercy, MERCY_KILL);

    /* Enraged muls do not show mercy */
    if (GET_RACE(ch) == RACE_MUL && affected_by_spell(ch, AFF_MUL_RAGE))
        merciful = 0;

    if ((GET_POS(victim) <= POSITION_SLEEPING) && merciful && (attacktype != SPELL_POISON)) {
        act("Showing mercy, you withhold the killing blow.\n\r", FALSE, ch, 0, 0, TO_CHAR);
        if (ch && victim) {
            stop_fighting(ch, victim);
        } else
            gamelog("ch or victim not found when applying merciful code.");

        snprintf(buf, sizeof(buf), "Mercifully, %s spares %s\n\r", GET_NAME(ch), GET_NAME(victim));
        send_to_monitors(ch, victim, buf, MONITOR_FIGHT);
        return alive;
    }

    /* initialize the location */
    if (!(loc = get_random_hit_loc(victim, attacktype)))
        loc = WEAR_BODY;

    phys_dam = float_hpdam;
    stun_dam = float_stundam;
    mv_dam = float_mvdam;
    mana_dam = float_manadam;

    /* reduce by armor before wtype/location */
    if (!miss && IS_ATTACK(attacktype)) {
        if ((varm = victim->equipment[loc]) != NULL) {
            if (varm->obj_flags.type == ITEM_ARMOR) {
                phys_dam -= absorbtion(varm->obj_flags.value[0]);
                stun_dam -= absorbtion(varm->obj_flags.value[0]);
                varm = damage_armor(varm, phys_dam, attacktype);
            }
        }

        phys_dam -= absorbtion(GET_ARMOR(victim));
        stun_dam -= absorbtion(GET_ARMOR(victim));

        phys_dam *= phys_damage_mult[loc];
        phys_dam /= 100;
        stun_dam *= stun_damage_mult[loc];
        stun_dam /= 100;
    }

    if (phys_dam > 10 
     && attacktype != TYPE_BLUDGEON 
     && attacktype != SKILL_SAP) {
        if (victim->equipment[loc]) {
            sflag_to_obj(victim->equipment[loc], OST_BLOODY);
            if ((victim->equipment[loc]->obj_flags.material == MATERIAL_SILK
                 && (number(1, 100) < 40))
                || (victim->equipment[loc]->obj_flags.material == MATERIAL_CLOTH
                    && (number(1, 100) < 60))) {
                if (IS_SET(victim->equipment[loc]->obj_flags.state_flags, OST_TORN))
                    sflag_to_obj(victim->equipment[loc], OST_TATTERED);
                else
                    sflag_to_obj(victim->equipment[loc], OST_TORN);
            }
        }
    }

    switch (attacktype) {
    case TYPE_SLASH:
    case TYPE_WHIP:
    case TYPE_CLAW:
        stun_dam *= 40;
        break;
    case TYPE_CHOP:
        stun_dam *= 50;
        break;
    case TYPE_PIKE:
    case TYPE_POLEARM:
    case TYPE_TRIDENT:
    case TYPE_KNIFE:
    case TYPE_RAZOR:
    case TYPE_PIERCE:
    case TYPE_STAB:
    case TYPE_STING:
        stun_dam *= 110;
        break;
    case TYPE_BLUDGEON:
        stun_dam *= 180;
        phys_dam *= 90;
        phys_dam /= 100;
        break;
    case TYPE_PECK:
    case TYPE_BITE:
    case TYPE_PINCH:
        stun_dam *= 70;
        break;

    case TYPE_GORE:
        stun_dam *= 130;
        break;
    case TYPE_HIT:
        stun_dam *= 190;
        phys_dam *= 25;
        phys_dam /= 100;
        break;
    case SKILL_SAP:            /* special bludgeoning attack to the head */
        /* lets go ahead and let armor be a factor here */
        if ((varm = victim->equipment[WEAR_HEAD]))
            if (varm->obj_flags.type == ITEM_ARMOR) {
                phys_dam -= absorbtion(varm->obj_flags.value[0]);
                stun_dam -= absorbtion(varm->obj_flags.value[0]);
                varm = damage_armor(varm, phys_dam, TYPE_BLUDGEON);
            }

        phys_dam -= absorbtion(GET_ARMOR(victim));
        stun_dam -= absorbtion(GET_ARMOR(victim));

        phys_dam *= 50;
        phys_dam /= 100;
        stun_dam *= 180;        /* 180 for bludgeon */

        /* Commenting this out because skill_sap() has already figured in
         * multipliers for the weapon. Adding this applies another large
         * multiplier which results in stun points which are extremely large
         * values.  This results in sap always knocking-out the opponent.
         * - Tiernan 1/17/02
         */

        /* head location after armor absorbtion */
        /* stun_dam *= stun_damage_mult[WEAR_HEAD]; */
        break;

    default:
        stun_dam *= 100;
        break;
    }
    stun_dam /= 100;


    /* #define SHOW_LOCATION */
#ifdef SHOW_LOCATION
    sprintf(buf, "race type is %d, location is %s (%d)", GET_RACE_TYPE(victim),
            hit_loc_name[loc][GET_RACE_TYPE(victim)], loc);
    gamelog(buf);
#endif


    if (affected_by_spell(victim, SPELL_FIRE_ARMOR) && (!miss || !IS_ATTACK(attacktype)))
        for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next)
            if (tmp_af->type == SPELL_FIRE_ARMOR)
                if (number(0, 100) < (tmp_af->power * 10)) {

                    if ((burnit = ch->equipment[EP])
                        && (((number(0, 100) < 50) && burnit->obj_flags.material == MATERIAL_WOOD)
                            || ((number(0, 100) < 15)
                                && burnit->obj_flags.material == MATERIAL_CHITIN)
                            || ((number(0, 100) < 20)
                                && burnit->obj_flags.material == MATERIAL_HORN)
                            || ((number(0, 100) < 20)
                                && burnit->obj_flags.material == MATERIAL_BONE))) {
                        act("The flames surrounding $N burn $n's $p to ash!", FALSE, ch, burnit,
                            victim, TO_NOTVICT);
                        act("The flames surrounding $N burn your $p to ash!", FALSE, ch, burnit,
                            victim, TO_CHAR);
                        act("The flames surrounding you burn $n's $p to ash!", FALSE, ch, burnit,
                            victim, TO_VICT);
                        unequip_char(ch, EP);
                        extract_obj(burnit);
                    }

                    if ((burnit = ch->equipment[ES])
                        && (((number(0, 100) < 50) && burnit->obj_flags.material == MATERIAL_WOOD)
                            || ((number(0, 100) < 15)
                                && burnit->obj_flags.material == MATERIAL_CHITIN)
                            || ((number(0, 100) < 20)
                                && burnit->obj_flags.material == MATERIAL_HORN)
                            || ((number(0, 100) < 20)
                                && burnit->obj_flags.material == MATERIAL_BONE))) {
                        act("The flames surrounding $N burn $n's $p to ash!", FALSE, ch, burnit,
                            victim, TO_NOTVICT);
                        act("The flames surrounding $N burn your $p to ash!", FALSE, ch, burnit,
                            victim, TO_CHAR);
                        act("The flames surrounding you burn $n's $p to ash!", FALSE, ch, burnit,
                            victim, TO_VICT);
                        unequip_char(ch, ES);
                        extract_obj(burnit);
                    }

                    if ((burnit = ch->equipment[ETWO])
                        && (((number(0, 100) < 50) && burnit->obj_flags.material == MATERIAL_WOOD)
                            || ((number(0, 100) < 15)
                                && burnit->obj_flags.material == MATERIAL_CHITIN)
                            || ((number(0, 100) < 20)
                                && burnit->obj_flags.material == MATERIAL_HORN)
                            || ((number(0, 100) < 20)
                                && burnit->obj_flags.material == MATERIAL_BONE))) {
                        act("The flames surrounding $N burn $n's $p to ash!", FALSE, ch, burnit,
                            victim, TO_NOTVICT);
                        act("The flames surrounding $N burn your $p to ash!", FALSE, ch, burnit,
                            victim, TO_CHAR);
                        act("The flames surrounding you burn $n's $p to ash!", FALSE, ch, burnit,
                            victim, TO_VICT);
                        unequip_char(ch, ETWO);
                        extract_obj(burnit);
                    }
    
                    if (!generic_damage(ch,  2 * tmp_af->power, 0, 0, 0)) {
                        act("The flames surrounding $N's burn you as you get close enough to"
                            " attack, burning you to death!", FALSE, ch, 0, victim, TO_CHAR);
                        act("The flames surrounding you burn $n as $e gets close enough to"
                            " attack, burning $m to death!!", FALSE, ch, 0, victim, TO_VICT);
                        act("The flames surrounding $N's burn $n as $e gets close enough to"
                            " attack, burning $m to death!", FALSE, ch, 0, victim, TO_NOTVICT);                        
                        
                        stop_all_fighting(victim);
                        snprintf(buf, sizeof(buf),
                                 "%s has died to %s's fire shield in room %d Kaboom!\n\r", GET_NAME(ch),
                                 GET_NAME(victim), victim->in_room->number);
                        send_to_monitors(ch, victim, buf, MONITOR_FIGHT);
                        if (IS_NPC(ch))
                            shhlog(buf);
                        else
                            death_log(buf);
                        die(ch);
                        break;
                    } else {
                        act("The flames surrounding $N's burn you as you get close enough to attack!",
                            FALSE, ch, 0, victim, TO_CHAR);
                        act("The flames surrounding you burn $n as $e gets close enough to attack!",
                            FALSE, ch, 0, victim, TO_VICT);
                        act("The flames surrounding $N's burn $n as $e gets close enough to attack!",
                            FALSE, ch, 0, victim, TO_NOTVICT);
                        break;
                    }  
                }
    // WTF??  How could ch ever be null here??
    if (!ch)
        return 0;

    if (affected_by_spell(victim, SPELL_WIND_ARMOR) && (!miss || !IS_ATTACK(attacktype)))
        for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next)
            if (tmp_af->type == SPELL_WIND_ARMOR)
                if (number(0, 100) < (tmp_af->power * 10)) {
                    show_init_combat(ch, victim);
                    act("The winds surrounding $N reach out and slam against $n!", FALSE, ch,
                        burnit, victim, TO_NOTVICT);
                    act("The winds surrounding $N reach out and slam against you!", FALSE, ch,
                        burnit, victim, TO_CHAR);
                    act("The winds surrounding you reach out and slam against $n!", FALSE, ch,
                        burnit, victim, TO_VICT);
                    spell_repel(tmp_af->power, SPELL_TYPE_SPELL, victim, ch, 0);
                    break;
                }
    // repel may have pushed them out of the room
    // -Nessalin 7/3/2004
    if (ch->in_room != victim->in_room)
        return alive;

    /*  Doing the Invuln Boogie, right this time. - Kel  */
    if ((IS_AFFECTED(victim, CHAR_AFF_INVULNERABILITY))
        && (!miss || !IS_ATTACK(attacktype))) {
        affect_from_char(victim, SPELL_INVULNERABILITY);
        send_to_char("The creamy shell around you fades away.\n\r", victim);
        act("The creamy shell around $n fades away.", FALSE, victim, 0, 0, TO_ROOM);
        if (IS_AFFECTED(victim, CHAR_AFF_INVULNERABILITY))
            REMOVE_BIT(victim->specials.affected_by, CHAR_AFF_INVULNERABILITY);
        phys_dam = stun_dam = mv_dam = mana_dam = 0;
        if (GET_POS(victim) == POSITION_SLEEPING && !knocked_out(victim)) {
            if (!victim->specials.fighting)
                set_fighting(victim, ch);
            set_char_position(victim, POSITION_FIGHTING);
            act("$N wakes, and scrambles to $S feet.", FALSE, ch, 0, victim, TO_NOTVICT);
            act("You wake and scramble to your feet after $n attacks you.", TRUE, ch, 0, victim,
                TO_VICT);
        }
        return alive;
    }

    if ((affected_by_spell(victim, SPELL_SHADOW_ARMOR))
        && (elementtype < TYPE_ELEM_ALL)
        && (!miss || !IS_ATTACK(attacktype)))
        for (tmp_af = victim->affected; tmp_af; tmp_af = tmp_af->next) {
            if (tmp_af->type == SPELL_SHADOW_ARMOR)
                if (number(0, 100) < (tmp_af->power * 10)) {
                    phys_dam = stun_dam = 0;
                    break;
                }
        }

    if (!miss && IS_AFFECTED(victim, CHAR_AFF_SANCTUARY)) {
        phys_dam /= 2;
        stun_dam /= 2;
        mv_dam /= 2;
        mana_dam /= 2;
    }

    phys_dam = MIN(phys_dam, 200);
    phys_dam = MAX(phys_dam, 0);

    stun_dam = MIN(stun_dam, 200);
    stun_dam = MAX(stun_dam, 0);

    mv_dam = MIN(mv_dam, 200);
    mv_dam = MAX(mv_dam, 0);

    mana_dam = MIN(mana_dam, 200);
    mana_dam = MAX(mana_dam, 0);

    undying = find_ex_description("[UNDYING]", victim->ex_description, TRUE);
    merciful = IS_SET(ch->specials.mercy, MERCY_KILL);

    if (merciful) {
        if ((GET_HIT(victim) - phys_dam) < -4) {
            if (((GET_GUILD(ch) == GUILD_WARRIOR) && (number(1, 10) < 8)) || (number(1, 10) < 4))
                phys_dam = GET_HIT(victim) + 4;
        }
    }

    if (undying && ((GET_HIT(victim) - phys_dam) < -10)) {
        if (attacktype == SPELL_DISPEL_MAGICK) {
            rem_char_extra_desc_value(victim, "[UNDYING]");
            send_to_char("The magick preserving your life has been cancelled.\n\r", victim);
            act("The fiery light flashes through $n's form, leaving it a motionless corpse.",
                FALSE, victim, 0, 0, TO_ROOM);
            qgamelogf(QUIET_DEATH, "%s has been destroyed by dispel magick!", victim->name);
        }
    }

    if (IS_ATTACK(attacktype)) {
        int prev_health = GET_HIT(victim);
        int prev_stun = GET_STUN(victim);

        dam_message(phys_dam, ch, victim, attacktype, miss, loc, GET_RACE_TYPE(victim));
        if ((alive = generic_damage(victim, phys_dam, mana_dam, mv_dam, stun_dam))
                != FALSE && (phys_dam || stun_dam)
            && affected_by_spell(victim, SPELL_ENERGY_SHIELD))
            alive = process_energy_shield (ch, victim, phys_dam, stun_dam);

        if (alive 
         && daze_check(victim, prev_health, phys_dam, prev_stun, stun_dam, loc)) {
            set_dazed(victim, TRUE);
        }

    } else {
        alive = generic_damage(victim, phys_dam, mana_dam, mv_dam, stun_dam);
        send_fight_messages(ch, victim, attacktype, miss);

        if( !IS_IMMORTAL(victim) ) {
            if (alive
             && (phys_dam || stun_dam)
             && affected_by_spell(victim, SPELL_ENERGY_SHIELD))
                alive = process_energy_shield (ch, victim, phys_dam, stun_dam);

            if (is_in_command_q(victim)) {
                act("Your concentration is broken!", FALSE, victim, 0, 0, TO_CHAR);
                rem_from_command_q(victim);
            }
        }
    }                           /* end else !ATTACK_TYPE */

    // Let empaths know that the victim was injured
    if (phys_dam > 0) {
        snprintf(buf, sizeof(buf), "%s experiences pain.", MSTR(victim, short_descr));
        send_to_empaths(victim, buf);
    }

    if (GET_POS(victim) < POSITION_SLEEPING) {
        stop_fighting(ch, victim);
        stop_all_fighting(victim);      /*  Kel  */
    }

    if (GET_POS(victim) == POSITION_SLEEPING && !knocked_out(victim)) {
        if (!victim->specials.fighting)
            set_fighting(victim, ch);
        set_char_position(victim, POSITION_FIGHTING);
        act("$N wakes, and scrambles to $S feet.", FALSE, ch, 0, victim, TO_NOTVICT);
        act("You wake and scramble to your feet after $n attacks you.", TRUE, ch, 0, victim,
            TO_VICT);
    }

    if (GET_POS(victim) == POSITION_DEAD) {
        alive = 0;
        if (ch == victim) {
            switch (attacktype) {
            case SPELL_POISON:
                sprintf(buf, "%s %s%s%shas died from poison in room #%d.",
                        IS_NPC(victim) ? MSTR(victim, short_descr)
                        : GET_NAME(victim), !IS_NPC(victim) ? "(" : "",
                        !IS_NPC(victim) ? victim->account : "", !IS_NPC(victim) ? ") " : "",
                        victim->in_room->number);
                break;
            default:
                sprintf(buf,
                        "%s %s%s%shas died from loss of blood in room #%d, come have a look.",
                        IS_NPC(victim) ? MSTR(victim, short_descr)
                        : GET_NAME(victim), !IS_NPC(victim) ? "(" : "",
                        !IS_NPC(victim) ? victim->account : "", !IS_NPC(victim) ? ") " : "",
                        victim->in_room->number);
                break;
            }
        } else {
            sprintf(buf, "%s %s%s%shas been killed by %s %s%s%sin room #%d, amen.",
                    IS_NPC(victim) ? MSTR(victim, short_descr) : GET_NAME(victim),
                    !IS_NPC(victim) ? "(" : "",
                    !IS_NPC(victim) ? victim->account : "",
                    !IS_NPC(victim) ? ") " : "",
                    IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch),
                    !IS_NPC(ch) ? "(" : "",
                    !IS_NPC(ch) ? ch->account : "",
                    !IS_NPC(ch) ? ") " : "",
                    ch->in_room->number);
        }

        die_with_message(ch, victim, buf );
    }
    return alive;
}

void
die_with_message(CHAR_DATA *ch, CHAR_DATA *victim, char *buf ) {
    struct time_info_data playing_time;

    if (IS_NPC(victim))
        shhlog(buf);
    else {
        death_log(buf);

        /* Added by Morg to allow us to see how they died */
        playing_time =
            real_time_passed((time(0) - victim->player.time.logon) + victim->player.time.played,
                             0);

        if (!IS_IMMORTAL(victim)
            && (playing_time.day > 0 || playing_time.hours > 2
                || (victim->player.dead && !strcmp(victim->player.dead, "rebirth")))
            && !IS_SET(victim->specials.act, CFL_NO_DEATH)) {
            if (victim->player.info[1])
                free(victim->player.info[1]);

            victim->player.info[1] = strdup(buf);
        }
    }

    strncat(buf, "\n\r", sizeof(buf));
    send_to_monitors(ch, victim, buf, MONITOR_FIGHT);
    die(victim);
}

int
count_opponents(CHAR_DATA * ch)
{
    int n = 0;
    CHAR_DATA *f;

    for (f = combat_list; f; f = f->next_fighting)
        if (f->specials.fighting == ch 
         && !trying_to_disengage(f)
         && f->in_room == ch->in_room)
            n++;
    return n;
}



int
run_damage_specials(CHAR_DATA * ch, OBJ_DATA * wp_pri, CHAR_DATA * victim, OBJ_DATA * shield_vict,
                    int dam, int c_wtype, int actual_damage)
{
    bool alive = TRUE;
    bool ch_alive = TRUE;
    int ran_damage = FALSE;
    CHAR_DATA *tch = 0;

    on_damage_attacker = ch;
    on_damage_defender = victim;


    /* allow a script to start after a mobile is damaged    */
    /* This should probably be driven into ws_damage but    */
    /* ws_damage would need to have the weapon passed to it. */
    /* This is for things like a special where a mobile     */
    /* can do additional damage to the weapon attacking it, */
    /* or does an auto-heal or somesuch special             */
    /* The script shoud be invoked with 2 paramaters        */
    /*     1st for damage done (in HP)                      */
    /*     2nd one for weapon type (integer)                */
    /*     3rd for actual damage done (after armor & loc)   */
    /* otherwise ch is attacking character, npc is attacked */
    /* and obj is the weapon used to attack (if any)        */
    /*                                                      */
    /* If both attacker or defender has script, then        */
    /* on damage is called 2 times, each round              */
    /* once when they get attacked, and once when they      */
    /* attack. To determine who was attacked,check          */
    /*                                                      */
    /* global on_damage_attacker and                        */
    /* global on_damage_defender                            */
    /*                                                      */
    /*                                                      */
    /* Added 2003/02/03 -Tenebrius                          */
    /*     Suggested by Zagren                              */

    if (alive && has_special_on_cmd(ch->specials.programs, NULL, CMD_ON_DAMAGE)) {
        char tmpstr[200];

        ran_damage = TRUE;
        sprintf(tmpstr, "%d %d %d", dam, c_wtype, actual_damage);
        execute_npc_program(victim, ch, CMD_ON_DAMAGE, tmpstr);

        /* in theory, either ch or victim could die from this */
        for (tch = character_list; tch && tch != ch; tch = tch->next);
        if (!tch)
            ch_alive = FALSE;
        for (tch = character_list; tch && tch != victim; tch = tch->next);
        if (!tch)
            alive = FALSE;

    }

    if (alive && ch_alive && wp_pri &&
        has_special_on_cmd(wp_pri->programs, NULL, CMD_ON_DAMAGE)) {  /* weapon used by attacker */
        char tmpstr[200];

        ran_damage = TRUE;

        sprintf(tmpstr, "%d %d %d", dam, c_wtype, actual_damage);
        execute_obj_program(wp_pri, ch, victim, CMD_ON_DAMAGE, tmpstr);

        /* in theory, either ch or victim could die from this */
        for (tch = character_list; tch && tch != ch; tch = tch->next);
        if (!tch)
            ch_alive = FALSE;
        for (tch = character_list; tch && tch != victim; tch = tch->next);
        if (!tch)
            alive = FALSE;
    }

    if (alive && ch_alive && shield_vict &&
        has_special_on_cmd(shield_vict->programs, NULL, CMD_ON_DAMAGE)) {     /* shield used by victim */
        char tmpstr[200];
        CHAR_DATA *tch;

        ran_damage = TRUE;

        sprintf(tmpstr, "%d %d %d", dam, c_wtype, actual_damage);
        execute_obj_program(shield_vict, ch, victim, CMD_ON_DAMAGE, tmpstr);

        /* in theory, either ch or victim could die from this */
        for (tch = character_list; tch && tch != ch; tch = tch->next);
        if (!tch)
            ch_alive = FALSE;
        for (tch = character_list; tch && tch != victim; tch = tch->next);
        if (!tch)
            alive = FALSE;
    } else
        ran_damage = FALSE;

    on_damage_attacker = 0;
    on_damage_defender = 0;

    return (alive && ch_alive);
}

bool
parry_skill_success(CHAR_DATA *ch, int offense, int defense) {
    int skill = 0;

    // if they have the parry skill
    if( has_skill(ch, SKILL_PARRY) ) {
        skill = get_skill_percentage(ch, SKILL_PARRY);
    } 
    // otherwise base chance based on agility
    else {
        skill = GET_AGL(ch) / 4;
    }

    // mod parry check by difference between offense and defense
    skill -= (offense - defense) / 50;
    
    return skill_success(ch, NULL, skill);
}

void
calc_offense_defense(CHAR_DATA * ch, CHAR_DATA * victim, int type, OFF_DEF_DATA *ret)
{
    OBJ_DATA *wp_pri = NULL, *wp_sec = NULL, *wp_vict = NULL;
    OBJ_DATA *shield_vict = NULL;
    int off, def, c_wtype, v_wtype;
    int prev_off, prev_def;
    int n, i;

    c_wtype = 0;                /* Initialize variable */
    off = 0;
    prev_off = 0;
    def = 0;
    prev_def = 0;

    /* Basic offense and defense */
    off += GET_OFFENSE(ch);
    def += GET_DEFENSE(victim);

    if( diffport )
        qroomlogf(QUIET_COMBAT, ch->in_room,  "base: off = %d, def = %d", off, def);

    prev_off = off;
    prev_def = def;
    /* Figure out how char is attacking... */
    n = -1;
    wp_pri = get_weapon(ch, &n);
    if (!wp_pri)
        n = -1;
    else
        c_wtype = get_weapon_type(wp_pri->obj_flags.value[3]);

    switch (n) {
    case EP:
        off += proficiency_bonus(ch, c_wtype);
        i = ES;
        wp_sec = get_weapon(ch, &i);
        if (!wp_sec) {
            i = WEAR_WRIST_L;
            wp_sec = get_weapon(ch, &i);
        }
        break;
    case WEAR_WRIST_R:
        i = ES;
        wp_sec = get_weapon(ch, &i);

        if (GET_GUILD(ch) == GUILD_WARRIOR)
            off += number(5, 20);
        if (!wp_sec) {
            i = WEAR_WRIST_L;
            wp_sec = get_weapon(ch, &i);
        }
        break;
    case ES:
        off = MAX(off + proficiency_bonus(ch, c_wtype) - 10, 1);
        wp_sec = 0;
        break;
    case ETWO:
        off += proficiency_bonus(ch, c_wtype);
        if( has_skill(ch, SKILL_TWO_HANDED))
            off += ch->skills[SKILL_TWO_HANDED]->learned;
        wp_sec = 0;
        break;
    case WEAR_WRIST_L:
        wp_sec = 0;
        if (GET_GUILD(ch) == GUILD_WARRIOR)
            off += number(1, 10);
        break;
    default:
        wp_sec = 0;
        if (GET_RACE(ch) > 0)
            c_wtype = race[(int) GET_RACE(ch)].attack->type;
        else
            c_wtype = TYPE_HIT;
        break;
    }

    /* Figure out how victim is defending... */
    /* determine ability to parry */
    n = -1;
    wp_vict = get_weapon(victim, &n);

    if (!wp_vict) {
        if (GET_RACE(victim) > 0)
            v_wtype = race[(int) GET_RACE(victim)].attack->type;
        else
            v_wtype = TYPE_HIT;
    } else
        v_wtype = get_weapon_type(wp_vict->obj_flags.value[3]);

    if (n == ETWO) {
        if (has_skill(victim, SKILL_TWO_HANDED))
            def += victim->skills[SKILL_TWO_HANDED]->learned;
    }

    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room,  
         "weapon skill: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

    if (has_skill(victim, SKILL_SHIELD) ) {
       int shield_loc = get_shield_loc(victim);

       if( shield_loc != -1 ) {
           shield_vict = victim->equipment[shield_loc];
       }
    } else
        shield_vict = 0;

    /* Offense/defense bonus/penalty for enraged muls */
    if (GET_RACE(ch) == RACE_MUL && affected_by_spell(ch, AFF_MUL_RAGE))
        off += 15;

    if (GET_RACE(victim) == RACE_MUL && affected_by_spell(victim, AFF_MUL_RAGE))
        def += 15;

    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room,  
         "mul rage: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

    /* Penalty to victim if relaxed */
    def -= ((POSITION_FIGHTING - GET_POS(victim)) * 20);

    /* Penalty to ch if relxed */
    off -= ((POSITION_FIGHTING - GET_POS(ch)) * 20);

    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room,  
         "position: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

    /* Bad conditions for victim... */

    if (!AWAKE(victim)
        || IS_SET(victim->specials.act, CFL_FROZEN)
/* Loose defense/parry if you're subdued -Morg */
        || IS_SET(victim->specials.affected_by, CHAR_AFF_SUBDUED))
/* old code -Morg
       || (ch->specials.subduing && (ch->specials.subduing == victim)))
*/
    {
        def = 0;
        if( diffport ) 
            qroomlogf(QUIET_COMBAT, ch->in_room,  
             "asleep/frozen/subdued: off = %d, def = %d", off, def);
        prev_off = off;
        prev_def = def;
    }


    /* knowledge bonuses based on how much each combatant understands their
     * opponent's weapon type.  -- Tiernan 12/2/2003
     */

#define ALLOW_OPP_BONUS
#ifdef ALLOW_OPP_BONUS
    if (proficiency_bonus(ch, v_wtype))
        off += number(0, proficiency_bonus(ch, v_wtype));
    if (proficiency_bonus(victim, c_wtype))
        def += number(0, proficiency_bonus(victim, c_wtype));
//    if( prev_off != off || prev_def != def ) {
        gamelogf("Opp's weapon knowledge: off = %d, prev_off = %d, def = %d, prev_def = %d", off, prev_off, def, prev_def);
//    }
    prev_off = off;
    prev_def = def;
#endif

    /* mounted penalties for ch and victim */

    // if the ch is riding
    if (ch->specials.riding) {
        // subtract 100 from offense
        if (has_skill(ch, SKILL_RIDE)) 
            // add riding skill - 50, or offense, whichever is less
            off += MIN(GET_OFFENSE(ch), 
             get_skill_percentage(ch, SKILL_RIDE) - 50);
        else
            // no riding skill, 50% penalty
            off -= 50;

        if (!victim->specials.riding 
         && get_char_size(victim) <= get_char_size(ch->specials.riding)
         && has_skill(ch, SKILL_RIDE)) 
            off += MIN(GET_OFFENSE(ch), 
             number(2, get_skill_percentage(ch, SKILL_RIDE)) / 2);
    } 

    // if the victim is riding
    if (victim->specials.riding) {
        // subtract 100 from defense
        if (has_skill(victim, SKILL_RIDE)) 
            // add riding skill - 50, or defense, whichever is less
            def += MIN(GET_DEFENSE(victim),
             get_skill_percentage(victim, SKILL_RIDE) - 50);
        else
            // no riding skill, 50% penalty
            def -= 50;

        if (!ch->specials.riding 
         && get_char_size(ch) <= get_char_size(victim->specials.riding)
         && has_skill(victim, SKILL_RIDE)) 
            def += MIN(GET_DEFENSE(victim), number(2, get_skill_percentage(victim, SKILL_RIDE)) / 2);
    }

    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room,  
         "Riding: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

    /* agility mods for ch and victim */
    off += agl_app[GET_AGL(ch)].reaction;
    def += agl_app[GET_AGL(victim)].reaction;

    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room,  
         "Agility Reaction: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

 /*****************************************************************************/
    /* add in up to 50 percentages for knowing how to use a weapon vs a racetype */
    /*        -Tenebrius 1/2/00                                                  */
 /*****************************************************************************/
    if (wp_pri && victim)
#undef OLD_RACETYPE_WEAPON_PROF
#ifdef OLD_RACETYPE_WEAPON_PROF
        off +=
            (50.0 * 100.0 /
             get_weapon_racetype_proficiency(ch, wp_pri->obj_flags.value[3],
                                             GET_RACE_TYPE(victim)));
#else
        off +=
            MIN(50,
                (get_weapon_racetype_proficiency
                 (ch, wp_pri->obj_flags.value[3], GET_RACE_TYPE(victim)) / 2));
#endif
    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room,  
         "Race weapon type: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

    /* encumberance penalties for ch and victim */
    off = (10 - MAX(get_encumberance(ch) - 4, 0)) * off / 10;
    def = (10 - MAX(get_encumberance(victim) - 4, 0)) * def / 10;

    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room,  
         "encumberance: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

    /* gang-bang mods */
    /*
     * if( !IS_SET( victim->specials.act, CFL_NOGANG )
     * && ( ( n = count_opponents( victim ) ) > 1 ) ) 
     */
    n = count_opponents(victim);
    if (!IS_SET(victim->specials.act, CFL_NOGANG)
        && (n > (victim->specials.alt_fighting ? 2 : 1))) {
        def /= (float) ((n + 1) / 2);
        if( diffport && (prev_off != off || prev_def != def )) {
            qroomlogf(QUIET_COMBAT, ch->in_room,  
             "Gang: off = %d, def = %d", off, def);
        }
        prev_off = off;
        prev_def = def;
    }

    /* Add penalty to offense if ch is drunk */
    switch (is_char_drunk(ch)) {
    case 0:
        break;
    case DRUNK_LIGHT:
        off = (off * .9);
        break;
    case DRUNK_MEDIUM:
        off = (off * .7);
        break;
    case DRUNK_HEAVY:
        off = (off * .5);
        break;
    case DRUNK_SMASHED:
        off = (off * .3);
        break;
    case DRUNK_DEAD:
        off = (off * .05);
        break;
    }

    /* Add penalty to defense if victim is drunk */
    switch (is_char_drunk(victim)) {
    case 0:
        break;
    case DRUNK_LIGHT:
        def = (def * .9);
        break;
    case DRUNK_MEDIUM:
        def = (def * .7);
        break;
    case DRUNK_HEAVY:
        def = (def * .5);
        break;
    case DRUNK_SMASHED:
        def = (def * .3);
        break;
    case DRUNK_DEAD:
        def = (def * .05);
        break;
    }
    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room,  
         "Drunk: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

    /* barehanded vs. weapon */
    /* Shield counts as bare-handed.  What we're looking at here is if the
     * attacker would be cautious against someone with a sword, or go
     * balls out against someone barehanded or with just a shield 
     * -Nessalin */
    if (((wp_vict || (!wp_vict && v_wtype != TYPE_HIT))
         && (!wp_pri && !wp_sec && c_wtype == TYPE_HIT)))
        off /= 2;

    if (v_wtype == TYPE_HIT && !wp_vict && (!shield_vict)
        && (wp_pri || wp_sec || (!wp_pri && !wp_sec && c_wtype != TYPE_HIT))) {
        def /= 2;
    }

    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room, 
         "Unarmed Mods: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

    /* check to see if attacker can see victim and vice versa */
    if (!CAN_SEE(ch, victim)) {
        int blind_fight = get_skill_percentage(ch, SKILL_BLIND_FIGHT);

        /* blind fighting can offset the penalty */
        if (number(1, 100) > blind_fight) {
            gain_skill(ch, SKILL_BLIND_FIGHT, 1);
            /* 50% off offense if you fail */
            off /= 2;
        } else {
            /* reduce the 50% penalty based on skill */
            off = (off * (50 * (100 - blind_fight) / 100)) / 100;
        }
    }

    if (!CAN_SEE(victim, ch)) {
        int blind_fight = get_skill_percentage(victim, SKILL_BLIND_FIGHT);

        /* blind fighting can offset the penalty */
        if (number(1, 100) > blind_fight) {
            gain_skill(victim, SKILL_BLIND_FIGHT, 1);
            /* 50% penalty if you fail */
            def /= 2;
        } else {
            /* reduce the 50% penalty based on skill */
            def = (def * (50 * (100 - blind_fight) / 100)) / 100;
        }
    }
    if( diffport && (prev_off != off || prev_def != def )) {
        qroomlogf(QUIET_COMBAT, ch->in_room,  
         "Can't See mods: off = %d, def = %d", off, def);
    }
    prev_off = off;
    prev_def = def;

    ret->offense = off;
    ret->defense = def;
    ret->wp_pri = wp_pri;
    ret->wp_sec = wp_sec;
    ret->wp_vict = wp_vict;
    ret->c_wtype = c_wtype;
    ret->v_wtype = v_wtype;
    ret->shield_vict = shield_vict;
}
static int RoundCounter = 0;
/* COMBAT START */
int
hit(CHAR_DATA * ch, CHAR_DATA * victim, int type)
{
    char bug[MAX_STRING_LENGTH];
    char vict_name[MAX_STRING_LENGTH];
    OBJ_DATA *wp_pri, *wp_sec, *wp_vict;
    OBJ_DATA *shield_vict = NULL;
    int off, def, c_wtype, v_wtype, dam, percent;
    int prev_off, prev_def, prev_dam;
    int n = 0;
    int prev_health;
    int prev_stun;
    int lost_health;
    int lost_stun;
    int merciful = 0;
    bool alive = TRUE;
    bool is_hit;
    OFF_DEF_DATA od_data;
RoundCounter++;
gamelogf("Function hit: Round %d",RoundCounter);
    if (ch->in_room != victim->in_room) {
        stop_fighting(ch, victim);
        return (TRUE);
    }

    if (ch == victim) {
        stop_fighting(ch, victim);
        return (TRUE);
    }

    if (!CAN_FIGHT(ch) && ch->specials.fighting && !CAN_FIGHT(victim)
        && victim->specials.fighting) {
        stop_all_fighting(ch);
        stop_all_fighting(victim);
        return (TRUE);
    }

    /* If the attacker is FROZEN, don't do any combat at all -Tiernan 12/5/01 */
    if (IS_SET(ch->specials.act, CFL_FROZEN))
        return (TRUE);

    if (ch_starts_falling_check(ch)) {
      return TRUE;
    }

    if (ch_starts_falling_check(victim)) {
      return TRUE;
    }

    if (ch && victim && ch->in_room)
        set_combat_tracks_in_room(ch, victim, ch->in_room);

    strcpy( vict_name, GET_NAME(victim));
    off = 0;
    def = 0;
    /// 'tally' the offense and defence for ch & victim
    calc_offense_defense(ch, victim, type, &od_data );
    wp_pri = od_data.wp_pri;
    wp_sec = od_data.wp_sec;
    wp_vict = od_data.wp_vict;
    shield_vict = od_data.shield_vict;
    c_wtype = od_data.c_wtype;
    v_wtype = od_data.v_wtype;
    off = od_data.offense;
    def = od_data.defense;

    prev_off = off;
    prev_def = def;
    /* Give random bases for offense, defense */
    off += number(1, 100);
    def += number(1, 100);

/*    if( prev_off != off || prev_def != def ) {
        qroomlogf( QUIET_COMBAT, ch->in_room, 
         "%s vs %s - primary attack - offense %d vs. defense %d\n\r", 
         GET_NAME(ch), GET_NAME(victim), off, def );
    } */
    prev_off = off;
    prev_def = def;

    /* Right before the actual damage assesment phase */
    //roomlogf(ch->in_room, "%s vs %s: off %d, def %d", MSTR(ch, name), MSTR(victim, name), off, def);

    /* figure out the damage */
    is_hit = (off > def);
    if (!is_hit) {
        alive = ws_damage(ch, victim, 0, 0, 0, 0, c_wtype, 0);
        percent = number(1, 500);
        if (GET_GUILD(ch)) {
            if ((percent <= guild[(int) GET_GUILD(ch)].off_kpc)
                && !number(0, OFFENSE_LAG))
                gain_proficiency(ch, c_wtype);
        }
        if (wp_pri && victim)
            gain_weapon_racetype_proficiency(ch, wp_pri->obj_flags.value[3], GET_RACE_TYPE(victim));
    } else {

        /* add mercy code to prevent weapon damage check */
        merciful = IS_SET(ch->specials.mercy, MERCY_KILL);
        /* Enraged muls do not show mercy */
        if (GET_RACE(ch) == RACE_MUL && affected_by_spell(ch, AFF_MUL_RAGE))
            merciful = 0;

        int shield_loc = get_shield_loc(victim);
        /* Try to block the attack with a shield if they are awake and not paralyzed*/
        if (AWAKE(victim) && !is_paralyzed(victim) && shield_vict &&
            skill_success(victim, NULL,
                          get_skill_percentage(victim, SKILL_SHIELD) +
                          (shield_loc == ETWO ? 20 : (shield_loc == EP ? 10 : 0)))) {
            alive &= ws_damage(ch, victim, 0, 0, 0, 0, SKILL_SHIELD, 0);
    
            if (wp_pri && !number(0, 4))
                shield_vict = damage_armor(shield_vict, GET_STR(ch),
                             get_weapon_type(wp_pri->obj_flags.value[3]));
    
            ch->specials.combat_wait += agl_app[GET_AGL(ch)].c_delay + number(-2, 2);
    
            if (IS_AFFECTED(ch, CHAR_AFF_SLOW))
                ch->specials.combat_wait *= 2;
            ch->specials.combat_wait = MAX(ch->specials.combat_wait, 1);
        /* Try to parry the attack if they are awake and not paralyzed */
        } else if (AWAKE(victim) && !is_paralyzed(victim) && 
                   wp_vict && parry_skill_success(victim, off, def)) {
            alive &= ws_damage(ch, victim, 0, 0, 0, 0, SKILL_PARRY, 0);

            /* Added mercy check - don't weapon break if hit is withheld */
            if (wp_pri && !((GET_POS(victim) <= POSITION_SLEEPING) && merciful))
            /* changed wielded to wp_pri.  AZ */
                wp_pri = damage_weapon(wp_pri, wp_vict, GET_STR(ch));

            ch->specials.combat_wait += agl_app[GET_AGL(ch)].c_delay + number(-2, 2);

            if (IS_AFFECTED(ch, CHAR_AFF_SLOW))
                ch->specials.combat_wait *= 2;
            ch->specials.combat_wait = MAX(ch->specials.combat_wait, 1);
        } else {
            // Failed to shield?
			gamelogf("shield_vict = %d, has_skill = %d", shield_vict, has_skill(victim, SKILL_SHIELD));
            if (shield_vict && has_skill(victim, SKILL_SHIELD))
                gain_skill(victim, SKILL_SHIELD, 1);
            // Failed to parry?
            else if (wp_vict && has_skill(victim, SKILL_PARRY))
                gain_skill(victim, SKILL_PARRY, 1);

            if (!wp_pri)        /* changed wielded to wp_pri.  AZ */
                dam =
                    dice(race[(int) GET_RACE(ch)].attack->damdice,
                         race[(int) GET_RACE(ch)].attack->damsize);
            else if (wp_pri->obj_flags.value[1] && wp_pri->obj_flags.value[2])

                dam = dice(wp_pri->obj_flags.value[1], wp_pri->obj_flags.value[2])
                    + wp_pri->obj_flags.value[0];
            else
                dam = 0;

            prev_dam = dam;

            if( diffport )
                qroomlogf(QUIET_COMBAT, ch->in_room, "Rolled damage: %d", dam);
            /* Add up all the damage bonuses */

            dam += str_app[GET_STR(ch)].todam;

            if( diffport && prev_dam != dam)
                qroomlogf(QUIET_COMBAT, ch->in_room, "Plus strength: %d", dam);
            prev_dam = dam;

            // plus half str bonus if using etwo
            wp_pri = get_weapon(ch, &n);
            if( n == ETWO )
                dam += str_app[GET_STR(ch)].todam / 2;

            if( diffport && prev_dam != dam)
                qroomlogf(QUIET_COMBAT, ch->in_room, "Plus Etwo: %d", dam);
            prev_dam = dam;

            // two handed skill can add skill% worth of str bonus
            if( n == ETWO && has_skill(ch, SKILL_TWO_HANDED))
               dam += str_app[GET_STR(ch)].todam * get_skill_percentage(ch, SKILL_TWO_HANDED) / 100;

            if( diffport && prev_dam != dam)
                qroomlogf(QUIET_COMBAT, ch->in_room, "Plus Two Handed: %d", dam);
            prev_dam = dam;

            dam += race[(int) GET_RACE(ch)].attack->damplus;
            if( diffport && prev_dam != dam)
                qroomlogf(QUIET_COMBAT, ch->in_room, "Plus race: %d", dam);
            prev_dam = dam;

            if (GET_POS(victim) < POSITION_FIGHTING && GET_POS(victim) > POSITION_STUNNED) {      /*  Kel  */ 
                dam *= 1 + (POSITION_FIGHTING - GET_POS(victim)) / 3.0;
                if( diffport && prev_dam != dam)
                    qroomlogf(QUIET_COMBAT, ch->in_room, "Posistion: %d", dam);
                prev_dam = dam;
            }

            dam += (((off - agl_app[GET_AGL(victim)].reaction) - def) / 50);
            if( diffport && prev_dam != dam) {
                qroomlogf(QUIET_COMBAT, ch->in_room, 
                 "off v def bonus: %d", dam);
            }
            prev_dam = dam;
            dam = MAX(1, dam);

            if( diffport && prev_dam != dam) {
                qroomlogf(QUIET_COMBAT, ch->in_room, 
                 "At least 1: %d", dam);
            }
            prev_dam = dam;

            /* Add bonus for a raging mul */
            if (GET_RACE(ch) == RACE_MUL && affected_by_spell(ch, AFF_MUL_RAGE)) {
                dam += 5;
                if( diffport && prev_dam != dam) {
                    qroomlogf( QUIET_COMBAT, ch->in_room, "mul rage: %d", dam );
                }
                prev_dam = dam;
            }

            /* Do the damage... */

            prev_health = GET_HIT(victim);
            prev_stun = GET_STUN(victim);

            alive &= ws_damage(ch, victim, dam, 0, 0, dam, c_wtype, 0);

            if (alive) {
                lost_health = prev_health - GET_HIT(victim);
                lost_stun = prev_stun - GET_STUN(victim);
            }

            // Added by Nessalin 8/18/2004
            // wp_pri might be bogus after ws_damage removes the item for some spells
            n = -1;
            wp_pri = get_weapon(ch, &n);

            if (alive)
                alive &=
                    run_damage_specials(ch, wp_pri, victim, shield_vict, dam, c_wtype, lost_health);

            /* Added mercy check - don't weapon break if hit is withheld */
            if (alive && wp_pri && !((GET_POS(victim) <= POSITION_SLEEPING)
             && merciful)) {
            /* changed wielded to wp_pri */
                wp_pri = damage_weapon(wp_pri, 0, dam);
                if( wp_pri && c_wtype != TYPE_BLUDGEON 
                 && prev_health - GET_HIT(victim) > 5 ) {
                    sflag_to_obj(wp_pri, OST_BLOODY);
                }
            }

            /* Inflict poison... */
            /* if (alive) was this -Morg */
            /* only if a hit was scored -Morg */
            /* need the alive check to make sure you don't poison a corpse
             * -Morg 10/16/02
             */
            if (alive && lost_health > 0) {
                if (wp_pri) {
                    if (GET_RACE(victim) != RACE_VAMPIRE)
                        if (IS_SET(wp_pri->obj_flags.extra_flags, OFL_POISON)
                            && !affected_by_spell(victim, SPELL_POISON))
                            if (!check_end(victim, 0)) {
                                sprintf(bug,
                                        "Incorrect poison type: combat.c, " "Room: %d, Char: %s.",
                                        ch->in_room->number, MSTR(ch, short_descr));
                                if ((wp_pri->obj_flags.value[5] < 0)
                                    || (wp_pri->obj_flags.value[5] > MAX_POISON))
                                    gamelog(bug);
                                poison_char(victim, wp_pri->obj_flags.value[5], number(1, 4), 0);
                                REMOVE_BIT(wp_pri->obj_flags.extra_flags, OFL_POISON);
                                wp_pri->obj_flags.value[5] = 0;
                                send_to_char("You feel very sick...\n\r", victim);
                            }
                } /* end if wp_pri */
                else
                    /* npc can poison naturally */
                {
                    if (((number(1, 3)) > 2) && IS_SET(ch->specials.act, CFL_CAN_POISON)) {
                        poison_char(victim, racial_poison(GET_RACE(ch)), number(1, 4), 0);
                        send_to_char("You feel very sick...\n\r", victim);
                    }
                }               /* end if !wp_pri */
            }                   /* end if alive */
        }                       /* end if damage */
    }                           /* end if hit */


    /* Warrior mods - Tiernan 8/6/2003 
     * This checks for a secondary opponent.  If there is, this means the
     * warrior is using split-opponents skill, striking at their main
     * opponent with EP and their secondary opponent with ES. This requires
     * the warrior use bare hands or two weapons, otherwise there is no
     * second strike.
     */
    if (!alive && ch->specials.alt_fighting) {
        victim = ch->specials.fighting = ch->specials.alt_fighting;
        ch->specials.alt_fighting = 0;
        alive = TRUE;
    } else if (ch->specials.alt_fighting) {
        // TODO - need to recalculate v_wtype, shield_vict, and weap_vict
        victim = ch->specials.alt_fighting;
    }

    /* Try for second attack */
    /* (allow two punches, assuming both hands empty and using punches/hit) */
    if (alive
     && (wp_sec || (!ch->equipment[EP] && !ch->equipment[ES]
                    && !ch->equipment[ETWO]
                    && race[(int) GET_RACE(ch)].attack->type == TYPE_HIT))) {
        if (wp_sec)
            c_wtype = get_weapon_type(wp_sec->obj_flags.value[3]);
        else if (GET_RACE(ch) > 0)
            c_wtype = race[(int) GET_RACE(ch)].attack->type;
        else
            c_wtype = TYPE_HIT;

        off = proficiency_bonus(ch, c_wtype);
        def = GET_DEFENSE(victim);
        prev_off = off;
        prev_def = def;
        if( diffport ) {
            qroomlogf(QUIET_COMBAT, ch->in_room,  
             "2nd base: off = %d, def = %d", off, def);
        }

        off += number(1, 100);
        def += number(1, 100);
        if( diffport && (prev_off != off || prev_def != def) ) {
            qroomlogf(QUIET_COMBAT, ch->in_room,  
             "random: off = %d, def = %d", off, def);
        }
        prev_off = off;
        prev_def = def;

        if (has_skill(ch, SKILL_DUAL_WIELD)) {
            off += get_skill_percentage(ch, SKILL_DUAL_WIELD);
            if( diffport  && (prev_off != off || prev_def != def) ) {
                qroomlogf(QUIET_COMBAT, ch->in_room,  
                 "dual wield: off = %d, def = %d", off, def);
            }
            prev_off = off;
            prev_def = def;
        }

        def -= ((POSITION_FIGHTING - GET_POS(victim)) * 10);
        off -= ((POSITION_FIGHTING - GET_POS(ch)) * 10);
        if( diffport  && (prev_off != off || prev_def != def) ) {
            qroomlogf(QUIET_COMBAT, ch->in_room,  
             "posistion: off = %d, def = %d", off, def);
        }
        prev_off = off;
        prev_def = def;

        if (!AWAKE(victim)
            || IS_SET(victim->specials.act, CFL_FROZEN)
            /* Loose defense/parry if you're subdued -Morg */
            || IS_SET(victim->specials.affected_by, CHAR_AFF_SUBDUED)) {
            def = 0;
            qroomlogf(QUIET_COMBAT, ch->in_room,  "vict not conscious: off = %d, def = %d", off, def);
            if (diffport  && (prev_off != off || prev_def != def)) {
                qroomlogf(QUIET_COMBAT, ch->in_room,  
                 "posistion: off = %d, def = %d", off, def);
            }
            prev_off = off;
            prev_def = def;
        }

        is_hit = (off > def);
/*        qroomlogf(QUIET_COMBAT, ch->in_room, 
                  "%s vs %s - secondary attack - offense %d vs. defense %d", 
                  GET_NAME(ch), GET_NAME(victim), off, def );
*/
        if (!is_hit) {
            percent = number(1, 500);
            if ((percent < DUAL_WIELD_KPC) && (has_skill(ch, SKILL_DUAL_WIELD)))
                gain_skill(ch, SKILL_DUAL_WIELD, 1);
            alive = ws_damage(ch, victim, 0, 0, 0, 0, c_wtype, 0);
        } else {
            int shield_loc = get_shield_loc(victim);
            /* Try to block the attack with a shield */
            if (shield_vict && skill_success(victim, NULL,
                                             get_skill_percentage(victim, SKILL_SHIELD) +
                                             (shield_loc == ETWO ? 20 : (shield_loc == EP ? 10 : 0)))) {
                alive &= ws_damage(ch, victim, 0, 0, 0, 0, SKILL_SHIELD, 0);

                if (wp_sec && !number(0, 4))
                    shield_vict = damage_armor(shield_vict, GET_STR(ch),
                                 get_weapon_type(wp_sec->obj_flags.value[3]));
    
                ch->specials.combat_wait += agl_app[GET_AGL(ch)].c_delay + number(-2, 2);
    
                if (IS_AFFECTED(ch, CHAR_AFF_SLOW))
                    ch->specials.combat_wait *= 2;
                ch->specials.combat_wait = MAX(ch->specials.combat_wait, 1);
            /* Try to parry the attack */
            } else if (wp_vict && parry_skill_success(victim, off, def)) {
                alive &= ws_damage(ch, victim, 0, 0, 0, 0, SKILL_PARRY, 0);
 
            /* Added mercy check - don't weapon break if hit is withheld */
            if (wp_sec && wp_vict && !((GET_POS(victim) <= POSITION_SLEEPING)
             && merciful))
                    wp_sec = damage_weapon(wp_sec, wp_vict, GET_STR(ch));

                ch->specials.combat_wait += agl_app[GET_AGL(ch)].c_delay + number(-5, 5);
                if (IS_AFFECTED(ch, CHAR_AFF_SLOW))
                    ch->specials.combat_wait *= 2;
            } else {
	        // Failed to parry?
                if (wp_vict && (has_skill(victim, SKILL_PARRY)) && (number(1, 6) < 6))
                    gain_skill(victim, SKILL_PARRY, 1);

                if (wp_sec && wp_sec->obj_flags.value[1] &&     /* wieldedto wp_sec */
                    wp_sec->obj_flags.value[2]) /* AZ */
                    dam =
                        dice(wp_sec->obj_flags.value[1],
                             wp_sec->obj_flags.value[2]) + wp_sec->obj_flags.value[0];
                else if (!wp_sec)
                    dam =
                        dice(race[(int) GET_RACE(ch)].attack->damdice,
                             race[(int) GET_RACE(ch)].attack->damsize);
                else
                    dam = 0;

                prev_dam = dam;
                if( diffport )
                    qroomlogf( QUIET_COMBAT, ch->in_room, 
                     "2nd attack Rolled damage: %d", dam );

                /* Add up all the damage bonuses */

                dam += str_app[GET_STR(ch)].todam;
                if( diffport && prev_dam != dam ) {
                    qroomlogf( QUIET_COMBAT, ch->in_room, 
                     "Plus strength: %d", dam );
                }
                prev_dam = dam;

                dam += race[(int) GET_RACE(ch)].attack->damplus;
                if( diffport && prev_dam != dam ) {
                    qroomlogf( QUIET_COMBAT, ch->in_room, 
                     "Plus race: %d", dam );
                }
                prev_dam = dam;

                if ((GET_POS(victim) < POSITION_FIGHTING) && GET_POS(victim) > POSITION_STUNNED) {        /*  Kel  */
                    dam *= 1 + (POSITION_FIGHTING - GET_POS(victim)) / 3.0;
                    if( diffport && prev_dam != dam ) {
                        qroomlogf( QUIET_COMBAT, ch->in_room, 
                         "Posistion: %d", dam );
                    }
                    prev_dam = dam;
                }

                dam += ((off - def) / 40);
                if( diffport && prev_dam != dam ) {
                    qroomlogf( QUIET_COMBAT, ch->in_room, 
                     "off vs def: %d", dam );
                }
                prev_dam = dam;

                dam = MAX(1, dam);
                if( diffport && prev_dam != dam ) {
                    qroomlogf( QUIET_COMBAT, ch->in_room, 
                     "min limit: %d", dam );
                }
                prev_dam = dam;

                /* Do the damage... */
                prev_health = GET_HIT(victim);
                prev_stun = GET_STUN(victim);

                alive &= ws_damage(ch, victim, dam, 0, 0, dam, c_wtype, 0);

                if (alive) {
                    lost_health = prev_health - GET_HIT(victim);
                    lost_stun = prev_stun - GET_STUN(victim);
                }

                if (alive)      // Only run the special if the victim still lives.
                    alive &= run_damage_specials(ch, wp_sec, victim, shield_vict, dam, c_wtype,
                                                 prev_health - GET_HIT(victim));

                /* Added mercy check - don't weapon break if hit is withheld */
                if (alive && wp_sec && !((GET_POS(victim) <= POSITION_SLEEPING)
                 && merciful)) {
                /* changed wielded to wp_pri */
                    wp_sec = damage_weapon(wp_sec, 0, dam);
                    if( wp_sec && c_wtype != TYPE_BLUDGEON 
                     && prev_health - GET_HIT(victim) > 5 ) {
                        sflag_to_obj(wp_sec, OST_BLOODY);
                    }
                }

                /* Inflict poison... */
                if ((GET_RACE(ch) != RACE_VAMPIRE) && alive && wp_sec
                    && IS_SET(wp_sec->obj_flags.extra_flags, OFL_POISON)
                    && !affected_by_spell(victim, SPELL_POISON) && !check_end(victim, 0)) {
                    sprintf(bug, "Incorrect poison type: combat.c, " "Room: %d, Char: %s.",
                            ch->in_room->number, MSTR(ch, short_descr));
                    if ((wp_sec->obj_flags.value[5] < 0)
                        || (wp_sec->obj_flags.value[5] > MAX_POISON))
                        gamelog(bug);
                    poison_char(victim, wp_sec->obj_flags.value[5], number(1, 27 - GET_END(victim)),
                                0);
                    send_to_char("You feel very sick...\n\r", victim);
                    REMOVE_BIT(wp_sec->obj_flags.extra_flags, OFL_POISON);
                    wp_sec->obj_flags.value[5] = 0;
                }
            }                   /* end if damaged */
        }                       /* end if hit */
    }

    /* Check the status of both opponents to see if either is still alive */
    if (alive) {
        ch->specials.combat_wait += agl_app[GET_AGL(ch)].c_delay + number(-5, 5);

        if (IS_AFFECTED(ch, CHAR_AFF_SLOW))
            ch->specials.combat_wait *= 2;

        /* Are they using a two-handed weapon? */
        if (has_skill(ch, SKILL_TWO_HANDED) && ch->equipment[ETWO]) {
            if (number(1, 100) <= ch->skills[SKILL_TWO_HANDED]->learned)
                ch->specials.combat_wait -= (ch->specials.combat_wait * MIN(60, number(0, ch->skills[SKILL_TWO_HANDED]->learned)) / 100);  
            else {
                if ((number(1, 500) < TWO_HANDED_KPC) && (has_skill(ch, SKILL_TWO_HANDED)))
                    gain_skill(ch, SKILL_TWO_HANDED, 1);
            }
        }
    }

    return (alive);
}



/* The Path stuff follows */

ROOM_DATA *search_head_room;
ROOM_DATA *search_tail_room;
int visited_nodes = 0;

void
add_to_path_list(ROOM_DATA * room, ROOM_DATA * prev, char *command)
{
    /* this adds a path from prev to room with via the command string */

    if (prev->path_list.run_number == room->path_list.run_number)
        return;
    room->path_list.prev_list = prev;
    if (!search_head_room)
        search_head_room = room;
    if (search_tail_room)
        search_tail_room->path_list.next_search = room;
    search_tail_room = room;
    room->path_list.next_search = 0;
    room->path_list.run_number = prev->path_list.run_number;
    room->path_list.distance = prev->path_list.distance + 1;
    strcpy(room->path_list.command, command);
    visited_nodes++;
};



ROOM_DATA *
top_from_path_list(void)
{
    ROOM_DATA *tmp;
    tmp = search_head_room;
    if (tmp)
        search_head_room = search_head_room->path_list.next_search;
    if (search_head_room == 0)
        search_tail_room = 0;
    return (tmp);
}

int
generic_searcher(ROOM_DATA * in_room, int (*done_yet) (CHAR_DATA *, ROOM_DATA *, void *),
                 void *data, int max_dist, int search_up, CHAR_DATA * ch)
{
    /*
     * max_dist of 2 will search up to
     * 
     * +-------+ +-+ +---+
     * |in_room|-| |-|tgt|
     * +-------+ +-+ +---+
     * 
     * Searches all rooms around in_room, until the done function returns 1. 
     * 
     * if ch is not 0 it stores the following, assuming the char can do it.
     * ----open exits return the exit direction 'north', 'east', ect...
     * ----closed exits have 'open keyword' where keyword is the first keyword.
     * ----locked exits have 'unlock keyword' where keyword is the first keyword.
     * ----wagons will have 'enter keyword' where keyword is the first keyword.
     * 
     * if ch is 0, then it returns a simple one argument
     * ----that is 'east' 'north' 'in wagon' ect... for information only.
     * 
     * 
     * The function returns
     * -1 if there is an error (a room is a null pointer or others)
     * -1 if no path is found, or
     * distance and a string in path string if is found.
     * 
     * 
     */

    static int run_num = 10000;
    ROOM_DATA *tmp_room;
    OBJ_DATA *obj;
    char command[100], temp[100];
    int i;

    if (run_num == 10000) {
        for (tmp_room = room_list; tmp_room; tmp_room = tmp_room->next) {
            tmp_room->path_list.run_number = 0;
        }
        shhlog("Reseting run_numbers for all rooms 10,000 paths have been executed");
        run_num = 1;
    } else
        run_num++;

    if (!in_room)
        return -1;

    strcpy(command, "");
    in_room->path_list.run_number = run_num;
    in_room->path_list.distance = 0;
    in_room->path_list.prev_list = 0;
    in_room->path_list.next_search = 0;

    strcpy(in_room->path_list.command, "");
    search_head_room = in_room;
    search_tail_room = in_room;

    while ((in_room = top_from_path_list())) {
        if (in_room->path_list.distance >= max_dist)
            continue;           /* go back and get next item in queue */

        if (done_yet(ch, in_room, data)) {
            return (in_room->path_list.distance);
        }

        for (obj = in_room->contents; obj; obj = obj->next_content)
            if (obj->obj_flags.type == ITEM_WAGON
                && !IS_SET(obj->obj_flags.value[3], WAGON_NO_ENTER))
                if ((tmp_room = get_room_num(obj->obj_flags.value[0]))) {
                    if (ch)
                        strcpy(command, "enter ");
                    else
                        strcpy(command, "in ");
                    if (sscanf(OSTR(obj, name), "%s", temp))
                        strcat(command, temp);
                    strcat(command, ";");
                    add_to_path_list(tmp_room, in_room, command);
                }

        if (in_room->wagon_exit)
            add_to_path_list(in_room->wagon_exit, in_room, "leave;");

        for (i = 0; i < 6; i++) {
            if ((!search_up) && (i == DIR_UP))
                continue;

            /* if they will fall in the room they want to go, they wont
             * even get info to go that way */
            if (in_room->direction[i] && in_room->direction[i]->to_room && ch
                && will_char_fall_in_room(ch, in_room->direction[i]->to_room))
                continue;

            /* players get called to this with 0, so here is a kludge */
            if (!ch && in_room->direction[i] && in_room->direction[i]->to_room
                && (IS_SET(in_room->direction[i]->to_room->room_flags, RFL_FALLING))
                && (IS_SET(in_room->direction[i]->to_room->room_flags, RFL_NO_CLIMB))
                && (in_room->direction[i]->to_room->direction[DIR_DOWN]))
                continue;

            if (in_room->direction[i] && in_room->direction[i]->to_room
                && in_room->direction[i]->to_room->path_list.run_number != run_num) {
                strcpy(command, "");
                if (ch) {
                    if (IS_SET(in_room->direction[i]->exit_info, EX_LOCKED)) {
                        int wait = ch->specials.act_wait;
                        if (has_key(ch, in_room->direction[i]->key)) {
                            if (sscanf(in_room->direction[i]->keyword, "%s", temp)) {
                                strcat(command, "unlock ");
                                strcat(command, temp);
                                strcat(command, ";");
                            } else {
                                sprintf(temp, "Room #%d exit %s has no exit name.", in_room->number,
                                        dir_name[i]);
                                gamelog(temp);
                                return (-1);
                            }
                        } else {
                            ch->specials.act_wait = wait;
                            continue;   /* the for loop of exits in the room */
                        }
                    }
                }

                if (IS_SET(in_room->direction[i]->exit_info, EX_SPL_SAND_WALL)) {
                    /* Is there a way to dispell sandwalls? */
                    continue;   /* continue to next exit */
                }

#ifdef CHECK_FALLING_TWICE
                if (ch && !IS_IMMORTAL(ch)
                    && IS_SET(in_room->direction[i]->to_room->room_flags, RFL_FALLING)
                    && IS_SET(ch->specials.affected_by, CHAR_AFF_FLYING)
                    && !affected_by_spell(ch, SPELL_LEVITATE)
                    && !affected_by_spell(ch, SPELL_FEATHER_FALL)) {
                    /* so they dont hunt into an air room, unless can fly */
                    continue;
                }
#endif

                if (IS_SET(in_room->direction[i]->exit_info, EX_SPL_SAND_WALL)) {
                    /* Is there a way to dispell sandwalls? */
                    continue;   /* continue to next exit */
                }
                if (ch && IS_SET(in_room->direction[i]->exit_info, EX_CLOSED)) {
                    if (in_room->direction[i]->keyword) {
                        if (1 == sscanf(in_room->direction[i]->keyword, "%s", temp)) {
                            strcat(command, "open ");
                            strcat(command, temp);
                            strcat(command, ";");
                        } else {
                            sprintf(temp, "Room #%d exit %s has no exit name.", in_room->number,
                                    dir_name[i]);
                            gamelog(temp);
                            return (-1);
                        }
                    } else {
                        sprintf(temp, "Room #%d exit %s has no exit keywords.", in_room->number,
                                dir_name[i]);
                        gamelog(temp);
                        return (-1);
                    };
                }
                strcat(command, dir_name[i]);
                strcat(command, ";");
                add_to_path_list(in_room->direction[i]->to_room, in_room, command);
            }
        }
        /* search has been exausted */
    }
    return -1;
};

struct find_closest_hates_type
{
    CHAR_DATA *ch;
};

int
find_closest_hates_fun(CHAR_DATA * ch, ROOM_DATA * in_room, void *data)
{
    struct find_closest_hates_type *fc;
    CHAR_DATA *tmp_char;

    fc = data;
    for (tmp_char = in_room->people; tmp_char; tmp_char = tmp_char->next_in_room)
        if (CAN_SEE(ch, tmp_char) && does_hate(ch, tmp_char)) {
            fc->ch = tmp_char;
            return (1);
        };


    return (0);
}

CHAR_DATA *
find_closest_hates(CHAR_DATA * ch, int max_dist)
{
    struct find_closest_hates_type fc;

    if (-1 == generic_searcher(ch->in_room, find_closest_hates_fun, &fc, max_dist, FALSE, ch))
        return (0);
    else
        return (fc.ch);
}

struct find_complete_type
{
    char *string;
    int len;
    ROOM_DATA *tgt_room;
};

int
find_first_fun(CHAR_DATA * ch, ROOM_DATA * in_room, void *data)
{
    struct find_complete_type *fc;
    ROOM_DATA *tmp;
    char *tmp_str;

    fc = data;

    if (in_room != fc->tgt_room)
        return (0);

    strcpy(fc->string, "");
    for (tmp = in_room; tmp->path_list.prev_list && tmp->path_list.prev_list->path_list.prev_list; tmp = tmp->path_list.prev_list);     /* find the room one before original */

    if (tmp) {
        strcpy(fc->string, tmp->path_list.command);
        for (tmp_str = fc->string; *tmp_str != '\0' && *tmp_str != ';'; tmp_str++);     /* do nothing advance */

        *tmp_str = '\0';
    }

    return (1);
}

int
choose_exit_name_for(ROOM_DATA * in_room, ROOM_DATA * tgt_room, char *command, int max_dist,
                     CHAR_DATA * ch)
{
    struct find_complete_type fc;
    /* this returns the first command the person should perform to get from
     * point a to be, command should be larger than 100 chars.
     * It returns 1 if a path was found, 0 otherwize.
     * If a  path was found, then copys the command into command 
     */
    fc.string = command;
    fc.tgt_room = tgt_room;
    return (generic_searcher(in_room, find_first_fun, &fc, max_dist, FALSE, ch));
}


int
find_complete_fun(CHAR_DATA * ch, ROOM_DATA * in_room, void *data)
{
    struct find_complete_type *fc;
    ROOM_DATA *tmp;
    char tmp_str[MAX_STRING_LENGTH];
    int length = 0;

    fc = data;

    if (in_room != fc->tgt_room)
        return (0);

    strcpy(fc->string, "");
    for (tmp = in_room; tmp; tmp = tmp->path_list.prev_list) {
        length = strlen(tmp->path_list.command) + strlen(fc->string);
        if ((length > fc->len - 2) || (length > MAX_STRING_LENGTH - 2))
            strcpy(tmp_str, "....");
        else
            strcpy(tmp_str, fc->string);

        strcpy(fc->string, tmp->path_list.command);
        strcat(fc->string, tmp_str);
    };
    /* should get rid of that last semi-colon here */
    return (1);
}

int
path_from_to_for(ROOM_DATA * in_room, ROOM_DATA * tgt_room, int max_dist, int max_string_len,
                 char *path, CHAR_DATA * ch)
{
    struct find_complete_type fc;
    fc.string = path;
    fc.tgt_room = tgt_room;
    fc.len = max_string_len;
    return (generic_searcher(in_room, find_complete_fun, &fc, max_dist, FALSE, ch));
}

int
find_d_fun(CHAR_DATA * ch, ROOM_DATA * in_room, void *data)
{
    return (in_room == (ROOM_DATA *) data);
}

int
find_dist_for(ROOM_DATA * from, ROOM_DATA * to, CHAR_DATA * ch)
{
    return (generic_searcher(from, find_d_fun, to, 1000, TRUE, ch));
}

int
get_shield_loc(CHAR_DATA *ch) {
    if (is_shield(ch->equipment[EP]))
        return EP;
    else if (is_shield(ch->equipment[ES]))
        return ES;
    else if (is_shield(ch->equipment[ETWO]))
        return ETWO;
    return -1;
}


int
get_random_hit_loc(CHAR_DATA * ch, int type)
{
    int loc, num;

    if( type == SKILL_SHIELD) {
        if (is_shield(ch->equipment[EP]))
            return EP;
        else if (is_shield(ch->equipment[ES]))
            return ES;
        else if (is_shield(ch->equipment[ETWO]))
            return ETWO;
    }

    if (!IS_ATTACK(type) && type != SKILL_KICK && type != SKILL_THROW && type != SKILL_SAP
        && type != TYPE_BOW)
        return WEAR_BODY;

    if (!AWAKE(ch) || IS_SET(ch->specials.act, CFL_FROZEN)) {
        switch (type) {
        case TYPE_HIT:
        case TYPE_BLUDGEON:
        case SKILL_SAP:
        case SKILL_KICK:
            return WEAR_HEAD;
        case TYPE_PIERCE:
        case TYPE_STAB:
            return WEAR_BACK;
        case TYPE_CHOP:
        case TYPE_SLASH:
        case TYPE_BOW:
        case SKILL_THROW:
            return WEAR_NECK;
        case TYPE_STING:
            return WEAR_BODY;
        default:
            loc = number(0, 2);
            if (loc)
                return WEAR_BODY;
            else
                return WEAR_HEAD;
            break;
        }
    }

    switch (GET_RACE_TYPE(ch)) {
    case RTYPE_OTHER:
        return WEAR_BODY;
    case RTYPE_HUMANOID:
    case RTYPE_INSECTINE:
    case RTYPE_REPTILIAN:
        loc = number(1, 100);
        if (loc >= 95)
            return WEAR_HEAD;
        else if (loc >= 90)
            return WEAR_NECK;
        else if (loc >= 80)
            return WEAR_ARMS;
        else if (loc >= 77)
            return WEAR_WRIST_R;
        else if (loc >= 74)
            return WEAR_WRIST_L;
        else if (loc >= 70)
            return WEAR_HANDS;
        else if (loc >= 30)
            return WEAR_BODY;
        else if (loc >= 14)
            return WEAR_LEGS;
        else if (loc >= 10)
            return WEAR_FEET;
        else if (loc >= 8)
            return WEAR_WAIST;
        else
            return WEAR_BODY;
        break;
    case RTYPE_OPHIDIAN:
        num = number(1, 100);
        if (num >= 90)
            return WEAR_HEAD;
        else if (num >= 30)
            return WEAR_BODY;
        else
            return WEAR_WAIST;
        break;
    case RTYPE_ARACHNID:
        loc = number(1, 100);
        if (loc >= 95)
            return WEAR_HEAD;
        else if (loc >= 80)
            return WEAR_ARMS;
        else if (loc >= 77)
            return WEAR_WRIST_R;
        else if (loc >= 74)
            return WEAR_WRIST_L;
        else if (loc >= 70)
            return WEAR_HANDS;
        else if (loc >= 30)
            return WEAR_BODY;
        else if (loc >= 14)
            return WEAR_LEGS;
        else if (loc >= 10)
            return WEAR_FEET;
        else if (loc >= 8)
            return WEAR_WAIST;
        else
            return WEAR_BODY;
        break;
    case RTYPE_AVIAN_FLYING:
    case RTYPE_AVIAN_FLIGHTLESS:
        num = number(1, 100);
        if (num >= 92)
            return WEAR_HEAD;
        else if (num >= 88)
            return WEAR_NECK;
        else if (num >= 50)
            return WEAR_BODY;
        else if (num >= 30)
            return WEAR_ARMS;
        else if (num >= 20)
            return WEAR_WAIST;
        else if (num >= 5)
            return WEAR_LEGS;
        else
            return WEAR_FEET;
        break;
    case RTYPE_MAMMALIAN:
        num = number(1, 100);
        if (num >= 94)
            return WEAR_HEAD;
        else if (num >= 90)
            return WEAR_NECK;
        else if (num >= 55)
            return WEAR_BODY;
        else if (num >= 35)
            return WEAR_ARMS;
        else if (num >= 20)
            return WEAR_LEGS;
        else if (num >= 10)
            return WEAR_WAIST;
        else if (num >= 5)
            return WEAR_HANDS;
        else
            return WEAR_FEET;
        break;
    case RTYPE_PLANT:
        num = number(1, 100);
        if (num >= 50)
            return WEAR_BODY;
        else if (num >= 10)
            return WEAR_ARMS;
        else
            return WEAR_FEET;
        break;
    default:
        return WEAR_BODY;
    }
}


void
break_weapon(OBJ_DATA * weapon)
{
    char buf[256];
    char buf2[256];

    if (weapon->equipped_by) {
        sprintf(buf, "$n's %s shatters!", first_name(OSTR(weapon, name), buf2));
        act(buf, FALSE, weapon->equipped_by, 0, 0, TO_ROOM);
        sprintf(buf, "Your %s shatters!", first_name(OSTR(weapon, name), buf2));
        act(buf, FALSE, weapon->equipped_by, 0, 0, TO_CHAR);
    } else if (weapon->carried_by) {
        sprintf(buf, "Your %s shatters!", first_name(OSTR(weapon, name), buf2));
        act(buf, FALSE, weapon->carried_by, 0, 0, TO_CHAR);
    } else if (weapon->in_room) {
        sprintf(buf, "%s shatters!\n\r", OSTR(weapon, short_descr));
        send_to_room(buf, weapon->in_room);
    }
    extract_obj(weapon);
}

OBJ_DATA *
damage_weapon(OBJ_DATA * att_weapon, OBJ_DATA * def_weapon, int damage)
{
    static int weapon_mat[] = { 9,      /* None     */
        5,                      /* Wood     */
        0,                      /* Metal    */
        4,                      /* Obsidian */
        1,                      /* Stone    */
        3,                      /* Chitin   */
        2,                      /* Gem      */
        6,                      /* Glass    */
        7,                      /* Skin     */
        8,                      /* Cloth    */
        5,                      /* Leather  */
        3,                      /* Bone     */
        4,                      /* Ceramic  */
        3,                      /* Horn     */
        9,                      /* Graft    */
        8
    };                          /* Tissue   */
    int chance = 0, percent, strwgt = 0;

    percent = number(0, 999);
    if (!att_weapon)
        return att_weapon;

    if (!def_weapon) {
        /* add modifier for heavy weapons to account for thickness */
        if (att_weapon->obj_flags.weight > 10)
            strwgt = (att_weapon->obj_flags.weight - 10)/3;
        chance += (damage - (16 - weapon_mat[(int) att_weapon->obj_flags.material]) - strwgt);
        if ((percent <= chance) && (att_weapon->obj_flags.material != MATERIAL_METAL)) { /*  Kel  */
            break_weapon(att_weapon);
	    return NULL;
	   }
    } else {
         /* add modifier for heavy weapons to account for thickness */
         if (def_weapon->obj_flags.weight > 10)
            strwgt = (def_weapon->obj_flags.weight - 10)/3;
         /* the attacking weapon is sturdier or damage is strong */
         if (percent <= str_app[damage].todam
          + (weapon_mat[(int) def_weapon->obj_flags.material] -
          weapon_mat[(int) att_weapon->obj_flags.material]) - strwgt)
              break_weapon(def_weapon);
    }
    return att_weapon;
}


OBJ_DATA *
damage_armor(OBJ_DATA * armor, int dam, int wtype)
{
  int tp, mat;
  int dam_by_wtype[17][30] = {
    
    // Columns are material types. Grep MAT out of *.h to see list
    // 101 is impossible to damage by that weapon type
    // 0 means it is shreded by any form of attack

    // 0   1    2    3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29    

    {101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 0,   0,   101, 101, 101, 101, 101, 101, 101, 0,   0,   0,   0,   0,   0,   0}, /* hit */
    {10,  12,  14,  11,  18,  18,  16,  8,   30,  101, 30,  23,  13,  28,  0,   0,   101, 8,   13,  23,  18,  0,   45,  0,   0,   0,   0,   0,   0,   0}, /* bludgeon */
    {10,  16,  25,  22,  30,  16,  18,  10,  16,  14,  16,  21,  15,  26,  0,   0,   14,  6,   11,  21,  16,  0,   31,  0,   0,   0,   0,   0,   0,   0}, /* pierce */
    {10,  18,  30,  25,  40,  18,  20,  12,  16,  14,  16,  23,  17,  28,  0,   0,   14,  8,   13,  23 , 18,  0,   31,  0,   0,   0,   0,   0,   0,   0}, /* stab */
    {10,  8,   15,  11,  16,  14,  16,  8,   22,  25,  22,  19,  13,  24,  0,   0,   25,  4,   9,   19,  14,  0,   37,  0,   0,   0,   0,   0,   0,   0}, /* chop */
    {10,  16,  25,  18,  20,  16,  18,  10,  14,  12,  14,  21,  15,  26,  0,   0,   12,  6,   11,  21,  16,  0,   29,  0,   0,   0,   0,   0,   0,   0}, /* slash */
    {25,  20,  40,  30,  50,  20,  50,  25,  20,  18,  20,  25,  30,  30,  0,   0,   18,  10,  15,  25,  20,  0,   35,  0,   0,   0,   0,   0,   0,   0}, /* claw */
    {101, 101, 101, 101, 101, 101, 101, 101, 40,  20,  40,  101, 101, 101, 0,   0,   101, 101, 101, 101, 101, 0,   101, 0,   0,   0,   0,   0,   0,   0}, /* whip */
    {10,  8,   15,  11,  16,  14,  16,  8,   22,  25,  22,  19,  13,  24,  0,   0,   25,  4,   9,   19,  14,  0,   37,  0,   0,   0,   0,   0,   0,   0}, /* pinch, new */
    {10,  18,  30,  25,  40,  18,  20,  12,  16,  14,  16,  23,  17,  28,  0,   0,   14,  8,   13,  23,  18,  0,   31,  0,   0,   0,   0,   0,   0,   0}, /* sting, new */
    {10,  16,  25,  22,  30,  16,  18,  10,  16,  14,  16,  19,  15,  26,  0,   0,   14,  6,   11,  21,  16,  0,   31,  0,   0,   0,   0,   0,   0,   0}, /* bite, new */
    {10,  16,  25,  22,  30,  16,  18,  10,  16,  14,  16,  21,  15,  26,  0,   0,   14,  6,   11,  21,  16,  0,   31,  0,   0,   0,   0,   0,   0,   0}, /* peck, new */
    {10,  18,  30,  25,  40,  18,  20,  12,  16,  14,  16,  23,  17,  28,  0,   0,   14,  8,   13,  23,  18,  0,   31,  0,   0,   0,   0,   0,   0,   0}, /* pike */
    {10,  18,  30,  25,  40,  18,  20,  12,  16,  14,  16,  23,  17,  28,  0,   0,   14,  8,   13,  23,  18,  0,   31,  0,   0,   0,   0,   0,   0,   0}, /* trident */
    {10,  18,  30,  25,  40,  18,  20,  12,  16,  14,  16,  23,  17,  28,  0,   0,   14,  8,   13,  23,  18,  0,   31,  0,   0,   0,   0,   0,   0,   0}, /* polearm */
    {10,  18,  30,  25,  40,  18,  20,  12,  16,  14,  16,  23,  17,  28,  0,   0,   14,  8,   13,  23,  18,  0,   31,  0,   0,   0,   0,   0,   0,   0}, /* knife */
    {10,  18,  30,  25,  40,  18,  20,  12,  16,  14,  16,  23,  17,  28,  0,   0,   14,  8,   13,  23,  18,  0,   31,  0,   0,   0,   0,   0,   0,   0}  /* razor */
  };
  
  if (wtype == SKILL_KICK) {
    wtype = TYPE_BLUDGEON;
  }
  
  if ((wtype < 300) || (wtype > 311)) {
    return armor;
  }
  
  if (armor->obj_flags.type != ITEM_ARMOR) {
    return armor;
  }
  
  tp = wtype - 300;
  mat = armor->obj_flags.material;
  
  if (dam >= dam_by_wtype[tp][mat]) {
    // figure out what 1/2 damage is for this item
    int half_dam = armor->obj_flags.value[1] / 2;
    
    armor->obj_flags.value[0] -= 1;
    armor->obj_flags.value[0] = MAX(armor->obj_flags.value[0], 0);
    
    // permanent armor damage
    // if arm_damage drops below half points
    if (armor->obj_flags.value[0] < half_dam 
        // closer to 0, the more likely permanent damage is done
        && !number(0, armor->obj_flags.value[0] / 2)) {
      // drop 'points' by 5, reflecting permanent damage
      armor->obj_flags.value[1] = MAX(armor->obj_flags.value[1] - 5, 0);
      
      // arm_damage can't be more than points
      armor->obj_flags.value[0] = MIN(armor->obj_flags.value[0], armor->obj_flags.value[1]);
    }
  }
  
  if (armor->obj_flags.value[0] <= 0) {
    break_weapon(armor);
    return NULL;
  }

  return armor;
}

int
absorbtion(int armor)
{
    int base, mod;

    base = armor / 10;
    mod = armor % 10;

    if (!mod)
        return base;

    if (number(1, 10) > mod)
        return base;
    else
        return base + 1;
}

int
get_weapon_type(int n)
{
    if (n == 1)
        return TYPE_BLUDGEON;
    else if (n == 2)
        return TYPE_PIERCE;
    else if (n == 3)
        return TYPE_STAB;
    else if (n == 4)
        return TYPE_CHOP;
    else if (n == 5)
        return TYPE_SLASH;
    else if (n == 6)
        return TYPE_CLAW;
    else if (n == 7)
        return TYPE_WHIP;
    else if (n == 8)
        return TYPE_PINCH;
    else if (n == 9)
        return TYPE_STING;
    else if (n == 10)
        return TYPE_BITE;
    else if (n == 11)
        return TYPE_PECK;
    else if (n == 12)
        return TYPE_GORE;
    else if (n == 13)
        return TYPE_PIKE;
    else if (n == 14)
        return TYPE_TRIDENT;
    else if (n == 15)
        return TYPE_POLEARM;
    else if (n == 16)
        return TYPE_KNIFE;
    else if (n == 17)
        return TYPE_RAZOR;

    return TYPE_HIT;
}


void
cmd_kill(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg, sizeof(arg));

    if ((GET_LEVEL(ch) < OVERLORD) || IS_NPC(ch)) {
        if (*arg) {
            victim = get_char_room_vis(ch, arg);
            if (victim) {
                if (IS_CALMED(ch)) {
                    send_to_char("You feel too at ease right now to " "attack anyone.\n\r", ch);
                    return;
                }
                if (victim == ch) {
                    send_to_char("You swing futilely through the air.\n\r", ch);
                    act("$n swipes futilely at the air.", FALSE, ch, 0, victim, TO_ROOM);
                } else {
                  if (is_charmed(ch) &&
                      (ch->master == victim)) {
                    act("$N is just such a good friend, " "you simply can't attack $M.", FALSE, ch, 0, victim, TO_CHAR);
                    return;
                  }
    
                    if ((ch->specials.riding) && (ch->specials.riding == victim)) {
                        act("That might be difficult while you're mounted on $M.", FALSE, ch, 0, victim,
                            TO_CHAR);
                        return;
                    }
    
                    if( victim->master == ch 
                     && IS_SET(victim->specials.act, CFL_MOUNT)) {
                       cmd_hitch(ch, argument, CMD_HITCH, 0);
                       return;
                    }
    
                    if (is_char_ethereal(ch) != is_char_ethereal(victim)) {
                        act("Your blow passes right through $M.", FALSE, ch, 0, victim, TO_CHAR);
                        act("$n swings at you, but $s blow passes through you.", FALSE, ch, 0, victim,
                            TO_VICT);
                        act("$n swings at $N, but the blow passes through $M.", FALSE, ch, 0, victim,
                            TO_NOTVICT);
                        return;
                    }
    
                    if (!CAN_FIGHT(ch)) {
                        act("What do you plan to do, bonk $M on the head ?", FALSE, ch, 0, victim,
                            TO_CHAR);
                        return;
                    }
    
                    if (guard_check(ch, victim))
                        return;
    
    
                    if ((GET_POS(ch) == POSITION_STANDING)
                        && !ch->specials.fighting && (victim != ch->specials.fighting)) {
                        if (MSTR(ch, name) && MSTR(victim, name)) {
                            sprintf(buf, "%s hits %s.\n\r", MSTR(ch, name), MSTR(victim, name));
                            send_to_monitors(ch, victim, buf, MONITOR_FIGHT);
                        }
    
                        show_init_combat(ch, victim);
    
                        hit(ch, victim, TYPE_UNDEFINED);
                        WAIT_STATE(ch, PULSE_VIOLENCE + 2);
                    } else {
                        if (trying_to_disengage(ch)
                            && ch->specials.fighting == victim) {
                            clear_disengage_request(ch);
    
                            if (MSTR(ch, name) && MSTR(victim, name)) {
                                sprintf(buf, "%s stops trying to disengage %s.\n\r", MSTR(ch, name),
                                        MSTR(victim, name));
                                send_to_monitors(ch, victim, buf, MONITOR_FIGHT);
                            }
    
                            hit(ch, victim, TYPE_UNDEFINED);
                            WAIT_STATE(ch, PULSE_VIOLENCE + 2);
                        } else {
                            send_to_char("You do the best you can!\n\r", ch);
                        }
                    }
                }
            } else {
                send_to_char("They aren't here.\n\r", ch);
            }
        } else {
            send_to_char("Kill who?\n\r", ch);
        }
        return;
    }

    if (!*arg) {
        send_to_char("Kill who?\n\r", ch);
    } else {
        if (!(victim = get_char_room_vis(ch, arg)))
            send_to_char("They aren't here.\n\r", ch);
        else if (ch == victim)
            send_to_char("Your mother would be so sad.. :(\n\r", ch);
        else {
            act("You send a bolt of red flame at $N, turning $M into dust.", FALSE, ch, 0, victim,
                TO_CHAR);
            act("$n sends a bolt of red flame at you, turning you into dust.", FALSE, ch, 0, victim,
                TO_VICT);
            act("$n sends a bolt of red flame at $N, turning $M into dust.", FALSE, ch, 0, victim,
                TO_NOTVICT);
            die(victim);
        }
    }
}



void
cmd_hit(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    cprintf(ch, "Brawling is not allowed here, use 'kill' if you really mean it.\n\r");
}

int
is_unit(CHAR_DATA *ch) {
   if( !ch ) return FALSE;
   return isname("unit", MSTR(ch, name));
}

void
cmd_order(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[100], message[256], temp_message[256];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char buf[256];
    bool found = FALSE;
    const char *tokptr = temp_message;
    ROOM_DATA *org_room;
    CHAR_DATA *victim = NULL;
    CHAR_DATA *guy;
    struct follow_type *k;

    argument = one_argument(argument, name, sizeof(name));
    strcpy(message, argument);

    if (!*name || !*message) {
        send_to_char("Order who to do what?\n\r", ch);
        return;
    }
    else if (!(victim = get_char_room_vis(ch, name)) && strcmp("follower", name)
             && strcmp("followers", name)) {
        send_to_char("You don't see that person here.\n\r", ch);
        return;
    } else if (ch == victim) {
        send_to_char("Doesn't that feel funny?\n\r", ch);
        return;
    }

    if (is_charmed(ch)) {
      send_to_char("Leave giving orders to your superior.\n\r", ch);
      return;
    }

    strcpy(temp_message, message);
    tokptr = temp_message;
    tokptr = one_argument(tokptr, arg1, sizeof(arg1));

    if (victim) {
        sprintf(buf, "$N orders you to '%s'", message);
        act(buf, FALSE, victim, 0, ch, TO_CHAR);
        act("$n gives $N an order.", FALSE, ch, 0, victim, TO_ROOM);

        if (!is_cmd_oob_command(victim, arg1) && victim->specials.act_wait ) {
            cprintf(ch, "%s is still busy and unable to respond.\n\r", capitalize(PERS(ch, victim)));
            return;
        }

        if ((victim->master != ch) || !is_charmed(victim))
          act("$n has an indifferent look.", FALSE, victim, 0, 0, TO_ROOM);
        else {
            /* Soldiers wont kill another templar */
            if (IS_TRIBE(victim, 8) || IS_TRIBE(victim, 12) || is_unit(victim)) {
                if (!str_prefix(arg1, "hit") || !str_prefix(arg1, "kill")
                 || !str_prefix(arg1, "bash") || !str_prefix(arg1, "subdue")
                 || !str_prefix(arg1, "disarm") || !str_prefix(arg1, "kick")
                 || !str_prefix(arg1, "backstab")) {

                    tokptr = one_argument(tokptr, arg2, sizeof(arg2));
                    guy = get_char_room_vis(ch, arg2);

                    if (guy != NULL) {
                        // units can't be ordered to attack
                        if( is_unit(victim) && guy
                         && !is_unit(guy) ) {
                            cprintf(ch, "%s can only attack other units.\n\r", capitalize(PERS(ch, victim)));
                            return;
                        }

                        if ((IS_TRIBE(victim, 8) || IS_TRIBE(victim,12))
                         && IS_IN_SAME_TRIBE(victim, guy)
                            && (GET_GUILD(guy) == GUILD_TEMPLAR
                                || GET_GUILD(guy) == GUILD_LIRATHU_TEMPLAR
                                || GET_GUILD(guy) == GUILD_JIHAE_TEMPLAR)) {
                            act("$n has an indifferent look.", FALSE, victim, 0, 0, TO_ROOM);
                            return;
                        }
                    }
                }           /* End if violent commands */
            }               /* End if tribe 8/12 */
            send_to_char("Ok.\n\r", ch);
            parse_command(victim, message);
        }
    } else {
        /* This is order "followers" */
        act("$n issues an order to all who will heed it.", FALSE, ch, 0, ch, TO_ROOM);

        org_room = ch->in_room;

        tokptr = one_argument(tokptr, arg2, sizeof(arg2));
        guy = get_char_room_vis(ch, arg2);

        for (k = ch->followers; k; k = k->next) {
            if (org_room == k->follower->in_room)
              if (is_charmed(k->follower)) {
                found = TRUE;
                
                if (!is_cmd_oob_command(k->follower, arg1) 
                    && k->follower->specials.act_wait ) {
                  cprintf(ch, "%s is still busy and unable to respond.\n\r",
                          capitalize(PERS(ch, k->follower)));
                  continue;
                }

                    /* Soldiers wont kill another templar */
                    if (IS_TRIBE(k->follower, 8) 
                     || IS_TRIBE(k->follower, TRIBE_ALLANAK_MIL)
                     || is_unit(k->follower)) {
                        if (!str_prefix(arg1, "hit") 
                         || !str_prefix(arg1, "kill")
                         || !str_prefix(arg1, "bash")
                         || !str_prefix(arg1, "subdue")
                         || !str_prefix(arg1, "disarm")
                         || !str_prefix(arg1, "kick")
                         || !str_prefix(arg1, "backstab")) {

                            if (guy != NULL) {
                                // units can't be ordered to attack
                                if (is_unit(k->follower) && !is_unit(guy)) {
                                    cprintf(ch, "%s can only attack other units.\n\r", capitalize(PERS(ch, k->follower)));
                                    continue;
                                }

                                if ((IS_TRIBE(k->follower, 8) || IS_TRIBE(k->follower,12))
                                 && IS_IN_SAME_TRIBE(k->follower, guy)
                                 && (GET_GUILD(guy) == GUILD_TEMPLAR
                                  || GET_GUILD(guy) == GUILD_LIRATHU_TEMPLAR
                                  || GET_GUILD(guy) == GUILD_JIHAE_TEMPLAR)) {
                                    act("$n has an indifferent look.", FALSE, k->follower, 0, 0,
                                        TO_ROOM);
                                    continue;
                                }
                            }
                        }   /* End if violent commands */
                    }       /* End if tribe 8/12 */
                    parse_command(k->follower, message);
                }
        }
        if (found)
            send_to_char("Ok.\n\r", ch);
        else
            send_to_char("Nobody here are loyal subjects of yours!\n\r", ch);
    }
}

int
wagon_in_room(OBJ_DATA * obj, void *room_num)
{
    long int rnum = (long int) room_num;

    if (obj && (obj->obj_flags.type == ITEM_WAGON)
        && (obj->obj_flags.value[0] == rnum))
        return 1;

    return 0;
}

void
cmd_flee(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  int i, attempt, loose, die = 0, can_flee, char_flying, ct = 0;
  int health;
  int room_noclimb;
  int room_airsect;
  int already_crim = 0;
  char arg1[256];
  char buf[256];
  CHAR_DATA *tch;
  OBJ_DATA *wagon = (OBJ_DATA *) 0;
  ROOM_DATA *old_room;
  bool flee_self = FALSE;
  struct affected_type *af;
  struct affected_type caf;
  char args[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  
  memset(&caf, 0, sizeof(af));
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  if (GET_POS(ch) <= POSITION_SLEEPING) {
    return;
  }
  
  if (is_paralyzed(ch)) {
    send_to_char("You are paralyzed and cannot move!\n\r", ch);
    return;
  }

  if (is_charmed(ch)) {
    send_to_char("Leave the side of your master?!  Perish the thought!\n\r", ch);
    return;
  }
  
  if (affected_by_spell(ch, AFF_MUL_RAGE)) {
    send_to_char("The blood-lust inside you is too strong to force yourself from combat.\n\r", ch);
    return;
  }
  
  if (ch->specials.subduing) {
    for (tch = ch->in_room->people; tch; tch = tch->next_in_room) {
      if (tch == ch->specials.subduing) {
        break;
      }
    }

    if (tch) {
      act("You quickly release $N.", FALSE, ch, 0, tch, TO_CHAR);
      act("$n quickly releases you.", FALSE, ch, 0, tch, TO_VICT);
      act("$n quickly releases $N.", FALSE, ch, 0, tch, TO_NOTVICT);
      ch->specials.subduing = (CHAR_DATA *) 0;
      REMOVE_BIT(tch->specials.affected_by, CHAR_AFF_SUBDUED);
    }
  }
  
  for (tch = ch->in_room->people; tch; tch = tch->next_in_room) {
    if (tch->specials.subduing == ch) {
      break; 
    }
  }
  
  if (tch) {
    if (GET_MOVE(ch) > 5) {
      if ((check_str(ch, -5) && !check_str(tch, 0)) ||
          (check_agl(ch, -5) && !check_agl(tch, 0))) {
        
        if (find_ch_keyword(tch, ch, arg1, sizeof(arg1))) {
          sprintf(to_room, "@ struggles against ~%s and breaks free", arg1);
          sprintf(to_char, "you struggle against ~%s and break free", arg1);
        } else {
          sprintf(to_room, "@ struggles and breaks free");
          sprintf(to_char, "you struggle and break free");
        }
        send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_FIGHT);
        
        ct = room_in_city(tch->in_room);
        if (ct && 
            IS_TRIBE(tch, city[ct].tribe) &&
            IS_SET(ch->specials.act, city[ct].c_flag)) {
          for (af = ch->affected; af; af = af->next) {
            if (af->type == TYPE_CRIMINAL && af->modifier == city[ct].c_flag) {
              if (af->power > CRIME_UNKNOWN && af->power <= MAX_CRIMES) {
                already_crim = 1;
                break;
              }
            }
          }
          
          if (IS_SET(ch->specials.nosave, NOSAVE_ARREST)) {
            cmd_nosave(ch, "arrest", CMD_NOSAVE, 0);
          }
          
          if (!already_crim) {
            caf.type = TYPE_CRIMINAL;
            caf.location = CHAR_APPLY_CFLAGS;
            caf.duration = number(5, 10);
            caf.modifier = city[ct].c_flag;
            caf.bitvector = 0;
            caf.power = CRIME_ASSAULT;
            affect_to_char(ch, &caf);
            send_to_char("You're now wanted!\n\r", ch);
          }
          
          /* In your hometown, there's a 1 in 5 chance of NPC soldiers attacking you,
           * if not, a 1 in 2 chnace.
           */
          bool attack_fleer = GET_HOME(ch) ? !number(0,4) : !number(0,1);
          if (IS_NPC(tch) &&
              attack_fleer) {
            find_ch_keyword(ch, tch, buf, sizeof(buf));
            cmd_kill(tch, buf, CMD_KILL, 0);
          }
        }
        
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_SUBDUED);
        tch->specials.subduing = (CHAR_DATA *) 0;
        
        WAIT_STATE(tch, 3);
      } else {
        if (find_ch_keyword(tch, ch, arg1, sizeof(arg1))) {
          sprintf(to_room, "@ struggles in vain against ~%s", arg1);
          sprintf(to_char, "you struggle in vain against ~%s", arg1);
        } else {
          sprintf(to_room, "@ struggles in vain");
          sprintf(to_char, "you struggle in vain");
        }
        send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_FIGHT);
        
        ct = room_in_city(tch->in_room);
        if (ct && IS_TRIBE(tch, city[ct].tribe) &&
            IS_SET(ch->specials.act, city[ct].c_flag)) {
          for (af = ch->affected; af; af = af->next) {
            if (af->type == TYPE_CRIMINAL && af->modifier == city[ct].c_flag) {
              if (af->power > CRIME_UNKNOWN && af->power <= MAX_CRIMES) {
                already_crim = 1;
                break;
              }
            }
          }
          
          if (IS_SET(ch->specials.nosave, NOSAVE_ARREST)) {
            cmd_nosave(ch, "arrest", CMD_NOSAVE, 0);
          }
          
          if (!already_crim) {
            caf.type = TYPE_CRIMINAL;
            caf.location = CHAR_APPLY_CFLAGS;
            caf.duration = number(5, 10);
            caf.modifier = city[ct].c_flag;
            caf.bitvector = 0;
            caf.power = CRIME_ASSAULT;
            affect_to_char(ch, &caf);
            send_to_char("You're now wanted!\n\r", ch);
          }
          
          if (!number(0, 4)) {
            find_ch_keyword(tch, ch, buf, sizeof(buf));
            cmd_kill(tch, buf, CMD_KILL, 0);
          }
        }
        
        adjust_move(ch, -5);
        WAIT_STATE(ch, 5);
      }
      return;
    } else {
      act("$N starts to struggle against you, but appears too exhausted.", FALSE, tch, 0, ch, TO_VICT);
      act("You are too exhausted to try and escape from $n.", FALSE, tch, 0, ch, TO_VICT);
      act("$N begins struggling against $n, but is too exhausted to continue.", FALSE, tch, 0, ch, TO_VICT);
      
      WAIT_STATE(ch, 5);
      
      return;
    }
  }
  
  if (ch->in_room->sector_type != SECT_SILT) {
    GET_SPEED(ch) = SPEED_RUNNING;
  }
  
  argument = one_argument(args, arg1, sizeof(arg1));
  
  if ((!(ch->specials.fighting) || GET_POS(ch) != POSITION_FIGHTING) && 
      (!IS_NPC(ch))) {
    if (!STRINGS_SAME(arg1, "self") || !*arg1) {
      send_to_char("You aren't in combat, to flee anyway, type: flee self." "\n\r", ch);
      return;
    } else {
      flee_self = TRUE;
    }
  }
  
  if (ch->specials.riding) {
    if (!has_skill(ch, SKILL_RIDE)) {
      send_to_char("Your mount cannot break away from combat!\n\r", ch);
      return;
    }
    
    if ((number(1, 100) > ch->skills[SKILL_RIDE]->learned) ||
        ((GET_GUILD(ch) == GUILD_RANGER) &&
         (number(1, 70) > ch->skills[SKILL_RIDE]->learned))) {
      send_to_char("PANIC! You couldn't escape!\n\r", ch);
      return;
    }
    
    GET_SPEED(ch->specials.riding) = SPEED_RUNNING;
  }

  wagon = find_exitable_wagon_for_room(ch->in_room);
  
  /* six attempts to find an exit */
  for (i = 0; i < 6; i++) {
    /* If they can flee well enough, let them pick a direction */
    if (has_skill(ch, SKILL_FLEE) && 
        get_skill_percentage(ch, SKILL_FLEE) > 30) {
      attempt = get_dir_from_name(arg1);
      if (DIR_UNKNOWN == attempt) {
        attempt = number(0,5);
      }
    } else {
      attempt = number(0, 5);     /* Select a random direction */
    }

    can_flee = FALSE;
    
    if (CAN_GO(ch, attempt)) {
      if (exit_guarded(ch, attempt, CMD_FLEE)) {
        return;
      }
    }
    
    // TODO: Replace this with the is_flying check.
    char_flying = (IS_AFFECTED(ch, CHAR_AFF_FLYING) &&
                   (!(affected_by_spell(ch, SPELL_LEVITATE))) &&
                   (!(affected_by_spell(ch, SPELL_FEATHER_FALL))));
    
    if (CAN_GO(ch, attempt)) {
      room_noclimb = IS_SET(EXIT(ch, attempt)->to_room->room_flags, RFL_NO_CLIMB);
      room_airsect = EXIT(ch, attempt)->to_room->sector_type == SECT_AIR;
    } else {
      room_noclimb = TRUE;
      room_airsect = TRUE;
    };
    
    if (wagon) {
      can_flee = TRUE;
    } else if (!(CAN_GO(ch, attempt))) {
      can_flee = FALSE;
    }  else if (IS_SET(EXIT(ch, attempt)->exit_info, EX_SPL_SAND_WALL) ||
                IS_SET(EXIT(ch, attempt)->exit_info, EX_NO_FLEE) ||
                IS_SET(EXIT(ch, attempt)->to_room->room_flags, RFL_DEATH)) {
      can_flee = FALSE;
    } else if ((room_noclimb && !room_airsect) || (room_noclimb && room_airsect && !char_flying)) {
      can_flee = FALSE;
    } else {
      can_flee = TRUE;
    }
    
    if (can_flee) {
      sprintf(to_char, "@ attempt to flee");
      sprintf(to_room, "@ attempts to flee");
      
      send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_FIGHT);
      
      // skill check here, if they have the skill
      CHAR_DATA *rch;
      int count = 0;
      // if no skill, base 0 chance
      int flee_skill = !has_skill(ch, SKILL_FLEE) ? 0 : get_skill_percentage(ch, SKILL_FLEE);
      
      // agility reaction can improve chances of getting away
      flee_skill += agl_app[GET_AGL(ch)].reaction;
      
      // check against each attacker
      for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        // opponents agility is a negative
        int rch_reaction = agl_app[GET_AGL(rch)].reaction;
        bool alive = TRUE;
        
        // not fighting ch or disengaged
        if ((rch->specials.fighting != ch &&
             rch->specials.alt_fighting != ch) ||
            trying_to_disengage(rch) ||
            IS_SET(rch->specials.mercy, MERCY_FLEE)) {
          continue;
        }
        
        
        // if fleer fails, they get hit by the attacker
        // harder as the # of combatants go up
        if( number(1, 70) > flee_skill - rch_reaction - (10 * count)) {
          act("$n seizes the opening and attacks $N.", TRUE, rch, ch, NULL, TO_NOTVICT);
          act("$n seizes the opening and attacks you.", TRUE, rch, ch, NULL, TO_VICT);
          act("You seize the opening and attack $N.", FALSE, rch, ch, NULL, TO_CHAR);
          alive = hit(rch, ch, TYPE_UNDEFINED);
          gain_skill(ch, SKILL_FLEE, number(1, 2));
        }
        
        count++;
        
        // make sure they are still alive after each hit
        if (!alive || knocked_out(ch)) {
          return;
        }
      }
      
      old_room = ch->in_room;
      
      if (wagon) {
        // Indicates they weren't able to leave (out_guarded)
        if (out_guarded(ch)) {
          return;
        }
        
        if (ch && ch->specials.fighting)
          if (ch->specials.fighting || GET_POS(ch) == POSITION_FIGHTING) {
            for (tch = ch->in_room->people; tch; tch = tch->next_in_room) {
              if (tch->specials.fighting == ch ||
                  tch->specials.alt_fighting == ch) {
                stop_fighting(tch, ch);
                stop_fighting(ch, tch);
              }
              
              if (CAN_SEE(ch, tch)) {
                if (does_hate(ch, tch)) {
                  remove_from_hates(ch, tch);
                }
                if (!IS_SWITCHED(ch) && !does_fear(ch, tch)) {
                  add_to_fears(ch, tch); 
                }
              }
            }
            stop_all_fighting(ch);
          }
        cmd_leave(ch, "", 0, 0);
        return;
      } else {
        die = cmd_simple_move(ch, attempt, FALSE, NULL);
      }
      
      if ((die == 1) || (wagon)) {    /* The escape has succeded */
        
        loose = 1;  /* added this  ten */
        
        if (ch->skills[SKILL_FLEE]) {
          sprintf(buf, "You flee, heading %s.\n\r", dirs[attempt]);
          send_to_char(buf, ch); 
        } else {
          send_to_char("You flee head over heels.\n\r", ch);
        }
        
        if (ch && ch->specials.fighting)
          if (ch->specials.fighting || GET_POS(ch) == POSITION_FIGHTING) {
            for (tch = old_room->people; tch; tch = tch->next_in_room) {
              if (tch->specials.fighting == ch ||
                  tch->specials.alt_fighting == ch) {
                stop_fighting(tch, ch);
                stop_fighting(ch, tch);
              }
              
              if (CAN_SEE(ch, tch)) {
                if (does_hate(ch, tch)) {
                  remove_from_hates(ch, tch);
                }
                if (!does_fear(ch, tch)) {
                  add_to_fears(ch, tch);
                }
              }
            }
            stop_all_fighting(ch);
            return;
          }
        return;
      } else if (!die) {
        act("$n tries to flee, but is too exhausted!", TRUE, ch, 0, 0, TO_ROOM);
        return;
      } else if (die == -1) {
        /* they died */
        return;
      }
    }
  }                           /* for */

  /* No exits were found */
  send_to_char("PANIC! You couldn't escape!\n\r", ch);
  snprintf(buf, sizeof(buf), "%s panics.", MSTR(ch, short_descr));
  send_to_empaths(ch, buf);
  
  /* get the % of health remaining */
  if (GET_MAX_HIT(ch) == 0) {
    health = 100;
  } else {
    health = (GET_HIT(ch) * 100) / GET_MAX_HIT(ch);
  }
  
  /* get the percentage of health gone + 20% */
  health = 100 - health + 20;
  
  /* max of 95% chance */
  health = MIN(health, 95);
  
  /* add a skill gain here, only if not using 'flee self' -Morg */
  if (!flee_self && 
      number(1, 100) < health) {
    gain_skill(ch, SKILL_FLEE, number(1, 2));
  }
}


/* Called when a character that follows/is followed dies */
void
die_follower(CHAR_DATA * ch)
{
    struct follow_type *j, *k;

    if (ch->master)
        stop_follower(ch);

    for (k = ch->followers; k; k = j) {
        j = k->next;
        stop_follower(k->follower);
    }
}

