/*
 * File: SOCKET.C
 * Usage: Socket handling and communication routines.
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>

#include "telopts.h"
#include "constants.h"
#include "structs.h"
#include "utility.h"
#include "utils.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "skills.h"
#include "db.h"
#include "guilds.h"
#include "vt100c.h"
#include "event.h"
#include "limits.h"
#include "modify.h"
#include "wagon_save.h"
#include "threads.h"
#include "io_thread.h"
#include "psionics.h"
#include "watch.h"
#include "info.h"
#include "reporting.h"
#include "special.h"

#define DFLT_PORT 2000            /* default port */
#define MAX_NAME_LENGTH 15
#define MAX_HOSTNAME   256
#define OPT_USEC 250000


/* global variables */
int gameport;
int lock_mortal = 0;            /* lockout mortals from playing   */
int lock_new_char = 0;          /* lockout new character creation */
int lock_arena = 1;             /* lockout arena games */
DESCRIPTOR_DATA *descriptor_list = (DESCRIPTOR_DATA *) 0;
DESCRIPTOR_DATA *next_to_process = (DESCRIPTOR_DATA *) 0;
DESCRIPTOR_DATA *crash_processing = (DESCRIPTOR_DATA *) 0;

#define IO_THREAD_POOL_SIZE 5
queued_thread *io_thread[IO_THREAD_POOL_SIZE];

char do_negotiate_ws[] = { IAC, DO, TELOPT_NAWS, '\0' };

int perf_logging = 1;
int lawful = 0;
int slow_death = 0;
int shutdown_game = 0;
int mud_reboot = 0;
int no_specials = 0;
int unboot_for_memory_check = 0;
int test_boot = 0;
int maxdesc;
int avail_descs;
int real_avail_descs = 100;
int tics = 0;
int diffport = FALSE;

/* local functions */
extern void gather_who_stats();
extern int disease_check (CHAR_DATA *ch);


int get_from_q(struct txt_q *queue, char *dest);
int run_the_game();
int game_loop(int s);
int init_socket(int port);
int new_descriptor(int s);
int process_output(DESCRIPTOR_DATA * t);
int process_input();
void close_sockets(int s);
struct timeval timediff(struct timeval *a, struct timeval *b);
void flush_queues(DESCRIPTOR_DATA * d);
void nonblock(int s);
void parse_name();
void dc_signal(int signo, void (*func) ());

extern void save_zone(struct zone_data *zdata, int zone_num);

#define SADUS_PATCH 1
#ifdef SADUS_PATCH
/* Tenebrius recieved the code for dc_signal to replace select()  
   Sadus of Dark Castle, fscdo@camelot.acf-lab.alaska.edu
   */
void
dc_signal(int signo, void (*func) ())
{
    char buf[50];
    struct sigaction act;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART; /* SVR4, 4.3+BSD */
#endif

    if (sigaction(signo, &act, (struct sigaction *) NULL) < 0) {
        sprintf(buf, "failed sigaction: %d, in dc_signal() (comm.c)", signo);
        perror(buf);
        fflush(stderr);
    }
}
#endif

int
wild_match(const char *s, const char *d)
{
    while (*s) {
        switch (*s) {
        case '?':
            if (!*d)
                return 0;
            s++;
            d++;
            break;
        case '*':
            if (wild_match(s + 1, d))
                return 1;
            if (!*d)
                return 0;
            d++;
            break;
        default:
            if (LOWER(*s) != LOWER(*d))
                return 0;
            s++;
            d++;
        }
    }
    if (*d)
        return 0;
    return 1;
}

int
addrout(struct in_addr a, char *buf, int dodns)
{
    long i;

    i = ntohl(a.s_addr);
    sprintf(buf, "%ld.%ld.%ld.%ld", (i >> 24) & 0xff, (i >> 16) & 0xff, (i >> 8) & 0xff, i & 0xff);
    if (dodns)  
        io_thread_send_msg(NULL, NULL, 0, IO_T_MSG_DNS_LOOKUP, buf, strlen(buf) + 1);
    return (1);
}

char *
eval_prompt(char *pr, CHAR_DATA * ch)
{
    char real_name_buf[256];
    static char buf[MAX_STRING_LENGTH];
    char *p = pr;
    char *r = buf;
    char buf1[MAX_STRING_LENGTH];
    int i;

    bzero(buf, MAX_STRING_LENGTH);

    while (p && *p) {
        switch (*p) {
        case '%':
            p += 1;
            switch (*p) {
            case 'a':
                p += 1;         /* skip % and code */
                if (GET_ACCENT(ch) != LANG_NO_ACCENT) {

                    for (i = 0; accent_table[i].skillnum != LANG_NO_ACCENT; i++) {
                        if (accent_table[i].skillnum == GET_ACCENT(ch)) {
                            strcat(buf, accent_table[i].name);
                            r += strlen(accent_table[i].name);
                        }
                    }
                } else {
                    sprintf(buf1, "unknown");
                    strcat(buf, buf1);
                    r += strlen(buf1);
                }
                break;
            case 'A': {
                int hand = -1;

                p += 1;
                if( get_weapon(ch, &hand)) {
                    sprintf(buf1, "armed");
                    strcat( buf, buf1 );
                    r += strlen(buf1);
                }
                else {
                    sprintf(buf1, "unarmed");
                    strcat( buf, buf1 );
                    r += strlen(buf1);
                }
                break;
            }
            case 'D':
                if (IS_SWITCHED_IMMORTAL(ch)) {
                    p += 1;         /* skip % and code */
                    sprintf(buf1, "%d", npc_index[ch->nr].vnum);
                    strcat(buf, buf1);
                    r += strlen(buf1);
                }
                break;
            case 'd':
            {
                char buffer[512];

                p+= 1;
                if (!ch->in_room 
                 || has_special_on_cmd(ch->in_room->specials, "cave_emulator", -1)
                 || get_room_extra_desc_value(ch->in_room, "[time_failure]", buffer, sizeof(buffer))
                 || ch->in_room->sector_type == SECT_SOUTHERN_CAVERN
                 || ch->in_room->sector_type == SECT_CAVERN
                 || ch->in_room->sector_type == SECT_SEWER
                 || ch->in_room->zone == 34
                 || IS_AFFECTED(ch, CHAR_AFF_SLEEP)) {
                    strcpy(buf1, "unknown");
                } else {
                    strcpy(buf1, weekday_names[((time_info.day +1) % MAX_DOTW)]);
                }
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            }
            case 'e':
            {
                char buffer[512];

                p+= 1;
                if (!ch->in_room 
                 || has_special_on_cmd(ch->in_room->specials, "cave_emulator", -1)
                 || get_room_extra_desc_value(ch->in_room, "[time_failure]", buffer, sizeof(buffer))
                 || ch->in_room->sector_type == SECT_SOUTHERN_CAVERN
                 || ch->in_room->sector_type == SECT_CAVERN
                 || ch->in_room->sector_type == SECT_SEWER
                 || ch->in_room->zone == 34
                 || IS_AFFECTED(ch, CHAR_AFF_SLEEP)) {
                    strcpy(buf1, "unknown");
                } else {
                    switch (time_info.hours) {
                    case NIGHT_EARLY:
                        strcpy(buf1, "before dawn");
                        break;
                    case DAWN:
                        strcpy(buf1, "dawn");
                        break;
                    case RISING_SUN:
                        strcpy(buf1, "early morning");
                        break;
                    case MORNING:
                        strcpy(buf1, "late morning");
                        break;
                    case HIGH_SUN:
                        strcpy(buf1, "high sun");
                        break;
                    case AFTERNOON:
                        strcpy(buf1, "early afternoon");
                        break;
                    case SETTING_SUN:
                        strcpy(buf1, "late afternoon");
                        break;
                    case DUSK:
                        strcpy(buf1, "dusk");
                        break;
                    case NIGHT_LATE:
                        strcpy(buf1, "late at night");
                        break;
                    } 
                }
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            }
            case 'E':          /* Encumbrance */
                p += 1;        /* skip % and code */
                sprintf(buf1, "%s", encumb_class(get_encumberance(ch)));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'f':          /* skip % and code */
                p += 1;
                if (affected_by_spell(ch, SPELL_FEATHER_FALL))
                    sprintf(buf1, "feather fall");
                else if (affected_by_spell(ch, SPELL_LEVITATE))
                    sprintf(buf1, "levitating");
                else if (affected_by_spell(ch, SPELL_FLY))
                    sprintf(buf1, "flying");
                else if (IS_AFFECTED(ch, CHAR_AFF_FLYING))
                    sprintf(buf1, "flying");
                else
                    sprintf(buf1, "not flying");
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'h':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%d", GET_HIT(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'H':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%d", GET_MAX_HIT(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'i':
                p += 1;         /* skip % and code */
                if (IS_AFFECTED(ch, CHAR_AFF_INVISIBLE))
                    sprintf(buf1, "Invis");
                else
                    sprintf(buf1, "Vis");
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'I':
                if( IS_IMMORTAL(ch) ) {
                    p += 1;         /* skip % and code */
                    sprintf(buf1, "%d", ch->specials.il);
                    strcat(buf, buf1);
                    r += strlen(buf1);
                }
                break;
            case 'k':
                p += 1;
                if (!ch->specials.riding)       /* crashed it */
                    sprintf(buf1, "riding: none");
                else
                    sprintf(buf1, "riding: %s", PERS(ch, ch->specials.riding));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'l':
                p += 1;
                sprintf(buf1, "%s", ch->long_descr ? "[ldesc]" : "");
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'L':
                p += 1;
                sprintf(buf1, "%s%s", ch->long_descr ? "ldesc : " : "",
                        ch->long_descr ? ch->long_descr : "");
                if (ch->long_descr)
                    buf1[strlen(buf1) - 2] = '\0';
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'm':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%d", GET_MANA(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'M':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%d", GET_MAX_MANA(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'n':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%s", REAL_NAME(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'N':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%s", FORCE_NAME(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'o':          /* skip % and code */
                p += 1;
                sprintf(buf1, "%s", skill_name[GET_SPOKEN_LANGUAGE(ch)]);
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'O':
                p += 1;
                if( !get_char_mood(ch, buf1, sizeof(buf1))) {
                    strcpy( buf1, "neutral" );
                }
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'r':
                if (IS_SWITCHED_IMMORTAL(ch)) {
                    p += 1;
                    sprintf(buf1, "%d", ch->in_room ? ch->in_room->number : -1);
                    strcat(buf, buf1);
                    r += strlen(buf1);
                }
                break;
            case 's':
                p += 1;         /* skip % and code */
                switch (GET_POS(ch)) {
                case 0:
                    sprintf(buf1, "dead");
                    break;
                case 1:
                    sprintf(buf1, "mortally wounded");
                    break;
                case 2:
                    sprintf(buf1, "stunned");
                    break;
                case 3:
                    sprintf(buf1, "sleeping");
                    break;
                case 4:
                    sprintf(buf1, "resting");
                    break;
                case 5:
                    sprintf(buf1, "sitting");
                    break;
                case 6:
                    if (!ch->specials.fighting) /* crashed it */
                        sprintf(buf1, "fighting: none");
                    else
                        sprintf(buf1, "fighting: %s", PERS(ch, ch->specials.fighting));
                    break;
                case 7:
                    sprintf(buf1, "standing");
                    break;
                default:
                    gamelog("Illegal position in eval prompt");
                    break;
                }
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'S':
                p += 1;         /* skip % and code */
                switch (GET_POS(ch)) {
                case 0:
                    sprintf(buf1, "dead");
                    break;
                case 1:
                    sprintf(buf1, "mortally wounded");
                    break;
                case 2:
                    sprintf(buf1, "stunned");
                    break;
                case 3:
                    if (ch->on_obj) {
                        if (ch->on_obj->table)
                            sprintf(buf1, "sleeping at: %s", 
                             format_obj_to_char(ch->on_obj->table, ch, 1));
                        else
                            sprintf(buf1, "sleeping on: %s",
                             format_obj_to_char(ch->on_obj, ch, 1));
                    }
                    else
                        sprintf(buf1, "sleeping");
                    break;
                case 4:
                    if (ch->on_obj) {
                        if (ch->on_obj->table)
                            sprintf(buf1, "resting at: %s", 
                             format_obj_to_char(ch->on_obj->table, ch, 1));
                        else
                            sprintf(buf1, "resting on: %s",
                             format_obj_to_char(ch->on_obj, ch, 1));
                    }
                    else
                        sprintf(buf1, "resting");
                    break;
                case 5:
                    if (ch->on_obj) {
                        if (ch->on_obj->table)
                            sprintf(buf1, "sitting at: %s", 
                             format_obj_to_char(ch->on_obj->table, ch, 1));
                        else
                            sprintf(buf1, "sitting on: %s",
                             format_obj_to_char(ch->on_obj, ch, 1));
                    }
                    else
                        sprintf(buf1, "sitting");
                    break;
                case 6:
                    if (!ch->specials.fighting) /* crashed it */
                        sprintf(buf1, "fighting: none");
                    else
                        sprintf(buf1, "fighting: %s", PERS(ch, ch->specials.fighting));
                    break;
                case 7:
                    if (ch->specials.riding)
                       sprintf(buf1, "riding: %s", PERS(ch, ch->specials.riding));
                    else if (ch->on_obj) {
                        OBJ_DATA *on = ch->on_obj;

                        if (IS_SET(on->obj_flags.value[1], FURN_SKIMMER)
                         || IS_SET(on->obj_flags.value[1], FURN_WAGON)) {
                            sprintf(buf1, "standing on: %s", 
                             format_obj_to_char(on, ch, 1));
                        }
                        else
                            sprintf(buf1, "standing at: %s",
                             format_obj_to_char(on, ch, 1));
                    }
                    else
                       sprintf(buf1, "standing");
                    break;
                default:
                    gamelog("Illegal position in eval prompt");
                    break;
                }
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 't':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%d", GET_STUN(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'T':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%d", GET_MAX_STUN(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'v':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%d", GET_MOVE(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'V':
                p += 1;         /* skip % and code */
                sprintf(buf1, "%d", GET_MAX_MOVE(ch));
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            case 'w':          /* skip % and code */
                p += 1;
                if (GET_SPEED(ch) == SPEED_RUNNING)
                    sprintf(buf1, "running");
                else if (GET_SPEED(ch) == SPEED_WALKING)
                    sprintf(buf1, "walking");
                else if (GET_SPEED(ch) == SPEED_SNEAKING)
                    sprintf(buf1, "sneaking");
                else
                    sprintf(buf1, "unknownspeed");
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            default:
                *r++ = *p++;
                break;
            }
            break;

        case '\\':
            p += 1;
            switch (*p) {
            case 'n':
                p += 1;
                sprintf(buf1, "\n\r");
                strcat(buf, buf1);
                r += strlen(buf1);
                break;
            }
            break;

        default:
            *r++ = *p++;
            break;
        }
    }
    *r = '\0';
    return buf;
}

/**
	@memo	Main entry fuction of the program
	@doc	This is the start of the processing for the program.

	@param	argc	Number of arguments passed in the command line,
			including the command itself

	@param	argv	Array of pointers to each argument passed. For example,
			argv[0] is the program name used on the command line.

	@return		Exit value (Always returns 0)
*/
int
main(int argc, char **argv)
{
    char buf[512];
    int pos = 1;
    char *dir;
    char *dbname;

    /* Set the rescource limits on corefile size to unlimited.  This should
     * allow the game to override shell limits on corefiles size. (Expected
     * result, the shell may actually override the program).
     * Tiernan 7/16/2003
     */
    struct rlimit corelimit = { RLIM_INFINITY, RLIM_INFINITY };
    setrlimit(RLIMIT_CORE, &corelimit);

    gameport = DFLT_PORT;
    dir = DFLT_DIR;
 //   strcpy( database_name, DFLT_DB );

    diffport = FALSE;
    while ((pos < argc) && (*(argv[pos]) == '-')) {
        switch (*(argv[pos] + 1)) {
        case 'i':
            lock_mortal = 1;
            break;
        case 'n':
            lock_new_char = 1;
            break;
        case 'l':
            lawful = 1;
            break;
        case 'P':
            perf_logging = 0;
            break;
        case 'b':
            if(*(argv[pos] + 2))
                dbname = argv[pos] + 2;
            else if (++pos < argc)
                dbname = argv[pos];
            else {
                gamelog("Database name expected after option -b. (ignored in easy version)");
                exit(0);
            }
 //           strcpy( database_name, dbname );
            break;
        case 'd':
            if (*(argv[pos] + 2))
                dir = argv[pos] + 2;
            else if (++pos < argc)
                dir = argv[pos];
            else {
                gamelog("Directory arg expected after option -d.");
                exit(0);
            }
            diffport = TRUE;
            break;
        case 's':
            no_specials = 1;
            break;
        case 'u':
            unboot_for_memory_check = 1;
            break;
        case 't':
            test_boot = 1;
            break;
        default:
            sprintf(buf, "Unknown option -% in argument string.", *(argv[pos] + 1));
            gamelog(buf);
            break;
        }

        pos++;
    }

    if (pos < argc) {
        if (!isdigit(*argv[pos])) {
            fprintf(stderr, "Usage: %s [-l] [-s] [-d pathname] [ port # ]\n", argv[0]);
            exit(0);
        } else if (((gameport = atoi(argv[pos])) <= 1024)
                   && (gameport != 23)) {
            printf("Illegal port #\n");
            exit(0);
        }
    }

    sprintf(buf, "Running server on port %d: pid %d.", gameport, getpid());
    gamelog(buf);


    gamelog(lock_mortal ? "mortals locked out." : "mortals can play.");
    gamelog(lock_new_char ? "new character creation locked out." :
            "new character creation allowed.");
    gamelog(lawful ? "Lawful mode selected." : "Not in Lawful mode.");
    gamelog(test_boot ? "Test boot." : "Full world boot.");
    gamelog(no_specials ? "Suppressing assigning of specials." : "Specials will be assigned");
    gamelog(unboot_for_memory_check ? "Wont free memory when shutting down." :
            "Will free memory when shutting down.");

    gamelogf("Using %s as the data directory.", dir);

    if (chdir(dir) < 0) {
        perror("chdir");
        exit(0);
    }


    srandom(time(0));

    message_queue iot_inq, iot_outq;
    queue_init(&iot_inq);
    queue_init(&iot_outq);

    int i;
    for (i = 0; i < IO_THREAD_POOL_SIZE; i++) {
        io_thread[i] =
            create_queued_thread(io_thread_init, io_thread_destroy, io_thread_handler, &iot_inq,
                                 &iot_outq);
    }

    init_game_stats();
    init_game_stats_table();
//    reset_connections_after_reboot();

#ifdef SADUS_PATCH
    /* dc_signal is part of  patch recieved from 
     * Sadus of Dark Castle, fscdo@camelot.acf-lab.alaska.edu
     */
    dc_signal(SIGPIPE, SIG_IGN);
#endif

//    save_startup_information();     /* write a reboot to Database */

    run_the_game();

    for (i = 0; i < IO_THREAD_POOL_SIZE; i++) {
        shhlogf("awaiting thread %d end...", i);
        destroy_queued_thread(io_thread[i]);
        shhlogf("reaped thread %d", i);
    }


    fflush(stderr);

    return (0);
}



/* init sockets, run game, and cleanup sockets */
int
run_the_game()
{
    int s;
    void signal_setup(void);

    descriptor_list = NULL;

    /*    GC_enable_incremental(); */
    gamelog("Signal trapping.");
    signal_setup();

    boot_db();
    /* GC_dont_expand = 1; */

    gamelog("Opening mother connection.");
    s = init_socket(gameport);

    gamelog("Entering game loop.");

    game_loop(s);

    close_sockets(s);

    /* ony really need to unboot the db if debuggin
     * memory and stuff  
     */
    unboot_db();

    if (mud_reboot) {
        gamelog("Rebooting.");
        exit(52);
    }

    gamelog("Normal termination of game.");

    return 0;
}

void
customSelect(DESCRIPTOR_DATA * point)
{
    char peek;
    void *null = NULL;
    int input, output, exc = 0;

    if ((input = recv(point->descriptor, (void *) &peek, 1, MSG_PEEK)) < 0) {
        input = 0;
        if (errno != EINTR && errno != EWOULDBLOCK)
            exc = 1;
    } else
        input = 1;

    if ((output = send(point->descriptor, null, 0, 0)) < 0) {
        output = 0;
        if (errno != EWOULDBLOCK)
            exc = 1;
    } else {
        output = 1;
    }

    point->input_set = input;
    point->output_set = output;
    point->exc_set = exc;
}

void
update_characters() {
    CHAR_DATA *ch, *ch_next;
    struct timeval start;
    perf_enter(&start, "update_characters()");

    global_game_stats.npc_instances = 0;
    for (ch = character_list; ch; ch = ch_next) {
        ch_next = ch->next;

        if (!affect_update(ch)) continue;
        ch_next = ch->next;

        if (!npc_intelligence(ch)) continue;
        ch_next = ch->next;

        // Note that to have NPCs regen like PCs, remove this if check
        // they can regen, the problem is more AI is needed before this
        // becomes desired
        if (!IS_NPC(ch)) {
            if (!update_stats(ch)) continue;
            if (!disease_check(ch)) continue;
        } else {
            global_game_stats.npc_instances++;
        }
        ch_next = ch->next;
    }
    perf_exit("update_characters()", start);
}

/* accept new connects, relay commands, and process events */
DESCRIPTOR_DATA *next_point;
int
game_loop(int s)
{
    char comm[MAX_INPUT_LENGTH + 256], tmp[256], buf[256];
    int pulse = 0, wait;
    sigset_t mask;
    fd_set input_set, output_set, exc_set;
    struct timeval last_time, now, timespent, timeout, null_time, loop_start;
    static struct timeval opt_time;

    DESCRIPTOR_DATA *point;
    struct char_fall_data *cfd, *next_cfd;
    struct obj_fall_data *ofd, *next_ofd;
    int result = -1;
    int z, zone;

    null_time.tv_sec = 0;
    null_time.tv_usec = 0;

    opt_time.tv_usec = OPT_USEC;
    opt_time.tv_sec = 0;
    gettimeofday(&last_time, (struct timezone *) 0);


    z = FD_SETSIZE;
    real_avail_descs = getdtablesize() - 4;
    avail_descs = MIN(1000, real_avail_descs - 10);
    sprintf(buf, "Descriptors: %d, FD_SETSIZE: %d", avail_descs, z);
    gamelog(buf);

    memset(&mask, 0, sizeof(mask));
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGPIPE);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGURG);
    sigaddset(&mask, SIGXCPU);
    sigaddset(&mask, SIGHUP);

    while (!shutdown_game) {
        perf_enter(&loop_start, "game_loop");
        // commenting this out for now.  This needs to be performance tuned
        //report_game_stats();
        FD_ZERO(&input_set);
        FD_ZERO(&output_set);
        FD_ZERO(&exc_set);

        FD_SET(s, &input_set);

        maxdesc = s;
        for (point = descriptor_list; point; point = point->next) {
            maxdesc = MAX(maxdesc, point->descriptor);
            FD_SET(point->descriptor, &input_set);
            FD_SET(point->descriptor, &exc_set);
            FD_SET(point->descriptor, &output_set);
        }

        /* check how long in last loop */
        gettimeofday(&now, (struct timezone *) 0);
        timespent = timediff(&now, &last_time);
        timeout = timediff(&opt_time, &timespent);
        wait = (timeout.tv_sec > 0 || timeout.tv_usec > 0);

        last_time.tv_sec = now.tv_sec + timeout.tv_sec;
        last_time.tv_usec = now.tv_usec + timeout.tv_usec;

        if (last_time.tv_usec >= 1000000) {
            last_time.tv_usec -= 1000000;
            last_time.tv_sec++;
        }
        sigprocmask(SIG_SETMASK, &mask, NULL);

        result = -1;

        /* check for input on sockets */
        struct timeval socket_input_start;
        perf_enter(&socket_input_start, "input_on_sockets");
        while (result < 0) {
            result = select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time);
            if (result < 0 && errno != EINTR) {
                perror("Game_loop: select1: poll");
                exit(1);
            }
        }
        perf_exit("input_on_sockets", socket_input_start);

        /* wait if nothing to do */
        perf_enter(&socket_input_start, "input_on_sockets 2");
        if (wait) {
            int result = -1;
            while (result < 0) {
                result = select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout);
                if (result < 0 && errno != EINTR) {
                    perror("Game_loop: select2: poll");
                    exit(1);
                }
            }
        }
        perf_exit("input_on_sockets 2", socket_input_start);

        sigemptyset(&mask);
        sigprocmask(SIG_SETMASK, &mask, NULL);

        /* new connection? */
        if (FD_ISSET(s, &input_set))
            if (new_descriptor(s) < 0)
                perror("New connection");

        /* close dead descriptors */
        struct timeval dead_conn_start;
        perf_enter(&dead_conn_start, "dead connections");
        for (point = descriptor_list; point; point = next_point) {
            next_point = point->next;
            if (FD_ISSET(point->descriptor, &exc_set)) {
                FD_CLR(point->descriptor, &input_set);
                FD_CLR(point->descriptor, &output_set);
            }
        }
        perf_exit("dead connections", dead_conn_start);

        /* process input */
        struct timeval input_start;
        perf_enter(&input_start, "process input");
        for (point = descriptor_list; point; point = next_point) {
            next_point = point->next;
            if (FD_ISSET(point->descriptor, &input_set))
	        if (process_input(point) < 0)
                    close_socket(point);
        }
        perf_exit("process input", input_start);

        /* backup character scores */
        for (point = descriptor_list; point; point = point->next) {
            if (!point->connected && point->character && point->term
                && IS_SET(point->character->specials.act, CFL_INFOBAR)) {
                point->character->old.hp = GET_HIT(point->character);
                point->character->old.max_hp = GET_MAX_HIT(point->character);
                point->character->old.mana = GET_MANA(point->character);
                point->character->old.max_mana = GET_MAX_MANA(point->character);
                point->character->old.move = GET_MOVE(point->character);
                point->character->old.max_move = GET_MAX_MOVE(point->character);
                point->character->old.stun = GET_STUN(point->character);
                point->character->old.max_stun = GET_MAX_STUN(point->character);
            }
        }

        struct timeval oob_start;
        perf_enter(&oob_start, "oob commands");
        /* process OOB commands */
        for (point = descriptor_list; point; point = next_to_process) {
            next_to_process = point->next;

            if (point->character && get_from_q(&point->oob_input, comm)) {
                point->idle = 0;
                point->prompt_mode = 1;

                if (point->showstr_point)
                    show_string(point, comm);
                else
                    parse_command(point->character, comm);
            }
        }
        perf_exit("oob commands", oob_start);

        struct timeval commands_start;
        perf_enter(&commands_start, "commands");
        /* process commands */
        for (point = descriptor_list; point; point = next_to_process) {
            next_to_process = point->next;
            crash_processing = point;

            // There is a character and they're not delayed
            if (point->character) {
                // Tick down the delay timer
                point->character->specials.act_wait = MAX(0, point->character->specials.act_wait -1);
                if (!(point->character->specials.act_wait > 0)
                    && point->character->queued_commands) {
                    // Pop the command from the queue and perform it
                    point->prompt_mode = parse_command(point->character, pop_queued_command(point->character));
                    // Safe to continue since parse_command will adjust act_wait
                }
            }
            
            // There is input to be processed.  (Menu, editor, etc).
            if (get_from_q(&point->ib_input, comm)) {
                if (point->character && (point->connected == CON_PLYNG)
                    && point->character->specials.was_in_room) {
                    if (point->character->in_room)
                        char_from_room(point->character);
                    char_to_room(point->character, point->character->specials.was_in_room);
                    point->character->specials.was_in_room = (struct room_data *) 0;
                    act("$n has returned.", TRUE, point->character, 0, 0, TO_ROOM);
                }
            
                point->idle = 0;        /* Descriptor idle count reset to 0 */
            
                if (point->character)
                    point->character->specials.timer = 0;
            
                point->prompt_mode = 1;
            
                if (point->str) {
                    string_add(point, comm);
                } else if (point->editing) {
                    add_line_to_file(point, comm);
                } else if (point->showstr_point) {
                    show_string(point, comm);
                } else if (!point->connected) {
                    point->prompt_mode = parse_command(point->character, comm);
                } else {
                    menu_interpreter(point, comm);
                }
            } else {
                if (((point->idle++ > (WAIT_SEC * 20)))
                    && ((point->connected == CON_SLCT)  /* main menu */
                      ||(point->connected == CON_ANSI))) {    /* ansi select */
                    write_to_descriptor(point->descriptor,
                                        "\n\r\n\rYou have been idle too long, please reconnect when you are back.\n\r\n\r");
                    sprintf(tmp, "Disconnecting descriptor %d (%s), idle %d seconds in state %d",
                            point->descriptor, point->host, point->idle / WAIT_SEC,
                            point->connected);
                    shhlog(tmp);
                    close_socket(point);
                } else if ((point->idle > WAIT_SEC * 60 * 5)
                           && point->connected != CON_PLYNG) {
                    write_to_descriptor(point->descriptor,
                                        "\n\r\n\rYou have been idle too long, please reconnect when you are back.\n\r\n\r");
                    sprintf(tmp, "Disconnecting descriptor %d (%s), idle %d seconds in state %d",
                            point->descriptor, point->host, point->idle / WAIT_SEC,
                            point->connected);
                    shhlog(tmp);
                    close_socket(point);
                }
            }
        }
        perf_exit("commands", commands_start);

        struct timeval output_start;
        perf_enter(&output_start, "output");
        for (point = descriptor_list; point; point = next_point) {
            crash_processing = point;
            next_point = point->next;
            if (FD_ISSET(point->descriptor, &output_set) && point->output.head) {
                if (process_output(point) < 0)
                    close_socket(point);
                else
                    point->prompt_mode = 1;
            }
        }
        perf_exit("output", output_start);

        struct timeval thread_start;
        perf_enter(&thread_start, "thread_handling");
        while (thread_recv_message(0, io_thread[0]->outq, &io_thread_handle_replies)) ;  // Spin, handling messages from the I/O thread
        perf_exit("thread_handling", thread_start);

        /* give the people some prompts */
        for (point = descriptor_list; point; point = point->next) {
            if (point->prompt_mode) {
                if (point->str || point->editing) {
                    write_to_descriptor(point->descriptor, "-> ");
                } else if (point->showstr_point) {
                    if (point->connected)
                        write_to_descriptor(point->descriptor, "***** PRESS RETURN *****");
                    else
                        write_to_descriptor(point->descriptor, "\n\r[MORE]");
                } else if (!point->connected) {
                    CHAR_DATA *ch = point->character;

                    if (ch->player.prompt
                        && strcmp("(null)", ch->player.prompt)) {
                        sprintf(tmp, "%s",
                                eval_prompt(ch->player.prompt, ch));
                    } else 
                        sprintf(tmp, "%s",
                                eval_prompt("%hhp %vmv %tst>", ch));

                    if (IS_SET(ch->specials.brief, BRIEF_PROMPT)) {
                       // if we don't have a last prompt
                       if( ch->last_prompt == NULL
                        // or we do and it isn't the same as the new prompt
                        || (ch->last_prompt != NULL
                        && strcmp(tmp, ch->last_prompt))) { 
                          // show the prompt
                          write_to_descriptor(point->descriptor, tmp);
                          // if we had a last prompt, free it
                          if( ch->last_prompt != NULL ) free(ch->last_prompt);
                          // and store the new last prompt
                          ch->last_prompt = strdup(tmp);
                       }
                       // otherwise gag the prompt
                    } else {
                       // brief prompt not on, always show a prompt
                       write_to_descriptor(point->descriptor, tmp);
                    }
                }

                point->prompt_mode = 0;
            }
        }

        /* Pulse zones if neccisary */
        const int runfast = 0;

        gettimeofday(&now, NULL);
 
        pulse++;

        for (zone = 0; zone < top_of_zone_table; zone++) {
            if (zone_table[zone].is_booted && (now.tv_sec+(now.tv_usec/1000000.0) >= zone_table[zone].updated + 60))
                zone_update();
        }

        /* Pulse emails if neccisary */
        if (!(pulse % (SECS_PER_REAL_HOUR * WAIT_SEC) * 4)) {
            gamelog("Loading email addresses");
            load_email_addresses();
        }

        /* Deliver mail if neccisary */
        if (runfast || !(pulse % (SECS_PER_MUD_DAY * WAIT_SEC / (16 * 5)))) {
            /* HACK ALERT */
            deliver_mail();
        }

        /* update weather if neccisary */
        if (!(pulse % (SECS_PER_MUD_HOUR * WAIT_SEC))) {
            point_update();
            update_weather();
        }

        // New time-setting routine
        if (runfast || check_time(&time_info)) {
            update_shops();
            update_time();
        }

        /* process command q */
        if (runfast || !(pulse % (2 * WAIT_SEC)))
            process_command_q();

        if (runfast || !(pulse % (20 * WAIT_SEC))) {       /* every 10 minutes */
            add_rest_to_npcs_in_need();
        }

        /* process npcs */
        if (runfast || !(pulse % WAIT_SEC)) {
            update_characters();
            handle_combat();
            /* process event queue */
            heap_update(runfast ? 20 : 4);
        }

        /* reset pulse periodically */
        if (pulse >= 32000)
            pulse = 0;

        struct timeval fall_start;
        perf_enter(&fall_start, "falling obj");
        /* process falling objects */
        for (ofd = obj_fall_list; ofd; ofd = next_ofd) {
            next_ofd = ofd->next;
            fallob(ofd->falling);
        }
        perf_exit("falling obj", fall_start);

        perf_enter(&fall_start, "falling ch");
        /* process falling characters */
        for (cfd = char_fall_list; cfd; cfd = next_cfd) {
            next_cfd = cfd->next;
            if (!number(0, 10)) {
                fallch(cfd->falling, ++cfd->num_rooms);
            }
        }
        perf_exit("falling ch", fall_start);

        /* process virtual psionic contacts/messages */
        make_contacts();

        struct timeval infobar_start;

        perf_enter(&infobar_start, "infobar");
        /* give some more prompts */
        for (point = descriptor_list; point; point = point->next) {
            if (!point->connected && point->character && point->term
                && IS_SET(point->character->specials.act, CFL_INFOBAR)) {
                if ((GET_HIT(point->character) != point->character->old.hp)
                    || (GET_MAX_HIT(point->character) != point->character->old.max_hp)) {
                    sprintf(tmp, "%d(%d)", GET_HIT(point->character),
                            GET_MAX_HIT(point->character));
                    adjust_infobar(point, GINFOBAR_HP, tmp);
                }

                if ((GET_MANA(point->character) != point->character->old.mana)
                    || (GET_MAX_MANA(point->character) != point->character->old.max_mana)) {
                    sprintf(tmp, "%d(%d)", GET_MANA(point->character),
                            GET_MAX_MANA(point->character));
                    adjust_infobar(point, GINFOBAR_MANA, tmp);
                }

                if ((GET_MOVE(point->character) != point->character->old.move)
                    || (GET_MAX_MOVE(point->character) != point->character->old.max_move)) {
                    sprintf(tmp, "%d(%d)", GET_MOVE(point->character),
                            GET_MAX_MOVE(point->character));
                    adjust_infobar(point, GINFOBAR_STAM, tmp);
                }

                if ((GET_STUN(point->character) != point->character->old.stun)
                    || (GET_MAX_STUN(point->character) != point->character->old.max_stun)) {
                    sprintf(tmp, "%d(%d)", GET_STUN(point->character),
                            GET_MAX_STUN(point->character));
                    adjust_infobar(point, GINFOBAR_STUN, tmp);
                }
            }
        }
        perf_exit("infobar", infobar_start);


        release_extracted();
        /* do it all again! */
        tics++;
        perf_exit("game_loop", loop_start);
    }

    // Shutting down, do some housecleaning and save the save zones
    for (zone = 0; zone < top_of_zone_table; zone++) {
        if (zone_table[zone].is_booted && is_save_zone(zone))
        save_zone(&zone_table[zone], zone);
    }

    return 1;
}

/* UTILITY */

int
get_from_q(struct txt_q *queue, char *dest)
{
    struct txt_block *tmp;

    if (!queue->head) {
        return 0;
    }
    tmp = queue->head;
    strcpy(dest, queue->head->text);
    queue->head = queue->head->next;

    free(tmp->text);
    free(tmp);

    return 1;
}

void
write_to_q(const char *txt, struct txt_q *queue)
{
    struct txt_block *newtb;
    int n;

    if (!txt) {
        gamelogf("write_to_q (or SEND_TO_Q) called with null 'txt' parameter, bailing.");
        return;
    }

    CREATE(newtb, struct txt_block, 1);
    n = strlen(txt);
    if ((n < 0) || (n > MAX_STRING_LENGTH)) {
        gamelog("Bad strlen, write_to_q().");
        free(newtb);
        return;
    }
    CREATE(newtb->text, char, n + 1);

    strcpy(newtb->text, txt);

    if (!queue->head) {
        newtb->next = (struct txt_block *) 0;
        queue->head = queue->tail = newtb;
    } else {
        queue->tail->next = newtb;
        queue->tail = newtb;
        newtb->next = (struct txt_block *) 0;
    }
}


/* empty the queues before closing connection */
void
flush_queues(DESCRIPTOR_DATA * d)
{
    char dummy[MAX_STRING_LENGTH];

    while (get_from_q(&d->output, dummy))
        write_to_descriptor(d->descriptor, dummy);
    while (get_from_q(&d->ib_input, dummy));
    while (get_from_q(&d->oob_input, dummy));
}


/* SOCKET HANDLING */


/* allocate socket */
int
init_socket(int port)
{
    int opt = 1;                /* was char *opt */
    char hostname[MAX_HOSTNAME + 1];
    int s;

    struct sockaddr_in sa;
    struct hostent *hp;
    /*   struct linger ld;   */
    int maxbuf = 65535;
    socklen_t buffer;
    socklen_t buflen;
    char buf[MAX_STRING_LENGTH];

    bzero(&sa, sizeof(struct sockaddr_in));
    gethostname(hostname, MAX_HOSTNAME);
    if (!(hp = gethostbyname(hostname))) {
        perror("gethostbyname");
        exit(1);
    }

    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons(port);
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
/* put in for custom select - hal */
    nonblock(s);

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }
/*  Taking out the SO_LINGER so sockets will close faster - 4/27/99 Tiernan
  ld.l_onoff = 1;
  ld.l_linger = 1000;
  if (setsockopt(s, SOL_SOCKET, SO_LINGER, (caddr_t)&ld, sizeof(ld)) < 0) 
    {
      perror("setsockopt");
      exit(1);
    }
*/

    // DEBUG code to gamelog what output buffer size defaulted to
    buflen = sizeof(buffer);
    buffer = 0;
    if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, &buffer, &buflen) < 0) {
        perror("getsockopt");
        exit(1);
    }
    sprintf(buf, "SNDBUF SIZE WAS SET TO %d.", buffer);
    shhlog(buf);

    // This adjusts the output socket buffer to the maximum value allowed
    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &maxbuf, sizeof(maxbuf)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    sprintf(buf, "SNDBUF SIZE NOW SET TO %d.", maxbuf);
    shhlog(buf);

    // DEBUG code to gamelog what input buffer size defaulted to
    buflen = sizeof(buffer);
    buffer = 0;
    if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, &buffer, &buflen) < 0) {
        perror("getsockopt");
        exit(1);
    }
    sprintf(buf, "RCVBUF SIZE WAS SET TO %d.", buffer);
    shhlog(buf);
    // This adjusts the input socket buffer to the maximum value allowed
    if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &maxbuf, sizeof(maxbuf)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    sprintf(buf, "RCVBUF SIZE NOW SET TO %d.", maxbuf);
    shhlog(buf);

    /*    if (bind(s, &sa, sizeof(sa), 0) < 0) { */
    if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        perror("bind");
        close(s);
        exit(1);
    }

    listen(s, 100);

    return s;
}


/* set up a new descriptor struct */
int
new_descriptor(int s)
{
    char buf2[256], tmpaddr[49];
    int desc, number = 0;
    socklen_t i;

    DESCRIPTOR_DATA *point;
    struct sockaddr_in sock;
    char msg[100];
    DESCRIPTOR_DATA *newd, *nextdesc, **prevdesc;
    struct ban_t *tmp;
    /*  char *tmp_s; */

    i = sizeof(sock);

    /* this function is called 1nc per game loop to check for new
     * connections, accept returns a positive descriptor if someone
     * was waiting, and a if nobody is waiting returns a non-positive
     * and sets errorno to EWOULDBLOCK if nobody is waiting */

    if ((desc = accept(s, (struct sockaddr *) &sock, &i)) < 0) {
        return -1;
    }

    sprintf(msg, "New socket %d, accepted as descriptor %d", s, desc);
    shhlog(msg);

    nonblock(desc);
 //   if (addrout(sock.sin_addr, tmpaddr, 0)) {
 //       *(tmpaddr + 49) = '\0';
 //   } else {
        strcpy(tmpaddr, "localhost");
 //   }

    for (point = descriptor_list; point; point = point->next)
        number++;

    if (((number + 1) >= avail_descs) && (strcmp(tmpaddr, HOMEHOST))) {
        sprintf(buf2, "armSolo is full? Huh?  There is currently a limit of %d players.\n\r",
                avail_descs);
        write_to_descriptor(desc, buf2);
        close(desc);
        return 0;
    } else if (desc > maxdesc)
        maxdesc = desc;

    CREATE(newd, DESCRIPTOR_DATA, 1);

 //   if (addrout(sock.sin_addr, newd->host, 1)) {
 //       *(newd->host + 49) = '\0';
 //       strcpy(newd->hostname, newd->host);
 //   } else {
        strcpy(newd->host, "127.0.0.1");
        strcpy(newd->hostname, "localhost");
 //   }

#define DEBUG_SOCKETS
#ifdef DEBUG_SOCKETS
/*
    Using number below might be useful - Kel  
    But is just more spam, now that the problem is solved - Tenebrius
*/

    sprintf(buf2, "New connection from %s (number %d).", newd->host, number);
    shhlog(buf2);

    //sprintf(buf2, "/* New connection from %s, descriptor %d. */\n\r", newd->host, desc);
    //send_to_overlords(buf2);
#endif


    newd->descriptor = desc;
    newd->connected = CON_ANSI;
    newd->wait = 1;
    newd->idle = 0;
    newd->pagelength = 24;
    newd->linesize = 80;
    newd->prompt_mode = 0;
    *newd->buf = '\0';
    newd->str = 0;
    newd->max_str = MAX_STRING_LENGTH;
    newd->editing = 0;
    newd->showstr_head = 0;
    newd->showstr_point = 0;
    *newd->last_input = '\0';
    newd->output.head = 0;
    newd->ib_input.head = 0;
    newd->oob_input.head = 0;
    newd->character = 0;
    newd->original = 0;
    newd->snoop.snooping = 0;
    newd->snoop.snoop_by = 0;
    newd->term = 0;
    *newd->path = '\0';


    newd->player_info = 0;
    newd->pass_wrong = 0;
    newd->editing_display = 0;

#define SORT_DESCRIPTOR_BY_CONNECTION 1
#ifdef SORT_DESCRIPTOR_BY_CONNECTION
    /* insert alphabeticaly to list */
    for (prevdesc = &descriptor_list, nextdesc = descriptor_list;
         nextdesc && (strcmp(nextdesc->host, newd->host) < 0);
         prevdesc = &(nextdesc->next), nextdesc = nextdesc->next) {
        /* search thru until nextdesc->host is alphabeticaly equal to 
         * or less than the newd->host */
    };
    newd->next = *prevdesc;
    *prevdesc = newd;
#else
    /* prepend to list */
    newd->next = descriptor_list;
    descriptor_list = newd;
#endif
    write_to_descriptor(newd->descriptor, do_negotiate_ws);
    write_to_descriptor(newd->descriptor, IAC_WILL_MSSP);
    show_main_menu(newd);
    newd->connected = CON_SLCT;

    return 0;
}


int
process_output(DESCRIPTOR_DATA * t)
{
    char i[MAX_STRING_LENGTH + 1];

    if (!t) {
        shhlog("t was 0 into process_output");
        return (-1);
    };
    if (!t->prompt_mode && !t->connected)
        if (write_to_descriptor(t->descriptor, "\n\r") < 0)
            return -1;

    while (get_from_q(&t->output, i)) {
        if (t->snoop.snoop_by) {
            write_to_q("# ", &t->snoop.snoop_by->desc->output);
            write_to_q(i, &t->snoop.snoop_by->desc->output);
        }

        if (write_to_descriptor(t->descriptor, i))
            return -1;
    }

    if (!t->connected && !(t->character && !IS_NPC(t->character)
                           && IS_SET(t->character->specials.act, CFL_COMPACT)))
        if (write_to_descriptor(t->descriptor, "\n\r") < 0)
            return -1;

    return 1;
}


int
write_to_descriptor(int desc, const char *txt)
{
    char errbuf[256];
    int sofar, thisround, total;

/*
    if (txt[0] >= 'a' && txt[0] <= 'z')
        txt[0] = UPPER(txt[0]);
*/

    total = strlen(txt);
    sofar = 0;

    do {
        thisround = write(desc, txt + sofar, total - sofar);
        if (thisround < 0) {
            snprintf(errbuf, sizeof(errbuf), "write_to_descriptor: write() failed, %d: %s", thisround, strerror(errno));
            shhlog(errbuf);
            return -1;
        }

        sofar += thisround;
    } while (sofar < total);

    return 0;
}


void setup_infobar(struct descriptor_data *d);


int
process_input(DESCRIPTOR_DATA * t)
{
    int sofar, thisround, begin, squelch, i, k, flag;
    char tmp[MAX_INPUT_LENGTH + 2], buffer[MAX_INPUT_LENGTH + 60];
    char errbuf[1000];
//    char *name = "";
//    int room = -1;

    sofar = 0;
    flag = 0;
    begin = strlen(t->buf);

    /* Read in some stuff */
    do {
        if ((thisround =
             read(t->descriptor, t->buf + begin + sofar,
                  MAX_STRING_LENGTH - (begin + sofar) - 1)) > 0) {
            char *cpsource;

            /* set a pointer to the input received so we can scan it */
            cpsource = t->buf + begin + sofar;

            sofar += thisround;
            t->buf[begin + sofar] = '\0';       /* null terminate it */

            while (*cpsource != *(t->buf + begin + sofar)) {
		/* is it a telnet IAC command ? */
                if (*((unsigned char *) cpsource) == IAC) {
		    cpsource++;
		    switch (*((unsigned char *) cpsource)) {
			/* subnegotiation */
		    case WILL:
			cpsource++;
			break;	

		    case DO:
			cpsource++;
			switch (*((unsigned char *) cpsource)) {
			case TELOPT_MSSP:
			     {
				 cpsource++;
			     break;
			     }
			default:
			    cpsource++;
			    break;
			}
		    case SB:
                        cpsource++;
			switch (*((unsigned char *) cpsource)) {
			    /* negotiate window size */
                        case TELOPT_NAWS:
                            {
				int linesize, pagelength;
                                cpsource++;
				linesize = t->linesize;
                                t->linesize =
                                    *((unsigned char *) cpsource) * 255 +
                                    *((unsigned char *) (cpsource + 1));

                                cpsource += 2;

                                pagelength = t->pagelength;
                                t->pagelength =
                                    *((unsigned char *) cpsource) * 255 +
                                    *((unsigned char *) (cpsource + 1));
                                if (t->character && IS_SET(t->character->specials.act, CFL_INFOBAR)
                                    && (linesize != t->linesize || pagelength != t->pagelength)) {
                                    setup_infobar(t);
                                    display_infobar(t);
                                }

                                cpsource++;
                                break;
                            }
                        default:
                            cpsource++;
                            break;
                        }
                        /* end subnegotiation */
                    case SE:
                        cpsource++;
                        break;
                    default:
                        cpsource++;
                        break;
                    }
                } else
                    cpsource++;
            }
        } else {
            if (thisround < 0) {
                if (errno != EWOULDBLOCK) {
                    shhlogf(errbuf, "Read1 on descriptor %d : '%s'", t->descriptor, t->buf);
                    return (-1);
                } else {
                    break;
                }
            } else {
                return (-1);
            }
        }
    } while (!ISNEWL(*(t->buf + begin + sofar - 1)));

    *(t->buf + begin + sofar) = 0;
// never used
 //   if (t->character) {
 //       name = MSTR(t->character, name);
 //       if (t->character->in_room)
 //           room = t->character->in_room->number;
//    };


    /* if no newline is contained in input, return without proc'ing */
    for (i = begin; !ISNEWL(*(t->buf + i)); i++)
        if (!*(t->buf + i))
            return (0);

    /* input contains 1 or more newlines; process the stuff */
    for (i = 0, k = 0; *(t->buf + i);) {
        if (!ISNEWL(*(t->buf + i)) && !(flag = (k >= (MAX_INPUT_LENGTH - 2))))
            if (*(t->buf + i) == '\b') {        /* backspace */
                if (k) {        /* more than one char ? */
                    //if (*(tmp + --k) == '$')
                    //    k--;
		    k--;
                    i++;
                } else {
                    i++;        /* no or just one char.. Skip backsp */
                }
            } else {
                if (isascii(*(t->buf + i)) && isprint(*(t->buf + i))) {
                    /* trans char, double for '$' (printf)  */
                    *(tmp + k) = *(t->buf + i);
                    //if (*(tmp + k) == '$')
                    //    *(tmp + ++k) = '$';
                    k++;
                    i++;
                } else {
                    i++;
                }
        } else {
            *(tmp + k) = 0;
            if (*tmp == '!')
                strcpy(tmp, t->last_input);
            else
                strcpy(t->last_input, tmp);

            if (flag) {
                char *endOfTmp = tmp + strlen(tmp) - 6;

                for (; !isspace(*endOfTmp) && endOfTmp != tmp; endOfTmp--);

                *(endOfTmp) = '.';
                *(endOfTmp + 1) = '.';
                *(endOfTmp + 2) = '.';
                *(endOfTmp + 3) = '\0';

                sprintf(buffer, "Line too long. Truncated to:\n\r%s\n\r", tmp);
                if (write_to_descriptor(t->descriptor, buffer) < 0)
                    return (-1);

                /* skip the rest of the line */
                for (; !ISNEWL(*(t->buf + i)); i++);
            }

            if (is_oob_command(t, tmp)) {
                write_to_q(tmp, &t->oob_input);
            } else
                write_to_q(tmp, &t->ib_input);

            if ((t->snoop.snoop_by) && (t->snoop.snoop_by->desc)) {
                write_to_q("# ", &t->snoop.snoop_by->desc->output);
                write_to_q(tmp, &t->snoop.snoop_by->desc->output);
                write_to_q("\n\r", &t->snoop.snoop_by->desc->output);
            }


            /* find end of entry */
            for (; ISNEWL(*(t->buf + i)); i++);

            /* squelch the entry from the buffer */
            for (squelch = 0;; squelch++)
                if ((*(t->buf + squelch) = *(t->buf + i + squelch)) == '\0')
                    break;
            k = 0;
            i = 0;
        }
    }
    return (1);
}


void
close_sockets(int s)
{
    gamelog("Closing all sockets.");

    while (descriptor_list)
        close_socket(descriptor_list);

    close(s);
}

void
close_socket(DESCRIPTOR_DATA * d)
{
    DESCRIPTOR_DATA *tmp;
    char buf[100];

    if (d->term) {
        write_to_descriptor(d->descriptor, VT_INITSEQ);
        write_to_descriptor(d->descriptor, VT_CLENSEQ);
    }

    sprintf(buf, "closing descriptor: %d", d->descriptor);
    shhlog(buf);

    d->idle = 0;
    if (d->descriptor == maxdesc)
        --maxdesc;

    if (d->snoop.snooping)
        d->snoop.snooping->desc->snoop.snoop_by = 0;

    if (d->snoop.snoop_by) {
        send_to_char("Your victim is no longer among us.\n\r", d->snoop.snoop_by);
        d->snoop.snoop_by->desc->snoop.snooping = 0;
    }

    if (d->monitoring) {
        MONITOR_LIST *pl, *pl_next;

        for (pl = d->monitoring; pl; pl = pl_next) {
            pl_next = pl->next;
            free(pl);
        }
        d->monitoring = NULL;
        d->monitoring_all = FALSE;
        d->monitor_all_show = 0;
    }

    /* if they're switched, change them back to original -Morg 11/17/1999 */
    if (d->original && d->character) {
        cmd_return(d->character, "in_close_socket", 0, 0);
    }

    if (d->character) {
        int room_num = (d->character->in_room ? d->character->in_room->number : 0);
        sprintf(buf, "%s (%s) [%s] has lost link.\n\r", MSTR(d->character, name),
                d->player_info->name, d->hostname);
        send_to_monitor(d->character, buf, MONITOR_OTHER);

        logout_character(d->character);

        if (d->connected == CON_PLYNG) {
            sprintf(buf, "%s (%s) [%s] has lost link.", GET_NAME(d->character),
                    d->player_info->name, d->hostname);
            shhlog(buf);
            sprintf(buf, "/* %s (%s) [%s] has lost link. */\n\r", GET_NAME(d->character),
                    d->player_info->name, d->hostname);

            connect_send_to_higher(buf, STORYTELLER, d->character);
            d->character->desc = 0;

            if(affected_by_spell(d->character, SPELL_POSSESS_CORPSE)) {
                die(d->character);
            } else {
                cmd_save(d->character, "", CMD_SAVE, 0);

                remove_listener(d->character);      /* removes character from
                                                 * psionic listen list */
                save_char(d->character);
                if (room_num == 1012) {
                    parse_command(d->character, "leave");
                }
                act("$n has lost link.", TRUE, d->character, 0, 0, TO_ROOM);
            }
        } else {
            sprintf(buf, "%s (%s) [%s] has disconnected.", (d->character && (d->character->name)
                                                            && *(d->character->name)) ? (d->
                                                                                         character->
                                                                                         name) :
                    "(Unknown)", d->player_info
                    && d->player_info->name ? d->player_info->name : "(None)", d->hostname);
            shhlog(buf);
            sprintf(buf, "/* %s (%s) [%s] has disconnected. */\n\r",
                    (d->character && (d->character->name)
                     && *(d->character->name)) ? (d->character->name) : "(Unknown)", d->player_info
                    && d->player_info->name ? d->player_info->name : "(None)", d->hostname);
            if (d->character)
                connect_send_to_higher(buf, STORYTELLER, d->character);

            free_char(d->character);
            d->character = 0;
        }
    }

    if (d->player_info && d->player_info->name && d->player_info->email) {
        /* store the logout time */
        add_out_login_data(d->player_info, d->descriptor);
        save_player_info(d->player_info);
    }

    if (d->player_info) {
        PLAYER_INFO *pPInfo = d->player_info;

        d->player_info = NULL;
        free_player_info(pPInfo);
    }

    flush_queues(d);
    close(d->descriptor);

    if (next_to_process == d)
        next_to_process = next_to_process->next;

    if (d == descriptor_list)
        descriptor_list = descriptor_list->next;
    else {
        for (tmp = descriptor_list; tmp && (tmp->next != d); tmp = tmp->next);
        if (tmp)
            tmp->next = d->next;
        else
            errorlog("tried to close a descriptor not in descriptor_list");
    }

    if (d->showstr_head)
        free(d->showstr_head);

    free(d);
}


void
nonblock(int s)
{
    if (fcntl(s, F_SETFL, FNDELAY) == -1) {
        perror("fcntl");
        exit(1);
    }
}


/* COMMUNICATION */

void
send_to_char(const char *messg, CHAR_DATA * ch)
{
    if (ch == NULL)
        return;

    if (ch->desc && messg)
        write_to_q(messg, &ch->desc->output);
}

void
send_to_charf(CHAR_DATA * ch, const char *fmt, ...)
{
    char buf[16384];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    send_to_char(buf, ch);
}

void
send_to_all_wagon_rooms(char *msg, OBJ_DATA * wagon)
{
    char bug[MAX_STRING_LENGTH];

    extern void wagon_room_send(struct room_data *room, char *msg);

    if (wagon->obj_flags.type != ITEM_WAGON) {
        sprintf(bug, "Error in send_to_all_wagon_rooms:  Item %s(%d) is not a wagon.",
                OSTR(wagon, short_descr), obj_index[wagon->nr].vnum);
        gamelog(bug);
        return;
    }

    map_wagon_rooms(wagon, (void *) wagon_room_send, (void *) msg);

    return;
}


/* 08/22/2000 -- Azroen
  This function handles sending a message to everyone in adjacent rooms
  from where it was called.  It uses a $D modifier to substitute direction
  if used.  It can also be used in conjunction with act() using $D.

  Examples:

    send_to_nearby_rooms("You see a flash of light to the $D.\n\r", ch->in_room); 
  would be seen by a person north of character 'ch':
    "You see a flash of light to the south."

    act("You hear a twang from $D you, and see $n holding a bow.", 
                    FALSE, ch, 0, 0, TO_NEARBY_ROOMS);
  would be seen by a person below character 'ch':
    "You hear a twang from above you, and see the emerald-eyed elf holding a bow."
*/
void
send_to_nearby_rooms(const char *messg, ROOM_DATA * start_rm)
{
    register char *strp, *point, *i = NULL;
    char *str;
    char to_room_buf[MAX_STRING_LENGTH + 2];
    char buf[MAX_STRING_LENGTH];
    char bug[MAX_STRING_LENGTH];
    char dirmsg[20];
    int d;
    CHAR_DATA *to;
    ROOM_DATA *to_room;

    if (!messg)
        return;
    if (!*messg)
        return;

    if (strlen(messg) > MAX_STRING_LENGTH) {
        errorlog("input to send_to_nearby_rooms() was longer than MAX_STRING_LENGTH");
        return;
    }

    strcpy(to_room_buf, messg);

    for (d = DIR_NORTH; d <= DIR_DOWN; d++) {
        if (start_rm->direction[d] && (to_room = start_rm->direction[d]->to_room)) {
            to = to_room->people;
            for (; to;) {

                str = to_room_buf;
                if (d <= DIR_WEST)
                    sprintf(dirmsg, "%s", rev_dirs[d]);
                else
                    sprintf(dirmsg, "%s", (d == DIR_UP) ? "below" : "above");

                for (strp = str, point = buf;;)
                    if (*strp == '$') {
                        switch (*(++strp)) {
                        case 'D':
                            i = dirmsg;
                            break;
                        default:
                            gamelog("Illegal $-code to send_to_nearby_rooms():");
                            gamelog(str);
                            break;
                        }
                        while ((*point = *(i++)))
                            ++point;
                        ++strp;
                    } else if (!(*(point++) = *(strp++)))
                        break;

                sprintf(bug, "Sending to room %d.", to_room->number);
/*
gamelog(bug);
*/
                if (to->desc) {
                    write_to_q(CAP(buf), &to->desc->output);
                }
                to = to->next_in_room;
            }                   /* end for to exists */

        }                       /* End if to_room exists */
    }                           /* End for direction loop */

}

void
handler_for_wagon_sends(ROOM_DATA * room, char *mesg, int type)
{
    char bug[MAX_STRING_LENGTH];
    CHAR_DATA *i;
    OBJ_DATA *wagon, *next_obj;
    ROOM_DATA *pilot_room;

    for (wagon = room->contents; wagon; wagon = next_obj) {
        next_obj = wagon->next;
        if (wagon->obj_flags.type == ITEM_WAGON) {
            if (type == TO_WAGON)
                send_to_all_wagon_rooms(mesg, wagon);
            if (type == TO_PILOT) {
                pilot_room = get_room_num(wagon->obj_flags.value[2]);
                if (pilot_room) {
                    for (i = pilot_room->people; i; i = i->next_in_room)
                        if (i->desc)
                            write_to_q(mesg, &i->desc->output);

/*                send_to_room(mesg, pilot_room);  */
                } else {
                    sprintf(bug,
                            "Error in handler_for_wagon_sends: Wagon %s(%d)"
                            " in room %d has no pilot room.", OSTR(wagon, short_descr),
                            obj_index[wagon->nr].vnum, pilot_room->number);
                    gamelog(bug);
                }
            }
        }
    }                           /* End for statement */
}                               /* End handler_for_wagon_sends */

void
cprintf(CHAR_DATA * ch, const char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH];
    va_list argp;

    va_start(argp, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    va_end(argp);
    send_to_char(buf, ch);
}

void
send_to_all(const char *messg)
{
    DESCRIPTOR_DATA *i;

    if (messg)
        for (i = descriptor_list; i; i = i->next)
            if (!i->connected)
                write_to_q(messg, &i->output);
}

void
send_to_higher(const char *msg, int level, CHAR_DATA * ch)
{
    DESCRIPTOR_DATA *i;

    if (!msg)
        return;

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected && i->character && GET_LEVEL(i->character) >= level
            && !IS_SET(i->character->specials.quiet_level, QUIET_LOG) && !i->str
            && CAN_SEE(i->character, ch))
            write_to_q(msg, &i->output);
}

void
connect_send_to_higher(const char *msg, int level, CHAR_DATA * ch)
{
    DESCRIPTOR_DATA *i;

    if (!msg)
        return;

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected && i->character && GET_LEVEL(i->character) >= level
            && !IS_SET(i->character->specials.quiet_level, QUIET_CONNECT) && !i->str
            && CAN_SEE(i->character, ch))
            write_to_q(msg, &i->output);
}


void
send_to_overlords(const char *messg)
{
    DESCRIPTOR_DATA *i;

    if (messg)
        for (i = descriptor_list; i; i = i->next)
            if (!i->connected && i->character && (GET_LEVEL(i->character) >= OVERLORD)
                && !IS_SET(i->character->specials.quiet_level, QUIET_LOG) && !i->str)
                write_to_q(messg, &i->output);
}

void
send_to_outdoor(char *messg)
{
    DESCRIPTOR_DATA *i;

    if (messg)
        for (i = descriptor_list; i; i = i->next)
            if (!i->connected)
                if (OUTSIDE(i->character) && AWAKE(i->character))
                    if (!i->str)
                        write_to_q(messg, &i->output);
}

void
send_to_except(const char *messg, CHAR_DATA * ch)
{
    DESCRIPTOR_DATA *i;

    if (messg)
        for (i = descriptor_list; i; i = i->next)
            if (ch->desc != i && !i->connected)
                write_to_q(messg, &i->output);
}



void
send_to_room(const char *messg, struct room_data *room)
{
    CHAR_DATA *i;
    OBJ_DATA *o;
    if (room == 0) {
        errorlog("NULL SEND TO ROOM");
        return;
    }

    if (messg)
        for (i = room->people; i; i = i->next_in_room)
            if (i->desc)
                write_to_q(messg, &i->desc->output);

    if (messg)
        for (o = room->contents; o; o = o->next_content)
            if (o->obj_flags.type == ITEM_WAGON)
                send_to_wagon(messg, o);
}

void
send_to_room_except(const char *messg, struct room_data *room, CHAR_DATA * ch)
{
    CHAR_DATA *i;

    if (messg)
        for (i = room->people; i; i = i->next_in_room)
            if (i != ch && i->desc)
                write_to_q(messg, &i->desc->output);
}

void
send_to_room_except_two(const char *messg, struct room_data *room, CHAR_DATA * ch1, CHAR_DATA * ch2)
{
    CHAR_DATA *i;

    if (messg)
        for (i = room->people; i; i = i->next_in_room)
            if (i != ch1 && i != ch2 && i->desc)
                write_to_q(messg, &i->desc->output);
}


/* gvsprintf = Game Value String Print Formatted
 * Print to the specified print_str the message formatted with appropriate
 * game values.
 *
 *   print_str - string to print to
 *   print_len - amount of memory available in print_str
 *   to_ch     - character whose perspective we are seeing things
 *   msg       - message to format
 *   args      - list of objects to use to format the message
 */
void
gvsprintf(char *print_str, const size_t print_len, CHAR_DATA * to_ch, const char *msg,
          struct obj_list_type *args)
{
    const char *p;
    char *print = print_str;
    char *new = NULL;
    CHAR_DATA *ch = NULL;
    OBJ_DATA *obj = NULL;
    struct room_data *rm = NULL;
    char buf[MAX_INPUT_LENGTH];

    struct obj_list_type *args_p = args;

    if (!msg) {
        gamelogf("gvsprintf: null message");
        return;
    }

    args_p = args;
    bzero(print_str, print_len);
    print = print_str;
    for (p = msg; *p; p++) {
        if (!args_p && *p == '$') { // No args, just skip format char
            *print++ = *p;
            continue;
        }

        if (*p != '$') {
            *print++ = *p;
            continue;
        }

        if (!*++p) {
            gamelogf("gvprintf: string ends on '$'");
            return;
        }

        if (!args_p) {
            gamelogf("gvprintf: Not enough objects to match formatting codes");
            return;
        }

        ch = NULL;
        obj = NULL;
        rm = NULL;

        /* examine the first code to determine type of object */
        switch (*p) {
        case ('o'):
            obj = (OBJ_DATA *) args_p->obj;
            break;
        case ('c'):
            ch = (CHAR_DATA *) args_p->obj;
            break;
        case ('r'):
            rm = (struct room_data *) args_p->obj;
            break;
        default:
            gamelogf("gvprintf: Unknown object identifier '%c'", *p);
            return;
        }
        args_p = args_p->next;

        if (!*++p) {
            gamelogf("gvprint: Message ends before completion of formatting code");
            return;
        }

        /* examine how they want the object formatted */
        switch (*p) {
            /* short description with 'you' for to_ch */
        case ('s'):
            if (ch) {
                if (ch == to_ch)
                    new = strcpy(buf, "you");
                else
                    new = strcpy(buf, PERS(to_ch, ch));
            } else if (obj)
                new = strcpy(buf, OBJS(obj, to_ch));
            else {
                gamelogf("gvprintf: Illegal formatting code 's' for room object");
                return;
            }
            break;
            /* short description, no 'you' option */
        case ('t'):
            if (ch) {
                new = strcpy(buf, PERS(to_ch, ch));
            } else if (obj)
                new = strcpy(buf, OBJS(obj, to_ch));
            else {
                gamelogf("gvprintf: Illegal formatting code 't' for room object");
                return;
            }
            break;
            /* short description without the article */
        case ('a'):
            if (ch)
                new = strcpy(buf, smash_article(PERS(to_ch, ch)));
            else if (obj)
                new = strcpy(buf, smash_article(OBJS(obj, to_ch)));
            else {
                gamelogf("gvprintf: Illegal formatting code 'a' for room object");
                return;
            }
            break;
            /* possessive his/her/your/its */
        case ('p'):
            if (ch) {
                if (ch == to_ch)
                    new = strcpy(buf, "your");
                else {
                    new = strcpy(buf, HSHR(ch));
                }
            } else if (obj) {
                new = strcpy(buf, "its");
            } else {
                gamelogf("gvprintf: Formatting code 'p' only takes a char");
                return;
            }
            break;
            /* he/she/it/you */
        case ('e'):
            if (ch) {
                if (ch == to_ch)
                    new = strcpy(buf, "you");
                else {
                    new = strcpy(buf, HSSH(ch));
                }
            } else if (obj) {
                new = strcpy(buf, "it");
            } else {
                gamelogf("gvprintf: Formatting code 'e' only takes a char");
                return;
            }
            break;
            /* himself/herself/itself/yourself */
	case ('l'):
            if (ch) {
                if (ch == to_ch)
                    new = strcpy(buf, "yourself");
                else {
                    new = strcpy(buf, HMFHRF(ch));
                }
            } else if (obj) {
                new = strcpy(buf, "itself");
            } else {
                gamelogf("gvprintf: Formatting code 'l' only takes a char");
                return;
            }
            break;
            /* his/hers/its/yours */
	case ('g'):
            if (ch) {
                if (ch == to_ch)
                    new = strcpy(buf, "yours");
                else {
                    new = strcpy(buf, HSHRS(ch));
                }
            } else if (obj) {
                new = strcpy(buf, "its");
            } else {
                gamelogf("gvprintf: Formatting code 'g' only takes a char");
                return;
            }
            break;
            /* him/her/it/you */
        case ('m'):
            if (ch) {
                if (ch == to_ch)
                    new = strcpy(buf, "you");
                else {
                    new = strcpy(buf, HMHR(ch));
                }
            } else if (obj) {
                new = strcpy(buf, "it");
            } else {
                gamelogf("gvprintf: Formatting code 'm' only takes a char");
                return;
            }
            break;
            /* 's' if not to_ch */
        case ('y'):
            if (ch) {
                if (ch == to_ch)
                    new = strcpy(buf, "");
                else
                    new = strcpy(buf, "s");
            } else {
                gamelogf("gvprintf: Illegal formatting code 'y' for non-character " "object");
                return;
            }
            break;
            /* name */
        case ('n'):
            if (ch) {
                if (ch == to_ch)
                    new = strcpy(buf, "you");
                else
                    new = first_name(MSTR(ch, name), buf);
            } else if (obj)
                new = strcpy(buf, SANA(obj));
            else
                new = strcpy(buf, rm->name);
            break;
        default:
            gamelogf("gvprintf: Unknown formatting code '%c'", *p);
            return;
        }

        while (*++p && isalpha(*p)) {
            switch (*p) {
                /* initial capitalize */
            case ('c'):
                CAP(new);
                break;
                /* possessive */
            case ('p'):
                if (!strcasecmp(new, "you"))
                    strcat(new, "r");
                else if (new[strlen(new) - 1] == 's')
                    strcat(new, "'");
                else
                    strcat(new, "'s");
                break;
		/* alt possessive */
	    case ('g'):
                if (!strcasecmp(new, "you"))
                    strcat(new, "rs");
                else if (new[strlen(new) - 1] == 's')
                    strcat(new, "'");
                else
                    strcat(new, "'s");
                break;
            default:
                gamelogf("getvprintf: Unknown extra qualifying code '%c'", *p);
                return;
            }
        }
        --p;

        if ((strlen(new) + strlen(print)) >= (print_len - 1)) {
            gamelogf("gvsprintf: Formatted string too long");
            return;
        }

        print = strcat(print, new);
        while (*print)
            print++;
    }

    return;
}

/* gvprintf = Game Value Print Formatted 
 * Formats the specified message for the character and sends it to them or
 * the other people in the room (if to_rm is specified)
 *
 *   to_ch  - person whose perspective should be used
 *   to_rm  - room the message should be sent to (except to_ch)
 *            if null, formatted message will be sent to to_ch
 *   msg    - message to be formatted
 *   args   - list of object data to be used to format the message
 */
void
gvprintf(CHAR_DATA * actor, CHAR_DATA * to_ch, struct room_data *to_rm, const char *msg,
         struct obj_list_type *args, bool watch_to_see, bool silent)
{
    static char print_str[MAX_STRING_LENGTH];
    static char msg_str[MAX_STRING_LENGTH];
    CHAR_DATA *ignore_ch = NULL;

    /* if no message, error out */
    if (!msg) {
        gamelogf("gvprintf: null message");
        return;
    }

    /* if a room is specified */
    if (to_rm) {
        /* if to_ch is specified save it as ignored */
        if (to_ch)
            ignore_ch = to_ch;
        to_ch = to_rm->people;
    }

    /* iterate over the people to receive this message */
    while (to_ch) {
        /* if we are to ignore someone, make sure it's not this person */
        if (to_ch == ignore_ch) {
            /* move on to the next person in the room */
            if (to_rm) {
                to_ch = to_ch->next_in_room;
            } else {
                to_ch = NULL;
            }
            continue;
        }

        /* handle watch checks */
        if (watch_to_see && to_ch != actor) {
            if (is_char_actively_watching_char(to_ch, actor)
                || watch_success(to_ch, actor, NULL, 0, SKILL_NONE)) {
                sprintf(msg_str, "You notice: %s", msg);
            } else {            /* didn't see it, move on to the next one */
                if (to_rm) {
                    to_ch = to_ch->next_in_room;
                } else {
                    to_ch = NULL;
                }
                continue;
            }
        } else if (watch_to_see && to_ch == actor 
         && !IS_SET(to_ch->specials.brief, BRIEF_EMOTE)) {
            // show hidden emote to actor
            sprintf( msg_str, "(%s)", msg);
        } else if (silent && to_ch == actor
         && !IS_SET(to_ch->specials.brief, BRIEF_EMOTE)) {
            // show silent emote to actor
            sprintf( msg_str, "* %s", msg);
        } else {
            strcpy(msg_str, msg);
        }

        /* handle 'silent' messages */
        if (silent && !CAN_SEE(to_ch, actor)) {
            if (to_rm) {
                to_ch = to_ch->next_in_room;
            } else {
                to_ch = NULL;
            }
            continue;
        }

        /* format the message up into a string */
        gvsprintf(print_str, sizeof(print_str), to_ch, msg_str, args);

        /* send the message to to_ch */
        send_to_char(print_str, to_ch);

        if( print_str[strlen(print_str) - 1] != '\r'
         && print_str[strlen(print_str) - 2] != '\n') {
           send_to_char("\n\r", to_ch);
        }

        /* if we have a room, move on to the next person */
        if (to_rm)
            to_ch = to_ch->next_in_room;
        else                    /* set to_ch to null to end the loop */
            to_ch = NULL;
    }
}                               /* end gvprintf() */

void
gprintf(CHAR_DATA * to_ch, struct room_data *to_rm, const char *msg, ...)
{
    const char *p;
    int i;
    va_list argp;
    struct obj_list_type *argt = NULL;
    struct obj_list_type *argh = NULL;
    struct obj_list_type *new = NULL;

    for (p = msg, i = 0; (p = strstr(p, "$")); i++)
        p++;

    va_start(argp, msg);
    for (; i > 0; i--) {
        new = (struct obj_list_type *) malloc(sizeof *new);
        new->obj = va_arg(argp, void *);
        new->next = NULL;
        if (argh) {
            argt->next = new;
            argt = new;
        } else {
            argh = new;
            argt = argh;
        }
    }

    gvprintf(NULL, to_ch, to_rm, msg, argh, 0, 0);

    while ((new = argh)) {
        argh = argh->next;
        free(new);
    }

    return;
}

/* higher-level communication */
void
act(const char *in_str,         /* Text to send                          */
    int hide_invisible,         /* Show message if person can't be seen  */
    CHAR_DATA * ch,             /* Character                             */
    void *obj,                  /* Object                                */
    void *vict_obj,             /* second character                      */
    int type)
{                               /* Who to send message to (TO_ROOM, etc) */
    // *****************************************************************************
    // Might be listed as unused because of the #ifdef below, please leave them in
    struct descriptor_data *d;
    ROOM_DATA *curr_room, *curr_room2, *curr_room3, *wroom;
    int curr_dir;
    // *****************************************************************************

    register char *strp, *point;
    const char *i = NULL;
    OBJ_DATA *tmpob;
    CHAR_DATA *to, *vict_ch;
    char to_room_buf[MAX_STRING_LENGTH + 2];
    /*  char logbuf[MAX_STRING_LENGTH+2]; */
    char *str;
    char buf[MAX_STRING_LENGTH], real_name_buf[MAX_STRING_LENGTH];
    int newline = 1;
    int cap = 1;
    int doing_dir_watchers = 0;
    ROOM_DATA *room = 0;
    struct watching_room_list *wlist = 0;

    d = descriptor_list;

    if (!in_str)
        return;
    if (!*in_str)
        return;

    if (strlen(in_str) > MAX_STRING_LENGTH) {
        errorlog("input to act() was longer than MAX_STRING_LENGTH");
        return;
    }
    strcpy(to_room_buf, in_str);
    str = to_room_buf;

    vict_ch = (CHAR_DATA *) vict_obj;
    if (type == TO_VICT)
        to = vict_ch;
    else if (type == TO_CHAR)
        to = ch;
    else if (ch && (ch->in_room))
        to = ch->in_room->people;
    else if (vict_ch && vict_ch->in_room)
        to = vict_ch->in_room->people;
    else if ((OBJ_DATA *) obj && (((OBJ_DATA *) obj)->in_room))
        to = ((OBJ_DATA *) obj)->in_room->people;
    else {
        errorlog("act() error finding ROOM or NOTVICT");
        return;
    }

    if (to) {
        room = to->in_room;
        if (room)
            wlist = room->watched_by;
    }

    for (; to;) {
        if (to->desc
            && (to != ch || type == TO_CHAR || type == TO_NEARBY_ROOMS || type == TO_WAGON
                || type == TO_PILOT)
            && ((!ch || CAN_SEE(to, ch) == 1 || CAN_SEE(to, ch) == 8) || !hide_invisible)
            && AWAKE(to)
            && !((type == TO_NOTVICT || type == TO_NOTVICT_COMBAT) && (to == vict_ch))) {
            for (strp = str, point = buf;;)
                if (*strp == '$') {
                    switch (*(++strp)) {
                    case 'n': i = ch ? PERS(to, ch) : ""; break;
                    case 'N': i = vict_ch ? PERS(to, vict_ch) : ""; break;
                    case 'l': i = HMFHRF(ch); break;
                    case 'L': i = HMFHRF(vict_ch); break;
                    case 'g': i = HSHRS(ch); break;
                    case 'G': i = HSHRS(vict_ch); break;
                    case 'm': i = HMHR(ch); break;
                    case 'M': i = HMHR(vict_ch); break;
                    case 's': i = HSHR(ch); break;
                    case 'S': i = HSHR(vict_ch); break;
                    case 'e': i = HSSH(ch); break;
                    case 'E': i = HSSH(vict_ch); break;
                    case 'o': i = OBJN((OBJ_DATA *) obj, to); break;
                    case 'O': i = OBJN((OBJ_DATA *) vict_obj, to); break;
                    case 'p': i = obj ? OBJS((OBJ_DATA *) obj, to) : ""; break;
                    case 'P': i = OBJS((OBJ_DATA *) vict_obj, to); break;
                    case 'a': i = SANA((OBJ_DATA *) obj); break;
                    case 'A': i = SANA((OBJ_DATA *) vict_obj); break;
                    case 'h': i = PERS(to, ch->specials.subduing); break;
                    case 'H': i = PERS(to, vict_ch->specials.subduing); break;
                    case 'r': i = PERS(to, ch->specials.riding); break;
                    case 'R': i = PERS(to, vict_ch->specials.riding); break;
                    case 't': i = (char *) obj; break;
                    case 'T': i = (char *) vict_obj; break;
                    case 'F': i = first_name((char *) vict_obj, real_name_buf); break;
                    case '1': i = (to->desc && to->desc->term) ? i = VT_NORMALT : ""; break;
                    case '2': i = (to->desc && to->desc->term) ? i = VT_BOLDTEX : ""; break;
                    case '3': i = (to->desc && to->desc->term) ? i = VT_INVERTT : ""; break;
                    case 'b': i = "\a"; break;
                    case '$': i = "$"; break;
                    case 'c': i = ""; newline = 0; break;
                    case 'q': i = ""; cap = 0; break;
                    case 'D': i = "$D"; break;
                    default:
                        gamelog("Illegal $-code to act():");
                        gamelog(str);
                        break;
                    }
                    while (i && (*point = *(i++)))
                        ++point;
                    ++strp;
                } else if (!(*(point++) = *(strp++)))
                    break;

            if (newline) {
                *(--point) = '\n';
                *(++point) = '\r';
                *(++point) = '\0';
            } else
                *(--point) = '\0';

            if (type == TO_NEARBY_ROOMS) {
                send_to_nearby_rooms(buf, to->in_room);
                return;
            }
            if ((type >= TO_WAGON) && (type <= TO_PILOT)) {
                handler_for_wagon_sends(to->in_room, buf, type);
                return;
            }

            if (to->desc
                && (type != TO_NOTVICT_COMBAT
                    || (type == TO_NOTVICT_COMBAT && !IS_SET(to->specials.brief, BRIEF_COMBAT)))) {
                if (cap)
                    write_to_q(CAP(buf), &to->desc->output);
                else
                    write_to_q(buf, &to->desc->output);
            }
        }

        if ((type == TO_VICT) || (type == TO_CHAR))
            return;

        if (!doing_dir_watchers)
            to = to->next_in_room;

        /* reached the last person in the room, and has watching rooms */
        while (!to && wlist) {
            if (IS_SET(wlist->watch_type, WATCH_VIEW)) {
                /* check for any watching room */
                room = wlist->room_watching;
                if (room) {
                    to = room->people;
                } else {
                    gamelog("Error, room was bogus, but wlist was valid");
                }

                if ((strlen(in_str) + strlen(wlist->view_msg_string)) > MAX_STRING_LENGTH) {
                    errorlog("input to act() + wlist->view_msg_string was longer than MAX_STRING_LENGTH");
                    return;
                }
                strcpy(to_room_buf, wlist->view_msg_string);
                strcat(to_room_buf, in_str);
                str = to_room_buf;
            }

            wlist = wlist->next_watched_by;
        }

        if (!d || doing_dir_watchers)
            to = 0;

        while (!to && !wlist && d) {    /* Last person & watch rooms notified */
            doing_dir_watchers = 1;
            if ((d->character)
                /* blocks people sitting at the main menu from being used */
                && (d->character->in_room)
                && (is_char_watching_any_dir(d->character)
                    || is_char_watching_char(d->character, ch))) {
                if (is_char_watching_dir(d->character, DIR_OUT)
                    || is_char_watching_char(d->character, ch)) {
                    // only need to look at objects in ch's room
                    for (tmpob = room->contents; tmpob; tmpob = tmpob->next_content) {
                        if (tmpob->obj_flags.type == ITEM_WAGON
                         && (d->character->in_room->number == tmpob->obj_flags.value[2]
                          || d->character->in_room->number == tmpob->obj_flags.value[0])) {
                            sprintf(to_room_buf, "Outside %s: ", OSTR(tmpob, short_descr));
                            if ((strlen(in_str) + strlen(to_room_buf)) > MAX_STRING_LENGTH) {
                                errorlog("input to act() + to_room_buf was longer than MAX_STRING_LENGTH");
                                return;
                            }
                            strcat(to_room_buf, in_str);
                            to = d->character;
                            str = to_room_buf;
                        }
                    }
                }

                if (!is_char_watching_dir(d->character, DIR_OUT)) {
                    /* no point looking if there's no character to observe */
                    if (ch) {
                        /* set up the room to start from */
                        wroom = d->character->in_room;

                        /* loop over the directions if the person is watching the character, otherwise just the one iteration for current dir */
                        for (curr_dir = DIR_NORTH; curr_dir <= DIR_DOWN; curr_dir++) {
                            if (!is_char_watching_char(d->character, ch)
                                && !is_char_watching_dir(d->character, curr_dir))
                                continue;

                            curr_room = 0;
                            curr_room2 = 0;
                            curr_room3 = 0;

                            if (wroom->direction[curr_dir] && wroom->direction[curr_dir]->to_room
                                && !IS_SET(wroom->direction[curr_dir]->exit_info, EX_CLOSED)
                                && !IS_SET(wroom->direction[curr_dir]->exit_info, EX_SPL_SAND_WALL)
                                && !IS_SET(wroom->direction[curr_dir]->exit_info, EX_SPL_FIRE_WALL)
                                && !IS_SET(wroom->direction[curr_dir]->exit_info, EX_SPL_THORN_WALL)
                                && !IS_SET(wroom->direction[curr_dir]->exit_info,
                                           EX_SPL_BLADE_BARRIER)) {
                                curr_room = wroom->direction[curr_dir]->to_room;
                            }

                            /* 1 room away */
                            if (curr_room && curr_room == ch->in_room) {
                                if (curr_dir == DIR_UP)
                                    sprintf(to_room_buf, "Above you: ");
                                else if (curr_dir == DIR_DOWN)
                                    sprintf(to_room_buf, "Below you: ");
                                else
                                    sprintf(to_room_buf, "To the %s: ", dir_name[curr_dir]);

                                string_safe_cat(to_room_buf, in_str, MAX_STRING_LENGTH);
                                to = d->character;
                                str = to_room_buf;
                            } else if (curr_room) {
                                /* not found, move on to the second room */
                                if (curr_room->direction[curr_dir]
                                    && curr_room->direction[curr_dir]->to_room
                                    && !IS_SET(curr_room->direction[curr_dir]->exit_info, EX_CLOSED)
                                    && !IS_SET(curr_room->direction[curr_dir]->exit_info,
                                               EX_SPL_SAND_WALL)
                                    && !IS_SET(curr_room->direction[curr_dir]->exit_info,
                                               EX_SPL_FIRE_WALL)
                                    && !IS_SET(curr_room->direction[curr_dir]->exit_info,
                                               EX_SPL_THORN_WALL)
                                    && !IS_SET(curr_room->direction[curr_dir]->exit_info,
                                               EX_SPL_BLADE_BARRIER)) {
                                    curr_room2 = curr_room->direction[curr_dir]->to_room;
                                }

                                /* send a message if found */
                                if (curr_room2 && ch->in_room && (curr_room2 == ch->in_room)) { /* 2 room away */
                                    if (curr_dir == DIR_UP)
                                        sprintf(to_room_buf, "Far above you: ");
                                    else if (curr_dir == DIR_DOWN)
                                        sprintf(to_room_buf, "Far below you: ");
                                    else
                                        sprintf(to_room_buf, "Far to the %s: ", dir_name[curr_dir]);

                                    string_safe_cat(to_room_buf, in_str, MAX_STRING_LENGTH);
                                    to = d->character;
                                    str = to_room_buf;
                                }
                                /* not found, move on to the next room */
                                else if (curr_room2) {
                                    if (curr_room2->direction[curr_dir]
                                        && curr_room2->direction[curr_dir]->to_room
                                        && !IS_SET(curr_room2->direction[curr_dir]->exit_info,
                                                   EX_CLOSED)
                                        && !IS_SET(curr_room2->direction[curr_dir]->exit_info,
                                                   EX_SPL_SAND_WALL)
                                        && !IS_SET(curr_room2->direction[curr_dir]->exit_info,
                                                   EX_SPL_FIRE_WALL)
                                        && !IS_SET(curr_room2->direction[curr_dir]->exit_info,
                                                   EX_SPL_THORN_WALL)
                                        && !IS_SET(curr_room2->direction[curr_dir]->exit_info,
                                                   EX_SPL_BLADE_BARRIER)) {
                                        curr_room3 = curr_room2->direction[curr_dir]->to_room;
                                    }

                                    /* 3 rooms away */
                                    if (curr_room3 && ch->in_room && (curr_room3 == ch->in_room)) {
                                        if (curr_dir == DIR_UP)
                                            sprintf(to_room_buf, "Very far above you: ");
                                        else if (curr_dir == DIR_DOWN)
                                            sprintf(to_room_buf, "Very far below you: ");
                                        else
                                            sprintf(to_room_buf, "Very far to the %s: ",
                                                    dir_name[curr_dir]);

                                        string_safe_cat(to_room_buf, in_str, MAX_STRING_LENGTH);
                                        to = d->character;
                                        str = to_room_buf;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            d = d->next;
        }
    }
}


static int
get_host_address(char *name, struct in_addr *addr)
{
    union
    {
        long signed_thingy;
        unsigned long unsigned_thingy;
    }
    thingy;

    if (*name == '\0')
        gamelog("This cannot happen: No host address specified.");
    else if (isdigit(name[0])) {
        addr->s_addr = (unsigned long) inet_addr(name);
        thingy.unsigned_thingy = addr->s_addr;
        if (thingy.signed_thingy == -1)
            gamelog("Couldn't find host '%s'.");
    } else {
        struct hostent *the_host_entry_p;

        if (!(the_host_entry_p = gethostbyname(name)))
            gamelog("Couldn't find host '%s'.");
        bcopy(the_host_entry_p->h_addr, (char *) addr, sizeof(struct in_addr));
    }
    return 0;
}



