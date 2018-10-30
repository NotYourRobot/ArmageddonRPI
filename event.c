/*
 * File: EVENT.C
 * Usage: Event handling.
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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "constants.h"
#include "structs.h"
#include "parser.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "event.h"
#include "skills.h"
#include "handler.h"
#include "utility.h"
#include "limits.h"
#include "special.h"

char *event_names[MAX_EVENTS] = {
    "EVNT_NULL",
    "EVNT_SAVE",
    "EVNT_MOVE",
    "EVNT_NPC_PULSE",
    "EVNT_OBJ_PULSE",
    "EVNT_ROOM_PULSE",
    "EVNT_ATCK",
    "EVNT_REST",
    "EVNT_GAIN",
    "EVNT_SANDWALL_REMOVE",
    "EVNT_SHADE_REMOVE",
    "EVNT_SHUTDOWN",
    "EVNT_WEATHER",
    "EVNT_REMOVE_OBJECT",
    "EVNT_REMOVE_MOBILE",
    "EVNT_REWIND",
    "EVNT_REMOVE_ALARM",
    "EVNT_FIRE_BURN",
    "EVNT_REMOVE_DARK",
    "EVNT_REMOVE_NOELEMMAGICK",
    "EVNT_DETONATE",
    "EVNT_FIREWALL_REMOVE",
    "EVNT_WINDWALL_REMOVE",
    "EVNT_BLADEBARRIER_REMOVE",
    "EVNT_THORNWALL_REMOVE",
    "EVNT_FIREJAMBIYA_REMOVE",
    "EVNT_SANDJAMBIYA_REMOVE",
    "EVNT_LIGHTNING_REMOVE",
    "EVNT_SHADOWSWORD_REMOVE",
    "EVNT_WITHER_REMOVE",
    "EVNT_GOLEM_REMOVE",
    "EVNT_SHIELDOFMIST_REMOVE",
    "EVNT_SHIELDOFWIND_REMOVE",
    "EVNT_HEALINGMUD_REMOVE",
    "EVNT_FIRESEED_REMOVE",
    "EVNT_REMOVE_ASH",
    "EVNT_VAMPIRICBLADE_REMOVE",
    "EVNT_REMOVE_ROOM",
};

/* local functions */
int event_rewind_spell(struct heap_event *ev);
int event_remove_object(struct heap_event *ev);
int event_remove_mobile(struct heap_event *ev);
int event_remove_room(struct heap_event *ev);
int event_npc_pulse(struct heap_event *ev);
int event_obj_pulse(struct heap_event *ev);
int event_room_pulse(struct heap_event *ev);
int event_move(struct heap_event *ev);
int event_save(struct heap_event *ev);
int event_rest(struct heap_event *ev);
int event_gain(struct heap_event *ev);
int event_weather(struct heap_event *ev);
int event_null(struct heap_event *ev);  /* this function will remove
                                         * the event, for unknowns */
int event_remove_sandwall(struct heap_event *ev);
int event_remove_firewall(struct heap_event *ev);
int event_remove_windwall(struct heap_event *ev);
int event_remove_thornwall(struct heap_event *ev);
int event_remove_bladebarrier(struct heap_event *ev);
int event_remove_shade(struct heap_event *ev);
int event_remove_alarm(struct heap_event *ev);
int event_fire_burn(struct heap_event *ev);
int event_shutdown(struct heap_event *ev);
int event_remove_dark(struct heap_event *ev);
int event_remove_noelemmagick(struct heap_event *ev);
int event_detonate(struct heap_event *ev);
int event_remove_firejambiya(struct heap_event *ev);
int event_remove_sandjambiya(struct heap_event *ev);
int event_remove_lightning(struct heap_event *ev);
int event_remove_shadowsword(struct heap_event *ev);
int event_remove_wither(struct heap_event *ev);
int event_remove_golem(struct heap_event *ev);
int event_remove_shieldofmist(struct heap_event *ev);
int event_remove_shieldofwind(struct heap_event *ev);
int event_remove_healingmud(struct heap_event *ev);
int event_remove_fireseed(struct heap_event *ev);
int event_remove_ash(struct heap_event *ev);
int event_vampiricblade_remove(struct heap_event *ev);

void heapify(int i);

/* Hackish global */
int event_random_moving = 0;

struct event_procedure
{
    int (*evnt_proc) (struct heap_event * ev);
};

struct event_procedure event_procedures[MAX_EVENTS] = {
    {event_null},
    {event_save},               /* EVNT_SAVE   */
    {event_move},               /* EVNT_MOVE   */
    {event_npc_pulse},          /* EVNT_NPC_PULSE   */
    {event_obj_pulse},          /* EVNT_OBJ_PULSE */
    {event_room_pulse},         /* EVNT_ROOM_PULSE */
    {event_null},               /* EVNT_ATTACK */
    {event_rest},               /* EVNT_REST   */
    {event_gain},               /* EVNT_GAIN   */
    {event_remove_sandwall},    /* EVENT_REMOVE_SANDWALL */
    {event_remove_shade},       /* EVENT_REMOVE_SHADE */
    {event_shutdown},           /* EVNT_SHUTDOWN */
    {event_weather},            /* EVNT_WEATHER */
    {event_remove_object},      /* EVNT_REMOVE_OBJECT */
    {event_remove_mobile},      /* EVNT_REMOVE_MOBILE        */
    {event_rewind_spell},       /* EVNT_REWIND               */
    {event_remove_alarm},       /* EVNT_REMOVE_ALARM  - Kel  */
    {event_fire_burn},          /* EVNT_FIRE_BURN -Tenebrius */
    {event_remove_dark},        /* EVENT_REMOVE_DARK */
    {event_remove_noelemmagick},        /* EVNT_REMOVE_NOELEMMAGICK */
    {event_detonate},           /* EVNT_DETONATE */
    {event_remove_firewall},    /* EVNT_REMOVE_FIREWALL */
    {event_remove_windwall},    /* EVNT_REMOVE_WINDWALL */
    {event_remove_bladebarrier},        /* EVNT_REMOVE_BLADEBARRIER */
    {event_remove_thornwall},   /* EVNT_REMOVE_THORNWALL */
    {event_remove_firejambiya}, /* EVNT_FIREJAMBIYA_REMOVE */
    {event_remove_sandjambiya}, /* EVNT_SANDJAMBIYA_REMOVE */
    {event_remove_lightning},   /* EVNT_LIGHTNING_REMOVE */
    {event_remove_shadowsword}, /* EVNT_SHADOWSWORD_REMOVE */
    {event_remove_wither},      /* EVNT_WITHER_REMOVE */
    {event_remove_golem},       /* EVNT_GOLEM_REMOVE */
    {event_remove_shieldofmist},        /* EVNT_SHIELDOFMIST_REMOVE */
    {event_remove_shieldofwind},        /* EVNT_SHIELDOFWIND_REMOVE */
    {event_remove_healingmud},  /* EVNT_HEALINGMUD_REMOVE */
    {event_remove_fireseed},    /* EVNT_FIRESEED_REMOVE */
    {event_remove_ash},         /* EVENT_REMOVE_ASH */
    {event_vampiricblade_remove},       /* EVNT_VAMPIRICBLADE_REMOVE */
    {event_remove_room}         /* EVNT_REMOVE_ROOM          */
};

int
process_event(struct heap_event *ev)
{
    /* This is the interface, when an event needs to be proccesed
     * if this function returns 1, then the event is to be put back into
     * the heap, it is assumed that ev->time has been set to the number
     * of ticks until it is next called.
     */

    int res;

    if ((ev->type > 0) && (ev->type < MAX_EVENTS)) {
        res = (*event_procedures[(int) ev->type].evnt_proc) (ev);
    } else
        res = 0;

    return (res);
}

int
event_null(struct heap_event *ev)
{
    return (0);
}

int
event_rewind_spell(struct heap_event *ev)
{
    char buffer[MAX_INPUT_LENGTH + 200];

    if (-1 ==
        path_from_to_for(ev->ch->in_room, ev->room, ev->extra, MAX_INPUT_LENGTH, buffer, ev->ch)) {
        strcpy(buffer, "A blankness");
    } else {
        if (ev->room) {
            strcat(buffer, ev->room->name);
        }
    }

    send_to_char("You feel a burst of magickal energy coming from:\n\r", ev->ch);
    send_to_char(buffer, ev->ch);
    return (0);
}

int
event_remove_object(struct heap_event *ev)
{
    struct char_data *tar_ch;
    char fade_msg[512];

/* for ball of light, but messages should apply to any 
   maybe a case statement, or read the spell messages structure 
 */

    if (ev->tar_obj->obj_flags.type == ITEM_WAGON && ev->tar_obj->obj_flags.value[0] >= 73000
        && ev->tar_obj->obj_flags.value[0] <= 73999 && ev->tar_obj->in_room) {
        ROOM_DATA *to_room;
        OBJ_DATA *obj, *next_obj;
        CHAR_DATA *next_ch = NULL;

        if ((to_room = get_room_num(ev->tar_obj->obj_flags.value[0])) != NULL) {
            for (tar_ch = to_room->people; tar_ch; tar_ch = next_ch);
            {
                next_ch = tar_ch->next_in_room;
                char_from_room(tar_ch);
                char_to_room(tar_ch, ev->tar_obj->in_room);
                act("$p collapses around you.", FALSE, tar_ch, ev->tar_obj, NULL, TO_CHAR);
            }

            for (obj = to_room->contents; obj; obj = next_obj) {
                next_obj = obj->next_content;
                obj_from_room(obj);
                obj_to_room(obj, ev->tar_obj->in_room);
            }
        }
    }

    if (ev->tar_obj->in_room
        && get_obj_extra_desc_value(ev->tar_obj, "[room_fade_message]", fade_msg,
                                    sizeof(fade_msg))) {
        if (strlen(fade_msg) > 0)
            send_to_room(fade_msg, ev->tar_obj->in_room);
        extract_obj(ev->tar_obj);
        return 0;
    }

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (char_can_see_obj(tar_ch, ev->tar_obj))
                act("$p fades from existence.", FALSE, tar_ch, ev->tar_obj, 0, TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("With a soft implosion $p falls away into fading sparks.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_CHAR);
        act("With a soft implosion $p falls away from $n in fading sparks.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_ROOM);
        if ((ev->tar_obj->obj_flags.type = ITEM_LIGHT) && (ev->tar_obj->obj_flags.value[0])) {
            if (ev->tar_obj->equipped_by->in_room->light > 0)
                act("The area is plunged into darkness.", FALSE, ev->tar_obj->equipped_by, 0, 0,
                    TO_ROOM);
            ev->tar_obj->equipped_by->in_room->light--;
        }
    } else if (ev->tar_obj->carried_by) {
        act("$p fades away.", TRUE, ev->tar_obj->carried_by, ev->tar_obj, 0, TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and 
                                 * ev->type = EVNT_DELT 
                                 */
    return (0);
}

int
event_remove_firejambiya(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("$p's flames slowly die away, disappearing where it lies.", FALSE, tar_ch,
                    ev->tar_obj, 0, TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("$p's flames slowly die away in your hands, until it is no more.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_CHAR);
        act("The flames of $p in $n's hand slowly die away, until it is no more.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("$p's flames slowly die away, until it is no more.", TRUE, ev->tar_obj->carried_by,
            ev->tar_obj, 0, TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}

int
event_remove_sandjambiya(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("$p crumbles away into sand and dust where it lies.", FALSE, tar_ch,
                    ev->tar_obj, 0, TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("$p crumbles away into sand and dust in your hands.", TRUE, ev->tar_obj->equipped_by,
            ev->tar_obj, 0, TO_CHAR);
        act("$p crumbles away into sand and dust in $n's hands.", TRUE, ev->tar_obj->equipped_by,
            ev->tar_obj, 0, TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("$p crumbles away into sand and dust.", TRUE, ev->tar_obj->carried_by, ev->tar_obj, 0,
            TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}

int
event_remove_golem(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("$p slowly fades from view.", FALSE, tar_ch, ev->tar_obj, 0, TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("$p crumbles away in your hands.", TRUE, ev->tar_obj->equipped_by, ev->tar_obj, 0,
            TO_CHAR);
        act("$p crumbles away in $n's hands.", TRUE, ev->tar_obj->equipped_by, ev->tar_obj, 0,
            TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("$p crumbles away.", TRUE, ev->tar_obj->carried_by, ev->tar_obj, 0, TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}


int
event_remove_lightning(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("Scattering sparks across the ground, $p disappears.", FALSE, tar_ch,
                    ev->tar_obj, 0, TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("In a last shower of sparks, $p disappears from your hands.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_CHAR);
        act("In a last shower of sparks, $p disappears from $n's hands.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("In a last shower of sparks, $p disappears.", TRUE, ev->tar_obj->carried_by,
            ev->tar_obj, 0, TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}

int
event_remove_shadowsword(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("Shadows writhe as $p dissolves where it lies.", FALSE, tar_ch, ev->tar_obj, 0,
                    TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("Shadows writhe as $p dissolves in your hands.", TRUE, ev->tar_obj->equipped_by,
            ev->tar_obj, 0, TO_CHAR);
        act("Shadows writhe as $p dissolves in $n's hands.", TRUE, ev->tar_obj->equipped_by,
            ev->tar_obj, 0, TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("Shadows writhe as $p dissolves.", TRUE, ev->tar_obj->carried_by, ev->tar_obj, 0,
            TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}

int
event_remove_shieldofmist(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("$p slowly dissolves into a fine mist.", FALSE, tar_ch, ev->tar_obj, 0,
                    TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("$p slowly dissolves into a fine mist as you hold it.", TRUE, ev->tar_obj->equipped_by,
            ev->tar_obj, 0, TO_CHAR);
        act("$p slowly dissolves into a fine mist in $n's hands.", TRUE, ev->tar_obj->equipped_by,
            ev->tar_obj, 0, TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("$p slowly dissolves into a fine mist.", TRUE, ev->tar_obj->carried_by, ev->tar_obj, 0,
            TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}
int
event_remove_shieldofwind(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("$p slowly fades away as a strong breeze rushes past.", FALSE, tar_ch,
                    ev->tar_obj, 0, TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("$p slowly fades from your hands as a strong breeze rushes past.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_CHAR);
        act("$p slowly fades from $n's hands as a strong breeze rushes past.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("$p slowly fades from your inventory as a strong breeze rushes past.", TRUE,
            ev->tar_obj->carried_by, ev->tar_obj, 0, TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}

int
event_remove_fireseed(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("$p slowly fades away in a series of dim flashes.", FALSE, tar_ch, ev->tar_obj,
                    0, TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("$p slowly fades from your hands in a series of dim flashes.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_CHAR);
        act("$p slowly fades from $n's hands in a series of dim flashes.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("$p slowly fades from your inventory in a series of dim flashes.", TRUE,
            ev->tar_obj->carried_by, ev->tar_obj, 0, TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}


int
event_remove_healingmud(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("$p slowly dissolves into a sandy grit.", FALSE, tar_ch, ev->tar_obj, 0,
                    TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("$p slowly dissolves into a sandy grit as you hold it.", TRUE, ev->tar_obj->equipped_by,
            ev->tar_obj, 0, TO_CHAR);
        act("$p slowly dissolves into a sandy grit in $n's hands.", TRUE, ev->tar_obj->equipped_by,
            ev->tar_obj, 0, TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("$p slowly dissolves into a sandy grit.", TRUE, ev->tar_obj->carried_by, ev->tar_obj, 0,
            TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}

int
event_vampiricblade_remove(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("$p slowly decays into useless, black ash.", FALSE, tar_ch, ev->tar_obj, 0,
                    TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        act("$p slowly decays into a useless, black ash as you hold it.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_CHAR);
        act("$p slowly decays into a useless, blach ash in $n's hands.", TRUE,
            ev->tar_obj->equipped_by, ev->tar_obj, 0, TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        act("$p slowly decays into a useless, black ash.", TRUE, ev->tar_obj->carried_by,
            ev->tar_obj, 0, TO_CHAR);
    }
    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}

int
event_remove_wither(struct heap_event *ev)
{
    struct char_data *tar_ch;

    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("Slowly, the vegetation begins to revive.", FALSE, tar_ch, ev->tar_obj, 0,
                    TO_CHAR);
    }

    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and
                                 * ev->type = EVNT_DELT
                                 */
    return (0);
}

int
event_detonate(struct heap_event *ev)
{
    ROOM_DATA *target_room = NULL;
    CHAR_DATA *tar_ch = NULL, *next = NULL;
    OBJ_DATA *tempobj = NULL;
    int damage = 0, hit_damage = 0, door;
    char buf[MAX_STRING_LENGTH] = "";
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (ev->tar_obj->in_room) {
        target_room = ev->tar_obj->in_room;

        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("$p violently explodes!", FALSE, tar_ch, ev->tar_obj, 0, TO_CHAR);
            else
                act("Your body is rocked by the shockwave of a violent explosion!", FALSE, tar_ch,
                    ev->tar_obj, 0, TO_CHAR);
    } else if (ev->tar_obj->equipped_by) {
        target_room = ev->tar_obj->equipped_by->in_room;

        act("You shake as $p violently explodes!", TRUE, ev->tar_obj->equipped_by, ev->tar_obj, 0,
            TO_CHAR);
        act("$n shakes as $p violently explodes!", TRUE, ev->tar_obj->equipped_by, ev->tar_obj, 0,
            TO_ROOM);
    } else if (ev->tar_obj->carried_by) {
        target_room = ev->tar_obj->carried_by->in_room;

        act("$p violently explodes in your hands!", TRUE, ev->tar_obj->carried_by, ev->tar_obj, 0,
            TO_CHAR);
        act("$p violently explodes in $n's hands!", TRUE, ev->tar_obj->carried_by, ev->tar_obj, 0,
            TO_ROOM);
    }

    /* Tell everyone next-door about the explosion too */
    if (target_room)
        for (door = 0; door < 6; door++)
            if (target_room->direction[door]) {
                sprintf(buf, "You hear a loud explosion, and a cloud of dust blows in from %s.\n\r",
                        rev_dir_name[door]);
                send_to_room(buf, target_room->direction[door]->to_room);
            }

    if ((!target_room) && (!ev->tar_obj->carried_by)) {
        if ((ev->tar_obj->in_obj) && (ev->tar_obj->in_obj->in_room)) {
            if (ev->tar_obj->in_obj->obj_flags.type == ITEM_CONTAINER)
                act("There is a muffled explosion within $p!", TRUE, 0, ev->tar_obj->in_obj, 0,
                    TO_ROOM);
            if (ev->tar_obj->in_obj->obj_flags.type == ITEM_FURNITURE)
                act("There is a loud explosion on top $p!", TRUE, 0, ev->tar_obj->in_obj, 0,
                    TO_ROOM);
        } else if ((ev->tar_obj->in_obj) && (ev->tar_obj->in_obj->carried_by)) {
            if (ev->tar_obj->in_obj->obj_flags.type == ITEM_CONTAINER)
                act("There is a muffled explosion within $p!", TRUE,
                    ev->tar_obj->in_obj->carried_by, ev->tar_obj->in_obj, 0, TO_CHAR);
            if (ev->tar_obj->in_obj->obj_flags.type == ITEM_FURNITURE)
                act("There is a loud explosion on top $p!", TRUE, ev->tar_obj->in_obj->carried_by,
                    ev->tar_obj->in_obj, 0, TO_CHAR);
        } else if ((ev->tar_obj->in_obj) && (ev->tar_obj->in_obj->equipped_by)) {
            if (ev->tar_obj->in_obj->obj_flags.type == ITEM_CONTAINER)
                act("There is a muffled explosion within $p!", TRUE,
                    ev->tar_obj->in_obj->equipped_by, ev->tar_obj->in_obj, 0, TO_CHAR);
            if (ev->tar_obj->in_obj->obj_flags.type == ITEM_FURNITURE)
                act("There is a loud explosion on top $p!", TRUE, ev->tar_obj->in_obj->equipped_by,
                    ev->tar_obj->in_obj, 0, TO_CHAR);
        }

        else
            gamelog
                ("Error in Trap: Trapped item either not in room/obj/PC or two many levels deep to report message.");

        extract_obj(ev->tar_obj);       /* note calling this will make ev->tar_obj = 0 and 
                                         * ev->type = EVNT_DELT  */

        return (0);
    }


    /* Apply Damage here */
    if ((ev->tar_obj->obj_flags.type == ITEM_CONTAINER) && (target_room)) {
        damage = number(10, 25) + number(10, 25);
        /* add damage done by stuff in the obj */
        for (tempobj = ev->tar_obj->contains; tempobj; tempobj = tempobj->next_content) {
            /* Explosive stuff inside, set it off */
            if ((tempobj->obj_flags.type == ITEM_TOOL)
                && (tempobj->obj_flags.value[0] == TOOL_TRAP))
                damage += number(10, 25);
        }

        /* Room-wide damage */
        for (tar_ch = target_room->people; tar_ch; tar_ch = next) {
            next = tar_ch->next_in_room;
            hit_damage = number(10, 25) + number(10, 25);

            if ((ev->tar_obj->carried_by == tar_ch) || (ev->tar_obj->equipped_by == tar_ch))
                hit_damage = damage;

            if (!affected_by_spell(tar_ch, SPELL_DEAFNESS) && !IS_IMMORTAL(tar_ch)) {

                af.type = SPELL_DEAFNESS;
                af.duration = 1;
                af.power = 1;
                af.magick_type = MAGICK_NONE;
                af.location = CHAR_APPLY_AGL;
                af.modifier = -3;
                af.bitvector = CHAR_AFF_DEAFNESS;

                affect_to_char(tar_ch, &af);
                send_to_char("Your ears ring from the force of the explosion.\n\r", tar_ch);

                /* Put 'em on the ground */
                set_char_position(tar_ch, POSITION_RESTING);
            }

            if (!generic_damage(tar_ch, hit_damage, 0, 0, damage)) {
                sprintf(buf, "%s %s%s%shas been killed by an explosion in room #%d, boom.",
                        IS_NPC(tar_ch) ? MSTR(tar_ch, short_descr)
                        : GET_NAME(tar_ch), !IS_NPC(tar_ch) ? "(" : "",
                        !IS_NPC(tar_ch) ? tar_ch->account : "", !IS_NPC(tar_ch) ? ") " : "",
                        tar_ch->in_room->number);

                if (!IS_NPC(tar_ch)) {
                    struct time_info_data playing_time =
                        real_time_passed((time(0) - tar_ch->player.time.logon) +
                                         tar_ch->player.time.played, 0);

                    if ((playing_time.day > 0 || playing_time.hours > 2
                         || (tar_ch->player.dead && !strcmp(tar_ch->player.dead, "rebirth")))
                        && !IS_SET(tar_ch->specials.act, CFL_NO_DEATH)) {
                        if (tar_ch->player.info[1])
                            free(tar_ch->player.info[1]);
                        tar_ch->player.info[1] = strdup(buf);
                    }
                }

                if (IS_NPC(tar_ch))
                    shhlog(buf);
                else
                    gamelog(buf);

                die(tar_ch);
            }
        }
    }

    extract_obj(ev->tar_obj);   /* note calling this will
                                 * make ev->tar_obj = 0 and 
                                 * ev->type = EVNT_DELT 
                                 */
    return (0);
}

int
event_fire_burn(struct heap_event *ev)
{
    struct char_data *tar_ch;
    struct obj_data *obj, *obj2, *obj3;
    int obj_count;
    int rem_num;

    /*
     * For Item types of fire, should only be generated by
     * lighting something, then extinguishing it, the object
     * will be removed from the game, hence from this list
     */
    gamelog("event_fire_burn");
    if (ev->tar_obj->in_room) {
        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("The $p crackles.", FALSE, tar_ch, ev->tar_obj, 0, TO_CHAR);
    } else {
        /* this shouldent really happen */
        extract_obj(ev->tar_obj);       /* note calling this will
                                         * make ev->tar_obj = 0 and 
                                         * ev->type = EVNT_DELT 
                                         */
        return (0);
    }

    if (ev->tar_obj->obj_flags.value[1] > 0) {  /* Timer */
        /* knock something out of its contents */

        /* count up objects */
        for (obj = ev->tar_obj->contains, obj_count = 0; obj; obj = obj->next_content, obj_count++) {   /* dont really do anything */
        }

        /* pick a random number from the num of objects */
        rem_num = number(1, obj_count);

        /* get the pointer to that object */
        for (obj = ev->tar_obj->contains, obj_count = 1; obj && (obj_count != rem_num); obj = obj->next_content, obj_count++) { /* dont really do anything */
        }

        if (obj) {
            /* do something to this object */
            for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
                if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                    act("The $p bursts into flames.", FALSE, tar_ch, obj, 0, TO_CHAR);
            for (obj2 = obj->contains; obj2; obj2 = obj3) {
                obj3 = obj2->next_content;
                obj_from_obj(obj2);
                obj_to_obj(obj2, ev->tar_obj);
            }
            ev->tar_obj->obj_flags.value[1] += obj->obj_flags.weight;
            extract_obj(obj);
        } else {
            ev->tar_obj->obj_flags.value[0] -= 1;
        }

        (ev->tar_obj->obj_flags.value[1]) -= ev->tar_obj->obj_flags.value[2];
        ev->time = (1.0 / (double) (ev->tar_obj->obj_flags.value[2])) * number(1000, 2000);
        return (1);
    } else {
        /* anything that is left in the fire gets put into room */
        for (obj = ev->tar_obj->contains; obj; obj = obj2) {
            obj2 = obj->next_content;
            obj_from_obj(obj);
            obj_to_room(obj, ev->tar_obj->in_room);
        }

        for (tar_ch = ev->tar_obj->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
            if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                act("The $p dies out.", FALSE, tar_ch, ev->tar_obj, 0, TO_CHAR);

        extract_obj(ev->tar_obj);       /* note calling this will
                                         * make ev->tar_obj = 0 and 
                                         * ev->type = EVNT_DELT 
                                         */
        return (0);
    }
}

int
event_remove_mobile(struct heap_event *ev)
{
    /*  the is used by send_shadow -- 1200 */
    /* you have to do the corpse stuff here */
    act("You blip out of existence.", FALSE, ev->ch, 0, 0, TO_CHAR);
    act("$n blips out of existence.", FALSE, ev->ch, 0, 0, TO_ROOM);
    extract_char(ev->ch);       /* note calling this will
                                 * make ev->ch = 0 and 
                                 * ev->type = EVNT_DELT 
                                 */
    return (0);
}

int
event_shutdown(struct heap_event *ev)
{
    char buf[256];
    struct char_data *ch;

    if (--(ev->extra) > 0) {
        sprintf(buf, "Next shutdown in %d minutes.\n\r", ev->extra);
        send_to_all(buf);
        sprintf(buf, "Next shutdown in %d minutes.", ev->extra);
        shhlog(buf);
        ev->time = 130;
        return (1);
    }

    /* save everybody in game */
    for (ch = character_list; ch; ch = ch->next) {
        if (!(IS_NPC(ch)))
            cmd_save(ch, "", 0, 0);
    }

    /* save zones that should be saved unless on test port */
    if (!test_boot) {
        // auto_save(35); -- Removed 5/4/2004 Tiernan
        auto_save(3);
        auto_save(13);
        auto_save(33);
        auto_save(73);
    }
    shutdown_game = 1;
    return (0);
}

int
event_weather(struct heap_event *ev)
{

    save_weather_data();
    ev->time = 1000;
    return (1);
}

int
event_remove_sandwall(struct heap_event *ev)
{
    char buffer[256];
    struct char_data *tar_ch;
    if (ev->room)
        if ((ev->extra <= 5) && (ev->extra >= 0))
            if (ev->room->direction[ev->extra])
                if (IS_SET(ev->room->direction[ev->extra]->exit_info, EX_SPL_SAND_WALL)) {
                    REMOVE_BIT(ev->room->direction[ev->extra]->exit_info, EX_SPL_SAND_WALL);
                    sprintf(buffer,
                            "The wall of sand to %s collapses and " "crumbles to the ground.",
                            verbose_dirs[ev->extra]);
                    for (tar_ch = ev->room->people; tar_ch; tar_ch = tar_ch->next_in_room)
                        if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                            act(buffer, FALSE, tar_ch, 0, 0, TO_CHAR);
                }
    return (0);
}

int
event_remove_firewall(struct heap_event *ev)
{
    char buffer[256];
    struct char_data *tar_ch;
    if (ev->room)
        if ((ev->extra <= 5) && (ev->extra >= 0))
            if (ev->room->direction[ev->extra])
                if (IS_SET(ev->room->direction[ev->extra]->exit_info, EX_SPL_FIRE_WALL)) {
                    REMOVE_BIT(ev->room->direction[ev->extra]->exit_info, EX_SPL_FIRE_WALL);
                    sprintf(buffer,
                            "The wall of fire to %s expires and "
                            "disappears in a cloud of white smoke.", verbose_dirs[ev->extra]);
                    for (tar_ch = ev->room->people; tar_ch; tar_ch = tar_ch->next_in_room)
                        if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                            act(buffer, FALSE, tar_ch, 0, 0, TO_CHAR);
                }
    return (0);
}

int
event_remove_windwall(struct heap_event *ev)
{
    char buffer[256];
    struct char_data *tar_ch;
    if (ev->room)
        if ((ev->extra <= 5) && (ev->extra >= 0))
            if (ev->room->direction[ev->extra])
                if (IS_SET(ev->room->direction[ev->extra]->exit_info, EX_SPL_WIND_WALL)) {
                    REMOVE_BIT(ev->room->direction[ev->extra]->exit_info, EX_SPL_WIND_WALL);
                    sprintf(buffer,
                            "The wall of wind to %s begins to spin "
                            "itself into a small whirlind, then expands and " "disperses.",
                            verbose_dirs[ev->extra]);
                    for (tar_ch = ev->room->people; tar_ch; tar_ch = tar_ch->next_in_room)
                        if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                            act(buffer, FALSE, tar_ch, 0, 0, TO_CHAR);
                }
    return (0);
}

int
event_remove_thornwall(struct heap_event *ev)
{
    char buffer[256];
    struct char_data *tar_ch;
    if (ev->room)
        if ((ev->extra <= 5) && (ev->extra >= 0))
            if (ev->room->direction[ev->extra])
                if (IS_SET(ev->room->direction[ev->extra]->exit_info, EX_SPL_THORN_WALL)) {
                    REMOVE_BIT(ev->room->direction[ev->extra]->exit_info, EX_SPL_THORN_WALL);
                    sprintf(buffer,
                            "The wall of thorns to %s shrivels and " "retreats into the ground.",
                            verbose_dirs[ev->extra]);
                    for (tar_ch = ev->room->people; tar_ch; tar_ch = tar_ch->next_in_room)
                        if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                            act(buffer, FALSE, tar_ch, 0, 0, TO_CHAR);
                }
    return (0);
}

int
event_remove_bladebarrier(struct heap_event *ev)
{
    char buffer[256];
    struct char_data *tar_ch;
    if (ev->room)
        if ((ev->extra <= 5) && (ev->extra >= 0))
            if (ev->room->direction[ev->extra])
                if (IS_SET(ev->room->direction[ev->extra]->exit_info, EX_SPL_BLADE_BARRIER)) {
                    REMOVE_BIT(ev->room->direction[ev->extra]->exit_info, EX_SPL_BLADE_BARRIER);
                    sprintf(buffer,
                            "The whirling blades to %s begin to spin "
                            "outward until they fade from view.", verbose_dirs[ev->extra]);
                    for (tar_ch = ev->room->people; tar_ch; tar_ch = tar_ch->next_in_room)
                        if (!(IS_AFFECTED(tar_ch, CHAR_AFF_BLIND)))
                            act(buffer, FALSE, tar_ch, 0, 0, TO_CHAR);
                }
    return (0);
}


int
event_remove_noelemmagick(struct heap_event *ev)
{
    if (ev->room && IS_SET(ev->room->room_flags, RFL_NO_ELEM_MAGICK)) {
        REMOVE_BIT(ev->room->room_flags, RFL_NO_ELEM_MAGICK);
        send_to_room("Your skin tingles briefly.\n\r", ev->room);
    }
    return (0);
}

int
event_remove_dark(struct heap_event *ev)
{
    if (ev->room && IS_SET(ev->room->room_flags, RFL_DARK)) {
        REMOVE_BIT(ev->room->room_flags, RFL_DARK);
        send_to_room("The cloud of darkness slowly dissipates.\n\r", ev->room);
    }
    return (0);
}

int
event_remove_shade(struct heap_event *ev)
{
    if (ev->room && IS_SET(ev->room->room_flags, RFL_RESTFUL_SHADE)) {
        REMOVE_BIT(ev->room->room_flags, RFL_RESTFUL_SHADE);
        send_to_room("The area becomes hotter as the restful shade dissipates.\n\r", ev->room);
    }
    return (0);
}

int
event_remove_ash(struct heap_event *ev)
{
    if (ev->room && IS_SET(ev->room->room_flags, RFL_ASH)) {
        REMOVE_BIT(ev->room->room_flags, RFL_ASH);
        send_to_room("The grey, lifeless ash dissipates.\n\r", ev->room);
    }
    return (0);
}

int
event_remove_alarm(struct heap_event *ev)
{
    if (ev->room && IS_SET(ev->room->room_flags, RFL_SPL_ALARM)) {
	remove_alarm_spell(ev->room);
        send_to_room("A high-pitched tinkling sound echos in the back of your mind.\n\r", ev->room);
    }
    return (0);
}

int
event_npc_pulse(struct heap_event *ev)
{
    char arg[MAX_STRING_LENGTH] = "";
    if (!ev->ch || !has_special_on_cmd(ev->ch->specials.programs, NULL, CMD_PULSE))
        return 0;

    ev->time = PULSE_MOBILE + number(-5, 5);
    ev->time = MAX(ev->time, 1);

    execute_npc_program(ev->ch, 0, CMD_PULSE, arg);
    return 1;
}

int
event_obj_pulse(struct heap_event *ev)
{
    char arg[MAX_STRING_LENGTH] = "";

    if (!ev->tar_obj || !has_special_on_cmd(ev->tar_obj->programs, NULL, CMD_PULSE))
        return 0;

    ev->time = PULSE_OBJECT + number(-5, 5);
    ev->time = MAX(ev->time, 1);
    if (ev->extra > ev->time) {
        ev->time = ev->extra;
    }

    execute_obj_program(ev->tar_obj, 0, 0, CMD_PULSE, arg);
    return 1;
}

int
event_room_pulse(struct heap_event *ev)
{
    char arg[MAX_STRING_LENGTH] = "";

    if (!ev->room || !has_special_on_cmd(ev->room->specials, NULL, CMD_PULSE))
        return 0;

    ev->time = PULSE_ROOM + number(-5, 5);
    ev->time = MAX(ev->time, 1);

    execute_room_program(ev->room, 0, CMD_PULSE, arg);
    return 1;
}

int
event_remove_room(struct heap_event *ev)
{
    if (!ev->room)
	return 0;

    if (ev->room && ev->room->people) {
	// Someone is still in the room, try later
        ev->time = PULSE_ROOM + number(-5, 5);
        ev->time = MAX(ev->time, 1);
        return 1; // Keep it active
    }

    extract_room(ev->room, 1);
    return 0;
}

int
event_move(struct heap_event *ev)
{
    int dir;
    int prev_moves = 0;
    ROOM_DATA *in_room;

    if (!ev->ch || !IS_NPC(ev->ch) || (ev->ch->master) || (ev->ch->desc)
        || IS_SET(ev->ch->specials.act, CFL_SENTINEL))
        return (0);

    if (IS_AFFECTED(ev->ch, CHAR_AFF_SUBDUED))
        return (0);

    if ((in_room = ev->ch->in_room) == NULL) {
        gamelogf("Tried to move character not in a room: %d - %s", ev->ch->nr, MSTR(ev->ch, name));
        return 0;
    }

    if (zone_table[in_room->zone].reset_mode == 3)
        return (0);

    if (visibility(ev->ch) < 0)
        return 0;               // Don't wander in bad storms

    if (IS_DARK(ev->ch->in_room))
        return 0;

    // Pick a random non-up direction to wander in
    dir = number(0, 5);
    ev->time = (PULSE_MOBILE + number(-10, 10)) * 8;

    if (dir != DIR_UP && in_room->direction[dir]        // Should have been safe to assume this, right?
        && in_room->direction[dir]->to_room     // If there's a room to move to, in that direction
        // Are we allowed to wander between zone?  Is the next room in this zone?
        && (!IS_SET(ev->ch->specials.act, CFL_STAY_ZONE)
            || in_room->direction[dir]->to_room->zone == in_room->zone)
        && !IS_SET(in_room->direction[dir]->to_room->room_flags, RFL_NO_MOB)
        && GET_POS(ev->ch) == POSITION_STANDING) {
        /* I put in this horrible kludge so that npcs would not get added
         * to the regen heap due to random moving -Hal */
        event_random_moving = 1;
        prev_moves = GET_MOVE(ev->ch);

        // parse_command (ev->ch, dir_name[dir]);
        cmd_move(ev->ch, "", dir + 1, 0);

        // cmd_move may wind up killing the char, so check if 
        // is still valid 
        if (ev->ch)
            set_move(ev->ch, prev_moves);
        event_random_moving = 0;
    }

    return (1);
}

/* for use with event_save, taxes character at progressive rate */
void
tax_char(struct char_data *ch)
{
    int amount;
    float rate;

    if (ch->points.in_bank <= 0) {
        ch->points.in_bank = 0;
        return;
    }

    if (number(0, 6))
        return;

    /* 1.25% per each 5k in the bank over 10k */
    rate = 0.0125 * (MAX(0, ch->points.in_bank - 10000) * 0.0002);
    if (rate > 0.75)
        rate = 0.75;
    amount = (ch->points.in_bank * rate);
    ch->points.in_bank -= amount;
}

void
follower_save(struct char_data *ch)
{
    struct follow_type *fch;

    save_char(ch);
#ifdef SAVE_CHAR_OBJS
    save_char_objs(ch);
#endif

    /* recursive search through the pack, save followers of followers */
    for (fch = ch->followers; fch; fch = fch->next) {
        if (!IS_NPC(fch->follower) && fch->follower->in_room == ch->in_room)
            follower_save(fch->follower);
    }
}


int
event_save(struct heap_event *ev)
{
    char buf[256];
    struct follow_type *fch;

    if (!ev->ch || IS_NPC(ev->ch) || !ev->ch->account || !ev->ch->in_room || ev->ch->specials.was_in_room ||        /* don't save voided ppl */
        (ev->ch->desc && ev->ch->desc->connected))
        return (0);

    sprintf(buf, "Saving %s from event.c, #%d.", MSTR(ev->ch, name), ev->ch->in_room->number);
    shhlog(buf);

#ifdef BANK_TAXES
    tax_char(ev->ch);
#endif

    save_char(ev->ch);
#ifdef SAVE_CHAR_OBJS
    save_char_objs(ev->ch);
#endif

    ev->time = SAVE_DELAY;

    /* save the rest of the pack, but only if yer the head dog - Morg */
    /* or if your head dog has left you alone -Morg */
    if (!ev->ch->master || (ev->ch->master && ev->ch->master->in_room != ev->ch->in_room))
        for (fch = ev->ch->followers; fch; fch = fch->next)
            if (!IS_NPC(fch->follower)
                && fch->follower->in_room == ev->ch->in_room)
                follower_save(fch->follower);

    return (1);
}

int
mob_resting(struct char_data *ch)
{

    /* if she is stunned or worse, she will not recover */
    if (GET_POS(ch) <= POSITION_STUNNED)
        return 0;

    /* if she is resting or sleeping, she will recover */
    if (GET_POS(ch) <= POSITION_RESTING)
        return 1;

    /* if she is a mount and not resting or sleeping, she will not recover */
    if (IS_SET(ch->specials.act, CFL_MOUNT))
        return 0;

    /* if she is in combat, she will not recover */
    if (ch->specials.fighting)
        return 0;

    return 1;
}

int
event_rest(struct heap_event *ev)
{
    /*  This is the generic regeneration formula extended to
     * all mobiles, not just mounts.
     * if (!ev->ch || !IS_SET(ev->ch->specials.act, CFL_MOUNT)) 
     */

    int temp_int, end;

    /* will run once per min */
    temp_int = MAX(1, (GET_END(ev->ch) + number(1, 10)));
    end =
        dice((race[(int) GET_RACE(ev->ch)].enddice) ? (race[(int) GET_RACE(ev->ch)].enddice) : 1,
             (race[(int) GET_RACE(ev->ch)].endsize) ? (race[(int) GET_RACE(ev->ch)].endsize) : 1) +
        race[(int) GET_RACE(ev->ch)].endplus;
    ev->time = 60 * end * (1 / ((float) temp_int));
/*  ev->time = 60 * 25 * (1 / ((float) temp_int)); */

    if (!ev->ch)
        return 0;

    /*if (((GET_POS(ev->ch) <= POSITION_RESTING) && (GET_MOVE(ev->ch) <= 
     * GET_MAX_MOVE(ev->ch)))) */

    if (!IS_NPC(ev->ch)) {
        gamelogf("event_rest: Non npc put on rest heap.");
        ev->ch->specials.regenerating = 1;
        return 0;
    }

    if (!mob_resting(ev->ch))
        return 1;

    if (GET_MOVE(ev->ch) <= GET_MAX_MOVE(ev->ch))
        adjust_move(ev->ch, number(1, 10));

    if (GET_HIT(ev->ch) <= GET_MAX_HIT(ev->ch))
        adjust_hit(ev->ch, number(1, 10));

    if (!knocked_out(ev->ch) && GET_STUN(ev->ch) <= GET_MAX_STUN(ev->ch))
        adjust_stun(ev->ch, number(1, 10));

    /* if still hurt or tired, keep in the heap, else remove from the heap */
    if ((GET_HIT(ev->ch) < GET_MAX_HIT(ev->ch))
        || (GET_MOVE(ev->ch) < GET_MAX_MOVE(ev->ch))
        || GET_STUN(ev->ch) < GET_MAX_STUN(ev->ch)) {
        return 1;
    } else {
        ev->ch->specials.regenerating = 0;
        return 0;
    }
}


int
event_gain(struct heap_event *ev)
{
    if (!ev->ch || !IS_NPC(ev->ch) || (GET_OBSIDIAN(ev->ch) < 0))
        return (0);

    /* keep gaining until you reach the default for this npc */
    if (npc_default[ev->ch->nr].obsidian > GET_OBSIDIAN(ev->ch)) {
        GET_OBSIDIAN(ev->ch) =
            MIN(npc_default[ev->ch->nr].obsidian, GET_OBSIDIAN(ev->ch) + ev->extra);
        return (1);
    }

    return (0);
}



/*
   - heap_insert makes a copy of event, do not allocate space for
   an event before passing it to heap.

   - do not attempt to free up the event passed to process_event.

   - how to write functions for print and delete.

   The functions will go thru the heap and check each element
   to see if it satisfies the function you supply of the form.

   int event_cmp_char ( void *ptr, struct heap_event *evnt);
   {
      return ( (struct char_data *)ptr == evnt->ch );
   }

   that is if the function returns 1, value will be printed/deleted.
   then call the function delete/show like.

   delete ( ch, event_cmp_char );
   print_heap ( buffer, ch, event_cmp_char);

 */

int
event_ch_tar_ch_cmp(void *ch, struct heap_event *evnt)
{
    return (((struct char_data *) ch == evnt->ch)
            /* ||  add back in when use tar_chars
             * ( (struct char_data *) ch == evnt->tar_ch ) */
        );
}

int
event_obj_cmp(void *ch, struct heap_event *evnt)
{
    return ((struct obj_data *) ch == evnt->tar_obj);
}

int
event_all(void *null, struct heap_event *evnt)
{
    return (1);
}

int
event_shutdown_cmp(void *obj, struct heap_event *evnt)
{
    return (evnt->type == EVNT_SHUTDOWN);
}


int
event_room_cmp(void *room, struct heap_event *evnt)
{
    return ((struct room_data *) room == evnt->room);
}

/* 
   If you don't know how priority queue's work, remember these things.
   -A parent is greater than both its children.
   -This is in a binary-tree, in an array from [1...heap_num].
   --A parent has two children at 2*index, and 2*index+1.
   --The parent is at index/2.
   -When converting between index and the c-index, you have to subtract_one
 */

/* local global variables */
int heap_time = 0;
int heap_max = 0;
int heap_num = 0;
int heap_to_add = 0;
struct heap_event *heap = 0;


#define HEAP_P(n)  (n>>1)
#define HEAP_L(n)  (n<<1)
#define HEAP_R(n)  ((n<<1) + 1)

void
new_event(byte type, int time, struct char_data *ch, struct char_data *tar_ch,
          struct obj_data *tar_obj, struct room_data *room, int extra)
{

    int i;

    /* hacked this in for npc regen -Hal */
    if (type == EVNT_REST) {
        if ((event_random_moving) || (ch->specials.regenerating))
            //      if (ch->specials.regenerating)
            return;
        else
            ch->specials.regenerating = 1;
    }

    /* only one EVNT_GAIN per npc -Morg */
    if (type == EVNT_GAIN) {
        for (i = 0; i < heap_num; i++) {
            if (heap[i].ch == ch) {
                /* if this new increase amount is more, use it instead */
                if (heap[i].extra < extra)
                    heap[i].extra = extra;

                return;
            }
        }
    }
    /* only one EVNT_REMOVE_ASH per room -Tiernan */
    if (type == EVNT_REMOVE_ASH) {
        for (i = 0; i < heap_num; i++) {
            if (heap[i].type == type && heap[i].room == room) {
                heap[i].time += time;
                return;
            }
        }
    }

    i = heap_num + heap_to_add;
    heap_to_add++;
    if (!heap) {
        /* change heap_max = 750 to 768  - a multiple of 128 
         * the RECREATE looks like it expects things in the heap to be sized
         * off a multiple of 128 - I'm wondering if because the heap was set
         * to 750, we had problems when the heap needed to grow  -- Tiernan */
        CREATE(heap, struct heap_event, (heap_max = 768));
    }
    if ((heap_num + heap_to_add) > heap_max)
        RECREATE(heap, struct heap_event,
                 (heap_max = ((heap_num + heap_to_add) / 128) * 128 + 128));

    heap[i].type = type;
    heap[i].time = time;
    heap[i].ch = ch;
/*   heap[i].tar_ch  = tar_ch;    unused right now. */
    heap[i].tar_obj = tar_obj;
    heap[i].room = room;
    heap[i].extra = extra;

}

void
heap_swap(int a, int b)
{
    struct heap_event tmp;

    a--;
    b--;

    memcpy(&tmp, &heap[a], sizeof(struct heap_event));
    memcpy(&heap[a], &heap[b], sizeof(struct heap_event));
    memcpy(&heap[b], &tmp, sizeof(struct heap_event));

}

void
heap_insert_update(void)
{
    int i;
    for (; heap_to_add > 0; heap_to_add--, heap_num++) {
        if (heap[heap_num].time < 0) {
            shhlog("Error with inserting a negative time into heap" "in heap_insert");
            heap[heap_num].time = 1;
        }

        heap[heap_num].time += heap_time;

        i = heap_num + 1;
        while ((i > 1) && (heap[HEAP_P(i) - 1].time > heap[i - 1].time)) {
            heap_swap(i, HEAP_P(i));
            i = HEAP_P(i);
        }
    }
}

void
heap_delete_top(void)
{
    heap_swap(1, heap_num);
    --heap_num;
    heapify(1);

    /* make sure the events that need to be added are on the end */
    if (heap_to_add > 0)
        memmove(&heap[heap_num], &heap[heap_num + 1], sizeof(struct heap_event) * heap_to_add);
}

void
heap_print(char *buf, void *ptr, int (*compar) (void *, struct heap_event *))
{
    int i;
    char tmp[256];
    char tmp2[256];

    struct heap_event *ev;
    strcpy(buf, "");

    for (i = 0; i < heap_num; i++)
        if (compar(ptr, &heap[i])) {
            ev = &heap[i];
            switch (ev->type) {
            case EVNT_SAVE:
            case EVNT_MOVE:
            case EVNT_NPC_PULSE:
            case EVNT_ATTACK:
            case EVNT_REST:

            case EVNT_REMOVE_MOBILE:
                sprintf(tmp, "%s:[%5d]   %s\n\r", event_names[(int) ev->type], ev->time - heap_time,
                        (ev->ch) ? MSTR(ev->ch, name) : "NULL");
                break;
            case EVNT_GAIN:
                sprintf(tmp, "%s:[%5d]   %s   coins : %d\n\r", event_names[(int) ev->type],
                        ev->time - heap_time, (ev->ch) ? MSTR(ev->ch, name) : "NULL", ev->extra);
                break;
            case EVNT_REMOVE_OBJECT:
            case EVNT_FIRE_BURN:
            case EVNT_DETONATE:
            case EVNT_OBJ_PULSE:
            case EVNT_FIREJAMBIYA_REMOVE:
            case EVNT_GOLEM_REMOVE:
            case EVNT_HEALINGMUD_REMOVE:
            case EVNT_SANDJAMBIYA_REMOVE:
            case EVNT_LIGHTNING_REMOVE:
            case EVNT_SHIELDOFMIST_REMOVE:
            case EVNT_SHIELDOFWIND_REMOVE:
            case EVNT_SHADOWSWORD_REMOVE:
            case EVNT_VAMPIRICBLADE_REMOVE:
                sprintf(tmp, "%s:[%5d]   %s\n\r", event_names[(int) ev->type], ev->time - heap_time,
                        (ev->tar_obj) ? OSTR(ev->tar_obj, name) : "NULL");
                break;
            case EVNT_REWIND:
                sprintf(tmp,
                        "%s:[%5d] : back to room %s, spell max dist = %d," "person's dist = %d\n\r",
                        event_names[(int) ev->type], ev->time - heap_time,
                        (ev->room) ? ev->room->name : "NULL", ev->extra,
                        path_from_to_for(ev->ch->in_room, ev->room, ev->extra, 256, tmp2, ev->ch));
                break;
            case EVNT_REMOVE_SANDWALL:
                sprintf(tmp, "%s:[%5d]   %s[%d] to %s\n\r", event_names[(int) ev->type],
                        ev->time - heap_time, (ev->room) ? ev->room->name : "NULL",
                        (ev->room) ? ev->room->number : -1, ((ev->extra >= 0)
                                                             && (ev->extra <
                                                                 6)) ? verbose_dirs[ev->
                                                                                    extra] :
                        "unknown exit");
                break;
            case EVNT_REMOVE_WINDWALL:
                sprintf(tmp, "%s:[%5d]   %s[%d] to %s\n\r", event_names[(int) ev->type],
                        ev->time - heap_time, (ev->room) ? ev->room->name : "NULL",
                        (ev->room) ? ev->room->number : -1, ((ev->extra >= 0)
                                                             && (ev->extra <
                                                                 6)) ? verbose_dirs[ev->
                                                                                    extra] :
                        "unknown exit");
                break;
            case EVNT_REMOVE_THORNWALL:
                sprintf(tmp, "%s:[%5d]   %s[%d] to %s\n\r", event_names[(int) ev->type],
                        ev->time - heap_time, (ev->room) ? ev->room->name : "NULL",
                        (ev->room) ? ev->room->number : -1, ((ev->extra >= 0)
                                                             && (ev->extra <
                                                                 6)) ? verbose_dirs[ev->
                                                                                    extra] :
                        "unknown exit");
                break;
            case EVNT_REMOVE_BLADEBARRIER:
                sprintf(tmp, "%s:[%5d]   %s[%d] to %s\n\r", event_names[(int) ev->type],
                        ev->time - heap_time, (ev->room) ? ev->room->name : "NULL",
                        (ev->room) ? ev->room->number : -1, ((ev->extra >= 0)
                                                             && (ev->extra <
                                                                 6)) ? verbose_dirs[ev->
                                                                                    extra] :
                        "unknown exit");
                break;
            case EVNT_REMOVE_SHADE:
            case EVNT_REMOVE_ROOM:
            case EVNT_ROOM_PULSE:
            case EVNT_REMOVE_ALARM:
            case EVNT_REMOVE_DARK:
            case EVNT_REMOVE_ASH:
            case EVNT_REMOVE_NOELEMMAGICK:
                sprintf(tmp, "%s:[%5d]\n\r", event_names[(int) ev->type], ev->time - heap_time);
                break;
            case EVNT_SHUTDOWN:
                sprintf(tmp, "%s:[%5d] minutes = %d\n\r", event_names[(int) ev->type],
                        ev->time - heap_time, ev->extra);
                break;

            case EVNT_DELT:
            default:
                sprintf(tmp, "%s:[%5d]\n\r", event_names[0], ev->time - heap_time);
                break;

            case EVNT_REMOVE_FIREWALL:
                sprintf(tmp, "%s:[%5d]   %s[%d] to %s\n\r", event_names[(int) ev->type],
                        ev->time - heap_time, (ev->room) ? ev->room->name : "NULL",
                        (ev->room) ? ev->room->number : -1, ((ev->extra >= 0)
                                                             && (ev->extra <
                                                                 6)) ? verbose_dirs[ev->
                                                                                    extra] :
                        "unknown exit");
                break;
            }
            strcat(buf, tmp);
        }
}

void
heapify(int i)
{

    int l, r, smallest;

    /* 
     * char debg[100];
     * sprintf(debg, "heapify: i = %d\n", i );  
     * gamelog (debg );
     */
    l = HEAP_L(i);
    r = HEAP_R(i);

    if ((l <= heap_num) && (heap[l - 1].time < heap[i - 1].time))
        smallest = l;
    else
        smallest = i;

    if ((r <= heap_num) && (heap[r - 1].time < heap[smallest - 1].time))
        smallest = r;

    if (smallest != i) {
        heap_swap(i, smallest);
        heapify(smallest);
    }
}

void
heap_bubble_to_top(int i)
{
    /* this function moves the i'th position to the top of the heap
     * it has to be followed by heapify(1) or delete_top, otherwize
     * the heap is not valid
     */

    while (i > 1) {
        heap_swap(i, HEAP_P(i));
        i = HEAP_P(i);
    }
}

bool
heap_exists(void *data, int (*compar) (void *, struct heap_event *))
{
    int i;

    for (i = 0; i < (heap_num + heap_to_add); i++)
        if ((*compar) (data, &heap[i]))
            return TRUE;
    return FALSE;
}

void
heap_delete(void *data, int (*compar) (void *, struct heap_event *))
{
    int i;

    for (i = 0; i < (heap_num + heap_to_add); i++)
        if ((*compar) (data, &heap[i])) {
            heap[i].type = EVNT_DELT;
            /* over-write all with 0's */
            heap[i].ch = 0;
            heap[i].tar_obj = 0;
        }
}

void
heap_update(int time_step)
{
    int i, count;
    struct heap_event *ev;
    struct timeval start;

    perf_enter(&start, "heap_update()");
    /* changed this to increment by four when I moved the heap inside the
     * pulse -Hal */
    heap_time += time_step;
    heap_insert_update();
    count = 0;
    while ((heap_num > 0) && (heap[0].time <= heap_time) && (count < 8192)) {
        count++;
        if (!process_event(&heap[0])) {
            heap_delete_top();
        } else {
            if (heap[0].time <= 0) {
                shhlogf("Error with putting in a less than 1 time: type %d", heap[0].type);
                heap[0].time = 2;
            }
            heap[0].time += heap_time;
            heapify(1);
        }
    }
    heap_insert_update();
    if (count == 8192) {
        ev = &heap[0];
        shhlogf("running over on :\n time = %d eventnr = %d\n" "[%4d]  %10s  %10s  %10s\n\r",
                heap_time, ev->type, ev->time - heap_time, (ev->ch) ? MSTR(ev->ch, name) : "NULL",
                /*(ev->tar_ch) ? MSTR( ev->tar_ch, name ) : */ "NULL",
                (ev->tar_obj) ? OSTR(ev->tar_obj, name) : "NULL");
    }

    if (heap_time > 10000) {
        /* so the numbers do not do the wrap around thing */
        heap_time -= 10000;
        for (i = 0; i < heap_num; i++)
            heap[i].time -= 10000;
    }
    perf_exit("heap_update()", start);
}

void
unboot_heap(void)
{
    heap_time = 0;
    heap_max = 0;
    heap_num = 0;
    heap_to_add = 0;
    free(heap);
    heap = 0;
}

