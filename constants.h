/*
 * File: constants.h */
#include "core_structs.h"
#define MAX_ATTRIBUTE 4
#define ATT_STR 0
#define ATT_AGL 1
#define ATT_WIS 2
#define ATT_END 3

#define NUM_SKILL_LEVELS 5
#define SKILL_LEVEL_RANGE_MAX	4

extern bool use_shadow_moon;
extern bool use_global_darkness;
extern char ARM[];
extern char BACK[];
extern const char * const banned_name[];
extern char BODY[];
extern const char * const class_name[];
extern const char * const coin_name[];
extern const char * const color_liquid[];
extern const char * const command_name[];
extern const char * const connected_types[];
extern const char * const scan_dir[];
extern const char * const Crime[];
extern const char * const cure_forms[];
extern const char * const cure_tastes[];
extern const char * const dir_name[];
extern const char * const dirs[];
extern const char * const drinknames[];
extern const char * const drinks[];
extern const char * const Element[];
extern const char * const fabric[];
extern const char * const food_names[];
extern const char * const fullness[];
extern const char * const hallucinations[];
extern const char * const hard_condition[];
extern char HEAD[];
extern const char * const herb_colors[];
extern const char * const herb_types[];
extern char HINDLEG[];
extern const char * const hit_loc_name[20][10];
extern const char * const how_good_stat[];
extern const char * const how_good_skill[];
extern char LEG[];
extern const char * const level_name[];
extern const char * const MagickType[];
extern char MIDLEG[];
extern const char * const Mood[];
extern const char * const mortal_access[];
extern char NECK[];
extern char NN[];
extern char OSET_FLAGS[];
extern const char * const poison_types[];
extern const char * const position_types[];
extern const char * const attributes[];
extern char POTION_SPELL[];
extern const char * const Power[];
extern const char * const Reach[];
extern const char * const rev_dir_name[];
extern const char * const rev_dirs[];
extern const char * const san_dir_name[];
extern char SCROLL_SPELL[];
extern char SHIELD[];
extern const char * const skill_name[];
extern const char * const soft_condition[];
extern const char * const spell_wear_off_msg[];
extern const char * const spell_wear_off_room_msg[];
extern const char * const Sphere[];
extern const char * const spice_odor[];
extern char STAFF_WAND_CHARGE[];
extern const char * const sub_guilds[];
extern const char * const sun_phase[];
extern char TAIL[];
extern const char * const temperature_name[];
extern const char * const tool_name[];
extern const char * const verbose_dirs[];
extern char *vowel;
extern const char * const wear_name[];
extern const char * const weekday_names[];
extern const char * const where[];
extern char WRIST[];
extern const char * const year_one[];
extern const char * const year_two[];
extern int drink_aff[][8];
extern int food_aff[][3];
extern int rev_dir[];
extern struct age_stat_data age_stat_table;
extern struct agl_app_type agl_app[];
extern struct attribute_data application_type[];
extern struct attribute_data commodities[];
extern struct attribute_data instrument_type[];
extern struct attribute_data light_color[];
extern struct attribute_data light_type[];
extern struct attribute_data obj_apply_types[];
extern struct attribute_data obj_material[];
extern struct attribute_data obj_type[];
extern struct attribute_data playable_type[];
extern struct attribute_data room_sector[];
extern struct attribute_data speed_type[];
extern struct attribute_data track_types[];
extern struct attribute_data treasure_type[];
extern struct attribute_data weapon_type[];
extern struct coin_data coins[];
extern struct end_app_type end_app[];
extern struct flag_data affect_flag[];
extern struct flag_data brief_flag[];
extern struct flag_data quit_flag[];
extern struct flag_data char_flag[];
extern struct flag_data clan_job_flags[];
extern struct flag_data container_flags[];
extern struct flag_data exit_flag[];
extern struct flag_data food_flags[];
extern struct flag_data furniture_flags[];
extern struct flag_data grp_priv[];
extern struct flag_data jewelry_flags[];
extern struct flag_data kinfo_guilds[];
extern struct flag_data kinfo_races[];
extern struct flag_data light_flags[];
extern struct flag_data liquid_flags[];
extern struct flag_data mercy_flag[];
extern struct flag_data mineral_type[];
extern struct flag_data musical_flags[];
extern struct flag_data nosave_flag[];
extern struct flag_data note_flags[];
extern struct flag_data obj_adverbial_flag[];
extern struct flag_data obj_color_flag[];
extern struct flag_data obj_extra_flag[];
extern struct flag_data obj_state_flag[];
extern struct flag_data obj_wear_flag[];
extern struct flag_data pinfo_flags[];
extern struct flag_data powder_type[];
extern struct flag_data quiet_flag[];
extern struct flag_data room_flag[];
extern struct flag_data skill_flag[];
extern struct flag_data wagon_flags[];
extern struct flag_data worn_flags[];
extern struct group_data grp[];
extern struct language_data language_table[];
extern struct accent_data accent_table[];
extern struct oset_field_data oset_field[];
// extern struct smell_constant smells_table[];
extern struct str_app_type str_app[];
extern struct wear_edesc_struct wear_edesc[];
extern struct wear_edesc_struct wear_edesc_long[];
extern struct wis_app_type wis_app[];


#define DISABLE_GARBAGE 1

