/*
 * File: utility.h
 * Usage: interface for utility.c routines
 *
 * Copyright (C) 1996 all original source by Jim Tripp (Tenebrius), Mark
 * Tripp (Nessalin), Hal Roberts (Bram), and Mike Zeman (Azroen).  May 
 * favourable winds fill the sails of their skimmers.
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 *
 *                       Version 4.0  Generation 2 
 * ---------------------------------------------------------------------
 *
 * Original DikuMUD 1.0 courtesy of Katja Nyboe, Tom Madsen, Hans
 * Staerfeldt, Michael Seifert, and Sebastian Hammer.  Kudos.
 */

#ifndef __UTILITY_INCLUDED
#define __UTILITY_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#define DICE_NUM 1
#define DICE_SIZE 2
#define DICE_PLUS 3

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "guilds.h"
#include "skills.h"
#include "db.h"
#include <sys/time.h>

#define TALK_EMOTE_NONLISTENER

    void to_lower(char *str);
    int char_can_fight(CHAR_DATA * ch);

    bool get_char_mood(CHAR_DATA *ch, char *buf, int sizeofbuf);
    void change_mood(CHAR_DATA *ch, char *mood);
    void set_char_position(CHAR_DATA * ch, int pos);

    const char *real_sdesc(CHAR_DATA * ch, CHAR_DATA * looker, int vis);
    char *real_sdesc_keywords_obj(OBJ_DATA * object, CHAR_DATA *viewer);
    char *real_sdesc_keywords(CHAR_DATA * ch);

    int get_condition(int current, int max);
    int hands_free(CHAR_DATA * ch);
    int current_hour(void);
    int get_char_size(CHAR_DATA * ch);
    int char_init_height(int race);
    int char_init_weight(int race);
    int get_land_distance(ROOM_DATA * start_rom, ROOM_DATA * end_room);
    void set_hit(CHAR_DATA *ch, int newval);
    void adjust_hit(CHAR_DATA *ch, int amount);
    void set_mana(CHAR_DATA *ch, int newval);
    void adjust_mana(CHAR_DATA *ch, int amount);
    void set_move(CHAR_DATA *ch, int newval);
    void adjust_move(CHAR_DATA *ch, int amount);
    void set_stun(CHAR_DATA *ch, int newval);
    void adjust_stun(CHAR_DATA *ch, int amount);

    int IS_DARK(struct room_data *room);
    int IS_LIGHT(struct room_data *room);

    char *save_category(int num);

    char *number_suffix(int num);

/* return a number from 0..10 (maybe more) which is encumberance class */
    int get_encumberance(CHAR_DATA * ch);
    int get_obj_encumberance(OBJ_DATA * obj, CHAR_DATA * ch);

    char *encumb_class(int enc);

    int is_soldier(CHAR_DATA * ch);
    bool is_lawful_humanoid(CHAR_DATA * ch);

/* creates a random number in interval [from;to] */
/* this puppy can run into divide by zero problems, hence the 
 * extra lines.   --brian
 */
    int number(int from, int to);

/* simulates dice roll */
    int dice(int num, int size);

/* gets a random character from a room, non-immortal */
    CHAR_DATA *get_random_non_immortal_in_room(struct room_data *rm);

/* send str to file */
    void log_to_file(const char *str, char *file);
/* log a warning */
    void warning_log(const char *str);
    void warning_logf(const char *fmt, ...);

/* log for just the room */
    void roomlog(ROOM_DATA *rm, const char *str);
    void roomlogf(ROOM_DATA *rm, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
    void qroomlog(int quiet_level, ROOM_DATA *rm, const char *str);
    void qroomlogf(int quiet_level, ROOM_DATA *rm, const char *fmt, ...) __attribute__ ((format(printf, 3, 4)));


    struct timeval timediff(struct timeval *a, struct timeval *b);

    void perflogf(const char *fmt, ...);
    void perf_enter(struct timeval *enter_time, const char *method);
    void perf_exit(const char *method, struct timeval enter);

    struct time_info_data real_time_passed(time_t t2, time_t t1);

/* calculate game time passed between t1 and t2 */
    struct time_info_data mud_time_passed(time_t t1, time_t t2);
    struct time_info_data age(CHAR_DATA * ch);

/* begin stat aging routines */

/* adjust stat according to new age.  age_stat uses the age_stat_table 
   to determine the chance that a given stat will go up or down when 
   the character grows older or younger by one year.*/
    void age_stat(CHAR_DATA * ch, int age, int age_dir, char stat);

/* adjust stats according to new age.  Second argument can either be the 
   character's old age or -1.  In the latter case, age_stats will use the
   age of the character at the time of his last logon */
    void age_all_stats(CHAR_DATA * ch, int old_age);

    int set_age(CHAR_DATA * ch, int new_age);
    int set_age_raw(CHAR_DATA * ch, int new_age);

/* Accessors/modifiers to abilities */
    int get_char_str(CHAR_DATA * ch);
    void set_char_str(CHAR_DATA * ch, int val);
    int get_char_agl(CHAR_DATA * ch);
    void set_char_agl(CHAR_DATA * ch, int val);
    int get_char_wis(CHAR_DATA * ch);
    void set_char_wis(CHAR_DATA * ch, int val);
    int get_char_end(CHAR_DATA * ch);
    void set_char_end(CHAR_DATA * ch, int val);
    int get_char_off(CHAR_DATA * ch);
    void set_char_off(CHAR_DATA * ch, int val);
    int get_char_def(CHAR_DATA * ch);
    void set_char_def(CHAR_DATA * ch, int val);
    int get_char_armor(CHAR_DATA * ch);
    void set_char_armor(CHAR_DATA * ch, int val);
    int get_char_damroll(CHAR_DATA * ch);
    void set_char_damroll(CHAR_DATA * ch, int val);

/* returns:
    0 if sub cannot see obj at all
    1 if sub can see obj clearly
    2 if sub detects obj as a hidden form
    3 if sub detects obj as an invis form
    4 if sub detects obj as an ethereal form
    5 if sub detects obj as a heat shape (infra)
    6 if sub can barely see obj (weather)
 */
    int has_sight_spells_resident(CHAR_DATA * ch);
    int can_use_psionics(CHAR_DATA * ch);
    int char_can_see_char(CHAR_DATA * sub, CHAR_DATA * obj);
    int char_can_see_obj(CHAR_DATA * sub, OBJ_DATA * obj);

    int will_char_fall_in_room(CHAR_DATA * ch, struct room_data *room);

    int will_obj_fall_in_room(OBJ_DATA * obj, struct room_data *room);

    void all_cap(char *inp, char *out);
    int compute_stat_rating(int for_race, int stat, int minmax);

/* flag and attribute-related functions */
    void sprintbit(long vektor, char *names[], char *result);
    void sprinttype(int type, char *names[], char *result);
    void sprint_flag(long vektor, struct flag_data *flag, char *result);
    char *show_flags(struct flag_data *a, int b);
    char *show_attrib(int a, struct attribute_data *b);
    char *show_attrib_flags(struct attribute_data *a, int b);
    void sprint_attrib(int val, struct attribute_data *att, char *result);
    void send_attribs_to_char(struct attribute_data *att, CHAR_DATA * ch);
    void send_list_to_char_f(CHAR_DATA * ch, const char * const list[], int (*fun) (int i));
    void send_flags_to_char(struct flag_data *flag, CHAR_DATA * ch);
    int get_attrib_num(struct attribute_data *att, const char *arg);
    int get_attrib_val(struct attribute_data *att, char *arg);
    const char *get_attrib_name(struct attribute_data *att, int val);
    int get_flag_num(struct flag_data *flag, const char *arg);
    long get_flag_bit(struct flag_data *flag, const char *arg);

/* returns TRUE if char has priveleges */
    bool has_privs_in_zone(CHAR_DATA * ch, int zone, int mode);
    bool check_tunnel_size(CHAR_DATA * ch, int door);

    int validate_room_in_roomlist(struct room_data *room);
    int validate_char_in_charlist(CHAR_DATA * ch);
    int validate_obj_in_objlist(OBJ_DATA * obj);
    int validate_char_list_in_rooms_list(void);
    int validate_chars_in_rooms_in_charlist(void);
    int validate_obj_in_appropriate_list(OBJ_DATA * obj);
    int validate_objects_in_lists(void);
    int check_db(void);

    int is_char_noble(CHAR_DATA * ch, int city);
    void show_obj_to_char(OBJ_DATA * obj, CHAR_DATA * ch, int mode);
    char *format_obj_to_char(OBJ_DATA * obj, CHAR_DATA * ch, int mode);
    char *pluralize(char *argument);
    const char *xformat_string(const char *in_string, char *out, size_t len);
    char *format_string(char *in_string);
    char *indented_format_string(char *in_string);
    char *trim_string(char *in_string);
    char *generate_keywords_from_string(const char * str);
    char *generate_keywords(CHAR_DATA * ch);
    void add_keywords(CHAR_DATA * ch, char *words);

    void sheath_all_weapons(CHAR_DATA * ch);
    void draw_all_weapons(CHAR_DATA * ch);
    void remove_all_scripts(CHAR_DATA * ch);

    OBJ_DATA *find_head_in_room_from_edesc(ROOM_DATA * rm, char *name, char *ainfo);

    bool is_paralyzed(CHAR_DATA * ch);

    bool is_charmed(CHAR_DATA * ch);

    bool is_magicker(CHAR_DATA * ch);

    int is_ch_in_clan_list( CHAR_DATA *ch, const char *clans );

    bool is_on_same_plane(ROOM_DATA *src, ROOM_DATA *dst);

    void touchfile(const char *filename, const char *contents);
    void appendfile(const char *filename, const char *contents);
    void remfile(const char *filename);

    bool is_following(CHAR_DATA *follower, CHAR_DATA *leader);

    int is_cloak(CHAR_DATA *ch, OBJ_DATA *obj);
    int is_cloak_open(CHAR_DATA *ch, OBJ_DATA *obj);
    int is_closed_container(CHAR_DATA *ch, OBJ_DATA *obj);

    int handle_tunnel(ROOM_DATA *rm, CHAR_DATA *ch);
    int room_can_contain_char(ROOM_DATA *rm, CHAR_DATA *ch);

    void show_drink_container_fullness(CHAR_DATA *ch, OBJ_DATA *obj);

    #define nstrlen(str) (str ? strlen(str) : 0)
#ifdef __cplusplus
}
#endif

#endif

