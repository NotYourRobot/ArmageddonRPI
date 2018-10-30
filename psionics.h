/*
 * File: PSIONICS.H
 * Usage: Definitions for psionic skills and utilities.
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

#ifndef __PSIONICS_INCLUDED
#define __PSIONICS_INCLUDED

// Disabled abilties get stubbed out here
#define DISABLE_CLAIRVOYANCE
#define DISABLE_COERCE
#define DISABLE_IMITATE
#define DISABLE_MESMERIZE

/* Psionic Listeners List */
#define AREA_NONE       0
#define AREA_ROOM	1
#define AREA_ZONE	2
#define AREA_WORLD	3

/* Psionics Vanish Ranges */
#define VANISH_ROOM     0
#define VANISH_NEAR     1
#define VANISH_FAR      2
#define VANISH_VERY_FAR 3

bool blocked_psionics(CHAR_DATA * pCh, CHAR_DATA * tCh);
bool is_detected_psi(CHAR_DATA * pCh, CHAR_DATA * tCh);
void aggressive_psionics(CHAR_DATA * pCh);
bool is_psionic_command(int cmd);
bool is_in_psionic_range(CHAR_DATA * pCh, CHAR_DATA * tCh, int range);
int cease_all_psionics(CHAR_DATA * ch, bool echo);
int cease_psionic_skill(CHAR_DATA * ch, int sk, bool echo);
void clairaudient_listener(CHAR_DATA * ch, CHAR_DATA * tar_ch, int track, const char *mesg);
void mantis_commune(CHAR_DATA * ch, const char *arg, Command cmd, int count);
bool psi_skill_success(CHAR_DATA * pCh, CHAR_DATA * tCh, int sk, int bonus);
void remove_contact(CHAR_DATA * ch);
void send_comm_to_clairsentient(CHAR_DATA * lis, CHAR_DATA * ch, const char *how, const char *verb,
                                const char *exclaimVerb, const char *askVerb, const char *output,
                                bool seen);
void send_comm_to_clairsentients(CHAR_DATA * lis, CHAR_DATA * ch, CHAR_DATA * tch, const char *how,
                                 const char *verb, const char *exclaimVerb, const char *askVerb,
                                 const char *output);
void send_emotion_to_immersed(CHAR_DATA * ch, const char *msg);
void send_thought_to_immersed(CHAR_DATA * ch, const char *msg);
void send_to_empaths(CHAR_DATA * ch, const char *msg);
void send_thought_to_listeners(CHAR_DATA * ch, const char *msg);
int update_psionics(CHAR_DATA * ch);
int remove_psionics(CHAR_DATA * ch);
void make_contacts();
bool standard_drain(CHAR_DATA * ch, int sk);
int parse_psionic_command(CHAR_DATA *ch, const char *cmd);
#endif

