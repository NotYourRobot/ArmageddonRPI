/*
 * File: NPC.H
 * Usage: Structs and def's for npc intelligence.
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

struct npc_delay_data
{
    struct char_data *guy;      /* npc who is waiting to act */
    int command;                /* command waiting to be done */
    char *argument;             /* argument to the command */
    int timeout;                /* time till npc does his thing */
    struct npc_delay_data *next;        /* next in line */
};


#define COM_BACKSTAB  153       /* backstab someone */
#define COM_CAST       83       /* cast a spell */
#define COM_STEAL     155       /* steal some stuff */
#define COM_PICK      154       /* pick locks */
#define COM_HIT        70       /* attack */

