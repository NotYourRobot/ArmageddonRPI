/*
 * File: CLANS.C
 * Usage: Multiple Clan utility commands.
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "constants.h"
#include "structs.h"
#include "clan.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "limits.h"
#include "guilds.h"
#include "creation.h"
#include "modify.h"
#include "db_file.h"
#include "info.h"

/* Utilities */
unsigned int MAX_CLAN = 0;
extern char *clan_name[];

/* lookup a clan by name */
int
lookup_clan_by_name( const char *name ) {
    int c;
    for (c = 0; c < MAX_CLAN; c++)

        if (!str_prefix(name, clan_name[c]))
            break;
    if( c == MAX_CLAN ) return -1;
    return c;
}

/* is_clan_member
 * returns boolean if person is a member of a clan
 */
bool
is_clan_member(CLAN_DATA * clan, int num)
{
    for (; clan; clan = clan->next)
        if (clan->clan == num)
            return TRUE;

    return FALSE;
}



CLAN_DATA *
new_clan_data()
{
    CLAN_DATA *pClan;

    CREATE(pClan, CLAN_DATA, 1);
    pClan->next = NULL;
    pClan->clan = 0;
    pClan->flags = 0;
    pClan->rank = 0;
    return (pClan);
}

void
add_clan(CHAR_DATA * ch, int clan)
{
    CLAN_DATA *pClan, *tmp;

    if (!ch)
        return;

    pClan = new_clan_data();
    pClan->clan = clan;
    pClan->rank = 1;
    pClan->flags = 0;

    /* put the new clan at the tail of the list */
    if (!ch->clan) {
        pClan->next = ch->clan;
        ch->clan = pClan;
    } else {
        for (tmp = ch->clan; tmp->next; tmp = tmp->next);

        tmp->next = pClan;
        pClan->next = NULL;
    }
    return;
}

void
add_clan_data(CHAR_DATA * ch, CLAN_DATA * pClan)
{
    CLAN_DATA *tmp;

    if (!ch || !pClan)
        return;

    /* put the new clan at the tail of the list */
    if (!ch->clan) {
        pClan->next = ch->clan;
        ch->clan = pClan;
    } else {
        for (tmp = ch->clan; tmp->next; tmp = tmp->next);

        tmp->next = pClan;
        pClan->next = NULL;
    }
    return;
}

void
free_clan_data(CLAN_DATA * pClan)
{
    if (!pClan)
        return;

    free(pClan);
    return;
}


void
remove_clan(CHAR_DATA * ch, int clan)
{
    CLAN_DATA *pClan;

    if (!ch->clan)
        return;

    if (ch->clan->clan == clan) {
        pClan = ch->clan;
        ch->clan = ch->clan->next;
        free_clan_data(pClan);
        return;
    }

    for (pClan = ch->clan; pClan; pClan = pClan->next) {
        if (pClan->next->clan == clan) {
            CLAN_DATA *pClan2 = pClan->next;

            pClan->next = pClan->next->next;
            free_clan_data(pClan2);
            return;
        }
    }
}



/* set_clan_flag
 * sets the clan_data flag
 */
void
set_clan_flag(CLAN_DATA * clan, int num, int flag)
{
    for (; clan; clan = clan->next)
        if (clan->clan == num) {
            MUD_SET_BIT(clan->flags, flag);
            return;
        }
}


/* remove_clan_flag
 * removes the clan_data flag
 */
void
remove_clan_flag(CLAN_DATA * clan, int num, int flag)
{
    for (; clan; clan = clan->next)
        if (clan->clan == num) {
            REMOVE_BIT(clan->flags, flag);
            return;
        }
}


/* is_set_clan_flag
 * checks the clan_data flag
 */
bool
is_set_clan_flag(CLAN_DATA * clan, int num, int flag)
{
    for (; clan; clan = clan->next)
        if (clan->clan == num) {
            return IS_SET(clan->flags, flag);
        }

    return FALSE;
}


void
set_clan_rank(CLAN_DATA * clan, int num, int rank)
{
    for (; clan; clan = clan->next)
        if (clan->clan == num) {
            clan->rank = rank;
            return;
        }

    return;
}


int
get_clan_rank(CLAN_DATA * clan, int num)
{
    for (; clan; clan = clan->next)
        if (clan->clan == num)
            return clan->rank;

    return 0;
}


/* 
 * clan_rank_cmp
 * works like strcmp, 
 * returns -1 if ch is less than ch2 in rank in clan
 * returns 1 if ch is greater than ch2 in rank in clan
 * returns 0 if ch is the same rank as ch2 in clan
 */
int
clan_rank_cmp(CHAR_DATA * ch, CHAR_DATA * ch2, int clan)
{
    int rank, rank2;

    rank = get_clan_rank(ch->clan, clan);
    rank2 = get_clan_rank(ch2->clan, clan);

    /* ch in tribe, ch2 not */
    if (rank > rank2)
        return 1;

    if (rank < rank2)
        return -1;

    if (rank == rank2)
        return 0;

    return 0;
}

/*
 * set_number_of_clan_ranks
 * sets the number of clan ranks in a clan
 * reallocs and 
 */
bool
set_num_clan_ranks(int clan, int num)
{
    int i, old_num;
    char **newranks;

    /* stay within the limits */
    if (num > MAX_CLAN_RANK || num < MIN_CLAN_RANK)
        return FALSE;

    /* don't need to do anything if no change */
    if (num == clan_ranks[clan].num_ranks)
        return TRUE;

    /* save old number so we can go through and grab them */
    old_num = clan_ranks[clan].num_ranks;

    /* set the new number */
    clan_ranks[clan].num_ranks = num;

    CREATE(newranks, char *, num);

    /* initialize them all to null */
    for (i = 0; i < clan_ranks[clan].num_ranks; i++)
        newranks[i] = NULL;

    /* copy memory location of old ranks */
    for (i = 0; i < old_num; i++) {
        if (i >= clan_ranks[clan].num_ranks) {
            if (clan_ranks[clan].rank[i]) {
                free(clan_ranks[clan].rank[i]);
                clan_ranks[clan].rank[i] = NULL;
            }
        } else {
            newranks[i] = clan_ranks[clan].rank[i];
        }
    }

    if (clan_ranks[clan].rank)
        free(clan_ranks[clan].rank);

    clan_ranks[clan].rank = newranks;

    return TRUE;
}


/*
 * set_clan_rank_name
 * sets a clan's rank name
 * uses strdup on the given name
 */
bool
set_clan_rank_name(int clan, int num, const char *name)
{
    int i;

    /* decrement number to get it to array range */
    num--;

    /* stay within defined bounds */
    if (num < 0 || num >= clan_ranks[clan].num_ranks)
        return FALSE;

    /* if no memory is initialized, go ahead and allocate it */
    if (!clan_ranks[clan].rank) {
        CREATE(clan_ranks[clan].rank, char *, num);

        /* initialize them all to null */
        for (i = 0; i < clan_ranks[clan].num_ranks; i++)
            clan_ranks[clan].rank[i] = NULL;
    }

    /* free existing memory if needed */
    if (clan_ranks[clan].rank[num])
        free(clan_ranks[clan].rank[num]);

    /* store the new name */
    clan_ranks[clan].rank[num] = strdup(name);

    return TRUE;
}


/*
 * get_clan_rank_name
 * gets the name for a rank within a clan
 */
char *
get_clan_rank_name(int clan, int num)
{
    num--;

    if (clan < 0 || clan >= MAX_CLAN)
        return NULL;

    if (num < 0 || num >= clan_ranks[clan].num_ranks)
        return NULL;

    if (!clan_ranks[clan].rank)
        return NULL;

    return (clan_ranks[clan].rank[num]);
}


/* get_clan_rank_num
 * gets the number for a rank within a clan given the name
 */
int
get_clan_rank_num(int clan, char *name)
{
    int i;

    for (i = 0; i < clan_ranks[clan].num_ranks; i++)
        if (!strcasecmp(smash_article(clan_ranks[clan].rank[i]), name)
            || !strcasecmp(clan_ranks[clan].rank[i], name))
            return i + 1;

    return 0;
}


/* send_clan_ranks_to_char
 * send a name of clan ranks for a clan to the person
 */
void
send_clan_ranks_to_char(int clan, CHAR_DATA * ch)
{
    int i;
    char tmp[256], buf[4096];

    if (clan < 0 || clan >= MAX_CLAN) {
        /*       sprintf( buf, "clan '%s' out of range\n\r", clan ); */
        sprintf(buf, "clan '%d' out of range\n\r", clan);
        send_to_char(buf, ch);
        return;
    }

    strcpy(buf, "------------------------------------------------------------\n\r");

    for (i = 0; i < clan_ranks[clan].num_ranks; i++) {
        sprintf(tmp, "%s\n\r", clan_ranks[clan].rank[i]);
        strcat(buf, tmp);
    }

    send_to_char(buf, ch);
}


/* save_clan_rank_names
 * saves the rank names as set by the staff
 */
void
save_clan_rank_names_to_file(int clan)
{
    int j;
    FILE *fp;
    char filename[MAX_STRING_LENGTH];

    /* try and open the clan rank file */
    sprintf(filename, CLAN_RANK_FILE, clan);
    if ((fp = fopen(filename, "w")) == NULL) {
        clan_ranks[clan].rank = NULL;
        clan_ranks[clan].num_ranks = 0;
        return;
    }

    /* make sure they didn't end up with less than 0 ranks */
    clan_ranks[clan].num_ranks = MAX(0, clan_ranks[clan].num_ranks);

    /* write out the number of ranks */
    fprintf(fp, "%d\n", clan_ranks[clan].num_ranks);

    /* if there's no ranks defined, or num_ranks == 0 then move on */
    if (!clan_ranks[clan].rank || clan_ranks[clan].num_ranks == 0) {
        fclose(fp);
        return;
    }

    /* write out the ranks */
    for (j = 0; j < clan_ranks[clan].num_ranks; j++)
        if (clan_ranks[clan].rank[j])
            fprintf(fp, "%s~\n", clan_ranks[clan].rank[j]);
        else
            fprintf(fp, "(null)~");

    /* close this file out */
    fclose(fp);

    return;
}

void save_clan_rank_names( int clan ) {
    save_clan_rank_names_to_file(clan);
}


/*
 * unboot_clan_rank_names
 * go through clan_rank structure and deallocated associated memory
 */
void
unboot_clan_rank_names()
{
    int i, counter;

    for (i = 0; i < MAX_CLAN; i++) {
        if (clan_ranks[i].rank) {
            for (counter = 0; counter < clan_ranks[i].num_ranks; counter++)
                if (clan_ranks[i].rank[counter])
                    free(clan_ranks[i].rank[counter]);

            free(clan_ranks[i].rank);
            clan_ranks[i].rank = NULL;
        }

        clan_ranks[i].num_ranks = 0;
    }
    free(clan_ranks);
    clan_ranks = 0;
    return;
}



/* boot_clan_rank_names
 * loads the rank names and allocates memory as needed
 */
void
boot_clan_rank_names_from_file()
{
    int i, j;
    FILE *fp;
    char filename[MAX_STRING_LENGTH];

    if (clan_ranks)
        RECREATE(clan_ranks, struct clan_rank_struct, MAX_CLAN);
    else
        CREATE(clan_ranks, struct clan_rank_struct, MAX_CLAN);

    for (i = 0; i < MAX_CLAN; i++) {
        /* try and open the clan rank file */
        sprintf(filename, CLAN_RANK_FILE, i);
        if ((fp = fopen(filename, "r")) == NULL) {
            clan_ranks[i].num_ranks = 1;
            CREATE(clan_ranks[i].rank, char *, clan_ranks[i].num_ranks);
            clan_ranks[i].rank[0] = strdup("a member");
            continue;
        }

        /* get the number of ranks */
        clan_ranks[i].num_ranks = fread_number(fp);

        /* default to 1, set default name to member */
        if (clan_ranks[i].num_ranks == 0) {
            clan_ranks[i].num_ranks = 1;
            CREATE(clan_ranks[i].rank, char *, clan_ranks[i].num_ranks);
            clan_ranks[i].rank[0] = strdup("a member");
            fclose(fp);
            continue;
        }

        /* allocate the memory needed for that many ranks */
        CREATE(clan_ranks[i].rank, char *, clan_ranks[i].num_ranks);
        for (j = 0; j < clan_ranks[i].num_ranks && !feof(fp); j++) {
            clan_ranks[i].rank[j] = fread_string_no_whitespace(fp);

            if (!strcmp(clan_ranks[i].rank[j], "(null)")) {
                free(clan_ranks[i].rank[j]);
                clan_ranks[i].rank[j] = NULL;
            }
        }

        /* if found end of file before we got all the strings,
         * initialize the rest to NULL
         */
        for (; j < clan_ranks[i].num_ranks; j++)
            clan_ranks[i].rank[j] = NULL;

        /* close this file out */
        fclose(fp);
    }

    return;
}

void
boot_clan_rank_names() {
    boot_clan_rank_names_from_file();
}

/*
 * write_clan_bank_account
 * write out the bank account for a single clan.
 */
void
write_clan_bank_account_to_file(int clan)
{
    FILE *fp;
    char filename[MAX_STRING_LENGTH];

    /* try and open the clan rank file */
    sprintf(filename, CLAN_BANK_FILE, clan);
    if ((fp = fopen(filename, "w")) == NULL) {
        sprintf(filename, "Unable to open '%s' for writing", filename);
        gamelog(filename);
        return;
    }

    /* make sure they didn't end up with less than 0 sids */
    clan_table[clan].account = MAX(0, clan_table[clan].account);

    /* write out the number of sids */
    fprintf(fp, "%ld\n", clan_table[clan].account);

    /* close this file out */
    fclose(fp);
}

void
write_clan_bank_account(int clan) {
    write_clan_bank_account_to_file(clan);
}

/* save_clan_bank_account
 * saves the bank accounts of the clans
 */
void
save_clan_bank_accounts(int clan)
{
    int i;

    if (clan > 0 || clan < MAX_CLAN) {
        write_clan_bank_account(clan);
    } else {
        for (i = 0; i < MAX_CLAN; i++) {
            write_clan_bank_account(i);
        }
    }

    return;
}

/* boot_clan_bank_accounts
 * loads the bank accounts for clans
 */
void
boot_clan_bank_accounts()
{
    int i;
    FILE *fp;
    char filename[MAX_STRING_LENGTH];

    for (i = 0; i < MAX_CLAN; i++) {
        /* try and open the clan bank file */
        sprintf(filename, CLAN_BANK_FILE, i);
        if ((fp = fopen(filename, "r")) == NULL) {
            clan_table[i].account = 0;
            continue;
        }

        /* get the bank account balance */
        clan_table[i].account = fread_number(fp);
        fclose(fp);
    }
}

/* in_same_tribe
 * if both are in at least one tribe together, return it
 */
long
in_same_tribe(CHAR_DATA * ch, CHAR_DATA * victim)
{
    CLAN_DATA *cclan, *vclan;

    if (!ch | !victim)
        return FALSE;

    for (cclan = ch->clan; cclan; cclan = cclan->next)
        for (vclan = victim->clan; vclan; vclan = vclan->next)
            if (cclan->clan == vclan->clan)
                return cclan->clan;

    return FALSE;
}




/* cmd_ functions */

/* cmd_recruit
 * recruit a person into the clan
 */
void
cmd_recruit(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vch;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char cname[MAX_STRING_LENGTH];
    CLAN_DATA *pClan;
    int clan = 0;

    //    if (IS_NPC(ch))
    //        return;

    argument = one_argument(argument, arg1, sizeof(arg1));
    strcpy(arg2, argument);     /* copy the rest into arg2 */

    if (arg1[0] == '\0') {
        send_to_char("Syntax: recruit <person> [clan]\n\r", ch);
        return;
    }

    if ((vch = get_char_room_vis(ch, arg1)) == NULL) {
        send_to_char("You don't see that person here.\n\r", ch);
        return;
    }

    if (arg2[0] != '\0') {
        clan = lookup_clan_by_name(arg2);

        if (clan == -1) {
            send_to_char("No such clan.\n\r", ch);
            return;
        }
    } else {
        for (pClan = ch->clan; pClan; pClan = pClan->next) {
            if (pClan->clan && (IS_SET(pClan->flags, CJOB_RECRUITER)
                                || IS_SET(pClan->flags, CJOB_LEADER))) {
                clan = pClan->clan;
                break;
            }
        }

        if (!clan) {
            send_to_char("You can't recuit people for any clan!\n\r", ch);
            return;
        }
    }

    if (!IS_TRIBE(ch, clan) && !IS_IMMORTAL(ch)) {
        send_to_char("You're not even a member of that clan!\n\r", ch);
        return;
    }

    /* Need some sort of leader flag/check here */
    if (!is_set_clan_flag(ch->clan, clan, CJOB_RECRUITER)
        && !is_set_clan_flag(ch->clan, clan, CJOB_LEADER)
        && !IS_IMMORTAL(ch)) {
        send_to_char("You're not a recruiter for that clan.\n\r", ch);
        return;
    }

    if (IS_TRIBE(vch, clan)) {
        act("$N is already a member of $t.", FALSE, ch, clan_table[clan].name, vch, TO_CHAR);
        return;
    }

    /* if this is their first tribe, set the tribe variable so old system will
     * still work */
    if (!vch->clan)
        vch->player.tribe = clan;

    /* set them up as invited into the clan */
    add_clan(vch, clan);

    /* get the cname */
    if (clan > 0 && clan < MAX_CLAN)
        strcpy(cname, clan_table[clan].name);
    else
        sprintf(cname, "%d", clan);

    /* and now a quick little message */
    sprintf(buf, "You initiate $N into '%s'.", cname);
    act(buf, FALSE, ch, NULL, vch, TO_CHAR);
    sprintf(buf, "$n initiates you into '%s'.", cname);
    act(buf, FALSE, ch, NULL, vch, TO_VICT);

    sprintf(buf, "If you don't want to be a member, type 'REBEL %s' to quit." "\n\r", cname);

    return;
}



void
cmd_rebel(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[MAX_STRING_LENGTH];
    char cname[MAX_STRING_LENGTH];
    int clan = 0;

    for (; *argument == ' '; argument++);

    strcpy(arg, argument);
    if (arg[0] != '\0') {
        for (clan = 0; clan < MAX_CLAN; clan++)
            /* does the string match and are they a member of that tribe? */
            if (!str_prefix(arg, clan_table[clan].name) && IS_TRIBE(ch, clan))
                break;

        if (clan == MAX_CLAN) {
            send_to_char("You aren't a member of any such tribe.\n\r", ch);
            return;
        }
    } else {
        if (ch->clan)
            clan = ch->clan->clan;

        if (!clan) {
            send_to_char("You aren't a member of any clan.\n\r", ch);
            return;
        }
    }

    if (is_set_clan_flag(ch->clan, clan, CJOB_LEADER)) {
        send_to_char("You can't rebel, you're the LEADER.\n\r", ch);
        return;
    }

    remove_clan(ch, clan);

    /* if it's their first clan, remove their tribe variable */
    if (clan == ch->player.tribe) {
        if (ch->clan)
            ch->player.tribe = ch->clan->clan;
        else
            ch->player.tribe = 0;
    }

    if (clan > 0 && clan < MAX_CLAN)
        strcpy(cname, clan_table[clan].name);
    else
        sprintf(cname, "%d", clan);

    sprintf(arg, "You rebel from %s.\n\r", cname);
    send_to_char(arg, ch);
    return;
}



/*
 * cmd_dump
 * dump a person from a tribe
 */
void
cmd_dump(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vch;
    char cname[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CLAN_DATA *pClan;
    int clan = 0;

    //    if (IS_NPC(ch))
    //        return;

    argument = one_argument(argument, arg1, sizeof(arg1));
    strcpy(arg2, argument);     /* copy the rest into arg2 */

    if (arg1[0] == '\0') {
        send_to_char("Syntax: dump <person> [clan]\n\r", ch);
        return;
    }

    if ((vch = get_char_room_vis(ch, arg1)) == NULL) {
        send_to_char("Noone like that around to dump.\n\r", ch);
        return;
    }

    if (vch == ch) {
        send_to_char("You can't dump yourself!\n\r", ch);
        return;
    }

    if (arg2[0] != '\0') {
        clan = lookup_clan_by_name( arg2 );

        if (clan == -1) {
            send_to_char("No such clan.\n\r", ch);
            return;
        }
    } else {
        for (pClan = ch->clan; pClan; pClan = pClan->next) {
            CLAN_DATA *vClan;

            for (vClan = vch->clan; vClan; vClan = vClan->next) {
                if (pClan->clan == vClan->clan && (IS_SET(pClan->flags, CJOB_RECRUITER)
                                                   || IS_SET(pClan->flags, CJOB_LEADER))) {
                    clan = pClan->clan;
                    break;
                }
            }
        }

        if (!clan) {
            send_to_char("What clan do you want to dump them from?\n\r", ch);
            return;
        }
    }

    if (!IS_TRIBE(ch, clan) && !IS_IMMORTAL(ch)) {
        send_to_char("You're not even a member of that clan!\n\r", ch);
        return;
    }

    /* are we a leader or recruiter? */
    if (!is_set_clan_flag(ch->clan, clan, CJOB_RECRUITER)
        && !is_set_clan_flag(ch->clan, clan, CJOB_LEADER)
        && !IS_IMMORTAL(ch)) {
        send_to_char("You're not a recruiter for that clan.\n\r", ch);
        return;
    }

    /* is vch in the clan? */
    if (!IS_TRIBE(vch, clan)) {
        act("$N isn't a member of $t.", FALSE, ch, clan_table[clan].name, vch, TO_CHAR);
        return;
    }

    /* check rank 
     * clan_rank_cmp returns 0 for equal, 1 for ch > vch, -1 for ch < vch
     * let immortals do it regardless
     */
    if (clan_rank_cmp(ch, vch, clan) < 0 && !IS_IMMORTAL(ch)) {
        act("You don't have the authority to dump $N.", FALSE, ch, NULL, vch, TO_CHAR);
        return;
    }

    /* kick them out */
    remove_clan(vch, clan);

    /* if it's their first clan, remove their tribe variable */
    if (clan == vch->player.tribe) {
        if (vch->clan)
            vch->player.tribe = vch->clan->clan;
        else
            vch->player.tribe = 0;
    }

    if (clan > 0 && clan < MAX_CLAN)
        strcpy(cname, clan_table[clan].name);
    else
        sprintf(cname, "%d", clan);

    /* and now a quick little message */
    sprintf(buf, "You dump $N from '%s'.", cname);
    act(buf, FALSE, ch, NULL, vch, TO_CHAR);
    sprintf(buf, "$n dumps you from '%s'.", cname);
    act(buf, FALSE, ch, NULL, vch, TO_VICT);
    return;
}


/*
 * cmd_clanleader
 * Immortal command to set a player as a leader of a tribe
 */
void
cmd_clanleader(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vch;
    char cname[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int clan = 0;

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, arg1, sizeof(arg1));
    strcpy(arg2, argument);     /* copy the rest into arg2 */

    if (arg1[0] == '\0') {
        send_to_char("Syntax: clanleader <person> [clan]\n\r", ch);
        return;
    }

    if ((vch = get_char_room_vis(ch, arg1)) == NULL) {
        send_to_char("Noone like that around to make clanleader.\n\r", ch);
        return;
    }

    if (arg2[0] != '\0') {
        clan = lookup_clan_by_name( arg2 );

        if (clan == -1) {
            send_to_char("No such clan.\n\r", ch);
            return;
        }
    } else {
        CLAN_DATA *pClan;

        for (pClan = vch->clan; pClan; pClan = pClan->next)
            if (!IS_SET(vch->clan->flags, CJOB_LEADER))
                clan = pClan->clan;

        if (clan == 0) {
            send_to_char("Which clan do you want to make them a leader of?\n\r", ch);
            return;
        }
    }

    if (clan > 0 && clan < MAX_CLAN)
        strcpy(cname, clan_table[clan].name);
    else
        sprintf(cname, "%d", clan);

    if (!IS_TRIBE(vch, clan)) {
        act("$N isn't a member of $t.", FALSE, ch, cname, vch, TO_CHAR);
        return;
    }

    /* if they have it, drop it */
    if (is_set_clan_flag(vch->clan, clan, CJOB_LEADER)) {
        remove_clan_flag(vch->clan, clan, CJOB_LEADER);
        sprintf(buf, "You are no longer a clanleader of '%s'.\n\r", cname);
        send_to_char(buf, vch);

        sprintf(buf, "%s is no longer a clanleader of '%s'.\n\r", MSTR(vch, name), cname);
        send_to_char(buf, ch);

        /* and for legacy's sake... */
        if (clan == vch->player.tribe)
            REMOVE_BIT(vch->specials.act, CFL_CLANLEADER);
    } else {                    /* set it */

        set_clan_flag(vch->clan, clan, CJOB_LEADER);
        sprintf(buf, "You are now a clanleader of '%s'.\n\r", cname);
        send_to_char(buf, vch);

        sprintf(buf, "%s is now a clanleader of '%s'.\n\r", MSTR(vch, name), cname);
        send_to_char(buf, ch);

        /* and for legacy's sake... */
        if (clan == vch->player.tribe)
            MUD_SET_BIT(vch->specials.act, CFL_CLANLEADER);
    }

    return;
}


/* promote */
void
cmd_promote(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vch;
    char cname[MAX_STRING_LENGTH];
    char *rname;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    int clan = 0, flag, rank;
    CLAN_DATA *pClan;
    char *flags;

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    strcpy(arg3, argument);     /* copy the rest into arg3 */

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Syntax: promote <person> <job|rank> [clan]\n\r"
                     "        Rank is a name or number.\n\r" "        Job is one of:\n\r", ch);

        send_flags_to_char(clan_job_flags, ch);
        return;
    }

    if ((vch = get_char_room_vis(ch, arg1)) == NULL) {
        send_to_char("Noone like that around to promote.\n\r", ch);
        return;
    }

    if (arg3[0] == '\0') {
        for (pClan = vch->clan; pClan; pClan = pClan->next) {
            if (pClan->clan && (IS_TRIBE_LEADER(ch, pClan->clan)
                                || IS_IMMORTAL(ch))) {
                clan = pClan->clan;
                break;
            }
        }

        if (!clan) {
            send_to_char("You can't promote people for any clan!\n\r", ch);
            return;
        }
    } else {
        clan = lookup_clan_by_name( arg3 );

        if (clan == -1) {
            send_to_char("No such clan.\n\r", ch);
            return;
        }
    }

    if (!is_clan_member(ch->clan, clan) && !IS_IMMORTAL(ch)) {
        send_to_char("You're not even a member of that clan!\n\r", ch);
        return;
    }

    if (!IS_TRIBE(vch, clan)) {
        send_to_char("They're not in the clan, they have to be recruited first." "\n\r", ch);
        return;
    }

    if (!is_set_clan_flag(ch->clan, clan, CJOB_LEADER)
        && !IS_IMMORTAL(ch)) {
        send_to_char("You don't have the authority to promote within that clan." "\n\r", ch);
        return;
    }

    if (clan_rank_cmp(ch, vch, clan) < 0 && !IS_IMMORTAL(ch)) {
        send_to_char("You aren't high enough rank to promote them.\n\r", ch);
        return;
    }

    if (clan > 0 && clan < MAX_CLAN)
        strcpy(cname, clan_table[clan].name);
    else
        sprintf(cname, "%d", clan);

    if (is_number(arg2)) {
        rank = atoi(arg2);

        if (rank < MIN_CLAN_RANK || rank > clan_ranks[clan].num_ranks) {
            sprintf(buf, "Rank is a number from %d to %d.\n\r", MIN_CLAN_RANK,
                    clan_ranks[clan].num_ranks);
            send_to_char(buf, ch);
            return;
        }

        if (get_clan_rank(ch->clan, clan) < rank && !IS_IMMORTAL(ch)) {
            send_to_char("You can't promote to a rank higher than you've" " obtained!\n\r", ch);
            return;
        }

        if (get_clan_rank(vch->clan, clan) > rank) {
            send_to_char("They are of higher rank than that, try using" " demote.\n\r", ch);
            return;
        }

        if (!clan_ranks[clan].rank || clan_ranks[clan].num_ranks == 0) {
            send_to_char("Sorry, your clan immortal needs to set up your clan.\n\r"
                         "Contact them to set it up so you can set ranks.\n\r", ch);
            return;
        }

        if ((rname = get_clan_rank_name(clan, rank)) == NULL) {
            send_to_char("That is an invalid rank.\n\r", ch);
            return;
        }

        /* do the set */
        set_clan_rank(vch->clan, clan, rank);

        /* and now a word from our sponsors... */
        sprintf(buf, "You promoted %s to be %s of the %s.\n\r", PERS(ch, vch), rname, cname);
        send_to_char(buf, ch);

        sprintf(buf, "%s promoted you to be %s of the %s.\n\r", PERS(vch, ch), rname, cname);
        send_to_char(buf, vch);

        return;
    } else if ((rank = get_clan_rank_num(clan, arg2)) != 0) {
        if (rank < MIN_CLAN_RANK || rank > clan_ranks[clan].num_ranks) {
            sprintf(buf, "Rank is a number from %d to %d.\n\r", MIN_CLAN_RANK,
                    clan_ranks[clan].num_ranks);
            send_to_char(buf, ch);
            return;
        }

        if (get_clan_rank(ch->clan, clan) < rank && !IS_IMMORTAL(ch)) {
            send_to_char("You can't promote to a rank higher than you've" " obtained!\n\r", ch);
            return;
        }

        if (get_clan_rank(vch->clan, clan) > rank) {
            send_to_char("They are of higher rank than that, try using" " demote.\n\r", ch);
            return;
        }

        rname = get_clan_rank_name(clan, rank);

        /* do the set */
        set_clan_rank(vch->clan, clan, rank);

        /* and now a word from our sponsors... */
        sprintf(buf, "You promoted %s to be %s of the %s.\n\r", PERS(ch, vch), rname, cname);
        send_to_char(buf, ch);

        sprintf(buf, "%s promoted you to be %s of the %s.\n\r", PERS(vch, ch), rname, cname);

        send_to_char(buf, vch);

        return;
    } else if ((flag = get_flag_bit(clan_job_flags, arg2)) == -1) {
        send_to_char("No such job.\n\r", ch);
        return;
    }

    if (is_set_clan_flag(vch->clan, clan, flag)) {
        flags = show_flags(clan_job_flags, flag);
        sprintf(buf, "They're already %s%s.\n\r", indefinite_article(flags), flags);
        free(flags);
        send_to_char(buf, ch);
        return;
    }

    set_clan_flag(vch->clan, clan, flag);
    flags = show_flags(clan_job_flags, flag);

    sprintf(buf, "%s has been given the clan job '%s' in %s.\n\r", capitalize(PERS(ch, vch)), flags,
            cname);
    send_to_char(buf, ch);

    sprintf(buf, "%s gave you the clan job '%s' in %s.\n\r", capitalize(PERS(vch, ch)), flags,
            cname);
    send_to_char(buf, vch);

    free(flags);

    /* and for legacy's sake... */
    if (clan == vch->player.tribe && flag == CJOB_LEADER)
        MUD_SET_BIT(vch->specials.act, CFL_CLANLEADER);

    return;
}



/* demote a person in rank or remove a job flag */
void
cmd_demote(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vch;
    char cname[MAX_STRING_LENGTH];
    char *rname;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    int clan = 0, flag, rank;
    CLAN_DATA *pClan;
    char *flags;

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    strcpy(arg3, argument);     /* copy the rest into arg2 */

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Syntax: demote <person> <job|rank> <clan>\n\r", ch);

        sprintf(buf, "        rank is a number from %d to %d\n\r", MIN_CLAN_RANK, MAX_CLAN_RANK);
        send_to_char(buf, ch);

        send_to_char("        job is one of:\n\r", ch);

        send_flags_to_char(clan_job_flags, ch);
        return;
    }

    if ((vch = get_char_room_vis(ch, arg1)) == NULL) {
        send_to_char("Noone like that around to demote.\n\r", ch);
        return;
    }

    if (arg3[0] == '\0') {
        for (pClan = vch->clan; pClan; pClan = pClan->next) {
            if (pClan->clan && (IS_TRIBE_LEADER(ch, pClan->clan)
                                || IS_IMMORTAL(ch))) {
                clan = pClan->clan;
                break;
            }
        }

        if (!clan) {
            send_to_char("You can't demote people for any clan!\n\r", ch);
            return;
        }
    } else {
        clan = lookup_clan_by_name(arg3);

        if (clan == -1) {
            send_to_char("No such clan.\n\r", ch);
            return;
        }
    }

    if (!is_clan_member(ch->clan, clan) && !IS_IMMORTAL(ch)) {
        send_to_char("You're not even a member of that clan!\n\r", ch);
        return;
    }

    if (!is_clan_member(vch->clan, clan)) {
        send_to_char("They're not in the clan, they have to be recruited first." "\n\r", ch);
        return;
    }

    if (!is_set_clan_flag(ch->clan, clan, CJOB_LEADER)
        && !IS_IMMORTAL(ch)) {
        send_to_char("You don't have the authority to demote within that clan." "\n\r", ch);
        return;
    }

    if (clan_rank_cmp(ch, vch, clan) < 0 && !IS_IMMORTAL(ch)) {
        send_to_char("You aren't high enough rank to demote them.\n\r", ch);
        return;
    }

    if (clan > 0 && clan < MAX_CLAN)
        strcpy(cname, clan_table[clan].name);
    else
        sprintf(cname, "%d", clan);


    if (is_number(arg2)) {
        rank = atoi(arg2);

        if (rank < MIN_CLAN_RANK || rank > clan_ranks[clan].num_ranks) {
            sprintf(buf, "Rank is a number from %d to %d.\n\r", MIN_CLAN_RANK,
                    clan_ranks[clan].num_ranks);
            send_to_char(buf, ch);
            return;
        }

        if (get_clan_rank(ch->clan, clan) < rank && !IS_IMMORTAL(ch)) {
            send_to_char("You can't demote to a rank higher than you've" " obtained!\n\r", ch);
            return;
        }

        if (get_clan_rank(vch->clan, clan) <= rank) {
            send_to_char("They are already lower rank than that, try using" " promote instead.\n\r",
                         ch);
            return;
        }

        if (!clan_ranks[clan].rank || clan_ranks[clan].num_ranks == 0) {
            send_to_char("Sorry, your clan immortal needs to set up your clan.\n\r"
                         "Contact them to set it up so you can set ranks.\n\r", ch);
            return;
        }

        if ((rname = get_clan_rank_name(clan, rank)) == NULL) {
            send_to_char("That is an invalid rank.\n\r", ch);
            return;
        }

        set_clan_rank(vch->clan, clan, rank);

        /* and now a word from our sponsors... */
        sprintf(buf, "You demoted %s to be %s of the %s.\n\r", PERS(ch, vch), rname, cname);
        send_to_char(buf, ch);

        sprintf(buf, "%s demoted you to be %s of the %s.\n\r", PERS(vch, ch), rname, cname);
        send_to_char(buf, vch);

        return;
    } else if ((rank = get_clan_rank_num(clan, arg2)) != 0) {
        if (rank < MIN_CLAN_RANK || rank > clan_ranks[clan].num_ranks) {
            sprintf(buf, "Rank is a number from %d to %d.\n\r", MIN_CLAN_RANK,
                    clan_ranks[clan].num_ranks);
            send_to_char(buf, ch);
            return;
        }

        if (get_clan_rank(ch->clan, clan) < rank && !IS_IMMORTAL(ch)) {
            send_to_char("You can't demote to a rank higher than you've" " obtained!\n\r", ch);
            return;
        }

        if (get_clan_rank(vch->clan, clan) <= rank) {
            send_to_char("They are already lower rank than that, try using" " promote instead.\n\r",
                         ch);
            return;
        }

        rname = get_clan_rank_name(clan, rank);

        set_clan_rank(vch->clan, clan, rank);

        /* and now a word from our sponsors... */
        sprintf(buf, "You demoted %s to be %s of the %s.\n\r", PERS(ch, vch), rname, cname);
        send_to_char(buf, ch);

        sprintf(buf, "%s demoted you to be %s of the %s.\n\r", PERS(vch, ch), rname, cname);
        send_to_char(buf, vch);

        return;
    } else if ((flag = get_flag_bit(clan_job_flags, arg2)) == -1) {
        send_to_char("No such job.\n\r", ch);
        return;
    }

    if (!is_set_clan_flag(vch->clan, clan, flag)) {
        flags = show_flags(clan_job_flags, flag);
        sprintf(buf, "They aren't %s%s.\n\r", indefinite_article(flags), flags);
        free(flags);
        send_to_char(buf, ch);
        return;
    }

    remove_clan_flag(vch->clan, clan, flag);
    flags = show_flags(clan_job_flags, flag);

    sprintf(buf, "%s has had the clan job '%s' for %s removed.\n\r", capitalize(PERS(ch, vch)),
            flags, cname);
    send_to_char(buf, ch);

    sprintf(buf, "%s removed your clan job '%s' for %s.\n\r", capitalize(PERS(vch, ch)), flags,
            cname);
    send_to_char(buf, vch);

    free(flags);

    /* and for legacy's sake... */
    if (clan == vch->player.tribe && flag == CJOB_LEADER)
        REMOVE_BIT(vch->specials.act, CFL_CLANLEADER);

    return;
}


/* show the clans of a specified player, or yourself */
void
cmd_tribes(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vict = NULL;
    char buf[MAX_STRING_LENGTH];
    char old_arg[MAX_INPUT_LENGTH];
    char *rname;
    CLAN_DATA *clan;
    int c = 0;
    char *flags;

    for (; *argument == ' '; argument++);

    strcpy(old_arg, argument);
    argument = one_argument(argument, buf, sizeof(buf));

    if (IS_SWITCHED_IMMORTAL(ch) && buf[0] != '\0') {
        if (is_number(buf)) {
            c = atoi(buf);
            if (c <= 0 || c >= MAX_CLAN) {
                send_to_char("Invalid clan number.\n\r", ch);
                return;
            }
        } else if ((vict = get_char_world(ch, buf)) == NULL) {
            if (!strcmp(buf, "ranks") || !strcmp(buf, "rank")) {
                strcpy(old_arg, argument);
                if (buf[0] == '\0') {
                    send_to_char("Show the ranks of what tribe?\n\r", ch);
                    return;
                }
            }

            if (is_number(buf)) {
                c = atoi(buf);
                if (c <= 0 || c >= MAX_CLAN) {
                    send_to_char("Invalid clan number.\n\r", ch);
                    return;
                }
            } else {
                c = lookup_clan_by_name(old_arg);

                if (c == -1) {
                    send_to_char("You can't find anyone in the world like that.\n\r", ch);
                    return;
                }
            }
        }
    } else if (buf[0] != '\0') {        /* mortals looking to see ranks */
        /* if they supplied the optional 'ranks' argument, cut it out */
        if (!strcmp(buf, "ranks") || !strcmp(buf, "rank")) {
            strcpy(old_arg, argument);
            if (buf[0] == '\0') {
                send_to_char("Show the ranks of what tribe?\n\r", ch);
                return;
            }
        }

        /* search for matching clans that they are part of */
        for (c = 0; c < MAX_CLAN; c++)
            if (!str_prefix(old_arg, clan_table[c].name) && IS_TRIBE(ch, c))
                break;

        if (c == MAX_CLAN) {
            send_to_char("You aren't a member of that clan.\n\r", ch);
            return;
        }
    }

    if (c != 0) {
        sprintf(buf, "%s has the following ranks:\n\r", clan_table[c].name);
        send_to_char(buf, ch);

        send_clan_ranks_to_char(c, ch);
        return;
    }

    if (!vict)
        vict = ch;

    if (!vict->clan) {
        sprintf(buf, "%s %sn't a member of any tribe.\n\r", vict == ch ? "You" : MSTR(vict, name),
                vict == ch ? "are" : "is");
        send_to_char(buf, ch);
        return;
    }

    sprintf(buf, "%s %s:\n\r", vict == ch ? "You" : MSTR(vict, name), vict == ch ? "are" : "is");
    send_to_char(buf, ch);

    for (clan = vict->clan; clan; clan = clan->next) {
        flags = show_flags(clan_job_flags, clan->flags);

        if (clan->clan > 0 && clan->clan < MAX_CLAN) {
            c = clan->clan;
            rname = get_clan_rank_name(c, clan->rank);

            sprintf(buf, "%s%s%s%s%s.\n\r", rname ? capitalize(rname) : "", rname ? " of the " : "",
                    smash_article(clan_table[c].name), clan->flags ? ", jobs: " : "", flags);
        } else {
            sprintf(buf, "%d (Unknown)%s%s.\n\r", clan->clan, clan->flags ? ", jobs: " : "", flags);
        }
        send_to_char(buf, ch);
        free(flags);
    }
    return;
}
