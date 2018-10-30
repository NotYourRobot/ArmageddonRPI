/*
 * File: DB.C
 * Usage: Database reading/writing.
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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "modify.h"
#include "utility.h"
#include "db.h"
#include "forage.h"
#include "comm.h"
#include "handler.h"
#include "limits.h"
#include "guilds.h"
#include "clan.h"
#include "skills.h"
#include "event.h"
#include "cities.h"
#include "db_file.h"
#include "parser.h"
#include "barter.h"
#include "creation.h"
#include "wagon_save.h"
#include "object_list.h"
#include "player_accounts.h"
#include "character.h"
#include "watch.h"
#include "io_thread.h"
#include "reporting.h"
#include "special.h"
//#include "special_js.h"

static int bogus_cmds = 0;

/* global variable declarations */
struct bdesc_data **bdesc_list; /* list of background descs */
CHAR_DATA *character_list = 0;  /* linked list of all characters */
struct index_data *obj_index = 0;       /* index table for obj file */
struct npc_default_data *npc_default = 0;       /* npc default settings */
struct obj_default_data *obj_default = 0;       /* obj default settings */
int top_of_npc_t = 0;           /* top of npc index table */
int top_of_obj_t = 0;           /* top of obj index table */

int max_top_of_npc_t = 5000;
int max_top_of_obj_t = 5000;

ROOM_DATA *room_list = 0;       /* linked list of loaded rooms */
struct zone_data *zone_table;   /* zone information index */
int top_of_zone_table = 0;      /* number of zones */

OBJ_DATA *obj_free_list = 0;    /* "carved" list of dead structures */
CHAR_DATA *char_free_list = 0;  /* "carved" list of dead structures */
ROOM_DATA *room_free_list = 0;  /* "carved" list of dead structures */

struct message_list fight_messages[MAX_MESSAGES];       /* various damage messages */

struct ban_t *ban_list = 0;     /* banned sites */
struct ban_t *email_list = 0;   /* allowable email addresses */
struct index_data *npc_index = 0;       /* index table for npc file */

FILE *help_fl;                  /* help file */
struct help_index_element *help_index = 0;      /* help table index */
int top_of_helpt;               /* top of help index */

struct guild_data guild[MAX_GUILDS];    /* guild index */
struct sub_guild_data sub_guild[MAX_SUB_GUILDS];        /* sub_guild index */
struct race_data race[MAX_RACES];       /* race index */

struct clan_type *clan_table = 0;       /* clan info */
struct clan_rank_struct *clan_ranks = 0;       /* clan rank info */

int ch_mem = 0;
int ob_mem = 0;

struct time_info_data time_info;        /* game time data */
struct weather_data weather_info;       /* weather data */

struct char_fall_data *char_fall_list = 0;
struct obj_fall_data *obj_fall_list = 0;

struct weather_node *wn_list = (struct weather_node *) 0;
struct reset_q_type reset_q;

int done_booting = 0;

void load_immortal_names();
void unload_immortal_names();
IMM_NAME *immortal_name_list;   /* list of immortal names */

/* text files */
char *credits = 0;              /* the credits list */
char *areas = 0;                /* the area credits list */
char *news = 0;                 /* the news */
char *motd = 0;                 /* the message of the day */
char *help = 0;                 /* the main help page */
char *godhelp = 0;              /* the main help page */
char *info = 0;                 /* the info text */
char *wizlist = 0;              /* the wizlist */
char *menu = 0;                 /* the game menu */
char *menu_no_ansi = 0;         /* the game menu minus ansi */
char *welcome_msg = 0;          /* the welcome message */
char *banner = 0;               /* the banner */
char *introduction = 0;         /* the introduction */
char *doc_menu = 0;             /* the documentation menu */
char *doc_menu_no_ansi = 0;     /* the documentation menu */
char *mail_menu = 0;            /* the mail menu */
char *mail_menu_no_ansi = 0;    /* the mail menu */

/* docs */
char *desc_info = 0;
char *background_info = 0;
char *origin_info = 0;
char *tattoo_info = 0;
char *stat_order_info = 0;      /* the stat order info */
char *short_descr_info = 0;     /* the short_descr info */
char *long_descr_info = 0;      /* the long_descr info */
char *extkwd_info = 0;          /* the keyword info */
char *app_info = 0;             /* the death info */
char *end_app_info = 0;         /* the death info */
char *account_info = 0;         /* the account creation info */

struct city_data city[MAX_CITIES];

bool unbooting_db = FALSE;
time_t boot_time;

/* local functions */
char *read_object_from_file(FILE * fl, struct obj_default_data *objdef, int pos);
char *read_mobile_from_file(FILE * fl, int nr, int pos);
int read_zone_file(int zon);

/* load up the database */
void
boot_db(void)
{
    int i;
    ROOM_DATA *room;
    char buf[256];
    time_t ct;

    gamelog("**** EasyArm beginning boot-up routine *****");

    /* Save the time the game started booting */
    ct = time(0);
    boot_time = ct;

    gamelog("Resetting the game time.");
    reset_time();

    gamelog("Reading text files.");
    file_to_string(NEWS_FILE, &news);
    file_to_string(CREDITS_FILE, &credits);
    file_to_string(AREAS_FILE, &areas);
    if (!test_boot)
        file_to_string(MOTD_FILE, &motd);
    else
        file_to_string(MOTD_TEST_FILE, &motd);
    file_to_string(HELP_PAGE_FILE, &help);
    file_to_string(GODHELP_PAGE_FILE, &godhelp);
    file_to_string(MENUFILE, &menu);
    strdupcat(&menu, "\n\rChoose thy fate: ");
    sprintf(buf, "%s.no_ansi", MENUFILE);
    file_to_string(buf, &menu_no_ansi);
    strdupcat(&menu_no_ansi, "\n\rChoose thy fate: ");
    file_to_string(WELCFILE, &welcome_msg);
    file_to_string(BANNERFILE, &banner);
    file_to_string("text_files/doc_menu", &doc_menu);
    strdupcat(&doc_menu, "\n\rChoose a topic: ");
    file_to_string("text_files/doc_menu.no_ansi", &doc_menu_no_ansi);
    strdupcat(&doc_menu_no_ansi, "\n\rChoose a topic: ");
    file_to_string("text_files/mail_menu", &mail_menu);
    strdupcat(&mail_menu, "\n\rChoose an option: ");
    file_to_string("text_files/mail_menu.no_ansi", &mail_menu_no_ansi);
    strdupcat(&mail_menu_no_ansi, "\n\rChoose an option: ");
    file_to_string("text_files/desc_info", &desc_info);
    file_to_string("text_files/stat_order_info", &stat_order_info);
    file_to_string("text_files/background_info", &background_info);
    file_to_string("text_files/origin_info", &origin_info);
    file_to_string("text_files/tattoo_info", &tattoo_info);
    file_to_string("text_files/short_descr_info", &short_descr_info);
    strdupcat(&short_descr_info, "Short description: ");
    file_to_string("text_files/long_descr_info", &long_descr_info);
    strdupcat(&long_descr_info, "Long description: ");
    file_to_string("text_files/extkwd_info", &extkwd_info);
    /* strdupcat (&extkwd_info, "\n\rExtended keywords: "); */
    file_to_string("text_files/app_info", &app_info);
    file_to_string("text_files/end_app_info", &end_app_info);
    file_to_string("text_files/account_info", &account_info);

    if (!test_boot) {
        file_to_string(INFO_FILE, &info);
        file_to_string(WIZLIST_FILE, &wizlist);
    }

    gamelog("Resetting boards.");
    init_boards();

    /*    strdup("Before Guild Data"); this is to trigger the malloc tester */
    gamelog("Loading guild data.");
    boot_guilds();

    // Added by nessalin 10/14/2004
    /*    strdup("Before Sub Guild Data"); this is to trigger the malloc tester */
    gamelog("Loading sub guild data.");
    boot_subguilds();

    /*  strdup("Before Clan Data"); this is to trigger the malloc tester */
    gamelog("Loading clan data.");
    boot_clans();

    /*  strdup("Before Clan Rank Data"); this is to trigger the malloc tester */
    gamelog("Loading clan rank data.");
    boot_clan_rank_names();

#ifndef CLANS_FROM_DB
    gamelog("Loading clan bank data.");
    boot_clan_bank_accounts();
#endif

    /*  strdup("Before Cities"); this is to trigger the malloc tester */
    gamelog("Loading cities.");
    boot_cities();

    /*  strdup("Before Race Data"); this is to trigger the malloc tester */
    gamelog("Loading race data.");
//    boot_races();
	old_boot_races();

    gamelog("Loading forage data.");
    boot_forage();

    /*strdup("Before Email Data"); this is to trigger the malloc tester */
//    gamelog("Loading email addresses.");
//    load_email_addresses();

    gamelog("Loading immortal names.");
    load_immortal_names();

    gamelog("Clearing account lock files in lib/accountlock");
    sprintf(buf, "rm -f accountlock/*");
    system(buf);

    /* strdup("Before DMPL Data");  this is to trigger the malloc tester */
 //   gamelog("Assigning DMPL predefs.");
 //   init_predefs();

/*
    gamelog("Assigning DMPL programs.");
    read_programs();
*/    
    /* strdup ("Before ECHO Data"); this is to trigger the malloc tester 
     * it's time we took this out anywho -hal
     * gamelog("Booting Echo sourcefile.");
     * boot_echo(); */



    /* strdup ("Before ZONE Data"); this is to trigger the malloc tester */
    gamelog("Loading zone table.");
    boot_zones();

    /* strdup ("Before rooms Data"); this is to trigger the malloc tester */
    gamelog("Loading rooms.");
    boot_world();

    gamelog("Sorting rooms.");
    room_list = sort_rooms(room_list);
    for (i = 0; i <= top_of_zone_table; i++)
        zone_table[i].rooms = sort_rooms_in_zone(zone_table[i].rooms);
i=0;
    gamelog("Assigning room exits.");
    for (room = room_list; room; room = room->next) {
        fix_exits(room);
		i++;
	}

gamelogf("Room exits number of rooms: %d", i);
i = 0;
    gamelog("Assigning watching rooms.");
    for (room = room_list; room; room = room->next) {
        fix_watching(room);
		i++;
	}

gamelogf("Room watch fix number of rooms: %d", i);
    generate_npc_index();
    generate_obj_index();

    // Load the actual zone commands here
    int zone_num, total_cmds = 0;
    for (zone_num = 0; zone_num <= top_of_zone_table; zone_num++) {
        if (zone_table[zone_num].is_booted)
            total_cmds += read_zone_file(zone_num);
    }
    gamelogf("%d valid and %d bogus commands were found in zone cmd files", total_cmds, bogus_cmds);

    /* strdup ("Before BACKGROUND Data");  this is to trigger the malloc tester */
    gamelog("Loading background descriptions.");
    boot_back();

    gamelog("Renumbering zone table.");
    renum_zone_table();

    /* strdup ("Before weather Data"); this is to trigger the malloc tester */
    gamelog("Loading weather data.");
    load_weather();

    gamelog("Loading damage messages.");
    load_messages();

    gamelog("Loading social messages.");
    boot_social_messages();

    if (!no_specials) {
        gamelog("Assinging universal specials list.");
        gamelog("Booting shops.");
        boot_shops_old();
        boot_crafts();
        /*
         * gamelog("Assigning special procedures:");
         * gamelog("     Characters.");
         * assign_mobiles();
         * gamelog("     Objects.");
         * assign_objects();
         * gamelog("     Rooms.");
         * assign_rooms();
         * gamelog("     Zones.");
         * assign_zones();
         */
    }

    gamelog("Assigning function pointers:");
    gamelog("     Zones.");
    assign_zones();
    gamelog("     Commands.");
    assign_command_pointers();

#ifdef WAGON_SAVE
    /* have to do this before resets, to stop multi-wagon loading */
    gamelog("Loading wagons");
    load_wagon_saves();
#endif

    gamelog("Resetting open zones.");
    for (i = 0; i <= top_of_zone_table; i++)
        if (zone_table[i].is_booted)
            reset_zone(i, TRUE);
    reset_q.head = reset_q.tail = 0;

#ifdef WAGON_SAVE
    /* only do this after the first boot   */
    gamelog("Booting wagons");
    boot_wagons(-1);
     /**/
#endif
    done_booting = 1;
    gamelog("Database loaded successfully.");

#ifdef SCRUB_ANTI_MORTAL
    for (i = 0; i <= top_of_zone_table; i++) 
	save_objects(i);
#endif
}

/* Free up all the memory we can so that we can find whats not getting 
   de allocated */

void
unboot_db(void)
{
    gamelog("Deallocating Database");

    free(credits);              /* the credits list */
    free(areas);                /* the area credits list */
    free(news);                 /* the news */
    free(motd);                 /* the message of the day */
    free(help);                 /* the main help page */
    free(godhelp);              /* the main help page */
    free(info);                 /* the info text */
    free(wizlist);              /* the wizlist */
    free(menu);                 /* the game menu */
    free(menu_no_ansi);         /* the game menu minus ansi */
    free(welcome_msg);          /* the welcome message */
    free(banner);               /* the banner */
    free(introduction);         /* the introduction */
    free(doc_menu);             /* the documentation menu */
    free(doc_menu_no_ansi);     /* the documentation menu */
    free(mail_menu);            /* the mail menu */
    free(mail_menu_no_ansi);    /* the mail menu */
    free(desc_info);
    free(stat_order_info);
    free(background_info);
    free(tattoo_info);
    free(origin_info);
    free(short_descr_info);     /* the short_descr info */
    free(long_descr_info);      /* the long_descr info */
    free(extkwd_info);          /* the keyword info */
    free(app_info);             /* the death info */
    free(end_app_info);         /* the death info */
    free(account_info);         /* the account creation info */

    unbooting_db = TRUE;
    unboot_wagons();
    unboot_social_messages();
    unload_messages();
    unboot_weather();
    unboot_back();
    unboot_zones();

    /* should do things in reverse, but what the heck -Tenebrius */

    /* you know that this should be redundant ?  unboot_zones should 
     * extract all the characters and objects all on its lonesome b/c
     * extract_room extracts all the chars and objs in the room as 
     * well. -Hal */

    unboot_object_list();
    unboot_character_list();

    unload_immortal_names();
    uninit_predefs();
    unload_email_addresses();
    unboot_forage();
    unboot_crafts();
    unboot_clan_rank_names();
    unboot_clans();
    unboot_cities();
    unboot_races();
    unboot_subguilds();
    unboot_guilds();
    unboot_shops();
    unboot_boards();
    unboot_heap();

    /*  free((void * )123553); *//* cause a malloc error */
    /*  unboot_asl (); */
}

struct extra_descr_data *
copy_edesc_list(struct extra_descr_data *edesc_list)
{
    struct extra_descr_data *edescit, *newedesc = NULL, *lastedesc = NULL;
    for (edescit = edesc_list; edescit; edescit = edescit->next) {
        if (edescit->keyword && edescit->description) {
            global_game_stats.edescs++;
            CREATE(newedesc, struct extra_descr_data, 1);
            newedesc->keyword = strdup(edescit->keyword);
            newedesc->description = strdup(edescit->description);

            newedesc->next = lastedesc;
            newedesc->def_descr = 1;
            lastedesc = newedesc;
        }
    }
    return newedesc;
}

void
free_edesc_list(struct extra_descr_data *edesc_list)
{
    struct extra_descr_data *edesc;
    struct extra_descr_data *edesc_next;
    /* remove extra descriptions */
    for (edesc = edesc_list; edesc; edesc = edesc_next) {
        /* do nothing, find one wanted */
        edesc_next = edesc->next;

        free(edesc->description);
        free(edesc->keyword);
        free(edesc);
        global_game_stats.edescs--;
    }
}

void
add_to_character_list(CHAR_DATA * ch)
{
    CHAR_DATA * tmp = NULL;

    if (!ch)
        return;

    // Already on the list?
    for (tmp = character_list; tmp; tmp = tmp->next) {
        if (tmp == ch) // Yep
            return;
    }

    // Put them on the list
    ch->prev = (CHAR_DATA *) 0;
    ch->next = character_list;
    character_list = ch;
    if (ch->next)
        ch->next->prev = ch;
}

void
remove_from_character_list(CHAR_DATA * ch)
{
    if (!ch)
        return;

    if (ch->prev)
        ch->prev->next = ch->next;
    if (ch->next)
        ch->next->prev = ch->prev;
    if (ch == character_list)
        character_list = ch->next;
}

void
unboot_character_list(void)
{
    int nr;

    while (character_list)
        extract_char(character_list);

    for (nr = 0; nr < top_of_npc_t; nr++) {
        free(npc_default[nr].name);
        free(npc_default[nr].short_descr);
        free(npc_default[nr].long_descr);
        free(npc_default[nr].description);
        free(npc_default[nr].background);
        free_specials(npc_default[nr].programs);
        free_edesc_list(npc_default[nr].exdesc);

        CLAN_DATA *pClan, *pClan2;
        for (pClan = npc_default[nr].clan; pClan; pClan = pClan2) {
            pClan2 = pClan->next;
            free_clan_data(pClan);
        }
    }

    free(npc_default);
    free(npc_index);
}

/* reset the time in the game from file */
void
reset_time()
{
    char buf[MAX_STRING_LENGTH];

    check_time(&time_info);
    switch (time_info.hours) {
    case NIGHT_EARLY:
    case NIGHT_LATE:
        weather_info.sunlight = SUN_DARK;
        break;
    case DAWN:
    case RISING_SUN:
    case SETTING_SUN:
    case DUSK:
        weather_info.sunlight = SUN_LOW;
        break;
    case MORNING:
    case AFTERNOON:
        weather_info.sunlight = SUN_HIGH;
        break;
    case HIGH_SUN:
        weather_info.sunlight = SUN_ZENITH;
        break;
    default:
        weather_info.sunlight = SUN_LOW;
        break;
    }

    sprintf(buf, "     Current game time: %dH %dD %dM %dY.", time_info.hours, time_info.day,
            time_info.month, time_info.year);
    gamelog(buf);

    weather_info.pressure = number(231, 693);
    weather_info.moon[JIHAE] = time_info.day % 7;
    weather_info.moon[LIRATHU] = time_info.day  % 11;
    weather_info.moon[ECHRI] = time_info.day % 33;

    /* Calculate and update lunar tide value */
    static int tidal_jihae = 40;     // Lirathu and Jihae have the same 'pull'
    static int tidal_lirathu = 40;   // even though they are differnt sizes.
    static int tidal_echri = 20;     // Echri is further away or lower mass.
    int jihae_factor = 0;
    int lirathu_factor = 0;
    int echri_factor = 0;

    /* Calculate the phase factor.  */
    if (weather_info.moon[JIHAE] == JIHAE_RISE || weather_info.moon[JIHAE] == JIHAE_SET)
        jihae_factor = 5;
    else if (weather_info.moon[JIHAE] < JIHAE_RISE && weather_info.moon[JIHAE] > JIHAE_SET)
        jihae_factor = 0;
    else if (weather_info.moon[JIHAE] > JIHAE_RISE && weather_info.moon[JIHAE] < JIHAE_SET)
        jihae_factor = 10;

    if (weather_info.moon[LIRATHU] == LIRATHU_RISE || weather_info.moon[LIRATHU] == LIRATHU_SET)
        lirathu_factor = 5;
    else if (weather_info.moon[LIRATHU] < LIRATHU_RISE && weather_info.moon[LIRATHU] > LIRATHU_SET)
        lirathu_factor = 0;
    else if (weather_info.moon[LIRATHU] > LIRATHU_RISE && weather_info.moon[LIRATHU] < LIRATHU_SET)
        lirathu_factor = 10;

    if (weather_info.moon[ECHRI] == ECHRI_RISE || weather_info.moon[ECHRI] == ECHRI_SET)
        echri_factor = 5;
    else if (weather_info.moon[ECHRI] < ECHRI_RISE && weather_info.moon[ECHRI] > ECHRI_SET)
        echri_factor = 0;
    else if (weather_info.moon[ECHRI] > ECHRI_RISE && weather_info.moon[ECHRI] < ECHRI_SET)
        echri_factor = 10;

    int testing;
    testing = ((tidal_jihae * jihae_factor)/10) + ((tidal_lirathu * lirathu_factor)/10) + ((tidal_echri * echri_factor)/10);
    /* Will be a value between 0 and 100 where 0 is lowest tide and 100 is highest. */
    weather_info.tides = ((tidal_jihae * jihae_factor)/10) + ((tidal_lirathu * lirathu_factor)/10) + ((tidal_echri * echri_factor)/10);


    gamelogf("J: %d, L: %d, E: %d", weather_info.moon[JIHAE],weather_info.moon[LIRATHU],weather_info.moon[ECHRI]);
    weather_info.wind_direction = 0;
    weather_info.wind_velocity = 0;
}

char *
skip_record(FILE * fl)
{
    static char buf[256];
    while (!feof(fl)) {
        fgets(buf, sizeof(buf), fl);
        if (buf[0] == '#')
            break;
        else if (buf[0] == '$')
            break;
    }
    return buf;
}

/* generate index table for obj file */
void
generate_obj_index()
{
    int z;
    long pos;
    char buf[256], fn[256];
    FILE *fl;

    for (z = 0; z <= top_of_zone_table; z++) {
        sprintf(fn, "objects/z%d.obj", z);
        if (!(fl = fopen(fn, "r"))) {
            gamelogf("Couldn't open %s, skipping", fn);
            continue;
        }

        gamelogf("Reading zone %d objects.", z);

        fgets(buf, sizeof(buf), fl);
        while (1) {
            if (*buf == '#') {
                int new_virt, new_limit, new_index;

                sscanf(buf, "#%d %d", &new_virt, &new_limit);

                // Ignore objects that already exist (keep the first defaults)
                if (real_object(new_virt) != -1) {
                    strcpy(buf, skip_record(fl));
                    continue;
                }

                new_index = alloc_obj(new_virt, FALSE, FALSE);
                assert(new_index == real_object(new_virt));
                obj_index[new_index].number = 0;
                obj_index[new_index].list = (OBJ_DATA *) 0;

                strcpy(buf, read_object_from_file(fl, &obj_default[new_index], (pos = ftell(fl))));
            } else if (*buf == '$')
                break;
        }
        fclose(fl);
    }
}

/* generate index table for npc file */
void
generate_npc_index()
{
    int z;
    long pos;
    char buf[256], fn[256];
    FILE *fl;

    for (z = 0; z <= top_of_zone_table; z++) {
        sprintf(fn, "npcs/z%d.npc", z);
        if (!(fl = fopen(fn, "r"))) {
            gamelogf("Couldn't open %s, skipping: %s", fn, strerror(errno));
            continue;
        }

        gamelogf("Reading zone %d NPCs.", z);

        fgets(buf, sizeof(buf), fl);
        while (1) {
            if (*buf == '#') {
                int new_virt, new_limit, new_index;

                sscanf(buf, "#%d %d", &new_virt, &new_limit);

                // Ignore objects that already exist (keep the first defaults)
                if (real_mobile(new_virt) != -1) {
                    strcpy(buf, skip_record(fl));
                    continue;
                }

                new_index = alloc_npc(new_virt, FALSE, FALSE);
                assert(new_index == real_mobile(new_virt));
                npc_index[new_index].number = 0;
                npc_index[new_index].list = (CHAR_DATA *) 0;

                strcpy(buf, read_mobile_from_file(fl, new_index, (pos = ftell(fl))));
            } else if (*buf == '$')
                break;
        }

        fclose(fl);
    }
}


/* convert room exit numbers into pointers */
void
fix_exits(ROOM_DATA * room)
{
    int ex;
    ROOM_DATA *new;

    for (ex = 0; ex < 6; ex++) {
        if (room->direction[ex] && (new = get_room_num(room->direction[ex]->to_room_num))) {
            room->direction[ex]->to_room = new;
            room->direction[ex]->to_room_num = NOWHERE;
        } else if (room->direction[ex]) {
            free(room->direction[ex]->general_description);
            free(room->direction[ex]->keyword);
            free(room->direction[ex]);
            room->direction[ex] = 0;
        }
    }
}

/* convert room exit numbers into pointers */
void
fix_watching(ROOM_DATA * room)
{

    struct watching_room_list *wlist;
    for (wlist = room->watching; wlist; wlist = wlist->next_watching) {
        set_room_to_watch_room_resolve(wlist);
    }
}


/* load rooms in all open zones */
void
boot_world()
{
    char fn[256], buf[256];
    int zone;
    FILE *fl;

    for (zone = 0; zone <= top_of_zone_table; zone++) {
        if ((zone == 0) || zone_table[zone].is_open) {
            zone_table[zone].is_booted = TRUE;

            sprintf(buf, "Reading zone %d rooms.", zone);
            gamelog(buf);

            sprintf(fn, "areas/z%d.wld", zone);
            if (!(fl = fopen(fn, "r"))) {
                gamelogf("Couldn't open %s, skipping", fn);
                continue;
            }
            while (read_room(zone, fl));
            fclose(fl);
        }
    }
}

/* Boot background descriptions for open zones */
int
read_background(int zone, int index, FILE * fd)
{
    char *tmp;
    char *t;

    tmp = fread_text(fd);
    free(tmp);

    tmp = fread_text(fd);
    bdesc_list[zone][index].name = strdup(tmp);
    free(tmp);
    tmp = fread_string(fd);
    t = tmp;
    while (*tmp == '\n' || *tmp == '\r')
        tmp++;
    bdesc_list[zone][index].desc = strdup(tmp);
    free(t);

    /*
     * while (*bdesc_list[zone][index].desc == '\n' || 
     * *bdesc_list[zone][index].desc == '\r')
     * bdesc_list[zone][index].desc++;
     */

    return 1;
}

void
unboot_back(void)
{

    int zone, index;

    for (zone = 0; zone <= top_of_zone_table; zone++) {
        if ((zone == 0) || (zone_table[zone].is_open)) {
            for (index = 0; index < MAX_BDESC; index++) {
                free(bdesc_list[zone][index].name);
                free(bdesc_list[zone][index].desc);
            }
            free(bdesc_list[zone]);
        }
    }

    free(bdesc_list);
}

void
boot_back(void)
{
    FILE *fd;
    int zone;
    int index;
    char buf[256], fn[256];

    CREATE(bdesc_list, struct bdesc_data *, top_of_zone_table);

    for (zone = 0; zone <= top_of_zone_table; zone++) {
        if ((zone == 0) || (zone_table[zone].is_open)) {
            sprintf(buf, "Reading zone %d background.", zone);
            gamelog(buf);

            CREATE(bdesc_list[zone], struct bdesc_data, MAX_BDESC);

            sprintf(fn, "background/z%d.bdesc", zone);
            if (!(fd = fopen(fn, "r"))) {
                gamelogf("Couldn't open %s, skipping", fn);
                continue;
            }

            for (index = 0; index < MAX_BDESC; index++)
                read_background(zone, index, fd);
            fclose(fd);

/*          bdesc_list[zone][0].desc = strdup("");
   bdesc_list[zone][0].name = strdup("Reserved Empty.");
   for (index = 1; index < MAX_BDESC; index++)
   {
   bdesc_list[zone][index].desc = strdup("");      
   bdesc_list[zone][index].name = strdup("Empty");
   } */
        }
    }

    return;
}



/* save all open rooms */
void
save_world(void)
{
    int i;

    for (i = 0; i <= top_of_zone_table; i++) {
        if (zone_table[i].is_booted) {
            save_rooms(&zone_table[i], i);
        }
    }
}


void
write_zone_area_file(struct zone_data *zdata, const char *filename)
{
    ROOM_DATA *room;
    FILE *fp;

    if (!zdata->is_booted) {
        gamelog("Attempting to save rooms in closed zone.");
        return;
    }

    if (!(fp = fopen(filename, "w"))) {
        gamelogf("Couldn't open %s for writing.  Bailing out!", filename);
        exit(1);
    }

    for (room = zdata->rooms; room; room = room->next_in_zone) {
        if (IS_SET(room->room_flags, RFL_UNSAVED))
            REMOVE_BIT(room->room_flags, RFL_UNSAVED);
        if (IS_SET(room->room_flags, RFL_ASH))
            REMOVE_BIT(room->room_flags, RFL_ASH);
        write_room(room, fp);
    }

    fwrite_text("x", fp);

    fclose(fp);
}


/* save rooms in zone to a file */
void
save_rooms(struct zone_data *zdata, int zone_num)
{
    char fn2[256], fn[256];

    qgamelogf(QUIET_ZONE, "Saving rooms in zone %d.", zone_num);

    sprintf(fn, "areas/z%d.wld", zone_num);
    sprintf(fn2, "areas/z%d.wld.bak", zone_num);
    rename(fn, fn2);

    write_zone_area_file(zdata, fn);
}

/* bsave routines - bram */

/* write bdesc to file */
void
write_background(int zone, int index, FILE * fd)
{
    fwrite_text("n", fd);
    fwrite_text(bdesc_list[zone][index].name, fd);

    fprintf(fd, "\n");
    fwrite_string(bdesc_list[zone][index].desc, fd);

    return;
}

/* save background list */
void
save_background(int zone)
{
    char fn[256], fn2[256], buf[256];
    FILE *fd;
    int index;

    sprintf(buf, "Saving background in zone %d.", zone);
    gamelog(buf);

    sprintf(fn, "background/z%d.bdesc", zone);
    sprintf(fn2, "background/z%d.bdesc.bak", zone);
    rename(fn, fn2);

    if (!(fd = fopen(fn, "w"))) {
        gamelog("create the file first.");
        return;
    }

    for (index = 0; index < MAX_BDESC; index++) {
        write_background(zone, index, fd);
    }

    fwrite_text("x", fd);
    fclose(fd);

    return;
}

void
unallocate_reset_com(struct zone_data *zdata)
{
    int i;

    if (!zdata->cmd)
        return;

    /* ---------------------------------------------------- */
    /* first delete all of the obj4 pointers, if they exist */
    /*                      -Tenebrius 02/01/98             */
    /* ---------------------------------------------------- */
    shhlog("unallocate_reset_com:Start");
    for (i = 0; zdata->cmd[i].command != 'S'; i++) {
        free(zdata->cmd[i].stringarg1);
        zdata->cmd[i].stringarg1 = 0;

        free(zdata->cmd[i].stringarg2);
        zdata->cmd[i].stringarg2 = 0;
    }
    free(zdata->cmd);
    zdata->num_cmd_allocated = 0;
    zdata->num_cmd = 0;
    zdata->cmd = 0;
    shhlog("unallocate_reset_com:End");
}

void
copy_reset_com_excluding_room(struct zone_data *src, struct zone_data *dst, int room_num)
{
    dst->num_cmd_allocated = src->num_cmd_allocated;
    CREATE(dst->cmd, struct reset_com, dst->num_cmd_allocated);

    int i, dstindex = 0;
    for (i = 0; i < src->num_cmd; i++) {
        if (src->cmd[i].room != room_num) {
            memcpy(&dst->cmd[dstindex], &src->cmd[i], sizeof(struct reset_com));
            if (src->cmd[i].stringarg1) {
                dst->cmd[dstindex].stringarg1 = strdup(src->cmd[i].stringarg1);
            }
            if (src->cmd[i].stringarg2) {
                dst->cmd[dstindex].stringarg2 = strdup(src->cmd[i].stringarg2);
            }
            dstindex++;
        }
    }

    dst->num_cmd = dstindex;
}

/* allocates zone reset command structure element, for loading/rebuilding */
void
allocate_reset_com(struct zone_data *zdata)
{
    int com = zdata->num_cmd_allocated;
    if (!com) {
        assert(!zdata->cmd);
        CREATE(zdata->cmd, struct reset_com, 1);
    } else
        if (!
            (zdata->cmd =
             (struct reset_com *) realloc(zdata->cmd, (com) * sizeof(struct reset_com)))) {
        perror("allocate_reset_com");
        exit(0);
    }
}

int
remove_cmd(int zone, int command)
{
    return (TRUE);
}

int
zone_update_on_room(ROOM_DATA * rm, struct zone_data *zdata)
{
    /* add commands back for this room       */
    add_zone_data_for_room(rm, zdata, FALSE);

    /* write commands to file                */
    save_all_zone_items(zdata, rm->number / 1000, -1);

    return (TRUE);
}


void
save_zone_item(int zone, int coms, FILE * fp)
{
    char buf[MAX_INPUT_LENGTH * 128];
    int rval = 0;

    switch (zone_table[zone].cmd[coms].command) {
    case 'M':
        rval =
            snprintf(buf, MAX_INPUT_LENGTH * 128, "M 0 %d %d %d\n",
                     npc_index[zone_table[zone].cmd[coms].arg1].vnum,
                     zone_table[zone].cmd[coms].arg2, zone_table[zone].cmd[coms].arg3);
        break;

    case 'G':
        rval =
            snprintf(buf, MAX_INPUT_LENGTH * 128, "G 1 %d %d %d %d %d %d %d %d %ld\n",
                     obj_index[zone_table[zone].cmd[coms].arg1].vnum,
                     zone_table[zone].cmd[coms].arg2,
                     zone_table[zone].cmd[coms].arg4[0],
                     zone_table[zone].cmd[coms].arg4[1],
                     zone_table[zone].cmd[coms].arg4[2],
                     zone_table[zone].cmd[coms].arg4[3],
                     zone_table[zone].cmd[coms].arg4[4],
                     zone_table[zone].cmd[coms].arg4[5],
                     zone_table[zone].cmd[coms].state_flags);
        break;

    case 'E':
        rval =
            snprintf(buf, MAX_INPUT_LENGTH * 128, "E 1 %d 1 %d %d %d %d %d %d %d %ld\n",
                     obj_index[zone_table[zone].cmd[coms].arg1].vnum,
                     zone_table[zone].cmd[coms].arg3,
                     zone_table[zone].cmd[coms].arg4[0],
                     zone_table[zone].cmd[coms].arg4[1],
                     zone_table[zone].cmd[coms].arg4[2],
                     zone_table[zone].cmd[coms].arg4[3],
                     zone_table[zone].cmd[coms].arg4[4],
                     zone_table[zone].cmd[coms].arg4[5],
                     zone_table[zone].cmd[coms].state_flags);
        break;

    case 'O':
        rval =
            snprintf(buf, MAX_INPUT_LENGTH * 128, "O 0 %d %d %d %d %d %d %d %d %d %ld\n",
                     obj_index[zone_table[zone].cmd[coms].arg1].vnum,
                     zone_table[zone].cmd[coms].arg2,
                     zone_table[zone].cmd[coms].arg3,
                     zone_table[zone].cmd[coms].arg4[0],
                     zone_table[zone].cmd[coms].arg4[1],
                     zone_table[zone].cmd[coms].arg4[2],
                     zone_table[zone].cmd[coms].arg4[3],
                     zone_table[zone].cmd[coms].arg4[4],
                     zone_table[zone].cmd[coms].arg4[5],
                     zone_table[zone].cmd[coms].state_flags);
        break;

    case 'P':
        rval =
            snprintf(buf, MAX_INPUT_LENGTH * 128, "P %d %d %d %d %d %d %d %d %d %d %ld\n",
                     zone_table[zone].cmd[coms].depth,
                     obj_index[zone_table[zone].cmd[coms].arg1].vnum,
                     zone_table[zone].cmd[coms].arg2,
                     obj_index[zone_table[zone].cmd[coms].arg3].vnum,
                     zone_table[zone].cmd[coms].arg4[0],
                     zone_table[zone].cmd[coms].arg4[1],
                     zone_table[zone].cmd[coms].arg4[2],
                     zone_table[zone].cmd[coms].arg4[3],
                     zone_table[zone].cmd[coms].arg4[4],
                     zone_table[zone].cmd[coms].arg4[5],
                     zone_table[zone].cmd[coms].state_flags);
        break;

    case 'A':
        rval =
            snprintf(buf, MAX_INPUT_LENGTH * 128, "A %d %d %d %d %d %d %d %d %d %d %ld\n",
                     zone_table[zone].cmd[coms].depth,
                     obj_index[zone_table[zone].cmd[coms].arg1].vnum,
                     zone_table[zone].cmd[coms].arg2,
                     obj_index[zone_table[zone].cmd[coms].arg3].vnum,
                     zone_table[zone].cmd[coms].arg4[0],
                     zone_table[zone].cmd[coms].arg4[1],
                     zone_table[zone].cmd[coms].arg4[2],
                     zone_table[zone].cmd[coms].arg4[3],
                     zone_table[zone].cmd[coms].arg4[4],
                     zone_table[zone].cmd[coms].arg4[5],
                     zone_table[zone].cmd[coms].state_flags);
        break;

    case 'D':
        rval =
            snprintf(buf, MAX_INPUT_LENGTH * 128, "D 0 %d %d %d\n", zone_table[zone].cmd[coms].arg1,
                     zone_table[zone].cmd[coms].arg2, zone_table[zone].cmd[coms].arg3);
        break;

    case 'X':
        rval =
            snprintf(buf, sizeof(buf), "X 1 %d\n%s~\n%s~\n\n",
                     zone_table[zone].cmd[coms].arg1, zone_table[zone].cmd[coms].stringarg1,
                     zone_table[zone].cmd[coms].stringarg2);
        break;
    case 'Y':
        rval =
            snprintf(buf, sizeof(buf), "Y 1 %d\n%s~\n%s~\n\n",
                     zone_table[zone].cmd[coms].arg1, zone_table[zone].cmd[coms].stringarg1,
                     zone_table[zone].cmd[coms].stringarg2);
        break;
    case 'S':
        rval = snprintf(buf, MAX_INPUT_LENGTH * 128, "S\n");
        break;
    }

    if (rval == -1) {
        gamelog("returned -1 on snprintf in write_item:BUGABOO");
    }

    rval = fputs(buf, fp);
    if ((rval == EOF) || (rval < 0)) {
        gamelog("returned bad rval in fputs in write_item");
        gamelog(strerror(errno));
    }
}

int
save_all_zone_items(struct zone_data *zdata, int zone, int room_num)
{
    int coms;
    FILE *fp;
    char fn[256], fn2[256];

    sprintf(fn, "cmds/z%d.cmd", zone);
    sprintf(fn2, "cmds/z%d.cmd.bak", zone);
    rename(fn, fn2);

    if (room_num == -1) {
        qgamelogf(QUIET_ZONE, "Full zone save of zone #%d: %s", zone, zone_table[zone].name);
    } else {
        qgamelogf(QUIET_ZONE, "Writing zone file for zone #%d: %s, to record changes in room %d",
                  zone, zone_table[zone].name, room_num);
    }

    if (!(fp = fopen(fn, "w"))) {
        gamelog("Error opening zone file for saving.\n\r");
        exit(0);
    }

    for (coms = 0; 'S' != zone_table[zone].cmd[coms].command; coms++) {
        save_zone_item(zone, coms, fp);
    }

    save_zone_item(zone, coms, fp);

    fclose(fp);

    return (TRUE);
}

int
add_comm_to_zone(struct reset_com *comm, struct zone_data *zdata)
{
    assert(comm->command != 0);
    assert(comm->arg1 >= 0);

    if (zdata->num_cmd_allocated <= zdata->num_cmd + 1024) {
        zdata->num_cmd_allocated = (zdata->num_cmd + 1024) * 2;
        allocate_reset_com(zdata);
    }

    memcpy(&(zdata->cmd[zdata->num_cmd]), comm, sizeof(struct reset_com));
    assert(zdata->cmd[0].command);

    zdata->num_cmd += 1;

    return (TRUE);
}


void
add_zone_data_for_obj(const OBJ_DATA * obj, struct zone_data *zdata, int room_num, char cmd,
                      int if_flag, int number_in_list, int pos, int depth, int obj_number_in)
{
    struct obj_data *cobj;
    struct reset_com comm;
    int i;

    // Don't add data for unsavable stuff
    if (IS_SET(obj->obj_flags.extra_flags, OFL_NOSAVE))
        return;

    memset(&comm, 0, sizeof(comm));
    comm.command = cmd;
    comm.if_flag = if_flag;

    switch (cmd) {
    case 'P':
        assert(obj->nr >= 0);
        comm.if_flag = 1;
        comm.depth = depth;
        comm.room = room_num;
        comm.arg1 = obj->nr;
        comm.arg2 = number_in_list;
        comm.arg3 = obj_number_in;
        for (i = 0; i < 6; i++)
            comm.arg4[i] = obj->obj_flags.value[i];
        comm.state_flags = obj->obj_flags.state_flags;
        add_comm_to_zone(&comm, zdata);
        break;

    case 'A':
        assert(obj->nr >= 0);
        comm.if_flag = 1;
        comm.depth = depth;
        comm.room = room_num;
        comm.arg1 = obj->nr;
        comm.arg2 = number_in_list;
        comm.arg3 = obj_number_in;
        for (i = 0; i < 6; i++)
            comm.arg4[i] = obj->obj_flags.value[i];
        comm.state_flags = obj->obj_flags.state_flags;
        add_comm_to_zone(&comm, zdata);
        break;

    case 'O':
        /* added to keep notes from blanking if ch doesn't save before dropping */
        assert(obj->nr >= 0);
        if (obj->obj_flags.type == ITEM_NOTE)
            save_note((OBJ_DATA *) obj);

        comm.room = room_num;
        comm.arg1 = obj->nr;
        comm.arg2 = objs_existing_in_room(obj->nr, obj->in_room);
        comm.arg3 = obj->in_room->number;
        for (i = 0; i < 6; i++)
            comm.arg4[i] = obj->obj_flags.value[i];
        comm.state_flags = obj->obj_flags.state_flags;
        add_comm_to_zone(&comm, zdata);
        break;

    case 'G':
        assert(obj->nr >= 0);
        comm.room = room_num;
        comm.arg1 = obj->nr;
        comm.arg2 = number_in_list;
        comm.arg3 = 0;
        for (i = 0; i < 6; i++)
            comm.arg4[i] = obj->obj_flags.value[i];
        comm.state_flags = obj->obj_flags.state_flags;
        add_comm_to_zone(&comm, zdata);
        break;

    case 'E':
        assert(obj->nr >= 0);
        comm.room = room_num;
        comm.arg1 = obj->nr;
        comm.arg2 = 1;
        comm.arg3 = pos;
        for (i = 0; i < 6; i++)
            comm.arg4[i] = obj->obj_flags.value[i];
        comm.state_flags = obj->obj_flags.state_flags;
        add_comm_to_zone(&comm, zdata);
        break;
    }

    /* lets add extra desc saving at a differnt time -Tenebrius */

    {
        struct extra_descr_data *edesc;

        for (edesc = obj->ex_description; edesc; edesc = edesc->next) {
            if (strstr(edesc->keyword, "__tmp__")) continue;
            if (edesc->def_descr) continue; // Already on obj defaults

            comm.command = 'X';
            comm.room = room_num;
            comm.if_flag = 1;
            comm.arg1 = 1;

            comm.stringarg1 = edesc->keyword ? strdup(edesc->keyword) : strdup("");
            comm.stringarg2 = edesc->description ? strdup(edesc->description) : strdup("");
            add_comm_to_zone(&comm, zdata);
        }
    }


    /* recursive in/on */
    for (cobj = obj->contains; cobj; cobj = cobj->next_content) {
        add_zone_data_for_obj(cobj, zdata, room_num, 'P', 1,
                              objs_existing_in_list(cobj->nr, obj->contains), pos, depth + 1,
                              obj->nr);
    }

    /* recursive around */
    for (cobj = obj->around; cobj; cobj = cobj->next_content) {
        add_zone_data_for_obj(cobj, zdata, room_num, 'A', 1,
                              objs_existing_in_list(cobj->nr, obj->around), pos, depth + 1,
                              obj->nr);
    }
}

void
add_zone_data_for_npc(struct char_data *mob, struct zone_data *zdata, int room_num)
{
    struct obj_data *obj;
    int pos;
    struct reset_com comm;
    struct extra_descr_data *edesc;
    memset(&comm, 0, sizeof(comm));

    comm.command = 'M';
    comm.room = room_num;
    comm.if_flag = 0;
    comm.arg1 = mob->nr;
    comm.arg2 = mobs_existing_in_zone(mob->nr);
    comm.arg3 = mob->in_room->number;

    // Mob was previously saved in a different room and is now ALSO saved here
    if (mob->saved_room != -1 && mob->saved_room != comm.arg3) {
        qgamelogf(QUIET_ZONE, "NPC saved in room %d was saved in room %d, and now duplicated.",
                  comm.arg3, mob->saved_room);
    }
    mob->saved_room = comm.arg3;

    add_comm_to_zone(&comm, zdata);

    for (edesc = mob->ex_description; edesc; edesc = edesc->next) {
        if (strstr(edesc->keyword, "__tmp__")) continue;
        if (edesc->def_descr) continue; // Already on NPC defaults

        comm.command = 'Y';
        comm.room = room_num;
        comm.if_flag = 0;
        comm.arg1 = 0;

        comm.stringarg1 = edesc->keyword ? strdup(edesc->keyword) : strdup("");
        comm.stringarg2 = edesc->description ? strdup(edesc->description) : strdup("");
        add_comm_to_zone(&comm, zdata);
    }

    for (obj = mob->carrying; obj; obj = obj->next_content)
        if ((obj->nr != 0) && !IS_CORPSE(obj))
            add_zone_data_for_obj(obj, zdata, room_num, 'G', 1,
                                  objs_existing_in_list(obj->nr, mob->carrying), 0, 0, 0);


    for (pos = 0; pos < MAX_WEAR; pos++)
        if ((mob->equipment[pos]))
            if (mob->equipment[pos]->nr != 0)
                add_zone_data_for_obj(mob->equipment[pos], zdata, room_num, 'E', 1, 0, pos, 0, 0);
}

void
add_zone_data_for_directions(ROOM_DATA * room)
{
    int zone, ex;
    struct reset_com comm;

    memset(&comm, 0, sizeof(comm));
    zone = room->number / 1000;

    for (ex = 0; ex < 6; ex++) {
        if (room->direction[ex]) {
            if (IS_SET(room->direction[ex]->exit_info, EX_ISDOOR)) {
                comm.command = 'D';
                comm.if_flag = 0;
                comm.room = room->number;
                comm.arg1 = room->number;
                comm.arg2 = ex;
                comm.arg3 = 0;

                if (!IS_SET(room->direction[ex]->exit_info, EX_CLOSED)
                    && !IS_SET(room->direction[ex]->exit_info, EX_LOCKED))
                    comm.arg3 = 0;

                else if (IS_SET(room->direction[ex]->exit_info, EX_CLOSED)
                         && !IS_SET(room->direction[ex]->exit_info, EX_LOCKED))
                    comm.arg3 = 1;
                else if (IS_SET(room->direction[ex]->exit_info, EX_CLOSED)
                         && IS_SET(room->direction[ex]->exit_info, EX_LOCKED))
                    comm.arg3 = 2;
                else
                    comm.arg3 = 0;

                add_comm_to_zone(&comm, &zone_table[zone]);
            }
        }
    }

}

void
add_zone_data_for_room(ROOM_DATA * rm, struct zone_data *zdata, int zone_save)
{
    CHAR_DATA *mob;
    OBJ_DATA *obj;

    for (mob = rm->people; mob; mob = mob->next_in_room)
        if (IS_NPC(mob)) {
            add_zone_data_for_npc(mob, zdata, rm->number);
        }

    for (obj = rm->contents; obj; obj = obj->next_content)
/* wagons will save independant of this */
#ifdef WAGON_SAVE
        if (obj->obj_flags.type != ITEM_WAGON)
#endif
            add_zone_data_for_obj(obj, zdata, rm->number, 'O', 0, 0, 0, 0, 0);

    add_zone_data_for_directions(rm);

    // Upon saving a room, we will mark NPCs that are no longer in this room
    // as not being saved here, if they were previously.
    // For zone saves, we do this logic separately for performance reasons
    if (!zone_save) {
        CHAR_DATA *tmp;
        for (tmp = character_list; tmp; tmp = tmp->next) {
            if ((tmp->saved_room == rm->number) &&      // Was saved in this room
                (!tmp->in_room || tmp->in_room->number != rm->number)) {        // And either not in a room, or room has changed
                qgamelogf(QUIET_ZONE,
                          "NPC %d originally saved in room %d, now homeless (unsaved) in room %d.",
                          npc_index[tmp->nr].vnum, rm->number,
                          (tmp->in_room ? tmp->in_room->number : -1));
                tmp->saved_room = -1;
            }
        }
    }
}

int
check_zone_table(struct zone_data *zdata, int zone_num)
{
    ROOM_DATA *room;
    ROOM_DATA *troom;
    char buf[256];
    int found;

    for (room = room_list; room; room = room->next) {
        if ((room->number / 1000) == zone_num) {
            found = FALSE;
            for (troom = zdata->rooms; troom; troom = troom->next_in_zone) {
                if (troom == room)
                    found = TRUE;
            }
            if (!found) {
                sprintf(buf, "Error: room %d was not in room_list, but not zone table %d",
                        room->number, room->number / 1000);
                gamelog(buf);

                return (FALSE);
            }
        }
    }

    for (troom = zdata->rooms; troom; troom = troom->next_in_zone) {
        found = FALSE;
        for (room = room_list; room; room = room->next) {
            if (troom == room)
                found = TRUE;
        }
        if (!found) {
            sprintf(buf, "Error: room %d was found in zone table %d, but not in room_list",
                    room->number, room->number / 1000);
            gamelog(buf);
            return (FALSE);
        }
    }

    return (TRUE);
}

/* rebuilds zone reset data */
void
save_zone(struct zone_data *zdata, int zone_num)
{
    /*  int pos, ex; */
    /*  int i, coms = 0; */

    /*
     * int coms = 0, mn, on, com;
     * int save = FALSE;
     */
    ROOM_DATA *room;
    /*  CHAR_DATA *mob; */
    /*  OBJ_DATA *obj, *cobj; */

    unallocate_reset_com(zdata);

    // Upon saving a zone, we will mark NPCs that are no longer in this zone
    // as not being saved here, if they were previously.
    CHAR_DATA *tmp;
    for (tmp = character_list; tmp; tmp = tmp->next) {
        // Skip players
        if (!IS_NPC(tmp))
            continue;

        if ((tmp->saved_room / 1000 == zone_num)) {     // Was saved in this zone
            if (tmp->in_room && tmp->in_room->number / 1000 != zone_num) {
                qgamelogf(QUIET_ZONE,
                          "NPC %d spawned in room %d, is now homeless (unsaved) in room %d.",
                          npc_index[tmp->nr].vnum, tmp->saved_room,
                          (tmp->in_room ? tmp->in_room->number : -1));
            }
            tmp->saved_room = -1;
            // Otherwise, if they are now in this zone (about to be saved here),
            // and are also saved elsewhere
        } else if (tmp->saved_room != -1 && tmp->in_room
                   && (tmp->in_room->number / 1000 == zone_num)
                   && (tmp->saved_room / 1000 != zone_num)) {
            qgamelogf(QUIET_ZONE,
                      "NPC %d originally saved in zone %d, but is now also saved in room %d.",
                      npc_index[tmp->nr].vnum, tmp->saved_room,
                      (tmp->in_room ? tmp->in_room->number : -1));
            tmp->saved_room = -1;
        }
    }


    if (check_zone_table(zdata, zone_num)) {
        for (room = zdata->rooms; room; room = room->next_in_zone) {
            add_zone_data_for_room(room, zdata, TRUE);
        }
    } else {
        for (room = room_list; room; room = room->next) {
            if (room->number / 1000 == zone_num)
                add_zone_data_for_room(room, zdata, TRUE);
        }
    }

    struct reset_com comm;
    memset(&comm, 0, sizeof(comm));
    comm.command = 'S';
    add_comm_to_zone(&comm, zdata);

    save_all_zone_items(zdata, zone_num, -1);
}


/* counts npcs of a certain number in zone */
int
mobs_existing_in_zone(int nr)
{
    return npc_index[nr].number;
/*
   CHAR_DATA *mob;
   int amount;
   for (mob = character_list, amount = 0; mob; mob = mob->next)
   if ((mob->nr == nr) && mob->in_room && (mob->in_room->zone == zone))
   amount++;
   return amount;
 */
}

/* counts ofs of a certain number in room */
int
objs_existing_in_room(int nr, ROOM_DATA * room)
{
    OBJ_DATA *obj;
    int amount = 0;

    for (obj = room->contents; obj; obj = obj->next_content)
        if (obj->nr == nr)
            amount++;
    return amount;
}

/* counts objs of a certain number in list */
int
objs_existing_in_list(int nr, OBJ_DATA * list)
{
    OBJ_DATA *obj;
    int amount = 0;

    for (obj = list; obj; obj = obj->next_content)
        if (obj->nr == nr)
            amount++;
    return amount;
}


/* renumbers zone table after objs and npcs are loaded */
void
renum_zone_table()
{
    int zone, comm;

    for (zone = 0; zone <= top_of_zone_table; zone++) {
        if (!zone_table[zone].cmd)
            continue;
        for (comm = 0; zone_table[zone].cmd[comm].command != 'S'; comm++) {
            switch (zone_table[zone].cmd[comm].command) {
            case 'M':
                zone_table[zone].cmd[comm].arg1 = real_mobile(zone_table[zone].cmd[comm].arg1);
                break;
            case 'O':
                zone_table[zone].cmd[comm].arg1 = real_object(zone_table[zone].cmd[comm].arg1);
                break;
            case 'G':
                zone_table[zone].cmd[comm].arg1 = real_object(zone_table[zone].cmd[comm].arg1);
                break;
            case 'E':
                zone_table[zone].cmd[comm].arg1 = real_object(zone_table[zone].cmd[comm].arg1);
                assert( zone_table[zone].cmd[comm].arg1 >= -1 );
                break;
            case 'P':
                zone_table[zone].cmd[comm].arg1 = real_object(zone_table[zone].cmd[comm].arg1);
                zone_table[zone].cmd[comm].arg3 = real_object(zone_table[zone].cmd[comm].arg3);
                break;
            case 'A':
                zone_table[zone].cmd[comm].arg1 = real_object(zone_table[zone].cmd[comm].arg1);
                zone_table[zone].cmd[comm].arg3 = real_object(zone_table[zone].cmd[comm].arg3);
                break;
            }
        }
    }
}

void
save_zone_data(void)
{
    int i;
    struct zone_priv_data *temp;
    FILE *fl;

    if (!(fl = fopen(ZONE_FILE, "w"))) {
        perror("save_zone_data.fopen");
        exit(1);
    }

    for (i = 0; i <= top_of_zone_table; i++) {
        fprintf(fl, "#%d\n", i);
        fprintf(fl, "%s~\n", zone_table[i].name);
        fprintf(fl, "%d %d %d %d %d %d %d\n", zone_table[i].top, zone_table[i].lifespan,
                zone_table[i].reset_mode, zone_table[i].is_open, zone_table[i].uid,
                zone_table[i].groups, zone_table[i].flags);
        for (temp = zone_table[i].privs; temp; temp = temp->next) {
            fprintf(fl, "%d %d ~\n", temp->uid, temp->type);
        }
        fprintf(fl, "end priv~\n");

    }

    fprintf(fl, "#100\n$~\n");

    fclose(fl);
}

bool
is_save_zone(int zone)
{
    // Validate that the zone is a valid index
    if (zone < 0 || zone > top_of_zone_table)
        return FALSE;

    // If the func is set to auto_save, it's a save zone
    return (zone_table[zone].func == auto_save);
}

void
unboot_zones(void)
{
    int zon;
    struct reset_q_element *temp;

    for (zon = 0; zon <= top_of_zone_table; zon++) {
        gamelogf("Unbooting rooms in zone %d", zon);
        unboot_zone_privs(zon);
        free(zone_table[zon].name);
        while (zone_table[zon].rooms)
            extract_room(zone_table[zon].rooms, 0);     // Don't fixup links

//        unboot_zone_command_table(zon);
        unallocate_reset_com(&zone_table[zon]);
    }
    free(zone_table);



    while (NULL != (temp = reset_q.head)) {
        reset_q.head = temp->next;
        if (reset_q.tail == temp)
            reset_q.tail = NULL;

        free(temp);
    }
}

void
unboot_zone_privs(int zone)
{

    struct zone_priv_data *temp;

    while (NULL != (temp = zone_table[zone].privs)) {
        zone_table[zone].privs = temp->next;
        free(temp);
    }

}

int
com_get_room_arg(const struct reset_com *com)
{
    assert(com);

    if (com->command == 'M' || com->command == 'O')
        return com->arg3;

    if (com->command == 'D')
        return com->arg1;

    return -1;
}


int
is_valid_com(const char *zonefile, int cmd_no, int room_num, const struct reset_com *com)
{
    ROOM_DATA *room = 0;

    assert(com);

    room = get_room_num(room_num);
    if (!get_room_num(room_num)) {
        gamelogf("Zone %s, cmd number %d:  '%c' command references non-existant room %d", zonefile,
                 cmd_no, com->command, room_num);
        bogus_cmds++;
        return 0;
    }
    // All the reasons a com might be invalid go here
    switch (com->command) {
    case 'D':
        if ((com->arg1 == -1) || !(room = get_room_num(com->arg1)) || !room->direction[com->arg2]) {
            gamelogf("Invalid 'D' com while booting zone:  room #: %d, room ptr: %p, ->room@%s: %p",
                     com->arg1, room, dir_name[com->arg2], room ? room->direction[com->arg2] : 0);
            gamelogf("Zone %s, cmd number %d", zonefile, cmd_no + 1);
            bogus_cmds++;
            return 0;           // Not valid
        }
        break;
    case 'O':
    case 'P':
    case 'A':
    case 'G':
    case 'E':
        if (com->arg1 <= 0 || real_object(com->arg1) < 0) {
            gamelogf("Invalid '%c' com while booting zone:  room #: %d, onum == %d, index = %d", com->command,
                     com->room, com->arg1, real_object(com->arg1));
            bogus_cmds++;
            return 0;
        }
    default:
        break;
    }

    return 1;
}

int
read_zone_file(int zon)
{
    ////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Boot zone by number
    //
    ////////////////////////////////////////////////////////////////////////////////////////
    int current_room = -1;
    int cmd_no = 0;

    char fn[256], buf[256];
    FILE *comfile;
    snprintf(fn, sizeof(fn), "cmds/z%d.cmd", zon);
    gamelog(fn);
    if (!(comfile = fopen(fn, "r"))) {
        gamelogf("Error opening zone file %s, skipping.", fn);
        return 0;
    }

    for (;;) {
        struct reset_com newcmd;
        memset(&newcmd, 0, sizeof(newcmd));

        fscanf(comfile, " ");

        if (1 != fscanf(comfile, "%c", &newcmd.command)) {
            gamelogf("Error:Reading 1st character from cmd %d in zone file '%s'", cmd_no, fn);
            exit(1);
        }
        assert(newcmd.command != 0);

        if (newcmd.command == 'S') {
            add_comm_to_zone(&newcmd, &zone_table[zon]);
            break;
        }

        if (newcmd.command == '*') {
            fgets(buf, sizeof(buf), comfile);
            continue;
        }

        int tmp = 0;
        if (1 != fscanf(comfile, " %d", &tmp)) {
            gamelogf("Error:Reading if flag from cmd %d in zone file '%s', cmd = '%c'", cmd_no, fn,
                     newcmd.command);
            exit(1);
        }

        newcmd.if_flag = tmp;

        if (newcmd.command == 'M' ||    /*mobile */
            newcmd.command == 'N' || newcmd.command == 'D') {   /* exit */
            if (3 != fscanf(comfile, " %d %d %d", &newcmd.arg1, &newcmd.arg2, &newcmd.arg3)) {
                gamelogf("Error:Reading 3 integers from cmd %d in zone file '%s'", cmd_no, fn);
                exit(1);
            }

        } else if (newcmd.command == 'G') {     /* obj in inventory */
            if (9 !=
                fscanf(comfile, " %d %d %d %d %d %d %d %d %ld", &newcmd.arg1, &newcmd.arg2,
                       &newcmd.arg4[0], &newcmd.arg4[1], &newcmd.arg4[2], &newcmd.arg4[3],
                       &newcmd.arg4[4], &newcmd.arg4[5], &newcmd.state_flags)) {
                gamelogf("Error: Reading 8 integers from cmd %d in" "zone file '%s'", cmd_no, fn);
                exit(1);
            }

        } else if (newcmd.command == 'O'        /* obj in room */
                   || newcmd.command == 'E'     /* obj in equip */
                   || newcmd.command == 'P'     /* obj in obj */
                   || newcmd.command == 'A') {  /* obj around obj */
            if (10 !=
                fscanf(comfile, " %d %d %d %d %d %d %d %d %d %ld", &newcmd.arg1, &newcmd.arg2,
                       &newcmd.arg3, &newcmd.arg4[0], &newcmd.arg4[1], &newcmd.arg4[2],
                       &newcmd.arg4[3], &newcmd.arg4[4], &newcmd.arg4[5], &newcmd.state_flags)) {
                gamelogf("Error: Reading 9 integers from cmd %d in zone file '%s'", cmd_no, fn);
                exit(1);
            }
        } else if (newcmd.command == 'X') {
            if (1 != fscanf(comfile, " %d\n", &newcmd.arg1)) {
                gamelogf("Error:Reading 1 integers from cmd %d in zone file '%s'", cmd_no, fn);
                exit(1);
            }

            newcmd.stringarg1 = fread_string(comfile);
            newcmd.stringarg2 = fread_string(comfile);
            strip_crs(newcmd.stringarg2);
        } else if (newcmd.command == 'Y') {
            if (1 != fscanf(comfile, " %d\n", &newcmd.arg1)) {
                gamelogf("Error:Reading 1 integers from cmd %d in zone file '%s'", cmd_no, fn);
                exit(1);
            }

            newcmd.stringarg1 = fread_string(comfile);
            newcmd.stringarg2 = fread_string(comfile);
            strip_crs(newcmd.stringarg2);
        }

        if (com_get_room_arg(&newcmd) != -1)
            current_room = com_get_room_arg(&newcmd);
        // Because of an assumption that we're making about ordering, we require
        // that current_room not be == -1 at this point.
        assert(current_room != -1);
        newcmd.room = current_room;

        fgets(buf, sizeof(buf), comfile);

        if (is_valid_com(fn, cmd_no, current_room, &newcmd)) {
            add_comm_to_zone(&newcmd, &zone_table[zon]);
            cmd_no++;
        }

        assert(cmd_no == zone_table[zon].num_cmd);
    }

    fclose(comfile);
    return cmd_no;
}

/* load the zone table and command tables */
void
boot_zones(void)
{
    /* int ch */
    struct zone_priv_data *temp;

    int zon = 0;
    int foo;
    char *check;
    char zone_file_name[256];
    FILE *fl;

    gamelog("Entering bootzones routine.");

    if (test_boot)
        strcpy(zone_file_name, ZONE_TEST_FILE);
    else
        strcpy(zone_file_name, ZONE_FILE);

    if (!(fl = fopen(zone_file_name, "r"))) {
        gamelogf("Couldn't open zone file %s.  Bailing!", ZONE_FILE);
        exit(0);
    }

    gamelog("Reading from zones file.");
    for (;;) {
        fscanf(fl, " #%*d\n");
        check = fread_string(fl);

        if (!check || (*check == '$')) {
            if (check)
                free(check);
            break;
        }

        if (!zon)
            CREATE(zone_table, struct zone_data, 1);
        else if (!
                 (zone_table =
                  (struct zone_data *) realloc(zone_table, sizeof(struct zone_data) * (zon + 1)))) {
            gamelogf("Error boot_zones realloc failed, bailing out");
            exit(0);
        }

        memset(&zone_table[zon], 0, sizeof(zone_table[zon]));
        zone_table[zon].name = check;
        fscanf(fl, " %d ", &zone_table[zon].top);
        fscanf(fl, " %d ", &zone_table[zon].lifespan);
        fscanf(fl, " %d ", &zone_table[zon].reset_mode);
        fscanf(fl, " %d ", &zone_table[zon].is_open);

        fscanf(fl, " %d ", &foo);
        zone_table[zon].uid = (sh_int) foo;

        fscanf(fl, " %d ", &foo);
        zone_table[zon].groups = foo;

        fscanf(fl, " %d ", &foo);
        zone_table[zon].flags = foo;

        zone_table[zon].age = 0;
        zone_table[zon].func = 0;
        zone_table[zon].rcount = 0;
        zone_table[zon].rooms = 0;
        zone_table[zon].is_booted = FALSE;

        zone_table[zon].privs = (struct zone_priv_data *) 0;

        check = fread_string(fl);
        while (check && *check && (strncmp(check, "end", 3))) {
            temp = (struct zone_priv_data *) malloc(sizeof(struct zone_priv_data));
            if (2 != sscanf(check, " %d %d ", &(temp->uid), &(temp->type))) {
                gamelog("wasnt able to read 2 items");
            }

            free(check);

            temp->next = zone_table[zon].privs;
            zone_table[zon].privs = temp;
            check = fread_string(fl);
        }
        free(check);

        zone_table[zon].num_cmd = 0;
        zone_table[zon].num_cmd_allocated = 0;
        zone_table[zon].cmd = NULL;
        zon++;
    }

    top_of_zone_table = --zon;

    /* free (check); */
    fclose(fl);
}

/* WEATHER functions */

/* save weather to file */
void
save_weather_data(void)
{
    int dir, i;
    struct weather_node *wn;
    FILE *fl;

    char dc[] = { 'N', 'E', 'S', 'W' };

    if (!(fl = fopen(WEATHER_FILE, "w"))) {
        perror("open weather file for writing.");
        exit(1);
    }

    /* write the regular node data */
    for (wn = wn_list; wn; wn = wn->next) {
        fprintf(fl, "A %d %d %d %d %f %f %f\n", wn->zone, wn->life, wn->min_temp, wn->max_temp,
                wn->min_condition, wn->max_condition, wn->median_condition);
    }

    /* write the neighbors */
    for (wn = wn_list; wn; wn = wn->next) {
        for (dir = 0; dir < MAX_DIRECTIONS; dir++)
            for (i = 0; i < wn->neighbors[dir]; i++) {
                fprintf(fl, "%c %d %d\n", dc[dir], wn->zone, wn->near_node[dir][i]->zone);
            }
    }

    fprintf(fl, "$\n");
    fclose(fl);

    qgamelogf(QUIET_ZONE, "Saving weather data.");
}

/* max_life_in_zone()
 *
 * This looks at all the rooms in a zone which are for mortal usage and
 * figures out how much lifeforce the zone can contain.
 */
int
max_life_in_zone(int zone)
{
    int i, life = 0;
    ROOM_DATA *room = NULL;
    int start = zone * 1000;
    int end = start + 999;

    // No such zone
    if (zone > top_of_zone_table)
        return 0;

    // Zone's not booted
    if (!zone_table[zone].is_booted)
        return 0;

    for (i = start; i <= end; i++) {
        room = get_room_num(i); // Unused rooms return as NULL
        if (room && !IS_SET(room->room_flags, RFL_IMMORTAL_ROOM)) {
            switch (room->sector_type) {
            case SECT_HEAVY_FOREST:
                life += 5;
                break;
            case SECT_LIGHT_FOREST:
                life += 4;
                break;
            case SECT_FIELD:
            case SECT_COTTONFIELD:
            case SECT_GRASSLANDS:
            case SECT_THORNLANDS:
                life += 3;
                break;
            case SECT_HILLS:
            case SECT_MOUNTAIN:
            case SECT_RUINS:
            case SECT_ROAD:
            case SECT_CAVERN:
            case SECT_SOUTHERN_CAVERN:
            case SECT_SEWER:
                life += 2;
                break;
            case SECT_DESERT:
            case SECT_SALT_FLATS:
            case SECT_SILT:
            case SECT_SHALLOWS:
                life += 1;
                break;
            case SECT_AIR:
            case SECT_INSIDE:
            case SECT_CITY:
            case SECT_FIRE_PLANE:
            case SECT_WATER_PLANE:
            case SECT_EARTH_PLANE:
            case SECT_SHADOW_PLANE:
            case SECT_AIR_PLANE:
            case SECT_LIGHTNING_PLANE:
            case SECT_NILAZ_PLANE:
            default:
                break;
            }
        }
    }
    return life;
}

/* create a new weather node */
void
create_node(int zone, int life, int min_temp, int max_temp, double min_condition,
            double max_condition, double median_condition)
{
    int d;
    struct weather_node *wn;

    CREATE(wn, struct weather_node, 1);
    wn->zone = zone;
    wn->life = life;
    wn->min_temp = min_temp;
    wn->max_temp = max_temp;
    wn->min_condition = min_condition;
    wn->max_condition = max_condition;
    wn->median_condition = median_condition;

    wn->temp = number(min_temp, max_temp);
    wn->condition = min_condition;

    // Figure out the most plantlife this zone can contain
    wn->max_life = MAX(max_life_in_zone(zone), life);

    /* set neighbors to null */
    for (d = 0; d < MAX_DIRECTIONS; d++) {
        wn->neighbors[d] = 0;
        wn->near_node[d] = (struct weather_node **) 0;
    }

    wn->next = wn_list;
    wn_list = wn;
}

/* add a neighboring zone */
void
add_neighbor(struct weather_node *wn, struct weather_node *nb, int dir)
{
    if (wn->neighbors[dir] == 0)
        CREATE(wn->near_node[dir], struct weather_node *, 1);
    else
        RECREATE(wn->near_node[dir], struct weather_node *, wn->neighbors[dir] + 1);

    wn->near_node[dir][wn->neighbors[dir]] = nb;
    wn->neighbors[dir]++;
}


/* remove a neighboring zone */
int
remove_neighbor(struct weather_node *wn, struct weather_node *nb, int dir)
{
    int i;

    if (wn->neighbors[dir] == 0)
        return (FALSE);

    for (i = 0; (i < wn->neighbors[dir]) && (wn->near_node[dir][i] != nb); i++) {       /* do nothing, find matching item */
    };

    if ((i < wn->neighbors[dir]) && (wn->near_node[dir][i] == nb)) {
        for (; (i + 1) < wn->neighbors[dir]; i++) {
            wn->near_node[dir][i] = wn->near_node[dir][i + 1];
        }
        wn->neighbors[dir] -= 1;
        return (TRUE);
    };
    return (FALSE);
}


void
unboot_weather(void)
{
    int d;
    struct weather_node *wn, *next_wn;


    for (wn = wn_list; wn; wn = next_wn) {
        next_wn = wn->next;
        for (d = 0; d < MAX_DIRECTIONS; d++)
            if (wn->neighbors[d] != 0)
                free(wn->near_node[d]);
        free(wn);
    };

}

/* read the weather data from file */
void
load_weather()
{
    char ch;
    int lf, zn, mnt, mxt, nd, nb, dir;
    double mnc, mxc, mec;
    struct weather_node *source = NULL, *target = NULL, *wn;
    FILE *fl;


    if (!(fl = fopen(WEATHER_FILE, "r"))) {
        gamelogf("Couldn't open %s, bailing!", WEATHER_FILE);
        exit(0);
    }

    for (;;) {
        fscanf(fl, "%c", &ch);
        if (!ch || (ch == '$'))
            break;

        switch (ch) {
        case 'A':
            fscanf(fl, " %d ", &zn);
            fscanf(fl, "%d ", &lf);
            fscanf(fl, "%d ", &mnt);
            fscanf(fl, "%d ", &mxt);
            fscanf(fl, "%lf ", &mnc);
            fscanf(fl, "%lf", &mxc);
            char look_ahead[MAX_STRING_LENGTH];
            fgets(look_ahead, MAX_STRING_LENGTH, fl);
            
            if( look_ahead[0] == '\n' ) {
                mec = mnc + (mxc - mnc) / 2;
            } else {
                mec = atof(look_ahead);
            }

            create_node(zn, lf, mnt, mxt, mnc, mxc, mec);
            break;
        case 'N':
        case 'S':
        case 'E':
        case 'W':
            fscanf(fl, "%d ", &nd);
            fscanf(fl, "%d\n", &nb);

            for (wn = wn_list; wn; wn = wn->next) {
                if (wn->zone == nd)
                    source = wn;
                if (wn->zone == nb)
                    target = wn;
            }

            if (source && target) {
                if (ch == 'N')
                    dir = DIR_NORTH;
                else if (ch == 'S')
                    dir = DIR_SOUTH;
                else if (ch == 'E')
                    dir = DIR_EAST;
                else
                    dir = DIR_WEST;

                add_neighbor(source, target, dir);

            } else {
                perror("load_weather");
                gamelog("Illegal linkage in weather file.");
                exit(0);
            }
            break;
        default:
            perror("load_weather");
            gamelog("Illegal file code in weather data.");
            exit(0);
        }
    }

    fclose(fl);
    new_event(EVNT_WEATHER, 1200, 0, 0, 0, 0, 0);
}

void
unload_email_addresses(void)
{
    struct ban_t *et, *next_et;

    et = email_list;
    while (et) {
        next_et = et->next;
        free(et->name);
        free(et);
        et = next_et;
    }

}

/* I changed this so that it would not load an empty email list.  Hopefully
 * this will solve the disappearing email list problem */
void
load_email_addresses(void)
{
    FILE *fp;
    struct ban_t *et;
    char buf[80];

    if (!(fp = fopen(EMAIL_FILE, "r"))) {
        gamelogf("Error opening %s. Bailing out.", EMAIL_FILE);
        exit(0);
    }

    email_list = 0;

    while (fgets(buf, sizeof(buf), fp)) {
        if ((*buf != '#') && (strlen(buf) > 1)) {
            CREATE(et, struct ban_t, 1);
            et->name = strdup(buf);
            et->next = email_list;
            email_list = et;
        }
    }

    fclose(fp);
}

void
save_email_addresses(void)
{
    FILE *fp;
    struct ban_t *et;

    if (!(fp = fopen(EMAIL_FILE, "w"))) {
        gamelogf("Error opening %s. Bailing out.", EMAIL_FILE);
        exit(0);
    }

    for (et = email_list; et; et = et->next)
        fprintf(fp, "%s~\n", et->name);
    fprintf(fp, "*~\n");
    fclose(fp);
    gamelog("Saving email addresses.");
}


/* copy a specials struct */
struct specials_t *
copy_specials(struct specials_t *spec)
{
    int i;
    struct specials_t *newspecs;
    struct special *src;
    struct special *dst;

    if (!spec || spec->count < 1)
        return NULL;

    src = spec->specials;
    newspecs = (struct specials_t *)calloc(spec->count * sizeof(*dst) + sizeof(size_t), 1);
    newspecs->count = spec->count;
    dst = newspecs->specials;
    for (i = 0; i < spec->count; i++) {
        dst[i].cmd = src[i].cmd;
        dst[i].type = src[i].type;
        switch (dst[i].type) {
        case SPECIAL_CODED:
            dst[i].special.cspec = src[i].special.cspec;
            break;
        case SPECIAL_DMPL:
        case SPECIAL_JAVASCRIPT:
            dst[i].special.name = strdup(src[i].special.name);
            break;
        default:
            gamelogf("copy_specials: Unknown special type");
            return NULL;
        }
    }

    return newspecs;
}

/* read a npc from npc_default */
CHAR_DATA *
read_mobile(int nr, int type)
{
    int i, skill_nr;
    CHAR_DATA *mob;
    char gamelogmsg[256];
    const char *stat_str = NULL;
    char str_str[5], agl_str[5], wis_str[5], end_str[5];
    char height_str[5], weight_str[5];
    char buf[100];
    CLAN_DATA *pClan;

    /*
     * char letter;
     * char chk[10], 
     * long tmp, tmp2, tmp3;
     * int j;
     */

    i = nr;
    if (type == VIRTUAL) {
        if ((nr = real_mobile(nr)) < 0) {
            sprintf(buf, "NPC %d (V) does not exist in database.", i);
            return (0);
        }
    } else if ((nr < 0) || (nr > top_of_npc_t)) {
        sprintf(buf, "NPC %d (V) does not exist in database.", i);
        return (0);
    }

    global_game_stats.npcs++;
    CREATE(mob, CHAR_DATA, 1);
    clear_char(mob);

/*  Added to use the new STR() macro... */
    mob->name = NULL;
    mob->short_descr = NULL;
    mob->long_descr = NULL;
    mob->description = NULL;

/*  Removed to use the new STR() macro...
   mob->name = strdup(npc_default[nr].name);
   mob->short_descr = strdup(npc_default[nr].short_descr);
   mob->long_descr = strdup(npc_default[nr].long_descr);
   if (npc_default[nr].description) 
   mob->description = strdup(npc_default[nr].description);
   else
   mob->description = 0;
 */

    mob->player.extkwds = 0;

    mob->ex_description = copy_edesc_list(npc_default[nr].exdesc);

    mob->player.poofin = 0;
    mob->player.poofout = 0;

/* Can't do this, or on mupdate it'll clobber all others of this type's 
  background, use MSTR() macro instead.
  mob->background = npc_default[nr].background;
 */

    mob->background = NULL;

    mob->specials.act = npc_default[nr].act;
    mob->specials.flag = npc_default[nr].flag;
    if (!IS_SET(mob->specials.act, CFL_ISNPC))
        MUD_SET_BIT(mob->specials.act, CFL_ISNPC);
    mob->specials.affected_by = npc_default[nr].affected_by;
    mob->specials.language = npc_default[nr].language;
    mob->specials.to_process = FALSE;
    mob->specials.regenerating = 0;

    stop_watching_raw(mob);

    mob->specials.dir_guarding = -1;
    mob->specials.guarding = (CHAR_DATA *) 0;
    mob->specials.obj_guarding = (OBJ_DATA *) 0;
    mob->obj_master = (OBJ_DATA *) 0;

    mob->specials.harnessed_to = (OBJ_DATA *) 0;

    mob->specials.programs = copy_specials(npc_default[nr].programs);
    mob->specials.hates = (struct memory_data *) 0;
    mob->specials.fears = (struct memory_data *) 0;
    mob->specials.likes = (struct memory_data *) 0;

    GET_LEVEL(mob) = npc_default[nr].level;
    mob->abilities.off = npc_default[nr].off;
    mob->abilities.def = npc_default[nr].def;
    mob->abilities.armor = npc_default[nr].armor;
    mob->abilities.damroll = npc_default[nr].dam;


    mob->points.max_hit = npc_default[nr].max_hit;
    mob->points.hit = mob->points.max_hit;
    mob->points.obsidian = npc_default[nr].obsidian;
    mob->specials.gp_mod = npc_default[nr].gp_mod;
    if (mob->specials.gp_mod < 0) {
        mob->points.obsidian += number(mob->specials.gp_mod, 0);
        mob->points.obsidian = MAX(mob->points.obsidian, 0);
    } else if (mob->specials.gp_mod > 0) {
        mob->points.obsidian += number(0, mob->specials.gp_mod);
    }
    set_char_position(mob, npc_default[nr].pos);
    mob->specials.default_pos = npc_default[nr].def_pos;
    mob->player.sex = npc_default[nr].sex;
    mob->player.guild = npc_default[nr].guild;
    mob->player.sub_guild = SUB_GUILD_NONE;
    mob->player.tribe = npc_default[nr].tribe;
    mob->clan = NULL;

    for (pClan = npc_default[nr].clan; pClan; pClan = pClan->next) {
        CLAN_DATA *clan;

        clan = new_clan_data();
        clan->clan = pClan->clan;
        clan->rank = pClan->rank;
        clan->flags = pClan->flags;
        clan->next = mob->clan;
        mob->clan = clan;
    }
    mob->player.luck = npc_default[nr].luck;
    mob->player.race = npc_default[nr].race;

#define USE_DEFAULT_AGES 1
#ifdef USE_DEFAULT_AGES
    if (npc_default[nr].age == -1)
        mob->player.time.starting_age = race[(int) GET_RACE(mob)].max_age / 2;
    else
        mob->player.time.starting_age = npc_default[nr].age;
    /*
     * if (set_age(mob,npc_default[nr].age) )
     * shhlog ("SET A DAMN AGE");
     * else
     * shhlog ("DIDNT SET A DAMN AGE");
     * 
     */
#else
    mob->player.time.starting_age = race[(int) GET_RACE(mob)].max_age / 2;
#endif

    if (npc_default[nr].apparent_race == RACE_UNKNOWN) {
        mob->player.apparent_race = npc_default[nr].apparent_race;
    }

    mob->player.time.starting_time = time(0);
    mob->player.time.played = 0;
    mob->player.time.logon = time(0);
    mob->player.weight = char_init_weight(mob->player.race);
    mob->player.height = char_init_height(mob->player.race);

    for (i = 0; i < MAX_SKILLS; i++)
        mob->skills[i] = (struct char_skill_data *) 0;


    roll_abilities(mob);


    mob->points.move_bonus = 0;
    mob->points.move = move_limit(mob);

    mob->points.mana_bonus = 0;
    mob->points.mana = mana_limit(mob);

    mob->points.hit = hit_limit(mob);

    mob->points.max_stun = 75 + GET_END(mob) + number(1, 10);
    if (GET_GUILD(mob) == GUILD_WARRIOR)
        mob->points.max_stun += dice(2, 4);
    /* if( GET_RACE( mob ) == RACE_HALF_GIANT )
     * mob->points.max_stun += 20 + number( 1, 8 ); */
    set_stun(mob, GET_MAX_STUN(mob));

    for (i = 0; i < 3; i++)
        GET_COND(mob, i) = -1;

    for (i = 0; i < 5; i++)
        mob->specials.apply_saving_throw[i] = MAX(20 - GET_LEVEL(mob), 2);

    mob->tmpabilities = mob->abilities;

    if (find_ex_description("[MERCY_FLAG]", mob->ex_description, TRUE)) {
       mob->specials.mercy = MERCY_KILL;
    }

    if (find_ex_description("[UNIQUE_STATS]", mob->ex_description, TRUE)) {
        stat_str = find_ex_description("[UNIQUE_STATS]", mob->ex_description, TRUE);
        stat_str = one_argument(stat_str, str_str, sizeof(str_str));
        stat_str = one_argument(stat_str, agl_str, sizeof(agl_str));
        stat_str = one_argument(stat_str, wis_str, sizeof(wis_str));
        stat_str = one_argument(stat_str, end_str, sizeof(end_str));
        stat_str = one_argument(stat_str, height_str, sizeof(height_str));
        stat_str = one_argument(stat_str, weight_str, sizeof(weight_str));
        if (strlen(str_str) && strlen(agl_str) && strlen(wis_str) && strlen(end_str)) {
            SET_STR(mob, atoi(str_str));
            SET_AGL(mob, atoi(agl_str));
            SET_WIS(mob, atoi(wis_str));
            SET_END(mob, atoi(end_str));
        } else {
            sprintf(gamelogmsg,
                    "Found [UNIQUE_STATS] edesc on NPC #%d but it was incorrectly formatted.  Rolling random stats.",
                    npc_index[nr].vnum);
            gamelog(gamelogmsg);
        }
        if (strlen(height_str)) {
            if (!strlen(weight_str)) {
                sprintf(gamelogmsg,
                        "Found height in [UNIQUE_STATS edesc on NPC #%d but not weight.  Generating random height & weight.",
                        npc_index[nr].vnum);
                gamelog(gamelogmsg);
            } else {
                mob->player.height = atoi(height_str);
                mob->player.weight = atoi(weight_str);
            }
        }
    }


    for (i = 0; i < MAX_WEAR; i++)
        mob->equipment[i] = (OBJ_DATA *) 0;

    for (skill_nr = 0; skill_nr < MAX_SKILLS; skill_nr++)
        if (npc_default[nr].skills[skill_nr].learned > 0) {
            init_skill(mob, skill_nr, npc_default[nr].skills[skill_nr].learned);
            mob->skills[skill_nr]->rel_lev = MAX(0, (mob->skills[skill_nr]->learned - 30) / 10);
        }
    // Reach skills for magickers
    if (is_magicker(mob)) {
        if (!has_skill(mob, SKILL_REACH_NONE)) {
            init_skill(mob, SKILL_REACH_NONE, 100);
        }

        if (!has_skill(mob, SKILL_REACH_LOCAL)) {
            init_skill(mob, SKILL_REACH_LOCAL, 100);
        }
    }

    mob->specials.act_wait = 0;
    mob->in_room = NULL;
    mob->nr = nr;
    mob->desc = NULL;

    init_saves(mob);
    init_racial_skills(mob);
    init_languages(mob);
    init_psi_skills(mob);
    if( !has_skill(mob, TOL_PAIN) ) {
       init_skill(mob, TOL_PAIN, GET_END(mob));
       set_skill_taught(mob, TOL_PAIN);
    }
    /*    mob->specials.language = get_native_language(mob); */


    mob->specials.accent = default_accent(mob);

    mob->prev = (CHAR_DATA *) 0;
    mob->next = character_list;
    character_list = mob;
    if (mob->next)
        mob->next->prev = mob;

    mob->prev_num = (CHAR_DATA *) 0;
    mob->next_num = (CHAR_DATA *) npc_index[nr].list;
    npc_index[nr].list = mob;
    if (mob->next_num)
        mob->next_num->prev_num = mob;

    npc_index[nr].number++;

    if (has_special_on_cmd(mob->specials.programs, NULL, CMD_PULSE))
        new_event(EVNT_NPC_PULSE, PULSE_MOBILE, mob, 0, 0, 0, 0);

#ifdef NPC_MOVE
    if (!IS_SET(mob->specials.act, CFL_SENTINEL))
        new_event(EVNT_MOVE, MAX(1, (PULSE_MOBILE + number(-10, 10)) * 8), mob, 0, 0, 0, 0);
#endif

    execute_npc_program(mob, 0, CMD_WHEN_LOADED, "");

    return (mob);
}

int
alloc_npc(int vnum, int init, int allow_renum)
{
    CHAR_DATA *tmpnpc;
    int i, j, real_nr;
    bool done = FALSE, needs_renum = FALSE;

    assert(real_mobile(vnum) == -1);

    if (!npc_index)
        CREATE(npc_index, struct index_data, max_top_of_npc_t);

    if (!npc_default)
        CREATE(npc_default, struct npc_default_data, max_top_of_npc_t);

    if (top_of_npc_t >= max_top_of_npc_t) {
        max_top_of_npc_t += 250;
        RECREATE(npc_index, struct index_data, max_top_of_npc_t);
        RECREATE(npc_default, struct npc_default_data, max_top_of_npc_t);
        memset(&npc_index[top_of_npc_t], 0,
               sizeof(struct index_data) * (max_top_of_npc_t - top_of_npc_t));
        memset(&npc_default[top_of_npc_t], 0,
               sizeof(struct npc_default_data) * (max_top_of_npc_t - top_of_npc_t));
    }

    real_nr = top_of_npc_t++;

    for (i = real_nr; i >= 0 && !done; i--) {
        if (i && npc_index[i - 1].vnum > vnum) {
            needs_renum = TRUE;
            npc_index[i] = npc_index[i - 1];
            npc_default[i] = npc_default[i - 1];
        } else {
            int sk;             // For loop later
            npc_index[i].vnum = vnum;
            npc_index[i].number = 0;
            npc_index[i].limit = -1;
            npc_index[i].list = 0;

            if (init) {
                npc_default[i].name = strdup("clay lump");
                npc_default[i].short_descr = strdup("a lump of clay");
                npc_default[i].long_descr = strdup("A human-shaped lump of clay.\n\r");
                npc_default[i].background = strdup("");
                npc_default[i].description = strdup("");
                npc_default[i].exdesc = 0;
                npc_default[i].guild = GUILD_WARRIOR;
                npc_default[i].race = RACE_HUMAN;
                npc_default[i].sex = SEX_MALE;
                npc_default[i].obsidian = 0;
                npc_default[i].gp_mod = 0;
                npc_default[i].level = 1;
                npc_default[i].tribe = 0;
                /* need to initialize the clan pointer -Morg */
                npc_default[i].clan = NULL;
                npc_default[i].off = 0;
                npc_default[i].def = 0;
                npc_default[i].dam = 0;
                npc_default[i].affected_by = 0;
                npc_default[i].act = 0;
                MUD_SET_BIT(npc_default[i].act, CFL_ISNPC);
                npc_default[i].pos = POSITION_STANDING;
                npc_default[i].def_pos = POSITION_STANDING;
                npc_default[i].max_hit = 80;
                npc_default[i].max_stun = 80;
                for (sk = 0; sk < MAX_SKILLS; sk++)
                    npc_default[i].skills[sk].learned = 0;
                npc_default[i].language = 0;

                npc_default[i].programs = 0;
            } else {
                memset(&npc_default[i], 0, sizeof(struct npc_default_data));
            }

            done = TRUE;
            real_nr = i;
        }
    }

    if (!needs_renum || !allow_renum)
        return real_nr;         // We can bail if we know nothing needs renumbering

    // Renumber all NPC instances referencing NPCs with a higher real_num than this new one
    for (tmpnpc = character_list; tmpnpc; tmpnpc = tmpnpc->next)
        if (IS_NPC(tmpnpc) && tmpnpc->nr >= real_nr)
            tmpnpc->nr++;

    // Renumber all object commands in zone files referencing objects with a higher real_num than this one
    for (i = 0; i <= top_of_zone_table; i++) {
        for (j = 0; zone_table[i].cmd && zone_table[i].cmd[j].command != 'S'; j++) {
            switch (zone_table[i].cmd[j].command) {
            case 'M':
                if (zone_table[i].cmd[j].arg1 >= real_nr)
                    zone_table[i].cmd[j].arg1++;
                break;
            default:
                break;
            }
        }
    }

    return real_nr;
}

int
alloc_obj(int virtual_nr, int init, int allow_renum)
{
    OBJ_DATA *tmpobj;
    int i, j, real_nr;
    bool done = FALSE, needs_renum = FALSE;

    assert(real_object(virtual_nr) == -1);

    if (!obj_index)
        CREATE(obj_index, struct index_data, max_top_of_obj_t);

    if (!obj_default)
        CREATE(obj_default, struct obj_default_data, max_top_of_obj_t);

    if (top_of_obj_t >= max_top_of_obj_t) {
        max_top_of_obj_t += 250;
        RECREATE(obj_index, struct index_data, max_top_of_obj_t);
        RECREATE(obj_default, struct obj_default_data, max_top_of_obj_t);
        memset(&obj_index[top_of_obj_t], 0,
               sizeof(struct index_data) * (max_top_of_obj_t - top_of_obj_t));
        memset(&obj_default[top_of_obj_t], 0,
               sizeof(struct obj_default_data) * (max_top_of_obj_t - top_of_obj_t));
    }

    real_nr = top_of_obj_t++;

    for (i = real_nr; i >= 0 && !done; i--) {
        if (i && obj_index[i - 1].vnum > virtual_nr) {
            needs_renum = TRUE;
            obj_index[i] = obj_index[i - 1];
            obj_default[i] = obj_default[i - 1];
        } else {
            obj_index[i].vnum = virtual_nr;
            obj_index[i].number = 0;
            obj_index[i].limit = -1;
            obj_index[i].list = 0;

            if (init) {
                obj_default[i].name = strdup("clump dust");
                obj_default[i].short_descr = strdup("a small clump of dust");
                obj_default[i].long_descr = strdup("A small clump of dust.\n\r");
                obj_default[i].description = strdup("A small clump of dust.\n\r");

                obj_default[i].wear_flags = 0;
                MUD_SET_BIT(obj_default[i].wear_flags, ITEM_TAKE);
                obj_default[i].extra_flags = 0;

                obj_default[i].type = ITEM_OTHER;

                for (j = 0; j < MAX_OBJ_AFFECT; j++) {
                    obj_default[i].affected[j].location = 0;
                    obj_default[i].affected[j].modifier = 0;
                }

                for (j = 0; j < 6; j++)
                    obj_default[i].value[j] = 0;

                obj_default[i].weight = 100;
                obj_default[i].cost = 0;

                obj_default[i].exdesc = 0;

                obj_default[i].programs = NULL;
            } else {
                memset(&obj_default[i], 0, sizeof(struct obj_default_data));
            }

            done = TRUE;
            real_nr = i;
        }
    }

    assert(real_nr == real_object(virtual_nr));

    if (!needs_renum || !allow_renum)
        return real_nr;         // We can bail if we know nothing needs renumbering

    // Renumber all object instances referencing objects with a higher real_num than this new one
    for (tmpobj = object_list; tmpobj; tmpobj = tmpobj->next)
        if (tmpobj->nr >= real_nr)
            tmpobj->nr++;

    // Renumber all object commands in zone files referencing objects with a higher real_num than this one
    for (i = 0; i <= top_of_zone_table; i++) {
        for (j = 0; zone_table[i].cmd && zone_table[i].cmd[j].command != 'S'; j++) {
            switch (zone_table[i].cmd[j].command) {
            case 'O':
                if (zone_table[i].cmd[j].arg1 >= real_nr)
                    zone_table[i].cmd[j].arg1++;
                break;
            case 'P':
                if (zone_table[i].cmd[j].arg1 >= real_nr)
                    zone_table[i].cmd[j].arg1++;
                if (zone_table[i].cmd[j].arg3 >= real_nr)
                    zone_table[i].cmd[j].arg3++;
                break;
            case 'A':
                if (zone_table[i].cmd[j].arg1 >= real_nr)
                    zone_table[i].cmd[j].arg1++;
                if (zone_table[i].cmd[j].arg3 >= real_nr)
                    zone_table[i].cmd[j].arg3++;
                break;
            case 'G':
                if (zone_table[i].cmd[j].arg1 >= real_nr)
                    zone_table[i].cmd[j].arg1++;
                break;
            case 'E':
                if (zone_table[i].cmd[j].arg1 >= real_nr)
                    zone_table[i].cmd[j].arg1++;
                break;
            default:
                break;
            }
        }
    }

    return real_nr;
}

char *
get_line(char **from, char *to)
{

    char *p = *from;
    char *n;

    if (!(n = strstr(p, "\n")))
        n = p + strlen(p);
    strncpy(to, p, n - p);
    to[n - p] = '\0';

    while ((*n == '\n') || (*n == '\r'))
        n++;

    *from = n;

    return to;
}

struct specials_t *
read_specials(FILE * fp)
{
    char *p;
    struct specials_t *programs = NULL;

    while (strcmp((p = fread_string(fp)), "ep")) {
        if (p && strlen(p))
            programs = set_prog(NULL, programs, p);
        free(p);
        p = NULL;
    }

    free(p);
    return programs;
}


long vector_from_flags(struct flag_data *flag, char *str);

/* load npc from data file */
char *
read_mobile_from_file(FILE * fl, int nr, int pos)
{
    int skill_nr, perc;
    static char buf[100];
    char letter;
    long tmp;
    char extra_value_name[256], extra_value_string[256];
    struct extra_descr_data *new_descr;

    fseek(fl, pos, 0);

    npc_default[nr].clan = NULL;

    npc_default[nr].name = fread_string(fl);
    npc_default[nr].short_descr = fread_string(fl);
    npc_default[nr].long_descr = fread_string(fl);
    npc_default[nr].description = fread_string(fl);

    npc_default[nr].background = fread_string(fl);


    fscanf(fl, "%ld ", &tmp);
    npc_default[nr].act = tmp;
    MUD_SET_BIT(npc_default[nr].act, CFL_ISNPC);
    fscanf(fl, "%ld ", &tmp);
    npc_default[nr].flag = tmp;

    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].affected_by = tmp;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].language = tmp;

    fscanf(fl, " %c \n", &letter);

    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].level = tmp;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].off = tmp;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].def = tmp;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].armor = tmp;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].max_hit = tmp;
    fscanf(fl, " %ld \n", &tmp);
    npc_default[nr].dam = tmp;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].obsidian = tmp;
    fscanf(fl, " %ld \n", &tmp);
    npc_default[nr].gp_mod = tmp;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].pos = tmp;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].def_pos = tmp;
    fscanf(fl, " %ld ", &tmp);

    if (npc_default[nr].pos > POSITION_STANDING)
        npc_default[nr].pos = POSITION_STANDING;
    if (npc_default[nr].def_pos > POSITION_STANDING)
        npc_default[nr].def_pos = POSITION_STANDING;

    npc_default[nr].sex = tmp;
    fscanf(fl, " %ld ", &tmp);
    if (tmp >= MAX_GUILDS)
        tmp = GUILD_WARRIOR;
    npc_default[nr].guild = tmp;
    //  npc_deafult[nr].sub_guild = SUB_GUILD_NONE;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].tribe = tmp;
    fscanf(fl, " %ld ", &tmp);
    npc_default[nr].luck = tmp;

    fscanf(fl, " %ld \n", &tmp);
    npc_default[nr].race = tmp;


    npc_default[nr].programs = read_specials(fl);


    for (skill_nr = 0; skill_nr < MAX_SKILLS; skill_nr++) {
        npc_default[nr].skills[skill_nr].learned = 0;
        npc_default[nr].skills[skill_nr].rel_lev = 0;
        npc_default[nr].skills[skill_nr].taught_flags = 0;
        npc_default[nr].skills[skill_nr].last_gain = 0;
    }

    npc_default[nr].exdesc = 0;
    npc_default[nr].age = -1;
    while (fgets(buf, 256, fl)) {
        char msg[725];

        switch (*buf) {
        case 'S':
            sscanf(&buf[1], " %d %d \n", &skill_nr, &perc);
            if (skill_nr > MAX_SKILLS || skill_nr < 0)
                break;
            if (perc > 100 || perc < 0)
                break;
            npc_default[nr].skills[skill_nr].learned = perc;
            break;
        case 'E':
            global_game_stats.edescs++;
            CREATE(new_descr, struct extra_descr_data, 1);
            new_descr->keyword = fread_string(fl);
            new_descr->description = fread_string(fl);

            new_descr->next = npc_default[nr].exdesc;
            new_descr->def_descr = 1;
            npc_default[nr].exdesc = new_descr;
            break;
        case 'C':
            {
                CLAN_DATA *pClan;
                char *name;
                char *flags;
                int clan;

                name = fread_string(fl);
                flags = fread_string(fl);
                fscanf(fl, "%ld\n", &tmp);

                clan = lookup_clan_by_name(name);

                /* handle clans outside the 1..MAX_CLAN range */
                if (clan == -1) {
                    if (!is_number(name)) {
                        free(name);
                        free(flags);
                        break;
                    } else {
                        clan = atoi(name);

                        if (clan <= 0) {
                            free(name);
                            free(flags);
                            break;
                        }
                    }
                }

                pClan = new_clan_data();
                pClan->clan = clan;

                if (flags)
                    pClan->flags = vector_from_flags(clan_job_flags, flags);
                pClan->rank = tmp;
                pClan->next = npc_default[nr].clan;
                npc_default[nr].clan = pClan;
                /* free the memory allocated by fread_string() */
                free(name);
                free(flags);
                break;
            }

        case 'x':
            sscanf(&buf[1], " %s %s \n", extra_value_name, extra_value_string);
            if (!strcmp(extra_value_name, "age"))
                npc_default[nr].age = atoi(extra_value_string);
            else {
                sprintf(msg, "unidentified type X:%s:%s:", extra_value_name, extra_value_string);
                gamelog(msg);
            };
            break;
        default:
            /* if we've not already converted their old tribe to the new format
             * do so now
             */
            if (npc_default[nr].tribe > 0
                && !is_clan_member(npc_default[nr].clan, npc_default[nr].tribe)) {
                CLAN_DATA *pClan;

                pClan = new_clan_data();
                pClan->clan = npc_default[nr].tribe;
                pClan->rank = 1;

                /* don't forget to check to see if they're a clan leader */
                if (IS_SET(npc_default[nr].act, CFL_CLANLEADER))
                    MUD_SET_BIT(pClan->flags, CJOB_LEADER);
                pClan->next = npc_default[nr].clan;
                npc_default[nr].clan = pClan;
            }
            return buf;
        }
    }

    return NULL;
}

/* read an object from obj_default */
OBJ_DATA *
read_object(int nr, int type)
{
//  char buffer[MAX_STRING_LENGTH];

    OBJ_DATA *obj;
    char buf[256];
    int i;
    /*
     * int tmp;
     * 
     * char chk[50];
     */

    i = nr;
    if (type == VIRTUAL) {
        if ((nr = real_object(nr)) < 0) {
            sprintf(buf, "Object %d (V) does not exist in database.", i);
            return (0);
        }
    } else if ((nr < 0) || (nr >= top_of_obj_t)) {
        sprintf(buf, "Object %d (R) does not exist in database.", i);
        return (0);
    }

    CREATE(obj, OBJ_DATA, 1);
    clear_object(obj);

    /*  Added for use of new STR() Macro - Morg */
    obj->name = NULL;
    obj->short_descr = NULL;
    obj->long_descr = NULL;
    obj->ldesc = NULL;
    obj->description = NULL;

    obj->obj_flags.type = obj_default[nr].type;
    obj->obj_flags.extra_flags = obj_default[nr].extra_flags;
    obj->obj_flags.wear_flags = obj_default[nr].wear_flags;
    obj->obj_flags.state_flags = obj_default[nr].state_flags;

    for (i = 0; i < 6; i++)
        obj->obj_flags.value[i] = obj_default[nr].value[i];

    obj->obj_flags.weight = obj_default[nr].weight;
    /* HACK */
    obj->obj_flags.size = 0;
    obj->obj_flags.length = obj_default[nr].length;

    /* 
     * val1(      plus)--2             val2(   numdice)-1
     * val3(  sizedice)-4              val4(     wtype)-stab
     */

    if (obj->obj_flags.type == ITEM_WEAPON) {
        /*
         * obj->dam_dice_size = obj->obj_flags.value[2];
         * obj->dam_num_dice  = obj->obj_flags.value[1];
         * obj->dam_plus      = obj->obj_flags.value[0];
         */
        obj->dam_dice_size = obj_default[nr].dam_dice_size;
        obj->dam_num_dice = obj_default[nr].dam_num_dice;
        obj->dam_plus = obj_default[nr].dam_plus;
    } else {
        obj->dam_dice_size = 1;
        obj->dam_num_dice = (obj_default[nr].weight / 100) / 5;
        obj->dam_plus = 0;
    }

    if (obj_default[nr].max_hit_points != 0) {
        obj->max_hit_points = obj_default[nr].max_hit_points;
        obj->hit_points = obj_default[nr].hit_points;
    } else {
        if (obj->obj_flags.type == ITEM_ARMOR) {
            obj->max_hit_points = obj_default[nr].value[1];
            obj->hit_points = obj_default[nr].value[0];
        } else {
            int pts = obj->obj_flags.weight / 100;

            switch (obj->obj_flags.material) {
            case MATERIAL_METAL:
            case MATERIAL_STONE:
            case MATERIAL_GEM:
                pts *= 2;
                pts *= 2;
                pts *= 2;
                break;

            case MATERIAL_OBSIDIAN:
            case MATERIAL_CHITIN:
            case MATERIAL_BONE:
            case MATERIAL_HORN:
            case MATERIAL_IVORY:
            case MATERIAL_DUSKHORN:
            case MATERIAL_TORTOISESHELL:
                pts *= 1.5;
                break;

            case MATERIAL_CLOTH:
            case MATERIAL_SILK:
            case MATERIAL_SKIN:
            case MATERIAL_TISSUE:
            case MATERIAL_GRAFT:
            case MATERIAL_GLASS:
            case MATERIAL_CERAMIC:
            case MATERIAL_SALT:
            case MATERIAL_GWOSHI:
            case MATERIAL_PAPER:
            case MATERIAL_PLANT:
                pts /= 2;
                break;

            case MATERIAL_WOOD:
            default:
                break;
            }
            obj->max_hit_points = obj->hit_points = pts;
        }
    }

/*
  if (obj->obj_flags.type == ITEM_WAGON)
    {
      obj->wagon_damage.tongue = 0;
      obj->wagon_damage.wheels = 0;
      obj->wagon_damage.hull = 0;
    }
*/
    if (obj->obj_flags.type == ITEM_ARMOR) {
        /* watch for 0 max_points */
        if (obj->obj_flags.value[3] == 0) {
            /* set max_points to points then */
            obj->obj_flags.value[3] = obj->obj_flags.value[1];
        }
    }

    obj->obj_flags.length = 1;
    obj->obj_flags.cost = obj_default[nr].cost;
#ifdef USE_OBJ_FLAG_TEMP
    obj->obj_flags.temp = obj_default[nr].temp;
#endif
    obj->obj_flags.material = obj_default[nr].material;

    obj->ex_description = copy_edesc_list(obj_default[nr].exdesc);

    if( get_obj_extra_desc_value(obj, "[DROP_DESC]", buf, sizeof(buf))) {
        obj->ldesc = strdup(buf);
    }

    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        obj->affected[i].location = obj_default[nr].affected[i].location;
        obj->affected[i].modifier = obj_default[nr].affected[i].modifier;
    }

    for (; (i < MAX_OBJ_AFFECT); i++) {
        obj->affected[i].location = CHAR_APPLY_NONE;
        obj->affected[i].modifier = 0;
    }

    obj->in_room = (ROOM_DATA *) 0;

    obj->prev_content = (OBJ_DATA *) 0;

    obj->next_content = (OBJ_DATA *) 0;

    obj->wagon_master = (OBJ_DATA *) 0;

    obj->carried_by = (CHAR_DATA *) 0;
    obj->equipped_by = (CHAR_DATA *) 0;
    obj->in_obj = (OBJ_DATA *) 0;
    obj->contains = (OBJ_DATA *) 0;
    obj->nr = nr;

    object_list_insert(obj);

    obj->prev_num = (OBJ_DATA *) 0;
    obj->next_num = (OBJ_DATA *) obj_index[nr].list;
    obj_index[nr].list = obj;
    if (obj->next_num)
        obj->next_num->prev_num = obj;

    obj_index[nr].number++;

    obj->programs = copy_specials(obj_default[nr].programs);

    /* this needs a the obj->nr field filled in so at bottom */
    if (has_special_on_cmd(obj->programs, NULL, CMD_PULSE))
        new_event(EVNT_OBJ_PULSE, 40, 0, 0, obj, 0, 0);

    /* make boards into type rumor_board */
    if (is_board(obj_index[nr].vnum)) {
        obj->obj_flags.type = ITEM_RUMOR_BOARD;
    }
    return obj;
}

#define IS_BAD_APPLY(a)  ((a == 4) || (a == 19) || ((a >=6) && (a <= 11)))

/* read obj from data file */
char *
read_object_from_file(FILE * fl, struct obj_default_data *objdef, int pos)
{
    int tmp, i;
    static char chk[255];
    char *tmpstr;
    struct extra_descr_data *new_descr;

    fseek(fl, pos, 0);

    objdef->name = fread_string(fl);
    objdef->short_descr = fread_string(fl);
    objdef->long_descr = fread_string(fl);
    objdef->description = fread_string(fl);

    fscanf(fl, " %d ", &tmp);
    objdef->type = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->extra_flags = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->wear_flags = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->state_flags = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->material = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->value[0] = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->value[1] = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->value[2] = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->value[3] = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->value[4] = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->value[5] = tmp;

    /* FIX */
    fscanf(fl, " %d \n", &tmp);
    objdef->weight = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->cost = tmp;
    fscanf(fl, " %d\n", &tmp);
    objdef->temp = tmp;

    fscanf(fl, " %d ", &tmp);
    objdef->length = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->dam_num_dice = tmp;
    fscanf(fl, " %d ", &tmp);
    objdef->dam_dice_size = tmp;
    fscanf(fl, " %d\n", &tmp);
    objdef->dam_plus = tmp;

    objdef->programs = read_specials(fl);

    objdef->temp = 0;

    objdef->exdesc = 0;

    while (fgets(chk, sizeof(chk), fl), *chk == 'E') {
        CREATE(new_descr, struct extra_descr_data, 1);
        new_descr->keyword = fread_string(fl);
        if (!strcmp((new_descr->keyword + 1), objdef->name)
            || !strcmp(new_descr->keyword, objdef->name)) {
            /* use the already set one if not null */
            if (objdef->description && objdef->description[0] == '\0') {
                free(objdef->description);
                objdef->description = NULL;
            }

            if (!objdef->description) {
                objdef->description = fread_string(fl);
            } else {
                tmpstr = fread_string(fl);
                free(tmpstr);
            }

            free(new_descr->keyword);
            free(new_descr);
        } else {
            new_descr->description = fread_string(fl);
            new_descr->def_descr = 1;
            new_descr->next = objdef->exdesc;
            objdef->exdesc = new_descr;
            global_game_stats.edescs++;
        }
    }

    for (i = 0; (i < MAX_OBJ_AFFECT) && (*chk == 'A'); i++) {
        fscanf(fl, " %d ", &tmp);
        if (!IS_BAD_APPLY(tmp)) {
            objdef->affected[i].location = tmp;
            fscanf(fl, " %d \n", &tmp);
            objdef->affected[i].modifier = tmp;
            fgets(chk, sizeof(chk), fl);
        } else {
            objdef->affected[i].location = 0;
            objdef->affected[i].modifier = 0;
            fscanf(fl, " %d \n", &tmp);
            fgets(chk, sizeof(chk), fl);
        }

        if (objdef->affected[i].location == 19)
            if (objdef->type != ITEM_WEAPON) {
                objdef->affected[i].location = 0;
                objdef->affected[i].modifier = 0;
            }
    }

    for (; (i < MAX_OBJ_AFFECT); i++) {
        objdef->affected[i].location = CHAR_APPLY_NONE;
        objdef->affected[i].modifier = 0;
    }

    /* hack to fix 'max_points' on armor */
    if (objdef->type == ITEM_ARMOR) {
        if (objdef->value[3] == 0) {
            objdef->value[3] = objdef->value[1];
        }
    }


    return chk;
}


/* copy object data into default structure */
void
obj_update(OBJ_DATA * obj)
{
    int nr = obj->nr;
    int i;

    if (nr < 0)
        return;

    if (obj_default[nr].name && obj->name) {
        free((char *) obj_default[nr].name);
        obj_default[nr].name = obj->name;
        obj->name = NULL;
    }

    if (obj_default[nr].short_descr && obj->short_descr) {
        free((char *) obj_default[nr].short_descr);
        obj_default[nr].short_descr = strdup(obj->short_descr);
        obj->short_descr = NULL;
    }

    if (obj_default[nr].long_descr && obj->long_descr) {
        free((char *) obj_default[nr].long_descr);
        obj_default[nr].long_descr = obj->long_descr;
        obj->long_descr = NULL;
    }

    if (obj_default[nr].description && obj->description) {
        free((char *) obj_default[nr].description);
        obj_default[nr].description = NULL;
    }

    if (!obj_default[nr].description && obj->description) {
        obj_default[nr].description = obj->description;
        obj->description = NULL;
    }


    obj_default[nr].type = obj->obj_flags.type;
    obj_default[nr].extra_flags = obj->obj_flags.extra_flags;
    obj_default[nr].wear_flags = obj->obj_flags.wear_flags;
    obj_default[nr].state_flags = obj->obj_flags.state_flags;
    for (i = 0; i < 6; i++)
        obj_default[nr].value[i] = obj->obj_flags.value[i];

    /*if (obj->obj_flags.type == ITEM_NOTE)
     * obj_default[nr].value[1] = 0; */
    obj_default[nr].weight = obj->obj_flags.weight;
    /* HACK */
    obj_default[nr].size = 0;

    obj_default[nr].cost = obj->obj_flags.cost;
    obj_default[nr].material = obj->obj_flags.material;
    obj_default[nr].length = obj->obj_flags.length;
    obj_default[nr].dam_dice_size = obj->dam_dice_size;
    obj_default[nr].dam_num_dice = obj->dam_num_dice;
    obj_default[nr].dam_plus = obj->dam_plus;

    free_edesc_list(obj_default[nr].exdesc);
    obj_default[nr].exdesc = copy_edesc_list(obj->ex_description);

    free_specials(obj_default[nr].programs);
    obj_default[nr].programs = copy_specials(obj->programs);

    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        obj_default[nr].affected[i].location = obj->affected[i].location;
        obj_default[nr].affected[i].modifier = obj->affected[i].modifier;
    }
}

/* copy npc data into default structure */
void
npc_update(CHAR_DATA * mob)
{
    int nr = mob->nr, sk;
    CLAN_DATA *pClan, *pClan2;

    if (npc_default[nr].name && mob->name) {
        free(npc_default[nr].name);
        npc_default[nr].name = NULL;
    }

    if (mob->name) {
        npc_default[nr].name = strdup(mob->name);
        mob->name = NULL;
    }

    if (npc_default[nr].short_descr && mob->short_descr) {
        free(npc_default[nr].short_descr);
        npc_default[nr].short_descr = NULL;
    }

    if (mob->short_descr) {
        npc_default[nr].short_descr = strdup(mob->short_descr);
        mob->short_descr = NULL;
    }

    if (npc_default[nr].long_descr && mob->long_descr) {
        free(npc_default[nr].long_descr);
        npc_default[nr].long_descr = NULL;
    }

    if (mob->long_descr) {
        npc_default[nr].long_descr = strdup(mob->long_descr);
        mob->long_descr = NULL;
    }

    if (npc_default[nr].description && mob->description) {
        free(npc_default[nr].description);
        npc_default[nr].description = NULL;
    }

    if (mob->description) {
        npc_default[nr].description = strdup(mob->description);
        mob->description = NULL;
    }

    if (npc_default[nr].background && mob->background) {
        free(npc_default[nr].background);
        npc_default[nr].background = NULL;
    }

    if (mob->background) {
        npc_default[nr].background = strdup(mob->background);
        mob->background = NULL;
    }

    npc_default[nr].act = mob->specials.act;
    npc_default[nr].flag = mob->specials.flag;
    npc_default[nr].affected_by = mob->specials.affected_by;
    npc_default[nr].language = mob->specials.language;
    npc_default[nr].level = mob->player.level;
    npc_default[nr].off = mob->abilities.off;
    npc_default[nr].def = mob->abilities.def;
    npc_default[nr].armor = mob->abilities.armor;
    npc_default[nr].max_hit = mob->points.max_hit;
    npc_default[nr].dam = mob->abilities.damroll;
    npc_default[nr].obsidian = mob->points.obsidian;
    npc_default[nr].gp_mod = mob->specials.gp_mod;

    if (GET_POS(mob) != POSITION_FIGHTING) {
        npc_default[nr].pos = GET_POS(mob);
        npc_default[nr].def_pos = GET_POS(mob);
    } else {
        npc_default[nr].pos = POSITION_STANDING;
        npc_default[nr].def_pos = POSITION_STANDING;
    }

    npc_default[nr].sex = mob->player.sex;
    npc_default[nr].guild = mob->player.guild;
    npc_default[nr].sub_guild = SUB_GUILD_NONE;
    npc_default[nr].tribe = mob->player.tribe;

    /* clear the old default clans */
    for (pClan = npc_default[nr].clan; pClan; pClan = pClan2) {
        pClan2 = pClan->next;
        free_clan_data(pClan);
    }
    npc_default[nr].clan = NULL;

    /* iterate through the mob's clans and add them to default */
    for (pClan = mob->clan; pClan; pClan = pClan->next) {
        CLAN_DATA *tmp;

        /* create the space and copy fields */
        pClan2 = new_clan_data();
        pClan2->clan = pClan->clan;
        pClan2->rank = pClan->rank;
        pClan2->flags = pClan->flags;

        /* find the end of the list and tack it on it */
        if (!npc_default[nr].clan) {    /* first clan in the list */
            pClan2->next = NULL;
            npc_default[nr].clan = pClan2;
        } else {                /* everyone else, search for the end */

            for (tmp = npc_default[nr].clan; tmp->next; tmp = tmp->next);
            pClan2->next = NULL;
            tmp->next = pClan2;
        }
    }

    npc_default[nr].luck = mob->player.luck;
    npc_default[nr].race = mob->player.race;
    npc_default[nr].age = age(mob).year;

    free_edesc_list(npc_default[nr].exdesc);
    npc_default[nr].exdesc = copy_edesc_list(mob->ex_description);
    free_specials(npc_default[nr].programs);

    npc_default[nr].programs = copy_specials(mob->specials.programs);

    for (sk = 0; sk < MAX_SKILLS; sk++) {
        if (has_skill(mob, sk))
            /*     if (mob->skills[sk]) */
            npc_default[nr].skills[sk].learned = mob->skills[sk]->learned;
        else
            npc_default[nr].skills[sk].learned = 0;
    }
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void
zone_update(void)
{
    int i;
    struct reset_q_element *update_u, *temp;
    struct timeval start, now;

    perf_enter(&start, "zone_update()");

    gettimeofday(&now, NULL);

    for (i = 0; i <= top_of_zone_table; i++) {
        if (zone_table[i].is_booted) {
            zone_table[i].updated = now.tv_sec+(now.tv_usec/1000000.0);
            if ((zone_table[i].age < zone_table[i].lifespan) && zone_table[i].reset_mode) {
                zone_table[i].age++;
            }
            else if (zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
                CREATE(update_u, struct reset_q_element, 1);

                update_u->zone_to_reset = i;
                update_u->next = 0;

                if (!reset_q.head)
                    reset_q.head = reset_q.tail = update_u;
                else {
                    reset_q.tail->next = update_u;
                    reset_q.tail = update_u;
                }
                zone_table[i].age = ZO_DEAD;
            }
        }
    }

    for (update_u = reset_q.head; update_u; update_u = update_u->next)
        if (((zone_table[update_u->zone_to_reset].reset_mode == 2)
             || is_empty(update_u->zone_to_reset))
            && (zone_table[update_u->zone_to_reset].reset_mode != 0)
            && (zone_table[update_u->zone_to_reset].reset_mode != 3)
            && zone_table[update_u->zone_to_reset].is_booted) {
            reset_zone(update_u->zone_to_reset, FALSE);

            if (update_u == reset_q.head)
                reset_q.head = reset_q.head->next;
            else {
                for (temp = reset_q.head; temp->next != update_u; temp = temp->next);

                if (!update_u->next)
                    reset_q.tail = temp;

                temp->next = update_u->next;
            }

            free(update_u);
        }
    perf_exit("zone_update()", start);
}

void
set_obj_flags_from_cmd(OBJ_DATA *obj, struct reset_com *cmd)
{
    int stored_light_flags = 0, light_type = 0, light_range = 0, light_color = 0, max_amount = 0, max_points = 0;

    if (GET_ITEM_TYPE(obj) == ITEM_LIGHT) {
        stored_light_flags = obj->obj_flags.value[5] & (LIGHT_FLAG_CAMPFIRE | LIGHT_FLAG_TORCH | LIGHT_FLAG_LAMP | LIGHT_FLAG_LANTERN | LIGHT_FLAG_CANDLE | LIGHT_FLAG_REFILLABLE | LIGHT_FLAG_CONSUMES);
        max_amount = obj->obj_flags.value[1];
        light_type = obj->obj_flags.value[2];
        light_range = obj->obj_flags.value[3];
        light_color = obj->obj_flags.value[4];
    }
    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR) {
        max_points = obj->obj_flags.value[3];
    }
 
    int asz = 0;
    int csz = 0;

    // hold what the size of the item was
    if (GET_ITEM_TYPE(obj) == ITEM_WORN)
        asz = obj->obj_flags.value[0];
    else if( GET_ITEM_TYPE(obj) == ITEM_ARMOR)
        asz = obj->obj_flags.value[2];

    int i; 
    for (i = 0; i < 6; i++)
        obj->obj_flags.value[i] = cmd->arg4[i];

    // get the new size
    if (GET_ITEM_TYPE(obj) == ITEM_WORN)
        csz = obj->obj_flags.value[0];
    else if( GET_ITEM_TYPE(obj) == ITEM_ARMOR)
        csz = obj->obj_flags.value[2];

    // adjust weight for size change
    if( asz > 0 && asz != csz ) {
        obj->obj_flags.weight = obj->obj_flags.weight * csz / asz;
    }
    
    obj->obj_flags.value[5] |= stored_light_flags;
    
    if (GET_ITEM_TYPE(obj) == ITEM_LIGHT) {
        if (obj->obj_flags.value[1] == 0)
            obj->obj_flags.value[1] = max_amount;
        if (obj->obj_flags.value[2] == 0)
            obj->obj_flags.value[2] = light_type;
        if (obj->obj_flags.value[3] == 0)
            obj->obj_flags.value[3] = light_range;
        if (obj->obj_flags.value[4] == 0)
            obj->obj_flags.value[4] = light_color;
    }
    
    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR)
        if (obj->obj_flags.value[3] == 0)
            obj->obj_flags.value[3] = max_points;
    
    obj->obj_flags.state_flags = cmd->state_flags;
}

/* execute the reset command table of a given zone */
void
reset_zone(int zone, int unlimited_spawn)
{
    int cmd_no, last_cmd = 1, i;
    struct timeval now;
    ROOM_DATA *room = NULL;
    CHAR_DATA *mob = NULL, *last_npc = NULL;
    /*
     * CHAR_DATA *tmpch;
     * OBJ_DATA *next_obj;
     */
    OBJ_DATA *obj = NULL, *obj_to = NULL, *last_obj = NULL, *last_around = NULL;
    int last_depth;
    int this_depth;

    gettimeofday(&now, NULL);
    double save_time = zone_table[zone].time + (zone_table[zone].lifespan * 60);
    double cur_time = now.tv_sec+(now.tv_usec/1000000.0);

    if (cur_time >= save_time){  
        if (zone_table[zone].func) {
            (void) (*zone_table[zone].func) (zone);
            zone_table[zone].time = now.tv_sec+(now.tv_usec/1000000.0);
         }
    }

    if (!zone_table[zone].cmd) {
        gamelogf("No data for zone %d, skipping reset", zone);
        return;
    }

    qgamelogf(QUIET_ZONE, "Resetting zone %d: %s", zone, zone_table[zone].name);

    for (cmd_no = 0;; cmd_no++) {
        struct reset_com ZCMD;
        memcpy(&ZCMD, &zone_table[zone].cmd[cmd_no], sizeof(ZCMD));

        if (ZCMD.command == 'S')
            break;

        if (last_cmd || !ZCMD.if_flag)
            switch (ZCMD.command) {
            case 'M':
                if ((unlimited_spawn || mobs_existing_in_zone(ZCMD.arg1) < ZCMD.arg2)
                    && ((npc_index[ZCMD.arg1].limit == -1)
                        || (npc_index[ZCMD.arg1].number < npc_index[ZCMD.arg1].limit))) {
                    if (NULL != (mob = read_mobile(ZCMD.arg1, REAL))) {
                        if (NULL != (room = get_room_num(ZCMD.arg3))) {
                            char_to_room(mob, room);
                            mob->saved_room = room->number;
                            last_cmd = 1;
                            last_npc = mob;

                        } else {
                            gamelog("Attempting to load mobile to nonexistant room.");
                            extract_char(mob);
                            mob = 0;
                            last_cmd = 0;
                            last_npc = 0;
                        }
                    } else {
                        gamelog("Attempting to load nonexistant mobile to room.");
                        last_cmd = 0;
                        mob = 0;
                        last_npc = 0;
                    }
                } else {
//                    shhlogf("NPC %d excluded from zone reset in room %d", ZCMD.arg1, ZCMD.arg3);
                    last_cmd = 0;
                    mob = 0;
                    last_npc = 0;
                }
                break;

            case 'O':
                if ((ZCMD.arg1 >= 0) && (ZCMD.arg1 < top_of_obj_t)
                    && ((obj_index[ZCMD.arg1].limit == -1)
                        || (obj_index[ZCMD.arg1].number < obj_index[ZCMD.arg1].limit))) {
                    if (NULL != (room = get_room_num(ZCMD.arg3))) {
                        if (objs_existing_in_room(ZCMD.arg1, room) < ZCMD.arg2) {
                            if (NULL != (obj = read_object(ZCMD.arg1, REAL))) {
#ifdef WAGON_SAVE
                                /* uncomment after first boot */
                                if (obj->obj_flags.type == ITEM_WAGON) {
                                    extract_obj(obj);
                                    last_obj = 0;
                                    last_cmd = 0;
                                    break;
                                }
                                /**/
#endif
                                set_obj_flags_from_cmd(obj, &ZCMD);
                                obj_to_room(obj, room);

                                if (obj->obj_flags.type == ITEM_NOTE)
                                    read_note(obj);
                                last_cmd = 1;
                                last_obj = obj;

                                if (obj->obj_flags.type == ITEM_FURNITURE
                                    && IS_SET(obj->obj_flags.value[1], FURN_TABLE))
                                    last_around = obj;
                                else 
                                    last_around = 0;

                                if (GET_ITEM_TYPE(obj) == ITEM_LIGHT && IS_LIT(obj))
                                    increment_room_light(room, obj->obj_flags.value[4]);
                            } else {
                                extract_obj(obj);
                                obj = 0;
                                gamelog("Attempting to load nonexistant object to room.");
                                last_cmd = 0;
                                last_obj = 0;
                                last_around = 0;
                            }
                        }
                    } else {
                        gamelog("Attempting to load object to nonexistant room.");
                        last_cmd = 0;
                        last_obj = 0;
                        last_around = 0;
                    }
                } else {
                    last_cmd = 0;
                    last_obj = 0;
                    last_around = 0;
                }
                break;

            case 'P':
                /* code for stuff put in stuff                    */
                /* the 'flag' has turned into 'depth' here,       */
                /* the last object's depth can be counted out     */
                /* until it is no longer in an item.              */
                /* if the current item to load is a last item +1  */
                /*   put it in last item                          */
                /* else                                           */
                /*   count out last-item's depth - this depth     */
                /*   (if equal put in last-item's container)      */

                this_depth = ZCMD.if_flag;

                for (last_depth = 0, obj_to = last_obj; (last_depth < 1000) && obj_to && obj_to->in_obj; obj_to = obj_to->in_obj, last_depth++) {       /* do nothing, count up */
                }

                for (i = last_depth + 1, obj_to = last_obj; i > this_depth && obj_to && obj_to->in_obj; i--, obj_to = obj_to->in_obj) { /* do nothing, count out */
                }

                if (obj_to && (obj_to->obj_flags.type == ITEM_CONTAINER || obj_to->obj_flags.type == ITEM_FURNITURE) && (obj_to->nr != 0)) {
                    /*
                     * if ((ZCMD.arg1 >= 0) && (ZCMD.arg1 < top_of_obj_t) &&
                     * (objs_existing_in_list (ZCMD.arg1, obj_to->contains) <
                     * ZCMD.arg2) && ((obj_index[ZCMD.arg1].limit == -1) ||
                     * (obj_index[ZCMD.arg1].number <
                     * obj_index[ZCMD.arg1].limit)))
                     * {
                     */
                    if ((NULL != (obj = read_object(ZCMD.arg1, REAL))) && (obj->nr != 0)) {
                        obj_to_obj(obj, obj_to);
                        set_obj_flags_from_cmd(obj, &ZCMD);

                        if (obj->obj_flags.type == ITEM_NOTE)
                            read_note(obj);
                        last_obj = obj;
                        last_cmd = 1;

                        if (obj->obj_flags.type == ITEM_FURNITURE
                            && (IS_SET(obj->obj_flags.value[1], FURN_TABLE)))
                            last_around = obj;

                        if (GET_ITEM_TYPE(obj_to) == ITEM_FURNITURE
                            && IS_SET(obj_to->obj_flags.value[1], FURN_PUT)
                            && obj_to->in_room) {
                            if (GET_ITEM_TYPE(obj) == ITEM_LIGHT && IS_LIT(obj))
                                increment_room_light(room, obj->obj_flags.value[4]);
                        }
                    } else {
                        if (obj) {
                            extract_obj(obj);
                            obj = 0;
                            last_obj = 0;
                            last_around = 0;
                        }
                        gamelogf("Attempting to put nonexistant object (%d) in another (%d).",
                                 ZCMD.arg1, obj_to->nr);
                        last_obj = 0;
                        last_around = 0;
                        last_cmd = 0;
                    }
                }
                break;

            case 'X':          /* extra info on last object */
                switch (ZCMD.arg1) {
                case 1:        /* extra description on object */
                    if (last_cmd && last_obj && ZCMD.stringarg1 && ZCMD.stringarg2) {
                        set_obj_extra_desc_value(last_obj, ZCMD.stringarg1, ZCMD.stringarg2);
                        if( !strcmp( "[DROP_DESC]", ZCMD.stringarg1 ) ) {
                            if( obj->ldesc != NULL ) {
                                free( obj->ldesc );
                            }
                            obj->ldesc = strdup( ZCMD.stringarg2 );
                        }
                    }
                    break;

                case 2:        /* sdesc on object */
                case 3:        /* ldesc on object  */
                case 4:        /* keywords on object */
                    break;
                }
                break;
            case 'Y':          /* extra info on last NPC */
                if (last_cmd && last_npc) {
                    set_char_extra_desc_value(last_npc, ZCMD.stringarg1, ZCMD.stringarg2);
                }
                break;
            case 'A':

                /* code for stuff put around stuff                */
                /* the 'flag' has turned into 'depth' here,       */
                /* the last object's depth can be counted out     */
                /* until it is no longer in an item.              */
                /* if the current item to load is a last item +1  */
                /*   put it in last item                          */
                /* else                                           */
                /*   count out last-item's depth - this depth     */
                /*   (if equal put in last-item's table)          */

                this_depth = ZCMD.if_flag;

                for (last_depth = 0, obj_to = last_around; (last_depth < 1000) && obj_to && obj_to->table; obj_to = obj_to->table, last_depth++) {      /* do nothing, count up */
                }

                for (i = last_depth + 1, obj_to = last_around; i > this_depth && obj_to && obj_to->table; i--, obj_to = obj_to->table) {        /* do nothing, count out */
                }

                if (obj_to && (obj_to->nr != 0)) {
                    if ((NULL != (obj = read_object(ZCMD.arg1, REAL))) && (obj->nr != 0)) {
#if 0
                        shhlogf("ZONE_RESET: obj_to's number r%d, v%d, around's number r%d v%d.",
                                obj->nr, obj_index[obj->nr].vnum, obj_to->nr,
                                obj_index[obj_to->nr].vnum);
#endif
                        set_obj_flags_from_cmd(obj, &ZCMD);
                        obj_around_obj(obj, obj_to);

                        if (obj->obj_flags.type == ITEM_FURNITURE
                            && IS_SET(obj->obj_flags.value[1], FURN_TABLE))
                            last_around = obj;
                        last_cmd = 1;
                    } else {
                        if (obj) {
                            extract_obj(obj);
                            obj = 0;
                            last_obj = 0;
                            last_around = 0;
                        }
                        gamelog("Attempting to put nonexistant object in another.");
                        obj = 0;
                        last_obj = 0;
                        last_cmd = 0;
                        last_around = 0;
                    }
                }
                break;
            case 'G':
                if ((ZCMD.arg1 >= 0) && (ZCMD.arg1 < top_of_obj_t) && mob
                    && (objs_existing_in_list(ZCMD.arg1, mob->carrying) < ZCMD.arg2)
                    && ((obj_index[ZCMD.arg1].limit == -1)
                        || (obj_index[ZCMD.arg1].number < obj_index[ZCMD.arg1].limit))) {
                    if (NULL != (obj = read_object(ZCMD.arg1, REAL))) {
                        set_obj_flags_from_cmd(obj, &ZCMD);
                        obj_to_char(obj, mob);

                        if (obj->obj_flags.type == ITEM_NOTE)
                            read_note(obj);
                        last_cmd = 1;
                        last_obj = obj;
                    } else {
                        gamelog("Attempting to give nonexistant object to mobile.");
                        last_cmd = 0;
                        last_obj = 0;
                    }
                } else {
                    obj = 0;
                    last_cmd = 0;
                    last_obj = 0;
                }

                break;
            case 'E':
                if ((ZCMD.arg1 >= 0) && (ZCMD.arg1 < top_of_obj_t) && mob
                    && !mob->equipment[ZCMD.arg3] && ((obj_index[ZCMD.arg1].limit == -1)
                                                      || (obj_index[ZCMD.arg1].number <
                                                          obj_index[ZCMD.arg1].limit))) {
                    if (NULL != (obj = read_object(ZCMD.arg1, REAL))) {
                        set_obj_flags_from_cmd(obj, &ZCMD);
                        equip_char(mob, obj, ZCMD.arg3);

                        if (GET_ITEM_TYPE(obj) == ITEM_LIGHT) {
                            light_object(mob, obj, "");
                        }
                        if (obj->obj_flags.type == ITEM_NOTE)
                            read_note(obj);
                        last_cmd = 1;
                        last_obj = obj;
                    } else {
                        gamelog("Attempting to equip nonexistant object to " "mobile.");
                        last_cmd = 0;
                        last_obj = 0;
                    }
                } else {
                    last_cmd = 0;
                    last_obj = 0;
                };
                break;
            case 'D':
                if (!done_booting) {
                    if ((ZCMD.arg1 == -1) || !(room = get_room_num(ZCMD.arg1))
                        || !room->direction[ZCMD.arg2]) {
                        gamelogf("Bad 'D' cmd in zone reset.  THIS SHOULD NEVER HAPPEN.");
                        break;
                    }
                    switch (ZCMD.arg3) {
                    case 0:
                        REMOVE_BIT(room->direction[ZCMD.arg2]->exit_info, EX_LOCKED);
                        REMOVE_BIT(room->direction[ZCMD.arg2]->exit_info, EX_CLOSED);
                        break;
                    case 1:
                        MUD_SET_BIT(room->direction[ZCMD.arg2]->exit_info, EX_CLOSED);
                        REMOVE_BIT(room->direction[ZCMD.arg2]->exit_info, EX_LOCKED);
                        break;
                    case 2:
                        MUD_SET_BIT(room->direction[ZCMD.arg2]->exit_info, EX_LOCKED);
                        MUD_SET_BIT(room->direction[ZCMD.arg2]->exit_info, EX_CLOSED);
                        break;
                    }
                    last_cmd = 1;
                }
                break;

            default:
                gamelogf("Undefined cmd in reset table; zone %d cmd %d.\n\r", zone, cmd_no);
                exit(0);
        } else
            last_cmd = 0;

    }

    zone_table[zone].age = 0;
}

#undef ZCMD

/* checks to see if zone is empty */
int
is_empty(int zone_nr)
{
    struct descriptor_data *i;

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected)
            if (i->character->in_room->zone == zone_nr)
                return (0);

    return (1);
}



bool
player_exists(char *name, char *account, int status)
{
    int i;
    char buf[256];
    DESCRIPTOR_DATA d;
    CHAR_INFO *pChInfo;
#define PLAYER_EXISTS_ASSUME_FALSE
#ifdef PLAYER_EXISTS_ASSUME_FALSE
    /* sometimes fOld might be set to true, and throw off the free below */
    /* 2002.12.23  -Tenebrius                                            */
    bool fOld = FALSE, found;
#else
    bool fOld, found;
#endif



    strcpy(buf, name);
    for (i = 0; i < strlen(buf); i++)
        buf[i] = LOWER(buf[i]);

    /* search for existing player_info's first */
    if (((d.player_info = find_player_info(account)) == NULL))
        if ((fOld = load_player_info(&d, account)) == FALSE)
            return FALSE;

    /* found a player_info (already loaded or pulled from file) */
    if (!(pChInfo = lookup_char_info(d.player_info, name))) {
        if (fOld)
            free_player_info(d.player_info);
        return FALSE;
    }

    /* found the character, now compare status */

    if (status == -1)
        found = TRUE;
    else if (status == pChInfo->status)
        found = TRUE;
    else
        found = FALSE;

    if (fOld)
        free_player_info(d.player_info);

    return found;
}

/* merge sort routines for rooms, jdb */
ROOM_DATA *
split(ROOM_DATA * l)
{
    int i, k;
    ROOM_DATA *n, *p;

    if (!l)
        return 0;

    for (i = 0, n = l; n; n = n->next, i++);
    k = ((i - 1) / 2) + 1;

    for (i = 1, n = l; i < k; n = n->next, i++);
    p = n->next;
    n->next = 0;

    return p;
}

ROOM_DATA *
merge(ROOM_DATA * a, ROOM_DATA * b)
{
    ROOM_DATA *new = 0, *correct = 0, *next, *p;

    while (a || b) {
        if (a && b) {
            if (a->number < b->number) {
                p = a->next;
                a->next = new;
                new = a;
                a = p;
            } else {
                p = b->next;
                b->next = new;
                new = b;
                b = p;
            }
        } else if (a) {
            p = a->next;
            a->next = new;
            new = a;
            a = p;
        } else {
            p = b->next;
            b->next = new;
            new = b;
            b = p;
        }
    }

    for (p = new; p; p = next) {
        next = p->next;

        p->next = correct;
        correct = p;
    }

    return correct;
}

ROOM_DATA *
sort_rooms(ROOM_DATA * l)
{
    ROOM_DATA *left, *right, *new;

    /* split into left and right */
    left = l;
    right = split(left);

    if (left && left->next)
        left = sort_rooms(left);
    if (right && right->next)
        right = sort_rooms(right);

    new = merge(left, right);

    return new;
}

ROOM_DATA *
split_in_zone(ROOM_DATA * l)
{
    int i, k;
    ROOM_DATA *n, *p;

    if (!l)
        return 0;

    for (i = 0, n = l; n; n = n->next_in_zone, i++);
    k = ((i - 1) / 2) + 1;

    for (i = 1, n = l; i < k; n = n->next_in_zone, i++);
    p = n->next_in_zone;
    n->next_in_zone = 0;

    return p;
}

ROOM_DATA *
merge_in_zone(ROOM_DATA * a, ROOM_DATA * b)
{
    ROOM_DATA *new = 0, *correct = 0, *next, *p;

    while (a || b) {
        if (a && b) {
            if (a->number < b->number) {
                p = a->next_in_zone;
                a->next_in_zone = new;
                new = a;
                a = p;
            } else {
                p = b->next_in_zone;
                b->next_in_zone = new;
                new = b;
                b = p;
            }
        } else if (a) {
            p = a->next_in_zone;
            a->next_in_zone = new;
            new = a;
            a = p;
        } else {
            p = b->next_in_zone;
            b->next_in_zone = new;
            new = b;
            b = p;
        }
    }

    for (p = new; p; p = next) {
        next = p->next_in_zone;

        p->next_in_zone = correct;
        correct = p;
    }

    return correct;
}

ROOM_DATA *
sort_rooms_in_zone(ROOM_DATA * l)
{
    ROOM_DATA *left, *right, *new;

    /* split into left and right */
    left = l;
    right = split_in_zone(left);

    if (left && left->next_in_zone)
        left = sort_rooms_in_zone(left);
    if (right && right->next_in_zone)
        right = sort_rooms_in_zone(right);

    new = merge_in_zone(left, right);

    return new;
}

/* set room to watch room */
int
set_room_to_watch_room(ROOM_DATA * watched_room, ROOM_DATA * watching_room, const char *msg_string,
                       int type)
{
    struct watching_room_list *temp;

    // See if we have a node on the list already
    for (temp = watching_room->watching; temp; temp = temp->next_watching)
        if (temp->room_watched == watched_room)
            break;

    // Try to make a new node
    if (!temp) {
        temp = (struct watching_room_list *)
            calloc(1, sizeof(struct watching_room_list));

        if (!temp) {
            errorlog("Couldn't malloc a watching_room_list.");
            return (0);
        }

        // It's a new node, set up the basic info in the node
        temp->room_watching = watching_room;
        temp->watch_room_num = watched_room->number;
        temp->room_watched = watched_room;

        // Insert into the watching list
        temp->next_watching = watching_room->watching;
        watching_room->watching = temp;

        temp->next_watched_by = watched_room->watched_by;
        watched_room->watched_by = temp;
    }

    // Update the view msg
    if (IS_SET(type, WATCH_VIEW))
        strncpy(temp->view_msg_string, msg_string, sizeof(temp->view_msg_string));
    // Update the listen msg
    if (IS_SET(type, WATCH_LISTEN))
        strncpy(temp->listen_msg_string, msg_string, sizeof(temp->listen_msg_string));

    // Update the type
    temp->watch_type |= type;

    return (1);
}

int
set_room_to_watch_room_resolve(struct watching_room_list *temp)
{
    ROOM_DATA *watched_room;

    if (!temp) {
        errorlog("Given a bad watching_room_list_element.");
        return (0);
    }

    watched_room = get_room_num(temp->watch_room_num);
    if (!watched_room) {
        errorlogf("Could not find room %d, to set watching.", temp->watch_room_num);
        return (0);
    }

    temp->room_watched = watched_room;
    temp->next_watched_by = watched_room->watched_by;
    watched_room->watched_by = temp;

    return (1);
}

int
set_room_to_watch_room_num(int watched_room, ROOM_DATA * watching_room, const char *msg_string, int type)
{
    struct watching_room_list *temp;

    // See if we have a node on the list already
    for (temp = watching_room->watching; temp; temp = temp->next_watching)
        if (temp->watch_room_num == watched_room)
            break;

    // Doesn't exist.  Probably not loaded yet, so build a node
    if (!temp) {
        temp = (struct watching_room_list *)
            calloc(1, sizeof(struct watching_room_list));

        if (!temp) {
            errorlog("Couldn't malloc a watching_room_list.");
            return (0);
        }

        temp->room_watching = watching_room;
        temp->room_watched = 0;
        temp->watch_room_num = watched_room;

        temp->next_watching = watching_room->watching;
        watching_room->watching = temp;

        temp->next_watched_by = 0;
    }

    // Update the view msg
    if (IS_SET(type, WATCH_VIEW))
        strncpy(temp->view_msg_string, msg_string, sizeof(temp->view_msg_string));
    // Update the listen msg
    if (IS_SET(type, WATCH_LISTEN))
        strncpy(temp->listen_msg_string, msg_string, sizeof(temp->listen_msg_string));

    temp->watch_type |= type;

    return (1);
}

/* stop room from watching room */
int
set_room_not_watching_room(ROOM_DATA * watched_room, ROOM_DATA * watching_room, int type)
{
    struct watching_room_list *temp, *remove;

    for (temp = watched_room->watched_by; temp; temp = temp->next_watched_by)
        if ((temp->room_watching == watching_room) && (IS_SET(temp->watch_type, type)))
            break;

    if (!temp) {
        errorlog("Couldn't find the watched-watching room combo in set_room_not_watching_room");
        return (0);
    }

    // Remove the flag
    REMOVE_BIT(temp->watch_type, type);

    // Still has other bits set, don't remove it from the list
    if (temp->watch_type)
        return (1);

    remove = temp;

/***************************************/
/**** Remove from the watching list ****/
/***************************************/
    if (remove == watched_room->watched_by) {
        watched_room->watched_by = remove->next_watched_by;
        remove->next_watched_by = 0;
    } else {
        for (temp = watched_room->watched_by; temp && (temp->next_watched_by != remove); temp = temp->next_watched_by) {        /* do nothing, wait for match */
        }

        if (!temp) {
            errorlog("Couldn't find the watched-watching room combo in set_room_not_watching_room");
            return (0);
        }

        temp->next_watched_by = remove->next_watched_by;
        remove->next_watched_by = 0;
    }

/***************************************/
/**** Remove from the watched list  ****/
/***************************************/
    if (remove == watching_room->watching) {
        watching_room->watching = remove->next_watching;
        remove->next_watching = 0;
    } else {
        for (temp = watching_room->watching; temp && (temp->next_watching != remove); temp = temp->next_watching) {     /* do nothing, wait for match */
        }

        if (!temp) {
            errorlog("Couldn't find the watched-watching room combo in set_room_not_watching_room(2)");
            return (0);
        }

        temp->next_watching = remove->next_watching;
        remove->next_watching = 0;
    }

    /* 2002.12.14 added remove -JRT, picked up by valgrind */
    //  free(remove);

    /*
     * temp->room_watching = watching_room;
     * temp->room_watched  = watched_room;
     * 
     * temp->next_watching = watching_room->watching;
     * watching_room->watching = temp;
     * 
     * temp->next_watched = watched_room->watched_by;
     * watched_room->watched_by = temp;
     */


    /* do not free it for right now, in order to help prevent crashees 
     * during development */

    return (1);
}

void add_room_to_zone_in_order(struct zone_data *zdata, ROOM_DATA * room);


/* read a room from file */
int
read_room(int zone, FILE * fl)
{
    int d;
    char *com;
    char *watch_string;
    int watch_room;
    char tc;
    ROOM_DATA *room = (ROOM_DATA *) 0;
    struct room_direction_data *dir;
    struct extra_descr_data *edesc;
    /*
     * char c;
     * char buf[256];
     * int eds, i;
     */

    if (ftell(fl) == 0) {
        if (*(com = fread_text(fl)) == 'x') {
            free(com);
            return FALSE;
        } else
            free(com);
    }

    CREATE(room, ROOM_DATA, 1);

    room->name = fread_text(fl);
    room->number = fread_var(fl);
    room->sector_type = fread_var(fl);
    room->room_flags = fread_var(fl);
    room->bdesc_index = fread_var(fl);

    room->watching = 0;
    room->watched_by = 0;
    room->zone = zone;

    fscanf(fl, "\n");

    room->specials = read_specials(fl);

    /* read newline */
    room->description = fread_string(fl);
    /*while (*room->description == '\n' || *room->description == '\r')
     * room->description++; */

    for (d = 0; d < 6; d++)
        room->direction[d] = (struct room_direction_data *) 0;
    room->ex_description = 0;


    /* intialize the tracks */
    for (d = 0; d < MAX_TRACKS; d++) {
        room->tracks[d].type = TRACK_NONE;
    }

/*  room->program = get_room_program(room->number); */

    while ((*(com = fread_text(fl)) != 'n') && (*com != 'x')) {
        switch (*com) {
        case 'c':
            room->city = fread_var(fl);
            fscanf(fl, "\n");
            break;

        case 'd':
            d = fread_var(fl);

            CREATE(dir, struct room_direction_data, 1);
            dir->to_room_num = fread_var(fl);
            dir->keyword = fread_text(fl);
            dir->exit_info = fread_var(fl);
            dir->key = fread_var(fl);
            dir->general_description = fread_string(fl);
            dir->to_room = (ROOM_DATA *) 0;

            room->direction[d] = dir;

            break;

        case 'e':
            global_game_stats.edescs++;
            CREATE(edesc, struct extra_descr_data, 1);
            edesc->keyword = fread_text(fl);
            edesc->description = fread_string(fl);
            edesc->next = room->ex_description;
            room->ex_description = edesc;
            break;

        case 'w':
            watch_room = fread_var(fl);
            watch_string = fread_text(fl);

            set_room_to_watch_room_num(watch_room, room, watch_string, WATCH_VIEW);

            free(watch_string);

            break;

        case 'l':
            watch_room = fread_var(fl);
            watch_string = fread_text(fl);

            set_room_to_watch_room_num(watch_room, room, watch_string, WATCH_LISTEN);

            free(watch_string);

            break;
        default:
            break;
        }

        free(com);
    }

    room->light = 0;
    room->state = 0;
    room->people = (CHAR_DATA *) 0;
    room->contents = (OBJ_DATA *) 0;

    room->next = room_list;
    room_list = room;

	gamelogf("Adding room: %d", room->number);
    add_room_to_zone_in_order(&zone_table[zone], room);

    tc = *com;
    free(com);

    if (has_special_on_cmd(room->specials, NULL, CMD_PULSE))
        new_event(EVNT_ROOM_PULSE, PULSE_ROOM, 0, 0, 0, room, 0);

    return ((tc != 'x') && !feof(fl));
}

void
add_room_to_zone_in_order(struct zone_data *zdata, ROOM_DATA * room)
{
    ROOM_DATA *tmp = 0, *place = 0;

    room->next_in_zone = 0;

    zdata->rcount++;

    // Empty zone, this must be the first room.
    if (!zdata->rooms) {
        zdata->rooms = room;
        return;
    }
gamelogf("add_room_to_zone_in_order room %d", room->number);
    // This room should be the first in the zone, push it on in front
    if (zdata->rooms->number > room->number) {
        room->next_in_zone = zdata->rooms;
        zdata->rooms = room;
        return;
    }
    // Scan for a room in-order
    for (tmp = zdata->rooms; tmp && tmp->number < room->number; tmp = tmp->next_in_zone)
        place = tmp;

    room->next_in_zone = place->next_in_zone;
    place->next_in_zone = room;
gamelogf("zdata is %d", zdata->rcount);
}

void
write_specials(FILE * fp, struct specials_t *specials)
{
    char buf[MAX_STRING_LENGTH];

    display_specials(buf, ARRAY_LENGTH(buf), specials, 0);
    if (strlen(buf))
        fwrite_string(buf, fp);
    fwrite_string("ep", fp);
}

/* writes room to file */
void
write_room(ROOM_DATA * room, FILE * fl)
{
    struct extra_descr_data *edesc;
    int d, flags;
    struct watching_room_list *wlist;
    flags = room->room_flags;
    /*
     * REMOVE_BIT(flags, RFL_RESTFUL_SHADE);
     */

    fwrite_text("n", fl);
    fwrite_text(room->name, fl);
    fwrite_var(room->number, fl);
    fwrite_var(room->sector_type, fl);
    fwrite_var(flags, fl);
    fwrite_var(room->bdesc_index, fl);
    fprintf(fl, "\n");

    write_specials(fl, room->specials);
    fwrite_string(room->description, fl);

    /* save the city to file if it's worth saving */
    if (room_in_city(room) != CITY_NONE) {
        fwrite_text("c", fl);
        fwrite_var(room_in_city(room), fl);
        fprintf(fl, "\n");
    }

    for (d = 0; d < 6; d++) {
        if (room->direction[d] && room->direction[d]->to_room) {
            fwrite_text("d", fl);
            fwrite_var(d, fl);
            fwrite_var(room->direction[d]->to_room->number, fl);
            fwrite_text(room->direction[d]->keyword, fl);
            fwrite_var(room->direction[d]->exit_info, fl);
            fwrite_var(room->direction[d]->key, fl);

            fwrite_string(room->direction[d]->general_description, fl);
        }
    }

    for (edesc = room->ex_description; edesc; edesc = edesc->next) {
        if (edesc->keyword && edesc->description && !strstr(edesc->keyword, "__tmp__")) {
            fwrite_text("e", fl);
            fwrite_text(edesc->keyword, fl);
            fwrite_string(edesc->description, fl);
        }
    }
    for (wlist = room->watching; wlist; wlist = wlist->next_watching) {

	if (IS_SET(wlist->watch_type, WATCH_VIEW)) {
            fwrite_text("w", fl);
            fwrite_var(wlist->watch_room_num, fl);
            fwrite_text(wlist->view_msg_string, fl);
        }
	if (IS_SET(wlist->watch_type, WATCH_LISTEN)) {
            fwrite_text("l", fl);
            fwrite_var(wlist->watch_room_num, fl);
            fwrite_text(wlist->listen_msg_string, fl);
        }

    }
}

/* write player to file */
void
save_char(CHAR_DATA * ch)
{
    /* commented out to reduce gamelog spam - hal
     * sprintf(buf, "Saving %s", GET_NAME(ch));
     * shhlog(buf); 
     */

    if (IS_NPC(ch))
        return;

    save_char_text(ch);

    
 //   save_char_db(ch);

    // Hook to saving off a character file in XML format
    save_character_file_old(ch);
}



/* write character to waiting directory */
void
save_char_waiting_auth(CHAR_DATA * ch)
{
    char fn[256];
    int i;
    char buf[256];
    if (IS_NPC(ch))
        return;

    /* update character state */
    if (lookup_char_info(ch->desc->player_info, ch->name))
        update_char_info(ch->desc->player_info, ch->name, APPLICATION_ACCEPT);
    else
        add_new_char_info(ch->desc->player_info, ch->name, APPLICATION_ACCEPT);

    ch->application_status = APPLICATION_ACCEPT;
    save_player_info(ch->desc->player_info);
    if (ch->account) free(ch->account);
    ch->account = strdup(ch->desc->player_info->name);

    save_char(ch);

    strcpy(buf, GET_NAME(ch));

    for (i = 0; i < strlen(buf); i++)
        buf[i] = LOWER(buf[i]);

    /* don't move, send notification character has to be reviewed */

    /* tack account & character name into external file */
    sprintf(fn, "%s %s\n", ch->desc->player_info->name, buf);
    appendfile("applications", fn);

    return;
}

/* free a room structure and all its elements */
void
free_room(ROOM_DATA * rm)
{
    int ex;

    free(rm->name);
    free(rm->description);
    free_edesc_list(rm->ex_description);

    struct watching_room_list *wlist, *wnext;
    for (wlist = rm->watching; wlist; wlist = wnext) {
        wnext = wlist->next_watching;
        free(wlist);
    }

    for (ex = 0; ex < 6; ex++)
        if (rm->direction[ex]) {
            free(rm->direction[ex]->general_description);
            free(rm->direction[ex]->keyword);
            free((char *) rm->direction[ex]);
        }

    free_specials(rm->specials);
    free(rm);
}


extern CHAR_DATA *g_script_npc;

/* release memory allocated for a char struct */
void
free_char(CHAR_DATA * ch)
{
    int i;
    CHAR_DATA *p;
    CLAN_DATA *clan, *nextclan;
    EXTRA_CMD *pCmd, *pCmdNext;

    if (!ch) return;

    for (p = character_list; p && (p != ch); p = p->next);

    if (p == ch) {
        if (ch->prev)
            ch->prev->next = ch->next;
        if (ch->next)
            ch->next->prev = ch->prev;
        if (ch == character_list)
            character_list = ch->next;
    }

    free(ch->name);
    free(ch->short_descr);
    free(ch->long_descr);
    free(ch->player.extkwds);
    free(ch->description);
    free(ch->last_prompt);
    if( ch->temp_description)
        free(ch->temp_description);

    for (i = 0; i < 15; i++)
        if (ch->player.info[i])
            free(ch->player.info[i]);

    for (clan = ch->clan; clan; clan = nextclan) {
        nextclan = clan->next;
        free_clan_data(clan);
    }

    free(ch->account);
    free(ch->player.poofin);
    free(ch->player.poofout);
    free(ch->player.prompt);

    /* if NPC check if its the default background, and only free if not */
    /* the same pointer                                                 */
    if ((ch->background)
        && (!IS_NPC(ch)
            || ((ch->nr >= 0) && (ch->nr < top_of_npc_t)
                && (ch->background != npc_default[ch->nr].background)))) {
        free(ch->background);
    }

    free(ch->player.dead);
    free(ch->player.deny);

    remove_all_affects_from_char(ch);

    for (i = 0; i < MAX_SKILLS; i++)
        if (ch->skills[i])
            free(ch->skills[i]);

    for (pCmd = ch->granted_commands; pCmd; pCmd = pCmdNext) {
        pCmdNext = pCmd->next;
        free(pCmd);
    }

    for (pCmd = ch->revoked_commands; pCmd; pCmd = pCmdNext) {
        pCmdNext = pCmd->next;
        free(pCmd);
    }

    free_specials(ch->specials.programs);
    free_edesc_list(ch->ex_description);

    if (ch == g_script_npc) {
        g_script_npc = NULL;    // See g_script_object for rationale of this code
    }

    free(ch);
}

extern OBJ_DATA *g_script_obj;

/* release memory allocated for an obj struct */
void
free_obj(OBJ_DATA * obj)
{
    free(obj->name);
    free(obj->long_descr);
    free(obj->short_descr);
    free(obj->ldesc);
    free(obj->description);

    free_edesc_list(obj->ex_description);
    free_specials(obj->programs);

    // g_script_obj maintains a ptr to the object on
    // which scripts are currently being run.  If this is
    // that object, we'd better null it out so it doesn't
    // do bad dereferences
    if (g_script_obj == obj)
        g_script_obj = NULL;

    free(obj);
}

/* clear some of the the working variables of a char */
void
reset_char(CHAR_DATA * ch)
{
    int i;

    for (i = 0; i < MAX_WEAR; i++)
        ch->equipment[i] = 0;

    ch->followers = 0;
    ch->guards = 0;
    ch->master = 0;
    ch->obj_master = 0;
    ch->carrying = 0;
    ch->next = 0;
    ch->next_fighting = 0;
    ch->next_in_room = 0;
    ch->specials.fighting = 0;
    ch->queued_commands = 0;
    set_char_position(ch, POSITION_STANDING);
    ch->specials.default_pos = POSITION_STANDING;
    ch->specials.carry_items = 0;

    if (GET_HIT(ch) <= 0)
        set_hit(ch, 1);
    if (GET_STUN(ch) <= 0)
        set_stun(ch, 1);
    if (GET_MOVE(ch) <= 0)
        set_move(ch, 1);
    if (GET_MANA(ch) <= 0)
        set_mana(ch, 1);

    ch->old.hp = GET_HIT(ch);
    ch->old.max_hp = GET_MAX_HIT(ch);
    ch->old.stun = GET_STUN(ch);
    ch->old.max_stun = GET_MAX_STUN(ch);
    ch->old.mana = GET_MANA(ch);
    ch->old.max_mana = GET_MAX_MANA(ch);
    ch->old.move = GET_MOVE(ch);
    ch->old.max_move = GET_MAX_MOVE(ch);

    ch->logged_in = FALSE;
}


/* clear all working variables of a char */
void
clear_char(CHAR_DATA * ch)
{
    memset(ch, '\0', sizeof(CHAR_DATA));

    ch->in_room = (ROOM_DATA *) 0;
    ch->saved_room = -1;
    ch->logged_in = FALSE;
    ch->queued_commands = NULL;
    ch->specials.was_in_room = (ROOM_DATA *) 0;
    set_char_position(ch, POSITION_STANDING);
    ch->specials.default_pos = POSITION_STANDING;
    SET_ARMOR(ch, race[(int) GET_RACE(ch)].natural_armor);
}


/* clear all working variables of an object */
void
clear_object(OBJ_DATA * obj)
{
    memset(obj, '\0', sizeof(OBJ_DATA));

    obj->nr = 0;
    obj->in_room = (ROOM_DATA *) 0;
}

void
init_hp(CHAR_DATA * ch)
{
    ch->points.max_hit = 75 + GET_END(ch) + number(1, 10);
    if (GET_GUILD(ch) == GUILD_WARRIOR)
        ch->points.max_hit += dice(2, 4);
    if (GET_RACE(ch) == RACE_HALF_GIANT)
        ch->points.max_hit += 20 + number(1, 8);
    set_hit(ch, GET_MAX_HIT(ch));
}

void
init_stun(CHAR_DATA * ch)
{
    ch->points.max_stun = 75 + GET_END(ch) + number(1, 10);
    if (GET_GUILD(ch) == GUILD_WARRIOR)
        ch->points.max_stun += dice(2, 4);
    if (GET_RACE(ch) == RACE_HALF_GIANT)
        ch->points.max_stun += 20 + number(1, 8);
    set_stun(ch, GET_MAX_STUN(ch));
}

/* initialize a new character only if guild is set */
void
init_char(CHAR_DATA * ch, char *dead)
{
    int i;

    ch->player.time.starting_time = time(0);
    ch->player.time.played = 0;
    ch->player.time.logon = time(0);

    GET_LEVEL(ch) = MORTAL;

    if (ch->player.hometown != CITY_NONE
        && !(GET_RACE(ch) == RACE_DESERT_ELF && IS_TRIBE(ch, TRIBE_BLACKWING)))
        ch->player.hometown = CITY_NONE;

    if ((GET_GUILD(ch) == GUILD_MERCHANT) && !(GET_RACE(ch) == RACE_HALFLING))
        GET_OBSIDIAN(ch) = number(1800, 2200);
    else if (GET_GUILD(ch) == GUILD_TEMPLAR)
        GET_OBSIDIAN(ch) = number(4800, 5200);
    else if (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
        GET_OBSIDIAN(ch) = number(4800, 5200);
    else if (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)
        GET_OBSIDIAN(ch) = number(4800, 5200);
    else if (GET_RACE(ch) == RACE_HALFLING)
        GET_OBSIDIAN(ch) = 0;
    else
        GET_OBSIDIAN(ch) = number(800, 1200);


    // Reach skills for magickers
    if (is_magicker(ch)) {
        if (!has_skill(ch, SKILL_REACH_NONE)) {
            init_skill(ch, SKILL_REACH_NONE, 100);
        }

        if (!has_skill(ch, SKILL_REACH_LOCAL)) {
            init_skill(ch, SKILL_REACH_LOCAL, 100);
        }
    }

    SET_STR(ch, 0);
    SET_AGL(ch, 0);
    SET_WIS(ch, 0);
    SET_END(ch, 0);

    init_hp(ch);
    init_stun(ch);

    set_mana(ch, GET_MAX_MANA(ch));
    set_move(ch, GET_MAX_MOVE(ch));

    GET_COND(ch, THIRST) = (char) 48;
    GET_COND(ch, FULL) = (char) 48;
    GET_COND(ch, DRUNK) = (char) 0;

    ch->abilities.armor = race[(int) GET_RACE(ch)].natural_armor;

    reset_char_skills(ch);

    if (GET_LEVEL(ch) == OVERLORD) {
        for (i = 1; i < MAX_SKILLS; i++)
            init_skill(ch, i, 100);
    } else {
        ch->abilities.off = guild[(int) GET_GUILD(ch)].off_bonus;
        ch->abilities.off_last_gain = time(0);
        ch->abilities.def = guild[(int) GET_GUILD(ch)].def_bonus;
        ch->abilities.def_last_gain = time(0);
    }

    ch->specials.eco = 0;

    if ((ch->player.tribe < 0) || (ch->player.tribe >= MAX_CLAN))
        ch->player.tribe = 0;

    ch->player.luck = 0;

    ch->specials.affected_by = 0;
    ch->specials.to_process = FALSE;

    for (i = 0; i < 5; i++)
        ch->specials.apply_saving_throw[i] = 0;

    init_saves(ch);
    init_languages(ch);
    init_racial_skills(ch);
    init_psi_skills(ch);

 }


/*  Doing this change, because takes 35 seconds to renumber the
    cmd files when booting, and although the binary search is not
    good due to it getting out of whack, do a binary search first
    and if it is not found, then do a linear search
    -Tenebrius
    
    comment out following defines if goes whack -Tenebrius 4/30/99
*/

/* returns the real number of the npc with given vnum */
int
real_mobile(int vnum)
{
    int i;
    int bot, top, mid, done;

    if (!npc_index)
        return -1;

    bot = 0;
    top = top_of_npc_t - 1;

    done = FALSE;
    while (!done) {
        mid = (bot + top) / 2;

        if ((npc_index + mid)->vnum == vnum)
            return (mid);
        else if (bot >= top)
            done = TRUE;
        else if ((npc_index + mid)->vnum > vnum)
            top = mid - 1;
        else
            bot = mid + 1;
    }

    /* Do a linear search, just to make sure */
    for (i = 0; i < top_of_npc_t; i++) {
        if ((npc_index + i)->vnum == vnum)
            return (i);
    }

    return (-1);
}


/* returns the real number of the object with given vnum */
int
real_object(int vnum)
{

    int i;
    int bot, top, mid, done;

    bot = 0;
    top = top_of_obj_t - 1;

    done = FALSE;

    if (!obj_index)
        return -1;

    while (!done) {
        mid = (bot + top) / 2;

        if ((obj_index + mid)->vnum == vnum)
            return (mid);
        else if (bot >= top)
            done = TRUE;
        else if ((obj_index + mid)->vnum > vnum)
            top = mid - 1;
        else
            bot = mid + 1;
    }

    for (i = 0; i < top_of_obj_t; i++) {
        if ((obj_index + i)->vnum == vnum)
            return (i);
    }

    return (-1);
}


/* unload guild data */
void
unboot_guilds(void)
{
    int i;
    for (i = 1; i < MAX_GUILDS; i++) {
        free(guild[i].name);
        guild[i].name = 0;
        free(guild[i].abbrev);
        guild[i].abbrev = 0;
    };
}


/* load guild data */
void
boot_guilds(void)
{
    int i, j, skillnum;
    char *tmp;
    FILE *gf;
    /*  char *buf;       */
    /*  int counter = 0; */

    if (!(gf = fopen(GUILD_FILE, "r"))) {
        gamelogf("Couldn't load %s, bailing.", GUILD_FILE);
        exit(0);
    }

    for (i = 1; i < MAX_SKILLS; i++)
        guild[0].skill_prev[i] = TYPE_UNDEFINED;

    guild[0].pl_guild = FALSE;
    guild[0].can_use_pierce = TRUE;
    guild[0].can_use_slash = TRUE;
    guild[0].can_use_bludgeon = TRUE;

    for (i = 1; i < MAX_GUILDS; i++)
        for (skillnum = 1; *skill_name[skillnum] != '\n'; skillnum++)
            guild[i].skill_prev[skillnum] = TYPE_UNDEFINED;

    for (i = 1; i < MAX_GUILDS; i++) {
        guild[i].name = fread_string(gf);
        guild[i].abbrev = fread_string(gf);


        // gamelog(guild[i].name);
        fscanf(gf, "%d ", &j);
        guild[i].strmod = j;
        fscanf(gf, "%d ", &j);
        guild[i].aglmod = j;
        fscanf(gf, "%d ", &j);
        guild[i].wismod = j;
        fscanf(gf, "%d\n", &j);
        guild[i].endmod = j;

        fscanf(gf, "%d ", &j);
        guild[i].off_kpc = j;
        fscanf(gf, "%d ", &j);
        guild[i].def_kpc = j;
        fscanf(gf, "%d ", &j);
        guild[i].off_bonus = j;
        fscanf(gf, "%d\n", &j);
        guild[i].def_bonus = j;

        fscanf(gf, "%d\n", &j);
        guild[i].pl_guild = j;

        do {
            tmp = fread_string(gf);
            if (*tmp == 'S' && (*(tmp + 1) == '\0' || *(tmp + 1) == '~' || isspace(*(tmp + 1))))
                break;

            // gamelogf("%s skill: %s", guild[i].name, tmp);
            skillnum = old_search_block(tmp, 0, strlen(tmp), skill_name, 0) - 1;
            free((char *) tmp);

            if (skillnum > SPELL_NONE) {
                tmp = fread_string(gf);
                if (!strcasecmp(tmp, "none"))
                    guild[i].skill_prev[skillnum] = SPELL_NONE;
                else {
                    j = old_search_block(tmp, 0, strlen(tmp), skill_name, 0)
                        - 1;
                    if (j > SPELL_NONE)
                        guild[i].skill_prev[skillnum] = j;
                    else
                        guild[i].skill_prev[skillnum] = SPELL_NONE;
                }
                free((char *) tmp);

                fscanf(gf, "%d ", &j);
                guild[i].skill_max_learned[skillnum] = j;
                fscanf(gf, "%d\n", &j);
                guild[i].skill_percent[skillnum] = j;
            } else {
                tmp = fread_string(gf);
                free((char *) tmp);
                fscanf(gf, "%d ", &j);
                fscanf(gf, "%d\n", &j);
            }
        } while (!feof(gf));
        free((char *) tmp);

    }
    fclose(gf);
}


/* unload subguild data */
void
unboot_subguilds(void)
{
    int i;
    for (i = 1; i < MAX_SUB_GUILDS; i++) {
        free(sub_guild[i].name);
        sub_guild[i].name = 0;
        free(sub_guild[i].abbrev);
        sub_guild[i].abbrev = 0;
    };
}

/* load subguild data */
void
boot_subguilds(void)
{
    int i, j, skillnum;
    char *tmp;
    FILE *gf;
    /*  char *buf;       */
    /*  int counter = 0; */

    if (!(gf = fopen(SUB_GUILD_FILE, "r"))) {
        gamelogf("Couldn't load %s, bailing.", SUB_GUILD_FILE);
        exit(0);
    }

    for (i = 1; i < MAX_SKILLS; i++)
        sub_guild[0].skill_prev[i] = TYPE_UNDEFINED;

    sub_guild[0].pl_guild = FALSE;

    for (i = 1; i < MAX_SUB_GUILDS; i++)
        for (skillnum = 1; *skill_name[skillnum] != '\n'; skillnum++)
            sub_guild[i].skill_prev[skillnum] = TYPE_UNDEFINED;

    for (i = 1; i < MAX_SUB_GUILDS; i++) {
        sub_guild[i].name = fread_string(gf);
        sub_guild[i].abbrev = fread_string(gf);


        // gamelog(sub_guild[i].name);
        fscanf(gf, "%d ", &j);
        sub_guild[i].strmod = j;
        fscanf(gf, "%d ", &j);
        sub_guild[i].aglmod = j;
        fscanf(gf, "%d ", &j);
        sub_guild[i].wismod = j;
        fscanf(gf, "%d\n", &j);
        sub_guild[i].endmod = j;

        fscanf(gf, "%d ", &j);
        sub_guild[i].off_kpc = j;
        fscanf(gf, "%d ", &j);
        sub_guild[i].def_kpc = j;
        fscanf(gf, "%d ", &j);
        sub_guild[i].off_bonus = j;
        fscanf(gf, "%d\n", &j);
        sub_guild[i].def_bonus = j;

        fscanf(gf, "%d\n", &j);
        sub_guild[i].pl_guild = j;

        do {
            tmp = fread_string(gf);
            if (*tmp == 'S' && (*(tmp + 1) == '\0' || *(tmp + 1) == '~' || isspace(*(tmp + 1))))
                break;

            // gamelogf("Sub_Guild_Skill: %s", tmp);
            skillnum = old_search_block(tmp, 0, strlen(tmp), skill_name, 0) - 1;
            free((char *) tmp);

            if (skillnum > SPELL_NONE) {
                tmp = fread_string(gf);
                if (!strcasecmp(tmp, "none"))
                    sub_guild[i].skill_prev[skillnum] = SPELL_NONE;
                else {
                    j = old_search_block(tmp, 0, strlen(tmp), skill_name, 0)
                        - 1;
                    if (j > SPELL_NONE)
                        sub_guild[i].skill_prev[skillnum] = j;
                    else
                        sub_guild[i].skill_prev[skillnum] = SPELL_NONE;
                }
                free((char *) tmp);

                fscanf(gf, "%d ", &j);
                sub_guild[i].skill_max_learned[skillnum] = j;
                fscanf(gf, "%d\n", &j);
                sub_guild[i].skill_percent[skillnum] = j;
            } else {
                tmp = fread_string(gf);
                free((char *) tmp);
                fscanf(gf, "%d ", &j);
                fscanf(gf, "%d\n", &j);
            }
        } while (!feof(gf));
        free((char *) tmp);
    }
    fclose(gf);
}


/* load race data */
void
unboot_races(void)
{
    int i;
    struct skin_data *skin, *next_skin;
    struct attack_data *attack, *next_attack;

    for (i = 0; i < MAX_RACES; i++) {
        free(race[i].name);
        race[i].name = 0;
        free(race[i].abbrev);
        race[i].abbrev = 0;

        attack = race[i].attack;
        race[i].attack = 0;

        while (attack) {
            next_attack = attack->next;
            free(attack);
            attack = next_attack;
        }

        skin = race[i].skin;
        race[i].skin = 0;

        while (skin) {
            next_skin = skin->next;
            free(skin->text[0]);
            free(skin->text[1]);
            free(skin);
            skin = next_skin;
        }
    }
}

/* load race data */
void
old_boot_races(void)
{
    FILE *rf;
    int i, j, k, l, nr[5];
    struct skin_data *skin;
    struct attack_data *attack;

    if (!(rf = fopen(RACE_FILE, "r"))) {
        gamelogf("Error opening %s, bailing!", RACE_FILE);
        exit(0);
    }
    /*race[0].damdice = 1;
     * race[0].damsize = 1;
     * race[0].damplus = 0; */
    race[0].name = strdup("None");
    race[0].abbrev = strdup("Non");
    race[0].max_age = 0;

    for (i = 1; i < MAX_RACES && !feof(rf); i++) {
        race[i].name = fread_string(rf);
        race[i].abbrev = fread_string(rf);

        fscanf(rf, "STRENGTH: %dd%d+%d\n", &j, &k, &l);
        race[i].strdice = j;
        race[i].strsize = k;
        race[i].strplus = l;

        fscanf(rf, "AGILITY: %dd%d+%d\n", &j, &k, &l);
        race[i].agldice = j;
        race[i].aglsize = k;
        race[i].aglplus = l;

        fscanf(rf, "WISDOM: %dd%d+%d\n", &j, &k, &l);
        race[i].wisdice = j;
        race[i].wissize = k;
        race[i].wisplus = l;

        fscanf(rf, "ENDURANCE: %dd%d+%d\n", &j, &k, &l);
        race[i].enddice = j;
        race[i].endsize = k;
        race[i].endplus = l;

        fscanf(rf, "%d ", &j);
        race[i].off_base = j;
        fscanf(rf, "%d ", &j);
        race[i].def_base = j;
        fscanf(rf, "%d\n", &j);
        race[i].natural_armor = j;

        CREATE(attack, struct attack_data, 1);
        fscanf(rf, "%dd%d+%d\n", &j, &k, &l);
        attack->damdice = j;
        attack->damsize = k;
        attack->damplus = l;

        fscanf(rf, "%d ", &j);
        race[i].race_type = j;
        fscanf(rf, "%d ", &j);
        race[i].max_age = j;

        fscanf(rf, "%d\n", &j);
        attack->type = j;
        attack->next = NULL;
        race[i].attack = attack;


        fscanf(rf, "%d %d %d %d %d\n", &nr[0], &nr[1], &nr[2], &nr[3], &nr[4]);

        race[i].skin = 0;
        for (j = 0; j < 5; j++) {
            if (nr[j] != -1) {
                CREATE(skin, struct skin_data, 1);
                skin->item = nr[j];
                skin->text[0] = fread_string(rf);
                skin->text[1] = fread_string(rf);
                skin->next = race[i].skin;
                race[i].skin = skin;
            }
        }
    }

    fclose(rf);
}


/* saves banned sites */
void
save_bans(void)
{
    struct ban_t *tmp;
    FILE *bf;

    gamelog("Saving ban file.");

    if (!(bf = fopen(BANFILE, "w"))) {
        gamelog("Error opening ban file.");
        exit(0);
    }
    for (tmp = ban_list; tmp; tmp = tmp->next)
        fprintf(bf, "%s~\n", tmp->name);

    fprintf(bf, "S~\n");
    fclose(bf);
}


/* save objects in zone */
void
save_objects(int zone)
{
    struct extra_descr_data *exdesc;
    int i, l, x;
    char buf[MAX_STRING_LENGTH], buf2[256];
    FILE *o_f;

    sprintf(buf, "objects/z%d.obj", zone);
    sprintf(buf2, "objects/z%d.obj.bak", zone);
    rename(buf, buf2);


    if (!(o_f = fopen(buf, "w"))) {
        gamelogf("Couldn't save %s: %s", buf, strerror(errno));
        exit(0);
    }

    qgamelogf(QUIET_ZONE, "Saving objects in zone %d: %s.", zone, zone_table[zone].name);

    for (i = 0; i < top_of_obj_t; i++) {
        if ((obj_index[i].vnum / 1000) == zone) {
            fprintf(o_f, "#%d %d\n", obj_index[i].vnum, obj_index[i].limit);
            fprintf(o_f, "%s~\n", obj_default[i].name);
            fprintf(o_f, "%s~\n", obj_default[i].short_descr);
            strcpy(buf, "");
            x = 0;
            for (l = 0; l <= strlen(obj_default[i].long_descr); l++) {
                if (obj_default[i].long_descr[l] != 13)
                    buf[x++] = obj_default[i].long_descr[l];
            }
            buf[x] = '\0';
            fprintf(o_f, "%s~\n", buf);

            strcpy(buf, "");
            x = 0;
            if (obj_default[i].description) {
                for (l = 0; l <= strlen(obj_default[i].description); l++) {
                    if (obj_default[i].description[l] != 13)
                        buf[x++] = obj_default[i].description[l];
                }
                buf[x] = '\0';
            }
            fprintf(o_f, "%s~\n", buf);

            fprintf(o_f, "%d ", obj_default[i].type);
#ifdef SCRUB_ANTI_MORTAL
	    if (IS_SET(obj_default[i].extra_flags, OFL_ANTI_MORT))
		REMOVE_BIT(obj_default[i].extra_flags, OFL_ANTI_MORT);
#endif
            fprintf(o_f, "%d ", obj_default[i].extra_flags);
            fprintf(o_f, "%ld ", obj_default[i].wear_flags);
            fprintf(o_f, "%ld ", obj_default[i].state_flags);
            fprintf(o_f, "%d\n", obj_default[i].material);
            /* soon to come... Ur
             * fprintf(o_f, "%d\n", obj_default[i].size);
             */
            fprintf(o_f, "%d ", obj_default[i].value[0]);
            fprintf(o_f, "%d ", obj_default[i].value[1]);
            fprintf(o_f, "%d ", obj_default[i].value[2]);
            fprintf(o_f, "%d ", obj_default[i].value[3]);
            fprintf(o_f, "%d ", obj_default[i].value[4]);
            fprintf(o_f, "%d\n", obj_default[i].value[5]);

            fprintf(o_f, "%d ", obj_default[i].weight);
            fprintf(o_f, "%d ", obj_default[i].cost);
            fprintf(o_f, "%d\n", obj_default[i].temp);

            fprintf(o_f, "%d ", obj_default[i].length);
            fprintf(o_f, "%d ", obj_default[i].dam_num_dice);
            fprintf(o_f, "%d ", obj_default[i].dam_dice_size);
            fprintf(o_f, "%d\n", obj_default[i].dam_plus);


            write_specials(o_f, obj_default[i].programs);

            for (exdesc = obj_default[i].exdesc; exdesc; exdesc = exdesc->next) {
                x = 0;
                if (exdesc->description && exdesc->keyword) {
                    strcpy(buf, "");
                    x = 0;
                    for (l = 0; l <= strlen(exdesc->description); l++) {
                        if (exdesc->description[l] != 13)
                            buf[x++] = exdesc->description[l];
                    }
                    buf[x] = '\0';
                    fprintf(o_f, "E\n%s~\n%s~\n", exdesc->keyword, buf);
                }
            }

            for (l = 0; l < MAX_OBJ_AFFECT; l++) {
                fprintf(o_f, "A\n%d %d\n", obj_default[i].affected[l].location,
                        obj_default[i].affected[l].modifier);
            }
        }
    }

    fprintf(o_f, "$\n");
    fclose(o_f);
}


char *str_from_flags(struct flag_data *flags, long vector);

/* save npcs in zone */
void
save_mobiles(int zone)
{
    int i, l, x, sk;
    char buf[8192], buf2[256];
    FILE *m_f;
    struct extra_descr_data *exdesc;

    CLAN_DATA *pClan;

    sprintf(buf, "npcs/z%d.npc", zone);
    sprintf(buf2, "npcs/z%d.npc.bak", zone);
    rename(buf, buf2);


    if (!(m_f = fopen(buf, "w"))) {
        perror("save_mobiles");
        exit(0);
    }

    qgamelogf(QUIET_ZONE, "Saving npcs in zone %d: %s.", zone, zone_table[zone].name);

    for (i = 0; i < top_of_npc_t; i++) {
        if ((npc_index[i].vnum / 1000) == zone) {
            fprintf(m_f, "#%d %d\n", npc_index[i].vnum, npc_index[i].limit);
            fprintf(m_f, "%s~\n", npc_default[i].name);
            fprintf(m_f, "%s~\n", npc_default[i].short_descr);
            strcpy(buf, "");
            if (npc_default[i].long_descr) {
                x = 0;
                for (l = 0; l <= strlen(npc_default[i].long_descr); l++)
                    if (npc_default[i].long_descr[l] != 13)
                        buf[x++] = npc_default[i].long_descr[l];
                buf[x] = '\0';
            }
            fprintf(m_f, "%s~\n", buf);

            strcpy(buf, "");
            if (npc_default[i].description) {
                x = 0;
                for (l = 0; l <= strlen(npc_default[i].description); l++)
                    if (npc_default[i].description[l] != 13)
                        buf[x++] = npc_default[i].description[l];
                buf[x] = '\0';
            }
            fprintf(m_f, "%s~\n", buf);

            strcpy(buf, "");
            if (npc_default[i].background) {
                x = 0;
                for (l = 0; l <= strlen(npc_default[i].background); l++)
                    if (npc_default[i].background[l] != 13)
                        buf[x++] = npc_default[i].background[l];
                buf[x] = '\0';
            }
            fprintf(m_f, "%s~\n", buf);

            fprintf(m_f, "%ld ", npc_default[i].act);
            fprintf(m_f, "%ld ", npc_default[i].flag);
            fprintf(m_f, "%ld ", npc_default[i].affected_by);
            fprintf(m_f, "%d ", npc_default[i].language);

            fprintf(m_f, "S\n");

            fprintf(m_f, "%d ", npc_default[i].level);
            fprintf(m_f, "%d ", npc_default[i].off);
            fprintf(m_f, "%d ", npc_default[i].def);
            fprintf(m_f, "%d ", npc_default[i].armor);
            fprintf(m_f, "%d ", npc_default[i].max_hit);
            fprintf(m_f, "%d\n", npc_default[i].dam);
            fprintf(m_f, "%d ", npc_default[i].obsidian);
            fprintf(m_f, "%d\n", npc_default[i].gp_mod);
            fprintf(m_f, "%d ", npc_default[i].pos);
            fprintf(m_f, "%d ", npc_default[i].def_pos);

            fprintf(m_f, "%d ", npc_default[i].sex);
            fprintf(m_f, "%d ", npc_default[i].guild);
            fprintf(m_f, "%d ", npc_default[i].tribe);
            fprintf(m_f, "%d ", npc_default[i].luck);
            fprintf(m_f, "%d\n", npc_default[i].race);

            write_specials(m_f, npc_default[i].programs);

            for (exdesc = npc_default[i].exdesc; exdesc; exdesc = exdesc->next) {
                x = 0;
                if (exdesc->description && exdesc->keyword) {
                    strcpy(buf, "");
                    x = 0;
                    for (l = 0; l <= strlen(exdesc->description); l++) {
                        if (exdesc->description[l] != 13)
                            buf[x++] = exdesc->description[l];
                    }
                    buf[x] = '\0';
                    fprintf(m_f, "E\n%s~\n%s~\n", exdesc->keyword, buf);
                }
            }

            for (sk = 0; sk < MAX_SKILLS; sk++)
                if (npc_default[i].skills[sk].learned > 0)
                    fprintf(m_f, "S %d %d\n", sk, npc_default[i].skills[sk].learned);

            fprintf(m_f, "x age %d\n", npc_default[i].age);

            for (pClan = npc_default[i].clan; pClan; pClan = pClan->next) {
                if (pClan->clan < 0 || pClan->clan >= MAX_CLAN)
                    sprintf(buf, "%d", pClan->clan);
                else
                    sprintf(buf, "%s", clan_table[pClan->clan].name);

                fprintf(m_f, "C\n%s~\n%s~\n%d\n", buf, str_from_flags(clan_job_flags, pClan->flags),
                        pClan->rank);
            }
        }
    }

    fprintf(m_f, "$\n");
    fclose(m_f);
}


bool
put_char_obj(OBJ_DATA * obj, CHAR_DATA * ch, FILE * fl)
{
    int j;
    /* 
     * int wmod = 0; 
     * char fname[MAX_STRING_LENGTH]; 
     * OBJ_DATA *tmp;
     */
    struct obj_file_elem object;


    if (obj->nr > 0)
        object.nr = obj_index[obj->nr].vnum;
    else
        object.nr = 0;

    if (obj->equipped_by) {
        for (j = 0; j < MAX_WEAR; j++)
            if (obj->equipped_by->equipment[j] == obj)
                break;
        if (j == MAX_WEAR) {
            gamelog("error finding object's equipment position");
            object.eq_pos = -1;
        }

        object.eq_pos = j;
    } else
        object.eq_pos = -1;

    object.value[0] = obj->obj_flags.value[0];
    object.value[1] = obj->obj_flags.value[1];
    object.value[2] = obj->obj_flags.value[2];
    object.value[3] = obj->obj_flags.value[3];
/* FIX */
    object.extra_flags = obj->obj_flags.extra_flags;

    object.timer = obj->obj_flags.timer;
    object.bitvector = obj->obj_flags.bitvector;

    for (j = 0; j < MAX_OBJ_AFFECT; j++)
        object.affected[j] = obj->affected[j];

    if (fwrite(&object, sizeof(object), 1, fl) != 1) {
        gamelog("Error putting character's object in file.");
        return FALSE;
    }

    return TRUE;
}

bool
char_objs_to_store(OBJ_DATA * obj, CHAR_DATA * ch, FILE * fp)
{
    /* OBJ_DATA *tmp; */
    bool result;

    if (obj) {
        if (obj->contains)
            char_objs_to_store(obj->contains, ch, fp);
        if (obj->next_content)
            char_objs_to_store(obj->next_content, ch, fp);

        result = put_char_obj(obj, ch, fp);
        if (!result)
            return FALSE;
/*      shhlog ( "wrote an item to storage" ); */
/* Assume this was put in when we were loosing eq, took it out to */
/* reduce spam in the rungamelog. Sorry if it shoud have been left in */
/* -Mark */
    }
    return TRUE;
}

/* note loading / saving - hal */


/* finds the first available file name in the sequence note1, note2, note3,
   etc.  sets argument to the file name and returns the corresponding
   number */
int
tempname(char *argument)
{
    char buf[256];
    int i = 0;
    FILE *fd;

    int valid = FALSE;

    while (!valid) {
        sprintf(buf, "note%d", ++i);
        if (NULL != (fd = fopen(buf, "r")))
            fclose(fd);
        else
            valid = TRUE;
    }

    strcpy(argument, buf);
    return (i);
}

void
save_note(OBJ_DATA * paper)
{
    char filename[256];
    char *tmp_desc;
    int pagenum;
    int filenum;
    FILE *fd;
    int x, l;
    char buf[MAX_STRING_LENGTH];

    chdir("notes");

    sprintf(buf, "page_1");
    if (!find_page_ex_description(buf, paper->ex_description, FALSE)) {
        chdir("..");
        return;
    }

    if (paper->obj_flags.value[1] != 0) {
        sprintf(buf, "note%d", paper->obj_flags.value[1]);
        strcpy(filename, buf);
    } else {
        if (!(filenum = tempname(filename))) {
            gamelog("Unable to save note -- too many note files");
            chdir("..");
            return;
        }
        paper->obj_flags.value[1] = filenum;
    }

    if ((fd = fopen(filename, "w")) == NULL) {
        sprintf(buf, "Unable to open note %s.", filename);
        gamelog(buf);
        chdir("..");
        return;
    }

    for (pagenum = 1; pagenum <= 12; pagenum++) {
        sprintf(buf, "page_%d", pagenum);
        tmp_desc = find_page_ex_description(buf, paper->ex_description, FALSE);
        if (tmp_desc) {
            strcpy(buf, "");
            if (tmp_desc) {
                x = 0;
                for (l = 0; l <= strlen(tmp_desc); l++)
                    if (tmp_desc[l] != 13)
                        buf[x++] = tmp_desc[l];
                buf[x] = '\0';
            }
            fprintf(fd, "%s~\n", buf);
        } else
            break;
    }

    fprintf(fd, "*~\n");
    fclose(fd);
    chdir("..");
}

struct extra_descr_data *
find_edesc_info(const char *word, struct extra_descr_data *list)
{
    struct extra_descr_data *i;

    for (i = list; i; i = i->next)
        if (isname(word, i->keyword))
            return (i);

    return (0);
}

void
read_note(OBJ_DATA * obj)
{
    char buf[MAX_STRING_LENGTH];
    char filename[MAX_INPUT_LENGTH];
    char *page;
    struct extra_descr_data *edesc;
    FILE *fd;
    int i;

    page = (void *) 0;          /* Initialize page */

    if (!obj->obj_flags.value[1]) {
        return;
    }

    sprintf(filename, "notes/note%d", obj->obj_flags.value[1]);

    if (!(fd = fopen(filename, "r"))) {
        sprintf(buf, "Bad note filename or missing note file '%s'", filename);
        gamelog(buf);
        return;
    }

    /* sprintf( buf, "starting to read %s", filename );
     * gamelog( buf ); */

    for (i = 1; !feof(fd)
         && (page = fread_string(fd)) != NULL && strcmp(page, "*"); i++) {
        sprintf(buf, "page_%d", i);
        if (NULL != (edesc = find_edesc_info(buf, obj->ex_description))) {
            free(edesc->description);   // Free the old one
            edesc->description = page;
        } else {
            global_game_stats.edescs++;
            CREATE(edesc, struct extra_descr_data, 1);
            sprintf(buf, "page_%d", i);
            edesc->keyword = strdup(buf);
            edesc->description = page;

            edesc->next = obj->ex_description;
            obj->ex_description = edesc;
        }
    }

    if (page)
        free(page);

    /* if (!page)
     * {
     * gamelogf ("read_note: Error reading note file %d", obj->obj_flags.value[1]);
     * } */

    fclose(fd);

    /* sprintf( buf, "finished reading %s", filename );
     * gamelog( buf ); */

    return;
}

/* NEW STORAGE LOADING HERE */

/*
   In order to save the characters objects, I use the equip_pos value
   it is currently between 0 and MAX_WEAR for equip'ed objects.
   Inventory. Items in a char's inventory are not saved that way...
   So I will first move all objects into inventory, putting objects into other
   objects with the following format.
   if Something is in pos greater than or equal to -1 then goes into inventory.
   This is done recursivly, so that when it exits the recursion at the end
   of file, it will move all the WORN_ON items into the equipment fields.
   Right now mount is a cludge, it will not store anything. 


   | 0| item_worn_on_feet
   | 1| item_worn_on_legs
   | 2| item_worn_on_head
   |-1| item_1_in_inventory
   |-1| item_2_in_inventory
   | 3| bag_item_1_is_being held
   |-2|   item_in_bag_1_object_1
   |-2|   item_in_bag_1_object_2
   |-1| bag_item_2
   |-2|   item_in_bag_2_object_1
   |-2|   item_in_bag_2_object_2
   |-2|   bag_3_in_bag_2
   |-3|     item_in_bag_3_in_bag_2_object_1
   |-3|     item_in_bag_3_in_bag_2_object_2
   |-3|     bag_4_in_bag_3_in_bag_2
   |-4|       item_in_bag_4_in_bag_3_in_bag_1_object_1
   |-2|   bag_5_in_bag_2
   |-3|     item_in_bag_5_in_bag_2_object_1
   |-3|     item_in_bag_5_in_bag_2_object_1
   |-1| item_3

   This must be done recursivly in order to save what position the object
   will get moved to, (IE if something is in your shoes, first it is moved
   to the inventory, then everything else is read (in case it might be in the
   shoes) then the shoes are moved onto the feet.
   It assumes that the objects are read in order from first to last.

   Remember obj_to_char puts the object first thing in the persons carrying
   list.

 */
OBJ_DATA *
storage_to_char_objs(CHAR_DATA * ch, struct obj_file_elem * object)
{
    OBJ_DATA *obj, *head_object;
    int n, to_level;

    to_level = object->eq_pos;
    if ((n = real_object(object->nr)) < 0)
        return (0);
    obj = read_object(n, REAL);

    int asz = 0;
    int csz = 0;
    int max_points = 0;

    // remember what the size of the item was
    if (GET_ITEM_TYPE(obj) == ITEM_WORN)
        asz = obj->obj_flags.value[0];
    else if( GET_ITEM_TYPE(obj) == ITEM_ARMOR) {
        asz = obj->obj_flags.value[2];
        max_points = obj->obj_flags.value[3];
    }

    obj->obj_flags.value[0] = object->value[0];
    obj->obj_flags.value[1] = object->value[1];
    obj->obj_flags.value[2] = object->value[2];
    obj->obj_flags.value[3] = object->value[3];
    obj->obj_flags.value[4] = 0;
    obj->obj_flags.value[5] = 0;

    // hold onto the new size
    if (GET_ITEM_TYPE(obj) == ITEM_WORN)
        csz = obj->obj_flags.value[0];
    else if( GET_ITEM_TYPE(obj) == ITEM_ARMOR)
        csz = obj->obj_flags.value[2];

    // set the weight
    obj->obj_flags.weight = obj_default[obj->nr].weight;

    // adjust weight for size change
    if( asz > 0 && asz != csz ) {
        obj->obj_flags.weight = obj->obj_flags.weight * csz / asz;
    }

    obj->obj_flags.extra_flags = object->extra_flags;
/* HACK */
    obj->obj_flags.size = 0;
    obj->obj_flags.timer = object->timer;
    obj->obj_flags.bitvector = object->bitvector;

    if (obj->obj_flags.type == ITEM_NOTE)
        read_note(obj);

    if (to_level >= -1)
        obj_to_char(obj, ch);
    else {
        head_object = ch->carrying;
        while ((to_level < -2) && head_object) {
            head_object = head_object->contains;
            to_level++;
        }
        if (head_object)
            obj_to_obj(obj, head_object);
        else {
            obj_to_char(obj, ch);
            shhlog("CRAP!");
        }
    }
    return (obj);
}

void
recursive_load_objects(CHAR_DATA * ch, FILE * fl)
{
    CHAR_DATA *npc;
    OBJ_DATA *obj = NULL;
    struct obj_file_elem object;
    if (feof(fl) || (1 != fread(&object, sizeof(object), 1, fl)))
        return;

    if (ferror(fl)) {
        gamelog("Error loading character's objects.");
        fclose(fl);
        return;
    }
    /* 
     * sprintf (message,"Loading %d pos %d", object.nr, object.eq_pos );
     * shhlog(message);
     */
    if (!IS_SET(object.extra_flags, OFL_SAVE_MOUNT))
        obj = storage_to_char_objs(ch, &object);
    recursive_load_objects(ch, fl);

    if (IS_SET(object.extra_flags, OFL_SAVE_MOUNT)) {
        if ((npc = read_mobile(object.nr, VIRTUAL)) && IS_SET(npc->specials.act, CFL_MOUNT)) {
            set_char_position(npc, POSITION_STANDING);
            char_to_room(npc, ch->in_room);
            ch->specials.riding = npc;
        }
    } else {
        if ((obj) && (object.eq_pos > -1) && (object.eq_pos < MAX_WEAR)
            && !ch->equipment[object.eq_pos]) {
            obj_from_char(obj);
            equip_char(ch, obj, object.eq_pos);
        }
    }

}


/* The saving stuff follows */
bool
store_char_object(OBJ_DATA * obj, CHAR_DATA * ch, FILE * fl, int level)
{
    int j;
    /*   int wmod = 0; */
    struct obj_file_elem object;
    /*   OBJ_DATA *tmp; */

    if (obj->nr > 0)
        object.nr = obj_index[obj->nr].vnum;
    else
        object.nr = 0;

    /*  sprintf(message, "Saving object %d: level %d", object.nr, level);
     * shhlog(message); 
     */

    object.eq_pos = level;

    object.value[0] = obj->obj_flags.value[0];
    object.value[1] = obj->obj_flags.value[1];
    object.value[2] = obj->obj_flags.value[2];
    object.value[3] = obj->obj_flags.value[3];
    object.extra_flags = obj->obj_flags.extra_flags;

    object.timer = obj->obj_flags.timer;
    object.bitvector = obj->obj_flags.bitvector;
    object.state_flags = obj->obj_flags.state_flags;

    for (j = 0; j < MAX_OBJ_AFFECT; j++)
        object.affected[j] = obj->affected[j];

    if (obj->obj_flags.type == ITEM_NOTE)
        save_note(obj);

    if (fwrite(&object, sizeof(object), 1, fl) != 1) {
        gamelog("Error putting character's object in file.");
        return FALSE;
    }
    return TRUE;
}

bool
recursive_char_objs_to_store(OBJ_DATA * obj, CHAR_DATA * ch, FILE * fp, int level)
{
    /*   OBJ_DATA *tmp; */
    bool result;

    for (; obj; obj = obj->next_content) {
        result = store_char_object(obj, ch, fp, level);
        if (!result)
            return FALSE;
        if (obj->contains) {
            if (level > -1)
                /* this makes it so it will save the objs in equip */
                recursive_char_objs_to_store(obj->contains, ch, fp, -2);
            else
                recursive_char_objs_to_store(obj->contains, ch, fp, level - 1);
        }
    }
    return (TRUE);
}

void
save_all_chars_objects(CHAR_DATA * ch, FILE * fp)
{
    int j;
    if (!recursive_char_objs_to_store(ch->carrying, ch, fp, -1)) {
        return;
    }
    for (j = 0; j < MAX_WEAR; j++)
        if (ch->equipment[j])
            if (!recursive_char_objs_to_store(ch->equipment[j], ch, fp, j)) {
                return;
            }
};

#ifdef FUCKTHIS


#endif


void save_char_mount(CHAR_DATA * ch, CHAR_DATA * mount, char *filename);
void save_hitched_followers(CHAR_DATA * ch, char *filename, int *nump);



#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                    \
                    if ( !strcasecmp( word, literal ) )    \
                    {                                   \
                       field  = value;                  \
                       fMatch = TRUE;                   \
                       break;                           \
                    }

#ifdef SKEY
#undef SKEY
#endif

#define SKEY( literal, field )                          \
                    if( !strcasecmp( word, literal ) )     \
                    {                                   \
                       if( field )                      \
                          free( field );                \
                       field = fread_string_no_whitespace( fp ); \
                       fMatch = TRUE;                   \
                       break;                           \
                    }

void
fread_race(int n, FILE * fp)
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    int numdice, sizedice, plus;
    bool fMatch;
    char *t;

    race[n].race_type = 0;
    race[n].name = strdup("None");
    race[n].abbrev = strdup("Non");
    race[n].max_age = 0;
    race[n].attack = NULL;
    race[n].skin = NULL;

    while (TRUE) {
        word = feof(fp) ? "EndRace" : ((t = fread_word(fp)) ? t : "EndRace");
        fMatch = FALSE;

        switch (UPPER(word[0])) {
        case '*':              /* comments */
            fMatch = TRUE;
            fread_to_eol(fp);
            break;

        case 'A':
            SKEY("Abbrev", race[n].abbrev);
            KEY("Armor", race[n].natural_armor, fread_number(fp));
            if (!strcasecmp(word, "Agl")) {
                fscanf(fp, " %dd%d+%d\n", &numdice, &sizedice, &plus);
                race[n].agldice = numdice;
                race[n].aglsize = sizedice;
                race[n].aglplus = plus;
                fMatch = TRUE;
            }
            if (!strcasecmp(word, "Attack")) {
                int type, bare_handed;
                ATTACK_DATA *attack;

                fscanf(fp, " %d %d %dd%d+%d\n", &type, &bare_handed, &numdice, &sizedice, &plus);
                CREATE(attack, ATTACK_DATA, 1);
                attack->type = type;
                attack->bare_handed = bare_handed;
                attack->damdice = numdice;
                attack->damsize = sizedice;
                attack->damplus = plus;
                attack->next = race[n].attack;
                race[n].attack = attack;
                fMatch = TRUE;
            }
            break;

        case 'D':
            KEY("Def", race[n].def_base, fread_number(fp));
            break;

        case 'E':
            if (!strcasecmp(word, "EndRace")) {
                fMatch = TRUE;
                return;
            }
            if (!strcasecmp(word, "End")) {
                fscanf(fp, " %dd%d+%d\n", &numdice, &sizedice, &plus);
                race[n].enddice = numdice;
                race[n].endsize = sizedice;
                race[n].endplus = plus;
                fMatch = TRUE;
            }
            break;

        case 'M':
            KEY("MaxAge", race[n].max_age, fread_number(fp));
            break;

        case 'N':
            SKEY("Name", race[n].name);
            break;

        case 'O':
            KEY("Off", race[n].off_base, fread_number(fp));
            break;

        case 'R':
            KEY("Rtype", race[n].race_type, fread_number(fp));
            break;

        case 'S':
            if (!strcasecmp(word, "Skin")) {
                struct skin_data *skin;

                CREATE(skin, struct skin_data, 1);
                skin->item = fread_number(fp);
                skin->bonus = fread_number(fp);
                skin->text[0] = fread_string_no_whitespace(fp);
                skin->text[1] = fread_string_no_whitespace(fp);
                skin->next = race[n].skin;
                race[n].skin = skin;
                fMatch = TRUE;
            }
            if (!strcasecmp(word, "Str")) {
                fscanf(fp, " %dd%d+%d\n", &numdice, &sizedice, &plus);
                race[n].strdice = numdice;
                race[n].strsize = sizedice;
                race[n].strplus = plus;
                fMatch = TRUE;
            }
            break;

        case 'W':
            if (!strcasecmp(word, "Wis")) {
                fscanf(fp, " %dd%d+%d\n", &numdice, &sizedice, &plus);
                race[n].wisdice = numdice;
                race[n].wissize = sizedice;
                race[n].wisplus = plus;
                fMatch = TRUE;
            }
            break;
        }

        if (!fMatch) {
            sprintf(buf, "Fread_race: no match for '%s' in race %s", word, race[n].name);
            shhlog(buf);
        }
    }
}


void
boot_races()
{
    FILE *rf;
    char buf[MAX_STRING_LENGTH];
    int i;

    sprintf(buf, "%s", RACE_FILE);
    if (!(rf = fopen(buf, "r"))) {
        gamelogf("Error opening %s, bailing!", RACE_FILE);
        exit(0);
    }

    CREATE(race[0].attack, ATTACK_DATA, 1);
    race[0].attack->damdice = 1;
    race[0].attack->damsize = 1;
    race[0].attack->damplus = 0;
    race[0].name = strdup("None");
    race[0].abbrev = strdup("Non");
    race[0].max_age = 0;
    race[0].skin = NULL;

    for (i = 1; i < MAX_RACES && !feof(rf); i++)
        fread_race(i, rf);

    fclose(rf);
}


void
fread_city(int n, FILE * fp)
{
    //  char logbuf[256];
    char buf[MAX_STRING_LENGTH];
    char *word;
    int i;
    bool fMatch;
    char *t, *tmp;

    /* zero out the structure */
    city[n].name = strdup("None");
    city[n].of_city = strdup("None");
    city[n].c_flag = 0;
    city[n].zone = 0;
    city[n].tribe = 0;
    city[n].room = 1003;

    for (i = 0; i < MAX_COMMOD; i++)
        city[n].price[i] = 0.0;

    while (TRUE) {
        word = feof(fp) ? "End" : ((t = fread_word(fp)) ? t : "End");
        fMatch = FALSE;

        switch (UPPER(word[0])) {
        case '*':              /* comments */
            fMatch = TRUE;
            fread_to_eol(fp);
            break;

        case 'C':
            if (!strcasecmp(word, "Crim_flag")) {
                tmp = fread_string_no_whitespace(fp);
                if ((city[n].c_flag = get_flag_bit(char_flag, tmp)) == -1)
                    city[n].c_flag = 0;

                  sprintf(buf, "boot_cities: Read crim flag '%s' as value %d for city %d.", tmp, city[n].c_flag, n);
                  gamelog (buf);

                free(tmp);
                fMatch = TRUE;
                break;
            }

            if (!strcasecmp(word, "Commod")) {
                int commod;

                /* this will read and allocate space that  */
                /* needs to be freed                       */

                tmp = fread_string_no_whitespace(fp);
                if ((commod = get_attrib_val(commodities, tmp)) == -1) {
                    fread_float(fp);
                } else {
                    city[n].price[commod] = fread_float(fp);
                }
                fMatch = TRUE;

#define FREAD_CITY_MEMORY
#ifdef FREAD_CITY_MEMORY
                /* small memory leak - Tenebrius 2002.12.27           */
                /* remove Ifdef and comments on 2003.5.27 if still in */
                if (tmp)
                    free(tmp);
#endif

                break;
            }
            break;

        case 'E':
            if (!strcasecmp(word, "End")) {
                fMatch = TRUE;
                return;
            }
            break;

        case 'N':
            SKEY("Name", city[n].name);
            break;

        case 'O':
            SKEY("Of", city[n].of_city);
            break;

        case 'R':
            KEY("Room", city[n].room, fread_number(fp));
            break;

        case 'T':
            if (!strcmp(word, "Tribe")) {
                tmp = fread_string_no_whitespace(fp);
                if (is_number(tmp)) {
                    city[n].tribe = atoi(tmp);
                } else {
                    i = lookup_clan_by_name(tmp);

                    if (i == -1)
                        city[n].tribe = TRIBE_NONE;
                    else
                        city[n].tribe = i;
                }
                free(tmp);
                fMatch = TRUE;
                break;
            }
            break;

        case 'Z':
            KEY("Zone", city[n].zone, fread_number(fp));
            break;
        }

        if (!fMatch) {
            sprintf(buf, "Fread_cities: no match for '%s'", word);
            shhlog(buf);
        }
    }
}


void
boot_cities()
{
    FILE *fp;
    char buf[MAX_STRING_LENGTH];
    int i;

    sprintf(buf, "%s", CITIES_FILE);
    if (!(fp = fopen(buf, "r"))) {
        gamelog("Error opening cities");
        exit(0);
    }

    for (i = 0; i < MAX_CITIES && !feof(fp); i++)
        fread_city(i, fp);

    fclose(fp);
}


void
unboot_cities()
{
    int i;

    for (i = 0; i < MAX_CITIES; i++) {
        int commod;

        free(city[i].name);
        free(city[i].of_city);
        city[i].c_flag = 0;
        city[i].zone = 0;
        city[i].tribe = 0;
        city[i].room = 0;
        for (commod = 0; commod < MAX_COMMOD; commod++)
            city[i].price[commod] = 0.0;
    }
}


void
fwrite_city(int ct, FILE * fp)
{
    int commod = 0;

    fprintf(fp, "Name      %s~\n", city[ct].name);
    fprintf(fp, "Of        %s~\n", city[ct].of_city);

    if (city[ct].c_flag) {
        for (commod = 0; char_flag[commod].name[0] != '\0'; commod++)
            if (char_flag[commod].bit == city[ct].c_flag)
                fprintf(fp, "Crim_flag %s~\n", char_flag[commod].name);
    } else {
        fprintf(fp, "Crim_flag none~\n");
    }

    fprintf(fp, "Zone      %d\n", city[ct].zone);

    if (city[ct].tribe > 0 && city[ct].tribe < MAX_CLAN)
        fprintf(fp, "Tribe     %s~\n", clan_table[city[ct].tribe].name);
    else
        fprintf(fp, "Tribe     %d~\n", city[ct].tribe);

    fprintf(fp, "Room      %d\n", city[ct].room);

    for (commod = 0; commod < MAX_COMMOD; commod++) {
        int i;

        for (i = 0; commodities[i].name[0] != '\0'; i++)
            if (commodities[i].val == commod) {
                fprintf(fp, "Commod %s~ %f\n", commodities[i].name, city[ct].price[commod]);
                break;
            }
    }

    fprintf(fp, "End\n");
}

void
save_cities()
{
    int ct;
    FILE *fp;
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "%s", CITIES_FILE);
    if (!(fp = fopen(buf, "w"))) {
        gamelog("Error writing cities");
        exit(1);
    }

    for (ct = 0; ct < MAX_CITIES; ct++)
        fwrite_city(ct, fp);

    fflush(fp);
    fclose(fp);

}

char *
add_crs(const char *str)
{
    int i, j, len, lines;
    char *output = NULL;

    if (str == NULL)
        return NULL;

    len = strlen(str);
    lines = count_lines_in_str((char *) str);
    CREATE(output, char, len + lines + 1);

    for (i = 0, j = 0; i < len; i++, j++) {
        output[j] = str[i];
        if (str[i] == '\n') {   // It's LF
            if (((i + 1) < len) && (str[i + 1] == '\r'))        // Next char is CR
                continue;
            // Add CR
            output[++j] = '\r';
        }
    }
    output[j] = '\0';           // NULL-terminate the string
    return output;
}

void
strip_crs(char *str)
{
    int i;

    if (str == 0)
        return;

    for (i = 0; i < strlen(str);) {
        if (str[i] != 13)
            i++;
        else {
            /*shorten string by 1 character, this oen */
            memmove(&(str[i]), &(str[i + 1]), strlen(&(str[i])));
        }
    }
};


/***************************************************************************/
/***************************************************************************/
/*                                                                         */
/*                            DATABASE CONNECTIONS                         */
/*                                                                         */
/***************************************************************************/
/***************************************************************************/
/* functions used down below */


void
login_character(CHAR_DATA *ch) {
    if( IS_NPC(ch) || ch->logged_in == TRUE || !ch->desc) return;

    // mark them logged in
    ch->logged_in = TRUE;
}

void logout_character(CHAR_DATA *ch) {
    if(IS_NPC(ch) || ch->logged_in == FALSE) return;

    ch->logged_in = FALSE;
    flush_queued_commands(ch, TRUE);
}


