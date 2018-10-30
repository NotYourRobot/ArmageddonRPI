/*
 * File: SMELLS.C
 * Usage: Smells functions
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

#ifdef SMELLS

/*
 * new_smell_data()
 * allocate and initialize the space for a new smell
 */
SMELL_DATA *
new_smell_data()
{
    SMELL_DATA *pSmell;

    CREATE(pSmell, SMELL_DATA, 1);
    pSmell->type.index = -1;
    pSmell->type.description = NULL;
    pSmell->intensity = 0;
    pSmell->timer = -1;
    pSmell->next = NULL;
    return pSmell;
}


/*
 * free_smell_data()
 * dellocate the space used by a smell
 */
void
free_smell_data(SMELL_DATA * pSmell)
{
    if (!pSmell)
        return;

    if (pSmell->type.description)
        free(pSmell->type.description);

    free(pSmell);
}


/*
 * is_same_smell()
 * determines if two smells are of the same type
 */
bool
is_same_smell(SMELL_DATA * a, SMELL_DATA * b)
{
    if (!a || !b)
        return FALSE;

    if (a->type.index != -1 && a->type.index == b->type.index)
        return TRUE;

    if (!strcmp(a->type.description, b->type.description))
        return TRUE;

    return FALSE;
}


/*
 * has_smell()
 * checks list of smells a to see if it has a smell of type b in it
 */
SMELL_DATA *
has_smell(SMELL_DATA * a, SMELL_DATA * b)
{
    if (!a || !b)
        return NULL;

    for (; a; a = a->next)
        if (is_same_smell(a, b))
            return a;

    return NULL;
}


int
get_smell_intensity(SMELL_DATA * pList, SMELL_DATA * pSmell)
{
    SMELL_DATA *p;

    p = has_smell(pList, pSmell);
    return (p->intensity);
}


/*
 * get_accustomed_intensity()
 * get the intensity of the smell which the character is accustomed to
 */
int
get_accustomed_intensity(CHAR_DATA * ch, SMELL_DATA * pSmell)
{
    SMELL_DATA *pTmp;

    pTmp = has_smell(ch->accustomed_smells, pSmell);
    return (pTmp->intensity);
}


/* can_smell()
 * checks to see if a character can smell a particular type smell
 * returns the intensity of the maximum, plus 1% per 10% of other
 * of that type in the room
 */
int
can_smell(CHAR_DATA * ch, SMELL_DATA * pSmell)
{
    CHAR_DATA *rch;
    OBJ_DATA *obj, *in_obj;
    SMELL_DATA *fSmell = NULL;
    int max = 0;
    int intense_mod = 0;

    if (!ch || !pSmell)
        return FALSE;

    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
        if ((fSmell = has_smell(obj->smells, pSmell)) != NULL
            && (number(0, MAX_SMELL_INTENSITY) <= fSmell->intensity)) {
            max = MAX(max, fSmell->intensity);
            intense_mod += fSmell->intensity / 10;
        }

        for (in_obj = obj->contains; in_obj; in_obj = in_obj->next_content)
            if ((fSmell = has_smell(in_obj->smells, pSmell)) != NULL
                && (number(0, MAX_SMELL_INTENSITY) <= (fSmell->intensity / 2))) {
                max = MAX(max, fSmell->intensity / 2);
                intense_mod += fSmell->intensity / 10;
            }
    }

    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if ((fSmell = has_smell(rch->smells, pSmell)) != NULL
            && (number(0, MAX_SMELL_INTENSITY) <= fSmell->intensity)) {
            max = MAX(max, fSmell->intensity);
            intense_mod += fSmell->intensity / 10;
        }

        for (obj = rch->carrying; obj; obj = obj->next_content)
            if ((fSmell = has_smell(obj->smells, pSmell)) != NULL
                && (number(0, MAX_SMELL_INTENSITY) <= (fSmell->intensity * 3 / 4))) {
                max = MAX(max, fSmell->intensity * 3 / 4);
                intense_mod += fSmell->intensity / 10;
            }
    }

    if ((fSmell = has_smell(ch->in_room->smells, pSmell)) != NULL
        && (number(0, MAX_SMELL_INTENSITY) <= fSmell->intensity)) {
        max = MAX(max, fSmell->intensity);
        intense_mod += fSmell->intensity / 10;
    }

    /* maximum in area, + 1% per 10% of others in area - 10% of accustomed */
    return max + intense_mod - (get_accustomed_intensity(ch, pSmell) / 10);
}

/*
 * add_smell()
 * adds a smell into a smell list, adding intensity & timers if it already
 * exists in the list, otherwise adds to the list
 */
void
add_smell(SMELL_DATA ** pList, SMELL_DATA * pSmell)
{
    SMELL_DATA *fSmell;

    if (!pSmell)
        return;

    /* if the smell is in the list, add intensities and timers */
    if ((fSmell = has_smell(*pList, pSmell)) != NULL) {
        fSmell->intensity += MAX(1, (pSmell->intensity / 10));

        /* add 1/10 of the new intensity to the timer */
        fSmell->timer += MAX(1, (fSmell->intensity / 10));
    } else {
        /* otherwise not found, just add */
        fSmell = new_smell_data();
        fSmell->type.index = pSmell->type.index;

        if (pSmell->type.description)
            fSmell->type.description = strdup(pSmell->type.description);

        fSmell->intensity = MAX(1, (pSmell->intensity / 10));
        /* use 1/10th of the intensity as the timer */
        fSmell->timer = MAX(1, (fSmell->intensity / 10));
        *pList = fSmell;
    }

    return;
}





const char *smell_intensity[10] = {
    "trace scent ",
    "faint scent ",
    "weak scent ",
    "mild smell ",
    "certain smell ",
    "smell ",
    "noticeable odor ",
    "strong odor ",
    "very strong odor ",
    "overwhelming odor "
};

const char *smell_area[10] = {
    "tickles your nose",
    "lingers in the air",
    "wafts through the air",
    "floats in the air",
    "drifts in the air",
    "can be sensed in the area",
    "lies in the air",
    "colors the area",
    "lays thick in the air",
    "fills the area",
};


/*
 * smell_echo()
 * echo a list of smells to a person
 */
void
smell_echo(CHAR_DATA * ch, SMELL_DATA * pList)
{
    SMELL_DATA *pSmell;
    char buf[MAX_STRING_LENGTH];

    for (pSmell = pList; pSmell; pSmell = pSmell->next) {
        int adj_intensity = pSmell->intensity - get_accustomed_intensity(ch, pSmell);

        if (adj_intensity > 0 && number(0, MAX_SMELL_INTENSITY - adj_intensity) == 0) {
            int intensity_index = adj_intensity * 10 / MAX_SMELL_INTENSITY;

            sprintf(buf, "The %sof %s %s.\n\r", smell_intensity[intensity_index],
                    pSmell->type.index ==
                    -1 ? pSmell->type.description : smells_table[pSmell->type.index].description,
                    smell_area[intensity_index]);

            send_to_char(buf, ch);
        }
    }
}



/*
 * Gathers a list of smells a person can smell, and sends them to them
 */
void
smell_random_echoes(CHAR_DATA * ch)
{
    CHAR_DATA *rch;
    OBJ_DATA *obj, *in_obj;
    int loc;
    SMELL_DATA *pSmell;         /* current smell */
    SMELL_DATA *pSmelt = NULL;  /* list of smells in the area */
    SMELL_DATA *fSmell;         /* smell found in area smells */

    /* smells on the character */
    for (pSmell = ch->smells; pSmell; pSmell = pSmell->next) {
        if ((fSmell = has_smell(pSmelt, pSmell)) != NULL)
            fSmell->intensity += pSmell->intensity / 10;
        else {
            fSmell = new_smell_data();
            fSmell->type.index = pSmell->type.index;
            if (pSmell->type.description)
                fSmell->type.description = strdup(pSmell->type.description);
            fSmell->intensity = pSmell->intensity;
            fSmell->next = pSmelt;
            pSmelt = fSmell;
        }
    }

    /* smells on objects he's carrying */
    for (obj = ch->carrying; obj; obj = obj->next_content)
        for (pSmell = obj->smells; pSmell; pSmell = pSmell->next) {
            if ((fSmell = has_smell(pSmelt, pSmell)) != NULL)
                fSmell->intensity += pSmell->intensity * 3 / 4;
            else {
                fSmell = new_smell_data();
                fSmell->type.index = pSmell->type.index;
                if (pSmell->type.description)
                    fSmell->type.description = strdup(pSmell->type.description);
                fSmell->intensity = pSmell->intensity * 3 / 4;
                fSmell->next = pSmelt;
                pSmelt = fSmell;
            }
        }

    for (loc = 0; loc < MAX_WEAR; loc++)
        if (ch->equipment[loc])
            for (pSmell = ch->equipment[loc]->smells; pSmell; pSmell = pSmell->next) {
                if ((fSmell = has_smell(pSmelt, pSmell)) != NULL)
                    fSmell->intensity += pSmell->intensity * 3 / 4;
                else {
                    fSmell = new_smell_data();
                    fSmell->type.index = pSmell->type.index;
                    if (pSmell->type.description)
                        fSmell->type.description = strdup(pSmell->type.description);
                    fSmell->intensity = pSmell->intensity * 3 / 4;
                    fSmell->next = pSmelt;
                    pSmelt = fSmell;
                }
            }


    /* smells in the room */
    for (pSmell = ch->in_room->smells; pSmell; pSmell = pSmell->next) {
        if ((fSmell = has_smell(pSmelt, pSmell)) != NULL)
            fSmell->intensity += pSmell->intensity / 10;
        else {
            fSmell = new_smell_data();
            fSmell->type.index = pSmell->type.index;
            if (pSmell->type.description)
                fSmell->type.description = strdup(pSmell->type.description);
            fSmell->intensity = pSmell->intensity;
            fSmell->next = pSmelt;
            pSmelt = fSmell;
        }
    }

    /* smells on objects in the room */
    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
        for (pSmell = obj->smells; pSmell; pSmell = pSmell->next) {
            if ((fSmell = has_smell(pSmelt, pSmell)) != NULL)
                fSmell->intensity += pSmell->intensity / 10;
            else {
                fSmell = new_smell_data();
                fSmell->type.index = pSmell->type.index;
                if (pSmell->type.description)
                    fSmell->type.description = strdup(pSmell->type.description);
                fSmell->intensity = pSmell->intensity / 10;
                fSmell->next = pSmelt;
                pSmelt = fSmell;
            }
        }

        /* smells on objects in objects in the room */
        for (in_obj = obj->contains; in_obj; in_obj = in_obj->next_content)
            for (pSmell = in_obj->smells; pSmell; pSmell = pSmell->next) {
                if ((fSmell = has_smell(pSmelt, pSmell)) != NULL)
                    fSmell->intensity += pSmell->intensity / 2;
                else {
                    fSmell = new_smell_data();
                    fSmell->type.index = pSmell->type.index;
                    if (pSmell->type.description)
                        fSmell->type.description = strdup(pSmell->type.description);
                    fSmell->intensity = pSmell->intensity / 2;
                    fSmell->next = pSmelt;
                    pSmelt = fSmell;
                }
            }
    }

    /* smells on people in the room */
    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        for (pSmell = rch->smells; pSmell; pSmell = pSmell->next) {
            if ((fSmell = has_smell(pSmelt, pSmell)) != NULL)
                fSmell->intensity += pSmell->intensity / 10;
            else {
                fSmell = new_smell_data();
                fSmell->type.index = pSmell->type.index;
                if (pSmell->type.description)
                    fSmell->type.description = strdup(pSmell->type.description);
                fSmell->intensity = pSmell->intensity;
                fSmell->next = pSmelt;
                pSmelt = fSmell;
            }
        }

        for (obj = rch->carrying; obj; obj = obj->next_content)
            for (pSmell = obj->smells; pSmell; pSmell = pSmell->next) {
                if ((fSmell = has_smell(pSmelt, pSmell)) != NULL)
                    fSmell->intensity += pSmell->intensity * 3 / 4;
                else {
                    fSmell = new_smell_data();
                    fSmell->type.index = pSmell->type.index;
                    if (pSmell->type.description)
                        fSmell->type.description = strdup(pSmell->type.description);
                    fSmell->intensity = pSmell->intensity * 3 / 4;
                    fSmell->next = pSmelt;
                    pSmelt = fSmell;
                }
            }

        for (loc = 0; loc < MAX_WEAR; loc++)
            if (rch->equipment[loc])
                for (pSmell = rch->equipment[loc]->smells; pSmell; pSmell = pSmell->next) {
                    if ((fSmell = has_smell(pSmelt, pSmell)) != NULL)
                        fSmell->intensity += pSmell->intensity * 3 / 4;
                    else {
                        fSmell = new_smell_data();
                        fSmell->type.index = pSmell->type.index;
                        if (pSmell->type.description)
                            fSmell->type.description = strdup(pSmell->type.description);
                        fSmell->intensity = pSmell->intensity * 3 / 4;
                        fSmell->next = pSmelt;
                        pSmelt = fSmell;
                    }
                }


    }

    smell_echo(ch, pSmelt);

    for (pSmell = pSmelt; pSmell; pSmell = fSmell) {
        fSmell = pSmell->next;
        free_smell_data(pSmell);
    }
}


/*
 * smell_intensity_update()
 * updates the intensities of all smells in the list
 */
void
smell_intensity_update(SMELL_DATA ** pList)
{
    SMELL_DATA *pSmell, *pSmell_next, *pSmell_last;

    if (!pList || !*pList)
        return;

    for (pSmell = *pList; pSmell; pSmell = pSmell_next) {
        pSmell_next = pSmell->next;

        pSmell->intensity = MIN(pSmell->intensity, MAX_SMELL_INTENSITY);
        pSmell->intensity = MAX(pSmell->intensity, 0);

        /* chance for drop in intensity */
        if (pSmell->timer != -1 && pSmell->timer <= 0) {
            pSmell->intensity--;
            pSmell->timer = pSmell->intensity;

            if (pSmell->intensity <= 0) {
                if (pSmell_last)
                    pSmell_last->next = pSmell->next;
                free_smell_data(pSmell);
            } else
                pSmell_last = pSmell;
        } else
            pSmell_last = pSmell;
    }
}


void
smell_rub_off(SMELL_DATA ** target, SMELL_DATA * source)
{
    SMELL_DATA *pSmell;

    for (pSmell = source; pSmell; pSmell = pSmell->next) {
        if (pSmell->intensity >= SMELL_NOTICEABLE) {
            add_smell(target, pSmell);
        }
    }
}

/* 
 * smell_accustomed_intensity_update()
 * moves a characters accustomed smells up and down as they are around
 * or not around the source of them
 */
void
smell_accustomed_intensity_update(CHAR_DATA * ch)
{
    SMELL_DATA *pSmell, *pSmell_next, *pSmell_last;
    int can_smell_intensity;

    for (pSmell = ch->accustomed_smells; pSmell; pSmell = pSmell_next) {
        pSmell_next = pSmell->next;

        pSmell->intensity = MIN(pSmell->intensity, MAX_SMELL_INTENSITY);
        pSmell->intensity = MAX(pSmell->intensity, 0);

        can_smell_intensity = can_smell(ch, pSmell);

        /* chance for drop in intensity */
        if (can_smell_intensity < pSmell->intensity && pSmell->timer != -1 && pSmell->timer-- <= 0) {
            pSmell->intensity--;
            pSmell->timer = pSmell->intensity;

            if (pSmell->intensity <= 0) {
                if (pSmell_last)
                    pSmell_last->next = pSmell->next;
                free_smell_data(pSmell);
            } else
                pSmell_last = pSmell;
        } else if (can_smell_intensity >= pSmell->intensity && pSmell->timer != -1
                   && pSmell->timer++ >= pSmell->intensity) {
            pSmell->intensity++;
            pSmell->timer = pSmell->intensity / 2;
            pSmell_last = pSmell;
        }
    }
}


void
char_smell_update(CHAR_DATA * ch)
{
    int loc;
    OBJ_DATA *obj;

    /* check the intensities of the smells on them */
    smell_intensity_update(&ch->smells);

    /* check the intensities of everything they're carrying (inventory) */
    for (obj = ch->carrying; obj; obj = obj->next_content)
        if (obj->smells)
            smell_intensity_update(&obj->smells);

    /* check the intensities of everything they're wearing (equipment) */
    for (loc = 0; loc < MAX_WEAR; loc++)
        if (ch->equipment[loc] && ch->equipment[loc]->smells)
            smell_intensity_update(&ch->equipment[loc]->smells);

    /* rub the smells off onto the accustomed smells */
    smell_rub_off(&ch->accustomed_smells, ch->in_room->smells);
    smell_rub_off(&ch->accustomed_smells, ch->smells);

    for (obj = ch->carrying; obj; obj = obj->next_content)
        if (obj->smells)
            smell_rub_off(&ch->accustomed_smells, obj->smells);

    for (loc = 0; loc < MAX_WEAR; loc++)
        if (ch->equipment[loc] && ch->equipment[loc]->smells)
            smell_rub_off(&ch->accustomed_smells, ch->equipment[loc]->smells);


    /* rub off the smells into our smells */
    smell_rub_off(&ch->smells, ch->in_room->smells);

/* why would your own smells rub off on yourself?
   smell_rub_off( &ch->smells, ch->smells ); */

    for (obj = ch->carrying; obj; obj = obj->next_content)
        if (obj->smells)
            smell_rub_off(&ch->smells, obj->smells);

    for (loc = 0; loc < MAX_WEAR; loc++)
        if (ch->equipment[loc] && ch->equipment[loc]->smells)
            smell_rub_off(&ch->smells, ch->equipment[loc]->smells);


    /* rub off our smell and the room we're in onto what we're wearing */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        smell_rub_off(&ch->equipment[loc]->smells, ch->smells);
        smell_rub_off(&ch->equipment[loc]->smells, ch->in_room->smells);
    }

    for (loc = 0; loc < MAX_WEAR; loc++)
        if (ch->equipment[loc]) {
            smell_rub_off(&ch->equipment[loc]->smells, ch->smells);
            smell_rub_off(&ch->equipment[loc]->smells, ch->in_room->smells);
        }

    /* check the intensities of the smells they're accustomed to */
    smell_accustomed_intensity_update(ch);

    smell_random_echoes(ch);
}

void
room_smell_update(ROOM_DATA * room)
{
    OBJ_DATA *obj;
    CHAR_DATA *rch;

    /* check the intensities of the smells on them */
    smell_intensity_update(&room->smells);

    /* check the intensities of everything in the room (objs) */
    for (obj = room->contents; obj; obj = obj->next_content) {
        if (obj->smells)
            smell_intensity_update(&obj->smells);

        /* rub off our smells into their smells */
        smell_rub_off(&obj->smells, room->smells);
    }

    /* rub off the smells into our smells of people in the room */
    for (rch = room->people; rch; rch = rch->next_in_room) {
        smell_rub_off(&room->smells, rch->smells);
    }
}


int
lookup_smell(char *name)
{
    int i;

    for (i = 0; smells_table[i].name[0] != '\0'; i++)
        if (!str_prefix(name, smells_table[i].name))
            return i;

    return -1;
}

#endif

