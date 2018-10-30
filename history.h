/*
 * File: HISTORY.H
 * Usage: History / Accounting definition
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

#ifndef __HISTORY_INCLUDED
#define __HISTORY_INCLUDED

/* Uncomment this line to stop recording login history */
#define RECORD_HISTORY

typedef struct history_info HISTORY_INFO;

struct history_info
{
    char *site;                 /* Site logged in from */
    int logins;                 /* Times logged in from this site */
    HISTORY_INFO *next;         /* Next login site in list */
};

/* Reads in a history file for an account */
HISTORY_INFO *read_history(PLAYER_INFO * pPInfo);

/* Write out a history file for an account */
void write_history(PLAYER_INFO * pPInfo, HISTORY_INFO * pHistory);

/* Updates an account's history */
void update_history(PLAYER_INFO * pPInfo, char *site);

#endif

