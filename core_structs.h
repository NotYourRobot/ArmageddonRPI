/*
 * File: CORE_STRUCTS.H
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

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __CORE_STRUCTS_INCLUDED
#define __CORE_STRUCTS_INCLUDED

#include <time.h>
#include <stdio.h>

#ifdef MEMDEBUG
#include <memdebug.h>
#endif


/* Added for testing days of the week */
#define DOTW_SUPPORT 1
#define MAX_DOTW 11

/* Added for testing multiple clans, remove to restore the old system */
#define MAX_CLAN_RANK 20
#define MIN_CLAN_RANK 1

#define CLANS_FROM_DB 1

/* Added for reading cities from file instead of stored statically -Morg */
/* comment out to remove */
#define USE_NEW_ROOM_IN_CITY

/* Settings used for time conversion */
#define ZAL_HOURS_PER_DAY   9
#define ZAL_DAYS_PER_MONTH  231
#define ZAL_MONTHS_PER_YEAR 3

#define RT_ZAL_HOUR     600
#define MAX_HOUR	9
#define MAX_DAY         231
#define MAX_MONTH	3
#define MAX_YEAR        77
#define RT_ZAL_DAY      (RT_ZAL_HOUR * ZAL_HOURS_PER_DAY)
#define RT_ZAL_MONTH    (RT_ZAL_DAY * ZAL_DAYS_PER_MONTH)
#define RT_ZAL_YEAR     (RT_ZAL_MONTH * ZAL_MONTHS_PER_YEAR)
#define ZT_EPOCH	1259

typedef signed char sbyte;
typedef unsigned char ubyte;
typedef signed short int sh_int;
typedef unsigned short int ush_int;
#define bool signed char
typedef signed char byte;

typedef struct char_data CHAR_DATA;
typedef struct char_info CHAR_INFO;
typedef struct clan_data CLAN_DATA;

typedef struct descriptor_data DESCRIPTOR_DATA;

typedef struct queued_cmd QUEUED_CMD;
typedef struct extra_cmd EXTRA_CMD;

typedef struct imm_name IMM_NAME;

typedef struct login_data LOGIN_DATA;

typedef struct monitor_list MONITOR_LIST;

typedef struct obj_data OBJ_DATA;

typedef struct plyr_list PLYR_LIST;
typedef struct player_info PLAYER_INFO;

typedef struct room_data ROOM_DATA;

  typedef struct help_data HELP_DATA;

#define HELP_FILE "text_files/requested_helps"  /* For undefined helps */

/* change this to give you more logins saved in the account file */
#define MAX_LOGINS_SAVED 20

/*#define SMELLS    ** comment this to remove smells -Morg */
#ifdef SMELLS
typedef struct smell_data SMELL_DATA;
typedef struct smell_type SMELL_TYPE;
#endif

typedef struct off_def_data OFF_DEF_DATA;

/* time data for players */
struct time_info_data
{
    byte hours;
    int day;
    byte month;
    int year;
};

/* timer for game */
extern struct time_info_data time_info;

extern int gameport;

/* GAME PARAMETERS */
#define PERMANENT_DEATH

/* GAME CONSTANT DEFINITIONS */
#define WAIT_SEC         12     /* thrash - 4 */

#define PULSE_ZONE       (60 * WAIT_SEC)
#define PULSE_MOBILE     20
#define PULSE_OBJECT     40
#define PULSE_ROOM       60
#define PULSE_VIOLENCE   3

#define SECS_PER_REAL_MIN  60
#define SECS_PER_REAL_HOUR (60 * SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY  (24 * SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR (365 * SECS_PER_REAL_DAY)

#define SECS_PER_MUD_HOUR  100
#define SECS_PER_MUD_DAY   (27 * SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH (231 * SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR  (3 * SECS_PER_MUD_MONTH)


#define MAX_STRING_LENGTH  16384
#define MAX_LINE_LENGTH	    1024
#define MAX_INPUT_LENGTH     256
#define MAX_NOTE_LENGTH     2000
#define MAX_BACKGROUND_LENGTH 1810 // needed for web, otherwise gets cut off
#define MAX_MESSAGES          60
#define MAX_ITEMS            153
#define MAX_GUILDS            16
#define MAX_ALIAS             10
#define MAX_GROUPS            11

/* Doubled length, (was 24576), such a kludge - Tiernan */
/* Upped length again, (was 59152), such a kludge - Nessalin */
#define MAX_PAGE_STRING    80000

#define MESS_ATTACKER 1
#define MESS_VICTIM   2
#define MESS_ROOM     3

/* mail constants */
#define NUM_POSTS               5
#define MAIL_EXPIRATION_TIME    40

/* moving money around */
#define MM_CH2OBJ         0     /* ch->coins to coin_obj */
#define MM_CH2POUCH       1     /* put money in cont */
#define MM_OBJ2CH         2     /* coin_obj to ch->coins */
#define MM_CH2CH          3     /* money exchange */



/* OBJECT DEFINITONS */

#define MAX_OBJ_AFFECT 2        /* max number of 'affects' on an obj */
#define OBJ_NOTIMER    -7000000 /* number for no timer */

/* object types */
typedef enum {
    ITEM_LIGHT       =  1,
    ITEM_COMPONENT   =  2,
    ITEM_WAND        =  3,
    ITEM_STAFF       =  4,
    ITEM_WEAPON      =  5,
    ITEM_ART         =  6,
    ITEM_TOOL        =  7,
    ITEM_TREASURE    =  8,
    ITEM_ARMOR       =  9,
    ITEM_POTION      = 10,
    ITEM_WORN        = 11,
    ITEM_OTHER       = 12,
    ITEM_SKIN        = 13,
    ITEM_SCROLL      = 14,
    ITEM_CONTAINER   = 15,
    ITEM_NOTE        = 16,
    ITEM_DRINKCON    = 17,
    ITEM_KEY         = 18,
    ITEM_FOOD        = 19,
    ITEM_MONEY       = 20,
    ITEM_PEN         = 21,
    ITEM_BOAT        = 22,
    ITEM_WAGON       = 23,
    ITEM_DART        = 24,
    ITEM_SPICE       = 25,
    ITEM_CHIT        = 26,
    ITEM_POISON      = 27,
    ITEM_BOW         = 28,
    ITEM_TELEPORT    = 29,
    ITEM_FIRE        = 30,
    ITEM_MASK        = 31,
    ITEM_FURNITURE   = 32,
    ITEM_HERB        = 33,
    ITEM_CURE        = 34,
    ITEM_SCENT       = 35,
    ITEM_PLAYABLE    = 36,
    ITEM_MINERAL     = 37,
    ITEM_POWDER      = 38,
    ITEM_JEWELRY     = 39,
    ITEM_MUSICAL     = 40,
    ITEM_RUMOR_BOARD = 41,

    MAX_ITEM_TYPE    = 42
} ItemType;



/* playable types */
#define PLAYABLE_NONE            0
#define PLAYABLE_DICE            1
#define PLAYABLE_DICE_WEIGHTED   2
#define PLAYABLE_BONES           3
#define PLAYABLE_HALFLING_STONES 4
#define PLAYABLE_GYPSY_DICE      5


/* quiet levels for immortals */
#define QUIET_LOG       (1 << 0)
#define QUIET_WISH      (1 << 1)
#define QUIET_IMMCHAT   (1 << 2)
#define QUIET_GODCHAT   (1 << 3)
#define QUIET_PSI       (1 << 4)
#define QUIET_TYPO      (1 << 5)
#define QUIET_BUG       (1 << 6)
#define QUIET_IDEA      (1 << 7)
#define QUIET_CONT_MOB  (1 << 8)
#define QUIET_DEATH     (1 << 9)
#define QUIET_CONNECT   (1 << 10)
#define QUIET_ZONE      (1 << 11)
#define QUIET_REROLL    (1 << 12)
#define QUIET_COMBAT    (1 << 13)
#define QUIET_MISC      (1 << 14)
#define QUIET_CALC      (1 << 15)
#define QUIET_BANK      (1 << 16)
#define QUIET_NPC_AI    (1 << 17)
#define QUIET_DEBUG     (1 << 18)
#define QUIET_BLDCHAT   (1 << 19)

/* wear flags */
#define ITEM_TAKE              (1 << 0)
#define ITEM_WEAR_FINGER       (1 << 1)
#define ITEM_WEAR_NECK         (1 << 2)
#define ITEM_WEAR_BODY         (1 << 3)
#define ITEM_WEAR_HEAD         (1 << 4)
#define ITEM_WEAR_LEGS         (1 << 5)
#define ITEM_WEAR_FEET         (1 << 6)
#define ITEM_WEAR_HANDS        (1 << 7)
#define ITEM_WEAR_ARMS         (1 << 8)
#define ITEM_WEAR_BACK         (1 << 9)
#define ITEM_WEAR_ABOUT        (1 << 10)
#define ITEM_WEAR_WAIST        (1 << 11)
#define ITEM_WEAR_WRIST        (1 << 12)
#define ITEM_EP                (1 << 13)
#define ITEM_ES                (1 << 14)
#define ITEM_WEAR_BELT         (1 << 15)
#define ITEM_WEAR_ON_BELT      (1 << 16)
#define ITEM_WEAR_IN_HAIR      (1 << 17)
#define ITEM_WEAR_FACE         (1 << 18)
#define ITEM_WEAR_ANKLE        (1 << 19)
#define ITEM_WEAR_LEFT_EAR     (1 << 20)
#define ITEM_WEAR_RIGHT_EAR    (1 << 21)
#define ITEM_WEAR_FOREARMS     (1 << 22)
#define ITEM_WEAR_ABOUT_HEAD   (1 << 23)
#define ITEM_WEAR_ABOUT_THROAT (1 << 24)
#define ITEM_WEAR_SHOULDER     (1 << 25)
#define ITEM_WEAR_OVER_SHOULDER (1 << 26)

/* item materials */
#define MATERIAL_NONE                0
#define MATERIAL_WOOD                1
#define MATERIAL_METAL               2
#define MATERIAL_OBSIDIAN            3
#define MATERIAL_STONE               4
#define MATERIAL_CHITIN              5
#define MATERIAL_GEM                 6
#define MATERIAL_GLASS               7
#define MATERIAL_SKIN                8
#define MATERIAL_CLOTH               9
#define MATERIAL_LEATHER            10
#define MATERIAL_BONE               11
#define MATERIAL_CERAMIC            12
#define MATERIAL_HORN               13
#define MATERIAL_GRAFT              14
#define MATERIAL_TISSUE             15
#define MATERIAL_SILK               16
#define MATERIAL_IVORY              17
#define MATERIAL_DUSKHORN           18
#define MATERIAL_TORTOISESHELL      19
#define MATERIAL_ISILT              20  /* It is like hardened bone or something */
#define MATERIAL_SALT               21
#define MATERIAL_GWOSHI             22
#define MATERIAL_DUNG               23
#define MATERIAL_PLANT              24
#define MATERIAL_PAPER              25
#define MATERIAL_FOOD               26
#define MATERIAL_FEATHER            27
#define MATERIAL_FUNGUS             28
#define MATERIAL_CLAY               29
#define MAX_MATERIAL                31

/* extra flags */
#define OFL_GLOW              (1 << 0)
#define OFL_HUM               (1 << 1)
#define OFL_DARK              (1 << 2)
#define OFL_LOCK              (1 << 3)
#define OFL_EVIL              (1 << 4)
#define OFL_INVISIBLE         (1 << 5)
#define OFL_MAGIC             (1 << 6)
#define OFL_NO_DROP           (1 << 7)
#define OFL_NO_FALL           (1 << 8)
#define OFL_ARROW             (1 << 9)
#define OFL_MISSILE_S         (1 << 10)
#define OFL_MISSILE_L         (1 << 11)
#define OFL_DISINTIGRATE      (1 << 12)
#define OFL_NO_ES             (1 << 13)
#define OFL_ETHEREAL          (1 << 14)
#define OFL_ANTI_MORT         (1 << 15)
#define OFL_NO_SHEATHE        (1 << 16)
#define OFL_HIDDEN            (1 << 17)
#define OFL_GLYPH             (1 << 18)
#define OFL_SAVE_MOUNT        (1 << 19)
#define OFL_POISON            (1 << 20)
#define OFL_BOLT              (1 << 21)
#define OFL_MISSILE_R         (1 << 22) /*  Kel  */
#define OFL_HARNESS           (1 << 23)
#define OFL_FLAME             (1 << 24)
#define OFL_NOSAVE            (1 << 25)
#define OFL_DART              (1 << 26)
#define OFL_DYABLE            (1 << 27)
#define OFL_CAST_OK           (1 << 28) /* Nessalin 9/2/2004 */
#define OFL_ETHEREAL_OK       (1 << 29) /* Nessalin 9/25/2004 */

/* object adverbial flags */
#define OAF_PATTERNED         (1 << 0)
#define OAF_CHECKERED         (1 << 1)
#define OAF_STRIPED           (1 << 2)
#define OAF_BATIKED           (1 << 3)
#define OAF_DYED              (1 << 4)

/* object color flags */
#define OCF_RED               (1 << 0)
#define OCF_BLUE              (1 << 1)
#define OCF_YELLOW            (1 << 2)
#define OCF_GREEN             (1 << 3)
#define OCF_PURPLE            (1 << 4)
#define OCF_ORANGE            (1 << 5)
#define OCF_BLACK             (1 << 6)
#define OCF_WHITE             (1 << 7)
#define OCF_BROWN             (1 << 8)
#define OCF_GREY              (1 << 9)
#define OCF_SILVERY           (1 << 10)

/* object state flags */
#define OST_BLOODY            (1 << 0)
#define OST_DUSTY             (1 << 1)
#define OST_STAINED           (1 << 2)
#define OST_REPAIRED          (1 << 3)
#define OST_GITH              (1 << 4)
#define OST_BURNED            (1 << 5)
#define OST_SEWER             (1 << 6)
#define OST_OLD               (1 << 7)
#define OST_SWEATY            (1 << 8)
#define OST_PATCHED           (1 << 9)
#define OST_EMBROIDERED       (1 << 10)
#define OST_FRINGED           (1 << 11)
#define OST_LACED             (1 << 12)
#define OST_MUDCAKED          (1 << 13)
#define OST_TORN              (1 << 14)
#define OST_TATTERED          (1 << 15)
#define OST_ASH          (1 << 16)

/* coins */
#define COIN_ALLANAK    0
#define COIN_NORTHLANDS 1
#define COIN_KURAC      2
#define COIN_REYNOLTE   3
#define COIN_SOUTHLANDS 4
#define COIN_SALARR     5

#define MAX_COIN_TYPE   6

/* liquids */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_SPICE_ALE  6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_SPIRITS    15
#define LIQ_SAND       16
#define LIQ_SILT       17
#define LIQ_GLOTH            18
#define LIQ_HONEY_MEAD       19
#define LIQ_BRANDY           20
#define LIQ_SPICE_BRANDY     21
#define LIQ_BELSHUN_WINE     22
#define LIQ_JALLAL_WINE      23
#define LIQ_JAPUAAR_WINE     24
#define LIQ_KALAN_WINE       25
#define LIQ_PETOCH_WINE      26
#define LIQ_HORTA_WINE       27
#define LIQ_GINKA_WINE       28
#define LIQ_LICHEN_WINE      29
#define LIQ_OCOTILLO_WINE    30
#define LIQ_BLACK_ANAKORE    31
#define LIQ_FIRESTORM_FLAME  32
#define LIQ_ELF_TEA          33
#define LIQ_DWARF_ALE        34
#define LIQ_JALUR_WINE       35
#define LIQ_REYNOLTE_DRY     36
#define LIQ_SPICED_WINE      37
#define LIQ_HONIED_WINE      38
#define LIQ_HONIED_ALE       39
#define LIQ_HONIED_BRANDY    40
#define LIQ_SPICED_MEAD      41
#define LIQ_PAINT           42
#define LIQ_OIL             43
#define LIQ_VIOLET_OCOTILLO_WINE  44
#define LIQ_PURPLE_OCOTILLO_WINE  45
#define LIQ_OCOTILLO_SPIRITS 46
#define LIQ_CLEANSER         47
#define LIQ_HONIED_TEA       48
#define LIQ_FRUIT_TEA        49
#define LIQ_MINT_TEA         50
#define LIQ_CLOVE_TEA        51
#define LIQ_KUMISS           52
#define LIQ_JIK              53
#define LIQ_BADU             54
#define LIQ_WATER_VIVADU     55 /* For create water spell */
#define LIQ_WATER_VERY_DIRTY 56 /* Worst water possible   */
#define LIQ_WATER_DIRTY      57 /* Middle quality water   */
#define LIQ_LIQUID_FIRE      58 /* Same as gloth without being called gloth */
#define LIQ_SAP              59
#define LIQ_SLAVEDRIVER      60 /* Some private reserve of the Sand Lords for Gesht */
#define LIQ_SPICEWEED_TEA    61 /* Added for Vendyra */
#define LIQ_SPICED_BELSHUN_WINE   62    /* Added for Mekeda 2/14/04 */
#define LIQ_SPICED_GINKA_WINE     63    /* Added for Mekeda 2/14/04 */
#define LIQ_AGVAT                 64    /* Added for Mekeda 3/20/2004 */
#define LIQ_BELSHUN_JUICE         65
#define LIQ_GINKA_JUICE           66
#define LIQ_HORTA_JUICE           67
#define LIQ_JAPUAAR_JUICE         68
#define LIQ_KALAN_JUICE           69
#define LIQ_MELON_JUICE           70
#define LIQ_THORNFRUIT_JUICE      71
#define LIQ_FRUIT_JUICE           72
#define LIQ_YEZZIR_WHISKEY	  73
#define LIQ_SEJAH_WHISKEY	  74
#define LIQ_BLACKSTEM_TEA	  75
#define LIQ_DWARFFLOWER_TEA	  76
#define LIQ_GREENMANTLE_TEA	  77
#define LIQ_KANJEL_TEA	          78
#define LIQ_TULUKI_COMMON_TEA     79  // Tuluki specializations
#define LIQ_TULUKI_CHOSEN_TEA     80  // Tuluki specializations
#define LIQ_WATCHMAN_TEA          81  // Akai Sjir recipes
#define LIQ_SPICER_TEA            82  // Akai Sjir recipes
#define LIQ_MOON_TEA              83  // Akai Sjir recipes
#define LIQ_SPARKLING_WINE        84
#define LIQ_BAM_BERRY_SPIRITS     85  // Kuraci Mastercraft for Bam
#define LIQ_MARILLUM_BASIC        86  // Kadian Mastercraft for Mar
#define LIQ_MARILLUM_AGED         87  // Kadian Mastercraft for Mar

#define MAX_LIQ        88

#define LIQFL_POISONED (1 << 0)
#define LIQFL_INFINITE (1 << 1)

/* food */
#define FOOD_MEAT	0
#define FOOD_BREAD	1
#define FOOD_FRUIT	2
#define FOOD_HORTA	3
#define FOOD_GINKA	4
#define FOOD_PETOCH	5
#define FOOD_FRUIT2	6
#define FOOD_MUSH	7
#define FOOD_SWEET      8
#define FOOD_SNACK      9
#define FOOD_SOUP       10
#define FOOD_TUBER      11
#define FOOD_BURNED     12
#define FOOD_SPOILED    13
#define FOOD_CHEESE     14
#define FOOD_PEPPER     15
#define FOOD_SOUR       16
#define FOOD_VEGETABLE  17
#define FOOD_HONEY      18
#define FOOD_MELON      19
#define FOOD_GRUB       20
#define FOOD_ANT        21
#define FOOD_INSECT     22
#define MAX_FOOD	23

#define FOOD_POISONED (1 << 0)

/* containers */
#define CONT_CLOSEABLE      ( 1 << 0 )
#define CONT_PICKPROOF      ( 1 << 1 )
#define CONT_CLOSED         ( 1 << 2 )
#define CONT_LOCKED         ( 1 << 3 )
#define CONT_TRAPPED        ( 1 << 4 )
#define CONT_HOODED         ( 1 << 5 )
#define CONT_HOOD_UP        ( 1 << 6 )
#define CONT_CORPSE_HEAD    ( 1 << 7 )
#define CONT_CLOAK          ( 1 << 8 )
#define CONT_SHOW_CONTENTS  ( 1 << 10 )
#define CONT_KEY_RING       ( 1 << 11 )
#define CONT_SEALED         ( 1 << 12 )
#define CONT_SEAL_BROKEN    ( 1 << 13 )

/* furniture */
#define FURN_SIT            (1 << 0)
#define FURN_SLEEP          (1 << 1)
#define FURN_REST           (1 << 2)
#define FURN_PUT            (1 << 3)
#define FURN_TABLE          (1 << 4)
#define FURN_SKIMMER        (1 << 5)
#define FURN_STAND          (1 << 6)
#define FURN_WAGON          (1 << 7)

/* lights */
#define LIGHT_TYPE_FLAME    1
#define LIGHT_TYPE_MAGICK   2
#define LIGHT_TYPE_PLANT    3
#define LIGHT_TYPE_MINERAL  4
#define LIGHT_TYPE_ANIMAL   5

/* light colors */
#define LIGHT_COLOR_NONE     0
#define LIGHT_COLOR_RED      1
#define LIGHT_COLOR_BLUE     2
#define LIGHT_COLOR_YELLOW   3
#define LIGHT_COLOR_GREEN    4
#define LIGHT_COLOR_PURPLE   5
#define LIGHT_COLOR_ORANGE   6
#define LIGHT_COLOR_WHITE    7
#define LIGHT_COLOR_AMBER    8
#define LIGHT_COLOR_PINK     9
#define LIGHT_COLOR_SENSUAL 10
#define LIGHT_COLOR_BLACK 11

/* light flags */
#define LIGHT_FLAG_LIT         (1 << 0)
#define LIGHT_FLAG_REFILLABLE  (1 << 1)
#define LIGHT_FLAG_CONSUMES    (1 << 2)
#define LIGHT_FLAG_TORCH       (1 << 3)
#define LIGHT_FLAG_LAMP        (1 << 4)
#define LIGHT_FLAG_LANTERN     (1 << 5)
#define LIGHT_FLAG_CANDLE      (1 << 6)
#define LIGHT_FLAG_CAMPFIRE    (1 << 7)
#define LIGHT_FLAGS            (0xff)

#define NEW_LIGHT_CODE

/* wagons */
/*
#define WAGON_CAN_GO_LAND      1
#define WAGON_CAN_GO_SILT      2
#define WAGON_CAN_GO_AIR       3
#define WAGON_NO_LEAVE         4
*/

#define NOTE_SEALED            (1 << 0)
#define NOTE_BROKEN            (1 << 1)
#define NOTE_SEALABLE            (1 << 2)

#define WAGON_CAN_GO_LAND      (1 << 0)
#define WAGON_CAN_GO_SILT      (1 << 1)
#define WAGON_CAN_GO_AIR       (1 << 2)
#define WAGON_NO_LEAVE         (1 << 3)
#define WAGON_REINS_LAND       (1 << 4)
#define WAGON_MOUNT_NO         (1 << 5)
#define WAGON_NO_ENTER         (1 << 6)
#define WAGON_NO_PSI_PATH      (1 << 7)

/* rein positions */
#define REINS_LAND          0   /* on the ground outside of the wagon */
#define REINS_WAGON         1   /* in the wagon */

/* tool types */
#define TOOL_NONE           0
#define TOOL_CLIMB          1   /* ropes, grappling hooks... */
#define TOOL_TRAP           2   /* exploding traps */
#define TOOL_UNTRAP         3   /* removing traps */
#define TOOL_SNARE          4   /* snares to catch people, animals */
#define TOOL_BANDAGE        5   /* wrap up nasty wounds */
#define TOOL_LOCKPICK       6   /* lockpicking kits */
#define TOOL_HONE           7   /* sharpen stuff */
#define TOOL_MEND_SOFT      8   /* work on soft materials */
#define TOOL_MEND_HARD      9   /* work on hard materials */
#define TOOL_TENT          10   /* pitchable tents */
#define TOOL_FUSE          11   /* timed exploding traps */
#define TOOL_CHISEL        12   /* tool of type chisel */
#define TOOL_HAMMER        13   /* tool of type hammer */
#define TOOL_SAW           14   /* he picked up his hammer and saw.. */
#define TOOL_NEEDLE        15   /* Needle, as in sewing */
#define TOOL_AWL           16   /* Hole poker thingy */
#define TOOL_SMOOTHING     17   /* files, sandpaper, etc */
#define TOOL_SHARPENING    18   /* whetstones, etc */
#define TOOL_GLASS         19   /* magnifying, lenses, etc */
#define TOOL_PAINTBRUSH    20   /* paint paint paint */
#define TOOL_SCRAPER       21   /* remove the above paint */
#define TOOL_FLETCHERY     22   /* Native American wrench--NOT METRIC */
#define TOOL_DRILL         23   /* drilling things */
#define TOOL_FABRIC        24   /* making things out of cloth */
#define TOOL_HOUSEHOLD     25   /* brooms, rakes, etc */
#define TOOL_SCENT         26   /* incense */
#define TOOL_WOODCARVING   27   /* self-explanatory */
#define TOOL_BUILDING      28   /* nails and such */
#define TOOL_VISE          29   /* vise */
#define TOOL_INCENSE       30   /* incense */
#define TOOL_CLEANING      31   /* soap and such */
#define TOOL_SPICE         32   /* drug paraphernalia */
#define TOOL_COOK          33   /* seasonings and other cooking stuff */
#define TOOL_BREW          34
#define TOOL_KILN          35   /* Used for making ceramics that require consistent/high tempreature, glazing, etc... */
#define TOOL_LOOM_SMALL    36   
#define TOOL_LOOM_LARGE    37   
#define MAX_TOOLS          38


/* treasure types */
#define TREASURE_GEM         0
#define TREASURE_FIGURINE    1
#define TREASURE_ART         2
#define TREASURE_WIND        3
#define TREASURE_STRINGED    4
#define TREASURE_PERCUSSION  5
#define TREASURE_ORE         6
#define TREASURE_JEWELRY     7

#define MAX_TREASURE	     8

/* mineral types */
/* intended for use with brew and forage -sanvean */

#define MINERAL_NONE         0  /* Just in case.            */
#define MINERAL_STONE        1  /* Stones and rocks.        */
#define MINERAL_ASH          2  /* Ash and dusts.           */
#define MINERAL_SALT         3  /* Salts and concentrates.  */
#define MINERAL_ESSENCE      4  /* Essences and essentials. */
#define MINERAL_MAGICKAL     5  /* Special stuff.           */

#define MAX_MINERAL          6

/* powder types */
/* intended for use with brew -sanvean */

#define POWDER_NONE          0  /* Just in case.            */
#define POWDER_FINE          1  /* The usual form.          */
#define POWDER_GRIT          2  /* Forageable.              */
#define POWDER_TEA           3  /* Fairly rare.             */
#define POWDER_SALT          4  /* Requires mineral salt.   */
#define POWDER_ESSENCE       5  /* Essential nature.        */
#define POWDER_MAGICKAL      6  /* Special stuff.           */

#define MAX_POWDER           7

/* instrument types */
#define INSTRUMENT_NONE 		0
#define INSTRUMENT_BELLS 		1
#define INSTRUMENT_BRASS		2
#define INSTRUMENT_CTHULHU		3
#define INSTRUMENT_GONG			4
#define INSTRUMENT_GOURD		5
#define INSTRUMENT_LARGE_DRUM	        6
#define INSTRUMENT_MAGICK_FLUTE         7
#define INSTRUMENT_MAGICK_HORN	        8
#define INSTRUMENT_PERCUSSION	        9
#define INSTRUMENT_RATTLE		10
#define INSTRUMENT_SMALL_DRUM	        11
#define INSTRUMENT_STRINGED		12
#define INSTRUMENT_WHISTLE		13
#define INSTRUMENT_WIND			14
#define INSTRUMENT_DRONE                15

#define MAX_INSTRUMENT          16

#ifdef SMELLS

#define SMELL_TRACE		  0
#define SMELL_FAINT		 10
#define SMELL_WEAK		 20
#define SMELL_MILD		 30
#define SMELL_CERTAIN		 40
#define SMELL_NORMAL             50
#define SMELL_NOTICEABLE	 60
#define SMELL_STRONG 		 70
#define SMELL_VERY_STRONG	 80
#define SMELL_OVERWHELMING	 90
#define MAX_SMELL_INTENSITY	100



/* the type of smell */
struct smell_type
{
    int index;
    char *description;
};

/* generic smells */
/* smell data that will be on every obj, ch, room */
struct smell_data
{
    int intensity;
    int timer;
    SMELL_TYPE type;
    SMELL_DATA *next;
};

struct smell_constant
{
    char *name;
    char *description;
};

extern struct smell_constant smells_table[];
#endif

/* offense/defense computations */
struct off_def_data {
    OBJ_DATA *wp_pri;
    OBJ_DATA *wp_sec;
    OBJ_DATA *wp_vict;
    OBJ_DATA *shield_vict;
    int c_wtype;
    int v_wtype;
    int offense;
    int defense;
};


/* generic void * linked list */
struct obj_list_type
{
    void *obj;
    struct obj_list_type *next;
};

/* oset structure */
struct oset_field_data
{
    const char *set[6];
    const char *description[6];
    const char *extra_desc[6];
};

struct coin_data
{
    int parent;                 /* The parent coin type          */
    int rel_value;              /* Relative value to parent coin */
    int sing_vnum;              /* The vnum for 1 coin           */
    int some_vnum;              /* The vnum for 2-20 coins       */
    int many_vnum;              /* The vnum for 21+ coins        */
    char *title;                /* string used by game for coins */
};

/* edesc structure */
struct extra_descr_data
{
    char *keyword;              /* keyword list for edesc */
    char *description;          /* description */
    unsigned char def_descr;            /* this data resides on the default npc/obj */
    struct extra_descr_data *next;
};

// FIXME:  Xygax -- make "type" be "ItemType" here, and fix the warnings
// generated by that change.
struct obj_flag_data
{
    int value[6];               /* generic item attribute values */
    byte type;                  /* item type (weapon, container, food, etc.) */
    byte material;              /* what the item is made of */
    long wear_flags;            /* bitvector for 'wear' locations */
    int extra_flags;            /* various other object flags */
    long state_flags;           /* bitvector for 'state' */
    long color_flags;           /* bitvector for 'color' */
    long adverbial_flags;       /* bitvector for 'adverbial' */
    int weight;                 /* object weight (stone) */
    int size;                   /* longest dimension (usually) */
    int length;                 /* No, really, this is length */
    int cost;                   /* object's 'real' value */
#define USE_OBJ_FLAG_TEMP 1
#ifdef USE_OBJ_FLAG_TEMP
    int temp;                   /* <zap sometime soon>, used in glyph for damage */
#endif
    int timer;                  /* timer for object */
    int hometown;               /* city the object originated from */
    int clan;                   /* clan that made the object */
    long bitvector;             /* character flags to set */
};

struct obj_affected_type
{
    int location;               /* which attribute to modify */
    sbyte modifier;             /* how much to modify it by */
};

struct obj_data
{
    int nr;                     /* object number                       */
    ROOM_DATA *in_room;         /* what room the object is in (if any) */
    struct obj_flag_data obj_flags;     /* flag data for objects               */
    unsigned int random_seed;   /* seed for the randomness             */

    struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* object affects */

    char *name;                 /* object keyword list            */
    char *long_descr;           /* description in room            */
    char *short_descr;          /* short string describing object */
    char *description;          /* actual description             */
    char *ldesc;                /* temporary long description     */

    struct extra_descr_data *ex_description;    /* extra descriptions */

    struct specials_t *programs;        /* special programs */

    struct guard_type *guards;  /* list of characters guarding obj   */
    struct follow_type *followers;      /* list of characters following obj  */

#define AZROEN_OBJ_FOLLOW_CODE 1
#ifdef AZROEN_OBJ_FOLLOW_CODE
    struct object_follow_type *object_followers;        /* list of objects following obj */
    OBJ_DATA *wagon_master;     /* Object that obj is following */
#endif

    struct harness_type *beasts;        /* list of creatures pulling the obj */

#ifdef WAGON_DAMAGE_VALUES
    struct wagon_damage_type wagon_damage;      /* wagon damage locations */
#endif

    PLYR_LIST *lifted_by;       /* people who are lifting the object */

    CHAR_DATA *carried_by;      /* person who's carrying the object  */
    CHAR_DATA *equipped_by;     /* person who equipped the object    */

    OBJ_DATA *in_obj;           /* what object is this one inside    */
    OBJ_DATA *contains;         /* list of objects inside this one   */

/* Furniture stuff - Morg */
    PLYR_LIST *occupants;       /* list of characters on the object  */
    OBJ_DATA *around;           /* what objects are around the obj   */
    OBJ_DATA *table;            /* what object the obj is around     */
/* End Furniture stuff */


#ifdef SMELLS
    SMELL_DATA *smells;         /* smells on the object */
#endif

    /* every item will have a damage thing, for if  */
    /*  wielded                                     */

    byte dam_dice_size;
    byte dam_num_dice;
    byte dam_plus;

    /* every item will have a damage capacity (hit points) for being damaged */
    byte max_hit_points;
    byte hit_points;

    OBJ_DATA *prev_content;
    OBJ_DATA *next_content;

    OBJ_DATA *prev;
    OBJ_DATA *next;

    OBJ_DATA *prev_num;
    OBJ_DATA *next_num;

    void *jsobj;
};


struct obj_default_data
{
    char *name;
    char *short_descr;
    char *long_descr;
    char *description;

    struct extra_descr_data *exdesc;

    int value[6];
    ItemType type;
    byte material;
    long wear_flags;
    int extra_flags;
    long state_flags;
    long color_flags;
    long adverbial_flags;

    int weight;
    sh_int size;
    int cost;
    int temp;
    int length;

    int dam_dice_size;
    int dam_num_dice;
    int dam_plus;

    /* every item will have a damage capacity (hit points) for being damaged */
    int max_hit_points;
    int hit_points;

#ifdef SMELLS
    SMELL_DATA *smells;
#endif

    struct specials_t *programs;

    struct obj_affected_type affected[MAX_OBJ_AFFECT];
};

/* ROOM DEFINITIONS */

#define NOWHERE    -1           /* for non-existent rooms */

/* room flags */
#define RFL_DARK           (1 << 0)
#define RFL_DEATH          (1 << 1)
#define RFL_NO_MOB         (1 << 2)
#define RFL_INDOORS        (1 << 3)
#define RFL_SAFE           (1 << 4)
#define RFL_PLANT_SPARSE   (1 << 5)
#define RFL_PLANT_HEAVY    (1 << 6)
#define RFL_NO_MAGICK      (1 << 7)
#define RFL_TUNNEL         (1 << 8)
#define RFL_PRIVATE        (1 << 9)
#define RFL_SANDSTORM      (1 << 10)
#define RFL_FALLING        (1 << 11)
#define RFL_DMANAREGEN     (1 << 12)
#define RFL_UNSAVED        (1 << 13)
#define RFL_NO_CLIMB       (1 << 14)
#define RFL_NO_MOUNT       (1 << 15)
#define RFL_NO_WAGON       (1 << 16)
#define RFL_NO_FLYING      (1 << 17)
#define RFL_NO_FORAGE      (1 << 18)
#define RFL_POPULATED      (1 << 19)
#define RFL_RESTFUL_SHADE  (1 << 20)
#define RFL_NO_PSIONICS    (1 << 21)
#define RFL_ASH            (1 << 22)
#define RFL_SPL_ALARM      (1 << 23)
#define RFL_MAGICK_OK      (1 << 24)
#define RFL_ALLANAK        (1 << 25)
#define RFL_TULUK          (1 << 26)
#define RFL_LIT            (1 << 27)
#define RFL_IMMORTAL_ROOM  (1 << 28)
#define RFL_CARGO_BAY      (1 << 29)
#define RFL_NO_ELEM_MAGICK (1 << 30)
#define RFL_NO_HIDE        (1 << 31)

/* directions */
#define DIR_UNKNOWN -1
#define DIR_NORTH    0
#define DIR_EAST     1
#define DIR_SOUTH    2
#define DIR_WEST     3
#define DIR_UP       4
#define DIR_DOWN     5

#define MAX_DIRECTIONS     4


#define DIR_OUT 6               // This is ONLY used for cmd_guard (and weather) (and cmd_shoot)
#define DIR_IN  7

#define MAX_BDESC	 100

/* exit flags */
#define EX_ISDOOR            (1 << 0)
#define EX_CLOSED            (1 << 1)
#define EX_LOCKED            (1 << 2)
#define EX_RSCLOSED          (1 << 3)
#define EX_RSLOCKED          (1 << 4)
#define EX_SECRET            (1 << 6)
#define EX_SPL_SAND_WALL     (1 << 7)
#define EX_NO_CLIMB          (1 << 8)
#define EX_TRAPPED           (1 << 9)
#define EX_RUNES             (1 << 10)
#define EX_NO_FLEE           (1 << 11)
#define EX_DIFF_1            (1 << 12)
#define EX_DIFF_2            (1 << 13)
#define EX_DIFF_4            (1 << 14)
#define EX_TUNN_1            (1 << 15)
#define EX_TUNN_2            (1 << 16)
#define EX_TUNN_4            (1 << 17)
#define EX_GUARDED           (1 << 18)
#define EX_THORN             (1 << 19)
#define EX_SPL_FIRE_WALL     (1 << 20)
#define EX_SPL_WIND_WALL     (1 << 21)
#define EX_SPL_BLADE_BARRIER (1 << 22)
#define EX_SPL_THORN_WALL    (1 << 23)
#define EX_TRANSPARENT       (1 << 24)
#define EX_PASSABLE          (1 << 25)
#define EX_MISSILE_OK        (1 << 26)
#define EX_WINDOWED          (1 << 27)
#define EX_NO_KEY            (1 << 28)
#define EX_NO_OPEN           (1 << 29)
#define EX_NO_LISTEN         (1 << 30)
#define EX_CLIMBABLE         (1 << 31)
#define EX_RAISE_LOWER       (1 << 32)

/* sector types */
#define SECT_INSIDE          0
#define SECT_CITY            1
#define SECT_FIELD           2
#define SECT_DESERT          3
#define SECT_HILLS           4
#define SECT_MOUNTAIN        5
#define SECT_SILT            6
#define SECT_AIR             7
#define SECT_NILAZ_PLANE     8
#define SECT_LIGHT_FOREST    9
#define SECT_HEAVY_FOREST    10
#define SECT_SALT_FLATS      11
#define SECT_THORNLANDS      12
#define SECT_RUINS           13
#define SECT_CAVERN          14
#define SECT_ROAD            15
#define SECT_SHALLOWS        16
#define SECT_SOUTHERN_CAVERN 17
#define SECT_SEWER           18
#define SECT_COTTONFIELD     19
#define SECT_GRASSLANDS      20

#define SECT_FIRE_PLANE      21
#define SECT_WATER_PLANE     22
#define SECT_EARTH_PLANE     23
#define SECT_SHADOW_PLANE    24
#define SECT_AIR_PLANE       25
#define SECT_LIGHTNING_PLANE 26


#define MAX_SECTS 26

/* direction structure */
struct room_direction_data
{
    char *general_description;  /* for looking in a direction */
    char *keyword;              /* door keywords */
    int exit_info;              /* exit/door flags */
    int key;                    /* number of key to unlock door */
    byte lock_class;            /* how good is the lock? */

    int to_room_num;            /* temporary exit room number */
    ROOM_DATA *to_room;         /* exit room */
};

int path_from_to_for(ROOM_DATA * in_room, ROOM_DATA * tgt_room, int max_dist, int max_string_len,
                     char *path, CHAR_DATA * ch);

/* structure for fast path from to execution */
struct path_list_type
{
    int run_number;             /* how to keep track weather searched or not */
    int distance;               /* distance from starting room */
    ROOM_DATA *prev_list;       /* room previous to this one   */
    ROOM_DATA *next_search;     /* room to search next after this one */
    char command[100];          /* string of how to get here from prev room    */
};

struct character_track
{
    int race;
    int weight;
    byte dir;
    byte into;
};

struct group_track
{
    int race;
    int number;
};

struct wagon_track
{
    byte dir;
    byte into;
    int weight;
};

struct sleep_track
{
    int race;
    int hours;
};

struct battle_track
{
    /*  int race1; */
    /*  int race2; */
    int number;
};

struct tent_track
{
    int weight;
};

struct fire_track
{
    int number;
};

struct pour_track
{
    int number;
    int liquid_type;
};

struct storm_track
{
    int number;
};

#define TRACK_NONE                0
#define TRACK_WALK_CLEAN          1
#define TRACK_RUN_CLEAN           2
#define TRACK_SNEAK_CLEAN         3
#define TRACK_WALK_BLOODY         4
#define TRACK_RUN_BLOODY          5
#define TRACK_SNEAK_BLOODY        6
#define TRACK_GROUP               7
#define TRACK_WAGON               8
#define TRACK_SLEEP               9
#define TRACK_BATTLE              10
#define TRACK_SLEEP_BLOODY        11
#define TRACK_WALK_GLOW           12
#define TRACK_RUN_GLOW            13
#define TRACK_SNEAK_GLOW          14
#define TRACK_TENT                15
#define TRACK_FIRE                16
#define TRACK_POUR                17
#define TRACK_STORM               18

union all_tracks
{
    struct character_track ch;  /* I use this as the generic one */
    struct group_track group;
    struct wagon_track wagon;
    struct sleep_track sleep;
    struct battle_track battle;
    struct tent_track tent;
    struct fire_track fire;
};

/* structure for remembering who passed through room */
struct room_tracks
{
    time_t time;                /* time stamp the tracks were left */
    int skill;                  /* skill roll needed to spot them  */
    unsigned char type;         /* up to 256 differnt types        */
    union all_tracks track;
};


#define MAX_TRACKS 5

/********************************/
/* special_js.c --- Javascripts */
/* added 2002/09/21 -Tenebrius  */
/********************************/
#define MAX_COMPILED_SCRIPTS 2000
struct compiled_script_values
{
    char name[200];
    void *script;
    void *script_obj;
    char *script_text;
    int num_times_run;
    long long int avg_clocks;
    long long int max_clocks;
};

struct compile_scripts
{
    int num_scripts;
    struct compiled_script_values script_item[MAX_COMPILED_SCRIPTS];

};

#define WATCH_VIEW        (1 << 0)
#define WATCH_LISTEN      (1 << 1)

/* watching rooms list */
struct watching_room_list
{
    ROOM_DATA *room_watching;
    ROOM_DATA *room_watched;
    char view_msg_string[80];
    char listen_msg_string[80];
    int watch_room_num;         /* temporary exit room number, for loading */
    int watch_type;

    struct watching_room_list *next_watching;
    struct watching_room_list *next_watched_by;
};

/* room structure */

struct room_data
{
    int number;                 /* room number */

    struct path_list_type path_list;    /* this is to speed up searching rooms */

    int zone;                   /* zone room belongs to */
    byte sector_type;           /* sector type */
    char *name;                 /* room name */
    int bdesc_index;            /* index of background description */
    char *description;          /* room description */

    struct extra_descr_data *ex_description;    /* 'extra' descriptions in the room */

    struct room_direction_data *direction[6];   /* direction structures */

    int room_flags;             /* room flags */
    byte light;                 /* number of lightsources in room */

    int city;                   /* current city the room is in */

    struct specials_t *specials;        /* special procedures for this mob/obj      */

    byte state;                 /* special, used in asml/c programs */

    OBJ_DATA *contents;         /* list of items in room */
    CHAR_DATA *people;          /* list of chars in room */
    ROOM_DATA *next;
    ROOM_DATA *next_in_zone;
    ROOM_DATA *wagon_exit;

    struct room_tracks tracks[MAX_TRACKS];
    struct watching_room_list *watching;
    struct watching_room_list *watched_by;

#ifdef SMELLS
    SMELL_DATA *smells;
#endif

    void *jsobj;
};

/* background descriptions */

struct bdesc_data
{
    char *name;                 /* name of background desc.  never shown to players */
    char *desc;                 /* background description */
};

/* CHARACTER DEFINITIONS */

/* wear locations */
#define WEAR_BELT        0
#define WEAR_FINGER_R    1
#define WEAR_FINGER_L    2
#define WEAR_NECK        3
#define WEAR_BACK        4
#define WEAR_BODY        5
#define WEAR_HEAD        6
#define WEAR_LEGS        7
#define WEAR_FEET        8
#define WEAR_HANDS       9
#define WEAR_ARMS       10
#define WEAR_ON_BELT_1  11
#define WEAR_ABOUT      12
#define WEAR_WAIST      13
#define WEAR_WRIST_R    14
#define WEAR_WRIST_L    15
#define EP              16
#define ES                17
#define WEAR_ON_BELT_2    18
#define ETWO              19
#define WEAR_IN_HAIR      20
#define WEAR_FACE         21
#define WEAR_ANKLE        22
#define WEAR_LEFT_EAR     23
#define WEAR_RIGHT_EAR    24
#define WEAR_FOREARMS     25
#define WEAR_ABOUT_HEAD   26
#define WEAR_ANKLE_L      27
#define WEAR_FINGER_R2    28
#define WEAR_FINGER_L2    29
#define WEAR_FINGER_R3    30
#define WEAR_FINGER_L3    31
#define WEAR_FINGER_R4    32
#define WEAR_FINGER_L4    33
#define WEAR_FINGER_R5    34
#define WEAR_FINGER_L5    35
#define WEAR_ABOUT_THROAT 36
#define WEAR_SHOULDER_R   37
#define WEAR_SHOULDER_L   38
#define WEAR_ON_BACK      39
#define WEAR_OVER_SHOULDER_R   40
#define WEAR_OVER_SHOULDER_L   41

#define MAX_WEAR      42        /* maximum 'wear' locations */

/* limits for character attrributes */
#define OLD_MAX_TONGUE  3

#define MAX_TONGUES   14        /* maximum languages */

#include "guilds.h"
/* whenever this number goes up, it should match MAX_SKL_AFF_NAME    */
/* and an entry made in constants.c to reflect the skill name        */


/* conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2


/* DRINK_CONTAINER_ */
/*drunk, hunger, thirst, flamability */

#define DRINK_CONTAINER_DRUNK        0
#define DRINK_CONTAINER_FULL         1
#define DRINK_CONTAINER_THIRST       2
#define DRINK_CONTAINER_FLAMABILITY  3
#define DRINK_CONTAINER_RACE         4
#define DRINK_CONTAINER_DRUNK2       5
#define DRINK_CONTAINER_FULL2        6
#define DRINK_CONTAINER_THIRST2      7

/* flying levels */
#define FLY_NO       0          /* Doesnt have CHAR_AFF_FLYING */
#define FLY_FEATHER  1          /* affected by feather fall spell */
#define FLY_LEVITATE 2          /* affected by levitate spell */
#define FLY_SPELL    3          /* affected by fly spell */
#define FLY_NATURAL  4          /* flying, not affected by any spell */

/* Drunk levels */
#define DRUNK_LIGHT   1         /* Buzzing, and thats about it */
#define DRUNK_MEDIUM  2         /* Got a good beer buzz, slurring speech */
#define DRUNK_HEAVY   3         /*  Time to call a cab home */
#define DRUNK_SMASHED 4         /* Can't walk/talk straight, etc */
#define DRUNK_DEAD    5         /* Totally intoxed.. passing out stage */

/* languages spoken */
#define SIRIHISH     0
#define BENDUNE      1
#define ALLUNDEAN    2
#define MIRUKKIM     3
#define KENTU        4
#define NRIZKT       5
#define UNTAL        6
#define CAVILISH     7
#define TATLUM       8
#define ANYAR        9
#define HESRAK      10
#define VLORAN      11
#define DOMAT       12
#define GHATTI      13



/* Added for enhanced language support */
struct language_data
{
    int spoken;                 /* The skill number for speaking the language */
    int written;                /* The skill number for writing/reading the language */
    int vowel_perc;             /* The percentage of vowels per word in the language */
};

struct accent_data
{
    int skillnum;               /* The skill number of the accent */
    char *name;                 /* The name of the accent (for change) */
    char *msg;                  /* The string which describes the accent */
};

#define MAX_WEAR_EDESC 18

struct wear_edesc_struct
{
    char *name;
    char *edesc_name;
    int location_that_covers;
};


/* different spell/skill/other effects */
#define CHAR_AFF_BLIND             (1 << 0)
#define CHAR_AFF_INVISIBLE         (1 << 1)
#define CHAR_AFF_NOTHING           (1 << 2)
#define CHAR_AFF_DETECT_INVISIBLE  (1 << 3)
#define CHAR_AFF_DETECT_MAGICK     (1 << 4)
#define CHAR_AFF_INVULNERABILITY   (1 << 5)
#define CHAR_AFF_DETECT_ETHEREAL   (1 << 6)
#define CHAR_AFF_SANCTUARY         (1 << 7)
#define CHAR_AFF_GODSPEED          (1 << 8)
#define CHAR_AFF_SLEEP             (1 << 9)
#define CHAR_AFF_NOQUIT            (1 << 10)
#define CHAR_AFF_FLYING            (1 << 11)
#define CHAR_AFF_CLIMBING          (1 << 12)
#define CHAR_AFF_SLOW              (1 << 13)
#define CHAR_AFF_SUMMONED          (1 << 14)
#define CHAR_AFF_BURROW            (1 << 15)
#define CHAR_AFF_ETHEREAL          (1 << 16)
#define CHAR_AFF_INFRAVISION       (1 << 17)
#define CHAR_AFF_CATHEXIS          (1 << 18)
#define CHAR_AFF_SNEAK             (1 << 19)
#define CHAR_AFF_HIDE              (1 << 20)
#define CHAR_AFF_FEEBLEMIND        (1 << 21)
#define CHAR_AFF_CHARM             (1 << 22)
#define CHAR_AFF_SCAN              (1 << 23)
#define CHAR_AFF_CALM              (1 << 24)
#define CHAR_AFF_DAMAGE            (1 << 25)
#define CHAR_AFF_LISTEN            (1 << 26)
#define CHAR_AFF_DETECT_POISON     (1 << 27)
#define CHAR_AFF_SUBDUED           (1 << 28)
#define CHAR_AFF_DEAFNESS          (1 << 29)
#define CHAR_AFF_SILENCE           (1 << 30)
#define CHAR_AFF_PSI               (1 << 31)

/* skill flags */
#define SKILL_FLAG_LEARNED         (1 << 0)
#define SKILL_FLAG_NOGAIN          (1 << 1)
#define SKILL_FLAG_HIDDEN          (1 << 2)

/* damaged body locations */
#define DAMAGED_HEAD               (1 << 0)
#define DAMAGED_LARM               (1 << 1)
#define DAMAGED_RARM               (1 << 2)
#define DAMAGED_LLEG               (1 << 3)
#define DAMAGED_RLEG               (1 << 4)
#define DAMAGED_LHAND              (1 << 5)
#define DAMAGED_RHAND              (1 << 6)
#define DAMAGED_LFOOT              (1 << 7)
#define DAMAGED_RFOOT              (1 << 8)
#define DAMAGED_BODY               (1 << 9)

/* 'affect' locations */
#define CHAR_APPLY_NONE              0
#define CHAR_APPLY_STR               1
#define CHAR_APPLY_AGL               2
#define CHAR_APPLY_WIS               3
#define CHAR_APPLY_END               5
#define CHAR_APPLY_THIRST            6
#define CHAR_APPLY_HUNGER            7
#define CHAR_APPLY_MANA             12
#define CHAR_APPLY_HIT              13
#define CHAR_APPLY_MOVE             14
#define CHAR_APPLY_OBSIDIAN         15
#define CHAR_APPLY_DAMAGE           16
#define CHAR_APPLY_AC               17
#define CHAR_APPLY_ARMOR            17
#define CHAR_APPLY_OFFENSE          18
#define CHAR_APPLY_DETOX	    19  /* for the spice detox - hal */
#define CHAR_APPLY_SAVING_PARA      20
#define CHAR_APPLY_SAVING_ROD       21
#define CHAR_APPLY_SAVING_PETRI     22
#define CHAR_APPLY_SAVING_BREATH    23
#define CHAR_APPLY_SAVING_SPELL     24
#define CHAR_APPLY_SKILL_SNEAK      25
#define CHAR_APPLY_SKILL_HIDE       26
#define CHAR_APPLY_SKILL_LISTEN     27
#define CHAR_APPLY_SKILL_CLIMB      28
#define CHAR_APPLY_DEFENSE          29
#define CHAR_APPLY_CFLAGS           30
#define CHAR_APPLY_STUN             31
#define CHAR_APPLY_PSI              32

#define CHAR_APPLY_MAX              32

#define CHAR_APPLY_SKILL_OFFSET     35

/* sex */
#define SEX_NEUTER    0
#define SEX_MALE      1
#define SEX_FEMALE    2

/* positions */
#define POSITION_DEAD       0
#define POSITION_MORTALLYW  1
#define POSITION_STUNNED    2
#define POSITION_SLEEPING   3
#define POSITION_RESTING    4
#define POSITION_SITTING    5
#define POSITION_FIGHTING   6
#define POSITION_STANDING   7

/* levels */
#define MORTAL                 0
#define LEGEND                 1
#define BUILDER                2
#define CREATOR                3
#define QUESTMASTER            3
#define STORYTELLER            3
#define DIRECTOR               4
#define HIGHLORD               4
#define OVERLORD               5

/* groups */
#define GROUP_PUBLIC    (1 << 0)
#define GROUP_ALLANAK   (1 << 1)
#define GROUP_TULUK     (1 << 2)
#define GROUP_DESERT    (1 << 3)
#define GROUP_NDESERT   (1 << 4)
#define GROUP_VILLAGES  (1 << 5)
#define GROUP_HOUSES    (1 << 6)
#define GROUP_TLANDS    (1 << 7)
#define GROUP_SEA       (1 << 8)
#define GROUP_AIR       (1 << 9)
#define GROUP_LANTHIL   (1 << 10)

/* speeds for movement */
#define SPEED_WALKING          0
#define SPEED_RUNNING          1
#define SPEED_SNEAKING         2

/* clan job flags */
#define CJOB_RECRUITER ( 1 << 0 )
#define CJOB_LEADER    ( 1 << 1 )
#define CJOB_WITHDRAW  ( 1 << 2 )

struct clan_data
{
    int clan;
    int flags;
    int rank;
    CLAN_DATA *next;
};

#define MERCY_KILL         (1 << 0)
#define MERCY_FLEE         (1 << 1)

// Nosave flags
#define NOSAVE_ARREST      (1 << 0)
#define NOSAVE_CLIMB       (1 << 1)
#define NOSAVE_MAGICK      (1 << 2)
#define NOSAVE_PSIONICS    (1 << 3)
#define NOSAVE_SKILLS      (1 << 4)
#define NOSAVE_SUBDUE      (1 << 5)
#define NOSAVE_THEFT       (1 << 6)
#define NOSAVE_COMBAT      (1 << 7)

/* brief flags */
#define BRIEF_COMBAT       (1 << 0)
#define BRIEF_ROOM         (1 << 1)
#define BRIEF_OOC          (1 << 2)
#define BRIEF_STAFF_ONLY_NAMES (1 << 3)
#define BRIEF_EQUIP        (1 << 4)
#define BRIEF_SONGS        (1 << 5)
#define BRIEF_NOVICE       (1 << 6)
#define BRIEF_EXITS        (1 << 7)
#define BRIEF_SKILLS       (1 << 8)
#define BRIEF_EMOTE        (1 << 9)
#define BRIEF_PROMPT       (1 << 10)

/* boards */
#define MAX_BOARDS      500


/* character flags */
#define CFL_NOHELP         (1 << 0)
#define CFL_SENTINEL       (1 << 1)
#define CFL_SCAVENGER      (1 << 2)
#define CFL_ISNPC          (1 << 3)
#define CFL_CRIM_RS        (1 << 4)
#define CFL_AGGRESSIVE     (1 << 5)
#define CFL_STAY_ZONE      (1 << 6)
#define CFL_WIMPY          (1 << 7)
#define CFL_BRIEF          (1 << 8)     // this is going away for the BRIEF_ flags
#define CFL_NO_DEATH       (1 << 9)
#define CFL_COMPACT        (1 << 10)
#define CFL_CRIM_CAI_SHYZN (1 << 11)
#define CFL_CMDLOG         (1 << 12)
#define CFL_CRIM_BW        (1 << 13)
#define CFL_FROZEN         (1 << 14)
#define CFL_UNDEAD         (1 << 15)
#define CFL_CRIM_LUIRS     (1 << 16)
#define CFL_INFOBAR        (1 << 17)
#define CFL_NOSAVE         (1 << 18)
#define CFL_CAN_POISON     (1 << 19)
#define CFL_CLANLEADER     (1 << 20)
#define CFL_NOGANG         (1 << 21)
#define CFL_NEW_SKILL      (1 << 22)
#define CFL_CRIM_MAL_KRIAN (1 << 23)
#define CFL_NOWISH         (1 << 24)
#define CFL_SILT_GUY       (1 << 25)
#define CFL_MOUNT          (1 << 26)
#define CFL_CRIM_ALLANAK   (1 << 27)
#define CFL_CRIM_TULUK     (1 << 28)
#define CFL_UNIQUE         (1 << 29)
#define CFL_FLEE           (1 << 30)
#define CFL_DEAD           (1 << 31)

#define QFL_INSERT_HEAD    (1 << 0)
#define QFL_PARSE_ALIAS    (1 << 1)
#define QFL_NO_STOP        (1 << 2)
#define QFL_DEFAULT        (QFL_PARSE_ALIAS)

struct age_stat_data
{
    int str[21];
    int end[21];
    int agl[21];
    int wis[21];
};

struct time_data
{
    time_t starting_time;       /* time at which player starting playing the char */
    time_t logon;               /* current logon */
    int starting_age;           /* age at which player starting playing the char */
    int played;                 /* total amount of time played */
};

/* who npcs remember, and how they feel about them */
struct memory_data
{
    CHAR_DATA *remembers;
    struct memory_data *next;
};


struct extra_cmd
{
    int cmd;
    int level;
    EXTRA_CMD *next;
};

struct queued_cmd {
    char *command;
    QUEUED_CMD *next;
    int queue_flags;
};

/* New account fields for account-level indexing of players -Morg */

/* index of a character for player_info */
struct char_info
{
    char *name;
    int status;
    CHAR_INFO *next;
};

/*
 * Player Information - account level
 */
struct player_info
{
    long id;
    char *name;
    char *email;
    char *password;
    char *comments;
    char *kudos;
    sh_int num_chars;
    sh_int num_chars_alive;
    sh_int num_chars_applying;
    sh_int num_chars_allowed;
    int flags;
    int karma;
    char *karmaLog;
    LOGIN_DATA *last_on[MAX_LOGINS_SAVED];
    LOGIN_DATA *last_failed[MAX_LOGINS_SAVED];
    CHAR_INFO *characters;
    PLAYER_INFO *next;
    char edit_end;
    unsigned long karmaRaces;
    unsigned long karmaGuilds;
    unsigned long tempKarmaRaces;
    unsigned long tempKarmaGuilds;
    char *created;
};


/* Flags for Player Info */
#define PINFO_NONE              0
#define PINFO_NO_NEW_CHARS      (1 << 0)
#define PINFO_BAN               (1 << 1)
#define PINFO_CAN_MULTIPLAY     (1 << 2)
#define PINFO_CONFIRMED_EMAIL   (1 << 3)
#define PINFO_VERBOSE_HELP      (1 << 4)
#define PINFO_BRIEF_MENUS       (1 << 5)
#define PINFO_ANSI_VT100        (1 << 6)
#define PINFO_FREE_EMAIL_OK     (1 << 7)
#define PINFO_WANTS_RP_FEEDBACK (1 << 8)
#define PINFO_NO_SPECIAL_APPS   (1 << 9)

/* Flags for Karma Info */
#define KINFO_GUILD_BURGLAR		(1 << 0)
#define KINFO_GUILD_PICKPOCKET		(1 << 1)
#define KINFO_GUILD_WARRIOR		(1 << 2)
#define KINFO_GUILD_TEMPLAR		(1 << 3)
#define KINFO_GUILD_RANGER		(1 << 4)
#define KINFO_GUILD_MERCHANT		(1 << 5)
#define KINFO_GUILD_ASSASSIN		(1 << 6)
#define KINFO_GUILD_PSIONICIST		(1 << 7)
#define KINFO_GUILD_FIRE_CLERIC		(1 << 8)
#define KINFO_GUILD_WATER_CLERIC	(1 << 9)
#define KINFO_GUILD_STONE_CLERIC	(1 << 10)
#define KINFO_GUILD_WIND_CLERIC		(1 << 11)
#define KINFO_GUILD_SHADOW_CLERIC	(1 << 12)
#define KINFO_GUILD_LIGHTNING_CLERIC	(1 << 13)
#define KINFO_GUILD_VOID_CLERIC		(1 << 14)
#define KINFO_GUILD_SORCERER		(1 << 15)
#define KINFO_GUILD_LIRATHU_TEMPLAR     (1 << 16)
#define KINFO_GUILD_JIHAE_TEMPLAR       (1 << 17)
#define KINFO_GUILD_NOBLE               (1 << 18)

#define KINFO_GUILD_MAGICKER ( KINFO_GUILD_FIRE_CLERIC + \
                               KINFO_GUILD_WATER_CLERIC + \
                               KINFO_GUILD_STONE_CLERIC + \
                               KINFO_GUILD_WIND_CLERIC + \
                               KINFO_GUILD_SHADOW_CLERIC + \
                               KINFO_GUILD_LIGHTNING_CLERIC + \
                               KINFO_GUILD_VOID_CLERIC )

#define KINFO_GUILD_DEFAULT ( KINFO_GUILD_BURGLAR + \
                              KINFO_GUILD_PICKPOCKET + \
                              KINFO_GUILD_WARRIOR + \
                              KINFO_GUILD_RANGER + \
                              KINFO_GUILD_MERCHANT + \
                              KINFO_GUILD_ASSASSIN )

#define KINFO_GUILD_ALL ( KINFO_GUILD_BURGLAR + \
                          KINFO_GUILD_PICKPOCKET + \
                          KINFO_GUILD_WARRIOR + \
                          KINFO_GUILD_TEMPLAR + \
                          KINFO_GUILD_RANGER + \
                          KINFO_GUILD_MERCHANT + \
                          KINFO_GUILD_ASSASSIN	 + \
                          KINFO_GUILD_PSIONICIST + \
                          KINFO_GUILD_FIRE_CLERIC + \
                          KINFO_GUILD_WATER_CLERIC + \
                          KINFO_GUILD_STONE_CLERIC + \
                          KINFO_GUILD_WIND_CLERIC + \
                          KINFO_GUILD_SHADOW_CLERIC + \
                          KINFO_GUILD_LIGHTNING_CLERIC + \
                          KINFO_GUILD_VOID_CLERIC + \
                          KINFO_GUILD_SORCERER + \
                          KINFO_GUILD_LIRATHU_TEMPLAR + \
                          KINFO_GUILD_JIHAE_TEMPLAR + \
                          KINFO_GUILD_NOBLE)

#define KINFO_RACE_HUMAN		(1 << 0)
#define KINFO_RACE_ELF			(1 << 1)
#define KINFO_RACE_DESERT_ELF		(1 << 2)
#define KINFO_RACE_BLACKWING_ELF	(1 << 3)
#define KINFO_RACE_HALF_ELF		(1 << 4)
#define KINFO_RACE_HALF_GIANT		(1 << 5)
#define KINFO_RACE_HALFLING		(1 << 6)
#define KINFO_RACE_MUL			(1 << 7)
#define KINFO_RACE_MANTIS		(1 << 8)
#define KINFO_RACE_GITH			(1 << 9)
#define KINFO_RACE_DWARF		(1 << 10)
#define KINFO_RACE_DRAGON		(1 << 11)
#define KINFO_RACE_AVANGION		(1 << 12)

#define KINFO_RACE_DEFAULT ( KINFO_RACE_HUMAN + \
                             KINFO_RACE_ELF + \
                             KINFO_RACE_HALF_ELF + \
                             KINFO_RACE_DWARF)

#define KINFO_RACE_ALL ( KINFO_RACE_HUMAN + \
                         KINFO_RACE_ELF + \
                         KINFO_RACE_DESERT_ELF + \
                         KINFO_RACE_BLACKWING_ELF + \
                         KINFO_RACE_HALF_ELF + \
                         KINFO_RACE_HALF_GIANT + \
                         KINFO_RACE_HALFLING + \
                         KINFO_RACE_MUL + \
                         KINFO_RACE_MANTIS + \
                         KINFO_RACE_GITH + \
                         KINFO_RACE_DWARF + \
                         KINFO_RACE_DRAGON + \
                         KINFO_RACE_AVANGION )

#define KINFO_RACE_COMMON ( KINFO_RACE_HUMAN + \
                            KINFO_RACE_ELF + \
                            KINFO_RACE_DESERT_ELF + \
                            KINFO_RACE_BLACKWING_ELF + \
                            KINFO_RACE_HALF_ELF + \
                            KINFO_RACE_HALF_GIANT + \
                            KINFO_RACE_MUL + \
                            KINFO_RACE_DWARF )

/*
 * Applying Status
 */
#define APPLICATION_NONE        0
#define APPLICATION_APPLYING    1
#define APPLICATION_REVISING    2
#define APPLICATION_REVIEW      3
#define APPLICATION_ACCEPT      4
#define APPLICATION_DENY        5
#define APPLICATION_DEATH       6
#define APPLICATION_STORED      7

/*
 * for immortal_name_list
 */
struct imm_name
{
    char *name;
    char *account;
    IMM_NAME *next;
};


/* character structures */
struct char_player_data
{
    char *prompt;               /* player-defined prompt */

    char *info[15];             /* 0 is email, 1 is objective, rest are info */
    /* char *background;            background of the character */
    char *extkwds;              /* extra keyword list */
    char *dead;                 /* buffer for name of dead char */

    byte sex;                   /* character's gender */
    GuildType    guild;         /* guild character belongs to */
    SubguildType sub_guild;     /* sub_guild character bleongs to */
    int race;                   /* character's race */
    int apparent_race;          /* character's apparent race */
    byte level;                 /* level of character */
    long level_id;              /* id of level in db */
    byte orig_level;            /* original level, for extra commands -Morg */

    int tribe;                  /* who npcs assist */
    ubyte luck;                 /* luck */
    int hometown;               /* origin/home city */
    int start_city;             /* starting city */

    struct time_data time;      /* player age structure */

    int weight;                 /* character's weight */
    int height;                 /* character's height */

    char *poofin;               /* for appearing in rooms */
    char *poofout;              /* for disappearing from rooms */

    char *deny;                 /* for review process deny reason */
};

struct char_ability_data
{
    sbyte str;                  /* strength */
    sbyte agl;                  /* agility */
    sbyte wis;                  /* wisdom */
    sbyte end;                  /* endurance */

    sh_int armor;               /* armor points */
    sbyte off;                  /* any bonus or penalty to offense         */
    long off_last_gain;         /* time character last gained in offense */
    sbyte def;                  /* any bonus or penalty to defense         */
    long def_last_gain;         /* time character last gained in defense */
    sbyte damroll;              /* any bonus or penalty to damage          */
};

struct char_point_data
{
    sh_int stun;                /* stun points */
    sh_int max_stun;            /* max stun points */
    sh_int mana;                /* magickal energy */
    sh_int mana_bonus;          /* bonus to mana */
    sh_int hit;                 /* health points */
    sh_int max_hit;             /* max health points */
    sh_int move;                /* stamina points */
    sh_int move_bonus;          /* bonus to stamina */



    int obsidian;               /* money carried */
    int in_bank;                /* money in bank */


};

struct char_watching_data
{
    CHAR_DATA *victim;          /* victim the player is watching */
    int dir;                    /* direction the player is watching */
};

struct char_special_data
{
    CHAR_DATA *fighting;        /* opponent                      */
    struct memory_data *hates;  /* who npcs hate                 */
    struct memory_data *fears;  /* who npcs fear                 */
    struct memory_data *likes;  /* who npcs like                 */
    CHAR_DATA *guarding;        /* who char is guarding          */
    CHAR_DATA *subduing;        /* who char is subduing          */
    CHAR_DATA *riding;          /* mount char is riding          */
    CHAR_DATA *contact;         /* psionic contact               */
    OBJ_DATA *harnessed_to;     /* what the npc is pulling       */
    OBJ_DATA *obj_guarding;     /* what the char is guarding     */

    int dir_guarding;           /* which direction char is guarding */
    struct char_watching_data watching; /* watching data */

    byte speed;                 /* movement speed */
    sbyte eco;                  /* ecological balance */
    long affected_by;           /* spells/skills affecting character */

    byte position;              /* character position */
    byte default_pos;           /* default position for npcs */
    long act;                   /* cflags re:actions */
    long flag;                  /* cflags re:status */
    long brief;                 /* brief options */
    long nosave;                /* nosave options */
    long mercy;                 /* mercy options */

    byte carry_items;           /* carried items */
    int timer;                  /* timer for update */
    ROOM_DATA *was_in_room;     /* storage of location for linkdead people */
    int apply_saving_throw[5];  /* saving throw bonuses */
    int conditions[3];          /* hungry, thirsty, etc. */

    byte language;              /* currently spoken language */

    sh_int uid;                 /* user id */
    int group;                  /* group id bitvector */
    byte il;                    /* invisibility level */
    int quiet_level;            /* quiet level */

    byte state;                 /* dummy variale for asml/c programs */

    sh_int gp_mod;              /* random modifier to npc starting money */

    char *gone;                 /* 'gone' message */

    struct specials_t *programs;        /* programs attached to player */

    int regenerating;           /* whether an npc is on the rest heap */

    int act_wait;               /* wait to perform next command */
    int combat_wait;            /* combat wait */
    bool to_process;            /* process flag for delayed commands */
    struct delayed_com_data *delayed_cmd;       /* delayed command */
    CHAR_DATA *alt_fighting;    /* alternate opponent for warriors */

    int accent;                 /* current accent */
};

struct char_skill_data
{
    int learned;               /* % chance for success, 0 = not learned */
    byte rel_lev;               /* relative level difference */
    bool taught_flags;          /* char has been taught the skill, and other things */
    long last_gain;             /* time character last gained in skill */
};



/* magick_type below is one of: */
#define MAGICK_NONE      0
#define MAGICK_OTHER     1
#define MAGICK_OBJECT    2
#define MAGICK_SORCEROR  3
#define MAGICK_ELEMENTAL 4
#define MAGICK_PSIONIC   5


struct affected_type
{
    int type;                   /* type of spell/something else causing this */
    sh_int duration;            /* duration of effects */
    sh_int power;               /* power it was cast at */
    sh_int magick_type;         /* type of magick (other, sorc, elem) */
    long modifier;              /* modifier to attribute */
    int location;               /* location of modified attribte */
    unsigned long bitvector;    /* character affects to set */
    time_t initiation;          /* time affect was initiated */
    time_t expiration;          /* time affect will expire */

    struct affected_type *next;
};

struct wagon_damage_type
{
    int tongue;                 /* where beasts are attached to the harness */
    int wheels;                 /* wagon wheels */
    int hull;                   /* main structure */
};

struct object_follow_type
{
    OBJ_DATA *object_follower;  /* object following object */
    struct object_follow_type *next;
};

struct obj_follow_type
{
    CHAR_DATA *obj_follower;    /* follower of object */
    struct obj_follow_type *next;
};

struct follow_type
{
    CHAR_DATA *follower;        /* follower of char */
    struct follow_type *next;
};

struct guard_type
{
    CHAR_DATA *guard;           /* guard of character */
    struct guard_type *next;    /* next guard of character */
};

struct harness_type
{
    CHAR_DATA *beast;           /* beast of character */
    struct harness_type *next;  /* next guard of character */
};


/*
 * Added this to start making a generic version to replace harnass_type,
 * guard_type and follow_type (no need to have one for each, essentially
 * the same structure. - Morg
 */
struct plyr_list
{
    CHAR_DATA *ch;              /* player */
    PLYR_LIST *next;            /* next player */
};


#define QUIT_NONE           0
#define QUIT_OOC            ( 1 << 0 )  /* Player has quit OOC, may not quit OOC again till cleared */
#define QUIT_OOC_REVOKED    ( 1 << 1 )  /* Player not allowed to use Quit OOC */
#define QUIT_WILDERNESS     ( 1 << 2 )  /* Currently unused, maybe for wildernress quit as a skill */

#define MONITOR_NONE        0
#define MONITOR_COMMS       ( 1 << 0 )
#define MONITOR_UNUSED      ( 1 << 1 )
#define MONITOR_MERCHANT    ( 1 << 2 )
#define MONITOR_POSITION    ( 1 << 3 )
#define MONITOR_ROGUE       ( 1 << 4 )
#define MONITOR_FIGHT       ( 1 << 5 )
#define MONITOR_MAGICK      ( 1 << 6 )
#define MONITOR_MOVEMENT    ( 1 << 7 )
#define MONITOR_PSIONICS    ( 1 << 8 )
#define MONITOR_CRAFTING    ( 1 << 9 )
#define MONITOR_OTHER       ( 1 << 10 )
#define MONITOR_IN_ROOM     ( 1 << 11 )
#define MONITOR_INCLUDE_NPC ( 1 << 12 )
#define MONITOR_IGNORE      ( 1 << 13 )
#define MONITOR_TABLE       ( 1 << 14 )

#define MONITOR_DEFAULT ( MONITOR_COMMS | MONITOR_ROGUE | MONITOR_FIGHT | MONITOR_MAGICK \
                        | MONITOR_PSIONICS | MONITOR_OTHER | MONITOR_CRAFTING)

#define MONITOR_ALL     ( MONITOR_COMMS | MONITOR_MERCHANT | MONITOR_POSITION \
                        | MONITOR_ROGUE | MONITOR_FIGHT | MONITOR_MAGICK \
                        | MONITOR_MOVEMENT | MONITOR_CRAFTING | MONITOR_IN_ROOM \
                        | MONITOR_INCLUDE_NPC | MONITOR_PSIONICS | MONITOR_OTHER )

#define MONITOR_COMPLETE	(MONITOR_ALL | MONITOR_IGNORE)


/*
 * monitor_list
 * like a plyr_list, but with flags of what to monitor
 */
struct monitor_list
{
    CHAR_DATA *ch;              /* player */
    ROOM_DATA *room;
    int clan;
    int guild;
    int show;                   /* types of output to show */
    MONITOR_LIST *next;         /* next player */
};

/* login information */
struct login_data
{
    char *site;
    char *in;
    char *out;
    int descriptor;
};


/* aliases */
struct alias_data
{
    char alias[16];
    char text[61];
};

/* extra data for infobar */
struct char_old_data
{
    int hp;
    int max_hp;
    int stun;
    int max_stun;
    int mana;
    int max_mana;
    int move;
    int max_move;
};

struct char_data
{
    int nr;                     /* npc number      */
    ROOM_DATA *in_room;         /* room char is in */
    int saved_room;
    long id;                    /* database index */

    struct char_player_data player;     /* data for players */
    char *name;                 /* keyword list for characters */
    char *short_descr;          /* short string describing character */
    char *long_descr;           /* for characters in rooms */
    char *description;          /* extra descriptions */
    char *temp_description;     /* temporary descriptions */
    char *background;           /* character's background */
    char *account;              /* character's account */

    struct extra_descr_data *ex_description;    /* extra descriptions */

    long int action_hour_seed;  /* this changes on the character every hour */

    struct char_ability_data abilities; /* abilities        */
    struct char_ability_data tmpabilities;      /* backup abilities */
    struct char_point_data points;      /* game mechanics   */
    struct char_special_data specials;  /* other data       */

    struct char_skill_data *skills[MAX_SKILLS]; /* skill data */

    struct affected_type *affected;     /* effects acting on char */
    OBJ_DATA *equipment[MAX_WEAR];      /* equipment array        */

    struct alias_data alias[MAX_ALIAS]; /* aliases                 */
    struct char_old_data old;   /* backup data for infobar */

    OBJ_DATA *lifting;          /* What character is lifting */
    OBJ_DATA *carrying;         /* list of carried objects */
    DESCRIPTOR_DATA *desc;      /* socket descriptor       */

    PLYR_LIST *riders;          /* list of characters riding    */
    struct guard_type *guards;  /* list of characters guarding  */
    struct follow_type *followers;      /* list of characters following */
    CHAR_DATA *master;          /* who character is following   */
    OBJ_DATA *obj_master;       /* what character is following  */

    CLAN_DATA *clan;            /* multiple clans, to replace player->tribe */

/* More Furniture stuff - Morg */
    OBJ_DATA *on_obj;           /* object the char is on (sit/rest/sleep) */

    CHAR_DATA *prev;
    CHAR_DATA *next;

    CHAR_DATA *prev_in_room;
    CHAR_DATA *next_in_room;

    CHAR_DATA *next_fighting;

    CHAR_DATA *prev_num;
    CHAR_DATA *next_num;

    EXTRA_CMD *granted_commands;
    EXTRA_CMD *revoked_commands;
    sh_int pagelength;

#ifdef SMELLS
    SMELL_DATA *smells;
    SMELL_DATA *accustomed_smells;
#endif

    sh_int application_status;

    sh_int logged_in;
    sh_int quit_flags;
    QUEUED_CMD *queued_commands;
    char *last_prompt;

    void *jsobj;
};

/* default npc data */

struct npc_default_data
{
    char *name;
    char *short_descr;
    char *long_descr;
    char *description;
    char *background;

    struct extra_descr_data *exdesc;

    int obsidian;
    byte gp_mod;
    sh_int armor;
    sbyte off;
    sbyte def;
    sbyte dam;
    long affected_by;
    byte pos;
    byte def_pos;
    long act;
    long flag;
    sh_int language;
    sh_int max_hit;
    sh_int max_stun;
    byte sex;
    byte guild;
    byte sub_guild;
    int race;
    int age;
    int apparent_race;
    byte level;
    int tribe;

    CLAN_DATA *clan;            /* multiple clans, to replace player->tribe */

    int luck;

    struct specials_t *programs;

    struct char_skill_data skills[MAX_SKILLS];

#ifdef SMELLS
    SMELL_DATA *smells;
    SMELL_DATA *accustomed_smells;
#endif
};

struct npc_brain_data
{
    int state;                  /* Current state of the NPC   */

    ROOM_DATA *sleep_room;      /* Room the NPC sleeps in     */
    ROOM_DATA *work_room;       /* Room the NPC works in      */
    ROOM_DATA *rest_room;       /* Room the NPC rests in      */
    ROOM_DATA *heal_room;       /* Room the NPC heals in      */

    char sleep_ldesc[80];       /* LDESC of NPC in sleep room */
    char work_ldesc[80];        /* LDESC of NPC in work room  */
    char rest_ldesc[80];        /* LDESC Of NPC in rest room  */
    char heal_ldesc[80];        /* LDESC Of NPC in heal room  */

    int sleep_hour;             /* Hour to change to sleeping */
    int work_hour;              /* Hour to change to working  */
    int rest_hour;              /* Hour to change to resting  */

    int heal_perc;              /* % max hp to start healing  */

    int active;                 /* For turning node on/off    */

    CHAR_DATA *npc;             /* Pointer to the NPC         */
    struct npc_brain_data *next;        /* Next NPC in the list       */
};

struct listen_data
{
    char pad1[64];
    CHAR_DATA *listener;
    char pad2[64];
    byte area;
    char pad3[64];
    struct listen_data *next;
    char pad4[64];
};

/* WEATHER DEFINITONS */

#define SUN_DARK	0
#define SUN_LOW         1
#define SUN_HIGH        2
#define SUN_ZENITH      3

struct weather_data
{
    int sunlight;               /* how hot is the sun */
    int moon[3];                /* state of each moon */
    byte wind_direction;        /* which way is the wind blowing */
    int wind_velocity;          /* how hard is the wind blowing */
    int pressure;               /* buildup of fronts to change wind */
    int tides;                  /* level of the tides */
};

struct weather_node
{
    int zone;                   /* which zone the node is for */

    int life;                   /* plantlife in zone */
    int max_life;               /* maximum plantlife in zone */
    int temp;                   /* temperature in farenheit */
    int min_temp;               /* minimum temperature */
    int max_temp;               /* maximum... */
    double condition;           /* clear or stormy, range 0.0 to 10.0 */
    double min_condition;       /* minimum storminess */
    double max_condition;       /* maximum... */
    double median_condition;    /* median condition */

    int neighbors[MAX_DIRECTIONS];
    struct weather_node **near_node[MAX_DIRECTIONS];

    struct weather_node *next;
};



/* TIME DEFINITIONS */

/* periods of the day (hours) */
#define NIGHT_EARLY  0
#define DAWN         1
#define RISING_SUN   2
#define MORNING      3
#define HIGH_SUN     4
#define AFTERNOON    5
#define SETTING_SUN  6
#define DUSK         7
#define NIGHT_LATE   8

/* phases of the year */
#define DESCENDING_SUN  0
#define LOW_SUN         1
#define ASCENDING_SUN   2

/* the moons */
#define JIHAE       0
#define LIRATHU     1
#define ECHRI       2

#define JIHAE_RISE      2
#define JIHAE_SET       5
#define LIRATHU_RISE    3
#define LIRATHU_SET     8
#define ECHRI_RISE      7
#define ECHRI_SET       23


#define MAX_OBJ_SAVE 50         /* maximum number of objects in player obj file */

/* object data for player object file */
struct obj_file_elem
{
    int nr;

    int eq_pos;

    int value[4];
    int extra_flags;
    int weight;
    int timer;
    long bitvector;
    long state_flags;
    long color_flags;
    long adverbial_flags;
    struct obj_affected_type affected[MAX_OBJ_AFFECT];
};

/* object file structure */
struct obj_file_u
{
    char owner[20];             /* name of player */
    int obsidian_left;          /* number of coins left at owner */
    int total_cost;             /* the cost for all items, per day */
    long last_update;           /* time in seconds, when last updated */

    struct obj_file_elem objects[MAX_OBJ_SAVE];
};


/* SOCKET DEFINITIONS */

/* what is the player doing */
#define CON_PLYNG         0
#define CON_NME	          1
#define CON_NMECNF        2
#define CON_PWDNRM        3
#define CON_QSEX          6
#define CON_WAIT_SLCT     7
#define CON_SLCT          8
#define CON_EXDSCR        9
#define CON_QGUILD        10
#define CON_LDEAD         11
#define CON_PWDNEW        12
#define CON_PWDNCNF       13
#define CON_DNET          14
#define CON_QRACE         15
#define CON_ED_MENU       16
#define CON_QFILE         17
#define CON_EDITING       18
#define CON_VT100         19
#define CON_HOME          20
#define CON_DOC           21
#define CON_WAIT_DOC      22
#define CON_WAIT_MOTD     23
#define CON_AUTO_DESC_END 25
#define CON_AUTO_SDESC    26
#define CON_AUTO_LDESC    27
#define CON_AUTO_EXTKWD   28
#define CON_SDESC         29
#define CON_LDESC         30
#define CON_EXTKWD        31
#define CON_REQUEST       32
#define CON_NEWNAME       33
#define CON_APP_END       35

#ifdef PERMANENT_DEATH
#define CON_DEAD_NAME     36
#define CON_DEAD_PASS     37
#endif

#define CON_ANSI	  38
#define CON_HEIGHT        39
#define CON_WEIGHT        40
/* #define CON_VIEW_STATS    41  unused (no need for a CON_* to do it -Morg */

#define CON_MAIL_MENU     50
#define CON_MAIL_LIST     51
#define CON_MAIL_READ_D   52
#define CON_MAIL_READ_S   53
#define CON_MAIL_DELETE   54
#define CON_MAIL_SEND     55
#define CON_ENTER_MAIL    56
#define CON_PWDNRM        3
#define CON_PWDGET        4
#define CON_PWDCNF        5

/* this is for character creation */
#define CON_CC               57
#define CON_ENTER_CC         58
#define CON_CC_VIEW_STATS    59
#define CON_CC_SUBMIT        60
#define CON_CC_EXIT          61
#define CON_CC_RACE          62
#define CON_CC_HEIGHT        63
#define CON_CC_WEIGHT        64
#define CON_CC_SDESC         65
#define CON_CC_LDESC         66
#define CON_CC_EXTKWD        67
#define CON_EMAIL            68
#define CON_EMAIL_VERIFY     69
#define CON_OBJECTIVE        70
#define CON_AGE              71
#define CON_BACKGROUND       72
#define CON_AUTO_BACKGROUND_END 73
#define CON_MUL_SLAVE_CHOICE 74
#define CON_SORC_CHOICE      75

#define CON_GET_ACCOUNT_NAME 76
#define CON_CONFIRM_NEW_ACCOUNT 77
#define CON_GET_ACCOUNT_PASSWORD 78
#define CON_GET_NEW_ACCOUNT_PASSWORD 79
#define CON_CONFIRM_NEW_ACCOUNT_PASSWORD 80
#define CON_WAIT_DISCONNECT  81
#define CON_CONFIRM_DELETE   82
#define CON_REVISE_CHAR_MENU 83
#define CON_OPTIONS_MENU     84
#define CON_GET_PAGE_LEN     85
#define CON_GET_EDIT_END     86
#define CON_CONNECT_SKIP_PASSWORD 87

#define CON_WHERE_SURVEY_Q   88
#define CON_SECOND_SKILL_PACK_CHOICE      89
#define CON_GET_NEW_ACCOUNT_NAME 90
#define CON_NOBLE_CHOICE     91
#define CON_PSI_CHOICE       92
#define CON_ORDER_STATS      93
#define CON_ORDER_STATS_CONFIRM      94
#define CON_WAIT_CONFIRM_EXIT 95
#define CON_HALF_ELF_APP_CHOICE 96

#define CON_LOCATION		97
#define CON_TULUK_TATTOO	98
#define CON_SORC_SUB_CHOICE     99

/* remember to change connected_types in constants.c for new CONs */

struct snoop_data
{
    CHAR_DATA *snooping;        /* who character is snooping */
    CHAR_DATA *snoop_by;        /* who's snooping the character */
};

struct txt_block
{
    char *text;
    struct txt_block *next;
};

struct txt_q
{
    struct txt_block *head;
    struct txt_block *tail;
};

struct descriptor_data
{
    int descriptor;             /* file descriptor for socket */
    int connected;              /* mode of 'connectedness' */
    int wait;                   /* wait for how many loops */
    int idle;                   /* # loops with no input */

    int exc_set;                /* results of custom select */
    int input_set;
    int output_set;

    int pagelength;             /* length to page out to */
    int linesize;               /* line (cols) size */

    char *showstr_head;         /* for paging through texts */
    const char *showstr_point;  /* for paging through texts */
    char **str;                 /* for the modify-str system */
    char *tmp_str;              /* for editing files */
    char *editing_header;       /* header for file if editing as string */
    char *editing_filename;     /* name of the file being edited (not always set) */
    char *editing_display;      /* String to display when done, and for log */

    int max_str;                /* for the modify-str system */
    int prompt_mode;            /* control of prompt-printing */

    char buf[MAX_STRING_LENGTH];        /* buffer for raw input */
    char last_input[MAX_INPUT_LENGTH];  /* the last input */

    struct txt_q output;        /* q of strings to send */
    struct txt_q ib_input;      /* q of unprocessed input */
    struct txt_q oob_input;     /* q of unprocessed out-of-band input */

    char host[50];              /* IP of host */
    char hostname[100];         /* hostname by DNS name */
    byte term;                  /* terminal type */

    FILE *editing;              /* file player is editing */
    char mail[MAX_STRING_LENGTH];       /* mail buffer */
    char path[256];             /* path in directory system */

    struct snoop_data snoop;    /* to snoop people */
    MONITOR_LIST *monitoring;   /* list of ch they are monitoring */
    sh_int monitoring_all;      /* Is character doing monitor all? */
    int monitor_all_show;       /* What are they monitoring all on */
    sh_int monitoring_review;   /* Is character doing monitor review? */
    int monitor_review_show;    /* What are they monitoring review on */

    PLAYER_INFO *player_info;   /* account information */
    int pass_wrong;             /* password incorrect attempts */
    sh_int brief_menus;         /* brief menus? */

    CHAR_DATA *character;       /* linked to char */
    CHAR_DATA *original;        /* original char */

    DESCRIPTOR_DATA *next;      /* link to next descriptor */
};

/* MESSAGE DEFINITIONS */

struct msg_type
{
    char *attacker_msg;         /* message to attacker */
    char *victim_msg;           /* message to victim */
    char *room_msg;             /* message to room */
};

struct message_type
{
    struct msg_type die_msg;    /* messages when death */
    struct msg_type miss_msg;   /* messages when miss */
    struct msg_type hit_msg;    /* messages when hit */
    struct msg_type god_msg;    /* messages when hit on god */
    struct message_type *next;
};

struct message_list
{
    int a_type;                 /* attack type */
    int number_of_attacks;      /* how many different messages */
    struct message_type *msg;   /* list of messages */
};

/* SKILL BONUS DEFINITIONS */

struct dex_skill_type
{
    sh_int p_pocket;
    sh_int p_locks;
    sh_int traps;
    sh_int sneak;
    sh_int hide;
};

struct agl_app_type
{
    sh_int reaction;
    sh_int stealth_manip;
    sh_int carry_num;
    sh_int missile;
    sh_int c_delay;
};

struct str_app_type
{
    sh_int todam;
    sh_int carry_w;
    sh_int bend_break;
};

struct wis_app_type
{
    sh_int learn;
    sh_int percep;
    sh_int wis_saves;
};

struct end_app_type
{
    sh_int end_saves;
    sh_int shock;
    sh_int move_bonus;
};

/* OTHER DEFINITIONS */

struct ban_t
{
    char *name;
    struct ban_t *next;
};

struct flag_data
{
    char *name;
    long bit;
    int priv;
};

struct attribute_data
{
    char *name;
    int val;
    int priv;
};

struct char_fall_data
{
    CHAR_DATA *falling;
    int num_rooms;
    struct char_fall_data *next;
};

struct obj_fall_data
{
    OBJ_DATA *falling;
    struct obj_fall_data *next;
};

struct group_data
{
    char *name;
    int bitv;
};


#define CRAFT_GROUP_SIZE 5
struct create_item_type
{
    /* All of the object(s) needed to create the item(s), and names */
    /* All of the object(s) created by the item(s), and a name to call them  */
    /* All of the object(s) created by the item(s), and a name to call them  */
    /* Skill and minimum percentage needed to create     */
    /* name - what they type in like 'craft <item> <item> <name>'  */
    /* desc - what they see when like 'craft <item> <item>'        */
    /*              and it gives them a list of things to create   */
    /*                                                             */
    /* Difficulty modifier, roll 100 vs skill,           */
    /*  -50 = Modify skill by -50 (harder to make)       */
    /*  0 = Strait Chance.                               */
    /*  50 = Modify skill by +50 (Easier to make)        */
    /*                                                   */
    /* Delay in seconds                                  */
    /* Delay minimum, if has 100% in crafting skill      */
    /* Delay maximum, if has 0% in crafting skill        */
    int from[CRAFT_GROUP_SIZE];
    int into_success[CRAFT_GROUP_SIZE];
    int into_fail[CRAFT_GROUP_SIZE];
    char *name;
    char *desc;
    int skill;
    int percentage_min;
    int percentage_mod;
    int delay_min;
    int delay_max;

    char *to_char_succ;
    char *to_room_succ;

    char *to_char_fail;
    char *to_room_fail;

    int only_tribe;
 
    int require_fire;

    int tool_type;
    int tool_quality;
};



typedef enum {
    CMD_NORTH = 1,
    CMD_EAST = 2,
    CMD_SOUTH = 3,
    CMD_WEST = 4,
    CMD_UP = 5,
    CMD_DOWN = 6,
    CMD_ENTER = 7,
    CMD_EXITS = 8,
    CMD_KEYWORD = 9,
    CMD_GET = 10,
    CMD_DRINK = 11,
    CMD_EAT = 12,
    CMD_WEAR = 13,
    CMD_WIELD = 14,
    CMD_LOOK = 15,
    CMD_SCORE = 16,
    CMD_SAY = 17,
    CMD_SHOUT = 18,
    CMD_REST = 19,
    CMD_INVENTORY = 20,
    CMD_QUI = 21,
    CMD_SNORT = 22,
    CMD_SMILE = 23,
    CMD_DANCE = 24,
    CMD_KILL = 25,
    CMD_CACKLE = 26,
    CMD_LAUGH = 27,
    CMD_GIGGLE = 28,
    CMD_SHAKE = 29,
    CMD_SHUFFLE = 30,
    CMD_GROWL = 31,
    CMD_SCREAM = 32,
    CMD_INSULT = 33,
    CMD_COMFORT = 34,
    CMD_NOD = 35,
    CMD_SIGH = 36,
    CMD_SLEE = 37,
    CMD_HELP = 38,
    CMD_WHO = 39,
    CMD_EMOTE = 40,
    CMD_ECHO = 41,
    CMD_STAND = 42,
    CMD_SIT = 43,
    CMD_RS = 44,
    CMD_SLEEP = 45,
    CMD_WAKE = 46,
    CMD_FORCE = 47,
    CMD_TRANSFER = 48,
    CMD_HUG = 49,
    CMD_SERVER = 50,
    CMD_CUDDLE = 51,
    CMD_STOW = 52,
    CMD_CRY = 53,
    CMD_NEWS = 54,
    CMD_EQUIPMENT = 55,
    CMD_OLDBUY = 56,
    CMD_SELL = 57,
    CMD_VALUE = 58,
    CMD_LIST = 59,
    CMD_DROP = 60,
    CMD_GOTO = 61,
    CMD_WEATHER = 62,
    CMD_READ = 63,
    CMD_POUR = 64,
    CMD_GRAB = 65,
    CMD_REMOVE = 66,
    CMD_PUT = 67,
    CMD_SHUTDOW = 68,
    CMD_SAVE = 69,
    CMD_HIT = 70,
    CMD_OCOPY = 71,
    CMD_GIVE = 72,
    CMD_QUIT = 73,
    CMD_STAT = 74,
    CMD_RSTAT = 75,
    CMD_TIME = 76,
    CMD_PURGE = 77,
    CMD_SHUTDOWN = 78,
    CMD_IDEA = 79,
    CMD_TYPO = 80,
    CMD_BUY = 81,
    CMD_WHISPER = 82,
    CMD_CAST = 83,
    CMD_AT = 84,
    CMD_ASK = 85,
    CMD_ORDER = 86,
    CMD_SIP = 87,
    CMD_TASTE = 88,
    CMD_SNOOP = 89,
    CMD_FOLLOW = 90,
    CMD_RENT = 91,
    CMD_OFFER = 92,
    CMD_POKE = 93,
    CMD_ADVANCE = 94,
    CMD_ACCUSE = 95,
    CMD_GRIN = 96,
    CMD_BOW = 97,
    CMD_OPEN = 98,
    CMD_CLOSE = 99,
    CMD_LOCK = 100,
    CMD_UNLOCK = 101,
    CMD_LEAVE = 102,
    CMD_APPLAUD = 103,
    CMD_BLUSH = 104,
    CMD_REJUVENATE = 105,
    CMD_CHUCKLE = 106,
    CMD_CLAP = 107,
    CMD_COUGH = 108,
    CMD_CURTSEY = 109,
    CMD_CEASE = 110,
    CMD_FLIP = 111,
    CMD_STOP = 112,
    CMD_FROWN = 113,
    CMD_GASP = 114,
    CMD_GLARE = 115,
    CMD_GROAN = 116,
    CMD_SLIP = 117,
    CMD_HICCUP = 118,
    CMD_LICK = 119,
    CMD_GESTURE = 120,
    CMD_MOAN = 121,
    CMD_READY = 122,
    CMD_POUT = 123,
    CMD_STUDY = 124,
    CMD_RUFFLE = 125,
    CMD_SHIVER = 126,
    CMD_SHRUG = 127,
    CMD_SING = 128,
    CMD_SLAP = 129,
    CMD_SMIRK = 130,
    CMD_SNAP = 131,
    CMD_SNEEZE = 132,
    CMD_SNICKER = 133,
    CMD_SNIFF = 134,
    CMD_SNORE = 135,
    CMD_SPIT = 136,
    CMD_SQUEEZE = 137,
    CMD_STARE = 138,
    CMD_BIOGRAPHY = 139, // not implemented
    CMD_THANK = 140,
    CMD_TWIDDLE = 141,
    CMD_WAVE = 142,
    CMD_WHISTLE = 143,
    CMD_WIGGLE = 144,
    CMD_WINK = 145,
    CMD_YAWN = 146,
    CMD_NOSAVE = 147,
    CMD_WRITE = 148,
    CMD_HOLD = 149,
    CMD_FLEE = 150,
    CMD_SNEAK = 151,
    CMD_HIDE = 152,
    CMD_BACKSTAB = 153,
    CMD_PICK = 154,
    CMD_STEAL = 155,
    CMD_BASH = 156,
    CMD_RESCUE = 157,
    CMD_KICK = 158,
    CMD_COERCE = 159,
    CMD_UNLATCH = 160,
    CMD_LATCH = 161,
    CMD_TICKLE = 162,
    CMD_MESMERIZE = 163,
    CMD_EXAMINE = 164,
    CMD_TAKE = 165,
    CMD_DONATE = 166,
    CMD_SAY_SHORT = 167,
    CMD_GUARDING = 168,
    CMD_CURSE = 169,
    CMD_USE = 170,
    CMD_WHERE = 171,
    CMD_REROLL = 172,
    CMD_PRAY = 173,
    CMD_EMOTE_SHORT = 174,
    CMD_BEG = 175,
    CMD_BLEED = 176,
    CMD_CRINGE = 177,
    CMD_DAYDREAM = 178,
    CMD_FUME = 179,
    CMD_GROVEL = 180,
    CMD_VANISH = 181,
    CMD_NUDGE = 182,
    CMD_PEER = 183,
    CMD_POINT = 184,
    CMD_PONDER = 185,
    CMD_PUNCH = 186,
    CMD_SNARL = 187,
    CMD_SPANK = 188,
    CMD_STEAM = 189,
    CMD_TACKLE = 190,
    CMD_TAUNT = 191,
    CMD_THINK = 192,
    CMD_WHINE = 193,
    CMD_WORSHIP = 194,
    CMD_FEEL = 195,
    CMD_BRIEF = 196,
    CMD_WIZLIST = 197,
    CMD_CONSIDER = 198,
    CMD_CUNSET = 199,
    CMD_RESTORE = 200,
    CMD_RETURN = 201,
    CMD_SWITCH = 202,
    CMD_QUAFF = 203,
    CMD_RECITE = 204,
    CMD_USERS = 205,
    CMD_POSE = 206,
    CMD_AWARD = 207,
    CMD_WIZHELP = 208,
    CMD_CREDITS = 209,
    CMD_COMPACT = 210,
    CMD_SKILLS = 211,
    CMD_JUMP = 212,
    CMD_MESSAGE = 213,
    CMD_RCOPY = 214,
    CMD_DISARM = 215,
    CMD_KNOCK = 216,
    CMD_LISTEN = 217,
    CMD_TRAP = 218,
    CMD_TENEBRAE = 219,
    CMD_HUNT = 220,
    CMD_RSAVE = 221,
    CMD_KNEEL = 222,
    CMD_BITE = 223,
    CMD_MAKE = 224,
    CMD_SMOKE = 225,
    CMD_PEEK = 226,
    CMD_RZSAVE = 227,
    CMD_RZCLEAR = 228,
    CMD_MCOPY = 229,
/*    CMD_NONEWS = 230, */
    CMD_RCREATE = 231,
    CMD_DAMAGE = 232,
    CMD_IMMCOM = 233,
    CMD_IMMCOM_SHORT = 234,
    CMD_ZASSIGN = 235,
    CMD_SHOW = 236,
    CMD_OSAVE = 237,
    CMD_OCREATE = 238,
    CMD_TITLE = 239,
    CMD_PILOT = 240,
    CMD_OSET = 241,
    CMD_NUKE = 242,
    CMD_SPLIT = 243,
    CMD_ZSAVE = 244,
    CMD_BACKSAVE = 245,
    CMD_MSAVE = 246,
    CMD_CSET = 247,
    CMD_MCREATE = 248,
    CMD_RZRESET = 249,
    CMD_ZCLEAR = 250,
    CMD_ZRESET = 251,
    CMD_MDUP = 252,
    CMD_LOG = 253,
    CMD_BEHEAD = 254,
    CMD_PLIST = 255,
    CMD_CRIMINAL = 256,
    CMD_FREEZE = 257,
    CMD_BSET = 258,
    CMD_GONE = 259,
    CMD_DRAW = 260,
    CMD_ZSET = 261,
    CMD_DELETE = 262,
    CMD_RACES = 263,
    CMD_HAYYEH = 264,
    CMD_APPLIST = 265,
    CMD_PULL = 266,
    CMD_REVIEW = 267,
    CMD_BAN = 268,
    CMD_UNBAN = 269,
    CMD_BSAVE = 270,
    CMD_CRAFT = 271,
    CMD_PROMPT = 272,
    CMD_MOTD = 273,
    CMD_TAN = 274,
    CMD_INVIS = 275,
    CMD_ECHOALL = 276,
    CMD_PATH = 277,
    CMD_EP = 278,
    CMD_ES = 279,
    CMD_REPLY = 280,
    CMD_QUIET = 281,
    CMD__WSAVE = 282,
    CMD_GODCOM = 283,
    CMD_GODCOM_SHORT = 284,
    CMD__WCLEAR = 285,
    CMD__WRESET = 286,
    CMD__WZSAVE = 287,
    CMD__WUPDATE = 288,
    CMD_BANDAGE = 289,
    CMD_TEACH = 290,
    CMD_DIG = 291,
    CMD_ALIAS = 292,
    CMD_SYSTEM = 293,
    CMD_NONET = 294,
/*    CMD_DNSTAT = 295, */
    CMD_PLMAX = 296,
    CMD_IDLE = 297,
    CMD_IMMCMDS = 298,
    CMD_FILL = 299,
    CMD_LS = 300,
    CMD_PAGE = 301,
    CMD_EDIT = 302,
    CMD_RESET = 303,
    CMD_BEEP = 304,
    CMD_RECRUIT = 305,
    CMD_REBEL = 306,
    CMD_DUMP = 307,
    CMD_CLANLEADER = 308,
    CMD_THROW = 309,
    CMD_MAIL = 310,
    CMD_SUBDUE = 311,
    CMD_PROMOTE = 312,
    CMD_DEMOTE = 313,
    CMD_WISH = 314,
    CMD_VIS = 315,
    CMD_JUNK = 316,
    CMD_DEAL = 317,
    CMD_OUPDATE = 318,
    CMD_MUPDATE = 319,
    CMD_CWHO = 320,
    CMD_NOTITLE = 321,
    CMD_HEAR = 322,
    CMD_WALK = 323,
    CMD_RUN = 324,
    CMD_BALANCE = 325,
    CMD_DEPOSIT = 326,
    CMD_WITHDRAW = 327,
    CMD_APPROVE = 328,
    CMD_REJECT = 329,
    CMD_RUPDATE = 330,
    CMD_REACH = 331,
    CMD_BREAK = 332,
    CMD_CONTACT = 333,
    CMD_LOCATE = 334,
    CMD__UNAPPROVE = 335,
    CMD_NOWISH = 336,
    CMD_SYMBOL = 337,
    CMD_PUSH = 338,
    CMD_MEMORY = 339,
    CMD_MOUNT = 340,
    CMD_DISMOUNT = 341,
    CMD_ZOPEN = 342,
    CMD_ZCLOSE = 343,
    CMD_RDELETE = 344,
    CMD_OSTAT = 345,
    CMD_BARTER = 346,
    CMD_SETTIME = 347,
    CMD_SKILLRESET = 348,
    CMD_GRP = 349,
    CMD_CSTAT = 350,
    CMD_CALL = 351,
    CMD_RELIEVE = 352,
    CMD_INCRIMINATE = 353,
    CMD_PARDON = 354,
    CMD_TALK = 355,
    CMD_OOC = 356,
    CMD_APPLY = 357,
    CMD_SKIN = 358,
    CMD_GATHER = 359,
    CMD_RELEASE = 360,
    CMD_POISONING = 361,
    CMD_SHEATHE = 362,
    CMD_ASSESS = 363,
    CMD_PACK = 364,
    CMD_UNPACK = 365,
    CMD_ZORCH = 366,
    CMD_CHANGE = 367,
    CMD_SHOOT = 368,
    CMD_LOAD = 369,
    CMD_UNLOAD = 370,
    CMD_DWHO = 371,
    CMD_SHIELD = 372,
    CMD_CATHEXIS = 373,
    CMD_PSI = 374,
    CMD_ASSIST = 375,
    CMD_POOFIN = 376,
    CMD_POOFOUT = 377,
    CMD_CD = 378,
    CMD_RSET = 379,
    CMD_RUNSET = 380,
    CMD_OUNSET = 381,
    CMD_ODELETE = 382,
    CMD_CDELETE = 383,
    CMD_BET = 384,
    CMD_PLAY = 385,
    CMD_RM = 386,
    CMD_DISENGAGE = 387,
    CMD_INFOBAR = 388,
    CMD_SCAN = 389,
    CMD_SEARCH = 390,
    CMD_TAILOR = 391,
    CMD_FLOAD = 392,
    CMD_PINFO = 393,
    CMD_RP = 394,
    CMD_SHADOW = 395,
    CMD_HITCH = 396,
    CMD_UNHITCH = 397,
    CMD_PULSE = 398,
    CMD_TAX = 399,
    CMD_PEMOTE = 400,
    CMD_WSET = 401,
    CMD_EXECUTE = 402,
    CMD_ETWO = 403,
    CMD_PLANT = 404,
    CMD_TRACE = 405,
    CMD_BARRIER = 406,
    CMD_DISPERSE = 407,
    CMD_BUG = 408,
    CMD_HARNESS = 409,
    CMD_TOUCH = 411,
    CMD_UNHARNESS = 410,
    CMD_TRADE = 412,
    CMD_EMPATHY = 413,
    CMD_SHADOWWALK = 414,
    CMD_ALLSPEAK = 415,
    CMD_COMMUNE = 416,
    CMD_EXPEL = 417,
    CMD_EMAIL = 418,
    CMD_TELL = 419,
    CMD_CLANSET = 420,
    CMD_CLANSAVE = 421,
    CMD_AREAS = 422,
    CMD_TRAN = 423,
    CMD_LAND = 424,
    CMD_FLY = 425,
    CMD_LIGHT = 426,
    CMD_EXTINGUISH = 427,
    CMD_SHAVE = 428,
    CMD_HEAL = 429,
    CMD_CAT = 430,
    CMD_PROBE = 431,
    CMD_MINDWIPE = 432,
    CMD_COMPEL = 433,
    CMD_RAISE = 434,
    CMD_LOWER = 435,
    CMD_CONTROL = 436,
    CMD_CONCEAL = 437,
    CMD_DOME = 438,
    CMD_CLAIRAUDIENCE = 439,
    CMD_DIE = 440,
    CMD_INJECT = 441,
    CMD_BREW = 442,
    CMD_COUNT = 443,
    CMD_MASQUERADE = 444,
    CMD_ILLUSION = 445,
    CMD_COMMENT = 446,
    CMD_MONITOR = 447,
    CMD_CGIVE = 448,
    CMD_CREVOKE = 449,
    CMD_PAGELENGTH = 450,
    CMD_ROLL = 451,
    CMD_DOAS = 452,
    CMD_VIEW = 453,
    CMD_AINFO = 454,
    CMD_ASET = 455,
    CMD_PFIND = 456,
    CMD_AFIND = 457,
    CMD_RESURRECT = 458,
    CMD_SAP = 459,
    CMD_STORE = 460,
    CMD_UNSTORE = 461,
    CMD_TO_ROOM = 462,
    CMD_FROM_ROOM = 463,
    CMD_SCRIBBLE = 464,
    CMD_LAST = 465,
    CMD_WIZLOCK = 466,
    CMD_TRIBES = 467,
    CMD_FORAGE = 468,
    CMD_CITYSTAT = 469,
    CMD_CITYSET = 470,
    CMD_REPAIR = 471,
    CMD_NESSKICK = 472,
    CMD_CHARDELET = 473,
    CMD_CHARDELETE = 474,
    CMD_SENSE_PRESENCE = 475,
    CMD_PALM = 476,
    CMD_SUGGEST = 477,
    CMD_BABBLE = 478,
    CMD_DISORIENT = 479,
    CMD_ANALYZE = 480,
    CMD_CLAIRVOYANCE = 481,
    CMD_IMITATE = 482,
    CMD_PROJECT = 483,
    CMD_CLEAN = 484,
    CMD_MERCY = 485,
    CMD_WATCH = 486,
    CMD_OINDEX = 487,
    CMD_MAGICKSENSE = 488,
    CMD_MINDBLAST = 489,
    CMD_WHEN_LOADED = 490,
    CMD_ATTEMPT_GET_QUIET = 491,
    CMD_ATTEMPT_DROP = 492,
    CMD_ATTEMPT_DROP_QUIET = 493,
    CMD_SYSTAT = 494,
    CMD_ON_DAMAGE = 495,
    CMD_CHARGE = 496,
    CMD_RTWO = 497,
    CMD_ON_EQUIP = 498,
    CMD_ON_REMOVE = 499,
    CMD_SEND = 500,
    CMD_XPATH = 501,
    CMD_SALVAGE = 502,
    CMD_SILENT_CAST = 503,
    CMD_KISS = 504,
    CMD_HEMOTE = 505,
    CMD_PHEMOTE = 506,
    CMD_SEMOTE = 507,
    CMD_PSEMOTE = 508,
    CMD_ADDKEYWOR = 509,
    CMD_ADDKEYWORD = 510,
    CMD_IOOC = 511,
    CMD_ARRANGE = 512,
    CMD_DELAYED_EMOTE = 513,
    CMD_JSEVAL = 514,
    CMD_SNUFF = 515,
    CMD_IOOC_SHORT = 516,
    CMD_EMPTY = 517,
    CMD_TDESC = 518,
/*    CMD_WARNNEWS = 519, */
    CMD_TRAMPLE = 520,
    CMD_DISCUSS = 521,
    CMD_DIRECTIONS = 522,
    CMD_BURY = 523,
    CMD_BLDCOM = 524,
    CMD_BLDCOM_SHORT = 525,
    MAX_CMD_LIST
} Command;

#endif // __CORE_STRUCTS_INCLUDED

#ifdef __cplusplus
}
#endif

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

