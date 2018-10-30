/*
 * File: CLAN_RELATIONS.C
 * Usage: Commands involving clans & their relations.
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "constants.h"
#include "structs.h"
#include "cities.h"
#include "clan.h"
#include "modify.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "db_file.h"
#include "skills.h"
#include "limits.h"
#include "guilds.h"
#include "clan.h"

void save_clan_data(int clan);


int
which_city_is_clan(CHAR_DATA * ch)
{
    int city;

    if (!ch)
        return CITY_NONE;

    if (IS_TRIBE(ch, TRIBE_ALLANAK_MIL)
        || IS_TRIBE(ch, 22)
        || IS_TRIBE(ch, 23)
        || IS_TRIBE(ch, 26)
        || IS_TRIBE(ch, 29)
        || IS_TRIBE(ch, 36)
        || IS_TRIBE(ch, 40)
        || IS_TRIBE(ch, 41)
        || IS_TRIBE(ch, 50)
        || IS_TRIBE(ch, 51)
        || IS_TRIBE(ch, 53))
        city = CITY_ALLANAK;
    else if (IS_TRIBE(ch, 1)
             || IS_TRIBE(ch, 8)
             || IS_TRIBE(ch, 15)
             || IS_TRIBE(ch, 47)
             || IS_TRIBE(ch, 52))
        city = CITY_TULUK;
    else if (IS_TRIBE(ch, 25))
        city = CITY_BW;
    else
        city = CITY_NONE;

    return city;
}

void
unboot_clans(void)
{
    int i;
    struct clan_rel_type *crl, *next_crl;

    for (i = 0; i < MAX_CLAN; i++) {
        free(clan_table[i].name);
        crl = clan_table[i].relations;
        while (crl) {
            next_crl = crl->next;
            free(crl);
            crl = next_crl;
        }
    }
    free(clan_table);
    clan_table = 0;
}

void
boot_clans(void) {

//   boot_clans_from_db();
   boot_clans_from_file();
}


void
boot_clans_from_file(void)
{
    FILE *cf;
    struct clan_rel_type *crl;
    char c[128];
    char buf[MAX_INPUT_LENGTH];
    int n1, n2, n3, n4;
    int sz_clan_table = 0;

    MAX_CLAN = MAX_CLAN_FROM_FILE;

    if (!(cf = fopen(CLAN_FILE, "r"))) {
        gamelog("Error opening clan file.");
        exit(0);
    }

    if (clan_table)
        RECREATE(clan_table, struct clan_type, MAX_CLAN);
    else
        CREATE(clan_table, struct clan_type, MAX_CLAN);

    while (sz_clan_table < MAX_CLAN && !feof(cf)) {

        sprintf(buf, "Reading clan %d of %d", sz_clan_table, MAX_CLAN - 1);
        shhlog(buf);

        sz_clan_table++;
        clan_table[sz_clan_table - 1].name = fread_string(cf);

#define CLEAN_NAME
        /*
         * Remove #ifdef stanzas if still working after 9/23/2005
         * (NOTE, there is another #ifdef below at save_clan0
         * -Tenebrius 6/23/2005
         */
#ifdef CLEAN_NAME
        while ((clan_table[sz_clan_table - 1].name[0] == '\n')
               || (clan_table[sz_clan_table - 1].name[0] == '\r'))
            memmove(&(clan_table[sz_clan_table - 1].name[0]),
                    &(clan_table[sz_clan_table - 1].name[1]),
                    strlen(clan_table[sz_clan_table - 1].name));
#endif

        sprintf(buf, "read clan '%s'", clan_table[sz_clan_table - 1].name);
        shhlog(buf);



        fscanf(cf, "%d %d %d %d", &n1, &n2, &n3, &n4);

        /*
         * sprintf(buf,"read the 4 numbers");
         * shhlog(buf);      
         */

        clan_table[sz_clan_table - 1].type = n1;
        clan_table[sz_clan_table - 1].status = n2;
        clan_table[sz_clan_table - 1].store = n3;
        clan_table[sz_clan_table - 1].liege = n4;

        clan_table[sz_clan_table - 1].relations = 0;


        fscanf(cf, "%s", c);
        while (c[0] == 'R') {
            fscanf(cf, "%d %d", &n1, &n2);
            CREATE(crl, struct clan_rel_type, 1);
            crl->nr = n1;
            crl->rel_val = n2;
            crl->next = clan_table[sz_clan_table - 1].relations;
            clan_table[sz_clan_table - 1].relations = crl;
            fscanf(cf, "%s", c);
        }
    }
    fclose(cf);
}

void
reboot_clans(void)
{
    int i;
    struct clan_rel_type *clr;

    for (i = 0; i < MAX_CLAN; i++) {
        if (clan_table[i].name)
            free(clan_table[i].name);
        clr = clan_table[i].relations;
        while (clan_table[i].relations) {
            clan_table[i].relations = clr->next;
            clr->next = 0;
            free((char *) clr);
            clr = clan_table[i].relations;
        }
    }
    free((char *) clan_table);
    clan_table = 0;
    unboot_clan_rank_names();
    boot_clans();
    boot_clan_rank_names();
}

/* get clan relation type */
int
get_clan_rel(int clan1, int clan2)
{
    struct clan_rel_type *clr;

    if ((clan1 < 0) || (clan1 >= MAX_CLAN) || (clan2 < 0) || (clan2 >= MAX_CLAN))
        return 0;

    for (clr = clan_table[clan1].relations; clr; clr = clr->next)
        if (clr->nr == clan2)
            break;
    if (!clr)
        return 0;

    return (clr->rel_val);
}

int
is_greater_vassal(int clan1, int clan2, int city_tribe)
{
    if ((clan1 < 0) || (clan1 >= MAX_CLAN))     /* no, its not a vassal at all */
        return FALSE;
    if ((clan2 < 0) || (clan2 >= MAX_CLAN))     /* yes, it must be */
        return TRUE;

    /* Premature return added until I switch this over to integer 
     * -nessalin 1/12/97 */
    return TRUE;
}


void
zorch_clan_rel(int clan1, int clan2)
{
    struct clan_rel_type *c, *Zc;

    Zc = (struct clan_rel_type *) 0;
    if ((clan1 < 0) || (clan1 >= MAX_CLAN) || (clan2 < 0) || (clan2 >= MAX_CLAN))
        return;

    for (c = clan_table[clan1].relations; c; Zc = c, c = c->next)
        if (c->nr == clan2)
            break;
    if (!c)
        return;
    if (c == clan_table[clan1].relations) {
        clan_table[clan1].relations = c->next;
        c->next = 0;
        free((char *) c);
    } else {
        Zc->next = c->next;
        c->next = 0;
        free((char *) c);
    }
}

void
drop_clan_status(int clan, int amount)
{
    if ((clan < 0) || (clan >= MAX_CLAN))
        return;

    clan_table[clan].status = MAX(0, clan_table[clan].status - amount);
    save_clan_data(clan);
}

/* test for clan relations */
int
is_rel_to_clan(int clan1, int clan2, int type)
{
    if ((clan1 < 0) || (clan1 >= MAX_CLAN) || (clan2 < 0) || (clan2 >= MAX_CLAN))
        return 0;

    return (get_clan_rel(clan1, clan2) == type);
}

int
is_vassal_clan(int clan1, int clan2)
{
    if ((clan1 < 0) || (clan1 >= MAX_CLAN))
        return 0;

    return (clan_table[clan1].liege == clan2);
}



void
cmd_clanset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int i, j, clan, spaces, loc;
    char buf1[256], buf2[256], buf3[256], buf4[256], buf5[256];
    char tmp1[256], tmp2[256];
    struct clan_rel_type *clr;
    struct room_data *Troom;

    const char * const field[] = {
        "relationship",
        "status",
        "storehouse",
        "numranks",
        "ranks",
        "\n"
    };

    char *desc[] = {
        "(set relationships to other clans)",
        "(set status level)",
        "(set room number of clan's warehouse)",
        "(set the number of ranks in the clan)",
        "(set a name for a rank in the clan)",
        "\n"
    };

    SWITCH_BLOCK(ch);

    argument = one_argument(argument, buf1, sizeof(buf1));      /* clan name */
    clan = atoi(buf1);
    if ((clan < 0) || (clan >= MAX_CLAN)) {
        send_to_char("That clan number is out of range.\n\r", ch);
        return;
    }

    argument = one_argument(argument, buf2, sizeof(buf2));      /* field */
    if (buf2[0] == '\0') {
        send_to_char("Syntax: clanset <tribe> <option> <argument>\n\r", ch);

        for (i = 0; *field[i] != '\n'; i++) {
            spaces = 15 - strlen(field[i]);
            strcpy(tmp2, "");
            /*            sprintf(tmp2, ""); */
            for (j = 0; j < spaces; j++)
                strcat(tmp2, " ");
            sprintf(tmp1, "%s%s%s\n\r", field[i], tmp2, desc[i]);
            send_to_char(tmp1, ch);
        }
        send_to_char("Note: to see the ranks in a clan type:\n\r"
                     "   clanset <tribenumber> rank show\n\r"
                     "   example: clanset 12 rank show\n\r", ch);
        return;
    }

    loc = (old_search_block(buf2, 0, strlen(buf2), field, 0));
    if (loc == -1) {
        send_to_char("That field does not exist.\n\r", ch);
        return;
    }
    if (loc != 4 && loc != 5) {
        send_to_char("You may only set numranks or rank.\n\r", ch);
        return;
    }

    switch (loc) {
    case 1:                    /* set relationship */
        argument = one_argument(argument, buf3, sizeof(buf3));  /* set or remove */
        switch (buf3[0]) {
        case 's':              /* set */
            argument = one_argument(argument, buf4, sizeof(buf4));      /* type of ally */
            if (!strnicmp(buf4, "friend", 1)) {
                argument = one_argument(argument, buf5, sizeof(buf5));  /* friendly clan */
                i = atoi(buf5);
                if ((i < 0) || (i >= MAX_CLAN)) {
                    send_to_char("That clan does not exist.\n\r", ch);
                    return;
                }
                if (get_clan_rel(clan, i)) {
                    send_to_char("Those clans already have a relationship." "\n\r", ch);
                    return;
                }
                CREATE(clr, struct clan_rel_type, 1);
                clr->nr = i;
                clr->rel_val = 0;
                clr->next = clan_table[clan].relations;
                clan_table[clan].relations = clr;
                send_to_char("Done.\n\r", ch);
            } else if (!strnicmp(buf4, "vassal", 1)) {
                argument = one_argument(argument, buf5, sizeof(buf5));  /* servant clan */
                i = atoi(buf5);
                if ((i < 0) || (i >= MAX_CLAN)) {
                    send_to_char("That clan does not exist.\n\r", ch);
                    return;
                }
                if (get_clan_rel(clan, i)) {
                    send_to_char("Those clans already have a relationship." "\n\r", ch);
                    return;
                }
                if (clan_table[i].liege) {
                    sprintf(tmp1, "%s already has a liege.\n\r", clan_table[i].name);
                    send_to_char(tmp1, ch);
                    return;
                }
                CREATE(clr, struct clan_rel_type, 1);
                clr->nr = i;
                clr->rel_val = 0;
                clr->next = clan_table[clan].relations;
                clan_table[clan].relations = clr;
                clan_table[i].liege = clan;
                sprintf(tmp1, "%s is now a vassal of %s.\n\r", clan_table[i].name,
                        clan_table[clan].name);
                send_to_char(tmp1, ch);
                return;
            } else
                send_to_char("syntax: clanset clan ally set friend "
                             "friend_clan\n\rsyntax: clanset "
                             "num_of_liege ally set vassal num_of_vassal" "\n\r", ch);
            break;
        case 'r':              /* remove */
            argument = one_argument(argument, buf4, sizeof(buf4));      /* dumped clan */
            i = atoi(buf4);
            if ((i < 0) || (i >= MAX_CLAN)) {
                send_to_char("That clan does not exist.\n\r", ch);
                return;
            }
            if (get_clan_rel(clan, i)) {
                zorch_clan_rel(clan, i);
                sprintf(tmp1, "%s no longer has a relationship with %s.\n\r", clan_table[clan].name,
                        clan_table[i].name);
                send_to_char(tmp1, ch);
                return;
            } else
                send_to_char("Those clans do not have a relationship.\n\r", ch);
            break;
        default:
            send_to_char("You may (s)et or (r)emove allies.\n\r", ch);
            break;
        }
        break;
    case 2:                    /* status */
        argument = one_argument(argument, buf3, sizeof(buf3));
        if (buf3[0] == '\0') {
            send_to_char("Supply a number, 0 through 1000, representing "
                         "the general honor and/or\n\r popularity of the clan."
                         "\n\rAverage = 50, High = 200, Fanatical = 500\n\r", ch);
            return;
        }
        i = atoi(buf3);
        if ((i < 0) || (i > 1000)) {
            send_to_char("The status must be between 0 and 1000.\n\r", ch);
            return;
        }
        clan_table[clan].status = i;
        sprintf(tmp1, "%s's status is now %d.\n\r", clan_table[clan].name, i);
        send_to_char(tmp1, ch);
        break;
    case 3:                    /* storehouse */
        argument = one_argument(argument, buf3, sizeof(buf3));
        if (buf3[0] == '\0') {
            send_to_char("Supply a room number.\n\r", ch);
            return;
        }
        i = atoi(buf3);
        Troom = get_room_num(i);
        if (!Troom) {
            send_to_char("Please supply the number of an existant room.\n\r", ch);
            return;
        }
        clan_table[clan].store = i;
        sprintf(tmp1, "%s's storehouse is now room %d.\n\r", clan_table[clan].name, i);
        send_to_char(tmp1, ch);
        break;
    case 4:                    /* numranks */
        argument = one_argument(argument, buf3, sizeof(buf3));

        if (buf3[0] == '\0') {
            send_to_char("You must supply a number of ranks.\n\r", ch);
            return;
        }

        i = atoi(buf3);

        if (i < MIN_CLAN_RANK || i > MAX_CLAN_RANK) {
            sprintf(tmp1, "Numranks is in the range %d to %d.\n\r", MIN_CLAN_RANK, MAX_CLAN_RANK);
            send_to_char(tmp1, ch);
            return;
        }

        if (i == clan_ranks[clan].num_ranks) {
            send_to_char("Already that many ranks, no change made.\n\r", ch);
            return;
        }

        set_num_clan_ranks(clan, i);
        save_clan_rank_names(clan);

        sprintf(tmp1, "Set %s number of ranks to %d.\n\r", clan_table[clan].name, i);
        send_to_char(tmp1, ch);
        break;
    case 5:                    /* rank name */
        argument = one_argument(argument, buf3, sizeof(buf3));

        if (buf3[0] == '\0') {
            send_to_char("You must specify which rank to set.\n\r", ch);
            return;
        }

        if (is_number(buf3)) {
            i = atoi(buf3);

            if (i < MIN_CLAN_RANK || i > clan_ranks[clan].num_ranks) {
                sprintf(tmp1, "Rank # is in the range %d to %d.\n\r", MIN_CLAN_RANK,
                        clan_ranks[clan].num_ranks);
                send_to_char(tmp1, ch);
                return;
            }

            set_clan_rank_name(clan, i, argument);
            save_clan_rank_names(clan);

            sprintf(tmp1, "Set %s rank %d to %s.\n\r", clan_table[clan].name, i, argument);
            send_to_char(tmp1, ch);
        } else if (!strcmp(buf3, "show")) {
            sprintf(tmp1, "%s has the following ranks defined:\n\r", clan_table[clan].name);
            send_to_char(tmp1, ch);

            send_clan_ranks_to_char(clan, ch);
        } else {
            send_to_char("Syntax: clanset <clan #> rank <rank #> <name>\n\r"
                         "        clanset <clan #> ranks show\n\r", ch);
        }
        break;
    default:
        gamelog("Unknown state in clanset().");
        break;
    }
}


void
save_clan_data_to_file()
{
    FILE *cf;
    int i;
    struct clan_rel_type *clr;

    if (!(cf = fopen(CLAN_FILE, "w"))) {
        gamelog("Error: unable to save clan file.");
        return;
    }

    for (i = 0; i < MAX_CLAN; i++) {
        fprintf(cf, "%s~\n", clan_table[i].name);

        fprintf(cf, "%d %d %d %d\n", clan_table[i].type, clan_table[i].status, clan_table[i].store,
                clan_table[i].liege);
        for (clr = clan_table[i].relations; clr; clr = clr->next)
            fprintf(cf, "R %d %d\n", clr->nr, clr->rel_val);

#ifdef CLEAN_NAME
        fprintf(cf, "~\n");
#else
        fprintf(cf, "~");
#endif
    }
    fclose(cf);
}


void
save_clan_data( int clan ) {
   save_clan_data_to_file();
}


void
cmd_clansave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int clan;
    char arg[MAX_INPUT_LENGTH];

    SWITCH_BLOCK(ch);

    one_argument(argument, arg, sizeof(arg));

    // if it's empty and not 'all'
    if( arg[0] != '\0' && str_prefix( arg, "all" ) ) {
        clan = lookup_clan_by_name(arg);
        if( clan >= MAX_CLAN || clan < 0 ) {
           cprintf( ch, 
            "No such clan '%s', see 'show c' for a list of clans\n\r", arg);
           return;
        }
        cprintf(ch, "Saving clan '%s'.\n\r", clan_table[clan].name);
    } 
    // all clans
    else {
        clan = -1;
        cprintf(ch, "Saving all clans.\n\r");
    }
    save_clan_data( clan );
    return;
}

