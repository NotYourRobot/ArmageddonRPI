/*
 * File: TELOPTS.H
 * Usage: Define the structures varianbles used in additional telnet 
 *        options such as IAC MSSP.
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
 * 04/19/2013: original file created
 */


/* telnet options */
#define TELOPT_MSSP          70   /* used to send mud server information */

/* TELOPT_MSSP */
#define MSSP_VAR              1
#define MSSP_VAL              2

/* commands */
char IAC_SE[]        = { IAC, SE, '\0' };
char IAC_DO_MSSP[]   = { IAC, DO, TELOPT_MSSP, '\0' };
char IAC_SB_MSSP[]   = { IAC, SB, TELOPT_MSSP, '\0' };
char IAC_WILL_MSSP[] = { IAC, WILL, TELOPT_MSSP, '\0' };

struct MSSPData{

/* -------------- Required -------------- */

char NAME[15];                	/* Name of the MUD. */
int  PLAYERS;                 	/* Current number of logged in players. */
int  UPTIME;                  	/* Unix time value of the startup time of the MUD. */

/* -------------- Generic -------------- */

int  CRAWL_DELAY;             	/* Preferred minimum number of hours between crawls. Send -1 */
                               	/* to use the crawler's default. */

char HOSTNAME[25];            	/* Current or new hostname. */
int  PORT;                    	/* Current or new port number. Can be used multiple times, */
                              	/* most important port last. */

char CODEBASE[20];            	/* Name of the codebase, eg Merc 2.1. */

char CONTACT[25];             	/* Email address for contacting the MUD. */
int  CREATED;                 	/* Year the MUD was created. */
char ICON[80];                	/* URL to a square image in bmp, png, jpg, or gif format. */
                              	/* The icon should be equal or larger than 32x32 pixels, */
                              	/* with a filesize no larger than 32KB. */
char IP_ADDY[16];             	/* Current or new IP address. */
char LANGUAGE[15];            	/* Name of the language used, eg German or English */
char LOCATION[15];            	/* Full English name of the country where the server is */
                              	/* located, using ISO 3166. */
int  MINIMUM_AGE;             	/* Current minimum age requirement, use 0 if not */
                              	/* applicable. */
char WEBSITE[40];             	/* URL to MUD website. */

/* -------------- Categorization -------------- */

char FAMILY[15];              	/* AberMUD, CoffeeMUD, DikuMUD, LPMud, MajorMUD, MOO, */
                              	/* Mordor, Rapture, SocketMud, TinyMUD, Custom. Report */
                              	/* Custom unless it's a well established family. */

                              	/* You can report multiple generic codebases using */
                              	/* the array format, make sure to report the most */
                              	/* distant codebase (the ones listed above) last. */

                              	/* Check the MUD family tree for naming and capitalization. */

char GENRE[15];               	/* Adult, Fantasy, Historical, Horror, */
                              	/* Modern, None, Science Fiction */

char GAMEPLAY[15];            	/* Adventure, Educational, Hack and Slash, None, */
                              	/* Player versus Player, Player versus Environment, */
                              	/* Roleplaying, Simulation, Social, Strategy */

char STATUS[5];               	/* Alpha, Closed Beta, Open Beta, Live */

char GAMESYSTEM[8];          	/* D&D, d20 System, World of Darkness, Etc. Use Custom if */
                            	/* using a custom gamesystems. Use None if not available. */

char INTERMUD[1];           	/* AberChat, I3, IMC2, MudNet, Etc. */
                            	/* Can be used multiple times if you support several */
                               	/* protocols, most important protocol last. Leave empty */
                               	/* if no Intermud protocol is supported. */

char SUBGENRE[15];             	/* LASG, Medieval Fantasy, World War II, */
                               	/* Frankenstein, Cyberpunk, Dragonlance, Etc. */
                               	/* Use None if not available. */

/* -------------- World -------------- */

int AREAS;                     	/* Current number of areas. */
int HELPFILES;           	/* Current number of help files. */
int MOBILES;            	/* Current number of unique mobiles. */
int OBJECTS;            	/* Current number of unique objects. */
int ROOMS;              	/* Current number of unique rooms, use 0 if roomless. */

int CLASSES;           		/* Number of player classes, use 0 if classless. */
int LEVELS;            		/* Number of player levels, use 0 if level-less. */
int RACES;            		/* Number of player races, use 0 if raceless. */
int SKILLS;            		/* Number of player skills, use 0 if skill-less. */

/* -------------- Protocols -------------- */

int ANSI;               	/* Supports ANSI colors ? 1 or 0 */
int GMCP;               	/* Supports GMCP ? 1 or 0 */
int MCCP;               	/* Supports MCCP ? 1 or 0 */
int MCP;                	/* Supports MCP ? 1 or 0 */
int MSDP;               	/* Supports MSDP ? 1 or 0 */
int MSP;                	/* Supports MSP ? 1 or 0 */
int MXP;                	/* Supports MXP ? 1 or 0 */
int PUEBLO;             	/* Supports Pueblo ?  1 or 0 */
int UTF_8;              	/* Supports UTF-8 ? 1 or 0 */
int VT_100;              	/* Supports VT100 interface ?  1 or 0 */
int XTERM_256_COLORS;   	/* Supports xterm 256 colors ?  1 or 0 */

/* -------------- Commercial -------------- */

int PAY_TO_PLAY;        	/* Pay to play ? 1 or 0 */
int PAY_FOR_PERKS;      	/* Pay for perks ? 1 or 0 */

/* -------------- Hiring -------------- */

int HIRING_BUILDERS;    	/* Game is hiring builders ? 1 or 0 */
int HIRING_CODERS;      	/* Game is hiring coders ? 1 or 0 */ 

/* ========== MudBytes - Extended Variabled ==========*/
/* -------------- World -------------- */

int DBSIZE;                     /* Total number of rooms, exits, objects, mobiles, and players. */
int EXITS;                      /* Current number of exits. */
//int EXTRA_DESCRIPTIONS;     	/* Current number of extra descriptions. */
//int MUDPROGS;               	/* Current number of mud program lines. */
//int MUDTRIGS;               	/* Current number of mud program triggers. */
//int RESETS;                  	/* Current number of resets. */

/* -------------- Game -------------- */ 

int ADULT_MATERIAL;             /* "0" or "1" */
int MULTICLASSING;              /* "0" or "1" */
int NEWBIE_FRIENDLY;            /* "0" or "1" */
int PLAYER_CITIES;              /* "0" or "1" */
int PLAYER_CLANS;               /* "0" or "1" */
int PLAYER_CRAFTING;            /* "0" or "1" */
int PLAYER_GUILDS;              /* "0" or "1" */
char EQUIPMENT_SYSTEM[5];       /* "None", "Level", "Skill", "Both" */
char MULTIPLAYING[10];          /* "None", "Restricted", "Full" */
char PLAYERKILLING[10];         /* "None", "Restricted", "Full" */
char QUEST_SYSTEM[15];          /* "None", "Immortal Run", "Automated", "Integrated" */
char ROLEPLAYING[12];           /* "None", "Accepted", "Encouraged", "Enforced" */
char TRAINING_SYSTEM[8];        /* "None", "Level", "Skill", "Both" */
char WORLD_ORIGINALITY[15];     /* "All Stock", "Mostly Stock", "Mostly Original", "All Original" */

/* -------------- Protocols -------------- */

int ATCP;                       /* Supports ATCP? "1" or "0" */
int SSL;                        /* SSL port, use "0" if not supported. */
int ZMP;                        /* Supports ZMP? "1" or "0" */

};
