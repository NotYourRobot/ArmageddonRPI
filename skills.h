/*
 * File: SKILLS.H
 * Usage: Definitions for skills and spells.
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

/* Revision history:
 * 05/15/00 Added skills stonecrafting, jewelrymaking, tentmaking, wagonmaking
 *          swordmaking, spearmaking, sleight of hand and clothworking.  Changed
 *          PICK_MAKING to PICKMAKING.  -Sanvean
 */

#ifndef __SKILLS_INCLUDED
#define __SKILLS_INCLUDED


#define MAX_POISON                  10
#define MAX_CURE_TASTES             8
#define MAX_CURE_FORMS              2

#define MAX_HAL                     49


#define RANGED_STAMINA_DRAIN



#define TYPE_UNDEFINED               -1
#define SPELL_NONE                    0
#define SKILL_NONE                    0

#define SPELL_ARMOR                   1 /* ruk pret grol */
#define SPELL_TELEPORT                2 /* whira locror viod */
#define SPELL_BLIND                   3 /* drov mutur hurn */
#define SPELL_CREATE_FOOD             4 /* ruk wilith wril */
#define SPELL_CREATE_WATER            5 /* vivadu wilith wril */
#define SPELL_CURE_BLINDNESS          6 /* drov viqrol wril */
#define SPELL_DETECT_INVISIBLE        7 /* whira divan inrof */
#define SPELL_DETECT_MAGICK           8 /* suk-krath divan inrof */
#define SPELL_DETECT_POISON           9 /* vivadu divan inrof */
#define SPELL_EARTHQUAKE             10 /* ruk nathro echri */
#define SPELL_FIREBALL               11 /* suk-krath divan hekro */
#define SPELL_HEAL                   12 /* vivadu viqrol wril */
#define SPELL_INVISIBLE              13 /* whira iluth grol */
#define SPELL_LIGHTNING_BOLT         14 /* elkros divan hekro */
#define SPELL_POISON                 15 /* vivadu viqrol hurn */
#define SPELL_REMOVE_CURSE           16 /* drov divan wril */
#define SPELL_SANCTUARY              17 /* whira viqrol grol */
#define SPELL_SLEEP                  18 /* ruk psiak chran */
#define SPELL_STRENGTH               19 /* ruk mutur wril */
#define SPELL_SUMMON                 20 /* whira dreth wril */
#define SPELL_RAIN_OF_FIRE           21 /* suk divan echri */
#define SPELL_CURE_POISON            22 /* vivadu nathro wril */
#define SPELL_RELOCATE               23 /* whira locror wril */
#define SPELL_FEAR                   24 /* drov iluth hurn */
#define SPELL_REFRESH                25 /* elkros viqrol wril */
#define SPELL_LEVITATE               26 /* whira mutur wril */
#define SPELL_LIGHT                  27 /* suk-krath wilith viod */
#define SPELL_ANIMATE_DEAD           28 /* nilaz morz viod */
#define SPELL_DISPEL_MAGICK          29 /* suk-krath psiak echri */
#define SPELL_CALM                   30 /* vivadu psiak chran */
#define SPELL_INFRAVISION            31 /* drov mutur wril */
#define SPELL_FLAMESTRIKE            32 /* suk-krath viqrol hekro */
#define SPELL_SAND_JAMBIYA           33 /* ruk wilith viod */
#define SPELL_STONE_SKIN             34 /* krok pret grol */
#define SPELL_WEAKEN                 35 /* ruk morz hurn */
#define SPELL_GATE                   36 /* nilaz dreth chran */
#define SPELL_OASIS                  37 /* vivadu divan wril */
#define SPELL_SEND_SHADOW            38 /* drov iluth chran */
#define SPELL_SHOW_THE_PATH          39 /* ruk nathro irof */
#define SPELL_DEMONFIRE              40 /* suk-krath morz hekro */
#define SPELL_SANDSTORM              41 /* whira nathro echri */
#define SPELL_HANDS_OF_WIND          42 /* whira nathro hekro */
#define SPELL_ETHEREAL               43 /* drov mutur nikiz */
#define SPELL_DETECT_ETHEREAL        44 /* drov divan inrof */
#define SPELL_BANISHMENT             45 /* whira locror hurn */
#define SPELL_BURROW                 46 /* ruk divan grol */
#define SPELL_EMPOWER                47 /* suk-krath psiak viod */
#define SPELL_FEEBLEMIND             48 /* ruk mutur hurn */
#define SPELL_FURY                   49 /* ruk psiak hurn */
#define SPELL_GUARDIAN               50 /* whira dreth hekro */
#define SPELL_GLYPH                  51 /* suk-krath psiak hurn */
#define SPELL_TRANSFERENCE           52 /* whira locror nikiz */
#define SPELL_INVULNERABILITY        53 /* vivadu pret grol */
#define SPELL_DRAGON_DRAIN           54 /* nilaz morz hurn */
#define SPELL_MARK                   55 /* nilaz psiak viod */
#define SPELL_MOUNT                  56 /* ruk wilith chran */
#define SPELL_PSEUDO_DEATH           57 /* nilaz iluth echri */
#define SPELL_PSI_SUPPRESSION        58 /* nilaz psiak echri */
#define SPELL_RESTFUL_SHADE          59 /* drov nathro wril */
#define SPELL_FLY                    60 /* whira divan chran */
#define SPELL_STALKER                61 /* whira dreth echri */
#define SPELL_THUNDER                62 /* vivadu divan viod */
#define SPELL_DETERMINE_RELATIONSHIP 63 /* vivadu nathro inrof */
#define SPELL_WALL_OF_SAND           64 /* ruk mutur grol */
#define SPELL_UNDEATH                65 /* */
#define SPELL_PYROTECHNICS           66 /* suk-krath iluth nikiz */
#define SPELL_INSOMNIA               67 /* elkros psiak echri */
#define SPELL_TONGUES                68 /* suk-krath mutur inrof */
#define SPELL_CHARM_PERSON           69 /* nilaz mutur nikiz */
#define SPELL_ALARM                  70 /* ruk mutur inrof */
#define SPELL_FEATHER_FALL           71 /* whira mutur grol */
#define SPELL_SHIELD_OF_NILAZ        72 /* nilaz divan grol */
#define SPELL_DEAFNESS               73 /* vivadu iluth hurn */
#define SPELL_SILENCE                74 /* vivadu psiak hurn */
#define SPELL_SOLACE                 75 /* nilaz viqrol wril */
#define SPELL_SLOW                   76 /* elkros mutur chran */

#define SPELL_DRAGON_BANE            83 /* nilaz divan hekro */
#define SPELL_REPEL                  84 /* whira locror chran */
#define SPELL_HEALTH_DRAIN           85 /* vivadu divan hurn */
#define SPELL_AURA_DRAIN             86 /* nilaz divan hurn */
#define SPELL_STAMINA_DRAIN          87 /* elkros divan hurn */

#define SKILL_PEEK                   88
#define SKILL_NIGHT_VISION           89

#define SPELL_GODSPEED               90 /* krok mutur wril */
#define SKILL_WILLPOWER              91

/* psionic skills--these go from easiest to hardest */
/* these are the easy ones--disciplines */
#define PSI_CONTACT                  77
#define PSI_BARRIER                  78
#define PSI_LOCATE                   79
#define PSI_PROBE                    80
#define PSI_TRACE                    81
#define PSI_EXPEL                    82

/* these are the hard ones--sciences */
#define PSI_CATHEXIS                 92
#define PSI_EMPATHY                  93
#define PSI_COMPREHEND_LANGS         94
#define PSI_MINDWIPE                 95
#define PSI_SHADOW_WALK              96
#define PSI_HEAR		     97
#define PSI_CONTROL   		     98
#define PSI_COMPEL                   99
#define PSI_CONCEAL                 100

#define LANG_COMMON                 101
#define LANG_NOMAD                  102
#define LANG_ELVISH                 103
#define LANG_DWARVISH               104
#define LANG_HALFLING               105
#define LANG_MANTIS                 106
#define LANG_GALITH                 107
#define LANG_MERCHANT               108
#define LANG_ANCIENT                109

#define SKILL_ARCHERY               110
#define SKILL_SNEAK                 111
#define SKILL_HIDE                  112
#define SKILL_STEAL                 113
#define SKILL_BACKSTAB              114
#define SKILL_PICK_LOCK             115
#define SKILL_KICK                  116
#define SKILL_BASH                  117
#define SKILL_RESCUE                118
#define SKILL_DISARM                119
#define SKILL_PARRY                 120
#define SKILL_LISTEN                121
#define SKILL_TRAP                  122
#define SKILL_HUNT                  123
#define SKILL_HAGGLE                124
#define SKILL_CLIMB                 125
#define SKILL_BANDAGE               126
#define SKILL_DUAL_WIELD            127
#define SKILL_THROW                 128
#define SKILL_SUBDUE                129
#define SKILL_SCAN                  130
#define SKILL_SEARCH                131
#define SKILL_VALUE                 132
#define SKILL_RIDE                  133
#define SKILL_PILOT                 134
#define SKILL_POISON                135
#define SKILL_GUARD                 136

#define PROF_PIERCING               137
#define PROF_SLASHING               138
#define PROF_CHOPPING               139
#define PROF_BLUDGEONING            140

#define SPICE_METHELINOC            141
#define SPICE_MELEM_TUEK            142
#define SPICE_KRELEZ                143
#define SPICE_KEMEN                 144
#define SPICE_AGGROSS               145
#define SPICE_ZHARAL                146
#define SPICE_THODELIV              147
#define SPICE_KRENTAKH              148
#define SPICE_QEL                   149
#define SPICE_MOSS                  150

#define MIN_SPICE                   141
#define MAX_SPICE                   150

#define RW_LANG_COMMON              151
#define RW_LANG_NOMAD               152
#define RW_LANG_ELVISH              153
#define RW_LANG_DWARVISH            154
#define RW_LANG_HALFLING            155
#define RW_LANG_MANTIS              156
#define RW_LANG_GALITH              157
#define RW_LANG_MERCHANT            158
#define RW_LANG_ANCIENT             159

/* More psi shit */
#define PSI_DOME                    160
#define PSI_CLAIRAUDIENCE           161
#define PSI_MASQUERADE              162
#define PSI_ILLUSION                163

/* More skills */
#define SKILL_BREW                  169

/* More spells */
#define SPELL_CREATE_DARKNESS       170 /* drov wilith echri */
#define SPELL_PARALYZE              171 /* elkros mutur hurn */
#define SPELL_CHAIN_LIGHTNING       172 /* elkros dreth echri */
#define SPELL_LIGHTNING_STORM       173 /* elkros nathro hurn */
#define SPELL_ENERGY_SHIELD         174 /* elkros wilith grol */
#define SPELL_FLASH                 175 /* */
#define SPELL_CURSE                 176 /* drov psiak chran */
#define SPELL_FLOURESCENT_FOOTSTEPS 177 /* elkros mutur inrof */
#define SPELL_REGENERATE            178 /* elkros divan wril */
#define SPELL_FIREBREATHER          179 /* suk-krath nathro viod */
#define SPELL_PARCH                 180 /* suk-krath nathro echri */
#define SPELL_TRAVEL_GATE           181 /* nilaz locror chran */

#define SPELL_FORBID_ELEMENTS       183 /* nilaz nathro echri */
#define SPELL_TURN_ELEMENT          184 /* nilaz pret grol */
#define SPELL_PORTABLE_HOLE         185 /* nilaz wilith viod */
#define SPELL_ELEMENTAL_FOG         186 /* nilaz iluth viod */
#define SPELL_PLANESHIFT            187 /* nilaz locror hurn */
#define SPELL_QUICKENING            188 /* elkros viqrol hekro */

/* NOTICE 
 * If you add spells, make sure you add the numbers to the can_cast_magick()
 * function in utility.c so that it is considered a castable spell for npcs
 */

#define WEAPON_BULLET               189
#define WEAPON_CHATCHKA             190
#define WEAPON_POLEARM              191
#define WEAPON_BOLT                 192
#define WEAPON_ARROW                193
#define WEAPON_AXE                  194
#define WEAPON_SPEAR                195
#define WEAPON_CLUB                 196
#define WEAPON_STAFF                197
#define WEAPON_SWORD                198
#define WEAPON_KNIFE                199


/* everything below here has nothing to do with skills! 
 * they are arranged at the top of the skills list for the
 * purpose of getting a name, nothing more */

/* HACK: max_skills is going to go up, so i've renumbered
 * all types to the new scheme...some affects might get fucked,
 * but that's ok in the short run */

#define TOL_ALCOHOL                 200
#define TOL_GENERIC                 201
#define TOL_GRISHEN                 202
#define TOL_SKELLEBAIN              203
#define TOL_METHELINOC              204
#define TOL_TERRADIN                205
#define TOL_PERAINE                 206
#define TOL_HERAMIDE                207
#define TOL_PLAGUE                  208
#define TOL_PAIN                    209

#define TYPE_HUNGER_THIRST          210 /* Nessalin */
#define TYPE_CRIMINAL               211
#define TYPE_COMBAT                 212 /*  Kel  */
#define TYPE_SHAPE_CHANGE           213 /* Nessalin */
#define TYPE_BURNING                214 /* Azroen */
#define TYPE_LOCKED_INPUT           215 /* Tenebrius */

#define AFF_MUL_RAGE                219 /* Mul berzerking rage */

#define POISON_GENERIC              220 /* General damaging poison */
#define POISON_GRISHEN              221 /* Drops strength and endurance */
#define POISON_SKELLEBAIN           222 /* Halucinogenic Envrionmental*/
#define POISON_METHELINOC           223 /* Deadly. No cure but elf/kank */
#define POISON_TERRADIN             224 /* Causes vomiting */
#define POISON_PERAINE              225 /* Causes paralysis */
#define POISON_HERAMIDE             226 /* Causes a coma */
#define POISON_PLAGUE               227 /* Black Plague */
#define POISON_SKELLEBAIN_2         228 /* Halucinogenic Tripping*/
#define POISON_SKELLEBAIN_3         229 /* Halucinogenic Physical*/


#define LANG_ANYAR                  235 /* Speak Language Anyar */
#define RW_LANG_ANYAR               236 /* RW Language Anyar */

#define LANG_HESHRAK                237 /* Speak Language of GIth */
#define RW_LANG_HESHRAK             238 /* RW Language of Gith */

#define LANG_VLORAN                 239 /* Speak Undead bLanguage */
#define RW_LANG_VLORAN              240 /* RW Undead bLanguage */

#define LANG_DOMAT                  241 /* Speak Thornwalker */
#define RW_LANG_DOMAT               242 /* RW Thornwalker Language */

#define LANG_GHATTI                 243 /* Speak Ghatti */
#define RW_LANG_GHATTI              244 /* RW Ghatti Language */

#define SKILL_SHIELD                245 /* Use of a shield */
#define SKILL_SAP                   246 /* using a sap */

#define SPELL_POSSESS_CORPSE        247 /* spell possess corpse */
#define SPELL_PORTAL                248 /* spell portal */
#define SPELL_SAND_SHELTER          249 /* spell sand shelter */

#define HIT_VS_HUMANOID             250
#define BLUDGEON_VS_HUMANOID        251
#define PIERCE_VS_HUMANOID          252
#define STAB_VS_HUMANOID            253
#define CHOP_VS_HUMANOID            254
#define SLASH_VS_HUMANOID           255
#define WHIP_VS_HUMANOID            256

#define HIT_VS_MAMMALIAN            257
#define BLUDGEON_VS_MAMMALIAN       258
#define PIERCE_VS_MAMMALIAN         259
#define STAB_VS_MAMMALIAN           260
#define CHOP_VS_MAMMALIAN           261
#define SLASH_VS_MAMMALIAN          262
#define WHIP_VS_MAMMALIAN           263

#define HIT_VS_INSECTINE            264
#define BLUDGEON_VS_INSECTINE       265
#define PIERCE_VS_INSECTINE         266
#define STAB_VS_INSECTINE           267
#define CHOP_VS_INSECTINE           268
#define SLASH_VS_INSECTINE          269
#define WHIP_VS_INSECTINE           270

#define HIT_VS_FLIGHTLESS           271
#define BLUDGEON_VS_FLIGHTLESS      272
#define PIERCE_VS_FLIGHTLESS        273
#define STAB_VS_FLIGHTLESS          274
#define CHOP_VS_FLIGHTLESS          275
#define SLASH_VS_FLIGHTLESS         276
#define WHIP_VS_FLIGHTLESS          277

#define HIT_VS_OPHIDIAN             278
#define BLUDGEON_VS_OPHIDIAN        279
#define PIERCE_VS_OPHIDIAN          280
#define STAB_VS_OPHIDIAN            281
#define CHOP_VS_OPHIDIAN            282
#define SLASH_VS_OPHIDIAN           283
#define WHIP_VS_OPHIDIAN            284

#define HIT_VS_REPTILIAN            285
#define BLUDGEON_VS_REPTILIAN       286
#define PIERCE_VS_REPTILIAN         287
#define STAB_VS_REPTILIAN           288
#define CHOP_VS_REPTILIAN           289
#define SLASH_VS_REPTILIAN          290
#define WHIP_VS_REPTILIAN           291

#define HIT_VS_PLANT                292
#define BLUDGEON_VS_PLANT           293
#define PIERCE_VS_PLANT             294
#define STAB_VS_PLANT               295
#define CHOP_VS_PLANT               296
#define SLASH_VS_PLANT              297
#define WHIP_VS_PLANT               298

#define HIT_VS_ARACHNID             299
#define BLUDGEON_VS_ARACHNID        300
#define PIERCE_VS_ARACHNID          301
#define STAB_VS_ARACHNID            302
#define CHOP_VS_ARACHNID            303
#define SLASH_VS_ARACHNID           304
#define WHIP_VS_ARACHNID            305

#define HIT_VS_AVIAN_FLYING         306
#define BLUDGEON_VS_AVIAN_FLYING    307
#define PIERCE_VS_AVIAN_FLYING      308
#define STAB_VS_AVIAN_FLYING        309
#define CHOP_VS_AVIAN_FLYING        310
#define SLASH_VS_AVIAN_FLYING       311
#define WHIP_VS_AVIAN_FLYING        312


#define SKILL_SKINNING              313
#define SKILL_BASKETWEAVING         314
#define SKILL_TANNING               315

#define SKILL_FORAGE                316 /* skill forage */

/* Adding some unarmed combat skills for Ness here - Tiernan */
#define SKILL_NESSKICK              317
#define SKILL_FLEE                  318

/* moving some skills down that were repeats above -Nessalin */
#define SPELL_GOLEM                 319 /* krok dreth chran */
#define SPELL_FIRE_JAMBIYA          320 /* suk-krath dreth hekro */
#define SKILL_FLETCHERY             321
#define SKILL_DYEING                322
#define SKILL_PICKMAKING            323
#define SKILL_LUMBERJACKING         324
#define SKILL_FEATHER_WORKING       325
#define SKILL_LEATHER_WORKING       326
#define SKILL_ROPE_MAKING           327
#define SKILL_BOW_MAKING            328
#define SKILL_INSTRUMENT_MAKING     329
#define SKILL_BANDAGEMAKING         330
#define SKILL_WOODWORKING           331
#define SKILL_ARMORMAKING           332
#define SKILL_KNIFE_MAKING          333
#define SKILL_COMPONENT_CRAFTING    334
#define SKILL_FLORISTRY             335
#define SKILL_COOKING               336
#define SPELL_IDENTIFY              337 /* nilaz threl inrof */
#define SPELL_WALL_OF_FIRE          338 /* suk-krath mutur grol */
#define SKILL_STONECRAFTING         339
#define SKILL_JEWELRYMAKING         340
#define SKILL_TENTMAKING            341
#define SKILL_WAGONMAKING           342
#define SKILL_SWORDMAKING           343
#define SKILL_SPEARMAKING           344
#define SKILL_SLEIGHT_OF_HAND       345
#define SKILL_CLOTHWORKING          346
#define SPELL_WALL_OF_WIND          347 /* whira mutur viod */
#define SPELL_BLADE_BARRIER         348 /* nilaz mutur grol */
#define SPELL_WALL_OF_THORN         349 /* vivadu mutur grol */
#define SPELL_WIND_ARMOR            350 /* whira pret grol */
#define SPELL_FIRE_ARMOR            351 /* suk-krath pret grol */
#define SKILL_ARMOR_REPAIR          352
#define SKILL_WEAPON_REPAIR         353
#define PSI_SENSE_PRESENCE          354
#define PSI_THOUGHTSENSE            355
#define PSI_SUGGESTION              356
#define PSI_BABBLE                  357
#define PSI_DISORIENT               358
#define PSI_CLAIRVOYANCE            359
#define PSI_IMITATE                 360
#define PSI_PROJECT_EMOTION         361
#define PSI_VANISH                  362
#define PSI_BEAST_AFFINITY          363
#define PSI_COERCION                364
#define PSI_WILD_CONTACT            365
#define PSI_WILD_BARRIER            366
#define SKILL_ANALYZE               367
#define SKILL_ALCHEMY               368
#define SKILL_TOOLMAKING            369
#define SPELL_HAUNT                 370 /* drov dreth chran */
#define SKILL_WAGON_REPAIR          371
#define SPELL_CREATE_WINE           372 /* vivadu wilith chran */
#define SPELL_WIND_FIST             373 /* whira divan hurn */
#define SPELL_LIGHTNING_WHIP        374 /* elkros wilith viod */
#define SPELL_SHADOW_SWORD          375 /* drov wilith viod */
#define SPELL_PSIONIC_DRAIN	    376 /* nilaz divan echri */
#define SPELL_INTOXICATION          377 /* vivadu mutur hurn */
#define SPELL_SOBER                 378 /* vivadu mutur viod */
#define SPELL_WITHER                379 /* vivadu nathro hurn */
#define SPELL_DELUSION              380 /* whira iluth nikiz */
#define SPELL_ILLUMINANT            381 /* elkros iluth nikiz */
#define SPELL_MIRAGE                382 /* vivadu iluth nikiz */
#define SPELL_PHANTASM              383 /* nilaz iluth nikiz */
#define SPELL_SHADOWPLAY            384 /* drov iluth nikiz */

/* Defining stuff for the sniff stuff  */
#define SCENT_DOODIE                385
#define SCENT_ROSES                 386
#define SCENT_MUSK                  387
#define SCENT_CITRON                388
#define SCENT_SPICE                 389
#define SCENT_SHRINTAL              390
#define SCENT_LANTURIN              391
#define SCENT_GLIMMERGRASS          392
#define SCENT_BELGOIKISS            393
#define SCENT_LAVENDER              394
#define SCENT_JASMINE               395
#define SCENT_CLOVE                 396
#define SCENT_MINT                  397
#define SCENT_PYMLITHE              398
#define SKILL_DRAWING               399 /* Adding for a project. -San */

#define LANG_RINTH_ACCENT           400 /* Speak Rinthi */
#define LANG_NORTH_ACCENT           402 /* Speak Northlands */
#define LANG_SOUTH_ACCENT           404 /* Speak Southlands */

#define PROF_PIKE                   406
#define PROF_TRIDENT                407
#define PROF_POLEARM                408
#define PROF_KNIFE                  409
#define PROF_RAZOR                  410
#define SPELL_SANDSTATUE            411 /* krok wilith nikiz */
#define SKILL_HEADSHRINKING         412

/* Lirathuan Psi skills */
#define PSI_MAGICKSENSE		    413
#define PSI_MINDBLAST		    414

#define SKILL_CHARGE		    415
#define SKILL_SPLIT_OPPONENTS	    416
#define SKILL_TWO_HANDED	    417

#define LANG_ANYAR_ACCENT           418

#define LANG_BENDUNE_ACCENT         420
#define SPELL_DISEMBODY             422
#define SKILL_AXEMAKING             423
#define SKILL_CLUBMAKING            424
#define SKILL_WEAVING               425
#define SPELL_WIND_SHEILD           426
#define SPELL_SHIELD_OF_MIST        427
#define SPELL_DROWN                 428
#define SPELL_HEALING_MUD           429
#define SPELL_CAUSE_DISEASE         430
#define SPELL_CURE_DISEASE          431
#define SPELL_ACID_SPRAY            432
#define SPELL_PUDDLE                433
#define SPELL_SHADOW_ARMOR          434
#define SPELL_FIRE_SEED             435 /* suk-krath wilith hekro */

#define LANG_WATER_ACCENT           436
#define LANG_FIRE_ACCENT            437
#define LANG_WIND_ACCENT            438
#define LANG_EARTH_ACCENT           439
#define LANG_SHADOW_ACCENT          440
#define LANG_LIGHTNING_ACCENT       441
#define LANG_NILAZ_ACCENT           442
#define SPELL_BREATHE_WATER         443
#define SPELL_LIGHTNING_SPEAR       444

#define SKILL_DIRECTION_SENSE	    445
#define PSI_MESMERIZE		    446

#define SPELL_SHATTER  		    447 /* Krok divan echri */
#define SPELL_MESSENGER		    448 /* whira dreth viod */
#define SKILL_GATHER		    449
#define PSI_REJUVENATE		    450

#define SKILL_BLIND_FIGHT	    451

#define SPELL_DAYLIGHT              452 /* suk-krath wilith wril */
#define SPELL_DISPEL_INVISIBLE      453 /* whira psiak echri */
#define SPELL_DISPEL_ETHEREAL       454 /* drov psiak echri */
#define SPELL_REPAIR_ITEM           455 /* ruk mutur chran */
#define SPELL_CREATE_RUNE           456 /* whira wilith viod */

#define SKILL_WATCH                 457

#define SPELL_IMMOLATE              458 /* suk-krath psiak hekro */

#define SKILL_REACH_LOCAL           459
#define SKILL_REACH_NONE            460
#define SKILL_REACH_POTION          461
#define SKILL_REACH_SCROLL          462
#define SKILL_REACH_WAND            463
#define SKILL_REACH_STAFF           464
#define SKILL_REACH_ROOM            465
#define SKILL_REACH_SILENT          466

#define SPELL_ROT_ITEMS             467 /* nilaz nathro chran */
#define SPELL_VAMPIRIC_BLADE        468 /* nilaz psiak hekro */
#define SPELL_DEAD_SPEAK            469 /* nilaz morz chran */

#define DISEASE_BOILS               470
#define DISEASE_CHEST_DECAY         471
#define DISEASE_CILOPS_KISS         472
#define DISEASE_GANGRENE            473
#define DISEASE_HEAT_SPLOTCH        474
#define DISEASE_IVORY_SALT_SICKNESS 475
#define DISEASE_KANK_FLEAS          476
#define DISEASE_KRATHS_TOUCH        477
#define DISEASE_MAAR_POX            478
#define DISEASE_PEPPERBELLY         479
#define DISEASE_RAZA_RAZA           480
#define DISEASE_SAND_FEVER          481
#define DISEASE_SAND_FLEAS          482
#define DISEASE_SAND_ROT            483
#define DISEASE_SCRUB_FEVER         484
#define DISEASE_SLOUGH_SKIN         485
#define DISEASE_YELLOW_PLAGUE       486

#define PSI_IMMERSION               487

#define SPELL_HERO_SWORD            488
#define SPELL_RECITE                489

#define SCENT_EYNANA                490
#define SCENT_KHEE                  491
#define SCENT_FILANYA               492
#define SCENT_LADYSMANTLE           493
#define SCENT_LIRATHUFAVOR          494
#define SCENT_PFAFNA                495
#define SCENT_REDHEART              496
#define SCENT_SWEETBREEZE           497
#define SCENT_TEMBOTOOTH            498
#define SCENT_THOLINOC              499

#define SKILL_TRAMPLE               500
#define LANG_DESERT_ACCENT          501
#define LANG_EASTERN_ACCENT         502
#define LANG_WESTERN_ACCENT         503
#define SCENT_MEDICHI               504
#define SKILL_CLAYWORKING           505
#define SCENT_JOYLIT		    506
#define MAX_SKILLS                  507

/* whenever this number goes up, it should match MAX_SKILLS in structs.h */
/* and an entry made in constants.c to reflect the skill name */



/* everything below here has nothing to do with EITHER skills
 * or affect names...they should be greater than 300 to keep them
 * away from everything above, but that's all */

#define TYPE_HIT                    300
#define TYPE_BLUDGEON               301
#define TYPE_PIERCE                 302
#define TYPE_STAB                   303
#define TYPE_CHOP                   304
#define TYPE_SLASH                  305
#define TYPE_CLAW                   306
#define TYPE_WHIP                   307
#define TYPE_PINCH                  308
#define TYPE_STING                  309 /*  Kel  */
#define TYPE_BITE                   310 /*  Az   */
#define TYPE_PECK                   311 /*  Az   */
#define TYPE_GORE                   312
#define TYPE_PIKE                   313
#define TYPE_TRIDENT                314
#define TYPE_POLEARM                315
#define TYPE_KNIFE                  316
#define TYPE_RAZOR                  317

#define TYPE_MAX                    350

#define TYPE_SUFFERING              400
#define TYPE_THIRST                 401

#define TYPE_BOW                    411

#define TYPE_ELEM_ALL               412
#define TYPE_ELEM_FIRE              413
#define TYPE_ELEM_WATER             414
#define TYPE_ELEM_EARTH             415
#define TYPE_ELEM_WIND              416
#define TYPE_ELEM_SHADOW            417
#define TYPE_ELEM_LIGHTNING         418
#define TYPE_ELEM_VOID              419

#define DUAL_WIELD_KPC              20
#define TWO_HANDED_KPC              20

#define MAX_TYPES 70

#define HERB_NONE     0
#define HERB_ACIDIC   1
#define HERB_SWEET    2
#define HERB_SOUR     3
#define HERB_BLAND    4
#define HERB_HOT      5
#define HERB_COLD     6
#define HERB_CALMING  7
#define HERB_REFRESHING 8

#define MAX_HERB_TYPE   8

#define HERB_RED      0
#define HERB_BLUE     1
#define HERB_GREEN    2
#define HERB_BLACK    3
#define HERB_PURPLE   4
#define HERB_YELLOW   5

#define MAX_HERB_COLORS 5

#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4

#define MAX_SPL_LIST	128


#define CLASS_NONE        0
#define CLASS_MAGICK      1
#define CLASS_COMBAT      2
#define CLASS_STEALTH     3
#define CLASS_MANIP       4
#define CLASS_PERCEP      5
#define CLASS_BARTER      6
#define CLASS_SPICE       7
#define CLASS_LANG        8
#define CLASS_WEAPON      9
#define CLASS_PSIONICS   10
#define CLASS_TOLERANCE  11
#define CLASS_KNOWLEDGE  12
#define CLASS_CRAFT      13
#define CLASS_SCENT      14
#define CLASS_REACH      15

#define TAR_IGNORE		(1 << 0)
#define TAR_CHAR_ROOM		(1 << 1)
#define TAR_CHAR_WORLD		(1 << 2)
#define TAR_FIGHT_SELF		(1 << 3)
#define TAR_FIGHT_VICT		(1 << 4)
#define TAR_SELF_ONLY		(1 << 5)
#define TAR_SELF_NONO		(1 << 6)
#define TAR_OBJ_INV		(1 << 7)
#define TAR_OBJ_ROOM		(1 << 8)
#define TAR_OBJ_WORLD		(1 << 9)
#define TAR_OBJ_EQUIP		(1 << 10)
#define TAR_CHAR_DIR		(1 << 11)
#define TAR_USE_EQUIP		(1 << 12)
#define TAR_CHAR_CONTACT	(1 << 13)
#define TAR_MORTAL_PLANE	(1 << 14)
#define TAR_ETHEREAL_PLANE	(1 << 15)

struct skill_data
{
    void (*function_pointer) (byte, struct char_data *, const char *, int, struct char_data *,
                              struct obj_data *);
    byte delay;
    byte min_pos;
    byte sk_class;
    int targets;
    byte min_mana;
    byte start_p;
    byte element;
    byte sphere;
    byte mood;
    int components;
    int powerlevel;
};

extern struct skill_data skill[MAX_SKILLS];

/* Possible Targets:
   
   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

/* Some categories of crimes */
#define CRIME_UNKNOWN      0
#define CRIME_ASSAULT      1
#define CRIME_THEFT        2
#define CRIME_CONTRABAND   3
#define CRIME_DEFILING     4
#define CRIME_SALES        5
#define CRIME_TREASON      6

#define MAX_CRIMES         6

#define POWER_WEK          0
#define POWER_YUQA         1
#define POWER_KRAL         2
#define POWER_EEN          3
#define POWER_PAV          4
#define POWER_SUL          5
#define POWER_MON          6

#define LAST_POWER         6

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4
#define SPELL_TYPE_SPICE   5
#define SPELL_TYPE_ROOM    6    /* Used for the room reach */


/* Spell symbols */

#define REACH_LOCAL            1
#define REACH_NONE             2
#define REACH_POTION           3
#define REACH_SCROLL           4
#define REACH_WAND             5
#define REACH_STAFF            6
#define REACH_ROOM             7
#define REACH_SILENT           8

#define LAST_REACH             8

#define ELEMENT_FIRE           1        /* suk-krath */
#define ELEMENT_WATER          2        /* vivadu */
#define ELEMENT_EARTH          3        /* ruk */
#define ELEMENT_STONE          4        /* krok */
#define ELEMENT_AIR            5        /* whira */
#define ELEMENT_SHADOW         6        /* drov */
#define ELEMENT_ELECTRIC       7        /* elkros */
#define ELEMENT_VOID           8        /* nilaz */

#define LAST_ELEMENT           8

#define SPHERE_ABJURATION      1        /* pret */
#define SPHERE_ALTERATION      2        /* mutur */
#define SPHERE_CONJURATION     3        /* dreth */
#define SPHERE_ENCHANTMENT     4        /* psiak */
#define SPHERE_ILLUSION        5        /* iluth */
#define SPHERE_INVOCATION      6        /* divan */
#define SPHERE_DIVINATION      7        /* threl */
#define SPHERE_NECROMANCY      8        /* morz */
#define SPHERE_CLERICAL        9        /* viqrol */
#define SPHERE_NATURE         10        /* nathro */
#define SPHERE_TELEPORTATION  11        /* locror */
#define SPHERE_CREATION       12        /* wilith */

#define LAST_SPHERE           12

#define MOOD_AGGRESSIVE        1        /* hekro */
#define MOOD_PROTECTIVE        2        /* grol */
#define MOOD_DOMINANT          3        /* chran */
#define MOOD_DESTRUCTIVE       4        /* echri */
#define MOOD_BENEFICIAL        5        /* wril */
#define MOOD_PASSIVE           6        /* nikiz */
#define MOOD_HARMFUL           7        /* hurn */
#define MOOD_REVEALING         8        /* inrof */
#define MOOD_NEUTRAL           9        /* viod */

#define LAST_MOOD              9

#define COMP_ABJURATION        1
#define COMP_ALTERATION        2
#define COMP_CONJURATION       3
#define COMP_ENCHANTMENT       4
#define COMP_ILLUSION          5
#define COMP_INVOCATION        6
#define COMP_DIVINATION        7
#define COMP_NECROMANCY        8
#define COMP_CLERICAL          9
#define COMP_NATURE           10
#define COMP_TELEPORTATION    11
#define COMP_CREATION         12

/* Attacktypes with grammar */

struct attack_hit_type
{
    char *singular;
    char *plural;
};

bool check_str(struct char_data *ch, int bonus);
bool check_agl(struct char_data *ch, int bonus);
bool check_end(struct char_data *ch, int bonus);
bool check_wis(struct char_data *ch, int bonus);

int increase_or_give_char_skill(CHAR_DATA * ch, int skillno, int skill_perc);
#endif

