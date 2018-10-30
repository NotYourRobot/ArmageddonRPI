/*
 * File: STRUCTS.H
 * Usage: Definitions of central data structures.
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

/* #define FD_SETSIZE 512
*/
#ifndef __STRUCTS_INCLUDED
#define __STRUCTS_INCLUDED

#ifdef MEMDEBUG
#include <memdebug.h>
#endif

#include "core_structs.h"
#include "delay.h"

bool is_merchant(CHAR_DATA * ch);
char *find_ex_description_all_words(char *word, struct extra_descr_data *list, bool brackets_ok);
char *find_ex_description(char *word, struct extra_descr_data *list, bool brackets_ok);
char *find_page_ex_description(char *word, struct extra_descr_data *list, bool brackets_ok);
int get_worn_positon(CHAR_DATA * ch, OBJ_DATA * obj);
int perform_command(CHAR_DATA * ch, Command cmd, const char *arg, int count);
OBJ_DATA *get_item_held(CHAR_DATA * ch, int itype);
OBJ_DATA *get_object_in_equip_vis(CHAR_DATA * ch, char *arg, OBJ_DATA * equipment[], int *j);
void append_desc(CHAR_DATA * ch, char *text_to_append);
void make_corpse(CHAR_DATA * ch);
void make_head_from_corpse(OBJ_DATA * from_corpse);
void roll_abilities(CHAR_DATA * ch);
void string_safe_cat(char *original, const char *add, int maxlen);

bool knocked_out(CHAR_DATA * ch);

#ifdef NEW_LIGHT_CODE
/* extinguish an object, no message as to why, return pointer to obj */
OBJ_DATA *extinguish_light(OBJ_DATA * light, ROOM_DATA * room);

/* extinguish an object, giving messages for ch putting it out */
OBJ_DATA *extinguish_object(CHAR_DATA * ch, OBJ_DATA * obj);

/* light an object, giving appropriate messages for ch lighting it */
void light_object(CHAR_DATA * ch, OBJ_DATA * obj, char *arg);

/* light a light, no message as to why */
void light_a_light(OBJ_DATA * light, ROOM_DATA * room);

/* Hastily dropped light, returns pointer to the obj */
OBJ_DATA *drop_light(OBJ_DATA * obj, ROOM_DATA * room);

/* drop weapons when fall unconscious */
void drop_weapons(CHAR_DATA * victim);

/* handle adding the light value of obj to fire */
void obj_to_fire(OBJ_DATA * obj, OBJ_DATA * fire);

/* handle adding the light value of obj from fire */
void obj_from_fire(OBJ_DATA * obj, OBJ_DATA * fire);

/* update all the lights on a character */
void update_char_lights(CHAR_DATA * ch);

/* update an individual light, returning a pointer to the obj */
OBJ_DATA *update_light(OBJ_DATA * obj);

/* add all of ch's lights to their current room */
void ch_lights_to_room(CHAR_DATA * ch);

/* remove all of ch's lights from their current room */
void ch_lights_from_room(CHAR_DATA * ch);



#endif

/* increase the light in a room by 1 of the given color */
void increment_room_light(ROOM_DATA * rm, int color);

/* increase the light in a room by the amount of the given color */
void increase_room_light(ROOM_DATA * rm, int amount, int color);

/* decrease the light in a room by 1 */
void decrement_room_light(ROOM_DATA * rm);

/* decrease the light in a room by the given amount */
void decrease_room_light(ROOM_DATA * rm, int amount);

void draw_delay(CHAR_DATA *ch, OBJ_DATA *obj);
void sheath_delay(CHAR_DATA *ch, OBJ_DATA *obj, int newpos);
void fix_exits(ROOM_DATA * room);
void reset_zone(int zone, int unlimited_spawn);
ROOM_DATA *sort_rooms(ROOM_DATA * l);
ROOM_DATA *sort_rooms_in_zone(ROOM_DATA * l);
int read_room(int zone, FILE * fl);
void write_room(ROOM_DATA * room, FILE * fl);
const char *skip_spaces(const char *string);
void reset_char_skills(CHAR_DATA * victim);
int get_obj_size(OBJ_DATA * obj);
int already_wearing(CHAR_DATA * ch, int keyword);
int get(CHAR_DATA * ch, OBJ_DATA * obj_object, OBJ_DATA * sub_object, int sub_type, int cmd, char *preHow, char *postHow, int showFailure);
int race_no_wear(CHAR_DATA * ch, int wear_loc);
int size_match(CHAR_DATA * ch, OBJ_DATA * obj);
int get_likely_wear_location(OBJ_DATA *obj);
void remove_char_all_barter(CHAR_DATA * ch);
void die_follower(CHAR_DATA * ch);
void die_guard(CHAR_DATA * ch);
void remove_harness(CHAR_DATA * beast);
ROOM_DATA *home_room(CHAR_DATA * ch);

void save_cities(void);

void save_weather_data(void);
int is_poisoned_char(CHAR_DATA * ch);
int is_spiced_char(CHAR_DATA * ch);
char *numberize(int n);
int is_room_indoors(ROOM_DATA * room);
int is_char_ethereal(CHAR_DATA * ch);
bool is_covered(CHAR_DATA * ch, CHAR_DATA * vict, int pos);
bool is_desc_covered(CHAR_DATA * ch, CHAR_DATA * vict, int pos);
char *is_are(char *text);
char *lower_article_first(const char *str);
bool money_mgr(int type, int amt, void *ref1, void *ref2);
int visibility(CHAR_DATA * ch);
int room_visibility(ROOM_DATA * room);
int age_class(int virt_age);
int get_virtual_age(int years, int race);
void update_no_barter(void);
void update_char_objects(CHAR_DATA * ch);
void dump_obj_contents(OBJ_DATA * body);
OBJ_DATA *find_reserved_obj(void);
ROOM_DATA *find_reserved_room(void);
void free_reserved_room(ROOM_DATA * room);
int stamina_loss(CHAR_DATA * ch);
int lag_time(CHAR_DATA * ch);
int succeed_sneak(CHAR_DATA * ch, CHAR_DATA * tch, int roll);
bool circle_follow(CHAR_DATA * ch, CHAR_DATA * victim);
bool circle_follow_ch_obj(CHAR_DATA * ch, OBJ_DATA * wagon);
bool new_circle_follow_ch_ch(CHAR_DATA * ch, CHAR_DATA * leader);
bool circle_follow_obj_ch(OBJ_DATA * obj, CHAR_DATA * leader);

int armor_penalties(CHAR_DATA * ch);
void display_infobar(DESCRIPTOR_DATA * d);

void save_zone_data();
int read_background(int zone, int index, FILE * fd);
int affect_bonus(OBJ_DATA * obj, int loc);
void affect_modify(CHAR_DATA * ch, int loc, long mod, long bitv, bool add);
void check_idling(CHAR_DATA * ch);

extern DESCRIPTOR_DATA *descriptor_list;
extern struct clan_type *clan_table;
extern struct clan_rank_struct *clan_ranks;

ROOM_DATA *get_room_num(int num);
/* external structures */
OBJ_DATA *get_weapon(CHAR_DATA * ch, int *ref);
int get_clan_rel(int clan1, int clan2);
int is_greater_vassal(int clan1, int clan2, int city_tribe);
void drop_clan_status(int clan, int amount);
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct race_data race[];

/* external functions */
void stop_follower(CHAR_DATA * ch);
void set_fighting(CHAR_DATA * ch, CHAR_DATA * vict);
int special(CHAR_DATA * ch, Command cmd, const char *arg);
int guard_check(CHAR_DATA * ch, CHAR_DATA * victim);
int get_skill_wait_time(CHAR_DATA *ch, int sk );
void gain_skill(CHAR_DATA * ch, int sk, int gain);
void npc_act_combat(CHAR_DATA * ch);
void poison_char(CHAR_DATA * ch, int poison, int duration, int silent);
int racial_poison(int race);
int find_max_offense_for_char(CHAR_DATA * ch);
int is_char_drunk(CHAR_DATA * ch);
/* external functions */
void write_clan_bank_account(int clan);
void load_messages(void);
void unload_messages(void);
void weather_and_time(void);
void assign_command_pointers(void);
void unassign_command_pointers(void);
void read_programs(void);
void init_predefs(void);
void uninit_predefs(void);
void init_boards(void);
extern int no_specials;
extern int test_boot;
extern struct weather_node *wn_list;
int choose_exit_name_for(ROOM_DATA * in_room, ROOM_DATA * tgt_room, char *command, int max_dist,
                         CHAR_DATA * ch);
void affect_to_char(CHAR_DATA * ch, struct affected_type *af);
float calc_object_weight(OBJ_DATA * obj);
float calc_object_weight_for_cargo_bay(OBJ_DATA * obj);
extern struct guild_data guild[];
extern struct sub_guild_data sub_guild[];

extern struct zone_data *zone_table;
void auto_save(int zone);
extern int shutdown_game;
int dmpl_name(char *);
extern struct char_fall_data *char_fall_list;
extern struct obj_fall_data *obj_fall_list;
extern int top_of_zone_table;
int map_wagon_rooms(OBJ_DATA * wagon, void (*fun) (ROOM_DATA *, void *), void *arg);
void add_room_weight(ROOM_DATA * room, void *arg);
void remove_obj_all_barter(OBJ_DATA * object);
extern CHAR_DATA *combat_list;
extern struct bdesc_data **bdesc_list;
extern struct help_index_element *help_index;
extern int top_of_helpt;
extern struct ban_t *email_list;
/* external functs */
int _parse_name(char *, char *);
int is_board(int);
int wild_match(const char *s, const char *d);
void write_programs_to_buffer(char *);
void init_hp(CHAR_DATA * ch);
void init_stun(CHAR_DATA * ch);
int path_string(ROOM_DATA * in_room, ROOM_DATA * tgt_room, int max_dist, char *path);

struct help_index_element *build_help_index(FILE * fl, int *num);
void send_fight_messages(CHAR_DATA *ch, CHAR_DATA *victim, int attacktype, int miss);
bool generic_damage(CHAR_DATA *, int, int, int, int);
bool update_pos_after_damage(CHAR_DATA *victim) ;
void die_with_message(CHAR_DATA *ch, CHAR_DATA *victim, char *buf );
extern int avail_descs, real_avail_descs, maxdesc;
void assign_universal_special_list(void);
extern struct weather_data weather_info;
extern FILE *help_fl;
int coded_name(char *name);
int start_npc_prog(CHAR_DATA * ch, CHAR_DATA * npc, Command cmd, char *arg, char *file);
int start_obj_prog(CHAR_DATA * ch, OBJ_DATA * obj, Command cmd, char *arg, char *file);
extern int ch_mem, ob_mem;
double spice_factor(CHAR_DATA *);
int in_spice_wd(CHAR_DATA *);
int is_guild_elementalist(int guild);
extern struct shop_data *shop_index;
extern int max_shops;
void gain_spice_skill(CHAR_DATA * ch, int sk, int gain);
void gain_tolerance_skill(CHAR_DATA * ch, int sk, int gain);
extern int saving_throw[11][5];
bool trying_to_disengage(CHAR_DATA * ch);
void set_disengage_request(CHAR_DATA * ch);
void clear_disengage_request(CHAR_DATA * ch);
void stop_fighting(CHAR_DATA * ch, CHAR_DATA * victim);
void stop_all_fighting(CHAR_DATA * ch);
void remove_listener(CHAR_DATA *);
void remove_npc_brain(CHAR_DATA *);
int poison_stop_hps(CHAR_DATA * ch);
void ch_act_agg(CHAR_DATA * ch);
void flag_mul_rage(CHAR_DATA * ch);
/* extern structures */
bool ws_damage(CHAR_DATA * ch, CHAR_DATA * victim, int hpdam, int manadam,
          int mvdam, int stundam, int attacktype, int elementtype);
bool does_save(CHAR_DATA * ch, int save_type, int mod);
bool wearing_allanak_black_gem(const CHAR_DATA * ch);
void check_criminal_flag(CHAR_DATA * ch, CHAR_DATA * victim);
OBJ_DATA *get_component(CHAR_DATA * ch, int type, int power);
int get_componentB(CHAR_DATA * ch,      /* the caster         */
                   int type,    /* sphere             */
                   int power,   /* level of spell     */
                   char *chsuccmesg,    /* succ message to ch */
                   char *rmsuccmesg,    /* succ message to rm */
                   char *chfailmesg);   /* fail message to ch */
int find_dist_for(ROOM_DATA * from, ROOM_DATA * to, CHAR_DATA * ch);
void save_note(OBJ_DATA *);
void read_note(OBJ_DATA *);
void memory_print_for(CHAR_DATA * vict, char *mesg, int len, CHAR_DATA * ch);
void get_arg_char_world_vis(CHAR_DATA * i, CHAR_DATA * j, char *buffer);

/* extern structures */

extern CHAR_DATA *on_damage_attacker;
extern CHAR_DATA *on_damage_defender;

extern int slow_death;
extern int tics;
void extract_obj(OBJ_DATA * obj);
extern int movement_loss[];
/* external functs */
void add_obj_follower(CHAR_DATA * ch, OBJ_DATA * obj_leader);

OBJ_DATA *get_object_in_equip(char *arg, OBJ_DATA * equipment[], int *j);

void make_char_drunk(CHAR_DATA * ch, int gain);
void remove_note_file(OBJ_DATA *);
extern int oval_defaults[MAX_ITEM_TYPE][6];

void del_char_objs(CHAR_DATA * ch);
int obj_guarded(CHAR_DATA * ch, OBJ_DATA * obj, int CMD);
OBJ_DATA *get_obj_in_list_vnum(int v, OBJ_DATA * list);
void room_to_send(ROOM_DATA * room, char *messg);
void add_to_command_q(CHAR_DATA * ch, const char *arg, Command cmd, int spell, int count);
int mana_amount_level(int vdiff, byte power, int spl);
extern int lock_mortal;
extern int lock_new_char;
extern int lock_arena;
extern char *motd;
extern char *news;
extern char *menu;
extern char *menu_no_ansi;
extern char *doc_menu;
extern char *doc_menu_no_ansi;
extern char *mail_menu;
extern char *mail_menu_no_ansi;
extern char *welcome_msg;
extern char *greetings;
extern char *desc_info;
extern char *credits;
extern char *areas;
extern char *wizlist;
extern char *background_info;
extern char *origin_info;
extern char *tattoo_info;
extern char *account_info;
extern char *app_info;
extern char *end_app_info;
extern char *email_info;
extern char *extkwd_info;
extern char *dead_info;
extern char *rp;
extern char *intro;
extern char *history;
extern char *tuluk;
extern char *allanak;
extern char *villages;
extern char *people;
extern char *profession;
extern char *magick;
extern char *start;
extern char *help;
extern char *godhelp;
extern char *short_descr_info;
extern char *stat_order_info;
extern int diffport;
/* external fcntls */
void add_listener(CHAR_DATA *);

void load_char_objs_name(CHAR_DATA * ch, char *fl);
char *find_email_site(char *s);

extern struct char_file_data current;
void affect_from_char(CHAR_DATA * ch, int skill);
void update_pos(CHAR_DATA * victim);
void show_spec_stats_to_char(CHAR_DATA * ch, CHAR_DATA * viewer);
/* extern vars */
/* extern functions */
int room_in_city(ROOM_DATA * in_room);
int char_in_city(CHAR_DATA * ch);

bool skill_success(CHAR_DATA * ch, CHAR_DATA * tar_ch, int learn_percent);
int exit_guarded(CHAR_DATA * ch, int dir, int cmd);
int out_guarded(CHAR_DATA * ch);
int revintcmp(const void *v1, const void *v2);
int intcmp(const void *v1, const void *v2);

/* extern functions */
int affect_update(CHAR_DATA *ch);
void point_update(void);
int update_stats(CHAR_DATA *ch);
struct weather_node *find_weather_node(ROOM_DATA * room);
int npc_intelligence(CHAR_DATA *ch);
void process_command_q(void);
void update_time(void);
int check_time(struct time_info_data *);
struct time_info_data convert_time(time_t);
int mud_hours_passed(time_t t1, time_t t2);
void update_weather(void);
void update_shops(void);
void gr(int s);
void fallch(CHAR_DATA * ch, int num_rooms);
void fallob(OBJ_DATA * ob);
void handle_combat(void);
int is_templar_in_city(CHAR_DATA * ch);
void remove_all_affects_from_char(CHAR_DATA * ch);
bool drain_char(CHAR_DATA * ch, int sk);
void send_psi_help(CHAR_DATA * ch, int sk);
int is_psionicist(CHAR_DATA * ch);
int is_psi_skill(int skill);
void add_guard(CHAR_DATA * ch, CHAR_DATA * victim);

int seed_quest_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc,
                      ROOM_DATA * rm, OBJ_DATA * obj);
int seedling_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                    OBJ_DATA * obj);
int sapling_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                   OBJ_DATA * obj);
int young_tree_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc,
                      ROOM_DATA * rm, OBJ_DATA * obj);
int old_tree_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                    OBJ_DATA * obj);
int water_life_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc,
                      ROOM_DATA * rm, OBJ_DATA * obj);
int water_death_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc,
                       ROOM_DATA * rm, OBJ_DATA * obj);
int well_clue_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                     OBJ_DATA * obj);
int torch_riddle_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc,
                        ROOM_DATA * rm, OBJ_DATA * obj);
int ankh_rune_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc, ROOM_DATA * rm,
                     OBJ_DATA * obj);
int armor_riddle_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc,
                        ROOM_DATA * rm, OBJ_DATA * obj);
int coffin_riddle_source(CHAR_DATA * ch, Command cmd, const char *argument, CHAR_DATA * npc,
                         ROOM_DATA * rm, OBJ_DATA * obj);

extern int done_booting;
int get_number(const char **name);
int which_city_is_clan(CHAR_DATA * ch);
OBJ_DATA *get_component_nr(CHAR_DATA * ch, int num);
void fix_rooms_city(void);
void fix_watching(ROOM_DATA * room);
void gain_poison_tolerance(CHAR_DATA * ch, int sk, int gain);
void generate_npc_index();
void generate_obj_index();
void get_arg_char_room_vis(CHAR_DATA * i, CHAR_DATA * j, char *buffer);
void load_email_addresses(void);
void load_weather(void);
void move_storm_cloud(OBJ_DATA * storm);
void save_char_objs_name(CHAR_DATA * ch, char *name);
void send_to_outdoor(char *messg);
void unassign_universal_special_list(void);
void unload_email_addresses(void);
void unload_weather(void);
int is_empty(int zone_nr);
void renum_world(void);
void renum_zone_table(void);
void reset_time(void);
void load_dn_data(void);
int find_att_die_roll(int for_race, int att, int type);
char *height_weight_descr(CHAR_DATA * ch, CHAR_DATA * looker);
int spice_in_system(CHAR_DATA * ch, int spice);
int max_psi_skills(CHAR_DATA * ch, int skill);
CHAR_DATA *find_closest_hates(CHAR_DATA * ch, int max_dist);
int count_plyr_list(PLYR_LIST * cl, CHAR_DATA * ch);
int get_plyr_list_weight(PLYR_LIST * cl);
PLYR_LIST *find_in_plyr_list(PLYR_LIST * cl, CHAR_DATA * ch);
void add_occupant(OBJ_DATA * obj, CHAR_DATA * ch);
void remove_occupant(OBJ_DATA * obj, CHAR_DATA * ch);
bool show_remove_occupant(CHAR_DATA * ch, char *preHow, char *postHow);

bool is_monitoring(CHAR_DATA * ch, CHAR_DATA * target, int show);
MONITOR_LIST *find_ch_in_monitoring(MONITOR_LIST * cl, CHAR_DATA * ch);
MONITOR_LIST *find_in_monitoring(MONITOR_LIST * cl, CHAR_DATA * ch, int show);
void add_monitoring(DESCRIPTOR_DATA * d, CHAR_DATA * ch, int show);
void add_monitoring_for_mobile(CHAR_DATA * ch, CHAR_DATA * mob);
void remove_monitoring(DESCRIPTOR_DATA * d, CHAR_DATA * ch, int show);
void send_comm_to_monitor(CHAR_DATA * ch, const char *how, const char *verb,
                          const char *exclaimVerb, const char *askVerb, const char *text);
void send_comm_to_monitors(CHAR_DATA * ch, CHAR_DATA * tch, const char *how, const char *verb,
                           const char *exclaimVerb, const char *askVerb, const char *text);
void send_to_monitor_parsed(CHAR_DATA * ch, const char *output, int show);
void send_to_monitor(CHAR_DATA * ch, const char *message, int show);
void send_to_monitorf(CHAR_DATA * ch, int show, const char *message, ...) __attribute__ ((format(printf, 3, 4)));
void send_to_monitors(CHAR_DATA * ch, CHAR_DATA * tch, const char *message, int show);

int drop_lifting(CHAR_DATA * ch, OBJ_DATA * lifting);
int get_lifting_capacity(PLYR_LIST * lifted_by);
void add_lifting(OBJ_DATA * obj, CHAR_DATA * ch);
void remove_all_lifting(OBJ_DATA * obj);
void remove_lifting(OBJ_DATA * obj, CHAR_DATA * ch);


void print_spice_run_table(CHAR_DATA * ch, OBJ_DATA * obj);

char * show_more_die(int *dice, int numdie);
void print_more_die(CHAR_DATA * ch, int *dice, int numdie);
char * show_dice(int die1, int die2);
void print_dice(CHAR_DATA * ch, int die1, int die2);
void print_bones(CHAR_DATA * ch, OBJ_DATA * obj);
void print_halfling_stones(CHAR_DATA * ch, OBJ_DATA * obj, bool roller);
void print_gypsy_dice(CHAR_DATA * ch, OBJ_DATA * obj);

int get_obj_extra_desc_value(OBJ_DATA * obj, const char *want_name, char *buffer, int length);
int set_obj_extra_desc_value(OBJ_DATA * obj, const char *set_name, const char *set_desc);
int rem_obj_extra_desc_value(OBJ_DATA * obj, const char *rem_name);

int get_char_extra_desc_value(CHAR_DATA * ch, const char *want_name, char *buffer, int length);
int set_char_extra_desc_value(CHAR_DATA * ch, const char *set_name, const char *set_desc);
int add_char_extra_desc_value(CHAR_DATA * ch, const char *set_name, const char *set_desc);
int rem_char_extra_desc_value(CHAR_DATA * ch, const char *rem_name);

int get_room_extra_desc_value(ROOM_DATA * obj, const char *want_name, char *buffer, int length);
int set_room_extra_desc_value(ROOM_DATA * obj, const char *set_name, const char *set_desc);
int rem_room_extra_desc_value(ROOM_DATA * obj, const char *rem_name);

int zone_update_on_room(struct room_data *rm, struct zone_data *zdata);

/* Functions for account level player structure */
/* from player_accounts.c */
void add_new_char_info(PLAYER_INFO * pPInfo, const char *name, int status);
CHAR_INFO *lookup_char_info(PLAYER_INFO * pPInfo, const char *name);
void remove_char_info(PLAYER_INFO * pPInfo, const char *name);
void update_char_info(PLAYER_INFO * pPInfo, const char *name, int status);

PLAYER_INFO *alloc_player_info(void);
PLAYER_INFO *find_player_info(char *email);
CHAR_INFO *alloc_char_info(void);

void free_player_info(PLAYER_INFO * pPInfo);
void free_char_info(CHAR_INFO * pChInfo);

void save_player_info(PLAYER_INFO * pPInfo);

void convert_player_information(PLAYER_INFO * pPInfo);

const char *application_status(int d);

extern char *pwdreset_text;
void scramble_and_mail_account_password(PLAYER_INFO * pPInfo, char *text);

/* functions for storing login/logout data for an account */
void add_in_login_data(PLAYER_INFO * pPInfo, int descriptor, char *site);
void add_out_login_data(PLAYER_INFO * pPInfo, int descriptor);
void add_failed_login_data(PLAYER_INFO * pPInfo, char *site);

/* from show_menu.c */
void show_main_menu(DESCRIPTOR_DATA * d);

/* from revise_char_menu.c */
void revise_char_menu(DESCRIPTOR_DATA * d, char *argument);
void print_revise_char_menu(DESCRIPTOR_DATA * d);
void option_menu(DESCRIPTOR_DATA * d, char *argument);
void print_option_menu(DESCRIPTOR_DATA * d);


/* object.c */
void set_tent_tracks_in_room(OBJ_DATA * obj, ROOM_DATA * room);
void set_combat_tracks_in_room(CHAR_DATA * ch, CHAR_DATA * vict, ROOM_DATA * room);

/* from parser.c */
void print_height_info(DESCRIPTOR_DATA * d);
void print_weight_info(DESCRIPTOR_DATA * d);
void print_age_info(DESCRIPTOR_DATA * d);

bool is_immortal_name(const char *name);
void add_immortal_name(const char *name, const char *account);
bool remove_immortal_name(const char *name);
void save_immortal_names();

bool is_immortal_account(char *account);

struct specials_t *copy_specials(struct specials_t *spec);

void create_node(int zone, int life, int min_temp, int max_temp, double min_condition,
                 double max_condition, double median_condition);

void add_neighbor(struct weather_node *wn, struct weather_node *nb, int dir);
int remove_neighbor(struct weather_node *wn, struct weather_node *nb, int dir);

/* from comm.c */
extern int skill_weap_rtype[9 /* race type */ ][8 /* Weapon# */ ];

int gain_weapon_racetype_proficiency(CHAR_DATA * ch, int wtype, int rtype);

/* these return 0 if they didnt have the skill to begin with
   or if it was already set to taught ,
   return 1 if they have skill, and it wasnt taught, and got set to taught
*/
int remove_skill_taught(CHAR_DATA * ch, int skilno);
int remove_skill_taught_raw(struct char_skill_data *skill);
int set_skill_taught(CHAR_DATA * ch, int skilno);
int set_skill_taught_raw(struct char_skill_data *skill);
int is_skill_taught(CHAR_DATA * ch, int skilno);
int is_skill_taught_raw(struct char_skill_data *skill);
int remove_skill_nogain(CHAR_DATA * ch, int skilno);
int remove_skill_nogain_raw(struct char_skill_data *skill);
int set_skill_nogain(CHAR_DATA * ch, int skilno);
int set_skill_nogain_raw(struct char_skill_data *skill);
int is_skill_nogain(CHAR_DATA * ch, int skilno);
int is_skill_nogain_raw(struct char_skill_data *skill);
int remove_skill_hidden(CHAR_DATA * ch, int skilno);
int remove_skill_hidden_raw(struct char_skill_data *skill);
int set_skill_hidden(CHAR_DATA * ch, int skilno);
int set_skill_hidden_raw(struct char_skill_data *skill);
int is_skill_hidden(CHAR_DATA * ch, int skilno);
int is_skill_hidden_raw(struct char_skill_data *skill);


/* Find the max skill % for guild/subguild */
int get_max_skill(CHAR_DATA * ch, int nSkill);

/* Find the max skill % for character's attributes */
int get_innate_max_skill(CHAR_DATA * ch, int nSkill);

/* returns 0 if they dont have the skill, and 1 if they do */
int has_skill(CHAR_DATA * ch, int skilno);

/* returns percentage if they have the skill and 0 if they dont. */
int get_skill_percentage(CHAR_DATA * ch, int skilno);
int get_skill_percentage_raw(struct char_skill_data *skill);

/* returns percentage if they have the skill and 0 if they dont, sets skill */
int set_skill_percentage(CHAR_DATA * ch, int skilno, int perc);
int set_skill_percentage_raw(struct char_skill_data *skill, int perc);

/* tool functions */
bool use_tool(OBJ_DATA * obj, int tool_type);

#ifdef SMELLS
/* updates the smells on a person */
void char_smell_update(CHAR_DATA * ch);

/* updates the smells on a room */
void room_smell_update(ROOM_DATA * ch);

void add_smell(SMELL_DATA ** target, SMELL_DATA * source);

int lookup_smell(char *name);
#endif

void knock_out(CHAR_DATA * ch, const char *reason, int duration);

/* magick.c */
void remove_alarm_spell(ROOM_DATA * room);

/* movement.c */
int get_room_weight(ROOM_DATA * room);
int get_room_max_weight(ROOM_DATA * room);
int get_used_room_weight(ROOM_DATA * room);

/* other.c */
void stop_gone(CHAR_DATA * ch);



/* craft.c */

/*
  |<CRAFT>
  |  <ITEM>
  |    <NAME>                  </NAME>
  |    <DESC>                  </DESC>
  |    <FROM>                  </FROM>
  |    <INTO_SUCCESS>          </INTO_SUCCESS>
  |    <INTO_FAIL>             </INTO_FAIL>
  |    <SKILL>                 </SKILL>
  |    <PERC_MIN>              </PERC_MIN>
  |    <PERC_MOD>              </PERC_MOD>
  |    <DELAY_MIN>             </DELAY_MIN>
  |    <DELAY_MAX>             </DELAY_MAX>
  |    <TO_CHAR_SUCCESS>       </TO_CHAR_SUCCESS>
  |    <TO_ROOM_SUCCESS>       </TO_CHAR_SUCCESS>
  |    <ONLY_TRIBE>            </ONLY_TRIBE>
  |  </ITEM>
  |</CRAFT>
*/

extern struct create_item_type *make_item_list;
extern int silence_log;

extern int top_of_npc_t;
extern int top_of_obj_t;

#define stricmp strcasecmp
#define strnicmp strncasecmp

#endif

