/*
 * File: DB.H
 * Usage: Database file definitions.
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

#ifndef __DB_INCLUDED
#define __DB_INCLUDED


#ifdef __cplusplus
extern "C"
{
#endif

#define MUD_DIR           "/cygdrive/e/CodeDumpEasy/RunDir"
#define DFLT_DIR          "/cygdrive/e/CodeDumpEasy/RunDir/lib"
#define LIB_DIR           "/cygdrive/e/CodeDumpEasy/RunDir/lib"
#define IMMORT_DIR  	  "/cygdrive/e/CodeDumpEasy/RunDir/lib/immortals"

/* new directory structure for saving player files to */
#ifndef MUDUTILS
#define ACCOUNT_DIR       "accounts"
#define ACCOUNTLOCK_DIR   "accountlock"
#define ALT_ACCOUNT_DIR   "email_accounts"
#define SERVER_FILE       "easyArm"
#endif

#define NEW_INFO_DIR  "data_files/new_info"
#define DEL_DIR       PLAYERS_DIR "/deleted"
#define IMMORTAL_DIR  "immortals"
#define USER_DIR       IMMORTAL_DIR "/users"

#define WARNING_LOG_FILE   	"warning_log"


#define ZONE_FILE         "data_files/zones"    /* zone command tables  */
#define ZONE_TEST_FILE    "data_files/zones.test"       /* zone command tables 
                                                         * for test boot  */
#define GUILD_FILE        "data_files/guilds"   /* the guild data       */
#define SUB_GUILD_FILE     "data_files/subguilds"       /* the guild data       */
#define RACE_FILE         "data_files/races"    /* the race data        */

#define CITIES_FILE       "data_files/cities"   /* the city data        */
#define COMMAND_FILE      "data_files/commands" /* the command data     */

#define WEATHER_FILE      "data_files/weather"  /* weather info         */
#define TIME_FILE         "data_files/time"     /* game calendar infor  */
#define EMAIL_FILE        "data_files/addresses.banned" /* banned emails */
#define CLAN_FILE         "data_files/clans"    /* clan structs       */

#define CREDITS_FILE      "text_files/credits"  /* game credits         */
#define AREAS_FILE        "text_files/areas"    /* area credits         */
#define NEWS_FILE         "text_files/news"     /* game news            */
#define MOTD_FILE         "text_files/motd"     /* message of the day   */
#define MOTD_TEST_FILE     "text_files/motd.test"       /* message of the day   
                                                         * for test boot */
#define IDEA_FILE         "text_files/ideas"    /* ideas from players   */
#define TYPO_FILE         "text_files/typos"    /* typos found          */
#define BUG_FILE          "text_files/bugs"     /* bugs found           */
#define MESS_FILE         "data_files/messages" /* damage messages      */
#define SOCMESS_FILE      "data_files/actions"  /* social commands      */
#define HELP_KWRD_FILE    "data_files/help_table"       /* for HELP <keyword>   */
#define HELP_PAGE_FILE    "text_files/help"     /* general information  */
#define GODHELP_PAGE_FILE "text_files/godhelp"  /* immortal info        */
#define INFO_FILE         "text_files/info"     /* role-playing info    */
#define DEAD_INFO         "text_files/dead"     /* info on death        */
#define WIZLIST_FILE      "text_files/godlist"  /* list of immortals    */
#define POSEMESS_FILE     "data_files/poses"    /* poses                */
#define BANFILE		  "data_files/banned_sites"     /* banned sites         */
#define MENUFILE	  "text_files/menu"     /* options menu         */
#define STORYFILE         "text_files/story"    /* background story     */
#define GREETFILE         "text_files/greetings"        /* game greeting        */
#define WELCFILE	  "text_files/welcome"  /* welcome message      */
#define BANNERFILE        "text_files/banner"   /* banner               */
#define PROGRAM_FILE	  "data_files/programs"
#define CRAFT_FILE        "data_files/crafting.xml"     /* crafting */

#define HOMEHOST          "127.0.0.1"

/* public procedures in db.c */
    extern void boot_db(void);
    extern void unboot_db(void);
    extern void save_char(struct char_data *ch);

    int save_char_text(struct char_data *ch);
    int save_char_db(struct char_data *ch);

    void zone_update(void);
    void init_char(struct char_data *ch, char *dead);
    void clear_char(struct char_data *ch);
    void clear_object(struct obj_data *obj);
    void reset_char(struct char_data *ch);
    void free_char(struct char_data *ch);
    void free_specials(struct specials_t *specs);
    void free_edesc_list(struct extra_descr_data *edesc_list);
    struct extra_descr_data *copy_edesc_list(struct extra_descr_data *list);
    struct extra_descr_data *find_edesc_info(const char *word, struct extra_descr_data *list);

    extern int real_object(int vnum);
    extern int real_mobile(int vnum);

    extern bool is_save_zone(int zone);
#define REAL     0
#define VIRTUAL  1

    struct obj_data *read_object(int nr, int type);
    struct char_data *read_mobile(int nr, int type);

#define TYPO_MSG "What?\n\r"

#define MODE_SEARCH     (1 << 0)
#define MODE_CONTROL    (1 << 1)
#define MODE_CREATE     (1 << 2)
#define MODE_STR_EDIT   (1 << 3)
#define MODE_MODIFY     (1 << 4)
#define MODE_GRANT      (1 << 5)
#define MODE_LOAD       (1 << 6)
#define MODE_WRITE      (1 << 7)
#define MODE_OPEN       (1 << 8)
#define MODE_CLOSE      (1 << 9)
#define MODE_STAT       (1 << 10)

    struct reset_com
    {
        int room;
        char command;           /* current command                      */
        bool if_flag;           /* if TRUE: exe only if preceding exe'd */
        int depth;
        int arg1;               /*                                      */
        int arg2;               /* Arguments to the command             */
        int arg3;               /*                                      */
        int arg4[6];
        long int state_flags;
        long int color_flags;

        char *stringarg1;
        char *stringarg2;

        /*
         * *  Commands:              *
         * *  'M': Read a mobile     *
         * *  'O': Read an object    *
         * *  'G': Give obj to mob   *
         * *  'P': Put obj in obj    *
         * *  'G': Obj to char       *
         * *  'E': Obj to char equip *
         * *  'D': Set state of door *
         * *  'X': X-tra for obj     *
         */
    };


    struct zone_priv_data
    {
        int uid;
        int type;
        struct zone_priv_data *next;
    };

/* zone definition structure. for the 'zone-table'   */
    struct zone_data
    {
        char *name;             /* name of this zone                  */
        int lifespan;           /* how long between resets (minutes)  */
        int age;                /* current age of this zone (minutes) */
        int top;                /* upper limit for rooms in this zone */
        int reset_mode;         /* conditions for reset (see below)   */
        int is_open;            /* being worked on or open?           */
        int is_booted;          /* zopened?                           */

        sh_int uid;             /* user-id of owner                   */
        sh_int groups;          /* group-id bitvector                 */
        int flags;              /* group privilege flags              */
        struct zone_priv_data *privs;   /* new privlages structure -tene 9/97 */

        int rcount;             /* number of rooms in zone            */
        struct room_data *rooms;        /* list of rooms in zone              */

        struct reset_com *cmd;  /* command table for reset            */
        int num_cmd;            /* number of commands                 */
        int num_cmd_allocated;  /* number of commands allocated.      */
        int time;               /* time stamp of last save            */
        double updated;            /* time stamp of the last zone update */

        void (*func) ();
    };

    struct coded_special
    {
        int (*function)(CHAR_DATA *ch, int cmd, const char *arg, CHAR_DATA *npc, ROOM_DATA *rm, OBJ_DATA *obj_in);
        const char *name;
    };
#ifndef SWIG
    extern struct coded_special coded_specials_list[];
#endif

#define SPECIAL_CODED        1
#define SPECIAL_DMPL         2
#define SPECIAL_JAVASCRIPT   3  /* Javascript special type */

    struct special
    {
        int type;
        int cmd;
        union {
            struct coded_special *cspec;
            char *name;
        } special;
    };

    struct specials_t
    {
        size_t count;
        struct special specials[1];
    };

/* element in npc and object index-tables   */
    struct index_data
    {
        int vnum;
        /* virtual number of this mob/obj           */
        int number;             /* number of existing units of this mob/obj */
        int limit;              /* maximum existing                         */
        /*int num_specials;            */
        /*struct specials_t *specials; special procedures for this mob/obj      */
        void *list;             /* list of characters with number           */
    };


/* for queueing zones for update  */
    struct reset_q_element
    {
        int zone_to_reset;      /* ref to zone_data */
        struct reset_q_element *next;
    };

/* structure for the update queue     */
    struct reset_q_type
    {
        struct reset_q_element *head;
        struct reset_q_element *tail;
    };

    extern struct reset_q_type reset_q;

    struct player_index_element
    {
        char *name;
        int nr;
    };

    struct help_index_element
    {
        char *keyword;
        long pos;
    };

    int mobs_existing_in_zone(int nr);
    int objs_existing_in_room(int nr, struct room_data *room);
    int objs_existing_in_list(int nr, struct obj_data *list);
    void assign_zones(void);


    void stop_guard(struct char_data *ch, bool showFailure);
/****
  stop guard stops all people from guarding the CH.
  stops the CH from guarding anyone else.
  stops the CH from guarding exits.
  ****/

    void stop_guarding(struct char_data *ch);
/****
  stops the CH from guarding anyone else
  stops the CH from guarding exits.
  ****/

    void stop_guard_from_guarding(struct char_data *guarder, struct char_data *guardee);
/****
  stop guarder from guarding guardee
  ***/

    void stop_all_guarding(struct char_data *ch);
/****
  stop guard stops all people from guarding the CH.
  ***/





/* for rooms watching other rooms */
    int set_room_to_watch_room(struct room_data *watched_room, struct room_data *watching_room,
                               const char *mesg, int type);
/* stop room from watching room */
    int set_room_not_watching_room(struct room_data *watched_room, struct room_data *watching_room,
                                   int type);


    int set_room_to_watch_room_resolve(struct watching_room_list *temp);
    int set_room_to_watch_room_num(int watched_room, struct room_data *watching_room,
                                   const char *msg_string, int type);
    void set_rprog(CHAR_DATA * ch, const char *args);
    void unset_rprog(CHAR_DATA * ch, const char *args);



    void login_character(CHAR_DATA *ch);
//    void save_login_information(const char *player_name, const char *account,
//                                const char *hostname);
    void logout_character(CHAR_DATA *ch);
//    void save_logout_information(const char *player_name, const char *account);


    void save_startup_information();
    void save_shutdown_information();
    void db_save_app_approve(const char *approver, const char *account, const char *player);
    void db_save_rej_reject(const char *player_name, const char *account, const char *player);

    void boot_back();
    void boot_cities();
    void boot_clan_bank_accounts();
    void boot_clans();
    void boot_clans_from_db();
    void boot_clans_from_file();
    void boot_crafts();
    void boot_guilds();
    void boot_races();
    void boot_races_from_db();
    void boot_social_messages();
    void boot_subguilds();
    void boot_world();
    void boot_zones();

    void reboot_clans();

    void unboot_back();
    void unboot_boards();
    void unboot_character_list();
    void unboot_cities();
    void unboot_clan_bank_accounts();
    void unboot_clans();
    void unboot_crafts();
    void unboot_guilds();
    void unboot_races();
    void unboot_social_messages();
    void unboot_subguilds();
    void unboot_weather();
    void unboot_world();
    void unboot_zone_command_table();
    void unboot_zone_privs();
    void unboot_zones();

    void reset_connections_after_reboot();

    void add_to_character_list(CHAR_DATA *);
    void remove_from_character_list(CHAR_DATA *);

    void strip_crs(char *str);
    char *add_crs(const char *str);

    extern struct index_data *obj_index;
    extern struct obj_default_data *obj_default;
    extern struct index_data *npc_index;
    extern struct npc_default_data *npc_default;
    extern struct char_data *character_list;
    extern struct room_data *room_list;
    extern struct ban_t *ban_list;

    void copy_reset_com_excluding_room(struct zone_data *src, struct zone_data *dst, int room_num);
    void add_zone_data_for_room(ROOM_DATA * rm, struct zone_data *zdata, int zone_save);
    int add_comm_to_zone(struct reset_com *comm, struct zone_data *zdata);
    void unallocate_reset_com(struct zone_data *zdata);
    int save_all_zone_items(struct zone_data *zdata, int zone, int room_num);

#ifdef __cplusplus
}
#endif

#endif

