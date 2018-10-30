/*
 * File: OBJECT.C
 * Usage: Commands involving object manipulation.
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

/* Revision history:
 * 05/06/2000 Added some more to component crafting.  -Sanvean
 * 05/14/2000 Added RACE_VULTURE to those that can eat corpses. -San
 * 05/16/2000 Added a check for skill SLEIGHT_OF_HAND in cmd_get, so
 *            if you have the skill, the room doesn't see you get an
 *            object from a container.  -San
 * 05/17/2000 Added to armormaking and cooking.  
 * 05/26/2000 Fixed typos.
 * 06/02/2000 Added objects to armormaking, cooking, clothworking,
 *            feather working, jewelry making, leatherworking,
 *            bandagemaking.  -San
 * 06/10/2000 Added objects to armormaking, clothworking, cooking,
 *            dyeing, fletchery, floristy, jewelrymaking, knifemaking,
 *            leatherworking, lumberjacking, spearmaking, swordmaking,
 *            and woodworking.  -San
 * 06/17/2000 Fixed some typos in crafting.  Changed it so ldesc changes
 *            back to default once crafting is finished.  -San
 * 09/21/2000 Added RACE_CILOPS to those that can eat corpses. Fixed it
 *            so RACE_VULTURE actually succeeds at eating a corpse.
 * 12/01/2000 Moved around order of obj updating & echos in cmd_taste. -Savak
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include "constants.h"
#include "structs.h"
#include "limits.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "modify.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "guilds.h"
#include "cities.h"
#include "clan.h"
#include "watch.h"
#include "info.h"

extern int get_room_max_weight(ROOM_DATA * room);
extern int get_normal_condition(CHAR_DATA * ch, int cond);
extern int ness_alalyze_from_crafted_item(OBJ_DATA * obj, CHAR_DATA * ch);

/* #define NESS_MONEY*/

/* check to see if ch is referring to the table/chair they are at
 * returns the object we think they're referring to.
 */
OBJ_DATA *
is_referring_to_their_table(const CHAR_DATA * ch, const OBJ_DATA * sub_object, const char *arg)
{
    /* convenience variables */
    OBJ_DATA *chair;
    OBJ_DATA *table;

    /* if they aren't sitting on anything, can't be referring to their table */
    if (!ch->on_obj)
        return NULL;

    /* save what they're on as chair */
    chair = ch->on_obj;

    /* if they are on the object, then yes */
    if (chair == sub_object)
        return chair;

    /* save what the chair is around as table */
    table = chair->table;

    /* if on a chair that is around a table, and the table matches, yes */
    if (table && table == sub_object)
        return table;

    /* at this point we know that it doesn't match, try looking at arg */

    /* first look at the table */
    if (table                   /* if at a table */
        /* and can refer to the table with arg */
        && isallname(arg, OSTR(table, name)))
        return table;

    /* now see if we can refer to the chair with arg */
    if (isallname(arg, OSTR(chair, name)))
        return chair;

    /* couldn't match it, so fail (return 0) */
    return NULL;
}

/* Started 8/4/2001 by Nessalin
   A version of is_covered() that doesn't worry about wether something
   is visible.  Used by the sflag_to_*() functions to decide what gets
   damaged */
int
is_protected(CHAR_DATA * ch, int pos)
{
    switch (pos) {

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
        if (!ch->equipment[WEAR_HANDS])
            return FALSE;
        return TRUE;
    case WEAR_BODY:
    case WEAR_BELT:
    case WEAR_ON_BELT_1:
    case WEAR_ON_BELT_2:
    case WEAR_WAIST:
    case WEAR_LEGS:
    case WEAR_ARMS:
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:
        if (!ch->equipment[WEAR_ABOUT])
            return FALSE;
        return TRUE;
    default:
        return FALSE;
    }
}


/* Started 8/4//2001 by Nessalin
   Generic function for adding state flags to items, pass in the 
   object and the damage type.  The function decides which item
   types and materials will actually get the flags. */

void
sflag_to_obj(OBJ_DATA * obj, long sflag)
{
    int skip = 0;
    /* Some objects can never have state flags */
    if (obj->obj_flags.type == ITEM_TELEPORT)
        return;
    if (IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL))
        return;

    switch (sflag) {
    case OST_TORN:
    case OST_TATTERED:
        /* Materials that can be torn or tattered */
        if (!(obj->obj_flags.material == MATERIAL_SILK)
            || (obj->obj_flags.material == MATERIAL_CLOTH))
            skip = 1;
        break;
    case OST_BLOODY:
        /* Materials that can be bloodied */
        if (!((obj->obj_flags.material == MATERIAL_WOOD)
              || (obj->obj_flags.material == MATERIAL_STONE)
              || (obj->obj_flags.material == MATERIAL_CHITIN)
              || (obj->obj_flags.material == MATERIAL_SILK)
              || (obj->obj_flags.material == MATERIAL_SKIN)
              || (obj->obj_flags.material == MATERIAL_CLOTH)
              || (obj->obj_flags.material == MATERIAL_LEATHER)
              || (obj->obj_flags.material == MATERIAL_METAL)
              || (obj->obj_flags.material == MATERIAL_BONE)
              || (obj->obj_flags.material == MATERIAL_HORN)
              || (obj->obj_flags.material == MATERIAL_TISSUE)))
            skip = 1;
        /* object types that can't be bloodied */
        break;
    case OST_DUSTY:
        /* object types that can't be dusted */
        if ((obj->obj_flags.type == ITEM_TREASURE) || (obj->obj_flags.type == ITEM_KEY))
            skip = 1;
        break;
        /* Only worn items can be embroidered/laced/fringed  */
    case OST_EMBROIDERED:
    case OST_LACED:
    case OST_FRINGED:
        if (!(obj->obj_flags.type == ITEM_WORN))
            skip = 1;
        break;
    case OST_MUDCAKED:
    case OST_STAINED:
    case OST_REPAIRED:
    case OST_GITH:
        break;
    case OST_BURNED:
        /* materials that can be burned */
        if (!((obj->obj_flags.material == MATERIAL_WOOD)
              || (obj->obj_flags.material == MATERIAL_CHITIN)
              || (obj->obj_flags.material == MATERIAL_SKIN)
              || (obj->obj_flags.material == MATERIAL_CLOTH)
              || (obj->obj_flags.material == MATERIAL_LEATHER)
              || (obj->obj_flags.material == MATERIAL_BONE)
              || (obj->obj_flags.material == MATERIAL_TISSUE)))
            skip = 1;
        /* obj types that can't be burned */
        if ((obj->obj_flags.type == ITEM_SCENT) || (obj->obj_flags.type == ITEM_FIRE))
            skip = 1;
        break;
    case OST_SEWER:
    case OST_OLD:
        break;
    case OST_SWEATY:
        /* Materials that be made sweaty */
        if (!((obj->obj_flags.material == MATERIAL_CLOTH)
              || (obj->obj_flags.material == MATERIAL_LEATHER)
              || (obj->obj_flags.material == MATERIAL_SKIN)
              || (obj->obj_flags.material == MATERIAL_TISSUE)))
            skip = 1;
        /* object types that can be made sweaty */
        if (!((obj->obj_flags.type == ITEM_ARMOR) || (obj->obj_flags.type == ITEM_WORN)
              || (obj->obj_flags.type == ITEM_OTHER) || (obj->obj_flags.type == ITEM_SKIN)
              || (obj->obj_flags.type == ITEM_NOTE) || (obj->obj_flags.type == ITEM_SCROLL)))
            skip = 1;
        break;
    default:
        break;
    }

    if (skip)
        return;

    /* To get this far the obj type & material must be OK */
    if (!IS_SET(obj->obj_flags.state_flags, sflag))
        MUD_SET_BIT(obj->obj_flags.state_flags, sflag);
}

/* Started 8/4/2001 by Nessalin
   Throw an sflag at a character and let the code decide what objects
   on the player to affect based on material type, object type,
   location worn, in inventory, etc... */
void
sflag_to_char(CHAR_DATA * ch, long sflag)
{
    int counter = 0, chance = 3;

    if (is_char_ethereal(ch))
        return;

    if (IS_IMMORTAL(ch))
        return;

    switch (sflag) {
    case OST_SWEATY:
        /* Bottom layer only, not floating around head and not about body
         * if they're wearing something on body */
        for (counter = 0; counter < MAX_WEAR; counter++)
            if (ch->equipment[counter])
                if (counter == WEAR_BODY || counter == WEAR_LEGS || counter == WEAR_ARMS
                    || counter == WEAR_WRIST_R || counter == WEAR_WRIST_L || counter == WEAR_HANDS
                    || counter == WEAR_NECK || counter == WEAR_WAIST || counter == WEAR_HEAD
                    || (counter == WEAR_ABOUT && (!(ch->equipment[WEAR_BODY]))))
                    sflag_to_obj(ch->equipment[counter], sflag);
        break;
    case OST_BLOODY:
        chance = 2;
    case OST_DUSTY:
        /* Top layer clothing only */
        for (counter = 0; counter < MAX_WEAR; counter++)
            if (ch->equipment[counter])
                if (!is_protected(ch, counter)) {
                    if (!number(0, chance))
                        sflag_to_obj(ch->equipment[counter], sflag);
                }
        break;
    case OST_BURNED:
        /* Top layer and inventory only */
        for (counter = 0; counter < MAX_WEAR; counter++)
            if (ch->equipment[counter])
                if (!is_protected(ch, counter))
                    sflag_to_obj(ch->equipment[counter], sflag);
        break;
    case OST_SEWER:
        /* all layers */
        for (counter = 0; counter < MAX_WEAR; counter++)
            if (ch->equipment[counter])
                sflag_to_obj(ch->equipment[counter], sflag);
        break;
    default:
        break;
    }
}

/* Started 8/4/2001 by Nessalin
   Throw an sflag at a room and let the code decide which players
   to set the flag on */
void
sflag_to_room(ROOM_DATA * rm, long sflag)
{
    CHAR_DATA *tmp_ch;
    OBJ_DATA *tmp_obj;

    for (tmp_ch = rm->people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
        sflag_to_char(tmp_ch, sflag);

    for (tmp_obj = rm->contents; tmp_obj; tmp_obj = tmp_obj->next_content)
        sflag_to_obj(tmp_obj, sflag);
}


void
set_crafter_edesc(CHAR_DATA *ch, OBJ_DATA *obj) {
  char crafter_name[MAX_STRING_LENGTH];

  sprintf(crafter_name, "[CRAFTED_BY_%s_(%s)]", ch->name, ch->account);

  if (!IS_NPC(ch))
    sprintf(crafter_name, "[CRAFTED_BY_%s_(%s)]", ch->name, ch->account);
  else {
    if (!ch->desc)
      sprintf(crafter_name, "[CRAFTED_BY__m%d]", ch->nr);
    else
      if (ch->desc->original)
	sprintf(crafter_name, "[CRAFTED_BY_%s_(%s)_while_playing_m%d]", ch->desc->original->name, ch->desc->original->account, ch->nr);
  }

  set_obj_extra_desc_value (obj, crafter_name,  "");
}

bool
circle_obj_follow(CHAR_DATA * ch, OBJ_DATA * target_object)
{
    return (FALSE);
}

void
add_obj_follower(CHAR_DATA * ch, OBJ_DATA * obj_leader)
{
    struct follow_type *temp_struct;

    assert(!ch->obj_master);

    ch->obj_master = obj_leader;

    CREATE(temp_struct, struct follow_type, 1);

    temp_struct->follower = ch;
    temp_struct->next = obj_leader->followers;
    obj_leader->followers = temp_struct;

    act("You now follow $p.", FALSE, ch, obj_leader, 0, TO_CHAR);
}

void
add_obj_obj_follower(CHAR_DATA * ch, OBJ_DATA * follower, OBJ_DATA * leader)
{
    char buf[MAX_STRING_LENGTH];
    struct object_follow_type *temp_struct;

    if (follower->wagon_master)
        follower->wagon_master = 0;

    follower->wagon_master = leader;

    CREATE(temp_struct, struct object_follow_type, 1);

    temp_struct->object_follower = follower;
    temp_struct->next = leader->object_followers;
    leader->object_followers = temp_struct;

    sprintf(buf, "$p is now hitched to %s.", OSTR(leader, short_descr));
    act(buf, FALSE, ch, follower, 0, TO_CHAR);
}

/* utility function, usually for corpses */
void
dump_obj_contents(OBJ_DATA * body)
{
    OBJ_DATA *obj, *tmp_obj;

    for (obj = body->contains; obj; obj = tmp_obj) {
        tmp_obj = obj->next_content;
        obj_from_obj(obj);
        if (body->in_obj)
            obj_to_obj(obj, body->in_obj);
        else if (body->carried_by) {
            if (body->carried_by->in_room)
                obj_to_room(obj, body->carried_by->in_room);
            else
                extract_obj(obj);
        } else if (body->in_room)
            obj_to_room(obj, body->in_room);
        /*     else
         * assert (FALSE); */
    }
}


/* procedures related to get */
int
get(CHAR_DATA * ch, OBJ_DATA * obj_object, OBJ_DATA * sub_object, int sub_type, int cmd, char *preHow, char *postHow, int showFailure)
{
    int j = 0, can_carry, single, temp, old_def;
    char buffer[MAX_STRING_LENGTH];
    char keyword[MAX_STRING_LENGTH];
    char keyword2[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    /*   char to_room_string[MAX_STRING_LENGTH]; */
    CHAR_DATA *tmp_ch, *rch;
    int palm_bonus = 0;
    /*  OBJ_DATA *tmp_obj=0; */
    /*
     * I moved all the checks for weight/number of itmes
     * into here, since they are all basicly the same,
     * returning if the move was ok.
     * You still need to check the CAN_SEE before this.
     * A message will always be sent by this procedure.
     */

    if (ch->lifting) {
        sprintf(buffer, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
        send_to_char(buffer, ch);
        if (!drop_lifting(ch, ch->lifting))
            return FALSE;
    }

    /* check furniture for occupants before lifting */
    if (GET_ITEM_TYPE(obj_object) == ITEM_FURNITURE) {
        PLYR_LIST *sCh;

        for (sCh = obj_object->occupants; sCh; sCh = sCh->next) {
            if (sCh->ch && sCh->ch != ch) {
                if (showFailure)
                    act("$N is still sitting on it, $E must stand up first.", FALSE, ch, NULL, sCh->ch, TO_CHAR);
                return FALSE;
            } else if( sCh->ch == ch ) {
                if (showFailure)
                    act("You are still sitting on it, maybe you should stand up first?", FALSE, ch, NULL, NULL, TO_CHAR);
                return FALSE;
            }
        }
    }

    if ((ch->specials.riding) && (obj_object->in_room || (sub_object && sub_object->in_room))) {
        act("You cannot reach the ground from the back of your mount.", FALSE, ch, 0, 0, TO_CHAR);
        return (FALSE);
    }

    if (GET_POS(ch) == POSITION_FIGHTING) {
        for (tmp_ch = ch->in_room->people; tmp_ch; tmp_ch = tmp_ch->next)
            /* ch might be null if the player has been killed */
            if (ch && (GET_POS(tmp_ch) == POSITION_FIGHTING && !trying_to_disengage(tmp_ch))
                && (tmp_ch->specials.fighting == ch || tmp_ch->specials.alt_fighting == ch)) {
                stop_fighting(tmp_ch, ch);

                act("$n attacks as you stop fighting to pick something up.", FALSE, tmp_ch, 0, ch, TO_VICT);
                act("You attack $N as $E stops fighting to pick something up.", FALSE, tmp_ch, 0, ch, TO_CHAR);
                act("$n attacks $N as $E stops fighting to pick something up.", FALSE, tmp_ch, 0, ch, TO_NOTVICT);
                old_def = (int) ch->abilities.def;
                ch->abilities.def -= 25;
                ch->abilities.def = (sbyte) MAX(0, ch->abilities.def - 25);

                /* 11/5/2000 -Tenebrius assume correct if working on 11/5/2001 */
                if (!hit(tmp_ch, ch, TYPE_UNDEFINED)) /* they bees dying, deadlike */
                    return (FALSE);

                if (ch)
                    ch->abilities.def = (sbyte) old_def;
            }
    }

    /* Note: obj_guarded sends all needed messages */
    if (obj_guarded(ch, obj_object, CMD_GET))
        return (FALSE);

    /* Added conditional so if you're affected by Invulnerability */
    /* The glyph doesn't harm you                                 */
    if ((IS_SET(obj_object->obj_flags.extra_flags, OFL_GLYPH))
        && !(affected_by_spell(ch, SPELL_INVULNERABILITY))) {
        act("$p sparks as $n tries to pick it up, burning $s skin.", FALSE, ch, obj_object, 0,
            TO_ROOM);
        act("$p sparks as you try to pick it up, burning your skin.", FALSE, ch, obj_object, 0,
            TO_CHAR);
        generic_damage(ch, dice(obj_object->obj_flags.temp, 4), 0, 0,
                       dice(obj_object->obj_flags.temp, 6));
        REMOVE_BIT(obj_object->obj_flags.extra_flags, OFL_GLYPH);
        REMOVE_BIT(obj_object->obj_flags.extra_flags, OFL_MAGIC);
        return (FALSE);
    }

    if (!CAN_WEAR(obj_object, ITEM_TAKE)) {
        if (showFailure)
            act("You cannot take $p.", 0, ch, obj_object, 0, TO_CHAR);
        return (FALSE);
    }

    /* Palming off a corpse should be a lot harder than your own equipment
     * or furniture since bodies tend to draw attention. -Tiernan 6/8/2005 */
    if (cmd == CMD_PALM && sub_object && IS_CORPSE(sub_object)) {
        palm_bonus -= 15;       // Harder to palm off a corpse
    }
    palm_bonus += agl_app[GET_AGL(ch)].stealth_manip;

    /* Adding in a weight check to determine whether or not the object can
     * be palmed at all.  Later we'll see if they make their skill check and
     * actually get it undetected. -Tiernan 6/8/2005 */
    if (cmd == CMD_PALM && GET_OBJ_WEIGHT(obj_object) > (get_char_size(ch) / 3 + 1)) {
        if (showFailure)
            send_to_char("It is way too heavy, surely someone would notice.\n\r", ch);
        return FALSE;
    }
    // Can't palm items from the room undetected if they can't be stolen from
    // the room either. - Tiernan 6/24/05
    if (cmd == CMD_PALM && (isnamebracketsok("[no_steal]", OSTR(obj_object, name)))
        && (!IS_IMMORTAL(ch)) && (sub_type == FIND_OBJ_ROOM)) {
        if (showFailure)
            act("You couldn't palm that without being noticed.", FALSE, ch, 0, 0, TO_CHAR);
        return FALSE;
    }

    find_obj_keyword(obj_object, FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
                     ch, keyword, sizeof(keyword));

    if (is_merchant(ch) || IS_IMMORTAL(ch)) {
        can_carry = TRUE;
        if (obj_object->lifted_by)
            can_carry = FALSE;
    } else {

        /* this is where it was found, weather or
         * not an actual call to find_object was made
         * ie, get all, will pass in FIND_ROOM       */

        switch (sub_type) {
        case FIND_OBJ_ROOM:
            can_carry = !obj_object->lifted_by && (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj_object)) <= CAN_CARRY_W(ch);
            break;
        case FIND_OBJ_EQUIP:
            can_carry =
                (IS_CARRYING_W(ch) + (GET_OBJ_WEIGHT(obj_object) * 0.65)) <= CAN_CARRY_W(ch);
            break;
        case FIND_OBJ_INV:
            can_carry = IS_CARRYING_W(ch) <= CAN_CARRY_W(ch);
            break;
        default:
            errorlog("with get(ch,obj1,obj2,type)");
            can_carry = 0;
            break;
        }
    }

    if (!can_carry) {
        if (sub_type == FIND_OBJ_ROOM) {
            int old_capacity, new_capacity, obj_weight;

            if (ch->equipment[EP] || ch->equipment[ES]
                || ch->equipment[ETWO] || ch->specials.riding || ch->specials.subduing) {
                act("You cannot carry $p, too heavy.", 0, ch, obj_object, 0, TO_CHAR);
                act("You need to have empty hands if you want to try and lift it.", 0, ch,
                    obj_object, 0, TO_CHAR);
                return FALSE;
            }

            if (ch->desc && ch->desc->str) {
                if (showFailure)
                    send_to_char("Not while editing a string.\n\r", ch);
                return FALSE;
            }

            /* how much weight could the group of people carrying it carry */
            old_capacity = get_lifting_capacity(obj_object->lifted_by);

            /* add new person to the list */
            add_lifting(obj_object, ch);

            /* what's the new weight carrying capacity */
            new_capacity = get_lifting_capacity(obj_object->lifted_by);

            /* store the weight of the object */
            obj_weight = GET_OBJ_WEIGHT(obj_object);

            /* send message about ch lifting the obj */
            /* is it already off the ground? */
            if (old_capacity >= obj_weight) {
                sprintf(to_char, "you lift ~%s", keyword);
                sprintf(to_room, "@ lifts ~%s", keyword);
            } else {            /* have to put their back into it */
                sprintf(to_char, "you lift ~%s with all your strength", keyword);
                sprintf(to_room, "@ strains as #me lifts ~%s", keyword);
            }

            if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
             to_room, to_room,
             (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
                remove_lifting(obj_object, ch);
                return FALSE;
            }

            /* give the status of the object */
            /* was it on the ground, and now off? */
            if (old_capacity < obj_weight && new_capacity >= obj_weight) {
                act("$p lifts off the ground.", 0, ch, obj_object, NULL, TO_CHAR);
                act("$p lifts off the ground.", 0, ch, obj_object, NULL, TO_ROOM);
            }
            /* was it not even half off the ground and now is? */
            else if (old_capacity < (obj_weight / 2)
                     && new_capacity >= (obj_weight / 2)) {
                act("$p half rises from the ground.", 0, ch, obj_object, NULL, TO_CHAR);
                act("$p half rises from the ground.", 0, ch, obj_object, NULL, TO_ROOM);
            } else if (old_capacity < (obj_weight / 2)
                       && new_capacity < (obj_weight / 2)) {
                act("$p doesn't move.", 0, ch, obj_object, NULL, TO_CHAR);
                act("$p doesn't move.", 0, ch, obj_object, NULL, TO_ROOM);
            }

#ifdef NEW_FURNITURE_CHECKS_FOR_GET
            /* 
             * if a furniture object is being picked up
             * (this person make it more than half the objects weight)
             * and the table has stuff around it, drop it in the room.
             */
            if (new_capacity >= (obj_weight / 2)
                && GET_ITEM_TYPE(obj_object) == ITEM_FURNITURE && obj_object->around) {
                OBJ_DATA *around, *around_next;

                for (around = obj_object->around; around; around = around_next) {
                    around_next = around->next_content;
                    obj_from_around_obj(around);
                    obj_to_room(around, ch->in_room);
                }
            }
#endif
            return FALSE;
        }

        if (showFailure)
            act("You cannot carry $p, too heavy.", 0, ch, obj_object, 0, TO_CHAR);
        return (FALSE);
    }

    if (!is_merchant(ch) && !IS_IMMORTAL(ch) 
     && IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        if (showFailure)
            act("You cannot carry $p, you have too many items.", FALSE, ch, 
             obj_object, 0, TO_CHAR);
        return (FALSE);
    }

    /* it has been determined that the object can be moved */
    if (obj_object->ldesc) {
        free(obj_object->ldesc);
        obj_object->ldesc = NULL;
        rem_obj_extra_desc_value(obj_object, "[DROP_DESC]");
    }

#ifdef NEW_FURNITURE_CHECKS_FOR_GET
    /* if a furniture object is being picked up and has stuff around it,
     *  drop it in the room.
     */
    if (GET_ITEM_TYPE(obj_object) == ITEM_FURNITURE && obj_object->around) {
        OBJ_DATA *around, *around_next;

        for (around = obj_object->around; around != NULL; around = around_next) {
            around_next = around->next_content;
            obj_from_around_obj(around);
            obj_to_room(around, ch->in_room);
        }
    }
#endif

    /* reset the values 1-4 for playable type objects when they pick it up */
    if (obj_object->obj_flags.type == ITEM_PLAYABLE)
        for (j = 1; j < 4; j++)
            obj_object->obj_flags.value[j] = 0;

    if (sub_object) {
        if (sub_type == FIND_OBJ_EQUIP) {
            for (j = 0; j < MAX_WEAR && (ch->equipment[j] != sub_object); j++) {        /* skip thru till found */
            };
            if ((j < 0) || (j == MAX_WEAR) || (sub_object != unequip_char(ch, j))) {
                gamelog("Generic find gave bogus results in cmd_get\n");
                return (FALSE);
            };
        } else if (sub_type == FIND_OBJ_INV)
            obj_from_char(sub_object);

#ifdef NESS_MONEY
        if (obj_object->obj_flags.type == ITEM_MONEY)
            for (tmp_obj = ch->carrying; tmp_obj; tmp_obj = tmp_obj->next_content)
                if ((tmp_obj->obj_flags.value[1] == obj_object->obj_flags.value[1])
                    && (tmp_obj->obj_flags.type == ITEM_MONEY))
                    break;
#endif

        obj_from_obj(obj_object);
        obj_to_char(obj_object, ch);

        /* Moved these from after the messages so that cloaks will hide sdesc 
         * -Nessalin 1/20/2001 */
        if (sub_type == FIND_OBJ_EQUIP)
            equip_char(ch, sub_object, j);
        else if (sub_type == FIND_OBJ_INV)
            obj_to_char(sub_object, ch);

        find_obj_keyword(obj_object, 
         FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
         ch, keyword, sizeof(keyword));

        find_obj_keyword(sub_object, 
         FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
         ch, keyword2, sizeof(keyword2));

        /* check for sleight of hand skill, if they have it, no room mesg-sv */
        if ((cmd == CMD_PALM)
            && (ch->skills[SKILL_SLEIGHT_OF_HAND]
                && skill_success(ch, NULL,
                                 ch->skills[SKILL_SLEIGHT_OF_HAND]->learned + palm_bonus))) {
            sprintf(to_char, "you palm ~%s from ~%s", keyword, keyword2);
            sprintf(to_room, "you notice @ palm ~%s from ~%s", keyword, keyword2);

            if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow, to_char,
             to_room, to_room, MONITOR_ROGUE, -20, SKILL_SLEIGHT_OF_HAND)) {
                obj_from_char(obj_object);
                obj_to_obj(obj_object, sub_object);
                return FALSE;
            }
        } else {
            sprintf(to_char, "you get ~%s from ~%s", keyword, keyword2);
            sprintf(to_room, "@ gets ~%s from ~%s", keyword, keyword2);

            if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
             to_room, to_room, 
             (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
                obj_from_char(obj_object);
                obj_to_obj(obj_object, sub_object);
                return FALSE;
            }

            if (cmd == CMD_PALM) {      // Failed to palm
		int witnesses = 0;
                for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
                    if (watch_success(rch, ch, NULL, 20, SKILL_SLEIGHT_OF_HAND)) {
                        cprintf(rch, "You notice %s was trying to be furtive about it.\n\r", PERS(rch, ch));
			witnesses++;
                    } 
                }
		if (witnesses)
                    gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
            } 
        }

        /* Former location of the re-equip/putobj back on PC 
         * -Nessalin 1/20/2001 */

        if (GET_ITEM_TYPE(sub_object) == ITEM_LIGHT && IS_CAMPFIRE(sub_object)) {
            obj_from_fire(obj_object, sub_object);
        }

    } else {
#ifdef NESS_MONEY
        if (obj_object->obj_flags.type == ITEM_MONEY)
            for (tmp_obj = ch->carrying; tmp_obj; tmp_obj = tmp_obj->next_content)
                if ((tmp_obj->obj_flags.value[1] == obj_object->obj_flags.value[1])
                    && (tmp_obj->obj_flags.type == ITEM_MONEY))
                    break;
#endif
        
        sprintf(to_char, "you pick up ~%s", keyword);
        sprintf(to_room, "@ picks up ~%s", keyword);

        if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
         to_room, to_room, 
         (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
            return FALSE;
        }

        obj_from_room(obj_object);
        obj_to_char(obj_object, ch);
    }

    // if it's a light
    if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT) {
        // if extinguishing the object destroys it
        if (extinguish_object(ch, obj_object) == NULL)
            return (FALSE);
    }

    if (obj_object->obj_flags.type == ITEM_MONEY) {
        single = (obj_object->obj_flags.value[0] == 1) ? 1 : 0;
        sprintf(buffer, "There %s %d %s.\n\r", single ? "was" : "were",
                obj_object->obj_flags.value[0], single ? "coin" : "coins");
        send_to_char(buffer, ch);
#ifdef NESS_MONEY
        if (tmp_obj) {
            obj_object->obj_flags.value[0] += tmp_obj->obj_flags.value[0];
            sprintf(buffer, "About to extract %s.\n\r", OSTR(tmp_obj, short_descr));
            extract_obj(tmp_obj);
        }
#else
        money_mgr(MM_OBJ2CH, 0, (void *) obj_object, (void *) ch);
#endif
    }

    sprintf(buffer, "It is %s", encumb_class(get_obj_encumberance(obj_object, ch)));

    switch (obj_object->obj_flags.type) {
    case ITEM_CONTAINER:

        if (!(obj_object->contains)) {
            strcat(buffer, ", and empty.\n\r");
        } else {
            temp =
                GET_OBJ_WEIGHT(obj_object) * 3 /
                (obj_object->obj_flags.value[0] ? obj_object->obj_flags.value[0] : 1);
            temp = MAX(0, MIN(3, temp));

            sprintf(buf2, ", and %sfull.\n\r", fullness[temp]);
            strcat(buffer, buf2);
        }

        break;
    case ITEM_DRINKCON:
        if (obj_object->obj_flags.value[1] <= 0) {
            strcat(buffer, ", and empty.\n\r");
        } else {
            temp = obj_object->obj_flags.value[1] * 3 / obj_object->obj_flags.value[0];
            temp = MAX(0, MIN(3, temp));

            sprintf(buf2, ", and %sfull.\n\r", fullness[temp]);
            strcat(buffer, buf2);
        }
        break;
    default:
        strcat(buffer, ".\n\r");
        break;
    }
    send_to_char(buffer, ch);

    return (TRUE);
}

#define NESS_GET_COMBAT 1

void
cmd_get(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH];
    OBJ_DATA *sub_object;
    OBJ_DATA *obj_object;
    OBJ_DATA *next_obj;
    CHAR_DATA *temp_char;
    bool found = FALSE;
    int type = 3, sub_type;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char orig_arg[MAX_STRING_LENGTH];

    strcpy(orig_arg, argument);
    // Add delay for slip
    if (!ch->specials.to_process && cmd == CMD_PALM) {
        add_to_command_q(ch, orig_arg, cmd, 0, 0);
	return;
    }

    extract_emote_arguments(argument, preHow, sizeof(preHow), args,
                            sizeof(args), postHow, sizeof(postHow));

    argument = two_arguments(args, arg1, sizeof(arg1), arg2, sizeof(arg2));

    /* get type */
    if (!*arg1)
        type = 0;

    if (*arg1 && !*arg2) {
        // if it starts with all
        if (strcasestr(arg1, "all") == arg1)
            type = 1;
        else
            type = 2;
    }

    if (*arg1 && *arg2) {
        if (strcasestr(arg1, "all") == arg1) {
            if (!strcmp(arg2, "all"))
                type = 3;
            else
                type = 4;
        } else {
            if (!strcmp(arg2, "all"))
                type = 5;
            else
                type = 6;
        }
    }

    found = FALSE;

    switch (type) {
    case 0:                    /* get */
        cprintf(ch, "Get what?\n\r");
        break;
    case 1: {                    /* get all */
        char *search = NULL;

        if( strlen(arg1) > 4 && strcasestr(arg1, "all.") == arg1 ) {
           search = arg1 + 4;
        } 

        sub_object = 0;
        for (obj_object = ch->in_room->contents; obj_object && ch->in_room; obj_object = next_obj) {
            next_obj = obj_object->next_content;
            /* Added a check for hidden flags on objs for 'get all'  - Kelvik */
            if (CAN_SEE_OBJ(ch, obj_object)
                && ((search == NULL
                  && !IS_SET(obj_object->obj_flags.extra_flags, OFL_HIDDEN))
                 || (search 
                  && (isname(search, OSTR(obj_object, name)) 
                   || isname(search, real_sdesc_keywords_obj(obj_object, ch)))))){
                if (!get(ch, obj_object, sub_object, FIND_OBJ_ROOM, cmd, preHow, postHow, FALSE)) {
                    continue;
                }
                found = TRUE;

                if (!IS_IMMORTAL(ch)) {
                    if (ch->specials.act_wait <= 0)
                        ch->specials.act_wait = number(3,5);
                    sprintf( buffer, "get %s", arg1);
                    parse_command(ch, buffer);
                    return;
                }
            }
        }

        if (found) 
            send_to_char("OK.\n\r", ch);
        else {
            if( search == NULL )
                send_to_char("You see nothing here.\n\r", ch);
            else
                cprintf(ch, "You do not see any '%s' here.\n\r", search);
        }
        break;
    }
    case 2:                    /* get ??? */
        sub_object = 0;
        obj_object = get_obj_in_list_vis(ch, arg1, ch->in_room->contents);
        if (obj_object)
            get(ch, obj_object, sub_object, FIND_OBJ_ROOM, cmd, preHow, postHow, TRUE);
        else {
            sprintf(buffer, "You do not see a '%s' here.\n\r", arg1);
            send_to_char(buffer, ch);
        }
        break;
    case 3:                    /* get all all */
        send_to_char("That doesn't make sense.\n\r", ch);
        break;
    case 4: {                  /* get all ??? */
        char *search = NULL;

        if( strlen(arg1) > 4 && strcasestr(arg1, "all.") == arg1 ) {
           search = arg1 + 4;
        } 
        sub_type =
            generic_find(arg2, FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV, ch, &temp_char,
                         &sub_object);
        if (sub_type) {
            /* Handle 'get all <fire>' */
            if (GET_ITEM_TYPE(sub_object) == ITEM_FIRE) {
                /* look through the things in the fire */
                for (obj_object = sub_object->contains; obj_object; obj_object = next_obj) {
                    next_obj = obj_object->next_content;

                    /* if you can see it, get it */
                    if (CAN_SEE_OBJ(ch, obj_object)
                     && (search == NULL || (search 
                       && (isname(search, OSTR(obj_object, name)) 
                        || isname(search, real_sdesc_keywords_obj(obj_object, ch)))))){
                        if (!get(ch, obj_object, sub_object, sub_type, cmd, preHow, postHow, FALSE)) 
                            continue;

                        found = TRUE;

                        if (!IS_IMMORTAL(ch)) {
                            if (ch->specials.act_wait <= 0)
                                ch->specials.act_wait = number(3,5);
                            sprintf(buffer, "get %s %s", arg1, arg2);
                            parse_command(ch, buffer);
                            return;
                        }
                    }
                }

                /* if nothing found */
                if (!found) {
                    if( search == NULL )
                        cprintf(ch, "You do not see anything in %s.\n\r", 
                         format_obj_to_char(sub_object, ch, 1));
                    else
                        cprintf(ch, "You do not see anything like '%s' in %s.\n\r",
                         search, format_obj_to_char(sub_object, ch, 1));
                }
            }
            /* Handle 'get all <container>' */
            else if (GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER) {
                if (!is_closed_container(ch, sub_object)) {
                    for (obj_object = sub_object->contains; obj_object; obj_object = next_obj) {
                        next_obj = obj_object->next_content;
                        if (CAN_SEE_OBJ(ch, obj_object)
                         && (search == NULL || (search 
                           && (isname(search, OSTR(obj_object, name)) 
                            || isname(search, real_sdesc_keywords_obj(obj_object, ch)))))){
                            if (!get(ch, obj_object, sub_object, sub_type, cmd, preHow, postHow, FALSE))
                                continue;
                            found = TRUE;

                            if (!IS_IMMORTAL(ch)) {
                                if (ch->specials.act_wait <= 0)
                                    ch->specials.act_wait = number(3,5);
                                sprintf(buffer, "get %s %s", arg1, arg2);
                                parse_command(ch, buffer);
                                return;
                            }
                        }
                    }

                    if (!found) {
                        if( search == NULL )
                            cprintf(ch, "You do not see anything in %s.\n\r", 
                             format_obj_to_char(sub_object, ch, 1));
                        else
                            cprintf(ch, "You do not see anything like '%s' in %s.\n\r",
                             search, format_obj_to_char(sub_object, ch, 1));
                    }
                } else
                    cprintf(ch, "%s is closed.", 
                     capitalize(format_obj_to_char(sub_object, ch, 1)));
            }
            /* Handle 'get all <furniture>' */
            else if (GET_ITEM_TYPE(sub_object) == ITEM_FURNITURE) {
                /* object we think they might be referring to */
                OBJ_DATA *tmp_referring = NULL;

                /* If on an object && referring to their table */
                if (sub_object->carried_by != ch && ch->on_obj
                    && (tmp_referring =
                        is_referring_to_their_table(ch, sub_object, arg2)) == NULL) {
                    /* tell them they can't reach it and stop trying */
                    cprintf(ch, "You can't reach %s from where you are.", 
                     format_obj_to_char(sub_object, ch, 1));
                    return;
                }

                /* switch and use the object they're referring to */
                if (tmp_referring != NULL)
                    sub_object = tmp_referring;

                /* Make sure things can be put on it */
                if (IS_SET(sub_object->obj_flags.value[1], FURN_PUT)) {
                    /* look at all the objects on it */
                    for (obj_object = sub_object->contains; obj_object; obj_object = next_obj) {
                        next_obj = obj_object->next_content;
                        /* if you can see it, get it */
                        if (CAN_SEE_OBJ(ch, obj_object)
                         && (search == NULL || (search 
                           && (isname(search, OSTR(obj_object, name)) 
                            || isname(search, real_sdesc_keywords_obj(obj_object, ch)))))){
                            if (!get(ch, obj_object, sub_object, sub_type, cmd, preHow, postHow, FALSE))
                                continue;
                            found = TRUE;

                            if (!IS_IMMORTAL(ch)) {
                                if (ch->specials.act_wait <= 0)
                                    ch->specials.act_wait = number(3,5);
                                sprintf(buffer, "get %s %s", arg1, arg2);
                                parse_command(ch, buffer);
                                return;
                            }
                        }
                    }

                    /* if not found */
                    if (!found) {
                        if( search == NULL )
                            cprintf(ch, "You do not see anything in %s.\n\r", 
                             format_obj_to_char(sub_object, ch, 1));
                        else
                            cprintf(ch, "You do not see anything like '%s' in %s.\n\r",
                             search, format_obj_to_char(sub_object, ch, 1));
                    }
                } else          /* not FURN_PUT */
                    cprintf(ch, "You can't get anything from %s.\n\r", 
                     format_obj_to_char(sub_object, ch, 1));
            } else              /* can't get anything from it */
                cprintf(ch, "You can't get anything from %s.\n\r", 
                 format_obj_to_char(sub_object, ch, 1));
        } else {
            cprintf(ch, "You do not see or have '%s'.\n\r", arg2);
        }
        break;
    }
    case 5:
        cprintf(ch, "You can't take a thing from more than one container.\n\r");
        break;
    case 6:
        /* get obj obj */
        sub_type =
            generic_find(arg2, FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV, ch, &temp_char,
                         &sub_object);

        if (sub_type) {
            if (GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER) {
                if (!is_closed_container(ch, sub_object)) {
                    /* Always let them get a light source */
                    obj_object = get_obj_in_list(ch, arg1, sub_object->contains);
                    if (obj_object && GET_ITEM_TYPE(obj_object) != ITEM_LIGHT)
                        obj_object = get_obj_in_list_vis(ch, arg1, sub_object->contains);
                    if (obj_object)
                        get(ch, obj_object, sub_object, sub_type, cmd, preHow, postHow, TRUE);
                    else {
                        sprintf(buffer, "The $o does not contain '%s'.\n\r", arg1);
                        act(buffer, 0, ch, sub_object, 0, TO_CHAR);
                    }
                } else {
                    act("$p is closed.", 0, ch, sub_object, 0, TO_CHAR);
                }
            } else if (GET_ITEM_TYPE(sub_object) == ITEM_FURNITURE) {
                /* object we think they might be referring to */
                OBJ_DATA *tmp_referring = NULL;

                /* If on an object && referring to their table */
                if (sub_object->carried_by != ch && ch->on_obj 
                    && (tmp_referring =
                        is_referring_to_their_table(ch, sub_object, arg2)) == NULL) {
                    /* tell them they can't reach it and stop trying */
                    act("You can't reach $p from where you are.", FALSE, ch, sub_object, 0,
                        TO_CHAR);
                    return;
                }

                /* switch and use the object they're referring to */
                if (tmp_referring != NULL)
                    sub_object = tmp_referring;

                /* if the sub_object is a table */
                if (IS_SET(sub_object->obj_flags.value[1], FURN_TABLE)) {
                    OBJ_DATA *around;
                    PLYR_LIST *sCh;

                    /* check to see if a object is around it like arg1 */
                    if ((around = get_obj_in_list_vis(ch, arg1, sub_object->around)) != NULL) {
                        /* make sure noone is sitting on it */
                        for (sCh = around->occupants; sCh; sCh = sCh->next) {
                            if (sCh->ch && sCh->ch != ch) {
                                act("$N is still sitting on it," " $E must stand up first.", FALSE,
                                    ch, NULL, sCh->ch, TO_CHAR);
                                return;
                            }
                        }

                        /* remove it from around and put it in the room */
                        obj_from_around_obj(around);
                        obj_to_room(around, ch->in_room);
                        act("$n pulls $p away from $P.", FALSE, ch, around, sub_object, TO_ROOM);
                        act("You pull $p away from $P.", FALSE, ch, around, sub_object, TO_CHAR);
                        return;
                    } else if (!IS_SET(sub_object->obj_flags.value[1], FURN_PUT)) {
                        /* can't put anything on it, so fail out */
                        act("Nothing like that around $p.", FALSE, ch, sub_object, NULL, TO_CHAR);
                        return;
                    }
                }

                /* if things can be put on this furniture object... */
                if (IS_SET(sub_object->obj_flags.value[1], FURN_PUT)) {
                    /* look and see if we see what they're referring to */
                    obj_object = get_obj_in_list_vis(ch, arg1, sub_object->contains);
                    if (obj_object)
                        get(ch, obj_object, sub_object, sub_type, cmd, preHow, postHow, TRUE);
                    else {
                        sprintf(buffer, "There's no '%s' on $p.\n\r", arg1);
                        act(buffer, 0, ch, sub_object, 0, TO_CHAR);
                    }
                } else {
                    /* can't get anything from it */
                    act("You can't get anything from $p.", FALSE, ch, sub_object, 0, TO_CHAR);
                }
            } else if (GET_ITEM_TYPE(sub_object) == ITEM_LIGHT && IS_CAMPFIRE(sub_object)) {
                obj_object = get_obj_in_list_vis(ch, arg1, sub_object->contains);

                if (obj_object)
                    get(ch, obj_object, sub_object, sub_type, cmd, preHow, postHow, TRUE);
                else if (!strcasecmp(arg1, "brand")
                         || !strcasecmp(arg1, "branch")
                         || !strcasecmp(arg1, "torch")) {
                    if (sub_object->obj_flags.value[0] < 2) {
                        act("You can't find any branches big enough in $p.", FALSE, ch, sub_object,
                            0, TO_CHAR);
                        return;
                    }

                    obj_object = read_object(120, VIRTUAL);

                    if (IS_LIT(sub_object))
                        MUD_SET_BIT(obj_object->obj_flags.value[5], LIGHT_FLAG_LIT);

                    obj_to_obj(obj_object, sub_object);
                    get(ch, obj_object, sub_object, sub_type, cmd, preHow, postHow, TRUE);
                } else {
                    sprintf(buffer, "There's no '%s' in $p.\n\r", arg1);
                    act(buffer, 0, ch, sub_object, 0, TO_CHAR);
                }
            } else {
                act("$p is not a container.", 0, ch, sub_object, 0, TO_CHAR);
            }
        } else {
            sprintf(buffer, "You neither see nor have the '%s'.\n\r", arg2);
            send_to_char(buffer, ch);
        }
        break;

    }
}

int
weight_in_room(struct room_data *room)
{
    CHAR_DATA *ch;
    OBJ_DATA *obj;
    int weight = 0;

    /* add weight of people in room */
    for (ch = room->people; ch; ch = ch->next_in_room)
        weight = weight + ch->player.weight + (calc_carried_weight(ch) / 10);

    /* add weight of objects in room */
    for (obj = room->contents; obj; obj = obj->next_content)
        weight = weight + (calc_object_weight(obj) / 10);

    return weight;
}


bool
is_cargo_bay(ROOM_DATA *rm) 
{
   int zone = rm->number / 1000;

   if (IS_SET(rm->room_flags, RFL_CARGO_BAY)
    || zone == 3 || zone == 13 || zone == 33 || zone== 73)
      return TRUE;

   return FALSE;
}


/* This program returns a 1 (TRUE) if the character can drop the item in
   question.  If they cannot it will both send the correct error message
   for why they cannot drop it as well as returning a 0 (FALSE).
   -the Shade of Nessalin 7/27/96                          */
int
can_drop(CHAR_DATA * ch, OBJ_DATA * obj)
{
    char buffer[MAX_STRING_LENGTH];     /* for sending messages */
    char buffer2[MAX_STRING_LENGTH];    /* for sending messages */
    int weight = 0;             /* stores the weight of the room */
    int rm_max_weight = 0;

    /* Item is NO_DROP? */
    if (IS_SET(obj->obj_flags.extra_flags, OFL_NO_DROP)) {
        sprintf(buffer, "You cannot drop the %s, it is cursed!.\n\r",
                first_name(OSTR(obj, name), buffer2));
        send_to_char(buffer, ch);

        return 0;
    }

    /* There is enough space in the room for the object? */
    if (is_cargo_bay(ch->in_room)) {
        weight = get_room_weight(ch->in_room);

        rm_max_weight = get_room_max_weight(ch->in_room);

        if (!IS_IMMORTAL(ch)
         && (weight + calc_object_weight_for_cargo_bay(obj)) > rm_max_weight) {
            send_to_char("There's no more room for that here.\n\r", ch);
            return 0;
        }
    }
    /* end cargo_bays */
    return 1;                   /* No problems sending a TRUE back to the calling procedure */
}


/* this is for if GET_LIST is defined */
int
drop_lifting(CHAR_DATA * ch, OBJ_DATA * lifting)
{
    int old_capacity, new_capacity, obj_weight;

    old_capacity = get_lifting_capacity(lifting->lifted_by);
    remove_lifting(lifting, ch);
    new_capacity = get_lifting_capacity(lifting->lifted_by);

    obj_weight = GET_OBJ_WEIGHT(lifting);

    act("You stop lifting $p.", TRUE, ch, lifting, 0, TO_CHAR);
    act("$n stops lifting $p.", TRUE, ch, lifting, 0, TO_ROOM);

    if (old_capacity >= (obj_weight / 2)
        && new_capacity < (obj_weight / 2)) {
        act("$p settles to the ground.", 0, ch, lifting, 0, TO_CHAR);
        act("$p settles to the ground.", 0, ch, lifting, 0, TO_ROOM);
    } else if (old_capacity >= obj_weight && new_capacity < obj_weight) {
        act("$p half rests on the ground.", 0, ch, lifting, 0, TO_CHAR);
        act("$p half rests on the ground.", 0, ch, lifting, 0, TO_ROOM);
    }
    return 1;
}

int
do_dropdesc(CHAR_DATA *ch, OBJ_DATA *obj, const char *argument) {
    char buf[MAX_INPUT_LENGTH];

    if (obj->ldesc)
        free(obj->ldesc);

    if (strstr(argument, "%s")) {
        send_to_char("You can't use '%s' in drops.\n\r", ch);
        return FALSE;
    }
    else if (strstr(argument, "~")) {
        char *tmp;
        const char *pos;

        buf[0] = '\0';
        tmp = buf;

        /* copy up to the '~' */
        for (pos = argument; *pos != '\0' && *pos != '~'; pos++, tmp++)
            *tmp = *pos;

        /* pos should be '~' now */
        pos++;

        /* null terminate buf */
        *tmp = '\0';

        /* if they didn't put a space after it */
        /* ignore till space */
        while (*pos && !ispunct(*pos) && !isspace(*pos))
            pos++;

        strcat(buf, "%s");
        if (*pos)
            strcat(buf, pos);
    }
    else {
        sprintf(buf, "%s", argument);
    }
    obj->ldesc = strdup(buf);
    set_obj_extra_desc_value(obj, "[DROP_DESC]", buf);
    return TRUE;
}

void
cmd_arrange(CHAR_DATA *ch, const char *argument, Command cmd, int count) {
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args,
     sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, arg, sizeof(arg));

    obj = get_obj_room_vis(ch, arg);

    if (obj == NULL) {
        cprintf(ch, "You don't see a '%s' here to arrange.\n\r", arg);
        return;
    }

    if (!*argument) {
        cprintf(ch, "How do you want to arrange it?\n\r");
        return;
    }

    if (IS_SET(obj->obj_flags.extra_flags, OFL_HIDDEN)) {
        cprintf(ch, "You are unable to arrange %s.\n\r", format_obj_to_char(obj, ch, 1));
        return;
    }

    // need to do this here, as we have to send messages before doing the ddesc
    if (strstr(argument, "%s")) {
        send_to_char("You can't use '%s' in arrange.\n\r", ch);
        return;
    }

    find_obj_keyword(obj, 
     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
     ch, buf, sizeof(buf));

    sprintf(to_char, "you arrange ~%s", buf);
    sprintf(to_room, "@ arranges ~%s", buf);

    if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
     to_room, to_room, MONITOR_OTHER)) {
        return;
    }

    if (!do_dropdesc(ch, obj, argument)) {
        return;
    }

    cprintf(ch, "Shown to the room as:\n\r%s\n\r",
     format_obj_to_char(obj, ch, 0));

    if (!IS_SWITCHED_IMMORTAL(ch) && !CAN_WEAR(obj, ITEM_TAKE)) {
        if (IS_NPC(ch)) {
            if (!ch->desc) {
                gamelogf("%s [m%d] is arranging no-taked '%s' [o%d]:\n\r  %s",
                 GET_NAME(ch), npc_index[ch->nr].vnum, OSTR(obj, short_descr), 
                 obj_index[obj->nr].vnum, format_obj_to_char(obj, ch, 0));
            }
            else {
                gamelogf("%s (%s) is arranging '%s' no-taked [o%d]:\n\r  %s",
                 GET_NAME(ch), ch->desc->player_info->name,
                 OSTR(obj, short_descr), 
                 obj_index[obj->nr].vnum, format_obj_to_char(obj, ch, 0));
            }
        } else {
            gamelogf("%s (%s) is arranging '%s' no-taked [o%d]:\n\r  %s",
             GET_NAME(ch), ch->account, OSTR(obj, short_descr), 
             obj_index[obj->nr].vnum, format_obj_to_char(obj, ch, 0));
        }
    }
    return;
}

void
cmd_drop(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *tmp_object, *next_obj;
#ifdef NESS_MONEY
    OBJ_DATA *cash_obj, *new_cash;
#endif
    int amount;                 /* if they're dropping money */
    char arg[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    bool test = FALSE;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char keyword[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args,
     sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, arg, sizeof(arg));

    if (!stricmp("coins", arg) || !stricmp("coin", arg)
     || !stricmp("sids", arg) || !stricmp("sid", arg)) {
        strcpy(arg2, arg);
        if (!stricmp("coins", arg) || !stricmp("sids", arg))
            sprintf(arg, "%d", GET_OBSIDIAN(ch));
        else if (!stricmp("coin", arg) || !stricmp("sid", arg))
            strcpy(arg, "1");
    } else if (is_number(arg)) {
        argument = one_argument(argument, arg2, sizeof(arg2));
    }

    if (is_number(arg)) {
        amount = atoi(arg);
       
#ifdef NESS_MONEY
        if (!(cash_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
            act("You do not have that money item.", TRUE, ch, 0, 0, TO_CHAR);
            return;
        } else {
            if (cash_obj->obj_flags.type != ITEM_MONEY) {
                act("You can only use numbers with money.", TRUE, ch, 0, 0, TO_CHAR);
                return;
            } else {
                find_obj_keyword(cash_obj, 
                 FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
                 ch, keyword, sizeof(keyword));

                if (amount < 1) {
                    act("You have to drop at least one coin.", TRUE, ch, 0, 0, TO_CHAR);
                    return;
                }
                if (amount > cash_obj->obj_flags.value[0]) {
                    act("You don't have that many.", TRUE, ch, 0, 0, TO_CHAR);
                    return;
                }
                if (amount == cash_obj->obj_flags.value[0]) {
                    sprintf(to_char, "you drop ~%s", keyword);
                    sprintf(to_room, "@ drops ~%s", keyword);

                    if (!send_to_char_and_room_parsed(ch, preHow, postHow, 
                     to_char, to_room, to_room, MONITOR_OTHER)) {
                        return;
                    }

                    obj_from_char(cash_obj);
                    obj_to_room(cash_obj, ch->in_room);
                    return;
                } else {
                    new_cash = read_object(coins[cash_obj->obj_flags.value[1]].many_vnum, VIRTUAL);
                    if (!new_cash) {
                        gamelog("Money item not found in database, using Allanak " "coins.");
                        new_cash = read_object(1998, VIRTUAL);
                        if (!new_cash) {
                            gamelog("Allanak coins not found in database, exiting " "out of drop.");
                            return;
                        } else
                            new_cash->obj_flags.value[1] = cash_obj->obj_flags.value[1];
                    }
                    new_cash->obj_flags.value[0] = amount;
                    cash_obj->obj_flags.value[0] = cash_obj->obj_flags.value[0] - amount;

                    sprintf(to_char, "you drop ~%s", keyword);
                    sprintf(to_room, "@ drops ~%s", keyword);

                    if (!send_to_char_and_room_parsed(ch, preHow, postHow, 
                     to_char, to_room, to_room, MONITOR_OTHER)) {
                        return;
                    }

                    obj_to_room(new_cash, ch->in_room);
                    return;
                }
            }
        }
#else
        if (strcmp("coins", arg2) && strcmp("coin", arg2)
         && strcmp("sids", arg2) && strcmp("sid", arg2)) {
            send_to_char("When dropping numbers use 'coin' or 'coins'.\n\r", ch);
            send_to_char("ex. 'drop 20 coins' or 'drop 1 coin'.\n\r", ch);
        } else if (amount < 1)
            send_to_char("You have to drop at least one coin.\n\r", ch);
        else if (GET_OBSIDIAN(ch) < amount) { 
            send_to_char("You haven't got that many coins.\n\r", ch);
        } else {
            sprintf(to_char, "you drop %s coin%s",
             amount > 1 ? numberize(amount) : "a",
             amount == 1 ? "" : "s");

            sprintf(to_room, "@ drops %s coin%s",
             amount > 1 ? numberize(amount) : "a",
             amount == 1 ? "" : "s");

            if (!send_to_char_and_room_parsed(ch, preHow, postHow, 
             to_char, to_room, to_room, MONITOR_OTHER)) {
                return;
            }

            money_mgr(MM_CH2OBJ, amount, (void *) ch, 0);
        } 
        return;
#endif
    } else if (*arg) {          /* make sure they're even trying to drop something */
        char *search = NULL;
        bool dropall = FALSE;

        // Check to see if the arg starts with "all"
        if (strcasestr(arg, "all") == arg) {
            if( strlen(arg) > 4 && arg[3] == '.' ) {
                    search = arg + 4;
                    dropall = TRUE;
            } else if(!strcmp(arg, "all")) // This is "all ", maybe w/ drop-desc
                    dropall = TRUE;
        }

        if (dropall) {      /* begin droping all */
            if (ch->lifting)
                if (drop_lifting(ch, ch->lifting))
                    test = TRUE;

            for (tmp_object = ch->carrying; tmp_object; tmp_object = next_obj) {        /* Go through the loop and drop each item */
                next_obj = tmp_object->next_content;
                if (search == NULL || (search
                   && (isname(search, OSTR(tmp_object, name))
                  || isname(search, real_sdesc_keywords_obj(tmp_object, ch))))){
                    if (can_drop(ch, tmp_object)) {
                        // Drop desc provided?
                        if (argument && argument[0] != '\0')
                            do_dropdesc(ch, tmp_object, argument); 
                        if (*preHow || *postHow) {
                            sprintf(to_char, "you drop ~%s", keyword);
                            sprintf(to_room, "@ drops ~%s", keyword);

                            if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
                                return;
                            }
                            cprintf(ch, "Shown to the room as:\n\r%s",
                            format_obj_to_char(tmp_object, ch, 0));
                        }
                        else if (ch->in_room) {
                            switch (ch->in_room->sector_type) {
                            case SECT_CAVERN:
                            case SECT_SOUTHERN_CAVERN:
                            case SECT_HILLS:
                            case SECT_MOUNTAIN:
                                sprintf(buf, "You drop $p, which falls to the rocky ground.\n\r%s",
                                        format_obj_to_char(tmp_object, ch, 0));
                                act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                act("$n drops $p, which falls to the rocky ground.", TRUE, ch,
                                    tmp_object, 0, TO_ROOM);
                                break;
                            case SECT_DESERT:
                                sprintf(buf, "You drop $p, which settles to the sand.\n\r%s",
                                        format_obj_to_char(tmp_object, ch, 0));
                                act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                act("$n drops $p, which settles to the sand.", TRUE, ch, tmp_object, 0,
                                    TO_ROOM);
                                break;
                            case SECT_COTTONFIELD:
                            case SECT_FIELD:
                            case SECT_RUINS:
                            case SECT_THORNLANDS:
                                sprintf(buf, "You drop $p, which falls to the dusty ground.\n\r%s",
                                        format_obj_to_char(tmp_object, ch, 0));
                                act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                act("$n drops $p, which falls to the dusty ground.", TRUE, ch,
                                    tmp_object, 0, TO_ROOM);
                                break;
                            case SECT_SALT_FLATS:
                                sprintf(buf, "You drop $p, which falls to the salty ground.\n\r%s",
                                        format_obj_to_char(tmp_object, ch, 0));
                                act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                act("$n drops $p, which falls to the salty ground.", TRUE, ch,
                                    tmp_object, 0, TO_ROOM);
                                break;
                            case SECT_SEWER:
                                sprintf(buf, "You drop $p, which settles into the sludge.\n\r%s",
                                        format_obj_to_char(tmp_object, ch, 0));
                                act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                act("$n drops $p, which settles into the sludge.", TRUE, ch, tmp_object,
                                    0, TO_ROOM);
                                break;
                            case SECT_SILT:
                            case SECT_SHALLOWS:
                                if (tmp_object->obj_flags.type == ITEM_FURNITURE
                                    && IS_SET(tmp_object->obj_flags.value[1], FURN_SKIMMER)) {
                                    sprintf(buf, "You drop $p, which rests in the silt.\n\r%s",
                                            format_obj_to_char(tmp_object, ch, 0));
                                    act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                    act("$n drops $p, which rests in the silt.", TRUE, ch, tmp_object,
                                        0, TO_ROOM);
                                } else {
                                    sprintf(buf,
                                            "You drop $p, which is swallowed up by the silt.\n\r%s",
                                            format_obj_to_char(tmp_object, ch, 0));
                                    act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                    act("$n drops $p, which is swallowed up by the silt.", TRUE, ch,
                                        tmp_object, 0, TO_ROOM);
                                }
                                break;
                            default:
                                sprintf(buf, "You drop $p.\n\r%s",
                                        format_obj_to_char(tmp_object, ch, 0));
                                act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                act("$n drops $p.", TRUE, ch, tmp_object, 0, TO_ROOM);
                                break;
                            }
                        }

                        if (IS_SET(tmp_object->obj_flags.extra_flags, OFL_DISINTIGRATE))
                            extract_obj(tmp_object);
                        else {
                            obj_from_char(tmp_object);
                            obj_to_room(tmp_object, ch->in_room);
                        }

                        if (!IS_IMMORTAL(ch)) {
                            if (ch->specials.act_wait <= 0)
                                ch->specials.act_wait = number(3,5);
                            sprintf( buf, "drop %s", arg);
                            parse_command(ch, buf);
                            return;
                        }

                        test = TRUE;        /* verify something was dropped */
                    }  /* end able to drop the item */
                }  /* end is targetting the item */
            }  /* end drop all loop */

            if (!test)
                send_to_char("You do not seem to have anything.\n\r", ch);
        } /* end drop all */
        else {                  /* begin drop single item */
            CHAR_DATA *temp_char;
            int sub_type = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP,
                                        ch, &temp_char, &tmp_object);
            if (tmp_object) {   /* Item they're attempting to drop is found */
                find_obj_keyword(tmp_object, 
                 FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
                 ch, keyword, sizeof(keyword));

                if (can_drop(ch, tmp_object)) { /* Able to drop item */

                    if (argument && argument[0] != '\0') {
                        if (!do_dropdesc(ch, tmp_object, argument)) {
                            return;
                        }
                    }

                    /* need to remove it first */
                    if (sub_type == FIND_OBJ_EQUIP) {
                        int j = 0;

                        for (j = 0; j < MAX_WEAR; j++)
                            if (tmp_object == ch->equipment[j])
                                break;

                        if (j < MAX_WEAR) {
                            if (j != ES && j != EP && j != ETWO) {
                                act("You must remove $p first.", FALSE, ch, tmp_object, 0, TO_CHAR);
                                return;
                            }

                            obj_to_char(unequip_char(ch, j), ch);
                        }
                    }

                    if (*preHow || *postHow) {
                        sprintf(to_char, "you drop ~%s", keyword);
                        sprintf(to_room, "@ drops ~%s", keyword);

                        if (!send_to_char_and_room_parsed(ch, preHow, postHow, 
                         to_char, to_room, to_room, MONITOR_OTHER)) {
                            return;
                        }
                        cprintf(ch, "Shown to the room as:\n\r%s",
                         format_obj_to_char(tmp_object, ch, 0));
                    }
                    else if (ch->in_room)
                        switch (ch->in_room->sector_type) {
                        case SECT_CAVERN:
                        case SECT_SOUTHERN_CAVERN:
                        case SECT_HILLS:
                        case SECT_MOUNTAIN:
                            sprintf(buf,
                                    "You drop $p, which falls to the rocky ground. Shown to the room as:\n\r%s",
                                    format_obj_to_char(tmp_object, ch, 0));
                            act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                            act("$n drops $p, which falls to the rocky ground.", TRUE, ch,
                                tmp_object, 0, TO_ROOM);
                            break;
                        case SECT_DESERT:
                            sprintf(buf,
                                    "You drop $p, which settles to the sand. Shown to the room as:\n\r%s",
                                    format_obj_to_char(tmp_object, ch, 0));
                            act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                            act("$n drops $p, which settles to the sand.", TRUE, ch, tmp_object, 0,
                                TO_ROOM);
                            break;
                        case SECT_COTTONFIELD:
                        case SECT_FIELD:
                        case SECT_RUINS:
                        case SECT_THORNLANDS:
                            sprintf(buf,
                                    "You drop $p, which falls to the dusty ground. Shown to the room as:\n\r%s",
                                    format_obj_to_char(tmp_object, ch, 0));
                            act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                            act("$n drops $p, which falls to the dusty ground.", TRUE, ch,
                                tmp_object, 0, TO_ROOM);
                            break;
                        case SECT_SALT_FLATS:
                            sprintf(buf,
                                    "You drop $p, which falls to the salty ground. Shown to the room as:\n\r%s",
                                    format_obj_to_char(tmp_object, ch, 0));
                            act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                            act("$n drops $p, which falls to the salty ground.", TRUE, ch,
                                tmp_object, 0, TO_ROOM);
                            break;
                        case SECT_SILT:
                        case SECT_SHALLOWS:
                            if (tmp_object->obj_flags.type == ITEM_FURNITURE
                                && IS_SET(tmp_object->obj_flags.value[1], FURN_SKIMMER)) {
                                sprintf(buf, "You drop $p, which rests in the silt.\n\r%s",
                                        format_obj_to_char(tmp_object, ch, 0));
                                act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                act("$n drops $p, which rests in the silt.", TRUE, ch, tmp_object,
                                    0, TO_ROOM);
                            } else {
                                sprintf(buf,
                                        "You drop $p, which is swallowed up by the silt.\n\r%s",
                                        format_obj_to_char(tmp_object, ch, 0));
                                act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                                act("$n drops $p, which is swallowed up by the silt.", TRUE, ch,
                                    tmp_object, 0, TO_ROOM);
                            }
                            break;
                        default:
                            sprintf(buf, "You drop $p.  Shown to the room as:\n\r%s",
                                    format_obj_to_char(tmp_object, ch, 0));
                            act(buf, TRUE, ch, tmp_object, 0, TO_CHAR);
                            act("$n drops $p.", TRUE, ch, tmp_object, 0, TO_ROOM);
                            break;

                        }
                    obj_from_char(tmp_object);
                    obj_to_room(tmp_object, ch->in_room);
                }               /* unable to drop item */
            } else if (ch->lifting && (isname(arg, OSTR(ch->lifting, name)))) {  /* End item found */
                drop_lifting(ch, ch->lifting);
            } else
                act("You do not have that item.", TRUE, ch, 0, 0, TO_CHAR);
        }                       /* End drop single item */
    } else /* End verficiation that they're trying to drop something */
        act("Drop what?", TRUE, ch, 0, 0, TO_CHAR);
}

int
put(CHAR_DATA * ch, OBJ_DATA * obj_object, int obj_type, OBJ_DATA * sub_object,
   int sub_type, int cmd, char *preHow, char *postHow, int showFailure,
   int furn_type, int putting_coins, int amount)
{
    int j = 0;
    char keyword[MAX_STRING_LENGTH];
    char keyword2[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *rch;
    int slip_bonus = 0;

    if( !obj_object || !sub_object ) return FALSE;

    if (obj_object == sub_object) {
        if( showFailure ) 
            cprintf(ch, "You can't put %s in itself.",
             format_obj_to_char(sub_object, ch, 1));
        return FALSE;
    }

    // Cursed and cargo bay size check
    if (sub_object->in_room && !can_drop(ch, obj_object)) {
        return FALSE;
    }
    
    // max size per item in container check
    if (GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER
     && sub_object->obj_flags.value[4] > 0
     && GET_OBJ_WEIGHT(obj_object) > sub_object->obj_flags.value[4]
     && GET_ITEM_TYPE(obj_object) != ITEM_MONEY) {
        if( showFailure ) {
            sprintf( buf, "%s is too large to fit in %%s.\n\r",
             capitalize(format_obj_to_char( obj_object, ch, 1)));
            cprintf(ch, buf, format_obj_to_char( sub_object, ch, 1));
        }

        if (putting_coins) {
            money_mgr(MM_OBJ2CH, 0, (void *) obj_object, (void *) ch);
        }
        return FALSE;
    }

    // max weight on a piece of furniture
    if (GET_ITEM_TYPE(sub_object) != ITEM_LIGHT && furn_type != 2
        && (GET_OBJ_WEIGHT(sub_object) + GET_OBJ_WEIGHT(obj_object)) >=
        sub_object->obj_flags.value[0]) {
        if( showFailure ) {
            sprintf( buf, "%s will not fit on %%s.\n\r",
             capitalize(format_obj_to_char( obj_object, ch, 1)));
            cprintf(ch, buf, format_obj_to_char( sub_object, ch, 1));
        }
        if (putting_coins) {
            money_mgr(MM_OBJ2CH, 0, (void *) obj_object, (void *) ch);
        }
        return FALSE;
    }

    if (GET_ITEM_TYPE(sub_object) == ITEM_LIGHT && IS_CAMPFIRE(sub_object)
        && obj_object->max_hit_points >= sub_object->obj_flags.value[1]) {
        if( showFailure ) {
            sprintf( buf, 
             "%s is too small, you would ruin it if you put %%s in.\n\r",
             capitalize(format_obj_to_char( obj_object, ch, 1)));
            cprintf(ch, buf, format_obj_to_char( sub_object, ch, 1));
        }
        if (putting_coins) {
            money_mgr(MM_OBJ2CH, 0, (void *) obj_object, (void *) ch);
        }
        return FALSE;
    }
    
    /* need to remove it first */
    if (obj_type == FIND_OBJ_EQUIP) {
        int j = 0;
    
        for (j = 0; j < MAX_WEAR; j++)
            if (obj_object == ch->equipment[j])
                break;

        if (j < MAX_WEAR) {
            if (j != ES && j != EP && j != ETWO) {
                if (GET_POS(ch) == POSITION_FIGHTING) {
                    cprintf(ch, "Not while fighting!\n\r");
                    if (putting_coins) {
                        money_mgr(MM_OBJ2CH, 0, (void *) obj_object, (void *) ch);
                    }
                    return FALSE;
                }
                act("[Removing $p first.]", FALSE, ch, obj_object, 0, TO_CHAR);
                act("$n removes $p.", FALSE, ch, obj_object, 0, TO_ROOM);

                if (j == WEAR_BELT) {
                    if (ch->equipment[WEAR_ON_BELT_1]) {
                        char buffer[MAX_STRING_LENGTH];
                        char buffer1[MAX_STRING_LENGTH];
                        char output[MAX_STRING_LENGTH];

                        sprintf(buffer, "[ Taking off %s",
                                OSTR(ch->equipment[WEAR_ON_BELT_1], short_descr));
                        obj_to_char(unequip_char(ch, WEAR_ON_BELT_1), ch);

                        if (ch->equipment[WEAR_ON_BELT_2]) {
                            sprintf(buffer1, "and %s",
                                    OSTR(ch->equipment[WEAR_ON_BELT_2], short_descr));
                            obj_to_char(unequip_char(ch, WEAR_ON_BELT_2), ch);
                        } else
                            strcpy(buffer1, "");
                        sprintf(output, "%s %s first. ]\n\r", buffer, buffer1);
                        send_to_char(output, ch);
                    }
                }

                if ((obj_object->obj_flags.type == ITEM_WORN
                  || obj_object->obj_flags.type == ITEM_CONTAINER)
                 && IS_SET(obj_object->obj_flags.value[1], CONT_HOODED)
                 && IS_SET(obj_object->obj_flags.value[1], CONT_HOOD_UP)) {
                    REMOVE_BIT(obj_object->obj_flags.value[1], CONT_HOOD_UP);
                    act("$n lowers the hood of $p.", TRUE, ch, obj_object, NULL, TO_ROOM);
                    act("You lower the hood of $p.", FALSE, ch, obj_object, NULL, TO_CHAR);
                }
            }

            obj_to_char(unequip_char(ch, j), ch);
        }
    }

    if ((GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER
      || (GET_ITEM_TYPE(sub_object) == ITEM_LIGHT && !IS_CAMPFIRE(sub_object))
      || (GET_ITEM_TYPE(sub_object) == ITEM_FURNITURE
       && !IS_SET(sub_object->obj_flags.value[1], FURN_PUT)))
     && GET_ITEM_TYPE(obj_object) == ITEM_LIGHT && IS_LIT(obj_object)) {
        obj_object = extinguish_object(ch, obj_object);
        if (obj_object == NULL)
            return FALSE;
    }

    /* How large/heavy the object is you're palming is based on your size.
     * The larger you are, the larger an object you can slip -Tiernan */
    if (cmd == CMD_SLIP 
     && GET_OBJ_WEIGHT(obj_object) > MIN(get_char_size(ch) / 3 + 1, 5)) {
        if( showFailure ) 
            cprintf(ch, "%s is way too heavy, surely someone would notice.\n\r",
             format_obj_to_char(obj_object, ch, 1));
        if (putting_coins) {
            money_mgr(MM_OBJ2CH, 0, (void *) obj_object, (void *) ch);
        }
        return FALSE;
    }

    find_obj_keyword(obj_object,
     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
     ch, keyword, sizeof(keyword));

    find_obj_keyword(sub_object,
     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
     ch, keyword2, sizeof(keyword2));

    slip_bonus += agl_app[GET_AGL(ch)].stealth_manip;

    /* it has been moved, send the messages */
    if (GET_ITEM_TYPE(sub_object) != ITEM_FURNITURE) {
        if ((cmd == CMD_SLIP)
            && (ch->skills[SKILL_SLEIGHT_OF_HAND]
                && skill_success(ch, NULL,
                                 ch->skills[SKILL_SLEIGHT_OF_HAND]->learned + slip_bonus))) {
            sprintf(to_char, "you slip ~%s into ~%s", keyword, keyword2);
            sprintf(to_room, "@ slips ~%s into ~%s", keyword, keyword2);
            if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow, to_char,
             to_room, to_room, MONITOR_ROGUE, -20, SKILL_SLEIGHT_OF_HAND)) {
                if (putting_coins)  {
                    GET_OBSIDIAN(ch) += amount; 
                    extract_obj(obj_object);
                }
                return FALSE;
            }
        } else {
            sprintf(to_char, "you put ~%s into ~%s", keyword, keyword2);
            sprintf(to_room, "@ puts ~%s into ~%s", keyword, keyword2);

            if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
             to_room, to_room, 
             (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
                if (putting_coins)  {
                    GET_OBSIDIAN(ch) += amount; 
                    extract_obj(obj_object);
                }
                return FALSE;
            }

            if (cmd == CMD_SLIP) {      // Failed to slip
		int witnesses = 0;
                for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
                    if (watch_success(rch, ch, NULL, 20, SKILL_SLEIGHT_OF_HAND)) {
                        cprintf(rch, "You notice %s was trying to be furtive about it.\n\r", PERS(rch, ch));
			witnesses++;
                    } 
                }
		if (witnesses)
                    gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
            } 
        }
    } else if (furn_type == 2) {
        sprintf(to_char, "you pull ~%s around ~%s", keyword, keyword2);
        sprintf(to_room, "@ pulls ~%s around ~%s", keyword, keyword2);

        if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
         to_room, to_room, 
         (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
            if (putting_coins)  {
                GET_OBSIDIAN(ch) += amount; 
                extract_obj(obj_object);
            }
            return FALSE;
        }
    } else {
        if ((cmd == CMD_SLIP)
            && (ch->skills[SKILL_SLEIGHT_OF_HAND]
                && skill_success(ch, NULL,
                                 ch->skills[SKILL_SLEIGHT_OF_HAND]->learned + slip_bonus))) {
            sprintf(to_char, "you slip ~%s onto ~%s", keyword, keyword2);
            sprintf(to_room, "@ slips ~%s onto ~%s", keyword, keyword2);
            if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow, to_char,
             to_room, to_room, MONITOR_ROGUE, -20, SKILL_SLEIGHT_OF_HAND)) {
                if (putting_coins)  {
                    GET_OBSIDIAN(ch) += amount; 
                    extract_obj(obj_object);
                }
                return FALSE;
            }
        } else {
            sprintf(to_char, "you put ~%s onto ~%s", keyword, keyword2);
            sprintf(to_room, "@ puts ~%s onto ~%s", keyword, keyword2);

            if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
             to_room, to_room, 
             (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
                if (putting_coins)  {
                    GET_OBSIDIAN(ch) += amount; 
                    extract_obj(obj_object);
                }
                return FALSE;
            }

            if (cmd == CMD_SLIP) {      // Failed to slip
		int witnesses = 0;
                for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
                    if (watch_success(rch, ch, NULL, 20, SKILL_SLEIGHT_OF_HAND)) {
                        cprintf(rch, "You notice %s was trying to be furtive about it.\n\r", PERS(rch, ch));
			witnesses++;
                    } 
                }
		if (witnesses)
                    gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
            } 
        }
    }

    /* 
     * obj_object is ok, and it will fit into the sub_object,
     * now, remove both objects, put obj_object into sub_object
     * and return the sub_object to the person. This will avoid
     * tinkering with the players weight/num_items.
     */

    /* take object from char if on person */
    switch (sub_type) {
    case FIND_OBJ_ROOM:
        break;
    case FIND_OBJ_EQUIP:
        for (j = 0; j < MAX_WEAR && (ch->equipment[j] != sub_object); j++) {    /* skip thru till found */
        }

        if ((j < 0) || (j == MAX_WEAR)
            || (sub_object != unequip_char(ch, j))) {
            gamelog("Generic find gave bogus results in cmd_put\n");
            return FALSE;
        }
        break;
    case FIND_OBJ_INV:
        obj_from_char(sub_object);
        break;
    default:
        gamelog("Error in cmd_put\n");
        return FALSE;
    }

    if (obj_object->in_room)
        obj_from_room(obj_object);
    else
        obj_from_char(obj_object);

    /* move the object's, now that they have been removed */
    if (furn_type == 2)
        obj_around_obj(obj_object, sub_object);
    else
        obj_to_obj(obj_object, sub_object);

    /* move the sub_object back onto the player, unless in room */
    switch (sub_type) {
    case FIND_OBJ_EQUIP:
        if (j == MAX_WEAR) {
            gamelog("Generic find gave bogus results in cmd_put(2)\n");
            return FALSE;
        };
        equip_char(ch, sub_object, j);
        break;
    case FIND_OBJ_ROOM:
        break;
    case FIND_OBJ_INV:
        obj_to_char(sub_object, ch);
        break;
    default:
        gamelog("Error in cmd_get(2)\n");
        return FALSE;
    }

    if (GET_ITEM_TYPE(sub_object) == ITEM_LIGHT && IS_CAMPFIRE(sub_object)) {
        obj_to_fire(obj_object, sub_object);
    }

    return TRUE;
}

void
cmd_put(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int sub_type, putting_coins, amount = 0;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char arg3[MAX_STRING_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    CHAR_DATA *tmp_char;

    /*  OBJ_DATA *new_cash; */
    OBJ_DATA *obj_object;
    OBJ_DATA *sub_object;

#ifdef NESS_MONEY
    OBJ_DATA *cash_obj;
#endif

    int furn_type = 0;
    char orig_arg[MAX_STRING_LENGTH];

    strcpy(orig_arg, argument);
    // Add delay for palm
    if (!ch->specials.to_process && cmd == CMD_SLIP) {
        add_to_command_q(ch, orig_arg, cmd, 0, 0);
	return;
    }

    extract_emote_arguments(argument, preHow, sizeof(preHow), args,
     sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));

    if (!*arg1 || !*arg2) {
        act("What do you want to put, and what do you want to put it in/on?", FALSE, ch, 0, 0,
            TO_CHAR);
        return;
    }

    if (!stricmp("coins", arg1) || !stricmp("coin", arg1)
     || !stricmp("sids", arg1) || !stricmp("sid", arg1)) {
        strcpy(arg3, arg2);
        strcpy(arg2, arg1);
        if (!stricmp("coins", arg1)|| !stricmp("sids", arg1)) 
            sprintf(arg1, "%d", GET_OBSIDIAN(ch));
        else
            strcpy(arg1, "1");
    }

    if (*arg3 && is_number(arg1)) {
        if (stricmp("coins", arg2) && stricmp("coin", arg2) && stricmp("obsidian", arg2) && stricmp("sids", arg2) && stricmp("sid", arg2)) {
            send_to_char("When putting numbers use 'coin' or 'coins'.\n\r", ch);
            send_to_char("ex. 'put 20 coins bag' or 'put 1 coin bag'.\n\r", ch);
            return;
        }
        putting_coins = TRUE;
    } else
        putting_coins = FALSE;

    if (*arg3 && !is_number(arg1)) {
        if (!strcmp(arg2, "on")) {
            furn_type = 1;
            strcpy(arg2, arg3);
            arg3[0] = '\0';
        } else if (!strcmp(arg2, "around")) {
            furn_type = 2;
            strcpy(arg2, arg3);
            arg3[0] = '\0';
        }
    }

    if (putting_coins)
        sub_type =
            generic_find(arg3, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char,
                         &sub_object);
    else
        sub_type =
            generic_find(arg2, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char,
                         &sub_object);

    if (!sub_type) {
        send_to_char("You neither see nor have that item to put into.\n\r", ch);
        return;
    }

    if (GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER) {
        if (is_closed_container(ch, sub_object)) {
            act("$P seems to be closed.", FALSE, ch, 0, sub_object, TO_CHAR);
            return;
        }
    } else if (GET_ITEM_TYPE(sub_object) == ITEM_LIGHT && IS_CAMPFIRE(sub_object)) {
        /* do nothing */
    } else if (GET_ITEM_TYPE(sub_object) == ITEM_FIRE) {
        /* make sure the fire is big enough to put it in...or something */
    } else if (GET_ITEM_TYPE(sub_object) == ITEM_FURNITURE) {
        /* object we think they might be referring to */
        OBJ_DATA *tmp_referring = NULL;

        /* If on an object && referring to their table */
        if (sub_object->carried_by != ch && ch->on_obj
            && (tmp_referring = is_referring_to_their_table(ch, sub_object, arg2)) == NULL) {
            /* tell them they can't reach it and stop trying */
            act("You can't reach $p from where you are.", FALSE, ch, sub_object, 0, TO_CHAR);
            return;
        }

        /* switch and use the object they're referring to */
        if (tmp_referring != NULL)
            sub_object = tmp_referring;

        if (!IS_SET(sub_object->obj_flags.value[1], FURN_PUT)
            && !IS_SET(sub_object->obj_flags.value[1], FURN_TABLE)) {
            act("You can't put anything on or around $P.", FALSE, ch, 0, sub_object, TO_CHAR);
            return;
        }
    } else {
        act("You can't put anything in, around or on $P.", FALSE, ch, 0, sub_object, TO_CHAR);
        return;
    }

    if (ch->lifting) {
        char buf[512];
        sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
        send_to_char(buf, ch);
        if (!drop_lifting(ch, ch->lifting))
            return;
    }

    /* 
     * at this point we have determined the subject object to be valid.
     * It has not been removed from the character yet.
     */

    if (putting_coins) {        /* coins section */

#ifdef NESS_MONEY
        amount = atoi(arg1);

        if (!(cash_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
            act("You do not have that money item.", TRUE, ch, 0, 0, TO_CHAR);
            return;
        } else {                /* character is carrying an object with right keyword */
            if (cash_obj->obj_flags.type != ITEM_MONEY) {
                act("You can only use numbers with money.", TRUE, ch, 0, 0, TO_CHAR);
                return;
            } else {            /* they are trying to put an item that is TYPE_MONEY */
                if (amount < 1) {
                    act("You have to drop at least one coin.", TRUE, ch, 0, 0, TO_CHAR);
                    return;
                }
                if (amount > cash_obj->obj_flags.value[0]) {
                    sprintf(buf, "There are not %d coins in %s.", amount,
                            OSTR(cash_obj, short_descr));
                    act(buf, TRUE, ch, 0, 0, TO_CHAR);
                    return;
                }
                if (amount == cash_obj->obj_flags.value[0])
                    obj_object = cash_obj;
                else {          /* creating new money object */
                    obj_object =
                        read_object(coins[cash_obj->obj_flags.value[1]].many_vnum, VIRTUAL);
                    sprintf(buf, "Created %s (obj: %d).\n\r", OSTR(obj_object, short_descr),
                            coins[cash_obj->obj_flags.value[1]].many_vnum);
                    act(buf, TRUE, ch, 0, 0, TO_CHAR);

                    if (!obj_object) {
                        gamelog("Money item not found in database, using Allanak " "coins.");
                        obj_object = read_object(1998, VIRTUAL);
                        if (!obj_object) {
                            gamelog("Allanak coins not found in database, exiting " "out of drop.");
                            return;
                        } else
                            obj_object->obj_flags.value[1] = cash_obj->obj_flags.value[1];
                    }

                    obj_object->obj_flags.value[0] = amount;

                    if ((GET_OBJ_WEIGHT(sub_object) + GET_OBJ_WEIGHT(obj_object)) >=
                        sub_object->obj_flags.value[0]) {
                        act("$p will not fit inside $P.", FALSE, ch, obj_object, sub_object,
                            TO_CHAR);
                        extract_obj(obj_object);
                        return;
                    }

                    cash_obj->obj_flags.value[0] -= amount;
                }
            }
        }
#else
        /* begin put coins */
        amount = atoi(arg1);
        if (amount < 1) {
            cprintf(ch, "How silly!\n\r");
            return;
        }

        if (amount > GET_OBSIDIAN(ch)) {
            cprintf(ch, "You don't have %d (%s) coins, putting %d coins instead.\n\r", amount, arg1, GET_OBSIDIAN(ch));
            amount = GET_OBSIDIAN(ch);
        }

        if (!(obj_object = create_money(amount, COIN_ALLANAK))) {
            gamelog("cmd_put: Unable to create money object.");
            return;
        }

        obj_to_char(obj_object, ch);

        if ((GET_OBJ_WEIGHT(sub_object) + GET_OBJ_WEIGHT(obj_object)) >=
            sub_object->obj_flags.value[0]) {
            act("$p will not fit inside $P.", FALSE, ch, obj_object, sub_object, TO_CHAR);
            extract_obj(obj_object);
            return;
        }

        GET_OBSIDIAN(ch) -= amount;     /* amount was checked above */

        put(ch, obj_object, FIND_OBJ_INV, sub_object, sub_type, cmd, preHow,
         postHow, TRUE, furn_type, putting_coins, amount);

#endif
    } /* end put coins */
    else {
        if( strcasestr(arg1, "all") == arg1 ) {
            bool found = FALSE;
            char *search = NULL;
            char buffer[MAX_STRING_LENGTH];
            OBJ_DATA *next_obj;

            if( strlen(arg1) > 4 && strcasestr(arg1, "all.") == arg1 ) {
               search = arg1 + 4;
            } 

            for (obj_object = ch->carrying; obj_object; obj_object = next_obj) {
                next_obj = obj_object->next_content;
                /* Added a check for hidden flags on objs for 'get all'  - Kelvik */
                if (CAN_SEE_OBJ(ch, obj_object)
                    && (search == NULL || (search 
                      && (isname(search, OSTR(obj_object, name)) 
                       || isname(search, real_sdesc_keywords_obj(obj_object, ch)))))){
                    if (!put(ch, obj_object, FIND_OBJ_INV, sub_object, sub_type, cmd, preHow, postHow, TRUE, furn_type, FALSE, 0)) {
                        continue;
                    }
                    found = TRUE;
    
                    if (!IS_IMMORTAL(ch)) {
                        if (ch->specials.act_wait <= 0)
                            ch->specials.act_wait = number(3,5);
                        sprintf( buffer, "put %s %s", arg1, arg2);
                        parse_command(ch, buffer);
                        return;
                    }
                }
            }
    
            if (found) 
                send_to_char("OK.\n\r", ch);
            else {
                if( search == NULL )
                    send_to_char("You are not carrying anything.\n\r", ch);
                else
                    cprintf(ch, "You are not carrying any '%s'.\n\r", search);
            }
        } else {
            /* begin put object */
            CHAR_DATA *temp_char;
            int obj_type = generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP,
                                        ch, &temp_char, &obj_object);
            if (!obj_object) {
                if (sub_object->obj_flags.type == ITEM_FURNITURE
                    && IS_SET(sub_object->obj_flags.value[1], FURN_TABLE)
                    && sub_type == FIND_OBJ_ROOM && furn_type != 1) {
                    OBJ_DATA *around;
                    int count = 0;
    
                    furn_type = 2;
                    if ((obj_object = get_obj_in_list_vis(ch, arg1, ch->in_room->contents)) == NULL) {
                        send_to_char("You do not see that item here.\n\r", ch);
                        return;
                    }
    
                    if (obj_object->obj_flags.type != ITEM_FURNITURE
                        || (obj_object->obj_flags.type == ITEM_FURNITURE
                            && !IS_SET(obj_object->obj_flags.value[1], FURN_SIT))) {
                        send_to_char("You can't pull that up to it.\n\r", ch);
                        return;
                    }
    
                    for (around = sub_object->around; around; around = around->next_content)
                        count += around->obj_flags.value[2];
    
                    if ((count_plyr_list(sub_object->occupants, ch)
                         + count + obj_object->obj_flags.value[2])
                        > sub_object->obj_flags.value[2]) {
                        act("$p is too full to bring $P up to.", FALSE, ch, sub_object, obj_object,
                            TO_CHAR);
                        return;
                    }
                } else {
                    send_to_char("You do not have that item.\n\r", ch);
                    return;
                }
            }

            put(ch, obj_object, obj_type, sub_object, sub_type, cmd, preHow,
             postHow, TRUE, furn_type, putting_coins, amount);

        }
    }
}


void
cmd_plant(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int roll, skill, ct;
    char arg1[256], arg2[256];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj = NULL, *item = NULL;
    CHAR_DATA *tar_ch = NULL;
    CHAR_DATA *rch;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char real_name_buf[MAX_STRING_LENGTH];
    char victim_name[256] = "", item_name[256] = "";
    int eq_pos = -1;

    if (hands_free(ch) < 1) {
        send_to_char("You'll need at least one free hand to do that.\n\r", ch);
        return;
    }

    if (ch->specials.riding) {
        send_to_char("You can't do that while riding.\n\r", ch);
        return;
    }

    extract_emote_arguments(argument, preHow, sizeof(preHow), args,
                            sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));

    if (!*arg1 || !*arg2) {
        send_to_char("Usage: plant <obj> <character|room>\n\r", ch);
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
        send_to_char("You don't have that object.\n\r", ch);
        return;
    }

    if (!strcasecmp(arg2, "room")) {    /* They want to drop it quietly. */
        tar_ch = NULL;
    } else {				/* They are targetting someone. */
        /* Need to check for person's item here */
        strcpy(victim_name, arg2);
        if (!tar_ch && (strlen(victim_name) > 2)
            && (((victim_name[strlen(victim_name) - 2] == '\'')
                 && (victim_name[strlen(victim_name) - 1] == 's'))
                || (((victim_name[strlen(victim_name) - 1] == '\'')
                     && (victim_name[strlen(victim_name) - 2] == 's'))))) {
            /* crop off the possessives */
            if (victim_name[strlen(victim_name) - 1] == '\'')
                victim_name[strlen(victim_name) - 1] = 0;   /* nul terminate */
            else
                victim_name[strlen(victim_name) - 2] = 0;   /* nul terminate */

            tar_ch = get_char_room_vis(ch, victim_name);
            argument = one_argument(argument, item_name, sizeof(item_name));
        } else if (!(tar_ch = get_char_room_vis(ch, arg2))) {
            send_to_char("That person isn't here.\n\r", ch);
            return;
        }

        if ((GET_OBJ_WEIGHT(obj) > 3) && (tar_ch) && (!IS_IMMORTAL(ch))) {
            act("$p is too heavy.  $E'd surely notice if you tried.", FALSE, ch, obj, tar_ch, TO_CHAR);
            return;
        }

        // Can't plant on yourself or immortals of higher rank
        if (tar_ch == ch) {
            send_to_char("Come on now, that's rather stupid.\n\r", ch);
            return;
        } else if (GET_LEVEL(ch) < GET_LEVEL(tar_ch) && !IS_NPC(tar_ch)) {
            send_to_char("You really don't want to do that.\n\r", ch);
            return;
        }
    }

    /* compute chance of success */
    skill = ch->skills[SKILL_STEAL] ? ch->skills[SKILL_STEAL]->learned : 0;
    skill = MAX(skill, 30);

    if (IS_SET(ch->in_room->room_flags, RFL_POPULATED))
        skill += 30;

    if (IS_SET(ch->in_room->room_flags, RFL_INDOORS))
        skill = (skill * 3 / 4);

    if (tar_ch) {
        /* if target is watching them give a  penalty */
        if (is_char_actively_watching_char(tar_ch, ch))
            skill -= get_watched_penalty(tar_ch);

        if (is_merchant(tar_ch))
            skill = -1;
        if (GET_POS(tar_ch) <= POSITION_SLEEPING)
            skill = 100;
    } else if (!can_drop(ch, obj)) {
        return;
    }

    skill = MIN(100, skill);
    skill = MAX(1, skill);

    if (IS_IMMORTAL(ch))
        skill = 101;

    roll = number(1, 101);
    if (roll <= skill) {
        if (!tar_ch) {          /* drop it in the room quietly */
            obj_from_char(obj);
            obj_to_room(obj, ch->in_room);

            if (argument && argument[0] != '\0') {
                if (!do_dropdesc(ch, obj, argument)) {
                    return;
		}
	    }
            sprintf(to_char, "you set down %s", OSTR(obj, short_descr));
	    sprintf(to_room, "@ sets down %s", OSTR(obj, short_descr));

            if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow,
		to_char, to_room, to_room, MONITOR_ROGUE, -20, SKILL_STEAL)) {
		act("You carefully set $p down.", FALSE, ch, obj, 0, TO_CHAR);
	    }
	    cprintf(ch, "Shown to the room as:\n\r%s", format_obj_to_char(obj, ch, 0));
	    return;
        }
	// There's a targetted character
        if ((GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(tar_ch)) > CAN_CARRY_W(tar_ch)) {
            act("$N can't carry that much weight.", FALSE, ch, 0, tar_ch, TO_CHAR);
            return;
        }

	/* A good place to do the plant into worn-obj stuff */
	if (*item_name) {
            if (!(item = get_object_in_equip_vis(ch, item_name, tar_ch->equipment, &eq_pos))) {
                act("$E doesn't have anything like that.", FALSE, ch, 0, tar_ch, TO_CHAR);
                return;
	    }

            if (is_covered(ch, tar_ch, eq_pos) && !IS_IMMORTAL(ch)) {
                act("Surely, $E would notice you planting into that.\n\r", FALSE, ch, 0, tar_ch, TO_CHAR);
                return;
            }

            if (item->obj_flags.type != ITEM_CONTAINER) {
                act("You can't plant anything into $p.", FALSE, ch, item, tar_ch, TO_CHAR);
                return;
            } else if (is_closed_container(ch, item) && !IS_IMMORTAL(ch)) {
                act("$p is closed.", FALSE, ch, item, tar_ch, TO_CHAR);
                return;
            } else if ((GET_OBJ_WEIGHT(item) + GET_OBJ_WEIGHT(obj)) >=
                        item->obj_flags.value[0]) {
                act("$p will not fit inside $P.", FALSE, ch, obj, item, TO_CHAR);
		return;
	    }

	    // Okay, plant it
            sprintf(to_char, "you plant %s into %%%s %s", OSTR(obj, short_descr), FORCE_NAME(tar_ch), OSTR(item, short_descr));
	    sprintf(to_room, "@ plants something on %s", FORCE_NAME(tar_ch));
	    // Echo it
            if (!send_hidden_to_char_vict_and_room_parsed(ch, tar_ch, preHow, postHow, to_char, NULL,
                to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
                act("You slip $p into $S pocket without $M noticing.", FALSE, ch, obj, tar_ch, TO_CHAR);
	    // logging stuff
            snprintf(buf, sizeof(buf), "%s plants %s into %s's container.\n\r",
                GET_NAME(ch), OSTR(obj, short_descr), GET_NAME(tar_ch));
            shhlogf("%s: %s", __FUNCTION__, buf);
	    // Move it
	    obj_from_char(obj);
	    obj_to_obj(obj, item);
	} else {
            // Can the victim's inventory handle one more thing?
            if ((1 + IS_CARRYING_N(tar_ch)) > CAN_CARRY_N(tar_ch)) {
                act("$N seems to have $S hands full.", FALSE, ch, 0, tar_ch, TO_CHAR);
                return;
            }
	    // Okay, plant it to inventory instead
            sprintf(to_char, "you plant %s on ~%s", OSTR(obj, short_descr), FORCE_NAME(tar_ch));
	    sprintf(to_room, "@ plants something on ~%s", FORCE_NAME(tar_ch));
	    // Echo it
            if (!send_hidden_to_char_vict_and_room_parsed(ch, tar_ch, preHow, postHow, to_char, NULL,
                to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
                act("You slip $p into $S pocket without $M noticing.", FALSE, ch, obj, tar_ch, TO_CHAR);
	    // logging stuff
            snprintf(buf, sizeof(buf), "%s plants %s on %s.\n\r",
                GET_NAME(ch), OSTR(obj, short_descr), GET_NAME(tar_ch));
            shhlogf("%s: %s", __FUNCTION__, buf);
            // Plant it into inventory
            obj_from_char(obj);
            obj_to_char(obj, tar_ch);

	}
    } else {
        if (!tar_ch) {          /* They failed to drop it quietly. */
            cmd_drop(ch, arg1, 0, 0);
            for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
                if (ch != rch && watch_success(rch, ch, NULL, 20, SKILL_STEAL)) {
                    sprintf(buf, "%s", format_obj_to_char(obj, rch, 1));
                    cprintf(rch, "You notice %s trying to be sneaky about dropping %s.\n\r", PERS(rch, ch), buf);
                }
            }
            return;
        }

        roll = number(1, 101);
        if (roll > skill) {
            REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
            act("$N catches you trying to plant something on $M.", FALSE, ch, 0, tar_ch, TO_CHAR);
            act("$n tries to plant something on you.", FALSE, ch, 0, tar_ch, TO_VICT);
            act("$n tries to plant something on $N.", TRUE, ch, 0, tar_ch, TO_NOTVICT);

            check_theft_flag(ch, tar_ch);
            if (IS_NPC(tar_ch)) {
                if (GET_RACE_TYPE(tar_ch) == RTYPE_HUMANOID) {
                    cmd_shout(tar_ch, "Thief! Thief!", 0, 0);
                }
                if ((ct = room_in_city(ch->in_room)) == CITY_NONE)
                    hit(tar_ch, ch, TYPE_UNDEFINED);
                else if (!IS_SET(tar_ch->specials.act, CFL_SENTINEL))
                    cmd_flee(tar_ch, "", 0, 0);
            }
        } /* End char got seen */
        else {                  /* Char wasn't seen */
            act("You cover your mistake before $E discovers you.", FALSE, ch, 0, tar_ch, TO_CHAR);
            act("You feel a hand in your belongings, but are unable to " "catch the culprit.",
                FALSE, ch, 0, tar_ch, TO_VICT);

            sprintf(to_char, "you fail to plant %s on ~%s", OSTR(obj, short_descr), FORCE_NAME(tar_ch));
            sprintf(to_room, "@ tries to plant something on ~%s", FORCE_NAME(tar_ch));
            if (!send_hidden_to_char_vict_and_room_parsed(ch, tar_ch, preHow,
				 postHow, to_char, NULL, to_room, to_room, MONITOR_ROGUE, 0, SKILL_STEAL))
		return;

            if (IS_NPC(tar_ch)) {
                if (GET_RACE_TYPE(tar_ch) == RTYPE_HUMANOID) {
                    cmd_shout(tar_ch, "Thief! Thief!", 0, 0);
                } else if (!IS_SET(tar_ch->specials.act, CFL_SENTINEL)) {
                    cmd_flee(tar_ch, "", 0, 0);
                }
            }
        }                       /* Char isn't seen */
    }
}


void
cmd_give(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char obj_name[256], vict_name[256];
    char arg[256];
    int amount;
    CHAR_DATA *vict;
    OBJ_DATA *obj;
#ifdef NESS_MONEY
    OBJ_DATA *new_cash;
#endif
    char args[MAX_STRING_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_vict[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow,
                            sizeof(postHow));

    argument = one_argument(args, obj_name, sizeof(obj_name));

#ifdef NESS_MONEY
    if (is_number(obj_name)) {
        amount = atoi(obj_name);

        argument = one_argument(argument, arg, sizeof(arg));

        if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
            send_to_char("Sorry, you don't seem to have any of those.\n\r", ch);
            return;
        }

        if (obj->obj_flags.type != ITEM_MONEY) {
            send_to_char("Sorry, numbers can only be used with money.\n\r", ch);
            return;
        }

        if (amount <= 0) {
            send_to_char("Sorry, you must give at least 1 coin.\n\r", ch);
            return;
        }

        if (obj->obj_flags.value[0] < amount) {
            send_to_char("You haven't got that many of those coins!\n\r", ch);
            return;
        }

        argument = one_argument(argument, vict_name, sizeof(vict_name));

        if (!*vict_name) {
            send_to_char("To whom?\n\r", ch);
            return;
        }

        if (!(vict = get_char_room_vis(ch, vict_name))) {
            send_to_char("To whom?\n\r", ch);
            return;
        }

        new_cash = read_object(coins[obj->obj_flags.value[1]].many_vnum, VIRTUAL);

        new_cash->obj_flags.value[0] = amount;
        obj->obj_flags.value[0] -= amount;

        obj_to_char(new_cash, ch);

        find_obj_keyword(new_cash, FIND_OBJ_INV, ch, obj_name, sizeof(obj_name));

        sprintf(to_char, "you give ~%s to ~%s", obj_name, vict_name);
        sprintf(to_vict, "@ gives you ~%s", obj_name);
        sprintf(to_room, "@ gives ~%s to ~%s", obj_name, vict_name);

        // if the emote fails, move the cash back
        if (!send_to_char_vict_and_room_parsed
            (ch, vict, preHow, postHow, to_char, to_vict, to_room, to_room,
             (preHow[0] != '\0' || postHow[0] != '\0')
             ? MONITOR_OTHER : MONITOR_NONE)) {
            // failed, restore cash to original object
            obj_from_char(new_cash);
            obj->obj_flags.value[0] += amount;
            return;
        }
        // success, move object to vict
        obj_from_char(new_cash);
        obj_to_char(new_cash, vict);
        return;
    }
#else
    if (is_number(obj_name)) {
        amount = atoi(obj_name);
        argument = one_argument(argument, arg, sizeof(arg));
        if (strcmp("coins", arg) && strcmp("coin", arg)
         && strcmp("sids", arg) && strcmp("sid", arg)) {
            send_to_char("Sorry, you can't do that.\n\r", ch);
            return;
        }
        if (amount <= 0) {
            send_to_char("Sorry, you can't do that.\n\r", ch);
            return;
        }
        if (GET_OBSIDIAN(ch) < amount) {
            send_to_char("You haven't got that many coins!\n\r", ch);
            return;
        }
        argument = one_argument(argument, vict_name, sizeof(vict_name));
        if (!*vict_name) {
            send_to_char("To whom?\n\r", ch);
            return;
        }
        if (!(vict = get_char_room_vis(ch, vict_name))) {
            send_to_char("To whom?\n\r", ch);
            return;
        }

        if (amount == 1) {
            sprintf(to_char, "you give ~%s a coin", vict_name);
            sprintf(to_vict, "@ gives you a coin");
        } else {
            sprintf(to_char, "you give ~%s %d coins", vict_name, amount);
            sprintf(to_vict, "@ gives you %d coins", amount);
        }
        sprintf(to_room, "@ gives some coins to ~%s", vict_name);

        if (!send_to_char_vict_and_room_parsed
            (ch, vict, preHow, postHow, to_char, to_vict, to_room, to_room,
             (preHow[0] != '\0' || postHow[0] != '\0')
             ? MONITOR_OTHER : MONITOR_NONE)) {
            return;
        }

        money_mgr(MM_CH2CH, amount, (void *) ch, (void *) vict);
        return;
    }
#endif

    argument = one_argument(argument, vict_name, sizeof(vict_name));

    if (!*obj_name || !*vict_name) {
        send_to_char("Give what to who?\n\r", ch);
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying))) {
        send_to_char("You do not seem to have anything like that.\n\r", ch);
        return;
    }
    find_obj_keyword(obj, FIND_OBJ_INV, ch, obj_name, sizeof(obj_name));

    if (IS_SET(obj->obj_flags.extra_flags, OFL_NO_DROP)) {
        send_to_char("You can't let go of it! Yeech!!\n\r", ch);
        return;
    }

    if (!(vict = get_char_room_vis(ch, vict_name))) {
        send_to_char("No one by that name around here.\n\r", ch);
        return;
    }

    if (((1 + IS_CARRYING_N(vict)) > CAN_CARRY_N(vict))
        && !is_merchant(vict)
        && !IS_IMMORTAL(vict)) {
        act("$N seems to have $S hands full.", 0, ch, 0, vict, TO_CHAR);
        return;
    }

    /* Added 7/6/96 by Nessalin. Temporary, will remove if voted down */
    if (is_merchant(vict)
        && !IS_IN_SAME_TRIBE(ch, vict)
        && !IS_IMMORTAL(ch)) {
        act("$N politely refuses your offer.", 0, ch, 0, vict, TO_CHAR);
        return;
    }

    if (IS_SET(vict->specials.act, CFL_MOUNT) && !IS_IMMORTAL(ch)) {
        act("$N, being a mount, does not accept your offer. Try the pack " "command.", 0, ch, 0,
            vict, TO_CHAR);
        return;
    }
    /* end Nessalin's temporary additions */

    if ((GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
        && !IS_IMMORTAL(vict)
        && !is_merchant(vict)) {
        act("$E can't carry that much weight.", 0, ch, 0, vict, TO_CHAR);
        return;
    }

    sprintf(to_char, "you give ~%s to ~%s", obj_name, vict_name);
    sprintf(to_vict, "@ gives you ~%s", obj_name);
    sprintf(to_room, "@ gives ~%s to ~%s", obj_name, vict_name);

    if (!send_to_char_vict_and_room_parsed
        (ch, vict, preHow, postHow, to_char, to_vict, to_room, to_room,
         (preHow[0] != '\0' || postHow[0] != '\0')
         ? MONITOR_OTHER : MONITOR_NONE)) {
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, vict);

    if (!CAN_SEE_OBJ(vict, obj)) {
        act("It falls from your hands.", FALSE, vict, 0, NULL, TO_CHAR);
        act("$N fumbles and drops it.", FALSE, vict, obj, NULL, TO_ROOM);
        obj_from_char(obj);
        obj_to_room(obj, vict->in_room);
        return;
    }
}


/* outa here!   Ur

   void weight_change_object(OBJ_DATA *obj, int weight)
   {
   OBJ_DATA *tmp_obj;
   CHAR_DATA *tmp_ch;

   if (obj->in_room) {
   GET_OBJ_WEIGHT(obj) += weight;
   GET_OBJ_WEIGHT(obj) = MAX(GET_OBJ_WEIGHT(obj), 1);
   } else if (tmp_ch = obj->carried_by) {
   obj_from_char(obj);
   GET_OBJ_WEIGHT(obj) += weight;
   GET_OBJ_WEIGHT(obj) = MAX(GET_OBJ_WEIGHT(obj), 1);
   obj_to_char(obj, tmp_ch);
   } else if (tmp_obj = obj->in_obj) {
   obj_from_obj(obj);
   GET_OBJ_WEIGHT(obj) += weight;
   GET_OBJ_WEIGHT(obj) = MAX(GET_OBJ_WEIGHT(obj), 1);
   obj_to_obj(obj, tmp_obj);
   } else {
   gamelog("Unknown attempt to subtract weight from an object.");
   }
   }

 */


void
cmd_drink(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    OBJ_DATA *temp, *vial;
    int amount;
    int hunger_value, thirst_value, drunk_value;

    if (GET_POS(ch) == POSITION_FIGHTING) {
        send_to_char("Not while fighting.\n\r", ch);
        return;
    }

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow,
                            sizeof(postHow));

    argument = one_argument(args, buf, sizeof(buf));


    {
        int rval;
        CHAR_DATA *tar_ch;
        rval =
            generic_find(buf,
                         FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_LIQ_CONT_INV |
                         FIND_LIQ_CONT_ROOM, ch, &tar_ch, &temp);
        if ((rval &
             (FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_LIQ_CONT_INV |
              FIND_LIQ_CONT_ROOM)) == 0) {
            act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    }

    if (obj_guarded(ch, temp, CMD_DRINK))
        return;

    if ((temp->obj_flags.type != ITEM_DRINKCON) && (temp->obj_flags.type != ITEM_CURE)) {
        act("You can't drink from that!", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* If the item is alcoholic, you can keep drinking regardless of hunger */
    /* or thirst.  You can drink til you pass out. */
    if ((!drink_aff[temp->obj_flags.value[2]][DRUNK]) && (GET_RACE(ch) != RACE_VAMPIRE)) {
        if (GET_COND(ch, THIRST) > (get_normal_condition(ch, THIRST) - 4))
        {                       /* they are full in thirst */
            if (temp->obj_flags.value[1] > 0)
                act("Your stomach can't contain anymore!", FALSE, ch, 0, 0, TO_CHAR);
            else
                act("It's empty already.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    }

    if (ch->lifting) {
        sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
        send_to_char(buf, ch);
        if (!drop_lifting(ch, ch->lifting))
            return;
    }

    find_obj_keyword(temp, FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, buf,
                     sizeof(buf));

    if (temp->obj_flags.type == ITEM_CURE) {
        if (temp->obj_flags.value[1]) { /* not in vial form */
            act("You can't drink from that!", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        sprintf(to_char, "you drink from ~%s", buf);
        sprintf(to_room, "@ drinks from ~%s", buf);

        if (!send_to_char_and_room_parsed
            (ch, preHow, postHow, to_char, to_room, to_room,
             (preHow[0] != '\0' || postHow[0] != '\0')
             ? MONITOR_OTHER : MONITOR_NONE)) {
            return;
        }

        if (affected_by_spell(ch, (temp->obj_flags.value[0] + 219)))
            affect_from_char(ch, (temp->obj_flags.value[0] + 219));
        if (temp->obj_flags.value[2])
            poison_char(ch, temp->obj_flags.value[2], number(1, 3), 0);
        extract_obj(temp);
        /* ok.. they've drank the vial.. return to them the empty vial */
        if (!(vial = read_object(13248, VIRTUAL))) {
            gamelog("Error in cmd_drink.  Can't find object #13248.");
            return;
        }
        obj_to_char(vial, ch);
        return;
    }

    if (temp->obj_flags.type == ITEM_DRINKCON) {
        if (temp->obj_flags.value[1] > 0) {     /* Not empty */

            if ((drink_aff[temp->obj_flags.value[2]][DRINK_CONTAINER_RACE] < 0)
                || (GET_RACE(ch) == drink_aff[temp->obj_flags.value[2]][DRINK_CONTAINER_RACE])) {
                hunger_value = drink_aff[temp->obj_flags.value[2]][DRINK_CONTAINER_FULL];
                thirst_value = drink_aff[temp->obj_flags.value[2]][DRINK_CONTAINER_THIRST];
                drunk_value = drink_aff[temp->obj_flags.value[2]][DRINK_CONTAINER_DRUNK];
            } else {
                hunger_value = drink_aff[temp->obj_flags.value[2]][DRINK_CONTAINER_FULL2];
                thirst_value = drink_aff[temp->obj_flags.value[2]][DRINK_CONTAINER_THIRST2];
                drunk_value = drink_aff[temp->obj_flags.value[2]][DRINK_CONTAINER_DRUNK2];
            }


            sprintf(to_char, "you drink the %s", drinks[temp->obj_flags.value[2]]);
            sprintf(to_room, "@ drinks %s from ~%s", drinks[temp->obj_flags.value[2]], buf);

            if (!send_to_char_and_room_parsed
                (ch, preHow, postHow, to_char, to_room, to_room,
                 (preHow[0] != '\0' || postHow[0] != '\0')
                 ? MONITOR_OTHER : MONITOR_NONE)) {
                return;
            }

            amount = number(3, 10);

            amount = MIN(amount, temp->obj_flags.value[1]);

            if (drunk_value)
                make_char_drunk(ch, drunk_value);

            /* Decrement the drink container by amount drank */
            if (!IS_SET(temp->obj_flags.value[3], LIQFL_INFINITE)) {
                temp->obj_flags.value[1] -= amount;
            };

            if (temp->obj_flags.value[1] < 1) {
                temp->obj_flags.value[2] = 0;
                temp->obj_flags.value[3] = 0;
            };

            if (affected_by_spell(ch, SPELL_FIREBREATHER)
                && (temp->obj_flags.value[2] == LIQ_WATER)) {
                act("As you drink from $p you feel an intense burning sensation.", FALSE, ch,
                    temp, 0, TO_CHAR);
                act("$n grasps $s chest and screams in pain.", FALSE, ch, temp, 0, TO_ROOM);
                generic_damage(ch, amount * 4, 0, 0, amount * 4);
                if (GET_POS(ch) == POSITION_DEAD) {
                    act("You grab your chest and scream as the pain consumes your pitiful life.",
                        FALSE, ch, 0, 0, TO_CHAR);
                    die(ch);
                } else
                    act("You grab your chest and scream in pain.", FALSE, ch, 0, 0, TO_CHAR);
                return;
            }

            /* Vampires only get nourisment from biting */
            if (GET_RACE(ch) != RACE_VAMPIRE)
                gain_condition(ch, FULL, (int) hunger_value * amount / 4);

            gain_condition(ch, THIRST, (int) thirst_value * amount / 4);

            if (GET_COND(ch, THIRST) > (get_normal_condition(ch, THIRST) - 4))
                act("You do not feel thirsty.", FALSE, ch, 0, 0, TO_CHAR);

            if (GET_COND(ch, FULL) > (get_normal_condition(ch, FULL) - 4))
                act("You are full.", FALSE, ch, 0, 0, TO_CHAR);

            // gamelog ("Checking for poison.");
            if ((temp->obj_flags.value[5] > 0) && (temp->obj_flags.value[5] < MAX_POISON)
                && (!IS_IMMORTAL(ch))) {
                // gamelog("Poison found.");
                if (GET_RACE(ch) == RACE_VAMPIRE)
                    act("Appears to have been poisoned...very amusing.", FALSE, ch, 0, 0, TO_CHAR);
                else {
                    //if (temp->obj_flags.value[5] == 0) /* old poison system */
                    //temp->obj_flags.value[5] = 1;   /* Generic poison */
                    act("Oops, it tasted rather strange...", FALSE, ch, 0, 0, TO_CHAR);
                    act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);
                    if (!does_save(ch, SAVING_PARA, -40))
                        poison_char(ch, temp->obj_flags.value[5], (amount * 3), 0);
                }
            }

	        // It's laced with spice
	        if (skill[temp->obj_flags.value[4]].sk_class == CLASS_SPICE) {
		        ((*skill[temp->obj_flags.value[4]].function_pointer)
		         (3, ch, "", SPELL_TYPE_SPICE, ch, 0));
	        }

            show_drink_container_fullness(ch, temp);

            return;
        }
    }
    act("It's empty already.", FALSE, ch, 0, 0, TO_CHAR);
    temp->obj_flags.value[2] = 0;       /* just in case */
    temp->obj_flags.value[3] = 0;
    temp->obj_flags.value[4] = 0;
    temp->obj_flags.value[5] = 0;

    return;
}


void
cmd_eat(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_INPUT_LENGTH];
    bool reject = FALSE;
    OBJ_DATA *temp;
    int food_left = 0, food_begin, food_type, food_eaten, liquid;
    float new_weight;
    char args[MAX_STRING_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];

    /*  struct affected_type *tmp_af = 0; */
    /*  struct affected_type *next_af = 0; */


    if (GET_POS(ch) == POSITION_FIGHTING) {
        send_to_char("Not while fighting.\n\r", ch);
        return;
    }

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow,
                            sizeof(postHow));

    argument = one_argument(args, buf, sizeof(buf));

    // The races in this list are able to eat things off the ground,
    // so we check the room->contents list to find their food
    if (!(temp = get_obj_in_list_vis(ch, buf, ch->carrying))) {
        if ((GET_RACE(ch) == RACE_HALFLING) || (GET_RACE(ch) == RACE_MANTIS)
            || (GET_RACE(ch) == RACE_JAKHAL) || (GET_RACE(ch) == RACE_VAMPIRE)
            || (GET_RACE(ch) == RACE_GORTOK) || (GET_RACE(ch) == RACE_QUIRRI)
            || (GET_RACE(ch) == RACE_TEMBO) || (GET_RACE(ch) == RACE_GWOSHI)
            || (GET_RACE(ch) == RACE_KALICH) || (GET_RACE(ch) == RACE_SAND_RAPTOR)
            || (GET_RACE(ch) == RACE_WOLF) || (GET_RACE(ch) == RACE_BRAXAT)
            || (GET_RACE(ch) == RACE_ANAKORE) || (GET_RACE(ch) == RACE_MEKILLOT)
            || (GET_RACE(ch) == RACE_XILL) || (GET_RACE(ch) == RACE_CILOPS)
            || (GET_RACE(ch) == RACE_BAHAMET) || (GET_RACE(ch) == RACE_VULTURE)
            || (GET_RACE(ch) == RACE_FIREANT) || (GET_RACE(ch) == RACE_TARANTULA)
            || (GET_RACE(ch) == RACE_SILTFLYER) || (GET_RACE(ch) == RACE_SILT)
            || (GET_RACE(ch) == RACE_SCRAB) || (GET_RACE(ch) == RACE_BIG_INSECT)
            || (GET_RACE(ch) == RACE_WAR_BEETLE) || (GET_RACE(ch) == RACE_RANTARRI)) {
            if (!(temp = get_obj_in_list_vis(ch, buf, ch->in_room->contents))) {
                act("You can't find it.", FALSE, ch, 0, 0, TO_CHAR);
                return;
            }
        } else {
            act("You can't find it.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    }

    // The races in this list can eat corpses and food
    if ((GET_RACE(ch) == RACE_MANTIS) || (GET_RACE(ch) == RACE_JAKHAL)
        || (GET_RACE(ch) == RACE_GORTOK) || (GET_RACE(ch) == RACE_TEMBO)
        || (GET_RACE(ch) == RACE_SAND_RAPTOR) || (GET_RACE(ch) == RACE_WOLF)
        || (GET_RACE(ch) == RACE_ANAKORE) || (GET_RACE(ch) == RACE_MEKILLOT)
        || (GET_RACE(ch) == RACE_CILOPS) || (GET_RACE(ch) == RACE_FIREANT)
        || (GET_RACE(ch) == RACE_TARANTULA) || (GET_RACE(ch) == RACE_SILTFLYER)
        || (GET_RACE(ch) == RACE_SILT) || (GET_RACE(ch) == RACE_BIG_INSECT)
        || (GET_RACE(ch) == RACE_WAR_BEETLE) || (GET_RACE(ch) == RACE_SCRAB)) {
        if ((!IS_CORPSE(temp)) && (!((temp->obj_flags.type == ITEM_FOOD)
            && (temp->obj_flags.value[1] == 0))))
            reject = TRUE;
    // Otherwise, races in this list can eat corpses, food, and cures
    } else if ((GET_RACE(ch) == RACE_HALFLING) || (GET_RACE(ch) == RACE_VAMPIRE)
             || (GET_RACE(ch) == RACE_GWOSHI) || (GET_RACE(ch) == RACE_KALICH)
             || (GET_RACE(ch) == RACE_QUIRRI) || (GET_RACE(ch) == RACE_BRAXAT)
             || (GET_RACE(ch) == RACE_BAHAMET) || (GET_RACE(ch) == RACE_XILL)
             || (GET_RACE(ch) == RACE_VULTURE) || (GET_RACE(ch) == RACE_RANTARRI)) {
        if ((temp->obj_flags.type != ITEM_FOOD) && (!IS_CORPSE(temp))
            && (temp->obj_flags.type != ITEM_HERB) && (temp->obj_flags.type != ITEM_CURE))
            reject = TRUE;
    // Otherwise, only food, cures and herbs for everyone else
    } else if ((temp->obj_flags.type != ITEM_FOOD) && (temp->obj_flags.type != ITEM_HERB)
               && (temp->obj_flags.type != ITEM_CURE))
        reject = TRUE;

    if (IS_IMMORTAL(ch)) {
        act("$n eats $p.", TRUE, ch, temp, 0, TO_ROOM);
        act("You eat $p.", FALSE, ch, temp, 0, TO_CHAR);
        extract_obj(temp);
        return;
    }

    if (temp->obj_flags.type != ITEM_CURE &&
	GET_COND(ch, FULL) > (get_normal_condition(ch, FULL) - 4)) {    /* Stomach full */
        act("You are too full to eat more.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

/* Terradin poison affects the stomach, inducing vomiting and making eating */
/* impossible. */
    if ((affected_by_spell(ch, POISON_TERRADIN)) && (temp->obj_flags.type != ITEM_CURE))
        reject = TRUE;

    if (reject) {
        act("Your stomach refuses to eat that!", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    /*
     * sprintf(buf, "get_normal_condition of %s is %d.", 
     * MSTR(ch, name), get_normal_condition(ch, FULL));
     * gamelog (buf);
     * sprintf(buf, "G_C isi: %d,  g_n_c - 4 is: %d", GET_COND(ch, FULL),
     * (get_normal_condition(ch, FULL) - 4));
     * gamelog (buf);
     */
    find_obj_keyword(temp, FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, buf, sizeof(buf));

    if ((temp->obj_flags.type == ITEM_CURE) && (!(GET_RACE(ch) == RACE_VAMPIRE))) {
        int affect_type = temp->obj_flags.value[0] + 219;

        if (temp->obj_flags.value[1] != 1) {    /* Tablet form */
            send_to_char("Your stomach refuses to eat that!\n\r", ch);
            return;
        }

        sprintf(to_char, "you swallow ~%s", buf);
        sprintf(to_room, "@ swallows ~%s", buf);

        if (!send_to_char_and_room_parsed
            (ch, preHow, postHow, to_char, to_room, to_room,
             (preHow[0] != '\0' || postHow[0] != '\0')
             ? MONITOR_OTHER : MONITOR_NONE)) {
            return;
        }
        // #define NESS_CURE_3_29_2003
#ifdef NESS_CURE_3_29_2003
        if (affected_by_spell(ch, affect_type)) {
            tmp_af = ch->affected;
            while (tmp_af) {
                if (tmp_af->next)
                    next_af = tmp_af->next;
                else
                    next_af = 0;
                if (tmp_af->type == affect_type) {
                    tmp_af->duration = (tmp_af->duration * temp->obj_flags.value[4]) / 100;
                    gamelog("Found poison affect.");
                }
                tmp_af = next_af;
            }
        }
#else
        if (affected_by_spell(ch, affect_type)) {
            struct affected_type *hjp;
            struct affected_type *next_hjp;

            for (hjp = ch->affected; hjp; hjp = next_hjp) {
                next_hjp = hjp->next;

                // skip non-damage for generic poison
                if (affect_type == POISON_GENERIC
                 && hjp->type == POISON_GENERIC
                 && !IS_SET(hjp->bitvector, CHAR_AFF_DAMAGE)) {
                    continue;
                }

                if (hjp->type == affect_type)
                    affect_remove(ch, hjp);
            }
        }
#endif
        if (temp->obj_flags.value[2])   /* Side effect */
            poison_char(ch, temp->obj_flags.value[2], number(1, 3), 0);


        extract_obj(temp);
        return;
    }

    if (ch->lifting) {
        cprintf(ch, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
        if (!drop_lifting(ch, ch->lifting))
            return;
    }


    if (!IS_CORPSE(temp)) {
        if (GET_RACE(ch) == RACE_VAMPIRE) {
            sprintf(to_char, "you eat ~%s", buf);
            sprintf(to_room, "@ eats ~%s", buf);

            if (!send_to_char_and_room_parsed
                (ch, preHow, postHow, to_char, to_room, to_room,
                 (preHow[0] != '\0' || postHow[0] != '\0')
                 ? MONITOR_OTHER : MONITOR_NONE)) {
                return;
            }

            adjust_move(ch, -5);
            act("You find $p to be very coarse, hurting you on its way down.", FALSE, ch, temp, 0,
                TO_CHAR);
        } else {
            if (temp->obj_flags.type == ITEM_HERB) {
                if (!temp->obj_flags.value[1]) {        /* Not edible */
                    send_to_char("Your stomach refuses to eat that!\n\r", ch);
                    return;
                }

                sprintf(to_char, "you eat ~%s", buf);
                sprintf(to_room, "@ eats ~%s", buf);

                if (!send_to_char_and_room_parsed
                    (ch, preHow, postHow, to_char, to_room, to_room,
                     (preHow[0] != '\0' || postHow[0] != '\0')
                     ? MONITOR_OTHER : MONITOR_NONE)) {
                    return;
                }

                gain_condition(ch, FULL, 1);
                if (temp->obj_flags.value[1] == 2) {    /* poisoned */
                    if (GET_RACE(ch) == RACE_VAMPIRE)
                        act("Hmm...appears to have been poisoned, very amusing.", FALSE, ch, 0, 0,
                            TO_CHAR);
                    else {
                        act("Ugh, it tasted strange!", FALSE, ch, 0, 0, TO_CHAR);
                        act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);
                        if (!does_save(ch, SAVING_PARA, -40))
                            poison_char(ch, 1, 1, 0);   /* generic poison, 1 hours */
                    }
                }
                extract_obj(temp);
                return;
            }

/* Azroen's Food hack begins: */
            food_begin = temp->obj_flags.value[0];
            food_type = temp->obj_flags.value[1];
            /* 
             * just looked at the food_aff length to get the 
             * range of [0,6] for the array, might be a better way
             * to determine range.
             * -Tenebrius
             */
            /* This doesn't accomodate new food types - removed 3/3/99 Tiernan
             * food_type = MAX (0, MIN (6, food_type));
             */
            if (food_type > MAX_FOOD)
                food_type = 0;

            if (food_aff[food_type][FULL] < 0) {
                food_left = MAX(0, (food_begin + food_aff[food_type][FULL]));
                liquid = (food_left) ? food_aff[food_type][THIRST]
                    : ((temp->obj_flags.value[0] / food_aff[food_type][FULL]) *
                       food_aff[food_type][THIRST] * -1);
            } else {
                food_left = MAX(0, (food_begin - food_aff[food_type][FULL]));
                liquid = (food_left) ? food_aff[food_type][THIRST]
                    : ((temp->obj_flags.value[0] / food_aff[food_type][FULL]) *
                       food_aff[food_type][THIRST]);
            }

            food_eaten =
                (food_aff[food_type][FULL] > food_begin) ? food_begin : food_aff[food_type][FULL];

            if (food_left && !IS_IMMORTAL(ch)) {
                sprintf(to_char, "you eat part of ~%s", buf);
                sprintf(to_room, "@ eats a portion of ~%s", buf);

                if (!send_to_char_and_room_parsed
                    (ch, preHow, postHow, to_char, to_room, to_room,
                     (preHow[0] != '\0' || postHow[0] != '\0')
                     ? MONITOR_OTHER : MONITOR_NONE)) {
                    return;
                }

/* weight functions will handle this...  --No, actually they wont.
   This affects the actual weight of the object, and has nothing to
   do with carrying weight which the weight functions handle.  */
                new_weight =
                    ((((float) (food_aff[food_type][FULL])) / temp->obj_flags.value[0]) *
                     temp->obj_flags.weight);
                new_weight = MAX(1, new_weight);
                if (new_weight < temp->obj_flags.weight)
                    temp->obj_flags.weight = new_weight;
                temp->obj_flags.value[0] = food_left;
            } else {
                sprintf(to_char, "you eat ~%s", buf);
                sprintf(to_room, "@ eats ~%s", buf);

                if (!send_to_char_and_room_parsed
                    (ch, preHow, postHow, to_char, to_room, to_room,
                     (preHow[0] != '\0' || postHow[0] != '\0')
                     ? MONITOR_OTHER : MONITOR_NONE)) {
                    return;
                }
            }

            liquid = MAX(-10, liquid);
            gain_condition(ch, FULL, food_eaten);

            // Restricting the amount of liquid points you can get from eating.
            // 9/13/2003 -Nessalin
            if (GET_COND(ch, THIRST) > 10) {
                // Let "thirst-causing" foods cause thirst, but no help
                liquid = MIN(liquid, 0);
            }

            gain_condition(ch, THIRST, liquid);

        }
        if (GET_COND(ch, FULL) > (get_normal_condition(ch, FULL) - 4))
            act("You are full.", FALSE, ch, 0, 0, TO_CHAR);

        if (temp->obj_flags.value[3] && !IS_IMMORTAL(ch)) {     /* The shit was poisoned ! */
            if (GET_RACE(ch) == RACE_VAMPIRE)
                act("Hmm...appears to have been poisoned, very amusing.", FALSE, ch, 0, 0, TO_CHAR);
            else {
                act("Ugh, it tasted strange!", FALSE, ch, 0, 0, TO_CHAR);
                act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);
                if (!does_save(ch, SAVING_PARA, -40))
                    poison_char(ch, temp->obj_flags.value[3], (temp->obj_flags.value[0] * 2), 0);
            }
        }
        /* End if temp is poisoned */

	// It's laced with spice
	if (skill[temp->obj_flags.value[5]].sk_class == CLASS_SPICE) {
		((*skill[temp->obj_flags.value[5]].function_pointer)
		 (3, ch, "", SPELL_TYPE_SPICE, ch, 0));
	}

        if (!food_left || IS_IMMORTAL(ch))
            extract_obj(temp);

        return;
    } /* End if (!IS_CORPSE(temp)) */
    else {
        sprintf(to_char, "you feast on ~%s", buf);
        sprintf(to_room, "@ devours ~%s", buf);

        if (!send_to_char_and_room_parsed
            (ch, preHow, postHow, to_char, to_room, to_room,
             (preHow[0] != '\0' || postHow[0] != '\0')
             ? MONITOR_OTHER : MONITOR_NONE)) {
            return;
        }

        if (GET_RACE(ch) == RACE_VAMPIRE)
            gain_condition(ch, FULL, 2);
        else
            gain_condition(ch, FULL, 20);
        if ((temp->obj_flags.value[3] == RACE_GITH) || (temp->obj_flags.value[3] == RACE_GALITH)) {
            send_to_char("Jolts of pain sear through you as you eat the tainted corpse.\n", ch);
            ch->points.hit = MAX(1, ch->points.hit - 10);
            ch->points.move = MAX(1, ch->points.move - 10);
        } else {
            if (GET_COND(ch, FULL) > (get_normal_condition(ch, FULL) - 4))
                send_to_char("You are full.\n\r", ch);
            if ((GET_RACE(ch) == RACE_MANTIS) || (GET_RACE(ch) == RACE_HALFLING)
                || (GET_RACE(ch) == RACE_GWOSHI)) {
                adjust_move(ch, 75);
            }
        }
        dump_obj_contents(temp);
    }                           /* End if corpse */

    if (temp->obj_flags.type == ITEM_NOTE)
        remove_note_file(temp);

    extract_obj(temp);
}                               /* End cmd_eat */


void
cmd_pour(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char orig_args[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char arg3[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char from_keyword[MAX_STRING_LENGTH];
    char to_keyword[MAX_STRING_LENGTH];
    OBJ_DATA *from_obj;
    OBJ_DATA *temp_obj = 0;
    OBJ_DATA *to_obj;
    int amount;
    int size;
    bool is_water = FALSE;
    struct weather_node *wn;
    bool pour_on = FALSE;
    CHAR_DATA *temp_char;
    int sub_type;

    strcpy(orig_args, argument);

    extract_emote_arguments(argument, preHow, sizeof(preHow), 
     args, sizeof(args), postHow, sizeof(postHow));

    argument = two_arguments(args, arg1, sizeof(arg1), arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));

    if (!*arg1) {               /* No arguments */
        act("What do you want to pour from?", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
        act("You aren't carrying anything like that.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if ((from_obj->obj_flags.type == ITEM_CONTAINER)
     || (from_obj->obj_flags.type == ITEM_FURNITURE)) {
        cmd_empty(ch, orig_args, CMD_EMPTY, 0);
        return;
    }

    if ((from_obj->obj_flags.type != ITEM_DRINKCON)
     && (from_obj->obj_flags.type != ITEM_CURE)) {
        act("You can't pour anything from $p.",
         FALSE, ch, from_obj, 0, TO_CHAR);
        return;
    }

    if ((from_obj->obj_flags.type == ITEM_CURE) 
     && (from_obj->obj_flags.value[1])) {
        act("You can't pour anything from $p.", 
         FALSE, ch, from_obj, 0, TO_CHAR);
        return;
    }

    if ((from_obj->obj_flags.value[1] == 0)
     && (from_obj->obj_flags.type != ITEM_CURE)) {
        sprintf(buf, "The %s is empty.",
         first_name(OSTR(from_obj, name), buf2));
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!*arg2) {
        act("You'll need to specify where you want to pour it.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!find_obj_keyword(from_obj, FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, from_keyword, sizeof(from_keyword))) {
        gamelogf("Unable to find a keyword for object %s (%d) in pour for %s%s%s%s",
                 temp_obj ? OSTR(temp_obj, short_descr) : "",
                 from_obj->nr,
                 IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch),
                 IS_NPC(ch) ? "" : " (",
                 IS_NPC(ch) ? "" : ch->account, IS_NPC(ch) ? "" : ")");
        strcpy(from_keyword, arg1);
    }

    if (!strcmp(arg2, "out") || !strcmp(arg2, "ground")) {
        /* pouring water on the ground */
        sprintf(to_char, "you pour ~%s on the ground", from_keyword);
        sprintf(to_room, "@ pours ~%s on the ground", from_keyword);

        if (!send_to_char_and_room_parsed
            (ch, preHow, postHow, to_char, to_room, to_room, (preHow[0] != '\0' || postHow[0] != '\0')
             ? MONITOR_OTHER : MONITOR_NONE)) {
            return;
        }
 
        if (from_obj->obj_flags.type == ITEM_CURE)
            extract_obj(from_obj);

        /* Empty */
        if ((from_obj->obj_flags.value[2] == LIQ_WATER)
            || (from_obj->obj_flags.value[2] == LIQ_WATER_VIVADU)
            || (from_obj->obj_flags.value[2] == LIQ_WATER_DIRTY)
            || (from_obj->obj_flags.value[2] == LIQ_WATER_VERY_DIRTY))
            is_water = TRUE;

        from_obj->obj_flags.value[1] = 0;
        from_obj->obj_flags.value[2] = 0;
        from_obj->obj_flags.value[3] = 0;

        // Added 2/29/2004 by Nessalin
        // Removes the poison value when item is emptied
        from_obj->obj_flags.value[4] = 0;
        from_obj->obj_flags.value[5] = 0;


        if (!ch->in_room)
            return;

        if ((IS_SET(ch->in_room->room_flags, RFL_PLANT_SPARSE)
             || IS_SET(ch->in_room->room_flags, RFL_PLANT_HEAVY)) && is_water) {
            for (wn = wn_list; wn; wn = wn->next)
                if (wn->zone == ch->in_room->zone)
                    break;
            if (wn) {
                if (!number(0, 5)) {    /* successfully watered the land */
                    wn->life = MIN(wn->life + 1, 10000);
                    ch->specials.eco = MIN(ch->specials.eco + 1, 100);
                }
            }
        }
        return;

    }
    if (!strcmp(arg2, "in") || !strcmp(arg2, "into"))
        strcpy(arg2, arg3);

    if (!strcmp(arg2, "on") || !strcmp(arg2, "onto")) {
        strcpy(arg2, arg3);
        pour_on = TRUE;
    }

    if (!*arg2) {
        act("You'll need to specify where you want to pour it.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    sub_type =
        generic_find(arg2, FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM_ON_OBJ, ch,
                     &temp_char, &to_obj);

    if (to_obj == NULL) {
        sprintf(buf, "You can't find any '%s' to pour it %s.\n\r", arg2, pour_on ? "into" : "onto");
        send_to_char(buf, ch);
        return;
    }

    if (to_obj == from_obj) {
        act("You can't pour $p into itself.", FALSE, ch, from_obj, 0, TO_CHAR);
        return;
    }

    if (!find_obj_keyword(to_obj, FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, to_keyword, sizeof(to_keyword))) {
        gamelogf("Unable to find a keyword for object %s (%d) in pour for %s%s%s%s",
         OSTR(to_obj, short_descr), to_obj->nr,
         IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch),
         IS_NPC(ch) ? "" : " (",
         IS_NPC(ch) ? "" : ch->account, IS_NPC(ch) ? "" : ")");
        strcpy(to_keyword, arg2);
    }

    if (GET_ITEM_TYPE(to_obj) != ITEM_DRINKCON || pour_on) {
        if (GET_ITEM_TYPE(from_obj) == ITEM_CURE) {
            sprintf(to_char, "you pour ~%s onto ~%s", from_keyword, to_keyword);
            sprintf(to_room, "@ pours ~%s onto ~%s", from_keyword, to_keyword);

            if (!send_to_char_and_room_parsed
                (ch, preHow, postHow, to_char, to_room, to_room, (preHow[0] != '\0' || postHow[0] != '\0')
                 ? MONITOR_OTHER : MONITOR_NONE)) {
                return;
            }

            if (GET_ITEM_TYPE(to_obj) == ITEM_FOOD) {
                if (to_obj->obj_flags.value[3] == from_obj->obj_flags.value[0])
                    to_obj->obj_flags.value[3] = 0;
                if (from_obj->obj_flags.value[2] && !to_obj->obj_flags.value[3])
                    to_obj->obj_flags.value[3] = from_obj->obj_flags.value[2];
            }
            extract_obj(from_obj);
        } else if (GET_ITEM_TYPE(from_obj) == ITEM_DRINKCON) {
            
            if (GET_ITEM_TYPE(to_obj) == ITEM_LIGHT) {
                sprintf(to_char, "you pour ~%s into ~%s", from_keyword, to_keyword);
                sprintf(to_room, "@ pours ~%s into ~%s", from_keyword, to_keyword);
            }
            else {
                sprintf(to_char, "you pour ~%s onto ~%s", from_keyword, to_keyword);
                sprintf(to_room, "@ pours ~%s onto ~%s", from_keyword, to_keyword);
            }

            if (!send_to_char_and_room_parsed
                (ch, preHow, postHow, to_char, to_room, to_room, (preHow[0] != '\0' || postHow[0] != '\0')
                 ? MONITOR_OTHER : MONITOR_NONE)) {
                return;
            }

            if (GET_ITEM_TYPE(to_obj) == ITEM_LIGHT) {
                /* how much, take into account flamability */
                size = from_obj->obj_flags.value[1] * drink_aff[from_obj->obj_flags.value[2]][3];

                if (IS_LIT(to_obj)) {
                    switch (to_obj->obj_flags.value[2]) {       /* light type */
                    default:   /* mineral, plant, animal, magick don't go out */
                        act("$p does not change.", TRUE, ch, to_obj, 0, TO_ROOM);
                        act("$p does not change.", FALSE, ch, to_obj, 0, TO_CHAR);

                        /* empty it out */
                        from_obj->obj_flags.value[1] = 0;
                        from_obj->obj_flags.value[2] = 0;
                        from_obj->obj_flags.value[3] = 0;
                        return;

                    case LIGHT_TYPE_FLAME:     /* flames can go out */
                        /* candles go out at first drop */
                        if (size <= 0 && IS_CANDLE(to_obj)) {
                            from_obj->obj_flags.value[1]--;
                            act("$p winks out, leaving a small trail of" " black smoke.", FALSE, ch,
                                to_obj, NULL, TO_CHAR);
                            act("$p winks out, leaving a small trail of" " black smoke.", FALSE, ch,
                                to_obj, NULL, TO_ROOM);
                            extinguish_light(to_obj, ch->in_room);
                            return;
                        }
                        /* lamps/lanterns go out quickly */
                        else if (size <= 0 && (IS_LAMP(to_obj) || IS_LANTERN(to_obj))) {
                            from_obj->obj_flags.value[1] -= 2;
                            act("$p flickers out.", FALSE, ch, to_obj, NULL, TO_CHAR);
                            act("$p flickers out.", FALSE, ch, to_obj, NULL, TO_ROOM);
                            extinguish_light(to_obj, ch->in_room);
                            return;
                        }
                        /* all else have to be thorougly doused */
                        else if (size <= 0 && abs(size) >= to_obj->obj_flags.value[0]) {
                            /* subtract amount needed only */
                            from_obj->obj_flags.value[1] =
                                MAX(0,
                                    (abs(size) -
                                     to_obj->obj_flags.value[0]) /
                                    drink_aff[from_obj->obj_flags.value[2]][3]);

                            act("$p sizzles and smokes as it goes out.", FALSE, ch, to_obj, NULL,
                                TO_CHAR);
                            act("$p sizzles and smokes as it goes out.", FALSE, ch, to_obj, NULL,
                                TO_ROOM);

                            /* none left */
                            to_obj->obj_flags.value[0] = 0;
                            extinguish_light(to_obj, ch->in_room);
                            return;
                        } else {
                            from_obj->obj_flags.value[1] = 0;
                            from_obj->obj_flags.value[2] = 0;
                            from_obj->obj_flags.value[3] = 0;

                            // Added 2/29/2004 by Nessalin
                            // Removes poison flag when emptied.
                            from_obj->obj_flags.value[4] = 0;
                            from_obj->obj_flags.value[5] = 0;

                            /* don't give them hours, just burns bright for a sec */
                            if (size > 0) {
                                act("$p's flame burns brighter for a second.", FALSE, ch, to_obj, 0,
                                    TO_ROOM);
                                act("$p's flame burns brighter for a second.", FALSE, ch, to_obj, 0,
                                    TO_CHAR);
                            }
                            /* no change */
                            else if (size == 0) {
                                act("$p's flame does not change.", FALSE, ch, to_obj, 0, TO_ROOM);
                                act("$p's flame does not change.", FALSE, ch, to_obj, 0, TO_CHAR);
                            }
                            /* smaller amount of hours left */
                            else if (size < 0) {
                                act("$p's flame lessens.", FALSE, ch, to_obj, 0, TO_ROOM);
                                act("$p's flame lessens.", FALSE, ch, to_obj, 0, TO_CHAR);

                                /* add size (subtracts from hours left) */
                                to_obj->obj_flags.value[0] += size;
                            }
                        }
                        return;
                    }
                } else {        /* not lit */

                    switch (to_obj->obj_flags.value[2]) {       /* light type */
                    default:
                        break;
                    case LIGHT_TYPE_FLAME:
                        if (IS_TORCH(to_obj) || IS_CAMPFIRE(to_obj)) {
                            /* torches/campfires can have flamables poured on
                             * them when unlit to increase burn time by a bit
                             */
                            if (size > 0) {
                                /* add some for flamables, keep from going too
                                 * far over max (1)
                                 */
                                to_obj->obj_flags.value[0] =
                                    MIN(to_obj->obj_flags.value[1] + 1, to_obj->obj_flags.value[0]
                                        + (IS_TORCH(to_obj) ? size / 2 : size));
                            }
                            /* subtract appropriately for negative sizes */
                            else
                                to_obj->obj_flags.value[0] =
                                    MAX(0, to_obj->obj_flags.value[0] + size);
                        } else if (IS_REFILLABLE(to_obj) && size > 0) {
                            to_obj->obj_flags.value[0] =
                                MIN(to_obj->obj_flags.value[1], to_obj->obj_flags.value[0] + size);
                        }
                        break;
                    }
                }
            }
            // if it's a weapon, clear off the poison.
            else if ( GET_ITEM_TYPE(to_obj) == ITEM_WEAPON) {
                if (to_obj->obj_flags.value[5] != 0 ) {
                    // remove poison
                    act("The taint on $p washes off.", 
                     FALSE, ch, to_obj, 0, TO_CHAR);
                    to_obj->obj_flags.value[5] = 0;
                    REMOVE_BIT(to_obj->obj_flags.extra_flags, OFL_POISON);
                }
            }

            /* Empty It */
            from_obj->obj_flags.value[1] = 0;
            from_obj->obj_flags.value[2] = 0;
            from_obj->obj_flags.value[3] = 0;

            // Added 2/29/2004 by Nessalin
            // Removes poison flag when item is emptied.
            from_obj->obj_flags.value[4] = 0;
            from_obj->obj_flags.value[5] = 0;
        } else {
            act("You can't pour anything from that.", FALSE, ch, 0, 0, TO_CHAR);
        }
    } else if (from_obj->obj_flags.type == ITEM_CURE) {
        sprintf(to_char, "you pour ~%s into ~%s", from_keyword, to_keyword);
        sprintf(to_room, "@ pours ~%s into ~%s", from_keyword, to_keyword);

        if (!send_to_char_and_room_parsed
            (ch, preHow, postHow, to_char, to_room, to_room, (preHow[0] != '\0' || postHow[0] != '\0')
             ? MONITOR_OTHER : MONITOR_NONE)) {
            return;
        }

        /* Is the the object getting poured on poisoned?  Will the cure cure it? */
        if (to_obj->obj_flags.value[5] == from_obj->obj_flags.value[0])
            to_obj->obj_flags.value[5] = 0;

        /* Does the cure have a side effect that it could leave? */
        if (from_obj->obj_flags.value[2] && !to_obj->obj_flags.value[5])
            to_obj->obj_flags.value[5] = from_obj->obj_flags.value[2];

        extract_obj(from_obj);
    } else { // drink container to drink container
        if ((to_obj->obj_flags.value[1])
            && (to_obj->obj_flags.value[2] != from_obj->obj_flags.value[2])) {
            act("There is already another liquid in it.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (to_obj->obj_flags.value[1] >= to_obj->obj_flags.value[0]) {
            act("There is no room for more.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        strcpy(buf2, color_liquid[from_obj->obj_flags.value[2]]);
    
        sprintf(to_char, "you pour a%s %s liquid from ~%s to ~%s", 
         IS_VOWEL(buf2[0]) ? "n" : "", buf2, from_keyword, to_keyword);
        sprintf(to_room, "@ pours a%s %s liquid from ~%s to ~%s", 
         IS_VOWEL(buf2[0]) ? "n" : "", buf2, from_keyword, to_keyword);
    
        if (!send_to_char_and_room_parsed
         (ch, preHow, postHow, to_char, to_room, to_room, (preHow[0] != '\0' || postHow[0] != '\0')
         ? MONITOR_OTHER : MONITOR_NONE)) {
            return;
        }
    
        /* First same type liq. */
        to_obj->obj_flags.value[2] = from_obj->obj_flags.value[2];
    
        /* Then how much to pour */
        amount =
            MIN(from_obj->obj_flags.value[1],
                (to_obj->obj_flags.value[0] - to_obj->obj_flags.value[1]));
        from_obj->obj_flags.value[1] -= amount;
        to_obj->obj_flags.value[1] += amount;
    
        /* Then the poison boogie */
        //  if (IS_SET(from_obj->obj_flags.value[3], LIQFL_POISONED))
        if ((from_obj->obj_flags.value[5] > 0) && (from_obj->obj_flags.value[5] < MAX_POISON)) {
            //    MUD_SET_BIT(to_obj->obj_flags.value[3], LIQFL_POISONED);
            //    if (from_obj->obj_flags.value[5])
            to_obj->obj_flags.value[5] = from_obj->obj_flags.value[5];
            //else
            //  to_obj->obj_flags.value[5] = from_obj->obj_flags.value[5] = 1;
    
        }
    
    
        if (!from_obj->obj_flags.value[1]) {
            from_obj->obj_flags.value[2] = 0;
            from_obj->obj_flags.value[3] = 0;
            from_obj->obj_flags.value[5] = 0;
        }
    }
    return;
}

void
cmd_empty(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char orig_args[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char arg3[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char from_keyword[MAX_STRING_LENGTH];
    char to_keyword[MAX_STRING_LENGTH];
    OBJ_DATA *from_obj;
    OBJ_DATA *temp_obj = 0;
    CHAR_DATA *temp_char;
    OBJ_DATA *to_obj;
    int sub_type;
    int num_items = 0;
    bool empty_on = FALSE;

    strcpy(orig_args, argument);

    extract_emote_arguments(argument, preHow, sizeof(preHow), 
     args, sizeof(args), postHow, sizeof(postHow));

    argument = two_arguments(args, arg1, sizeof(arg1), arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));

    if (!*arg1) {               /* No arguments */
        cprintf(ch, "What do you want to empty?\n\r");
        return;
    }

    if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
        cprintf(ch, "You aren't carrying anything like that.\n\r");
        return;
    }

    if ((from_obj->obj_flags.type == ITEM_DRINKCON)
     || (from_obj->obj_flags.type == ITEM_CURE)) {
        cmd_pour(ch, orig_args, CMD_POUR, 0);
        return;
    }

    if (from_obj->obj_flags.type != ITEM_CONTAINER
     && (from_obj->obj_flags.type != ITEM_FURNITURE
      || (from_obj->obj_flags.type == ITEM_FURNITURE
       && !IS_SET(from_obj->obj_flags.value[1], FURN_PUT)
       && !IS_SET(from_obj->obj_flags.value[1], FURN_TABLE)))) {
        cprintf(ch, "You can't empty %s.\n\r", 
         format_obj_to_char( from_obj, ch, 1));
        return;
    }

    if (from_obj->contains == NULL ) {
        cprintf(ch, "%s is empty.\n\r", 
         capitalize(format_obj_to_char(from_obj, ch, 1)));
        return;
    }

    if (from_obj->obj_flags.type == ITEM_CONTAINER 
     && IS_SET(from_obj->obj_flags.value[1], CONT_CLOSED)) {
        cprintf(ch, "%s is closed.\n\r",
         capitalize(format_obj_to_char(from_obj, ch, 1)));
        return;
    }

    if (from_obj->obj_flags.type == ITEM_FURNITURE 
     && !IS_SET(from_obj->obj_flags.value[1], FURN_PUT)
     && !IS_SET(from_obj->obj_flags.value[1], FURN_TABLE)) {
        cprintf(ch, "You can't put anything on %s.\n\r",
         format_obj_to_char(from_obj, ch, 1));
        return;
    }

    if (!*arg2) {
        cprintf(ch, "You'll need to specify where you want to empty it.\n\r");
        return;
    }

    if (!find_obj_keyword(from_obj, FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, from_keyword, sizeof(from_keyword))) {
        gamelogf("Unable to find a keyword for object %s (%d) in empty for %s%s%s%s",
                 temp_obj ? OSTR(temp_obj, short_descr) : "",
                 from_obj->nr,
                 IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch),
                 IS_NPC(ch) ? "" : " (",
                 IS_NPC(ch) ? "" : ch->account, IS_NPC(ch) ? "" : ")");
        strcpy(from_keyword, arg1);
    }


    if (!strcmp(arg2, "out") || !strcmp(arg2, "ground")) {
        /* pouring water on the ground */
        sprintf(to_char, "you empty ~%s onto the ground", from_keyword);
        sprintf(to_room, "@ empties ~%s onto the ground", from_keyword);

        if (!send_to_char_and_room_parsed
            (ch, preHow, postHow, to_char, to_room, to_room, (preHow[0] != '\0' || postHow[0] != '\0')
             ? MONITOR_OTHER : MONITOR_NONE)) {
            return;
        }

        if (!ch->in_room)
            return;

        while( (temp_obj = from_obj->contains) != NULL ) {
           if( !can_drop(ch, temp_obj) ) return;

           obj_from_obj(temp_obj);
           obj_to_room(temp_obj, ch->in_room);
           num_items++;
        }

        if (!IS_IMMORTAL(ch)) WAIT_STATE(ch, MAX(1, num_items / 5));
        return;
    }

    if (!strcmp(arg2, "in") || !strcmp(arg2, "into"))
        strcpy(arg2, arg3);

    if (!strcmp(arg2, "on") || !strcmp(arg2, "onto")) {
        strcpy(arg2, arg3);
        empty_on = TRUE;
    }

    if (!*arg2) {
        cprintf(ch, "You'll need to specify where you want to empty %s.\n\r", 
         format_obj_to_char(from_obj, ch, 1));
        return;
    }

    sub_type =
        generic_find(arg2, FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM_ON_OBJ, ch, &temp_char, &to_obj);

    if (to_obj == NULL) {
        cprintf(ch, "You can't find any '%s' to empty it %s.\n\r", 
         arg2, empty_on ? "into" : "onto");
        return;
    }

    if (to_obj == from_obj) {
        act("You can't empty $p into itself.", FALSE, ch, from_obj, 0, TO_CHAR);
        return;
    }

    switch( GET_ITEM_TYPE(to_obj)) {
    case ITEM_CONTAINER:
        empty_on = FALSE;
        break;
    case ITEM_FURNITURE:
        empty_on = TRUE;
        // if 'put' flag is on the furniture
        if( IS_SET(to_obj->obj_flags.value[1], FURN_PUT)) {
            // it's ok
            break;
        }
        // otherwise give an error message
    default:
        if( empty_on ) {
            act("You can't empty anything onto $p.", 
             FALSE, ch, to_obj, 0, TO_CHAR );
        } else {
            act("You can't empty anything into $p.", 
             FALSE, ch, to_obj, 0, TO_CHAR );
        }
        return;
    }

    if (IS_SET(to_obj->obj_flags.value[1], CONT_CLOSED)) {
        cprintf(ch, "%s is closed.\n\r",
         capitalize(format_obj_to_char(to_obj, ch, 1)));
        return;
    }

    if (!find_obj_keyword(to_obj, FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, to_keyword, sizeof(to_keyword))) {
        gamelogf("Unable to find a keyword for object %s (%d) in empty for %s%s%s%s",
         OSTR(to_obj, short_descr), to_obj->nr, 
         IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch),
         IS_NPC(ch) ? "" : " (",
         IS_NPC(ch) ? "" : ch->account, IS_NPC(ch) ? "" : ")");
        strcpy(to_keyword, arg2);
    }

    if (to_obj->in_room && !can_drop(ch, from_obj)) {
        return;
    }

    if( empty_on ) {
        sprintf(to_char, "you empty ~%s onto ~%s", from_keyword, to_keyword);
        sprintf(to_room, "@ empties ~%s onto ~%s", from_keyword, to_keyword);
    } else {
        sprintf(to_char, "you empty ~%s into ~%s", from_keyword, to_keyword);
        sprintf(to_room, "@ empties ~%s into ~%s", from_keyword, to_keyword);
    }

    if (!send_to_char_and_room_parsed
        (ch, preHow, postHow, to_char, to_room, to_room, (preHow[0] != '\0' || postHow[0] != '\0')
         ? MONITOR_OTHER : MONITOR_NONE)) {
        return;
    }

    /* Empty It */
    while( (temp_obj = from_obj->contains) != NULL ) {
        if ((GET_OBJ_WEIGHT(to_obj) + GET_OBJ_WEIGHT(temp_obj)) >=
         to_obj->obj_flags.value[0]) {
            act("$p will not fit in $P, so you stop.", 
             FALSE, ch, temp_obj, to_obj, TO_CHAR);
            act("$n stops as $P fills.", 
             FALSE, ch, NULL, to_obj, TO_ROOM);
            return;
        }
        if (GET_ITEM_TYPE(to_obj) == ITEM_CONTAINER 
         && (to_obj->obj_flags.value[5] > 0)
         && GET_OBJ_WEIGHT(temp_obj) > to_obj->obj_flags.value[5]) {
            act("$p will not fit in $P, so you stop.", 
             FALSE, ch, temp_obj, to_obj, TO_CHAR);
            act("$n stops as $P fills.", 
             FALSE, ch, NULL, to_obj, TO_ROOM);
            return;
        }

        obj_from_obj(temp_obj);
        obj_to_obj(temp_obj, to_obj);
        num_items++;
    }

    WAIT_STATE(ch, MAX(1, num_items / 4));
    return;
}

void
cmd_spice(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
	// Magic happens to lace food and drinks with spice
}

void
cmd_sip(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
  char arg[MAX_STRING_LENGTH];
  char keyword[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  int dnk;
  int amount;
  OBJ_DATA *temp;
  
#if defined(OLD_DRUNK_CODE)
  struct affected_type af;
  int start_level, drunk_level, dnk;
  
  memset(&af, 0, sizeof(af));
#endif
  
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow,
                          sizeof(postHow));
  
  one_argument(args, arg, sizeof(arg));
  
  {
    int rval;
    CHAR_DATA *tar_ch;
    rval =
      generic_find(arg,
                   FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_LIQ_CONT_INV |
                   FIND_LIQ_CONT_ROOM, ch, &tar_ch, &temp);
    if ((rval &
         (FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_LIQ_CONT_INV |
          FIND_LIQ_CONT_ROOM)) == 0) {
      act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  
  if (obj_guarded(ch, temp, CMD_SIP))
    return;
  
  if (temp->obj_flags.type != ITEM_DRINKCON) {
    act("You can't sip from that!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
  if (temp->obj_flags.value[1] < 1) {
    act("But there is nothing in it?", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  
  if (!find_obj_keyword
      (temp, FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, keyword,
       sizeof(keyword))) {
    gamelogf("Unable to find a keyword for object %s (%d) in sip for %s%s%s%s",
             OSTR(temp, short_descr), temp->nr, IS_NPC(ch) ? MSTR(ch,
                                                                  short_descr) : GET_NAME(ch),
             IS_NPC(ch) ? "" : " (", IS_NPC(ch) ? "" : ch->account, IS_NPC(ch) ? "" : ")");
    strcpy(keyword, arg);
  }
  
  sprintf(to_char, "you sip from ~%s", keyword);
  sprintf(to_room, "@ sips from ~%s", keyword);
  
  if (!send_to_char_and_room_parsed
      (ch, preHow, postHow, to_char, to_room, to_room, (preHow[0] != '\0' || postHow[0] != '\0')
       ? MONITOR_OTHER : MONITOR_NONE)) {
    return;
  }
  
  if (affected_by_spell(ch, SPELL_FIREBREATHER) && (temp->obj_flags.value[2] == LIQ_WATER)) {
    amount = number(3, 10);
    amount = MIN(amount, temp->obj_flags.value[1]);
    act("As you drink from $p you feel an intense burning sensation.", FALSE, ch, temp, 0,
        TO_CHAR);
    act("$n grasps $s chest and screams in pain.", FALSE, ch, temp, 0, TO_ROOM);
    generic_damage(ch, amount * 2, 0, 0, amount * 2);
    if (GET_POS(ch) == POSITION_DEAD) {
      act("You grab your chest and scream as the pain consumes" " your pitiful life.", FALSE,
          ch, 0, 0, TO_CHAR);
      die(ch);
    } else
      act("You grab your chest and scream in pain.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  /* Okay, let's take a hack at this, using cases.  -Sanvean. */
  switch (temp->obj_flags.value[2]) {
  case 0:                    /* Water        */
    sprintf(buf, "This tastes like ordinary water. \n\r");
    break;
  case 1:                    /* beer         */
    sprintf(buf, "This tastes like ordinary beer. \n\r");
    break;
  case 2:                    /* wine         */
    sprintf(buf, "This tastes like ordinary wine. \n\r");
    break;
  case 3:                    /* ale          */
    sprintf(buf, "This tastes like ordinary ale. \n\r");
    break;
  case 4:                    /* Dark ale     */
    sprintf(buf, "This tastes like strong ale. \n\r");
    break;
  case 5:                    /* whisky    */
    sprintf(buf, "This inexecrable crap tastes like the local whisky. \n\r");
    break;
  case 6:                    /* spicedale   */
    sprintf(buf, "This crap tastes like strongly spiced ale. \n\r");
    break;
  case 7:                    /* Firebreather */
    sprintf(buf, "This tastes like strong, raw spirits. \n\r");
    break;
  case 8:                    /* Red Sun      */
    sprintf(buf, "This liquid has the bitter almond aftertaste of Redsun. \n\r");
    break;
  case 9:                    /* Crap         */
    sprintf(buf, "This tastes horrible beyond belief. \n\r");
    break;
  case 10:                   /* Milk         */
    sprintf(buf, "This tastes like ordinary milk. \n\r");
    break;
  case 11:                   /* Tea          */
    sprintf(buf, "This tastes like ordinary tea. \n\r");
    break;
  case 12:                   /* Coffee       */
    sprintf(buf, "This tastes like ordinary coffee. \n\r");
    break;
  case 13:                   /* Blood        */
    sprintf(buf, "This tastes like blood. \n\r");
    break;
  case 14:                   /* Salt         */
    sprintf(buf, "This liquid tastes foul and salty. \n\r");
    break;
  case 15:                   /* spirits      */
    sprintf(buf, "This burning liquid tastes like potent spirits. \n\r");
    break;
  case 16:                   /* sand         */
    sprintf(buf, "This liquid is bitter and dry, totally unrefreshing. \n\r");
    break;
  case 17:                   /* silt         */
    sprintf(buf, "This liquid is oily and permeated with gritty bits. \n\r");
    break;
  case 18:                   /* gloth        */
    sprintf(buf, "This strong sweet liquid tastes potent. \n\r");
    break;
  case 19:                   /* green honey mead */
    sprintf(buf, "This liquid tastes cloyingly sweet. \n\r");
    break;
  case 20:                   /* brandy */
    sprintf(buf, "This tastes like ordinary brandy. \n\r");
    break;
  case 21:                   /* Spiced brandy */
    sprintf(buf, "This tastes like strongly spiced brandy. \n\r");
    break;
  case 22:                   /* belshun wine */
    sprintf(buf, "This rich, dark wine is full-bodied and redolent with the flavors of "
            "ripe belshun fruit. It smells faintly of warm baking spices and, gently "
            "sweet, offers a smooth, lingering finish.\n\r");
    break;
  case 23:                   /* jallal wine */
    sprintf(buf, "This wine tastes strongly of jallal fruit. \n\r");
    break;
  case 24:                   /* japuaar wine */
    sprintf(buf, "This wine tastes strongly of japuaar fruit. \n\r");
    break;
  case 25:                   /* kalan wine */
    sprintf(buf, "A very faint floral aroma lightly teases the senses as you sip this "
            "delicate vintage, bringing with it a sense of mellow refreshment. The wine "
            "tastes of perfectly ripened kalan fruit and is crisp, dry, and bright on the palate.\n\r");
    break;
  case 26:                   /* petoch wine */
    sprintf(buf, "This wine tastes strongly of petoch fruit. \n\r");
    break;
  case 27:                   /* horta wine */
    sprintf(buf, "While initially sharp, the wine's tang is soon smothered by a pleasant\n\r"
            "sweetness.  The drink leaves a refreshingly bitter aftertaste.\n\r");
    break;
  case 28:                   /* ginka wine */
    sprintf(buf, "The heady, sweet taste of ginka is smooth and enhanced by a subtle, spicy heat "
            "that compliments the rich, austere flavor. There is an almost hedonistic quality to "
            "the silken texture, warmth tingling on the lips as it goes down.\n\r");
    break;
  case 29:                   /* lichen wine */
    sprintf(buf, "This thin wine tastes strongly of lichen and ash. \n\r");
    break;
  case 30:                   /* ocotillo wine */
    sprintf(buf, "Reasonably smooth in spite of its acidity, this wine has a light, crisp and"
            " somewhat dry taste. While not complex in flavor, the distinctly sour notes"
            " of ocotillo are present, as well, as undertones of pepper and citrus.\n\r");
    break;
  case 31:                   /* black anakore */
    sprintf(buf, "This thick black liquid wine reeks of strong alcohol. \n\r");
    break;
  case 32:                   /* Firestorm's flame */
    sprintf(buf, "This thick barrel-aged ale foams up beautifully and is bold, assertive and "
            "redolent with brandy-soaked fruits. The brew is dense and creamy and the "
            "spicy-sweet flavors linger long after the smooth, toasty finish.\n\r");
    break;
  case 33:                   /* Elven Tea */
    sprintf(buf, "A vaguely floral taste fills your mouth. \n\r");
    break;
  case 34:                   /* Dwarven Ale */
    sprintf(buf, "A strong, bitter taste fills your mouth. \n\r");
    break;
  case 35:                   /* jaluar-wine */
    sprintf(buf, "This wine tastes strongly of jaluar fruit. \n\r");
    break;
  case 36:                   /* reynolte-dry */
    sprintf(buf, "This wine tastes strongly of a mixture of fruits. \n\r");
    break;
  case 37:                   /* honied-wine */
    sprintf(buf, "Honey has been used to heavily sweeten this wine. \n\r");
    break;
  case 38:                   /* spiced-wine */
    sprintf(buf, "This wine has been heavily mixed with spice. \n\r");
    break;
  case 39:                   /* honied-ale */
    sprintf(buf, "This ale has been mixed with honey to sweeten it. \n\r");
    break;
  case 40:                   /* honied-brandy */
    sprintf(buf, "This strong brandy has been fixed with honey to sweeten it. \n\r");
    break;
  case 41:                   /* spiced-mead */
    sprintf(buf, "This mead has been heavily mixed with spice. \n\r");
    break;
  case 42:                   /* paint */
    sprintf(buf, "This liquid tastes vile and has an unpleasantly thick consistency. \n\r");
    break;
  case 43:                   /* oil */
    sprintf(buf, "This thick liquid tastes unpleasantly like lamp oil.\n\r");
    break;
  case 44:                   /* violet ocotillo */
    sprintf(buf, "Crisp, lemon-like notes of citrodora meld beautifully with the sour, "
            "onion-like flavor of ocotillo.  The aromatic herb embraces the harshness "
            "of the ocotillo, softening and mellowing it.  If savored, hints of a peppery "
            "spice are revealed along with a faint dry, sweetness lent to the wine from a "
            "maar wood cask.  Aging has created a depth to this full-bodied wine as each "
            "flavor discovers and claims its place.\n\r");
    break;
  case 45:                   /* purple ocotillo */
    sprintf(buf, "Complex in nature, this wine is has subtle nuances meant to be discovered and "
            "savored.  Seductively smooth as it greets the palate, aging has mellowed the "
            "bold, tart taste of ocotillo, allowing lighter flavors to flirt with it; not blending, "
            "but weaving together to create a tapestry of flavor.  Pfafna lends sweet mint "
            "and citrus notes that dance with peppery hints of spice and tangy fruit.  Each "
            "flavor leaves its mark, enhancing and complementing, but never marrying the "
            "ocotillo.\n\r");
    break;
  case 46:                   /* ocotillo spirits */
    sprintf(buf, "Lacking even the faintest pretense of subtlety, these spirits are strong and "
            "sour and make no apology. The ocotillo mash used in distilling makes its "
            "presence known via the distinct earthy, sour flavor of the liquor.  Not aged, "
            "the liquor has a raw quality and the stinging strength of the alcohol is felt as "
            "it burns on the way down.\n\r");
    break;
  case 47:                   /* cleanser */
    sprintf(buf, "Ugh! That tastes horrible! You almost vomit!\n\r");
    break;
  case 48:                   /* honied tea */
    sprintf(buf, "Strong and sweet, this tea tastes of honey.\n\r");
    break;
  case 49:                   /* fruit tea */
    sprintf(buf, "This tea has delicate overtones of belshun and kalan fruit.\n\r");
    break;
  case 50:                   /* mint tea */
    sprintf(buf, "This tea smells and tastes strongly of fragrant mint.\n\r");
    break;
  case 51:                   /* clove tea */
    sprintf(buf, "This aromatic tea tastes of cloves and escru milk.\n\r");
    break;
  case 52:                   /* kumiss */
    sprintf(buf, "This drink tastes like milk, but has alcohol in it.\n\r");
    break;
  case 53:                   /* jik */
    sprintf(buf, "This drink is sweet and fruity, followed by a bitter aftertaste.\n\r");
    break;
  case 54:                   /* badu */
    sprintf(buf, "This strong, sweet alcohol tastes like honied fire.\n\r");
    break;
  case 55:                   /* Vivadu water */
    sprintf(buf, "It tastes like water, but is suprisingly refreshing.\n\r");
    break;
  case 56:                   /* Muddy water */
    sprintf(buf, "Tastes like water, although there's a lot of debris in it.\n\r");
    break;
  case 57:                   /* greyish water */
    sprintf(buf, "Tastes like water, although there's a little scum in it.\n\r");
    break;
  case 58:                   /* liquid fire */
    sprintf(buf, "Smooth on the tongue, this liquid hits the back of your throat like a fireball.\n\r");
    break;
  case 59:                   /* sap */
    sprintf(buf, "Sweet and cool, this liquid is refreshing and invigorating.\n\r");
    break;
    case 60:                   /* Slavedriver */
      sprintf(buf, "Although very sweet this drink also has a strong burn from all the alcohol.\n\r");
      break;
  case 61:                   /* Spiceweed Tea */
    sprintf(buf, "This sweet pink tea is touched by the heady flavor of cinnamon and clove.\n\r");
    break;
  case 62:                   /* Spiced belshun wine */
    sprintf(buf, "Thick and sweet, belshun's flavor is coupled with a numbing feeling of deep contentment.\n\r");
    break;
  case 63:                   /* spiced ginka wine */
    sprintf(buf, "An oddly refreshing sense of alertness accompanies the rich, intoxicating flavor of ginka.\n\r");
    break;
  case 64:                   /* agvat */
    sprintf(buf, "This foul concoction of distilled cactus pollutes your breath and leaves a tingling burning"
            " in the throat.\n\r");
    break;
  case 65:                   /* belshun fruit juice */
    sprintf(buf, "This sweet juice has the savor of belshun fruit.\n\r");
    break;
  case 66:                   /* ginka juice */
    sprintf(buf, "This juice has an almost intoxicating taste, its flavor sweet and tantalizing.\n\r");
    break;
  case 67:                   /* horta juice */
    sprintf(buf, "The sweet frothy liquid has the savor of fresh horta fruit.\n\r");
    break;
  case 68:                   /* japuaar juice */
    sprintf(buf, "This sweet juice has the savor of japuaar fruit.\n\r");
    break;
  case 69:                   /* kalan juice */
    sprintf(buf, "This sweet juice has the savor of kalan fruit.\n\r");
    break;
  case 70:                   /* melon juice */
    sprintf(buf, "This juice has a sweet, fruity savor to it.\n\r");
    break;
  case 71:                   /* thornfruit juice */
    sprintf(buf, "Mingling sweet and tart together, this juice has a refreshing savor to it.\n\r");
    break;
  case 72:                   /* generic juice */
    sprintf(buf, "This refreshing juice has the savor of fresh fruit.\n\r");
    break;
  case 73:			/* yezzir whiskey */
    if (GET_RACE(ch) == drink_aff[temp->obj_flags.value[2]][DRINK_CONTAINER_RACE]) {
      sprintf(buf, "This potent whiskey has been expertly brewed with hints of black fire pepper, "
              "ginka and horta fruits, and a lasting, refreshing touch of mint at the very end.\n\r");
    } else {
      sprintf(buf, "At first harsh on the tongue, this whiskey mellows out after the initial impact, "
              "granting hints of peppers, fruits, and exotic desert spices.\n\r");
    }
    break;
  case 74:			/* sejah whiskey */
    sprintf(buf, "This refined whiskey has a strong base of pungent ocotillo so powerful it almost "
            "burns stronger than the liquor itself.  Lingering on afterwards, enhancing the heat, are "
            "hints of cinnamon and clove.\n\r");
    break;
  case 75:           /* blackstem tea */
    sprintf(buf, "This full-bodied tea is full of rich earthiness, with a minty aftertaste.\n\r");
    break;
  case 76:           /* dwarfflower tea */
    sprintf(buf, "This delicate tea's fruity tones dance on your tongue.\n\r");
    break;
  case 77:           /* greenmantle tea */
    sprintf(buf, "This tea is almost cloyingly sweet, but finishes with an acidic bite.\n\r");
    break;
  case 78:           /* kanjel tea */
    sprintf(buf, "Complex flavors combine in this tea to give you the sense of walking through lush grasslands.\n\r");
    break;
  case 79:
    sprintf(buf, "Soft tones of citrus and hints of mint play along the tongue as this gentle tea is imbibed.\n\r");
    break;
  case 80:
    sprintf(buf, "This heady tea involves the robust taste of blackstem underscored by hints of alnon and belgoikiss.\n\r");
    break;
  case 81:
    sprintf(buf, "The sharp taste of this tea is acrid at first, but with a long, lingering sweetness that "
            "plays on the tongue.  As the tea goes down, you feel more alert and awake.\n\r");
    break;
  case 82:
    sprintf(buf, "This very bitter brew is sharp on the tongue, but lingers for many moments with the "
            "cloying tastes of honey and melon.  As the tea goes down, your head feels clear and small "
            "aches and pains fade.\n\r");
    break;
  case 83:
    sprintf(buf, "The tastes of citrus and kutai fruit play along the tongue as this heady, rich tea is "
            "imbibed.  As the tea goes down, a mellow calm settles over you, and you feel a bit more at ease.\n\r");
    break;
  case 84:
    sprintf(buf, "Frolicking along the tongue, the bubbles in this gentle wine tickle and pop as they go down.  "
            "The taste is many layered, sour yet light, the medley backed by fruity undertones and a hint of citrus.\n\r");
    break;
  case 85:
    sprintf(buf, "A tingling numbness envelops your mouth, following the liquid down before fading.\n\r");
    break;
  case 86:
    sprintf(buf, "This burns like fire going down.  It leaves a lasting heat in the mouth and sinuses, followed "
            "by an aftertaste of burnt sugar.\n\r");
    break;
  case 87:
    sprintf(buf, "The smooth dark flavor of this drink lingers pleasantly in your mouth.  As you swallow you can "
            "feel the distinct burn of alcohol, but it in no way diminishes the rich and complex flavors of aged "
            "wood, caramelized marilla, and a subtle hint of honey.\n\r");
    break;
    
  default:
    sprintf(buf, "You're not quite sure what that tasted like. \n\r");
    break;
  }
  
  send_to_char(buf, ch);
  
  gain_condition(ch, FULL, (int) (drink_aff[temp->obj_flags.value[2]][FULL] / 4));
  
  gain_condition(ch, THIRST, (int) (drink_aff[temp->obj_flags.value[2]][THIRST] / 4));
  
  dnk = ((int) (drink_aff[temp->obj_flags.value[2]][DRUNK] / 4));
  dnk = (MAX(1, dnk));
  
  if (drink_aff[temp->obj_flags.value[2]][DRUNK] > 0) {
    make_char_drunk(ch, dnk);
  }
  
  if (GET_COND(ch, THIRST) > (get_normal_condition(ch, THIRST) - 4)) {
    act("You do not feel thirsty.", FALSE, ch, 0, 0, TO_CHAR);
  }
  
  if (GET_COND(ch, FULL) > (get_normal_condition(ch, FULL) - 4)) {
    act("You are full.", FALSE, ch, 0, 0, TO_CHAR);
  }
  
  if (temp->obj_flags.value[5] > 0 && temp->obj_flags.value[5] < MAX_POISON) {
    act("But it also had a strange taste!", FALSE, ch, 0, 0, TO_CHAR);
    if (!does_save(ch, SAVING_PARA, 0)) {
      poison_char(ch, temp->obj_flags.value[5], 3, 0);
    }
  }
  if (!IS_SET(temp->obj_flags.value[3], LIQFL_INFINITE)) {
    temp->obj_flags.value[1] -= 3;
  }
  
  // It's laced with spice
  if (skill[temp->obj_flags.value[4]].sk_class == CLASS_SPICE) {
    ((*skill[temp->obj_flags.value[4]].function_pointer)
     (1, ch, "", SPELL_TYPE_SPICE, ch, 0));
  }
  
  if (temp->obj_flags.value[1] < 1) { /* The last bit */
    temp->obj_flags.value[1] = 0;
    temp->obj_flags.value[2] = 0;
    temp->obj_flags.value[3] = 0;
    temp->obj_flags.value[4] = 0;
    temp->obj_flags.value[5] = 0;
  }
  
  show_drink_container_fullness(ch, temp);
  
  return;
}

void
cmd_taste(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char *message;
  OBJ_DATA *temp;
  int remaining;
  char args[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  
  extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));
  
  argument = one_argument(args, arg, sizeof(arg));
  
  if (GET_RACE(ch) == RACE_MANTIS) {
    send_to_char("Your mouthparts are not well suited to small bites.\n\r", ch);
    return;
  }
  
  if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying)))
    if (!(temp = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
      act("You can't find it.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  
  if (temp->obj_flags.type == ITEM_DRINKCON) {
    cmd_sip(ch, argument, 0, 0);
    return;
  }
  
  if ((temp->obj_flags.type != ITEM_FOOD) && (temp->obj_flags.type != ITEM_HERB)
      && (temp->obj_flags.type != ITEM_CURE)) {
    act("Taste that?!? Your stomach refuses!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (affected_by_spell(ch, POISON_TERRADIN)) {
    send_to_char("Your stomach refuses to taste that!\n\r", ch);
    return;
  }
  
  if (temp->obj_flags.type == ITEM_CURE) {
    switch (temp->obj_flags.value[3]) {
    case 0:                /* none */
      act("$p has no discernable taste.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 1:                /* acidic       */
      act("$p tastes slightly acidic.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 2:                /* bitter       */
      act("$p has a bitter flavor.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 3:                /* foul       */
      act("$p tastes very foul.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 4:                /* minty       */
      act("$p leaves a minty taste in your mouth.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 5:                /* sour       */
      act("$p is sour.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 6:                /* spicy       */
      act("$p has a flavor much like spice.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 7:                /* sweet       */
      act("$p is very sweet tasting.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 8:                /* strong       */
      act("$p has a strong flavor.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 9:                /* citric       */
      act("$p has a citric flavor.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 10:               /* sharp       */
      act("$p's flavor is very sharp .", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 11:               /* awful       */
      act("$p tastes awful!", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 12:               /* fruity       */
      act("$p has a mild, fruity taste.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 13:               /* chalky       */
      act("$p is flat, almost chalky in flavor.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 14:               /* salty       */
      act("$p is very salty.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    case 15:               /* gritty       */
      act("$p has a gritty taste to it.", FALSE, ch, temp, 0, TO_CHAR);
      break;
    }
  }
  
  
  if (temp->obj_flags.type == ITEM_HERB) {
    switch (temp->obj_flags.value[0]) {
      /* The reason there is no case 0 is b/c that value */
      /* is 'none'.                                      */
    case 1:                /* acidic       */
      sprintf(buf, "This tastes strongly acidic. \n\r");
      break;
    case 2:                /* sweet */
      sprintf(buf, "This tastes overwhelmingly sweet. \n\r");
      break;
    case 3:                /* sour */
      sprintf(buf, "Your tongue twists at the sour taste. \n\r");
      break;
    case 4:                /* bland */
      sprintf(buf, "This tastes bland, not like much at all. \n\r");
      break;
    case 5:                /* hot */
      sprintf(buf, "Your tongue burns at the strong taste. \n\r");
      break;
    case 6:                /* cold */
      sprintf(buf, "The inside of your mouth goes a little numb. \n\r");
      break;
    case 7:                /* calming */
      sprintf(buf, "A heavy, soothing feeling fills your mouth. \n\r");
      break;
    case 8:                /* refreshing */
      sprintf(buf, "Your mouth tingles at the taste. \n\r");
      break;
    case 9:                /* salty */
      sprintf(buf, "This tastes overwhelmingly salty. \n\r");
      break;
    case 10:               /* warming */
      sprintf(buf, "A rush of warmth fills your mouth. \n\r");
      break;
    case 11:               /* aromatic */
      sprintf(buf, "This tastes fragrant and sweet. \n\r");
      break;
    case 12:               /* bitter */
      sprintf(buf, "This tastes overwhelmingly bitter. \n\r");
      break;
    default:
      sprintf(buf, "You're not really sure what that tasted like. \n\r");
      break;
    }
    send_to_char(buf, ch);
  }
  
  if (temp->obj_flags.type == ITEM_FOOD) {
    /* Moved stuff around to fix crashing due to game trying to act 
     * on an already extracted object. -sanvean
     * Moved s'more around to fix typo due to order in which obj updated. -savak
     */
    remaining = temp->obj_flags.value[0];
    remaining--;
    
    find_obj_keyword(temp, FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch,
                     arg, sizeof(arg));
    
    if (remaining < 1) {    /* Nothing left */
      sprintf(to_room, "@ takes the last bite of ~%s", arg);
      sprintf(to_char, "you take the last bite of ~%s", arg);
    } else {
      sprintf(to_room, "@ takes a bite of ~%s", arg);
      sprintf(to_char, "you take a bite of ~%s", arg);
    }
    
    if (!send_to_char_and_room_parsed
        (ch, preHow, postHow, to_char, to_room, to_room,
         (preHow[0] != '\0' || postHow[0] != '\0')
         ? MONITOR_OTHER : MONITOR_NONE)) {
      return;
    }
    
    /* Added for individualized taste messages */
    message = find_ex_description("[TASTE]", temp->ex_description, TRUE);
    if (message) {
      send_to_char(message, ch);
    } else {
      switch (temp->obj_flags.value[1]) {
      case 0:            /* meat */
        sprintf(buf, "This tastes like ordinary meat. \n\r");
        break;
      case 1:            /* bread */
        sprintf(buf, "This tastes like ordinary bread. \n\r");
        break;
      case 2:            /* fruit */
        sprintf(buf, "This tastes like ordinary fruit. \n\r");
        break;
      case 3:            /* horta */
        sprintf(buf, "Sweet and fragrant, this tastes like horta fruit. \n\r");
        break;
      case 4:            /* ginka */
        sprintf(buf, "Sweet and juicy, this tastes like ginka fruit. \n\r");
        break;
      case 5:            /* petoch */
        sprintf(buf, "The tender petoch fruit yields up its sweetness. \n\r");
        break;
      case 6:            /* dried fruit */
        sprintf(buf, "This tastes like ordinary dried fruit. \n\r");
        break;
      case 7:            /* mush */
        sprintf(buf, "This tastes bland and mushy. \n\r");
        break;
      case 8:            /* sweet */
        sprintf(buf, "This tastes sweet and candylike. \n\r");
        break;
      case 9:            /* snack */
        sprintf(buf, "This tastes salty and unsatisfying. \n\r");
        break;
      case 10:           /* soup */
        sprintf(buf, "This tastes like hot, salty liquid. \n\r");
        break;
      case 11:           /* tuber */
        sprintf(buf, "This tastes strongly of the earth. \n\r");
        break;
      case 12:           /* burned */
        sprintf(buf, "This tastes burned and unpalatable. \n\r");
        break;
      case 13:           /* spoiled */
        sprintf(buf, "This tastes nauseating; your stomach objects.\n\r");
        break;
      case 14:           /* cheese */
        sprintf(buf, "This tastes like tangy, pungent cheese.\n\r");
        break;
      case 15:           /* pepper */
        sprintf(buf, "You gasp and reach for a drink of water. \n\r");
        break;
      case 16:           /* sour */
        sprintf(buf, "This has a tongue-twisting, sour taste. \n\r");
        break;
      case 17:           /* vegetable */
        sprintf(buf, "This tastes like ordinary vegetable. \n\r");
        break;
      case 18:           /* honey */
        sprintf(buf, "This tastes of sweet honey. \n\r");
        break;
      case 19:           /* melon */
        sprintf(buf, "This has a sweet, refreshing tang. \n\r");
        break;
      case 20:           /* grub */
        sprintf(buf, "This tastes meaty and moist, although very chewy.\n\r");
        break;
      case 21:           /* ants */
        sprintf(buf, "This tastes bitter and acidic, faintly citrus.\n\r");
        break;
      case 22:           /* insect */
        sprintf(buf, "Beneath its chitinous carapace, this morsel has a strong meaty taste.\n\r");
        break;
      default:
        sprintf(buf, "You're not really sure what that tasted like. \n\r");
        break;
      }
      send_to_char(buf, ch);
    }
    gain_condition(ch, FULL, 1);
    
    if (GET_COND(ch, FULL) > (get_normal_condition(ch, FULL) - 4)) {
      act("You are full.", FALSE, ch, 0, 0, TO_CHAR);
    }
    
    if (temp->obj_flags.value[3]) { /* The shit was poisoned ! */
      act("Oops, it didn't taste good at all!", FALSE, ch, 0, 0, TO_CHAR);
      if (!does_save(ch, SAVING_PARA, 0)) {
        poison_char(ch, temp->obj_flags.value[3], 1, 0);
      }
    }
    
    // It's laced with spice
    if (skill[temp->obj_flags.value[5]].sk_class == CLASS_SPICE) {
      ((*skill[temp->obj_flags.value[5]].function_pointer)
       (1, ch, "", SPELL_TYPE_SPICE, ch, 0));
    }
    
    if (remaining < 1) {
      act("There is nothing left now.", FALSE, ch, 0, 0, TO_CHAR);
      extract_obj(temp);
    } else {
      temp->obj_flags.value[0]--;
    }
  }
  /* End item = TYPE_FOOD */
  
  return;
}


/* functions related to wear */

void
perform_wear(CHAR_DATA * ch, OBJ_DATA * obj_object, int loc, char *preHow, char *postHow)
{
    char message[MAX_STRING_LENGTH];
    char *tmp_desc;
    char buf2[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char obj_keyword[MAX_STRING_LENGTH];
    int i;

    buf2[0] = '\0';
    obj_keyword[0] = '\0';
    find_obj_keyword(obj_object,
     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch,
     obj_keyword, sizeof(obj_keyword));

    for (i = 0; i < MAX_WEAR_EDESC; i++) {
        if (loc == wear_edesc[i].location_that_covers) {
            tmp_desc = find_ex_description(wear_edesc[i].edesc_name, ch->ex_description, TRUE);
            if (tmp_desc && !is_covered(ch,ch,i))
                sprintf(buf2, "covering %s", tmp_desc);
        }
    }
 
    if (postHow[0] != '\0' && buf2[0] != '\0')
        strcat(buf2, ", ");

    strcat(buf2, postHow);

    switch (loc) {
    case WEAR_BELT:
        sprintf(to_room, "@ straps on ~%s as a belt", obj_keyword);
        break;
    case WEAR_FINGER_R:
    case WEAR_FINGER_R2:
    case WEAR_FINGER_R3:
    case WEAR_FINGER_R4:
    case WEAR_FINGER_R5:
        sprintf(to_room, "@ slips ~%s onto a finger on ^me right hand", obj_keyword);
        break;
    case WEAR_FINGER_L:
    case WEAR_FINGER_L2:
    case WEAR_FINGER_L3:
    case WEAR_FINGER_L4:
    case WEAR_FINGER_L5:
        sprintf(to_room, "@ slips ~%s onto a finger on ^me left hand", obj_keyword);
        break;
    case WEAR_NECK:
        sprintf(to_room, "@ bows ^me head, placing ~%s about ^me neck", obj_keyword);
        break;
    case WEAR_BODY:
        sprintf(to_room, "@ wears ~%s on ^me body", obj_keyword);
        break;
    case WEAR_HEAD:
        sprintf(to_room, "@ places ~%s on ^me head", obj_keyword);
        break;
    case WEAR_LEGS:
        sprintf(to_room, "@ wears ~%s on ^me legs", obj_keyword);
        break;
    case WEAR_FEET:
        sprintf(to_room, "@ slips ^me feet into ~%s", obj_keyword);
        break;
    case WEAR_HANDS:
        sprintf(to_room, "@ pulls ~%s onto ^me hands", obj_keyword);
        break;
    case WEAR_ARMS:
        sprintf(to_room, "@ fits ~%s on ^me arms", obj_keyword);
        break;
    case WEAR_ABOUT:
        sprintf(to_room, "@ wears ~%s about ^me body", obj_keyword);
        break;
    case WEAR_WAIST:
        sprintf(to_room, "@ straps ~%s about ^me waist", obj_keyword);
        break;
    case WEAR_WRIST_R:
        sprintf(to_room, "@ fastens ~%s around ^me right wrist", obj_keyword);
        break;
    case WEAR_WRIST_L:
        sprintf(to_room, "@ fastens ~%s around ^me left wrist", obj_keyword);
        break;
    case EP:
        sprintf(to_room, "@ brandishes ~%s", obj_keyword);
        break;
    case ES:
        sprintf(to_room, "@ holds ~%s", obj_keyword);
        break;
    case WEAR_BACK:
        sprintf(to_room, "@ wears ~%s on ^me back", obj_keyword);
        break;
    case WEAR_ON_BELT_1:
    case WEAR_ON_BELT_2:
        sprintf(to_room, "@ hangs ~%s on ^me belt", obj_keyword);
        break;
    case ETWO:
        sprintf(to_room, "@ brandishes ~%s in both hands", obj_keyword);
        break;
    case WEAR_IN_HAIR:
        sprintf(to_room, "@ wears ~%s in ^me hair", obj_keyword);
        break;
    case WEAR_FACE:
        sprintf(to_room, "@ places ~%s onto ^me face", obj_keyword);
        break;
    case WEAR_ANKLE:
        sprintf(to_room, "@ straps ~%s onto ^me right ankle", obj_keyword);
        break;
    case WEAR_ANKLE_L:
        sprintf(to_room, "@ straps ~%s onto ^me left ankle", obj_keyword);
        break;
    case WEAR_LEFT_EAR:
        sprintf(to_room, "@ clips ~%s onto ^me left ear", obj_keyword);
        break;
    case WEAR_RIGHT_EAR:
        sprintf(to_room, "@ clips ~%s onto ^me right ear", obj_keyword);
        break;
    case WEAR_FOREARMS:
        sprintf(to_room, "@ slips ~%s onto ^me forearms", obj_keyword);
        break;
    case WEAR_ABOUT_HEAD:
        sprintf(to_room, "@ tosses ~%s into the air which hovers briefly before assuming an orbit around ^me head", obj_keyword);
        break;
    case WEAR_ABOUT_THROAT:
        sprintf(to_room, "@ tilts ^me head forward and fastens ~%s about ^me throat", obj_keyword);
        break;
    case WEAR_SHOULDER_R:
        sprintf(to_room, "@ reaches up and places ~%s on ^me right shoulder", obj_keyword);
        break;
    case WEAR_SHOULDER_L:
        sprintf(to_room, "@ reaches up and places ~%s on ^me left shoulder", obj_keyword);
        break;
    case WEAR_OVER_SHOULDER_R:
        sprintf(to_room, "@ places ~%s over ^me right shoulder", obj_keyword);
        break;
    case WEAR_OVER_SHOULDER_L:
        sprintf(to_room, "@ places ~%s over ^me left shoulder", obj_keyword);
        break;
    }

    /* Generate a message for the room */
    prepare_emote_message(preHow, to_room, buf2, message);
    send_to_room_parsed(ch, ch->in_room, message, FALSE);
    if (*preHow || *postHow) {
        send_to_monitor_parsed(ch, message, MONITOR_OTHER);
    }

    return;
}

int
already_wearing(CHAR_DATA *ch, int keyword) {
    switch(keyword) {
    case 0: 
        if (ch->equipment[WEAR_BELT])
            return TRUE;
        break;
    case 1: 
        if ((ch->equipment[WEAR_FINGER_L]) && (ch->equipment[WEAR_FINGER_R])
         && (ch->equipment[WEAR_FINGER_L2]) && (ch->equipment[WEAR_FINGER_R2])
         && (ch->equipment[WEAR_FINGER_L3]) && (ch->equipment[WEAR_FINGER_R3])
         && (ch->equipment[WEAR_FINGER_L4]) && (ch->equipment[WEAR_FINGER_R4])
         && (ch->equipment[WEAR_FINGER_L5]) && (ch->equipment[WEAR_FINGER_R5]))
            return TRUE;
        break;
    case 2:
        if (ch->equipment[WEAR_NECK]) 
            return TRUE;
        break;
    case 3:
        if (ch->equipment[WEAR_BODY]) 
            return TRUE;
        break;
    case 4:
        if (ch->equipment[WEAR_HEAD])
            return TRUE;
        break;
    case 5:
        if (ch->equipment[WEAR_LEGS])
            return TRUE;
        break;
    case 6:
        if (ch->equipment[WEAR_FEET])
            return TRUE;
        break;
    case 7:
        if (ch->equipment[WEAR_HANDS])
            return TRUE;
        break;
    case 8:
        if (ch->equipment[WEAR_ARMS])
            return TRUE;
        break;
    case 9:
        if (ch->equipment[WEAR_ABOUT]) 
            return TRUE;
        break;
    case 10:
        if (ch->equipment[WEAR_WAIST])
            return TRUE;
        break;
    case 11:
        if ((ch->equipment[WEAR_WRIST_L]) && (ch->equipment[WEAR_WRIST_R]))
            return TRUE;
        break;
    case 27:
        if (ch->equipment[WEAR_WRIST_R])
            return TRUE;
        break;
    case 28:
        if (ch->equipment[WEAR_WRIST_L])
            return TRUE;
        break;
    case 12:
        if (ch->equipment[EP])
            return TRUE;
        break;
    case 13:
        if (ch->equipment[ES])
            return TRUE;
        break;
    case 14:
        if (ch->equipment[WEAR_BACK])
            return TRUE;
        break;
    case 15:
        if (ch->equipment[WEAR_ON_BELT_1] && ch->equipment[WEAR_ON_BELT_2])
            return TRUE;
        break;
    case 16:
        if (ch->equipment[ETWO])
            return TRUE;
        break;
    case 17:
        if (ch->equipment[WEAR_IN_HAIR])
            return TRUE;
        break;
    case 18:
        if (ch->equipment[WEAR_FACE])
            return TRUE;
        break;
    case 19:
        if (ch->equipment[WEAR_ANKLE] && ch->equipment[WEAR_ANKLE_L])
            return TRUE;
        break;
    case 29:
        if (ch->equipment[WEAR_ANKLE])
            return TRUE;
        break;
    case 30:
        if (ch->equipment[WEAR_ANKLE_L])
            return TRUE;
        break;
    case 20:
        if (ch->equipment[WEAR_LEFT_EAR])
            return TRUE;
        break;
    case 21:
        if (ch->equipment[WEAR_RIGHT_EAR])
            return TRUE;
        break;
    case 22:
        if (ch->equipment[WEAR_FOREARMS])
            return TRUE;
        break;
    case 23:
        if (ch->equipment[WEAR_ABOUT_HEAD])
            return TRUE;
        break;
    case 24:
        if (ch->equipment[WEAR_ABOUT_THROAT])
            return TRUE;
        break;
    case 25:
        if (ch->equipment[WEAR_SHOULDER_R] && ch->equipment[WEAR_SHOULDER_L])
            return TRUE;
        break;
    case 31:
        if (ch->equipment[WEAR_SHOULDER_R])
            return TRUE;
        break;
    case 32:
        if (ch->equipment[WEAR_SHOULDER_L])
            return TRUE;
        break;
    case 26:
        if (ch->equipment[WEAR_OVER_SHOULDER_R] && ch->equipment[WEAR_OVER_SHOULDER_L])
            return TRUE;
        break;
    case 33:
        if (ch->equipment[WEAR_OVER_SHOULDER_R])
            return TRUE;
        break;
    case 34:
        if (ch->equipment[WEAR_OVER_SHOULDER_L])
            return TRUE;
        break;

    default:
        break;
    }
    return FALSE;
}

void
wear(CHAR_DATA * ch, OBJ_DATA * obj_object, int keyword, char *preHow, char *postHow)
{
    char to_char[MAX_STRING_LENGTH];
    char obj_keyword[MAX_STRING_LENGTH];
    char buffer2[MAX_STRING_LENGTH];

    find_obj_keyword(obj_object,
     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
     ch, obj_keyword, sizeof(obj_keyword));

    switch (keyword) {
    case 0: /* worn as belt */
        if (CAN_WEAR(obj_object, ITEM_WEAR_BELT)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear a belt.\n\r");
            } else {
                sprintf(to_char, "you strap on ~%s as a belt", obj_keyword);

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_BELT, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_BELT);
            }
        } else {
            cprintf(ch, "You can't wear that as a belt.\n\r");
        }
        break;

    case 1: /* finger */
        if (CAN_WEAR(obj_object, ITEM_WEAR_FINGER)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You are already wearing something on all of" " your fingers.\n\r");
            } else {
                int loc;
                if (!ch->equipment[WEAR_FINGER_L]) {
                    sprintf(to_char, "you slip ~%s onto the index finger of your left hand", obj_keyword);
                    loc = WEAR_FINGER_L;
                } else if (!ch->equipment[WEAR_FINGER_R]) {
                    sprintf(to_char, "you slip ~%s onto the index finger of your right hand", obj_keyword);
                    loc = WEAR_FINGER_R;
                } else if (!ch->equipment[WEAR_FINGER_L2]) {
                    sprintf(to_char, "you slip ~%s onto the middle finger of your left hand", obj_keyword);
                    loc = WEAR_FINGER_L2;
                } else if (!ch->equipment[WEAR_FINGER_R2]) {
                    sprintf(to_char, "you slip ~%s onto the middle finger of your right hand", obj_keyword);
                    loc = WEAR_FINGER_R2;
                } else if (!ch->equipment[WEAR_FINGER_L3]) {
                    sprintf(to_char, "you slip ~%s onto the ring finger of your left hand", obj_keyword);
                    loc = WEAR_FINGER_L3;
                } else if (!ch->equipment[WEAR_FINGER_R3]) {
                    sprintf(to_char, "you slip ~%s onto the ring finger of your right hand", obj_keyword);
                    loc = WEAR_FINGER_R3;
                } else if (!ch->equipment[WEAR_FINGER_L4]) {
                    sprintf(to_char, "you slip ~%s onto the pinky finger of your left hand", obj_keyword);
                    loc = WEAR_FINGER_L4;
                } else if (!ch->equipment[WEAR_FINGER_R4]) {
                    sprintf(to_char, "you slip ~%s onto the pinky finger of your right hand", obj_keyword);
                    loc = WEAR_FINGER_R4;
                } else if (!ch->equipment[WEAR_FINGER_L5]) {
                    sprintf(to_char, "you slip ~%s onto the thumb of your left hand", obj_keyword);
                    loc = WEAR_FINGER_L5;
                } else if (!ch->equipment[WEAR_FINGER_R5]) {
                    sprintf(to_char, "you slip ~%s onto the thumb of your right hand", obj_keyword);
                    loc = WEAR_FINGER_R5;
                } else {
                    errorlog("Trying to wear on finger incorrectly.");
                    return;
                }

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
                perform_wear(ch, obj_object, loc, preHow, postHow);
            }
        } else {
            cprintf(ch, "You can't wear that on your finger.\n\r");
        }
        break;
    case 2:
        if (CAN_WEAR(obj_object, ITEM_WEAR_NECK)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch,"You already wear something around your neck.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you bow your head, placing ~%s about your neck", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_NECK, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_NECK);
            }
        } else {
            cprintf(ch, "You can't wear that around your neck.\n\r");
        }
        break;
    case 3:
        if (CAN_WEAR(obj_object, ITEM_WEAR_BODY)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on your body.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you wear ~%s on your body", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_BODY, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_BODY);
            }
        } else {
            cprintf(ch, "You can't wear that on your body.\n\r");
        }
        break;
    case 4:
        if (CAN_WEAR(obj_object, ITEM_WEAR_HEAD)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on your head.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you place ~%s on your head", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_HEAD, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_HEAD);
            }
        } else {
            cprintf(ch, "You can't wear that on your head.\n\r");
        }
        break;
    case 5:
        if (CAN_WEAR(obj_object, ITEM_WEAR_LEGS)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on your legs.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you wear ~%s on your legs", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_LEGS, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_LEGS);
            }
        } else {
            cprintf(ch,"You can't wear that on your legs.\n\r");
        }
        break;
    case 6:
        if (CAN_WEAR(obj_object, ITEM_WEAR_FEET)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on your feet.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you slip ~%s onto your feet", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_FEET, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_FEET);
                if ((obj_object->obj_flags.type == ITEM_ARMOR)
                    && (IS_AFFECTED(ch, CHAR_AFF_SNEAK)))
                    REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_SNEAK);
            }
        } else {
            cprintf(ch, "You can't wear that on your feet.\n\r");
        }
        break;
    case 7:
        if (CAN_WEAR(obj_object, ITEM_WEAR_HANDS)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on your hands.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you pull ~%s onto your hands", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_HANDS, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_HANDS);
            }
        } else {
            cprintf(ch, "You can't wear that on your hands.\n\r");
        }
        break;
    case 8:
        if (CAN_WEAR(obj_object, ITEM_WEAR_ARMS)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on your arms.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you fit ~%s onto your arms", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_ARMS, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_ARMS);
            }
        } else {
            cprintf(ch, "You can't wear that on your arms.\n\r");
        }
        break;
    case 9:
        if (CAN_WEAR(obj_object, ITEM_WEAR_ABOUT)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something about your body.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you wear ~%s about your body", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_ABOUT, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_ABOUT);

                if( is_cloak(ch, obj_object)
                 && IS_SET(obj_object->obj_flags.value[1], CONT_CLOSED))
                    REMOVE_BIT(obj_object->obj_flags.value[1], CONT_CLOSED);

            }
        } else {
            cprintf(ch, "You can't wear that about your body.\n\r");
        }
        break;
    case 10:
        if (CAN_WEAR(obj_object, ITEM_WEAR_WAIST)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something about your waist.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you strap ~%s about your waist", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_WAIST, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_WAIST);
            }
        } else {
            cprintf(ch, "You can't wear that about your waist.\n\r");
        }
        break;
    case 11:
        if (CAN_WEAR(obj_object, ITEM_WEAR_WRIST)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something around both your wrists.\n\r");
            } else {
                int loc;

                if (ch->equipment[WEAR_WRIST_R]) {
                    sprintf(to_char, "you fasten ~%s about your left wrist", obj_keyword);
                    loc = WEAR_WRIST_L;
                } else {
                    sprintf(to_char, "you fasten ~%s about your right wrist", obj_keyword);
                    loc = WEAR_WRIST_R;
                }

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that around your wrist.\n\r");
        }
        break;
    case 12:
        if (CAN_WEAR(obj_object, ITEM_EP)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You are already wielding something.\n\r");
            } else {
                if (GET_OBJ_WEIGHT(obj_object) > GET_STR(ch)) {
                    cprintf(ch, "It is too heavy for you to use.\n\r");
                } else {
                    sprintf(to_char, "you brandish ~%s", obj_keyword);

                    if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                        return;

                    if (GET_ITEM_TYPE(obj_object) == ITEM_WEAPON)
                        draw_delay(ch, obj_object);
                    perform_wear(ch, obj_object, EP, preHow, postHow);
                    obj_from_char(obj_object);
                    equip_char(ch, obj_object, EP);

                    if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                        light_object(ch, obj_object, NULL);

                    /* Fix for drawing while subduing someone - Tiernan */
                    if (ch->specials.subduing != (struct char_data *) 0) {
                        act("$n releases $N, who immediately moves away.", FALSE, ch, 0,
                            ch->specials.subduing, TO_NOTVICT);
                        act("$n releases you, and you immediately move away.", FALSE, ch, 0,
                            ch->specials.subduing, TO_VICT);
                        act("You release $N, and $E immediately moves away.", FALSE, ch, 0,
                            ch->specials.subduing, TO_CHAR);

                        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
                        ch->specials.subduing = (struct char_data *) 0;
                    }
                }
            }
        } else {
            cprintf(ch, "You can't wield that.\n\r");
        }
        break;

    case 13:
        if (CAN_WEAR(obj_object, ITEM_ES)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch,"You are already holding something.\n\r");
            } else {
                if (GET_OBJ_WEIGHT(obj_object) > GET_STR(ch)) {
                    cprintf(ch, "It is too heavy for you to use.\n\r");
                } else {
                    sprintf(to_char, "you hold ~%s", obj_keyword);

                    if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                        return;

                    if (GET_ITEM_TYPE(obj_object) == ITEM_WEAPON)
                        draw_delay(ch, obj_object);
                    perform_wear(ch, obj_object, ES, preHow, postHow);
                    obj_from_char(obj_object);
                    equip_char(ch, obj_object, ES);

                    if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                        light_object(ch, obj_object, NULL);

                    /* Fix for drawing while subduing someone - Tiernan */
                    if (ch->specials.subduing != (struct char_data *) 0) {
                        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
                        ch->specials.subduing = (struct char_data *) 0;
                    }
                }
            }
        } else {
            cprintf(ch, "You can't hold this.\n\r");
        }
        break;
    case 14:
        if (CAN_WEAR(obj_object, ITEM_WEAR_BACK)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on your back.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you strap ~%s across your back", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_BACK, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_BACK);
            }
        } else {
            cprintf(ch, "You can't wear that on your back.\n\r");
        }
        break;
    case 15:
        if (CAN_WEAR(obj_object, ITEM_WEAR_ON_BELT)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You can't wear any more on your belt.\n\r");
            } else {
                if (ch->equipment[WEAR_BELT]) {
                    char obj_keyword2[MAX_STRING_LENGTH];

                    find_obj_keyword(ch->equipment[WEAR_BELT],
                     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
                     ch, obj_keyword2, sizeof(obj_keyword2));

                    if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                        light_object(ch, obj_object, NULL);

                    sprintf(to_char, "you hang ~%s on ~%s", obj_keyword, obj_keyword2);

                    if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                        return;
                    int loc = WEAR_ON_BELT_1;

                    if (ch->equipment[WEAR_ON_BELT_1])
                        loc = WEAR_ON_BELT_2;

                    perform_wear(ch, obj_object, loc, preHow, postHow);
                    obj_from_char(obj_object);
                    equip_char(ch, obj_object, loc);
                } else {
                    cprintf(ch, "You aren't wearing a belt!\n\r");
                }
            }
        } else {
            cprintf(ch, "You can't wear that on your belt.\n\r");
        }
        break;
    case 16:                   /* equip two */
        if (CAN_WEAR(obj_object, ITEM_EP)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already hold something in both hands.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you take hold of ~%s with both hands", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                if (GET_ITEM_TYPE(obj_object) == ITEM_WEAPON)
                    draw_delay(ch, obj_object);
                perform_wear(ch, obj_object, ETWO, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, ETWO);
            }
        } else {
            cprintf(ch, "You can't hold that with two hands.\n\r");
        }
        break;
    case 17:                   /* in hair */
        if (CAN_WEAR(obj_object, ITEM_WEAR_IN_HAIR)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch,"You already have something in your hair.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you put ~%s in your hair", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_IN_HAIR, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_IN_HAIR);
            }
        } else {
            cprintf(ch, "You can't wear that in your hair.\n\r");
        }
        break;
    case 18:                   /* on face */
        if (CAN_WEAR(obj_object, ITEM_WEAR_FACE)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already have something on your face.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you fasten ~%s across your face", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_FACE, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_FACE);
            }
        } else {
            cprintf(ch, "You can't wear that on your face.\n\r");
        }
        break;
    case 19:                   /* on ankle */
        if (CAN_WEAR(obj_object, ITEM_WEAR_ANKLE)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already have something on both ankles.\n\r");
            } else {
                int loc;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                if (!ch->equipment[WEAR_ANKLE]) {
                    sprintf(to_char, "you fasten ~%s around your right ankle", obj_keyword);
                    loc = WEAR_ANKLE;
                } else {
                    sprintf(to_char, "you fasten ~%s around your left ankle", obj_keyword);
                    loc = WEAR_ANKLE_L;
                }

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that on your ankle.\n\r");
        }
        break;
    case 29:                   /* right ankle */
        if (CAN_WEAR(obj_object, ITEM_WEAR_ANKLE)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already have something on your right ankle.\n\r");
            } else {
                int loc;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you fasten ~%s around your right ankle", obj_keyword);
                loc = WEAR_ANKLE;

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that on your ankle.\n\r");
        }
        break;
    case 30:                   /* left ankle */
        if (CAN_WEAR(obj_object, ITEM_WEAR_ANKLE)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already have something on your left ankle.\n\r");
            } else {
                int loc;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you fasten ~%s around your left ankle", obj_keyword);
                loc = WEAR_ANKLE_L;

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that on your ankle.\n\r");
        }

    case 20:                   /* on left_ear */
        if (CAN_WEAR(obj_object, ITEM_WEAR_LEFT_EAR)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already have something on your left ear.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you fasten ~%s onto your left ear", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_LEFT_EAR, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_LEFT_EAR);
            }
        } else {
            send_to_char("You can't wear that on your left ear.\n\r", ch);
        }
        break;
    case 21:                   /* on right_ear */
        if (CAN_WEAR(obj_object, ITEM_WEAR_RIGHT_EAR)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already have something on your right ear.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you fasten ~%s onto your right ear", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_RIGHT_EAR, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_RIGHT_EAR);
            }
        } else {
            cprintf(ch, "You can't wear that on your left ear.\n\r");
        }
        break;
    case 22:                   /* on forearms */
        if (CAN_WEAR(obj_object, ITEM_WEAR_FOREARMS)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already have something on your forearms.\n\r");
            } else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you fasten ~%s around your forearms", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_FOREARMS, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_FOREARMS);
            }
        } else {
            cprintf(ch, "You can't wear that on your forearms.\n\r");
        }
        break;
    case 23:                   /* on about head */
        if (CAN_WEAR(obj_object, ITEM_WEAR_ABOUT_HEAD)) {
            if (already_wearing(ch, keyword))
                cprintf(ch, "There is already something floating about " "your head.\n\r");
            else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you toss ~%s into the air, where it hovers briefly before assuming an orbit around your head", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_ABOUT_HEAD, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_ABOUT_HEAD);
            }
        } else {
            cprintf(ch, "You toss it into the air, and barely catch it before it falls to the ground."
                 "  Apparently it will not float about your head.\n\r");
        }
        break;
    case 24:                   /* on about_throat */
        if (CAN_WEAR(obj_object, ITEM_WEAR_ABOUT_THROAT)) {
            if (already_wearing(ch, keyword))
                cprintf(ch, "There is already something about your throat.\n\r");
            else {
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you tilt your head forward and fasten ~%s about your throat", obj_keyword);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, WEAR_ABOUT_THROAT, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, WEAR_ABOUT_THROAT);
            }
        } else {
            cprintf(ch, "You can't wear that about your throat.\n\r");
        }
        break;
    case 25:                   /* on shoulder */
        if (CAN_WEAR(obj_object, ITEM_WEAR_SHOULDER)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on both shoulders.\n\r");
            } else {
                int loc;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                if (!ch->equipment[WEAR_SHOULDER_L]) {
                    sprintf(to_char, "you reach up and place ~%s on your left shoulder", obj_keyword);
                    loc = WEAR_SHOULDER_L;
                } else  {
                    sprintf(to_char, "you reach up and place ~%s on your right shoulder", obj_keyword);
                    loc = WEAR_SHOULDER_R;
                }

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that on your shoulder.\n\r");
        }
        break;
    case 31:                   /* on right shoulder */
        if (CAN_WEAR(obj_object, ITEM_WEAR_SHOULDER)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on your right shoulder.\n\r");
            } else {
                int loc;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you reach up and place ~%s on your right shoulder", obj_keyword);
                loc = WEAR_SHOULDER_R;

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that on your shoulder.\n\r");
        }
        break;
    case 32:                   /* on left shoulder */
        if (CAN_WEAR(obj_object, ITEM_WEAR_SHOULDER)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something on your left shoulder.\n\r");
            } else {
                int loc;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you reach up and place ~%s on your left shoulder", obj_keyword);
                loc = WEAR_SHOULDER_L;

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that on your shoulder.\n\r");
        }
        break;
    case 26:                   /* over shoulder */
        if (CAN_WEAR(obj_object, ITEM_WEAR_OVER_SHOULDER)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something over both shoulders.\n\r");
            } else {
                int loc;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                if (!ch->equipment[WEAR_OVER_SHOULDER_L]) {
                    sprintf(to_char, "you sling ~%s over your left shoulder", obj_keyword);
                    loc = WEAR_OVER_SHOULDER_L;
                } else  {
                    sprintf(to_char, "you sling ~%s over your right shoulder", obj_keyword);
                    loc = WEAR_OVER_SHOULDER_R;
                }

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that over your shoulder.\n\r");
        }
        break;
    case 33:                   /* over right shoulder */
        if (CAN_WEAR(obj_object, ITEM_WEAR_OVER_SHOULDER)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something over your right shoulder.\n\r");
            } else {
                int loc;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you sling ~%s over your right shoulder", obj_keyword);
                loc = WEAR_OVER_SHOULDER_R;

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that over your shoulder.\n\r");
        }
        break;
    case 34:                   /* over right shoulder */
        if (CAN_WEAR(obj_object, ITEM_WEAR_OVER_SHOULDER)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something over your left shoulder.\n\r");
            } else {
                int loc;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                sprintf(to_char, "you sling ~%s over your left shoulder", obj_keyword);
                loc = WEAR_OVER_SHOULDER_L;

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that over your shoulder.\n\r");
        }
        break;
    case 27:
        if (CAN_WEAR(obj_object, ITEM_WEAR_WRIST)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something around your right wrist.\n\r");
            } else {
                int loc;

                sprintf(to_char, "you fasten ~%s about your right wrist", obj_keyword);
                loc = WEAR_WRIST_R;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that around your wrist.\n\r");
        }
        break;
    case 28:
        if (CAN_WEAR(obj_object, ITEM_WEAR_WRIST)) {
            if (already_wearing(ch, keyword)) {
                cprintf(ch, "You already wear something around your left wrist.\n\r");
            } else {
                int loc;

                sprintf(to_char, "you fasten ~%s about your left wrist", obj_keyword);
                loc = WEAR_WRIST_L;

                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    light_object(ch, obj_object, NULL);

                if (!send_how_to_char_parsed(ch, ch, to_char, preHow, postHow))
                    return;

                perform_wear(ch, obj_object, loc, preHow, postHow);
                obj_from_char(obj_object);
                equip_char(ch, obj_object, loc);
            }
        } else {
            cprintf(ch, "You can't wear that around your wrist.\n\r");
        }
        break;
    case -1:
        cprintf(ch, "Wear the %s where?.\n\r", 
         first_name(OSTR(obj_object, name), buffer2));
        break;
    case -2:
        cprintf(ch, "You can't wear the %s.\n\r",
         first_name(OSTR(obj_object, name), buffer2));
        break;
    default:
        gamelog("Unknown type called in wear.");
        break;
    }
}

int
race_no_wear(CHAR_DATA * ch, int keyword)
{
    if (GET_RACE_TYPE(ch) != RTYPE_HUMANOID) {
        if (GET_RACE(ch) == RACE_MANTIS) {
            if ((keyword == 2) ||      /* neck       */
                (keyword == 9) ||      /* back       */
                (keyword == 10) ||     /* waist      */
                (keyword == 15) ||     /* belt       */
                (keyword == 16) ||     /* on belt    */
                (keyword == 23))       /* about head */
                return (0);
            else {
                switch (keyword) {
                case 1:        /* finger */
                    send_to_char("That won't fit onto your claw.\n\r", ch);
                    break;
                case 3:        /* body */
                    send_to_char("You there's no room for your second pair of "
                                 "arms inside that.\n\r", ch);
                    break;
                case 4:        /* head */
                    send_to_char("Your head isn't shaped properly to wear " "that.\n\r", ch);
                    break;
                case 5:        /* legs     */
                    send_to_char("The claws on your feet prevent you from "
                                 "wearing these on your legs.\n\r", ch);
                    break;
                case 6:        /* feet     */
                    send_to_char("The claws on your feet prevent you from "
                                 "wearing these on your feet.\n\r", ch);
                    break;
                case 7:        /* hands    */
                    send_to_char("You cannot wear that on your hands, your "
                                 "claws get in the way.\n\r", ch);
                    break;
                case 8:        /* arms     */
                    send_to_char("Your claws prevent you from getting your arms "
                                 "into those, even just two of them.\n\r", ch);
                    break;
                case 11:       /* wrist    */
                case 27:       /* wrist_r  */
                case 28:       /* wrist_l  */
                    send_to_char("Your wrists are too narrow to wear this " "correctly.\n\r", ch);
                    break;
                case 17:       /* in hair  */
                    send_to_char("But you have no hair to wear that in?\n\r", ch);
                    break;
                case 18:       /* face     */
                    send_to_char("Your face is not shaped correctly for this " "to fit.\n\r", ch);
                    break;
                case 19:       /* ankle    */
                    send_to_char("Your ankles are too narrow to wear this" " properly.\n\r", ch);
                    break;
                case 20:       /* left ear */
                    send_to_char("But you have no ears to wear that on?\n\r", ch);
                    break;
                case 21:       /* right ear */
                    send_to_char("But you have no ears to wear that on?\n\r", ch);
                    break;
                case 22:       /* forearms */
                    send_to_char("Your forearms are too narrow to wear " "this properly.\n\r", ch);
                    break;
                default:       /* Don't know what it is, but still can't wear it */
                    break;
                }
                return (1);
            }
        }
        switch (GET_RACE_TYPE(ch)) {
        case RTYPE_AVIAN_FLYING:
        case RTYPE_AVIAN_FLIGHTLESS:
            if ((keyword == 2) ||      /* neck */
                (keyword == 23) ||     /* about head */
                (keyword == 3))        /* body */
                return (0);
            else {
                return (1);
            }
            break;
        case RTYPE_REPTILIAN:
            if ((keyword == 2) ||      /* neck */
                (keyword == 3) ||      /* body */
                (keyword == 23) ||     /* about head */
                (keyword == 9) ||      /* back */
                (keyword == 27) ||     /* wrist_l */
                (keyword == 28) ||     /* wrist_r */
                (keyword == 11))       /* wrist */
                return (0);
            else
                return (1);
            break;
        default:
            return (1);
        }
    } else {
        if (GET_RACE(ch) == RACE_GITH) {
            switch (keyword) {
            case 1:            /* finger */
                send_to_char("Your fingers are too twisted and knobby for it to " "stay on.\n\r",
                             ch);
                return (1);
            case 6:            /* feet   */
                send_to_char("Your long, twisted toes and their claws prevent "
                             "your feet from fitting into those.\n\r", ch);
                return (1);
            case 7:            /* hands  */
                send_to_char("Your long, twisted fingers and their claws prevent "
                             "your hands from fitting into those.\n\r", ch);
                return (1);
            default:
                return (0);
            }
        }
        if (GET_RACE(ch) == RACE_MUL || GET_RACE(ch) == RACE_DWARF) {
            switch (keyword) {
            case 17:           /* in hair  */
                send_to_char("But you have no hair to wear that in?\n\r", ch);
                return (1);
            default:
                break;
            }
        }
        return (0);
    }
}


int
get_obj_size(OBJ_DATA * obj)
{
    if (obj->obj_flags.type == ITEM_ARMOR)
        return obj->obj_flags.value[2];

    if (obj->obj_flags.type == ITEM_WORN)
        return obj->obj_flags.value[0];

    return 0;                   // as they say, size doesn't matter.
}


int
size_match(CHAR_DATA * ch, OBJ_DATA * obj)
{
    int sz, v;

    sz = get_char_size(ch);
    v = (sz / 20) + 1;
    if (obj->obj_flags.type == ITEM_ARMOR) {
        if (obj->obj_flags.value[2] >= (sz - v)) {
            if (obj->obj_flags.value[2] <= (sz + v)) {
                return TRUE;
            } else {
                act("$p is too large for you to wear.", FALSE, ch, obj, 0, TO_CHAR);
                return FALSE;
            }
        } else {
            act("$p is too small for you to wear.", FALSE, ch, obj, 0, TO_CHAR);
            return FALSE;
        }
    } else if (obj->obj_flags.type == ITEM_WORN) {
        if (obj->obj_flags.value[0] >= (sz - v)) {
            if (obj->obj_flags.value[0] <= (sz + v)) {
                return TRUE;
            } else {
                act("$p is too large for you to wear.", FALSE, ch, obj, 0, TO_CHAR);
                return FALSE;
            }
        } else {
            act("$p is too small for you to wear.", FALSE, ch, obj, 0, TO_CHAR);
            return FALSE;
        }
    } else
        return TRUE;
}

int
get_likely_wear_location(OBJ_DATA *obj) {
    int keyword = -2;

    if (CAN_WEAR(obj, ITEM_WEAR_OVER_SHOULDER))
        keyword = 26;
    if (CAN_WEAR(obj, ITEM_WEAR_SHOULDER))
        keyword = 25;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT_THROAT))
        keyword = 24;
    if (CAN_WEAR(obj, ITEM_WEAR_LEFT_EAR))
        keyword = 20;
    if (CAN_WEAR(obj, ITEM_WEAR_RIGHT_EAR))
        keyword = 21;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT_HEAD))
        keyword = 23;
    if (CAN_WEAR(obj, ITEM_WEAR_FOREARMS))
        keyword = 22;
    if (CAN_WEAR(obj, ITEM_WEAR_IN_HAIR))
        keyword = 17;
    if (CAN_WEAR(obj, ITEM_WEAR_FACE))
        keyword = 18;
    if (CAN_WEAR(obj, ITEM_WEAR_ANKLE))
        keyword = 19;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
        keyword = 10;
    if (CAN_WEAR(obj, ITEM_WEAR_BELT))
        keyword = 0;
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
        keyword = 1;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK))
        keyword = 2;
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
        keyword = 11;
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
        keyword = 8;
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
        keyword = 7;
    if (CAN_WEAR(obj, ITEM_WEAR_FEET))
        keyword = 6;
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
        keyword = 5;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
        keyword = 9;
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
        keyword = 4;
    if (CAN_WEAR(obj, ITEM_WEAR_BODY))
        keyword = 3;
    if (CAN_WEAR(obj, ITEM_WEAR_BACK))
        keyword = 14;
    if (CAN_WEAR(obj, ITEM_WEAR_ON_BELT))
        keyword = 15;
    return keyword;
}


void
cmd_wear(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[256];
    char buffer[MAX_STRING_LENGTH];
    OBJ_DATA *obj_object;
    OBJ_DATA *obj;
    OBJ_DATA *next_obj;
    int keyword;

    static const char * const keywords[] = {
        "belt",                 /* 0 */
        "finger",               /* 1 */
        "neck",                 /* 2 */
        "body",                 /* 3 */
        "head",                 /* 4 */
        "legs",                 /* 5 */
        "feet",                 /* 6 */
        "hands",                /* 7 */
        "arms",                 /* 8 */
        "about",                /* 9 */
        "waist",                /* 10 */
        "wrist",                /* 11 */
        "primary hand",         /* 12 */
        "secondary hand",       /* 13 */
        "back",                 /* 14 */
        "on belt",              /* 15 */
        "both hands",           /* 16 */
        "hair",                 /* 17 */
        "face",                 /* 18 */
        "ankle",                /* 19 */
        "left ear",             /* 20 */
        "right ear",            /* 21 */
        "forearms",             /* 22 */
        "about head",           /* 23 */
        "about throat",         /* 24 */
        "shoulder",             /* 25 */
        "over shoulder",	/* 26 */
        "right wrist",          /* 27 */
        "left wrist",           /* 28 */
        "right ankle",          /* 29 */
        "left ankle",           /* 30 */
        "right shoulder",       /* 31 */
        "left shoulder",        /* 32 */
        "over right shoulder",  /* 33 */
        "over left shoulder",   /* 34 */
        "\n"
    };

    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];

    if (GET_POS(ch) == POSITION_FIGHTING) {
        send_to_char("Not while fighting.\n\r", ch);
        return;
    }

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, arg1, sizeof(arg1));
    strcpy(arg2, argument);     /* store the rest, some locs have multi-words */

    if (!stricmp(arg1, "all")) {
        for (obj = ch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (CAN_SEE_OBJ(ch, obj)) {
                keyword = get_likely_wear_location(obj);
                if (race_no_wear(ch, keyword) && (keyword > 0))
                    cprintf(ch, "You can't seem to wear %s correctly.\n\r",
                     format_obj_to_char(obj, ch, 1));
                else if (keyword >= 0 
                 && size_match(ch, obj)
                 && !execute_obj_program(obj, ch, 0, CMD_ON_EQUIP, "wear")
                 && !already_wearing(ch, keyword)) {
                    wear(ch, obj, keyword, preHow, postHow);
                    if (!IS_IMMORTAL(ch)) {
                        if (ch->specials.act_wait <= 0)
                            ch->specials.act_wait = number(3,5);
                        parse_command(ch, "wear all");
                        return;
                    }
                }
            }
        }
        return;
    }

    if ((strlen(arg1) > 4) && !strnicmp(arg1, "all.", 4)) {
        strcpy(buf, arg1 + 4);
        for (obj = ch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;

            if (isname(buf, OSTR(obj, name))) {
                keyword = get_likely_wear_location(obj);
                if (race_no_wear(ch, keyword) && (keyword > 0))
                    cprintf(ch, "You can't seem to wear %s correctly.\n\r",
                     format_obj_to_char(obj, ch, 1));
                else if (keyword >= 0
                 && size_match(ch, obj)
                 && !execute_obj_program(obj, ch, 0, CMD_ON_EQUIP, "wear")
                 && !already_wearing(ch, keyword)) {
                    wear(ch, obj, keyword, preHow, postHow);
                    if (!IS_IMMORTAL(ch)) {
                        if (ch->specials.act_wait <= 0)
                             ch->specials.act_wait = number(3,5);
                        sprintf(buffer, "wear all.%s", buf);
                        parse_command(ch, buffer);
                        return;
                    }
                }
            }
        }
        return;
    }

    if (*arg1) {
        obj_object = get_obj_in_list_vis(ch, arg1, ch->carrying);
        if (obj_object) {
            if (*arg2 || (FALSE /* New code for open/closed cloaks 4/3/98 -Morg */
                          && (strcmp(arg2, "closed") || strcmp(arg2, "open")))) {
                keyword = search_block(arg2, keywords, FALSE);

                /* Partial Match */
                if (keyword == -1) {
                    cprintf(ch, "%s is an unknown body location.\n\r", arg2);
                } else {
                    if (race_no_wear(ch, keyword))
                        cprintf(ch, "You can't seem to wear %s correctly.\n\r",
                         format_obj_to_char(obj_object, ch, 1));
                    else if (size_match(ch, obj_object)
                     && !execute_obj_program(obj_object, ch, 0, CMD_ON_EQUIP, "wear")){
                        wear(ch, obj_object, keyword, preHow, postHow);
                    }
                }
            } else {
                keyword = get_likely_wear_location(obj_object);
                if ((GET_ARMOR_MAX_WEIGHT(ch) > GET_OBJ_WEIGHT(obj_object))
                    && (GET_ARMOR_MAX_WEIGHT(ch) != -1)) {
                    cprintf(ch, "Such armor would restrict your abilities, and you are forbidden to wear it.\n\r");
                    return;
                }
                if (race_no_wear(ch, keyword) && (keyword > 0))
                    cprintf(ch, "You can't seem to wear %s correctly.\n\r",
                     format_obj_to_char(obj_object, ch, 1));
                else if (size_match(ch, obj_object)
                 && !execute_obj_program(obj_object, ch, 0, CMD_ON_EQUIP, "wear")){
                    wear(ch, obj_object, keyword, preHow, postHow);
                }
            }
        } else {
            cprintf(ch, "You do not seem to have the '%s'.\n\r", arg1);
        }
    } else {
        cprintf(ch, "Wear what?\n\r");
    }
}

void
cmd_rtwo(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *obj;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char keyword[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

    if ((obj = ch->equipment[ETWO])) {
        find_obj_keyword(obj,
         FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch,
         keyword, sizeof(keyword));

        sprintf(to_char, "you stop holding ~%s", keyword);
        sprintf(to_room, "@ stops holding ~%s", keyword);

        if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
         to_room, to_room,
         (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
            return;
        }

        obj_to_char(unequip_char(ch, ETWO), ch);

        if (GET_ITEM_TYPE(obj) == ITEM_LIGHT)
            extinguish_object(ch, obj);
    } else
        send_to_char("You aren't holding anything in both hands.\n\r", ch);
}

void
cmd_rp(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *obj;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char keyword[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

    if ((obj = ch->equipment[EP])) {
        find_obj_keyword(obj,
         FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch,
         keyword, sizeof(keyword));

        sprintf(to_char, "you stop holding ~%s.", keyword);
        sprintf(to_room, "@ stops holding ~%s.", keyword);

        if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
         to_room, to_room,
         (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
            return;
        }

        obj_to_char(unequip_char(ch, EP), ch);

        if (GET_ITEM_TYPE(obj) == ITEM_LIGHT)
            extinguish_object(ch, obj);
    } else
        send_to_char("You aren't holding anything in your primary hand.\n\r", ch);
}



void
cmd_rs(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *obj;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char keyword[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

    if ((obj = ch->equipment[ES])) {
        find_obj_keyword(obj,
         FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch,
         keyword, sizeof(keyword));

        sprintf(to_char, "you stop holding ~%s.", keyword);
        sprintf(to_room, "@ stops holding ~%s.", keyword);

        if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
         to_room, to_room,
         (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
            return;
        }

        obj_to_char(unequip_char(ch, ES), ch);

        if (GET_ITEM_TYPE(obj) == ITEM_LIGHT)
            extinguish_object(ch, obj);
    } else
        cprintf(ch, "You aren't holding anything in your secondary hand.\n\r");
}


void
cmd_etwo(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *obj;
    char buf[256], buffer[512];
    int keyword = 16;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, buf, sizeof(buf));

    if (*buf) {
        obj = get_obj_in_list_vis(ch, buf, ch->carrying);
        if (obj) {
            if (ch->equipment[EP] || ch->equipment[ES]) {
                send_to_char("You'll need two free hands.\n\r", ch);
                return;
            }
            if (GET_OBJ_WEIGHT(obj) > (GET_STR(ch) + 2)) {
                send_to_char("It's too heavy for you to use.\n\r", ch);
                return;
            }
            if (ch->lifting) {
                sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
                send_to_char(buf, ch);
                if (!drop_lifting(ch, ch->lifting))
                    return;
            }
            wear(ch, obj, keyword, preHow, postHow);
        } else {
            sprintf(buffer, "You don't seem to have the '%s'.\n\r", buf);
            send_to_char(buffer, ch);
        }
    } else {
        send_to_char("Two-handed wield what?\n\r", ch);
    }
}

void
cmd_wield(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH];
    OBJ_DATA *obj_object;
    int keyword = 12;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, buf, sizeof(buf));

    if (*buf) {
        obj_object = get_obj_in_list(ch, buf, ch->carrying);

        if (obj_object) {
            if (IS_SET(obj_object->obj_flags.extra_flags, OFL_NO_ES)) {
                cmd_etwo(ch, buf, 0, 0);
                return;
            }
            if (ch->equipment[ETWO]) {
                act("You have enough problems holding onto $p.", FALSE, ch, ch->equipment[ETWO], 0,
                    TO_CHAR);
                return;
            }
            if (ch->lifting) {
                sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
                send_to_char(buf, ch);
                if (!drop_lifting(ch, ch->lifting))
                    return;
            }

            wear(ch, obj_object, keyword, preHow, postHow);
        } else {
            sprintf(buffer, "You don't seem to have the '%s'.\n\r", buf);
            send_to_char(buffer, ch);
        }
    } else {
        send_to_char("Wield what?\n\r", ch);
    }
}


void
cmd_grab(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH];
    OBJ_DATA *obj_object;
    int keyword = 13;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, buf, sizeof(buf));

    if (*buf) {
        obj_object = get_obj_in_list(ch, buf, ch->carrying);
        if (obj_object) {
            if (IS_SET(obj_object->obj_flags.extra_flags, OFL_NO_ES)) {
                cmd_etwo(ch, buf, 0, 0);
                return;
            }
            if (ch->equipment[ETWO]) {
                act("You have enough trouble holding onto $p.", FALSE, ch, ch->equipment[ETWO], 0,
                    TO_CHAR);
                return;
            }
            if (ch->lifting) {
                sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
                send_to_char(buf, ch);
                if (!drop_lifting(ch, ch->lifting))
                    return;
            }

            wear(ch, obj_object, keyword, preHow, postHow);
        } else {
            sprintf(buffer, "You do not seem to have the '%s'.\n\r", buf);
            send_to_char(buffer, ch);
        }
    } else {
        send_to_char("Hold what?\n\r", ch);
    }
}


void
cmd_remove(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH];
    char buffer1[MAX_STRING_LENGTH];
    OBJ_DATA *obj_object;
    int j;
    char *tmp_desc;
    char buf[MAX_STRING_LENGTH];
    int i;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char keyword[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, arg1, sizeof(arg1));

    if (*arg1) {                /* if they provided an argument for what to remove */
        obj_object = get_object_in_equip(arg1, ch->equipment, &j);
        if (obj_object) {       /* if they have that object equiped */
            if (GET_POS(ch) == POSITION_FIGHTING) {     /* if the player is fighting */
                if ((j != EP) && (j != ES) && (j != ETWO)) {
                    send_to_char("Not while fighting!\n\r", ch);
                    return;
                }

                find_obj_keyword(obj_object,
                 FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
                 ch, keyword, sizeof(keyword));

                sprintf(to_char, "you drop ~%s", keyword);
                sprintf(to_room, "@ drops ~%s", keyword);

                if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
                 to_room, to_room,
                 (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
                    return;
                }

                obj_to_room(unequip_char(ch, j), ch->in_room);
             
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT)
                    if (drop_light(obj_object, ch->in_room))
                        cprintf(ch, "There is nothing left now.\n\r");
                return;
            }
            /* if the player is fighting */
            if (CAN_CARRY_N(ch) != IS_CARRYING_N(ch)) { /* if the player isn't carrying maximum */

                if (j == WEAR_BELT) {
                    if (ch->equipment[WEAR_ON_BELT_1]) {
                        sprintf(buffer, "[ Taking off %s",
                                OSTR(ch->equipment[WEAR_ON_BELT_1], short_descr));
                        obj_to_char(unequip_char(ch, WEAR_ON_BELT_1), ch);
                        if (ch->equipment[WEAR_ON_BELT_2]) {
                            sprintf(buffer1, "and %s",
                                    OSTR(ch->equipment[WEAR_ON_BELT_2], short_descr));
                            obj_to_char(unequip_char(ch, WEAR_ON_BELT_2), ch);
                        } else
                            strcpy(buffer1, "");
                        cprintf(ch, "%s %s first. ]\n\r", buffer, buffer1);
                    }
                }
                if ((obj_object->obj_flags.type == ITEM_WORN
                     || obj_object->obj_flags.type == ITEM_CONTAINER)
                    && IS_SET(obj_object->obj_flags.value[1], CONT_HOODED)
                    && IS_SET(obj_object->obj_flags.value[1], CONT_HOOD_UP)) {
                    REMOVE_BIT(obj_object->obj_flags.value[1], CONT_HOOD_UP);
                    act("$n lowers the hood of $p.", TRUE, ch, obj_object, NULL, TO_ROOM);
                    act("You lower the hood of $p.", FALSE, ch, obj_object, NULL, TO_CHAR);
                }
                if( is_cloak(ch, obj_object) 
                 && IS_SET(obj_object->obj_flags.value[1],CONT_CLOSED))
                    REMOVE_BIT(obj_object->obj_flags.value[1],CONT_CLOSED);

                obj_to_char(unequip_char(ch, j), ch);
                strcpy(buf, "");       /* clear it */
                for (i = 0; i < MAX_WEAR_EDESC; i++) {
                    if (j == wear_edesc[i].location_that_covers) {
                        tmp_desc =
                            find_ex_description(wear_edesc[i].edesc_name, ch->ex_description, TRUE);
                        if (tmp_desc && !is_covered(ch, ch, i))
                            sprintf(buf, "revealing %s", tmp_desc);
                    }
                }

                find_obj_keyword(obj_object,
                 FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
                 ch, keyword, sizeof(keyword));

                sprintf(to_char, "you stop using ~%s", keyword);
                sprintf(to_room, "@ stops using ~%s", keyword);

                if (postHow[0] != '\0' && buf[0] != '\0') 
                    strcat(buf, ", ");

                strcat(buf, postHow);

                if (!send_to_char_and_room_parsed(ch, preHow, buf, to_char,
                 to_room, to_room,
                 (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
                    equip_char(ch, obj_object, j);
                    return;
                }

                /* the extinguish could extract the obj, so move the light
                 * checks to here
                 */
                if (GET_ITEM_TYPE(obj_object) == ITEM_LIGHT && IS_LIT(obj_object))
                    extinguish_object(ch, obj_object);
            } else {
                cprintf(ch, "You can't carry that many items.\n\r");
            }
        } else {
            cprintf(ch, "You are not using it.\n\r");
        }
    } else {
        cprintf(ch, "Remove what?\n\r");
    }
}

void
cmd_fill(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *temp = 0, *from = 0;
    int amount;
    int room_liq_type;

    room_liq_type = -1;

    argument = two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

    if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying))) {
        act("You can't find that.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    from = 0;

    if ((!*arg2) && !from) {
        for (from = ch->in_room->contents; from; from = from->next_content)
            if ((from->obj_flags.type == ITEM_DRINKCON) && (from->obj_flags.value[1]))
                break;
    }

    if ((*arg2) && !from) {
        int rval;
        CHAR_DATA *tar_ch;
        rval =
            generic_find(arg2,
                         FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_LIQ_CONT_INV | FIND_LIQ_CONT_ROOM |
                         FIND_LIQ_ROOM, ch, &tar_ch, &from);
        if (rval == FIND_LIQ_ROOM) {
            from = 0;
            if (ch->in_room)
                switch (ch->in_room->sector_type) {
                case SECT_SILT:
                    room_liq_type = LIQ_SILT;
                    break;
                case SECT_DESERT:
                    room_liq_type = LIQ_SAND;
                    break;
                }
        } else if ((rval & (FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_LIQ_CONT_INV | FIND_LIQ_CONT_ROOM))
                   == 0) {
            act("That water source is not here.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    }

    if (room_liq_type == -1) {
        if (from && from == temp) {
           cprintf(ch, 
            "You can't fill %s from itself, choose something else.\n\r", 
            format_obj_to_char(from, ch, 1));
           return;
        }
        if (!from || (from->obj_flags.type != ITEM_DRINKCON)) {
            act("You can't fill up anything from that.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        switch (GET_ITEM_TYPE(temp)) {
        case ITEM_DRINKCON:
            if (temp->obj_flags.value[0] == temp->obj_flags.value[1]) { /* Full */
                act("But it's already full.", FALSE, ch, 0, 0, TO_CHAR);
                return;
            }
            if (!from->obj_flags.value[1]) {
                act("But $p is already empty.", FALSE, ch, from, 0, TO_CHAR);
                return;
            }
            if ((temp->obj_flags.value[1] != 0)
                && (temp->obj_flags.value[2] != from->obj_flags.value[2])) {
                act("There is already another liquid in it.", FALSE, ch, 0, 0, TO_CHAR);
                return;
            }

            amount =
                MIN((temp->obj_flags.value[0] - temp->obj_flags.value[1]),
                    from->obj_flags.value[1]);

            temp->obj_flags.value[1] += amount;

            /*  Should fill up with the same liquid!  */
            if (temp->obj_flags.value[2] != from->obj_flags.value[2])
                temp->obj_flags.value[2] = from->obj_flags.value[2];

            /*  If poisoned, liquid stays poisoned  */
            //      if (IS_SET(from->obj_flags.value[3], LIQFL_POISONED))
            //        MUD_SET_BIT(temp->obj_flags.value[3], LIQFL_POISONED);

            if (from->obj_flags.value[5] > 0 && from->obj_flags.value[5] < MAX_POISON)
                temp->obj_flags.value[5] = from->obj_flags.value[5];

            if (!IS_SET(from->obj_flags.value[3], LIQFL_INFINITE))
                from->obj_flags.value[1] -= amount;

            if (from->obj_flags.value[5])
                temp->obj_flags.value[5] = from->obj_flags.value[5];

            send_to_char("Ok.\n\r", ch);
            sprintf(buf, "$n fills up %s from $p.", OSTR(temp, short_descr));
            act(buf, TRUE, ch, from, 0, TO_ROOM);
            break;

        case ITEM_LIGHT:
            if (!IS_REFILLABLE(temp)) {
                act("$p doesn't seem to be refillable, perhaps you could" " just pour it on?",
                    FALSE, ch, temp, 0, TO_CHAR);
                return;
            }

            if (drink_aff[from->obj_flags.value[2]][3] <= 0) {
                send_to_char("You don't think that would serve as a very" " good fuel.\n\r", ch);
                return;
            }

            act("You fill $P from $p.", FALSE, ch, from, temp, TO_CHAR);
            act("$n fills $P from $p.", TRUE, ch, from, temp, TO_ROOM);

            amount =
                MIN(temp->obj_flags.value[1] - temp->obj_flags.value[0],
                    drink_aff[from->obj_flags.value[2]][3] * from->obj_flags.value[1]);

            temp->obj_flags.value[0] += amount;

            if (!IS_SET(from->obj_flags.value[3], LIQFL_INFINITE))
                from->obj_flags.value[1] -= MAX(1, amount / drink_aff[from->obj_flags.value[2]][3]);
            break;
        default:
            act("You can't fill up anything from that.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    } else {
        switch (GET_ITEM_TYPE(temp)) {
        case ITEM_DRINKCON:
            if ((temp->obj_flags.value[1] != 0) && (temp->obj_flags.value[2] != room_liq_type)) {
                act("There is already another liquid in it.", FALSE, ch, 0, 0, TO_CHAR);
                return;
            }

            /*  Should fill up with the same liquid!  */
            amount = temp->obj_flags.value[0] - temp->obj_flags.value[1];
            temp->obj_flags.value[1] += amount;
            temp->obj_flags.value[2] = room_liq_type;

            sprintf(buf, "You fill up $o with %s from the ground.", drinknames[room_liq_type]);
            act(buf, TRUE, ch, temp, 0, TO_CHAR);

            sprintf(buf, "$n fills up $o with %s from the ground.", drinknames[room_liq_type]);
            act(buf, TRUE, ch, temp, 0, TO_ROOM);
            break;

        default:
            act("You can't fill up anything from that.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    }
}

/* comment this out if hoods/veils start acting screwey -Tenebrius */
/* #define REVERSE_HOODS_FOR_VEILS XXX */

void
cmd_raise(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
#ifdef REVERSE_HOODS_FOR_VEILS
    OBJ_DATA *hood;
    OBJ_DATA *tmp;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int j;
    int reverse = FALSE;


    argument = one_argument(argument, arg, sizeof(arg));

    if ((hood = get_object_in_equip_vis(ch, arg, ch->equipment, &j)) == NULL) {
        if (strcmp(arg, "hood")) {
            if (strcmp(arg, "veil")) {

                send_to_char("You aren't wearing anything like that.\n\r", ch);
                return;
            } else {
                reverse = TRUE;
            }
        } else {
            reverse = FALSE;
        }


        /* Find a hood object worn */
        for (j = 0; j < MAX_WEAR; j++) {
            tmp = ch->equipment[j];
            if (reverse) {
                if (tmp && ((tmp->obj_flags.type == ITEM_WORN)
                            || (tmp->obj_flags.type == ITEM_CONTAINER))
                    && IS_SET(tmp->obj_flags.value[1], CONT_HOODED)
                    && !IS_SET(tmp->obj_flags.value[1], CONT_HOOD_UP)) {
                    hood = tmp;
                    break;
                }
            } else if (tmp && ((tmp->obj_flags.type == ITEM_WORN)
                               || (tmp->obj_flags.type == ITEM_CONTAINER))
                       && IS_SET(tmp->obj_flags.value[1], CONT_HOODED)
                       && IS_SET(tmp->obj_flags.value[1], CONT_HOOD_UP)) {
                hood = tmp;
                break;
            }

        }

        if (!hood) {
            if (reverse)
                send_to_char("You're not wearing anything with a veil.\n\r", ch);
            else
                send_to_char("You're not wearing anything with a hood.\n\r", ch);
            return;
        }
    }
    if (reverse) {
        if (((hood->obj_flags.type != ITEM_WORN)
             && hood->obj_flags.type != ITEM_CONTAINER)
            || IS_SET(hood->obj_flags.value[1], CONT_HOODED)) {
            send_to_char("You can't find the veil to raise.\n\r", ch);
            return;
        }
    } else {
        if (((hood->obj_flags.type != ITEM_WORN)
             && hood->obj_flags.type != ITEM_CONTAINER)
            || !IS_SET(hood->obj_flags.value[1], CONT_HOODED)) {
            send_to_char("You can't find the hood to raise.\n\r", ch);
            return;
        }
    };

    if (reverse) {
        if (!IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
            send_to_char("The veil is already up!\n\r", ch);
            return;
        }
    } else {
        if (IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
            send_to_char("The hood is already up!\n\r", ch);
            return;
        }
    };

    if (reverse) {
        tmp = ch->equipment[WEAR_ABOUT];
        if (tmp && (hood != tmp)
            && ((tmp->obj_flags.type == ITEM_WORN)
                || (tmp->obj_flags.type == ITEM_CONTAINER))
            && IS_SET(tmp->obj_flags.value[1], CONT_HOODED)
            && !IS_SET(tmp->obj_flags.value[1], CONT_HOOD_UP)) {
            sprintf(buf, "You must first lower %s's viel.\n\r", OSTR(tmp, short_descr));
            send_to_char(buf, ch);
            return;
        }
    } else {
        tmp = ch->equipment[WEAR_ABOUT];
        if (tmp && (hood != tmp)
            && ((tmp->obj_flags.type == ITEM_WORN)
                || (tmp->obj_flags.type == ITEM_CONTAINER))
            && IS_SET(tmp->obj_flags.value[1], CONT_HOODED)
            && IS_SET(tmp->obj_flags.value[1], CONT_HOOD_UP)) {
            sprintf(buf, "You must first lower %s's hood.\n\r", OSTR(tmp, short_descr));
            send_to_char(buf, ch);
            return;
        }
    }

    if (reverse) {
        act("$n raises the veil of $p.", TRUE, ch, hood, NULL, TO_ROOM);
        act("You raise the veil of $p.", FALSE, ch, hood, NULL, TO_CHAR);
        MUD_SET_BIT(hood->obj_flags.value[1], CONT_HOOD_UP);
        return;
    } else {
        act("$n raises the hood of $p.", TRUE, ch, hood, NULL, TO_ROOM);
        act("You raise the hood of $p.", FALSE, ch, hood, NULL, TO_CHAR);
        MUD_SET_BIT(hood->obj_flags.value[1], CONT_HOOD_UP);
        return;
    }


#else
    OBJ_DATA *hood = NULL;
    OBJ_DATA *raised = NULL;
    OBJ_DATA *tmp = NULL;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    int j;

    extract_emote_arguments(argument, preHow, sizeof(preHow), args,
                            sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, arg, sizeof(arg));

    if ((hood = get_object_in_equip_vis(ch, arg, ch->equipment, &j)) == NULL) {
        if (strcmp(arg, "hood")) {
            send_to_char("You aren't wearing anything like that.\n\r", ch);
            return;
        }

        /* Find a hood object worn */
        for (j = 0; j < MAX_WEAR; j++) {
            tmp = ch->equipment[j];
            if (tmp && ((tmp->obj_flags.type == ITEM_WORN)
                        || (tmp->obj_flags.type == ITEM_CONTAINER))
                && IS_SET(tmp->obj_flags.value[1], CONT_HOODED)) {
                if (IS_SET(tmp->obj_flags.value[1], CONT_HOOD_UP))
                    raised = tmp;
                else
                    hood = tmp;
                break;
            }
        }

        if (!raised && !hood) {
            send_to_char("You're not wearing anything with a hood.\n\r", ch);
            return;
        } else if (raised) {
            act("$p already has its hood up.", FALSE, ch, raised, NULL, TO_CHAR);
            return;
        }
    }

    if (((hood->obj_flags.type != ITEM_WORN)
         && hood->obj_flags.type != ITEM_CONTAINER)
        || !IS_SET(hood->obj_flags.value[1], CONT_HOODED)) {
        send_to_char("You can't find the hood to raise.\n\r", ch);
        return;
    }

    if (IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
        send_to_char("The hood is already up!\n\r", ch);
        return;
    }

    tmp = ch->equipment[WEAR_ABOUT];
    if (tmp && (hood != tmp)
        && ((tmp->obj_flags.type == ITEM_WORN)
            || (tmp->obj_flags.type == ITEM_CONTAINER))
        && IS_SET(tmp->obj_flags.value[1], CONT_HOODED)
        && IS_SET(tmp->obj_flags.value[1], CONT_HOOD_UP)) {
        sprintf(buf, "You must first lower %s's hood.\n\r", OSTR(tmp, short_descr));
        send_to_char(buf, ch);
        return;
    }

    find_obj_keyword(hood, 
     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
     ch, buf, sizeof(buf));

    sprintf(to_char, "you raise the hood of ~%s", buf);
    sprintf(to_room, "@ raises the hood of ~%s", buf);

    if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
     to_room, to_room, MONITOR_OTHER)) {
        return;
    }

    MUD_SET_BIT(hood->obj_flags.value[1], CONT_HOOD_UP);
    return;
#endif
}


void
cmd_lower(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *hood = NULL;
    OBJ_DATA *lowered = NULL;
    OBJ_DATA *tmp = NULL;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    int j;

    extract_emote_arguments(argument, preHow, sizeof(preHow), args,
                            sizeof(args), postHow, sizeof(postHow));

    argument = one_argument(args, arg, sizeof(arg));

    if ((hood = get_object_in_equip_vis(ch, arg, ch->equipment, &j)) == NULL) {
        if (strcmp(arg, "hood")) {
            send_to_char("You aren't wearing anything like that.\n\r", ch);
            return;
        }

        /* Find a hood object worn */
        for (j = 0; j < MAX_WEAR; j++) {
            tmp = ch->equipment[j];
            if (tmp && ((tmp->obj_flags.type == ITEM_WORN)
                        || (tmp->obj_flags.type == ITEM_CONTAINER))
                && IS_SET(tmp->obj_flags.value[1], CONT_HOODED)) {
                if (IS_SET(tmp->obj_flags.value[1], CONT_HOOD_UP))
                    hood = tmp;
                else
                    lowered = tmp;
                break;
            }
        }

        if (!lowered && !hood) {
            send_to_char("You're not wearing anything with a hood.\n\r", ch);
            return;
        } else if (lowered) {
            act("The hood of $p is already lowered.", FALSE, ch, lowered, NULL, TO_CHAR);
            return;
        }
    }

    if ((hood->obj_flags.type != ITEM_WORN && hood->obj_flags.type != ITEM_CONTAINER)
        || !IS_SET(hood->obj_flags.value[1], CONT_HOODED)) {
        send_to_char("You can't find the hood to raise.\n\r", ch);
        return;
    }

    if (!IS_SET(hood->obj_flags.value[1], CONT_HOOD_UP)) {
        send_to_char("The hood is already down!\n\r", ch);
        return;
    }

    tmp = ch->equipment[WEAR_ABOUT];
    if (tmp && (hood != tmp)
        && (tmp->obj_flags.type == ITEM_WORN || tmp->obj_flags.type == ITEM_CONTAINER)
        && IS_SET(tmp->obj_flags.value[1], CONT_HOODED)
        && IS_SET(tmp->obj_flags.value[1], CONT_HOOD_UP)) {
        sprintf(buf, "You must first lower %s's hood.\n\r", OSTR(tmp, short_descr));
        send_to_char(buf, ch);
        return;
    }

    REMOVE_BIT(hood->obj_flags.value[1], CONT_HOOD_UP);
    find_obj_keyword(hood, 
     FIND_CHAR_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM,
     ch, buf, sizeof(buf));

    sprintf(to_char, "you lower the hood of ~%s", buf);
    sprintf(to_room, "@ lowers the hood of ~%s", buf);

    if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
     to_room, to_room, MONITOR_OTHER)) {
        MUD_SET_BIT(hood->obj_flags.value[1], CONT_HOOD_UP);
        return;
    }

    return;
}

#ifdef GGGG
struct create_item_type
{
    /* All of the object(s) needed to create the item(s), and names */
    /* All of the object(s) created by the item(s), and a name to call them  */
    /* All of the object(s) created by the item(s), and a name to call them  */
    /* Skill and minimum percentage needed to create     */
    /* name - what they type in like 'craft <item> <item> <name>'  */
    /* desc - what they see when like 'craft <item> <item>'        */
    /*              and it gives them a list of things to create   */
    /*                                                             */
    /* Difficulty modifier, roll 100 vs skill,           */
    /*  -50 = Modify skill by -50 (harder to make)       */
    /*  0 = Strait Chance.                               */
    /*  50 = Modify skill by +50 (Easier to make)        */
    /*                                                   */
    /* Delay in seconds                                  */
    /* Delay minimum, if has 100% in crafting skill      */
    /* Delay maximum, if has 0% in crafting skill        */
    int from[5];
    int into_success[5];
    int into_fail[5];
    char *name;
    char *desc;
    int skill;
    int percentage_min;
    int percentage_mod;
    int delay_min;
    int delay_max;

    char *to_char_succ;
    char *to_room_succ;

    char *to_char_fail;
    char *to_room_fail;

    int only_tribe;
};
#endif
#define TRIBE_ANY -1


void replace_raw_materials(CHAR_DATA * ch, OBJ_DATA **obj_vec, int *obj_found_loc, OBJ_DATA **obj_on_obj_vec);
int show_list_of_item_could_craft_into(CHAR_DATA * ch, OBJ_DATA **obj_vec, const char *substr);
int find_item_trying_to_craft(CHAR_DATA * ch, OBJ_DATA **obj_vec, const char *into, size_t *matches);

const int PARSE_BUF_SIZE = 256;
int
parse_craft_command(const char *from, char *to, char obj_arg[][PARSE_BUF_SIZE])
{
    int i;
    char obj6[256];

    /* Blank out all of the return strings */
    strcpy(to, "");
    for (i = 0; i < CRAFT_GROUP_SIZE; i++) {
        obj_arg[i][0] = 0;
    }

    /* find the first 'into' word, the first cannot be into */
    from = one_argument(from, obj_arg[0], 256);
    for (i = 1; i < CRAFT_GROUP_SIZE; i++) {
        from = one_argument(from, obj_arg[i], 256);
        if (!strcmp(obj_arg[i], "into")) {
            obj_arg[i][0] = 0;
            strcpy(to, from);
            return TRUE;
        }
    }

    from = one_argument(from, obj6, 256);           
    if (!strcmp(obj6, "into")) {         
        strcpy(to, from);        
        return (TRUE);       
    }

    strcpy(to, from);
    return (FALSE);
}

int
get_room_skill_bonus_msg(ROOM_DATA * search_room, int skillno, char *bonus_msg) {
    char bonus_msg_str[MAX_STRING_LENGTH];
    
    if (!search_room || 
        skillno < 0 || 
        skillno >= MAX_SKILLS) {
      return FALSE;
    }
    
    sprintf(bonus_msg_str, "[SKILL_BONUS_MSG_%s]", skill_name[skillno]);

    return get_room_extra_desc_value(search_room, bonus_msg_str, bonus_msg, MAX_STRING_LENGTH);
}

int
get_room_skill_bonus(ROOM_DATA * search_room, int skillno)
{
    /* Generic room bonus,  2004/12/18.
     * Remove ifelse  on 2005/12/18 if still working ok
     * So can genericly add skill bonus's to rooms, for like a loom for weaving
     * or a fire for cooking
     * 
     * Search for the edesc [SKILL_BONUS_*skillname*] and if has it, read
     * the integer bonus to give.
     * 
     * -Tenebrius
     */

    char bonus_str[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH] = "";
    int bonus;
    
    if (!search_room || 
        skillno < 0 || 
        skillno >= MAX_SKILLS) {
      return (0);
    }
    
    sprintf(bonus_str, "[SKILL_BONUS_%s]", skill_name[skillno]);

    /* gamelog(find_str); */
    get_room_extra_desc_value(search_room, bonus_str, buffer, MAX_STRING_LENGTH);

    /*  gamelog(buffer); */
    bonus = atoi(buffer);

    return (bonus);
}

void
cmd_salvage(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{

/* ----- This happens before and after the delay ----- */

    // Constants
    const int max_results = 10;

    // Variables
    OBJ_DATA *result;
    OBJ_DATA *target;
    char buf[256];
    char monitormsg[256];
    char success_suffix[256];
    char success_prefix[256];
    int resultvnum;
    int resultquantity;
    int i;
    int weight;

    // Find the object
    one_argument(argument, buf, sizeof(buf));
    if (!*buf) {
        send_to_char("Salvage what?\n\r", ch);
        return;
    }
    target = get_obj_in_list_vis(ch, buf, ch->carrying);
    if (!target) {
        send_to_char("You aren't carrying that.\n\r", ch);
        return;
    }
    if (target->contains) {
        send_to_char("Empty it out first.\n\r", ch);
        return;
    }

/* ----- This happens before the delay ----- */

    if (!ch->specials.to_process) {

        act("You begin salvaging.", FALSE, ch, target, NULL, TO_CHAR);
        add_to_command_q(ch, argument, cmd, 3, 0);
        return;

    }

/* ----- This happens after the delay ----- */

    // Decide how much is salvaged
    weight = GET_OBJ_WEIGHT(target);
    resultquantity = number(0, weight);
    if (resultquantity > max_results) {
        resultquantity = max_results;
    } else if (resultquantity < 1) {
        resultquantity = 1;
    }
    if (isnamebracketsok("[no_salvage]", OSTR(target, name))) {
        resultquantity = 0;
    }
    if (weight < 1) {
        resultquantity = 0;
    }
    if (resultquantity < 1) {
        snprintf(success_suffix, sizeof(success_suffix),
                 ", but do not produce anything useful.\n\r");
    } else if (resultquantity == 1) {
        snprintf(success_suffix, sizeof(success_suffix), " and produce a useful piece.\n\r");
    } else if (resultquantity == 2) {
        snprintf(success_suffix, sizeof(success_suffix),
                 " and produce a couple of useful pieces.\n\r");
    } else {
        snprintf(success_suffix, sizeof(success_suffix), " and produce some useful pieces.\n\r");
    }

    // Decide what is salvaged
    switch (target->obj_flags.material) {
    case MATERIAL_CLOTH:
        resultvnum = 3283;
        act("$n tears apart $p.", FALSE, ch, target, NULL, TO_ROOM);
        snprintf(success_prefix, sizeof(success_prefix), "You tear apart $p");
        act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
        break;
    case MATERIAL_OBSIDIAN:
        resultvnum = 3284;
        act("$n shatters $p.", FALSE, ch, target, NULL, TO_ROOM);
        snprintf(success_prefix, sizeof(success_prefix), "You shatter $p");
        act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
        break;
    case MATERIAL_GLASS:
        resultvnum = 3285;
        act("$n shatters $p.", FALSE, ch, target, NULL, TO_ROOM);
        snprintf(success_prefix, sizeof(success_prefix), "You shatter $p");
        act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
        break;
    case MATERIAL_CHITIN:
        resultvnum = 3286;
        act("$n breaks $p into pieces.", FALSE, ch, target, NULL, TO_ROOM);
        snprintf(success_prefix, sizeof(success_prefix), "You break apart $p");
        act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
        break;
    case MATERIAL_SKIN:
    case MATERIAL_LEATHER:
        resultvnum = 3287;
        act("$n tears apart $p.", FALSE, ch, target, NULL, TO_ROOM);
        snprintf(success_prefix, sizeof(success_prefix), "You tear apart $p");
        act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
        break;
    case MATERIAL_ISILT:
    case MATERIAL_BONE:
        resultvnum = 3288;
        act("$n shatters $p.", FALSE, ch, target, NULL, TO_ROOM);
        snprintf(success_prefix, sizeof(success_prefix), "You shatter $p");
        act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
        break;
    case MATERIAL_WOOD:
        resultvnum = 3289;
        act("$n breaks $p into pieces.", FALSE, ch, target, NULL, TO_ROOM);
        snprintf(success_prefix, sizeof(success_prefix), "You break apart $p");
        act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
        break;
    case MATERIAL_STONE:
        resultvnum = 3293;
        act("$n breaks $p into pieces.", FALSE, ch, target, NULL, TO_ROOM);
        snprintf(success_prefix, sizeof(success_prefix), "You break apart $p");
        act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
        break;
    case MATERIAL_SILK:
        resultvnum = 3295;
        act("$n tears apart $p.", FALSE, ch, target, NULL, TO_ROOM);
        snprintf(success_prefix, sizeof(success_prefix), "You tear apart $p");
        act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
        break;
    case MATERIAL_CLAY:
      resultvnum = 18687;
      act("$n smashes $p into a scrap of clay.", FALSE, ch, target, NULL, TO_ROOM);
      snprintf(success_prefix, sizeof(success_prefix), "You smash $p into a scrap of clay");
      act(strcat(success_prefix, success_suffix), FALSE, ch, target, NULL, TO_CHAR);
      break;
    default:
        send_to_char("You can't salvage that.\n\r", ch);
        return;
    }

    // Create the resultant object(s)
    for (i = 0; i < resultquantity; i++) {
        result = read_object(resultvnum, VIRTUAL);
        if (!CAN_WEAR(result, ITEM_TAKE)) {
            obj_to_room(result, ch->in_room);
        } else {
            obj_to_char(result, ch);
        }
    }

    // Monitor and gamelog the command
    sprintf(monitormsg, "%s salvages %s.\n\r", MSTR(ch, name), OSTR(target, short_descr));
    send_to_monitor(ch, monitormsg, MONITOR_CRAFTING);

    // Destroy the salvaged object
    extract_obj(target);

}

void
cmd_craft(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
  int i, recipe_index, bonus, wait;
  int die_roll = 0;
  int worked = TRUE;
  int vnums[CRAFT_GROUP_SIZE];
  int obj_found_loc[CRAFT_GROUP_SIZE];
  
  char monitormsg[256],
    buf[MAX_STRING_LENGTH],
    oldarg[MAX_STRING_LENGTH],
    into[MAX_STRING_LENGTH];
  char args[CRAFT_GROUP_SIZE][PARSE_BUF_SIZE];

  OBJ_DATA *raw_obj[CRAFT_GROUP_SIZE];
  OBJ_DATA *obj_on_obj[CRAFT_GROUP_SIZE];
  OBJ_DATA *new_obj[CRAFT_GROUP_SIZE];
  OBJ_DATA *tool_obj = 0;
  
  memset(&vnums, 0, sizeof(vnums));
  memset(&raw_obj, 0, sizeof(raw_obj));
  memset(&obj_found_loc, 0, sizeof(obj_found_loc));
  memset(&obj_on_obj, 0, sizeof(obj_on_obj));
  memset(&new_obj, 0, sizeof(new_obj));
  memset(&args, 0, sizeof(args));
  
  if (!make_item_list) {
    send_to_char("Crafting system temporarily unavailable, try again soon\n\r", ch);
    return;
  }
  
  strcpy(oldarg, argument);
  
  parse_craft_command(argument, into, args);
  
  if (!*args[0]) {
    cprintf(ch, "Craft what?\n\r");

    return;
  }
  
  for (i = 0; i < CRAFT_GROUP_SIZE; i++) {
    if (*args[i]) {
      OBJ_DATA *tmp_obj;
      CHAR_DATA *tmp_char;
      int sub_type = generic_find(args[i], 
                                  FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_ROOM_ON_OBJ, 
                                  ch, 
                                  &tmp_char, 
                                  &tmp_obj);
      
      if (!tmp_obj) {
        cprintf(ch, "You cannot find %s to craft with.\n\r", args[i]);

        return;
      }
      
      if (GET_ITEM_TYPE(tmp_obj) == ITEM_FOOD &&
          tmp_obj->obj_flags.value[0] < obj_default[tmp_obj->nr].value[0]) {
        cprintf(ch, "You cannot craft with partially eaten food.\n\r");

        return;
      }
      
      for (int j = 0; j < i; j++) {
        if (raw_obj[j] == tmp_obj) {
          cprintf(ch, 
                  "You are already using '%s' to craft with.\n\r"
                  "Use the 'keyword' command to find number references to use for each object.\n\r", 
                  args[i]);
          
          return;
        }
      }
      
      raw_obj[i] = tmp_obj;
      switch (sub_type) {
      case FIND_OBJ_EQUIP: {
        int loc = 0;
        
        for (loc = 0; loc < MAX_WEAR; loc++) {
          if (tmp_obj->equipped_by->equipment[loc] == tmp_obj) {
            break;
          }
        }
        
        if (loc == MAX_WEAR) {
          gamelog("cmd_craft: equipped item not found in equipment!");
          cprintf(ch, "You cannot find %s to craft with.\n\r", args[i]);

          return;
        }
        obj_found_loc[i] = loc;
        break;
      }
      case FIND_OBJ_INV:
        obj_found_loc[i] = -1;
        break;
      case FIND_OBJ_ROOM:
        obj_found_loc[i] = -2;
        break;
      case FIND_OBJ_ROOM_ON_OBJ:
        obj_on_obj[i] = tmp_obj->in_obj;
        obj_found_loc[i] = -3;
        break;
      }
    }
  }
  
  for (i = 0; i < CRAFT_GROUP_SIZE; i++) {
    if (NULL == raw_obj[i]) {
      break;
    }
    
    switch (obj_found_loc[i]) {
    case -1:
      obj_from_char(raw_obj[i]);
      break;
    case -2:
      obj_from_room(raw_obj[i]);
      break;
    case -3:
      obj_from_obj(raw_obj[i]);
      break;
    default: 
      unequip_char(ch, obj_found_loc[i]);
      obj_from_char(raw_obj[i]);
      break;
    }
  }
  
  if (!*into) {
    show_list_of_item_could_craft_into(ch, raw_obj, NULL);
    goto restore_objs;
  }
  
  size_t matches;
  recipe_index = find_item_trying_to_craft(ch, raw_obj, into, &matches);
  
  if (matches > 1) {
    cprintf(ch, "There were %d matches, please be more specific.\n\r", matches);
    show_list_of_item_could_craft_into(ch, raw_obj, into);
    goto restore_objs;
  }
  
  if (matches <= 0) {
    cprintf(ch, "No recipes matched '%s'.\n\r", into);
    goto restore_objs;
  }
  
  struct create_item_type recipe = make_item_list[recipe_index];
  
  if (recipe.tool_type != -1) {
    int tool_found = FALSE;
    int quality_found = FALSE;
    
    OBJ_DATA *current_obj;
    
    // Equipped
    for (int eq_index = 0;eq_index < MAX_WEAR; eq_index++) {
      if (ch->equipment[eq_index]) {
        current_obj = ch->equipment[eq_index];
        if (ITEM_TOOL == current_obj->obj_flags.type) {
          if (current_obj->obj_flags.value[0] == recipe.tool_type ||
              (TOOL_LOOM_SMALL == recipe.tool_type && TOOL_LOOM_LARGE == current_obj->obj_flags.value[0])) {
            tool_found = TRUE;
            tool_obj = current_obj;
            if (current_obj->obj_flags.value[1] >= recipe.tool_quality) {
              quality_found = TRUE;
              break;
            }
          }
        }
      }
    }
    
    // Carrying
    if (FALSE == quality_found) {
      current_obj = ch->carrying;
      while (current_obj) {
        if (ITEM_TOOL == current_obj->obj_flags.type) {
          if (current_obj->obj_flags.value[0] == recipe.tool_type ||
              (TOOL_LOOM_SMALL == recipe.tool_type && TOOL_LOOM_LARGE == current_obj->obj_flags.value[0])) {
            tool_found = TRUE;
            tool_obj = current_obj;
            if (current_obj->obj_flags.value[1] >= recipe.tool_quality) {
              quality_found = TRUE;
              break;
            }
          }
        }
        current_obj = current_obj->next_content;
      }
    }
    
    // in room         
    if (FALSE == quality_found) {
      current_obj = ch->in_room->contents;
      while (current_obj) {
        if (ITEM_TOOL == current_obj->obj_flags.type) {
          if (current_obj->obj_flags.value[0] == recipe.tool_type ||
              (TOOL_LOOM_SMALL == recipe.tool_type && TOOL_LOOM_LARGE == current_obj->obj_flags.value[0])) {
            tool_found = TRUE;
            tool_obj = current_obj;
            if (current_obj->obj_flags.value[1] >= recipe.tool_quality) {
              quality_found = TRUE;
              break;
            }
          }
        }
        current_obj = current_obj->next_content;
      }
    }
    
    if (FALSE == tool_found && FALSE == quality_found) {
      cprintf(ch, 
              "Crafting that requires a %s.\n\r", 
              tool_name[recipe.tool_type]);
      goto restore_objs;
    }
    
    if (TRUE == tool_found && FALSE == quality_found) {
      cprintf(ch, 
              "Crafting that requires a %s of higher quality than %s.\n\r", 
              tool_name[recipe.tool_type], 
              OSTR(tool_obj, short_descr));
      goto restore_objs;
    }
   
  }
  
  if (TRUE == recipe.require_fire) {
    int found_fire = FALSE;
    OBJ_DATA *current_obj = ch->in_room->contents;
    while (current_obj) {
      if (current_obj->obj_flags.type == ITEM_LIGHT &&
          IS_LIT(current_obj) &&
          IS_CAMPFIRE(current_obj)) {
        found_fire = TRUE;
        break;
      }
      current_obj = current_obj->next_content;
    }
    
    if (FALSE == found_fire) {
      cprintf(ch, "You'll need a fire to do that.\n\r");
      goto restore_objs;
    }
  }
  
  if (!ch->specials.to_process) {
    char new_ld[MAX_INPUT_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char *tmpstr;
    strcpy(new_ld, "is here, crafting");
    
    if (tool_obj) {
      sprintf(buf, "Using %s you begin crafting %s%s from ",
              OSTR(tool_obj, short_descr),
              indefinite_article(make_item_list[recipe_index].name), 
              make_item_list[recipe_index].name);
    } else {
      sprintf(buf, "You begin crafting %s%s from ",
              indefinite_article(make_item_list[recipe_index].name), 
              make_item_list[recipe_index].name);
    }
    
    for (i = 0; i < CRAFT_GROUP_SIZE && raw_obj[i]; i++) {
      if (i > 0 && raw_obj[i + 1] == NULL)
        string_safe_cat(buf, "and ", MAX_STRING_LENGTH);
      
      switch (obj_found_loc[i]) {
      case -1: // inventory
        sprintf(buf2, "%s you are carrying", format_obj_to_char(raw_obj[i], ch, 1));
        break;
      case -2: // in the room
        sprintf(buf2, "%s in the room", format_obj_to_char(raw_obj[i], ch, 1));
        break;
      case -3: // on furniture
        sprintf(buf2, "%s", format_obj_to_char(raw_obj[i], ch, 1));
        string_safe_cat(buf, buf2, MAX_STRING_LENGTH);
        sprintf(buf2, " on %s", format_obj_to_char(obj_on_obj[i], ch, 1));
        break;
      default: // worn
        sprintf(buf2, "%s you are wearing", format_obj_to_char(raw_obj[i], ch, 1));
        break;
      }
      string_safe_cat(buf, buf2, MAX_STRING_LENGTH);
      
      // if there's a next
      if( raw_obj[i + 1] ) {
        string_safe_cat(buf, ", ", MAX_STRING_LENGTH);
      }
    }
    
    string_safe_cat(buf, ".", MAX_STRING_LENGTH);
    tmpstr = strdup(buf);
    tmpstr = format_string(tmpstr);
    cprintf(ch, "%s", tmpstr);
    free(tmpstr);
    
    OBJ_DATA *tmp = read_object(make_item_list[recipe_index].into_success[0], VIRTUAL);
    if (tmp) {
      switch (tmp->obj_flags.material) {
      case MATERIAL_WOOD:
        act("The scent of sawdust fills the air as you begin to work with the wood.", FALSE,
            ch, NULL, NULL, TO_CHAR);
        act("The scent of sawdust fills the air as $n begins to work with some wood.",
            FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with wood.");
        break;
      case MATERIAL_METAL:
        act("Soft ringing is audible as you begin to work the metal.", FALSE, ch, NULL,
            NULL, TO_CHAR);
        act("$n begins to work with some metal, accompanied by a muffled ringing sound.",
            FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here bent over a metallic object.");
        break;
      case MATERIAL_OBSIDIAN:
        act("A fine black dust rises and tiny chips of stone splinter as you begin to work the obsidian.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("A fine black dust rises as $n begins to work with obsidian, tiny chips of stone splintering into the air.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with inky black stone.");
        break;
      case MATERIAL_STONE:
        act("Tiny shards of stone scatter into the air as you begin to work the rock.",
            FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n works a piece of stone, sending tiny shards scattering into the air nearby.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here shaping a piece of stone.");
        break;
      case MATERIAL_CHITIN:
        act("You begin to carefully shape the rigid chitin.", FALSE, ch, NULL, NULL,
            TO_CHAR);
        act("$n carefully shapes a piece of rigid chitin.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with a piece of chitin.");
        break;
      case MATERIAL_GEM:
        act("You begin to polish the gemstone.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins to polish a gemstone.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with a small gleaming object.");
        break;
      case MATERIAL_GLASS:
        act("You begin to work with the glass.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins to work with a piece of glass.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with glass.");
        break;
      case MATERIAL_CLOTH:
        act("You lay out the cloth to begin work on it.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n lays out some cloth and begins to work it.", FALSE, ch, NULL, NULL,
            TO_ROOM);
        strcpy(new_ld, "is here working on some cloth.");
        break;
      case MATERIAL_LEATHER:
        act("Positioning the leather, you begin to work with the tough material.", FALSE,
            ch, NULL, NULL, TO_CHAR);
        act("$n positions some leather and begins to work the tough material.", FALSE, ch,
            NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here crafting some leather.");
        break;
      case MATERIAL_BONE:
        act("Tiny slivers of bone break off as you begin to work the brittle material.",
            FALSE, ch, NULL, NULL, TO_CHAR);
        act("Tiny slivers of bone chip off as $n begins to work the brittle material.",
            FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with bone.");
        break;
      case MATERIAL_CERAMIC:
        act("You begin to work in ceramic.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins to work in ceramics.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with ceramics.");
        break;
      case MATERIAL_HORN:
        act("Dangerously sharp shards of needle-thin bone splinter as you begin to shape the horn.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("Dangerously sharp shards of needle-thin bone splinter as $n begins to shape a horn.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here shaping a piece of horn.");
        break;
      case MATERIAL_TISSUE:
        act("You begin to work with the tissue.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins to work with some tissue.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with some tissue.");
        break;
      case MATERIAL_SILK:
        act("You lay the fine fabric out and begin to sew.", FALSE, ch, NULL, NULL,
            TO_CHAR);
        act("$n lays a section of fine fabric out and begins to sew.", FALSE, ch, NULL,
            NULL, TO_ROOM);
        strcpy(new_ld, "is here sewing some fine material.");
        break;
      case MATERIAL_IVORY:
        act("Carefully, you begin to shape the delicate ivory.", FALSE, ch, NULL, NULL,
            TO_CHAR);
        act("$n carefully begins to work with a delicate piece of ivory.", FALSE, ch, NULL,
            NULL, TO_ROOM);
        strcpy(new_ld, "is here carefully shaping some ivory.");
        break;
      case MATERIAL_DUSKHORN:
        act("You begin to work with the duskhorn.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins to work with the duskhorn.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with some duskhorn.");
        break;
      case MATERIAL_TORTOISESHELL:
        act("You begin to work with the shell.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins to work with the shell.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with some shell.");
        break;
      case MATERIAL_ISILT:
        act("You begin to work with the isilt.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins to work with the isilt.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with some isilt.");
        break;
      case MATERIAL_SALT:
        act("The salt displays its natural glow as you begin to work it.", FALSE, ch, NULL,
            NULL, TO_CHAR);
        act("$n works with some salt, the crystalline material shimmering.", FALSE, ch,
            NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with some crystalline salt.");
        break;
      case MATERIAL_GWOSHI:
        act("You begin to work with the material.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins to work with some material.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with some material.");
        break;
      case MATERIAL_DUNG:
        act("A ripe, unpleasant scent rises as you begin to work with the dung.", FALSE, ch,
            NULL, NULL, TO_CHAR);
        act("A ripe, unpleasant scent fills the air as $n begins to work with some dung.",
            FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here amidst the stench of dung.");
        break;
      case MATERIAL_PLANT:
        act("You begin to work with the plant.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins to work with the plant.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here working with a plant.");
        break;
      case MATERIAL_PAPER:
        act("Carefully, you begin to shape the delicate paper.", FALSE, ch, NULL, NULL,
            TO_CHAR);
        act("$n begins to carefully shape some delicate paper.", FALSE, ch, NULL, NULL,
            TO_ROOM);
        strcpy(new_ld, "is here shaping a delicate object.");
        break;
      case MATERIAL_FOOD:
        act("You start preparing some food.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins preparing some food.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here, preparing some food.");
        break;
      case MATERIAL_FEATHER:
        act("You start working with some feathers.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins working with some feathers.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here, working with some feathers.");
        break;
      case MATERIAL_FUNGUS:
        act("You start working with some fungus.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins working with some fungus.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here, working with some fungus.");
        break;
      case MATERIAL_CLAY:
        act("Working carefully, you begin to mold and shape the greasy clay.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("Working carefully, $n begins to mold and shape some greasy clay.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here, working with some clay.");
        break;
      default:
        act("You set to work crafting.", FALSE, ch, NULL, NULL, TO_CHAR);
        act("$n begins crafting.", FALSE, ch, NULL, NULL, TO_ROOM);
        strcpy(new_ld, "is here, crafting");
        break;
      }
      
      extract_obj(tmp);
    } else {
      gamelogf("%s tried a crafting recipe[%d] that had no into_success", MSTR(ch, name),
               recipe_index);
      cprintf(ch, "The recipe you attempted seems to be broken, please report it as a bug.\n\r");
      goto restore_objs;
    }
    
    sprintf(monitormsg, "%s begins crafting.\n\r", MSTR(ch, name));
    send_to_monitor(ch, monitormsg, MONITOR_CRAFTING);
    
    if (!change_ldesc(new_ld, ch)) {
      shhlogf("ERROR: %s's ldesc is too long in cmd_craft", MSTR(ch, name));
    }
    
    wait = make_item_list[recipe_index].delay_min;
    
    {
      float time_diff = make_item_list[recipe_index].delay_max - make_item_list[recipe_index].delay_min;
      float skill_range = 100 - make_item_list[recipe_index].percentage_min;
      float skill_relative =
        (ch->skills[make_item_list[recipe_index].skill]->learned -
         make_item_list[recipe_index].percentage_min) / skill_range;
      wait += time_diff * (1.0 - skill_relative);
    }
    
    wait /= 10;
    add_to_command_q(ch, oldarg, cmd, wait, 0);
    goto restore_objs;
  }
  
  bonus = get_room_skill_bonus(ch->in_room, make_item_list[recipe_index].skill);
  char bonus_msg[MAX_STRING_LENGTH];
  if (get_room_skill_bonus_msg(ch->in_room, make_item_list[recipe_index].skill, bonus_msg)) {
    send_to_char(bonus_msg, ch);
  }
  die_roll = dice(1, 100);
  
  if (ch->player.level > 0) {
    cprintf(ch, "Percentage mod: %d\n\r", make_item_list[recipe_index].percentage_mod);
    cprintf(ch, "plus a die roll of 1-100: %d\n\r", die_roll);
    cprintf(ch, "Must be less than...\n\r");
    cprintf(ch, "Your skill: %d\n\r", ch->skills[make_item_list[recipe_index].skill]->learned);
    cprintf(ch, "Plus a room bonus of: %d\n\r\n\r", bonus);
    cprintf(ch, "Is %d less than %d?\n\r", make_item_list[recipe_index].percentage_mod + die_roll,
            ch->skills[make_item_list[recipe_index].skill]->learned + bonus);
    if ((make_item_list[recipe_index].percentage_mod + die_roll) <
        (ch->skills[make_item_list[recipe_index].skill]->learned + bonus)) {
      send_to_char("YES.\n\r", ch);
    } else {
      send_to_char("NO.\n\r", ch);
      
    }
  }
  
  worked = ((make_item_list[recipe_index].percentage_mod + die_roll) < (ch->skills[make_item_list[recipe_index].skill]->learned + bonus));
  
  for (i = 0; i < CRAFT_GROUP_SIZE; i++) {
    if (raw_obj[i]) {
      vnums[i] = obj_index[raw_obj[i]->nr].vnum;
      extract_obj(raw_obj[i]);
    }
  }
  
  memset(&new_obj, sizeof(new_obj), 0);
  
  if (worked) {
    for (i = 0; i < CRAFT_GROUP_SIZE; i++) {
      if (make_item_list[recipe_index].into_success[i] > 0) {
        new_obj[i] = read_object(make_item_list[recipe_index].into_success[i], VIRTUAL);
        if (new_obj[i]) {
          set_crafter_edesc(ch, new_obj[i]);
          sprintf(monitormsg, "%s successfully crafts %s.\n\r",
                  MSTR(ch, name), OSTR(new_obj[i], short_descr));
          send_to_monitor(ch, monitormsg, MONITOR_CRAFTING);
        } else {
          sprintf(monitormsg, "unloadable object %d in recipe.\n\r",
                  make_item_list[recipe_index].into_success[i]);
          send_to_monitor(ch, monitormsg, MONITOR_CRAFTING);
        }
      }
    }
  } else {
    for (i = 0; i < CRAFT_GROUP_SIZE; i++) {
      if (make_item_list[recipe_index].into_fail[i] > 0) {
        new_obj[i] = read_object(make_item_list[recipe_index].into_fail[i], VIRTUAL);
        if (new_obj[i]) {
          set_crafter_edesc(ch, new_obj[i]);
          sprintf(monitormsg, "%s botches crafting, leaving %s.\n\r",
                  MSTR(ch, name), OSTR(new_obj[i], short_descr));
          send_to_monitor(ch, monitormsg, MONITOR_CRAFTING);
        } else {
          sprintf(monitormsg, "unloadable object %d in recipe.\n\r",
                  make_item_list[recipe_index].into_success[i]);
          send_to_monitor(ch, monitormsg, MONITOR_CRAFTING);
        }
      }
    }
  }
  
  for (i = 0; i < CRAFT_GROUP_SIZE; i++) {
    if (new_obj[i]) {
      if (CAN_GET_OBJ(ch, new_obj[i])) {
        obj_to_char(new_obj[i], ch);
      } else {
        obj_to_room(new_obj[i], ch->in_room);
      }
    }
  }
  
  if (worked) {
    act(make_item_list[recipe_index].to_char_succ, FALSE, ch, NULL, NULL, TO_CHAR);
    act(make_item_list[recipe_index].to_room_succ, FALSE, ch, NULL, NULL, TO_ROOM);
  } else {
    if (tool_obj &&
        IS_SET(tool_obj->obj_flags.wear_flags, ITEM_TAKE)) {
      int chSkill = ch->skills[recipe.skill]->learned;
      int recipeMod = recipe.percentage_mod;
      int toolQualityMod = tool_obj->obj_flags.value[1] * 10;
      
      int d300Roll = number(1,300);
      
      if (GET_LEVEL(ch) >= STORYTELLER) {
        cprintf(ch, "Staff Only: skill: %d, recipeMod: %d, qualityMod: %d, d300Roll: %d\n\r", 
                chSkill,
                recipeMod,
                toolQualityMod,
                d300Roll);
      }
      
      if ((chSkill + recipeMod + toolQualityMod) < d300Roll) {
        tool_obj->obj_flags.value[1] = tool_obj->obj_flags.value[1] - 1;
        if (0 > tool_obj->obj_flags.value[1]) {
          cprintf(ch, "%s breaks in attempting to craft.\n\r", OSTR(tool_obj, short_descr));
          goto restore_objs;
        } else {
          cprintf(ch, "%s degrades in quality.\n\r", OSTR(tool_obj, short_descr));
        }
      }
    }

    act(make_item_list[recipe_index].to_char_fail, FALSE, ch, NULL, NULL, TO_CHAR);
    act(make_item_list[recipe_index].to_room_fail, FALSE, ch, NULL, NULL, TO_ROOM);
    
    sprintf(monitormsg, "%s fails their crafting attempt.\n\r", MSTR(ch, name));
    send_to_monitor(ch, monitormsg, MONITOR_CRAFTING);
    
    gain_skill(ch, make_item_list[recipe_index].skill, 2);
  }
  WIPE_LDESC(ch);
  
  return;
  
 restore_objs:
  replace_raw_materials(ch, raw_obj, obj_found_loc, obj_on_obj);
  return;
}

void
replace_raw_materials(CHAR_DATA * ch, OBJ_DATA **obj_vec, int *obj_found_loc, OBJ_DATA **obj_on_obj_vec)
{
    int i;
    for (i = 0; i < CRAFT_GROUP_SIZE; i++) {
        if (obj_vec[i]) {
            switch (obj_found_loc[i]) {
            case -1: // inventory
                obj_to_char(obj_vec[i], ch);
                break;
            case -2: // room
                obj_to_room(obj_vec[i], ch->in_room);
                break;
            case -3: // on an item in the room
                obj_to_obj(obj_vec[i], obj_on_obj_vec[i]);
                break;
            default: // worn
                equip_char(ch, obj_vec[i], obj_found_loc[i]);
                break;
            }
            obj_vec[i] = 0;
        }
    }
}

int
show_list_of_item_could_craft_into(CHAR_DATA * ch, OBJ_DATA **obj_vec, const char *substr)
{
    int num_colors = 0;
    int ncount, i, j, tmp;
    char buf[MAX_INPUT_LENGTH];
    ncount = 0;

    // Looks like this can happen if the crafting.xml fails to parse
    // and then someone tries to get a list of craftable items, as in
    // "craft fruit"
    if (!make_item_list)
        return 1;

    for (i = 0; make_item_list[i].skill != SPELL_NONE; i++) {
        if ((ch->skills[make_item_list[i].skill])
            && (ch->skills[make_item_list[i].skill]->learned >= make_item_list[i].percentage_min)
            && ((make_item_list[i].only_tribe == TRIBE_ANY)
                || (IS_TRIBE(ch, make_item_list[i].only_tribe))))
        {
            int fobj[CRAFT_GROUP_SIZE];
            int tobj[CRAFT_GROUP_SIZE];

            for (j = 0; j < CRAFT_GROUP_SIZE; j++) {
                fobj[j] = obj_vec[j] ? obj_index[obj_vec[j]->nr].vnum : 0;
                tobj[j] = make_item_list[i].from[j];
            }
            qsort(fobj, CRAFT_GROUP_SIZE, sizeof(int), intcmp);
            qsort(tobj, CRAFT_GROUP_SIZE, sizeof(int), intcmp);

            if (memcmp(fobj, tobj, sizeof(fobj)) != 0)
                continue;

            if (substr && !strstr(make_item_list[i].name, substr))
                continue;

            int color_accum = 0;
            for (j = 0; j < CRAFT_GROUP_SIZE; j++) {
                if (obj_vec[j]) {
                    color_accum |= obj_vec[j]->obj_flags.color_flags;
                }
            }

            num_colors = 0;
            tmp = color_accum;
            for (num_colors = 0; tmp; num_colors++) {
                tmp &= tmp - 1;
            }

            if (num_colors == 0) {
                snprintf(buf, sizeof(buf), "You could make %s%s from that.\n\r",
                         indefinite_article(make_item_list[i].name), make_item_list[i].name);
            } else if (num_colors > 2) {
                snprintf(buf, sizeof(buf), "You could make a multi-colored %s from that.\n\r", make_item_list[i].name);
            } else {
                char color_buf[256] = "";
                int second = 0;
                for (j = 0; *obj_color_flag[j].name; j++) {
                    if (obj_color_flag[j].bit & color_accum) {
                        if (second) {
                            strncat(color_buf, " and ", sizeof(color_buf));
                            strncat(color_buf, obj_color_flag[j].name, sizeof(color_buf));
                        } else {
                            strcpy(color_buf, obj_color_flag[j].name);
                        }
                    }
                }
                sprintf(buf, "You could make %s%s %s from that.\n\r",
                        indefinite_article(color_buf), color_buf, make_item_list[i].name);
            }

            send_to_char(buf, ch);
            ncount++;
            num_colors = 0;
        }
    }

    if (ncount == 0) {
        send_to_char("You don't think you could craft that into anything.\n\r", ch);
    }

    return (1);

}


int
find_item_trying_to_craft(CHAR_DATA * ch, OBJ_DATA **obj_vec, const char *into, size_t *matches)
{
    int i, j, first_match = -1;
    char new_into[512];

    strcpy(new_into, smash_article(into));

    //    gamelog (new_into);
    into = new_into;
    *matches = 0;
    for (i = 0; make_item_list[i].skill != SPELL_NONE; i++) {
        if ((ch->skills[make_item_list[i].skill])
            && (ch->skills[make_item_list[i].skill]->learned >= make_item_list[i].percentage_min)
            && ((make_item_list[i].only_tribe == TRIBE_ANY)
                || (IS_TRIBE(ch, make_item_list[i].only_tribe))))
        {
            int fobj[CRAFT_GROUP_SIZE];
            int tobj[CRAFT_GROUP_SIZE];

            for (j = 0; j < CRAFT_GROUP_SIZE; j++) {
                fobj[j] = obj_vec[j] ? obj_index[obj_vec[j]->nr].vnum : 0;
                tobj[j] = make_item_list[i].from[j];
            }
            qsort(fobj, CRAFT_GROUP_SIZE, sizeof(int), intcmp);
            qsort(tobj, CRAFT_GROUP_SIZE, sizeof(int), intcmp);

            if (!memcmp(fobj, tobj, sizeof(fobj)) && strstr(make_item_list[i].name, into)) {
                if (first_match == -1)
                    first_match = i;
                (*matches)++;
            }
        }
    }

    return first_match;
}


int
ness_analyze_from_crafted_item(OBJ_DATA * source_obj, CHAR_DATA * ch)
{
    OBJ_DATA *tmp_obj;
    char tmp[512];
    char output[512];
    int i;
    /*  int j; */
    int f0 = 0;
    int f1 = 0;
    int f2 = 0;
    int f3 = 0;
    int f4 = 0;


    strcpy(output, "");

    for (i = 0; make_item_list[i].skill != SPELL_NONE; i++) {


        if ((obj_index[source_obj->nr].vnum == make_item_list[i].into_success[0])
            || (obj_index[source_obj->nr].vnum == make_item_list[i].into_success[1])
            || (obj_index[source_obj->nr].vnum == make_item_list[i].into_success[2])
            || (obj_index[source_obj->nr].vnum == make_item_list[i].into_success[3])
            || (obj_index[source_obj->nr].vnum == make_item_list[i].into_success[4])) {
            f0 = make_item_list[i].from[0];
            f1 = make_item_list[i].from[1];
            f2 = make_item_list[i].from[2];
            f3 = make_item_list[i].from[3];
            f4 = make_item_list[i].from[4];

            if (!(has_skill(ch, make_item_list[i].skill))) {
              if (!IS_IMMORTAL(ch)) {
                send_to_char("You cannot tell how that is made.\n\r", ch);
                return TRUE;
              }
            }

            if ((make_item_list[i].only_tribe > 0)
                && (!(IS_TRIBE(ch, make_item_list[i].only_tribe)))) {
              if (!IS_IMMORTAL(ch)) {
                send_to_char ("The style is unfamiliar to you, and you're not sure how to replicate it.\n\r", ch);
                return TRUE;
              }
            }
            
            if (make_item_list[i].skill > 0 &&
                make_item_list[i].skill < MAX_SKILLS &&
                has_skill(ch, make_item_list[i].skill) &&
                ch->skills[make_item_list[i].skill]->learned < make_item_list[i].percentage_min) {
              if (!IS_IMMORTAL(ch)) {
                send_to_char("You have an idea how that is made, but you're not sure.\n\r", ch);
                return TRUE;
              }
            }

            if (f0) {
                tmp_obj = read_object(f0, VIRTUAL);
                if (tmp_obj) {
                  if (IS_IMMORTAL(ch)) {
                    sprintf(tmp, "That appears to be made from %s (%d)", OSTR(tmp_obj, short_descr), obj_index[tmp_obj->nr].vnum);
                  } else {
                    sprintf(tmp, "That appears to be made from %s", OSTR(tmp_obj, short_descr));
                  }
                  strcat(output, tmp);
                  extract_obj(tmp_obj);
                }
            }

            if (f1) {
                tmp_obj = read_object(f1, VIRTUAL);
                if (tmp_obj) {
                  if (IS_IMMORTAL(ch)) {
                    sprintf(tmp, " and %s (%d)", OSTR(tmp_obj, short_descr), obj_index[tmp_obj->nr].vnum);
                  } else {
                    sprintf(tmp, " and %s", OSTR(tmp_obj, short_descr));
                  }
                  strcat(output, tmp);
                  extract_obj(tmp_obj);
                }
            }

            if (f2) {
                tmp_obj = read_object(f2, VIRTUAL);
                if (tmp_obj) {
                  if (IS_IMMORTAL(ch)) {
                    sprintf(tmp, " and %s (%d)", OSTR(tmp_obj, short_descr), obj_index[tmp_obj->nr].vnum);
                  } else {
                    sprintf(tmp, " and %s", OSTR(tmp_obj, short_descr));
                  }
                  strcat(output, tmp);
                  extract_obj(tmp_obj);
                }
            }

            if (f3) {
                tmp_obj = read_object(f3, VIRTUAL);
                if (tmp_obj) {
                  if (IS_IMMORTAL(ch)) {
                    sprintf(tmp, " and %s (%d)", OSTR(tmp_obj, short_descr), obj_index[tmp_obj->nr].vnum);
                  } else {
                    sprintf(tmp, " and %s", OSTR(tmp_obj, short_descr));
                  }
                  strcat(output, tmp);
                  extract_obj(tmp_obj);
                }
            }

            if (f4) {
                tmp_obj = read_object(f4, VIRTUAL);
                if (tmp_obj) {
                  if (IS_IMMORTAL(ch)) {
                    sprintf(tmp, " and %s (%d)", OSTR(tmp_obj, short_descr), obj_index[tmp_obj->nr].vnum);
                  } else {
                    sprintf(tmp, " and %s", OSTR(tmp_obj, short_descr));
                  }
                  strcat(output, tmp);
                  extract_obj(tmp_obj);
                }
            }

            strcat(output, ".\n\r");
            
            if (make_item_list[i].require_fire == TRUE) {
              if (IS_IMMORTAL(ch)) {
                sprintf(tmp, "Requires a campfire item in the room.\n\r");
              } else {
                sprintf(tmp, "You would need a fire to cook that.\n\r");
              }
              strcat(output, tmp);
            }
            
            if (make_item_list[i].tool_type != -1) {
              if (IS_IMMORTAL(ch)) {
                sprintf(tmp, "Requires a tool_type '%s' at quality %d.\n\r", tool_name[make_item_list[i].tool_type], make_item_list[i].tool_quality);
              } else {
                sprintf(tmp, "You would need a %s tool to make that.\n\r", tool_name[make_item_list[i].tool_type]);
              }
              strcat(output, tmp);
            }
            
            send_to_char(output, ch);
        }
        strcpy(output, "");
    }
    return TRUE;
}

