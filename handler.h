/*
 * File: HANDLER.H
 * Usage: Prototypes for handler functions.
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __HANDLER_H__
#define __HANDLER_H__

static const int RESERVED_Z = 73;

OBJ_DATA *get_tool(CHAR_DATA *ch, int tool_type);
int get_raw_skill(CHAR_DATA *ch, int sk_num);
void affect_total(CHAR_DATA * ch);
struct affected_type *affected_by_spell(CHAR_DATA * ch, int spell);
void affect_join(CHAR_DATA * ch, struct affected_type *af, bool avg_dur, bool avg_mod);
void affect_remove(CHAR_DATA * ch, struct affected_type *af);

int affected_by_type_in_location(CHAR_DATA * ch, int type, int loc);
int find_apply_modifier_from_location(CHAR_DATA * ch, int type, int loc);
int find_apply_duration_from_location(CHAR_DATA * ch, int type, int loc);
int find_apply_power_from_location(CHAR_DATA * ch, int type, int loc);
int affected_by_type_in_bitvector(CHAR_DATA * ch, int type, int bvt);
struct affected_type *affected_by_spice(CHAR_DATA * ch, int spice);

/* utility */
OBJ_DATA *create_money(int amount, int type);
int isname(const char *str, const char *namelist);
int isallname(const char *str, const char *namelist);
int isnameprefix(const char *str, const char *namelist);
int isnamebracketsok(const char *str, const char *namelist);
int isallnamebracketsok(const char *str, const char *namelist);
int isanynamebracketsok(const char *str, const char *namelist);
char *first_name(const char *namelist, char *into);
void set_reins_pos(OBJ_DATA * wagon, int pos);
bool get_reins_pos(OBJ_DATA * wagon);
int calc_wagon_weight(OBJ_DATA * wagon);
int calc_wagon_weight_to_beasts(OBJ_DATA * wagon, int weight);
int calc_wagon_weight_empty(OBJ_DATA * wagon);

/* ******** objects *********** */

void obj_to_char(OBJ_DATA * object, CHAR_DATA * ch);
void obj_from_char(OBJ_DATA * object);

void equip_char(CHAR_DATA * ch, OBJ_DATA * obj, int pos);
OBJ_DATA *unequip_char(CHAR_DATA * ch, int pos);
OBJ_DATA *get_obj_equipped(CHAR_DATA * ch, const char *name, int vis);
OBJ_DATA *get_obj_in_list(CHAR_DATA *ch, const char *name, OBJ_DATA * list);
OBJ_DATA *get_obj_in_list_num(int num, OBJ_DATA * list);
OBJ_DATA *get_obj(const char *name);
OBJ_DATA *get_obj_num(int nr);
OBJ_DATA *get_obj_zone(int zone_num, const char *name);
OBJ_DATA *get_obj_keyword_vnum(const char *key, int vnum);
OBJ_DATA *get_obj_num_offset(int nr, int offset);
OBJ_DATA *get_obj_zone_num_offset(int zone, int nr, int offset);
void obj_to_room(OBJ_DATA * object, ROOM_DATA * room);
void obj_from_room(OBJ_DATA * object);
void obj_to_obj(OBJ_DATA * obj, OBJ_DATA * obj_to);
void obj_from_obj(OBJ_DATA * obj);
void obj_around_obj(OBJ_DATA * obj, OBJ_DATA * obj_to);
void obj_from_around_obj(OBJ_DATA * obj);
void object_list_new_owner(OBJ_DATA * list, CHAR_DATA * ch);

OBJ_DATA *find_artifact_cache(ROOM_DATA * room);
OBJ_DATA *create_artifact_cache(ROOM_DATA * room);
void obj_to_artifact_cache(OBJ_DATA * object, ROOM_DATA * room);

struct weather_node *get_weather_node_room(ROOM_DATA * rm);

/* ******* characters ********* */
CHAR_DATA *get_char(const char *name);
CHAR_DATA *get_char_num(int nr);
CHAR_DATA *get_char_pc_account(const char *name, const char *account);
CHAR_DATA *get_char_pc(const char *name);
CHAR_DATA *get_char_room(const char *name, ROOM_DATA * room);
CHAR_DATA *get_char_other_room_vis(CHAR_DATA *ch, ROOM_DATA *room, const char *name);
CHAR_DATA *get_char_room_num(int num, ROOM_DATA * room);
CHAR_DATA *get_char_vnum(int vnum);
CHAR_DATA *get_char_keyword_vnum(const char *key, int vnum);
CHAR_DATA *get_char_num_offset(int vnum, int offset);
CHAR_DATA *get_char_zone_num_offset(int zone, int vnum, int offset);
int mana_limit(CHAR_DATA * ch);

void char_from_room(CHAR_DATA * ch);
void char_to_room(CHAR_DATA * ch, ROOM_DATA * room);

/* base char to/from room functions */
bool remove_char_from_room(CHAR_DATA * ch);
bool add_char_to_room(CHAR_DATA * ch, ROOM_DATA * room);


/* find if character can see */
CHAR_DATA *get_char_room_vis(CHAR_DATA * ch, const char *name);
CHAR_DATA *get_char_vis(CHAR_DATA * ch, const char *name);
CHAR_DATA *get_char_world(CHAR_DATA * ch, const char *name);
CHAR_DATA *get_char_world_raw(CHAR_DATA * ch, const char *name, bool useCurrSdesc);
CHAR_DATA *get_char_world_all_name(CHAR_DATA * ch, const char *name);
CHAR_DATA *get_char_world_all_name_raw(CHAR_DATA * ch, const char *name, bool useCurrSdesc);
CHAR_DATA *get_char_zone(int zone, const char *name);
CHAR_DATA *get_char_zone_vis(int zone, CHAR_DATA * ch, const char *name);
CHAR_DATA *get_char_zone_num(int zone, int num);

bool check_char_name_world_raw(CHAR_DATA * ch, CHAR_DATA * tar_ch, const char *arg, bool useCurrSdesc);
bool check_char_name_world_all_name_raw(CHAR_DATA * ch, CHAR_DATA * tar_ch, char *arg, bool useCurrSdesc);



OBJ_DATA *get_obj_in_list_vis(CHAR_DATA * ch, const char *name, OBJ_DATA * list);
OBJ_DATA *get_obj_room_vis(CHAR_DATA * ch, const char *name);
OBJ_DATA *get_obj_vis(CHAR_DATA * ch, const char *name);

void extract_char(CHAR_DATA * ch);

/* rooms */
void extract_room(ROOM_DATA * room, int fixup_links);
ROOM_DATA *get_room_distance(CHAR_DATA * ch, int dist, int dir);
ROOM_DATA *get_wagon_entrance_room(OBJ_DATA * wagon);

/* Generic Find */
int generic_find(const char *arg, int bitvector, CHAR_DATA * ch, CHAR_DATA ** tar_ch,
                 OBJ_DATA ** tar_obj);

int find_ch_keyword(CHAR_DATA * tar_ch, CHAR_DATA * ch, char *keyword, size_t keyword_len);
int find_obj_keyword(OBJ_DATA * tar_obj, int bitvector, CHAR_DATA * ch, char *keyword,
                     size_t keyword_len);
void show_keyword_obj_to_char(CHAR_DATA * ch, int count, const char *keyword, OBJ_DATA * obj);
void show_keyword_char_to_char(CHAR_DATA * ch, int count, const char *keyword, CHAR_DATA * vict);
void show_keyword_found(const char *keyword, int bitvector, CHAR_DATA * ch);

#define FIND_CHAR_ROOM          1
#define FIND_CHAR_WORLD         2
#define FIND_OBJ_INV            4
#define FIND_OBJ_ROOM           8
#define FIND_OBJ_WORLD         16
#define FIND_OBJ_EQUIP         32
#define FIND_LIQ_CONT_INV      64
#define FIND_LIQ_CONT_ROOM    128
#define FIND_LIQ_CONT_EQUIP   256
#define FIND_LIQ_ROOM         512
#define FIND_OBJ_ROOM_ON_OBJ 1024

/* falling room stuff */
bool is_char_falling(CHAR_DATA * ch);
void add_char_to_falling(CHAR_DATA * ch);
void remove_char_from_falling(CHAR_DATA * ch);

bool is_obj_expired(OBJ_DATA *obj);
bool is_obj_falling(OBJ_DATA * obj);
void add_obj_to_falling(OBJ_DATA * obj);
void remove_obj_from_falling(OBJ_DATA * obj);


/* actually in other files, but this is a good place for them */
void add_follower(CHAR_DATA * ch, CHAR_DATA * leader);
bool file_exists(char *fname);
bool generic_damage_immortal(CHAR_DATA * victim, int phys_dam, int mana_dam, int mv_dam, int stun_dam);
void add_rest_to_npcs_in_need(void);

void adjust_ch_infobar( CHAR_DATA *ch, int stat);
void adjust_infobar(struct descriptor_data *d, int stat, char *display);
void die(CHAR_DATA * ch);
void death_cry(CHAR_DATA * ch);
void save_char_objs(CHAR_DATA * ch);
OBJ_DATA *has_key(CHAR_DATA * ch, int key);
int cmd_simple_move(CHAR_DATA * ch, int cmd, int following, char *how);
void gain_language(CHAR_DATA * ch, int lang, int amount);
void gain_accent(CHAR_DATA * ch, int accent, int amount);
void send_to_immort_remote(char *msg);
int change_ldesc(const char *new_ldesc, CHAR_DATA * ch);
int change_objective(const char *new_objective, CHAR_DATA * ch);
void page_string(struct descriptor_data *d, const char *str, int keep_internal);
void free_room(ROOM_DATA * rm);
void free_obj(OBJ_DATA * obj);
int hit(CHAR_DATA * ch, CHAR_DATA * victim, int type);
void save_email_addresses(void);
int execute_npc_program(CHAR_DATA * npc, CHAR_DATA * ch, int cmd, const char *argument);
int execute_obj_program(OBJ_DATA * obj, CHAR_DATA * ch, CHAR_DATA * vict, int cmd,
                        const char *argument);
int execute_room_program(ROOM_DATA * room, CHAR_DATA * ch, int cmd, const char *argument);
int is_number(const char *str);
void close_socket(struct descriptor_data *d);
int count_rooms_in_zone(int zone);

bool player_exists(char *name, char *account, int status);

void write_char_file(void);
int is_flying_char(CHAR_DATA * ch);
void char_from_room_move(CHAR_DATA * ch);
int stat_num(CHAR_DATA * ch, int stat);
int stat_num_raw(int for_race, int stat, int cur);
char *list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, bool show);

int get_char_bulk_penalty(CHAR_DATA *ch);

int old_room_in_city(ROOM_DATA * in_room);

void gain_condition(CHAR_DATA * ch, int condition, int value);
void wear(CHAR_DATA * ch, OBJ_DATA * obj_object, int keyword, char *preHow, char *postHow);
void do_damage(CHAR_DATA * victim, int phys_dam, int stun_dam);
int get_direction(CHAR_DATA * ch, char *arg);
bool missile_damage(CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * arrow, int direction, int dam,
                    int attacktype);
void make_smoke(CHAR_DATA * ch, const char *argument, int cmd);
void appear(CHAR_DATA * ch);
int connect_to_mud(char *host_string, char *port_string);
void send_to_remote_char(int d, char *msg, char *name);
int load_char_mnts_and_objs(CHAR_DATA * ch);
void save_char_waiting_auth(CHAR_DATA * ch);

int get_char_file_is_dead(char *name);
int get_char_file_password(char *name, char *account, char *pass);
int get_char_file_karma(char *name, char *account, int *karma);
char *get_char_file_karmaLog(char *name, char *account);

void save_rooms(struct zone_data *zdata, int zone_num);
void save_world(void);
void save_background(int zone);
void add_line_to_file(struct descriptor_data *d, char *line);
void deliver_mail(void);
int is_mud_connected(int mud);
void update_spice(CHAR_DATA * ch, struct affected_type *af);
int update_magick(CHAR_DATA * ch, struct affected_type *af, int changed_duration);
bool suffering_from_poison(CHAR_DATA *ch, int type);
int update_poison(CHAR_DATA * ch, struct affected_type *af, int changed_duration);
void send_psi_translated(CHAR_DATA * ch, CHAR_DATA * tar_ch, char *text_pure);

void find_spell(char *arg, int *power_return, int *reach_return, int *spell);
void check_theft_flag(CHAR_DATA * ch, CHAR_DATA * victim);
int exec_echopchar(CHAR_DATA * pcdCh, char *arg, int pcdCmd, CHAR_DATA * npc, char *efn);
int exec_echopobj(CHAR_DATA * pcdCh, char *arg, int pcdCmd, OBJ_DATA * obj, char *efn);
int exec_echoproom(CHAR_DATA * pcdCh, char *arg, int pcdCmd, ROOM_DATA * rm, char *efn);
int exec_echopglobal(CHAR_DATA * pcdCh, char *arg, int pcdCmd, char *efn);
int find_door(CHAR_DATA * ch, char *type, char *dir);
int get_weapon_type(int n);
int proficiency_bonus(CHAR_DATA * ch, int attacktype);
int PERCENTAGE(int amount, int max);

void JS_clear_char_pointer(struct char_data *ch);
void JS_clear_obj_pointer(struct obj_data *obj);
void JS_clear_room_pointer(struct room_data *room);

int absorbtion(int armor);
int get_direction_from_name(const char *direction_name);
int calc_carried_weight_no_lift(CHAR_DATA * ch);
int calc_subdued_weight(CHAR_DATA * ch);
int get_random_hit_loc(CHAR_DATA * ch, int type);
int get_shield_loc(CHAR_DATA * ch);
OBJ_DATA *damage_armor(OBJ_DATA * armor, int dam, int wtype);
OBJ_DATA *damage_weapon(OBJ_DATA * att_weapon, OBJ_DATA * def_weapon, int damage);

void planeshift_return(CHAR_DATA * ch);
void release_extracted();
#endif // __HANDLER_H__

#ifdef __cplusplus
}
#endif

