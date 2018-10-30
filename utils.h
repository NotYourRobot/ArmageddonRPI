/*
 * File: UTILS.H
 * Usage: Utility macro definitions.
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

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdlib.h>

#define TRUE  1
#define FALSE 0


#define LOWER(c) (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))

#define UPPER(c) (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

/* Functions in utility.c                     */
static inline int
MAX(int x, int y)
{
    return ((x > y) ? x : y);
}
static inline int
MIN(int x, int y)
{
    return ((x < y) ? x : y);
}

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

#define IF_STR(st) ((st) ? (st) : "\0")

#define IS_VOWEL(c) ((c) == 'a' || (c) == 'e' || (c) == 'i' || \
                     (c) == 'o' || (c) == 'u')

#define GET_NPC_DEF( dat, field ) (((dat)->nr >= 0) && ((dat)->nr < top_of_npc_t) ? \
                          npc_default[(dat)->nr].field : "_destroyed_(bug)" )

#define MSTR( dat, field ) ((dat->field != NULL ? dat->field : \
                          npc_default[(dat)->nr].field))

#define OSTR( dat, field ) ((dat->field != NULL ? dat->field : \
                          obj_default[(dat)->nr].field))
#define CAP(st)  (*(st) = UPPER(*(st)), st)
#define UNCAP(st) (*(st) = LOWER(*(st)), st)

#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
		{ perror("calloc failure"); abort(); } } while(0)

/* Going to modify RECREATE when I get a chance - Tiernan 7/28 
extern void log(char *str);
char err[80] = "REALLOC FAILURE";
#define RECREATE(result, type, number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("realloc failure");\
                  log(err);\
                  abort(); } } while(0)
*/

#define RECREATE(result, type, number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("realloc failure"); abort(); } } while(0)

#define DESTROY(mem)	 do {free((mem));(mem) = NULL;} while(0)

#define IS_SET(flag,bit)  ((flag) & (bit))

#define SWITCH(a,b) { (a) ^= (b); \
                      (b) ^= (a); \
                      (a) ^= (b); }

#define ECHO_OFF(d) SEND_TO_Q("\xff\xfb\x01", d);

#define ECHO_ON(d)  SEND_TO_Q("\xff\xfc\x01\n\r", d);

#define IS_AFFECTED(ch,skill) IS_SET((ch)->specials.affected_by, (skill))

#define IS_CALMED(ch)  IS_AFFECTED(ch, CHAR_AFF_CALM)

#define IS_SUBDUED(ch)  IS_AFFECTED(ch, CHAR_AFF_SUBDUED)

#define IS_ATTACK(at)  ((at >= TYPE_HIT) && (at <= TYPE_MAX))

#define MUD_SET_BIT(var, bit)  ((var) = (var) | (bit))

#define REMOVE_BIT(var, bit)  ((var) = (var) & ~(bit))

#define TOGGLE_BIT( var, bit )  ( ( var ) ^= ( bit ) )


/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj)   char_can_see_char(sub, obj)

#define CAN_FIGHT(ch) char_can_fight(ch)

#define HSHR(ch) ((ch)->player.sex ?					\
	(((ch)->player.sex == 1) ? "his" : "her") : "its")

#define HSSH(ch) ((ch)->player.sex ?					\
	(((ch)->player.sex == 1) ? "he" : "she") : "it")

#define HMHR(ch) ((ch)->player.sex ? 					\
	(((ch)->player.sex == 1) ? "him" : "her") : "it")

#define HMFHRF(ch) ((ch)->player.sex ? 					\
	(((ch)->player.sex == 1) ? "himself" : "herself") : "itself")

#define HSHRS(ch) ((ch)->player.sex ?					\
	(((ch)->player.sex == 1) ? "his" : "hers") : "its")

#define ANA(obj) (index("aeiouyAEIOUY", *(OSTR( (obj), name ))) ? "An" : "A")

#define SANA(obj) (index("aeiouyAEIOUY", *(OSTR( (obj), name ))) ? "an" : "a")

#define IS_NPC(ch)  (IS_SET((ch)->specials.act, CFL_ISNPC))

#define IS_MOB(ch)  (IS_SET((ch)->specials.act, CFL_ISNPC) && ((ch)->nr >-1))

#define IS_CREATING(ch) ((ch->desc) && ((ch->desc->str) || (ch->desc->editing)))

#define IS_SPELL(sp)    (skill[(sp)].sk_class == CLASS_MAGICK)
#define IS_SPICE(sp)    (skill[(sp)].sk_class == CLASS_SPICE)
#define IS_SKILL(sp)    (!IS_SPELL(sp) && !IS_SPICE(sp))

#define IS_LIT( obj )   (IS_SET( (obj)->obj_flags.value[5], LIGHT_FLAG_LIT ) )
#define IS_REFILLABLE( obj )  (IS_SET( (obj)->obj_flags.value[5],\
				 LIGHT_FLAG_REFILLABLE ) )
#define IS_WAGON(obj)    ((obj)->obj_flags.type == ITEM_WAGON)
#define LIGHT_CONSUMES( obj )  (IS_SET( (obj)->obj_flags.value[5],\
				 LIGHT_FLAG_CONSUMES ) )

#define IS_TORCH( obj ) (IS_SET((obj)->obj_flags.value[5], LIGHT_FLAG_TORCH))
#define IS_CANDLE( obj ) (IS_SET((obj)->obj_flags.value[5], LIGHT_FLAG_CANDLE))
#define IS_LAMP( obj ) (IS_SET((obj)->obj_flags.value[5], LIGHT_FLAG_LAMP))
#define IS_LANTERN( obj ) (IS_SET((obj)->obj_flags.value[5], \
				 LIGHT_FLAG_LANTERN))
#define IS_CAMPFIRE( obj ) (IS_SET((obj)->obj_flags.value[5], \
				 LIGHT_FLAG_CAMPFIRE))

#define GET_POS(ch)     ((ch)->specials.position)

#define GET_COND(ch, i) ((ch)->specials.conditions[(i)])

#define GET_NAME(ch)    (MSTR( (ch), name ) )

#define GET_LEVEL(ch)   ((ch)->player.level)

static inline GuildType GET_GUILD(const CHAR_DATA *ch)            { return ch->player.guild; }
static inline void      SET_GUILD(CHAR_DATA *ch, GuildType guild) { ch->player.guild = guild; }

#define GET_SUB_GUILD(ch)   ((ch)->player.sub_guild)

#define HAS_CITY_STEALTH(ch) (GET_GUILD(ch) == GUILD_ASSASSIN \
                           || GET_GUILD(ch) == GUILD_BURGLAR \
                           || GET_GUILD(ch) == GUILD_PICKPOCKET \
                           || GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_NOBLE_SPY \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_SLIPKNIFE \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_ROGUE \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_SORC_ASSASSIN \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_SORC_BURGLAR \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_SORC_PICKPOCKET \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_THIEF)

#define CITY_HIDER(ch) (HAS_CITY_STEALTH(ch))

#define HAS_WILD_STEALTH(ch) (GET_GUILD(ch) == GUILD_RANGER \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_SORC_ARCHER \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_SORC_RANGER \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_REBEL \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_OUTDOORSMAN \
                           || GET_SUB_GUILD(ch) == SUB_GUILD_HUNTER \
                           || GET_RACE(ch) == RACE_HALFLING \
                           || GET_RACE(ch) == RACE_MANTIS \
                           || GET_RACE(ch) == RACE_DESERT_ELF)

#define WILD_HIDER(ch) (HAS_WILD_STEALTH(ch))

#define HIDE_CITY_AREA(room) ((room)->sector_type <= SECT_CITY)

#define HIDE_WILD_AREA(room) ((room)->sector_type > SECT_CITY \
                       || IS_SET((room)->room_flags, RFL_PLANT_SPARSE ) \
                       || IS_SET((room)->room_flags, RFL_PLANT_HEAVY ))

#define CAN_HIDE_IN_CITY(ch) (CITY_HIDER(ch) && HIDE_CITY_AREA((ch)->in_room))

#define CAN_HIDE_IN_WILDS(ch) (WILD_HIDER(ch) && HIDE_WILD_AREA((ch)->in_room))

#define HIDE_OK(ch) (CAN_HIDE_IN_CITY(ch) || CAN_HIDE_IN_WILDS(ch))

#define CITY_SNEAKER(ch) (HAS_CITY_STEALTH(ch))

#define WILD_SNEAKER(ch) (HAS_WILD_STEALTH(ch))

#define CAN_SNEAK_IN_CITY(ch) (CITY_SNEAKER(ch) \
                               && ((ch)->in_room->sector_type == SECT_CITY \
                                || (ch)->in_room->sector_type == SECT_INSIDE))

#define CAN_SNEAK_IN_WILDS(ch) (WILD_SNEAKER(ch) \
                                && (ch)->in_room->sector_type != SECT_CITY \
                                && (ch)->in_room->sector_type != SECT_INSIDE)

#define SNEAK_OK(ch) (CAN_SNEAK_IN_CITY(ch) || CAN_SNEAK_IN_WILDS(ch))

#define CITY_TRACKER(ch) (GET_GUILD(ch) == GUILD_ASSASSIN \
                          || GET_SUB_GUILD(ch) == SUB_GUILD_SORC_ASSASSIN)

#define WILD_TRACKER(ch) (GET_GUILD(ch) == GUILD_RANGER \
                        || GET_SUB_GUILD(ch) == SUB_GUILD_FORESTER \
                        || GET_SUB_GUILD(ch) == SUB_GUILD_HUNTER \
                        || GET_SUB_GUILD(ch) == SUB_GUILD_REBEL \
                        || GET_SUB_GUILD(ch) == SUB_GUILD_GREBBER \
                        || GET_SUB_GUILD(ch) == SUB_GUILD_OUTDOORSMAN \
                        || GET_SUB_GUILD(ch) == SUB_GUILD_SORC_NOMAD \
                        || GET_SUB_GUILD(ch) == SUB_GUILD_SORC_RANGER)

#define CAN_TRACK_IN_CITY(ch) (CITY_TRACKER(ch)\
                        && ((ch)->in_room->sector_type == SECT_CITY \
                         || (ch)->in_room->sector_type == SECT_INSIDE))

#define CAN_TRACK_IN_WILDS(ch) (WILD_TRACKER(ch)\
                        && (ch)->in_room->sector_type != SECT_CITY \
                        && (ch)->in_room->sector_type != SECT_INSIDE)

#define TRACK_OK(ch) (CAN_TRACK_IN_CITY(ch) || CAN_TRACK_IN_WILDS(ch))

#define GET_RACE(ch)    ((ch)->player.race)

#define GET_PAGELENGTH( ch )   ((ch)->pagelength ? (ch)->pagelength \
                                : ( (ch)->desc ? (ch)->desc->pagelength : 24))

/* multi-clan functions */
int lookup_clan_by_name( const char *name );
bool is_clan_member(CLAN_DATA * clan, int num);
CLAN_DATA *new_clan_data(void);
void add_clan(CHAR_DATA * ch, int clan);
void add_clan_data(CHAR_DATA * ch, CLAN_DATA * pClan);
void free_clan_data(CLAN_DATA * pClan);
void remove_clan(CHAR_DATA * ch, int clan);

void set_clan_flag(CLAN_DATA * clan, int num, int flag);
void remove_clan_flag(CLAN_DATA * clan, int num, int flag);
bool is_set_clan_flag(CLAN_DATA * clan, int num, int flag);

void set_clan_rank(CLAN_DATA * clan, int num, int rank);
int get_clan_rank(CLAN_DATA * clan, int num);
int clan_rank_cmp(CHAR_DATA * ch, CHAR_DATA * ch2, int clan);

long in_same_tribe(CHAR_DATA * ch, CHAR_DATA * victim);


/* #defines for multi clans */
#define IS_TRIBE_LEADER( ch, tribe )   ( is_set_clan_flag( (ch)->clan, (tribe),\
                                         CJOB_LEADER ) )
#define IS_IN_SAME_TRIBE( ch, vch )    ( in_same_tribe( (ch), (vch) ) )
#define IS_TRIBE( ch, tribe )          ( is_clan_member( (ch)->clan, (tribe) ) )
#define GET_TRIBE( ch )                ( (ch)->clan ? (ch)->clan->clan : 0 )



#define GET_RACE_TYPE(ch)  (race[(int)GET_RACE(ch)].race_type)

#define GET_HOME(ch)	((ch)->player.hometown)

#define GET_START(ch)	((ch)->player.start_city)

#define GET_AGE(ch)     (age(ch).year)

#define GET_SPEED(ch)   ((ch)->specials.speed)

#define GET_STR(ch)     (get_char_str((ch)))
#define SET_STR(ch, v)  (set_char_str((ch), (v)))

#define GET_AGL(ch)     (get_char_agl((ch)))
#define SET_AGL(ch, v)  (set_char_agl((ch), (v)))

#define GET_WIS(ch)     (get_char_wis((ch)))
#define SET_WIS(ch, v)  (set_char_wis((ch), (v)))

#define GET_END(ch)     (get_char_end((ch)))
#define SET_END(ch, v)  (set_char_end((ch), (v)))

#define GET_OFF(ch)     (get_char_off((ch)))
#define SET_OFF(ch, v)  (set_char_off((ch), (v)))

#define GET_DEF(ch)     (get_char_def((ch)))
#define SET_DEF(ch, v)  (set_char_def((ch), (v)))

#define GET_ARMOR(ch)     (get_char_armor((ch)))
#define SET_ARMOR(ch, v)  (set_char_armor((ch), (v)))

#define GET_DAMROLL(ch)     (get_char_damroll((ch)))
#define SET_DAMROLL(ch, v)  (set_char_damroll((ch), (v)))

#define GET_OFFENSE(ch) ((ch)->abilities.off + race[(int)GET_RACE(ch)].off_base + \
  guild[(int)GET_GUILD(ch)].off_bonus + GET_STR(ch) + GET_AGL(ch))

#define GET_DEFENSE(ch) ((ch)->abilities.def + race[(int)GET_RACE(ch)].def_base + \
  guild[(int)GET_GUILD(ch)].def_bonus + (GET_AGL(ch) * 2))

#define GET_STATE(ch)   ((ch)->specials.state)

#define IS_IMMORTAL(ch) (!IS_NPC(ch) && (GET_LEVEL(ch) > MORTAL))

#define IS_SWITCHED(ch) (IS_NPC(ch) && (ch)->desc && (ch)->desc->original)

#define SWITCHED_PLAYER(ch) (IS_SWITCHED(ch) ? (ch)->desc->original : (ch))

#define IS_SWITCHED_IMMORTAL(ch) (IS_IMMORTAL(SWITCHED_PLAYER(ch)))

#define IS_PLAYER(ch) (!IS_NPC(ch) || (IS_SWITCHED(ch) && !IS_SWITCHED_IMMORTAL(ch)))

#define IS_ADMIN(ch)  ((GET_LEVEL(ch) >= HIGHLORD) && !IS_NPC(ch))

#define GET_HIT(ch)     ((ch)->points.hit)

#define GET_MAX_HIT(ch) (hit_limit(ch))

#define GET_STUN(ch)     ((ch)->points.stun)

#define GET_MAX_STUN(ch) (stun_limit(ch))

#define GET_MOVE(ch)    ((ch)->points.move)

#define GET_MAX_MOVE(ch) (move_limit(ch))

#define GET_MANA(ch)    ((ch)->points.mana)

#define GET_MAX_MANA(ch) (mana_limit(ch))

#define GET_OBSIDIAN(ch) ((ch)->points.obsidian)

#define GET_HEIGHT(ch)  ((ch)->player.height)

#define GET_WEIGHT(ch)  ((ch)->player.weight)

#define GET_SEX(ch)     ((ch)->player.sex)

#define AWAKE(ch) ((ch)->specials.position > POSITION_SLEEPING ? 1 : 0)

#define WAIT_STATE(ch, cycle)  (((ch)->specials.act_wait) += (WAIT_SEC * (cycle)))


#define CH( d )     ((d)->original ? (d)->original : (d)->character )


/* Object And Carry related macros */

#define CAN_SEE_OBJ(sub, obj) char_can_see_obj(sub, obj)

#define GET_ITEM_TYPE(obj) ((obj)->obj_flags.type)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags,part))

#define GET_OBJ_WEIGHT(obj) (calc_object_weight((obj)))

#define CAN_CARRY_W(ch) (str_app[GET_STR(ch)].carry_w)

#define CAN_CARRY_N(ch) (agl_app[GET_AGL(ch)].carry_num)

#define CAN_SHEATH_BACK(obj) (GET_OBJ_WEIGHT((obj)) > 5  \
                     || (obj)->obj_flags.type == ITEM_BOW \
                     || IS_SET((obj)->obj_flags.extra_flags, OFL_NO_ES))

int calc_carried_weight(struct char_data *ch);

int calc_wagon_max_damage(OBJ_DATA * wagon);

#define IS_CARRYING_W(ch) (calc_carried_weight((ch)))

#define IS_CARRYING_N(ch) ((ch)->specials.carry_items)

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_TAKE) && CAN_CARRY_OBJ((ch),(obj)) &&          \
    CAN_SEE_OBJ((ch),(obj)))

#define IS_OBJ_STAT(obj,stat) (IS_SET((obj)->obj_flags.extra_flags,stat))

#define IS_CORPSE(obj) (((obj)->obj_flags.type == ITEM_CONTAINER) && \
  ((obj)->obj_flags.value[3]))

#define IS_CORPSE_HEAD(obj) (((obj)->obj_flags.type == ITEM_CONTAINER) && \
  ((obj)->obj_flags.value[3]) &&\
  IS_SET( (obj)->obj_flags.value[1], CONT_CORPSE_HEAD ) )

#define IS_CONTAINER(obj) ((obj)->obj_flags.type == ITEM_CONTAINER)

/* char name/short_desc(for mobs) or someone?  */

#define PERS(ch, vict)   (real_sdesc((vict), (ch), CAN_SEE((ch), (vict))))

#define VOICE(ch, vict)   ( CAN_SEE( (ch), (vict) ) \
                          ? real_sdesc( (vict), (ch), CAN_SEE((ch), (vict)) ) \
                          : GET_SEX( (vict) ) == SEX_MALE ? "a male voice" \
                          : GET_SEX( (vict) ) == SEX_FEMALE ? "a female voice" \
                          : "a voice" )

#define GENDER(ch)    (GET_SEX( (ch) ) == SEX_MALE ? "male" \
                      : GET_SEX( (ch) ) == SEX_FEMALE ? "female" : "")

/* note you must have a real_name_buf defined to use the next 2 functions */
#define REAL_NAME(ch) (!IS_NPC(ch) ? GET_NAME(ch) : (IS_SET(ch->specials.act, CFL_UNIQUE) ? first_name(GET_NAME(ch), real_name_buf) : MSTR( ch, short_descr )))

#define FORCE_NAME(ch) (!IS_NPC(ch) ? GET_NAME(ch) : first_name(GET_NAME(ch), real_name_buf))


#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
			 first_name( OSTR((obj), name ),real_name_buf) : "something")

#define THNG(ch, vict)  (CAN_SEE((vict), (ch)) ? (MSTR( (ch), short_descr ) ) : "something")

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	format_obj_to_char( (obj), (vict), 1 ) : "something" )

#define OUTSIDE(ch) ((ch) && \
 ch->in_room &&  \
 !IS_SET(ch->in_room->room_flags, RFL_INDOORS)     &&  \
 ch->in_room->sector_type != SECT_WATER_PLANE     &&  \
 ch->in_room->sector_type != SECT_AIR_PLANE       &&  \
 ch->in_room->sector_type != SECT_EARTH_PLANE     &&  \
 ch->in_room->sector_type != SECT_SHADOW_PLANE    &&  \
 ch->in_room->sector_type != SECT_FIRE_PLANE      &&  \
 ch->in_room->sector_type != SECT_LIGHTNING_PLANE &&  \
 ch->in_room->sector_type != SECT_NILAZ_PLANE     &&  \
 ch->in_room->sector_type != SECT_INSIDE )

#define EXIT(ch, door)  ((ch)->in_room->direction[door])

#define CAN_GO(ch, door) (EXIT(ch, door) && EXIT(ch, door)->to_room && \
                          !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

#define IS_TUNNEL(ch, door) ((IS_SET(EXIT(ch, door)->exit_info, EX_TUNN_1)) ||\
			     (IS_SET(EXIT(ch, door)->exit_info, EX_TUNN_2)) ||\
			     (IS_SET(EXIT(ch, door)->exit_info, EX_TUNN_4)))

#define GET_LANGUAGE(ch) ((ch)->specials.language)

#define STAT_STRING(ch, s)  (how_good_stat[stat_num(ch, s)])

#define STRINGS_SAME(a, b) (strcmp(a, b) ? FALSE : TRUE)

#define STRINGS_I_SAME(a, b) (stricmp(a, b) ? FALSE : TRUE)

#define REPLACE_STRING( pstr, nstr ) \
	{ free( (pstr) ); pstr = strdup( (nstr) ); }

#define IS_NULLSTR( str )       ((str) == NULL || (str)[0] == '\0')

#define WIPE_LDESC(ch) if( (ch)->long_descr ) { \
                           free( (ch)->long_descr ); \
                           (ch)->long_descr = NULL; \
                       }

/* This determines tameable mounts */
#define IS_MOUNTABLE(vict) ((GET_RACE(vict) == RACE_INIX) ||(GET_RACE(vict) == RACE_ERDLU) || (GET_RACE(vict) == RACE_THLON) || (GET_RACE(vict) == RACE_ASLOK) || (GET_RACE(vict) == RACE_GWOSHI) || (GET_RACE(vict) == RACE_KANK) || (GET_RACE(vict) == RACE_CHARIOT) || (GET_RACE(vict) == RACE_ROC) || (GET_RACE(vict) == RACE_SUNBACK) || (GET_RACE(vict) == RACE_WAR_BEETLE) || (GET_RACE(vict) == RACE_RATLON) || (GET_RACE(vict) == RACE_PLAINSOX) || (GET_RACE(vict) == RACE_HORSE) || (GET_RACE(vict)==RACE_SUNLON))

/* For enhanced language support */
#define GET_SPOKEN_LANGUAGE(ch) (language_table[(int)(ch)->specials.language].spoken)
#define GET_WRITTEN_LANGUAGE(ch) (language_table[(int)(ch)->specials.language].written)
#define GET_ACCENT(ch) ((ch)->specials.accent)

#define SWITCH_BLOCK(ch) if( IS_NPC((ch)) ) { cprintf((ch), "Not while switched.\n\r" ); return; } 


/* For regeneration balance */
/* Still investigating this one
#define REGEN_MAX(ch) (1.5 * (dice (race[(int) GET_RACE (ch)].enddice, race[(int) GET_RACE (ch)].endsize))) */
#define REGEN_MAX(ch) (35)

/* Javascript */
#define USE_JAVASCRIPT 1

#define isspecial(x) ((x == '~') || (x == '!') || (x == '%') || (x == '^') || (x == '#') || (x == '&') || (x == '+') || (x == '='))

#endif // __UTIL_H__

