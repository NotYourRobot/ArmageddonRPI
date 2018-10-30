/*
 * File: CREATION.H
 * Usage: Predefs for rcreation.c, ocreation.c, and mcreation.c
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

/* external variables */


/* external functions */
extern void obj_update(struct obj_data *obj);
extern void npc_update(struct char_data *mob);
extern void save_objects(int zone);
extern void save_mobiles(int zone);
extern void save_zone(struct zone_data *zdata, int zone_num);
extern void update_shopkeepers(int nr);
extern void update_shopkeepers_d(int nr);
extern void update_producing(int nr);
extern void update_producing_d(int nr);
extern int alloc_obj(int nr, int init, int allow_renum);
extern int alloc_npc(int nr, int init, int allow_renum);

void display_specials(char *buf, size_t bufsize, struct specials_t *specials, char stat);
struct specials_t *set_prog(CHAR_DATA *ch, struct specials_t *specials, const char *args);
struct specials_t *unset_prog(CHAR_DATA *ch, struct specials_t *specials, const char *args);

/* creation limits */

#define MIN_DAMAGE	-20
#define MAX_DAMAGE	20
#define MIN_HITS	-11
#define MAX_HITS	2000
#define MIN_STUN    0
#define MAX_STUN    2000
#define MIN_ARMOR	0
#define MAX_ARMOR	100
#define MIN_OFF		-100
#define MAX_OFF		100
#define MIN_DEF		-100
#define MAX_DEF		100
#define MIN_OBSIDIAN	0
#define MIN_ABILITY	1
#define MAX_ABILITY	100
#define MAX_STRENGTH    100
#define MAX_SDESC	79
#define MAX_LDESC	79
#define MAX_DESC	2000
#define MIN_HEIGHT	0
#define MIN_WEIGHT	0
#define MIN_MANA	0
#define MIN_MOVE	0
#define MIN_CONDITION	-1
#define MAX_CONDITION	48
#define MIN_LIFE	-100
#define MAX_LIFE	100
#define MAX_POOF	79
#define MIN_SKILL_P	0
#define MAX_SKILL_P	100

#define MAX_OVALS	6

