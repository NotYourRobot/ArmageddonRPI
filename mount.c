/*
 * File: MOUNT.C
 * Usage: Routines for mounts & manipulating them.
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
 * 04/29/2006: Pulled functions from various places to here -Morgenes
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
#include "memory.h"
#include "mount.h"

extern int can_drop(CHAR_DATA * ch, OBJ_DATA * obj);

bool
is_mount(CHAR_DATA *mount) {
  if (!mount) {
    return FALSE;
  }
  
  if (IS_SET(mount->specials.act, CFL_MOUNT)) {
    return TRUE;
  }
  
  return FALSE;
}

/* 
 * Determine if the given mount is hitched to the given character.
 * Looks through the mount's master (and his master, etc.) until ch is found.
 * Since you can chain mounts together, have to watch for the CFL_MOUNT flag.
 */
bool
mount_hitched_to_char(CHAR_DATA * ch, CHAR_DATA * mount) {
  CHAR_DATA *curr_master;
  
  if (mount->in_room != ch->in_room) {
    return FALSE;
  }
  
  for (curr_master = mount->master; 
       curr_master; 
       curr_master = curr_master->master) {
    if (curr_master == ch) {
      return TRUE;
    } else if (!is_mount(curr_master)) {
      return FALSE;
    }
  }
  
  return FALSE;
}

/*
 * Determine if the given character 'owns' the mount.  Either actively riding
 * it, or hitched to it.
 */
bool
mount_owned_by_ch(CHAR_DATA * ch, CHAR_DATA * mount) {
  if (ch->specials.riding == mount || 
      mount_hitched_to_char(ch, mount)) {
    return TRUE;
  }
  
  return FALSE;
}

/*
 * Find the first mount the character is either riding or has hitched to them.
 * Mainly used if the player didn't specify what mount to work with for
 * the pack and unpack commands.
 */
CHAR_DATA *
find_mount(CHAR_DATA * ch) {
  struct follow_type *curr_follower;
  
  if (ch->specials.riding) {
    return ch->specials.riding;
  }
  
  for (curr_follower = ch->followers; 
       curr_follower; 
       curr_follower = curr_follower->next) {
    if (curr_follower->follower->in_room == ch->in_room &&
        is_mount(curr_follower->follower)) {
      return curr_follower->follower;
    }
  }
  
  return NULL;
}

/*
 * Determine if the given mount belongs to someone else and give appropriate
 * echoes if they do.
 *
 * Starts out by checking to see if the character 'owns' it.
 * Then looks through other people in the room and sees if they own it.
 *
 * Unforutnately this function is repsonsible for sending messages.
 *
 */
bool
mount_owned_by_someone_else(CHAR_DATA * ch, CHAR_DATA * mount) {
  CHAR_DATA *curr_ch;
  char buf[MAX_STRING_LENGTH];
  
  if (mount_owned_by_ch(ch, mount)) {
    return FALSE;
  }
  
  for (curr_ch = ch->in_room->people; 
       curr_ch; 
       curr_ch = curr_ch->next_in_room) {
    if (curr_ch != mount && 
        curr_ch != ch && 
        !is_mount(curr_ch)) {
      if (curr_ch->specials.riding == mount) {
        sprintf(buf, "%s", capitalize(PERS(ch, mount)));
        cprintf(ch, "%s is being ridden by %s.\n\r", buf, PERS(ch, curr_ch));

        return TRUE;
      }
      
      /* if they are hitched to the mount */
      if (mount_hitched_to_char(curr_ch, mount)) {
        sprintf(buf, "%s", capitalize(PERS(ch, mount)));
        cprintf(ch, "%s is hitched to %s.\n\r", buf, PERS(ch, curr_ch));

        return TRUE;
      }
    }
  }
  
  return FALSE;
}


/* Count the number of mounts that are following the character */
int
count_mount_followers(CHAR_DATA * ch) {
  struct follow_type *curr_follower;
  int num_followers = 0;
  
  if (ch->specials.riding) {
    num_followers = count_mount_followers(ch->specials.riding);
  }
  
  for (curr_follower = ch->followers; 
       curr_follower; 
       curr_follower = curr_follower->next) {
    if (ch->in_room == curr_follower->follower->in_room &&
        is_mount(curr_follower->follower)) {
      num_followers++; // This is for the curr_follower
      num_followers += count_mount_followers(curr_follower->follower);
    }
  }
  
  return num_followers;
}


/*
 * Pack an item on a mount or see what's on the mount.
 * Can only do this to mounts that aren't 'owned' by someone else.
 * If no mount is specified, finds the first mount owned by the character.
 */
void
cmd_pack(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  char arg1[256];
  char arg2[256];
  OBJ_DATA *obj = NULL;
  CHAR_DATA *mount = NULL;
  char args[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char keyword[MAX_STRING_LENGTH];
  
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  argument = two_arguments(args, arg1, sizeof(arg1), arg2, sizeof(arg2));
  
  if (*arg2) {
    if (!(mount = get_mount_room(ch, arg2))) {
      return;
    }
  }
  
  if (!*arg1) {
    mount = find_mount(ch);
  } else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
    if (!(mount = get_char_room_vis(ch, arg1))) {
      cprintf(ch, "You aren't carrying any '%s'.\n\r", arg1);
      return;
    } else if (!is_mount(mount)) {
      cprintf(ch, "%s is not a proper mount.\n\r", capitalize(PERS(ch, mount)));
      return;
    } else if (mount_owned_by_someone_else(ch, mount)) {
      /* messages sent by function, sadly */
      return;
    }
  } else if (!*arg2) {
    mount = find_mount(ch);
  }
  
  if (!mount) {
    cprintf(ch, "You aren't riding a mount.\n\r");
    return;
  }
  
  if (ch->specials.riding && 
      ch->specials.riding != mount) {
    cprintf(ch, "You are already riding %s, dismount first.\n\r", PERS(ch, ch->specials.riding));
    return;
  }
  
  find_ch_keyword(mount, ch, keyword, sizeof(keyword));
  
  if (!obj && mount) {
    int num_vis_objects = 0;
    for (obj = mount->carrying; 
         obj; 
         obj = obj->next_content) {
      if (CAN_SEE_OBJ(ch, obj)) {
        num_vis_objects++;
      }
    }
    
    /* if they're not mounted to it, show the room */
    if (*preHow || 
        *postHow || 
        ch->specials.riding != mount) {
      sprintf(to_room, "@ looks over the things strapped to ~%s", keyword);
      sprintf(to_char, "you look over the things strapped to ~%s", keyword);
      
      if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
        return;
      }
    }
    
    if (num_vis_objects > 0) {
      cprintf(ch, 
              "%s is carrying:\n\r%s\n\r", 
              capitalize(PERS(ch, mount)), 
              list_obj_to_char(mount->carrying, ch, 1, 0));
    } else {
      cprintf(ch, "%s isn't carrying anything.\n\r", capitalize(PERS(ch, mount)));
    }
    
    return;
  }
  
  if ((1 + IS_CARRYING_N(mount)) > CAN_CARRY_N(mount)) {
    cprintf(ch, "%s cannot carry anything more.\n\r", capitalize(PERS(ch, mount)));
    return;
  }
  
  if ((GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(mount)) > CAN_CARRY_W(mount)) {
    cprintf(ch, "%s cannot carry that much weight.\n\r", capitalize(PERS(ch, mount)));
    return;
  }
  
  if (GET_ITEM_TYPE(obj) != ITEM_CONTAINER && 
      GET_ITEM_TYPE(obj) != ITEM_DRINKCON) {
    cprintf(ch, "You can only pack mounts with bags.\n\r");
    return;
  }
  
  /* send messages */
  sprintf(to_char, "you strap ~%s to %%%s back", arg1, keyword);
  sprintf(to_room, "@ straps ~%s to %%%s back", arg1, keyword);
  
  if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
    return;
  }
  
  obj_from_char(obj);
  obj_to_char(obj, mount);
}

/*
 * Handles unpacking the an object from a mount and giving it to a character.
 */
int
unpack_obj(CHAR_DATA * ch, CHAR_DATA * mount, OBJ_DATA * obj, char *preHow, char *postHow) {
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char okeyword[MAX_STRING_LENGTH];
  char mkeyword[MAX_STRING_LENGTH];
  bool can_carry = TRUE;
  
  if (!mount && 
      obj && 
      obj->carried_by) {
    mount = obj->carried_by;
  } else if (!mount) {
    cprintf(ch, "Unpack what mount?\n\r");
    return FALSE;
  }
  
  if (!obj) {
    cprintf(ch, "There is nothing packed upon %s.\n\r", PERS(ch, mount));
    return FALSE;
  }
  
  if (((1 + IS_CARRYING_N(ch)) > CAN_CARRY_N(ch)) ||
      ((GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(ch)) > CAN_CARRY_W(ch))) {
    can_carry = FALSE;
  }
  
  if (!can_carry && 
      !can_drop(ch, obj)) { // Has its own denial message
    return FALSE;
  }

  obj_from_char(obj);

  if (can_carry) {
    obj_to_char(obj, ch);
  } else {
    obj_to_room(obj, ch->in_room);
  }

  find_obj_keyword(obj, FIND_OBJ_ROOM | FIND_OBJ_INV, ch, okeyword, sizeof(okeyword));
  find_ch_keyword(mount, ch, mkeyword, sizeof(mkeyword));
  
  sprintf(to_char, "you unstrap ~%s from %%%s back", okeyword, mkeyword);
  sprintf(to_room, "@ unstraps ~%s from %%%s back", okeyword, mkeyword);
  
  if (!can_carry) {
    strncat(to_char, ", which slides off to the ground", sizeof(to_char));
    strncat(to_room, ", which slides off to the ground", sizeof(to_room));
  }
  
  
  if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
    if (obj->carried_by) {
      obj_from_char(obj);
    } else {
      obj_from_room(obj);
    }
    obj_to_char(obj, mount);
    return FALSE;
  }
  
  return TRUE;
}

CHAR_DATA *
get_mount_room(CHAR_DATA * ch, char *arg) {
  CHAR_DATA *mount = NULL;
  
  if (!(mount = get_char_room_vis(ch, arg))) {
    cprintf(ch, "You don't see a '%s' here.\n\r", arg);
    return NULL;
  } else if (!is_mount(mount)) {
    cprintf(ch, "%s is not a proper mount.\n\r", capitalize(PERS(ch, mount)));
    return NULL;
  } else if (mount_owned_by_someone_else(ch, mount)) {
    /* messages sent by function, sadly */
    return NULL;
  }
  
  return mount;
}

void
cmd_unpack(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  int num_visible_objects = 0;
  char arg1[256];
  char arg2[256];
  CHAR_DATA *mount = NULL;
  OBJ_DATA *obj = NULL;
  char args[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow,
                          sizeof(postHow));
  
  /* grab two arguments */
  argument = two_arguments(args, arg1, sizeof(arg1), arg2, sizeof(arg2));
  
  if (!*arg1) {
    mount = find_mount(ch);
  } else if (*arg2) {
    if (!(mount = get_mount_room(ch, arg2))) {
      return;
    }
    
    if (!(obj = get_obj_in_list_vis(ch, arg1, mount->carrying))) {
      cprintf(ch, "You can't find a '%s' on %s.\n\r", arg1, PERS(ch, mount));
      return;
    }
  } else {  /* no arg2, but there is an arg1 */
    mount = find_mount(ch);
    if (stricmp("all", arg1) &&
        (!mount || (mount && !(obj = get_obj_in_list_vis(ch, arg1, mount->carrying))))) {
      /* maybe they specified a mount */
      if (!(mount = get_char_room_vis(ch, arg1))) {
        mount = find_mount(ch);
        if (mount) {
          cprintf(ch, "You can't find a '%s' on %s.\n\r", arg1, PERS(ch, mount));
        } else {
          cprintf(ch, "You don't see any '%s' here to unpack.\n\r", arg1);
          return;
        }
        return;
      } else if (!is_mount(mount)) {
        cprintf(ch, "%s is not a proper mount.\n\r", capitalize(PERS(ch, mount)));
        return;
      } else if (mount_owned_by_someone_else(ch, mount)) {
        return;        /* messages sent by function */
      }
    }
  }
  
  if (!mount) {
    cprintf(ch, "You don't have a mount to unpack.\n\r");
    return;
  }
  
  if (obj) {
    unpack_obj(ch, mount, obj, preHow, postHow);
  } else if (!stricmp(arg1, "all")) {
    OBJ_DATA *obj_next;
    
    // TODO: make this time delayed
    for (obj = mount->carrying; 
         obj; 
         obj = obj_next) {
      obj_next = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj)) {
        if (!unpack_obj(ch, mount, obj, preHow, postHow)) {
          break;
        }
        num_visible_objects++;
      }
    }
    
    if (!num_visible_objects) {
      cprintf(ch, "There's nothing to unpack on %s.\n\r", PERS(ch, mount));
    }
  } else {
    for (obj = mount->carrying; 
         obj; 
         obj = obj->next_content) {
      if (CAN_SEE_OBJ(ch, obj)) {
        unpack_obj(ch, mount, obj, preHow, postHow);
        num_visible_objects++;
        break;
      }
    }
    
    if (!num_visible_objects) {
      cprintf(ch, "There's nothing to unpack on %s.\n\r", PERS(ch, mount));
    }
    return;
  }
}

/* 
 * Hitch a mount to the character or another of the characer's mounts.
 *
 * New Hitch code written by Nessalin 2/25/97 
 * Revised by Morgenes 4/30/06
 */
void
cmd_hitch(CHAR_DATA * ch,       /* Person doing the hitching     */
          const char *argument, /* What they typed after 'hitch' */
          Command cmd,          /* What command they used */
          int count) {
  /* begin Cmd_Hitch */
  char arg1[256], arg2[256];  /* arguments character typed after 'hitch' */
  CHAR_DATA *follower;        /* char to hitch to leader                 */
  CHAR_DATA *leader;          /* char follower will be hitched to        */
  char args[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  
  int maxlead = 0;
  OBJ_DATA *wagon;
  
  /* To be used for hitching to wagons */
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  argument = two_arguments(args, arg1, sizeof(arg1), arg2, sizeof(arg2));
  
  if (!*arg1) {
    send_to_char("Usage: hitch <follower> <leader>\n\r", ch);
    return;
  }
  
  if (!*arg2) {
    strcpy(arg2, "me");
  }
  
  if (!(follower = get_char_room_vis(ch, arg1))) {
    cprintf(ch, "You don't see '%s' here.\n\r", arg1);
    return;
  }
  
  if (!is_mount(follower)) {
    cprintf(ch, "%s isn't a suitable mount.\n\r", capitalize(PERS(ch, follower)));
    return;
  }
  
  if (GET_POS(follower) == POSITION_FIGHTING) {
    cprintf(ch, "%s is in the midst of battle!\n\r", capitalize(PERS(ch, follower)));
    return;
  }
  
  /* Cannot hitch creatures following wagons */
  wagon = follower->obj_master;
  if (wagon) {
    if (wagon->in_room == follower->in_room) {
      cprintf(ch, "%s is currently following %s, and cannot hitched.\n\r",
              capitalize(PERS(ch, follower)), format_obj_to_char(wagon, ch, 1));
      return;
    } else {
      stop_follower(follower);
    }
  }
  
  if ((follower->master) && (follower->master == ch)) {
    cprintf(ch, "%s is already following you.\n\r", capitalize(PERS(ch, follower)));
    return;
  }
  
  if (mount_owned_by_someone_else(ch, follower)) {
    return;    /* function gives error messages */
  }
  
  stop_follower(follower);
  /* ---- End Follower Checks ---- */
  
  /* ---- Begin Leader Checks ---- */
  if (!(leader = get_char_room_vis(ch, arg2))) {
    cprintf(ch, "You don't see '%s' here.\n\r", arg2);
    return;
  }
  
  if (leader != ch) {
    if (!is_mount(leader)) {
      cprintf(ch, "%s is not a proper mount.\n\r", capitalize(PERS(ch, leader)));
      return;
    }
    
    if (mount_owned_by_someone_else(ch, leader)) {
      return;      /* owned_by_someone_else handles sending messages */
    }
    
    if (!mount_owned_by_ch(ch, leader)) {
      cprintf(ch, "%s isn't ridden by or hitched to you.\n\r", capitalize(PERS(ch, leader)));
      return;
    }
  }
  
  /* ----  End Leader Checks  ---- */

  if (leader == follower) {
    cprintf(ch, "You can't hitch %s to itself!\n\r", PERS(ch, follower));
    return;
  }
  
  /* give rangers and merchants a bonus for how many can be hitched */
  /* These values were reduced by one on 7/6/96 by Nessalin */
  if ((GET_GUILD(ch) == GUILD_RANGER) || 
      (GET_GUILD(ch) == GUILD_MERCHANT) ||
      (GET_RACE(ch) == RACE_HALF_ELF)) {
    maxlead = 1;
  }
  
  if (count_mount_followers(ch) > maxlead) {
    cprintf(ch, "To lead any more mounts would be pure chaos!\n\r");
    return;
  }
  
  if (new_circle_follow_ch_ch(follower, leader)) {
    cprintf(ch, "Following in loops is not allowed.\n\r");
    return;
  }
  
  if (leader == ch) {
    sprintf(to_char, "you begin leading ~%s", arg1);
    sprintf(to_room, "@ begins leading ~%s", arg1);
  } else {
    sprintf(to_char, "you hitch ~%s to ~%s.", arg1, arg2);
    sprintf(to_room, "@ hitches %s to %s", arg1, arg2);
  }
  
  /* emote failed */
  if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
    return;
  }
  
  add_follower(follower, leader);
}                               /* end Cmd_Hitch */


/*
 * Handle unhitching a mount.  Stops them from following you.
 */
void
cmd_unhitch(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  char arg1[256];
  CHAR_DATA *mount;
  char args[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  argument = one_argument(args, arg1, sizeof(arg1));
  
  if (!*arg1) {
    cprintf(ch, "Syntax: unhitch <mount>\n\r");
    return;
  }
  
  if (!(mount = get_char_room_vis(ch, arg1))) {
    cprintf(ch, "You don't see any '%s' here.\n\r", arg1);
    return;
  }
  
  if (!mount->master) {
    cprintf(ch, "%s is not hitched to anyone.\n\r", capitalize(PERS(ch, mount)));
    return;
  }
  
  if (mount_owned_by_someone_else(ch, mount)) {
    return;
  }
  
  if (!mount_hitched_to_char(ch, mount)) {
    /* handle when the mount's master is in another room */
    if (mount->master) {
      stop_follower(mount);
      cprintf(ch, "%s is not hitched to anyone.\n\r", capitalize(PERS(ch, mount)));
      return;
    }
    
    cprintf(ch, "%s is not hitched to you.\n\r", capitalize(PERS(ch, mount)));
    return;
  }
  
  sprintf(to_char, "you stop leading ~%s", arg1);
  sprintf(to_room, "@ stops leading ~%s", arg1);
  
  if (!send_to_char_and_room_parsed (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
    return;
  }
  
  stop_follower(mount);
}



void
cmd_mount(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  CHAR_DATA *mount, *rider;
  char cmd_string[MAX_INPUT_LENGTH];
  char arg1[256], buf[256];
  int fightcheck;
  int vict_flight = FLY_NO;
  int ch_flight = FLY_NO;
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  argument = one_argument(args, arg1, sizeof(arg1));
  
  if (!*arg1) {
    mount = find_mount(ch); // Possibly hitched mount
    if (!mount) {
      /* Same message as having no followers, to prevent abusing mount to detect shadowers. */
      cprintf(ch, "You don't have a hitched animal to ride.\n\r");
      return;
    }
  } else {
    if (!(mount = get_char_room_vis(ch, arg1))) {
      cprintf(ch, "There is no '%s' to ride in the room.\n\r", arg1);
      return;
    }
  }
  
  if ((GET_RACE_TYPE(ch) == RTYPE_INSECTINE) ||
      (GET_RACE_TYPE(ch) == RTYPE_OPHIDIAN) ||
      (GET_RACE_TYPE(ch) == RTYPE_AVIAN_FLIGHTLESS) ||
      (GET_RACE_TYPE(ch) == RTYPE_AVIAN_FLYING)) {
    cprintf(ch, "That would be impossible.\n\r");
    return;
  }
  
  /* If victim is affected by the Rooted ability, they can't mount */
  if (find_ex_description("[ROOTED]", ch->ex_description, TRUE)) {
    act("You cannot do that while rooted.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
  if (ch->specials.riding) {
    cprintf(ch, "You're already riding %s.\n\r", PERS(ch, ch->specials.riding));
    return;
  }
  
  if (!is_mount(mount) && !IS_MOUNTABLE(mount)) {
    cprintf(ch, "You can't ride %s!\n\r", PERS(ch, mount));
    return;
  }
  
  /* Flying mounts can't be mounted unless ch is doing the same */
  /* Or if the ch is a beastspeaker and is liked by the mount -- see */
  /* mounting flying critters below */
  vict_flight = is_flying_char(mount);
  ch_flight = is_flying_char(ch);
  if (vict_flight > FLY_LEVITATE && 
      ch_flight == FLY_NO) {
    act("You try to mount $N, but it flies out of your reach.", FALSE, ch, 0, mount, TO_CHAR);
    act("$n tries to mount $N, but it flies out of $s reach.", FALSE, ch, 0, mount, TO_ROOM);
    return;
  }
  
  /* Levitating mounts can't be mounted unless ch is flying/levitating */
  /* Same for levitating mounts as flying */
  if (vict_flight == FLY_LEVITATE && 
      ch_flight < FLY_LEVITATE) {
    act("You try to mount $N, but it floats out of your reach.", FALSE, ch, 0, mount, TO_CHAR);
    act("$n tries to mount $N, but it floats out of $s reach.", FALSE, ch, 0, mount, TO_ROOM);
    return;
  }
  
  if (is_mount(ch)) {
    cprintf(ch, "How about letting someone else get on your back instead?\n\r");
    return;
  }
  
  if (!AWAKE(mount)) {
    cprintf(ch, "You won't get far on a sleeping mount.\n\r");
    return;
  }
  
  if (mount_owned_by_someone_else(ch, mount)){
    return;
  } else if (mount->master && mount->master != ch) {
    stop_follower(mount);
  }
  
  for (rider = character_list; 
       rider; 
       rider = rider->next) {
    if (rider->specials.riding == mount) {
      break;
    }
  }
  
  if (rider) {
    if (CAN_SEE(ch, rider)) {
      sprintf(buf, "%s is already riding %s.\n\r", PERS(ch, rider), PERS(ch, mount));
      send_to_char(buf, ch);
    } else {
      sprintf(buf, "$N tries to mount %s, but runs into $n.", MSTR(mount, short_descr));
      act(buf, FALSE, rider, 0, ch, TO_NOTVICT);
      sprintf(buf, "$N tries to mount %s, but runs into you.", MSTR(mount, short_descr));
      act(buf, FALSE, rider, 0, ch, TO_CHAR);
      sprintf(buf, "You try to mount %s, but run into $n who's" " already riding!", MSTR(mount, short_descr));
      act(buf, FALSE, rider, 0, ch, TO_VICT);
    }
    return;
  }
  
  if (get_char_size(mount) < (get_char_size(ch) * 1.25)) {
    cprintf(ch, "%s is too small to support your bulk.", capitalize(PERS(ch, mount)));
    return;
  }
  
  if (ch->lifting) {
    cprintf(ch, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
    if (!drop_lifting(ch, ch->lifting)) {
      return;
    }
  }

  if (!is_mount(mount) &&
      !(IS_SET(mount->specials.act, CFL_AGGRESSIVE))) {     
    if (GET_GUILD(ch) == GUILD_RANGER && 
        has_skill(ch, SKILL_RIDE)) {
      if (GET_POS(ch) != POSITION_STANDING) {
        send_to_char("You are in no position to tame this beast.", ch);
        return;
      }
      
      if ((ch->skills[SKILL_RIDE]->learned > 60) &&
          (number(1, 100) < ch->skills[SKILL_RIDE]->learned)) {
        act("$n slowly climbs atop of you.", FALSE, ch, 0, mount, TO_VICT);
        act("$n gingerly approaches $N, whispering softly.", FALSE, ch, 0, mount, TO_NOTVICT);
        act("$N seems fairly calm as you slowly approach.", FALSE, ch, 0, mount, TO_CHAR);
        
        if (!IS_SET(mount->specials.act, CFL_MOUNT)) {
          MUD_SET_BIT(mount->specials.act, CFL_MOUNT);
        }
        
        if (!IS_SET(mount->specials.act, CFL_SENTINEL)) {
          MUD_SET_BIT(mount->specials.act, CFL_SENTINEL); 
        }

        if (IS_SET(mount->specials.act, CFL_FLEE)) {
          REMOVE_BIT(mount->specials.act, CFL_FLEE);
        }
      } else {
        act("$n tries to climb onto your back, but you don't let $m.", FALSE, ch, 0, mount, TO_VICT);
        act("$n unsuccessfully tries to mount $N.", FALSE, ch, 0, mount, TO_NOTVICT);
        act("You unsuccessfully try to convince $M to let you ride.", FALSE, ch, 0, mount, TO_CHAR);
        
        gain_skill(ch, SKILL_RIDE, 1);
        WAIT_STATE(ch, 3);
        hit(mount, ch, TYPE_UNDEFINED);
        return;
      }
    } else {
      act("$n tries to climb onto your back, but you don't let $m.", FALSE, ch, 0, mount, TO_VICT);
      act("$n unsuccessfully tries to mount $N, landing on $s backside.", FALSE, ch, 0, mount, TO_NOTVICT);
      act("$N throws you from $S back when you climb up.", FALSE, ch, 0, mount, TO_CHAR);
      set_char_position(ch, POSITION_SITTING);

      return;
    }
  }
  
  if ((!is_mount(mount) && 
       IS_SET(mount->specials.act, CFL_AGGRESSIVE)) &&
      !does_like(mount, ch)) {
    act("You try to mount $N, but $E throws you off.", FALSE, ch, 0, mount, TO_CHAR);
    act("$n tries to mount you, but you throw $m off.", FALSE, ch, 0, mount, TO_VICT);
    act("$n tries to mount $N, but $E throws $m off.", FALSE, ch, 0, mount, TO_NOTVICT);

    return;
  }
  
  if ((GET_STR(mount) > GET_STR(ch)) && 
      IS_SET(mount->specials.act, CFL_AGGRESSIVE)) {
    act("You try to mount $N, but $E throws you off.", FALSE, ch, 0, mount, TO_CHAR);
    act("$n tries to mount you, but you throw $m off.", FALSE, ch, 0, mount, TO_VICT);
    act("$n tries to mount $N, but $E throws $m off.", FALSE, ch, 0, mount, TO_NOTVICT);

    return;
  }
  if (ch->specials.fighting &&
      !is_paralyzed(ch->specials.fighting)) {
    if (GET_GUILD(ch) == GUILD_RANGER || 
        GET_RACE(ch) == RACE_HALF_ELF) {
      fightcheck = 70;
    } else {
      fightcheck = 100;
    }
    
    if (ch->skills[SKILL_RIDE] && 
        (number(1, fightcheck) > ch->skills[SKILL_RIDE]->learned)) {
      act("You try to mount $N, but $E moves away.", FALSE, ch, 0, mount, TO_CHAR);
      act("$n tries to mount you, but you move away from $m.", FALSE, ch, 0, mount, TO_VICT);
      act("$n tries to mount $N, but $E moves away from $m.", FALSE, ch, 0, mount, TO_NOTVICT);
      if (number(1, 100) <= 30) {
        gain_skill(ch, SKILL_RIDE, 1);
      }
      WAIT_STATE(ch, 1);
      
      return;
    }
  }
  
  find_ch_keyword(mount, ch, buf, sizeof(buf));
  
  if (ch_flight > FLY_NO) {
    sprintf(to_char, "you land upon %%%s back", buf);
    sprintf(to_room, "@ alights on %%%s back", buf);
    if (affected_by_spell(ch, SPELL_LEVITATE)) {
      affect_from_char(ch, SPELL_LEVITATE);
    }
  } else {
    if (vict_flight >= FLY_LEVITATE) {
      sprintf(to_char, "you jump onto %%%s back, as #%s swoops by", buf, buf);
      sprintf(to_room, "@ jumps onto %%%s back, as #%s swoops by", buf, buf);
    } else {
      sprintf(to_char, "you jump up onto %%%s back", buf);
      sprintf(to_room, "@ jumps up onto %%%s back", buf);
    }
  }
  
  if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
    return;
  }
  
  if (GET_POS(mount) < POSITION_STANDING) {
    strcpy(cmd_string, "");
    cmd_stand(mount, cmd_string, CMD_STAND, 0);
  }
  
  ch->specials.riding = mount;
  
  if (GET_SPEED(ch) == SPEED_SNEAKING) {
    GET_SPEED(ch) = SPEED_WALKING;
  }
  
  WIPE_LDESC(ch);
}

/*
 * Handle dismounting off a mount.
 * Can also be used by the mount to 'buck' the rider off.
 */
void
cmd_dismount(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  int buck, defense;
  CHAR_DATA *mount;
  CHAR_DATA *rider;
  char buf[MAX_STRING_LENGTH];
  int mount_flight = FLY_NO;
  int ch_flight = FLY_NO;
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  if (!ch->specials.riding) {
    for (rider = ch->in_room->people; 
         rider; 
         rider = rider->next_in_room) {
      if (rider && 
          rider != ch &&
          ch == rider->specials.riding)
        break;
    }
    
    if (rider && 
        rider != ch && 
        ch == rider->specials.riding) {
      buck = GET_AGL(ch) + number(1, 100);
      defense = GET_AGL(rider) +
        (has_skill(rider, SKILL_RIDE) ? rider->skills[SKILL_RIDE]->learned : 0);
      if (defense >= buck) {
        sprintf(buf, "Panicked, %s bucks wildly, but you grip your reins and hang on.\n\r", MSTR(ch, short_descr));
        send_to_char(buf, rider);
        act("You attempt to throw $n off your back, but $e holds on.", TRUE, rider, 0, ch, TO_VICT);
        act("$n bucks wildly, but $N holds on.", TRUE, ch, 0, rider, TO_ROOM);

        return;
      } else {
        rider->specials.riding = 0;
        
        if (defense >= number(1, 100)) {
          act("$n bucks wildly, and you jump quickly to the ground.", FALSE, ch, 0, rider, TO_VICT);
          act("$n bucks wildly and $N jumps quickly to the ground.", TRUE, ch, 0, rider, TO_ROOM);
          act("You buck wildly, and $N jumps off your back.", FALSE, ch, 0, rider, TO_CHAR);
          return;
        } else {
          act("$n bucks wildly, and you are thrown to the ground.", FALSE, ch, 0, rider, TO_VICT);
          act("$n bucks wildly and $N is thrown to the ground.", TRUE, ch, 0, rider, TO_ROOM);
          act("You buck wildly, and throw $N off your back.", FALSE, ch, 0, rider, TO_CHAR);
          
          if (generic_damage(rider, number(5, 10), 0, 0, number(10, 20))) {
            set_char_position(rider, POSITION_SITTING);
          }
          return;
        }
      }
    } else {
      cprintf(ch, "You aren't riding an animal.\n\r");

      return;
    }
  }
  
  mount = ch->specials.riding;
  find_ch_keyword(mount, ch, buf, sizeof(buf));
  
  mount_flight = is_flying_char(mount);
  ch_flight = is_flying_char(ch);
  
  if (mount_flight >= FLY_LEVITATE && ch_flight == FLY_FEATHER) {
    sprintf(to_char, "you jump off %%%s back, and float gently to the ground", buf);
    sprintf(to_room, "@ jumps off %%%s back, and floats gently to the ground", buf);
  } else if (mount_flight >= FLY_LEVITATE && ch_flight < FLY_FEATHER) {
    cprintf(ch, "Dismount from this height?  Perhaps you should land first.\n\r");

    return;
  } else {
    sprintf(to_char, "You swing your legs to the side and dismount");
    sprintf(to_room, "@ swings %s legs to the side and dismounts", HSHR (ch));
  }
  
  if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
    return;
  }

  ch->specials.riding = 0;

  WIPE_LDESC(ch);
}

