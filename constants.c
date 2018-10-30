/*
 * File: CONSTANTS.C
 * Usage: Definitions of constants used elsewhere in the game.
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

/* Revision History:
 * 05/15/00 Added stonecrafting, jewelrymaking, tentmaking, wagonmaking,
 *          spearmaking, swordmaking, sleight of hand and clothworking
 *          as skills.  -Sanvean
 * 07/01/00 Added alchemy and toolmaking as skills. -San
 */

#include <stdio.h>

#include "constants.h"
#include "structs.h"
#include "db.h"
#include "limits.h"
#include "skills.h"
#include "utils.h"
#include "guilds.h"
#include "clan.h"
#include "cities.h"
#include "comm.h"

bool use_shadow_moon = TRUE;
bool use_global_darkness = FALSE;

const char * const command_name[] = {
/*   0 */ "",
/*   1 */ "north",
/*   2 */ "east",
/*   3 */ "south",
/*   4 */ "west",
/*   5 */ "up",
/*   6 */ "down",
/*   7 */ "enter",
/*   8 */ "exits",
/*   9 */ "keyword",
/*  10 */ "get",
/*  11 */ "drink",
/*  12 */ "eat",
/*  13 */ "wear",
/*  14 */ "wield",
/*  15 */ "look",
/*  16 */ "score",
/*  17 */ "say",
/*  18 */ "shout",
/*  19 */ "rest",
/*  20 */ "inventory",
/*  21 */ "qui",
/*  22 */ "snort",
/*  23 */ "smile",
/*  24 */ "dance",
/*  25 */ "kill",
/*  26 */ "cackle",
/*  27 */ "laugh",
/*  28 */ "giggle",
/*  29 */ "shake",
/*  30 */ "shuffle",
/*  31 */ "growl",
/*  32 */ "scream",
/*  33 */ "insult",
/*  34 */ "comfort",
/*  35 */ "nod",
/*  36 */ "sigh",
/*  37 */ "slee",
/*  38 */ "help",
/*  39 */ "who",
/*  40 */ "emote",
/*  41 */ "echo",
/*  42 */ "stand",
/*  43 */ "sit",
/*  44 */ "rs",
/*  45 */ "sleep",
/*  46 */ "wake",
/*  47 */ "force",
/*  48 */ "tran",
/*  49 */ "hug",
/*  50 */ "server",
/*  51 */ "cuddle",
/*  52 */ "stow",
/*  53 */ "cry",
/*  54 */ "news",
/*  55 */ "equipment",
/*  56 */ "oldbuy",
/*  57 */ "sell",
/*  58 */ "value",
/*  59 */ "list",
/*  60 */ "drop",
/*  61 */ "goto",
/*  62 */ "weather",
/*  63 */ "read",
/*  64 */ "pour",
/*  65 */ "grab",
/*  66 */ "remove",
/*  67 */ "put",
/*  68 */ "shutdow",
/*  69 */ "save",
/*  70 */ "hit",
/*  71 */ "ocopy",
/*  72 */ "give",
/*  73 */ "quit",
/*  74 */ "stat",
/*  75 */ "rstat",
/*  76 */ "time",
/*  77 */ "purge",
/*  78 */ "shutdown",
/*  79 */ "idea",
/*  80 */ "typo",
/*  81 */ "buy",
/*  82 */ "whisper",
/*  83 */ "cast",
/*  84 */ "at",
/*  85 */ "ask",
/*  86 */ "order",
/*  87 */ "sip",
/*  88 */ "taste",
/*  89 */ "snoop",
/*  90 */ "follow",
/*  91 */ "rent",
/*  92 */ "offer",
/*  93 */ "poke",
/*  94 */ "advance",
/*  95 */ "accuse",
/*  96 */ "grin",
/*  97 */ "bow",
/*  98 */ "open",
/*  99 */ "close",
/* 100 */ "lock",
/* 101 */ "unlock",
/* 102 */ "leave",
/* 103 */ "applaud",
/* 104 */ "blush",
/* 105 */ "rejuvenate",
/* 106 */ "chuckle",
/* 107 */ "clap",
/* 108 */ "cough",
/* 109 */ "curtsey",
/* 110 */ "cease",
/* 111 */ "flip",
/* 112 */ "stop",
/* 113 */ "frown",
/* 114 */ "gasp",
/* 115 */ "glare",
/* 116 */ "groan",
/* 117 */ "slip",
/* 118 */ "hiccup",
/* 119 */ "lick",
/* 120 */ "gesture",
/* 121 */ "moan",
/* 122 */ "ready",
/* 123 */ "pout",
/* 124 */ "study",
/* 125 */ "ruffle",
/* 126 */ "shiver",
/* 127 */ "shrug",
/* 128 */ "sing",
/* 129 */ "slap",
/* 130 */ "smirk",
/* 131 */ "snap",
/* 132 */ "sneeze",
/* 133 */ "snicker",
/* 134 */ "sniff",
/* 135 */ "snore",
/* 136 */ "spit",
/* 137 */ "squeeze",
/* 138 */ "stare",
/* 139 */ "biography",
/* 140 */ "thank",
/* 141 */ "twiddle",
/* 142 */ "wave",
/* 143 */ "whistle",
/* 144 */ "wiggle",
/* 145 */ "wink",
/* 146 */ "yawn",
/* 147 */ "nosave",
/* 148 */ "write",
/* 149 */ "hold",
/* 150 */ "flee",
/* 151 */ "sneak",
/* 152 */ "hide",
/* 153 */ "backstab",
/* 154 */ "pick",
/* 155 */ "steal",
/* 156 */ "bash",
/* 157 */ "rescue",
/* 158 */ "kick",
/* 159 */ "coerce",
/* 160 */ "unlatch",
/* 161 */ "latch",
/* 162 */ "tickle",
/* 163 */ "mesmerize",
/* 164 */ "examine",
/* 165 */ "take",
/* 166 */ "donate",
/* 167 */ "'",
/* 168 */ "guarding",
/* 169 */ "curse",
/* 170 */ "use",
/* 171 */ "where",
/* 172 */ "reroll",
/* 173 */ "pray",
/* 174 */ ":",
/* 175 */ "beg",
/* 176 */ "bleed",
/* 177 */ "cringe",
/* 178 */ "daydream",
/* 179 */ "fume",
/* 180 */ "grovel",
/* 181 */ "vanish",
/* 182 */ "nudge",
/* 183 */ "peer",
/* 184 */ "point",
/* 185 */ "ponder",
/* 186 */ "punch",
/* 187 */ "snarl",
/* 188 */ "spank",
/* 189 */ "steam",
/* 190 */ "tackle",
/* 191 */ "taunt",
/* 192 */ "think",
/* 193 */ "whine",
/* 194 */ "worship",
/* 195 */ "feel",
/* 196 */ "brief",
/* 197 */ "wizlist",
/* 198 */ "consider",
/* 199 */ "cunset",
/* 200 */ "restore",
/* 201 */ "return",
/* 202 */ "switch",
/* 203 */ "quaff",
/* 204 */ "recite",
/* 205 */ "users",
/* 206 */ "pose",
/* 207 */ "award",
/* 208 */ "wizhelp",
/* 209 */ "credits",
/* 210 */ "compact",
/* 211 */ "skills",
/* 212 */ "jump",
/* 213 */ "message",
/* 214 */ "rcopy",
/* 215 */ "disarm",
/* 216 */ "knock",
/* 217 */ "listen",
/* 218 */ "trap",
/* 219 */ "tenebrae",
/* 220 */ "hunt",
/* 221 */ "rsave",
/* 222 */ "kneel",
/* 223 */ "bite",
/* 224 */ "make",
/* 225 */ "smoke",
/* 226 */ "peek",
/* 227 */ "rzsave",
/* 228 */ "rzclear",
/* 229 */ "mcopy",
/* 230 */ "nonews",
/* 231 */ "rcreate",
/* 232 */ "damage",
/* 233 */ "immcom",
/* 234 */ "]",
/* 235 */ "zassign",
/* 236 */ "show",
/* 237 */ "osave",
/* 238 */ "ocreate",
/* 239 */ "title",
/* 240 */ "pilot",
/* 241 */ "oset",
/* 242 */ "nuke",
/* 243 */ "split",
/* 244 */ "zsave",
/* 245 */ "backsave",
/* 246 */ "msave",
/* 247 */ "cset",
/* 248 */ "mcreate",
/* 249 */ "rzreset",
/* 250 */ "zclear",
/* 251 */ "zreset",
/* 252 */ "mdup",
/* 253 */ "log",
/* 254 */ "behead",
/* 255 */ "plist",
/* 256 */ "criminal",
/* 257 */ "freeze",
/* 258 */ "bset",
/* 259 */ "gone",
/* 260 */ "draw",
/* 261 */ "zset",
/* 262 */ "delete",
/* 263 */ "races",
/* 264 */ "hayyeh",
/* 265 */ "applist",
/* 266 */ "pull",
/* 267 */ "review",
/* 268 */ "ban",
/* 269 */ "unban",
/* 270 */ "bsave",
/* 271 */ "craft",
/* 272 */ "prompt",
/* 273 */ "motd",
/* 274 */ "tan",
/* 275 */ "invis",
/* 276 */ "(",
/* 277 */ "path",
/* 278 */ "ep",
/* 279 */ "es",
/* 280 */ "reply",
/* 281 */ "quiet",
/* 282 */ "_wsave",
/* 283 */ "godcom",
/* 284 */ "}",
/* 285 */ "_wclear",
/* 286 */ "_wreset",
/* 287 */ "_wzsave",
/* 288 */ "_wupdate",
/* 289 */ "bandage",
/* 290 */ "teach",
/* 291 */ "dig",
/* 292 */ "alias",
/* 293 */ "system",
/* 294 */ "nonet",
/* 295 */ "dnstat",
/* 296 */ "plmax",
/* 297 */ "idle",
/* 298 */ "immcmds",
/* 299 */ "fill",
/* 300 */ "ls",
/* 301 */ "page",
/* 302 */ "edit",
/* 303 */ "reset",
/* 304 */ "beep",
/* 305 */ "recruit",
/* 306 */ "rebel",
/* 307 */ "dump",
/* 308 */ "clanleader",
/* 309 */ "throw",
/* 310 */ "mail",
/* 311 */ "subdue",
/* 312 */ "promote",
/* 313 */ "demote",
/* 314 */ "wish",
/* 315 */ "vis",
/* 316 */ "junk",
/* 317 */ "deal",
/* 318 */ "oupdate",
/* 319 */ "mupdate",
/* 320 */ "cwho",
/* 321 */ "notitle",
/* 322 */ "hear",
/* 323 */ "walk",
/* 324 */ "run",
/* 325 */ "balance",
/* 326 */ "deposit",
/* 327 */ "withdraw",
/* 328 */ "approve",
/* 329 */ "reject",
/* 330 */ "rupdate",
/* 331 */ "reach",
/* 332 */ "break",
/* 333 */ "contact",
/* 334 */ "locate",
/* 335 */ "_unapprove",
/* 336 */ "nowish",
/* 337 */ "symbol",
/* 338 */ "push",
/* 339 */ "memory",
/* 340 */ "mount",
/* 341 */ "dismount",
/* 342 */ "zopen",
/* 343 */ "zclose",
/* 344 */ "rdelete",
/* 345 */ "ostat",
/* 346 */ "barter",
/* 347 */ "settime",
/* 348 */ "skillreset",
/* 349 */ "grp",
/* 350 */ "cstat",
/* 351 */ "call",
/* 352 */ "relieve",
/* 353 */ "incriminate",
/* 354 */ "pardon",
/* 355 */ "talk",
/* 356 */ "ooc",
/* 357 */ "apply",
/* 358 */ "skin",
/* 359 */ "gather",
/* 360 */ "release",
/* 361 */ "poisoning",
/* 362 */ "sheathe",
/* 363 */ "assess",
/* 364 */ "pack",
/* 365 */ "unpack",
/* 366 */ "zorch",
/* 367 */ "change",
/* 368 */ "shoot",
/* 369 */ "load",
/* 370 */ "unload",
/* 371 */ "dwho",
/* 372 */ "shield",
/* 373 */ "cathexis",
/* 374 */ "psi",
/* 375 */ "assist",
/* 376 */ "poofin",
/* 377 */ "poofout",
/* 378 */ "cd",
/* 379 */ "rset",
/* 380 */ "runset",
/* 381 */ "ounset",
/* 382 */ "odelete",
/* 383 */ "cdelete",
/* 384 */ "bet",
/* 385 */ "play",
/* 386 */ "rm",
/* 387 */ "disengage",
/* 388 */ "infobar",
/* 389 */ "scan",
/* 390 */ "search",
/* 391 */ "tailor",
/* 392 */ "fload",
/* 393 */ "pinfo",
/* 394 */ "rp",
/* 395 */ "shadow",
/* 396 */ "hitch",
/* 397 */ "unhitch",
/* 398 */ "pulse",
/* 399 */ "tax",
/* 400 */ "pemote",
/* 401 */ "wset",
/* 402 */ "execute",
/* 403 */ "etwo",
/* 404 */ "plant",
/* 405 */ "trace",
/* 406 */ "barrier",
/* 407 */ "disperse",
/* 408 */ "bug",
/* 409 */ "harness",
/* 410 */ "unharness",
/* 411 */ "touch",
/* 412 */ "trade",
/* 413 */ "empathy",
/* 414 */ "shadowwalk",
/* 415 */ "allspeak",
/* 416 */ "commune",
/* 417 */ "expel",
/* 418 */ "email",
/* 419 */ "tell",
/* 420 */ "clanset",
/* 421 */ "clansave",
/* 422 */ "areas",
/* 423 */ "transfer",
/* 424 */ "land",
/* 425 */ "fly",
/* 426 */ "light",
/* 427 */ "extinguish",
/* 428 */ "shave",
/* 429 */ "heal",
/* 430 */ "cat",
/* 431 */ "probe",
/* 432 */ "mindwipe",
/* 433 */ "compel",
/* 434 */ "raise",
/* 435 */ "lower",
/* 436 */ "control",
/* 437 */ "conceal",
/* 438 */ "dome",
/* 439 */ "clairaudience",
/* 440 */ "die",
/* 441 */ "inject",
/* 442 */ "brew",
/* 443 */ "count",
/* 444 */ "masquerade",
/* 445 */ "illusion",
/* 446 */ "comment",
/* 447 */ "monitor",
/* 448 */ "cgive",
/* 449 */ "crevoke",
/* 450 */ "pagelength",
/* 451 */ "roll",
/* 452 */ "doas",
/* 453 */ "view",
/* 454 */ "ainfo",
/* 455 */ "aset",
/* 456 */ "pfind",
/* 457 */ "afind",
/* 458 */ "resurrect",
/* 459 */ "sap",
/* 460 */ "store",
/* 461 */ "unstore",
    /* 462 */ "to_room",
    /* bogus command for specials */
    /* 463 */ "from_room",
    /* bogus command for specials */
/* 464 */ "scribble",
/* 465 */ "last",
/* 466 */ "wizlock",
/* 467 */ "tribes",
/* 468 */ "forage",
/* 469 */ "citystat",
/* 470 */ "cityset",
/* 471 */ "repair",
/* 472 */ "nesskick",
/* 473 */ "chardelet",
/* 474 */ "chardelete",
/* 475 */ "sense presence",
/* 476 */ "palm",
/* 477 */ "suggest",
/* 478 */ "babble",
/* 479 */ "disorient",
/* 480 */ "analyze",
/* 481 */ "clairvoyance",
/* 482 */ "imitate",
/* 483 */ "project",
/* 484 */ "clean",
/* 485 */ "mercy",
/* 486 */ "watch",
/* 487 */ "oindex",
/* 488 */ "magicksense",
/* 489 */ "mindblast",
/* 490 */ "load_trigger",
/* 491 */ "attempt_get_quiet",
/* 492 */ "attempt_drop",
/* 493 */ "attempt_drop_quiet",
/* 494 */ "systat",
/* 495 */ "on_damage",
/* 496 */ "charge",
/* 497 */ "rtwo",
/* 498 */ "on_equip",
/* 499 */ "on_remove",
/* 500 */ "send",
/* 501 */ "xpath",
/* 502 */ "salvage",
/* 503 */ "cast",
/* 504 */ "kiss",
/* 505 */ "hemote",
/* 506 */ "phemote",
/* 507 */ "semote",
/* 508 */ "psemote",
/* 509 */ "addkeywor",
/* 510 */ "addkeyword",
/* 511 */ "iooc",
/* 512 */ "arrange",
/* 513 */ "delayed_emote",
/* 514 */ "jseval",
/* 515 */ "snuff",
/* 516 */ "[",
/* 517 */ "empty",
/* 518 */ "tdesc",
/* 519 */ "warnnews",
/* 520 */ "trample",
/* 521 */ "discuss",
/* 522 */ "directions",
/* 523 */ "bury",
/* 524 */ "bldcom",
/* 525 */ "}",
    "\n"
};

 /* adding 1 so the skill_name list can be "\n" terminated, so can
  * use the old search block on it   -Tenebrius
  * 
  */
const char * const skill_name[MAX_SKILLS + 1] = {
/* 0  */ "none",
/* 1  */ "armor",
/* 2  */ "teleport",
/* 3  */ "blind",
/* 4  */ "create food",
/* 5  */ "create water",
/* 6  */ "cure blindness",
/* 7  */ "detect invisible",
/* 8  */ "detect magick",
/* 9  */ "detect poison",
/* 10  */ "earthquake",
/* 11  */ "fireball",
/* 12  */ "heal",
/* 13  */ "invisibility",
/* 14  */ "lightning bolt",
/* 15  */ "poison",
/* 16  */ "remove curse",
/* 17  */ "sanctuary",
/* 18  */ "sleep",
/* 19  */ "strength",
/* 20  */ "summon",
/* 21  */ "rain of fire",
/* 22  */ "cure poison",
/* 23  */ "relocate",
/* 24  */ "fear",
/* 25  */ "refresh",
/* 26  */ "levitate",
/* 27  */ "ball of light",
/* 28  */ "animate dead",
/* 29  */ "dispel magick",
/* 30  */ "calm",
/* 31  */ "infravision",
/* 32  */ "flamestrike",
/* 33  */ "sand jambiya",
/* 34  */ "stone skin",
/* 35  */ "weaken",
/* 36  */ "gate",
/* 37  */ "oasis",
/* 38  */ "send shadow",
/* 39  */ "show the path",
/* 40  */ "demonfire",
/* 41  */ "sandstorm",
/* 42  */ "hands of wind",
/* 43  */ "ethereal",
/* 44  */ "detect ethereal",
/* 45  */ "banishment",
/* 46  */ "burrow",
/* 47  */ "empower",
/* 48  */ "feeblemind",
/* 49  */ "fury",
/* 50  */ "guardian",
/* 51  */ "glyph",
/* 52  */ "transference",
/* 53  */ "invulnerability",
/* 54  */ "determine relationship",
/* 55  */ "mark",
/* 56  */ "mount",
/* 57  */ "pseudo death",
/* 58  */ "psionic suppression",
/* 59  */ "restful shade",
/* 60  */ "fly",
/* 61  */ "stalker",
/* 62  */ "thunder",
/* 63  */ "dragon drain",
/* 64  */ "wall of sand",
/* 65  */ "rewind",
/* 66  */ "pyrotechnics",
/* 67  */ "insomnia",
/* 68  */ "tongues",
/* 69  */ "charm person",
/* 70  */ "alarm",
/* 71  */ "feather fall",
/* 72  */ "shield of nilaz",
/* 73  */ "deafness",
/* 74  */ "silence",
/* 75  */ "solace",
/* 76  */ "slow",
/* 77  */ "contact",
/* 78  */ "barrier",
/* 79  */ "locate",
/* 80  */ "probe",
/* 81  */ "trace",
/* 82  */ "expel",
/* 83  */ "dragon bane",
/* 84  */ "repel",
/* 85  */ "health drain",
/* 86  */ "aura drain",
/* 87  */ "stamina drain",
/* 88  */ "peek",
/* 89  */ "night vision",
/* 90  */ "godspeed",
/* 91  */ "willpower",
/* 92  */ "cathexis",
/* 93  */ "empathy",
/* 94  */ "allspeak",
/* 95  */ "mindwipe",
/* 96  */ "shadowwalk",
/* 97  */ "hear",
/* 98  */ "control",
/* 99  */ "compel",
/* 100  */ "conceal",
/* 101  */ "sirihish",
/* 102  */ "bendune",
/* 103  */ "allundean",
/* 104  */ "mirukkim",
/* 105  */ "kentu",
/* 106  */ "nrizkt",
/* 107  */ "untal",
/* 108  */ "cavilish",
/* 109  */ "tatlum",
/* 110  */ "archery",
/* 111  */ "sneak",
/* 112  */ "hide",
/* 113  */ "steal",
/* 114  */ "backstab",
/* 115  */ "pick",
/* 116  */ "kick",
/* 117  */ "bash",
/* 118  */ "rescue",
/* 119  */ "disarm",
/* 120  */ "parry",
/* 121  */ "listen",
/* 122  */ "trap",
/* 123  */ "hunt",
/* 124  */ "haggle",
/* 125  */ "climb",
/* 126  */ "bandage",
/* 127  */ "dual wield",
/* 128  */ "throw",
/* 129  */ "subdue",
/* 130  */ "scan",
/* 131  */ "search",
/* 132  */ "value",
/* 133  */ "ride",
/* 134  */ "pilot",
/* 135  */ "poisoning",
/* 136  */ "guarding",
/* 137  */ "piercing weapons",
/* 138  */ "slashing weapons",
/* 139  */ "chopping weapons",
/* 140  */ "bludgeoning weapons",
/* 141  */ "spice methelinoc",
/* 142  */ "spice melem tuek",
/* 143  */ "spice krelez",
/* 144  */ "spice kemen",
/* 145  */ "spice aggross",
/* 146  */ "spice zharal",
/* 147  */ "spice thodeliv",
/* 148  */ "spice krentakh",
/* 149  */ "spice qel",
/* 150  */ "spice moss",
/* 151  */ "RW Sirihish",
/* 152  */ "RW Bendune",
/* 153  */ "RW Allundean",
/* 154  */ "RW Mirukkim",
/* 155  */ "RW Kentu",
/* 156  */ "RW Nrizkt",
/* 157  */ "RW Untal",
/* 158  */ "RW Cavilish",
/* 159  */ "RW Tatlum",
/* 160  */ "dome",
/* 161  */ "clairaudience",
/* 162  */ "masquerade",
/* 163  */ "illusion",
/* 164  */ "",
/* 165  */ "",
/* 166  */ "",
/* 167  */ "",
/* 168  */ "",
/* 169  */ "brew",
/* 170  */ "create darkness",
/* 171  */ "paralyze",
/* 172  */ "chain lightning",
/* 173  */ "lightning storm",
/* 174  */ "energy shield",
/* 175  */ "",
/* 176  */ "curse",
/* 177  */ "fluorescent footsteps",
/* 178  */ "regenerate",
/* 179  */ "firebreather",
/* 180  */ "parch",
/* 181  */ "travel gate",
/* 182  */ "cure disease_182",
/* 183  */ "forbid elements",
/* 184  */ "turn element",
/* 185  */ "portable hole",
/* 186  */ "elemental fog",
/* 187  */ "planeshift",
/* 188  */ "quickening",
/* 189  */ "",
/* 190  */ "",
/* 191  */ "",
/* 192  */ "",
/* 193  */ "",
/* 194  */ "",
/* 195  */ "",
/* 196  */ "",
/* 197  */ "",
/* 198  */ "",
/* 199  */ "",
/* 200  */ "alcohol",
/* 201  */ "tol generic",
/* 202  */ "tol grishen",
/* 203  */ "tol skellebain",
/* 204  */ "tol methelinoc",
/* 205  */ "tol terradin",
/* 206  */ "tol peraine",
/* 207  */ "tol heramide",
/* 208  */ "tol plague",
/* 209  */ "tol pain",
/* 210  */ "",
/* 211  */ "criminal",
/* 212  */ "combat", /* no quit */
/* 213  */ "shape_change", /* altered forms */
/* 214  */ "locked_input", /* is frozen */
/* 215  */ "",
/* 216  */ "",
/* 217  */ "",
/* 218  */ "",
/* 219  */ "mul rage", /* Bad mul!  Bad mul! */
/* 220  */ "general poison",
/* 221  */ "poison grishen",
/* 222  */ "poison skellebain",
/* 223  */ "poison methelinoc",
/* 224  */ "poison terradin",
/* 225  */ "poison peraine",
/* 226  */ "poison heramide",
/* 227  */ "black plague",
/* 228  */ "poison skellebain",
/* 229  */ "poison skellebain",
/* 230  */ "",
/* 231  */ "",
/* 232  */ "",
/* 233  */ "",
/* 234  */ "",
/* 235  */ "anyar",
/* 236  */ "RW Anyar",
/* 237  */ "heshrak",
/* 238  */ "RW Heshrak",
/* 239  */ "vloran",
/* 240  */ "RW Vloran",
/* 241  */ "domat",
/* 242  */ "RW Domat",
/* 243  */ "ghatti",
/* 244  */ "RW Ghatti",
/* 245  */ "shield use",
/* 246  */ "sap",
/* 247  */ "possess corpse",
/* 248  */ "portal",
/* 249  */ "sand shelter",
/* 250  */ "HIT_VS_HUMANOID",
/* 251  */ "BLUDGEON_VS_HUMANOID",
/* 252  */ "PIERCE_VS_HUMANOID",
/* 253  */ "STAB_VS_HUMANOID",
/* 254  */ "CHOP_VS_HUMANOID",
/* 255  */ "SLASH_VS_HUMANOID",
/* 256  */ "WHIP_VS_HUMANOID",
/* 257  */ "HIT_VS_MAMMALIAN",
/* 258  */ "BLUDGEON_VS_MAMMALIAN",
/* 259  */ "PIERCE_VS_MAMMALIAN",
/* 260  */ "STAB_VS_MAMMALIAN",
/* 261  */ "CHOP_VS_MAMMALIAN",
/* 262  */ "SLASH_VS_MAMMALIAN",
/* 263  */ "WHIP_VS_MAMMALIAN",
/* 264  */ "HIT_VS_INSECTINE",
/* 265  */ "BLUDGEON_VS_INSECTINE",
/* 266  */ "PIERCE_VS_INSECTINE",
/* 267  */ "STAB_VS_INSECTINE",
/* 268  */ "CHOP_VS_INSECTINE",
/* 269  */ "SLASH_VS_INSECTINE",
/* 270  */ "WHIP_VS_INSECTINE",
/* 271  */ "HIT_VS_FLIGHTLESS",
/* 272  */ "BLUDGEON_VS_FLIGHTLESS",
/* 273  */ "PIERCE_VS_FLIGHTLESS",
/* 274  */ "STAB_VS_FLIGHTLESS",
/* 275  */ "CHOP_VS_FLIGHTLESS",
/* 276  */ "SLASH_VS_FLIGHTLESS",
/* 277  */ "WHIP_VS_FLIGHTLESS",
/* 278  */ "HIT_VS_OPHIDIAN",
/* 279  */ "BLUDGEON_VS_OPHIDIAN",
/* 280  */ "PIERCE_VS_OPHIDIAN",
/* 281  */ "STAB_VS_OPHIDIAN",
/* 282  */ "CHOP_VS_OPHIDIAN",
/* 283  */ "SLASH_VS_OPHIDIAN",
/* 284  */ "WHIP_VS_OPHIDIAN",
/* 285  */ "HIT_VS_REPTILIAN",
/* 286  */ "BLUDGEON_VS_REPTILIAN",
/* 287  */ "PIERCE_VS_REPTILIAN",
/* 288  */ "STAB_VS_REPTILIAN",
/* 289  */ "CHOP_VS_REPTILIAN",
/* 290  */ "SLASH_VS_REPTILIAN",
/* 291  */ "WHIP_VS_REPTILIAN",
/* 292  */ "HIT_VS_PLANT",
/* 293  */ "BLUDGEON_VS_PLANT",
/* 294  */ "PIERCE_VS_PLANT",
/* 295  */ "STAB_VS_PLANT",
/* 296  */ "CHOP_VS_PLANT",
/* 297  */ "SLASH_VS_PLANT",
/* 298  */ "WHIP_VS_PLANT",
/* 299  */ "HIT_VS_ARACHNID",
/* 300  */ "BLUDGEON_VS_ARACHNID",
/* 301  */ "PIERCE_VS_ARACHNID",
/* 302  */ "STAB_VS_ARACHNID",
/* 303  */ "CHOP_VS_ARACHNID",
/* 304  */ "SLASH_VS_ARACHNID",
/* 305  */ "WHIP_VS_ARACHNID",
/* 306  */ "HIT_VS_AVIAN_FLYING",
/* 307  */ "BLUDGEON_VS_AVIAN_FLYING",
/* 308  */ "PIERCE_VS_AVIAN_FLYING",
/* 309  */ "STAB_VS_AVIAN_FLYING",
/* 310  */ "CHOP_VS_AVIAN_FLYING",
/* 311  */ "SLASH_VS_AVIAN_FLYING",
/* 312  */ "WHIP_VS_AVIAN_FLYING",
/* 313 */ "skinning",
/* 314 */ "basketweaving",
/* 315 */ "tanning",
/* 316 */ "forage",
/* 317 */ "nesskick",
/* 318 */ "flee",
/* 319 */ "golem",
/* 320 */ "fire jambiya",
/* 321 */ "fletchery",
/* 322 */ "dyeing",
/* 323 */ "pick making",
/* 324 */ "lumberjacking",
/* 325 */ "feather working",
/* 326 */ "leather working",
/* 327 */ "rope making",
/* 328 */ "bow making",
/* 329 */ "instrument making",
/* 330 */ "bandagemaking",
/* 331 */ "woodworking",
/* 332 */ "armor making",
/* 333 */ "knife making",
/* 334 */ "component crafting",
/* 335 */ "floristry",
/* 336 */ "cooking",
/* 337 */ "identify",
/* 338 */ "wall of fire",
/* 339 */ "stonecrafting",
/* 340 */ "jewelrymaking",
/* 341 */ "tentmaking",
/* 342 */ "wagonmaking",
/* 343 */ "swordmaking",
/* 344 */ "spearmaking",
/* 345 */ "sleight of hand",
/* 346 */ "clothworking",
/* 347 */ "wall of wind",
/* 348 */ "blade barrier",
/* 349 */ "wall of thorns",
/* 350 */ "wind armor",
/* 351 */ "fire armor",
/* 352 */ "armor repair",
/* 353 */ "weapon repair",
/* 354 */ "sense presence",
/* 355 */ "thoughtsense",
/* 356 */ "suggest",
/* 357 */ "babble",
/* 358 */ "disorient",
/* 359 */ "clairvoyance",
/* 360 */ "imitate",
/* 361 */ "project",
/* 362 */ "vanish",
/* 363 */ "beast affinity",
/* 364 */ "coerce",
/* 365 */ "wild contact",
/* 366 */ "wild barrier",
/* 367 */ "analyze",
/* 368 */ "alchemy",
/* 369 */ "toolmaking",
/* 370 */ "haunt",
/* 371 */ "wagon repair",
/* 372 */ "create wine",
/* 373 */ "wind fist",
/* 374 */ "lightning whip",
/* 375 */ "shadow sword",
/* 376 */ "psionic drain",
/* 377 */ "intoxication",
/* 378 */ "sober",
/* 379 */ "wither",
/* 380 */ "delusion",
/* 381 */ "illuminant",
/* 382 */ "mirage",
/* 383 */ "phantasm",
/* 384 */ "shadowplay",
/* 385 */ "scent_doodie",
/* 386 */ "scent_roses",
/* 387 */ "scent_musk",
/* 388 */ "scent_citron",
/* 389 */ "scent_spice",
/* 390 */ "scent_shrintal",
/* 391 */ "scent_lanturin",
/* 392 */ "scent_glimmergrass",
/* 393 */ "scent_belgoikiss",
/* 394 */ "scent_lavender",
/* 395 */ "scent_jasmine",
/* 396 */ "scent_clove",
/* 397 */ "scent_mint",
/* 398 */ "scent_pymlithe",
/* 399 */ "drawing",
/* 400 */ "rinthi",
/* 401 */ "none",
/* 402 */ "NorthAccent",
/* 403 */ "",
/* 404 */ "SouthAccent",
/* 405 */ "",
/* 406 */ "pike weapons",
/* 407 */ "trident weapons",
/* 408 */ "polearm weapons",
/* 409 */ "knife weapons",
/* 410 */ "razor weapons",
/* 411 */ "sand statue",
/* 412 */ "shrinking heads",
/* 413 */ "magicksense",
/* 414 */ "mindblast",
/* 415 */ "charge",
/* 416 */ "split",
/* 417 */ "two handed",
/* 418 */ "AnyarAccent",
/* 419 */ "",
/* 420 */ "BenduneAccent",
/* 421 */ "",
/* 422 */ "disembody",
/* 423 */ "axe making",
/* 424 */ "club making",
/* 425 */ "weaving",
/* 426 */ "shield of wind",
/* 427 */ "shield of mist",
/* 428 */ "drown",
/* 429 */ "healing mud",
/* 430 */ "cause disease",
/* 431 */ "cure disease",
/* 432 */ "acid spray",
/* 433 */ "puddle",
/* 434 */ "shadow armor",
/* 435 */ "fire seed",
/* 436 */ "WaterAccent",
/* 437 */ "FireAccent",
/* 438 */ "WindAccent",
/* 439 */ "EarthAccent",
/* 440 */ "ShadowAccent",
/* 441 */ "LightningAccent",
/* 442 */ "NilazAccent",
/* 443 */ "breathe water",
/* 444 */ "lightning spear",
/* 445 */ "direction sense",
/* 446 */ "mesmerize",
/* 447 */ "shatter",
/* 448 */ "messenger",
/* 449 */ "gather",
/* 450 */ "rejuvenate",
/* 451 */ "blind fighting",
/* 452 */ "daylight",
/* 453 */ "dispel invisibility",
/* 454 */ "dispel ethereal",
/* 455 */ "repair item",
/* 456 */ "create rune",
/* 457 */ "watch",
/* 458 */ "immolate",
/* 459 */ "reach un",
/* 460 */ "reach nil",
/* 461 */ "reach oai",
/* 462 */ "reach tuk",
/* 463 */ "reach phan",
/* 464 */ "reach mur",
/* 465 */ "reach kizn",
/* 466 */ "reach xaou",
/* 467 */ "rot items",
/* 468 */ "vampiric blade",
/* 469 */ "dead speak",
/* 470 */ "disease boils",
/* 471 */ "disease chest decay",
/* 472 */ "disease cilops kiss",
/* 473 */ "disease gangrene",
/* 474 */ "disease heat splotch",
/* 475 */ "disease ivory salt sickness",
/* 476 */ "disease kank fleas",
/* 477 */ "disease kraths touch",
/* 478 */ "disease maar pox",
/* 479 */ "disease pepperbelly",
/* 480 */ "disease raza raza",
/* 481 */ "disease sand fever",
/* 482 */ "disease sand fleas",
/* 483 */ "disease sand rot",
/* 484 */ "disease scrub fever",
/* 485 */ "disease slough skin",
/* 486 */ "disease yellow plague",
/* 487 */ "immersion",
/* 488 */ "hero sword",
/* 489 */ "recite",
/* 490 */ "scent eynana",
/* 491 */ "scent khee",
/* 492 */ "scent filanya",
/* 493 */ "scent ladysmantle",
/* 494 */ "scent lirathufavor",
/* 495 */ "scent pfafna",
/* 496 */ "scent redheart",
/* 497 */ "scent sweetbreeze",
/* 498 */ "scent tembotooth",
/* 499 */ "scent tholinoc",
/* 500 */ "trample",
/* 501 */ "DesertAccent",
/* 502 */ "EasternAccent",
/* 503 */ "WesternAccent",
/* 504 */ "scent_medichi",
/* 505 */ "clayworking",
    "\n"
};

int skill_weap_rtype[9 /* race type */ ][8 /* Weapon# */ ] =
{
    {
     HIT_VS_HUMANOID,
     BLUDGEON_VS_HUMANOID,
     PIERCE_VS_HUMANOID,
     STAB_VS_HUMANOID,
     CHOP_VS_HUMANOID,
     SLASH_VS_HUMANOID,
     HIT_VS_HUMANOID,
     WHIP_VS_HUMANOID},
    {
     HIT_VS_MAMMALIAN,
     BLUDGEON_VS_MAMMALIAN,
     PIERCE_VS_MAMMALIAN,
     STAB_VS_MAMMALIAN,
     CHOP_VS_MAMMALIAN,
     SLASH_VS_MAMMALIAN,
     HIT_VS_HUMANOID,
     WHIP_VS_MAMMALIAN},
    {
     HIT_VS_INSECTINE,
     BLUDGEON_VS_INSECTINE,
     PIERCE_VS_INSECTINE,
     STAB_VS_INSECTINE,
     CHOP_VS_INSECTINE,
     SLASH_VS_INSECTINE,
     HIT_VS_HUMANOID,
     WHIP_VS_INSECTINE},
    {
     HIT_VS_FLIGHTLESS,
     BLUDGEON_VS_FLIGHTLESS,
     PIERCE_VS_FLIGHTLESS,
     STAB_VS_FLIGHTLESS,
     CHOP_VS_FLIGHTLESS,
     SLASH_VS_FLIGHTLESS,
     HIT_VS_HUMANOID,
     WHIP_VS_FLIGHTLESS},
    {
     HIT_VS_OPHIDIAN,
     BLUDGEON_VS_OPHIDIAN,
     PIERCE_VS_OPHIDIAN,
     STAB_VS_OPHIDIAN,
     CHOP_VS_OPHIDIAN,
     SLASH_VS_OPHIDIAN,
     HIT_VS_HUMANOID,
     WHIP_VS_OPHIDIAN},
    {
     HIT_VS_REPTILIAN,
     BLUDGEON_VS_REPTILIAN,
     PIERCE_VS_REPTILIAN,
     STAB_VS_REPTILIAN,
     CHOP_VS_REPTILIAN,
     SLASH_VS_REPTILIAN,
     HIT_VS_HUMANOID,
     WHIP_VS_REPTILIAN},
    {
     HIT_VS_PLANT,
     BLUDGEON_VS_PLANT,
     PIERCE_VS_PLANT,
     STAB_VS_PLANT,
     CHOP_VS_PLANT,
     SLASH_VS_PLANT,
     HIT_VS_HUMANOID,
     WHIP_VS_PLANT},
    {
     HIT_VS_ARACHNID,
     BLUDGEON_VS_ARACHNID,
     PIERCE_VS_ARACHNID,
     STAB_VS_ARACHNID,
     CHOP_VS_ARACHNID,
     SLASH_VS_ARACHNID,
     HIT_VS_HUMANOID,
     WHIP_VS_ARACHNID},
    {
     HIT_VS_AVIAN_FLYING,
     BLUDGEON_VS_AVIAN_FLYING,
     PIERCE_VS_AVIAN_FLYING,
     STAB_VS_AVIAN_FLYING,
     CHOP_VS_AVIAN_FLYING,
     SLASH_VS_AVIAN_FLYING,
     HIT_VS_HUMANOID,
     WHIP_VS_AVIAN_FLYING},
};

const char * const spell_wear_off_msg[MAX_SKILLS] = {
    "",                         /* spell_none */
    "The protection of Ruk fades from your body.",
    "",                         /* teleport */
    "The cloak of blindness disappears from your eyes.",
    "",                         /* spell_create_food */
    "",                         /* spell_create_water */
    "",                         /* spell_cure_blindness */
    "The tingle in your eyes disappears.",
    "The glow of Suk-Krath fades from your eyes.",
    "",                         /* spell_detect_poison */
    "",                         /* spell_earthquake */
    "",                         /* spell_fireball */
    "",                         /* spell_heal */
    "Your body fades back into existence.",
    "",                         /* spell_lightning_bolt */
    "The burning poison in your veins disappears.",
    "",                         /* spell_remove_curse */
    "The glowing aura around your body fades away.",
    "Energy seeps back into your limbs, leaving you less weary.",
    "Your strength ebbs, leaving you weak and dizzy.",
    "",                         /* spell_summon */
    "",                         /* spell_ventriloquate */
    "",                         /* spell_cure_poison */
    "",                         /* spell_relocate */
    "Slowly, your terror begins to ebb away.",  /* SPELL_FEAR */
    "",                         /* spell_refresh */
    "You descend to the ground as Whira's hands release you.",
    "",                         /* spell_light */
    "",                         /* spell_animate_dead */
    "",                         /* spell_dispel_magick */
    "Your serenity vanishes, and the world seems less calm.",
    "The shadowy tinge in your eyes disappears.",
    "",                         /* spell_flamestrike */
    "Your eyes' silvery hue suddenly fades away.",
    "Your stone-hard skin becomes as soft as flesh again.",
    "Strength returns to your weak limbs.",
    "",                         /* spell_gate */
    "",                         /* spell_oasis */
    "",                         /* spell_send_shadow */
    "",                         /* spell_show_the_path */
    "",                         /* spell_demonfire */
    "",                         /* spell_sandstorm */
    "",                         /* spell_hands_of_wind */
    "Your form drifts back out of Drov's plane.",
    "Your eyes' shadowy hue suddenly fades away.",
    "",                         /* spell_banishment */
    "",                         /* spell_burrow */
    "",                         /* spell_empower */
    "Your mind begins to clear.",
    "The fighting rage leaves you and your head clears.",
    "",                         /* spell_guardian */
    "",                         /* spell_glyph */
    "",                         /* spell_transference */
    "The cream-colored shell around your body collapses to the ground.",
    "",                         /* spell_knock */
    "",                         /* spell_mark */
    "",                         /* spell_mount */
    "",                         /* spell_pseudo_death */
    "You feel more psionically potent.",
    "",                         /* spell_restful_shade */
    "You descend to the ground as Whira's power fades from your body.",
    "SPELL_STALKER",
    "SPELL_THUNDER",
    "SPELL_WEB",
    "SPELL_WALL_OF_SAND",
    "SPELL_UNDEATH",
    "SPELL_PYROTECHNICS",
    "You don't feel so energetic anymore.",
    "Your tongue no longer tingles.",
    "You no longer feel bound to follow your master.",
    "SPELL_ALARM",
    "You stop drifting slowly, all of a sudden.",
    "You feel the translucent bubble about you crumble.",       /* spell_shield_of */
    "Sounds suddenly rush into your ears as you recover your hearing.",
    "Your throat unwinds, allowing you to speak again.",        /* spell_silence */
    "SPELL_SOLACE",
    "You suddenly feel a burst of energy, and speed up again.",
    "Your psionic connection slowly fades away.",
    "Your mental barrier slowly fades away.",
    "SPELL_TRANSMUTE_ROCK_MUD",
    "SPELL_TRANSMUTE_WATER_DUST",
    "SPELL_TRANSMUTE_METAL_DUST",
    "SPELL_TRASMUTE_WOOD_MUD",
    "SPELL_DRAGON_BANE",
    "SPELL_REPEL",
    "SPELL_HEALTH_DRAIN",
    "SPELL_AURA_DRAIN",
    "SPELL_STAMINA_DRAIN",
    "SPELL_DUST_DEVIL",
    "SPELL_BLADE_BARRIER",
    "Your feet slow down again.",       /* godspeed */
    "",
    "",
    "",
    "Your mental awareness returns to normal.", /* allspeak */
    "You feel as if a weight has been lifted off your mind.",   /* mindwipe */
    "",
    "A soothing silence fills your head as you stop concentrating on listening.",       /* hear */
    "",                         /* control */
    "",                         /* compel */
    "Your concentration is broken and you cannot maintain your concealment.",   /* conceal */
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "SKILL_STEAL",
    "SKILL_BACKSTAB",
    "SKILL_PICK_LOCK",
    "SKILL_KICK",
    "SKILL_BASH",
    "SKILL_RESCUE",
    "SKILL_DISARM",
    "",
    "",
    "SKILL_TRAP",
    "SKILL_HUNT",
    "SKILL_HAGGLE",
    "",
    "SKILL_BANDAGE",
    "SKILL_DUAL_WIELD",
    "SKILL_THROW",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "SKILL_GUARD",
    "",
    "",
    "",
    "",
    "A dull ache pounds into the back of your head.\n", /* methelinoc */
    "Your stomach roils with nerves.\n",        /* melem tuek */
    "A heavy pain courses through your limbs, sapping your " "energy.\n",       /* krelez */
    "An unsettling weakness settles into your limbs.\n",        /* kemen */
    "A general nausea sets into your body as your heart skips a few " "beats.\n",       /* aggross */
    "You sense a menacing shadow over your shoulder and desperately " "want to hide.\n",        /* zharal */
    "Stinging nettles roam the entire surface of your skin, as " "if your entire body has lost circulation.\n", /* thodeliv */
    "A splitting pain stabs through your eyes.\n",      /* krentakh */
    "Your balance with nature returns to its normal state.\n",  /* qel */
    "You momentarily lose your balance as the blood rushes to your " "head.\n", /* moss */
/* 151  */ "",
/* 152  */ "",
/* 153  */ "",
/* 154  */ "",
/* 155  */ "",
/* 156  */ "",
/* 157  */ "",
/* 158  */ "",
/* 159  */ "",
    /* 160  */ "Your mental shroud around the area subsides.",
    /* dome */
    /* 161  */ "You lose your focus and stop listening.",
    /* clairaudience */
/* 162  */ "",
/* 163  */ "",
/* 164  */ "",
/* 165  */ "",
/* 166  */ "",
/* 167  */ "",
/* 168  */ "",
/* 169  */ "SKILL_BREW",
    /* 170  */ "SPELL_CREATE_DARKNESS",
    /* spell create darkness */
/* 171  */ "Movement returns to your limbs.",
/* 172  */ "SPELL_CHAIN_LIGHTNING",
/* 173  */ "SPELL_LIGHTNING_STORM",
/* 174  */ "The energy shield around your body fades away.",
/* 175  */ "SPELL_FLASH",
/* 176  */ "You don't feel quite as clumsy.",
/* 177  */ "Your feet cease to leave glowing footprints.",
/* 178  */ "You feel your body's natural functions slow back to normal.",
/* 179  */ "Your body cools down to its normal level.",
/* 180  */ "SPELL_PARCH",
/* 181  */ "SPELL_TRAVEL_GATE",
/* 182  */ "",
/* 183  */ "SPELL_FORBID_ELEMENTS",
/* 184  */ "The greasy feeling on your skin slowly fades away.",
/* 185  */ "SPELL_PORTABLE_HOLE",
/* 186  */ "The fog around your body dissipates.",
/* 187  */ "You are drawn back to the mortal plane.",
/* 188  */ "Your body seems to slow back down to normal.",
/* 189  */ "",
/* 190  */ "",
/* 191  */ "",
/* 192  */ "",
/* 193  */ "",
/* 194  */ "",
/* 195  */ "",
/* 196  */ "",
/* 197  */ "",
/* 198  */ "",
/* 199  */ "",
/* 200  */ "",
/* 201  */ "",
/* 202  */ "",
/* 203  */ "",
/* 204  */ "",
/* 205  */ "",
/* 206  */ "",
/* 207  */ "",
/* 208  */ "",
/* 209  */ "",
/* 210  */ "",
/* 211  */ "",
    /* 212  */ "",
    /*  noquit  */
    /* 213  */ "",
    /* altered forms */
/* 214  */ "",
/* 215  */ "",
/* 216  */ "",
/* 217  */ "",
/* 218  */ "",
/* 219  */ "Your fury seems to subside, and your blood-lust clears.",
/* 220  */ "The burning poison in your veins disappears.",
/* 221  */ "Your sweating ceases as the poison exits your veins.",
/* 222  */ "Your head stops spinning, and your senses return to normal.",
/* 223  */ "The coldness in your veins cease as the poison leaves your"
        " system",
/* 224  */ "The queasiness in your stomach has stopped.",
/* 225  */ "Your muscles relax allowing you to move again.",
/* 226  */ "The numbness inside your head seems to recede.",
/* 227  */ "The swelling in your head subsides as the fever dissipates.",
/* 228  */ "Your head stops spinning, and your senses return to normal.",
/* 229  */ "Your head stops spinning, and your senses return to normal.",
/* 230  */ "",
/* 231  */ "",
/* 232  */ "",
/* 233  */ "",
/* 234  */ "",
/* 235  */ "",
/* 236  */ "",
/* 237  */ "",
/* 238  */ "",
/* 239  */ "",
/* 240  */ "",
/* 241  */ "",
/* 242  */ "",
/* 243  */ "",
/* 244  */ "",
/* 245  */ "",
/* 246  */ "",
    /* 247  */ "",
    /* spell possess_corpse */
    /* 248  */ "",
    /* spell portal */
/* 249  */ "",
/* 250  */ "",
/* 251  */ "",
/* 252  */ "",
/* 253  */ "",
/* 254  */ "",
/* 255  */ "",
/* 256  */ "",
/* 257  */ "",
/* 258  */ "",
/* 259  */ "",
/* 260  */ "",
/* 261  */ "",
/* 262  */ "",
/* 263  */ "",
/* 264  */ "",
/* 265  */ "",
/* 266  */ "",
/* 267  */ "",
/* 268  */ "",
/* 269  */ "",
/* 270  */ "",
/* 271  */ "",
/* 272  */ "",
/* 273  */ "",
/* 274  */ "",
/* 275  */ "",
/* 276  */ "",
/* 277  */ "",
/* 278  */ "",
/* 279  */ "",
/* 280  */ "",
/* 281  */ "",
/* 282  */ "",
/* 283  */ "",
/* 284  */ "",
/* 285  */ "",
/* 286  */ "",
/* 287  */ "",
/* 288  */ "",
/* 289  */ "",
/* 290  */ "",
/* 291  */ "",
/* 292  */ "",
/* 293  */ "",
/* 294  */ "",
/* 295  */ "",
/* 296  */ "",
/* 297  */ "",
/* 298  */ "",
/* 299  */ "",
/* 300  */ "",
/* 301  */ "",
/* 302  */ "",
/* 303  */ "",
/* 304  */ "",
/* 305  */ "",
/* 306  */ "",
/* 307  */ "",
/* 308  */ "",
/* 309  */ "",
/* 310  */ "",
/* 311  */ "",
    /* 312  */ "",
    /*  noquit  */
    /* 313  */ "",
    /* altered forms */
/* 314  */ "",
/* 315  */ "",
/* 316  */ "SKILL_FORAGE",
/* 317  */ "SKILL_NESSKICK",
/* 318  */ "",
/* 319  */ "",
/* 320  */ "",
/* 321  */ "",
/* 322  */ "",
/* 323  */ "",
/* 324  */ "",
/* 325  */ "",
/* 326  */ "",
/* 327  */ "",
/* 328  */ "",
/* 329  */ "",
/* 330  */ "",
/* 331  */ "",
/* 332  */ "",
/* 333  */ "",
/* 334  */ "",
/* 335  */ "",
/* 336  */ "",
/* 337  */ "Your awareness with the elements returns to normal.",
/* 338 */ "",
/* 339 */ "",
/* 340 */ "",
/* 341 */ "",
/* 342 */ "",
/* 343 */ "",
/* 344 */ "",
/* 345 */ "",
/* 346 */ "",
/* 347 */ "",
/* 348 */ "",
/* 349 */ "",
/* 350 */ "The winds surrounding you dissipate slowly.",
/* 351 */ "The fires surrounding you suddenly disperse into nothing.",
/* 352 */ "",
/* 353 */ "",
    /* 354 */ "You feel less aware of your surroundings.",
    /* sense presence */
/* 355 */ "",
/* 356 */ "",
    /* 357 */ "Your tongue feels less swollen and stops tingling.",
    /* babble */
    /* 358 */ "You feel less clumsy.",
    /* disorient */
    /* 359  */ "You lose your focus and stop your enchanced vision.",
    /* clairaudience */
    /* 360 */ "You are unable to sustain your mental image.",
    /* imitate */
/* 361 */ "",
    /* 362 */ "You are unable to mask your image from other's perception.",
    /* vanish */
/* 363 */ "",
    /* 364 */ "You cannot continue to coerce the thoughts of another.",
    /* coercion */
/* 365 */ "",
/* 366 */ "",
/* 367 */ "",
/* 368 */ "",
/* 369 */ "",
/* 370 */ "",
/* 371 */ "",
/* 372 */ "",
/* 373 */ "",
/* 374 */ "",
/* 375 */ "",
/* 376 */ "",
/* 377 */ "",
/* 378 */ "",
/* 379 */ "",
/* 380 */ "",
/* 381 */ "",
/* 382 */ "",
/* 383 */ "",
/* 384 */ "",
/* 385 */ "",
/* 386 */ "",
/* 387 */ "",
/* 388 */ "",
/* 389 */ "",
/* 390 */ "",
/* 391 */ "",
/* 392 */ "",
/* 393 */ "",
/* 394 */ "",
/* 395 */ "",
/* 396 */ "",
/* 397 */ "",
/* 398 */ "",
/* 399 */ "",
/* 400 */ "",
/* 401 */ "",
/* 402 */ "",
/* 403 */ "",
/* 404 */ "",
/* 405 */ "",
/* 406 */ "",
/* 407 */ "",
/* 408 */ "",
/* 409 */ "",
/* 410 */ "",
/* 411 */ "",
/* 412 */ "",
    /* 413 */ "Your magickal awareness returns to normal.",
    /* magicksense */
/* 414 */ "",
/* 415 */ "",
/* 416 */ "",
/* 417 */ "",
/* 418 */ "",
/* 419 */ "",
/* 420 */ "",
/* 421 */ "",
/* 422 */ "You return to your body.",
/* 423 */ "",
/* 424 */ "",
/* 425 */ "",
/* 426 */ "",
/* 427 */ "",
/* 428 */ "",
/* 429 */ "",
/* 430 */ "",
/* 431 */ "",
/* 432 */ "",
/* 433 */ "",
/* 434 */ "Your shadow armor fades away.",
/* 435 */ "",
/* 436 */ "",
/* 437 */ "",
/* 438 */ "",
/* 439 */ "",
/* 440 */ "",
/* 441 */ "",
/* 442 */ "",
/* 443 */ "You again require air to breathe.",
/* 444 */ "",
/* 445 */ "",
/* 446 */ "",
/* 447 */ "",
/* 448 */ "",
/* 449 */ "",
/* 450 */ "",
/* 451 */ "",
/* 452 */ "",
/* 453 */ "",
/* 454 */ "",
/* 455 */ "",
/* 456 */ "",
/* 457 */ "",
/* 458 */ "The ravaging fires burning you suddenly die out with a wisp of smoke.",
/* 459 */ "",
/* 460 */ "",
/* 461 */ "",
/* 462 */ "",
/* 463 */ "",
/* 464 */ "",
/* 465 */ "",
/* 466 */ "",
/* 467 */ "",
/* 468 */ "",
/* 469 */ "",
/* 470 */ "",
/* 471 */ "",
/* 472 */ "",
/* 473 */ "",
/* 474 */ "",
/* 475 */ "",
/* 476 */ "",
/* 477 */ "Your head clears and you feel much better.",
/* 478 */ "",
/* 479 */ "",
/* 480 */ "",
/* 481 */ "",
/* 482 */ "",
/* 483 */ "",
/* 484 */ "Your head clears and you feel much better.",
/* 485 */ "",
/* 486 */ "",
/* 487 */ "",
/* 488 */ "",
/* 489 */ "You are no longer compelled to recite.",
};

const char * const spell_wear_off_room_msg[MAX_SKILLS] = {
    "",                         /* spell_none */
    "The layer of sand armoring $n falls away.",        /* armor */
    "",                         /* teleport */
    "",
    "",                         /* spell_create_food */
    "",                         /* spell_create_water */
    "",                         /* spell_cure_blindness */
    "",
    "",
    "",                         /* spell_detect_poison */
    "",                         /* spell_earthquake */
    "",                         /* spell_fireball */
    "",                         /* spell_heal */
    "$n fades into existence.",
    "",                         /* spell_lightning_bolt */
    "",
    "",                         /* spell_remove_curse */
    "The glowing aura around $n's body fades away.",
    "",
    "",
    "",                         /* spell_summon */
    "",                         /* spell_ventriloquate */
    "",                         /* spell_cure_poison */
    "",                         /* spell_relocate */
    "",                         /* SPELL_FEAR */
    "",                         /* spell_refresh */
    "$n slowly descends to the ground.",
    "",                         /* spell_light */
    "",                         /* spell_animate_dead */
    "",                         /* spell_dispel_magick */
    "",
    "",
    "",                         /* spell_flamestrike */
    "",
    "$n's stone-hard skin becomes as soft as flesh again.",
    "",                         /* spell_weaken */
    "",                         /* spell_gate */
    "",                         /* spell_oasis */
    "",                         /* spell_send_shadow */
    "",                         /* spell_show_the_path */
    "",                         /* spell_demonfire */
    "",                         /* spell_sandstorm */
    "",                         /* spell_hands_of_wind */
    "$n's form slowly coalesces from the shadows.",
    "",
    "",                         /* spell_banishment */
    "",                         /* spell_burrow */
    "",                         /* spell_empower */
    "",
    "",
    "",                         /* spell_guardian */
    "",                         /* spell_glyph */
    "",                         /* spell_transference */
    "The cream-colored shell around $n's body collapses to the ground.",
    "",                         /* spell_knock */
    "",                         /* spell_mark */
    "",                         /* spell_mount */
    "",                         /* spell_pseudo_death */
    "",
    "",                         /* spell_restful_shade */
    "Slowly, $n drifts to the ground.",
    "SPELL_STALKER",
    "SPELL_THUNDER",
    "SPELL_WEB",
    "SPELL_WALL_OF_SAND",
    "SPELL_UNDEATH",
    "SPELL_PYROTECHNICS",
    "",
    "",
    "",
    "SPELL_ALARM",
    "$n stops drifting slowly, all of a sudden.",
    "The translucent bubble about $n crumbles.",        /* spell_shield_of */
    "",
    "",                         /* spell_silence */
    "SPELL_SOLACE",
    "",
    "",
    "",
    "SPELL_TRANSMUTE_ROCK_MUD",
    "SPELL_TRANSMUTE_WATER_DUST",
    "SPELL_TRANSMUTE_METAL_DUST",
    "SPELL_TRASMUTE_WOOD_MUD",
    "SPELL_DRAGON_BANE",
    "SPELL_REPEL",
    "SPELL_HEALTH_DRAIN",
    "SPELL_AURA_DRAIN",
    "SPELL_STAMINA_DRAIN",
    "SPELL_DUST_DEVIL",
    "SPELL_BLADE_BARRIER",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "SKILL_WILLPOWER",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "SKILL_STEAL",
    "SKILL_BACKSTAB",
    "SKILL_PICK_LOCK",
    "SKILL_KICK",
    "SKILL_BASH",
    "SKILL_RESCUE",
    "SKILL_DISARM",
    "",
    "",
    "SKILL_TRAP",
    "SKILL_HUNT",
    "SKILL_HAGGLE",
    "",
    "SKILL_BANDAGE",
    "SKILL_DUAL_WIELD",
    "SKILL_THROW",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "SKILL_GUARD",
    "",
    "",
    "",
    "",
    "",
    "",
    "",                         /* krelez */
    "",
    "",
    "",                         /* aggross */
    "",
    "",
    "",                         /* moss */
/* 151  */ "",
/* 152  */ "",
/* 153  */ "",
/* 154  */ "",
/* 155  */ "",
/* 156  */ "",
/* 157  */ "",
/* 158  */ "",
/* 159  */ "",
/* 160  */ "",
/* 161  */ "",
/* 162  */ "",
/* 163  */ "",
/* 164  */ "",
/* 165  */ "",
/* 166  */ "",
/* 167  */ "",
/* 168  */ "",
/* 169  */ "SKILL_BREW",
    /* 170  */ "SPELL_CREATE_DARKNESS",
    /* spell create darkness */
/* 171  */ "$n's shakes and shivers briefly.",
/* 172  */ "SPELL_CHAIN_LIGHTNING",
/* 173  */ "SPELL_LIGHTNING_STORM",
    /* 174  */ "",
    /* SPELL_ENERGY_SHIELD */
/* 175  */ "SPELL_FLASH",
/* 176  */ "",
/* 177  */ "",
/* 178  */ "",
/* 179  */ "",
/* 180  */ "SPELL_PARCH",
/* 181  */ "SPELL_TRAVEL_GATE",
/* 182  */ "",
/* 183  */ "SPELL_FORBID_ELEMENTS",
/* 184  */ "",
/* 185  */ "SPELL_PORTABLE_HOLE",
/* 186  */ "The fog around $n's body dissipates.",
/* 187  */ "SPELL_PLANESHIFT",
/* 188  */ "",
/* 189  */ "",
/* 190  */ "",
/* 191  */ "",
/* 192  */ "",
/* 193  */ "",
/* 194  */ "",
/* 195  */ "",
/* 196  */ "",
/* 197  */ "",
/* 198  */ "",
/* 199  */ "",
/* 200  */ "",
/* 201  */ "",
/* 202  */ "",
/* 203  */ "",
/* 204  */ "",
/* 205  */ "",
/* 206  */ "",
/* 207  */ "",
/* 208  */ "",
/* 209  */ "",
/* 210  */ "",
/* 211  */ "",
    /* 212  */ "",
    /*  noquit  */
    /* 213  */ "",
    /* altered forms */
/* 214  */ "",
/* 215  */ "",
/* 216  */ "",
/* 217  */ "",
/* 218  */ "",
/* 219  */ "",
/* 220  */ "",
/* 221  */ "",
/* 222  */ "",
/* 223  */ "",
    " system",
/* 224  */ "",
/* 225  */ "$n slumps slightly.",
/* 226  */ "",
/* 227  */ "",
/* 228  */ "",
/* 229  */ "",
/* 230  */ "",
/* 231  */ "",
/* 232  */ "",
/* 233  */ "",
/* 234  */ "",
/* 235  */ "",
/* 236  */ "",
/* 237  */ "",
/* 238  */ "",
/* 239  */ "",
/* 240  */ "",
/* 241  */ "",
/* 242  */ "",
/* 243  */ "",
/* 244  */ "",
/* 245  */ "",
/* 246  */ "",
    /* 247  */ "",
    /* spell possess_corpse */
    /* 248  */ "",
    /* spell portal */
/* 249  */ "",
/* 250  */ "",
/* 251  */ "",
/* 252  */ "",
/* 253  */ "",
/* 254  */ "",
/* 255  */ "",
/* 256  */ "",
/* 257  */ "",
/* 258  */ "",
/* 259  */ "",
/* 260  */ "",
/* 261  */ "",
/* 262  */ "",
/* 263  */ "",
/* 264  */ "",
/* 265  */ "",
/* 266  */ "",
/* 267  */ "",
/* 268  */ "",
/* 269  */ "",
/* 270  */ "",
/* 271  */ "",
/* 272  */ "",
/* 273  */ "",
/* 274  */ "",
/* 275  */ "",
/* 276  */ "",
/* 277  */ "",
/* 278  */ "",
/* 279  */ "",
/* 280  */ "",
/* 281  */ "",
/* 282  */ "",
/* 283  */ "",
/* 284  */ "",
/* 285  */ "",
/* 286  */ "",
/* 287  */ "",
/* 288  */ "",
/* 289  */ "",
/* 290  */ "",
/* 291  */ "",
/* 292  */ "",
/* 293  */ "",
/* 294  */ "",
/* 295  */ "",
/* 296  */ "",
/* 297  */ "",
/* 298  */ "",
/* 299  */ "",
/* 300  */ "",
/* 301  */ "",
/* 302  */ "",
/* 303  */ "",
/* 304  */ "",
/* 305  */ "",
/* 306  */ "",
/* 307  */ "",
/* 308  */ "",
/* 309  */ "",
/* 310  */ "",
/* 311  */ "",
    /* 312  */ "",
    /*  noquit  */
    /* 313  */ "",
    /* altered forms */
/* 314  */ "",
/* 315  */ "",
/* 316  */ "SKILL_FORAGE",
/* 317  */ "SKILL_NESSKICK",
/* 318  */ "",
/* 319  */ "",
/* 320  */ "",
/* 321  */ "",
/* 322  */ "",
/* 323  */ "",
/* 324  */ "",
/* 325  */ "",
/* 326  */ "",
/* 327  */ "",
/* 328  */ "",
/* 329  */ "",
/* 330  */ "",
/* 331  */ "",
/* 332  */ "",
/* 333  */ "",
/* 334  */ "",
/* 335  */ "",
/* 336  */ "",
/* 337  */ "",
/* 338 */ "",
/* 339 */ "",
/* 340 */ "",
/* 341 */ "",
/* 342 */ "",
/* 343 */ "",
/* 344 */ "",
/* 345 */ "",
/* 346 */ "",
/* 347 */ "",
/* 348 */ "",
/* 349 */ "",
/* 350 */ "The winds around $n slowly dissipate.",
/* 351 */ "The fire surrounding $n suddenly dissipates into nothing.",
/* 352 */ "",
/* 353 */ "",
/* 354 */ "",
/* 355 */ "",
/* 356 */ "",
/* 357 */ "",
/* 358 */ "",
/* 359 */ "",
/* 360 */ "",
/* 361 */ "",
/* 362 */ "",
/* 363 */ "",
/* 364 */ "",
/* 365 */ "",
/* 366 */ "",
/* 367 */ "",
/* 368 */ "",
/* 369 */ "",
/* 370 */ "",
/* 371 */ "",
/* 372 */ "",
/* 373 */ "",
/* 374 */ "",
/* 375 */ "",
/* 376 */ "",
/* 377 */ "",
/* 378 */ "",
/* 379 */ "",
/* 380 */ "",
/* 381 */ "",
/* 382 */ "",
/* 383 */ "",
/* 384 */ "",
/* 385 */ "",
/* 386 */ "",
/* 387 */ "",
/* 388 */ "",
/* 389 */ "",
/* 390 */ "",
/* 391 */ "",
/* 392 */ "",
/* 393 */ "",
/* 394 */ "",
/* 395 */ "",
/* 396 */ "",
/* 397 */ "",
/* 398 */ "",
/* 399 */ "",
/* 400 */ "",
/* 401 */ "",
/* 402 */ "",
/* 403 */ "",
/* 404 */ "",
/* 405 */ "",
/* 406 */ "",
/* 407 */ "",
/* 408 */ "",
/* 409 */ "",
/* 410 */ "",
/* 411 */ "",
/* 412 */ "",
/* 413 */ "",
/* 414 */ "",
/* 415 */ "",
/* 416 */ "",
/* 417 */ "",
/* 418 */ "",
/* 419 */ "",
/* 420 */ "",
/* 421 */ "",
/* 422 */ "",
/* 423 */ "",
/* 424 */ "",
/* 425 */ "",
/* 426 */ "",
/* 427 */ "",
/* 428 */ "",
/* 429 */ "",
/* 430 */ "",
/* 431 */ "",
/* 432 */ "",
/* 433 */ "",
/* 434 */ "$n's shadow armor slowly melts away.",
/* 435 */ "",
/* 436 */ "",
/* 437 */ "",
/* 438 */ "",
/* 439 */ "",
/* 440 */ "",
/* 441 */ "",
/* 442 */ "",
/* 443 */ "",
/* 444 */ "",
/* 445 */ "",
/* 446 */ "",
/* 447 */ "",
/* 448 */ "",
/* 449 */ "",
/* 450 */ "",
    /* 451 */ "",
    /* blind fight */
    /* 452 */ "",
    /* daylight */
    /* 453 */ "",
    /* dispel invis */
    /* 454 */ "",
    /* dispel ethereal */
/* 455 */ "",
/* 456 */ "",
/* 457 */ "",
/* 458 */ "The ravaging fires burning $n suddenly die out with a wisp of smoke.",
};

const char * const banned_name[] = {
    "in",
    "from",
    "with",
    "the",
    "on",
    "at",
    "to",
    "me",
    "him",
    "her",
    "it",
    "sword",
    "guard",
    "you",
    "self",
    "soldier",
    "warrior",
    "ranger",
    "sorcerer",
    "assassin",
    "elf",
    "human",
    "dwarf",
    "halfing",
    "mul",
    "\n"
};

const char * const level_name[] = {
    "mortal",
    "Legend",
    "Builder",
    "Storyteller",
    "Highlord",
    "Overlord",
    "\n"
};

const char * const soft_condition[] = {
    "destroyed",
    "tattered",
    "ragged",
    "torn",
    "worn out",
    "used",
    "new",
    "\n"
};

const char * const hard_condition[] = {
    "destroyed",
    "wrecked",
    "battered",
    "dented",
    "cracked",
    "used",
    "new",
    "\n"
};

/*
   const char * const sharp_condition[] = {
   "destroyed",
   "broken",
   "battered",
   "dented",
   "nicked",
   "blunt",
   "sharp",
   "\n"
   };
 */

int rev_dir[] = {
    2,
    3,
    0,
    1,
    5,
    4
};

const char * const dir_name[] = {
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "out",
    "in",
    "\n"
};

const char * const san_dir_name[] = {
    "the north",
    "the east",
    "the south",
    "the west",
    "above",
    "below",
    "\n"
};

const char * const rev_dir_name[] = {
    "the south",
    "the west",
    "the north",
    "the east",
    "below",
    "above",
    "\n"
};

const char * const rev_dirs[] = {
    "south",
    "west",
    "north",
    "east",
    "down",
    "up",
    "in",
    "out",
    "\n"
};

const char * const const scan_dir[] = {
    "to the north", "to the east", "to the south", "to the west",
    "up above", "down below"
};

const char * const temperature_name[] = {
    "freezing",
    "freezing",
    "very cold",
    "cold",
    "cool",
    "warm",
    "hot",
    "very hot",
    "extremely hot",
    "infernally hot",
    "\n"
};


int movement_loss[] = {
    -2,                         /* Inside     */
    0,                          /* City       */
    3,                          /* Field      */
    5,                          /* Desert     */
    6,                          /* Hills      */
    8,                          /* Mountains  */
    10,                         /* Silt       */
    9,                          /* Air        */
    5,                          /* Nilaz Plane */
    6,                          /* light forest */
    9,                          /* heavy forest */
    5,                          /* salt flats */
    5,                          /* thornlands */
    4,                          /* ruins */
    4,                          /* cavern */
    2,                          /* road */
    7,                          /* silt shallows */
    4,                          /* southern caverns */
    8,                          /* sewer */
    3,                          /* cotton field */
    3,                          /* grasslands */
    0,                          /* fire_plane */
    0,                          /* water_plane */
    0,                          /* earth_plane */
    0,                          /* shadow_plane */
    0,                          /* air_plane */
    0,                          /* lightning_plane */
};

const char * const dirs[] = {
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "\n"
};

char *vowel = "aeiouy";

const char * const verbose_dirs[] = {
    "the north",
    "the east",
    "the south",
    "the west",
    "above",
    "below",
    "\n"
};

/* stuff for dates and time */
const char * const year_one[11] = {
    "Jihae",
    "Drov",
    "Desert",
    "Ruk",
    "Whira",
    "Dragon",
    "Vivadu",
    "King",
    "Silt",
    "Suk-Krath",
    "Lirathu"
};

const char * const year_two[7] = {
    "Anger",
    "Peace",
    "Vengeance",
    "Slumber",
    "Defiance",
    "Reverence",
    "Agitation"
};

const char * const sun_phase[3] = {
    "Descending Sun",
    "Low Sun",
    "Ascending Sun"
};

const char * const wear_name[] = {
    "belt",
    "right finger",
    "left finger",
    "neck",
    "back",
    "body",
    "head",
    "legs",
    "feet",
    "hands",
    "arms",
    "belt 1",
    "about",
    "waist",
    "right wrist",
    "left wrist",
    "ep",
    "es",
    "belt 2",
    "both hands",
    "in hair",
    "face",
    "ankle",
    "right ear",
    "left ear",
    "forearms",
    "about head",
    "left ankle",
    "right finger 2",
    "left finger  2",
    "right finger 3",
    "left finger  3",
    "right finger 4",
    "left finger  4",
    "right finger 5",
    "left finger  5",
    "about throat",
    "right shoulder",
    "left shoulder",
    "back sling",
    "over right shoulder",
    "over left shoulder",
    ""
};

const char * const where[] = {
    "<as belt>                ",
    "<on right index finger>  ",
    "<on left index finger>   ",
    "<around neck>            ",
    "<across back>            ",
    "<on torso>               ",
    "<on head>                ",
    "<on legs>                ",
    "<on feet>                ",
    "<on hands>               ",
    "<on arms>                ",
    "<hung from belt>         ",
    "<around body>            ",
    "<about waist>            ",
    "<around right wrist>     ",
    "<around left wrist>      ",
    "<primary hand>           ",
    "<secondary hand>         ",
    "<hung from belt>         ",
    "<both hands>             ",
    "<in hair>                ",
    "<on face>                ",
    "<around right ankle>     ",
    "<in left ear>            ",
    "<in right ear>           ",
    "<on forearms>            ",
    "<floating about head>    ",
    "<around left ankle>      ",
    "<on right middle finger> ",
    "<on left middle finger>  ",
    "<on right ring finger>   ",
    "<on left ring finger>    ",
    "<on right pinky>         ",
    "<on left pinky>          ",
    "<on right thumb>         ",
    "<on left thumb>          ",
    "<about throat>           ",
    "<on right shoulder>      ",
    "<on left shoulder>       ",
    "<slung across back>      ",
    "<over right shoulder>    ",
    "<over left shoulder>     ",
    ""
};

struct wear_edesc_struct wear_edesc[MAX_WEAR_EDESC] = { /* char *name         char *edesc_name     int location_that_covers */
    {"head", "[HEAD_SDESC]", WEAR_HEAD},
    {"face", "[FACE_SDESC]", WEAR_FACE},
    {"neck", "[NECK_SDESC]", WEAR_NECK},
    {"throat", "[THROAT_SDESC]", WEAR_ABOUT_THROAT},
    {"body", "[BODY_SDESC]", WEAR_BODY},
    {"back", "[BACK_SDESC]", WEAR_BACK},
    {"forearms", "[FOREARMS_SDESC]", WEAR_FOREARMS},
    {"arms", "[ARMS_SDESC]", WEAR_ARMS},
    {"right wrist", "[WRIST_R_SDESC]", WEAR_WRIST_R},
    {"left wrist", "[WRIST_L_SDESC]", WEAR_WRIST_L},
    {"hands", "[HANDS_SDESC]", WEAR_HANDS},
    {"waist", "[WAIST_SDESC]", WEAR_WAIST},
    {"legs", "[LEGS_SDESC]", WEAR_LEGS},
    {"right ankle", "[ANKLE_R_SDESC]", WEAR_ANKLE},
    {"left ankle", "[ANKLE_L_SDESC]", WEAR_ANKLE_L},
    {"feet", "[FEET_SDESC]", WEAR_FEET},
    {"right shoulder", "[SHOULDER_R_SDESC]", WEAR_SHOULDER_R},
    {"left shoulder", "[SHOULDER_L_SDESC]", WEAR_SHOULDER_L}
};

struct wear_edesc_struct wear_edesc_long[MAX_WEAR_EDESC] = {    /* char *name         char *edesc_name     int location_that_covers */
    {"head", "[HEAD_LDESC]", WEAR_HEAD},
    {"face", "[FACE_LDESC]", WEAR_FACE},
    {"neck", "[NECK_LDESC]", WEAR_NECK},
    {"throat", "[THROAT_LDESC]", WEAR_ABOUT_THROAT},
    {"body", "[BODY_LDESC]", WEAR_BODY},
    {"back", "[BACK_LDESC]", WEAR_BACK},
    {"forearms", "[FOREARMS_LDESC]", WEAR_FOREARMS},
    {"arms", "[ARMS_LDESC]", WEAR_ARMS},
    {"right wrist", "[WRIST_R_LDESC]", WEAR_WRIST_R},
    {"left wrist", "[WRIST_L_LDESC]", WEAR_WRIST_L},
    {"hands", "[HANDS_LDESC]", WEAR_HANDS},
    {"waist", "[WAIST_LDESC]", WEAR_WAIST},
    {"legs", "[LEGS_LDESC]", WEAR_LEGS},
    {"right ankle", "[ANKLE_R_LDESC]", WEAR_ANKLE},
    {"left ankle", "[ANKLE_L_LDESC]", WEAR_ANKLE_L},
    {"feet", "[FEET_LDESC]", WEAR_FEET},
    {"right shoulder", "[SHOULDER_R_LDESC]", WEAR_SHOULDER_R},
    {"left shoulder", "[SHOULDER_L_LDESC]", WEAR_SHOULDER_L}
};



const char * const tool_name[] = {
    "none",                     /* 0 */
    "climb",
    "trap",
    "untrap",
    "snare",
    "bandage",                  /* 5 */
    "lock pick",
    "hone",
    "mend_soft",
    "mend_hard",
    "tent",                     /* 10 */
    "fuse",
    "chisel",
    "hammer",
    "saw",
    "needle",                   /* 15 */
    "awl",
    "smoothing",
    "sharpening",
    "glass",
    "paintbrush",               /* 20 */
    "scraper",
    "fletchery",
    "drill",
    "fabric",
    "household",                /* 25 */
    "scent",
    "woodcarving",
    "building",
    "vise",
    "incense",                  /* 30 */
    "cleaning",
    "spice",
    "cooking",
    "brewing",
    "kiln",                     /* 35 */
    "small loom",
    "large loom",
    "\n"
};

struct coin_data coins[MAX_COIN_TYPE] = {
    /* 0 - Allanak */
    {0,                         /* Parent ID */
     1,                         /* Relative value to parent */
     1920,                      /* Vnum of single coin item */
     1998,                      /* Vnum of 2-20 coins item  */
     1921,                      /* vnum of 21+ coins item   */
     "Allanak Crowns"},         /* How the game refers to the coin type */
    /* 1 - Northlands Coins */
    {COIN_ALLANAK,
     1,
     0, 0, 0,                   /* This is just a coin type, so it has no items */
     "Northlands Currency"},
    /* 2 - Kurac */
    {COIN_NORTHLANDS,
     1,
     1924,
     1925,
     1925,
     "Kurac Dabloon"},
    /* 3 - Reynolte */
    {COIN_NORTHLANDS,
     1,
     1922,
     1923,
     1923,
     "Reynolte Coin"},
    /* 4 - Southlands Currency */
    {COIN_ALLANAK,
     1,
     0, 0, 0,                   /* This is a currency type, so no items */
     "Southlands Currency"},
    /* 5 - Salarr */
    {COIN_SOUTHLANDS,
     1,
     1926,
     1927,
     1927,
     "Salarr Tokens"}
};

const char * const coin_name[] = {
    "allanak",
    "northlands",
    "kurac",
    "reynolte",
    "southlands",
    "salarr",
    "\n"
};

const char * const herb_colors[] = {
    "red",
    "blue",
    "green",
    "black",
    "purple",
    "yellow",
    "white",
    "gray",
    "pink",
    "orange",
    "\n"
};

const char * const herb_types[] = {
    "none",
    "acidic",
    "sweet",
    "sour",
    "bland",
    "hot",
    "cold",
    "calming",
    "refreshing",
    "salty",
    "warming",
    "aromatic",
    "bitter",
    "\n"
};

/* When you add one here, increase MAX_CURE_TASTES in skills.h */
const char * const cure_tastes[] = {
    "none",
    "acidic",
    "bitter",
    "foul",
    "minty",
    "sour",
    "spicy",
    "sweet",
    "strong",
    "citric",
    "sharp",
    "awful",
    "fruity",
    "chalky",
    "salty",
    "gritty",
    "\n"
};

const char * const cure_forms[] = {
    "vial",
    "tablet",
    "syringe",
    "\n"
};

const char * const poison_types[] = {
    "none",
    "generic",
    "grishen",
    "skellebain",
    "methelinoc",
    "terradin",
    "peraine",
    "heramide",
    "plague",
    "skellebain_2",
    "skellebain_3",
    "\n"
};

const char * const fabric[] = {
    "sandcloth",
    "linen",
    "silk",
    "cotton",
    "hide",
    "\n"
};
const char * const hallucinations[] = {
    "the black-haired man",     /* 0 */
    "the green-haired mantis",
    "the yellow-eyed anakore",
    "the wart-nosed elf",
    "the green-shelled gaj",
    "the fat gypsy",            /* 5 */
    "the tanned elf",
    "the large half-giant",
    "the purple-haired gith",
    "the orange-leaved cactus",
    "the tall, muscular man",      /* 10 */
    "the headless dwarf",
    "the three-legged mul",
    "the elderly woman",
    "the agafari chair",
    "Suk-Krath the Sun",        /* 15 */
    "someone",
    "the grey-bearded man",
    "the buttery-skinned, elven male",
    "the pale-skinned, naked woman",
    "the well-endowed, naked man",        /* 20 */
    "A pair of tan dwarves",
    "the colossal, stone half-giant",
    "the auburn, delicate-boned woman",
    "the image of your mother",
    "the image of your father",        /* 25 */
    "the short figure in a dark, hooded cloak",
    "the man with no eyes or mouth",
    "the pale-skinned woman with three arms",
    "the many-mouthed insect with one eye",
    "a horrible nightmare creature",        /* 30 */
    "the demon with a flaming sword",
    "the well-dressed man with no head",
    "the elf with bright red eyes",
    "the sharp-toothed mul",
    "the blue-eyed, weathered man",        /* 35 */
    "the white-haired crone",
    "the white-eyed man with no mouth",
    "the woman with stitched eyes",
    "a shrunken head",
    "a female figure made entirely of liquid", /* 40 */
    "a male figure made entirely of liquid",
    "the tall figure in a dark, hooded cloak",
    "the short figure in a dark, hooded cloak",
    "a gigantic and obese faint shape",
    "a faint shape",                          /* 45 */
    "a tall and thin faint shape",
    "a strange shadow",
    "a tall strange shadow",
    "a tall, shadowy figure",
    "\n"
};

const char * const drinks[] = {
    "water",                    /* 0 */
    "beer",
    "wine",
    "ale",
    "dark ale",
    "whisky",                   /* 5 */
    "spice ale",
    "firebreather",
    "\"Red Sun\"",
    "disgusting, muddy water",
    "milk",                     /* 10 */
    "tea",
    "coffee",
    "blood",
    "salt water",
    "spirits",                  /* 15 */
    "sand",
    "silt",
    "gloth",
    "green honey mead",
    "brandy",                   /* 20 */
    "spice brandy",
    "belshun wine",
    "jallal wine",
    "japuaar wine",
    "kalan wine",               /* 25 */
    "petoch wine",
    "horta wine",
    "ginka wine",
    "lichen wine",
    "ocotillo wine",            /* 30 */
    "black anakore",
    "firestorm's flame",
    "elven tea",
    "Thrain's Revenge",
    "jaluar-wine",              /* 35 */
    "reynolte-dry",
    "honied-wine",
    "spiced-wine",
    "honied-ale",
    "honied-brandy",            /* 40 */
    "spiced-mead",
    "paint",
    "oil",
    "violet ocotillo wine",
    "purple ocotillo wine",     /* 45 */
    "ocotillo spirits",
    "cleanser",
    "honied tea",
    "fruit tea",
    "mint tea",                 /* 50 */
    "clove tea",
    "kumiss",
    "jik",                      /* anyali alcohol */
    "badu",                     /* halfling booze */
    "water",                    /* 55 Vivadu water */
    "muddy water",              /* muddy (worst) water */
    "greyish water",            /* greyish (medium) water */
    "liquid fire",              /* Liquid Fire */
    "sap",                      /* sap */
    "slavedriver",              /* 60 *//* slavedriver */
    "spiceweed tea",            /* Spiceweed Tea */
    "spiced belshun wine",      /* spiced belshun wine for Mekeda */
    "spiced ginka wine",        /* spiced ginka wine for Mekeda */
    "agvat",                    /* for Mekeda, created by Danu */
    "belshun juice",
    "ginka juice",
    "horta juice",
    "japuaar juice",
    "kalan juice",
    "melon juice",
    "thornfruit juice",
    "fruit juice",
    "yezzir",
    "sejah",
    "blackstem tea",
    "dwarfflower tea",
    "greenmantle tea",
    "kanjel tea",
    "tuluk common tea",
    "tuluk chosen tea",
    "watchman tea",
    "spicer tea",
    "moon tea",
    "sparkling wine",
    "bamberry spirits",
    "marillum",
    "sweetened marillum",
    "\n"
};

const char * const drinknames[] = {
    "water",                    /* 0 */
    "beer",
    "wine",
    "ale",
    "ale",
    "whisky",                   /* 5 */
    "spiceale",
    "firebreather",
    "redsun",
    "crapwater",
    "milk",                     /* 10 */
    "tea",
    "coffee",
    "blood",
    "salt",
    "spirits",                  /* 15 */
    "sand",
    "silt",
    "gloth",
    "mead",
    "brandy",                   /* 20 */
    "spicedbrandy",
    "belshun-wine",
    "jallal-wine",
    "japuaar-wine",
    "kalan-wine",               /* 25 */
    "petoch-wine",
    "horta-wine",
    "ginka-wine",
    "lichen-wine",
    "ocotillo-wine",            /* 30 */
    "black-anakore",
    "firestorm-flame",
    "elf-tea",
    "dwarf-ale",
    "jaluar-wine",              /* 35 */
    "reynolte-dry",
    "honied-wine",
    "spiced-wine",
    "honied-ale",
    "honied-brandy",            /* 40 */
    "spiced-mead",
    "paint",
    "oil",
    "violet ocotillo wine",
    "purple ocotillo wine",     /* 45 */
    "ocotillo spirits",
    "cleanser",
    "honied tea",
    "fruit tea",
    "mint tea",                 /* 50 */
    "clove tea",
    "kumiss",
    "jik",
    "badu",
    "bluish water",             /* 55 Vivadu water */
    "muddy water",              /* muddy water (worst) */
    "greyish water",            /* greysih water (medium) */
    "liquid fire",              /* liquid fire */
    "sap",                      /* sap */
    "slavedriver",              /* 60 *//* Slavedriver */
    "spiceweed tea",            /* Spiceweed Tea */
    "spiced belshun wine",      /* spiced belshun wine for Mekeda */
    "spiced ginka wine",        /* spiced ginka wine for Mekeda */
    "agvat",                    /* distilled cactus juice for Mekeda */
    "belshun juice",
    "ginka juice",
    "horta juice",
    "japuaar juice",
    "kalan juice",
    "melon juice",
    "thornfruit juice",
    "fruit juice",
    "yezzir whiskey",
    "sejah whiskey",
    "blackstem tea",
    "dwarfflower tea",
    "greenmantle tea",
    "kanjel tea",
    "tuluk common tea",
    "tuluk chosen tea",
    "watchman tea",
    "spicer tea",
    "Lirathu moon tea",
    "sparkling wine",
    "bamberry spirits",
    "marillum",
    "sweetened marillum",
    "\n"
};

/* drink values are:                              */
/*    dunkenness, hunger, thirst, flamability     */

int drink_aff[][8] = {
    /* first three numbers are: Drunk, Hunger, Thirst
     * fourth number is flamability
     * fifth number is the race that can benefits from these, the race -1 
     * means it affects all races the same
     * sixth, seventh and eight numbers are: Drunk, Hunger, Thirst for everyone
     * else (if race is great than -1 */
    {0, 1, 10, -2, -1, 0, 0, 0},        /*  0 - Water        */
    {3, 2, 2, 1, -1, 0, 0, 0},  /*      beer         */
    {5, 2, 1, 1, -1, 0, 0, 0},  /*      wine         */
    {2, 2, 2, 1, -1, 0, 0, 0},  /*      ale          */
    {1, 2, 3, 1, -1, 0, 0, 0},  /*      dark ale     */
    {6, 1, 2, 2, -1, 0, 0, 0},  /*  5 - Whiskey      */
    {8, 1, 0, 1, -1, 0, 0, 0},  /*      Spice ale    */
    {10, 0, 1, 1, -1, 0, 0, 0}, /*      Firebreather */
    {3, 3, 3, 1, -1, 0, 0, 0},  /*      Red Sun      */
    {0, 0, 1, 1, -1, 0, 0, 0},  /*      Crap         */
    {0, 2, 5, -1, -1, 0, 0, 0}, /* 10 - Milk         */
    {-1, 1, 6, -1, -1, 0, 0, 0},        /*      Tea          */
    {-5, 1, 6, -1, -1, 0, 0, 0},        /*      Coffee       */
    {0, 2, -1, -1, -1, 0, 0, 0},        /*      Blood        */
    {0, 1, -2, -1, -1, 0, 0, 0},        /*      Salt         */
    {10, 1, 1, 1, -1, 0, 0, 0}, /* 15 - spirits      */
    {0, -1, -5, -2, -1, 0, 0, 0},       /*      sand         */
    {0, -5, -5, -2, -1, 0, 0, 0},       /*      silt         */
    {25, 2, 5, 2, -1, 0, 0, 0}, /*      gloth        */
    {2, 2, 4, 1, -1, 0, 0, 0},  /*      green honey mead */
    {9, 0, 0, 1, -1, 0, 0, 0},  /* 20 - brandy */
    {15, 0, 1, 1, -1, 0, 0, 0}, /*      Spiced brandy */
    {10, 2, 1, 1, -1, 0, 0, 0}, /*      belshun wine */
    {9, 2, 1, 1, -1, 0, 0, 0},  /*      jallal wine */
    {8, 2, 1, 1, -1, 0, 0, 0},  /*      japuaar wine */
    {7, 2, 1, 1, -1, 0, 0, 0},  /* 25 - kalan wine */
    {6, 2, 1, 1, -1, 0, 0, 0},  /*      petoch wine */
    {5, 2, 1, 1, -1, 0, 0, 0},  /*      horta wine */
    {10, 2, 1, 1, -1, 0, 0, 0}, /*      ginka wine */
    {10, 2, 1, 2, -1, 0, 0, 0}, /*      lichen wine */
    {5, 2, 1, 1, -1, 0, 0, 0},  /* 30 - ocotillo wine */
    {20, 0, -1, 2, -1, 0, 0, 0},        /*      black anakore */
    {12, 1, 1, 5, -1, 0, 0, 0}, /*      Firestorm's flame */
    {-1, 2, 11, -1, RACE_ELF, 0, 0, -2},        /*  Elven Tea */
    {0, 2, 2, 2, RACE_DWARF, 2, 2, 2},  /*  Dwarven Ale */
    {8, 1, 1, 1, -1, 0, 0, 0},  /* 35 - jaluar-wine */
    {12, 1, 1, 1, -1, 0, 0, 0}, /*      reynolte-dry */
    {12, 1, 1, 1, -1, 0, 0, 0}, /*      spiced-wine */
    {12, 1, 1, 1, -1, 0, 0, 0}, /*      honied-wine */
    {12, 1, 1, 1, -1, 0, 0, 0}, /*      honied-ale */
    {12, 1, 1, 1, -1, 0, 0, 0}, /* 40 - honied-brandy */
    {12, 1, 1, 1, -1, 0, 0, 0}, /*      spiced-mead */
    {0, -2, -2, 1, -1, 0, 0, 0},        /*      paint */
    {0, -2, -2, 2, -1, 0, 0, 0},        /*      oil */
    {5, 2, 1, 1, -1, 0, 0, 0},  /*      violet ocotillo wine */
    {5, 2, 1, 1, -1, 0, 0, 0},  /* 45 - purple ocotillo wine */
    {12, 1, 1, 1, -1, 0, 0, 0}, /*      ocotillo spirits     */
    {0, 0, 0, 2, 0, 0, 0, 0},   /* cleanser */
    {-1, 1, 6, -1, -1, 0, 0, 0},        /*      honied tea    */
    {-1, 1, 6, -1, -1, 0, 0, 0},        /*      fruit tea     */
    {-2, 1, 6, -1, -1, 0, 0, 0},        /* 50   mint tea      */
    {-3, 1, 6, -1, -1, 0, 0, 0},        /*      clove tea     */
    {2, 1, 4, -1, -1, 2, 1, 4}, /*      kumiss        */
    {12, 1, 1, 2, -1, 0, 0, 0}, /*      jik           */
    {12, 1, 1, 2, -1, 0, 0, 0}, /*      badu          */
    {0, 1, 15, -3, -1, 0, 0, 0},        /* 55   vivadu water  */
    {0, 1, 3, -1, -1, 0, 0, 0}, /*      muddy water   */
    {0, 1, 6, -1, -1, 0, 0, 0}, /*      greysih water */
    {25, 2, 5, 5, -1, 0, 0, 0}, /*      liquid fire        */
    {0, 1, 10, -3, -1, 0, 0, 0},        /*      sap           */
    {20, 3, 4, 0, -1, 0, 0, 0}, /* 60 *//* Slavedriver */
    {-4, 1, 6, -1, -1, 0, 0, 0},        /* Spiceweed Tea */
    {12, 1, 1, 1, -1, 0, 0, 0}, /* spiced belshun wine */
    {12, 1, 1, 1, -1, 0, 0, 0}, /* spiced ginka wine */
    {12, 1, 1, 2, -1, 0, 0, 0}, /* agvat */
    {0, 2, 10, -2, -1, 0, 0, 0},        /*  belshun juice */
    {0, 2, 10, -2, -1, 0, 0, 0},        /* ginka juice */
    {0, 3, 10, -2, -1, 0, 0, 0},        /* horta juice */
    {0, 2, 10, -2, -1, 0, 0, 0},        /* japuaar juice */
    {0, 2, 10, -2, -1, 0, 0, 0},        /* kalan juice */
    {0, 2, 10, -2, -1, 0, 0, 0},        /* melon juice */
    {0, 2, 10, -2, -1, 0, 0, 0},        /* thornfruit juice */
    {0, 2, 10, -2, -1, 0, 0, 0},        /* generic fruit juice */
    {10, 0, 5, 7, RACE_DESERT_ELF, 23, 0, -5}, /* yezzir whiskey */
    {15, 0, 2, 2, -1, 0, 0, 0},			/* sejah whiskey */
    {-4, 1, 6, -1, -1, 0, 0, 0},			/* blackstem tea */
    {-4, 1, 6, -1, -1, 0, 0, 0},			/* dwarfflower tea */
    {-4, 1, 6, -1, -1, 0, 0, 0},			/* greenmantle tea */
    {-4, 1, 6, -1, -1, 0, 0, 0},			/* kanjel tea */
    {-4, 0, 2, -1, -1, 0, 0, 0},	/* Tuluki common tea */
    {-6, 0, 3, -1, -1, 0, 0, 0},	/* Tuluki chosen tea */
    {-8, 0, 1, -1, -1, 0, 0, 0},	/* Watchman tea */
    {-8, 0, 1, -1, -1, 0, 0, 0},	/* Spicer tea */
    {-4, 0, 3, -1, -1, 0, 0, 0},	/* Moon tea */
    {5, 0, 7, 2, -1, 0, 0, 0},		/* sparkling wine */
    {15, 0, 1, -2, -1, 0, 0, 0},	/* bamberry spirits */
    {12, 0, 1, -2, -1, 0, 0, 0},        /* marillum rum */
    {15, 0, 1, -2, -1, 0, 0, 0},        /* sweetened marillum rum */
    {0, 0, 0, 0, 0, 0, 0, 0}    /*      nada */
};

const char * const color_liquid[] = {
    "clear",                    /* 0 */
    "brown",
    "clear",
    "brown",
    "dark",
    "golden",                   /* 5 */
    "red",
    "green",
    "clear",
    "dark brown",
    "white",                    /* 10 */
    "brown",
    "black",
    "red",
    "clear",
    "black",                    /* 15 */
    "grainy brown",
    "fine brown",
    "caramel",
    "yellowish green",
    "yellowish",                /* 20 */
    "yellowish",
    "purple",
    "orange",
    "dark pink",
    "turquoise",                /* 25 */
    "purple",
    "black",
    "crimson",
    "white",
    "violet",                   /* 30 */
    "grainy black",
    "reddish",
    "light brown",
    "foamy brown",
    "light red",                /* 35 */
    "murky yellow",
    "yellowish",
    "yellowish",
    "yellowish",
    "yellowish",                /* 40 */
    "yellowish",
    "paint-like",
    "oily",
    "brightly colored, thick",
    "pale violet",              /* 45 */
    "deep purple",
    "purple",
    "bright pink",
    "pale pink",
    "pale green",               /* 50 */
    "pale brown",
    "white",
    "murky green",
    "deep amber",
    "clear",                    /* 55 *//* Vivadu Water */
    "muddy",                    /* Very dirty water */
    "greyish",                  /* Dirty water */
    "reddish",                  /* Liquid Fire */
    "translucent green",        /* sap */
    "murky brown",              /* 60 *//* Slavedriver */
    "dusky-pink",               /* Spiceweed Tea */
    "deep purple",              /* SPiced belshun wine */
    "blood-red",                /* spiced ginka wine */
    "sickly whitish-brown",     /* Agvat */
    "murky purple",             /* belshun fruit juice */
    "murky crimson",            /* ginka fruit juice */
    "dark murky",               /* horta fruit juice */
    "murky white",              /* japuaar fruit juice */
    "murky blue",               /* kalan fruit juice */
    "clear red",                /* melon juice */
    "murky red",                /* thornfruit juice */
    "sweet-smelling",           /* generic fruit juice */
    "bright orange",		/* yezzir whiskey */
    "deep crimson",     	/* sejah whiskey */
    "dusty green",      	/* blackstem tea */
    "translucent white",   	/* dwarfflower tea */
    "bright purple",      	/* greenmantle tea */
    "cerulean",         	/* kanjel tea */
    "murky brown",		/* Tuluki common tea */
    "greyish black",		/* Tuluki chosen tea */
    "deep black",		/* Watchman tea */
    "dusky red",		/* Spicer tea */
    "pale white",		/* Moon tea */
    "clear white",		/* sparkling wine */
    "thin black",		/* Bam berry spirit */
    "golden-brown",               /* Marillum rum*/
    "honey-green",               /* Sweetened marillum rum*/
};

const char * const fullness[] = {
    "less than half ",
    "about half ",
    "more than half ",
    ""
};

const char * const food_names[] = {
    "meat",
    "bread",
    "fruit",
    "horta",
    "ginka",
    "petoch",
    "dried fruit",
    "mush",
    "sweet",
    "snack",
    "soup",
    "tuber",
    "burned",
    "rotten",
    "cheese",
    "pepper",
    "sour",
    "vegetable",
    "honey",
    "melon",
    "grub",
    "ant",
    "insect",
    "\n"
};

int food_aff[MAX_FOOD][3] = {
    /* Bite size | Hunger | Thirst */
    {5, 8, 2},                  /* meat */
    {4, 4, -1},                 /* bread */
    {2, 2, 2},                  /* fruit */
    {2, 3, 3},                  /* horta */
    {2, 5, 1},                  /* ginka */
    {2, 4, 2},                  /* petoch */
    {2, 6, 0},                  /* dried fruit */
    {5, 6, 3},                  /* mush */
    {3, 1, 0},                  /* sweet */
    {1, 1, -3},                 /* snack */
    {2, 1, 2},                  /* soup */
    {3, 3, 1},                  /* tuber */
    {3, 2, 0},                  /* burned */
    {3, 1, 0},                  /* spoiled */
    {2, 3, 0},                  /* cheese */
    {2, 4, -2},                 /* pepper */
    {2, 3, -1},                 /* sour */
    {3, 2, 1},                  /* vegetable */
    {2, 6, 1},                  /* honey */
    {2, 1, 3},                  /* melon */
    {3, 3, 1},                  /* grub */
    {2, 4, -2},                 /* ants */
    {5, 6, 3},                  /* insect */
};

struct attribute_data obj_type[] = {
    {"light", ITEM_LIGHT, CREATOR},
    {"component", ITEM_COMPONENT, DIRECTOR},
    {"wand", ITEM_WAND, CREATOR},
    {"staff", ITEM_STAFF, CREATOR},
    {"weapon", ITEM_WEAPON, CREATOR},
    {"art", ITEM_ART, CREATOR},
    {"tool", ITEM_TOOL, CREATOR},
    {"treasure", ITEM_TREASURE, CREATOR},
    {"armor", ITEM_ARMOR, CREATOR},
    {"potion", ITEM_POTION, CREATOR},
    {"worn", ITEM_WORN, CREATOR},
    {"other", ITEM_OTHER, CREATOR},
    {"skin", ITEM_SKIN, CREATOR},
    {"container", ITEM_CONTAINER, CREATOR},
    {"note", ITEM_NOTE, CREATOR},
    {"drink", ITEM_DRINKCON, CREATOR},
    {"key", ITEM_KEY, CREATOR},
    {"scroll", ITEM_SCROLL, DIRECTOR},
    {"food", ITEM_FOOD, CREATOR},
    {"money", ITEM_MONEY, CREATOR},
    {"pen", ITEM_PEN, CREATOR},
    {"wagon", ITEM_WAGON, CREATOR},
    {"dart", ITEM_DART, CREATOR},
    {"spice", ITEM_SPICE, CREATOR},
    {"chit", ITEM_CHIT, CREATOR},
    {"poison", ITEM_POISON, CREATOR},
    {"bow", ITEM_BOW, CREATOR},
    {"teleport", ITEM_TELEPORT, CREATOR},
    {"Fire", ITEM_FIRE, CREATOR},
    {"mask", ITEM_MASK, CREATOR},
    {"furniture", ITEM_FURNITURE, CREATOR},
    {"herb", ITEM_HERB, CREATOR},
    {"cure", ITEM_CURE, CREATOR},
    {"scent", ITEM_SCENT, CREATOR},
    {"playable", ITEM_PLAYABLE, CREATOR},
    {"mineral", ITEM_MINERAL, CREATOR},
    {"powder", ITEM_POWDER, CREATOR},
    {"jewelry", ITEM_JEWELRY, CREATOR},
    {"musical", ITEM_MUSICAL, CREATOR},
    {"rumor_board", ITEM_RUMOR_BOARD, HIGHLORD},
    {"", 0, 0}
};

struct attribute_data obj_material[] = {
    {"none", MATERIAL_NONE, CREATOR},
    {"wood", MATERIAL_WOOD, CREATOR},
    {"metal", MATERIAL_METAL, CREATOR},
    {"obsidian", MATERIAL_OBSIDIAN, CREATOR},
    {"stone", MATERIAL_STONE, CREATOR},
    {"chitin", MATERIAL_CHITIN, CREATOR},
    {"gem", MATERIAL_GEM, CREATOR},
    {"glass", MATERIAL_GLASS, CREATOR},
    {"skin", MATERIAL_SKIN, CREATOR},
    {"cloth", MATERIAL_CLOTH, CREATOR},
    {"leather", MATERIAL_LEATHER, CREATOR},
    {"bone", MATERIAL_BONE, CREATOR},
    {"ceramic", MATERIAL_CERAMIC, CREATOR},
    {"horn", MATERIAL_HORN, CREATOR},
    {"graft", MATERIAL_GRAFT, CREATOR},
    {"tissue", MATERIAL_TISSUE, CREATOR},
    {"silk", MATERIAL_SILK, CREATOR},
    {"ivory", MATERIAL_IVORY, CREATOR},
    {"duskhorn", MATERIAL_DUSKHORN, CREATOR},
    {"tortoiseshell", MATERIAL_TORTOISESHELL, CREATOR},
    {"isilt", MATERIAL_ISILT, CREATOR},
    {"salt", MATERIAL_SALT, CREATOR},
    {"gwoshi", MATERIAL_GWOSHI, CREATOR},
    {"dung", MATERIAL_DUNG, CREATOR},
    {"plant", MATERIAL_PLANT, CREATOR},
    {"paper", MATERIAL_PAPER, CREATOR},
    {"food", MATERIAL_FOOD, CREATOR},
    {"feather", MATERIAL_FEATHER, CREATOR},
    {"fungus", MATERIAL_FUNGUS, CREATOR},
    {"clay", MATERIAL_CLAY, CREATOR},
    {"", 0, 0}
};

int oval_defaults[MAX_ITEM_TYPE][6] = {
    {0, 0, 0, 0, 0, 0},         /* 1 */
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},         /* 5 */
    {0, 1, 1, 3, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},         /* 10 */
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},         /* 15 */
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},         /* 20 */
    {1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0},         /* 25 */
    {1, 105, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {1, 8, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},         /* 30 */
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},         /* 35 */
    {0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0},         /* mineral */
    {0, 0, 0, 0, 0, 0},         /* powder  */
    {1, 0, 0, 0, 0, 0},         /* jewelry */
    {1, 0, 0, 0, 0, 0}          /* musical */

};

char SCROLL_SPELL[] =
    "This value determines which spell is perfoemed by using the scroll.\n\r"
    "Use any spell name, such as fireball.\n\r";

char POTION_SPELL[] =
    "This value determines which spell is performed by quaffing the potion.\n\r"
    "Use any spell name listed below.\n\r";

char STAFF_WAND_CHARGE[] =
    "The amount of mana that it has available for casting, it costs 10 times\n\r"
    "the normal amount of mana. The max-mana is the maximum a person can\n\r"
    "charge it up to.\n\r";

char OSET_FLAGS[] = "You may set the following flags:\n\r";

const char NOTHING[] = "";

struct oset_field_data oset_field[MAX_ITEM_TYPE] = {
    /* ITEM_NULL       */
    {
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}
     },

    /* ITEM_LIGHT      */
    {
     {
      "hours",
      "max-amount",
      "light_type",
      "range",
      "light_color",
      "light_flags"},
     {
      "(duration of light.  -1 for permanent)",
      "(max amount of hours)",
      "(what kind of light it is)",
      "(how far the light shines)",
      "(what color the light is)",
      "(flags on the state of the light)"},
     {
      "The light's duration is the amount of time (in game hours) that the\n\r"
      "light will stay on.  Use a value of -1 for an infinite duration.\n\r",
      "The max-contents for refilling the item, this can be set to less\n\r" "than the hours.\n\r",
      "The source of the light being used, fire, magick, or whatever.\n\r"
      "Different light types are succeptable to different environments.\n\r"
      "And have different light/extinguish echoes.\n\r" "Valid types are:\n\r",
      "How many rooms the light will illuminate in each direction.\n\r",
      "The color of the light.  Valid colors are:\n\r",
      "Flags for the light, valid flags are:\n\r"}
     },

    /* ITEM_COMPONENT  */
    {
     {
      "sphere",                 /* val1 */
      "Power",                  /* val2 */
      NOTHING,                  /* val3 */
      NOTHING,                  /* val4 */
      NOTHING,                  /* val5 */
      NOTHING},                 /* val6 */
     {
      "(Sphere of magick)",     /* val1 */
      "(Power of the component)",       /* val2 */
      NOTHING,                  /* val3 */
      NOTHING,                  /* val4 */
      NOTHING,                  /* val5 */
      NOTHING},                 /* val6 */
     {
      "This value represents the Sphere of magickal energy which the\n\r"
      "component is tied to. See 'help magick' for more detail on spheres.\n\r"
      "use one of the following spheres:\n\r",
      "This value determines how much magickal energy can by channelled with\n\r"
      "the component. This the analogous to Power use one of the following:\n\r",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING}},

    /* ITEM_WAND       */
    {
     {
      "max_mana",
      "power",
      "mana",
      "spell",
      NOTHING,
      NOTHING},
     {
      "(maximum mana)",
      "(level of spell)",
      "(mana left)",
      "(spell it casts)",
      NOTHING,
      NOTHING},
     {
      STAFF_WAND_CHARGE,
      "This value represents the level of the spell contained in the wand.\n\r"
      "It must be one of the following:\n\r",
      STAFF_WAND_CHARGE,
      "This value determines which spell is performed by using the wand.\n\r"
      "You need not use the spell number, but simply the name:\n\r",
      NOTHING,
      NOTHING},
     },

    /* ITEM_STAFF      */
    {
     {
      "max_mana",
      "power",
      "mana",
      "spell",
      NOTHING,
      NOTHING},
     {
      "(maximum mana)",
      "(level of spell)",
      "(mana left)",
      "(spell it casts)",
      NOTHING,
      NOTHING},
     {
      STAFF_WAND_CHARGE,
      "This value represents the level of the spell contained in the wand.\n\r"
      "It must be one of the following:\n\r",
      STAFF_WAND_CHARGE,
      "This value determines which spell is performed by using the staff.\n\r"
      "You need not use the spell number, but simply the name:\n\r",
      NOTHING,
      NOTHING}
     },

    /* ITEM_WEAPON     */
    {
     {
      "plus",
      "numdice",
      "sizedice",
      "wtype",
      NOTHING,
      "poison"},
     {
      "(bonus to total roll)",
      "(number of dice rolled)",
      "(sides on the dice rolled)",
      "(type of weapon used)",
      NOTHING,
      "(type of poison on the weapon)"},
     {
      "This value indicates the modifier to the dice roll which the\n\r"
      "weapon will receive when a blow is struck.\n\r",
      "This value represents the number of dice which will be rolled\n\r"
      "to determine the damage done by the weapon if a hit is scored.  For\n\r"
      "instance, if this value is 3, then three dice will be rolled.\n\r"
      "The type of dice is set with the SIZEDICE field.\n\r",
      "This value indicates the 'size' or 'type' of dice which will\n\r"
      "be rolled to determine the weapon's damage.  For example, if 6 is\n\r"
      "specified, then six-sided dice (\"regular\" dice) will be rolled.\n\r"
      "The number of dice to roll is determined by the NUMDICE field.\n\r",
      "The weapon type determines the attack type used by the weapon.\n\r"
      "You may set it to one of the following:\n\r",
      NOTHING,
      NOTHING}
     },

    /* ITEM_ART        */
    {
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}},

    /* ITEM_TOOL       */
    {
     {
      "tooltype",
      "quality",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "(type of tool)",
      "(quality of tool)",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "This represents the sort of tool. Tool types are:\n\r",
      "This represents the relative quality of the tool.  Quality can range\n\r"
      "from 0 to 10.\n\r",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING}
     },

    /* ITEM_TREASURE   */
    {
     {
      "ttype",
      "quality",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "(treasure type)",
      "quality",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "This value will determine what kind of treasure the object is:\n\r",
      "This value will determine the quality of the object:\n\r",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING}
     },

    /* ITEM_ARMOR      */
    {
     {
      "arm_damage",
      "points",
      "size",
      "max_points",
      NOTHING,
      NOTHING},
     {
      "(current armor points)",
      "(repairable armor points)",
      "(size)",
      "(max armor points)",
      NOTHING,
      NOTHING},
     {
      "Armor points are directly proportional to the amount of damage " "absorbed\n\r"
      "by a blow from a weapon on that position.  If the armor points value " "is\n\r"
      "divisible by ten, the armor will absord points / 10 damage points.  If " "it is\n\r"
      "in between multiples of ten, the amount will be semi-random.  For " "instance,\n\r"
      "say a helmet of 44 AP is hit for 10 damage.  There will be a 40% " "chance of\n\r"
      "the armor absorbing 5 damage points, and a 60% chance of it absorbing " "4.\n\r",
      "This value represents the current amount of armor points can get back to if repaired.\n\rIf not repaired frequently enough, this number will go down, making it\n\rpermanently damaged.\n\r",
      "This value represents the size of the person who the armor is designed "
      "for.\n\r  For a chart of sizes see the HELP SIZE help entry.\n\r"
      "Setting sizes for shields is irrelevant, since it is not checked.\n\r",
      "Max_points is the armor points this object had when it was new.\n\r",
      NOTHING,
      NOTHING}},

    /* ITEM_POTION     */
    {
     {
      "power",
      "spell1",
      "spell2",
      "spell3",
      NOTHING,
      NOTHING},
     {
      "(level of spells)",
      "(first spell)",
      "(second)",
      "(third)",
      NOTHING,
      NOTHING},
     {
      "ITEM_POTION: LEVEL\n\r\n\r" "This value represents the level of the spells contained in the "
      "potion.\n\rIt must be wek, yuqa, kral, een pav, sul or mon.\n\r",
      POTION_SPELL,
      POTION_SPELL,
      POTION_SPELL,
      NOTHING,
      NOTHING},
     },

    /* ITEM_WORN       */
    {
     {
      "size",
      "flags",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "(size of the clothing)",
      "(flags)",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "This value equals the size of the person whom the item was\n\r"
      "designed for.  For a chart of sizes see HELP SIZE.\n\r",
      OSET_FLAGS,
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING}
     },

    /* ITEM_OTHER      */
    {
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}
     },

    /* ITEM_SKIN       */
    {
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}
     },

    /* ITEM_SCROLL       */
    {
     {
      "power",
      "spell1",
      "spell2",
      "spell3",
      "source",
      "language"},
     {
      "(level of spells)",
      "(first spell)",
      "(second)",
      "(third)",
      "(source of the magick)",
      "(language the scroll is written in)"},
     {
      "ITEM_SCROLL: LEVEL\n\r\n\r" "This value represents the level of the spells contained in the "
      "scroll.\n\rIt must be wek, yuqa, kral, een pav, sul or mon.\n\r",
      SCROLL_SPELL,
      SCROLL_SPELL,
      SCROLL_SPELL,
      NOTHING,
      NOTHING},
     },

    /* ITEM_CONTAINER  */
    {
     {
      "capacity",
      "flags",
      "key-num",
      "race",
      "max_item_weight",
      "size"},
     {
      "(container's capacity)",
      "(container flags)",
      "(opening key)",
      "(race if it is a corpse)",
      "(max weight of any item)",
      "(size if it is a corpse)"},
     {
      "This value determines the maximum weight that the container can hold.\n\r"
      "The maximum weight is the weight of all items inside the container\n\r"
      "plus the weight of the container itself\n\r",
      OSET_FLAGS,
      "This value is the virtual object number for a key which can open the\n\r" "container",
      NOTHING,
      "This is the maximum weight any one item put into the container can have\n\r"
      "Good for quivers, where nothing more than weight 1 should fit in the opening.\n\r",
      "Sets the size of the corpse.  Should be the same size as it was when" " it\n\rwas living."}
     },

    /* ITEM_NOTE       */
    /* Words seen next to values when you ostat an object */
    {
     {
      "pages",                  /* val_1 */
      "note_file (do NOT oset)",        /* val_2 */
      "language",               /* val_3 */
      "flags",                  /* val_4 */
      "skill",                  /* val_5 */
      "MaxPerc"},               /* val_6 */

     /* Phrase seen with  'oset object' and changable fields are shown */
     {"(number of pages)",      /* val_1 */
      "(number of note file)",  /* val_2 */
      "(language of note)",     /* val_3 */
      "(note flags)",           /* val_4 */
      "(skill note teaches)",
      "(Max % note teaches)"},

     {
      /* val_1 */
      "This represents the number of pages in the 'note' object.  For\n\r"
      "instance, a book has seven pages in it that can be written on.\n\r"
      "Thus, the 'pages' value would be 7.\n\r",
      /* val_2 */
      "This is used by the code to remember where the note's text is saved.\n\r"
      "Do NOT oset this field, else you will likely scrwe up someone else's note;\n\r"
      "instead, just manually set the page_1, page_2, etc., fields.\n\r",
      /* val_3 */
      "This is the language you must be able to speak to read the note.\n\r",
      /* val_4 */
      "Flags settable only on notes\n\r" "You may set the following flags:\n\r"
      "------------------------------------------------------\n\r"
      "sealable   - The item may be sealed."
      "sealed     - The item has been sealed (and cannot be read)."
      "broken     - The item's seal has been broken (and can be read).",
      /* val_5  */
      "This is the skill that can be learned by studying the note.\n\r",
      /* val_6 */
      "This is the maximum skill % that can be gained from studying " "the note.\n\r"}
     },

    /* ITEM_DRINKCON   */
    {
     {
      "max",
      "amount",
      "liquid",
      "flags",
      "spice_num",
      "poison"},
     {
      "(capacity)",
      "(amount left)",
      "(liquid type)",
      "(flags?)",
      "(spice type)",
      "(poison type)"},
     {
      "This value represents the amount of liquid that the drink container\n\r" "can hold.\n\r",
      "This is the amount of liquid left in the container.\n\r"
      "This number is 3 times the maximum amount when full.\n\r",
      "Thirst, hunger, and drunk have two values each.  The first\n\r"
      "value is for the specified race under the race column, the second\n\r"
      "is for everyone else.  If a race is (null) then the first value is universal.\n\r"
      "You can set the following types of drinks:\n\r",
      OSET_FLAGS,
      "The spice type determines which effects are imbued upon the user of the\n\r"
      "spice.  It may be one of the currently available spices:\n\r",
      "This sets the type of poison that it is poisoned with.\n\r"}
     },

    /* ITEM_KEY        */
    {
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}
     },

    /* ITEM_FOOD       */
    {
     {
      "filling",
      "ftype",
      "age",
      "poison",
      "racefood",
      "spice_num"},
     {
      "(number of hours restored)",
      "(food type)",
      "(age)",
      "(poisoned?)",
      "(racefood)",
      "(spice type)"},
     {
      "This value represents the number of game hours which the food will fill\n\r" "you for.\n\r",
      "You can set the following food types:\n\r",
      "This value represents how many hours before the food will rot.\n\r",
      OSET_FLAGS,
      OSET_FLAGS,
      "The spice type determines which effects are imbued upon the user of the\n\r"
      "spice.  It may be one of the currently available spices:\n\r"}
     },

    /* ITEM_MONEY      */
    {
     {
      "numcoins",
      "cointype",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "(amount of money)",
      "(type of money)",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "The amount of obsidian contained in a \"obsidian object.\"\n\r",
      "The type of coin that is being dealt with.\n\r",
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING}
     },

    /* ITEM_PEN        */
    {
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}},

    /* ITEM_BOAT       */
    {
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}},

    /* ITEM_WAGON      */
    {
     {
      "entrance",
      "max_mounts",
      "front",
      "flags",
      "wagon_damage",
      "max_size"},
     {
      "(entrance room)",
      "(maximum mounts)",
      "(front room)",
      "(wagon flags)",
      "(wagon damage)",
      "(maximum size)"},
     {
      "The entrance to the wagon is the room which characters are put into\n\r"
      "when they enter.\n\r",

      "Max number of mounts which can pull the item. 0 if no mounts are needed.\n\r",

      "The front of the wagon is the room number from which commands to the\n\r"
      "lizard leading the wagon can be issued.\n\r",

      "You may set the following flags:\n\r"
      "------------------------------------------------------\n\r"
      "land       - Allows a wagon to move over land.\n\r"
      "silt       - Allows a wagon to move in silt.\n\r"
      "air        - Allows a wagon to move in the air.\n\r"
      "no_mounts  - Wagons with this flag do not require mounts.\n\r"
      "no_leave   - This prevents people from leaving a wagon. Ethereal and\n\r"
      "             immortals are not stopped.\n\r"
      "reins_land - This flag indicates that the reins are on the ground, and\n\r"
      "             not on the ground.  The 'reins_land' flag must be set before\n\r"
      "             mounts can be harnessed to a wagon object.\n\r",
      "How much damage the wagon has taken.  When this value is 0, it has\n\r"
      "taken no damage, or is fully repaired.\n\r",
      "The maximum size a person can be and still enter the wagon.\n\r"}
     },

    /* ITEM_DART       */
    {
     {
      "plus",
      "numdice",
      "sizedice",
      "poison",
      NOTHING,
      NOTHING},
     {
      "(plus)",
      "(numdice)",
      "(sizedice)",
      "(amount of poison)",
      NOTHING,
      NOTHING},
     {
      "The bonus after the dice are rolled.",
      "The number of dice rolled.",
      "The size of the dice rolled.",
      "The amount of poison on the dart.",
      NOTHING,
      NOTHING}
     },

    /* ITEM_SPICE      */
    {
     {
      "power",
      "spice_num",
      "smoke",
      "poison",
      NOTHING,
      NOTHING},
     {
      "(level of the spice)",
      "(spice type or spell)",
      "(smokeable?)",
      "(poisoned?)",
      NOTHING,
      NOTHING},
     {
      "The spice 'power' determines the strength and duration of the spice's\n\r"
      "effects.  It should be a number between 1 and 5, in most cases.\n\r",
      "The spice type determines which effects are imbued upon the user of the\n\r"
      "spice.  It may be a spell, or one of the currently available spices:\n\r",
      "The smoke attribute determines whether the spice item is smokeable or "
      "sniffable (0 for sniffable, anything else for smokeable)\n\r",
      "Has it been laced with a poison?\n\r",
      NOTHING,
      NOTHING}
     },

    /* ITEM_CHIT       */
    {
     {
      "tvnum",
      "time",
      "ovnum",
      "ornum",
      NOTHING,
      NOTHING},
     {
      "(tailors virt number)",
      "(time of completion)",
      "(objects virt number)",
      "(objects real number)",
      NOTHING,
      NOTHING},
     {
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING}
     },

    /* ITEM_POISON     */
    {
     {"poison", "amount", NOTHING, NOTHING, NOTHING, NOTHING},
     {"(poison type)", "(poison amount)", NOTHING, NOTHING, NOTHING, NOTHING},
     {"Sets the type of poison.  The following are available poisons:\n\r",
      "Sets amount of poison available.  Currently not used.\n\r",
      NOTHING, NOTHING, NOTHING, NOTHING}},

    /* ITEM_BOW        */
    {
     {
      "range",
      "pull",
      "loads",
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "(range of the bow)",
      "(strength to pull the bow)",
      "(requires loading)",
      NOTHING,
      NOTHING,
      NOTHING},
     {
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING,
      NOTHING}
     },

    /* ITEM_TELEPORT        */
    {
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}},

    /* ITEM_FIRE        */
    {
     {
      "size",
      "duration",
      "heat",
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "(if it is lit or not)",
      "(how long fire will burn)",
      "(how hot the fire is)",
      NOTHING,
      NOTHING,
      NOTHING},
     {
      "  This is a value 1 if the fire is lit, and 0 if not.",
      "  This is the duration in game ticks that the fire will"
      " last, it does not neccisarily represent the 'size' of"
      " the fire, as to how much light it sheds.",
      " This is the 'heat' of the fire, how often it gets pulsed",
      NOTHING,
      NOTHING,
      NOTHING}
     },

    /* ITEM MASK */
    {
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}},

    /* ITEM_FURNITURE */
    {
     {"capacity", "flags", "numoccupants", NOTHING, NOTHING, NOTHING},
     {"(max weight allowed on it)",
      "(furniture flags)",
      "(occupancy, chairs for tables, # people for chairs/beds)",
      NOTHING, NOTHING, NOTHING},
     {"  This sets the maximum weight in stones that can be on the object.\n\r",
      OSET_FLAGS,
      "  This sets the number of chairs allowed around a table (if the table\n\r"
      "  flag is set) or the number or people allowed on it for chairs/beds\n\r",
      NOTHING, NOTHING, NOTHING}},

    /* ITEM_HERB */
    {
     {"herb_type", "edibility", "herb_color", "poison", NOTHING, NOTHING},
     {"(Type of herb it is)",
      "(Non edible, or edible)",
      "(What color is the herb?",
      "(Poison type?)",
      NOTHING, NOTHING},
     {"  This sets the herb to a valid herb type.\n\r",
      "  0 = not edible, 1 = edible.\n\r",
      "  This sets a color on the herb, for mixing purposes.\n\r",
      "  This sets the herb with a poisonous side-effect.\n\r",
      NOTHING, NOTHING}},

    /* ITEM_CURE */
    {
     {"cure", "form", "poison", "taste", "strength", NOTHING},
     {"(What poison type does this cure?)",
      "(What form is this in?)",
      "(Does it have a side-effect?)",
      "(What does it taste like?)",
      "(How strong is it?)",
      NOTHING},
     {"  This sets the type of poison it cures.  Set using poison name, not number.\n\r\n\r"
      "They are, with numerical equivalent in parentheses:\n\r"
      "  none (0), generic (1), grishen (2), skellebain (3), methelinoc (4),\n\r"
      "  terradin (5), peraine (6), heramide (7), plague (8)\n\r",
      "  0 = vial, 1 = tablet, 2 = syringe.\n\r",
      "  Is it poisoned?  If so, what type of poison?\n\r",
      "What the cure tastes like when ... tasted.\n\r",
      "How strong the cure is, if it can succeed completely.\n\r"
      "0 is 0%, which is a dud. 100 is 100% which removes the entire affect.\n\r"
      "99 is 99%, which will always leave 0 hours left.\n\r", NOTHING}},

    /* ITEM_SCENT */
    {
     {"smell", "current", "max", NOTHING, NOTHING, NOTHING},
     {"(The scent it applies on use)",
      "(current # of uses left)",
      "(maximum # of uses)",
      NOTHING, NOTHING, NOTHING},
     {"See show s for a list of smells\n\r",
      "Current amount of smell left in the bottle, should be <= max",
      "Maximum amount of smell the bottle can contain (# of uses)",
      NOTHING, NOTHING, NOTHING}
     },

    /* ITEM_PLAYABLE */
    {
     {"play_type", NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {"(What type of playable object)",
      NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {"Set the type of playable", NOTHING, NOTHING, NOTHING, NOTHING, NOTHING}
     },

    /* ITEM_MINERAL                                 */
    /* Added by Sanvean, for forage and brew code.  */
    {
     {"mineral_type",
      "edibility",
      "grinding",
      "ore_type",
      NOTHING, NOTHING},
     {"(What type of mineral?) \n\r"
      "(Valid mineral types are: stone, ash, salt, essence, magickal.) \n\r",
      "(Is it edible?) \n\r" "(Values are edible, inedible.) \n\r",
      "(Does it need to be ground before it can be brewed?) \n\r",
      "(Is it smeltable?) \n\r",
      NOTHING, NOTHING},
     {"(Sets mineral type.) \n\r"
      "(Valid mineral types are: stone, ash, salt, essence, magickal.) \n\r",
      "(Sets edibility.) \n\r" "(Values are edible and inedible.) \n\r",
      "(Sets whether or not it requires grinding.) \n\r"
      "(Values are grindable, not grindable.) \n\r",
      "(Sets whether or not it's an ore, and which type.) \n\r"
      "(This is not implemented yet.) \n\r",
      NOTHING, NOTHING}},

    /* ITEM_POWDER                                   */
    /* Added by Sanvean, for brew code.              */
    {
     {"texture",
      "solubility",
      NOTHING, NOTHING, NOTHING, NOTHING},
     {"(What consistency of powder?) \n\r",
      "(Can it be dissolved in liquid?) \n\r",
      NOTHING, NOTHING, NOTHING, NOTHING},
     {"(Sets powder texture.) \n\r"
      "(Valid textures are: fine, grit, tea, salt , essence, magickal.) \n\r",
      "(Sets powder solubility.) \n\r" "(Values are soluble, not soluble.) \n\r",
      NOTHING, NOTHING, NOTHING, NOTHING}},

    /* ITEM_JEWELRY                                 */
    {
     {"quality",
      "flags", NOTHING, NOTHING, NOTHING, NOTHING},
     {"(1-10 integer, 1 being lowest quality.) \n\r",
      "(flags)", NOTHING, NOTHING, NOTHING, NOTHING},
     {"(Sets quality, 1-10 value, 1 is lowest quality.) \n\r",
      OSET_FLAGS, NOTHING, NOTHING, NOTHING, NOTHING}},

    /* ITEM_MUSICAL                                  */
    {
     {"quality",
      "instrument_type",
      "flags", NOTHING, NOTHING, NOTHING},
     {"(1-10 integer, 1 being lowest quality.) \n\r",
      "Type of instrument.) \n\r",
      "(flags)", NOTHING, NOTHING, NOTHING},
     {"(Sets quality, 1-10 value, 1 is lowest quality.) \n\r",
      "Sets musical instrument type: values are bells, brass, drone, gong, gourd, "
      "large drum, magick flute, magick horn, percussion, rattle, "
      "small drum, stringed, whistle, wind. )\n\r",
      OSET_FLAGS, NOTHING, NOTHING, NOTHING}},

    /* ITEM_RUMOR_BOARD      */
    {
     {"board#", NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {"(board # to show posts from)", NOTHING, NOTHING, NOTHING, NOTHING, NOTHING},
     {"The board # to show posts from, file needs to exist.", NOTHING, NOTHING, NOTHING, NOTHING,
      NOTHING}
     },

};

struct flag_data obj_state_flag[] = {
    {"bloody", OST_BLOODY, CREATOR},
    {"dusty", OST_DUSTY, CREATOR},
    {"stained", OST_STAINED, CREATOR},
    {"repaired", OST_REPAIRED, CREATOR},
    {"gith", OST_GITH, CREATOR},
    {"burned", OST_BURNED, CREATOR},
    {"sewer", OST_SEWER, CREATOR},
    {"old", OST_OLD, CREATOR},
    {"sweaty", OST_SWEATY, CREATOR},
    {"embroidered", OST_EMBROIDERED, CREATOR},
    {"fringed", OST_FRINGED, CREATOR},
    {"lace-edged", OST_LACED, CREATOR},
    {"mud-caked", OST_MUDCAKED, CREATOR},
    {"torn", OST_TORN, CREATOR},
    {"tattered", OST_TATTERED, CREATOR},
    {"ash", OST_ASH, CREATOR},
    {"", 0, 0}
};

struct flag_data obj_color_flag[] = {
    {"red", OCF_RED, CREATOR},
    {"blue", OCF_BLUE, CREATOR},
    {"yellow", OCF_YELLOW, CREATOR},
    {"green", OCF_GREEN, CREATOR},
    {"purple", OCF_PURPLE, CREATOR},
    {"orange", OCF_ORANGE, CREATOR},
    {"black", OCF_BLACK, CREATOR},
    {"white", OCF_WHITE, CREATOR},
    {"brown", OCF_BROWN, CREATOR},
    {"grey", OCF_GREY, CREATOR},
    {"silvery", OCF_SILVERY, CREATOR},
    {"", 0, 0}
};

struct flag_data obj_adverbial_flag[] = {
    {"patterned", OAF_PATTERNED, CREATOR},
    {"checkered", OAF_CHECKERED, CREATOR},
    {"striped", OAF_STRIPED, CREATOR},
    {"batiked", OAF_BATIKED, CREATOR},
    {"dyed", OAF_DYED, CREATOR},
    {"", 0, 0}
};


struct flag_data obj_wear_flag[] = {
    {"take", ITEM_TAKE, CREATOR},
    {"belt", ITEM_WEAR_BELT, CREATOR},
    {"finger", ITEM_WEAR_FINGER, CREATOR},
    {"neck", ITEM_WEAR_NECK, CREATOR},
    {"body", ITEM_WEAR_BODY, CREATOR},
    {"head", ITEM_WEAR_HEAD, CREATOR},
    {"legs", ITEM_WEAR_LEGS, CREATOR},
    {"feet", ITEM_WEAR_FEET, CREATOR},
    {"hands", ITEM_WEAR_HANDS, CREATOR},
    {"arms", ITEM_WEAR_ARMS, CREATOR},
    {"about", ITEM_WEAR_ABOUT, CREATOR},
    {"waist", ITEM_WEAR_WAIST, CREATOR},
    {"wrist", ITEM_WEAR_WRIST, CREATOR},
    {"ep", ITEM_EP, CREATOR},
    {"es", ITEM_ES, CREATOR},
    {"on_belt", ITEM_WEAR_ON_BELT, CREATOR},
    {"back", ITEM_WEAR_BACK, CREATOR},
    {"in_hair", ITEM_WEAR_IN_HAIR, CREATOR},
    {"face", ITEM_WEAR_FACE, CREATOR},
    {"ankle", ITEM_WEAR_ANKLE, CREATOR},
    {"left_ear", ITEM_WEAR_LEFT_EAR, CREATOR},
    {"right_ear", ITEM_WEAR_RIGHT_EAR, CREATOR},
    {"forearms", ITEM_WEAR_FOREARMS, CREATOR},
    {"about_head", ITEM_WEAR_ABOUT_HEAD, CREATOR},
    {"about_throat", ITEM_WEAR_ABOUT_THROAT, CREATOR},
    {"shoulder", ITEM_WEAR_SHOULDER, CREATOR},
    {"over_shoulder", ITEM_WEAR_OVER_SHOULDER, CREATOR},
    {"", 0, 0}
};

struct flag_data obj_extra_flag[] = {
    {"glow", OFL_GLOW, CREATOR},
    {"hum", OFL_HUM, CREATOR},
    {"dark", OFL_DARK, CREATOR},
    {"evil", OFL_EVIL, CREATOR},
    {"spl_invisible", OFL_INVISIBLE, CREATOR},
    {"magic", OFL_MAGIC, CREATOR},
    {"no_drop", OFL_NO_DROP, CREATOR},
    {"no_fall", OFL_NO_FALL, CREATOR},
    {"arrow", OFL_ARROW, CREATOR},
    {"missile_short", OFL_MISSILE_S, CREATOR},
    {"missile_long", OFL_MISSILE_L, CREATOR},
    {"disintigrate", OFL_DISINTIGRATE, HIGHLORD},
    {"spl_ethereal", OFL_ETHEREAL, CREATOR},
    {"anti_mortal", OFL_ANTI_MORT, HIGHLORD},
    {"no_sheathe", OFL_NO_SHEATHE, CREATOR},
    {"hidden", OFL_HIDDEN, CREATOR},
    {"spl_glyph", OFL_GLYPH, CREATOR},
    {"save_mount", OFL_SAVE_MOUNT, OVERLORD},
    {"poison", OFL_POISON, CREATOR},
    {"no_es", OFL_NO_ES, CREATOR},
    {"bolt", OFL_BOLT, HIGHLORD},
    {"missile_return", OFL_MISSILE_R, CREATOR}, /*  Kel  */
    {"harness", OFL_HARNESS, CREATOR},
    {"flaming", OFL_FLAME, CREATOR},
    {"nosave", OFL_NOSAVE, CREATOR},
    {"dart", OFL_DART, CREATOR},
    {"dyable", OFL_DYABLE, CREATOR},
    {"cast_ok", OFL_CAST_OK, CREATOR},
    {"ethereal_ok", OFL_ETHEREAL_OK, CREATOR},
    {"", 0, 0}
};

struct flag_data room_flag[] = {
    {"dark", RFL_DARK, CREATOR},
    {"death", RFL_DEATH, CREATOR},
    {"no_npc", RFL_NO_MOB, CREATOR},
    {"no_magick", RFL_NO_MAGICK, CREATOR},
    {"plant_sparse", RFL_PLANT_SPARSE, CREATOR},
    {"plant_heavy", RFL_PLANT_HEAVY, CREATOR},
    {"private", RFL_PRIVATE, CREATOR},
    {"sandstorm", RFL_SANDSTORM, CREATOR},
    {"tunnel", RFL_TUNNEL, CREATOR},
    {"falling", RFL_FALLING, CREATOR},
    {"unsaved", RFL_UNSAVED, OVERLORD},
    {"no_climb", RFL_NO_CLIMB, CREATOR},
    {"no_forage", RFL_NO_FORAGE, OVERLORD},
    {"populated", RFL_POPULATED, CREATOR},
    {"indoors", RFL_INDOORS, CREATOR},
    {"restful_shade", RFL_RESTFUL_SHADE, STORYTELLER},
    {"no_psionics", RFL_NO_PSIONICS, CREATOR},
    {"no_mount", RFL_NO_MOUNT, CREATOR},
    {"no_wagon", RFL_NO_WAGON, CREATOR},
    {"no_flying", RFL_NO_FLYING, CREATOR},
    {"magick_ok", RFL_MAGICK_OK, CREATOR},
    {"safe", RFL_SAFE, CREATOR},
    {"alarm", RFL_SPL_ALARM, CREATOR},
    {"ash", RFL_ASH, HIGHLORD},
    {"dmanaregen", RFL_DMANAREGEN, HIGHLORD},
    {"immortal", RFL_IMMORTAL_ROOM, CREATOR},
    {"cargo_bay", RFL_CARGO_BAY, CREATOR},
    {"no_elem_magick", RFL_NO_ELEM_MAGICK, CREATOR},
    {"no_hide", RFL_NO_HIDE, CREATOR},
    {"lit", RFL_LIT, CREATOR},
#ifndef USE_NEW_ROOM_IN_CITY
    {"allanak", RFL_ALLANAK, CREATOR},
    {"tuluk", RFL_TULUK, CREATOR},
#endif
    {"", 0, 0}
};

struct flag_data exit_flag[] = {
    {"door", EX_ISDOOR, CREATOR},
    {"closed", EX_CLOSED, CREATOR},
    {"locked", EX_LOCKED, CREATOR},
    {"rsclosed", EX_RSCLOSED, CREATOR},
    {"rslocked", EX_RSLOCKED, CREATOR},
    {"secret", EX_SECRET, CREATOR},
    {"spl_sand_wall", EX_SPL_SAND_WALL, CREATOR},
    {"no_climb", EX_NO_CLIMB, CREATOR},
    {"no_flee", EX_NO_FLEE, CREATOR},
    {"trapped", EX_TRAPPED, CREATOR},
    {"rune", EX_RUNES, CREATOR},
    {"diff_1", EX_DIFF_1, CREATOR},
    {"diff_2", EX_DIFF_2, CREATOR},
    {"diff_4", EX_DIFF_4, CREATOR},
    {"tunn_1", EX_TUNN_1, CREATOR},
    {"tunn_2", EX_TUNN_2, CREATOR},
    {"tunn_4", EX_TUNN_4, CREATOR},
    {"guarded", EX_GUARDED, HIGHLORD},
    {"thorns", EX_THORN, CREATOR},
    {"spl_fire_wall", EX_SPL_FIRE_WALL, CREATOR},
    {"spl_wind_wall", EX_SPL_WIND_WALL, CREATOR},
    {"spl_blade_barrier", EX_SPL_BLADE_BARRIER, CREATOR},
    {"spl_thorn_wall", EX_SPL_THORN_WALL, CREATOR},
    {"transparent", EX_TRANSPARENT, CREATOR},
    {"passable", EX_PASSABLE, CREATOR},
    {"windowed", EX_WINDOWED, CREATOR},
    {"no_key", EX_NO_KEY, CREATOR},
    {"no_open", EX_NO_OPEN, CREATOR},
    {"no_listen", EX_NO_LISTEN, CREATOR},
    {"climbable", EX_CLIMBABLE, CREATOR},
    //    {"raise_lower", EX_RAISE_LOWER, CREATOR},
    {"", 0, 0}
};

struct attribute_data room_sector[] = {
    {"inside", SECT_INSIDE, CREATOR},
    {"city", SECT_CITY, CREATOR},
    {"field", SECT_FIELD, CREATOR},
    {"desert", SECT_DESERT, CREATOR},
    {"hills", SECT_HILLS, CREATOR},
    {"mountains", SECT_MOUNTAIN, CREATOR},
    {"silt", SECT_SILT, CREATOR},
    {"air", SECT_AIR, CREATOR},
    {"nilaz_plane", SECT_NILAZ_PLANE, CREATOR},
    {"light_forest", SECT_LIGHT_FOREST, CREATOR},
    {"heavy_forest", SECT_HEAVY_FOREST, CREATOR},
    {"salt_flats", SECT_SALT_FLATS, CREATOR},   /* added 1/21/00 -San */
    {"thornlands", SECT_THORNLANDS, CREATOR},   /* added 2/4/00 -San  */
    {"ruins", SECT_RUINS, CREATOR},
    {"cavern", SECT_CAVERN, CREATOR},
    {"road", SECT_ROAD, CREATOR},
    {"shallow_silt", SECT_SHALLOWS, CREATOR},
    {"southern_cavern", SECT_SOUTHERN_CAVERN, CREATOR}, /* added 12/14/02 -ness */
    {"sewer", SECT_SEWER, CREATOR},     /* added for Ashyom 4/24/04 -Sanvean */
    {"cottonfield", SECT_COTTONFIELD, CREATOR}, /* Added for Vendyra 7/12/04 -Nessalin */
    {"grasslands", SECT_GRASSLANDS, CREATOR},   /* Added for Gilvar.  -San */
    {"fire_plane", SECT_FIRE_PLANE, CREATOR},   /* Added for Halasturd 1/15/04 -Nessalin */
    {"water_plane", SECT_WATER_PLANE, CREATOR}, /* Added for Halasturd 1/15/04 -Nessalin */
    {"earth_plane", SECT_EARTH_PLANE, CREATOR}, /* Added for Halasturd 1/15/04 -Nessalin */
    {"shadow_plane", SECT_SHADOW_PLANE, CREATOR},       /* Added for Halasturd 1/15/04 -Nessalin */
    {"air_plane", SECT_AIR_PLANE, CREATOR},     /* Added for Halasturd 1/15/04 -Nessalin */
    {"lightning_plane", SECT_LIGHTNING_PLANE, CREATOR}, /* Added for Halasturd 1/15/04 -Nessalin */
    {"", 0, 0}
};

struct flag_data quiet_flag[] = {
    {"nolog", QUIET_LOG, CREATOR},
    {"nowish", QUIET_WISH, CREATOR},
    {"noicomm", QUIET_IMMCHAT, CREATOR},
    {"nogcomm", QUIET_GODCHAT, CREATOR},
    {"nopsi", QUIET_PSI, CREATOR},
    {"notypo", QUIET_TYPO, CREATOR},
    {"nobug", QUIET_BUG, CREATOR},
    {"noidea", QUIET_IDEA, CREATOR},
    {"nocontact", QUIET_CONT_MOB, CREATOR},
    {"nodeath", QUIET_DEATH, CREATOR},
    {"noconnectlog", QUIET_CONNECT, CREATOR},
    {"nozone", QUIET_ZONE, CREATOR},
    {"reroll", QUIET_REROLL, CREATOR},
    {"combat", QUIET_COMBAT, CREATOR},
    {"calculations", QUIET_CALC, CREATOR},
    {"bank", QUIET_BANK, CREATOR},
    {"npc_ai", QUIET_NPC_AI, CREATOR},
    {"nobcomm", QUIET_BLDCHAT, CREATOR},
    {"", CHAR_AFF_BLIND, CREATOR}
};

struct flag_data affect_flag[] = {
    {"blind", CHAR_AFF_BLIND, CREATOR},
    {"invisible", CHAR_AFF_INVISIBLE, CREATOR},
    {"nothing", CHAR_AFF_NOTHING, CREATOR},
    {"detect_invisible", CHAR_AFF_DETECT_INVISIBLE, CREATOR},
    {"detect_magick", CHAR_AFF_DETECT_MAGICK, CREATOR},
    {"invulnerability", CHAR_AFF_INVULNERABILITY, CREATOR},
    {"detect_ethereal", CHAR_AFF_DETECT_ETHEREAL, CREATOR},
    {"sanctuary", CHAR_AFF_SANCTUARY, CREATOR},
    {"godspeed", CHAR_AFF_GODSPEED, CREATOR},
    {"sleep", CHAR_AFF_SLEEP, CREATOR},
    {"noquit", CHAR_AFF_NOQUIT, CREATOR},
    {"flying", CHAR_AFF_FLYING, CREATOR},
    {"climbing", CHAR_AFF_CLIMBING, CREATOR},
    {"slow", CHAR_AFF_SLOW, CREATOR},
    {"summoned", CHAR_AFF_SUMMONED, CREATOR},
    {"burrow", CHAR_AFF_BURROW, CREATOR},
    {"ethereal", CHAR_AFF_ETHEREAL, CREATOR},
    {"infravision", CHAR_AFF_INFRAVISION, CREATOR},
    {"cathexis", CHAR_AFF_CATHEXIS, CREATOR},
    {"sneak", CHAR_AFF_SNEAK, CREATOR},
    {"hide", CHAR_AFF_HIDE, CREATOR},
    {"feeblemind", CHAR_AFF_FEEBLEMIND, CREATOR},
    {"charm", CHAR_AFF_CHARM, HIGHLORD},
    {"scan", CHAR_AFF_SCAN, HIGHLORD},
    {"calm", CHAR_AFF_CALM, CREATOR},
    {"damage", CHAR_AFF_DAMAGE, HIGHLORD},
    {"listen", CHAR_AFF_LISTEN, CREATOR},
    {"detect_poison", CHAR_AFF_DETECT_POISON, CREATOR},
    {"subdued", CHAR_AFF_SUBDUED, HIGHLORD},
    {"deafness", CHAR_AFF_DEAFNESS, HIGHLORD},
    {"silence", CHAR_AFF_SILENCE, HIGHLORD},
    {"psi", CHAR_AFF_PSI, OVERLORD},
    {"", 0, 0}
};

struct flag_data skill_flag[] = {
    {"learned", SKILL_FLAG_LEARNED, CREATOR},
    {"nogain", SKILL_FLAG_NOGAIN, CREATOR},
    {"hidden", SKILL_FLAG_HIDDEN, CREATOR},
    {"", 0, 0}
};

struct attribute_data obj_apply_types[] = {
    {"nothing", CHAR_APPLY_NONE, CREATOR},
    {"aff_strength", CHAR_APPLY_STR, CREATOR},
    {"agility", CHAR_APPLY_AGL, CREATOR},
    {"wisdom", CHAR_APPLY_WIS, CREATOR},
    {"endurance", CHAR_APPLY_END, CREATOR},
    {"thirst", CHAR_APPLY_THIRST, CREATOR},
    {"hunger", CHAR_APPLY_HUNGER, CREATOR},
    {"mana", CHAR_APPLY_MANA, CREATOR},
    {"health", CHAR_APPLY_HIT, CREATOR},
    {"stamina", CHAR_APPLY_MOVE, CREATOR},
    {"obsidian", CHAR_APPLY_OBSIDIAN, OVERLORD},
    {"damage", CHAR_APPLY_DAMAGE, OVERLORD},
    {"AC", CHAR_APPLY_AC, OVERLORD},
    {"armor", CHAR_APPLY_ARMOR, CREATOR},
    {"offense", CHAR_APPLY_OFFENSE, CREATOR},
    {"detox", CHAR_APPLY_DETOX, CREATOR},
    {"save_poison", CHAR_APPLY_SAVING_PARA, CREATOR},
    {"save_rod", CHAR_APPLY_SAVING_ROD, CREATOR},
    {"save_petri", CHAR_APPLY_SAVING_PETRI, CREATOR},
    {"save_breath", CHAR_APPLY_SAVING_BREATH, CREATOR},
    {"save_spell", CHAR_APPLY_SAVING_SPELL, CREATOR},
    {"defense", CHAR_APPLY_DEFENSE, CREATOR},
    {"sneak", CHAR_APPLY_SKILL_SNEAK, CREATOR},
    {"hide", CHAR_APPLY_SKILL_HIDE, CREATOR},
    {"listen", CHAR_APPLY_SKILL_LISTEN, CREATOR},
    {"climb", CHAR_APPLY_SKILL_CLIMB, CREATOR},
    {"cflag", CHAR_APPLY_CFLAGS, OVERLORD},
    {"stun", CHAR_APPLY_STUN, CREATOR},
    {"", 0, 0}
};

struct flag_data mercy_flag[] = {
    {"kill", MERCY_KILL, OVERLORD},
    {"flee", MERCY_FLEE, OVERLORD},
    {"", 0, 0}
};

struct flag_data nosave_flag[] = {
    {"arrest", NOSAVE_ARREST, OVERLORD},
    {"climb", NOSAVE_CLIMB, OVERLORD},
    {"magick", NOSAVE_MAGICK, OVERLORD},
    {"psionics", NOSAVE_PSIONICS, OVERLORD},
    {"skills", NOSAVE_SKILLS, OVERLORD},
    {"subdue", NOSAVE_SUBDUE, OVERLORD},
    {"theft", NOSAVE_THEFT, OVERLORD},
    {"combat", NOSAVE_COMBAT, OVERLORD},
    {"", 0, 0}
};

struct flag_data brief_flag[] = {
    {"combat", BRIEF_COMBAT, OVERLORD},
    {"room", BRIEF_ROOM, OVERLORD},
    {"ooc", BRIEF_OOC, OVERLORD},
    {"names", BRIEF_STAFF_ONLY_NAMES, OVERLORD},
    {"equip", BRIEF_EQUIP, OVERLORD},
    {"novice", BRIEF_NOVICE, OVERLORD},
    {"exits", BRIEF_EXITS, OVERLORD},
    {"emote", BRIEF_EMOTE, OVERLORD},
    {"prompt", BRIEF_PROMPT, OVERLORD},
    {"", 0, 0}
};

struct flag_data quit_flag[] = {
    {"ooc", QUIT_OOC, HIGHLORD},
    {"ooc_revoked", QUIT_OOC_REVOKED, HIGHLORD},
    {"wilderness", QUIT_WILDERNESS, OVERLORD},
    {"", 0, 0}
};

struct flag_data char_flag[] = {
    {"clan_leader", CFL_CLANLEADER, STORYTELLER},
    {"nohelp", CFL_NOHELP, CREATOR},
    {"sentinel", CFL_SENTINEL, CREATOR},
    {"scavenger", CFL_SCAVENGER, CREATOR},
    {"npc", CFL_ISNPC, OVERLORD},
    {"aggressive", CFL_AGGRESSIVE, CREATOR},
    {"stay_zone", CFL_STAY_ZONE, CREATOR},
    {"wimpy", CFL_WIMPY, CREATOR},
    {"brief", CFL_BRIEF, OVERLORD},
    {"compact", CFL_COMPACT, OVERLORD},
    {"frozen", CFL_FROZEN, HIGHLORD},
    {"undead", CFL_UNDEAD, CREATOR},
    {"nosave", CFL_NOSAVE, OVERLORD},
    {"can_poison", CFL_CAN_POISON, HIGHLORD},
    {"silt_monster", CFL_SILT_GUY, CREATOR},
    {"mount", CFL_MOUNT, CREATOR},
    {"nogang", CFL_NOGANG, CREATOR},
    {"no_wish", CFL_NOWISH, OVERLORD},
    {"crim_allanak", CFL_CRIM_ALLANAK, STORYTELLER},
    {"crim_tuluk", CFL_CRIM_TULUK, STORYTELLER},
    {"crim_mal_krian", CFL_CRIM_MAL_KRIAN, STORYTELLER},
    {"crim_bw", CFL_CRIM_BW, STORYTELLER},
    {"crim_rs", CFL_CRIM_RS, STORYTELLER},
    {"crim_cai_shyzn", CFL_CRIM_CAI_SHYZN, STORYTELLER},
    {"flee", CFL_FLEE, CREATOR},
    {"unique", CFL_UNIQUE, CREATOR},
    {"dead", CFL_DEAD, OVERLORD},
    {"crim_luirs", CFL_CRIM_LUIRS, STORYTELLER},
    {"new_skill", CFL_NEW_SKILL, HIGHLORD},
    /* {"crim_cenyr", CFL_CRIM_CENYR, HIGHLORD},
     * {"thief", CFL_THIEF, CREATOR}, */
    {"infobar", CFL_INFOBAR, OVERLORD},
    {"", 0, 0},
    {"\n", 0, 0}
};

struct attribute_data weapon_type[] = {
    {"hit", TYPE_HIT, CREATOR},
    {"bludgeon", TYPE_BLUDGEON, CREATOR},
    {"pierce", TYPE_PIERCE, CREATOR},
    {"stab", TYPE_STAB, CREATOR},
    {"chop", TYPE_CHOP, CREATOR},
    {"slash", TYPE_SLASH, CREATOR},
    {"claw", TYPE_CLAW, CREATOR},
    {"whip", TYPE_WHIP, CREATOR},
    {"pinch", TYPE_PINCH, CREATOR},
    {"sting", TYPE_STING, CREATOR},
    {"bite", TYPE_BITE, CREATOR},
    {"peck", TYPE_PECK, CREATOR},
    {"pike", TYPE_PIKE, CREATOR},
    {"trident", TYPE_TRIDENT, CREATOR},
    {"polearm", TYPE_POLEARM, CREATOR},
    {"knife", TYPE_KNIFE, CREATOR},
    {"razor", TYPE_RAZOR, CREATOR},
    {"", 0, 0},
    {"\n", 0, 0}
};

struct flag_data grp_priv[] = {
    {"search", MODE_SEARCH, OVERLORD},
    {"control", MODE_CONTROL, OVERLORD},
    {"create", MODE_CREATE, OVERLORD},
    {"edit", MODE_STR_EDIT, OVERLORD},
    {"modify", MODE_MODIFY, OVERLORD},
    {"grant", MODE_GRANT, OVERLORD},
    {"load", MODE_LOAD, OVERLORD},
    {"write", MODE_WRITE, OVERLORD},
    {"open", MODE_OPEN, OVERLORD},
    {"close", MODE_CLOSE, OVERLORD},
    {"stat", MODE_STAT, OVERLORD},
    {"", 0, 0},
    {"\n", 0, 0}
};

const char * const position_types[] = {
    "dead",
    "mortally wounded",
    "stunned",
    "sleeping",
    "resting",
    "sitting",
    "fighting",
    "standing",
    "\n"
};

const char * const attributes[MAX_ATTRIBUTE] = {
    "strength",
    "agility",
    "wisdom",
    "endurance"
};

const char * const connected_types[] = {
/*  0  */
/*  1  */ "------Playing-------",
/*  2  */ "--Waiting for Name--",
/*  3  */ "--Waiting for Name--",
/*  4  */ "--Reading Password--",
/*  5  */ "--Reading Password--",
/*  6  */ "--Reading Password--",
/*  7  */ "-New Character Info-",
/*  8  */ "-----Main Menu------",
/*  9  */ "-----Main Menu------",
/* 10  */ "-New Character Info-",
/* 11  */ "-New Character Info-",
/* 12  */ "-New Character Info-",
/* 13  */ "--Reading Password--",
/* 14  */ "--Reading Password--",
/* 15  */ "------DikuNet-------",
/* 16  */ "-New Character Info-",
/* 17  */ "-------Editor-------",
/* 18  */ "-------Editor-------",
/* 19  */ "-----Main Menu------",
/* 20  */ "-New Character Info-",
/* 21  */ "---Documentation----",
/* 22  */ "---Documentation----",
/* 23  */ "-----Main Menu------",
/* 24  */ "----Reading MOTD----",
/* 25  */ "-New Character Info-",
/* 26  */ "-New Character Info-",
/* 27  */ "-New Character Info-",
/* 28  */ "-New Character Info-",
/* 29  */ "-New Character Info-",
/* 30  */ "-New Character Info-",
/* 31  */ "-New Character Info-",
/* 32  */ "-New Character Info-",
/* 33  */ "-New Character Info-",
/* 34  */ "-New Character Info-",
/* 35  */ "-New Character Info-",
/* 36  */ "-New Character Info-",
/* 37  */ "-New Character Info-",
/* 38  */ "-New Character Info-",
/* 39  */ "-----ANSI Check-----",
/* 40  */ "-New Character Info-",
/* 41  */ "-New Character Info-",
/* 42  */ "----Showing Char----",
/* 43  */ "43",
/* 44  */ "44",
/* 45  */ "45",
/* 46  */ "46",
/* 47  */ "47",
/* 48  */ "48",
/* 49  */ "49",
/* 50  */ "50",
/* 51  */ "------Mail Menu-----",
/* 52  */ "---CON_MAIL_LIST----",
/* 53  */ "---CON_MAIL_READ_D--",
/* 54  */ "---CON_MAIL_READ_S--",
/* 55  */ "---CON_MAIL_DELETE--",
/* 56  */ "---CON_MAIL_SEND----",
/* 57  */ "------Mail Menu-----",
/* 58  */ "58",
/* 59  */ "59",
/* 60  */ "60",
/* 61  */ "61",
/* 62  */ "62",
/* 63  */ "63",
/* 64  */ "64",
/* 65  */ "65",
/* 66  */ "66",
/* 67  */ "67",
/* 68  */ "68",
/* 69  */ "----Email Verify----",
/* 70  */ "-Entering Objective-",
/* 71  */ "----Entering Age----",
/* 72  */ "-----BACKGROUND-----",
/* 73  */ "---BACKGROUND END---",
/* 74  */ "--Mul Slave choice--",
/* 75  */ "--Sorceror Choice---",
/* 76  */ "--Get Account Name--",
/* 77  */ "-Confirm New Account",
/* 78  */ "--Account Password--",
/* 79  */ "New Account Password",
/* 80  */ "Confirm Acc Password",
/* 81  */ "---Wait Disconnect--",
/* 82  */ "---Confirm Delete---",
/* 83  */ "--Revise Char Menu--",
/* 84  */ "----Options Menu----",
/* 85  */ "---Get Page Length--",
/* 86  */ "----Get Edit End----",
/* 87  */ "-Connect Skip Psswd-",
/* 88  */ "----Where Survey----",
/* 89  */ "-Second Skill Pack--",
/* 90  */ "----Noble Choice----",
/* 92  */ "-----Psi Choice-----",
/* 93  */ "----Stat Ordering---",
/* 94  */ "-Confirm Stat Order-",
/* 95  */ "----Confirm Exit----",
/* 96  */ "\n\r",
/* 97  */ "---Origin Location--",
/* 98  */ "---Tattoo Location--"
};

char BODY[] = "body";
char HEAD[] = "head";
char NECK[] = "neck";
char HINDLEG[] = "hindleg";
char LEG[] = "leg";
char MIDLEG[] = "midleg";
char SHIELD[] = "shield";
char WRIST[] = "wrist";
char TAIL[] = "tail";
char BACK[] = "back";
char ARM[] = "arm";
char NN[] = "";
/* Hit locations based on race type */
const char * const hit_loc_name[20][10] = {
    {NN, NN, NN, NN, NN, NN, NN, NN, NN, NN},
    {NN, NN, NN, NN, NN, NN, NN, NN, NN, NN},
    {NN, NN, NN, NN, NN, NN, NN, NN, NN, NN},
    {NECK, NECK, NECK, NECK, NECK, HEAD, NECK, "trunk", HEAD, NECK},
    {BACK, BACK, BACK, "thorax", BACK, BACK, BACK, "trunk", "abdomen", BACK},
    {BODY, BODY, BODY, "thorax", BODY, BODY, BODY, "trunk", "abdomen", BODY},
    {NN, HEAD, HEAD, HEAD, HEAD, HEAD, HEAD, "trunk", HEAD, HEAD},
    {NN, LEG, HINDLEG, HINDLEG, LEG, NN, HINDLEG, NN, HINDLEG, LEG},
    {NN, "foot", "paw", "hindclaw", "claw", NN, "foot", "root", "hindclaw", "claw"},
    {NN, "hand", "paw", "claw", "wing", NN, "claw", NN, "claw", "wing"},
    {NN, ARM, "foreleg", ARM, "wing", NN, ARM, "branch", LEG, "wing"},
    {NN, NN, NN, NN, NN, NN, NN, NN, NN, NN},
    {NN, NN, NN, NN, NN, NN, NN, NN, NN, NN},
    {NN, "waist", TAIL, "abdomen", TAIL, TAIL, TAIL, NN, "abdomen", TAIL},
    {NN, WRIST, NN, MIDLEG, NN, NN, WRIST, NN, MIDLEG, NN},
    {NN, WRIST, NN, MIDLEG, NN, NN, WRIST, NN, MIDLEG, NN},
    {NN, SHIELD, NN, SHIELD, NN, NN, SHIELD, NN, NN, NN},
    {NN, SHIELD, NN, SHIELD, NN, NN, SHIELD, NN, NN, NN},
    {NN, NN, NN, NN, NN, NN, NN, NN, NN, NN},
    {NN, SHIELD, NN, SHIELD, NN, NN, SHIELD, NN, NN, NN}
};


/* saving throws based on race */
/* para, rod, petri, breath, spell */
int saving_throw[11][5] = {
    {50, 50, 50, 50, 50},       /* other */
    {55, 65, 60, 40, 45},       /* human */
    {45, 65, 50, 45, 50},       /* elf */
    {60, 35, 65, 45, 75},       /* dwarf */
    {85, 55, 50, 50, 75},       /* halfling */
    {75, 45, 65, 55, 35},       /* half-giant */
    {50, 65, 55, 40, 50},       /* half-elf */
    {60, 50, 60, 45, 55},       /* mul */
    {65, 60, 70, 55, 55},       /* mantis */
    {55, 65, 50, 55, 45},       /* gith */
    {90, 90, 90, 90, 90}        /* drg, ele, avg */
};


/*  strength apply  */
/*  to_dam, carry_w, bend_break  */
struct str_app_type str_app[101] = {
    {-9, 1, -1},                /* 0 */
    {-6, 5, 0},
    {-5, 10, 0},
    {-4, 20, 0},                /* 3 */
    {-4, 35, 0},
    {-3, 50, 0},
    {-3, 65, 0},                /* 6 */
    {-2, 80, 0},
    {-2, 95, 0},
    {-1, 105, 1},               /* 9 */
    {0, 110, 2},
    {0, 115, 3},
    {0, 117, 4},                /* 12 */
    {0, 120, 6},
    {0, 125, 9},
    {1, 140, 12},               /* 15 */
    {1, 155, 18},
    {2, 170, 24},
    {3, 195, 32},               /* 18 */
    {4, 230, 40},
    {5, 290, 48},
    {6, 380, 56},               /* 21 */
    {7, 430, 64},
    {8, 500, 72},
    {9, 640, 80},               /* 24 */
    {10, 680, 80},              /* 25 */
    {10, 700, 80},
    {11, 740, 80},
    {11, 780, 80},
    {12, 800, 80},
    {12, 820, 80},              /* 30 */
    {13, 840, 80},
    {13, 860, 80},
    {13, 880, 80},
    {14, 900, 80},
    {14, 910, 80},              /* 35 */
    {14, 920, 80},
    {14, 930, 80},
    {14, 940, 80},
    {15, 950, 80},
    {15, 960, 80},              /* 40 */
    {15, 970, 80},
    {15, 980, 80},
    {15, 990, 80},
    {16, 1000, 80},
    {16, 1010, 80},             /* 45 */
    {16, 1020, 80},
    {16, 1030, 80},
    {16, 1040, 80},
    {17, 1050, 80},
    {17, 1060, 80},             /* 50 */
    {17, 1070, 80},
    {17, 1080, 80},
    {17, 1090, 80},
    {18, 1100, 80},
    {18, 1110, 80},             /* 55 */
    {18, 1120, 80},
    {18, 1130, 80},
    {18, 1140, 80},
    {18, 1150, 80},
    {19, 1160, 80},             /* 60 */
    {19, 1170, 80},
    {19, 1200, 80},
    {20, 1220, 80},
    {20, 1240, 80},
    {20, 1260, 80},             /* 65 */
    {20, 1280, 80},
    {21, 1300, 80},
    {21, 1340, 80},
    {22, 1380, 80},
    {22, 1420, 80},             /* 70 */
    {23, 1460, 80},
    {23, 1500, 80},
    {24, 1540, 80},
    {24, 1580, 80},
    {25, 1620, 80},             /* 75 */
    {25, 1660, 80},
    {26, 1700, 80},
    {27, 1740, 80},
    {28, 1780, 80},
    {29, 1820, 80},             /* 80 */
    {30, 1860, 80},
    {31, 1900, 80},
    {32, 1940, 80},
    {33, 1980, 80},
    {34, 2020, 80},             /* 85 */
    {35, 2060, 80},
    {36, 2100, 80},
    {37, 2140, 80},
    {38, 2180, 80},
    {39, 2220, 80},             /* 90 */
    {40, 2260, 80},
    {42, 2300, 80},
    {44, 2400, 80},
    {46, 2500, 80},
    {48, 2600, 80},             /* 95 */
    {50, 2700, 80},
    {54, 2800, 80},
    {58, 2900, 80},
    {65, 3000, 80},
    {70, 5000, 90}              /* 100 */
};


/*  agility apply  */
/*  react, stealth_manip, carry number, missile bonus c_delay */
struct agl_app_type agl_app[101] = {
    {0, 0, 0, 0, 15},           /* 0 */
    {-30, -50, 1, -30, 10},
    {-20, -40, 2, -25, 8},
    {-15, -30, 2, -20, 7},      /* 3 */
    {-10, -25, 2, -15, 5},
    {-5, -20, 3, -10, 5},
    {-5, -15, 3, -5, 5},        /* 6 */
    {0, -10, 4, -5, 4},
    {0, -5, 4, 0, 4},
    {0, 0, 5, 0, 4},            /* 9 */
    {0, 0, 5, 0, 4},
    {0, 0, 6, 0, 3},
    {0, 0, 6, 0, 3},            /* 12 */
    {0, 5, 7, 0, 3},
    {0, 5, 7, 0, 3},
    {0, 5, 8, 5, 3},            /* 15 */
    {5, 10, 8, 5, 2},
    {5, 15, 8, 5, 2},
    {5, 20, 9, 10, 2},          /* 18 */
    {10, 25, 9, 10, 2},
    {10, 25, 9, 15, 2},
    {10, 25, 10, 15, 2},        /* 21 */
    {15, 25, 10, 20, 2},
    {15, 30, 10, 20, 1},
    {20, 35, 11, 25, 1},        /* 24 */
    {25, 40, 12, 25, 1},        /* 25 */
    {28, 40, 12, 28, 1},
    {28, 40, 12, 28, 1},
    {28, 40, 13, 28, 1},
    {28, 40, 13, 28, 1},
    {30, 40, 13, 30, 1},        /* 30 */
    {30, 40, 13, 30, 1},
    {30, 40, 13, 30, 1},
    {30, 40, 13, 30, 1},
    {30, 40, 14, 32, 1},
    {35, 40, 14, 32, 1},        /* 35 */
    {35, 40, 14, 32, 1},
    {35, 40, 14, 32, 1},
    {35, 40, 14, 35, 1},
    {35, 40, 14, 35, 1},
    {40, 40, 14, 35, 1},        /* 40 */
    {40, 40, 15, 35, 1},
    {40, 40, 15, 38, 1},
    {40, 40, 15, 38, 1},
    {40, 40, 15, 38, 1},
    {45, 40, 15, 38, 1},        /* 45 */
    {45, 40, 15, 40, 1},
    {45, 40, 15, 40, 1},
    {45, 40, 15, 40, 1},
    {45, 40, 15, 40, 1},
    {50, 40, 15, 43, 1},        /* 50 */
    {50, 40, 15, 43, 1},
    {50, 40, 15, 45, 1},
    {50, 40, 15, 45, 1},
    {50, 40, 16, 47, 1},
    {55, 40, 16, 47, 1},        /* 55 */
    {55, 40, 16, 49, 1},
    {55, 40, 16, 49, 1},
    {55, 40, 16, 51, 1},
    {55, 40, 16, 51, 1},
    {60, 40, 16, 53, 1},        /* 60 */
    {60, 40, 16, 53, 1},
    {60, 40, 16, 55, 1},
    {60, 40, 16, 55, 1},
    {60, 40, 16, 57, 1},
    {65, 40, 16, 57, 1},        /* 65 */
    {65, 40, 16, 59, 1},
    {65, 40, 16, 59, 1},
    {65, 40, 16, 61, 1},
    {65, 40, 16, 61, 1},
    {70, 40, 17, 63, 1},        /* 70 */
    {70, 40, 17, 63, 1},
    {70, 40, 17, 65, 1},
    {70, 40, 17, 65, 1},
    {70, 40, 17, 67, 1},
    {75, 40, 17, 67, 1},        /* 75 */
    {75, 40, 17, 69, 1},
    {75, 40, 17, 69, 1},
    {75, 40, 17, 71, 1},
    {75, 40, 17, 71, 1},
    {80, 40, 17, 73, 1},        /* 80 */
    {80, 40, 17, 73, 1},
    {80, 40, 17, 75, 1},
    {80, 40, 17, 75, 1},
    {80, 40, 17, 77, 1},
    {85, 40, 17, 77, 1},        /* 85 */
    {85, 40, 17, 79, 1},
    {85, 40, 18, 79, 1},
    {85, 40, 18, 81, 1},
    {85, 40, 18, 81, 1},
    {90, 40, 18, 83, 1},        /* 90 */
    {90, 40, 18, 83, 1},
    {90, 40, 18, 85, 1},
    {90, 40, 18, 85, 1},
    {90, 40, 19, 87, 1},
    {95, 40, 19, 87, 1},        /* 95 */
    {95, 40, 19, 89, 1},
    {95, 40, 19, 89, 1},
    {95, 40, 19, 91, 1},
    {95, 40, 19, 91, 1},
    {100, 100, 20, 100, 0}      /* 100 */
};


/* endurance apply  */
/* saving throws, system shock, move bonus */
struct end_app_type end_app[101] = {
    {-50, 0, 0},                /* 0 */
    {-50, 10, 1},               /* 1 */
    {-40, 20, 2},
    {-30, 30, 3},
    {-25, 40, 4},
    {-20, 45, 5},               /* 5 */
    {-15, 50, 6},
    {-10, 55, 7},
    {-5, 60, 8},
    {0, 65, 9},
    {0, 70, 10},                /* 10 */
    {0, 75, 11},
    {0, 80, 12},
    {5, 85, 13},
    {5, 88, 14},
    {5, 90, 15},                /* 15 */
    {10, 95, 17},
    {15, 97, 19},
    {20, 99, 21},
    {25, 99, 23},
    {25, 99, 25},               /* 20 */
    {30, 99, 28},
    {30, 99, 31},
    {35, 99, 34},
    {40, 99, 37},
    {42, 100, 40},              /* 25 */
    {43, 100, 44},
    {43, 100, 48},
    {44, 100, 52},
    {44, 100, 56},
    {45, 100, 58},              /* 30 */
    {45, 100, 60},
    {46, 100, 62},
    {46, 100, 64},
    {47, 100, 66},
    {47, 100, 68},              /* 35 */
    {48, 100, 70},
    {48, 100, 72},
    {49, 100, 74},
    {49, 100, 76},
    {50, 100, 78},              /* 40 */
    {50, 100, 80},
    {51, 100, 82},
    {51, 100, 84},
    {52, 100, 86},
    {52, 100, 88},              /* 45 */
    {53, 100, 90},
    {53, 100, 92},
    {50, 100, 94},
    {50, 100, 96},
    {50, 100, 98},              /* 50 */
    {50, 100, 100},
    {50, 100, 102},
    {50, 100, 104},
    {50, 100, 106},
    {50, 100, 108},             /* 55 */
    {50, 100, 110},
    {50, 100, 112},
    {50, 100, 114},
    {50, 100, 116},
    {50, 100, 118},             /* 60 */
    {50, 100, 120},
    {50, 100, 122},
    {50, 100, 124},
    {50, 100, 126},
    {50, 100, 128},             /* 65 */
    {50, 100, 130},
    {50, 100, 132},
    {50, 100, 134},
    {50, 100, 136},
    {50, 100, 138},             /* 70 */
    {50, 100, 140},
    {50, 100, 142},
    {50, 100, 144},
    {50, 100, 146},
    {50, 100, 148},             /* 75 */
    {50, 100, 150},
    {50, 100, 152},
    {50, 100, 154},
    {50, 100, 156},
    {50, 100, 158},             /* 80 */
    {50, 100, 160},
    {50, 100, 162},
    {50, 100, 164},
    {50, 100, 166},
    {50, 100, 168},             /* 85 */
    {50, 100, 170},
    {50, 100, 174},
    {50, 100, 178},
    {50, 100, 182},
    {50, 100, 186},             /* 90 */
    {50, 100, 190},
    {50, 100, 191},
    {50, 100, 192},
    {50, 100, 193},
    {50, 100, 194},             /* 95 */
    {50, 100, 195},
    {50, 100, 196},
    {50, 100, 197},
    {50, 100, 198},
    {50, 100, 200}              /* 100 */
};


/* wisdom apply  */
/* learn, perception, saves */
struct wis_app_type wis_app[101] = {
    {0, 0, 0},                  /* 0 */
    {1, -30, -50},
    {2, -20, -40},
    {4, -15, -30},              /* 3 */
    {6, -10, -25},
    {8, -5, -20},
    {10, -5, -15},              /* 6 */
    {12, 0, -10},
    {14, 0, -5},
    {16, 0, 0},                 /* 9 */
    {18, 0, 0},
    {20, 0, 0},
    {24, 0, 0},                 /* 12 */
    {28, 0, 0},
    {32, 0, 0},
    {38, 0, 5},                 /* 15 */
    {44, 5, 10},
    {50, 5, 15},
    {56, 5, 20},                /* 18 */
    {62, 10, 25},
    {68, 10, 30},
    {74, 10, 35},               /* 21 */
    {80, 15, 40},
    {86, 15, 45},
    {92, 20, 50},               /* 24 */
    {92, 20, 50},               /* 25 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 30 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 35 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 40 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 45 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 50 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 55 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 60 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 65 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 70 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 75 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 80 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 85 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 90 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},               /* 95 */
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {92, 20, 50},
    {100, 25, 60}               /* 100 */
};

#ifdef CLANS_FROM_DB
/* when adding to this list, also add to clan_name_code below
   without spaces and such, for use in dmpl
*/
const char * const clan_name[] = {
    "None",
    "Kutptei Clan",             /* tuluk noble clan */
    "House Kadius",
    "House Salarr",
    "House Kurac",
    "House Voryek",             /*  5  */
    "Black Moon Raiders",
    "Cai-Shyzn Pack",
    "Utep Sun Clan",            /* tuluk ruling clan */
    "PtarKen Tribe",
    "Order of Vivadu",          /*  10  */
    "Haruch Kemad",
    "Arm of the Dragon",
    "Black Hand",
    "Conclave",
    "Oriya Clan",               /*  15  *//* tuluk noble clan */
    "Order of the Elements",
    "House Tor",                /* allanak noble house */
    "Sun Runners",              /* Elven tribe */
    "Ironsword",
    "The Guild",                /*  20  */
    "Society of the Vault",
    "House Fale",               /* allanak noble house */
    "House Borsail",            /* allanak noble house */
    "Tan Muark",
    "Blackwing Tribe",          /*  25  */
    "Servants of House Fale",
    "Obsidian Order",
    "House Kohmar",
    "T'zai Byn",
    "Silt-Jackals",             /*  30  */
    "Southland Gith Clan",
    "Servants of the Shifting Sands",
    "Daine",
    "Allanaki Liberation Alliance",
    "Steinel Ruins Elves",      /*  35  */
    "House Oash",               /* allanak noble house */
    "Slaves of House Oash",
    "Slaves of House Tor",
    "Servants of Borsail",
    "Hand of the Dragon",       /*  40  */
    "Servants of Oash",
    "Cult of the Dragon",
    "Followers of the Lady",
    "Feydakin",
    "Plainsfolk",               /* 45 */
    "Hillpeople",
    "House Reynolte",           /* Tuluki noble house */
    "Gith Mesa Clan",
    "Guards of House Reynolte",
    "Slaves of House Borsail",  /* 50 */
    "Slaves of Allanak",
    "Slaves of House Reynolte",
    "Slaves of House Fale",
    "Seven Spear",
    "Baizan Nomadic Tribes",    /* 55 */
    "Thornwalkers",
    "House Delann",
    "Tor Scorpions",            /* Tor Guards, basically */
    "Slaves of Allanak",        /* General use clan */
    "Servants of Gol Krathu",   /* 60 */
    "Tar Kroh",
    "Kuraci Archives",
    "Rebellion",                /* Tuluki Rebellion clan */
    "Akei'ta Var",              /* Magicky/preserverlike delf tribe */
    "Red Storm Militia",        /* 65 */
    "House Kurac Military Operations",  /* Kurac Guards and Mercs */
    "Desert Talkers",           /* Desert Talkers, elf tribe */
    "House Nenyuk",             /* Banker clan */
    "Dust Runners",             /* Secret Kuraci smugglers */
    "al Seik",                  /* Tablelands tribe */
    "Benjari",                  /* Tablelands tribe */
    "Arabet",                   /* Tablelands tribe */
    "Spirit Trackers",          /* Spirit Trackers */
    "House Morlaine",           /* Allanak Noble */
    "Servants of House Morlaine",       /* Allanak Noble Servants */
    "House Winrothol",          /* Tuluki noble house */
    "Servants of House Winrothol",      /* their servants */
    "Two Moons",
    "The Pride",
    "Kre'Tahn",
    "Soh Lanah Kah",
    "Servants of Nenyuk",
    "Tuluki Templarate",
    "House Tenneshi",
    "Servants of Tenneshi",
    "Silt Winds",
    "Bards of Poets' Circle",
    "Blackmarketeers",
    "Servants of the Dragon",
    "Cenyr",
    "Fah Akeen",
    "The Atrium",
    "Ruene",
    "Dune Stalker",
    "Sand Jakhals",
    "Red Fangs",
    "Sandas",
    "Jul Tavan",
    "Zimand Gur",
    "Allanak Militia Recruits", /* 100 I believe */
    "Guests of the Atrium",
    "House Negean",
    "Servants of House Negean",
    "House Dasari",
    "Servants of House Dasari",
    "House Kassigarh",
    "Servants of House Kassigarh",
    "House Uaptal",
    "Servants of House Uaptal",
    "House Lyksae",
    "Servants of House Lyksae",
    "\n"
};


const char * const clan_name_code[] = {
    "none",
    "kutptei_clan",             /* tuluk noble clan */
    "house_kadius",
    "house_salarr",
    "house_kurac",
    "house_voryek",             /*  5  */
    "black_moon_raiders",
    "cai_shyzn_pack",
    "utep_sun_clan",            /* tuluk ruling clan */
    "ptarken_tribe",
    "order_of_vivadu",          /*  10  */
    "haruch_kemad",
    "arm_of_the_dragon",
    "black_hand",
    "conclave",
    "oriya_clan",               /*  15  *//* tuluk noble clan */
    "order_of_the_elements",
    "house_tor",                /* allanak noble house */
    "sun_runners",              /* Elven tribe */
    "ironsword",
    "the_guild",                /*  20  */
    "society_of_the_vault",
    "house_fale",               /* allanak noble house */
    "house_borsail",            /* allanak noble house */
    "tan_muark",
    "blackwing_tribe",          /*  25  */
    "house_fale_guards",
    "obsidian_order",
    "house_kohmar",
    "tzai_byn",
    "silt_jackals",             /*  30  */
    "southland_gith_clan",
    "servants_of_the_shifting_sands",
    "daine",
    "allanaki_liberation_alliance",
    "steinel_ruins_elves",      /*  35  */
    "house_oash",               /* allanak noble house */
    "slaves_of_house_oash",
    "slaves_of_house_tor",
    "servants_of_borsail",
    "hand_of_the_dragon",       /*  40  */
    "servants_of_oash",
    "cult_of_the_dragon",
    "followers_of_the_lady",
    "feydakin",
    "plainsfolk",               /* 45 */
    "hillpeople",
    "hous_reynolte",            /* Tuluki noble house */
    "gith_mesa_clan",
    "guards_of_house_reynolte",
    "slaves_of_house_borsail",  /* 50 */
    "slaves_of_allanak",
    "slaves_of_house_reynolte",
    "slaves_of_house_fale",
    "seven_spear",
    "baizan_nomadic_tribes",    /* 55 */
    "thornwalkers",
    "house_delann",
    "tor_scorpions",            /* Tor Guards, basically */
    "slaves_of_allanak",        /* General use clan */
    "servants_of_gol_krathu",
    "the_rebellion",            /* Tuluki Rebellion clan */
    "akei_ta_var",              /* Magicky/preserver'ish delf tribe */
    "dust_runners",             /* Secret Kuraci smugglers */
    "al_seik",                  /* Tablelands tribe */
    "benjari",                  /* Tablelands tribe */
    "arabet",                   /* Tablelands tribe */
    "spirit_trackers",
    "house_morlaine",
    "servants_of_house_morlaine",
    "house_winrothol",
    "servants_of_house_winrothol",
    "two_moons",
    "the_pride",
    "kre_tahn",
    "soh_lanah_kah",
    "servants_of_nenyuk",
    "tuluki_templarate",
    "house_tenneshi",
    "servants_of_house_tenneshi",
    "silt_winds",
    "bards_of_poets_circle",
    "blackmarketeers",
    "servants_of_the_dragon",
    "cenyr",
    "fah_akeen",
    "the_atrium",
    "ruene",
    "dune_stalker",
    "sand_jakhals",
    "red_fangs",
    "sandas",
    "jul_tavan",
    "zimand_gur",
    "allanak_militia_recruits", /* 100 I believe */
    "guests_of_the_atrium",
    "house_negean",
    "servants_of_house_negean",
    "house_dasari",
    "servants_of_house_dasari",
    "house_kassigarh",
    "servants_of_house_kassigarh",
    "house_uaptal",
    "servants_of_house_uaptal",
    "house_lyksae",
    "servants_of_house_lyksae",
    "\n"
};
#endif

const char * const mortal_access[] = {
    "kored",
    "kiril",
    "bashare",
    "silk",
    "questmaster",
    "aisynthe",
    "mandolin",
    "jethred",
    "anduin",
    "daven",
    "zaerach",
    "valerosa",
    "casana",
    "reggie",
    "sartakryn",
    "epis",
    "epsithian",
    "tyras",
    "tulkus",
    "samah",
    "mustapha",
    "lunater",                  /*  Black Moon begin  */
    "lethis",
    "thurok",
    "choralone",                /* Black Moon end */
    "slespid",                  /* tuluk templars end here */
    "marit",
    "ournep",
    "uinael",
    "kobura",
    "katarina",
    "lucretia",
    "khann",
    "shaia",                    /* Salarr start */
    "guthwulf",
    "gregory",
    "orius",                    /* Salarr end */
    "\n"
};

/* working on rewording these, see below
const char * const how_good_stat[] =
{
  "very poor",
  "poor",
  "below average",
  "average",
  "above average",
  "good",
  "very good",
  "extremely good",
  "absolutely incredible"
};
*/

const char * const how_good_stat[] = {
    "poor",                     /*     <   0% */
    "below average",            /*  0% -  14% */
    "average",                  /* 15% -  28% */
    "above average",            /* 29% -  42% */
    "good",                     /* 43% -  57% */
    "very good",                /* 58% -  71% */
    "extremely good",           /* 72% -  85% */
    "exceptional",              /* 86% - 100% */
    "absolutely incredible"     /*     > 100% */
};

const char * const how_good_skill[] = {
    "novice",		// < 20%
    "apprentice",	// 20 - 39%
    "journeyman",	// 40 - 59%
    "advanced",         // 60 - 79%
    "master"		// >= 80%
};

const char * const Crime[] = {
    "unknown",
    "assault",
    "theft",
    "possession of contraband",
    "illegal magick use",
    "illegal weapons sales",
    "treason",
    "\n"
};

const char * const MagickType[] = {
    "none",
    "another force",
    "object",
    "sorcerer",
    "elementalist",
    "psionicist",
    "\n"
};

const char * const Power[] = {
    "wek",
    "yuqa",
    "kral",
    "een",
    "pav",
    "sul",
    "mon",
    "\n"
};

const char * const Reach[] = {
    "un",                       /* local REACH_LOCAL   */
    "nil",                      /* none  REACH_NONE  */
    "oai",                      /* potions REACH_POTION */
    "tuk",                      /* scrolls  REACH_SCROLL */
    "phan",                     /* wands   REACH_WAND */
    "mur",                      /* staffs  REACH_STAFF */
    "kizn",                     /* room    REACH_ROOM */
    "xaou",                     /* distance REACH_SILENT */
    "\n"
};

const char * const Element[] = {
    "suk-krath",
    "vivadu",
    "ruk",
    "krok",
    "whira",
    "drov",
    "elkros",
    "nilaz",
    "\n"
};

const char * const Sphere[] = {
    "pret",
    "mutur",
    "dreth",
    "psiak",
    "iluth",
    "divan",
    "threl",
    "morz",
    "viqrol",
    "nathro",
    "locror",
    "wilith",
    "\n"
};

const char * const Mood[] = {
    "hekro",
    "grol",
    "chran",
    "echri",
    "wril",
    "nikiz",
    "hurn",
    "inrof",
    "viod",
    "hefre",
    "udcha",
    "\n"
};

struct attribute_data commodities[] = {
    {"None", COMMOD_NONE, CREATOR},
    {"Spice", COMMOD_SPICE, CREATOR},
    {"Gems", COMMOD_GEM, CREATOR},
    {"Fur", COMMOD_FUR, CREATOR},
    {"Weapon", COMMOD_WEAPON, CREATOR},
    {"Armor", COMMOD_ARMOR, CREATOR},
    {"Metal", COMMOD_METAL, CREATOR},
    {"Wood", COMMOD_WOOD, CREATOR},
    {"Magick", COMMOD_MAGICK, CREATOR},
    {"Art", COMMOD_ART, CREATOR},
    {"Obsidian", COMMOD_OBSIDIAN, CREATOR},
    {"Chitin", COMMOD_CHITIN, CREATOR},
    {"Bone", COMMOD_BONE, CREATOR},
    {"Glass", COMMOD_GLASS, CREATOR},
    {"Silk", COMMOD_SILK, CREATOR},
    {"Ivory", COMMOD_IVORY, CREATOR},
    {"Tortoiseshell", COMMOD_TORTOISESHELL, CREATOR},
    {"Duskhorn", COMMOD_DUSKHORN, CREATOR},
    {"Isilt", COMMOD_ISILT, CREATOR},
    {"Salt", COMMOD_SALT, CREATOR},
    {"Gwoshi", COMMOD_GWOSHI, CREATOR},
    {"Paper", COMMOD_PAPER, CREATOR},
    {"Food", COMMOD_FOOD, CREATOR},
    {"Feather", COMMOD_FEATHER, CREATOR},
    {"", 0, 0},
};

struct group_data grp[] = {
    {"Public", GROUP_PUBLIC},
    {"Allanak Quest Team", GROUP_ALLANAK},
    {"Tuluk Quest Team", GROUP_TULUK},
    {"Desert Quest Team", GROUP_DESERT},
    {"North Desert Quest Team", GROUP_NDESERT},
    {"Client Village Team", GROUP_VILLAGES},
    {"Player House Team", GROUP_HOUSES},
    {"Tablelands Group", GROUP_TLANDS},
    {"Sea of Silt Group", GROUP_SEA},
    {"Aerial Quest Team", GROUP_AIR},
    {"Lanthil Quest Team", GROUP_LANTHIL},
    {"None", 0}
};

const char * const class_name[] = {
    "None",
    "Magick",
    "Combat",
    "Stealth",
    "Manipulation",
    "Perception",
    "Barter",
    "Spice",
    "Language",
    "Weapon",
    "Psionic",
    "Tolerance",
    "Knowledge",
    "Craft",

    "\n"
};

struct flag_data liquid_flags[] = {
    {"poisoned", LIQFL_POISONED, CREATOR},
    {"infinite", LIQFL_INFINITE, CREATOR},
    {"", 0, 0}
};

struct flag_data food_flags[] = {
    {"poisoned", FOOD_POISONED, CREATOR},
    {"", 0, 0}
};

struct flag_data container_flags[] = {
    {"closeable", CONT_CLOSEABLE, CREATOR},
    {"pickproof", CONT_PICKPROOF, CREATOR},
    {"closed", CONT_CLOSED, CREATOR},
    {"locked", CONT_LOCKED, CREATOR},
    {"trapped", CONT_TRAPPED, CREATOR},
    {"corpse_head", CONT_CORPSE_HEAD, CREATOR},
    {"cloak", CONT_CLOAK, CREATOR},
    {"hooded", CONT_HOODED, CREATOR},
    {"hood_up", CONT_HOOD_UP, OVERLORD},
    {"key_ring", CONT_KEY_RING, CREATOR},
    {"sealed", CONT_SEALED, CREATOR},
    {"seal_broken", CONT_SEAL_BROKEN, CREATOR},
    {"", 0, 0}
};

struct flag_data worn_flags[] = {
    {"cloak", CONT_CLOAK, CREATOR},
    {"closed", CONT_CLOSED, CREATOR},
    {"hooded", CONT_HOODED, CREATOR},
    {"hood_up", CONT_HOOD_UP, OVERLORD},
    {"show_contents", CONT_SHOW_CONTENTS, CREATOR},
    {"", 0, 0}
};

struct flag_data musical_flags[] = {
    {"show_contents", CONT_SHOW_CONTENTS, CREATOR},
    {"", 0, 0}
};

struct flag_data jewelry_flags[] = {
    {"show_contents", CONT_SHOW_CONTENTS, CREATOR},
    {"", 0, 0}
};

struct flag_data mineral_type[] = {
    {"none", MINERAL_NONE, CREATOR},
    {"stone", MINERAL_STONE, CREATOR},
    {"ash", MINERAL_ASH, CREATOR},
    {"salt", MINERAL_SALT, CREATOR},
    {"essence", MINERAL_ESSENCE, CREATOR},
    {"magickal", MINERAL_MAGICKAL < CREATOR},
    {"", 0, 0}
};

struct flag_data powder_type[] = {
    {"none", POWDER_NONE, CREATOR},
    {"fine", POWDER_FINE, CREATOR},
    {"grit", POWDER_GRIT, CREATOR},
    {"tea", POWDER_TEA, CREATOR},
    {"salt", POWDER_SALT, CREATOR},
    {"essence", POWDER_ESSENCE, CREATOR},
    {"magickal", POWDER_MAGICKAL, CREATOR},
    {"", 0, 0}
};

struct attribute_data instrument_type[] = {
    {"none", INSTRUMENT_NONE, CREATOR},
    {"bells", INSTRUMENT_BELLS, CREATOR},
    {"brass", INSTRUMENT_BRASS, CREATOR},
    {"cthulhu", INSTRUMENT_CTHULHU, CREATOR},
    {"drone", INSTRUMENT_DRONE, CREATOR},
    {"gong", INSTRUMENT_GONG, CREATOR},
    {"gourd", INSTRUMENT_GOURD, CREATOR},
    {"large drum", INSTRUMENT_LARGE_DRUM, CREATOR},
    {"magick flute", INSTRUMENT_MAGICK_FLUTE, CREATOR},
    {"magick horn", INSTRUMENT_MAGICK_HORN, CREATOR},
    {"percussion", INSTRUMENT_PERCUSSION, CREATOR},
    {"rattle", INSTRUMENT_RATTLE, CREATOR},
    {"small drum", INSTRUMENT_SMALL_DRUM, CREATOR},
    {"stringed", INSTRUMENT_STRINGED, CREATOR},
    {"whistle", INSTRUMENT_WHISTLE, CREATOR},
    {"wind", INSTRUMENT_WIND, CREATOR},
    {"", 0, 0}
};

struct flag_data furniture_flags[] = {
    {"sit", FURN_SIT, CREATOR},
    {"rest", FURN_REST, CREATOR},
    {"sleep", FURN_SLEEP, CREATOR},
    {"stand", FURN_STAND, CREATOR},
    {"put", FURN_PUT, CREATOR},
    {"table", FURN_TABLE, CREATOR},
    {"skimmer", FURN_SKIMMER, CREATOR},
    {"wagon", FURN_WAGON, CREATOR},
    {"", 0, 0}
};

struct attribute_data playable_type[] = {
    {"none", PLAYABLE_NONE, CREATOR},
    {"dice", PLAYABLE_DICE, CREATOR},
    {"weighted_dice", PLAYABLE_DICE_WEIGHTED, CREATOR},
    {"bones", PLAYABLE_BONES, CREATOR},
    {"halfling_stones", PLAYABLE_HALFLING_STONES, CREATOR},
    {"gypsy_dice", PLAYABLE_GYPSY_DICE, CREATOR},
    {"", 0, 0}
};

/* struct attribute_data wagon_type[] = */
struct flag_data wagon_flags[] = {
    {"land", WAGON_CAN_GO_LAND, CREATOR},
    {"silt", WAGON_CAN_GO_SILT, CREATOR},
    {"air", WAGON_CAN_GO_AIR, CREATOR},
    {"no_leave", WAGON_NO_LEAVE, CREATOR},
    {"reins_land", WAGON_REINS_LAND, CREATOR},
    {"no_mounts", WAGON_MOUNT_NO, CREATOR},
    {"no_enter", WAGON_NO_ENTER, CREATOR},
    {"no_psi_path", WAGON_NO_LEAVE, CREATOR},
    {"", 0, 0}
};


struct flag_data note_flags[] = {
    {"sealed", NOTE_SEALED, CREATOR},
    {"broken", NOTE_BROKEN, CREATOR},
    {"sealable", NOTE_SEALABLE, CREATOR},
    {"", 0, 0}
};

struct attribute_data light_type[] = {
    {"flame", LIGHT_TYPE_FLAME, CREATOR},
    {"magick", LIGHT_TYPE_MAGICK, CREATOR},
    {"plant", LIGHT_TYPE_PLANT, CREATOR},
    {"mineral", LIGHT_TYPE_MINERAL, CREATOR},
    {"animal", LIGHT_TYPE_ANIMAL, CREATOR},
    {"", 0, 0}
};

struct attribute_data light_color[] = {
    {"none", LIGHT_COLOR_NONE, CREATOR},
    {"red", LIGHT_COLOR_RED, CREATOR},
    {"blue", LIGHT_COLOR_BLUE, CREATOR},
    {"yellow", LIGHT_COLOR_YELLOW, CREATOR},
    {"green", LIGHT_COLOR_GREEN, CREATOR},
    {"purple", LIGHT_COLOR_PURPLE, CREATOR},
    {"orange", LIGHT_COLOR_ORANGE, CREATOR},
    {"white", LIGHT_COLOR_WHITE, CREATOR},
    {"amber", LIGHT_COLOR_AMBER, CREATOR},
    {"pink", LIGHT_COLOR_PINK, CREATOR},
    {"sensual", LIGHT_COLOR_SENSUAL, HIGHLORD},
    {"black", LIGHT_COLOR_BLACK, CREATOR},
    {"", 0, 0}
};

struct flag_data light_flags[] = {
    {"lit", LIGHT_FLAG_LIT, CREATOR},
    {"refillable", LIGHT_FLAG_REFILLABLE, CREATOR},
    {"consumes", LIGHT_FLAG_CONSUMES, CREATOR},
    {"torch", LIGHT_FLAG_TORCH, CREATOR},
    {"lamp", LIGHT_FLAG_LAMP, CREATOR},
    {"lantern", LIGHT_FLAG_LANTERN, CREATOR},
    {"candle", LIGHT_FLAG_CANDLE, CREATOR},
    {"campfire", LIGHT_FLAG_CAMPFIRE, CREATOR},
    {"", 0, 0}
};

struct attribute_data treasure_type[] = {
    {"gem", TREASURE_GEM, CREATOR},
    {"figurine", TREASURE_FIGURINE, CREATOR},
    {"art", TREASURE_ART, CREATOR},
    {"wind", TREASURE_WIND, CREATOR},
    {"stringed", TREASURE_STRINGED, CREATOR},
    {"percussion", TREASURE_PERCUSSION, CREATOR},
    {"ore", TREASURE_ORE, CREATOR},
    {"jewelry", TREASURE_JEWELRY, CREATOR},
    {"", 0, 0}
};

struct attribute_data speed_type[] = {
    {"walk", SPEED_WALKING, CREATOR},
    {"run", SPEED_RUNNING, CREATOR},
    {"sneak", SPEED_SNEAKING, CREATOR},
    {"", 0, 0},
};

struct attribute_data track_types[] = {
    {"none", TRACK_NONE, CREATOR},
    {"walk_clean", TRACK_WALK_CLEAN, CREATOR},
    {"run_clean", TRACK_RUN_CLEAN, CREATOR},
    {"sneak_clean", TRACK_SNEAK_CLEAN, CREATOR},
    {"walk_bloody", TRACK_WALK_BLOODY, CREATOR},
    {"run_bloody", TRACK_RUN_BLOODY, CREATOR},
    {"sneak_bloody", TRACK_SNEAK_BLOODY, CREATOR},
    {"group", TRACK_GROUP, CREATOR},
    {"wagon", TRACK_WAGON, CREATOR},
    {"sleep", TRACK_SLEEP, CREATOR},
    {"battle", TRACK_BATTLE, CREATOR},
    {"sleep_bloody", TRACK_SLEEP_BLOODY, CREATOR},
    {"walk_glow", TRACK_WALK_GLOW, CREATOR},
    {"run_glow", TRACK_RUN_GLOW, CREATOR},
    {"sneak_glow", TRACK_SNEAK_GLOW, CREATOR},
    {"tent", TRACK_TENT, CREATOR},
/* Adding for more track events */
    {"fire", TRACK_FIRE, CREATOR},
    {"pour", TRACK_POUR, CREATOR},
    {"storm", TRACK_STORM, CREATOR},
    {"", 0, 0},
};

/* the following table contains the average percent of each stat
   as a character ages from 00.0% of its lifespan to 100.0% of its 
   lifespan */
struct age_stat_data age_stat_table = {
    {10, 100, 300, 600, 800, 850, 900, 950, 975, 1000, 1000, 975, 950, 925, 850, 800, 750, 650, 500,
     400, 250},
    {100, 350, 550, 750, 850, 950, 1000, 1000, 1000, 1000, 1000, 1000, 950, 900, 850, 750, 650, 550,
     450, 350, 250},
    {100, 500, 900, 1200, 1100, 1000, 1000, 1000, 987, 975, 967, 950, 937, 925, 900, 850, 800, 750,
     700, 650, 550},
    {100, 350, 550, 750, 850, 950, 985, 987, 988, 989, 990, 991, 992, 993, 994, 995, 996, 997, 998,
     999, 1000}
};

// Breakdown of age classification by name
const char * const age_names[] = {
    "young",
    "adult",
    "mature",
    "middle-aged",
    "old",
    "ancient"
};

/* The following structure will map the language # to the actual skill # 
   There should be at least the same # as MAX_TONGUES */
/* {SPOKEN SKILL, WRITTEN SKILL, PERCENT VOWELS } */
struct language_data language_table[] = {
    {101, 151, 50},             /* sirihish */
    {102, 152, 50},             /* bendune */
    {103, 153, 90},             /* allundean */
    {104, 154, 50},             /* mirukkim */
    {105, 155, 50},             /* kentu */
    {106, 156, 5},              /* nrizkt */
    {107, 157, 50},             /* untal */
    {108, 158, 50},             /* cavilish */
    {109, 159, 50},             /* tatlum */
    {235, 236, 50},             /* anyar */
    {237, 238, 50},             /* heshrak */
    {239, 240, 50},             /* vloran */
    {241, 242, 50},             /* domat */
    {243, 244, 50}              /* ghatti */
};

/* accent messages */
struct accent_data accent_table[] = {
    {LANG_NORTH_ACCENT, "northern", "northern-accented "},
    {LANG_SOUTH_ACCENT, "southern", "southern-accented "},
    {LANG_RINTH_ACCENT, "rinthi", "rinthi-accented "},
    {LANG_BENDUNE_ACCENT, "tribal", "tribal-accented "},
    {LANG_ANYAR_ACCENT, "anyali", "melodiously-accented "},
    {LANG_WATER_ACCENT, "vivaduan", "unnaturally flowing "},
    {LANG_FIRE_ACCENT, "suk-krathi", "unnaturally harsh "},
    {LANG_WIND_ACCENT, "whiran", "unnaturally whispery "},
    {LANG_EARTH_ACCENT, "rukian", "unnaturally gutteral "},
    {LANG_SHADOW_ACCENT, "drovian", "unnaturally sombre "},
    {LANG_LIGHTNING_ACCENT, "elkrosian", "unnaturally distorted "},
    {LANG_NILAZ_ACCENT, "nilazi", "unnaturally toneless "},
    {LANG_DESERT_ACCENT, "desert", "staccato-accented "},
    {LANG_EASTERN_ACCENT, "eastern", "eastern-accented "},
    {LANG_WESTERN_ACCENT, "western", "western-accented "},
    {LANG_NO_ACCENT, "none", "unaccented "}     // TERMINAL, must be last
};

#ifdef DOTW_SUPPORT
/* Note:  The way this works is that the day of the week is the modulus of 
 * the current day of the phase.  Since each phase is 231 days long, the
 * length of a week is 11, which is a factor of 231.  (231 = 11 * 21).
 * Thus, there will be 21 weeks in a phase, or 63 weeks in a year.  Changing
 * the number of days in a week to something OTHER than a factor of 231
 * would result in days being "skipped." (Bad thing (tm)) -- Tiernan 1/21/99
 */
const char * const weekday_names[] = {
    "Detal",
    "Ocandra",
    "Terrin",
    "Abid",
    "Cingel",
    "Nekrete",
    "Waleuk",
    "Yochem",
    "Huegel",
    "Dzeda",
    "Barani"
};
#endif

/* various application types for characters */
struct attribute_data application_type[] = {
    {"alive", APPLICATION_NONE, OVERLORD},
    {"applying", APPLICATION_APPLYING, OVERLORD},
    {"revising", APPLICATION_REVISING, OVERLORD},
    {"reviewing", APPLICATION_REVIEW, OVERLORD},
    {"accepted", APPLICATION_ACCEPT, OVERLORD},
    {"denied", APPLICATION_DENY, OVERLORD},
    {"dead", APPLICATION_DEATH, OVERLORD},
    {"stored", APPLICATION_STORED, OVERLORD},
    {"", 0, 0}
};

struct flag_data pinfo_flags[] = {
    {"no_new_chars", PINFO_NO_NEW_CHARS, HIGHLORD},
    {"ban", PINFO_BAN, HIGHLORD},
    {"free_email_ok", PINFO_FREE_EMAIL_OK, HIGHLORD},
    {"multiplay", PINFO_CAN_MULTIPLAY, OVERLORD},
    {"wants_rp_feedback", PINFO_WANTS_RP_FEEDBACK, HIGHLORD},
    {"no_spec_apps", PINFO_NO_SPECIAL_APPS, HIGHLORD},
    {"", 0, 0}
};

struct flag_data kinfo_races[] = {
    {"blackwing_elf", KINFO_RACE_BLACKWING_ELF, HIGHLORD},
    {"desert_elf", KINFO_RACE_DESERT_ELF, HIGHLORD},
    {"dwarf", KINFO_RACE_DWARF, HIGHLORD},
    {"elf", KINFO_RACE_ELF, HIGHLORD},
    {"gith", KINFO_RACE_GITH, HIGHLORD},
    {"half_elf", KINFO_RACE_HALF_ELF, HIGHLORD},
    {"half_giant", KINFO_RACE_HALF_GIANT, HIGHLORD},
    {"halfling", KINFO_RACE_HALFLING, HIGHLORD},
    {"human", KINFO_RACE_HUMAN, HIGHLORD},
    {"mantis", KINFO_RACE_MANTIS, HIGHLORD},
    {"mul", KINFO_RACE_MUL, HIGHLORD},
    {"dragon", KINFO_RACE_DRAGON, OVERLORD},
    {"avangion", KINFO_RACE_AVANGION, OVERLORD},
    {"", 0, 0}
};

struct flag_data kinfo_guilds[] = {
    {"assassin", KINFO_GUILD_ASSASSIN, HIGHLORD},
    {"burglar", KINFO_GUILD_BURGLAR, HIGHLORD},
    {"merchant", KINFO_GUILD_MERCHANT, HIGHLORD},
    {"pick_pocket", KINFO_GUILD_PICKPOCKET, HIGHLORD},
    {"psionicist", KINFO_GUILD_PSIONICIST, HIGHLORD},
    {"ranger", KINFO_GUILD_RANGER, HIGHLORD},
    {"templar", KINFO_GUILD_TEMPLAR, HIGHLORD},
    {"warrior", KINFO_GUILD_WARRIOR, HIGHLORD},
    {"fire_elem", KINFO_GUILD_FIRE_CLERIC, HIGHLORD},
    {"water_elem", KINFO_GUILD_WATER_CLERIC, HIGHLORD},
    {"stone_elem", KINFO_GUILD_STONE_CLERIC, HIGHLORD},
    {"wind_elem", KINFO_GUILD_WIND_CLERIC, HIGHLORD},
    {"shadow_elem", KINFO_GUILD_SHADOW_CLERIC, HIGHLORD},
    {"lightning_elem", KINFO_GUILD_LIGHTNING_CLERIC, HIGHLORD},
    {"void_elem", KINFO_GUILD_VOID_CLERIC, HIGHLORD},
    {"sorcerer", KINFO_GUILD_SORCERER, HIGHLORD},
    {"lirathu templar", KINFO_GUILD_LIRATHU_TEMPLAR, HIGHLORD},
    {"jihae templar", KINFO_GUILD_JIHAE_TEMPLAR, HIGHLORD},
    {"noble", KINFO_GUILD_NOBLE, HIGHLORD},
    {"", 0, 0}
};

struct flag_data clan_job_flags[] = {
    {"recruiter", CJOB_RECRUITER, MORTAL},      /* can add to clan */
    {"leader", CJOB_LEADER, MORTAL},    /* can set other jobs */
    {"banker", CJOB_WITHDRAW, MORTAL},  /* withdraw from account */
    {"", 0, 0}
};



const char * const spice_odor[] = { "dusty smelling",
    "musky",
    "heavy, bitter",
    "stinging",
    "bitter",
    "dusty smelling",
    "rich, heady",
    "odorless",
    "rich, mossy smelling",
    ""
};


#ifdef SMELLS
struct smell_constant smells_table[] = {
    /*{ name,                description } */
    {"shit", "offal"},
    {"methelinoc", "dusty spice"},
    {"melem tuek", "musky spice"},
    {"krelez", "bitter spice"},
    {"kemen", "pungent spice"},
    {"aggross", "bitter spice"},
    {"zharal", "dusty spice"},
    {"thodeliv", "rich, heady spice"},
    {"qel", "rich, mossy spice"},
    {"rose perfume", "roses"},
    {"jasmine perfume", "jasmine"},
    {"pymlithe perfume", "pymlithe"},
    {"glimmergrass perfume", "glimmergrass"},
    {"musk perfume", "musky perfume"},
    {"shrintal perfume", "shrintal"},
    {"lanturin perfume", "lanturin"},
    {"clove perfume", "clove"},
    {"citron perfume", "citron"},
    {"mint perfume", "mint"},
    {"belgoikiss perfume", "belgoikiss"},
    {"lavender perfume", "lavender"},
    {"ale", "ale"},
    {"baking bread", "baking bread"},
    {"blood", "blood"},
    {"brandy", "brandy"},
    {"burned flesh", "burned flesh"},
    {"cinnamon", "cinnamon"},
    {"coffee", "coffee"},
    {"cooking meat", "cooking meat"},
    {"dust", "dust"},
    {"human excrement", "human excrement"},
    {"kank excrement", "kank excrement"},
    {"floral", "floral"},
    {"fruit", "fruit"},
    {"honey", "honey"},
    {"mold", "mold"},
    {"ocotillo", "ocotillo"},
    {"ozone", "ozone"},
    {"pepper", "pepper"},
    {"rotting flesh", "rotting flesh"},
    {"rotting food", "rotting food"},
    {"sawdust", "sawdust"},
    {"fresh cut wood", "freshly cut wood"},
    {"sex", "sex"},
    {"smoke", "smoke"},
    {"soap", "soap"},
    {"spicy cooking", "spicy cooking"},
    {"sulphur", "sulphur"},
    {"sweat", "sweat"},
    {"sweetbreeze", "sweetbreeze"},
    {"tanning leather", "tanning leather"},
    {"tarantula meat", "tarantula meat"},
    {"urine", "urine"},
    {"vegetation", "vegetation"},
    {"vomit", "vomit"},
    {"whisky", "whisky"},
    {"wine", "wine"},

    {"", ""}
};
#endif
