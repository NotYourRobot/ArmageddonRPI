/*
 * File: GUILDS.H
 * Usage: Definitions for guilds and races.
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

#ifndef __GUILDS_INCLUDED
#define __GUILDS_INCLUDED

#include "skills.h"

typedef enum __guild_types {
    GUILD_NONE                   =  0,
    GUILD_DEFILER                =  1,
    GUILD_BURGLAR                =  2,
    GUILD_PICKPOCKET             =  3,
    GUILD_WARRIOR                =  4,
    GUILD_TEMPLAR                =  5,
    GUILD_RANGER                 =  6,
    GUILD_MERCHANT               =  7,
    GUILD_ASSASSIN               =  8,
    GUILD_FIRE_ELEMENTALIST      =  9,
    GUILD_WATER_ELEMENTALIST     = 10,
    GUILD_STONE_ELEMENTALIST     = 11,
    GUILD_WIND_ELEMENTALIST      = 12,
    GUILD_PSIONICIST             = 13,
    GUILD_SHADOW_ELEMENTALIST    = 14,
    GUILD_LIGHTNING_ELEMENTALIST = 15,
    GUILD_VOID_ELEMENTALIST      = 16,
    GUILD_LIRATHU_TEMPLAR        = 17,
    GUILD_JIHAE_TEMPLAR          = 18,
    GUILD_NOBLE                  = 19,
} GuildType;

typedef enum __subguild_types {
    SUB_GUILD_NONE                =  0,
    SUB_GUILD_ACROBAT             =  1,
    SUB_GUILD_ARCHER              =  2,
    SUB_GUILD_ARMOR_CRAFTER       =  3,
    SUB_GUILD_BARD                =  4,
    SUB_GUILD_CARAVAN_GUIDE       =  5,
    SUB_GUILD_CON_ARTIST          =  6,
    SUB_GUILD_FORESTER            =  7,
    SUB_GUILD_GENERAL_CRAFTER     =  8,
    SUB_GUILD_GUARD               =  9,
    SUB_GUILD_HOUSE_SERVANT       = 10,
    SUB_GUILD_HUNTER              = 11,
    SUB_GUILD_JEWELER             = 12,
    SUB_GUILD_LINGUIST            = 13,
    SUB_GUILD_MERCENARY           = 14,
    SUB_GUILD_NOMAD               = 15,
    SUB_GUILD_PHYSICIAN           = 16,
    SUB_GUILD_REBEL               = 17,
    SUB_GUILD_SCAVENGER           = 18,
    SUB_GUILD_STONE_CRAFTER       = 19,
    SUB_GUILD_TAILOR              = 20,
    SUB_GUILD_THIEF               = 21,
    SUB_GUILD_THUG                = 22,
    SUB_GUILD_TINKER              = 23,
    SUB_GUILD_WEAPON_CRAFTER      = 24,

    SUB_GUILD_NOBLE_AESTHETE      = 25,
    SUB_GUILD_NOBLE_ATHLETE       = 26,
    SUB_GUILD_NOBLE_FINANCIER     = 27,
    SUB_GUILD_NOBLE_MUSICIAN      = 28,
    SUB_GUILD_NOBLE_LINGUIST      = 29,
    SUB_GUILD_NOBLE_OUTDOORSMAN   = 30,
    SUB_GUILD_NOBLE_PHYSICIAN     = 31,
    SUB_GUILD_NOBLE_SCHOLAR       = 32,
    SUB_GUILD_NOBLE_SPY           = 33,

    SUB_GUILD_PATH_OF_ENCHANTMENT = 34,
    SUB_GUILD_PATH_OF_KNOWLEDGE   = 35,
    SUB_GUILD_PATH_OF_MOVEMENT    = 36,
    SUB_GUILD_PATH_OF_WAR         = 37,

    SUB_GUILD_SORC_ARCHER         = 38,
    SUB_GUILD_SORC_ASSASSIN       = 39,
    SUB_GUILD_SORC_BARD           = 40,
    SUB_GUILD_SORC_BURGLAR        = 41,
    SUB_GUILD_SORC_MERCENARY      = 42,
    SUB_GUILD_SORC_MERCHANT       = 43,
    SUB_GUILD_SORC_NOMAD          = 44,
    SUB_GUILD_SORC_PICKPOCKET     = 45,
    SUB_GUILD_SORC_RANGER         = 46,
    SUB_GUILD_SORC_WARRIOR        = 47,

    SUB_GUILD_MASTER_TAILOR       = 48,
    SUB_GUILD_MASTER_JEWELER      = 49,
    SUB_GUILD_MASTER_WEAPONSMITH  = 50,
    SUB_GUILD_MASTER_CRAFTER      = 51,
    SUB_GUILD_MASTER_ARMORSMITH   = 52,
    SUB_GUILD_MASTER_TRADER       = 53,
    SUB_GUILD_ROGUE               = 54,
    SUB_GUILD_SLIPKNIFE           = 55,
    SUB_GUILD_CUTPURSE            = 56,
    SUB_GUILD_OUTDOORSMAN         = 57,
    SUB_GUILD_GREBBER             = 58,
    SUB_GUILD_AGGRESSOR           = 59,
    SUB_GUILD_PROTECTOR           = 60,
    SUB_GUILD_ENCHANTMENT_MAGICK  = 61,
    SUB_GUILD_COMBAT_MAGICK       = 62,
    SUB_GUILD_MOVEMENT_MAGICK     = 63,
    SUB_GUILD_ENLIGHTENMENT_MAGICK= 64,
    SUB_GUILD_MASTER_POTTER       = 65,
    SUB_GUILD_CLAYWORKER          = 66,
    SUB_GUILD_MASTER_CHEF         = 67,
    SUB_GUILD_APOTHECARY          = 68,
    SUB_GUILD_MAJORDOMO           = 69,
    SUB_GUILD_BRUISER             = 70,
    SUB_GUILD_BERSERKER           = 71,
    SUB_GUILD_LANCER              = 72,
    MAX_SUB_GUILDS                = 25,
} SubguildType;

#define CAN_USE_SLASHING(ch)     (guild[(int)(GET_GUILD(ch))].can_use_slash)
#define CAN_USE_BLUDGEONING(ch)  (guild[(int)(GET_GUILD(ch))].can_use_bludgeon)
#define CAN_USE_PIERCING(ch)     (guild[(int)(GET_GUILD(ch))].can_use_pierce)
#define GET_ARMOR_MAX_WEIGHT(ch) (guild[(int)(GET_GUILD(ch))].armor_max_weight)

struct guild_data
{
    char *name;
    char *abbrev;

    int strmod;
    int aglmod;
    int wismod;
    int endmod;

    int off_kpc;
    int def_kpc;

    int off_bonus;
    int def_bonus;

    signed char pl_guild;

    signed char can_use_pierce;
    signed char can_use_bludgeon;
    signed char can_use_slash;
    int armor_max_weight;

    int skill_prev[MAX_SKILLS];
    int skill_mana[MAX_SKILLS];
    int skill_max_learned[MAX_SKILLS];
    int skill_percent[MAX_SKILLS];
};

// extern struct guild_data guild[MAX_GUILDS];

struct sub_guild_data
{
    char *name;
    char *abbrev;

    int strmod;
    int aglmod;
    int wismod;
    int endmod;

    int off_kpc;
    int def_kpc;

    int off_bonus;
    int def_bonus;

    signed char pl_guild;

    int skill_prev[MAX_SKILLS];
    int skill_mana[MAX_SKILLS];
    int skill_max_learned[MAX_SKILLS];
    int skill_percent[MAX_SKILLS];

};

// extern struct sub_guild_data sub_guild[MAX_SUB_GUILDS];

/* clan relations 
   removed by ness on 1/12/97
   This stuff is all numeric value now*/
/*
  #define CLAN_REL_NEUTRAL    0
                                                                                                           #define CLAN_REL_ALLY       1  *//* loose association */
                                                                      /* #define CLAN_REL_VASSAL     2 *//* subservient clan  */
                                                                      /* #define CLAN_REL_ENEMY      3 *//* warring clan      */

/* clan structs */

struct clan_rel_type
{
    int nr;                     /* clan index number     */
    int rel_val;                /* value of relationship */
    struct clan_rel_type *next; /* next in list of clans */
};


#define CLAN_NOTHING        0   /* no special variety      */
#define CLAN_MERCHANT       1   /* merchant houses n stuff */
#define CLAN_NOBLE          2   /* nobility                */
#define CLAN_TEMPLAR        3   /* priesthood              */


struct clan_type
{
    char *name;                 /* name of this clan             */
    long account;               /* bank account of the clan      */
    int type;                   /* type of clan                  */
    int flags;                  /* flags                         */
    int status;                 /* overall popularity counter    */
    int store;                  /* rnum of warehouse             */
    int liege;                  /* nr of this clan's master-clan */
    struct clan_rel_type *relations;
};

/* Clans moved to clan.h -Morg */

/* Races */
#define RACE_UNKNOWN     0
#define RACE_HUMAN       1
#define RACE_ELF         2
#define RACE_DWARF       3
#define RACE_MANTIS      4
#define RACE_SILT        5
#define RACE_DRAGON      6
#define RACE_DEMON       7
#define RACE_JOZHAL      8
#define RACE_ANAKORE     9
#define RACE_INIX       10
#define RACE_GIZHAT     11
#define RACE_HALFLING   12
#define RACE_SUNBACK    13
#define RACE_GESRA      14
#define RACE_SNAKE1     15      /* Poisonous Snakes */
#define RACE_VESTRIC    16
#define RACE_TARI       17      /* Rat-men */
#define RACE_VAMPIRE    18
#define RACE_GOLEM      19
#define RACE_GITH       20
#define RACE_GIANT      21
#define RACE_GHAATI     22
#define RACE_HALF_GIANT 23
#define RACE_AVANGION   24
#define RACE_ELEMENTAL  25
#define RACE_SAURIAN    26
#define RACE_THORNWALKER 27
#define RACE_ERDLU      28
#define RACE_KANK       29
#define RACE_JAKHAL     30
#define RACE_BELGOI     31
#define RACE_MUL        32
#define RACE_HALF_ELF   33
#define RACE_TANDU      34
#define RACE_CILOPS     35
#define RACE_BRAXAT     36
#define RACE_WEZER      37
#define RACE_RATLON     38      /* Bred mounts, cross between inix and erdlu */
#define RACE_SNAKE2     39      /* Non-poisonous Snakes */
#define RACE_VULTURE    40      /* Carrion Avians */
#define RACE_EAGLE      41      /* Birds of Prey */
#define RACE_WYVERN     42
#define RACE_GALITH     43      /* Good version of Gith */
#define RACE_THLON      44      /* A large feline used as a mount */
#define RACE_ASLOK      45      /* Big centipede used for hunting */
#define RACE_TEMBO      46      /* Large feline predator */
#define RACE_MEKILLOT   47      /* Extremelly large lizards */
#define RACE_TARANTULA  48      /* Poisonous spiders */
#define RACE_SPIDER     49      /* Non-poisonous spiders */
#define RACE_XILL       50      /* Ethereal nasties */
#define RACE_RODENT     51      /* Mice, Rats, etc */
#define RACE_SCORPION   52      /* Desert insect with pinchers */
#define RACE_MAGERA     53      /* Large ogrish type humanoid */
#define RACE_DJINN      54      /* A sub-set of elemental */
#define RACE_ZOMBIE     55      /* Undead */
#define RACE_JOHZA      56      /* Small lizard, commonly used as a pet */
#define RACE_COCHRA     57      /* Small beetles */
#define RACE_BANSHEE    58      /* Undead */
#define RACE_SILTFLYER  59      /* Avian predators that dwell in silt */
#define RACE_ROC        60      /* Giant avians */
#define RACE_CARRU      61      /* Deer like animals with claws and horns */
#define RACE_GWOSHI     62      /* A wooly quadriped reminiscent of a lion */
#define RACE_GAJ        63      /* A shelled horror that kills almost anything */
#define RACE_ANKHEG     64      /* worm/ant/centipede killer beast */
#define RACE_BIG_INSECT 65      /* Big insects that aren't kanks */
#define RACE_WORM       66      /* Big things */
#define RACE_SUB_ELEMENTAL 67   /* Elementals in humanoid form */
#define RACE_CHEOTAN    68      /* forest lizards, small */
#define RACE_THRYZN     69      /* mutants of the northwest */
#define RACE_WOLF       70      /* HOOOOOOOWWWWWWWWWWWL */
#define RACE_HORSE      71      /* Mounts used by nobles/templars/etc */
#define RACE_TREGIL     72      /* Small, hairless critter w/ big ears */
#define RACE_GOUDRA     73      /* Long-knecked critter with brown fur */
#define RACE_SHIK       74      /* Needle-beaked insect */
#define RACE_MERCH_BAT  75      /* Leather-winged bat thing */
#define RACE_SKEET      76      /* 6 legged critter with knobby skin */
#define RACE_GURTH      77      /* Squat reptile w/ spiky plates */
#define RACE_CHALTON    78      /* Kinda like cows.. herd animals */
#define RACE_QUIRRI     79
#define RACE_BAMUK      80
#define RACE_DUJAT_WORM 81
#define RACE_YOMPAR_LIZARD 82
#define RACE_DURRIT     83
#define RACE_AUROCH     84      /* Kinda like a buffalo */
#define RACE_SAND_RAPTOR 85     /* Similar to a velociraptor from Jurassic Park */
#define RACE_GHYRRAK    86
#define RACE_GAJAK      87
#define RACE_DUSKHORN   88      /* plains dwelling horned antelopes */
#define RACE_KYLORI     89      /* small winged humanoids, barely intelligent */
#define RACE_RITIKKI    90      /* large fanged grasshoppers */
#define RACE_BAHAMET    91      /* huge slow desert tortoises, kank sized */
#define RACE_THYGUN     92      /* Demon sub-race */
#define RACE_FIEND      93      /* Demon sub-race */
#define RACE_USELLIAM   94      /* Demon sub-race */
#define RACE_BEAST      95      /* Demon sub-race */
#define RACE_SHADOW     96      /* Used for send shadow and shadowwalk */
#define RACE_MELEP      97      /* Replacement for belgoi, yellow humanoids */
#define RACE_DESERT_ELF 98      /* Same as elf, but have stamina */
#define RACE_KRYL       99      /* Mob for Laerys's caverns, big insect */
#define RACE_KOLORI    100      /* Small flying humanoid, low int */
#define RACE_KALICH    101      /* Large feline, TM lands */
#define RACE_GORTOK    102      /* Savage, feral hyenalike dogs */
#define RACE_GETH      103      /* Large lizard, TM lands */
#define RACE_ALATAL    104      /* Large bear ape cross, TM lands */
#define RACE_VERRIN    105      /* Large flying predatory bird, northern */
#define RACE_PLAINSOX  106      /* Mount, found in northern plains. */
#define RACE_TURAAL    107      /* Scrub plains critter */
#define RACE_VISHITH   108      /* Hostile plant-life */
#define RACE_BALTHA    109      /* Baboon-type, northern plains */
#define RACE_ESCRU     110      /* Like sheep, used for meat/wool */
#define RACE_TOAD      111      /* Should be fairly easy to guess this one. */
#define RACE_GIANT_SPI 112      /* Giant spiders for Zagren */
#define RACE_WAR_BEETLE 113     /* War beetles are their own selves */
#define RACE_RAVEN      114     /* Raven */
#define RACE_KANK_FLY   115     /* Kank-Fly for Haldol */
#define RACE_SLUG       116     /* Slugs for Haldol */
#define RACE_TUSK_RAT   117     /* Tusk Rats for haldol */
#define RACE_CURR       118     /* Dog-like creatures for Sanvean */
#define RACE_GRETH      119     /* White feathered serpents, Grey Forest */
#define RACE_KIYET_LION 120     /* Mantis valley critters.  -sv */
#define RACE_SILTHOPPER 121     /* For Daigon, sea of silt creature -sv */
#define RACE_SILTSTRIDER 122    /* For Daigon, sea of silt creature -sv */
#define RACE_BATMOUNT    123    /* For Tlaloc, Conclave mount -sv */
#define RACE_GIMPKA_RAT  124    /* Small rodents on the northeastern plains. -sv */
#define RACE_FIREANT     125    /* For Zagren, z49 creature -sv */
#define RACE_SAND_FLIER  126    /* For Brixius, z64 creature -sv */
#define RACE_RANTARRI    127    /* For Halaster, z24 creature -Nessalin */
#define RACE_GHOUL       128    /* For Bhagharva, new beastie -Nessalin 3/15/2003 */
#define RACE_CHARIOT     129
#define RACE_GORMI       130    /* For Vendyra for grey forest, new creature -Nessalin 3/6/2004 */
#define RACE_ARACH       131    /* For Vendyra for grey forest, new creature -Nessalin 3/6/2004 */
#define RACE_BAT         132    /* For Vendyra for grey forest, new creature -Nessalin 3/6/2004 */
#define RACE_AGOUTI      133    /* For Vendyra for grey forest, new creature -Nessalin 3/6/2004 */
#define RACE_GNU         134    /* For Vendyra for grey forest, new creature -Nessalin 3/6/2004 */
#define RACE_SCRAB       135    /* FOr Gilvar, -Nessalin 7/24/2004 */
#define RACE_MEPHIT      136    /* For Halaster, -Nessalin 10/23/2004 */
#define RACE_SILT_TENTACLE 137  /* For Daigon, from Raesanos with love */
#define RACE_SUNLON      138    /* For Vanth -Nessalin 6/12/2007 */
#define MAX_RACES        4

#define RTYPE_OTHER       0
#define RTYPE_HUMANOID    1
#define RTYPE_MAMMALIAN   2
#define RTYPE_INSECTINE   3
#define RTYPE_AVIAN_FLIGHTLESS       4
#define RTYPE_OPHIDIAN    5
#define RTYPE_REPTILIAN   6
#define RTYPE_PLANT       7
#define RTYPE_ARACHNID    8     /*  Az  */
#define RTYPE_AVIAN_FLYING 9
#define RTYPE_SILT        10    /* San */

#define MAX_RYPTE 11



struct skin_data
{
    int item;
    int bonus;
    char *text[2];

    struct skin_data *next;
};

typedef struct attack_data ATTACK_DATA;

struct attack_data
{
    int type;

    int damdice;
    int damsize;
    int damplus;

    signed char bare_handed;

    ATTACK_DATA *next;
};


struct race_data
{
    char *name;
    char *abbrev;

    int strdice;
    int strsize;
    int strplus;

    int agldice;
    int aglsize;
    int aglplus;

    int wisdice;
    int wissize;
    int wisplus;

    int enddice;
    int endsize;
    int endplus;

    int off_base;
    int def_base;

    int natural_armor;

    struct attack_data *attack;

    int race_type;
    int max_age;

    struct skin_data *skin;
};

//extern struct race_data race[MAX_RACES];

#define CLAN_NONE            0
#define CLAN_DOOMBRINGERS    1

#endif

