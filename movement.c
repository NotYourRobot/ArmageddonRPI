/*
 * File: MOVEMENT.C
 * Usage: Commands for moving players around.
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

/* Revision history.  Pls. list changes here.  
 * 12/17/1999  fixing forest sectors, edited line 5036 -sanvean
 * 5/4/2000    cmd_mount; tamed mounts have FLEE flag removed on success -savak
 * 3/22/2002   moved cmd_light and cmd_extinguish to light.c -morgenes
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>

#include "constants.h"
#include "structs.h"
#include "limits.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "guilds.h"
#include "clan.h"
#include "event.h"
#include "modify.h"
#include "object_list.h"
#include "psionics.h"
#include "wagon_save.h"
#include "watch.h"
#include "mount.h"
#include "cities.h"

extern void sflag_to_char(CHAR_DATA * ch, long sflag);

/* local prototypes */
void death_cry(CHAR_DATA * ch);
int get_wagon_delay(OBJ_DATA * wagon);
bool wagon_back_up(OBJ_DATA * wagon, int dir);

int cmd_simple_obj_move(OBJ_DATA * obj_leader,  /* obj that is moving */
                        int cmd,                /* direction they're going in */
                        int following,          /* Whether or not they're followed */
                        int speed,              /* how fast is it being dragged */
                        const char *argument);  /* argument for cmd_move */

bool
ch_starts_falling_check(CHAR_DATA *ch) {
  char buf[MAX_STRING_LENGTH];
  if (!ch) {
    shhlog("ch not found in ch_starts_falling_check(), exiting.");
    return FALSE;
  }

  if (!ch->in_room) {
    shhlog("Character not in room, for some reason, exiting ch_starts_falling_check().");
    return FALSE;
  }
  
  if (IS_SET(ch->specials.affected_by, CHAR_AFF_CLIMBING) &&
      (!(is_flying_char(ch) >= FLY_LEVITATE)) &&
      (!IS_IMMORTAL(ch)) &&
      (GET_POS(ch) != POSITION_FIGHTING) &&
      (GET_POS(ch) != POSITION_SITTING) &&
      (GET_POS(ch) != POSITION_SLEEPING)) {
    REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_CLIMBING);

    if (IS_SET(ch->in_room->room_flags, RFL_FALLING)) {
      send_to_char("You let go of the wall...bad move.\n\r", ch);
      act("$n lets go of the wall.", TRUE, ch, 0, 0, TO_ROOM);
      
      sprintf(buf, "%s lets go of the wall.\n\r", MSTR(ch, name));
      send_to_monitor(ch, buf, MONITOR_POSITION);
      
      if (IS_SET(ch->specials.affected_by, CHAR_AFF_HIDE)) {
        sprintf(buf, "%s is no longer hidden.\n\r", MSTR(ch, name));
        send_to_monitor(ch, buf, MONITOR_POSITION);
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
      }
      
      set_char_position(ch, POSITION_STANDING);
      WIPE_LDESC(ch);
      
            add_char_to_falling(ch);
      //      fallch(ch, 0);
      return TRUE;
    }
  }
  return FALSE;
}

ROOM_DATA *
get_random_room_from_exits (CHAR_DATA *ch) {
  int cmd = number(1, 5);

  if (cmd == CMD_UP) {
    cmd = CMD_DOWN;
  }
  
  if (ch->in_room->direction[cmd-1]) {
    return ch->in_room->direction[cmd - 1]->to_room;
  }

  return NULL;
}

bool
damage_absorbed_by_invulnerability(CHAR_DATA *ch) {
  if (affected_by_spell(ch, SPELL_INVULNERABILITY)) {
    send_to_char("The creamy shell around you fades away.\n\r", ch);
    act("The creamy shell around $n fades away.", FALSE, ch, 0, 0, TO_ROOM);
    
    affect_from_char(ch, SPELL_INVULNERABILITY);

    if (IS_AFFECTED(ch, CHAR_AFF_INVULNERABILITY)) {
      REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_INVULNERABILITY);
    }

    return TRUE;
  }

  return FALSE;
}

#define RIDE_CHECK_FAILURE_CRITICAL -1
#define RIDE_CHECK_FAILURE 0
#define RIDE_CHECK_SUCCESS_PARTIAL 1
#define RIDE_CHECK_SUCCESS 2 
#define RIDE_CHECK_SUCCESS_CRITICAL 3
  
int
ride_check(CHAR_DATA *ch) {
  int skill = 0;
  int penalty = 0;
  
  if (!has_skill(ch, SKILL_RIDE)) {
    return RIDE_CHECK_FAILURE;
  }
  
  skill = ch->skills[SKILL_RIDE]->learned;
  
  if (!ch->specials.riding) {
    return RIDE_CHECK_SUCCESS;
  }
  
  /* Hands Free Penalty:
   *   2 hands free = 10% bonus
   *   1 hand = no bonus
   *   no hands = 10% penalty
   */
  penalty -= (hands_free(ch) - 1) * 10;
  
  // Speed modifier, running adds 10% penalty
  if (SPEED_RUNNING == GET_SPEED(ch->specials.riding)) {
    penalty += 10;
  }
  
  // sector/terrain mods 
  switch (ch->in_room->sector_type) {
    // easy to do
  case SECT_CITY:
  case SECT_INSIDE:
  case SECT_ROAD:
    penalty -= 15;
    break;
    
    // an average task
  case SECT_FIELD:
  case SECT_GRASSLANDS:
  case SECT_SALT_FLATS:
  case SECT_COTTONFIELD:
  case SECT_RUINS:
  default:
    penalty -= 10;
    break;
    
    // hard terrain 
  case SECT_CAVERN:
  case SECT_SOUTHERN_CAVERN:
  case SECT_HILLS:
  case SECT_DESERT:
  case SECT_SEWER:
  case SECT_SHALLOWS:
  case SECT_LIGHT_FOREST:
  case SECT_THORNLANDS:
    // ratlons/gwoshi are good at difficult terrain
    if (GET_RACE(ch->specials.riding) == RACE_RATLON ||
        GET_RACE(ch->specials.riding) == RACE_GWOSHI) {
      // reduces the penalty by one level
      penalty -= 10;
    } else {
      penalty += 0;
    }
    break;
    
    // difficult terrain 
  case SECT_MOUNTAIN:
  case SECT_HEAVY_FOREST:
  case SECT_SILT:
    // Ratlons and Gwoshi are good at difficult riding
    if( GET_RACE(ch->specials.riding) == RACE_RATLON ||
        GET_RACE(ch->specials.riding) == RACE_GWOSHI) {
      // reduces the penalty by one level
      penalty += 0;
    } else {
      penalty += 15;
    }
    break;
  }
  
  if (skill - penalty <= 0) {
    return RIDE_CHECK_FAILURE_CRITICAL;
  }

  if (skill - penalty > 40) {
    return RIDE_CHECK_SUCCESS_CRITICAL;
  }
  
  if (number(1, 40) > skill - penalty) {
    if (penalty > 0) {
      return RIDE_CHECK_FAILURE;
    }
    return RIDE_CHECK_SUCCESS_PARTIAL;
  }

  return RIDE_CHECK_SUCCESS;
}

#define BURROW_ZONE 73
bool
stand_from_burrow(CHAR_DATA * ch, bool forced) {
  struct room_data *surface_room, *burrow_room;
  char buf[MAX_STRING_LENGTH];
  
  if (!ch || 
      !ch->in_room ||
      (BURROW_ZONE != ch->in_room->zone)) {
    return FALSE;
  }
  
  if ((IS_AFFECTED(ch, CHAR_AFF_BURROW) ||
       affected_by_spell(ch, SPELL_BURROW) || 
       forced) &&
      ch->in_room->direction[DIR_UP] &&
      ch->in_room->direction[DIR_UP]->to_room) {
    act("You poke your head through the portal, and are transported above ground.", FALSE, ch, 0, 0, TO_CHAR);
    
    surface_room = ch->in_room->direction[DIR_UP]->to_room;
    burrow_room = ch->in_room;
    
    char_from_room(ch);
    char_to_room(ch, surface_room);
    
    if (!burrow_room->people) {
      free_reserved_room(burrow_room);
    }
    
    act("$n pokes $s head through the ground, and rises up to the surface.", FALSE, ch, 0, 0, TO_ROOM);
    cmd_look(ch, "", -1, 0);
    
    set_char_position(ch, POSITION_STANDING);
    
    sprintf(buf, "%s emerges from %s burrow.\n\r", MSTR(ch, name), HSHR(ch));
    send_to_monitor(ch, buf, MONITOR_MAGICK);
    
    sprintf(buf, "%s emerges into %s [%d].\n\r", MSTR(ch, name), ch->in_room->name, ch->in_room->number);
    send_to_monitor(ch, buf, MONITOR_MOVEMENT);
    
    affect_from_char(ch, SPELL_BURROW);
    REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_BURROW);

    return TRUE;
  }
  
  return FALSE;
}

bool
show_remove_occupant(CHAR_DATA * ch, char *preHow, char *postHow) {
  int search_bitvector = FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM;
  char used_arg[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  OBJ_DATA *furniture_obj;

  if (!ch->on_obj) {
    return TRUE;
  }
  
  furniture_obj = ch->on_obj;
  
  if (furniture_obj->table) {
    furniture_obj = furniture_obj->table;
  }
  
  /* get a usable keyword for the object they're on */
  find_obj_keyword(furniture_obj, search_bitvector, ch, used_arg, sizeof(used_arg));
  
  // Not sure how to better handle if you can't see the object you're on
  if (!CAN_SEE_OBJ(ch, furniture_obj)) {
    sprintf(to_char, "you stand up from something you can't see");
    sprintf(to_room, "@ stands up");
  } else {
    switch (GET_POS(ch)) {
    case POSITION_STANDING:
      /* on a skimmer? */
      if (IS_SET(furniture_obj->obj_flags.value[1], FURN_SKIMMER) ||
          IS_SET(furniture_obj->obj_flags.value[1], FURN_WAGON)) {
        sprintf(to_char, "you step off of ~%s", used_arg);
        sprintf(to_room, "@ steps off of ~%s", used_arg);
      } else {            /* must be standing at something */
        sprintf(to_char, "you push away from ~%s", used_arg);
        sprintf(to_room, "@ pushes away from ~%s", used_arg);
      }
      break;
      
    default:
      sprintf(to_char, "you stand up from ~%s", used_arg);
      sprintf(to_room, "@ stands up from ~%s", used_arg);
      break;
    }
  }
  
  if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
    return FALSE;
  }
  
  remove_occupant(ch->on_obj, ch);
  set_char_position(ch, POSITION_STANDING);

  return TRUE;
}


/* part of the sit/rest/sleep on furniture code by Morgenes */
bool
position_ch_on_obj(CHAR_DATA * ch, char *preHow, const char *argument, char *postHow, const int cmd) {
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char at_on[MAX_STRING_LENGTH];
  char used_arg[MAX_STRING_LENGTH];
  OBJ_DATA *chair;
  OBJ_DATA *table;
  int newPos;
  int reqFlag;
  char position[MAX_STRING_LENGTH];
  int bitvector = FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM;
  
  /* initialize used_arg */
  used_arg[0] = '\0';
  
  /* sleeping (or worse) and fighting should not be delt with here */
  if (GET_POS(ch) < POSITION_RESTING || 
      GET_POS(ch) > POSITION_STANDING) {
    return FALSE;
  }
  
  /* figure out what position they're wanting to go to from the CMD_ */
  switch (cmd) {
  case CMD_SIT:
    newPos = POSITION_SITTING;
    reqFlag = FURN_SIT;
    strcpy(position, "sit");
    break;
  case CMD_REST:
    newPos = POSITION_RESTING;
    reqFlag = FURN_REST;
    strcpy(position, "rest");
    break;
  case CMD_SLEEP:
    newPos = POSITION_SLEEPING;
    reqFlag = FURN_SLEEP;
    strcpy(position, "sleep");
    break;
  case CMD_STAND:
    newPos = POSITION_STANDING;
    reqFlag = FURN_STAND;
    strcpy(position, "stand");
    break;
  default:
    return FALSE;
  }
  
  /* Get the arguments */
  argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
  chair = NULL;
  table = NULL;
  
  /* Did they specify where they wanted to change their position to? */
  if (arg1[0] != '\0') {
    /* A second argument? ie: at/on/with/<chair> <table> */
    if (arg2[0] != '\0') {
      /* <position> at <table> */
      if (!strcmp(arg1, "at")) {
        /* if we don't find the table in the room */
        if ((table = get_obj_in_list_vis(ch, arg2, ch->in_room->contents)) == NULL) {
          /* if it's a number */
          if (is_number(arg2)) {
            int counter = 0;
            int num;
            
            num = atoi(arg2);
            /* look through the room contents for a table */
            for (table = ch->in_room->contents; table; table = table->next_content) {
              /* is it a table or a furniture object with
               * the required flag? */
              if (table->obj_flags.type == ITEM_FURNITURE &&
                  (IS_SET(table->obj_flags.value[1], FURN_TABLE) ||
                   (IS_SET(table->obj_flags.value[1], reqFlag) &&
                    table->obj_flags.value[2] > 1)) &&
                  CAN_SEE_OBJ(ch, table)) {
                /* if the counter matches, break */
                if (++counter == num) {
                  find_obj_keyword(table, bitvector, ch, used_arg, sizeof(used_arg));
                  break;
                }
              }
            }       /* end for each table */
          }
          
          /* end if it's a number */
          /* if no table, error out */
          if (!table) {
            sprintf(buf, 
                    "You don't see any '%s' here to %s %s.\n\r", 
                    arg2, 
                    position, 
                    newPos == POSITION_STANDING ? "at" : "on");
            send_to_char(buf, ch);
            return TRUE;
          }
        }
        /* end if we didn't find it in the room */
        /* else we found the table in the room */
        else {          /* save that we used arg2 */
          strcpy(used_arg, arg2);
        }
        
        /* see if we can sit on what they said */
        if (table->obj_flags.type != ITEM_FURNITURE) {
          sprintf(buf, "You can't %s on %s.\n\r", position, format_obj_to_char(table, ch, 1));
          send_to_char(buf, ch);
          return TRUE;
        }
        
        /* verify there is room at it */
        /* if it's a table object */
        if (IS_SET(table->obj_flags.value[1], FURN_TABLE)) {
          /* if they're asking to stand at it and you can */
          if (reqFlag == FURN_STAND && 
              IS_SET(table->obj_flags.value[1], reqFlag)) {
            int count = 0;
            
            /* take away seats that are pulled up */
            for (chair = table->around; chair; chair = chair->next_content) {
              count += chair->obj_flags.value[2];
            }
            
            if (count_plyr_list(table->occupants, ch) + count < MAX(table->obj_flags.value[2], 1)) {
              chair = table;
            } else {
              act("There is no space at $p.", FALSE, ch, table, NULL, TO_CHAR);
              return TRUE;
            }
          } else {    /* look for chairs */
            for (chair = table->around; chair; chair = chair->next_content) {
              /* if we can do what they requested on it 
               * and it's not full, break */
              if (IS_SET(chair->obj_flags.value[1], reqFlag) &&
                  count_plyr_list(chair->occupants, ch) < MAX(chair->obj_flags.value[2], 1))
                break;
            }
            
            if (!chair) {
              act("There is no space at $p.", FALSE, ch, table, NULL, TO_CHAR);
              return TRUE;
            }
          }
        }
        /* end if it's a table */
        /* else if we can do 'reqFlag' at it */
        else if (IS_SET(table->obj_flags.value[1], reqFlag)) {
          /* use it as a chair */
          chair = table;
          table = NULL;
          
          /* check to see if the chair is full */
          if (count_plyr_list(chair->occupants, ch) >=
              MAX(chair->obj_flags.value[2], 1)) {
            act("There is no space at $p.", FALSE, ch, chair, NULL, TO_CHAR);
            return TRUE;
          }
        } else {
          sprintf(buf, "You can't %s at $p.", position);
          act(buf, FALSE, ch, table, NULL, TO_CHAR);
          return TRUE;
        }
      } else if (!strcmp(arg1, "on")) {   /* <position> on <obj> */
        /* look for the chair in the room */
        if ((chair = get_obj_in_list_vis(ch, arg2, ch->in_room->contents)) == NULL) {
          /* didn't find it give a message and error out */
          sprintf(buf, "You don't see any '%s' here to %s on.\n\r", arg2, position);
          send_to_char(buf, ch);
          return TRUE;
        } else {        /* save that we used arg2 */
          strcpy(used_arg, arg2);
          
          if (!IS_SET(chair->obj_flags.value[1], reqFlag)) {
            sprintf(buf, "You can't %s on $p.", position);
            act(buf, FALSE, ch, chair, NULL, TO_CHAR);
            return TRUE;
          }
          
          /* check to see if the chair is full */
          if (count_plyr_list(chair->occupants, ch) >= MAX(chair->obj_flags.value[2], 1)) {
            act("There is no space on $p.", FALSE, ch, chair, NULL, TO_CHAR);
            return TRUE;
          }
        }
      } else if (!strcmp(arg1, "with")) { /* <position> with <person> */
        CHAR_DATA *rch;
        chair = NULL;
        table = NULL;
        
        if (NULL == (rch = get_char_room_vis(ch, arg2))) {
          /* didn't find it give a message and error out */
          sprintf(buf, "You don't see any '%s' here to %s with.\n\r", arg2, position);
          send_to_char(buf, ch);
          return TRUE;
        } else {
          if (rch == ch) {
            send_to_char("How do you plan to do that with yourself?\n\r", ch);
            return TRUE;
          }
          
          if (!rch->on_obj) {
            act("$N is not on anything.", FALSE, ch, NULL, rch, TO_CHAR);
            return TRUE;
          }
          
          /* look for open seats */
          /* is the person at a table? */
          if ((table = rch->on_obj->table) == NULL) {
            /* not at a table */
            chair = rch->on_obj;
            
            /* can you do reqFlag on chair? */
            if (!IS_SET(chair->obj_flags.value[1], reqFlag)) {
              sprintf(buf, "You can't %s on $p.", position);
              act(buf, FALSE, ch, chair, NULL, TO_CHAR);
              return TRUE;
            }
            
            /* if it's full */
            if (count_plyr_list(chair->occupants, ch) >= MAX(chair->obj_flags.value[2], 1)) {
              /* give error message and quit */
              act("There is no space with $N.", FALSE, ch, NULL, rch, TO_CHAR);
              return TRUE;
            }
          }
          
          /* if we haven't found a chair */
          if (!chair) {
            /* look for a chair */
            for (chair = table->around; chair; chair = chair->next_content) {
              /* if we can do reqFlag on the chair
               * and it's not full */
              if (IS_SET(chair->obj_flags.value[1], reqFlag) &&
                  count_plyr_list(chair->occupants, ch) < MAX(chair->obj_flags.value[2], 1))
                break;
            }
            
            /* if we haven't found a chair yet, 
             * and can do 'reqFlag' at/on the table */
            if (!chair && IS_SET(table->obj_flags.value[1], reqFlag)) {
              int count = 0;
              
              /* count up seats that are pulled up */
              for (chair = table->around; chair; chair = chair->next_content) {
                count += chair->obj_flags.value[2];
              }
              
              /* if the table isn't full */
              if (count_plyr_list(table->occupants, ch) + count < MAX(table->obj_flags.value[2], 1)) {
                chair = table;
              } else {
                act("There is no space at $p.", FALSE, ch, table, NULL, TO_CHAR);
                return TRUE;
              }
            } else if (!chair) {    /* no chairs */
              act("There is no space at $p.", FALSE, ch, table, NULL, TO_CHAR);
              return TRUE;
            }       /* end looking for chairs */
          }       /* end if we haven't found a chair */
        }       /* end else we found the person we're looking for */
      } else {            /* <position> <chair> <table> */
        /* if we can't find <table>? */
        if (NULL == (table = get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
          sprintf(buf, "You don't see any '%s' here to %s at.\n\r", arg2, position);
          send_to_char(buf, ch);
          return TRUE;
        } else {        /* we found the table */
          /* look for <chair> around <table> */
          if (NULL == (chair = get_obj_in_list_vis(ch, arg1, table->around))) {
            act("There's no $T around $p.", FALSE, ch, table, arg1, TO_CHAR);
            return TRUE;
          }
          
          /* can you do reqFlag on chair? */
          if (!IS_SET(chair->obj_flags.value[1], reqFlag)) {
            sprintf(buf, "You can't %s on $p.", position);
            act(buf, FALSE, ch, chair, NULL, TO_CHAR);
            return TRUE;
          }
          
          /* if it's full */
          if (count_plyr_list(chair->occupants, ch) >= MAX(chair->obj_flags.value[2], 1)) {
            /* give error message and quit */
            act("There is no space on $p.", FALSE, ch, chair, NULL, TO_CHAR);
            return TRUE;
          }
        }               /* end else we found the table */
      }                   /* end else <position> <chair> <table> */
    }
    /* end if arg2 specified */
    /* if we can find chair with arg1 */
    else if (NULL == (chair = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
      /* didn't find anything, but they specified an argument */
      sprintf(buf, 
              "You don't see any '%s' here to %s %s.\n\r", 
              arg1, 
              position, 
              newPos == POSITION_STANDING ? "at" : "on");
      send_to_char(buf, ch);
      return TRUE;
    } else {                /* we found the chair */
      /* save that we used arg1 */
      strcpy(used_arg, arg1);
      
      /* if is it furniture, and it has chairs around it */
      if (chair->obj_flags.type == ITEM_FURNITURE &&
          IS_SET(chair->obj_flags.value[1], FURN_TABLE)) {
        int count = 0;
        
        /* use it as a table */
        table = chair;
        
        /* look around for a chair */
        for (chair = table->around; chair; chair = chair->next_content) {
          /* save how many seats at the table are taken by chair */
          count += chair->obj_flags.value[2];
          
          /* if we can do reqFlag on the chair, and it's not full */
          if (IS_SET(chair->obj_flags.value[1], reqFlag) &&
              count_plyr_list(chair->occupants, ch) < MAX(chair->obj_flags.value[2], 1)) {
            break;
          }
        }
        
        /* if trying to stand around a table, let them if there
         * are open spots
         */
        if (!chair && IS_SET(table->obj_flags.value[1], reqFlag)) {
          if (count_plyr_list(table->occupants, ch) + count < MAX(table->obj_flags.value[2], 1)) {
            chair = table;
          } else {
            act("There's no space at $p.", FALSE, ch, table, NULL, TO_CHAR);
            return TRUE;
          }
        } else if (!chair) {
          act("There's no space at $p.", FALSE, ch, table, NULL, TO_CHAR);
          return TRUE;
        }
      }
      
      /* end if it is a table */
      /* can you sit on chair? */
      if (chair->obj_flags.type != ITEM_FURNITURE ||
          (chair->obj_flags.type == ITEM_FURNITURE &&
           !IS_SET(chair->obj_flags.value[1], reqFlag))) {
        sprintf(buf, "You can't %s on that.\n\r", position);
        send_to_char(buf, ch);
        return TRUE;
      }
      
      if (count_plyr_list(chair->occupants, ch) >= MAX(chair->obj_flags.value[2], 1)) {
        if (chair->obj_flags.value[2] > 1) {
          act("$p is full.", FALSE, ch, chair, NULL, TO_CHAR);
        } else {
          act("$N is already in it!", FALSE, ch, NULL, chair->occupants->ch, TO_CHAR);
        }
        return TRUE;
      }
      
    }   /* end if we have a chair */
  } else {  /* no arg1 */
    
    /* if they can do the new position on the existing chair */
    if (newPos != GET_POS(ch) &&
        ch->on_obj && IS_SET(ch->on_obj->obj_flags.value[1], reqFlag)) {
      chair = ch->on_obj;
      if (chair->table) {
        table = chair->table;
      }
    }
    // Watch for trying to stand up on a skimmer while in silt
    else if (ch->on_obj &&
             IS_SET(ch->on_obj->obj_flags.value[1], FURN_SKIMMER) &&
             ch->in_room->sector_type == SECT_SILT) {
      act("Getting off $p would be suicide!", FALSE, ch, ch->on_obj, NULL, TO_CHAR);
      return TRUE;
    } else {
      if (ch->on_obj && newPos != POSITION_STANDING) {
        send_to_char("[Standing first]\n\r", ch);
        show_remove_occupant(ch, "", "");
      }
      /* didn't specify anything to sit at/on, and we can't infer it */
      return FALSE;
    }
  }   /* end no arg1 */
  
  if (!chair) {
    /* if they're on something 
     * & and they're not going to be standing */
    if (ch->on_obj && newPos != POSITION_STANDING) {
      send_to_char("[Standing first]\n\r", ch);
      show_remove_occupant(ch, "", "");
    }
    return FALSE;
  }
  
  /* handle burrow if standing */
  if (newPos == POSITION_STANDING && stand_from_burrow(ch, FALSE)) {
    return TRUE;
  }
  
  /* if they're not already on the object */
  if (ch->on_obj != chair) {
    /* is the chair full, but the player thought it wasn't */
    if (count_plyr_list(chair->occupants, NULL) >= MAX(chair->obj_flags.value[2], 1) &&
        count_plyr_list(chair->occupants, ch) < MAX(chair->obj_flags.value[2], 1)) {
      sprintf(buf, "You start to %s but find $p already occupied.", position);
      act(buf, FALSE, ch, chair, NULL, TO_CHAR);
      sprintf(buf, "$n starts to %s but finds $p already occupied.", position);
      act(buf, TRUE, ch, chair, chair->occupants->ch, TO_ROOM);
      return TRUE;
    }
    
    /*
     * Weight checks?  Broken chairs?
     * times 10 to get weight in stones.
     * adds weight of all occupants + new guy
     */
    if (newPos != POSITION_STANDING &&
        ((GET_WEIGHT(ch) * 10) + get_plyr_list_weight(chair->occupants) > chair->obj_flags.value[0])) {
      sprintf(buf, "You are too heavy to %s on that.\n\r", position);
      send_to_char(buf, ch);
      return TRUE;
    }
  }
  
  /* Prevent ethereal people from ever being at a table/chair */
  if (is_char_ethereal(ch)) {
    send_to_char("You fall right through it.\n\r", ch);
    return TRUE;
  }
  
  /* If already in that position on that object, let them know */
  if (newPos == GET_POS(ch) && ch->on_obj == chair) {
    send_to_char("You are already doing that.\n\r", ch);
    return TRUE;
  }
  
  /* Same position as the current position */
  if (newPos == GET_POS(ch) ||
      (ch->on_obj && chair != ch->on_obj) ||
      (!ch->on_obj && GET_POS(ch) < POSITION_STANDING)) {
    if (GET_POS(ch) != POSITION_STANDING) {
      send_to_char("[Standing first]\n\r", ch);
    }
    show_remove_occupant(ch, "", "");
  }
  
  /* force the 'at' message for standing */
  if (newPos == POSITION_STANDING &&
      /* but not skimmers or wagons */
      !IS_SET(chair->obj_flags.value[1], FURN_SKIMMER) &&
      !IS_SET(chair->obj_flags.value[1], FURN_WAGON) &&
      table == NULL) {
    table = chair;
  }
  
  /* handle special case standing messages */
  if (GET_POS(ch) == POSITION_STANDING) {
    if (ch->specials.riding) {
      /* dismount messages */
      act("You swing your legs over and jump off of $N.", FALSE, ch, table, ch->specials.riding, TO_CHAR);
      act("$n swings $s legs over and jumps off of $N.", FALSE, ch, table, ch->specials.riding, TO_ROOM);
      act("$n swings $s legs over and jumps off of you.", FALSE, ch, table, ch->specials.riding, TO_VICT);
      
      sprintf(buf, "%s dismounts and %s.\n\r", MSTR(ch, name), position);
      send_to_monitor(ch, buf, MONITOR_POSITION);
      ch->specials.riding = 0;
    } /* end if riding */
    else if (affected_by_spell(ch, SPELL_LEVITATE)) {
      act("You stop floating.", FALSE, ch, chair, 0, TO_CHAR);
      act("$n stops floating.", FALSE, ch, chair, 0, TO_ROOM);
      
      sprintf(buf, "%s stops floating.\n\r", MSTR(ch, name));
      send_to_monitor(ch, buf, MONITOR_POSITION);
      affect_from_char(ch, SPELL_LEVITATE);
    } /* end if levitating */
  }
  
  /* determine if the position themselves 'at' or 'on' the object */
  if (table) {
    strcpy(at_on, "at");
  } else {
    strcpy(at_on, "on");
  }
  
  /* nothing in used_arg, find a keyword for table or chair */
  if (table) {
    find_obj_keyword(table, bitvector, ch, used_arg, sizeof(used_arg));
  } else {                    /* check chair */
    find_obj_keyword(chair, bitvector, ch, used_arg, sizeof(used_arg));
  }
  
  /* generate a message for the actor (ch) */
  sprintf(to_char, "you %s %s ~%s", position, at_on, used_arg);
  sprintf(to_room, "@ %ss %s ~%s", position, at_on, used_arg);
  
  if (!send_to_char_and_room_parsed (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION))
    return TRUE;
  
  /* Ok, messages sent, set the position and add them as an occupant */
  set_char_position(ch, newPos);
  add_occupant(chair, ch);
  
  /* success! */
  return TRUE;
}

void
get_speed_text(CHAR_DATA * ch, char *first, char *second) {
  
  if (GET_RACE(ch) == RACE_SNAKE1 || 
      GET_RACE(ch) == RACE_SNAKE2) {
    strcpy(first, "slither");
    strcpy(second, "slithers");
    return;
  } else if ((GET_RACE(ch) == RACE_WEZER || 
              GET_RACE(ch) == RACE_ROC || 
              GET_RACE(ch) == RACE_VULTURE ||
              GET_RACE(ch) == RACE_AVANGION || 
              GET_RACE(ch) == RACE_SILTFLYER ||
              GET_RACE(ch) == RACE_EAGLE || 
              GET_RACE(ch) == RACE_WYVERN) ||
             (GET_RACE_TYPE(ch) == RTYPE_AVIAN_FLYING && IS_AFFECTED(ch, CHAR_AFF_FLYING))) {
    strcpy(first, "fly");
    strcpy(second, "flies");
    return;
  }
  
  if (IS_AFFECTED(ch, CHAR_AFF_GODSPEED) && 
      affected_by_spell(ch, SPELL_FLY) &&
      GET_SPEED(ch) == SPEED_RUNNING) {
    strcpy(first, "fly");
    strcpy(second, "flies");
  } else if (IS_AFFECTED(ch, CHAR_AFF_GODSPEED) && 
             GET_SPEED(ch) == SPEED_RUNNING) {
    strcpy(first, "dash");
    strcpy(second, "dashes");
  } else if (affected_by_spell(ch, SPELL_FLY)) {
    strcpy(first, "fly");
    strcpy(second, "flies");
  } else if (ch->in_room->sector_type == (SECT_WATER_PLANE)) {
    strcpy(first, "swim");
    strcpy(second, "swims");
  } else if (affected_by_spell(ch, SPELL_LEVITATE) ||
             (affected_by_spell(ch, SPELL_FEATHER_FALL) && 
              is_char_falling(ch))) {
    strcpy(first, "float");
    strcpy(second, "floats");
  }
  
  /* Feather Fall never flies, this should only be for fly spell and natural
   * fliers -Morg */
  else if (IS_AFFECTED(ch, CHAR_AFF_FLYING) &&
           !affected_by_spell(ch, SPELL_FEATHER_FALL)) {
    strcpy(first, "fly");
    strcpy(second, "flies");
  } else if (GET_SPEED(ch) == SPEED_RUNNING) {
    strcpy(first, "run");
    strcpy(second, "runs");
  } else if (GET_SPEED(ch) == SPEED_SNEAKING) {
    strcpy(first, "stealthily move");
    strcpy(second, "stealthily moves");
  } else if (is_char_drunk(ch) > DRUNK_MEDIUM) {
    strcpy(first, "stagger");
    strcpy(second, "staggers");
  } else {
    strcpy(first, "walk");
    strcpy(second, "walks");
  }
}

void
set_wagon_tracks_in_room(OBJ_DATA * obj, struct room_data *from, struct room_data *to, int dir)
{
  if (!obj || !from || !to) {
    return;
  }
  
  /* bump the last ones out */
  memmove(&(to->tracks[1]), &(to->tracks[0]), sizeof(struct room_tracks) * 4);
  memmove(&(from->tracks[1]), &(from->tracks[0]), sizeof(struct room_tracks) * 4);
  
  to->tracks[0].type = from->tracks[0].type = TRACK_WAGON;
  to->tracks[0].time = from->tracks[0].time = time(0);
  to->tracks[0].skill = from->tracks[0].skill = 1;
  
  from->tracks[0].track.wagon.dir = dir;
  to->tracks[0].track.wagon.dir = rev_dir[dir];;
  
  if (obj) {
    from->tracks[0].track.wagon.weight = obj->obj_flags.weight;
    to->tracks[0].track.wagon.weight = obj->obj_flags.weight / 100;
  } else {
    from->tracks[0].track.wagon.weight = 0;
    to->tracks[0].track.wagon.weight = 0;
  }
}

void
set_tent_tracks_in_room(OBJ_DATA * obj, struct room_data *room) {
  if (!obj || !room) {
    return;
  }
  
  /* bump the last ones out */
  memmove(&(room->tracks[1]), &(room->tracks[0]), sizeof(struct room_tracks) * 4);
  
  room->tracks[0].type = TRACK_TENT;
  room->tracks[0].time = time(0);
  room->tracks[0].skill = 1;
  room->tracks[0].track.tent.weight = obj->obj_flags.weight / 100;
}


void
set_fire_tracks_in_room(OBJ_DATA * obj, struct room_data *room) {
  if (!obj || !room) {
    return;
  }
  
  /* bump the last ones out */
  memmove(&(room->tracks[1]), &(room->tracks[0]), sizeof(struct room_tracks) * 4);
  
  room->tracks[0].type = TRACK_FIRE;
  room->tracks[0].time = time(0);
  room->tracks[0].skill = 1;
  room->tracks[0].track.fire.number = obj->obj_flags.weight / 100;
}

void
set_combat_tracks_in_room(CHAR_DATA * ch, CHAR_DATA * victim, struct room_data *rm) {
  int type;
  int i;
  
  if (!ch || !victim || !rm) {
    return;
  }
  
  /* find a combat track, if there is one. */
  for (i = 0; i < 5; i++) {
    if (rm->tracks[i].type == TRACK_BATTLE) {
      break;
    }
  }
  
  if (i == 5) {
    /* no combat was found, so make room */
    memmove(&(rm->tracks[1]), &(rm->tracks[0]), sizeof(struct room_tracks) * 4);
    i = 0;
    rm->tracks[0].track.battle.number = 0;
  }
  
  type = TRACK_BATTLE;
  
  rm->tracks[i].type = type;
  rm->tracks[i].time = time(0);
  rm->tracks[i].skill = 1;
  
  rm->tracks[0].track.battle.number += 1;
}

void
set_tracks_sleeping(CHAR_DATA * ch, struct room_data *rm) {
  int type;
  
  if (!ch || !rm) {
    return;
  }

  /* bump the last ones out */
  memmove(&(rm->tracks[1]), &(rm->tracks[0]), sizeof(struct room_tracks) * 4);
  
  if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 2)) {
    type = TRACK_SLEEP_BLOODY;
  } else {
    type = TRACK_SLEEP;
  }
  
  rm->tracks[0].type = type;
  rm->tracks[0].time = time(0);
  rm->tracks[0].skill = 1;
  
  rm->tracks[0].track.ch.race = GET_RACE(ch);
  rm->tracks[0].track.ch.weight = GET_WEIGHT(ch);
}

void
set_tracks_in_room(CHAR_DATA * ch, struct room_data *from, struct room_data *to, int dir) {
  int type;
  
  if (!ch || !from || !to) {
    return;
  }
  
  if (IS_IMMORTAL(ch)) {
    return;
  }

  if (IS_AFFECTED(ch, CHAR_AFF_FLYING) &&
      !affected_by_spell(ch, SPELL_FEATHER_FALL)) {
    return;
  }

  // No tracks when you travel underground 
  if (GET_RACE(ch) == RACE_ANAKORE) {
    return;
  }
  
  if (is_char_ethereal(ch)) {
    return;
  }
  
  /* bump the last ones out */
  memmove(&(to->tracks[1]), &(to->tracks[0]), sizeof(struct room_tracks) * 4);
  memmove(&(from->tracks[1]), &(from->tracks[0]), sizeof(struct room_tracks) * 4);
  
  if (affected_by_spell(ch, SPELL_FLOURESCENT_FOOTSTEPS))
    switch (GET_SPEED(ch)) {
    case SPEED_RUNNING:
      type = TRACK_RUN_GLOW;
      break;
    case SPEED_WALKING:
      type = TRACK_WALK_GLOW;
      break;
    case SPEED_SNEAKING:
      type = TRACK_SNEAK_GLOW;
      break;
    default:
      type = TRACK_WALK_GLOW;
      break;
    } else {
    if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 2))
      switch (GET_SPEED(ch)) {
      case SPEED_RUNNING:
        type = TRACK_RUN_BLOODY;
        break;
      case SPEED_WALKING:
        type = TRACK_WALK_BLOODY;
        break;
      case SPEED_SNEAKING:
        type = TRACK_SNEAK_BLOODY;
        break;
      default:
        type = TRACK_WALK_BLOODY;
        break;
      } else
      switch (GET_SPEED(ch)) {
      case SPEED_RUNNING:
        type = TRACK_RUN_CLEAN;
        break;
      case SPEED_WALKING:
        type = TRACK_WALK_CLEAN;
        break;
      case SPEED_SNEAKING:
        type = TRACK_SNEAK_CLEAN;
        break;
      default:
        type = TRACK_WALK_BLOODY;
        break;
      }
  }
  
  to->tracks[0].type = from->tracks[0].type = type;
  to->tracks[0].time = from->tracks[0].time = time(0);
  to->tracks[0].skill = from->tracks[0].skill = 1;
  from->tracks[0].track.ch.dir = dir;
  from->tracks[0].track.ch.into = FALSE;
  
  to->tracks[0].track.ch.dir = rev_dir[dir];
  to->tracks[0].track.ch.into = TRUE;
  
  to->tracks[0].track.ch.race = (from->tracks[0].track.ch.race = GET_RACE(ch));
  to->tracks[0].track.ch.weight = (from->tracks[0].track.ch.weight = GET_WEIGHT(ch));
}

void
set_ch_position(CHAR_DATA * ch) {
  if (ch->on_obj) {
    act("You push off of $p and stand up.", FALSE, ch, ch->on_obj, 0, TO_CHAR);
    act("$n pushes off of $p and rises to $s feet.", TRUE, ch, ch->on_obj, 0, TO_ROOM);
    remove_occupant(ch->on_obj, ch);
  }
}

int
is_flying_char(CHAR_DATA * ch) {
  if (!ch) {
    gamelog("No ch passed to is_flying_char");
    return FLY_NO; /* 0 */
  }

  if (!IS_AFFECTED(ch, CHAR_AFF_FLYING)) {
    return FLY_NO;
  }

  if (affected_by_spell(ch, SPELL_FEATHER_FALL)) {
    return FLY_FEATHER;
  }

  if (affected_by_spell(ch, SPELL_LEVITATE)) {
    return FLY_LEVITATE;
  }

  if (affected_by_spell(ch, SPELL_FLY)) {
    return FLY_SPELL;
  }
  
  if (IS_AFFECTED(ch, CHAR_AFF_FLYING) && 
      !affected_by_spell(ch, SPELL_FEATHER_FALL) &&
      !affected_by_spell(ch, SPELL_LEVITATE) && 
      !affected_by_spell(ch, SPELL_FLY)) {
    return FLY_NATURAL;
  } else {
    gamelog("Error in is_flying_char");
    return FLY_NO;
  }
}

void
wagon_movement_noise(struct room_data *room, char *message) {
  send_to_room(message, room);
}

void
send_to_wagon(const char *message, OBJ_DATA * wagon) {
  struct room_data *inside;
  
  if (wagon->obj_flags.type != ITEM_WAGON) {
    return;
  }
  
  inside = get_room_num(wagon->obj_flags.value[2]);
  
  if (inside && wagon->in_room != inside &&
      inside && inside->people) {
    send_to_room(message, inside);
  }
}

/* Added 05/06/2000 -nessalin
   This checks, and handles all damage, for exits with thorns */
int
thorn_check(CHAR_DATA * mover, int dir) {
  int MAX_HP_COST = 10;
  int MAX_MV_COST = 40;
  int chance_escape = 0;
  int hp_cost = 0;
  int mv_cost = 0;
  char buf[MAX_STRING_LENGTH];
  
  if (!mover ||
      !(mover->in_room) ||
      !(mover->in_room->direction[dir]) ||
      (!IS_SET((mover->in_room->direction[dir]->exit_info), EX_THORN) &&
       !IS_SET((mover->in_room->direction[dir]->exit_info), EX_SPL_THORN_WALL)) ||
      is_char_ethereal(mover) || 
      IS_IMMORTAL(mover)) {
    return TRUE;
  }
  
  if (get_char_size(mover) > 40) {
    hp_cost = MAX_HP_COST;
    mv_cost = MAX_MV_COST;
  } else {
    hp_cost = (MAX_HP_COST * get_char_size(mover)) / 40;
    mv_cost = (MAX_MV_COST * get_char_size(mover)) / 40;
  }

  chance_escape = (40 - (get_char_size(mover) * 2));
  
  if (GET_GUILD(mover) == GUILD_RANGER) {
    chance_escape += 50;
  } else if (GET_RACE(mover) == RACE_DESERT_ELF) {
    chance_escape += 25;
  }
  
  switch (GET_SPEED(mover)) {
  case SPEED_RUNNING:
    chance_escape -= 15;
    hp_cost += 2;
    mv_cost += 4;
    break;
  case SPEED_WALKING:
    break;
  case SPEED_SNEAKING:
    chance_escape += 25;
    hp_cost -= 2;
    mv_cost -= 4;
    break;
  default:
    errorlog("Unknown speed in thorn_check().");
  }
  
  if (IS_AFFECTED(mover, CHAR_AFF_GODSPEED) || 
      affected_by_spell(mover, SPELL_GODSPEED)) {
    hp_cost += 2;
    mv_cost += 4;
  }
  
  if (affected_by_spell(mover, SPELL_SLOW) || 
      IS_AFFECTED(mover, CHAR_AFF_SLOW)) {
    hp_cost -= 2;
    mv_cost -= 4;
  }
  
  /* Don't let movement speed increase hp or mv cost beyond max */
  hp_cost = MIN(hp_cost, MAX_HP_COST);
  mv_cost = MIN(mv_cost, MAX_MV_COST);
  
  chance_escape = (MIN(95, chance_escape));
  
  if (number(1, 100) < chance_escape) {
    act("You carefully avoid any thorns as you move.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n carefully avoids any thorns as $e moves.", TRUE, mover, 0, 0, TO_ROOM);
    return TRUE;
  } else {
    if (GET_RACE(mover) == RACE_GHAATI) {
      act("Thorns tug at your fur ineffectually as you pass through them.", FALSE, mover, 0, 0, TO_CHAR);
      act("Thorns tug ineffectually at $n's fur as $e passes through them.", TRUE, mover,0, 0, TO_ROOM);
      return TRUE;
    }
    
    if (GET_RACE(mover) == RACE_MANTIS || 
        GET_RACE(mover) == RACE_CILOPS ||
        GET_RACE(mover) == RACE_BAHAMET) {
      act("Thorns scrape ineffectually at your carapace as you move through them.", FALSE, mover, 0, 0, TO_CHAR);
      act("Thorns scrape ineffectually at $n's carapace as $e moves through them.", TRUE, mover, 0, 0, TO_ROOM);
      return TRUE;
    }

    if ((affected_by_spell(mover, SPELL_ARMOR)) ||
        (affected_by_spell(mover, SPELL_STONE_SKIN)) ||
        (IS_AFFECTED(mover, CHAR_AFF_INVULNERABILITY)) ||
        (affected_by_spell(mover, SPELL_INVULNERABILITY))||
        (affected_by_spell(mover, SPELL_SANCTUARY)) ||
        (IS_AFFECTED(mover, CHAR_AFF_SANCTUARY))) {
      act("Thorns slide harmlessly over your magickal protection as you move through them.", FALSE, mover, 0, 0, TO_CHAR);
      act("Thorns slide harmlessly over $n's skin as $e moves through them.", TRUE, mover, 0, 0, TO_ROOM);
      return TRUE;
    }
    
    if ((GET_HIT(mover) - hp_cost) < -10) {
      act("$n is torn to shreds by the thorns.", FALSE, mover, 0, 0, TO_ROOM);
      sprintf(buf, "%s %s%s%shas been shredded to bits by a wall of thorns room #%d.",
              IS_NPC(mover) ? MSTR(mover, short_descr)
              : GET_NAME(mover), !IS_NPC(mover) ? "(" : "",
              !IS_NPC(mover) ? mover->account : "", !IS_NPC(mover) ? ") " : "",
              mover->in_room->number);
      if (mover->player.info[1]) {
        free(mover->player.info[1]);
      }
      
      gamelog(buf);
      mover->player.info[1] = strdup(buf);
      die(mover);
      return FALSE;
    } else {
      do_damage(mover, hp_cost, 0);
    }
    
    adjust_move(mover, -mv_cost);

    if (hp_cost < 3) {
      act("You are scratched as you move through some thorns.", FALSE, mover, 0, 0, TO_CHAR);
      act("$n is scratched as $e moves through some thorns.", TRUE, mover, 0, 0, TO_ROOM);
    } else if (hp_cost < 5) {
      act("You are scratched and cut as you move through some thorns.", FALSE, mover, 0, 0, TO_CHAR);
      act("$n is scratched and cut as $e moves through some thorns.", TRUE, mover, 0, 0, TO_ROOM);
    } else if (hp_cost < 7) {
      act("You are cut lightly as you move through some thorns.", FALSE, mover, 0, 0, TO_CHAR);
      act("$n is cut lightly as $e moves through some thorns.", TRUE, mover, 0, 0, TO_ROOM);
    } else if (hp_cost < 9) {
      act("You are heavily cut as you move through some thorns.", FALSE, mover, 0, 0, TO_CHAR);
      act("$n is heavily cut as $e moves through some thorns.", TRUE, mover, 0, 0, TO_ROOM);
    } else if (hp_cost < 11) {
      act("You are deeply cut as you move through some thorns.", FALSE, mover, 0, 0, TO_CHAR);
      act("$n is deeply cut as $e moves through some thorns.", TRUE, mover, 0, 0, TO_ROOM);
    } else {
      errorlog("hp_cost is greater than MAX_HP_COST in thorn_check.");
    }
  } 

  return TRUE;
}


/* Added 05/15/2000 -nessalin
   This checks, and handles all damage, for exits with wall of wind */
int
wall_of_wind_check(CHAR_DATA * mover, int dir) {
  char buf[MAX_STRING_LENGTH];
  
  if (!mover ||
      !mover->in_room ||
      !mover->in_room->direction[dir] ||
      !(IS_SET((mover->in_room->direction[dir]->exit_info), EX_SPL_WIND_WALL)) ||
      is_char_ethereal(mover) || 
      IS_IMMORTAL(mover)) {
    return TRUE;
  }
  
  act("You are pushed back by gusting winds.", FALSE, mover, 0, 0, TO_CHAR);
  sprintf(buf, "$n tries to move %s, but is pushed back by gusting winds.", dirs[dir]);
  act(buf, TRUE, mover, 0, 0, TO_ROOM);
  
  return FALSE;
}

/* Added 05/15/2000 -nessalin
   This checks, and handles all damage, for exits with blade barrier */
int
blade_barrier_check(CHAR_DATA * mover, int dir) {
  
  int MAX_HP_COST = 50;
  int MAX_MV_COST = 10;
  int hp_cost = 0;
  int mv_cost = 0;
  char buf[MAX_STRING_LENGTH];
  
  if (!mover ||
      !mover->in_room ||
      !mover->in_room->direction[dir] ||
      !(IS_SET((mover->in_room->direction[dir]->exit_info), EX_SPL_BLADE_BARRIER)) ||
      IS_IMMORTAL(mover)) {
    return TRUE;
  }
  
  if (is_char_ethereal(mover)) {
    send_to_char("You glide through the whirling blades unscathed.\n\r", mover);
    return TRUE;
  }
  
  if (get_char_size(mover) < 8) {
    hp_cost = MAX_HP_COST;
    mv_cost = MAX_MV_COST;
  } else {
    hp_cost = MAX_HP_COST - get_char_size(mover);
    mv_cost = MAX_MV_COST - get_char_size(mover);
  }
  
  switch (GET_SPEED(mover)) {
  case SPEED_RUNNING:
    hp_cost -= 2;
    mv_cost -= 4;
    break;
  case SPEED_WALKING:
    break;
  case SPEED_SNEAKING:
    hp_cost += 2;
    mv_cost += 4;
    break;
  default:
    errorlog("Unknown speed in blade_barrier_check().");
  }
  
  if ((IS_AFFECTED(mover, CHAR_AFF_GODSPEED)) || 
      (affected_by_spell(mover, SPELL_GODSPEED))) {
    hp_cost -= 2;
    mv_cost -= 4;
  }
  
  if ((affected_by_spell(mover, SPELL_SLOW)) || 
      (IS_AFFECTED(mover, CHAR_AFF_SLOW))) {
    hp_cost += 2;
    mv_cost += 4;
  }
  
  /* Make sure they take -some- damage for passing through the fire */
  hp_cost = MAX(15, hp_cost);
  mv_cost = MAX(2, mv_cost);
  
  /* Don't let movement speed increase hp or mv cost beyond max */
  hp_cost = MIN(hp_cost, MAX_HP_COST);
  mv_cost = MIN(mv_cost, MAX_MV_COST);
  
  if ((affected_by_spell(mover, SPELL_ARMOR)) || 
      (affected_by_spell(mover, SPELL_STONE_SKIN)) ||
      (IS_AFFECTED(mover, CHAR_AFF_INVULNERABILITY)) ||
      (affected_by_spell(mover, SPELL_INVULNERABILITY)) ||
      (affected_by_spell(mover, SPELL_SANCTUARY)) ||
      (IS_AFFECTED(mover, CHAR_AFF_SANCTUARY))) {
    act("You harmlessly move through some blades which bounce harmlessly off your magickal protection.", FALSE, mover, 0, 0, TO_CHAR);
    act("Whirling blades bounce harmlessly off $n's skin as $e moves through them.", TRUE, mover, 0, 0, TO_ROOM);
    return TRUE;
  }
  
  if ((GET_HIT(mover) - hp_cost) < -9) {
    act("You are dismembered by the blades as you attempt to pass through them.", FALSE, mover, 0, 0, TO_CHAR);
    sprintf(buf, "$n is dismembered by whirling blades as $e tries to leap %s.", dirs[dir]);
    act(buf, TRUE, mover, 0, 0, TO_ROOM);
    sprintf(buf, "%s %s%s%shas been sliced to bits by blade barrier room #%d.",
            IS_NPC(mover) ? MSTR(mover, short_descr) : GET_NAME(mover),
            !IS_NPC(mover) ? "(" : "", !IS_NPC(mover) ? mover->account : "",
            !IS_NPC(mover) ? ") " : "", mover->in_room->number);
    if (mover->player.info[1]) {
      free(mover->player.info[1]);
    }
    
    gamelog(buf);
    mover->player.info[1] = strdup(buf);
    die(mover);
    return FALSE;
  }
  
  if ((GET_HIT(mover) - hp_cost) < 1) {
    act("You are cut by the blades as you attempt to pass through them.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is cut by blades as $e tries to leap through them.", TRUE, mover, 0, 0, TO_ROOM);
    do_damage(mover, hp_cost, 0);
    return FALSE;
  }
  
  do_damage(mover, hp_cost, 0);
  
  adjust_move(mover, -mv_cost);
  if (hp_cost < 3) {
    act("You are lightly cut as you move through the blade barrier.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is lightly cut as $e moves through the blade barrier.", TRUE, mover, 0, 0, TO_ROOM);
  } else if (hp_cost < 5) {
    act("You are cut as you move through the blade barrier.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is cut as $e moves through the blade barrier.", TRUE, mover, 0, 0, TO_ROOM);
  } else if (hp_cost < 7) {
    act("You are heavily cut as you move through the blade barrier.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is heavily cut as $e moves through the blade barrier.", TRUE, mover, 0, 0, TO_ROOM);
  } else if (hp_cost < 9) {
    act("You are heavily lacerated as you move through the blade barrier.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is heavily lacerated as $e moves through the blade barrier.", TRUE, mover, 0, 0, TO_ROOM);
  } else if (hp_cost < 31) {
    act("You are horribly cut and lacerated as you move through the blade barrier.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is horribly cut and lacerated as $e moves through the blade barrier.", TRUE, mover, 0, 0, TO_ROOM);
  } else {
    act("You are horribly cut and lacerated as you move through the blade barrier.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is horribly cut and lacerated as $e moves through the blade barrier.", TRUE, mover, 0, 0, TO_ROOM);
  }
  return TRUE;
}

/* Added 05/06/2000 -nessalin
   This checks, and handles all damage, for exits with wall of fire */
int
wall_of_fire_check(CHAR_DATA * mover, int dir) {
  int MAX_HP_COST = 50;
  int MAX_MV_COST = 10;
  int hp_cost = 0;
  int mv_cost = 0;
  char buf[MAX_STRING_LENGTH];
  
  if (!mover ||
      !mover->in_room ||
      !mover->in_room->direction[dir] ||
      !(IS_SET((mover->in_room->direction[dir]->exit_info), EX_SPL_FIRE_WALL)) ||
      is_char_ethereal(mover) ||
      IS_IMMORTAL(mover) ||
      affected_by_spell(mover, SPELL_FIRE_ARMOR)) {
    return TRUE;
  }
      
  if (get_char_size(mover) < 8) {
    hp_cost = MAX_HP_COST;
    mv_cost = MAX_MV_COST;
  } else {
    hp_cost = MAX_HP_COST - get_char_size(mover);
    mv_cost = MAX_MV_COST - get_char_size(mover);
  }
  
  switch (GET_SPEED(mover)) {
  case SPEED_RUNNING:
    hp_cost -= 2;
    mv_cost -= 4;
    break;
  case SPEED_WALKING:
    break;
  case SPEED_SNEAKING:
    hp_cost += 2;
    mv_cost += 4;
    break;
  default:
    errorlog("Unknown speed in wall_of_fire_check().");
  }
  
  if (IS_AFFECTED(mover, CHAR_AFF_GODSPEED) || 
      affected_by_spell(mover, SPELL_GODSPEED)) {
    hp_cost -= 2;
    mv_cost -= 4;
  }
  
  if (affected_by_spell(mover, SPELL_SLOW) || 
      IS_AFFECTED(mover, CHAR_AFF_SLOW)) {
    hp_cost += 2;
    mv_cost += 4;
  }
  
  /* Make sure they take -some- damage for passing through the fire */
  hp_cost = MAX(15, hp_cost);
  mv_cost = MAX(2, mv_cost);
  
  /* Don't let movement speed increase hp or mv cost beyond max */
  hp_cost = MIN(hp_cost, MAX_HP_COST);
  mv_cost = MIN(mv_cost, MAX_MV_COST);
  
  if (affected_by_spell(mover, SPELL_ARMOR) || 
      affected_by_spell(mover, SPELL_STONE_SKIN) ||
      IS_AFFECTED(mover, CHAR_AFF_INVULNERABILITY) ||
      affected_by_spell(mover, SPELL_INVULNERABILITY) ||
      affected_by_spell(mover, SPELL_SANCTUARY) ||
      IS_AFFECTED(mover, CHAR_AFF_SANCTUARY)) {
    act("Flames lick harmlessly over your magickal protection as you move through them.", FALSE, mover, 0, 0, TO_CHAR);
    act("Flames lick harmlessly over $n's skin as $e moves through them.", TRUE, mover, 0, 0, TO_ROOM);
    return TRUE;
  }

  if ((GET_HIT(mover) - hp_cost) < -9) {
    act("You are consumed by the flames as you attempt to pass through them.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is consumed by the wall of flame as $e tries to leap through it.", TRUE, mover, 0, 0, TO_ROOM);

    sprintf(buf, "%s %s%s%shas been cremated by wall of fire room #%d.",
            IS_NPC(mover) ? MSTR(mover, short_descr) : GET_NAME(mover),
            !IS_NPC(mover) ? "(" : "", !IS_NPC(mover) ? mover->account : "",
            !IS_NPC(mover) ? ") " : "", mover->in_room->number);
    
    if (mover->player.info[1]) {
      free(mover->player.info[1]);
    }
    
    gamelog(buf);
    mover->player.info[1] = strdup(buf);
    die(mover);
    return FALSE;
  }
  
  if ((GET_HIT(mover) - hp_cost) < 1) {
    act("You are consumed by the flames as you attempt to pass through them.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is consumed by the wall of flame as $e tries to leap through it.", TRUE, mover, 0, 0, TO_ROOM);
    do_damage(mover, hp_cost, 0);
    return FALSE;
  }
  
  do_damage(mover, hp_cost, 0);
  
  adjust_move(mover, -mv_cost);
  if (hp_cost < 3) {
    act("You are lightly burned as you move through the wall of fire.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is lightly burned as $e moves through the wall of fire .", TRUE, mover, 0, 0, TO_ROOM);
  } else if (hp_cost < 5) {
    act("You are burned as you move through the wall of fire.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is burned as $e moves through the wall of fire.", TRUE, mover, 0, 0, TO_ROOM);
  } else if (hp_cost < 7) {
    act("You are heavily burned as you move through the wall of fire.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is heavily burned as $e moves through the wall of fire.", TRUE, mover, 0, 0, TO_ROOM);
  } else if (hp_cost < 9) {
    act("You are heavily charred as you move through the wall of fire.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is heavily charred as $e moves through the wall of fire.", TRUE, mover, 0, 0, TO_ROOM);
  } else if (hp_cost < 31) {
    act("You are horribly burned and charred as you move through the wall of fire.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is horribly burned and charred as $e moves through the wall of fire.", TRUE, mover, 0, 0, TO_ROOM);
  } else {
    act("You are horribly burned and charred as you move through the wall of fire.", FALSE, mover, 0, 0, TO_CHAR);
    act("$n is horribly burned and charred as $e moves through the wall of fire.", TRUE, mover, 0, 0, TO_ROOM);
  }
  return TRUE;
}

int
get_room_extra_capacity(ROOM_DATA *room) {
  int bonus = 0;
  OBJ_DATA *obj;
  
  for (obj = room->contents; obj; obj = obj->next_content) {
    if( obj->obj_flags.type == ITEM_FURNITURE) {
      bonus += obj->obj_flags.value[0];
    }
  }
  
  return bonus;
}

int
get_room_base_weight(ROOM_DATA * room) {
  char *tmp_desc;
  
  tmp_desc = find_ex_description("[cargo_size]", room->ex_description, TRUE);
  
  if (!tmp_desc) {
    return 1000;
  }
  
  /* if they supplied a number, round to the nearest 100 */
  if (is_number(tmp_desc)) {
    int amount = atoi(tmp_desc);
    
    /* round to the nearest 100 by dividing by 100 */
    amount /= 100;
    
    /* if they specified < 100, it'll be < 1, so set to 1 */
    if (amount < 1) {
      amount = 1;
    }
    
    /* multiply by 100 to get the rounded # */
    amount *= 100;
    
    return amount;
  }

  if (!(strncmp(tmp_desc, "small", 5))) {
    return 500;
  }
  
  if (!(strncmp(tmp_desc, "medium", 6))) {
    return 1000;
  }
  
  if (!(strncmp(tmp_desc, "large", 5))) {
    return 1500;
  }
  
  if (!(strncmp(tmp_desc, "warehouse", 9))) {
    return 3000;
  }
  
  if (!(strncmp(tmp_desc, "unlimited", 9))) {
    return 100000;
  }
  
  return 1000;
}

int
get_room_max_weight(ROOM_DATA * room) {
  int bonus = get_room_extra_capacity(room);
  int base = get_room_base_weight(room);
  int limit = ((base * 3)/2); // 1.5 multiplier limit off base capacity
  
  return MIN(limit, base + bonus);
}

int
get_room_weight(ROOM_DATA * room) {
  OBJ_DATA *obj;
  int weight = 0;
  
  for (obj = room->contents; obj; obj = obj->next_content) {
    weight += ((calc_object_weight_for_cargo_bay(obj)));
  }
  
  return weight;
}

int
get_used_room_weight(ROOM_DATA * room) {
  OBJ_DATA *obj;
  int weight = 0;
  
  for (obj = room->contents; obj; obj = obj->next_content) {
    weight += ((calc_object_weight(obj)));
  }

  return weight;
}

int
cmd_simple_obj_move(OBJ_DATA * obj_leader,  /* obj that is moving */
                    int cmd,                /* direction they're going in */
                    int following,          /* Whether or not they're followed */
                    int speed,              /* how fast is it being dragged */
                    const char *argument) { /* argument for cmd_move */
  char speed1[256], speed2[256];
  char buf[MAX_STRING_LENGTH];
  ROOM_DATA *was_in;
  struct follow_type *k, *next_dude;
  struct object_follow_type *obj_f;
  
  was_in = obj_leader->in_room;
  
  /* obj moving messages */
  if (obj_leader->wagon_master) {
    sprintf(buf, "%s following %s to the %s.", OSTR(obj_leader, short_descr),
            OSTR(obj_leader->wagon_master, short_descr), dirs[cmd]);
    gamelog(buf);
    sprintf(buf, "%s is drug to the %s by %s.\n\r", OSTR(obj_leader->wagon_master, short_descr),
            dirs[cmd], OSTR(obj_leader, short_descr));
    send_to_room(buf, obj_leader->in_room);
  }
  
  
  obj_from_room(obj_leader);
  
  if ((was_in->direction[cmd]) && (was_in->direction[cmd]->to_room)) {
    obj_to_room(obj_leader, was_in->direction[cmd]->to_room);
  } else {
    obj_to_room(obj_leader, was_in);
  }
  
  if (obj_leader->object_followers) {
    for (obj_f = obj_leader->object_followers; obj_f; obj_f = obj_f->next) {
      cmd_simple_obj_move(obj_f->object_follower, cmd, following, speed, argument);
    }
  }
  
  if (obj_leader->followers) {
    for (k = obj_leader->followers; k; k = next_dude) {
      next_dude = k->next;
      if ((was_in == k->follower->in_room) && (GET_POS(k->follower) >= POSITION_STANDING)) {
        if (GET_SPEED(k->follower) != speed) {
          GET_SPEED(k->follower) = speed;
        }
        get_speed_text(k->follower, speed1, speed2);
        sprintf(buf, "You follow $p and %s %s.", speed1, dirs[cmd]);
        act(buf, FALSE, k->follower, obj_leader, 0, TO_CHAR);
        cmd++;
        send_to_char("\n\r", k->follower);
        cmd_move(k->follower, argument, cmd, 0);
        cmd--;
      }
    }
  }

  return TRUE;
}

bool
wagon_back_up(OBJ_DATA * wagon, int dir) {
  if (!wagon->obj_flags.temp) {
    return FALSE;
  }
  
  if ((wagon->obj_flags.temp - 1) == rev_dir[dir]) {
    return TRUE;
  } else {
    return FALSE;
  }
}

int
wagon_damage_change(int dam, int old_dam, int max_dam) {
  int new_perc;
  int old_perc;
  
  new_perc = ((dam * 10) / max_dam);
  old_perc = ((old_dam * 10) / max_dam);
  
  if (new_perc == old_perc) {
    return 0;
  }
  
  /* 70% damaged, and can no longer move */
  if ((new_perc >= 7) && (old_perc < 7)) {
    return 4;
  }
  
  /* 60% damaged */
  if ((new_perc >= 6) && (old_perc < 6)) {
    return 3;
  }
  
  /* 40% damaged */
  if ((new_perc >= 4) && (old_perc < 4)) {
    return 2;
  }
  
  /* 20% damaged */
  if ((new_perc >= 2) && (old_perc < 2)) {
    return 1;
  }
  
  return 0;
}

void
damage_wagon_messages(OBJ_DATA * wagon, int change, int spell) {
  char pil_msg[MAX_STRING_LENGTH];
  
  switch (spell) {
  default:                   /* wagon movement */
    switch (change) {
    case 2:                /* 40% damage */
      act("A loud crack emenates from $p.", FALSE, 0, wagon, 0, TO_ROOM);
      sprintf(pil_msg, "A loud, jarring crack comes from %s.", OSTR(wagon, short_descr));
      send_to_wagon(pil_msg, wagon);
      break;
    case 3:                /* 60% damage */
      act("A loud cracking noise comes from $p, and it rolls less smoothly.", FALSE, 0, wagon, 0, TO_ROOM);
      sprintf(pil_msg, "A loud cracking noise comes from %s, and the ride feels noticeably rougher.", OSTR(wagon, short_descr));
      send_to_wagon(pil_msg, wagon);
      break;
    case 4:                /* 70% damage */
      act("The sounds of splintering wood are emitted from $d, and it comes to a stop.", FALSE, 0, wagon, 0, TO_ROOM);
      sprintf(pil_msg, "You hear the splintering of wood, and %s comes to a halt.", OSTR(wagon, short_descr));
      send_to_wagon(pil_msg, wagon);
      break;
    case 1:                /* 20% damage */
    default:
      sprintf(pil_msg, "You hear a crack from outside, and %s shudders slightly.\n\r", OSTR(wagon, short_descr));
      send_to_wagon(pil_msg, wagon);
      act("You hear a crack from $p, and it shudders slightly.", FALSE, 0, wagon, 0, TO_ROOM);
      break;
    }
    break;
  }
}

void
damage_wagon(OBJ_DATA * wagon, int spell)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    int maxdam;
    int olddam;
    int change = 0;
    int dam = 0;
    int perc = 0;
    int from_dir;
    char *wag_dam_type[] = {
      "movement",
      "too many beasts",
      "unknown"
    };

    ROOM_DATA *from_room = NULL;
    
    if (!wagon->obj_flags.value[1]) {
      return;
    }
    
    maxdam = calc_wagon_max_damage(wagon);
    
    if (!wagon->obj_flags.temp) {
      return;
    }
    
    from_dir = wagon->obj_flags.temp;
    from_dir = MAX(0, (from_dir - 1));
    
    if (wagon->in_room->direction[rev_dir[from_dir]]) {
      from_room = wagon->in_room->direction[rev_dir[from_dir]]->to_room;
    } else {                  /* Could be backing up */
      if (wagon->in_room->direction[from_dir]) {
        from_room = wagon->in_room->direction[from_dir]->to_room;
      } else {
        errorlogf("in damage_wagon:  %s came from direction "
                  "%s, but there is no link back %s in room %d.", OSTR(wagon, short_descr),
                  dirs[from_dir], dirs[rev_dir[from_dir]], wagon->in_room->number);
        return;
      }
    }
    
    if (!from_room) {
      errorlogf("in damage_wagon: from room does not exist:"
                " %s came from direction %s, but there is no link back %s in room %d.",
                OSTR(wagon, short_descr), dirs[from_dir], dirs[rev_dir[from_dir]],
                wagon->in_room->number);
      return;
    }
    
    switch (from_room->sector_type) {
    case SECT_INSIDE:
    case SECT_CITY:
    case SECT_AIR:
      dam = 0;
      perc = 0;
      break;
    case SECT_ROAD:
      dam = 1;
      perc = 1;
      break;
    case SECT_FIELD:
    case SECT_COTTONFIELD:
    case SECT_HILLS:
    case SECT_NILAZ_PLANE:
    case SECT_FIRE_PLANE:
    case SECT_WATER_PLANE:
    case SECT_EARTH_PLANE:
    case SECT_SHADOW_PLANE:
    case SECT_AIR_PLANE:
    case SECT_LIGHTNING_PLANE:
      dam = 1;
      perc = 30;
      break;
    case SECT_DESERT:
    case SECT_LIGHT_FOREST:
      dam = 1;
      perc = 50;
      break;
    case SECT_MOUNTAIN:    /* very bad to do */
      dam = 4;
      perc = 500;
      break;
    case SECT_SILT:
    case SECT_SALT_FLATS:
    case SECT_HEAVY_FOREST:
      dam = 1;
      perc = 60;
      break;
    case SECT_THORNLANDS:
      dam = 2;
      perc = 80;
      break;
    case SECT_SOUTHERN_CAVERN:
    case SECT_CAVERN:
    case SECT_RUINS:
      dam = 2;
      perc = 20;
      break;
    default:
      errorlogf("in damage_wagon: Room %d does not have a valid sector type.",
                from_room->number);
      dam = 0;
      perc = 0;
      break;
    }                       /* End switch (sector type) */
    
    /* Check percent to see if wagon is damaged */
    if (dam && (number(0, 1000) < perc)) {
      olddam = wagon->obj_flags.value[4];
      
      wagon->obj_flags.value[4] = MIN(maxdam, (wagon->obj_flags.value[4] + dam));
      
      sprintf(buf1, "from room (unknown) to ");
      
      sprintf(buf, "%s damaged %d pts due to %s(%sroom %d), and is at %d(%d) pts.",
              OSTR(wagon, short_descr), dam, wag_dam_type[spell], (from_room) ? buf1 : "",
              wagon->in_room->number, wagon->obj_flags.value[4], maxdam);
      gamelog(buf);
      
      /* insert room/wagon messages */
      change = wagon_damage_change(wagon->obj_flags.value[4], olddam, maxdam);
      damage_wagon_messages(wagon, change, spell);
      
      // remove after wagon repairing shops are implemented -- Azroen
#ifdef REM_WAGON_DAMAGE
      wagon->obj_flags.value[4] = 0;
      sprintf(buf, "Resetting %s to 0(%d) damage until wagon repair is working.",
              OSTR(wagon, short_descr), maxdam);
      gamelog(buf);
#endif
      
    }
    
    return;
}

bool is_cargo_bay(ROOM_DATA *rm);

bool does_like(struct char_data * ch, struct char_data * mem);

/* Possibly the farthest thing from a 'simple' move procedure */

int
cmd_simple_move(CHAR_DATA * ch, /* The person who is moving        */
                int cmd,        /* The direction they're moving in */
                int following,  /* Wether or not they're following  */
                char *how)      /* How they are moving */ {
  char tmp[256], buf[256], speed1[256], speed[256], message[MAX_STRING_LENGTH];
  struct room_data *was_in;
  int sector;
  int need_movement, chance = 0;
  int percent, j, sneak = 0;
  int flying = 0;
  int extra_lag = 0;
  int sneak_roll = number(1, 101);
  CHAR_DATA *tch, *next_tch, *mover, *rider;
  ROOM_DATA *to_room;
  bool fits;
  struct char_fall_data *curr_falling;
  
  /* 7/9/99 -Morg Falling block */
  for (curr_falling = char_fall_list; 
       curr_falling; 
       curr_falling = curr_falling->next) {
    if (curr_falling->falling == ch) {
      
      if (IS_SET(ch->in_room->room_flags, RFL_NO_CLIMB)) {
        send_to_char("You can't find anything to break your fall.\n\r", ch);
        return 0;
      }
      
      if ((has_skill(ch, SKILL_CLIMB) &&
           number(50, 150) < ch->skills[SKILL_CLIMB]->learned) ||
          (curr_falling->num_rooms * 10) + number(1, 25) <= GET_AGL(ch)) {
        send_to_char("You scramble for purchase, but find nothing.\n\r", ch);
        act("$n scrambles for purchase as $e falls.\n\r", FALSE, ch, NULL, NULL, TO_ROOM);
        
        if (curr_falling->num_rooms > 1) {
          curr_falling->num_rooms--;
        }
        
        if (generic_damage(ch, 15, 0, 0, 15)) {
          WAIT_STATE(ch, 1);
          return 0;
        } else {
          send_to_char("You hit your head and your vision goes black.\n\r", ch);
          die(ch);
          return -1;
        }
      } else {
        send_to_char("You scramble for purchase, and stop the fall.\n\r", ch);
        act("$n scrambles for purchase, and stops $s fall.\n\r", FALSE, ch, NULL, NULL, TO_ROOM);
        MUD_SET_BIT(ch->specials.affected_by, CHAR_AFF_CLIMBING);
        remove_char_from_falling(ch);
        
        if (generic_damage(ch, 15, 0, 0, 15)) {       /* they lived */
          WAIT_STATE(ch, 3);
          return 0;
        } else {
          send_to_char("You hit your head and your vision goes black.\n\r", ch);
          die(ch);
          return -1;
        }
      }
      
      return 0;
    }
  }
  
  /* who's doing the actual moving? */
  mover = ch->specials.riding ? ch->specials.riding : ch;
  to_room = mover->in_room->direction[cmd]->to_room;
  
  /* who is the rider? */
  for (rider = mover->in_room->people; rider; rider = rider->next_in_room) {
    if (rider->specials.riding == mover) {
      break;
    }
  }
  
  /* if there's no rider, then the person who instigated the move is
   * technically the rider (of themselves) */
  if (!rider) {
    rider = ch;
  }
  
  /* If the mount isn't in the same room as the rider, reset their pointer */
  if (rider->in_room && 
      mover->in_room && 
      (rider->in_room != mover->in_room)) {
    rider->specials.riding = (CHAR_DATA *) 0;
    mover = ch;
  }
  
  /* If the mount can't move, you aren't going anywhere */
  if (mover && 
      (mover == ch->specials.riding) && 
      IS_SET(mover->specials.act, CFL_FROZEN)) {
    sprintf(message, "%s refuses to move.\n\r", MSTR(mover, short_descr));
    send_to_char(message, ch);
    return 0;
  }
  
  /* plants can't walk, this is repeated from cmd_move because your mount
   * might be a GESRA */
  if (GET_RACE(mover) == RACE_GESRA) {
    return 0;
  }
  
  /* Neither can paralyzed people (or mounts) */
  if (is_paralyzed(mover)) {
    return 0;
  }
  
  /* how are they moving? */
  get_speed_text(mover, speed1, speed);
  
  /* check to see if there's enough room to enter the area */
  if (is_cargo_bay(to_room) && ch->lifting) {
    int obj_weight = calc_object_weight_for_cargo_bay(ch->lifting);
    int weight = get_room_weight(to_room);
    int rm_max_weight = get_room_max_weight(to_room);
    
    if( weight + obj_weight > rm_max_weight ) {
      sprintf( buf, "The %s you are dragging is too big to fit, drop it first.\n\r",
               smash_article(format_obj_to_char( ch->lifting, ch, 1)));
      send_to_char( buf, ch );
      return 0;
    }
  }
  
  /* only one person in a tunnel at a time / no brutes  -  Kel */
  if (!handle_tunnel(to_room, ch)) {
    return 0;
  }
  
  /* mount checks */
  if (ch->specials.riding == mover) {
    if (hands_free(ch) < 1 && (get_skill_percentage(ch, SKILL_RIDE) < 56)) {
      send_to_char("You'll need at least one hand free for the reins.\n\r", ch);
      return 0;
    }
    
    if (mover->specials.fighting) { /*  Kel  */
      if (number(1, 100) + 35 > get_skill_percentage(ch, SKILL_RIDE)) {
        send_to_char("Your mount cannot break away from combat!\n\r", ch);
        return (0);
      }
    }
    
    if (!init_learned_skill(ch, SKILL_RIDE, 10)) {
      cprintf(ch, "You have no idea how to ride.\n\r");
      return FALSE;
    }
    
    int result = ride_check(ch);
    // let's do a ride check
    switch (result) {
    case RIDE_CHECK_FAILURE:
      if (GET_GUILD(ch) == GUILD_MERCHANT) {
        gain_skill(ch, SKILL_RIDE, number(4, 6));
      } else {
        gain_skill(ch, SKILL_RIDE, number(2, 4));
      }
      
      // 1-4 extra lag cycles for failure
      extra_lag = number(0, 4);
      // if positive (4 out of 5 times)
      if( extra_lag ) {
        // let them know they've been slowed down
        switch( number(0,4)) {
        case 0:
        case 1:
          act("$N slowly responds to your commands.", FALSE, ch, 0, mover, TO_CHAR);
          break;
        case 2:
          act("$N veers towards a nearby plant, forcing you to urge $M onward.", FALSE, ch, 0, mover, TO_CHAR);
          break;
        case 3:
        case 4:
          act("$N slows slightly, forcing you to urge $M onward.", FALSE, ch, 0, mover, TO_CHAR);
          break;
        }
        break;
      }
      // 1 in 5 on failure, give them a second ride check
      else if( ride_check(ch) > 0 ) {
        // pass, it simply doesn't move
        act("$N refuses to move.", FALSE, ch, 0, mover, TO_CHAR);
        return 0;
      }
      // intentionally not breaking here, failure became critical
    case RIDE_CHECK_FAILURE_CRITICAL:
      // since a failure can become a critical fail
      if (result == RIDE_CHECK_FAILURE_CRITICAL) {
        if (GET_GUILD(ch) == GUILD_MERCHANT) {
          gain_skill(ch, SKILL_RIDE, number(2, 3));
        } else {
          gain_skill(ch, SKILL_RIDE, number(1, 2));
        }
      }
      
      ch->specials.riding = NULL;
      set_char_position(ch, POSITION_SITTING);
      WAIT_STATE(ch, PULSE_VIOLENCE * 2);
      
      if (!generic_damage(ch, number(1,10), 0, 0, number(2, 20))) {
        act("You fall off $N and die.", FALSE, ch, 0, mover, TO_CHAR);
        act("$n falls off $N and dies.", FALSE, ch, 0, mover, TO_ROOM);
      } else {
        act("You fall off $N.", FALSE, ch, 0, mover, TO_CHAR);
        act("$n falls off $N.", FALSE, ch, 0, mover, TO_ROOM);
      }
      
      // 1 in 4 chance of the mount going on the way you wanted to go
      if (!number(0, 3)) {
        ch = mover;
        rider = ch;
        // kill the 'how'
        if (how) {
          how[0] = '\0';
        }
      } else {
        return FALSE;
      }
      break;
    case RIDE_CHECK_SUCCESS_PARTIAL: // partial success
      if (GET_GUILD(ch) == GUILD_MERCHANT) {
        gain_skill(ch, SKILL_RIDE, number(2, 3));
      } else {
        gain_skill(ch, SKILL_RIDE, number(1, 2));
      }
      
      extra_lag = number(1, 2);
      switch( number(0,4)) {
      case 0:
      case 1:
        act("$N slowly responds to your commands.", FALSE, ch, 0, mover, TO_CHAR);
        break;
      case 2:
        act("$N veers towards a nearby plant, forcing you to urge $M onward.", FALSE, ch, 0, mover, TO_CHAR);
        break;
      case 3:
      case 4:
        act("$N slows slightly, forcing you to urge $M onward.", FALSE, ch, 0, mover, TO_CHAR);
        break;
      }
      break;
    case RIDE_CHECK_SUCCESS:
      break;
    case RIDE_CHECK_SUCCESS_CRITICAL:
      extra_lag = -1;
      break;
    }
  }
  
  /* how much stamina do they need? */
  need_movement = stamina_loss(mover);
  
  if (ch->lifting) {
    if (GET_OBJ_WEIGHT(ch->lifting) / 2 > get_lifting_capacity(ch->lifting->lifted_by)) {
      act("You try and drag $p, but it's too heavy.", 0, ch, ch->lifting, 0, TO_CHAR);
      act("$n strains, trying to drag $p, but it doesn't move.", 0, ch, ch->lifting, 0, TO_ROOM);
      return FALSE;
    } else if (GET_OBJ_WEIGHT(ch->lifting) > get_lifting_capacity(ch->lifting->lifted_by)) {
      /* if it's not off the ground, add drag to movement cost */
      /* ( half obj weight - group's capacity ) / # people lifting */
      need_movement += MAX(0, (GET_OBJ_WEIGHT(ch->lifting)
                               - get_lifting_capacity(ch->lifting->lifted_by))
                           / MIN(1, count_plyr_list(ch->lifting->lifted_by, NULL)));
    }
  }
  
  /* if the character is a led mount */
  if (IS_SET(ch->specials.act, CFL_MOUNT) && (mover == ch) && ch->master) {
    need_movement = MAX(0, need_movement - 1);
  }
  
  /* not enough stamina? */
  if ((GET_MOVE(mover) < need_movement)) {
    if (IS_NPC(mover) && (mover == ch->specials.riding)) {
      send_to_char("Your mount is too tired to proceed.\n\r", ch);
      send_to_char("You are too tired to proceed.\n\r", mover);
      return FALSE;
    }
    
    if (!following) {
      send_to_char("You are too exhausted.\n\r", ch);
    } else {
      send_to_char("You are too exhausted to follow.\n\r", ch);
      act("$N is exhausted and stops moving.", FALSE, ch, 0, ch, TO_ROOM);
      stop_follower(ch);
      /* check if they're following a wagon team leader */
    }
    return FALSE;
  }
  
  /* get em out of the queue */
  if (following && is_in_command_q(ch)) {
    act("You can't maintain your concentration while moving!", FALSE, ch, 0, 0, TO_CHAR);
    rem_from_command_q(ch);
  }
  
  /* stop people who are writing on boards from moving while following */
  if (following && ch->desc && ch->desc->str) {
    return FALSE;
  }
  
  if (affected_by_spell(ch, SPELL_CURSE)) {
    if (!number(0, 3)) {
      act("You trip over your feet!", FALSE, ch, 0, 0, TO_CHAR);
      act("$n trips over $s feet and falls to the ground.", FALSE, ch, 0, 0, TO_ROOM);
      set_char_position(ch, POSITION_SITTING);
      adjust_move(ch, -1);
      return FALSE;
    }
  }
  
  /* Check to see if they stumble and fall, if intoxicated  */
  if (!ch->specials.riding) {
    switch (is_char_drunk(ch)) {
    case 0:
      chance = 0;
      break;
    case DRUNK_LIGHT:
      chance = 0;
      break;
    case DRUNK_MEDIUM:
      chance = 5;
      break;
    case DRUNK_HEAVY:
      chance = 15;
      break;
    case DRUNK_SMASHED:
      chance = 30;
      break;
    case DRUNK_DEAD:
      chance = 55;
      break;
    }

    /* Looks like they have fallen, if they miss this check */
    if (chance > number(1, 100)) {
      if (GET_SPEED(ch) == SPEED_RUNNING) {
        act("$n attempts to run, but trips over $s feet.", FALSE, ch, 0, 0, TO_ROOM);
        act("You attempt to run, but stumble and fall on your face.", FALSE, ch, 0, 0, TO_CHAR);
      } else {
        act("$n attempts to walk, but trips over $s feet.", FALSE, ch, 0, 0, TO_ROOM);
        act("You attempt to walk, but trip and fall on your face.", FALSE, ch, 0, 0, TO_CHAR);
      }
      set_char_position(ch, POSITION_SITTING);
      adjust_move(ch, -1);
      return FALSE;
    }                       /* End chance check */
  }
  
  /* End intox code kludge */
  if (ch->desc && (ch->specials.act_wait > 50) && ch->master && following) {
    act("$N is too fast for you, so you lag behind.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$N is unable to keep up with you, and lags behind.", FALSE, ch->master, 0, ch, TO_CHAR);
    stop_follower(ch);
    return FALSE;
  }
  
  if (IS_SET(to_room->room_flags, RFL_FALLING) &&
      (!IS_SET(ch->in_room->room_flags, RFL_FALLING))) {
    struct follow_type *curr_follower, *next_follower;
    for (curr_follower = ch->followers; curr_follower; curr_follower = next_follower) {
      next_follower = curr_follower->next;
      if (!IS_IMMORTAL(curr_follower->follower) &&
          is_flying_char(curr_follower->follower) == FLY_NO) {
        if (ch->in_room->sector_type == SECT_FIELD||
            ch->in_room->sector_type == SECT_DESERT ||
            ch->in_room->sector_type == SECT_HILLS ||
            ch->in_room->sector_type == SECT_MOUNTAIN ||
            ch->in_room->sector_type == SECT_GRASSLANDS) {
          sprintf(buf, "You notice the %s give way to a hazardous fall, and stop following %s.\n\r",
                  room_sector[ch->in_room->sector_type].name,
                  MSTR(ch, short_descr));
          send_to_char(buf,  curr_follower->follower);
        } else {
          act("You notice $N moving towards a hazardous fall, and stop.", FALSE, curr_follower->follower, 0, ch, TO_CHAR);
        }
        stop_follower(curr_follower->follower);
      }
    }
  }

  if (ch->specials.subduing && (cmd == DIR_UP) &&
      (IS_SET(to_room->room_flags, RFL_FALLING))) {
    send_to_char("But you couldn't drag someone up there.\n\r", ch);
    return FALSE;
  }
 
  if (IS_SET(to_room->room_flags, RFL_NO_MOUNT) &&
      ((mover == ch->specials.riding) || (IS_SET(ch->specials.act, CFL_MOUNT)))) {
    send_to_char("You can't take a mount there.\n\r", ch);
    return FALSE;
  }
  
  if (IS_TUNNEL(ch, cmd)) {   /*  EX_TUNN_*  - Kel  */
    fits = check_tunnel_size(ch, cmd);
    if (!(fits)) {
      return FALSE;
    }
  }
  
  /* Begin: Wall of Wind stuff added 05/15/2000 -nessalin */
  if (!wall_of_wind_check(mover, cmd)) {
    return FALSE;
  }

  if (ch != mover) {
    if (!wall_of_wind_check(ch, cmd)) {
      return FALSE;
    }
  }
  
  /* End: Wall of Wind stuff added 05/15/2000 -nessalin */
  
  /* Begin: THORN stuff added 05/06/2000 nessalin */
  if (!thorn_check(mover, cmd)) {
    return FALSE;
  }

  if (ch != mover) {
    if (!thorn_check(ch, cmd)) {
      return FALSE;
    }
  }
  
  if (ch->specials.subduing) {
    thorn_check(ch->specials.subduing, cmd);
  }
  
  /* End: THORN stuff added 05/06/2000 nessalin */

  /* Begin: Wall of Fire stuff added 05/06/2000 -nessalin */
  if (!wall_of_fire_check(mover, cmd)) {
    return FALSE;
  }

  if (ch != mover) {
    if (!wall_of_fire_check(ch, cmd)) {
      return FALSE;
    }
  }
  
  if (ch->specials.subduing) {
    wall_of_fire_check(ch->specials.subduing, cmd);
  }
  /* End: Wall of Fire stuff added 05/06/2000 -nessalin */
  
  /* Begin: Blade Barrier stuff added 05/15/2000 -nessalin */
  if (!blade_barrier_check(mover, cmd)) {
    return FALSE;
  }

  if (ch != mover) {
    if (!blade_barrier_check(ch, cmd)) {
      return FALSE;
    }
  }
  
  if (ch->specials.subduing) {
    blade_barrier_check(ch->specials.subduing, cmd);
  }
  /* End: Blade Barrier stuff added 05/15/2000 -nessalin */
  
  /* get and hold onto their flying status */
  flying = is_flying_char(mover);
  
  /* if the to_room is a falling room */
  if (IS_SET(to_room->room_flags, RFL_FALLING) &&
      (!flying || /* if you're not flying */
       (flying == FLY_LEVITATE && cmd == DIR_UP) || /* levitate only has to do climb check if moving up */
       (flying == FLY_FEATHER)) &&         /* feather fall always has to check */
      !IS_IMMORTAL(ch)) {        /* immortals can't fall */
    /* If they are trying to fly into a no_flying room */
    if (flying && IS_SET(to_room->room_flags, RFL_NO_FLYING)) {
      cprintf(ch, "You fight against severe winds and buffeting.\n\r");
      return TRUE;
    }
    
    if (has_skill(mover, SKILL_CLIMB)) {
      percent = (mover->skills[SKILL_CLIMB]->learned);
      qroomlogf(QUIET_CALC, ch->in_room, "Climb Learned = %d", percent);
    } else {
      percent = ((ch->specials.riding == mover) ? 0 : 10);
      qroomlogf(QUIET_CALC, ch->in_room, "Climb Base Chance = %d", percent);
      
      // only need to apply skill bonus if they don't have the skill
      // if they ahve the skill, just wearing it applies the bonus.
      for (j = 0; j < MAX_WEAR; j++) {
        if (mover->equipment[j]) {
          percent += affect_bonus(mover->equipment[j], CHAR_APPLY_SKILL_CLIMB);
        }
      }
      qroomlogf(QUIET_CALC, ch->in_room, "w/equipment = %d", percent);
    }
    
    // being more agile makes it easier to climb
    percent += agl_app[GET_AGL(mover)].reaction;
    qroomlogf(QUIET_CALC, ch->in_room, "+ agility reaction = %d", percent);
    
    // being stronger makes it easier to climb
    percent += str_app[GET_STR(mover)].todam;
    qroomlogf(QUIET_CALC, ch->in_room, "+ strength todam = %d", percent);
    
    OBJ_DATA *climb_tool = get_tool(mover, TOOL_CLIMB);

    if (climb_tool != NULL) {
      percent += climb_tool->obj_flags.value[1];
      qroomlogf(QUIET_CALC, ch->in_room, "+ climb_tool = %d", percent);
    }
    
    int num_hands_free = hands_free(ch);
    if (climb_tool != NULL) {
      num_hands_free++;
    }

    if (num_hands_free > 0) {
      percent += num_hands_free * 3;
      if (num_hands_free == 1) {
        qroomlogf(QUIET_CALC, ch->in_room, "+ hands_free(1) = %d",  percent);
      } else {
        qroomlogf(QUIET_CALC, ch->in_room, "+ hands_free(2) = %d", percent);
      }
    }

    /* levitate makes climbing easier */
    if (flying == FLY_LEVITATE) {
      percent += 50;
      qroomlogf(QUIET_CALC, ch->in_room, "+ levitate = %d", percent);
    }
    
    /* Cheesy workaround so people can put NOSAVE on when
     * they want to fall. -Sanvean */
    if (IS_SET(ch->specials.nosave, NOSAVE_CLIMB)) {
      percent = 0;
      qroomlogf(QUIET_CALC, ch->in_room, "NOSAVE climb = %d", percent);
    }
    
    if (cmd == DIR_UP && 
        mover == ch->specials.riding &&
        percent <= 0) {
      send_to_char("Climb up while on a mount?\n\r", ch);
      return FALSE;
    }

    /* can't ride a mount up into the air */
    if (mover == ch->specials.riding && 
        cmd == DIR_UP && !is_flying_char(mover) &&
        IS_SET(ch->in_room->direction[cmd]->to_room->room_flags, RFL_NO_CLIMB)) {
      send_to_char("Your mount can't fly.\n\r", ch);
      return FALSE;
    }
    
    int roll = number(1,100);
    qroomlogf(QUIET_CALC, ch->in_room, "Rolled a %d", roll);
    /* End my workaround. Will add a check in skillgain for
     * nosave as well, so people can't use this to powergame */
    if (percent >= roll &&
        !IS_SET(to_room->room_flags, RFL_NO_CLIMB)) {
      /* if they're sneaking, give a chance to notice */
      if (GET_SPEED(ch) == SPEED_SNEAKING || IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
        for (tch = ch->in_room->people; tch; tch = next_tch) {
          next_tch = tch->next_in_room;
          if (ch != tch) {
            sneak = succeed_sneak(ch, tch, sneak_roll);
            if (!sneak) {
              sprintf(buf, "$n carefully climbs %s.", dirs[cmd]);
              act(buf, FALSE, ch, 0, tch, TO_VICT);
            }
          }
        }
      } else {
        sprintf(buf, "$n carefully climbs %s.", dirs[cmd]);
        act(buf, TRUE, ch, 0, 0, TO_ROOM);
      }
      
      qroomlogf(QUIET_CALC, ch->in_room, "Skill (%d) >= Roll (%d)  CLIMB SUCCESS!!!", percent, roll);
      if (climb_tool != NULL) {
        sprintf(buf, "Using %s, you carefully climb %s.", 
                OSTR(climb_tool, short_descr), 
                dirs[cmd]);
      } else {
        if (num_hands_free == 0) {
          if (ch->equipment[ETWO]) {
            sprintf(buf, "Struggling with %s in your hands, you carefully climb %s.",
                    OSTR(ch->equipment[ETWO], short_descr),
                    dirs[cmd]);
          } else {
            sprintf(buf, "Struggling with %s and %s in your hands, you carefully climb %s.",
                    OSTR(ch->equipment[EP], short_descr),
                    OSTR(ch->equipment[ES], short_descr),
                    dirs[cmd]);
          }         
        } else {
          sprintf(buf, "You carefully climb %s.", dirs[cmd]);
        }
      }

      act(buf, TRUE, ch, 0, 0, TO_CHAR);
      
      if (!IS_SET(ch->specials.affected_by, CHAR_AFF_CLIMBING)) {
        MUD_SET_BIT(ch->specials.affected_by, CHAR_AFF_CLIMBING);
      }
    } else {
      if (IS_SET(to_room->room_flags, RFL_NO_CLIMB)) {
        qroomlogf(QUIET_CALC, ch->in_room, "NO_CLIMB!!!");
        if (IS_SET(ch->in_room->room_flags, RFL_NO_CLIMB)) {
          act("$n falls...", FALSE, ch, 0, 0, TO_ROOM);
          act("You fall...", TRUE, ch, 0, 0, TO_CHAR);
        } else {
          sprintf(buf, "$n tries to go %s and falls.", dirs[cmd]);
          act(buf, TRUE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "You try to go %s but fall.", dirs[cmd]);
          act(buf, TRUE, ch, 0, 0, TO_CHAR);
        }
      } else {
        qroomlogf(QUIET_CALC, ch->in_room, "Skill (%d) < Roll (%d)  CLIMB FAIL!!!", percent, roll);
        act("$n tries to climb, but slips.", TRUE, ch, 0, 0, TO_ROOM);
        if (IS_SET(ch->specials.nosave, NOSAVE_CLIMB)) {
          act("You try to climb, but slip because you have your nosave flag on.\n\r", TRUE, ch, 0, 0, TO_CHAR);
        } else {
          act("You try to climb, but slip.", TRUE, ch, 0, 0, TO_CHAR);
        }
        
        /* Can't remove this as otherwise they can't let go if they
           get tired out -Morg 3/19/08 */
        if (cmd != DIR_UP &&
            IS_SET(ch->specials.affected_by, CHAR_AFF_CLIMBING)) {
          REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_CLIMBING);
        }
        
        if ((has_skill(ch, SKILL_CLIMB)) &&
            (!IS_SET(ch->specials.nosave, NOSAVE_CLIMB))) {
          gain_skill(ch, SKILL_CLIMB, 3);
        }
        
        adjust_move(ch, -5);
        
        /* no damage if flying (levitate/feather fall) -Morg */
        if (!number(0, 100) &&
            (!is_flying_char(ch) && !is_char_ethereal(ch))) {
          if (damage_absorbed_by_invulnerability(ch)) {
            // no-op
          } else if (generic_damage(ch, 15, 0, 0, 80 + number(1, 20) + (number(0, 1) ? -1 : 1) * number(1, 25))) {
            act("As $n falls, $e lands on $s neck!", TRUE, ch, 0, 0, TO_ROOM);
            cprintf(ch, "As you fall, you land on your neck!\n\r");
          } else {
            act("As $n falls, $e lands on $s neck, breaking it!", TRUE, ch, 0, 0, TO_ROOM);
            cprintf(ch,"As you fall, you land on your neck, breaking it!\n\r");
            die(ch);
            return (-1);
          }
        }
      }
      
      if (cmd == DIR_UP) {
        return FALSE;
      }
    }
  } else if (rider->specials.subduing) {
    /* Entering a room which is not falling OR char flying */
    if (IS_SET(rider->specials.affected_by, CHAR_AFF_CLIMBING)) {
      REMOVE_BIT(rider->specials.affected_by, CHAR_AFF_CLIMBING);
    }

    if (mover == rider->specials.riding) {
      if (to_room->sector_type == SECT_SILT) {
        sprintf(tmp,
                "$N %s %s, wading into the silt, carrying $n on $S "
                "back, who drags $h behind.", speed, dirs[cmd]);
        act(tmp, TRUE, rider, 0, mover, TO_NOTVICT);
        sprintf(tmp,
                "$N %s %s, wading into the silt, carrying $n while $e drags you behind.",
                speed, dirs[cmd]);
        act(tmp, TRUE, rider, 0, mover, TO_VICT);
        act("You wade into the silt, dragging $N behind you, and riding $r.", 
            TRUE, rider, 0, rider->specials.subduing, TO_CHAR);
      } else {
        sprintf(tmp, "$N %s %s, carrying $n on $S back, who drags $h behind.", speed, dirs[cmd]);
                act(tmp, TRUE, rider, 0, mover, TO_NOTVICT);
                sprintf(tmp, "$N %s %s, carrying $n while $e drags you behind.", speed, dirs[cmd]);
                act(tmp, TRUE, rider, 0, mover, TO_VICT);
                act("You drag $N behind you, riding $r.", TRUE, rider, 0, rider->specials.subduing, TO_CHAR);
      }
    } else {
      if (to_room->sector_type == SECT_SILT) {
        sprintf(tmp, "$n %s %s, wading into the silt, dragging $N behind $m.", speed, dirs[cmd]);
        act(tmp, TRUE, rider, 0, rider->specials.subduing, TO_NOTVICT);
        sprintf(tmp, "$n %s %s, wading into the silt, dragging you behind $m.", speed, dirs[cmd]);
        act(tmp, TRUE, rider, 0, rider->specials.subduing, TO_VICT);
        act("You wade into the silt and drag $N behind you.", TRUE, rider, 0, rider->specials.subduing, TO_CHAR);
      } else {
        sprintf(tmp, "$n %s %s, dragging $N behind $m.", speed, dirs[cmd]);
        act(tmp, TRUE, rider, 0, rider->specials.subduing, TO_NOTVICT);
        sprintf(tmp, "$n %s %s, dragging you behind $m.", speed, dirs[cmd]);
        act(tmp, TRUE, rider, 0, rider->specials.subduing, TO_VICT);
        act("You drag $N behind you.", TRUE, rider, 0, rider->specials.subduing, TO_CHAR);
      }
    }
  } else { /* normal movement */
    if (IS_SET(ch->specials.affected_by, CHAR_AFF_CLIMBING)) {
      REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_CLIMBING);
    }
    if (GET_SPEED(ch) == SPEED_SNEAKING || 
        IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
      for (tch = ch->in_room->people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;
        
        if (ch != tch) {
          sneak = succeed_sneak(ch, tch, sneak_roll);
          if (!sneak) {
            if (mover == rider->specials.riding) {
              if (to_room->sector_type == SECT_SILT) {
                sprintf(tmp, "$r %s %s, wading into the silt and carrying $n on $s back.", speed, dirs[cmd]);
              } else {
                sprintf(tmp, "$r %s %s, carrying $n on $s back.", speed, dirs[cmd]);
              }
            } else if (mover->lifting) {
              if (to_room->sector_type == SECT_SILT) {
                sprintf(tmp, "You wade into the silt and %s %s behind you.",
                        GET_OBJ_WEIGHT(mover->lifting) >
                        get_lifting_capacity(mover->lifting->lifted_by)
                        ? "drag" : "carry", format_obj_to_char(mover->lifting,
                                                               mover, 1));
                act(tmp, 0, mover, 0, 0, TO_CHAR);
                
                sprintf(tmp,
                        "$n %s %s, wading into the silt and %s %s behind $m.",
                        speed, dirs[cmd],
                        GET_OBJ_WEIGHT(mover->lifting) >
                        get_lifting_capacity(mover->lifting->lifted_by)
                        ? "dragging" : "carrying",
                        format_obj_to_char(mover->lifting, mover, 1));
              } else {
                sprintf(tmp, "You %s %s behind you.",
                        GET_OBJ_WEIGHT(mover->lifting) >
                        get_lifting_capacity(mover->lifting->lifted_by)
                        ? "drag" : "carry", format_obj_to_char(mover->lifting,
                                                               mover, 1));
                act(tmp, 0, mover, 0, 0, TO_CHAR);
                
                sprintf(tmp, "$n %s %s, %s %s behind $m.", speed, dirs[cmd],
                        GET_OBJ_WEIGHT(mover->lifting) >
                        get_lifting_capacity(mover->lifting->lifted_by)
                        ? "dragging" : "carrying",
                        format_obj_to_char(mover->lifting, mover, 1));
              }
            } else {
              sprintf(tmp, "%s%s$n %s %s.", (how && how[0] ? how : ""), (how && how[0] ? ", " : ""), speed, dirs[cmd]);
            }
            
            act(tmp, TRUE, rider, 0, tch, TO_VICT);
          }
        }
      }
    } else {
      if (mover == rider->specials.riding) {
        if (to_room->sector_type == SECT_SILT) {
          sprintf(tmp, "$N %s %s, wading into the silt and carrying $n on $S back.", speed, dirs[cmd]);
        } else {
          sprintf(tmp, "$N %s %s, carrying $n on $S back.", speed, dirs[cmd]);
        }
      } else if (mover->lifting) {
        if (to_room->sector_type == SECT_SILT) {
          sprintf(tmp, "You %s %s behind you and wade into the silt.",
                  GET_OBJ_WEIGHT(mover->lifting) >
                  get_lifting_capacity(mover->lifting->lifted_by)
                  ? "drag" : "carry", format_obj_to_char(mover->lifting, mover, 1));
          act(tmp, 0, mover, 0, 0, TO_CHAR);
          
          sprintf(tmp, "$n %s %s, wading into the silt, and %s %s behind $m.", speed,
                  dirs[cmd],
                  GET_OBJ_WEIGHT(mover->lifting) >
                  get_lifting_capacity(mover->lifting->lifted_by)
                  ? "dragging" : "carrying", format_obj_to_char(mover->lifting, mover,
                                                                1));
        } else {
          sprintf(tmp, "You %s %s behind you.",
                  GET_OBJ_WEIGHT(mover->lifting) >
                  get_lifting_capacity(mover->lifting->lifted_by)
                  ? "drag" : "carry", format_obj_to_char(mover->lifting, mover, 1));
          act(tmp, 0, mover, 0, 0, TO_CHAR);
          
          sprintf(tmp, "$n %s %s, %s %s behind $m.", speed, dirs[cmd],
                  GET_OBJ_WEIGHT(mover->lifting) >
                  get_lifting_capacity(mover->lifting->lifted_by)
                  ? "dragging" : "carrying", format_obj_to_char(mover->lifting, mover,
                                                                1));
        }
      } else {
        if (to_room->sector_type == SECT_SILT) {
          sprintf(tmp, "%s%s$n %s %s wading into the silt.", how && how[0] ? how : "", how
                  && how[0] ? ", " : "", speed, dirs[cmd]);
        } else {
          sprintf(tmp, "%s%s$n %s %s.", how && how[0] ? how : "", how
                  && how[0] ? ", " : "", speed, dirs[cmd]);
        }
      }
      
      act(tmp, TRUE, rider, 0, mover, TO_ROOM);
    }
  }
  
  if (!ch->in_room) {
    gamelog("simple_move: Unable to find ch->in_room");
    gamelog(GET_NAME(ch));
    /*      fflush(stderr); */
    /*       raise(11); */
    extract_char(ch);
    return (-1);
  }
  
  if (!no_specials && special(ch, CMD_FROM_ROOM, dirs[cmd])) {
    return (0);
  }
  
  if (!IS_IMMORTAL(mover)) {
    adjust_move(mover, -need_movement);
    
    if (IS_NPC(mover)) {
      new_event(EVNT_REST, (PULSE_MOBILE + number(-10, 10)) * 4, mover, 0, 0, 0, 0);
    }
  }
  
  was_in = ch->in_room;
  char_from_room_move(ch);
  
  if ((was_in->direction[cmd]) && (was_in->direction[cmd]->to_room)) {
    char_to_room(ch, was_in->direction[cmd]->to_room);
  } else {
    char_to_room(ch, was_in);
  }
  
  if (!no_specials && special(ch, CMD_TO_ROOM, dirs[cmd])) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return (0);
  }

  // Before the mover has a chance to look around, banish any
  // shadows if the mover has a dome.
  if (affected_by_spell(ch, PSI_DOME)) {
    for (tch = ch->in_room->people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;       // In case the tch goes away
      /* Someone is shadow-walking in the room */
      if (tch && affected_by_spell(tch, PSI_SHADOW_WALK) && tch->desc && tch->desc->original) {
        send_to_char("The air around you condenses and your form is squeezed to nothingness.\n\r", tch);
        parse_command(tch, "return");
      }
    }
  }
  
  cmd_look(ch, "", -1, 0);
  
  /* if the rider didn't instigate the move, move the rider too */
  if (rider != ch) {
    char_from_room_move(rider);
    char_to_room(rider, ch->in_room);
    cmd_look(rider, "", -1, 0);
  }
  
  set_tracks_in_room(mover, was_in, was_in->direction[cmd]->to_room, cmd);
  
  /* check for alarm spell and compare if the person is the caster or not */
  char edesc[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  char buf4[MAX_STRING_LENGTH];
  char name_string[MAX_INPUT_LENGTH] = "";
  char account_string[MAX_INPUT_LENGTH] = "";
  int is_caster = 0;
  struct descriptor_data *d;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int alrmcount = 1;
  
  if (get_room_extra_desc_value(ch->in_room, "[ALARM_SPL_PC_info]", edesc, sizeof(edesc))) {
    sprintf(name_string, "%s", edesc);
  }

  if (get_room_extra_desc_value(ch->in_room, "[ALARM_SPL_ACCOUNT_info]", edesc, sizeof(edesc))) {
    sprintf(account_string, "%s", edesc);
  }
  
  if (IS_NPC(ch)) {
    sprintf(buf3, "%d", npc_index[ch->nr].vnum);
    if (!strcmp(name_string, buf3)) {
      is_caster = 1;
    }
  } else {
    if (!strcmp(name_string, ch->name) &&
        !strcmp(account_string, ch->account)) {
      is_caster = 1;
    }
  }
  
  if ((IS_SET(ch->in_room->room_flags, RFL_SPL_ALARM) &&
       (!IS_IMMORTAL(ch))) &&
      (!is_char_ethereal(ch)) &&
      (!is_caster)) {
    
    for (d = descriptor_list; d; d = d->next) {
      if (!d->connected && /* playing */
          !d->str) {       /* not editing a string */
        CHAR_DATA *caster = d->original ? d->original : d->character;
        if ((!strcmp(caster->name, name_string)) &&
            (!strcmp(caster->account, account_string))) {
          sprintf(buf4, "You sense that %s has triggered your alarm in %s.\n\r", PERS(caster, ch), ch->in_room->name);
          send_to_char(buf4, caster);
        }
      }
    }
    
    sector = ch->in_room->sector_type;
    
    switch (sector) {
    case SECT_SOUTHERN_CAVERN:
    case SECT_CAVERN:
      send_to_room("A deep rumbling comes from the stony walls.\n\r", ch->in_room);
      break;
    case SECT_DESERT:
      send_to_room("The sands underfoot shift with an odd chiming sound.\n\r",
                   ch->in_room);
      break;
    case SECT_HILLS:
    case SECT_MOUNTAIN:
      send_to_room("Rocks stir underfoot with a groan of noise.\n\r", ch->in_room);
      break;
    case SECT_SILT:
    case SECT_SHALLOWS:
      send_to_room("The silt underfoot roils, an odd chiming sound "
                   "momentarily filling the air.\n\r", ch->in_room);
      break;
    case SECT_THORNLANDS:
      send_to_room("The thorny bushes all around rustle as the "
                   "ground beneath them stirs.\n\r", ch->in_room);
      break;
    default:
      send_to_room("A brief ringing sound fills the air.\n\r", ch->in_room);
      break;
    }
    
    if (TRUE == get_room_extra_desc_value(ch->in_room, "[ALARM_SPL_USES]", buf1, MAX_STRING_LENGTH)) {
      alrmcount = atoi(buf1);
    }
    
    if (alrmcount > 1) {
      alrmcount -= 1;
      sprintf(buf2, "%d", alrmcount);
      set_room_extra_desc_value(ch->in_room, "[ALARM_SPL_USES]", buf2);
    } else if (alrmcount <= 1) {
      remove_alarm_spell(ch->in_room);
    }
  }
  
  if ((mover->lifting) && (mover->lifting->lifted_by->ch == mover)) {
    sprintf(buf, "Outside: %s %s %s %s %s.\n\r", MSTR(mover, short_descr), speed, dirs[cmd],
            GET_OBJ_WEIGHT(mover->lifting)
            > get_lifting_capacity(mover->lifting->lifted_by)
            ? "dragging" : "carrying", OSTR(mover->lifting, short_descr));
    send_to_wagon(buf, mover->lifting);
    
    obj_from_room(mover->lifting);
    obj_to_room(mover->lifting, mover->in_room);
  }
  
  if (mover == ch->specials.riding) {
    was_in = mover->in_room;
    char_from_room_move(mover);
    char_to_room(mover, was_in->direction[cmd]->to_room);
    cmd_look(mover, "", -1, 0);
  }

  if (ch->specials.subduing) {
    char_from_room_move(ch->specials.subduing);
    char_to_room(ch->specials.subduing, was_in->direction[cmd]->to_room);
    cmd_look(ch->specials.subduing, "", -1, 0);
  }
  
  if (GET_SPEED(ch) == SPEED_SNEAKING || IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
    for (tch = ch->in_room->people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;
      
      if (ch != tch && rider != tch) {
        sneak = succeed_sneak(ch, tch, sneak_roll);
        if (!sneak) {
          if (cmd < DIR_UP) {
            sprintf(buf, "$n has arrived from the %s", dirs[rev_dir[cmd]]);
          } else if (cmd == DIR_UP){
            sprintf(buf, "$n has arrived from below");
          } else {
            sprintf(buf, "$n has arrived from above");
          }
          
          if (how && how[0]) {
            strcat(buf, ", ");
            strcat(buf, how);
          } else if (ch->lifting) {
            sprintf(tmp, ", %s %s behind $m",
                    GET_OBJ_WEIGHT(ch->lifting) >
                    get_lifting_capacity(ch->lifting->lifted_by)
                    ? "dragging" : "carrying", OSTR(mover->lifting, short_descr));
            strcat(buf, tmp);
          }
          
          if (ch->specials.subduing) {
            strcat(buf, ", dragging $h behind");
          }
          
          if (ch->specials.riding) {
            strcat(buf, ", riding $r.");
          } else {
            strcat(buf, ".");
          }
          
          act(buf, FALSE, ch, 0, tch, TO_VICT);
        }
      }
    }
  } else { /*  ch is not sneaking  */
    if ((cmd < DIR_UP) && (ch->in_room->sector_type == (SECT_WATER_PLANE))) {
      sprintf(buf, "$n swims in from the %s", dirs[rev_dir[cmd]]);
    } else if (cmd < DIR_UP) {
      sprintf(buf, "$n has arrived from the %s", dirs[rev_dir[cmd]]);
    } else if ((cmd == DIR_UP) && (ch->in_room->sector_type == (SECT_WATER_PLANE))) {
      sprintf(buf, "$n swims in from below");
    } else if (cmd == DIR_UP){
      sprintf(buf, "$n has arrived from below");
    } else if (ch->in_room->sector_type == (SECT_WATER_PLANE)) {
      sprintf(buf, "$n swims in from above");
    } else {
      sprintf(buf, "$n has arrived from above");
    }
    
    if (how && how[0]) {
      strcat(buf, ", ");
      strcat(buf, how);
    }
    
    if (rider->specials.subduing) {
      strcat(buf, ", dragging $h behind");
    }
    
    if (rider->specials.riding) {
      strcat(buf, ", riding $r");
    } else if (rider->lifting) {
      sprintf(tmp, ", %s %s behind $m",
              GET_OBJ_WEIGHT(rider->lifting) > get_lifting_capacity(rider->lifting->lifted_by)
              ? "dragging" : "carrying", OSTR(mover->lifting, short_descr));
      strcat(buf, tmp);
    }
    
    strcat(buf, ".");
    act(buf, TRUE, rider, 0, ch->specials.subduing, TO_NOTVICT);
  }
  
  /*  Add chances for hearing arrivals here. - Kel  */
  
  if (IS_SET(ch->in_room->room_flags, RFL_DEATH) && !IS_IMMORTAL(ch)) {
    if (mover == rider->specials.riding) {
      die(mover);
    }
    
    if (rider->specials.subduing) {
      CHAR_DATA *victim = rider->specials.subduing;
      
      sprintf(buf, "%s %s%s%swas dragged into death flag room %d and died.", GET_NAME(victim),
              !IS_NPC(victim) ? "(" : "", !IS_NPC(victim) ? victim->account : "",
              !IS_NPC(victim) ? ") " : "", victim->in_room->number);
      
      if (!IS_NPC(rider->specials.subduing)) {
        struct time_info_data playing_time =
          real_time_passed((time(0) - victim->player.time.logon) +
                           victim->player.time.played, 0);
        
        if ((playing_time.day > 0 || playing_time.hours > 2
             || (victim->player.dead && !strcmp(victim->player.dead, "rebirth")))
            && !IS_SET(victim->specials.act, CFL_NO_DEATH)) {
          if (victim->player.info[1]) {
            free(victim->player.info[1]);
          }
          victim->player.info[1] = strdup(buf);
        }
        gamelog(buf);
      } else {
        shhlog(buf);
      }
      
      die(rider->specials.subduing);
    }
    
    sprintf(buf, "%s %s%s%sentered into room %d and died of the death flag.", GET_NAME(rider),
            !IS_NPC(rider) ? "(" : "", !IS_NPC(rider) ? rider->account : "",
            !IS_NPC(rider) ? ") " : "", rider->in_room->number);
    
    if (!IS_NPC(rider)) {
      struct time_info_data playing_time =
        real_time_passed((time(0) - rider->player.time.logon) + rider->player.time.played, 0);
      
      if ((playing_time.day > 0 || playing_time.hours > 2 ||
           (rider->player.dead && !strcmp(rider->player.dead, "rebirth"))) &&
          !IS_SET(rider->specials.act, CFL_NO_DEATH)) {
        if (rider->player.info[1]) {
          free(rider->player.info[1]);
        }
        rider->player.info[1] = strdup(buf);
      }
      gamelog(buf);
    } else {
      shhlog(buf);
    }
    
    die(rider);
    
    return -1;
  }
  
  if (ch->desc && !IS_IMMORTAL(ch)) {
    ch->specials.act_wait += lag_time(mover) + extra_lag;
  }
  
  return TRUE;
}


/* global to keep track of if the current mover is following */
bool new_mover_is_following = FALSE;

void
cmd_move(CHAR_DATA * ch,        /* The character trying to move */
         const char *argument,  /* What they typed?             */
         Command cmd,           /* Which command they used      */
         int count) {
  char tmp[80];
  char how[MAX_STRING_LENGTH];
  char speed1[256], speed2[256];
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  const char *t;
  float condition = 0.0;      // Weather condition
  
  ROOM_DATA *to_room = NULL;
  struct room_data *was_in;
  struct room_data *dome_room;
  struct follow_type *k, *next_dude;
  struct weather_node *wn;
  
  extern int shadow_walk_into_dome_room(struct room_data *tar_room);
  
  if (!ch->in_room) {
    return;
  }
  
  /* Plants can't walk */
  if (GET_RACE(ch) == RACE_GESRA) {
    return;
  }
  
  if (is_paralyzed(ch)) {
    return;
  }
  
  for (wn = wn_list; wn; wn = wn->next) {
    if (wn->zone == ch->in_room->zone) {
      break;
    }
  }
  
  // hold onto the room they thing they're going to, if it exists
  if( ch->in_room->direction[cmd - 1] ) {
    to_room = ch->in_room->direction[cmd - 1]->to_room;
  }
  
  /* Added a check to the following so if you can't move in a   */
  /* particular direction you won't still get the sands message */
  /* when you try to go in that direction.   -San               */
  
  condition = 0.0;
  if (wn) {
    condition = wn->condition;
  }

  if (IS_SET(ch->in_room->room_flags, RFL_SANDSTORM)) {
    condition += 5.0;
  }

  if (IS_SET(ch->in_room->room_flags, RFL_INDOORS)) {
    condition -= 10.0;
  }

  if (affected_by_spell(ch, SPELL_WIND_ARMOR)) {
    condition -= 5.0;
  }
  
  if ((wn && !IS_SET(ch->in_room->room_flags, RFL_INDOORS)) &&
      (ch->in_room->direction[cmd - 1])) {
    if ((condition <= 2.50) && (condition >= 2.00)) {
      send_to_char("Sparse sands blow across your path.\n\r", ch);
    } else if ((condition <= 5.00) && (condition >= 2.00)) {
      send_to_char("Stinging sand swirls around you.\n\r", ch);
    } else if ((condition <= 7.00) && (condition >= 2.00)) {
      send_to_char("Terrible, biting sand whips around you.\n\r", ch);
    } else if (condition > 7.00 && condition <= 8.50) {
      send_to_char("Blinding, biting sands surround you.\n\r", ch);
    } else if (condition > 8.50) {
      send_to_char("A tremendous sandstorm surrounds you.\n\r", ch);
    }
  }
  
  /* infravision reduced the severity of the storm */
  if (affected_by_spell(ch, SPELL_INFRAVISION) ||
      IS_AFFECTED(ch, CHAR_AFF_INFRAVISION)) {
    condition -= 3.0;
  }
  
  if ((IS_SET(ch->in_room->room_flags, RFL_SANDSTORM)) ||
      ((wn && !IS_SET(ch->in_room->room_flags, RFL_INDOORS) && condition >= 5.00))) {
    if (ch->in_room->sector_type == SECT_CITY) {
      if (!number(0, 50)) {
        sflag_to_char(ch, OST_DUSTY);
      }
    } else
      sflag_to_char(ch, OST_DUSTY);
  }
  
  // affected by skellebane phase 3, disoriented movement.
  if (affected_by_spell(ch, POISON_SKELLEBAIN_3)) {
    if (number(0, 3) < 1) { // 25% of the movement commands.
      send_to_char("You feel very disoriented and stumble, moving in the wrong direction.\n\r", ch);
      to_room = get_random_room_from_exits(ch);
    }
  }
  
  if (!IS_DARK(ch->in_room) && 
      condition > 7.00  &&
      GET_GUILD(ch) != GUILD_LIGHTNING_ELEMENTALIST &&
      !affected_by_spell(ch, SPELL_ETHEREAL) &&
      (!IS_SET(ch->specials.act, CFL_MOUNT) || 
       (IS_SET(ch->specials.act, CFL_MOUNT) && !ch->master)) &&
      !IS_IMMORTAL(ch)) {
    // direction sense
    int skill = GET_WIS(ch);
    int roll = number(1, 100);
    OBJ_DATA *sunslits = ch->equipment[WEAR_FACE];
    
    qroomlogf(QUIET_CALC, ch->in_room, "%s has %d base direction sense for wis", MSTR(ch, name), skill);
    
    // if they have it, use it
    if( has_skill(ch, SKILL_DIRECTION_SENSE)) {
      skill += ch->skills[SKILL_DIRECTION_SENSE]->learned;
      qroomlogf(QUIET_CALC, ch->in_room, " +%d Direction sense = %d",
                ch->skills[SKILL_DIRECTION_SENSE]->learned, skill);
    }
    
    // sunslits drastically improve visibility through storms
    if (sunslits != NULL &&
        isname("sunslits", OSTR(sunslits, name))) {
      skill += 50;
      qroomlogf(QUIET_CALC, ch->in_room, " +50 sunslits = %d", skill);
    }
    
    // master has a direction sense and is in the 'to_room'
    if (ch->master && has_skill(ch->master, SKILL_DIRECTION_SENSE) &&
        ch->master->in_room == to_room) {
      skill += ch->master->skills[SKILL_DIRECTION_SENSE]->learned;
      qroomlogf(QUIET_CALC, ch->in_room, " +%d leader = %d", 
                ch->master->skills[SKILL_DIRECTION_SENSE]->learned, skill);
    }
    
    // skill contest
    if (roll < ((condition * 10.0) - skill)) {
      qroomlogf(QUIET_CALC, ch->in_room, 
                " roll of %d in condition %f vs %f, FAIL",
                roll, condition, ((condition * 10.0) - skill));
      send_to_char("Blowing, stinging sands cause you to lose your bearings.\n\r", ch);
      
      to_room = get_random_room_from_exits(ch);
      
      gain_skill(ch, SKILL_DIRECTION_SENSE, 1);
    }
  }
  
  
  /***********************************************************************/
  /*** settting up the description on a move, to pass onto simple_move ***/
  /***********************************************************************/
  
  t = argument;
  while (t && *t && isspace(*t)) {
    t++;
  }
  
  if (*t == '-' || *t == '(' || *t == '[' || *t == '*') {
    argument = one_argument_delim(argument, how, MAX_STRING_LENGTH, *t);
  } else {
    how[0] = '\0';
  }
  
  /*********************************************************************/
  /*** ending up the description on a move, to pass onto simple_move ***/
  /*********************************************************************/
  
  if (IS_DARK(ch->in_room) &&
      /* Allow Infravision to find their way */
      !IS_AFFECTED(ch, CHAR_AFF_INFRAVISION) &&
      GET_RACE(ch) != RACE_VAMPIRE &&
      GET_RACE(ch) != RACE_HALFLING &&
      // hitched mounts follow for free
      (!IS_SET(ch->specials.act, CFL_MOUNT) ||
       (IS_SET(ch->specials.act, CFL_MOUNT) && !ch->master)) &&
      (!to_room || (to_room && IS_DARK(to_room))) &&
      !IS_IMMORTAL(ch)) {
    // direction sense
    int skill = GET_WIS(ch);
    
    if (has_skill(ch, SKILL_DIRECTION_SENSE)) {
      skill += ch->skills[SKILL_DIRECTION_SENSE]->learned;
    }
    
    if (ch->master && has_skill(ch->master, SKILL_DIRECTION_SENSE) &&
        ch->master->in_room == to_room) {
      skill += ch->master->skills[SKILL_DIRECTION_SENSE]->learned;
    }
    
    if (number(1, 100) < (90 - skill)) {
      send_to_char("You stumble around in the darkness and lose your bearings.\n\r", ch);

      to_room = get_random_room_from_exits(ch);

      gain_skill(ch, SKILL_DIRECTION_SENSE, 1);
    }
  }
  
  --cmd;
  
  if (!ch->in_room->direction[cmd]) {
    send_to_char("Alas, you cannot go that way.\n\r", ch);
  } else {                    /* Direction is possible */
    
    /* Regardless of wether they can move, add sweat flags 
     * if they're moving fast and tired */
    if ((GET_MOVE(ch) < (GET_MAX_MOVE(ch) / 6)) && (GET_SPEED(ch) == SPEED_RUNNING)) {
      if (number(0, 100) < 80) {
        sflag_to_char(ch, OST_SWEATY);
      }
    }
    
    if (!(dome_room = ch->in_room->direction[cmd]->to_room)) {
      gamelog("No dome_room in command move.");
    }
    
    if (exit_guarded(ch, cmd, cmd)) {
      return;
    }
    
    if (IS_SET(EXIT(ch, cmd)->exit_info, EX_SPL_SAND_WALL) &&
        !is_char_ethereal(ch)) {
      send_to_char("A large wall of sand blocks your path.\n\r", ch);
      return;
    } else if (IS_SET(EXIT(ch, cmd)->exit_info, EX_CLOSED) && !is_char_ethereal(ch)) {
      if (!IS_SET(EXIT(ch, cmd)->exit_info, EX_SECRET))
        if (EXIT(ch, cmd)->keyword) {
          sprintf(tmp, "The %s seems to be closed.\n\r", first_name(EXIT(ch, cmd)->keyword, buf2));
          send_to_char(tmp, ch);
        } else {
          send_to_char("It seems to be closed.\n\r", ch);
        } else {        /* Secret Exit */
        
        send_to_char("Alas, you cannot go that way.\n\r", ch);
      }
    } else if (!EXIT(ch, cmd)->to_room) {
      send_to_char("Alas, you cannot go that way.\n\r", ch);
    } else if (IS_SET(EXIT(ch, cmd)->exit_info, EX_RUNES)
               && !IS_IMMORTAL(ch)) {
      if (!IS_SET(EXIT(ch, cmd)->exit_info, EX_SECRET)) {
        send_to_char("Something stops you from going that way.\n\r", ch);
      } else {
        send_to_char("Alas, you cannot go that way.\n\r", ch);
      }
    } else if (affected_by_spell(ch, PSI_SHADOW_WALK) && 
               dome_room &&
               shadow_walk_into_dome_room(dome_room)) {
      send_to_char("Something stops you from going that way.\n\r", ch);
    } else if (!IS_IMMORTAL(ch) && 
               affected_by_spell(ch, SPELL_SEND_SHADOW) && 
               dome_room &&
               IS_SET(dome_room->room_flags, RFL_NO_ELEM_MAGICK)) {
      /* No magick shadows allowed there */
      send_to_char("Your vision goes momentarily black...\n\r", ch);
      parse_command(ch, "return");
    } else if (!IS_IMMORTAL(ch) &&
               (affected_by_spell(ch, SPELL_ETHEREAL) || affected_by_spell(ch, SPELL_INVISIBLE)) && 
               dome_room &&
               IS_SET(dome_room->room_flags, RFL_NO_ELEM_MAGICK)) {
      send_to_char("Something stops you from going that way.\n\r", ch);
    } else if (!ch->followers && 
               !ch->master && 
               !ch->lifting &&
               !(ch->specials.riding && ch->specials.riding->followers)) {
      cmd_simple_move(ch, cmd, FALSE, how);
    } else {
      if (is_charmed(ch) &&
          (ch->in_room == ch->master->in_room)) {
        send_to_char("The thought of leaving your master makes you weep.\n\r", ch);
      } else {
        was_in = ch->in_room;
        if (cmd_simple_move(ch, cmd, new_mover_is_following, how) == 1) {
          if (ch->followers) {
            for (k = ch->followers; k; k = next_dude) {
              next_dude = k->next;
              if (k->follower && 
                  k->follower->in_room &&
                  (was_in == k->follower->in_room) &&
                  (GET_POS(k->follower) >= POSITION_STANDING)) {
                ROOM_DATA *new_room;
                
                if (GET_SPEED(k->follower) != GET_SPEED(ch)) {
                  GET_SPEED(k->follower) = GET_SPEED(ch);
                }
                
                get_speed_text(k->follower, speed1, speed2);
                
                /* temporarily save the room ch moved to, and
                 * move them back to their old room so they
                 * show up correctly
                 */
                new_room = ch->in_room;
                remove_char_from_room(ch);
                add_char_to_room(ch, k->follower->in_room);
                
                sprintf(buf, "You follow $N, and %s %s.", speed1, dirs[cmd]);
                
                act(buf, FALSE, k->follower, 0, ch, TO_CHAR);
                
                /* move ch back */
                remove_char_from_room(ch);
                add_char_to_room(ch, new_room);
                
                cmd++;
                send_to_char("\n\r", k->follower);
                new_mover_is_following = TRUE;
                cmd_move(k->follower, argument, cmd, 0);
                new_mover_is_following = FALSE;
                cmd--;
              } else {
                if (k->follower)
                  if (IS_SET(k->follower->specials.act, CFL_MOUNT) &&
                      (k->follower->in_room != ch->in_room)) {
                    act("You release $N's reins.", FALSE, ch, 0, k->follower, TO_CHAR);
                    act("Your reins are released.", FALSE, ch, 0, k->follower, TO_VICT);
                    stop_follower(k->follower);
                  }
              }
            }
          }           /* end if ch->followers */
          if (ch->specials.riding && ch->specials.riding->followers) {
            for (k = ch->specials.riding->followers; k; k = next_dude) {
              next_dude = k->next;
              if ((was_in == k->follower->in_room) &&
                  (GET_POS(k->follower) >= POSITION_STANDING)) {
                ROOM_DATA *new_room;
                
                if (GET_SPEED(k->follower) != GET_SPEED(ch)) {
                  GET_SPEED(k->follower) = GET_SPEED(ch);
                }
                
                get_speed_text(k->follower, speed1, speed2);
                
                /* temporarily save the room the mount moved to,
                 * and move them back to their old room so they
                 * show up correctly
                 */
                new_room = ch->specials.riding->in_room;
                remove_char_from_room(ch->specials.riding);
                add_char_to_room(ch->specials.riding, k->follower->in_room);
                
                sprintf(buf, "You follow $N, and %s %s.", speed1, dirs[cmd]);
                
                act(buf, FALSE, k->follower, 0, ch->specials.riding, TO_CHAR);
                
                /* put the mount back */
                remove_char_from_room(ch->specials.riding);
                add_char_to_room(ch->specials.riding, new_room);
                
                cmd++;
                send_to_char("\n\r", k->follower);
                new_mover_is_following = TRUE;
                cmd_move(k->follower, argument, cmd, 0);
                new_mover_is_following = FALSE;
                cmd--;
              }
            }
          }
          if (ch->lifting) {
            PLYR_LIST *pPl, *pPlNext;
            int obj_weight, capacity;
            
            for (pPl = ch->lifting->lifted_by; pPl; pPl = pPlNext) {
              pPlNext = pPl->next;
              
              if (pPl->ch && was_in == pPl->ch->in_room
                  && GET_POS(pPl->ch) >= POSITION_STANDING) {
                if (GET_SPEED(pPl->ch) != GET_SPEED(ch)) {
                  GET_SPEED(pPl->ch) = GET_SPEED(ch);
                }
                
                get_speed_text(ch, speed1, speed2);
                
                obj_weight = GET_OBJ_WEIGHT(ch->lifting);
                capacity = get_lifting_capacity(ch->lifting->lifted_by);
                
                sprintf(buf, "You help $N %s $p, and %s %s.",
                        (obj_weight / 100) < capacity ? "carry" : "drag", speed1,
                        dirs[cmd]);
                
                act(buf, FALSE, pPl->ch, ch->lifting, ch, TO_CHAR);
                
                cmd++;
                send_to_char("\n\r", pPl->ch);
                new_mover_is_following = TRUE;
                cmd_move(pPl->ch, argument, cmd, 0);
                new_mover_is_following = FALSE;
                cmd--;
              }
            }
          }
        }
      }
    }  
  }
}

int
find_door(CHAR_DATA * ch, char *type, char *dir) {
  char buf[MAX_STRING_LENGTH];
  int door;
  
  if (*dir) {                 /* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) == -1) {    /* Partial Match */
      send_to_char("That's not a direction.\n\r", ch);
      return (-1);
    }
    if (EXIT(ch, door)) {
      if (EXIT(ch, door)->keyword) {
        if (isname(type, EXIT(ch, door)->keyword))
          return (door);
        else {
          sprintf(buf, "I see no %s there.\n\r", type);
          send_to_char(buf, ch);
          return (-1);
        }
      } else
        return (door);
    } else {
      sprintf(buf, "I see no %s there.\n\r", type);
      send_to_char(buf, ch);
      return (-1);
    }
  } else {                    /* try to locate the keyword */
    if ((door = search_block(dir, dirs, FALSE)) > 0) {
      send_to_char("That doesn't make any sense.\n\r", ch);
      return (0);
    }
    
    for (door = 0; door <= 5; door++) {
      if (EXIT(ch, door) &&
          EXIT(ch, door)->keyword &&
          isname(type, EXIT(ch, door)->keyword)) {
        return (door);
      }
    }
    
    sprintf(buf, "I see no %s here.\n\r", type);
    send_to_char(buf, ch);
    return (-1);
  }
}

/* handles opening an object */
void
open_object(CHAR_DATA * ch, OBJ_DATA * obj) {
  int damage = 0;
  OBJ_DATA *tempobj;
  char buf[MAX_STRING_LENGTH];
  
  REMOVE_BIT(obj->obj_flags.value[1], CONT_CLOSED);
  
  if (!is_cloak(ch, obj) && 
      IS_SET(obj->obj_flags.value[1], CONT_TRAPPED)) {
    act("$p explodes!", FALSE, ch, obj, 0, TO_CHAR);
    act("$p explodes in $n's hands!", FALSE, ch, obj, 0, TO_ROOM);
    
    /* Remove the trap before applying damage */
    REMOVE_BIT(obj->obj_flags.value[1], CONT_TRAPPED);

    if (damage_absorbed_by_invulnerability(ch)) {
      // no-op
    } else {
      damage = number(10, 25) + number(10, 25);
      
      // TODO: Anything that is a tool adds damage? 
      for (tempobj = obj->contains; tempobj; tempobj = tempobj->next_content) {
        if (tempobj->obj_flags.type == ITEM_TOOL) {
          damage += number(10, 25);
        }
      }
      
      if (!generic_damage(ch, damage, 0, 0, number(10, 25) * number(1, 4))) {
        act("$n falls headlong, and dies before $e hits the ground.", FALSE, ch, obj, 0, TO_ROOM);
        {
          sprintf(buf, "%s %s%s%sopened a trapped object and blew up in room %d, dying.",
                  GET_NAME(ch), !IS_NPC(ch) ? "(" : "", !IS_NPC(ch) ? ch->account : "",
                  !IS_NPC(ch) ? ") " : "", ch->in_room->number);
          
          if (!IS_NPC(ch)) {
            struct time_info_data playing_time = real_time_passed((time(0) - ch->player.time.logon) +
                                                                  ch->player.time.played, 0);
            
            if ((playing_time.day > 0 || 
                 playing_time.hours > 2 ||
                 (ch->player.dead && !strcmp(ch->player.dead, "rebirth"))) &&
                !IS_SET(ch->specials.act, CFL_NO_DEATH)) {
              if (ch->player.info[1]) {
                free(ch->player.info[1]);
              }
              ch->player.info[1] = strdup(buf);
            }
            gamelog(buf);
          } else
            shhlog(buf);
          
          die(ch);
        }
      }
      extract_obj(obj);
    }
  }
}

void
cmd_open(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  int door, roll, skill, eq_pos = -1;
  struct room_data *other_room, *old_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  OBJ_DATA *obj;
  CHAR_DATA *victim = NULL, *findVict, *rch;
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char victim_name[256];
  char real_name_buf[MAX_STRING_LENGTH];
  bool quietly = FALSE;
  char orig_arg[MAX_STRING_LENGTH];
  
  strcpy(orig_arg, argument);
  
  // Add delay for unlatch
  if (!ch->specials.to_process && cmd == CMD_UNLATCH) {
    add_to_command_q(ch, orig_arg, cmd, 0, 0);
    return;
  }
  
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  argument = two_arguments(args, type, sizeof(type), dir, sizeof(dir));
  
  // unlatch stuff here for sleight of hand
  if (type) { // type holds the victim's name including the possessive "'s"
    strcpy(victim_name, type);
  } else {
    victim_name[0] = 0; // Null terminate the victim_name buffer.
  }
  
  
  /* Need to check for person's item here */
  if ((strlen(victim_name) > 2) &&
      (((victim_name[strlen(victim_name) - 2] == '\'') &&
        (victim_name[strlen(victim_name) - 1] == 's')) ||
       (((victim_name[strlen(victim_name) - 1] == '\'') &&
         (victim_name[strlen(victim_name) - 2] == 's'))))) {

    /* crop off the possessives */
    if (victim_name[strlen(victim_name) - 1] == '\'') {
      victim_name[strlen(victim_name) - 1] = 0;   /* nul terminate */
    } else {
      victim_name[strlen(victim_name) - 2] = 0;   /* nul terminate */
    }
    
    victim = get_char_room_vis(ch, victim_name);
  }
  
  if (!*type) {                 /* no argument was found */
    send_to_char("Open what?\n\r", ch);
  } else if (!*dir && generic_find(type, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &findVict, &obj)) {
    /* this is an object */
    if (obj_guarded(ch, obj, CMD_OPEN)) {
      return;
    } else if (is_cloak(ch, obj)) { // check to see if it's a cloak and worn around them
      if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED )) {
        cprintf( ch, "%s is already open.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
        return;
      }
    } else if (obj->obj_flags.type != ITEM_CONTAINER) {    // is it a container?
      cprintf(ch, "%s is not a container.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    } else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) { // is it already closed?
      cprintf(ch, "%s is already open!\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    } else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE)) {// is it closeable?
      cprintf(ch,"You can't find a way to open %s.\n\r", format_obj_to_char(obj, ch, 1));
      return;
    } else if ((IS_SET(obj->obj_flags.value[1], CONT_SEALED) &&    // is it sealed?
                (!IS_SET(obj->obj_flags.value[1], CONT_SEAL_BROKEN)))) {
      cprintf(ch, "%s is sealed.  You must break the seal before opening it.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    } else if (IS_SET(obj->obj_flags.value[1], CONT_LOCKED)) {// is it locked?
      cprintf(ch, "%s seems to be locked.\n\r",  capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    }
    

    find_obj_keyword(obj,
                     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
                     ch, buf, sizeof(buf));
    
    /* check for sleight of hand skill, if they have it, no room mesg-sv */
    if ((cmd == CMD_UNLATCH) &&
        (ch->skills[SKILL_SLEIGHT_OF_HAND] &&
         skill_success(ch, NULL, ch->skills[SKILL_SLEIGHT_OF_HAND]->learned))) {
      sprintf(to_char, "you unlatch ~%s", buf);
      sprintf(to_room, "@ unlatches ~%s", buf);
      
      if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow, to_char,
                                               to_room, to_room, MONITOR_ROGUE, 0, SKILL_SLEIGHT_OF_HAND)) {
        return;
      }
      open_object(ch, obj);
    } else {
      
      sprintf(to_char, "you open ~%s", buf);
      sprintf(to_room, "@ opens ~%s", buf);
      
      if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
        return;
      }
      
      if (cmd == CMD_UNLATCH) {      // Failed to unlatch
        gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
        for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
          if (watch_success(rch, ch, NULL, 20, SKILL_SLEIGHT_OF_HAND)) {
            cprintf(rch, "You notice %s was trying to be furtive about it.\n\r", PERS(rch, ch));
          } 
        }
      } 
      
      open_object(ch, obj);
    }
  } else if (cmd == CMD_UNLATCH && victim) {
    /* this is an object */
    obj = get_object_in_equip_vis(ch, dir, victim->equipment, &eq_pos);
    if (!obj) {
      if ((obj = get_obj_in_list_vis(ch, dir, victim->carrying)) == NULL)  {
        act("$E isn't wearing or carrying anything like that.", FALSE, ch, 0, victim, TO_CHAR);
        return;
      }
    }
    // check to see if the object is covered up (inaccessible)
    if (is_covered(ch, victim, eq_pos)) {
      cprintf( ch, "%s is covered up, making it too difficult to unlatch.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    }
    // is the object guarded?
    if (obj_guarded(victim, obj, CMD_OPEN)) {
      return;
    } else if (is_cloak(victim, obj)) {    // check to see if it's a cloak and worn around them
      if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED )) {
        cprintf( ch, "%s is already open.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
        return;
      }
    } else if (obj->obj_flags.type != ITEM_CONTAINER) {    // is it a container?
      cprintf(ch, "%s is not a container.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    }  else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) {    // is it already closed?
      cprintf(ch, "%s is already open!\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    } else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE)) {    // is it closeable?
      cprintf(ch,"You can't find a way to open %s.\n\r",
              format_obj_to_char(obj, ch, 1));
      return;
    } else if ((IS_SET(obj->obj_flags.value[1], CONT_SEALED) && // is it sealed?
                (!IS_SET(obj->obj_flags.value[1], CONT_SEAL_BROKEN)))) {
      cprintf(ch, "%s is sealed.  You must break the seal before opening it.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    } else if (IS_SET(obj->obj_flags.value[1], CONT_LOCKED)) {   // is it locked?
      cprintf(ch, "%s seems to be locked.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    }
    
    if ((ch->skills[SKILL_SLEIGHT_OF_HAND] &&
         skill_success(ch, NULL, ch->skills[SKILL_SLEIGHT_OF_HAND]->learned))) {
      sprintf(to_char, "you unlatch %%%s %s", FORCE_NAME(victim), OSTR(obj, short_descr));
      sprintf(to_room, "@ unlatches %%%s %s", FORCE_NAME(victim), OSTR(obj, short_descr));
      
      if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow, to_char, NULL,
                                                    to_room, to_room, MONITOR_ROGUE, 0, SKILL_SLEIGHT_OF_HAND)) {
      }
      open_object(ch, obj);
    } else {
      // Got caught, how bad?
      if (AWAKE(victim)) {
        /* Char was seen */
        roll = number(1, 101);
        /* skill without taking into account the object */
        skill = ch->skills[SKILL_SLEIGHT_OF_HAND] ?
          ch->skills[SKILL_SLEIGHT_OF_HAND]->learned: 0;
        if (skill < roll) {
          REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
          
          snprintf(buf, sizeof(buf), "%s got caught unlatching %s from %s.\n\r", GET_NAME(ch), OSTR(obj, short_descr), GET_NAME(victim));
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
            int ct;
            if ((ct = room_in_city(ch->in_room)) == CITY_NONE)
              hit(victim, ch, TYPE_UNDEFINED);
            else if (!IS_SET(victim->specials.act, CFL_SENTINEL))
              cmd_flee(victim, "", 0, 0);
          }
        } /* End char got seen */
        else {              /* Char wasn't seen */
          snprintf(buf, sizeof(buf),
                   "%s bungles an attempt to unlatch %s from %s, but is not caught.\n\r",
                   GET_NAME(ch), OSTR(obj, short_descr), GET_NAME(victim));
          send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
          shhlogf("%s: %s", __FUNCTION__, buf);
          
          sprintf(to_char, "you fail to unlatch ^%s %s", FORCE_NAME(victim), OSTR(obj, short_descr));
          sprintf(to_room, "@ tries to steal from ~%s", FORCE_NAME(victim));
          if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow,
                                                        to_char, NULL, to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL)) {
            return;
          }
          
          act("You cover your mistake before $E discovers you.", FALSE, ch, 0, victim, TO_CHAR);
          act("You feel a hand in your belongings, but are unable to catch the culprit.", FALSE, ch, 0, victim, TO_VICT);
          
          if (IS_NPC(victim)) {
            if (GET_RACE_TYPE(victim) == RTYPE_HUMANOID) {
              strcpy(buf, "Thief! Thief!");
              cmd_shout(victim, buf, 0, 0);
            } else if (!IS_SET(victim->specials.act, CFL_SENTINEL))
              cmd_flee(victim, "", 0, 0);
          }
        }                   /* Char isn't seen */
      }
      
      /* If the char is awake */
      /* only gain if ! paralyzed or low skill */
      if (!is_paralyzed(victim) || get_skill_percentage(ch, SKILL_SLEIGHT_OF_HAND) < 40) {
        gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
      }
    }
  } else if ((door = find_door(ch, type, dir)) >= 0) {
    /* perhaps it is a door */
    
    if (IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
      if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
        send_to_char("It's already open!\n\r", ch);
        return;         /* added by Morg to stop it from opening again */
      } else if (IS_SET(EXIT(ch, door)->exit_info, EX_NO_OPEN)) {
        send_to_char("You don't see a way to open it from this side.\n\r", ch);
        return;
      } else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
        send_to_char("It seems to be locked.\n\r", ch);
        return;
      } else if (IS_SET(EXIT(ch, door)->exit_info, EX_SPL_FIRE_WALL)) {
        send_to_char("Wall of Fire, cmd_open.\n\r", ch);
        return;
      }
      
      if (exit_guarded(ch, door, CMD_OPEN))
        return;
      else {
        /* check for sleight of hand skill, if they have it, no room mesg-sv */
        if ((cmd == CMD_UNLATCH) && 
            (ch->skills[SKILL_SLEIGHT_OF_HAND] && 
             skill_success(ch, NULL, ch->skills[SKILL_SLEIGHT_OF_HAND]->learned))) {
          quietly = TRUE;
        }
        sprintf(to_room, "@ %s the %s",
                quietly ? "unlatches" : "opens",
                EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "door");
        sprintf(to_char, "you %s the %s",
                quietly ? "unlatch" : "open",
                EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "door");
        
        if (quietly) {
          if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow, to_char,
                                                   to_room, to_room, MONITOR_ROGUE, 0 , SKILL_SLEIGHT_OF_HAND)) {
            return;
          }
        } else { // Echo like a regular open
          if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
                                            to_room, to_room, MONITOR_OTHER)) {
            return;
          }
          if (cmd == CMD_UNLATCH) {      // Failed to unlatch
            gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
            for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
              if (watch_success(rch, ch, NULL, 20, SKILL_SLEIGHT_OF_HAND)) {
                cprintf(rch, "You notice %s was trying to be furtive about it.\n\r", PERS(rch, ch));
              } 
            }
          } 
        }
        
        REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
        
        /* now for opening the OTHER side of the door! */
        if ((other_room = EXIT(ch, door)->to_room)) {
          if ((back = other_room->direction[rev_dir[door]])) {
            if (back->to_room == ch->in_room) {
              REMOVE_BIT(back->exit_info, EX_CLOSED);
              
              
              old_room = ch->in_room;
              remove_char_from_room(ch);
              add_char_to_room(ch, other_room);
              
              if (back->keyword) {
                sprintf(to_room, "@ %s the %s from the other side",
                        quietly ? "unlatches" : "opens", back->keyword);
              } else {
                sprintf(to_room, "@ %s the door from the other side",
                        quietly ? "unlatches" : "opens");
              }
              
              if (quietly) {
                if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow, NULL, to_room, to_room, 
                                                         MONITOR_ROGUE, 0, SKILL_SLEIGHT_OF_HAND))
                  return;
              } else {
                if (!send_to_char_and_room_parsed(ch, preHow, postHow, NULL, to_room, to_room, MONITOR_OTHER))
                  return;
              }
              
              // notify the other side about the explosion
              if (IS_SET(EXIT(ch, rev_dir[door])->exit_info, EX_TRAPPED)) {
                act("You hear an explosion!",
                    FALSE, ch, 0, 0, TO_ROOM);
              }
              remove_char_from_room(ch);
              add_char_to_room(ch, old_room);
            }
          }
        }
        
        // now that messages are sent to both sides, handle traps
        if (IS_SET(EXIT(ch, door)->exit_info, EX_TRAPPED)) {
          act("An explosion throws you backward!", FALSE, ch, 0, 0, TO_CHAR);
          act("An explosion throws $n backward!", FALSE, ch, 0, 0, TO_ROOM);
          
          /* Remove the trap before applying damage */
          REMOVE_BIT(EXIT(ch, door)->exit_info, EX_TRAPPED);
          if ((other_room = EXIT(ch, door)->to_room)) {
            if ((back = other_room->direction[rev_dir[door]])) {
              if (back->to_room == ch->in_room) {
                REMOVE_BIT(back->exit_info, EX_TRAPPED);
              }
            }
          }
          
          adjust_hit(ch, -(number(10, 25) + number(10, 25)));
          update_pos(ch);
          if (GET_POS(ch) == POSITION_DEAD) {
            act("$n falls headlong, and dies before $e hits the ground.", FALSE, ch, 0, 0, TO_ROOM);
            die(ch);
          }
        }
      }
    }
  }
}

void
cmd_close(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  int door, roll, skill, eq_pos = -1;
  struct room_data *other_room, *old_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  OBJ_DATA *obj;
  CHAR_DATA *victim = 0, *rch = 0;
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  bool quietly = FALSE;
  char victim_name[256];
  char real_name_buf[MAX_STRING_LENGTH];
  char orig_arg[MAX_STRING_LENGTH];
  
  strcpy(orig_arg, argument);
  // Add delay for latch
  if (!ch->specials.to_process && cmd == CMD_LATCH) {
    add_to_command_q(ch, orig_arg, cmd, 0, 0);
    return;
  }
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  argument = two_arguments(args, type, sizeof(type), dir, sizeof(dir));
  
  // unlatch stuff here for sleight of hand
  if (type) {// type holds the victim's name including the possessive "'s" 
    strcpy(victim_name, type);
  } else {
    victim_name[0] = 0; // Null terminate the victim_name buffer.
  }
  
  /* Need to check for person's item here */
  if ((strlen(victim_name) > 2) &&
      (((victim_name[strlen(victim_name) - 2] == '\'') &&
        (victim_name[strlen(victim_name) - 1] == 's')) ||
       (((victim_name[strlen(victim_name) - 1] == '\'') &&
         (victim_name[strlen(victim_name) - 2] == 's'))))) {
    /* crop off the possessives */
    if (victim_name[strlen(victim_name) - 1] == '\'') {
      victim_name[strlen(victim_name) - 1] = 0;   /* nul terminate */
    } else {
      victim_name[strlen(victim_name) - 2] = 0;   /* nul terminate */
    }
    
    victim = get_char_room_vis(ch, victim_name);
  }

  if (!*type) {
    send_to_char("Close what?\n\r", ch);
  } else if (!*dir && generic_find(type, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj)) {
    /* this is an object */
    // is it guarded?
    if (obj_guarded(ch, obj, CMD_CLOSE)) {
      return;
      // check to see if it's a cloak and worn around them
    } else if (is_cloak(ch, obj)) {
      if (IS_SET(obj->obj_flags.value[1], CONT_CLOSED )) {
        cprintf( ch, "%s is already closed around you.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
        return;
      }
    }
    // is it a container?
    else if (obj->obj_flags.type != ITEM_CONTAINER) {
      cprintf(ch, "%s is not a container.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    }
    // is it already closed?
    else if (IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) {
      cprintf(ch, "%s is already closed!\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    }
    // is it closeable?
    else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE)) {
      cprintf(ch,"You can't find a way to close %s.\n\r", format_obj_to_char(obj, ch, 1));
      return;
    }
    
    find_obj_keyword(obj,
                     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
                     ch, buf, sizeof(buf));
    
    // If they're being quiet about it
    if ((cmd == CMD_LATCH) && 
        (has_skill(ch, SKILL_SLEIGHT_OF_HAND) && skill_success(ch, NULL, ch->skills[SKILL_SLEIGHT_OF_HAND]->learned))) {
      quietly = TRUE;
    }
    sprintf(to_char, "you %s ~%s", quietly ? "latch" : "close", buf);
    sprintf(to_room, "@ %s ~%s", quietly ? "latches" : "closes", buf);
    
    if (quietly) {
      if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow,
                                               to_char, to_room, to_room, MONITOR_ROGUE, 0 , SKILL_SLEIGHT_OF_HAND)) {
        return;
      }
    } else {
      if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
                                        to_room, to_room, MONITOR_OTHER)) {
        return;
      }
    }
    
    // set the appropriate bit
    MUD_SET_BIT(obj->obj_flags.value[1], CONT_CLOSED);
  } else if (cmd == CMD_LATCH && victim) {
    obj = get_object_in_equip_vis(ch, dir, victim->equipment, &eq_pos);

    if (!obj) {
      if ((obj = get_obj_in_list_vis(ch, dir, victim->carrying)) == NULL)  {
        act("$E isn't wearing or carrying anything like that.", FALSE, ch, 0, victim, TO_CHAR);
        return;
      }
    }
    // check to see if the object is covered up (inaccessible)
    if (is_covered(ch, victim, eq_pos)) {
      cprintf( ch, "%s is covered up, making it too difficult to latch.\n\r",
               capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    }

    // is the object guarded?
    if (obj_guarded(victim, obj, CMD_CLOSE)) {
      return;
    // check to see if it's a cloak and worn around them
    } else if (is_cloak(victim, obj)) {
      if (IS_SET(obj->obj_flags.value[1], CONT_CLOSED )) {
        cprintf( ch, "%s is already closed.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
        return;
      }
    }
    // is it a container?
    else if (obj->obj_flags.type != ITEM_CONTAINER) {
      cprintf(ch, "%s is not a container.\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    }
    // is it already closed?
    else if (IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) {
      cprintf(ch, "%s is already closed!\n\r", capitalize(format_obj_to_char(obj, ch, 1)));
      return;
    }
    // is it closeable?
    else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE)) {
      cprintf(ch,"You can't find a way to close %s.\n\r", format_obj_to_char(obj, ch, 1));
      return;
    }
    
    if ((has_skill(ch, SKILL_SLEIGHT_OF_HAND) &&
         skill_success(ch, NULL, ch->skills[SKILL_SLEIGHT_OF_HAND]->learned))) {
      sprintf(to_char, "you latch %%%s %s", FORCE_NAME(victim), OSTR(obj, short_descr));
      sprintf(to_room, "@ latches %%%s %s", FORCE_NAME(victim), OSTR(obj, short_descr));
      
      if (!send_hidden_to_char_vict_and_room_parsed(ch, victim, preHow, postHow, to_char, NULL,
                                                    to_room, to_room, MONITOR_ROGUE, 0, SKILL_SLEIGHT_OF_HAND)) {
      }
      MUD_SET_BIT(obj->obj_flags.value[1], CONT_CLOSED);
    } else {
      // Got caught, how bad?
      if (AWAKE(victim)) {
        /* Char was seen */
        roll = number(1, 101);
        /* skill without taking into account the object */
        skill = ch->skills[SKILL_SLEIGHT_OF_HAND] ?
          ch->skills[SKILL_SLEIGHT_OF_HAND]->learned: 0;
        if (skill < roll) {
          REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
          
          snprintf(buf, sizeof(buf), "%s got caught latching %s from %s.\n\r", GET_NAME(ch), OSTR(obj, short_descr), GET_NAME(victim));
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
            int ct;
            if ((ct = room_in_city(ch->in_room)) == CITY_NONE)
              hit(victim, ch, TYPE_UNDEFINED);
            else if (!IS_SET(victim->specials.act, CFL_SENTINEL))
              cmd_flee(victim, "", 0, 0);
          }
        } /* End char got seen */
        else {              /* Char wasn't seen */
          snprintf(buf, sizeof(buf),
                   "%s bungles an attempt to latch %s from %s, but is not caught.\n\r",
                   GET_NAME(ch), OSTR(obj, short_descr), GET_NAME(victim));
          send_to_monitors(ch, victim, buf, MONITOR_ROGUE);
          shhlogf("%s: %s", __FUNCTION__, buf);
          act("You cover your mistake before $E discovers you.", FALSE, ch, 0, victim, TO_CHAR);
          act("You feel a hand in your belongings, but are unable to catch the culprit.", FALSE, ch, 0, victim, TO_VICT);
          
          if (IS_NPC(victim)) {
            if (GET_RACE_TYPE(victim) == RTYPE_HUMANOID) {
              strcpy(buf, "Thief! Thief!");
              cmd_shout(victim, buf, 0, 0);
            } else if (!IS_SET(victim->specials.act, CFL_SENTINEL))
              cmd_flee(victim, "", 0, 0);
          }
        }                   /* Char isn't seen */
      }
      
      /* If the char is awake */
      /* only gain if ! paralyzed or low skill */
      if (!is_paralyzed(victim) || get_skill_percentage(ch, SKILL_SLEIGHT_OF_HAND) < 40) {
        gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
      }
    }
  } else if ((door = find_door(ch, type, dir)) >= 0) {
    /* Or a door */
    
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
      send_to_char("That's absurd.\n\r", ch);
      return;
    }
    
    if (IS_SET(EXIT(ch, door)->exit_info, EX_SPL_FIRE_WALL)) {
      send_to_char("Wall of Fire, cmd_close.\n\r", ch);
      return;
    } else if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      send_to_char("It's already closed!\n\r", ch);
    } else {
      if (GET_POS(ch) == POSITION_FIGHTING) {
        send_to_char("You're too busy fighting!\n\r", ch);
        return;
      }
      // If they're being quiet about it
      if ((cmd == CMD_LATCH) && 
          (has_skill(ch, SKILL_SLEIGHT_OF_HAND) &&
           skill_success(ch, NULL, ch->skills[SKILL_SLEIGHT_OF_HAND]->learned))) {
        quietly = TRUE;
      }

      sprintf(to_room, "@ %s the %s",
              quietly ? "latches" : "closes",
              EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "door");
      sprintf(to_char, "you %s the %s",
              quietly ? "latch" : "close",
              EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "door");
      
      if (quietly) {
        if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow, to_char,
                                                 to_room, to_room, MONITOR_ROGUE, 0 , SKILL_SLEIGHT_OF_HAND))
          return;
      } else { // Echo like a regular open
        if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
                                          to_room, to_room, MONITOR_OTHER)) {
          return;
        }
        if (cmd == CMD_LATCH) {      // Failed to unlatch
          gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
          for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
            if (watch_success(rch, ch, NULL, 20, SKILL_SLEIGHT_OF_HAND)) {
              cprintf(rch, "You notice %s was trying to be furtive about it.\n\r", PERS(rch, ch));
            }
          }
        }
      }
      
      MUD_SET_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      
      /* now for closing the other side, too */
      if ((other_room = EXIT(ch, door)->to_room)) {
        if ((back = other_room->direction[rev_dir[door]])) {
          if (back->to_room == ch->in_room) {
            MUD_SET_BIT(back->exit_info, EX_CLOSED);
            
            old_room = ch->in_room;
            remove_char_from_room(ch);
            add_char_to_room(ch, other_room);
            
            sprintf(to_room, "@ %s the %s from the other side",
                    quietly ? "latches" : "closes",
                    back->keyword ? back->keyword : "door");
            
            if (quietly) {
              if (!send_hidden_to_char_and_room_parsed(ch,
                                                       preHow, postHow, NULL, to_room, to_room, MONITOR_ROGUE, 0, SKILL_SLEIGHT_OF_HAND))
                return;
            } else {
              if (!send_to_char_and_room_parsed(ch,
                                                preHow, postHow, NULL, to_room, to_room, MONITOR_OTHER))
                return;
            }
            remove_char_from_room(ch);
            add_char_to_room(ch, old_room);
          }
        }
      }
    }
  }
}

OBJ_DATA *
has_key(CHAR_DATA * ch, int key) {
  OBJ_DATA *o;
  OBJ_DATA *kobj = NULL;
  
  if (key == -1) {// No key exists for this lock
    return NULL;
  }
  
  for (o = ch->carrying; o; o = o->next_content) {
    if (obj_index[o->nr].vnum == key && !is_obj_expired(o)) {
      kobj = o;
      goto return_key;
    }
  }
  
  if (!ch->equipment[ES]) {
    return NULL;
  }
  
  OBJ_DATA *obj = ch->equipment[ES];
  if (obj_index[obj->nr].vnum == key) {
    kobj = obj;
    goto return_key;
  }
  
  if (obj->obj_flags.type == ITEM_CONTAINER && IS_SET(obj->obj_flags.value[1], CONT_KEY_RING)) {
    OBJ_DATA *in_obj, *fobj = NULL;
    int count = 0;
    
    for (in_obj = obj->contains; in_obj; in_obj = in_obj->next_content) {
      count++;
      if (obj_index[in_obj->nr].vnum == key && !is_obj_expired(in_obj)) {
        fobj = in_obj;
        // wait duration is based on number of keys total
      }
    }
    
    if (!ch->specials.to_process) {
      act("$n searches through $p.", FALSE, ch, obj, NULL, TO_ROOM);
      act("You search through $p, looking for the key.", FALSE, ch, obj, NULL, TO_CHAR);
      
      WAIT_STATE(ch, MAX(1, count / 5));
      return ((OBJ_DATA *) - 1);
    }
    
    if (fobj) {
      kobj = fobj;
      goto return_key;
    }
  }
  
 return_key:
  if (kobj && is_obj_expired(kobj)) {
    return NULL;
  }
  return kobj;
}


void
cmd_lock(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  int door;
  struct room_data *other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char orig_arg[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  OBJ_DATA *obj;
  CHAR_DATA *victim;
  
  strcpy(orig_arg, argument);
  argument = two_arguments(argument, type, sizeof(type), dir, sizeof(dir));
  
  if (ch->lifting) {
    sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
    send_to_char(buf, ch);
    if (!drop_lifting(ch, ch->lifting)) {
      return;
    }
  }
  
  if (!*type) {
    send_to_char("Lock what?\n\r", ch);
    return;
  }
  
  if (generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj)) {
    /* this is an object */
    if (obj_guarded(ch, obj, CMD_LOCK)) {
      return; 
    } else if (obj->obj_flags.type != ITEM_CONTAINER) {
      send_to_char("That's not a container.\n\r", ch);
    } else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) {
      send_to_char("Maybe you should close it first...\n\r", ch);
    } else if (obj->obj_flags.value[2] <= 0) {
      send_to_char("That thing can't be locked.\n\r", ch);
    } else if (IS_SET(obj->obj_flags.value[1], CONT_LOCKED)) {
      send_to_char("It is locked already.\n\r", ch);
    } else {
      OBJ_DATA *fkey = has_key(ch, obj->obj_flags.value[2]);
      
      if ((int)fkey == -1) {
        add_to_command_q(ch, orig_arg, cmd, 0, 0);
        return;
      }
      
      if ((fkey == NULL) && (!IS_IMMORTAL(ch))) {
        send_to_char("You don't seem to have the proper key.\n\r", ch);
        return;
      }
      
      MUD_SET_BIT(obj->obj_flags.value[1], CONT_LOCKED);
      if (fkey) {
        act("You lock $p with $P - *click*", FALSE, ch, obj, fkey, TO_CHAR);
        act("$n locks $p with $P.", FALSE, ch, obj, fkey, TO_ROOM);
      } else {
        act("You lock $p - *click*", FALSE, ch, obj, 0, TO_CHAR);
        act("$n locks $p.", FALSE, ch, obj, 0, TO_ROOM);
      }
    }
  } else if ((door = find_door(ch, type, dir)) >= 0) {
    /* a door, perhaps */
    
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
      send_to_char("That's absurd.\n\r", ch);
    } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      send_to_char("You have to close it first, I'm afraid.\n\r", ch);
    } else if ((EXIT(ch, door)->key < 0) && (!IS_SET(EXIT(ch, door)->exit_info, EX_NO_KEY))) {
      send_to_char("There does not seem to be a keyhole.\n\r", ch);
    } else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
      send_to_char("It's already locked!\n\r", ch);
    } else if (IS_SET(EXIT(ch, door)->exit_info, EX_SPL_FIRE_WALL)) {
      send_to_char("Wall of Fire, cmd_lock.\n\r", ch);
      return;
    } else {
      if (IS_SET(EXIT(ch, door)->exit_info, EX_NO_KEY)) {
        if (EXIT(ch, door)->keyword) {
          act("$n locks the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
          act("You lock the $F - *click*.", 0, ch, 0, EXIT(ch, door)->keyword, TO_CHAR);
        } else {
          act("$n locks the door.", 0, ch, 0, 0, TO_ROOM);
          act("You lock the door - *click*.", 0, ch, 0, 0, TO_CHAR);
        }
        MUD_SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
        if ((other_room = EXIT(ch, door)->to_room)) {
          if ((back = other_room->direction[rev_dir[door]])) {
            if (back->to_room == ch->in_room) {
              MUD_SET_BIT(back->exit_info, EX_LOCKED);
            }
          }
        }
        return;
      }
      
      OBJ_DATA *fkey = has_key(ch, EXIT(ch, door)->key);
      
      if ((fkey == NULL) && (!IS_IMMORTAL(ch))) {
        send_to_char("You don't have the proper key.\n\r", ch);
      } else if (((int) fkey) == -1) {
        add_to_command_q(ch, orig_arg, cmd, 0, 0);
      } else {
        MUD_SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
        if (EXIT(ch, door)->keyword) {
          if (fkey) {
            act("$n locks the $F with $p.", 0, ch, fkey, EXIT(ch, door)->keyword, TO_ROOM);
            act("You lock the $F with $p. - *click*", 0, ch, fkey, EXIT(ch, door)->keyword, TO_CHAR);
          } else {
            act("$n locks the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
            act("You lock the $F - *click*", 0, ch, 0, EXIT(ch, door)->keyword, TO_CHAR);
          }
        } else {
          if (fkey) {
            act("$n locks the door with $p.", FALSE, ch, fkey, 0, TO_ROOM);
            act("You lock the door with $p. - *click*", FALSE, ch, fkey, 0, TO_CHAR);
          } else {
            act("$n locks the door.", FALSE, ch, 0, 0, TO_ROOM);
            act("You lock the door - *click*", FALSE, ch, 0, 0, TO_CHAR);
          }
        }
        
        /* now for locking the other side, too */
        if ((other_room = EXIT(ch, door)->to_room)) {
          if ((back = other_room->direction[rev_dir[door]])) {
            if (back->to_room == ch->in_room) {
              MUD_SET_BIT(back->exit_info, EX_LOCKED);
            }
          }
        }
      }
    }
  }
}


void
cmd_unlock(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  int door;
  struct room_data *other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char orig_arg[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  OBJ_DATA *obj;
  CHAR_DATA *victim;
  
  strcpy(orig_arg, argument);
  argument = two_arguments(argument, type, sizeof(type), dir, sizeof(dir));
  
  if (ch->lifting) {
    sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
    send_to_char(buf, ch);
    if (!drop_lifting(ch, ch->lifting)) {
      return;
    }
  }
  
  if (!*type) {
    send_to_char("Unlock what?\n\r", ch);
  } else if (generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj)) {
    /* this is an object */
    
    if (obj_guarded(ch, obj, CMD_UNLOCK)) {
      return;
    } else if (obj->obj_flags.type != ITEM_CONTAINER) {
      send_to_char("That's not a container.\n\r", ch);
    } else if (!IS_SET(obj->obj_flags.value[1], CONT_CLOSED)) { 
      send_to_char("Silly - it ain't even closed!\n\r", ch);
    } else if (obj->obj_flags.value[2] <= 0) {
      send_to_char("Odd - you can't seem to find a keyhole.\n\r", ch);
    } else if (!IS_SET(obj->obj_flags.value[1], CONT_LOCKED)) {
      send_to_char("Oh.. it wasn't locked, after all.\n\r", ch);
    } else {
      OBJ_DATA *fkey = has_key(ch, obj->obj_flags.value[2]);
      
      if ((fkey == NULL) && (!IS_IMMORTAL(ch))) {
        send_to_char("You don't seem to have the proper key.\n\r", ch);
      } else if (((int) fkey) == -1) {
        add_to_command_q(ch, orig_arg, cmd, 0, 0);
      } else {
        REMOVE_BIT(obj->obj_flags.value[1], CONT_LOCKED);
        if (fkey) {
          act("You unlock $p with $P. - *click*", FALSE, ch, obj, fkey, TO_CHAR);
          act("$n unlocks $p with $P.", FALSE, ch, obj, fkey, TO_ROOM);
        } else {
          act("You unlock $p. - *click*", FALSE, ch, obj, 0, TO_CHAR);
          act("$n unlocks $p.", FALSE, ch, obj, fkey, TO_ROOM);
        }
      }
    }
  } else if ((door = find_door(ch, type, dir)) >= 0) {        /* it is a door */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
      send_to_char("That's absurd.\n\r", ch);
    } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      send_to_char("Heck.. it ain't even closed!\n\r", ch);
    } else if ((EXIT(ch, door)->key < 0) && (!IS_SET(EXIT(ch, door)->exit_info, EX_NO_KEY))) {
      send_to_char("You can't seem to spot any keyholes.\n\r", ch);
    } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
      send_to_char("It's already unlocked, it seems.\n\r", ch);
    } else if (IS_SET(EXIT(ch, door)->exit_info, EX_SPL_FIRE_WALL)) {
      send_to_char("Wall of Fire, cmd_unlock.\n\r", ch);
      return;
    } else {
      if (IS_SET(EXIT(ch, door)->exit_info, EX_NO_KEY)) {
        if (EXIT(ch, door)->keyword) {
          act("$n unlocks the $F.", FALSE, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
          act("You unlock the $F. - *click*", FALSE, ch, 0, EXIT(ch, door)->keyword, TO_CHAR);
        } else {
          act("$n unlocks the door.", FALSE, ch, 0, 0, TO_ROOM);
          act("You unlock the door. - *click*", FALSE, ch, 0, 0, TO_CHAR);
        }
        REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
        if ((other_room = EXIT(ch, door)->to_room)) {
          if ((back = other_room->direction[rev_dir[door]])) {
            if (back->to_room == ch->in_room) {
              REMOVE_BIT(back->exit_info, EX_LOCKED);
            }
          }
        }
        return;
      }
      
      OBJ_DATA *fkey = has_key(ch, EXIT(ch, door)->key);
      
      if ((fkey == NULL) && (!IS_IMMORTAL(ch))) {
        send_to_char("You do not have the proper key for that.\n\r", ch);
      } else if (((int) fkey) == -1) {
        add_to_command_q(ch, orig_arg, cmd, 0, 0);
      } else {
        REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
        if (EXIT(ch, door)->keyword) {
          if (fkey) {
            act("$n unlocks the $F with $p.", 0, ch, fkey, EXIT(ch, door)->keyword, TO_ROOM);
            act("You unlock the $F with $p. - *click*", 0, ch, fkey, EXIT(ch, door)->keyword, TO_CHAR);
          } else {
            act("$n unlocks the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
            act("You unlock the $F. - *click*", 0, ch, 0, EXIT(ch, door)->keyword, TO_CHAR);
          }
        } else {
          if (fkey) {
            act("$n unlocks the door with $p.", FALSE, ch, fkey, 0, TO_ROOM);
            act("You unlock the door with $p. - *click*", FALSE, ch, fkey, 0, TO_CHAR);
          } else {
            act("$n unlocks the door.", FALSE, ch, 0, 0, TO_ROOM);
            act("You unlock the door. - *click*", FALSE, ch, 0, 0, TO_CHAR);
            
          }
        }
        /* now for unlocking the other side, too */
        if ((other_room = EXIT(ch, door)->to_room)) {
          if ((back = other_room->direction[rev_dir[door]])) {
            if (back->to_room == ch->in_room) {
              REMOVE_BIT(back->exit_info, EX_LOCKED);
            }
          }
        }
      }
    }
  }
}

void
cmd_enter(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  int sector;
  char buf[MAX_INPUT_LENGTH], tmp[MAX_STRING_LENGTH];
  static char backup[MAX_INPUT_LENGTH];
  struct room_data *room;
  struct room_data *was_in;
  OBJ_DATA *wag;
  CHAR_DATA *mt, *sub, *tch, *t_next_in_room;
  struct follow_type *k, *next_dude;
  bool sneak;
  bool vortex;

  int sneak_roll = number(1, 101);
  
  if (!ch->in_room) {
    return;
  }
  
  strcpy(backup, argument);
  one_argument_delim(argument, buf, MAX_STRING_LENGTH, ' ');
  
  if (*buf) {
    wag = get_obj_in_list_vis(ch, buf, ch->in_room->contents);
    
    if (!wag) {
      send_to_char("You do not see that here.\n\r", ch);
      return;
    }
    
    if (wag->obj_flags.type != ITEM_WAGON) {
      send_to_char("You cannot enter that.\n\r", ch);
      return;
    }
    
    /* Does it enter anwhere but room zero? (default value) */
    if ((room = get_room_num(wag->obj_flags.value[0])) == 0) {
      send_to_char("You cannot enter that.\n\r", ch);
      return;
    }
    
    if (!(room = get_room_num(wag->obj_flags.value[0]))) {
      send_to_char("Error entering object.\n\r", ch);
      return;
    }
    
    if (((wag->obj_flags.value[5] > 0) &&
         (get_char_size(ch) > wag->obj_flags.value[5])) &&
        !is_char_ethereal(ch) &&
        !IS_IMMORTAL(ch)) {
      send_to_char("You are too big to enter that.\n\r", ch);
      return;
    }
    
    /* Can't leave your master */
    if (is_charmed(ch) &&
        (ch->in_room == ch->master->in_room)) {
      send_to_char("The thought of leaving your master makes you weep.\n\r", ch);
      return;
    }
    
    if (IS_SET(wag->obj_flags.value[3], WAGON_NO_ENTER) && 
        !is_char_ethereal(ch) &&
        !IS_IMMORTAL(ch)) {
      act("You can't find a way into $p.", FALSE, ch, wag, 0, TO_CHAR);
      return;
    }
    
    mt = ch->specials.riding;
    sub = ch->specials.subduing;
    
    if (!handle_tunnel(room, ch)) {
      return;
    }
    
    if (IS_SET(room->room_flags, RFL_NO_MOUNT) && 
        (mt)) {
      send_to_char("You can't take a mount there.\n\r", ch);
      return;
    }
    
    /* can they fit based on max_size? */
    if (mt) {
      if ((wag->obj_flags.value[5] > 0 &&
           get_char_size(mt) > wag->obj_flags.value[5]) &&
          !IS_IMMORTAL(mt)) {
        send_to_char("Your mount is too big to enter that.\n\r", ch);
        return;
      }
    }
    
    if (IS_SET(room->room_flags, RFL_NO_MOUNT) && 
        IS_SET(ch->specials.act, CFL_MOUNT)) {
      send_to_char("You can't fit in there, you're a mount!\n\r", ch);
      if (ch->master) {
        act("$N can't fit in $p and stops following you.", FALSE, ch->master, wag, ch, TO_CHAR);
        parse_command(ch, "follow self");
      }
      return;
    }
    
    if (IS_SET(room->room_flags, RFL_NO_MOUNT) && 
        sub && 
        IS_SET(sub->specials.act, CFL_MOUNT)) {
      send_to_char("You can't drag a mount in there!\n\r", ch);
    }
    
    if (ch->lifting && ch->lifting->in_room == ch->in_room) {
      if (GET_OBJ_WEIGHT(ch->lifting) / 2 > get_lifting_capacity(ch->lifting->lifted_by)) {
        act("You try and drag $p, but it's too heavy.", 0, ch, ch->lifting, 0, TO_CHAR);
        act("$n strains, trying to drag $p, but it doesn't move.", 0, ch, ch->lifting, 0, TO_ROOM);
        return;
      }
    }
    
    if (ch->lifting && IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
      roomlogf(ch->in_room, "%s is lifting, removing hide affect.", MSTR(ch, name));
      REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
    }
    
    if (obj_guarded(ch, wag, CMD_ENTER)) {
      return;
    }
    
    /* had to do this to allow sneak to work */
    for (tch = ch->in_room->people; tch; tch = t_next_in_room) {
      t_next_in_room = tch->next_in_room;
      
      if (GET_SPEED(ch) == SPEED_SNEAKING || IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
        sneak = succeed_sneak(ch, tch, sneak_roll);
      } else {
        sneak = FALSE;
      }
      
      if (mt) {
        if (mt == tch) {
          sprintf(tmp, "$n has entered %s, riding you.", OSTR(wag, short_descr));
          act(tmp, TRUE, ch, 0, tch, TO_VICT);
        } else if (ch == tch) {
          sprintf(tmp, "You enter %s, riding %s.", OSTR(wag, short_descr), PERS(tch, mt));
          act(tmp, TRUE, ch, 0, 0, TO_CHAR);
        } else {
          sprintf(tmp, "$n has entered %s, riding %s.", OSTR(wag, short_descr), PERS(tch, mt));
          act(tmp, TRUE, ch, 0, tch, TO_VICT);
        }
      } else {
        if (tch == ch) {
          sprintf(tmp, "You enter %s.", OSTR(wag, short_descr));
          act(tmp, FALSE, ch, 0, 0, TO_CHAR);
        }
        /* sneaking check, no sneak while subduing */
        else if (!sneak || sub) {
          sprintf(tmp, "$n enters %s.", OSTR(wag, short_descr));
          act(tmp, TRUE, ch, 0, tch, TO_VICT);
        }
      }
      
      if (sub) {
        if (sub == tch) {
          sprintf(tmp, "You are dragged into %s by %s.", OSTR(wag, short_descr), PERS(tch, ch));
          act(tmp, TRUE, sub, 0, 0, TO_CHAR);
        } else if (ch == tch) {
          sprintf(tmp, "You drag %s in as well.", PERS(tch, sub));
          act(tmp, TRUE, ch, 0, 0, TO_CHAR);
        } else {
          sprintf(tmp, "$n drags %s in as well.", PERS(tch, sub));
          act(tmp, TRUE, ch, 0, tch, TO_VICT);
        }
      } else if (ch->lifting && ch != tch) {
        sprintf(tmp, "$n %s %s behind $m.",
                GET_OBJ_WEIGHT(ch->lifting) > get_lifting_capacity(ch->lifting->lifted_by)
                ? "drags" : "carries", format_obj_to_char(ch->lifting, tch, 1));
        act(tmp, 0, ch, 0, tch, TO_VICT);
      }
      
      if ((IS_SET(ch->in_room->room_flags, RFL_SPL_ALARM) && (!IS_IMMORTAL(ch)))) {
        sector = ch->in_room->sector_type;
        
        switch (sector) {
        case SECT_SOUTHERN_CAVERN:
        case SECT_CAVERN:
          send_to_room("A deep rumbling comes from the stony walls.\n\r", ch->in_room);
          break;
        case SECT_DESERT:
          send_to_room("The sands underfoot shift with an odd chiming sound.\n\r", ch->in_room);
          break;
        case SECT_HILLS:
        case SECT_MOUNTAIN:
          send_to_room("Rocks stir underfoot with a groan of noise..\n\r", ch->in_room);
          break;
        case SECT_SILT:
          send_to_room("The silt underfoot roils, an odd chiming sound momentarily filling the air.\n\r", ch->in_room);
          break;
        case SECT_THORNLANDS:
          send_to_room("The thorny bushes all around rustle as the ground beneath them stirs.\n\r", ch->in_room);
          break;
        default:
          send_to_room("A brief ringing sound fills the air.\n\r", ch->in_room);
          break;
        }
        remove_alarm_spell(ch->in_room);
      }
    }
    
    if (ch->lifting && ch->lifting->in_room == ch->in_room) {
      sprintf(tmp, "You %s %s behind you.",
              GET_OBJ_WEIGHT(ch->lifting) > get_lifting_capacity(ch->lifting->lifted_by)
              ? "drag" : "carry", format_obj_to_char(ch->lifting, ch, 1));
      act(tmp, 0, ch, 0, 0, TO_CHAR);
    }
    
    if (is_char_watching(ch) && is_char_watching_dir(ch, DIR_OUT)) {
      stop_watching(ch, FALSE);
    }
    
    was_in = ch->in_room;
    char_from_room_move(ch);
    char_to_room(ch, get_room_num(wag->obj_flags.value[0]));
    cmd_look(ch, "", -1, 0);
    
    
    if (mt) {
      char_from_room_move(mt);
      char_to_room(mt, get_room_num(wag->obj_flags.value[0]));
      ch->specials.riding = mt;
      cmd_look(mt, "", -1, 0);
    }
    
    if (sub) {
      char_from_room_move(sub);
      char_to_room(sub, get_room_num(wag->obj_flags.value[0]));
      cmd_look(sub, "", -1, 0);
    }
    
    /* added this to allow sneak */
    for (tch = ch->in_room->people; tch; tch = t_next_in_room) {
      t_next_in_room = tch->next_in_room;
      if (tch == ch || tch == mt || tch == sub) {
        continue;
      }
      
      
      if (GET_SPEED(ch) == SPEED_SNEAKING || IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
        sneak = succeed_sneak(ch, tch, sneak_roll);
      } else {
        sneak = FALSE;
      }
      
      /* Vortex check */
      if (obj_index[wag->nr].vnum == 73001) {      /* true for a vortex */
        vortex = TRUE;
      } else {
        vortex = FALSE;
      }
      
      if (mt) {
        if (vortex) {
          sprintf(tmp, "$n fades into existence, riding %s", PERS(tch, mt));
        } else {
          sprintf(tmp, "$n has entered %s, riding %s", OSTR(wag, short_descr), PERS(tch, mt));
        }
        act(tmp, TRUE, ch, 0, tch, TO_VICT);
      } else {
        /* sneaking check, no sneak while subduing or mounted */
        if (!sneak || sub) {
          if (vortex) {
            strcpy(tmp, "$n fades into existence.");
          } else {
            sprintf(tmp, "$n has entered %s.", OSTR(wag, short_descr));
          }
          act(tmp, TRUE, ch, 0, tch, TO_VICT);
        }
      }
      
      if (sub) {
        sprintf(tmp, "$n drags %s in as well.", PERS(tch, sub));
        act(tmp, TRUE, ch, 0, tch, TO_VICT);
      } else if (ch->lifting && ch != tch) {
        sprintf(tmp, "$n %s %s behind $m.",
                GET_OBJ_WEIGHT(ch->lifting) > get_lifting_capacity(ch->lifting->lifted_by)
                ? "drags" : "carries", format_obj_to_char(ch->lifting, tch, 1));
        act(tmp, 0, ch, 0, tch, TO_VICT);
      }
    }
    
    if (ch->lifting && ch->lifting->in_room != ch->in_room) {
      PLYR_LIST *pPl, *pPlNext;
      int obj_weight, capacity;
      
      obj_from_room(ch->lifting);
      obj_to_room(ch->lifting, get_room_num(wag->obj_flags.value[0]));
      
      for (pPl = ch->lifting->lifted_by; pPl; pPl = pPlNext) {
        pPlNext = pPl->next;
        
        if (was_in == pPl->ch->in_room &&
            GET_POS(pPl->ch) >= POSITION_STANDING) {
          obj_weight = GET_OBJ_WEIGHT(ch->lifting);
          capacity = get_lifting_capacity(ch->lifting->lifted_by);
          
          sprintf(buf, "You help $N %s $p, and enter %s.",
                  (obj_weight / 100) < capacity ? "carry" : "drag",
                  format_obj_to_char(wag, pPl->ch, 1));
          
          act(buf, FALSE, pPl->ch, ch->lifting, ch, TO_CHAR);
          
          cmd++;
          send_to_char("\n\r", pPl->ch);
          cmd_enter(pPl->ch, backup, cmd, 0);
        }
      }
    }                       /*here */
    
    for (k = ch->followers; k; k = next_dude) {
      next_dude = k->next;
      if (was_in == k->follower->in_room && GET_POS(k->follower) >= POSITION_STANDING) {
        cmd_enter(k->follower, backup, cmd, 0);
      }
    }
    return;
  } else {
    send_to_char("You can't seem to find anything to enter.\n\r", ch);
  }
}


void
cmd_leave(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
    OBJ_DATA *wagon;
    CHAR_DATA *mount, *sub, *target_ch;
    char edesc[256];
    char command[100];
    char buf[100];
    struct follow_type *k, *next_dude;
    struct room_data *was_in;
    bool sneak;
    int sector;
    int sneak_roll = number(1, 101);
    
    if (GET_POS(ch) == POSITION_FIGHTING) {
      send_to_char("But you are in the middle of combat!  Use flee.\n\r", ch);
      return;
    }
    
    /* Can't leave your master */
    if (is_charmed(ch) &&
        ch->in_room == ch->master->in_room) {
      send_to_char("The thought of leaving your master makes you weep.\n\r", ch);
      return;
    }
    
    // TODO Document this, whatever it is.
    if (get_char_extra_desc_value(ch, "[ARENA_RETURN_ROOM_VALUE]", edesc, sizeof(edesc))) {
      char_from_room(ch);
      char_to_room(ch, get_room_num(atoi(edesc)));
      act("$n has entered the world.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You return to the game.\n\r", ch);
      parse_command(ch, "look");
      rem_char_extra_desc_value(ch, "[ARENA_RETURN_ROOM_VALUE]");
      return;
    }
    
    // TODO Document this, whatever it is.
    if (get_char_extra_desc_value(ch, "[LOC_DESC_RETURN_ROOM_VALUE]", edesc, sizeof(edesc))) {
      char_from_room(ch);
      char_to_room(ch, get_room_num(atoi(edesc)));
      act("$n has entered the world.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You return to the game.\n\r", ch);
      parse_command(ch, "look");
      rem_char_extra_desc_value(ch, "[LOC_DESC_RETURN_ROOM_VALUE]");
      return;
    }
    
    wagon = find_exitable_wagon_for_room(ch->in_room);
    
    if (!wagon) {
      send_to_char("But you're not inside anything.\n\r", ch);
      return;
    }
    
    if (wagon) {
      if (IS_SET(wagon->obj_flags.value[3], WAGON_NO_LEAVE) && 
          !is_char_ethereal(ch) &&
          !IS_IMMORTAL(ch)) {
        send_to_char("You can't seem to leave!\n\r", ch);
        return;
      }
      
      if (!wagon->in_room) {
        send_to_char("You can't seem to leave!\n\r", ch);
        return;
      }
      
      if (out_guarded(ch)) {
        return;
      }
      
      if (ch->lifting && ch->lifting->in_room == ch->in_room) {
        if (IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
          REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
        }
        
        if (GET_OBJ_WEIGHT(ch->lifting) / 2 > get_lifting_capacity(ch->lifting->lifted_by)) {
          act("You try and drag $p, but it's too heavy.", 0, ch, ch->lifting, 0, TO_CHAR);
          act("$n strains, trying to drag $p, but it doesn't move.", 0, ch, ch->lifting, 0, TO_ROOM);
          return;
        }
      }
      
      mount = ch->specials.riding;
      sub = ch->specials.subduing;
      
      for (target_ch = ch->in_room->people; target_ch; target_ch = target_ch->next_in_room) {
        if (target_ch == ch) {
          continue;
        }
        
        if (GET_SPEED(ch) == SPEED_SNEAKING || 
            IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
          sneak = succeed_sneak(ch, target_ch, sneak_roll);
        } else {
          sneak = FALSE;
        }
        
        if (mount) {
          if (mount != target_ch) {
            sprintf(buf, "$n leaves %s, riding %s.", OSTR(wagon, short_descr), MSTR(mount, short_descr));
            act(buf, TRUE, ch, 0, target_ch, TO_VICT);
          } else {
            sprintf(buf, "$n leaves %s, riding you.", OSTR(wagon, short_descr));
            act(buf, TRUE, ch, 0, target_ch, TO_VICT);
          }
        } else {
          /* sneak check, no sneaking while subduing */
          if (!sneak || sub) {
            sprintf(buf, "$n leaves %s.", OSTR(wagon, short_descr));
            act(buf, TRUE, ch, 0, target_ch, TO_VICT);
          }
        }
        
        if (sub) {
          if (sub != target_ch) {
            // TODO:  Does this reveal a hooded character's sdesc?
            sprintf(buf, "$n drags %s out as well.", MSTR(sub, short_descr));
            act(buf, TRUE, ch, 0, target_ch, TO_VICT);
          } else {
            sprintf(buf, "$n drags you out as well.");
            act(buf, TRUE, ch, 0, target_ch, TO_VICT);
          }
        }
        
        if (ch->lifting && ch != target_ch) {
          sprintf(buf, "$n %s %s behind $m.",
                  GET_OBJ_WEIGHT(ch->lifting) > get_lifting_capacity(ch->lifting->lifted_by)
                  ? "drags" : "carries", format_obj_to_char(ch->lifting, target_ch, 1));
          act(buf, 0, ch, 0, target_ch, TO_VICT);
        }
        
        if ((IS_SET(ch->in_room->room_flags, RFL_SPL_ALARM) && (!IS_IMMORTAL(ch)))) {
          sector = ch->in_room->sector_type;
          
          switch (sector) {
          case SECT_SOUTHERN_CAVERN:
          case SECT_CAVERN:
            send_to_room("A deep rumbling comes from the stony walls.\n\r", ch->in_room);
            break;
          case SECT_DESERT:
            send_to_room("The sands underfoot shift with an odd chiming sound.\n\r", ch->in_room);
            break;
          case SECT_HILLS:
          case SECT_MOUNTAIN:
            send_to_room("Rocks stir underfoot with a groan of noise..\n\r", ch->in_room);
            break;
          case SECT_SILT:
            send_to_room("The silt underfoot roils, an odd chiming sound momentarily filling the air.\n\r", ch->in_room);
            break;
          case SECT_THORNLANDS:
            send_to_room("The thorny bushes all around rustle as the ground beneath them stirs.\n\r", ch->in_room);
            break;
          default:
            send_to_room("A brief ringing sound fills the air.\n\r", ch->in_room);
            break;
          }
          remove_alarm_spell(ch->in_room);
        }
      }
      
      if (ch->lifting && ch->lifting->in_room == ch->in_room) {
        sprintf(buf, "You %s %s behind you.",
                GET_OBJ_WEIGHT(ch->lifting) > get_lifting_capacity(ch->lifting->lifted_by)
                ? "drag" : "carry", format_obj_to_char(ch->lifting, ch, 1));
        act(buf, 0, ch, 0, 0, TO_CHAR);
      }
      
      if (is_char_watching_dir(ch, DIR_OUT)) {
        stop_watching(ch, FALSE);
      }

      was_in = ch->in_room;
      char_from_room_move(ch);
      char_to_room(ch, wagon->in_room);
      
      if (mount) {
        char_from_room_move(mount);
        char_to_room(mount, wagon->in_room);
        cmd_look(mount, "", 15, 0);
      }
      
      if (sub) {
        char_from_room_move(sub);
        char_to_room(sub, wagon->in_room);
        cmd_look(sub, "", 15, 0);
      }
      
      for (target_ch = ch->in_room->people; target_ch; target_ch = target_ch->next_in_room) {
        if (target_ch == ch || 
            mount == target_ch || 
            sub == target_ch) {
          continue;
        }
        
        if (GET_SPEED(ch) == SPEED_SNEAKING || 
            IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
          sneak = succeed_sneak(ch, target_ch, sneak_roll);
        } else
          sneak = FALSE;
        
        if (mount) {
          sprintf(buf, "$n emerges from %s, riding %s.", OSTR(wagon, short_descr), MSTR(mount, short_descr));
          act(buf, TRUE, ch, 0, target_ch, TO_VICT);
        } else {
          if (!sneak || sub) {
            sprintf(buf, "$n emerges from %s.", OSTR(wagon, short_descr));
            act(buf, TRUE, ch, 0, target_ch, TO_VICT);
          }
        }
        
        if (sub) {
          sprintf(buf, "$n drags %s out as well.", MSTR(sub, short_descr));
          act(buf, TRUE, ch, 0, target_ch, TO_VICT);
        }
        
        if (ch->lifting && ch != target_ch) {
          sprintf(buf, "$n %s %s behind $m.",
                  GET_OBJ_WEIGHT(ch->lifting) > get_lifting_capacity(ch->lifting->lifted_by)
                  ? "drags" : "carries", format_obj_to_char(ch->lifting, target_ch, 1));
          act(buf, 0, ch, 0, target_ch, TO_VICT);
        }
      }
      
      send_to_char("You step out to...\n\r\n\r", ch);
      strcpy(command, "");
      cmd_look(ch, command, 15, 0);
      
      if (ch->lifting && ch->lifting->in_room != ch->in_room) {
        PLYR_LIST *pPl, *pPlNext;
        int obj_weight, capacity;
        
        obj_from_room(ch->lifting);
        obj_to_room(ch->lifting, wagon->in_room);
        
        for (pPl = ch->lifting->lifted_by; pPl; pPl = pPlNext) {
          pPlNext = pPl->next;
          
          if (was_in == pPl->ch->in_room &&
              GET_POS(pPl->ch) >= POSITION_STANDING) {
            obj_weight = GET_OBJ_WEIGHT(ch->lifting);
            capacity = get_lifting_capacity(ch->lifting->lifted_by);
            
            sprintf(buf, "You help $N %s $p, and leave %s.",
                    (obj_weight / 100) < capacity ? "carry" : "drag",
                    format_obj_to_char(wagon, pPl->ch, 1));
            
            act(buf, FALSE, pPl->ch, ch->lifting, ch, TO_CHAR);
            
            cmd++;
            send_to_char("\n\r", pPl->ch);
            cmd_leave(pPl->ch, argument, cmd, 0);
          }
        }
      }
      
      if (ch->followers) {    /* if character is being followed */
        for (k = ch->followers; k; k = next_dude) { /* as long as there are followers */
          next_dude = k->next;
          if (was_in == k->follower->in_room &&
              GET_POS(k->follower) >= POSITION_STANDING) {
            cmd_leave(k->follower, argument, cmd, 0);
          }
        }                   /* as long as there are followers */
      }                       /* End if character is being followed */
      return;
    }
}


void
cmd_stand(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  char buf[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH];
  CHAR_DATA *mount;
  
  if (strstr(argument, "nostand") != 0 && IS_NPC(ch)) {
    char edesc[256];
    if (!get_char_extra_desc_value(ch, "[NO_STAND]", edesc, sizeof(edesc))) {
      send_to_char("This NPC will not stand automatically, when you return.\n\r", ch);
      set_char_extra_desc_value(ch, "[NO_STAND]", "1");
    } else {
      send_to_char("This NPC stands automatically, when you return.\n\r", ch);
      rem_char_extra_desc_value(ch, "[NO_STAND]");
    }
    
    return;
  }
  
  /* extract how they are sitting */
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

  argument = one_argument(args, arg1, sizeof(arg1));
  
  /* stand <mount> */
  if (*arg1 && (mount = get_char_room_vis(ch, arg1))) {
    if (!IS_SET(mount->specials.act, CFL_MOUNT)) {
      cprintf(ch, "%s is not a proper mount.\n\r", capitalize(PERS(ch, mount)));
      return;
    } else if (mount_owned_by_someone_else(ch, mount)) {
      /* messages sent by function */
      return;
    }
    
    if (!mount_owned_by_ch(ch, mount)) {
      cprintf(ch, "%s isn't yours to make stand.\n\r", capitalize(PERS(ch, mount)));
      return;
    }
    
    if (GET_POS(mount) == POSITION_STANDING) {
      cprintf(ch, "%s is already standing.\n\r", capitalize(PERS(ch, mount)));
      return;
    }
    
    if (GET_POS(mount) == POSITION_SLEEPING) {
      cprintf(ch, "You need to wake %s first.\n\r", PERS(ch, mount));
      return;
    }
    
    if (GET_POS(mount) < POSITION_SLEEPING) {
      cprintf(ch, "%s is in no shape to stand up.\n\r", capitalize(PERS(ch, mount)));
      return;
    }
    
    if (GET_POS(ch) != POSITION_STANDING) {
      cprintf(ch, "You need to be standing to order %s to stand.\n\r", PERS(ch, mount));
      return;
    }
    
    sprintf(to_char, "you pull on %%%s reins", arg1);
    sprintf(to_room, "@ pulls on %%%s reins", arg1);
    
    if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
      return;
    }
    
    cmd_stand(mount, "", CMD_STAND, 0);
    
    sprintf(buf, "%s stands %s.\n\r", MSTR(ch, name), MSTR(mount, short_descr));
    send_to_monitor(ch, buf, MONITOR_POSITION);
    
    return;
  }
  
  /* call helper to see if they are positioning on an object,
   * if it succeeds, stop processing
   */
  if (position_ch_on_obj(ch, preHow, args, postHow, cmd)) {
    return;
  }
  
  switch (GET_POS(ch)) {
  case POSITION_STANDING:
    if (ch->on_obj) {
      if (!show_remove_occupant(ch, preHow, postHow)) {
        return;
      }
    } else {
      act("You are already standing.", FALSE, ch, 0, 0, TO_CHAR);
    }
    break;
    
  case POSITION_SITTING:
    /* call generic stand from burrow function */
    if (stand_from_burrow(ch, FALSE)) {
      return;
    }
    
    if (ch->on_obj) {
      if (!show_remove_occupant(ch, preHow, postHow)) {
        return;
      }
    } else {
      sprintf(to_char, "you stand up");
      sprintf(to_room, "@ stands up");
      
      if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
        return;
      }
    }
    
    if (ch->specials.fighting) {
      set_char_position(ch, POSITION_FIGHTING);
    } else {
      set_char_position(ch, POSITION_STANDING);
    }
    
    break;
    
  case POSITION_RESTING:
    /* call generic stand from burrow function */
    if (stand_from_burrow(ch, FALSE)) {
      return;
    }
    
    if (ch->on_obj) {
      if (!show_remove_occupant(ch, preHow, postHow)) {
        return;
      }
    } else {
      /* generate a message for the actor (ch) */
      
      if (preHow[0] != '\0' || postHow[0] != '\0') {
        sprintf(to_char, "you rise and stand");
        sprintf(to_room, "@ rises and stands");
      } else {
        sprintf(to_char, "you stop resting, and stand up");
        sprintf(to_room, "@ rises from the ground, and clambers to ^me feet");
      }
      
      if (!send_to_char_and_room_parsed (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
        return;
      }
    }
    
    set_char_position(ch, POSITION_STANDING);
    break;
    
  case POSITION_SLEEPING:
    act("You have to wake up first!", FALSE, ch, 0, 0, TO_CHAR);
    break;
    
  case POSITION_FIGHTING:
    act("Do you not consider fighting as standing?", FALSE, ch, 0, 0, TO_CHAR);
    break;
    
  default:
    if (!IS_NPC(ch)) {
      sprintf(buf, "Error: cmd_stand: %s (%s) was in bad position '%d'", ch->name, ch->desc->player_info->name, GET_POS(ch));
    } else {
      sprintf(buf, "Error: cmd_stand: %s [M%d] was in bad position '%d'",
              MSTR(ch, short_descr), npc_index[ch->nr].vnum, GET_POS(ch));
    }
    gamelog(buf);
    
    set_char_position(ch, POSITION_STANDING);
    act("You stop floating around, and put your feet on the ground.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  }
  WIPE_LDESC(ch);
}


void
cmd_sit(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  char buf[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  
  if (ch->specials.subduing) {
    act("Not while holding onto $N!", FALSE, ch, 0, ch->specials.subduing, TO_CHAR);
    return;
  }

  if (ch->lifting) {
    sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
    send_to_char(buf, ch);
    if (!drop_lifting(ch, ch->lifting)) {
      return;
    }
  }
  
  if (ch_starts_falling_check(ch)) {
    return;
  }

  if (ch->in_room->sector_type == SECT_AIR) {
    act("You can't sit in mid-air!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
  if (affected_by_spell(ch, AFF_MUL_RAGE)) {
    send_to_char("The blood-lust inside you is too strong for you to sit down.\n\r", ch);
    return;
  }
  
  /* extract how they are sitting */
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  /* call helper to see if they are positioning on an object,
   * if it succeeds, stop processing
   */
  if (position_ch_on_obj(ch, preHow, args, postHow, cmd)) {
    return;
  }
  
  switch (GET_POS(ch)) {
  case POSITION_STANDING:
    if (ch->specials.riding) {
      act("You swing your legs over, and jump off of $N.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
      act("$n swings $s legs over and jumps off of $N.", FALSE, ch, 0, ch->specials.riding, TO_ROOM);
      act("$n swings $s legs over and jumps off of you.", FALSE, ch, 0, ch->specials.riding, TO_VICT);
      sprintf(buf, "%s dismounts.\n\r", MSTR(ch, name));
      send_to_monitor(ch, buf, MONITOR_POSITION);
      
      ch->specials.riding = 0;
    }
    
    if (affected_by_spell(ch, SPELL_LEVITATE)) {
      act("You stop floating.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops floating.", FALSE, ch, 0, 0, TO_ROOM);
      
      sprintf(buf, "%s stops floating.\n\r", MSTR(ch, name));
      send_to_monitor(ch, buf, MONITOR_POSITION);
      affect_from_char(ch, SPELL_LEVITATE);
    }
    
    /* generate a message for the actor (ch) */
    sprintf(to_char, "you sit down");
    sprintf(to_room, "@ sits down");
    
    if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
      return;
    }
    
    set_char_position(ch, POSITION_SITTING);
    break;
    
  case POSITION_SITTING:
    send_to_char("You're sitting already.\n\r", ch);
    break;
    
  case POSITION_RESTING:
    /* generate a message for the actor (ch) */
    sprintf(to_char, "you stop resting, and sit up");
    sprintf(to_room, "@ stops resting");
    
    if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
      return;
    }
    
    set_char_position(ch, POSITION_SITTING);
    break;
    
  case POSITION_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
    
  case POSITION_FIGHTING:
    act("Sit down while fighting?", FALSE, ch, 0, 0, TO_CHAR);
    break;
    
  default:
    if (!IS_NPC(ch)) {
      sprintf(buf, "Error: cmd_sit: %s (%s) was in bad position '%d'", ch->name, ch->desc->player_info->name, GET_POS(ch));
    } else {
      sprintf(buf, "Error: cmd_sit: %s [M%d] was in bad position '%d'", 
              MSTR(ch, short_descr), npc_index[ch->nr].vnum, GET_POS(ch));
    }
    gamelog(buf);
    
    act("You stop floating around, and sit down.", FALSE, ch, 0, 0, TO_CHAR);
    set_char_position(ch, POSITION_SITTING);
    break;
  }
  
  WIPE_LDESC(ch);
}


void
cmd_rest(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *mount;
  
  if (ch->specials.subduing) {
    cprintf(ch, "Not while holding onto %s!\n\r", PERS(ch, ch->specials.subduing));
    return;
  }
  
  if (is_char_watching_any_dir(ch)) {
    stop_watching(ch, FALSE);
  }
  
  if (ch->lifting) {
    cprintf(ch, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
    if (!drop_lifting(ch, ch->lifting)) {
      return;
    }
  }
  
  if (ch_starts_falling_check(ch)) {
    return;
  }
  
  if (ch->in_room->sector_type == SECT_AIR) {
    cprintf(ch, "You can't rest in mid-air!\n\r");
    return;
  }
  
  if (affected_by_spell(ch, AFF_MUL_RAGE)) {
    cprintf(ch, "The blood-lust inside you is too strong for you to lie down.\n\r");
    return;
  }
  
  /* extract how they are sitting */
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  argument = one_argument(args, arg1, sizeof(arg1));
  
  /* rest <mount> */
  if (*arg1 && (mount = get_char_room_vis(ch, arg1))) {
    if (!IS_SET(mount->specials.act, CFL_MOUNT)) {
      cprintf(ch, "%s is not a proper mount.\n\r", capitalize(PERS(ch, mount)));
      return;
    } else if (mount_owned_by_someone_else(ch, mount)) {
      /* messages sent by function */
      return;
    }
    
    if (!mount_owned_by_ch(ch, mount)) {
      cprintf(ch, "%s isn't yours to rest.\n\r", capitalize(PERS(ch, mount)));
      return;
    }
    
    if (ch->specials.riding == mount) {
      sprintf(to_char, "you swing your legs over and jump off of ~%s", arg1);
      sprintf(to_room, "@ swings ^me legs over and jumps off of ~%s", arg1);
    } else {
      sprintf(to_char, "you pull on %%%s reins", arg1);
      sprintf(to_room, "@ pulls on %%%s reins", arg1);
    }
    
    if (!send_to_char_and_room_parsed
        (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION))
      return;
    
    if (affected_by_spell(mount, SPELL_LEVITATE)) {
      affect_from_char(mount, SPELL_LEVITATE);
      act("$N floats to the ground, and curls up.", FALSE, ch, 0, mount, TO_ROOM);
      act("$N floats to the ground, and curls up.", FALSE, ch, 0, mount, TO_CHAR);
    } else {
      act("$N curls up on the ground.", FALSE, ch, 0, mount, TO_ROOM);
      act("$N curls up on the ground.", FALSE, ch, 0, mount, TO_CHAR);
    }
    
    sprintf(buf, "%s rests %s.\n\r", MSTR(ch, name), MSTR(mount, short_descr));
    send_to_monitor(ch, buf, MONITOR_POSITION);
    
    set_char_position(mount, POSITION_RESTING);
    
    if (ch->specials.riding == mount) {
      set_char_position(ch, POSITION_STANDING);
      
      ch->specials.riding = 0;
    }
    return;
  }
  
  /* call helper to see if they are positioning on an object,
   * if it succeeds, stop processing   */
  if (position_ch_on_obj(ch, preHow, args, postHow, cmd)) {
    return;
  }
  
  switch (GET_POS(ch)) {
  case POSITION_STANDING:
    if (ch->specials.riding) {
      find_ch_keyword(ch->specials.riding, ch, arg1, sizeof(arg1));
      
      if (!*arg1) {
        sprintf(to_char, "you swing your legs over and jump off of your mount");
        sprintf(to_room, "@ swings ^me legs over and jumps off of your mount");
      } else {
        sprintf(to_char, "you swing your legs over and jump off of ~%s", arg1);
        sprintf(to_room, "@ swings ^me legs over and jumps off of ~%s", arg1);
      }
      
      if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
        return;
      }
      
      if (affected_by_spell(ch->specials.riding, SPELL_LEVITATE)) {
        affect_from_char(ch->specials.riding, SPELL_LEVITATE);
        act("$N floats to the ground, and curls up.", FALSE, ch, 0, ch->specials.riding, TO_ROOM);
        act("$N floats to the ground, and curls up.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
      } else {
        act("$N curls up on the ground.", FALSE, ch, 0, ch->specials.riding, TO_ROOM);
        act("$N curls up on the ground.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
      }
      
      sprintf(buf, "%s dismounts and rests %s.\n\r", MSTR(ch, name), MSTR(ch->specials.riding, short_descr));
      send_to_monitor(ch, buf, MONITOR_POSITION);
      
      set_char_position(ch->specials.riding, POSITION_RESTING);
      
      set_char_position(ch, POSITION_STANDING);
      
      ch->specials.riding = 0;
      return;
    } else {
      if (affected_by_spell(ch, SPELL_LEVITATE)) {
        sprintf(to_char, "you float to the ground to rest");
        sprintf(to_room, "@ floats to the ground and sits down");
        affect_from_char(ch, SPELL_LEVITATE);
        send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION);
      } else {
        /* generate a message for the actor (ch) */
        sprintf(to_char, "you sit down and rest your tired bones");
        sprintf(to_room, "@ sits down to rest");
        
        if (!send_to_char_and_room_parsed
            (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
          return;
        }
      }
    }
    set_char_position(ch, POSITION_RESTING);
    break;
    
  case POSITION_SITTING:
    /* generate a message for the actor (ch) */
    if (preHow[0] != '\0' || postHow[0] != '\0') {
      sprintf(to_char, "you lie down to rest");
      sprintf(to_room, "@ lies down to rest");
    } else {
      sprintf(to_char, "you rest your tired bones");
      sprintf(to_room, "@ lies down on the ground and rests");
    }
    
    if (!send_to_char_and_room_parsed (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
      return;
    }
    set_char_position(ch, POSITION_RESTING);
    break;
    
  case POSITION_RESTING:
    act("You are already resting.", FALSE, ch, 0, 0, TO_CHAR);
    break;
    
  case POSITION_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
    
  case POSITION_FIGHTING:
    act("Rest while fighting?", FALSE, ch, 0, 0, TO_CHAR);
    break;
    
  default:
    if (!IS_NPC(ch)) {
      sprintf(buf, "Error: cmd_rest: %s (%s) was in bad position '%d'", 
              ch->name, ch->desc->player_info->name, GET_POS(ch));
    } else {
      sprintf(buf, "Error: cmd_rest: %s [M%d] was in bad position '%d'",
              MSTR(ch, short_descr), npc_index[ch->nr].vnum, GET_POS(ch));
    }
    gamelog(buf);
    
    act("You stop floating around, and stop to rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
    set_char_position(ch, POSITION_RESTING);
    break;
  }
  WIPE_LDESC(ch);
}



void
cmd_sleep(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  
  if (GET_RACE(ch) == RACE_MANTIS) {
    send_to_char("You're a mantis...Mantises don't need to sleep.\n\r", ch);
    return;
  }
  
  if (ch->specials.subduing) {
    act("Not while holding onto $N!", FALSE, ch, 0, ch->specials.subduing, TO_CHAR);
    return;
  }
  
  if (is_char_watching(ch)) {
    stop_watching(ch, FALSE);
  }
  
  if (ch->specials.riding) {
    send_to_char("Not while riding.\n\r", ch);
    return;
  }
  
  if (affected_by_spell(ch, SPELL_INSOMNIA)) {
    send_to_char("You have too much energy to sleep.\n\r", ch);
    return;
  }
  
  if (affected_by_spell(ch, AFF_MUL_RAGE)) {
    send_to_char("The blood-lust inside you is too strong for you to sleep.\n\r", ch);
    return;
  }
  
  if (affected_by_spell(ch, SPELL_IMMOLATE)) {
    send_to_char("The flames immolating your body make it impossible to sleep.\n\r", ch);
    return;
  }
  
  if (ch->in_room->sector_type == SECT_AIR) {
    act("You can't sleep in mid-air!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
  if ((GET_COND(ch, FULL) < 2 && 
       !IS_NPC(ch)) && 
      !IS_IMMORTAL(ch)) {
    send_to_char("Your hunger pains prevent you from sleeping.\n\r", ch);
    return;
  }
  
  if (((GET_COND(ch, THIRST) < 2) && (!IS_NPC(ch))) && !IS_IMMORTAL(ch)) {
    send_to_char("You are too dehydrated to sleep.\n\r", ch);
    return;
  }
  
  if (ch->lifting) {
    sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
    send_to_char(buf, ch);
    if (!drop_lifting(ch, ch->lifting)) {
      return;
    }
  }
  
  /* extract how they are sitting */
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  /* call helper to see if they are positioning on an object,
   * if it succeeds, stop processing */
  if (position_ch_on_obj(ch, preHow, args, postHow, cmd)) {
    return;
  }
  
  switch (GET_POS(ch)) {
  case POSITION_STANDING:
  case POSITION_SITTING:
  case POSITION_RESTING:
    
    if (affected_by_spell(ch, SPELL_LEVITATE) || 
        (affected_by_spell(ch, SPELL_FLY))) {
      act("You stop floating around and lie down to sleep.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops floating around and lies down to sleep.", FALSE, ch, 0, 0, TO_ROOM);
      
      sprintf(buf, "%s stops floating and lies down to sleep.\n\r", MSTR(ch, name));
      send_to_monitor(ch, buf, MONITOR_POSITION);
      
      if (affected_by_spell(ch, SPELL_LEVITATE)) {
        affect_from_char(ch, SPELL_LEVITATE);
      }
      if (affected_by_spell(ch, SPELL_FLY)) {
        affect_from_char(ch, SPELL_FLY);
      }
    } else if (ch_starts_falling_check(ch)) {
      return;
    } else {
      /* generate a message for the actor (ch) */
      sprintf(to_char, "you go to sleep");
      sprintf(to_room, "@ lies down and falls asleep");
      
      if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_POSITION)) {
        return;
      }
    }
    
    snprintf(buf, sizeof(buf), "%s goes to sleep.", MSTR(ch, short_descr));
    send_to_empaths(ch, buf);
    set_char_position(ch, POSITION_SLEEPING);
    set_tracks_sleeping(ch, ch->in_room);
    break;
    
  case POSITION_SLEEPING:
    send_to_char("You are already sound asleep.\n\r", ch);
    break;
    
  case POSITION_FIGHTING:
    send_to_char("Sleep while fighting?\n\r", ch);
    break;
    
  default:
    if (!IS_NPC(ch)) {
      sprintf(buf, "Error: cmd_sleep: %s (%s) was in bad position '%d'", ch->name,
              ch->desc->player_info->name, GET_POS(ch));
    } else {
      sprintf(buf, "Error: cmd_sleep: %s [M%d] was in bad position '%d'",
              MSTR(ch, short_descr), npc_index[ch->nr].vnum, GET_POS(ch));
    }
    gamelog(buf);
    
    act("You stop floating around, and lie down to sleep.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and lies down to sleep.", TRUE, ch, 0, 0, TO_ROOM);
    set_char_position(ch, POSITION_SLEEPING);
    set_tracks_sleeping(ch, ch->in_room);
    break;
  }
  WIPE_LDESC(ch);
}

void
cmd_fly(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  if (!(GET_RACE_TYPE(ch) == RTYPE_AVIAN_FLYING || 
        GET_RACE(ch) == RACE_WYVERN ||
        (ch->specials.riding && 
         (GET_RACE_TYPE(ch->specials.riding) == RTYPE_AVIAN_FLYING ||
          GET_RACE(ch->specials.riding) == RACE_WYVERN)))) {
    if (ch->specials.riding) {
      act("You try as hard as you can, but you can't get $N to fly.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
    } else {
      act("$n jumps up and down and pretends to fly.", FALSE, ch, 0, 0, TO_ROOM);
      act("You jump up and down, pretending to fly.", FALSE, ch, 0, 0, TO_CHAR);
    }
    return;
  }
  if (GET_RACE_TYPE(ch) == RTYPE_AVIAN_FLYING || 
      GET_RACE(ch) == RACE_WYVERN) {
    if (IS_AFFECTED(ch, CHAR_AFF_FLYING)) {
      send_to_char("But you are already flying.\n\r", ch);
      return;
    }
    if (ch->specials.riding && (GET_RACE_TYPE(ch->specials.riding) != RTYPE_AVIAN_FLYING &&
                                GET_RACE(ch->specials.riding) != RACE_WYVERN)) {
      act("You leap off of $N's back, and begin to fly.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
      act("$n leaps off of $N's back, and begins to fly.", FALSE, ch, 0, ch->specials.riding, TO_ROOM);
      ch->specials.riding = 0;
    } else {
      act("You leap into the air, and begin flying.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n leaps into the air, and begins flying.", FALSE, ch, 0, 0, TO_ROOM);
    }
    MUD_SET_BIT(ch->specials.affected_by, CHAR_AFF_FLYING);
    return;
  }
  if (ch->specials.riding && 
      (GET_RACE_TYPE(ch->specials.riding) == RTYPE_AVIAN_FLYING ||
       GET_RACE(ch->specials.riding) == RACE_WYVERN)) {
    if (IS_AFFECTED(ch->specials.riding, CHAR_AFF_FLYING)) {
      act("But $N is already flying.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
    } else {
      act("$n barks a command at $N, who begins flying.", FALSE, ch, 0, ch->specials.riding, TO_ROOM);
      act("You pull on the reins, and $N, begins flying.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
      MUD_SET_BIT(ch->specials.riding->specials.affected_by, CHAR_AFF_FLYING);
    }
    return;
  }
  
  gamelog("Error in CMD_FLY");
}

void
cmd_land(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  int in_the_air = 0;

  if (is_flying_char(ch) > FLY_FEATHER) {
    act("You gently drop to the ground.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n gently drops to the ground.", FALSE, ch, 0, 0, TO_ROOM);
    
    if (affected_by_spell(ch, SPELL_LEVITATE)) {
      affect_from_char(ch, SPELL_LEVITATE);
    }
    
    if (affected_by_spell(ch, SPELL_FLY)) {
      affect_from_char(ch, SPELL_FLY);
    }
    
    if (IS_AFFECTED(ch, CHAR_AFF_FLYING)) {   /* In case they can fly w/o a spell */
      REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_FLYING);     
    }

    in_the_air = 1;
  }
  
  if (ch->specials.riding && 
      is_flying_char(ch->specials.riding) > FLY_FEATHER) {
    act("You have $N land upon the ground.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
    act("$n lands $N upon the ground.", FALSE, ch, 0, ch->specials.riding, TO_ROOM);
    if (affected_by_spell(ch->specials.riding, SPELL_LEVITATE)) {
      affect_from_char(ch->specials.riding, SPELL_LEVITATE);
    }
    if (affected_by_spell(ch->specials.riding, SPELL_FLY)) {
      affect_from_char(ch->specials.riding, SPELL_FLY);
    }
    /* Mounts that can fly w/o a spell */
    if (IS_AFFECTED(ch->specials.riding, CHAR_AFF_FLYING)) {
      REMOVE_BIT(ch->specials.riding->specials.affected_by, CHAR_AFF_FLYING);
    }
    in_the_air = 1;
  }
  if (in_the_air == 0) {
    send_to_char("Land, but you aren't flying!\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->in_room->room_flags, RFL_FALLING)) {
    fallch(ch, 0);
  }
  
  return;
}

void
cmd_wake(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  CHAR_DATA *tmp_char;
  char arg[MAX_STRING_LENGTH];
  char message[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  
  /* extract how they are sitting */
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  one_argument(args, arg, sizeof(arg));
  
  if (*arg) {
    if (GET_POS(ch) == POSITION_SLEEPING) {
      send_to_char("You can't wake people up if you are asleep yourself!\n\r", ch);
      return;
    } else {
      tmp_char = get_char_room_vis(ch, arg);
      if (tmp_char) {
        if (tmp_char == ch) {
          send_to_char("If you want to wake yourself up, just type 'wake'.\n\r", ch);
          return;
        } else {
          if (GET_POS(tmp_char) == POSITION_SLEEPING) {
            if (IS_AFFECTED(tmp_char, CHAR_AFF_SLEEP) &&
                !IS_IMMORTAL(ch)) {
              act("You cannot wake $M up!", FALSE, ch, 0, tmp_char, TO_CHAR);
              return;
            } else {
              sprintf(buf, "you wake %s up", HMHR(tmp_char));
              prepare_emote_message(preHow, buf, postHow, message);
              
              /* if their emote didn't parse, fail out */
              if (!send_to_char_parsed(ch, ch, message, FALSE)) {
                return;
              }
              
              /* let the room see 
               * note that since the victim is asleep, they
               * won't see the message, handy, huh?
               */
              sprintf(buf, "@ wakes ~%s up", arg);
              prepare_emote_message(preHow, buf, postHow, message);
              send_to_room_parsed(ch, ch->in_room, message, FALSE);
              
              if (IS_IMMORTAL(ch) &&
                  IS_AFFECTED(tmp_char, CHAR_AFF_SLEEP)) {
                if (affected_by_spell(tmp_char, SPELL_SLEEP)) {
                  affect_from_char(tmp_char, SPELL_SLEEP);
                }
                REMOVE_BIT(tmp_char->specials.affected_by, CHAR_AFF_SLEEP);
              }
              set_char_position(tmp_char, POSITION_SITTING);
              prepare_emote_message(preHow, "@ wakes you up", postHow, message);
              send_to_char_parsed(ch, tmp_char, message, FALSE);
            }
          } else
            act("$N is already awake.", FALSE, ch, 0, tmp_char, TO_CHAR);
          return;
        }
      } else {
        send_to_char("You do not see that person here.\n\r", ch);
        return;
      }
    }
  } else {    /* typed wake without an argument */
    if (IS_AFFECTED(ch, CHAR_AFF_SLEEP)) {
      send_to_char("You can't wake up!\n\r", ch);
    } else {
      if (GET_POS(ch) > POSITION_SLEEPING) {
        send_to_char("You are already awake...\n\r", ch);
      } else {
        if ((GET_MOVE(ch)) < (GET_MAX_MOVE(ch) * .75)) {
          send_to_char("You are too tired to wake just now.\n\r", ch);
        } else {
          OBJ_DATA *on_obj = NULL;
          
          /* prepare the message with pre/post how as specified */
          prepare_emote_message(preHow, "you wake, and sit up", postHow, message);
          
          /* if the send fails, return */
          if (!send_to_char_parsed(ch, ch, message, FALSE)) {
            return;
          }
          
          /* pepare and send a message to the room */
          prepare_emote_message(preHow, "@ awakens.", postHow, message);
          send_to_room_parsed(ch, ch->in_room, message, FALSE);
          
          if (ch->on_obj && IS_SET(ch->on_obj->obj_flags.value[1], FURN_SIT)) {
            on_obj = ch->on_obj;
          }
          set_char_position(ch, POSITION_SITTING);
          
          if (on_obj != NULL) {
            add_occupant(on_obj, ch);
          }
        }
      }
    }
  }
}

void
cmd_follow(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  char name[160];
  char buf2[256];
  CHAR_DATA *leader = (CHAR_DATA *) 0;
  OBJ_DATA *leader_obj = (OBJ_DATA *) 0;
  
  argument = one_argument(argument, name, sizeof(name));
  
  if (!(*name)) {
    if (ch->master) {
      act("You are following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    } else {
      if (ch->obj_master) {
        act("You are following $p.", FALSE, ch, ch->obj_master, 0, TO_CHAR);
      } else {
        send_to_char("Who do you wish to follow?\n\r", ch);
      }
    }
    return;
  }
  
  leader = get_char_room_vis(ch, name);
  if (!leader) {
    leader_obj = get_obj_in_list_vis(ch, name, ch->in_room->contents);
    if (!leader_obj) {
      send_to_char("I see no person by that name here!\n\r", ch);
      return;
    }
  }
  
  if (is_charmed(ch)) {
    act("And leave $N's side? Never!", FALSE, ch, 0, ch->master, TO_CHAR);
    return;
  }
  
  if (leader_obj) {
    send_to_char("You can only follow people.\n\r", ch);
    return;
  } else {
    if (leader == ch) {
      if (IS_IMMORTAL(ch)) {
        if (ch->master) {
          sprintf(buf2, "You are following %s.\n\r", PERS(ch, ch->master));
          send_to_char(buf2, ch);
        }
        if (ch->obj_master) {
          sprintf(buf2, "You are following %s.\n\r", OSTR(ch->obj_master, short_descr));
          send_to_char(buf2, ch);
        }
      }
      if ((!ch->master) && (!ch->obj_master)) {
        send_to_char("You are already following yourself.\n\r", ch);
        return;
      }
      
      send_to_char("You are no longer following anyone.\n\r", ch);
      stop_follower(ch);
      if (cmd == CMD_FOLLOW) {
        act("$n is no longer following you.", TRUE, ch, 0, leader, TO_VICT);
      }
    } else {
      if (new_circle_follow_ch_ch(ch, leader)) {
        act("Sorry, but following in loops is not allowed.", FALSE, ch, 0, 0, TO_CHAR);
        return;
      }
      
      if ((ch->master) || (ch->obj_master)) {
        stop_follower(ch);
      }
      
      add_follower(ch, leader);
    }
  }
}

void
fallch(CHAR_DATA * ch, int num_rooms) {
  struct room_data *was_in;
  int phys_dam, stun_dam, i;
  char buf[MAX_STRING_LENGTH];
  
  if (!ch ||
      !ch->in_room ||
      !ch->in_room->direction[DIR_DOWN] ||
      !ch->in_room->direction[DIR_DOWN]->to_room) {
    return;
  }

  /* only a 1/3 of a chance that feather fall people will 'fall' */
  if (affected_by_spell(ch, SPELL_FEATHER_FALL) && number(0, 2)) {
    return;
  }
  
  if (affected_by_spell(ch, SPELL_FEATHER_FALL)) {
    if (ch->in_room->sector_type != SECT_AIR_PLANE) {
      act("$n float to the ground below...", TRUE, ch, 0, 0, TO_ROOM);
      act("You float to the ground below...", FALSE, ch, 0, 0, TO_CHAR);
    } else {
      act("$n float down through the air...", TRUE, ch, 0, 0, TO_ROOM);
      act("You float down through the air...", FALSE, ch, 0, 0, TO_CHAR);
    }
  } else {
    if (ch->in_room->sector_type != SECT_AIR_PLANE) {
      act("$n plummets to the ground below...", TRUE, ch, 0, 0, TO_ROOM);
      act("You plummet to the ground below...", FALSE, ch, 0, 0, TO_CHAR);
    } else {
      act("$n plummets down through the air...", TRUE, ch, 0, 0, TO_ROOM);
      act("You plummet down through the air...", FALSE, ch, 0, 0, TO_CHAR);
    }
    
  }
  
  if (num_rooms == 0)
    num_rooms = 1;
  
  was_in = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, was_in->direction[DIR_DOWN]->to_room);
  
  
  
#ifndef DONT_DO_SPECIAL_FALL
  /* Call out the CMD_TO_ROOM special hook if they */
  /* Wind up falling into this room, if function   */
  /* returns FALSE, then they don't fall           */
  /* Remove #ifdef if still in place by 2006/5/01  */
  /*  2005/11/28 -Tenbones                         */
  if (!no_specials && special(ch, CMD_TO_ROOM, "fall")) {
    /* If they died in the special, this might do bad things */
    char_from_room(ch);
    char_to_room(ch, was_in);
  } else
#endif
    {
      
      
      cmd_look(ch, "", -1, 0);
      
      act("$n falls in from above...", TRUE, ch, 0, 0, TO_ROOM);
      
      if (IS_SET(ch->in_room->room_flags, RFL_DEATH)) {
        sprintf(buf, "%s %s%s%sentered into room %d and died of the death flag.", 
                GET_NAME(ch),
                !IS_NPC(ch) ? "(" : "", 
                !IS_NPC(ch) ? ch->account : "", 
                !IS_NPC(ch) ? ") " : "",
                ch->in_room->number);
        
        if (!IS_NPC(ch)) {
          struct time_info_data playing_time =
            real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
          
          if ((playing_time.day > 0 || 
               playing_time.hours > 2  ||
               (ch->player.dead && !strcmp(ch->player.dead, "rebirth"))) &&
              !IS_SET(ch->specials.act, CFL_NO_DEATH)) {
            if (ch->player.info[1]) {
              free(ch->player.info[1]);
            }
            ch->player.info[1] = strdup(buf);
          }
          gamelog(buf);
        } else
          shhlog(buf);
        
        die(ch);
      } else if (!affected_by_spell(ch, SPELL_FEATHER_FALL) &&
                 !will_char_fall_in_room(ch, ch->in_room)) {
        if (damage_absorbed_by_invulnerability(ch)) {
          // no-op
        } else {
          phys_dam = stun_dam = 0;
          
          for (i = 1; i <= num_rooms; i++) {
            phys_dam += (number(1, 5) * i) + 15;
            stun_dam += number(20, 50);
          }
          
          if (generic_damage(ch, phys_dam, 0, 0, stun_dam)) { /* they lived from the fall */
            if (GET_POS(ch) > POSITION_SLEEPING) {
              set_char_position(ch, POSITION_RESTING);
            }
          } else {
            sprintf(buf, "%s %s%s%sfell into room %d and died from the fall.", GET_NAME(ch),
                    !IS_NPC(ch) ? "(" : "", !IS_NPC(ch) ? ch->account : "",
                    !IS_NPC(ch) ? ") " : "", ch->in_room->number);
            
            if (!IS_NPC(ch) &&
                !affected_by_spell(ch, SPELL_POSSESS_CORPSE)) {
              struct time_info_data playing_time =
                real_time_passed((time(0) - ch->player.time.logon) +
                                 ch->player.time.played,
                                 0);
              
              if ((playing_time.day > 0 || 
                   playing_time.hours > 2 ||
                   (ch->player.dead && !strcmp(ch->player.dead, "rebirth"))) &&
                  !IS_SET(ch->specials.act, CFL_NO_DEATH)) {
                if (ch->player.info[1]) {
                  free(ch->player.info[1]);
                }
                ch->player.info[1] = strdup(buf);
              }
              
              death_log(buf);
            } else
              shhlog(buf);
            
            die(ch);
          }
          
        }
      }
    }
}

void
fallob(OBJ_DATA * ob) {
  char buf[256];
  struct room_data *going_to;
  
  if (!IS_SET(ob->obj_flags.extra_flags, OFL_NO_FALL)) {
    sprintf(buf, "%s plummets to the ground below...\n\r", OSTR(ob, short_descr));
    *buf = toupper(*buf);
    send_to_room(buf, ob->in_room);
    
    going_to = ob->in_room->direction[5]->to_room;
    
    if(ob->obj_flags.type == ITEM_LIGHT && IS_LIT(ob)) {
      decrement_room_light(ob->in_room);
    }

    obj_from_room(ob);
    obj_to_room(ob, going_to);
    
    if(ob->obj_flags.type == ITEM_LIGHT && IS_LIT(ob)) {
      increment_room_light(going_to, ob->obj_flags.value[4]);
    }
    
    sprintf(buf, "%s falls from above...\n\r", OSTR(ob, short_descr));
    *buf = toupper(*buf);
    send_to_room(buf, ob->in_room);
  }
}

int
lag_time(CHAR_DATA * ch) {
  int base, base_walk, base_run, base_sneak;
  
  switch (GET_RACE(ch)) {
  case RACE_ELF:
  case RACE_HALF_ELF:
    base_sneak = 6;
    base_walk = 5;
    base_run = 3;
    break;
  case RACE_ERDLU:
    base_sneak = 3;
    base_walk = 3;
    base_run = 1;
    break;
  case RACE_THLON:
    base_sneak = 4;
    base_walk = 4;
    base_run = 2;
    break;
  case RACE_ASLOK:
    base_sneak = 12;
    base_walk = 11;
    base_run = 9;
    break;
  case RACE_HALF_GIANT:
    base_sneak = 8;
    base_walk = 6;
    base_run = 5;
    break;
  case RACE_GWOSHI:
    base_sneak = 3;
    base_walk = 3;
    base_run = 2;
    break;
  case RACE_HORSE:
  case RACE_KANK:
    base_sneak = 4;
    base_walk = 4;
    base_run = 2;
    break;
  case RACE_RATLON:
    base_sneak = 6;
    base_walk = 3;
    base_run = 1;
    break;
  case RACE_SUNLON:
  case RACE_SUNBACK:
  case RACE_MEKILLOT:
    base_sneak = 12;
    base_walk = 10;
    base_run = 8;
    break;
  case RACE_CHEOTAN:
  case RACE_INIX:
    base_sneak = 5;
    base_walk = 5;
    base_run = 3;
    break;
  default:
    base_sneak = 8;
    base_walk = 7;
    base_run = 4;
    break;
  }
  
  if (affected_by_spell(ch, SPELL_LEVITATE)) {
    base = 3;
  } else if ((is_flying_char(ch) == FLY_SPELL) || (is_flying_char(ch) == FLY_NATURAL)) {
    base = 2;
  } else {
    switch (GET_SPEED(ch)) {
    case SPEED_WALKING:
      base = base_walk;
      break;
    case SPEED_RUNNING:
      base = base_run;
      break;
    case SPEED_SNEAKING:
      base = base_sneak;
      break;
    default:
      base = base_walk;
      break;
    }
  }
  
  if (ch->lifting) {
    base = base_sneak;
  }
  
  if (IS_AFFECTED(ch, CHAR_AFF_GODSPEED) && GET_SPEED(ch) == SPEED_RUNNING) {
        base -= 8;
  }

  if (IS_AFFECTED(ch, CHAR_AFF_SLOW)) {
        base += 4;
  }
  
  if (ch->in_room->sector_type == SECT_DESERT) {
    if (!((is_flying_char(ch) == FLY_SPELL) || (is_flying_char(ch) == FLY_NATURAL))) {
      base += 4;
    }
  }
  
  /* If victim is affected by the Rooted ability, severe movement penalty */
  if (find_ex_description("[ROOTED]", ch->ex_description, TRUE)) {
    base *= 3;
  }
  
  base = MAX(base, 0);
  
  return (base * 2);
}

int
stamina_loss(CHAR_DATA * ch) {
  int loss = 0, penalty = armor_penalties(ch);
  
  if (!ch->in_room) {
    return 0;
  }
  
  loss += movement_loss[(int) ch->in_room->sector_type];
  
  if ((ch->in_room->sector_type == SECT_FIRE_PLANE && (GET_GUILD(ch) != GUILD_FIRE_ELEMENTALIST)) ||
      (ch->in_room->sector_type == SECT_WATER_PLANE && (GET_GUILD(ch) != GUILD_WATER_ELEMENTALIST)) ||
      (ch->in_room->sector_type == SECT_EARTH_PLANE && (GET_GUILD(ch) != GUILD_STONE_ELEMENTALIST)) ||
      (ch->in_room->sector_type == SECT_AIR_PLANE && (GET_GUILD(ch) != GUILD_WIND_ELEMENTALIST)) ||
      (ch->in_room->sector_type == SECT_SHADOW_PLANE && (GET_GUILD(ch) != GUILD_SHADOW_ELEMENTALIST)) ||
      (ch->in_room->sector_type == SECT_NILAZ_PLANE && (GET_GUILD(ch) != GUILD_VOID_ELEMENTALIST)) ||
      (ch->in_room->sector_type == SECT_LIGHTNING_PLANE && (GET_GUILD(ch) != GUILD_LIGHTNING_ELEMENTALIST))) {
    loss += 2;
  }
  
  if (ch->specials.subduing) {
    loss += 2;
  }
  
  if (GET_SPEED(ch) == SPEED_RUNNING && !IS_AFFECTED(ch, CHAR_AFF_GODSPEED)) {
    loss += ((GET_RACE(ch) == RACE_ELF) ? 2 : 3);
  }
  
  /* Desert elves can run easier than walking.. */
  if ((GET_SPEED(ch) == SPEED_RUNNING) && (GET_RACE(ch) == RACE_DESERT_ELF)) {
    loss -= 5;
  }
  
  /* so can ghatti, but I'm lazy so doing it in a separate section. */
  if ((GET_SPEED(ch) == SPEED_RUNNING) && (GET_RACE(ch) == RACE_GHAATI)) {
    loss -= 5;
  }
  
  if ((GET_COND(ch, FULL) < 5) && (GET_COND(ch, FULL) > -1)) {
    loss += 1;
  }
  
  if ((GET_COND(ch, THIRST) < 5) && (GET_COND(ch, THIRST) > -1)) {
    loss += 2;
  }
  
  if (((GET_RACE(ch) == RACE_HALFLING) || (GET_RACE(ch) == RACE_CHEOTAN)) &&
      ((ch->in_room->sector_type == SECT_LIGHT_FOREST) || (ch->in_room->sector_type == SECT_HEAVY_FOREST))) {
    loss -= 8;
  }
  
  if ((ch->in_room->sector_type == SECT_DESERT) || (ch->in_room->sector_type == SECT_SALT_FLATS)) {
    loss += (penalty * 2);
  } else if ((ch->in_room->sector_type == SECT_LIGHT_FOREST) ||
             (ch->in_room->sector_type == SECT_THORNLANDS) ||
             (ch->in_room->sector_type == SECT_RUINS) ||
             (ch->in_room->sector_type == SECT_SOUTHERN_CAVERN) ||
             (ch->in_room->sector_type == SECT_CAVERN) ||
             (ch->in_room->sector_type == SECT_SHALLOWS)) {
    loss += (penalty * 2);
  } else if (ch->in_room->sector_type == SECT_HEAVY_FOREST) {
    loss += (penalty * 2);
  } else if (ch->in_room->sector_type == SECT_ROAD) {
    loss += (penalty * 3) / 4;
  } else if (ch->in_room->sector_type == SECT_CITY) {
    loss += (penalty * 2) / 3;
  } else if (ch->in_room->sector_type == SECT_INSIDE) {
    loss += (penalty / 2);
  } else {
    loss += penalty; 
  }
  
  if ((GET_RACE_TYPE(ch) == RTYPE_REPTILIAN) && 
      (ch->in_room->sector_type == SECT_DESERT)) {
    loss = (loss * 2) / 3;
  }
  
  if ((GET_RACE(ch) == RACE_TEMBO) &&
      ((ch->in_room->sector_type == SECT_LIGHT_FOREST) || (ch->in_room->sector_type == SECT_HEAVY_FOREST))) {
    loss = (loss * 2) / 3;
  }
  
  if ((GET_RACE_TYPE(ch) == RTYPE_INSECTINE) && 
      (ch->in_room->sector_type != SECT_DESERT)) {
    loss = (loss * 2) / 3;
  }

  if (GET_RACE(ch) == RACE_MANTIS) {
    loss = (loss * 2) / 3;  /* double good for mantises */
  }
  
  if ((ch->in_room->sector_type == SECT_SILT) && IS_SET(ch->specials.act, CFL_SILT_GUY)) {
    loss = (loss / 3);
  }
  
  loss += MAX(get_encumberance(ch) - 5, 0);
  
  if ((ch->in_room->sector_type != SECT_CITY) && 
      (ch->in_room->sector_type != SECT_INSIDE)) {
    loss = MAX(1, loss);
  } else {
    loss = MAX(0, loss);
  }
  
  if (affected_by_spell(ch, SPELL_LEVITATE)) {
    loss = MIN(2, loss);
  }
  
  if (GET_RACE(ch) == RACE_MUL && affected_by_spell(ch, AFF_MUL_RAGE)) {
    loss = MIN(2, loss);
  }
  
  if (is_flying_char(ch) == FLY_SPELL || 
      is_flying_char(ch) == FLY_NATURAL) {
    if (ch->in_room->sector_type == SECT_AIR || 
        ch->in_room->sector_type == SECT_AIR_PLANE) {
      loss = MIN(1, loss);
    } else {
      loss = MIN(4, loss);
    }
  }
  
  /* Tribes can run all day */
  // TODO is this defined?
#if defined(OLD_ELF_RUN)
  if ((GET_RACE(ch) == RACE_ELF) && (GET_SPEED(ch) == SPEED_RUNNING) && ((IS_TRIBE(ch, TRIBE_BLACKWING)) ||   /* Blackwing    */
                                                                         (IS_TRIBE(ch, TRIBE_SUN_RUNNERS)) || /* Sun Runners */
                                                                         (IS_TRIBE(ch, TRIBE_SEVEN_SPEARS)))) /* Seven Spears */
    loss = MIN(2, loss);
#endif
  
  if (is_char_ethereal(ch)) {
    loss = 0;
  }
  
  /* If victim is affected by the Rooted ability, severe movement penalty */
  if (find_ex_description("[ROOTED]", ch->ex_description, TRUE)) {
    act("You find it very hard to move while rooted.", FALSE, ch, 0, 0, TO_CHAR);
    loss *= 4;
    loss = MAX(10, loss);
  }
  
  return loss;
}

int
armor_penalties(CHAR_DATA * ch) {
  OBJ_DATA *armor;
  int total, penalty, pos;
  
  total = 0;
  for (pos = 0; pos < MAX_WEAR; pos++) {
    if ((armor = ch->equipment[pos]) && (armor->obj_flags.material == MATERIAL_METAL)) {
      total += armor->obj_flags.weight / 100;
    }
  }
  
  if ((ch->in_room) &&
      ((ch->in_room->sector_type == SECT_CITY) || (ch->in_room->sector_type == SECT_INSIDE))){
    penalty = (total / 40);
  } else {
    penalty = (total / 20);
  }
  
  return penalty;
}

/* Begin wagon map functions - hal */

/* the following four functions maintain a stack of room numbers to
   be visited in the wagon */

struct room_stack {
  int max_size;
  int cur_size;
  struct room_data **rooms;
};

struct room_stack wagon_stack;

void
init_room_stack(int size) {
  wagon_stack.rooms = (struct room_data **) malloc(size * sizeof *wagon_stack.rooms);
  wagon_stack.max_size = size;
  wagon_stack.cur_size = 0;
  
  return;
}

void
free_room_stack(void) {
  free(wagon_stack.rooms);
}

int
push_room(struct room_data *room) {
  if (wagon_stack.cur_size >= wagon_stack.max_size) {
    return 0;
  }
  
  wagon_stack.rooms[wagon_stack.cur_size] = room;
  wagon_stack.cur_size++;;
  
  return 1;
}

struct room_data *
pop_room(void) {
  if (wagon_stack.cur_size == 0) {
    return NULL;
  }
  
  wagon_stack.cur_size--;
  return wagon_stack.rooms[wagon_stack.cur_size];
}

/* end stack routines */

/* The following function adds a room number to the room visited list
   and returns 1 if successful.  If the room has already been added,
   it returns 0. If the room would exceed the maximum number of rooms
   the functions is allowed to visit, it returns -1.  To reset the
   function for the first room visisted, send it a room number of -1. */

#define MAX_WAGON_ROOMS 100

int
mark_room_visit(int room) {
  static int rooms[MAX_WAGON_ROOMS];
  static int num_rooms;
  int i;
  
  if (room == -1) {
    num_rooms = 0;
    return 1;
  }
  
  if (num_rooms == MAX_WAGON_ROOMS) {
    return -1;
  }
  
  for (i = 0; (i < num_rooms) && (rooms[i] != room); i++);
  
  if (i < num_rooms) {
    return 0;
  }
  
  rooms[num_rooms] = room;
  num_rooms++;
  
  return 1;
}

void
room_to_send(struct room_data *room, char *messg) {
  send_to_room(messg, room);
}

/* map_wagon_rooms traverses the rooms of a wagon, performing the
   function fun on each room.  Fun should be a function of type void
   with the following declaration:

   void fun (struct room_data *, void *);

   The first argument passed to fun will be a room in the wagon.  Fun
   will only be called once for each room in the wagon.  The third
   argument to map_wagon_rooms will be sent as the second argument to
   the given fun on each call.  map_wagon_rooms will return 1 if it
   succesfully traverses all of the rooms in the wagon and 0
   otherwise. */

int
map_wagon_rooms(OBJ_DATA * wagon, void (*fun) (struct room_data *, void *), void *arg) {
  struct room_data *entrance, *room, *exit;
  ROOM_DATA *tmp_room;
  int mark, i;
  char buf[256];
  
  /* make sure that the object is a wagon object */
  if (wagon->obj_flags.type != ITEM_WAGON) {
    sprintf(buf, "Tried to call map_wagon_rooms on non-wagon obj %d",
            obj_index[wagon->nr].vnum);
    gamelog(buf);
    return 0;
  }
  
  tmp_room = get_room_num(wagon->obj_flags.value[0]);
  
  if (!tmp_room) {
    sprintf(buf, "Tried to call map_wagon_rooms on wagon %d, but it has no entrance room.",
            obj_index[wagon->nr].vnum);
    gamelog(buf);
    return 0;
  }
  
  /* initialize the stack and the list of rooms visited */
  init_room_stack(MAX_WAGON_ROOMS);
  mark_room_visit(-1);
  
  /* push the entrance room on to the stack */
  entrance = get_room_num(wagon->obj_flags.value[0]);
  push_room(entrance);
  mark_room_visit(entrance->number);
  
  /* loop until the stack is empty */
  while ((room = pop_room())) {
    /* call fun on the current room */
    (*fun) (room, arg);
    
    /* push all the rooms connected to this one that have not been
     * visited yet onto the stack */
    for (i = 0; i < 6; i++) {
      if (!room->direction[i]) {
        continue;
      }
      exit = room->direction[i]->to_room;
      mark = mark_room_visit(exit->number);
      if (mark == 1) {    /* if the room hasn't been visited */
        if (!push_room(exit)) {
          gamelog("Too many rooms on the wagon room stack");
          return 0;
        }
      } else if (mark == -1) {    /* if visited rooms > max */
        sprintf(buf, "Maximum number of rooms exceeded in wagon %d", obj_index[wagon->nr].vnum);
        gamelog(buf);
        return 0;
      }
      /* else if (mark == 0) do nothing, since the room has already
       * been visited */
    }
  }
  
  free_room_stack();
  return 1;
  
}

/* end map wagon functions */

void
count_wagon_room(struct room_data *room, void *arg) {
  int *acc = (int *) arg;
  
  *acc = *acc + 1;
  
  return;
}

void
wagon_room_send(struct room_data *room, char *msg) {
  send_to_room(msg, room);
  return;
}

void
add_room_weight(struct room_data *room, void *arg) {
  CHAR_DATA *ch;
  OBJ_DATA *obj;
  
  // Compiler says this is unused. -Nessalin 10/14/2000
  //  char buf[MAX_STRING_LENGTH]; 
  
  int rm = 0, i;
  int *acc = (int *) arg;
  
  /* add weight of empty room */
  /* This should be set in the objects overall weight */
  i = get_room_max_weight(room);
  rm += (i * 10);
  /*
    sprintf(buf, "Room: %d adding %d weight", room->number, i);
    gamelog(buf);
  */
  
  /* add weight of people in room */
  for (ch = room->people; ch; ch = ch->next_in_room) {
    rm += (ch->player.weight * 10) + calc_carried_weight(ch);
  }
  
  /* add weight of objects in room */
  for (obj = room->contents; obj; obj = obj->next_content) {
    rm += calc_object_weight(obj);
  }
  
  *acc += (rm / 10);
  
  return;
}

void
add_room_weight_no_people(struct room_data *room, void *arg) {
  /*  char buf[MAX_STRING_LENGTH];  */
  int rm = 0, i;
  int *acc = (int *) arg;
  
  /* add weight of empty room */
  /* This should be set in the objects overall weight */
  i = get_room_max_weight(room);
  rm += i;
  /*
    sprintf(buf, "Room: %d adding %d weight", room->number, i);
    gamelog(buf);
  */
  *acc += (rm * 10);
  
  return;
}


int
get_wagon_delay(OBJ_DATA * wagon) {
  int lag = 0;
  
  lag = MAX(lag / 4, 1);
  
  return (lag);
}


/* Code used for those driving wagons */
void
cmd_pilot(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  CHAR_DATA *tmp_char = 0;
  OBJ_DATA *wagon;            /* the wagon being driven */
  PLYR_LIST *sCh, *sChNext;
  struct room_data *new_room, /* destination room       */
    *was_in;                   /* original room          */
  int dir, num_rooms = 0, weight = 0;
  int pilot_skill;
  int lag = 0;
  char arg1[256], buf[256], name[256];
  char buf2[MAX_STRING_LENGTH];
  char command[MAX_INPUT_LENGTH];
  char oldarg[MAX_INPUT_LENGTH];
  
  extern bool is_in_command_q_with_cmd(CHAR_DATA * ch, int cmd);
  
  strcpy(oldarg, argument);
  
  if (is_char_ethereal(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("Your hands pass through the reins.\n\r", ch);
    return;
  }
  
  if (!ch->in_room) {
    send_to_char("But you aren't inside anything!\n\r", ch);
    if (IS_IMMORTAL(ch)) {
      send_to_char("(!ch->in_room error.\n\r", ch);
    }
    return;
  }
  
  wagon = find_pilotable_wagon_for_room(ch->in_room);
  if (!wagon) {
    if (ch->on_obj) {
      if (ch->on_obj->obj_flags.type == ITEM_FURNITURE) {
        if (IS_SET(ch->on_obj->obj_flags.value[1], FURN_SKIMMER) ||
            IS_SET(ch->on_obj->obj_flags.value[1], FURN_WAGON)) {
          wagon = ch->on_obj;
        }
      }
    }
  }
  
  if (!wagon) {
    send_to_char("But you aren't inside anything!\n\r", ch);
    return;
  }
  
  if (!has_skill(ch, SKILL_PILOT)) {
    if (number(1, 100) <= wis_app[GET_WIS(ch)].learn) {
      init_skill(ch, SKILL_PILOT, 5);
      
      if (get_max_skill(ch, SKILL_PILOT) <= 0) {
        set_skill_taught(ch, SKILL_PILOT);
      }
    } else {
      send_to_char("You know nothing of driving wagons.\n\r", ch);
      return;
    }
  }

  if (wagon->obj_flags.type == ITEM_FURNITURE) {
    if (wagon->occupants && wagon->occupants->ch) {
      for (sCh = wagon->occupants; sCh; sCh = sCh->next) {
        if (AWAKE(sCh->ch) && (sCh->ch->desc || IS_NPC(sCh->ch))) {
          tmp_char = sCh->ch;
        }
      }
      
      if (!(ch == tmp_char)) {
        if (IS_SET(wagon->obj_flags.value[1], FURN_SKIMMER)) {
          send_to_char("Only the first person on the skimmer can pilot it.\n\r", ch);
        } else {
          send_to_char("Only the first person on the wagon can pilot it.\n\r", ch);
        }
        return;
      }
      
      /* Allow for different types of furniture movement */
      /* First, look at skimmers ... */
      if (IS_SET(wagon->obj_flags.value[1], FURN_SKIMMER)) {
        /* Skimmers can only move on silt and shallow rooms */
        if (!(ch->in_room->sector_type == SECT_SILT ||
              ch->in_room->sector_type == SECT_SHALLOWS)) {
          send_to_char("You can't pilot a skimmer anywhere but the silt and some shallows.\n\r", ch);
          return;
        }
      } else {
        /* other furniture movement types would go here */
        /* otherwise... */
        act("You can't seem to control $p.", FALSE, ch, wagon, 0, TO_CHAR);
        return;
      }
      
      argument = one_argument(argument, arg1, sizeof(arg1));
      
      dir = (old_search_block(arg1, 0, strlen(arg1), dirs, 0));
      
      if ((dir < CMD_NORTH) || (dir > CMD_DOWN)) {
        sprintf(buf, "You can't pilot the %s to do that.\n\r", first_name(OSTR(wagon, name), buf2));
        send_to_char(buf, ch);
        return;
      }
      
      pilot_skill = (has_skill(ch, SKILL_PILOT) 
                     ? ch->skills[SKILL_PILOT]->learned : 0);
      
      if (number(1, 60) > pilot_skill) {
        act("You can't seem to control $p.", FALSE, ch, wagon, 0, TO_CHAR);
        if (!(number(0, 3))) {
          dir = number(1, 4);
        } else {
          return;
        }
        gain_skill(ch, SKILL_PILOT, (GET_GUILD(ch) == GUILD_MERCHANT) ? 3 : 1);
      }
      
      dir--;
      
      if (!wagon->in_room->direction[dir] ||
          IS_SET(wagon->in_room->direction[dir]->exit_info, EX_CLOSED) ||
          IS_SET(wagon->in_room->direction[dir]->to_room->room_flags, RFL_NO_WAGON)) {
        send_to_char("You can't go in that direction.\n\r", ch);
        return;
      }
      
      new_room = wagon->in_room->direction[dir]->to_room;
      
      if (IS_SET(wagon->obj_flags.value[1], FURN_SKIMMER)) {
        if (!(new_room->sector_type == SECT_SILT || new_room->sector_type == SECT_SHALLOWS)) {
          act("You can't pilot there, the $p will only go into silt or shallows.", FALSE, ch, wagon, 0, TO_CHAR);
          return;
        }
        
        if (new_room->sector_type == SECT_AIR) {
          send_to_char("You can't go into the air!\n\r", ch);
          return;
        }
      }
      /* insert move checks for other furniture-based vehicles here */
      
      strcpy(name, OSTR(wagon, short_descr));
      *name = toupper(*name);
      
      if (!no_specials && special(ch, CMD_FROM_ROOM, "pilot")) {
        return;
      }
      
      sprintf(buf, "%s leaves %s.\n", name, dirs[dir]);
      send_to_room(buf, wagon->in_room);
      
      was_in = wagon->in_room;
      obj_from_room(wagon);
      obj_to_room(wagon, new_room);
      
      if (!no_specials && special(ch, CMD_TO_ROOM, "pilot")) {
        obj_from_room(wagon);
        obj_to_room(wagon, was_in);
      }
      
      if (dir < DIR_UP) {
        sprintf(buf, "%s has arrived from the %s.\n\r", name, dirs[rev_dir[dir]]);
      } else if (dir == DIR_DOWN) {
        sprintf(buf, "%s has arrived from above.\n\r", name);
      } else {
        sprintf(buf, "%s has arrived from below.\n\r", name);
      }
      
      send_to_room(buf, wagon->in_room);
      
      for (sCh = wagon->occupants; sCh; sCh = sChNext) {
        CHAR_DATA *sch = sCh->ch;
        
        sChNext = sCh->next;
        
        if ((IS_SET(wagon->obj_flags.value[1], FURN_SKIMMER) ||
             IS_SET(wagon->obj_flags.value[1], FURN_WAGON)) &&
            GET_POS(sch) == POSITION_STANDING) {
          if (GET_AGL(sch) < number(1, 20)) {
            /* failed */
            if (number(0, 9) == 0) {
              /* collosal failure, into the silt */
              act("The movement of $p makes you loose your balance, and you fall over the side.\n\r", 
                  FALSE, sch, wagon, 0, TO_CHAR);
              act("$n stumbles and falls out of $p.\n\r", 
                  FALSE, sch, wagon, 0, TO_ROOM);
              
              remove_occupant(sch->on_obj, sCh->ch);
              
              /* handle sinking */
              if (will_char_fall_in_room(sch, sch->in_room)) {
                if (!is_char_falling(sch)) {
                  add_char_to_falling(sch);
                }
                
                if (IS_SET(sch->in_room->room_flags, RFL_NO_FLYING) &&
                    IS_AFFECTED(sch, CHAR_AFF_FLYING)) {
                  /* strip their flying spell if not
                   * immortal and have flying in
                   * a no_fly room 
                   */
                  if (IS_AFFECTED(sch, CHAR_AFF_FLYING)) {
                    affect_from_char(sch, SPELL_FLY);
                  }
                  REMOVE_BIT(sch->specials.affected_by, CHAR_AFF_FLYING);
                }
              }
            } else {
              act("The movement of $p makes you loose your balance, forcing you to sit down.\n\r", 
                  FALSE, sch, wagon, 0, TO_CHAR);
              act("$n stumbles and falls into $p.\n\r", 
                  FALSE, sch, wagon, 0, TO_ROOM);
              /* set them to sitting, then add them back to wagon */
              set_char_position(sch, POSITION_SITTING);
              add_occupant(wagon, sch);
            }
          }
        }
      }
      
      for (sCh = wagon->occupants; sCh; sCh = sCh->next) {
        char_from_room(sCh->ch);
        char_to_room(sCh->ch, new_room);
        send_to_char("Looking around, you see...\n\r\n\r", sCh->ch);
        parse_command(sCh->ch, "look");
      }
      
      if (IS_SET(wagon->obj_flags.value[1], FURN_SKIMMER)) {
        if (number(0, 3) == 0) {
          sprintf(command, "\n\rAs it moves along, %s tilts about.\n\r\n\r", OSTR(wagon, short_descr));
          send_to_room(command, wagon->in_room);
        }
      }
      
      set_wagon_tracks_in_room(wagon, was_in, wagon->in_room, dir);
    } else {
      gamelog("Error: Somehow piloting object with no one on the object.");
    }
    return;
  } // FURNITURE ITEM
  
  if (calc_wagon_max_damage(wagon) <= wagon->obj_flags.value[4]) {
    send_to_char("The wagon is too damaged to move.\n\r", ch);
    return;
  }
  
  argument = one_argument(argument, arg1, sizeof(arg1));
  
  if (!strcmp(arg1, "COUNTROOMS")) {
    map_wagon_rooms(wagon, (void *) count_wagon_room, (void *) &num_rooms);
    sprintf(buf, "There are %d rooms in the wagon.\n", num_rooms);
    send_to_char(buf, ch);
  }

  if (!strcmp(arg1, "COUNTWEIGHT")) {
    map_wagon_rooms(wagon, (void *) add_room_weight, (void *) &weight);
    sprintf(buf, "The wagon weighs %d stones.\n", weight);
    send_to_char(buf, ch);
  }
  
  dir = (old_search_block(arg1, 0, strlen(arg1), dirs, 0));
  
  if ((dir == -1) || (dir == 0)) {
    send_to_char("You must specify a direction.\n\r", ch);
    return;
  }
  
  dir--;
  
  pilot_skill = (has_skill(ch, SKILL_PILOT) ? ch->skills[SKILL_PILOT]->learned : 0);
  if (number(1, 60) > pilot_skill) {
    send_to_char("You can't seem to control the wagon.\n\r", ch);
    gain_skill(ch, SKILL_PILOT, (GET_GUILD(ch) == GUILD_MERCHANT) ? 3 : 1);
    if (number(1, 60) > pilot_skill) {
      dir += number(-1, 1);
      
      if (dir <= -1) {
        dir = 3;
      }
      
      if (dir >= 4) {
        dir = 0;
      }
    } else {
      return;
    }
  }
  
  if (!wagon->in_room->direction[dir] ||
      IS_SET(wagon->in_room->direction[dir]->exit_info, EX_CLOSED) ||
      IS_SET(wagon->in_room->direction[dir]->to_room->room_flags, RFL_NO_WAGON)) {
    send_to_char("You can't go in that direction.\n\r", ch);
    return;
  }
  
  new_room = wagon->in_room->direction[dir]->to_room;
  
  if (!IS_SET(wagon->obj_flags.value[3], WAGON_CAN_GO_SILT) &&
      (new_room->sector_type == SECT_SILT)) {
    send_to_char("You can't go onto silt!\n\r", ch);
    return;
  }
  
  if (new_room->sector_type == SECT_HEAVY_FOREST) {
    send_to_char("The thick trees will not allow you to pass.\n\r", ch);
    return;
  }
  
  if (!IS_SET(wagon->obj_flags.value[3], WAGON_CAN_GO_LAND) &&
      (new_room->sector_type != SECT_SILT) && (new_room->sector_type != SECT_AIR)) {
    send_to_char("You can't go onto land!\n\r", ch);
    return;
  }
  
  if (!IS_SET(wagon->obj_flags.value[3], WAGON_CAN_GO_AIR) &&
      (new_room->sector_type == SECT_AIR)) {
    send_to_char("You can't go into the air!\n\r", ch);
    return;
  }
  
  strcpy(name, OSTR(wagon, short_descr));
  *name = toupper(*name);
  
  if (!no_specials && special(ch, CMD_FROM_ROOM, "pilot")) {
    return;
  }
  
  sprintf(buf, "%s leaves %s.\n", name, dirs[dir]);
  send_to_room(buf, wagon->in_room);
  
  was_in = wagon->in_room;
  obj_from_room(wagon);
  obj_to_room(wagon, new_room);
  
  if (!no_specials && special(ch, CMD_TO_ROOM, "pilot")) {
    obj_from_room(wagon);
    obj_to_room(wagon, was_in);
    return;
  }
  
  if (dir < DIR_UP) {
    sprintf(buf, "%s has arrived from the %s.\n", name, dirs[rev_dir[dir]]);
  } else if (dir == DIR_DOWN) {
    sprintf(buf, "%s has arrived from above.\n", name);
  } else {
    sprintf(buf, "%s has arrived from below.\n", name);
  }
  
  send_to_room(buf, wagon->in_room);
  
  send_to_char("Looking outside, you see...\n\r\n\r", ch);
  strcpy(command, "out");
  cmd_look(ch, command, 15, 0);
  
  sprintf(command, "As it moves along, %s rumbles and shakes.\n\r", OSTR(wagon, short_descr));
  map_wagon_rooms(wagon, (void *) wagon_movement_noise, (void *) command);
  set_wagon_tracks_in_room(wagon, was_in, wagon->in_room, dir);
  
  
  if ((wagon->obj_flags.value[4]) == 0) {
    WAIT_STATE(ch, 3);
    return;
  } else {
    // no more than 10 sec, no less than 3 sec
    lag = 10 - (10 * (wagon->obj_flags.value[4] / calc_wagon_max_damage(wagon)));
    lag = MIN(lag, 3);
    lag = MAX(lag, 10);
    WAIT_STATE(ch, lag);
  }
}

void
cmd_walk(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  if (ch->specials.riding) {
    GET_SPEED(ch) = SPEED_WALKING;
    if (!GET_SPEED(ch->specials.riding)) {
      act("$N is already walking slowly.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
      return;
    }
    
    act("You slap $N lightly on the side, and $E slows down.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
    GET_SPEED(ch->specials.riding) = SPEED_WALKING;
    return;
  }
  
  if (GET_SPEED(ch) == SPEED_WALKING) {
    send_to_char("You are already walking.\n\r", ch);
    return;
  }
  
  /* inserted to cheak if you're sneaking and adjust message */
  /* accordingly     -Sanvean 3/10/01                        */
  
  if (GET_SPEED(ch) == SPEED_SNEAKING) {
    send_to_char("You speed your pace a little.\n\r", ch);
  } else {
    send_to_char("You slow down to a brisk walk.\n\r", ch);
  }
  
  GET_SPEED(ch) = SPEED_WALKING;
}

void
cmd_run(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  if (ch->in_room->sector_type == SECT_SILT) {
    act("Run through silt?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
  /* If you are a shadow, you cannot run. */
  if (!IS_IMMORTAL(ch) &&
      (affected_by_spell(ch, PSI_SHADOW_WALK) || 
       (affected_by_spell(ch, SPELL_SEND_SHADOW) ||
        ((GET_RACE(ch) == RACE_SHADOW) || affected_by_spell(ch, SPELL_ETHEREAL))))) {
    act("On the shadow plane, all speeds are the same.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
  if (ch->specials.riding) {
    GET_SPEED(ch) = SPEED_RUNNING;
    if (GET_SPEED(ch->specials.riding)) {
      act("$N is already running.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
      return;
    }
    
    act("You slap $N lightly on the side, and $E begins to run.", FALSE, ch, 0, ch->specials.riding, TO_CHAR);
    GET_SPEED(ch->specials.riding) = SPEED_RUNNING;
    return;
  }
  
  if (GET_SPEED(ch) == SPEED_RUNNING) {
    send_to_char("You are already running.\n\r", ch);
    return;
  }
  
    if (affected_by_spell(ch, SPELL_LEVITATE)) {
      send_to_char("Your body floats to the ground.\n\r", ch);
      act("$n's body floats to the ground.\n\r", FALSE, ch, 0, 0, TO_ROOM);
      
      affect_from_char(ch, SPELL_LEVITATE);
    }
    
    send_to_char("You speed up to a fast run.\n\r", ch);
    GET_SPEED(ch) = SPEED_RUNNING;
}

void
cmd_sneak(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  if (GET_SPEED(ch) == SPEED_SNEAKING) {
    send_to_char("You are already trying to move silently.\n\r", ch);
    return;
  }
  
  send_to_char("You slow down and start moving carefully.\n\r", ch);
  
  if (is_char_drunk(ch) <= DRUNK_LIGHT) {
    GET_SPEED(ch) = SPEED_SNEAKING;
  } else {
    GET_SPEED(ch) = SPEED_WALKING;
  }
}

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void
stop_follower(CHAR_DATA * ch) {
  struct follow_type *j, *k;
  char buf[256];
  
  if (ch->obj_master) {
    if (!ch->obj_master->followers) {
      sprintf(buf, "Warning! %s has no followers!\n\r", OSTR(ch->obj_master, short_descr));
      gamelog(buf);
      return;
    }
    if (ch->obj_master->followers->follower == ch) {
      if (!ch->obj_master) {
        gamelog("WARNING! No obj_master!");
      }

      if (!ch->obj_master->followers) {
        gamelog("WARNING! No obj_master->followers!");
      }
      
      k = ch->obj_master->followers;
      ch->obj_master->followers = k->next;
      free(k);
    } else {
      for (k = ch->obj_master->followers; 
           k->next->follower != ch; 
           k = k->next);
      j = k->next;
      k->next = j->next;
      free(j);
    }
    ch->obj_master = (struct obj_data *) 0;
  }
  
  
  if (ch->master) {
    if (is_charmed(ch)) {
      act("Your mind reels momentarily as $N's hold over you wanes.", FALSE, ch, 0, ch->master, TO_CHAR);
      act("$n will no longer take your orders.", FALSE, ch, 0, ch->master, TO_VICT);
      if (affected_by_spell(ch, SPELL_CHARM_PERSON)) {
        affect_from_char(ch, SPELL_CHARM_PERSON);
      }
      
      if (affected_by_spell(ch, PSI_CONTROL)) {
        affect_from_char(ch, PSI_CONTROL);
      }
    }
    
    if (ch->lifting && ch->master->lifting == ch->lifting) {
      sprintf(buf, "[Dropping %s.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
      send_to_char(buf, ch);
      if (!drop_lifting(ch, ch->lifting)) {
        return;
      }
    }
    
    if (ch->master->followers->follower == ch) {
      k = ch->master->followers;
      ch->master->followers = k->next;
      free(k);
    } else {
      for (k = ch->master->followers; 
           k->next->follower != ch; 
           k = k->next);
      
      j = k->next;
      k->next = j->next;
      free(j);
    }
    
    ch->master = 0;
    REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_CHARM);
  } else {
    shhlog("ERROR: ch->master == NULL in stop_follower");
  }
}


/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void
add_follower(CHAR_DATA * ch, CHAR_DATA * leader) {
  struct follow_type *k;
  
  if (ch->master) {
    return;
  }
  
  if (ch == leader) {
    return;
  }
  
  ch->master = leader;
  
  CREATE(k, struct follow_type, 1);
  
  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;
  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  act("$n falls in behind you.", TRUE, ch, 0, leader, TO_VICT);
}

