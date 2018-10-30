/*
 * File: LIGHT.C
 * Usage: Commands for lighting/extinguishing lights
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
 * 03/22/2002 -  Created -morgenes
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>

#include "constants.h"
#include "structs.h"
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
#include "creation.h"
#include "info.h"

extern int set_fire_tracks_in_room(OBJ_DATA * obj, ROOM_DATA * room);


/* increase the light in the room by the given amount
 * handles given appropriate echoes for the room getting light
 */
void
increase_room_light(ROOM_DATA * room, int amount, int color)
{
    char buf[MAX_STRING_LENGTH];
    /* keep track of if the room was dark before */
    bool was_dark = IS_DARK(room);

    /* increase the room's light by amount */
    room->light += amount;

    /* if there's people in the room and it was dark and now isn't */
    if (room->people && was_dark && !IS_DARK(room)) {
        /* if color isn't specified... */
        if (color == 0) {
            act("The area is filled with light.", FALSE, room->people, 0, 0, TO_CHAR);
            act("The area is filled with light.", FALSE, room->people, 0, 0, TO_ROOM);
        } else {                /* specific light */
            sprintf(buf, "The area is filled with %s%s light.",
                    indefinite_article(light_color[color].name), light_color[color].name);
            act(buf, FALSE, room->people, 0, 0, TO_CHAR);
            act(buf, FALSE, room->people, 0, 0, TO_ROOM);
        }
    }
}


/* decrease the light in the room by the given amount
 * handles given appropriate echoes for the room getting dark
 */
void
decrease_room_light(ROOM_DATA * room, int amount)
{
    /* remember if the room was dark before */
    bool was_dark = IS_DARK(room);

    /* decrease the light in the room by amount */
    room->light -= amount;

    /* keep light from going below 0 */
    room->light = MAX(0, room->light);

    /* if there's people in the room and it wasn't dark, but is now */
    if (room->people && !was_dark && IS_DARK(room)) {
        act("The area is enveloped in darkness.", FALSE, room->people, NULL, NULL, TO_CHAR);
        act("The area is enveloped in darkness.", FALSE, room->people, NULL, NULL, TO_ROOM);
    }
}                               /* end decrease_room_light */


/* increase the light in the room by 1 */
void
increment_room_light(ROOM_DATA * room, int color)
{
    increase_room_light(room, 1, color);
}                               /* end increment_room_light */

/* decrease the light in the room by 1 */
void
decrement_room_light(ROOM_DATA * room)
{
    decrease_room_light(room, 1);
}                               /* end decrement_room_light */

/* determine how much throwing obj onto a fire will increase the fire's length
 * (mainly for bon-fires)
 */
int
light_value(OBJ_DATA * obj)
{
    /* keep track of the points, base of zero (no contribution) */
    int points = 0;

    switch (obj->obj_flags.material) {
        /* materials that can add to the fire's length... */
    case MATERIAL_WOOD:
        points = obj->hit_points;
        break;
    case MATERIAL_LEATHER:
    case MATERIAL_SKIN:
    case MATERIAL_TISSUE:
        points = obj->hit_points / 2;
        break;
    case MATERIAL_CLOTH:
        points = obj->hit_points / 3;
        break;
    case MATERIAL_SILK:
        points = obj->hit_points / 4;
        break;
    default:
        break;
    }

    /* return the result */
    return (points);
}                               /* end light_value */

/* put an object in a fire */
void
obj_to_fire(OBJ_DATA * obj, OBJ_DATA * fire)
{
    int points = light_value(obj);

    // increase the fire's duration by the light value of the object 
    fire->obj_flags.value[0] += points;

    // set the max hours to either the max of current duration or maxduration
    fire->obj_flags.value[1] = MAX(fire->obj_flags.value[1],
                                   fire->obj_flags.value[0]);

    // if the fire is lit, set the object as burned 
    if (IS_LIT(fire)) {
        MUD_SET_BIT(obj->obj_flags.state_flags, OST_BURNED);
    }
}                               /* end obj_to_fire */

/* get an object from the fire */
void
obj_from_fire(OBJ_DATA * obj, OBJ_DATA * fire)
{
    int points = light_value(obj);

    if (points > 0) {
        // subtract the obj remaining value
        fire->obj_flags.value[0] -= points - 1;

        // call update light for the last point
        update_light(fire);
        fire->obj_flags.value[0] = MAX(0, fire->obj_flags.value[0]);
        fire->obj_flags.value[0] = MAX(0, fire->obj_flags.value[0] - points);
    }
}                               /* end obj_from_fire */


/* command to handle lighting an object (torch, lantern, fire, whatever) */
void
cmd_light(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int sub_type;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *temp_char;
    OBJ_DATA *obj;

    /* pull off two arguments */
    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    /* error if they didn't specify what to light */
    if (arg1[0] == '\0') {
        send_to_char("What do you want to light?\n\r", ch);
        return;
    }

    /* look for arg1 as an object in the room, or in their equipment or
     * in the room on an object */
    sub_type =
        generic_find(arg1, FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_ROOM_ON_OBJ, ch, &temp_char,
                     &obj);

    /* if we didn't find the object, give an error message */
    if (!sub_type || !obj) {
        sprintf(buf, "You can't find a '%s' here to light.\n\r", arg1);
        send_to_char(buf, ch);
        return;
    }

    /* if it's not a light type object, error out */
    if (GET_ITEM_TYPE(obj) != ITEM_LIGHT) {
        cprintf(ch, "You can't light %s.\n\r", format_obj_to_char(obj, ch, 1));
        return;
    }

    /* if it's already lit, error out */
    if (IS_LIT(obj)) {
        send_to_char("It's lit already!\n\r", ch);
        return;
    }

    // One is ethereal and the other is not
    if ((!IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL)
         && IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))
        || (IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL)
         && !IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))) {
        cprintf(ch, "You can't light %s.\n\r", format_obj_to_char(obj, ch, 1));
        return;
    }

    /* campfires can't be held and lit */
    if (IS_CAMPFIRE(obj) && (obj->carried_by || obj->equipped_by)) {
        act("$p can't be lit while being carried, drop it first.", FALSE, ch, obj, NULL, TO_CHAR);
        return;
    }

    /* light the object */
    light_object(ch, obj, arg2);
}                               /* end cmd_light */

/*
 * Light a light, increasing the light in the room, echoes
 * appropriately if the room is suddenly lit.
 * Does a sanity check to make sure the obj is a light and is lit
 */
void
light_a_light(OBJ_DATA * obj, ROOM_DATA * rm)
{
    /* sanity check: it's a light & it's not lit */
    if (GET_ITEM_TYPE(obj) != ITEM_LIGHT || IS_LIT(obj))
        return;

    /* set the 'lit' bit */
    MUD_SET_BIT(obj->obj_flags.value[5], LIGHT_FLAG_LIT);

    /* increase the light in the room */
    increment_room_light(rm, obj->obj_flags.value[4]);

    /* if the object is in the room, add tracks */
    if (obj->in_room)
        set_fire_tracks_in_room(obj, obj->in_room);
}                               /* end light_a_light */

/* light an object */
void
light_object(CHAR_DATA * ch, OBJ_DATA * obj, char *arg)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *source = NULL;
    CHAR_DATA *temp_char;
    int sub_type;
    OBJ_DATA *my_obj, *other_obj;

    /* if the object is already lit, return */
    if (IS_LIT(obj)) {
        return;
    }

    /* campfires can't be held and lit */
    if (IS_CAMPFIRE(obj) && obj->carried_by) {
        return;
    }

    /* if it's completely spent */
    if (obj->obj_flags.value[0] == 0) {
        /* look at the type of light it is */
        switch (obj->obj_flags.value[2]) {
            /* handle flame types (candles, lamps, lanterns, campfires) */
        case LIGHT_TYPE_FLAME:
            /* if it's a candle... */
            if (IS_CANDLE(obj))
                act("$p's nothing but a nub, completely unusable.", FALSE, ch, obj, NULL, TO_CHAR);
            /* if a lamp or lantern... */
            else if (IS_LANTERN(obj) || IS_LAMP(obj))
                act("$p appears to be out of fuel.", FALSE, ch, obj, NULL, TO_CHAR);
            /* if a campfire... */
            else if (IS_CAMPFIRE(obj))
                send_to_char("All the wood is spent.", ch);
            else                /* catch everything else */
                act("$p appears to be spent.", FALSE, ch, obj, NULL, TO_CHAR);
            break;
            /* all other sources of light (magic...whatever) */
        default:
            act("$p appears to be spent.", FALSE, ch, obj, NULL, TO_CHAR);
            break;
        }
        return;
    }

    /* if they specified what to light it off of */
    if (arg != NULL && arg[0] != '\0') {
        /* look for the object to light it off of */
        sub_type =
            generic_find(arg, FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV, ch, &temp_char,
                         &source);

        /* if we didn't find it, error out */
        if (source == NULL) {
            sprintf(buf, "You can't find a '%s' here to light it with.\n\r", arg);
            send_to_char(buf, ch);
            return;
        }
    }

    /* if we got to this point and no source to light off */
    if (source == NULL) {
        act("You light $p.", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n lights $p.", FALSE, ch, obj, NULL, TO_ROOM);
        light_a_light(obj, ch->in_room);
        return;
    }

    if (obj->equipped_by == ch) {
        my_obj = obj;
        other_obj = source;
    } else {
        my_obj = source;
        other_obj = obj;
    }

    /* look at the type of the source object */
    switch (GET_ITEM_TYPE(source)) {
    case ITEM_LIGHT:
        /* look at the type of the object to be lit */
        switch (obj->obj_flags.value[2]) {
        case LIGHT_TYPE_FLAME:
            /* if the object to be lit is a torch, candle or lamp */
            if (IS_TORCH(obj) || IS_CANDLE(obj) || IS_LAMP(obj)) {
                /* look at the type of the source object */
                switch (source->obj_flags.value[2]) {
                case LIGHT_TYPE_MAGICK:
                    /* if it's magick and not any other type */
                    if (!IS_TORCH(source) && !IS_CANDLE(source)
                        && !IS_LAMP(source) && !IS_LANTERN(source)
                        && !IS_CAMPFIRE(source)) {
                        /* can't light it off it */
                        act("You hold $p to $P, but nothing happens.", FALSE, ch, my_obj, other_obj,
                            TO_CHAR);
                        act("$n holds $p to $P.", FALSE, ch, my_obj, other_obj, TO_ROOM);
                        return;
                    }
                    /* no break intentionally */
                case LIGHT_TYPE_FLAME:
                    /* flame to flame, light'er up! */
                    act("You hold $p to $P and flames burst forth from it.", FALSE, ch, my_obj,
                        other_obj, TO_CHAR);
                    act("$n holds $p to $P and flames burst forth from it.", FALSE, ch, my_obj,
                        other_obj, TO_ROOM);
                    light_a_light(obj, ch->in_room);
                    return;

                    /* some other type of light? */
                default:
                    act("You hold $p to $P, but nothing happens.", FALSE, ch, my_obj, other_obj,
                        TO_CHAR);
                    act("$n holds $p to $P.", FALSE, ch, my_obj, other_obj, TO_ROOM);
                    return;
                }
            }
            /* otherwise, if it's a lantern */
            else if (IS_LANTERN(obj)) {
                /* look at the source object's type of light */
                switch (source->obj_flags.value[2]) {
                    /* magick... */
                case LIGHT_TYPE_MAGICK:
                    /* if the source is not a normal source of light */
                    if (!IS_TORCH(source) && !IS_CANDLE(source)
                        && !IS_LAMP(source) && !IS_LANTERN(source)
                        && !IS_CAMPFIRE(source)) {
                        act("You hold $p to $P, but nothing happens.", FALSE, ch, my_obj, other_obj,
                            TO_CHAR);
                        act("$n holds $p to $P.", FALSE, ch, my_obj, other_obj, TO_ROOM);
                        return;
                    }
                    /* no break intentionally */
                    /* handle flame (and other magick types) */
                case LIGHT_TYPE_FLAME:
                    /* if source is a lamp, campfire or lantern */
                    if (IS_LAMP(source) || IS_CAMPFIRE(source)
                        || IS_LANTERN(source)) {
                        act("You can't get $p's flame close enough to" " light it.", FALSE, ch,
                            source, NULL, TO_CHAR);
                        return;
                    } else {
                        /* success! */
                        act("You hold $p to $P's wick and flames burst" " forth from it.", FALSE,
                            ch, source, obj, TO_CHAR);
                        act("$n holds $p to $P's wick and flames burst" " forth from it.", FALSE,
                            ch, source, obj, TO_ROOM);
                        light_a_light(obj, ch->in_room);
                        return;
                    }
                    break;

                    /* other type of light */
                default:
                    act("You hold $p to $P, but nothing happens.", FALSE, ch, my_obj, other_obj,
                        TO_CHAR);
                    act("$n holds $p to $P.", FALSE, ch, my_obj, other_obj, TO_ROOM);
                    return;
                }               /* end looking at source's light type */
            }
            /* end if it's a lantern */
            /* if it's a campfire */
            else if (IS_CAMPFIRE(obj)) {
                /* look at the source object type */
                switch (source->obj_flags.value[2]) {
                case LIGHT_TYPE_MAGICK:
                    if (!IS_TORCH(source) && !IS_CANDLE(source)
                        && !IS_LAMP(source) && !IS_LANTERN(source)
                        && !IS_CAMPFIRE(source)) {
                        act("You hold $p to $P, but nothing happens.", FALSE, ch, my_obj, other_obj,
                            TO_CHAR);
                        act("$n holds $p to $P.", FALSE, ch, my_obj, other_obj, TO_ROOM);
                        return;
                    }
                    /* intentionally no break here */
                case LIGHT_TYPE_FLAME:
                    if (IS_LANTERN(source) || IS_CAMPFIRE(source)) {
                        act("You can't get $p's flame close enough to" " light it.", FALSE, ch,
                            source, NULL, TO_CHAR);
                        return;
                    } else {
                        act("You hold $p to $P and flames burst forth from it.", FALSE, ch, source,
                            obj, TO_CHAR);
                        act("$n holds $p to $P and flames burst forth from it.", FALSE, ch, source,
                            obj, TO_ROOM);
                        light_a_light(obj, ch->in_room);
                        return;
                    }
                default:
                    act("You hold $p to $P, but nothing happens.", FALSE, ch, source, obj, TO_CHAR);
                    act("$n holds $p to $P.", FALSE, ch, source, obj, TO_ROOM);
                    return;
                }
            }
            /* end if campfire */
            /* else generic flame */
            else {
                /* look at source's light type */
                switch (source->obj_flags.value[2]) {
                case LIGHT_TYPE_MAGICK:
                    if (!IS_TORCH(source) && !IS_CANDLE(source)
                        && !IS_LAMP(source) && !IS_LANTERN(source)
                        && !IS_CAMPFIRE(source)) {
                        act("You hold $p to $P, but nothing happens.", FALSE, ch, my_obj, other_obj,
                            TO_CHAR);
                        act("$n holds $p to $P.", FALSE, ch, my_obj, other_obj, TO_ROOM);
                        return;
                    }
                    /* intentionally missing break */
                case LIGHT_TYPE_FLAME:
                    act("You hold $p to $P and flames burst forth from it.", FALSE, ch, my_obj,
                        other_obj, TO_CHAR);
                    act("$n holds $p to $P and flames burst forth from it.", FALSE, ch, my_obj,
                        other_obj, TO_ROOM);
                    light_a_light(obj, ch->in_room);
                    return;

                default:
                    act("You hold $p to $P, but nothing happens.", FALSE, ch, my_obj, other_obj,
                        TO_CHAR);
                    act("$n holds $p to $P.", FALSE, ch, my_obj, other_obj, TO_ROOM);
                    return;
                }
            }
            break;
        default:
            send_to_char("You can't light it with that.\n\r", ch);
            return;
        }                       // end looking at obj's light type
    default:
        send_to_char("You can't light it with that.\n\r", ch);
        return;
    }                           // end looking at source's item type

    return;
}                               // end light_object()


/* Handle extinguisihing a light object */
void
cmd_extinguish(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *temp_char;
    int sub_type;

    argument = one_argument(argument, arg, sizeof(arg));

    /* if they didn't specify what to extinguish */
    if (arg[0] == '\0') {
        /* if they don't have a light object held */
        if ((obj = get_item_held(ch, ITEM_LIGHT)) == NULL) {
            /* error out */
            send_to_char("What do you want to extinguish?\n\r", ch);
            return;
        }
    } else {                    /* they specified what to extinguish */
        /* look for the object to extinguish */
        sub_type =
            generic_find(arg, FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM_ON_OBJ,
                         ch, &temp_char, &obj);
    }

    /* if we didn't find an object, error out */
    if (obj == NULL) {
        sprintf(buf, "You can't find any '%s' to extinguish.\n\r", arg);
        send_to_char(buf, ch);
        return;
    }

    /* if it's not lit, error out */
    if (!IS_LIT(obj)) {
        send_to_char("It's extinguished already!\n\r", ch);
        return;
    }

    // One is ethereal and the other is not
    if ((!IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL)
         && IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))
        || (IS_SET(obj->obj_flags.extra_flags, OFL_ETHEREAL)
         && !IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))) {
        cprintf(ch, "You can't extinguish %s.\n\r", 
         format_obj_to_char(obj, ch, 1));
        return;
    }

    /* finish up, actually extinguish the object */
    extinguish_object(ch, obj);
    return;
}                               /* end cmd_extinguish */

/* handle extinguishing the light object */
OBJ_DATA *
extinguish_object(CHAR_DATA * ch, OBJ_DATA * obj)
{
    /* sanity check: if it isn't lit, return */
    if (!IS_LIT(obj))
        return (obj);

    /* look at the type of light */
    switch (obj->obj_flags.value[2]) {
        /* flames and otherwise... */
    default:
    case LIGHT_TYPE_FLAME:
        /* if it's a candle */
        if (IS_CANDLE(obj)) {
            /* 1 in 5 chance of pinching it out */
            if (number(0, 4) == 0) {
                act("You pinch out $p.", FALSE, ch, obj, NULL, TO_CHAR);
                act("$n pinches $p's flame, extinguishing it.", FALSE, ch, obj, NULL, TO_ROOM);
            } else {
                act("You blow out $p.", FALSE, ch, obj, NULL, TO_CHAR);
                act("$n blows on $p's flame, extinguishing it.", FALSE, ch, obj, NULL, TO_ROOM);
            }
        }
        /* else if it's a lantern */
        else if (IS_LANTERN(obj)) {
            act("You lower the wick of $p, putting it out.", FALSE, ch, obj, NULL, TO_CHAR);
            act("$n lowers the wick of $p, putting it out.", FALSE, ch, obj, NULL, TO_ROOM);
        }
        /* else if it's a lamp */
        else if (IS_LAMP(obj)) {
            act("You snuff $p.", FALSE, ch, obj, NULL, TO_CHAR);
            act("$n snuffs $p.", FALSE, ch, obj, NULL, TO_ROOM);
        }
        /* else give a generic message */
        else {
            act("You extinguish $p.", FALSE, ch, obj, NULL, TO_CHAR);
            act("$n extinguishes $p.", FALSE, ch, obj, NULL, TO_ROOM);
        }
        break;
    }

    char extmsg_char[512] = "", extmsg_room[512] = "";
    int charmsg = 0, roommsg = 0;
    if (get_obj_extra_desc_value(obj, "[LIGHT_BURNOUT_MSG_CHAR]", extmsg_char, sizeof(extmsg_char))) {
        charmsg = 1;
    }
    if (get_obj_extra_desc_value(obj, "[LIGHT_BURNOUT_MSG_ROOM]", extmsg_room, sizeof(extmsg_room))) {
        roommsg = 1;
    }
    OBJ_DATA *retval = extinguish_light(obj, ch->in_room);
    if (!retval) {
        if (charmsg)
            act(extmsg_char, FALSE, ch, obj, NULL, TO_CHAR);
	else
            cprintf(ch, "There is nothing left now.\n\r");
        if (roommsg)
            act(extmsg_room, FALSE, ch, obj, NULL, TO_CHAR);
    }

    return retval;
}                               /* end extinguish_object */


/*
 * Put out a light, decreasing the light in the room, echoes
 * appropriately if the room is suddenly dark.
 * Does a sanity check to make sure the obj is a light and is lit
 */
OBJ_DATA *
extinguish_light(OBJ_DATA * obj, ROOM_DATA * rm)
{
    /* sanity check: if it isn't a light or isn't lit, exit */
    if (GET_ITEM_TYPE(obj) != ITEM_LIGHT || !IS_LIT(obj))
        return (obj);

    /* remove the 'lit' flag */
    REMOVE_BIT(obj->obj_flags.value[5], LIGHT_FLAG_LIT);

    /* if being lit consumes the object */
    if (LIGHT_CONSUMES(obj)) {
        /* extract it and set our pointer to null */
        extract_obj(obj);
        obj = NULL;
    }

    /* handle decrementing the room's light */
    decrement_room_light(rm);

    return (obj);
}                               /* end extinguish_light() */


/*
 * for when the light was hastily dropped 
 * Does a sanity check to make sure the obj is a light and is lit
 */
OBJ_DATA *
drop_light(OBJ_DATA * obj, ROOM_DATA * room)
{
    /* sanity check: if it's not a light or not lit, exit */
    if (GET_ITEM_TYPE(obj) != ITEM_LIGHT || !IS_LIT(obj))
        return (obj);

    /* look at the type of light, and its flags,  */
    switch (obj->obj_flags.value[2]) {  /* light_type */
        /* if it's a flame */
    case LIGHT_TYPE_FLAME:
        if (IS_TORCH(obj)) {
            /* allow the light to light the room */
            return (obj);
        } else if (IS_LANTERN(obj) || IS_LAMP(obj)) {
            /* 50% it stays lit */
            if (number(0, 9) > 5)
                return (obj);
        } else if (IS_CANDLE(obj)) {
            /* 1 in 10 chance it stays lit */
            if (number(0, 9) == 0)
                return (obj);
        }

        break;
    default:                   /* always goes out */
        break;
    }

    /* if there is people in the room */
    if (room->people) {
        act("$p goes out.", FALSE, room->people, obj, NULL, TO_CHAR);
        act("$p goes out.", FALSE, room->people, obj, NULL, TO_ROOM);
    }

    /* extenguish the light */
    obj = extinguish_light(obj, room);
    return (obj);
}                               /* end drop_light */

/* handle removing lights from the room that the given character is wearing */
void
ch_lights_from_room(CHAR_DATA * ch)
{
    /* keep track of the room to check */
    ROOM_DATA *rm = ch->in_room;
    OBJ_DATA *obj;
    int i;
    /* keep track of how many lights need to be removed */
    int light_count = 0;

    /* iterate over the wear positions */
    for (i = 0; i < MAX_WEAR; i++) {
        /* if there's a light object in this location, and it's lit */
        if ((obj = ch->equipment[i]) != NULL && GET_ITEM_TYPE(obj) == ITEM_LIGHT && IS_LIT(obj)) {
            /* increment the number of lights to remove */
            light_count++;
        }
    }

    /* drop the light in the room by 'light_count' */
    decrease_room_light(rm, light_count);
}                               /* end ch_lights_from_room */

/* handle adding lights to the room that the given character is wearing */
void
ch_lights_to_room(CHAR_DATA * ch)
{
    /* keep track of the room the character is in */
    ROOM_DATA *rm = ch->in_room;
    OBJ_DATA *obj;
    int i;
    /* track the color of light */
    int color = -1;
    /* track the amount of light */
    int light_count = 0;

    /* iterate over the wear locations */
    for (i = 0; i < MAX_WEAR; i++) {
        /* if there's a light object in this location, and it's lit */
        if ((obj = ch->equipment[i]) != NULL && GET_ITEM_TYPE(obj) == ITEM_LIGHT && IS_LIT(obj)) {
            /* if we have a light color */
            if (obj->obj_flags.value[4] != 0) {
                /* if more than one color of light */
                if (color != -1 && color != obj->obj_flags.value[4])
                    color = 0;
                else            /* save the new color */
                    color = obj->obj_flags.value[4];
            }
            light_count++;
        }
    }

    /* if we have more than one light */
    if (light_count > 0) {
        /* add the light to the room */
        increase_room_light(rm, light_count, color);
    }
}                               /* end ch_lights_to_room */

/* update the lights that a character is wearing */
void
update_char_lights(CHAR_DATA * ch)
{
    OBJ_DATA *obj;
    int i;

    /* iterate over the wear locations */
    for (i = 0; i < MAX_WEAR; i++) {
        /* if there is a light object at this location, and it's lit */
        if ((obj = ch->equipment[i]) != NULL && GET_ITEM_TYPE(obj) == ITEM_LIGHT) {
            /* update the light */
            update_light(obj);
        }
    }
}                               /* end update_char_lights */

/* update a light */
OBJ_DATA *
update_light(OBJ_DATA * obj)
{
    /* see if the object is equipped */
    CHAR_DATA *ch = obj->equipped_by;

    /* see if the object is on an object */
    OBJ_DATA *on_obj = obj->in_obj;

    /* figure out what room it's in */
    ROOM_DATA *room =
        (ch != NULL ? ch->in_room : obj->in_room != NULL ? obj->in_room : on_obj !=
         NULL ? on_obj->in_room : NULL);

    /* find a character in the room */
    CHAR_DATA *rch = (room != NULL ? room->people : NULL);

    /* if it's a light object and it's lit */
    if (GET_ITEM_TYPE(obj) == ITEM_LIGHT && IS_LIT(obj)) {
        /* if it's a campfire */
        if (IS_CAMPFIRE(obj)) {
            OBJ_DATA *content, *next_content;

            /* iterate over the contents of the campfire */
            for (content = obj->contains; content; content = next_content) {
                next_content = content->next_content;

                /* look at the material type of the content and decrease it's
                 * hitpoints as appropriate
                 */
                switch (content->obj_flags.material) {
		case MATERIAL_DUNG:
		case MATERIAL_PLANT:
		case MATERIAL_FUNGUS:
                case MATERIAL_WOOD:
                    content->hit_points--;
                    break;
		case MATERIAL_FEATHER:
		case MATERIAL_GRAFT:
		case MATERIAL_PAPER:
		case MATERIAL_FOOD:
                case MATERIAL_SKIN:
                case MATERIAL_TISSUE:
                case MATERIAL_SILK:
                    content->hit_points -= 5;
                    break;
                case MATERIAL_CLOTH:
                    content->hit_points -= 4;
                    break;
                case MATERIAL_LEATHER:
                    content->hit_points -= 2;
                    break;

                default:
                    /* look at the item type of the content and decrease its
                     * hitpoints as appropriate
                     */
		    switch (GET_ITEM_TYPE(content)) {
		    case ITEM_HERB:
		    case ITEM_FOOD:
                        content->hit_points -= 5;
                        break;
                    default:
                        break;
		    }
                    break;
                }


                /* if it has run out of hitpoints */
                if (content->hit_points <= 0) {
                    OBJ_DATA *in_content, *next_in_content;

                    /* look at the type of this content */
                    switch (GET_ITEM_TYPE(content)) {
                        /* handle containers spilling out */
                    case ITEM_CONTAINER:
                        /* if it has stuff in it */
                        if (content->contains) {
                            act("$p is consumed in the $O, spilling its" " contents.", FALSE, rch,
                                content, obj, TO_ROOM);
                            act("$p is consumed in the $O, spilling its" " contents.", FALSE, rch,
                                content, obj, TO_CHAR);

                            /* iterate over the contents of the container */
                            for (in_content = content->contains; in_content;
                                 in_content = next_in_content) {
                                // keep track of what the next content would be 
                                next_in_content = in_content->next_content;

                                // remove the object from the container
                                obj_from_obj(in_content);
                                // put it in the campfire
                                obj_to_obj(in_content, obj);
                                // handle setting it ablaze
                                obj_to_fire(in_content, obj);
                            }
                        } else {        /* default message */
                            act("$p is consumed in the $O.", FALSE, rch, content, obj, TO_ROOM);
                            act("$p is consumed in the $O.", FALSE, rch, content, obj, TO_CHAR);
                        }
                        break;
                        /* handle furniture objects */
                    case ITEM_FURNITURE:
                        {
                            /* if it has contents on it */
                            if (content->contains) {
                                /* if it can be put on */
                                if (IS_SET(content->obj_flags.value[1], FURN_PUT)) {
                                    act("$p is consumed in the $O, spilling its" " contents.",
                                        FALSE, rch, content, obj, TO_ROOM);
                                    act("$p is consumed in the $O, spilling its" " contents.",
                                        FALSE, rch, content, obj, TO_CHAR);
                                } else {
                                    act("$p is consumed in the $O.", FALSE, rch, content, obj,
                                        TO_ROOM);
                                    act("$p is consumed in the $O.", FALSE, rch, content, obj,
                                        TO_CHAR);
                                }

                                /* iterate over the contents */
                                for (in_content = content->contains; in_content;
                                     in_content = next_in_content) {
                                    // keep track of the next content
                                    next_in_content = in_content->next_content;
                                    // pull the object from the furniture
                                    obj_from_obj(in_content);
                                    // put it in the campfire
                                    obj_to_obj(in_content, obj);
                                    // set it ablaze
                                    obj_to_fire(in_content, obj);
                                }
                            } else {    /* default message */
                                act("$p is consumed in the $O.", FALSE, rch, content, obj, TO_ROOM);
                                act("$p is consumed in the $O.", FALSE, rch, content, obj, TO_CHAR);
                            }
                            break;
                        }       /* end ITEM_FURNITURE */

                        /* handle drink containers */
                    case ITEM_DRINKCON:
                        /* if it has liquid in it */
                        if (content->obj_flags.value[1] > 0) {
                            char buf[MAX_STRING_LENGTH];
                            /* track the color of the liquid */
                            int tempcolor = content->obj_flags.value[2];
                            /* multiply amount by alcohol content */
                            int amount = content->obj_flags.value[1]
                                * drink_aff[content->obj_flags.value[2]][3];

                            tempcolor = MAX(0, MIN((MAX_LIQ - 1), tempcolor));

                            /* prepare a message */
                            sprintf(buf, "$p is consumed in the $O, releasing %s%s liquid.",
                                    indefinite_article(color_liquid[tempcolor]), color_liquid[tempcolor]);
                            act(buf, FALSE, rch, content, obj, TO_CHAR);
                            act(buf, FALSE, rch, content, obj, TO_ROOM);

                            // if it's not alcoholic, compare against the fire
                            if (amount <= 0 && abs(amount) >= obj->obj_flags.value[0]) {
                                /* it goes out */
                                act("$p sizzles and smokes as it goes out.", FALSE, rch, obj, NULL,
                                    TO_CHAR);
                                act("$p sizzles and smokes as it goes out.", FALSE, rch, obj, NULL,
                                    TO_ROOM);

                                /* none left */
                                obj->obj_flags.value[0] = 0;
                                extract_obj(content);
                                /* extinguish the light */
                                return (extinguish_light(obj, room));
                            }
                            /* else if alcoholic */
                            else if (amount > 0) {
                                act("$p's flame burns brighter for a second.", FALSE, rch, obj, 0,
                                    TO_ROOM);
                                act("$p's flame burns brighter for a second.", FALSE, rch, obj, 0,
                                    TO_CHAR);
                            }
                            /* smaller amount of hours left */
                            else if (amount < 0) {
                                act("$p's flame lessens.", FALSE, rch, obj, 0, TO_ROOM);
                                act("$p's flame lessens.", FALSE, rch, obj, 0, TO_CHAR);

                                /* add amount (subtracts from hours left) */
                                obj->obj_flags.value[0] += amount;
                            }
                        } else {
                            act("$p is consumed in the $O.", FALSE, rch, content, obj, TO_ROOM);
                            act("$p is consumed in the $O.", FALSE, rch, content, obj, TO_CHAR);
                        }
                        break;
                        /* end liquid containers */

                        /* everything else */
                    default:
                        act("$p is consumed in the $O.", FALSE, rch, content, obj, TO_ROOM);
                        act("$p is consumed in the $O.", FALSE, rch, content, obj, TO_CHAR);
                        break;
                    }
                    obj_from_obj(content);
                    extract_obj(content);
                }               /* end if it is destroyed by the fire */
            }                   /* end iterating over the contents of the fire */
        }

        /* end if it's a campfire */
        /* if it's not an infinite light */
        if (obj->obj_flags.value[1] != -1 && obj->obj_flags.value[0] > 0) {
            /* decrement the remaining value */
            obj->obj_flags.value[0]--;

            /* if no room */
            if (room == NULL) {
                errorlog("room was null in update_light");
                if (obj->obj_flags.value[0] <= 0) {
                    REMOVE_BIT(obj->obj_flags.value[5], LIGHT_FLAG_LIT);

                    if (LIGHT_CONSUMES(obj)) {
                        extract_obj(obj);
                        obj = NULL;
                    }
                }
                return (obj);
            }

            /* if on something that isn't furniture or doesn't have put flag */
            if (on_obj != NULL
                && (GET_ITEM_TYPE(on_obj) != ITEM_FURNITURE
                    || (GET_ITEM_TYPE(on_obj) == ITEM_FURNITURE
                        && !IS_SET(on_obj->obj_flags.value[1], FURN_PUT)))) {
                /* if it ran out of fuel */
                if (obj->obj_flags.value[0] <= 0) {
                    REMOVE_BIT(obj->obj_flags.value[5], LIGHT_FLAG_LIT);

                    if (LIGHT_CONSUMES(obj)) {
                        extract_obj(obj);
                        obj = NULL;
                    }
                }
                return (obj);
            }

            /* if out of fuel */
            if (obj->obj_flags.value[0] <= 0) {
                if (ch) {
                    act("Your $o goes out.", FALSE, ch, obj, 0, TO_CHAR);
                    act("$n's $o goes out.", FALSE, ch, obj, 0, TO_ROOM);
                } else if (on_obj && rch) {
                    act("$p on $P goes out.", FALSE, rch, obj, on_obj, TO_CHAR);
                    act("$p on $P goes out.", FALSE, rch, obj, on_obj, TO_ROOM);
                } else if (rch) {
                    act("$p goes out.", FALSE, rch, obj, 0, TO_CHAR);
                    act("$p goes out.", FALSE, rch, obj, 0, TO_ROOM);
                }
                obj = extinguish_light(obj, room);
            }
            /* if it's very dim */
            else if (obj->obj_flags.value[0] <= 1) {
                /* look at the type of light */
                switch (obj->obj_flags.value[2]) {
                case LIGHT_TYPE_FLAME:
                    if (ch) {
                        act("Your $o flickers weakly, about to go out.", FALSE, ch, obj, 0,
                            TO_CHAR);
                        act("$n's $o flickers weakly, about to go out.", FALSE, ch, obj, 0,
                            TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P flickers weakly, about to go out.", FALSE, rch, obj, on_obj,
                            TO_CHAR);
                        act("$p on $P flickers weakly, about to go out.", FALSE, rch, obj, on_obj,
                            TO_ROOM);
                    } else if (rch) {
                        act("$p flickers weakly, about to go out.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p flickers weakly, about to go out.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                case LIGHT_TYPE_MAGICK:
                    if (ch) {
                        act("Your $o grows very dim, its energies ebbing.", FALSE, ch, obj, 0,
                            TO_CHAR);
                        act("$n's $o grows very dim, its energies ebbing.", FALSE, ch, obj, 0,
                            TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P grows very dim, its energies ebbing.", FALSE, rch, obj,
                            on_obj, TO_CHAR);
                        act("$p on $P grows very dim, its energies ebbing.", FALSE, rch, obj,
                            on_obj, TO_ROOM);
                    } else if (rch) {
                        act("$p grows very dim, its energies ebbing.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p grows very dim, its energies ebbing.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                case LIGHT_TYPE_PLANT:
                    if (ch) {
                        act("Your $o grows very dim, fading with each moment.", FALSE, ch, obj, 0,
                            TO_CHAR);
                        act("$n's $o grows very dim, fading with each moment.", FALSE, ch, obj, 0,
                            TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P grows very dim, fading with each moment.", FALSE, rch, obj,
                            on_obj, TO_CHAR);
                        act("$p on $P grows very dim, fading with each moment.", FALSE, rch, obj,
                            on_obj, TO_ROOM);
                    } else if (rch) {
                        act("$p grows very dim, fading with each moment.", FALSE, rch, obj, 0,
                            TO_CHAR);
                        act("$p grows very dim, fading with each moment.", FALSE, rch, obj, 0,
                            TO_ROOM);
                    }
                    break;
                case LIGHT_TYPE_ANIMAL:
                    if (ch) {
                        act("Your $o wavers dimly, fading with each moment.", FALSE, ch, obj, 0,
                            TO_CHAR);
                        act("$n's $o wavers dimly, fading with each moment.", FALSE, ch, obj, 0,
                            TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P wavers dimly, fading with each moment.", FALSE, rch, obj,
                            on_obj, TO_CHAR);
                        act("$p on $P wavers dimly, fading with each moment.", FALSE, rch, obj,
                            on_obj, TO_ROOM);
                    } else if (rch) {
                        act("$p wavers dimly, fading with each moment.", FALSE, rch, obj, 0,
                            TO_CHAR);
                        act("$p wavers dimly, fading with each moment.", FALSE, rch, obj, 0,
                            TO_ROOM);
                    }
                    break;
                case LIGHT_TYPE_MINERAL:
                    if (ch) {
                        act("Your $o grows faint, ebbing slowly.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n's $o grows faint, ebbing slowly.", FALSE, ch, obj, 0, TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P grows faint, ebbing slowly.", FALSE, rch, obj, on_obj,
                            TO_CHAR);
                        act("$p on $P grows faint, ebbing slowly.", FALSE, rch, obj, on_obj,
                            TO_ROOM);
                    } else if (rch) {
                        act("$p grows faint, ebbing slowly.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p grows faint, ebbing slowly.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                default:
                    if (ch) {
                        act("Your $o grows faint, ebbing slowly.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n's $o grows faint, ebbing slowly.", FALSE, ch, obj, 0, TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P grows faint, ebbing slowly.", FALSE, rch, obj, on_obj,
                            TO_CHAR);
                        act("$p on $P grows faint, ebbing slowly.", FALSE, rch, obj, on_obj,
                            TO_ROOM);
                    } else if (rch) {
                        act("$p grows faint, ebbing slowly.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p grows faint, ebbing slowly.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                }
            }
            /* end if it's very dim */
            /* if it's dim */
            else if (obj->obj_flags.value[0] <= 5) {
                /* 2 in 3 chance for no echo if not at the 5 mark */
                if (obj->obj_flags.value[0] != 5 && number(0, 2) != 0)
                    return (obj);

                /* look at the type of light */
                switch (obj->obj_flags.value[2]) {
                case LIGHT_TYPE_FLAME:
                    if (ch) {
                        act("Your $o flickers feebly.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n's $o flickers feebly.", FALSE, ch, obj, 0, TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P flickers feebly.", FALSE, rch, obj, on_obj, TO_CHAR);
                        act("$p on $P flickers feebly.", FALSE, rch, obj, on_obj, TO_ROOM);
                    } else if (rch) {
                        act("$p flickers feebly.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p flickers feebly.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                case LIGHT_TYPE_MAGICK:
                    if (ch) {
                        act("Your $o grows dim.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n's $o grows dim.", FALSE, ch, obj, 0, TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P grows dim.", FALSE, rch, obj, on_obj, TO_CHAR);
                        act("$p on $P grows dim.", FALSE, rch, obj, on_obj, TO_ROOM);
                    } else if (rch) {
                        act("$p grows dim.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p grows dim.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                case LIGHT_TYPE_PLANT:
                    if (ch) {
                        act("Your $o grows dim.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n's $o grows dim.", FALSE, ch, obj, 0, TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P grows dim.", FALSE, rch, obj, on_obj, TO_CHAR);
                        act("$p on $P grows dim.", FALSE, rch, obj, on_obj, TO_ROOM);
                    } else if (rch) {
                        act("$p grows dim.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p grows dim.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                case LIGHT_TYPE_ANIMAL:
                    if (ch) {
                        act("Your $o wavers dimly.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n's $o wavers dimly.", FALSE, ch, obj, 0, TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P wavers dimly.", FALSE, rch, obj, on_obj, TO_CHAR);
                        act("$p on $P wavers dimly.", FALSE, rch, obj, on_obj, TO_ROOM);
                    } else if (rch) {
                        act("$p wavers dimly.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p wavers dimly.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                case LIGHT_TYPE_MINERAL:
                    if (ch) {
                        act("Your $o glows faint but steady.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n's $o glows faint but steady.", FALSE, ch, obj, 0, TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P glows faint but steady.", FALSE, rch, obj, on_obj, TO_CHAR);
                        act("$p on $P glows faint but steady.", FALSE, rch, obj, on_obj, TO_ROOM);
                    } else if (rch) {
                        act("$p glows faint but steady.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p glows faint but steady.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                default:
                    if (ch) {
                        act("Your $o glows faint but steady.", FALSE, ch, obj, 0, TO_CHAR);
                        act("$n's $o glows faint but steady.", FALSE, ch, obj, 0, TO_ROOM);
                    } else if (on_obj && rch) {
                        act("$p on $P glows faint but steady.", FALSE, rch, obj, on_obj, TO_CHAR);
                        act("$p on $P glows faint but steady.", FALSE, rch, obj, on_obj, TO_ROOM);
                    } else if (rch) {
                        act("$p glows faint but steady.", FALSE, rch, obj, 0, TO_CHAR);
                        act("$p glows faint but steady.", FALSE, rch, obj, 0, TO_ROOM);
                    }
                    break;
                }               /* end examining the type of light */
            }                   /* end if dim */
        }                       /* end if it's not an infinite light */
    }
    /* end if it's a light object and lit */
    return (obj);
}                               /* end update_light */

