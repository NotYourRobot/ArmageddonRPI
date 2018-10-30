/*
 * File: DELAY.H
 * Usage: Prototypes and structures for delay.c.
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

#ifndef __DELAY_INCLUDED
#define __DELAY_INCLUDED

#define MAX_DELAY         175

struct delayed_com_data
{
    struct char_data *character;        /* person who's doing delayed com */
    char *argument;             /* argument to the command */
    int command;                /* command number */
    int timeout;                /* how long to wait */
    int count;                  /* run counter for repeatable commands */
    char protect1[16];
    struct delayed_com_data *next;      /* next in line */
    char protect2[16];
};

bool is_in_command_q(CHAR_DATA * ch);
void rem_from_command_q(CHAR_DATA * ch);

#endif

