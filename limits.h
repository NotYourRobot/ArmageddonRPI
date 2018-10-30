/*
 * File: LIMITS.H
 * Usage: Prototypes and structures for limits.c.
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

/* Public Procedures */
int mana_limit(CHAR_DATA * ch);
int hit_limit(CHAR_DATA * ch);
int move_limit(CHAR_DATA * ch);
int stun_limit(CHAR_DATA * ch);

struct title_type
{
    char *title_m;
    char *title_f;
    int exp;
};

extern int xp_req[];



bool init_learned_skill(CHAR_DATA * ch, int sk, int perc);
void init_skill(CHAR_DATA * ch, int sk, int perc);
void init_psi_skills(CHAR_DATA * ch);
void init_racial_skills(CHAR_DATA * ch);
void init_saves(CHAR_DATA * ch);
void init_languages(CHAR_DATA * ch);
void delete_skill(CHAR_DATA * ch, int sk);

