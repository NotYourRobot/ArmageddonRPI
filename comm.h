/*
 * File: COMM.H
 * Usage: Prototypes for communication functions.
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

#ifndef __COMM_H__
#define __COMM_H__

#ifdef __cplusplus
extern "C"
{
#endif

    bool parse_emote_message(CHAR_DATA * ch, const char *arg, const bool use_name, char *msg,
                             size_t msg_len, struct obj_list_type **ch_list);

    void send_to_all(const char *messg);
    void send_to_char(const char *messg, CHAR_DATA * ch);
    void send_to_charf(CHAR_DATA * ch, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));

    bool send_how_to_char_parsed( CHAR_DATA *ch, CHAR_DATA *to_ch, char *to_char, char *preHow, char *postHow);
    bool send_to_char_and_room_parsed(CHAR_DATA * ch, char *preHow, char *postHow, char *to_char,
                                      char *to_room, char *to_monitor, int monitor_channel);
    bool send_hidden_to_char_vict_and_room_parsed(CHAR_DATA * ch, CHAR_DATA * vict, char *preHow, char *postHow, char *to_char, char *to_vict, char *to_room, char *to_monitor, int monitor_channel, int skillMod, int opposingSkill);
    bool send_hidden_to_char_and_room_parsed(CHAR_DATA * ch, char *preHow, char *postHow, char *to_char, char *to_room, char *to_monitor, int monitor_channel, int skillMod, int opposingSkill);
    bool send_to_char_vict_and_room_parsed(CHAR_DATA * ch, CHAR_DATA * vict, char *preHow, char *postHow, char *to_char, char *to_vict,
                                           char *to_room, char *to_monitor, int monitor_channel);
    bool send_to_char_parsed(CHAR_DATA * ch, CHAR_DATA * to_ch, char *arg, bool use_name);
    void send_to_room_parsed(CHAR_DATA * ch, ROOM_DATA * to_room, char *arg, bool show_invis);
    void send_hidden_to_room_parsed(CHAR_DATA * ch, ROOM_DATA * to_room, char *arg, bool show_invis, int skillMod, int opposingSkill);
    void send_to_room_ex_parsed(CHAR_DATA * ch, CHAR_DATA * vict, ROOM_DATA * to_room, char *arg, bool show_invis, bool show_hidden, int skillMod, int opposingSkill);

    void send_to_except(const char *messg, CHAR_DATA * ch);
    void send_to_room(const char *messg, struct room_data *room);
    void send_to_room_except(const char *messg, struct room_data *room, CHAR_DATA * ch);
    void send_to_room_except_two(const char *messg, struct room_data *room, CHAR_DATA * ch1,
                                 CHAR_DATA * ch2);
    void send_to_overlords(const char *messg);
    void connect_send_to_higher(const char *msg, int level, CHAR_DATA * ch);
    void send_to_higher(const char *msg, int level, CHAR_DATA * ch);
    void send_to_wagon(const char *msg, OBJ_DATA * wagon);
    void send_to_nearby_rooms(const char *messg, ROOM_DATA * start_rm);
    void send_translated(CHAR_DATA * ch, CHAR_DATA * tar_ch, const char *text_pure);

    void gprintf(CHAR_DATA * to_ch, struct room_data *to_rm, const char *fmt, ...);
    void gvprintf(CHAR_DATA * actor, CHAR_DATA * to_ch, struct room_data *to_rm, const char *fmt,
                  struct obj_list_type *args, bool watch_to_see, bool silent);
    void cprintf(CHAR_DATA * ch, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));

    void exhibit_emotion(CHAR_DATA * ch, const char *feeling);
#include "gamelog.h"

/* 
  I couldent find a definition for these fuctions - Tenebrius
  void perform_to_all(); 
  void perform_complex();
  void acthr();
  */
    extern void act(const char *str, int hide_invisible, CHAR_DATA * ch, void *obj, void *vict_obj,
                    int type);
    extern int write_to_descriptor(int desc, const char *txt);
    extern void write_to_q(const char *txt, struct txt_q *queue);

#define TO_ROOM    0
#define TO_VICT    1
#define TO_NOTVICT 2
#define TO_CHAR    3
#define TO_NEARBY_ROOMS 4
#define TO_WAGON   5
#define TO_PILOT   6
#define TO_NOTVICT_COMBAT 7

#define SEND_TO_Q(messg, desc)  write_to_q((messg), &(desc)->output)
    char *act_substitute(char *str, int hide_invisible, CHAR_DATA * ch, OBJ_DATA * obj,
                         void *vict_obj, int type, CHAR_DATA * to, char c, int *newline, int *cap);

    int apply_accent(char *buf, CHAR_DATA * ch, CHAR_DATA * lis);

    void learn_language(CHAR_DATA * ch, CHAR_DATA * lis, int language);
    int default_accent(CHAR_DATA * ch);

#define NEW_LANGUAGE_ODDS	25000
#define LANG_NO_ACCENT		0
#define MIN_ACCENT_LEVEL	60
#define MIN_LANG_LEVEL		70

#ifdef __cplusplus
}
#endif

#endif                          // __COMM_H__

