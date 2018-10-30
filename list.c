/*
 * File: LIST.C
 * Usage: Listing of players.
 * Copyright (C) 1993 by Dan Brumleve and the creators of DikuMUD.
 *
 * Part of:
 *   /\    ____                                     __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 *
 * Unauthorized distribution of this source code is punishable by 
 * eternal damnation to the hell pits of Suk-Krath.  Have a nice day.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "guilds.h"
#include "db.h"
#include "comm.h"


void
cmd_cwho(struct char_data *ch, const char *argument, int cmd)
{
    int clan;

    if (!(clan = ch->player.tribe)) {
        send_to_char("You aren't even in a clan!\n\r", ch);
        return;
    }
    return;
}

