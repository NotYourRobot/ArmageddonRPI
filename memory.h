/*
 * File: MEMORY.H
 * Usage: interface for NPC memory.
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

#ifndef __INCLUDED_MEMORY
#define __INCLUDED_MEMORY

#include "structs.h"
#include "utils.h"
#include "barter.h"

bool does_hate(struct char_data *ch, struct char_data *mem);
bool does_fear(struct char_data *ch, struct char_data *mem);
bool does_like(struct char_data *ch, struct char_data *mem);
void add_to_hates(struct char_data *ch, struct char_data *mem);
void add_to_hates_raw(struct char_data *ch, struct char_data *mem);
void add_to_fears(struct char_data *ch, struct char_data *mem);
void add_to_likes(struct char_data *ch, struct char_data *mem);
void remove_from_hates(struct char_data *ch, struct char_data *mem);
void remove_from_fears(struct char_data *ch, struct char_data *mem);
void remove_from_likes(struct char_data *ch, struct char_data *mem);
void remove_all_hates_fears(struct char_data *ch);

void add_to_no_barter(struct shop_data *shop, struct char_data *mem, int d);
struct no_barter_data *find_no_barter(struct shop_data *shop, struct char_data *mem);
void remove_no_barter(struct shop_data *shop, struct no_barter_data *mem);

#endif

