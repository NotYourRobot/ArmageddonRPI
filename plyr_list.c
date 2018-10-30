/*
 * File: PLYR_LIST.C
 * Usage: Player List functions.
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
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>

#include "constants.h"
#include "structs.h"
#include "handler.h"
#include "modify.h"
#include "comm.h"
#include "creation.h"
#include "cities.h"
#include "utility.h"
#include "utils.h"
#include "guilds.h"
#include "skills.h"
#include "db.h"
#include "utils.h"

PLYR_LIST *
find_in_plyr_list(PLYR_LIST * cl, CHAR_DATA * ch)
{
    PLYR_LIST *pPl;

    for (pPl = cl; pPl; pPl = pPl->next)
        if (pPl->ch == ch)
            break;

    return pPl;
}


int
count_plyr_list(PLYR_LIST * cl, CHAR_DATA * ch)
{
    PLYR_LIST *pPl;
    int counter = 0;

    for (pPl = cl; pPl; pPl = pPl->next)
        if (!ch || (ch && CAN_SEE(ch, pPl->ch)))
            counter++;

    return counter;
}


/* returns weight of all people on player list in stones */
int
get_plyr_list_weight(PLYR_LIST * cl)
{
    PLYR_LIST *pPl;
    int weight = 0;

    for (pPl = cl; pPl; pPl = pPl->next)
        weight += GET_WEIGHT(pPl->ch) * 10;

    return weight;
}



void
add_occupant(OBJ_DATA * obj, CHAR_DATA * ch)
{
    PLYR_LIST *pPl;

    if (ch == NULL) {
        gamelog("add_occupant: no ch");
        return;
    }

    if (!obj) {
        gamelog("add_occupant: no obj");
        return;
    }

    if (ch->on_obj) {
        if (obj == ch->on_obj)
            return;
        else
            remove_occupant(ch->on_obj, ch);
    }

    CREATE(pPl, PLYR_LIST, 1);
    pPl->ch = ch;
    pPl->next = obj->occupants;
    obj->occupants = pPl;
    ch->on_obj = obj;
    return;
}

void
remove_occupant(OBJ_DATA * obj, CHAR_DATA * ch)
{
    PLYR_LIST *pPl;
    PLYR_LIST *pPl2;

    if (!ch) {
        gamelog("remove_occupant: no ch");
        return;
    }

    if (!obj) {
        ch->on_obj = NULL;
        gamelog("remove_occupant: no obj");
        return;
    }

    if (!obj->occupants) {
        char buf[MAX_STRING_LENGTH];

        ch->on_obj = NULL;
        sprintf(buf, "%s was removed from %s, which had no occupants", GET_NAME(ch),
                OSTR(obj, short_descr));
        gamelog(buf);
    }

    if (obj->occupants->ch == ch) {
        pPl = obj->occupants;
        obj->occupants = obj->occupants->next;
        free(pPl);
    } else {
        for (pPl = obj->occupants; pPl; pPl = pPl->next) {
            if (pPl->next && pPl->next->ch == ch) {
                pPl2 = pPl->next;
                pPl->next = pPl->next->next;
                free(pPl2);
                break;
            }
        }
    }

    ch->on_obj = NULL;
}

void
add_lifting(OBJ_DATA * obj, CHAR_DATA * ch)
{
    PLYR_LIST *pPl;

    if (ch == NULL) {
        gamelog("add_lifting: no ch");
        return;
    }

    CREATE(pPl, PLYR_LIST, 1);
    pPl->ch = ch;
    pPl->next = obj->lifted_by;
    obj->lifted_by = pPl;
    ch->lifting = obj;
    return;
}

void
remove_all_lifting(OBJ_DATA * obj)
{
    PLYR_LIST *pPl;
    PLYR_LIST *pPl_next;

    if (obj->lifted_by) {
        for (pPl = obj->lifted_by; pPl; pPl = pPl_next) {
            pPl_next = pPl->next;
            if (pPl->ch) 
                pPl->ch->lifting = 0;
            free(pPl);
        }
        obj->lifted_by = NULL;  // To be safe
    }
}

void
remove_lifting(OBJ_DATA * obj, CHAR_DATA * ch)
{
    PLYR_LIST *pPl;
    PLYR_LIST *pPl2;

    if (obj->lifted_by->ch == ch) {
        pPl = obj->lifted_by;
        obj->lifted_by = obj->lifted_by->next;
        free(pPl);
    } else {
        for (pPl = obj->lifted_by; pPl; pPl = pPl->next) {
            if (pPl->next && pPl->next->ch == ch) {
                pPl2 = pPl->next;
                pPl->next = pPl->next->next;
                free(pPl2);
                break;
            }
        }
    }

    ch->lifting = NULL;
}

int
get_lifting_capacity(PLYR_LIST * lifted_by)
{
    PLYR_LIST *pPl;
    int capacity = 0;

    for (pPl = lifted_by; pPl; pPl = pPl->next)
        if (pPl->ch)
            capacity += CAN_CARRY_W(pPl->ch)
                - calc_carried_weight_no_lift(pPl->ch);

    return capacity;
}

