/*
 * File: EVENT.H
 * Usage: Definitions for event handling.
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

#define EVNT_DELT                  0
#define EVNT_SAVE                  1
#define EVNT_MOVE                  2
#define EVNT_NPC_PULSE             3
#define EVNT_OBJ_PULSE             4
#define EVNT_ROOM_PULSE            5
#define EVNT_ATTACK                6
#define EVNT_REST                  7
#define EVNT_GAIN                  8
#define EVNT_REMOVE_SANDWALL       9
#define EVNT_REMOVE_SHADE         10
#define EVNT_SHUTDOWN             11
#define EVNT_WEATHER              12
#define EVNT_REMOVE_OBJECT        13
#define EVNT_REMOVE_MOBILE        14
#define EVNT_REWIND               15
#define EVNT_REMOVE_ALARM         16
#define EVNT_FIRE_BURN            17
#define EVNT_REMOVE_DARK          18
#define EVNT_REMOVE_NOELEMMAGICK  19
#define EVNT_DETONATE             20
#define EVNT_REMOVE_FIREWALL      21
#define EVNT_REMOVE_WINDWALL      22
#define EVNT_REMOVE_BLADEBARRIER  23
#define EVNT_REMOVE_THORNWALL     24
#define EVNT_FIREJAMBIYA_REMOVE   25
#define EVNT_SANDJAMBIYA_REMOVE   26
#define EVNT_LIGHTNING_REMOVE     27
#define EVNT_SHADOWSWORD_REMOVE   28
#define EVNT_WITHER_REMOVE        29
#define EVNT_GOLEM_REMOVE         30
#define EVNT_SHIELDOFMIST_REMOVE  31
#define EVNT_SHIELDOFWIND_REMOVE  32
#define EVNT_HEALINGMUD_REMOVE    33
#define EVNT_FIRESEED_REMOVE      34
#define EVNT_REMOVE_ASH           35
#define EVNT_VAMPIRICBLADE_REMOVE 36
#define EVNT_REMOVE_ROOM          37
#define MAX_EVENTS 38

extern char *event_names[MAX_EVENTS];

#define NPC_MOVE
#define NPC_REST
#define OBJ_PULSE
#define ROOM_PULSE

#define SAVE_DELAY    1200

struct heap_event
{
    byte type;
    int time;

    struct char_data *ch;
    /*  struct char_data *tar_ch; */
    struct obj_data *tar_obj;
    struct room_data *room;
    int extra;
};

/* comparing functions */
int event_ch_tar_ch_cmp(void *ch, struct heap_event *evnt);
int event_obj_cmp(void *obj, struct heap_event *evnt);
int event_all(void *null, struct heap_event *evnt);
int event_room_cmp(void *obj, struct heap_event *evnt);
int event_shutdown_cmp(void *obj, struct heap_event *evnt);

/* interface commands */
void new_event(byte type, int time, struct char_data *ch, struct char_data *tar_ch,
               struct obj_data *tar_obj, struct room_data *room, int extra);

bool heap_exists(void *prt, int (*compar) (void *, struct heap_event *));
void heap_delete(void *prt, int (*compar) (void *, struct heap_event *));
void heap_insert(struct heap_event *event);

void heap_print(char *buf, void *ptr, int (*compar)
                  (void *, struct heap_event *));


void heap_update(int time_step);
void unboot_heap(void);

