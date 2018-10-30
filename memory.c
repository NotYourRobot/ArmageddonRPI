/*
 * File: MEMORY.C
 * Usage: Routines for NPC memory.
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

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "barter.h"
#include "db.h"
#include "clan.h"

void
memory_print_for(struct char_data *vict, char *mesg, int len, struct char_data *ch)
{
    char buff[256];
    struct memory_data *tmpmem;

    strcpy(mesg, "");
    if (!IS_NPC(vict) || IS_SET(vict->specials.act, CFL_UNIQUE))
        return;

    if (vict->specials.hates)
        strcat(mesg, "Hates\n\r");
    for (tmpmem = vict->specials.hates; tmpmem; tmpmem = tmpmem->next) {
        get_arg_char_world_vis(ch, tmpmem->remembers, buff);
        if ((strlen(buff) + strlen(mesg)) > len - 6) {
            strcpy(mesg, "To much in memory lists to print\n\r");
            return;
        } else {
            strcat(mesg, "  ");
            strcat(mesg, buff);
            strcat(mesg, "\n\r");
        };
    }
    if (vict->specials.fears)
        strcat(mesg, "Fears\n\r  ");
    for (tmpmem = vict->specials.fears; tmpmem; tmpmem = tmpmem->next) {
        get_arg_char_world_vis(ch, tmpmem->remembers, buff);
        if ((strlen(buff) + strlen(mesg)) > len - 6) {
            strcpy(mesg, "To much in memory lists to print\n\r");
            return;
        } else {
            strcat(mesg, "  ");
            strcat(mesg, buff);
            strcat(mesg, "\n\r");
        };
    }
    if (vict->specials.likes)
        strcat(mesg, "Likes\n\r  ");
    for (tmpmem = vict->specials.likes; tmpmem; tmpmem = tmpmem->next) {
        get_arg_char_world_vis(ch, tmpmem->remembers, buff);
        if ((strlen(buff) + strlen(mesg)) > len - 6) {
            strcpy(mesg, "To much in memory lists to print\n\r");
            return;
        } else {
            strcat(mesg, "  ");
            strcat(mesg, buff);
            strcat(mesg, "\n\r");
        };
    }
}

bool
does_hate(struct char_data *ch, struct char_data *mem)
{
    struct memory_data *tmpmem;

    if (!IS_NPC(ch) || IS_SET(ch->specials.act, CFL_UNIQUE))
        return FALSE;

    for (tmpmem = ch->specials.hates; tmpmem; tmpmem = tmpmem->next)
        if (tmpmem->remembers == mem)
            return TRUE;

    return FALSE;
}

bool
does_fear(struct char_data * ch, struct char_data * mem)
{
    struct memory_data *tmpmem;

    if (!IS_NPC(ch) || IS_SET(ch->specials.act, CFL_UNIQUE))
        return FALSE;

    for (tmpmem = ch->specials.fears; tmpmem; tmpmem = tmpmem->next)
        if (tmpmem->remembers == mem)
            return TRUE;
    return FALSE;
}

bool
does_like(struct char_data * ch, struct char_data * mem)
{
    struct memory_data *tmpmem;

    if (!IS_NPC(ch) || IS_SET(ch->specials.act, CFL_UNIQUE))
        return FALSE;

    for (tmpmem = ch->specials.likes; tmpmem; tmpmem = tmpmem->next)
        if (tmpmem->remembers == mem)
            return TRUE;
    return FALSE;
}

void
add_to_hates(struct char_data *ch, struct char_data *mem)
{
    struct memory_data *tmpmem;

    assert(!does_hate(ch, mem));

    if (ch == mem)
        return;

    /* Soldiers/templars have their own 'hates' -Morg */
    if (IS_TRIBE(ch, TRIBE_ALLANAK_MIL) || IS_TRIBE(ch, 8))
        return;

    CREATE(tmpmem, struct memory_data, 1);
    tmpmem->remembers = mem;

    tmpmem->next = ch->specials.hates;
    ch->specials.hates = tmpmem;
}

void
add_to_hates_raw(struct char_data *ch, struct char_data *mem)
{
    struct memory_data *tmpmem;

    CREATE(tmpmem, struct memory_data, 1);
    tmpmem->remembers = mem;

    tmpmem->next = ch->specials.hates;
    ch->specials.hates = tmpmem;
}

void
add_to_fears(struct char_data *ch, struct char_data *mem)
{
    struct memory_data *tmpmem;

    CREATE(tmpmem, struct memory_data, 1);
    tmpmem->remembers = mem;

    tmpmem->next = ch->specials.fears;
    ch->specials.fears = tmpmem;
}

void
add_to_likes(struct char_data *ch, struct char_data *mem)
{
    struct memory_data *tmpmem;

    CREATE(tmpmem, struct memory_data, 1);
    tmpmem->remembers = mem;

    tmpmem->next = ch->specials.likes;
    ch->specials.likes = tmpmem;
}

void
remove_from_hates(struct char_data *ch, struct char_data *mem)
{
    struct memory_data *tmpmem, *remove;

    if (!does_hate(ch, mem))
        return;

    if (ch->specials.hates->remembers == mem) {
        tmpmem = ch->specials.hates;
        ch->specials.hates = ch->specials.hates->next;

        free((char *) tmpmem);
    } else {
        for (tmpmem = ch->specials.hates; tmpmem->next->remembers != mem; tmpmem = tmpmem->next);
        remove = tmpmem->next;
        tmpmem->next = remove->next;

        free((char *) remove);
    }

    // Remove -ALL- of them (excuse the recursion, please)
    if (does_hate(ch, mem)) {
        remove_from_hates(ch, mem);
    }
}

void
remove_from_fears(struct char_data *ch, struct char_data *mem)
{
    struct memory_data *tmpmem, *remove;

    if (!does_fear(ch, mem))
        return;

    if (ch->specials.fears->remembers == mem) {
        tmpmem = ch->specials.fears;
        ch->specials.fears = ch->specials.fears->next;

        free((char *) tmpmem);
    } else {
        for (tmpmem = ch->specials.fears; tmpmem->next->remembers != mem; tmpmem = tmpmem->next);
        remove = tmpmem->next;
        tmpmem->next = remove->next;

        free((char *) remove);
    }
}

void
remove_from_likes(struct char_data *ch, struct char_data *mem)
{
    struct memory_data *tmpmem, *remove;

    if (!does_fear(ch, mem))
        return;

    if (ch->specials.fears->remembers == mem) {
        tmpmem = ch->specials.likes;
        ch->specials.likes = ch->specials.fears->next;

        free((char *) tmpmem);
    } else {
        for (tmpmem = ch->specials.likes; tmpmem->next->remembers != mem; tmpmem = tmpmem->next);
        remove = tmpmem->next;
        tmpmem->next = remove->next;

        free((char *) remove);
    }
}



void
remove_all_hates_fears(struct char_data *ch)
{
    struct memory_data *remove;

/* this caused an infinate loops when someone logged out
   and I assume something hated/feared her

   while ( ch->fears )
   remove_from_fears ( ch, ch->fears->remembers );
   while ( ch->hates )
   remove_from_hates ( ch, ch->hates->remembers );

   I think the following fixes the problem -Tenebrius
 */
    while (ch->specials.fears) {
        remove = ch->specials.fears;
        ch->specials.fears = ch->specials.fears->next;
        free((char *) remove);
    }
    while (ch->specials.hates) {
        remove = ch->specials.hates;
        ch->specials.hates = ch->specials.hates->next;
        free((char *) remove);
    }
    while (ch->specials.likes) {
        remove = ch->specials.likes;
        ch->specials.likes = ch->specials.likes->next;
        free((char *) remove);
    }
};




/*
 * add_to_no_barter - Merchant refuses to bargain with the person
 * Added 4/3/98 -Morg
 */
void
add_to_no_barter(struct shop_data *shop, struct char_data *mem, int dur)
{
    struct no_barter_data *tmpmem;

    CREATE(tmpmem, struct no_barter_data, 1);
    tmpmem->name = strdup(MSTR(mem, name));
    tmpmem->duration = dur;

    tmpmem->next = shop->no_barter;
    shop->no_barter = tmpmem;
}


/*
 * find_no_barter - Looks for a person's no_barter in a shop
 * Added 4/3/98 -Morg
 */
struct no_barter_data *
find_no_barter(struct shop_data *shop, struct char_data *mem)
{
    struct no_barter_data *tmp;

    for (tmp = shop->no_barter; tmp; tmp = tmp->next)
        if (!strcmp(MSTR(mem, name), tmp->name))
            break;

    return tmp;
}

void
remove_no_barter(struct shop_data *shop, struct no_barter_data *nb)
{
    struct no_barter_data *tmp;

    if (!shop || !shop->no_barter || !nb)
        return;

    if (shop->no_barter == nb)
        shop->no_barter = shop->no_barter->next;
    else
        for (tmp = shop->no_barter; tmp; tmp = tmp->next)
            if (tmp->next == nb)
                tmp->next = tmp->next->next;

    free(nb->name);
    free(nb);
    return;
}

