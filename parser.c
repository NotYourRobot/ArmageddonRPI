/*
 * File: PARSER.C
 * Usage: Command parsing module.
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

/* Revision history
 * 12/17/1999 added initial forage skills to sorcs.  -sv
 * 05/15/2000 Save doesn't make concentration falter.  -sv
 * 05/26/2000 Fiddled secondary skills packages. -sv
 * 06/03/2000 Talk doesn't make concentration falter.  Neither
 *            does typo/bug/ idea.  More
 *            fiddles to secondary skills packages.  -sv 
 * 06/17/2000 Who and reset don't make concentration falter. -sv
 */

/* #define FD_SETSIZE 512 
*/
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

#include "constants.h"
#include "immortal.h"
#include "structs.h"
#include "clan.h"
#include "comm.h"
#include "parser.h"
#include "db.h"
#include "utils.h"
#include "limits.h"
#include "guilds.h"
#include "skills.h"
#include "vt100c.h"
#include "cities.h"
#include "handler.h"
#include "event.h"
#include "pc_file.h"
#include "modify.h"
#include "utility.h"
#include "parser.h"
#include "player_accounts.h"
#include "psionics.h"
#include "pather.h"
#include "character.h"
#include "creation.h"

// Too much fucking trouble making unistd with XOPEN_SOURCE work
//char *crypt(const char *key, const char *salt);

#define COMMANDO(number,min_pos,pointer,min_level) {      \
	cmd_info[(number)].minimum_position = (min_pos);        \
	cmd_info[(number)].command_pointer = (pointer);         \
	cmd_info[(number)].minimum_level = (min_level); }


#define NOT !
#define AND &&
#define OR ||

/*  #define MORTAL_LOCK  */
/*  #define NEW_CHAR_LOCK  */
#define STATE(d) ((d)->connected)

extern IMM_NAME *immortal_name_list;
void immlog(CHAR_DATA * ch, const char *argument, int cmd);
void adjust_new_char_upon_login(CHAR_DATA * ch);
void adjust_char_upon_login(CHAR_DATA * ch);

/* Need this for adding subguild notification to pinfo */

char *searchdir[5] = {
    "players_new",
    "dead",
    "waiting",
    "storage",
    "approved"
};

int dir_status[5] = {
    APPLICATION_NONE,
    APPLICATION_DEATH,
    APPLICATION_APPLYING,
    APPLICATION_STORED,
    APPLICATION_ACCEPT
};

char *fread_string_no_whitespace(FILE * fp);

int
karma_race_ok(unsigned long int rflags, char race)
{
    switch (race) {
    case 'a': if (IS_SET(rflags, KINFO_RACE_HUMAN)) return TRUE; break;
    case 'b': if (IS_SET(rflags, KINFO_RACE_ELF)) return TRUE; break;
    case 'c': if (IS_SET(rflags, KINFO_RACE_DWARF)) return TRUE; break;
    case 'd': if (IS_SET(rflags, KINFO_RACE_HALFLING)) return TRUE; break;
    case 'e': if (IS_SET(rflags, KINFO_RACE_HALF_ELF)) return TRUE; break;
    case 'f': if (IS_SET(rflags, KINFO_RACE_MUL)) return TRUE; break;
    case 'g': if (IS_SET(rflags, KINFO_RACE_HALF_GIANT)) return TRUE; break;
    case 'h': if (IS_SET(rflags, KINFO_RACE_MANTIS)) return TRUE; break;
    case 'i': if (IS_SET(rflags, KINFO_RACE_GITH)) return TRUE; break;
    case 'j': if (IS_SET(rflags, KINFO_RACE_DRAGON)) return TRUE; break;
    case 'k': if (IS_SET(rflags, KINFO_RACE_AVANGION)) return TRUE; break;
    case 'l': if (IS_SET(rflags, KINFO_RACE_DESERT_ELF)) return TRUE; break;
    case 'm': if (IS_SET(rflags, KINFO_RACE_BLACKWING_ELF)) return TRUE; break;
    }

    return FALSE;
}

int
karma_guild_ok(unsigned long int guilds, int i)
{
    switch (i) {
    case GUILD_ASSASSIN: if (IS_SET(guilds, KINFO_GUILD_ASSASSIN)) return TRUE; break;
    case GUILD_BURGLAR: if (IS_SET(guilds, KINFO_GUILD_BURGLAR)) return TRUE; break;
    case GUILD_MERCHANT: if (IS_SET(guilds, KINFO_GUILD_MERCHANT)) return TRUE; break;
    case GUILD_PICKPOCKET: if (IS_SET(guilds, KINFO_GUILD_PICKPOCKET)) return TRUE; break;
    case GUILD_WARRIOR: if (IS_SET(guilds, KINFO_GUILD_WARRIOR)) return TRUE; break;
    case GUILD_RANGER: if (IS_SET(guilds, KINFO_GUILD_RANGER)) return TRUE; break;
    case GUILD_PSIONICIST: if (IS_SET(guilds, KINFO_GUILD_PSIONICIST)) return TRUE; break;
    case GUILD_DEFILER: if (IS_SET(guilds, KINFO_GUILD_SORCERER)) return TRUE; break;
    case GUILD_SHADOW_ELEMENTALIST: if (IS_SET(guilds, KINFO_GUILD_SHADOW_CLERIC)) return TRUE; break;
    case GUILD_FIRE_ELEMENTALIST: if (IS_SET(guilds, KINFO_GUILD_FIRE_CLERIC)) return TRUE; break;
    case GUILD_WIND_ELEMENTALIST: if (IS_SET(guilds, KINFO_GUILD_WIND_CLERIC)) return TRUE; break;
    case GUILD_WATER_ELEMENTALIST: if (IS_SET(guilds, KINFO_GUILD_WATER_CLERIC)) return TRUE; break;
    case GUILD_STONE_ELEMENTALIST: if (IS_SET(guilds, KINFO_GUILD_STONE_CLERIC)) return TRUE; break;
    case GUILD_LIGHTNING_ELEMENTALIST: if (IS_SET(guilds, KINFO_GUILD_LIGHTNING_CLERIC)) return TRUE; break;
    case GUILD_VOID_ELEMENTALIST: if (IS_SET(guilds, KINFO_GUILD_VOID_CLERIC)) return TRUE; break;
    case GUILD_TEMPLAR: if (IS_SET(guilds, KINFO_GUILD_TEMPLAR)) return TRUE; break;
    case GUILD_LIRATHU_TEMPLAR: if (IS_SET(guilds, KINFO_GUILD_LIRATHU_TEMPLAR)) return TRUE; break;
    case GUILD_JIHAE_TEMPLAR: if (IS_SET(guilds, KINFO_GUILD_JIHAE_TEMPLAR)) return TRUE; break;
    case GUILD_NOBLE: if (IS_SET(guilds, KINFO_GUILD_NOBLE)) return TRUE; break;
    }

    return FALSE;
}


void
convert_player_information(PLAYER_INFO * pinfo)
{
    FILE *out;
    char *name;
    char buf[MAX_STRING_LENGTH];
    char command[MAX_STRING_LENGTH];
    int i;

    for (i = 0; i < 5; i++) {
        sprintf(buf, "./findemail %s %s_emails", pinfo->email, searchdir[i]);
        if ((out = popen(buf, "r")) == NULL) {
            sprintf(buf, "Unable to convert player information, findemail failed.");
            gamelog(buf);
            return;
        }

        /* read each line and do it */
        while (!feof(out)
               && (name = fread_string_no_whitespace(out)) != NULL) {
            if (strcmp(name, "-end findemail-")) {
                add_new_char_info(pinfo, name, dir_status[i]);
                sprintf(command, "cp %s/%s %s/%s", searchdir[i], name, ACCOUNT_DIR, pinfo->name);
                system(command);

                /* grab karma & karmaLog */
                if (dir_status[i] == APPLICATION_NONE) {
                    if (pinfo->karmaLog) {
                        free(pinfo->karmaLog);
                        pinfo->karmaLog = NULL;
                    }
                } else if (dir_status[i] == APPLICATION_APPLYING) {
                    /* tack account & character name into external file */
                    sprintf(command, "echo %s %s >> applications", pinfo->name, name);
                    system(command);
                }
            } else
                break;
        }
        pclose(out);
    }
}

bool
is_free_email(char *email)
{
    return 0;
}

struct command_info cmd_info[MAX_CMD_LIST];

void show_subguild(struct descriptor_data *d);
void show_guild(struct descriptor_data *d);
void show_race_selection(struct descriptor_data *d);
int race_guild_ok(CHAR_DATA * ch, int i);
void mul_slave_choice(struct descriptor_data *d, char *choice);

int granted_cmd_level(EXTRA_CMD * ecmd, int cmd);

const char * const fill[] = {
    "in",
    "from",
    "with",
    "the",
    "on",
    "at",
    "to",
    "\n"
};

int
search_block(char *arg, const char * const *list, bool exact)
{
    register int i, l;

    /* Make into lower case, and get length of string */
    for (l = 0; *(arg + l); l++)
        *(arg + l) = LOWER(*(arg + l));

    if (exact) {
        for (i = 0; **(list + i) != '\n'; i++)
            if (!strcmp(arg, *(list + i)))
                return (i);
    } else {
        if (!l)
            l = 1;              /* Avoid "" to match the first available string */
        for (i = 0; **(list + i) != '\n'; i++)
            if (!strncmp(arg, *(list + i), l))
                return (i);
    }

    return (-1);
}


int
old_search_block(const char *argument, int begin, int length, const char * const *list, int mode)
{
    int guess, found, search;

    /* If the word contain 0 letters, then a match is already found */
    found = (length < 1);

    guess = 0;

    /* Search for a match */

    if (mode)
        while (NOT found AND * (list[guess]) != '\n') {
            found = (length == strlen(list[guess]));
            for (search = 0; (search < length AND found); search++)
                found = (*(argument + begin + search) == *(list[guess] + search));
            guess++;
    } else {
        while (NOT found AND * (list[guess]) != '\n') {
            found = 1;
            for (search = 0; (search < length AND found); search++)
                found = (*(argument + begin + search) == *(list[guess] + search));
            guess++;
        }
    }

    return (found ? guess : -1);
}


#define EMOTE_ABBREV     ':'
#define SAY_ABBREV      '\''
#define IMMCOM_ABBREV    ']'
#define GODCOM_ABBREV    '}'
#define OOC_ABBREV       '\"'
#define ASL_ABBREV	 '('
#define IOOC_ABBREV      '['
#define BLDCOM_ABBREV    ')'

#define COMMAND_UNKNOWN -1
#define COMMAND_NONE     0

int
get_command(char *com)
{
    int i;

    if (!strnicmp("'", com, strlen(com)))
        return CMD_SAY;
    if (!strnicmp("]", com, strlen(com)))
        return CMD_IMMCOM;
    if (!strnicmp("}", com, strlen(com)))
        return CMD_GODCOM;
    if (!strnicmp(":", com, strlen(com)))
        return CMD_EMOTE;
    if (!strnicmp("[", com, strlen(com)))
        return CMD_IOOC;
    if (!strnicmp(")", com, strlen(com)))
        return CMD_BLDCOM;

    for (i = 1; *command_name[i] != '\n'; i++)
        if (!strnicmp(command_name[i], com, strlen(com)))
            return i;

    return COMMAND_UNKNOWN;
}

int
can_do_command(CHAR_DATA *ch, int cmd) {
    int minlevel = cmd_info[cmd].minimum_level;
    CHAR_DATA *brain;

#define IMM_CMDS_WHILE_SWITCHED
#ifdef IMM_CMDS_WHILE_SWITCHED
    if( IS_NPC(ch) && ch->desc && ch->desc->original && IS_IMMORTAL(ch->desc->original) ) {
        brain = ch->desc->original;
    } else {
        brain = ch;
    }
#else
    brain = ch;
#endif

    if ((GET_LEVEL(brain) < minlevel
      && !has_extra_cmd(brain->granted_commands, cmd, minlevel, TRUE))
     || (GET_LEVEL(brain) >= minlevel
      && has_extra_cmd(brain->revoked_commands, cmd, -1, FALSE))
     || (IS_NPC(brain) && (minlevel > MORTAL))) {
        return FALSE;
    }
    return TRUE;
}

int
perform_command(CHAR_DATA * ch, Command cmd, const char *arg, int count)
{
    CHAR_DATA *subduer;
    struct obj_data *obj;

    obj = NULL;        /* Initialize pointer */

    if (!can_do_command(ch, cmd)) {
        send_to_char(TYPO_MSG, ch);
        return (0);
    }

    if (!cmd_info[cmd].command_pointer) {
        send_to_char(TYPO_MSG, ch);
        return (0);
    }

    if (GET_POS(ch) < cmd_info[cmd].minimum_position) {
        switch (GET_POS(ch)) {
        case POSITION_DEAD:
        case POSITION_MORTALLYW:
            send_to_char("You are on the verge of death, unable to do anything.\n\r", ch);
            // Removed because act() no longer works on people below POSITION_RESTING
            //            act ("You are on the verge of death, unable to do anything.", FALSE, ch, 0, 0, TO_CHAR);
            break;
        case POSITION_STUNNED:
            send_to_char("You are stunned, unable to move.\n\r", ch);
            // Removed because act() no longer works on people below POSITION_RESTING
            //            act ("You are stunned, unable to move.", FALSE, ch, 0, 0, TO_CHAR);
            break;
        case POSITION_SLEEPING:
            send_to_char("You'll have to wake up first.\n\r", ch);
            // Removed because act() no longer works on people below POSITION_RESTING
            //      act ("You'll have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
            break;
        case POSITION_RESTING:
            act("You feel too relaxed to do that.", FALSE, ch, 0, 0, TO_CHAR);
            break;
        case POSITION_SITTING:
            act("Maybe you should get on your feet first.", FALSE, ch, 0, 0, TO_CHAR);
            break;
        case POSITION_FIGHTING:
            act("You're fighting for your life!", FALSE, ch, 0, 0, TO_CHAR);
            break;
        }

        return 0;
    }

    // Commands that CAN be issued while paralyzed
    if (GET_LEVEL(ch) < OVERLORD && is_paralyzed(ch)
        && (cmd_info[cmd].command_pointer != cmd_think)
        && (cmd_info[cmd].command_pointer != cmd_feel)
        && (cmd_info[cmd].command_pointer != cmd_help)
        && (cmd_info[cmd].command_pointer != cmd_save)
        && (cmd_info[cmd].command_pointer != cmd_score)
        && (cmd_info[cmd].command_pointer != cmd_skills)
        && (cmd_info[cmd].command_pointer != cmd_stat)
        && (cmd_info[cmd].command_pointer != cmd_stop)
        && (cmd_info[cmd].command_pointer != cmd_look)
        && (cmd_info[cmd].command_pointer != cmd_wish)
        && (cmd_info[cmd].command_pointer != cmd_gone)
        && (cmd_info[cmd].command_pointer != cmd_return)
        && (cmd_info[cmd].command_pointer != cmd_ooc)
        && (cmd_info[cmd].command_pointer != cmd_iooc)
        && (cmd_info[cmd].command_pointer != cmd_mercy)
        && (cmd_info[cmd].command_pointer != cmd_nosave)
        && (cmd_info[cmd].command_pointer != cmd_bug)
        && (cmd_info[cmd].command_pointer != cmd_typo)
        && (cmd_info[cmd].command_pointer != cmd_idea)
        && (!is_psionic_command(cmd))) {
        send_to_char("You can't move.\n\r", ch);
        return 0;
    }

    if ((cmd == CMD_PURGE) || (cmd == CMD_SHUTDOWN) || (cmd == CMD_SNOOP) || (cmd == CMD_REROLL)
        || (cmd == CMD_RESTORE) || (cmd == CMD_SWITCH) || (cmd == CMD_AWARD) || (cmd == CMD_DAMAGE)
        || (cmd == CMD_NUKE) || (cmd == CMD_BACKSAVE) || (cmd == CMD_LOG) || (cmd == CMD_CRIMINAL)
        || (cmd == CMD_FREEZE) || (cmd == CMD_BSET) || (cmd == CMD_SYSTEM) || (cmd == CMD_PLMAX)
        || (cmd == CMD_CLANLEADER) || (cmd == CMD_NOWISH) || (cmd == CMD_SKILLRESET)
        || (cmd == CMD_ZORCH) || (cmd == CMD_FLOAD) || (cmd == CMD_PINFO) || (cmd == CMD_TAX)
        || (cmd == CMD_WSET) || (cmd == CMD_CLANSET) || (cmd == CMD_CLANSAVE)
        || (cmd == CMD_TRANSFER) || (cmd == CMD_HEAL) || (cmd == CMD_ECHO)
        || (cmd == CMD_WIZLOCK) || (cmd == CMD_CITYSET) || (cmd == CMD_DOAS)
        || (cmd == CMD_SWITCH) || (cmd == CMD_FORCE) || (cmd == CMD_ASET) || (cmd == CMD_CHARDELETE)
        || (cmd == CMD_RESURRECT) || (cmd == CMD_STORE) || (cmd == CMD_UNSTORE)
        || (cmd == CMD_CGIVE) || (cmd == CMD_CREVOKE) || (cmd == CMD_OCOPY)
        || (cmd == CMD_OCREATE) || (cmd == CMD_OSET) || (cmd == CMD_OSAVE)
        || (cmd == CMD_OUNSET) || (cmd == CMD_ODELETE) || (cmd == CMD_OUPDATE)
        || (cmd == CMD_MCOPY) || (cmd == CMD_MCREATE) || (cmd == CMD_MSAVE) || (cmd == CMD_CSET)
        || (cmd == CMD_CUNSET) || (cmd == CMD_CDELETE) || (cmd == CMD_MUPDATE)
        || (cmd == CMD_MDUP) || (cmd == CMD_RCOPY) || (cmd == CMD_RCREATE) || (cmd == CMD_RSAVE)
        || (cmd == CMD_RSET) || (cmd == CMD_RUNSET) || (cmd == CMD_RDELETE)
        || (cmd == CMD_ZSAVE) || (cmd == CMD_ZSET) || (cmd == CMD_ZOPEN) || (cmd == CMD_ZCLOSE)
        || (cmd == CMD_AINFO) || (cmd == CMD_ZCLEAR) || (cmd == CMD_ZRESET)
        || (cmd == CMD_RZSAVE) || (cmd == CMD_RZRESET) || (cmd == CMD_JSEVAL)
        || (cmd == CMD_GIVE) || (cmd == CMD_DROP) || (cmd == CMD_HIT) || (cmd == CMD_KILL) 
        || (cmd == CMD_CSTAT))
        immlog(ch, arg, cmd);

    // Commands that DON'T break concentration
    if (is_in_command_q(ch) && !ch->specials.to_process
        && (cmd_info[cmd].command_pointer != cmd_look) &&
        /* added this so socials won't break commands -Morg */
        (cmd_info[cmd].command_pointer != cmd_change) &&
        /* added this so only chnage hands breaks commands -Ness */
        (cmd_info[cmd].command_pointer != cmd_action) &&
        /* added this so lowering hoods won't break commands -Ness */
        (cmd_info[cmd].command_pointer != cmd_lower)
        && (cmd_info[cmd].command_pointer != cmd_alias)
        && (cmd_info[cmd].command_pointer != cmd_assess)
        && (cmd_info[cmd].command_pointer != cmd_biography)
        && (cmd_info[cmd].command_pointer != cmd_break)
        && (cmd_info[cmd].command_pointer != cmd_brief)
        && (cmd_info[cmd].command_pointer != cmd_bug)
        && (cmd_info[cmd].command_pointer != cmd_cease)
        && (cmd_info[cmd].command_pointer != cmd_cset)
        && (cmd_info[cmd].command_pointer != cmd_cstat)
        && (cmd_info[cmd].command_pointer != cmd_discuss)
        && (cmd_info[cmd].command_pointer != cmd_emote)
        && (cmd_info[cmd].command_pointer != cmd_equipment)
        && (cmd_info[cmd].command_pointer != cmd_examine)
        && (cmd_info[cmd].command_pointer != cmd_exits)
        && (cmd_info[cmd].command_pointer != cmd_godcom)
        && (cmd_info[cmd].command_pointer != cmd_gone)
        && (cmd_info[cmd].command_pointer != cmd_help)
        && (cmd_info[cmd].command_pointer != cmd_idea)
        && (cmd_info[cmd].command_pointer != cmd_immcom)
        && (cmd_info[cmd].command_pointer != cmd_infobar)
        && (cmd_info[cmd].command_pointer != cmd_inventory)
        && (cmd_info[cmd].command_pointer != cmd_keyword)
        && (cmd_info[cmd].command_pointer != cmd_mcreate)
        && (cmd_info[cmd].command_pointer != cmd_mercy)
        && (cmd_info[cmd].command_pointer != cmd_monitor)
        && (cmd_info[cmd].command_pointer != cmd_motd)
        && (cmd_info[cmd].command_pointer != cmd_news)
        && (cmd_info[cmd].command_pointer != cmd_nosave)
        && (cmd_info[cmd].command_pointer != cmd_ocreate)
        && (cmd_info[cmd].command_pointer != cmd_ooc)
        && (cmd_info[cmd].command_pointer != cmd_iooc)
        && (cmd_info[cmd].command_pointer != cmd_oset)
        && (cmd_info[cmd].command_pointer != cmd_ostat)
        && (cmd_info[cmd].command_pointer != cmd_pagelength)
        && (cmd_info[cmd].command_pointer != cmd_pemote)
        && (cmd_info[cmd].command_pointer != cmd_prompt)
        && (cmd_info[cmd].command_pointer != cmd_quiet)
        && (cmd_info[cmd].command_pointer != cmd_read)
        && (cmd_info[cmd].command_pointer != cmd_rcreate)
        && (cmd_info[cmd].command_pointer != cmd_recite)
        && (cmd_info[cmd].command_pointer != cmd_reset)
        && (cmd_info[cmd].command_pointer != cmd_return)
        && (cmd_info[cmd].command_pointer != cmd_review)
        && (cmd_info[cmd].command_pointer != cmd_rset)
        && (cmd_info[cmd].command_pointer != cmd_rstat)
        && (cmd_info[cmd].command_pointer != cmd_save)
        && (cmd_info[cmd].command_pointer != cmd_say)
        && (cmd_info[cmd].command_pointer != cmd_score)
        && (cmd_info[cmd].command_pointer != cmd_shout)
        && (cmd_info[cmd].command_pointer != cmd_sing)
        && (cmd_info[cmd].command_pointer != cmd_skills)
        && (cmd_info[cmd].command_pointer != cmd_sniff)
        && (cmd_info[cmd].command_pointer != cmd_stat)
        && (cmd_info[cmd].command_pointer != cmd_stop)
        && (cmd_info[cmd].command_pointer != cmd_talk)
        && (cmd_info[cmd].command_pointer != cmd_tdesc)
        && (cmd_info[cmd].command_pointer != cmd_tell)
        && (cmd_info[cmd].command_pointer != cmd_think)
        && (cmd_info[cmd].command_pointer != cmd_feel)
        && (cmd_info[cmd].command_pointer != cmd_time)
        && (cmd_info[cmd].command_pointer != cmd_tribes)
        && (cmd_info[cmd].command_pointer != cmd_typo)
        && (cmd_info[cmd].command_pointer != cmd_weather)
        && (cmd_info[cmd].command_pointer != cmd_whisper)
        && (cmd_info[cmd].command_pointer != cmd_who)
        && (cmd_info[cmd].command_pointer != cmd_wish)
        && (cmd_info[cmd].command_pointer != cmd_wizlist)
        && (cmd_info[cmd].command_pointer != cmd_bldcom)) {
        act("Your concentration falters...", FALSE, ch, 0, 0, TO_CHAR);
        rem_from_command_q(ch);
    }

    if (ch->specials.gone && (cmd_info[cmd].command_pointer != cmd_gone))
        stop_gone(ch);

    // They are frozen, not paralyzed.  They can do nothing at all.
    if (GET_LEVEL(ch) < OVERLORD && IS_SET(ch->specials.act, CFL_FROZEN)
        && !is_paralyzed(ch)) {
        send_to_char("You can't move.\n\r", ch);
        return 0;
    }

    // Commands that CAN be issued while paralyzed
    if (GET_LEVEL(ch) < OVERLORD && is_paralyzed(ch)
        && (cmd_info[cmd].command_pointer != cmd_think)
        && (cmd_info[cmd].command_pointer != cmd_feel)
        && (cmd_info[cmd].command_pointer != cmd_score)
        && (cmd_info[cmd].command_pointer != cmd_stat)
        && (cmd_info[cmd].command_pointer != cmd_stop)
        && (cmd_info[cmd].command_pointer != cmd_look)

        && (cmd_info[cmd].command_pointer != cmd_wish)
        && (cmd_info[cmd].command_pointer != cmd_gone)
        && (cmd_info[cmd].command_pointer != cmd_return)
        && (cmd_info[cmd].command_pointer != cmd_ooc)
        && (cmd_info[cmd].command_pointer != cmd_iooc)
        && (cmd_info[cmd].command_pointer != cmd_mercy)
        && (cmd_info[cmd].command_pointer != cmd_nosave)
        && (cmd_info[cmd].command_pointer != cmd_bug)
        && (cmd_info[cmd].command_pointer != cmd_typo)
        && (cmd_info[cmd].command_pointer != cmd_idea)
        && (!is_psionic_command(cmd))) {
        send_to_char("You can't move.\n\r", ch);
        return 0;
    }

    /* If someone is subdued, but subduer is not present for whatever reason */
    if (IS_AFFECTED(ch, CHAR_AFF_SUBDUED)) {
        for (subduer = ch->in_room->people; subduer; subduer = subduer->next_in_room) {
            if (subduer->specials.subduing == ch)
                break;
        }
        if (!subduer) {
            gamelogf("%s has subdue affect, but subduer not present.  Removing subdued affect.", GET_NAME(ch));
            REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_SUBDUED);
        }
    }

    // Affected by contact, but there's no contact
    if (affected_by_spell(ch, PSI_CONTACT) && !ch->specials.contact)
        remove_contact(ch);

    // Commands that can be performed while subdued
    if ((cmd_info[cmd].minimum_position > POSITION_SLEEPING)
        && IS_AFFECTED(ch, CHAR_AFF_SUBDUED) && (cmd_info[cmd].command_pointer != cmd_order)
        && (cmd_info[cmd].command_pointer != cmd_look)
        && (cmd_info[cmd].command_pointer != cmd_say)
        && (cmd_info[cmd].command_pointer != cmd_tell)
        && (cmd_info[cmd].command_pointer != cmd_ooc)
        && (cmd_info[cmd].command_pointer != cmd_iooc)
        && (cmd_info[cmd].command_pointer != cmd_whisper)
        && (cmd_info[cmd].command_pointer != cmd_shout)
        && (cmd_info[cmd].command_pointer != cmd_flee)
        && (cmd_info[cmd].command_pointer != cmd_assess) && (cmd != CMD_LISTEN)
        && (cmd_info[cmd].command_pointer != cmd_remove)
        && (cmd_info[cmd].command_pointer != cmd_drop)
        && (cmd_info[cmd].command_pointer != cmd_stop)
        && (cmd_info[cmd].command_pointer != cmd_change)
        && (cmd_info[cmd].command_pointer != cmd_think)
        && (cmd_info[cmd].command_pointer != cmd_feel)
        && (cmd_info[cmd].command_pointer != cmd_gone)
        && (cmd_info[cmd].command_pointer != cmd_wish)
        && (!is_psionic_command(cmd))) {
        act("You are held tight, and unable to do anything.", FALSE, ch, 0, 0, TO_CHAR);
        return (0);
    }

    /* hacked this to allow alkyone to cast while ethereal.  she
     * only gets two spells - hands of wind and banish - so she
     * should be fairly harmless. -hal */
    /*if (((IS_AFFECTED(ch, CHAR_AFF_ETHEREAL)) && !IS_IMMORTAL(ch)) && */
    if ((is_char_ethereal(ch) && !IS_IMMORTAL(ch))
        && ((cmd_info[cmd].command_pointer == cmd_assist)
            || (cmd_info[cmd].command_pointer == cmd_shoot)
            || (cmd_info[cmd].command_pointer == cmd_skin)
            || (cmd_info[cmd].command_pointer == cmd_mount)
            || (cmd_info[cmd].command_pointer == cmd_get)
            || (cmd_info[cmd].command_pointer == cmd_put)
            || (cmd_info[cmd].command_pointer == cmd_open)
            || (cmd_info[cmd].command_pointer == cmd_close)
            || (cmd_info[cmd].command_pointer == cmd_skill) ||
            /*    (cmd_info[cmd].command_pointer == cmd_kill) ||
             * (cmd_info[cmd].command_pointer == cmd_hit) ||      */
            ((cmd_info[cmd].command_pointer == cmd_cast)
             && strcasecmp(MSTR(ch, name), "alkyone")))) {
        act("You can't seem to do that while ethereal.", FALSE, ch, 0, 0, TO_CHAR);
        return (0);
    }

    /* Hack to stop people from using '.d' to move round while editing */
    if (ch->desc && ch->desc->str && !IS_IMMORTAL(ch)
        && (cmd_info[cmd].command_pointer == cmd_move
            || cmd_info[cmd].command_pointer == cmd_enter
            || cmd_info[cmd].command_pointer == cmd_leave
            || cmd_info[cmd].command_pointer == cmd_assist
            || cmd_info[cmd].command_pointer == cmd_shoot
            || cmd_info[cmd].command_pointer == cmd_mount
            || cmd_info[cmd].command_pointer == cmd_hit
            || cmd_info[cmd].command_pointer == cmd_kill
            || cmd_info[cmd].command_pointer == cmd_open
            || cmd_info[cmd].command_pointer == cmd_close
            || cmd_info[cmd].command_pointer == cmd_cast
            || cmd_info[cmd].command_pointer == cmd_hitch
            || cmd_info[cmd].command_pointer == cmd_harness
            || cmd_info[cmd].command_pointer == cmd_quit)) {
        send_to_char("You can't do that while editing a string.\n\r", ch);
        return (0);
    }

    if (!no_specials && special(ch, cmd, arg))
        return (0);

    // Commands that don't break hide
    if ((IS_AFFECTED(ch, CHAR_AFF_HIDE)) && (cmd_info[cmd].minimum_position > POSITION_SLEEPING)
        && (cmd_info[cmd].command_pointer != cmd_look)
        && (cmd_info[cmd].command_pointer != cmd_peek)
        && (cmd != CMD_LISTEN)   /* listen */
        && (cmd_info[cmd].command_pointer != cmd_follow)
        && (cmd_info[cmd].command_pointer != cmd_walk)
        && (cmd_info[cmd].command_pointer != cmd_watch)
        && (cmd_info[cmd].command_pointer != cmd_sneak)
        && (cmd_info[cmd].command_pointer != cmd_gone)
        && (cmd_info[cmd].command_pointer != cmd_run)
        && (cmd_info[cmd].command_pointer != cmd_scan)
        && (cmd_info[cmd].command_pointer != cmd_listen)
        && (cmd_info[cmd].command_pointer != cmd_move)
        && (cmd_info[cmd].command_pointer != cmd_enter)
        && (cmd_info[cmd].command_pointer != cmd_leave)
        && (cmd_info[cmd].command_pointer != cmd_assess)
        && (cmd_info[cmd].command_pointer != cmd_break)
        && (cmd_info[cmd].command_pointer != cmd_cease)
        && (cmd_info[cmd].command_pointer != cmd_time)
        && (cmd_info[cmd].command_pointer != cmd_weather)
        && (cmd_info[cmd].command_pointer != cmd_commune)
        && (cmd_info[cmd].command_pointer != cmd_think)
        && (cmd_info[cmd].command_pointer != cmd_feel)
        && (cmd_info[cmd].command_pointer != cmd_inventory)
        && (cmd_info[cmd].command_pointer != cmd_equipment)
        && (cmd_info[cmd].command_pointer != cmd_stop)
        && (cmd_info[cmd].command_pointer != cmd_wish)
        && (cmd != CMD_PALM)	/* palm (uses cmd_get) */
        && (cmd != CMD_SLIP)	/* slip (used cmd_put) */
        && (cmd != CMD_LATCH)	/* latch (uses cmd_close) */
        && (cmd != CMD_UNLATCH)	/* unlatch (uses cmd_open) */
        /* The following three psi skills have a check if the skill 
         * is < 25% it'll remove the hidden flag while using them -Morg */
        && (cmd_info[cmd].command_pointer != cmd_psi)
        && (cmd != CMD_BARRIER)
        && (cmd != CMD_CONTACT)
        && (cmd != CMD_PLANT) 
        && (cmd != CMD_STEAL)) { /* steal has special handling for hide */
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
    }

    if (ch->specials.subduing && (hands_free(ch) < 1) && (GET_RACE(ch) != RACE_MANTIS)) {
        send_to_char("You don't have enough hands!\n\r", ch);

        if (ch->equipment[ETWO])
            obj = ch->equipment[ETWO];
        if (ch->equipment[ES])
            obj = ch->equipment[ES];
        if (ch->equipment[EP])
            obj = ch->equipment[EP];

        act("You are forced to release $h as you fumble with $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("$h easily steps away from $n as $e fumbles with $s $o.", TRUE, ch, obj,
            ch->specials.subduing, TO_NOTVICT);
        act("$n relaxes $s grip as $e fumbles with $s $o, and you are free!", FALSE, ch, obj,
            ch->specials.subduing, TO_VICT);

        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
        ch->specials.subduing = (CHAR_DATA *) 0;
    }

    if (!cmd_info[cmd].command_pointer) {
        errorlog("command pointer null.");
        return (0);
    }

    ch->player.orig_level = -1;
    int levelchg = 0;
    /* switch to new level for granted commands */
    if (cmd != CMD_FORCE    /* don't allow 'force' to use this */
        && cmd != CMD_AT    /* don't allow 'at' to use this */
        && cmd != CMD_ORDER /* don't allow 'order' to use this */
        && has_extra_cmd(ch->granted_commands, cmd, cmd_info[cmd].minimum_level, TRUE)
        && granted_cmd_level(ch->granted_commands, cmd) != -1) {
        levelchg = 1;
        ch->player.orig_level = ch->player.level;
        ch->player.level = granted_cmd_level(ch->granted_commands, cmd);
    }
    // PERFORM THE COMMAND
    ((*cmd_info[cmd].command_pointer) (ch, arg, cmd, count));

    // Expand this to include commands that should auto save
    if (cmd == CMD_GET
        || cmd == CMD_PUT
        || cmd == CMD_EAT
        || cmd == CMD_SELL
        || cmd == CMD_DROP
        || cmd == CMD_BARTER
        || cmd == CMD_GIVE
        || cmd == CMD_BUY
        || cmd == CMD_STEAL
        || cmd == CMD_TAKE
        || cmd == CMD_SUBDUE
        || cmd == CMD_JUNK
        || cmd == CMD_DEPOSIT
        || cmd == CMD_WITHDRAW
        || cmd == CMD_PACK
        || cmd == CMD_UNPACK
        || cmd == CMD_SHOOT
        || cmd == CMD_TAILOR
        || cmd == CMD_PLANT
        || cmd == CMD_FORAGE
        || cmd == CMD_PALM
        || cmd == CMD_SALVAGE
        || cmd == CMD_BURY)
	save_char(ch);

    /* replace their original level */
    if (levelchg) {
        ch->player.level = ch->player.orig_level;
    }
    return (0);
}

void
alias_substitute(CHAR_DATA * ch, char *arg)
{
    int i, c;
    char buf[MAX_STRING_LENGTH], com[256], list[MAX_STRING_LENGTH];

    for (c = 0; arg[c] && (arg[c] != ' '); c++)
        com[c] = arg[c];
    com[c] = '\0';

    strcpy(list, &arg[c]);

    for (i = 0; i < MAX_ALIAS; i++)
        if (!stricmp(ch->alias[i].alias, com))
            break;
    if (i == MAX_ALIAS)
        return;

    strcpy(buf, ch->alias[i].text);
    strcat(buf, list);
    strncpy(arg, buf, MAX_INPUT_LENGTH);
    arg[MAX_INPUT_LENGTH - 1] = '\0';
}

const char *
get_alias(CHAR_DATA * ch, const char *arg)
{
    int i;
    for (i = 0; i < MAX_ALIAS; i++) {
        if (strcmp(arg, ch->alias[i].alias) == 0)
            return ch->alias[i].alias;
    }

    return NULL;
}

int
num_queued_commands( CHAR_DATA *ch ) {
    QUEUED_CMD *qcmd;
    int count = 0;

    for( qcmd = ch->queued_commands; qcmd; qcmd = qcmd->next )
	count++;
    return count;
}

void 
show_queued_commands( CHAR_DATA *viewer, CHAR_DATA *ch ) {
    QUEUED_CMD *qcmd;
    int count = 1;

    for( qcmd = ch->queued_commands; qcmd; qcmd = qcmd->next ) {
        cprintf( viewer, " %2d - %s\n\r", count++, qcmd->command );
    }
    return;
}

int
flush_queued_commands(CHAR_DATA *ch, bool force_all) {
    QUEUED_CMD *qcmd, *next_qcmd, *head = NULL, *tail = NULL;
    int count = 0;

    for (qcmd = ch->queued_commands; qcmd; qcmd = next_qcmd) {
        next_qcmd = qcmd->next;
        if (!IS_SET(qcmd->queue_flags, QFL_NO_STOP) || force_all) {
            free_queued_command(qcmd);
        } else {
            // Start the new queued_commands list
            if (!head)
               head = qcmd;
        
            // Add this to the new queued_commands list
            if (tail)
                tail->next = qcmd;
            tail = qcmd;
            tail->next = NULL;
        }
        count++;
    }
    ch->queued_commands = head;

    return count;
}

void
push_queued_command(CHAR_DATA *ch, const char *cmd, int queue_flags) {
    QUEUED_CMD *qcmd, *last;

    if( !ch || !cmd || !*cmd ) return;

    CREATE( qcmd, QUEUED_CMD, 1);

    qcmd->command = strdup( cmd );
    qcmd->next = NULL;
    qcmd->queue_flags = queue_flags;

    // if they already have queued commands
    if( ch->queued_commands ) {
	if (IS_SET(queue_flags, QFL_INSERT_HEAD)) {
	    qcmd->next = ch->queued_commands;
            ch->queued_commands = qcmd;
	} else {
        // look for the last queued command
            for( last = ch->queued_commands; last->next != NULL; last = last->next);
            // add it onto the end
            last->next = qcmd;
	}
    }
    else { // no commands queued
        // put it at the front of the list
        ch->queued_commands = qcmd;
    }
}

void
free_queued_command(QUEUED_CMD *qcmd ) {
    if( !qcmd ) return;

    if( qcmd->command ) free( qcmd->command );
    qcmd->next = NULL;
    free(qcmd);
}

char *
pop_queued_command( CHAR_DATA *ch ) {
    static char buf[MAX_STRING_LENGTH];
    QUEUED_CMD *qcmd;

    buf[0] = '\0';
    if( !ch || !ch->queued_commands ) return buf;

    qcmd = ch->queued_commands;
    if( qcmd->command ) 
        strcpy( buf, qcmd->command );

    // move the queue on to the next command
    ch->queued_commands = qcmd->next;
    free_queued_command(qcmd);

    /*gamelogf( "Unqueueing command for %s: %s", 
     IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch), buf );
    */
    return buf;
}

int
parse_command(CHAR_DATA * ch, const char *argument)
{
    return parse_command_with_flags(ch, argument, QFL_DEFAULT);
}

int
parse_command_with_flags(CHAR_DATA * ch, const char *argument, int queue_flags)
{
    int c, cmd = 0;
    char com[MAX_INPUT_LENGTH], list[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH], tmp[MAX_STRING_LENGTH + 256];
    char *argp;

    if (ch->in_room == NULL)
        return TRUE;

    if (!argument || !*argument)
        return TRUE;

    strncpy(arg, argument, MAX_INPUT_LENGTH - 5);
    arg[MAX_INPUT_LENGTH - 5] = '\0';
    argp = arg;

    for (; *argp == ' '; argp++);

    if ((*argp == EMOTE_ABBREV) || (*argp == SAY_ABBREV) || (*argp == OOC_ABBREV)
        || (*argp == IMMCOM_ABBREV) || (*argp == GODCOM_ABBREV) || (*argp == IOOC_ABBREV)
        || (*argp == BLDCOM_ABBREV)) {
        sprintf(tmp, "%c %s", *argp, argp + 1);
        strcpy(argp, tmp);
    } else if (*argp == ASL_ABBREV) {
        sprintf(tmp, "asl %s", argp);
        strcpy(argp, tmp);
    }

    if (IS_SET(queue_flags, QFL_PARSE_ALIAS))
        alias_substitute(ch, argp);

    for (c = 0; argp[c] && (argp[c] != ' '); c++)
        com[c] = argp[c];
    com[c] = '\0';

    if (!*com)
        return TRUE;

    cmd = get_command(com);

    if (ch && !is_cmd_oob_command(ch, com) && ch->specials.act_wait > 0) {
        push_queued_command(ch, argument, queue_flags);
        return FALSE; // We didn't perform anything
    }

    if (ch && ch->desc ) {
        shhlogf("(%s): [%d] %s (%s): \"%s\"",
                (*com && cmd != COMMAND_UNKNOWN) ? command_name[cmd] : "?",
                ch->in_room ? ch->in_room->number : 0, 
                IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch),
                ch->desc && ch->desc->player_info ? ch->desc->player_info->name : "Unknown",
                arg);
    }

    strcpy(list, &argp[c]);

    if (cmd == COMMAND_UNKNOWN) {
        if (!no_specials && special(ch, cmd, arg))
            special(ch, cmd, argument);

        send_to_char(TYPO_MSG, ch);
        return TRUE;
    }

    perform_command(ch, cmd, list, 0);
 
    // apply a one pulse delay before other commands will parse
    if (ch->specials.act_wait <= 0 && !IS_IMMORTAL(ch)) {
        ch->specials.act_wait = 1;
    }
    return TRUE;
}



int
is_cmd_oob_command( CHAR_DATA *ch, const char *cmd ) {
    char _cmd[MAX_STRING_LENGTH];
    int cmd_num;

    // Handle potentially aliased commands
    if (ch && get_alias(ch, cmd)) {
        one_argument(get_alias(ch, cmd), _cmd, sizeof(_cmd));
    } else {
        strcpy( _cmd, cmd );
    }

    // Why not handle unknown commands immediately?
    //  Here's why not:  When you do this, you potentially
    //  eat commands that might be meaningful at the menu
    //  menu prompt after someone quits out.
    if ((cmd_num = get_command(_cmd)) == COMMAND_UNKNOWN) {
        //gamelogf( "unknown command '%s' in is_cmd_oob_command", cmd );
        return 0;
    }

    if (cmd_num == 0) {
        //gamelogf( "cmd_num is 0 for '%s' in is_cmd_oob_command", cmd );
        return 0;
    }

    // Expand this to include your favorite OOB commands (these commands disregard delay)
    if (cmd_info[cmd_num].command_pointer == cmd_alias
        || cmd_info[cmd_num].command_pointer == cmd_assess
        || cmd_info[cmd_num].command_pointer == cmd_beep
        || cmd_info[cmd_num].command_pointer == cmd_brief
        || cmd_info[cmd_num].command_pointer == cmd_bug
        || cmd_info[cmd_num].command_pointer == cmd_compact
        || cmd_info[cmd_num].command_pointer == cmd_disengage
        || cmd_info[cmd_num].command_pointer == cmd_emote
        || cmd_info[cmd_num].command_pointer == cmd_equipment
        || cmd_info[cmd_num].command_pointer == cmd_examine
        || cmd_info[cmd_num].command_pointer == cmd_follow
        || cmd_info[cmd_num].command_pointer == cmd_gone
        || cmd_info[cmd_num].command_pointer == cmd_help
        || cmd_info[cmd_num].command_pointer == cmd_idea
        || cmd_info[cmd_num].command_pointer == cmd_inventory
        || cmd_info[cmd_num].command_pointer == cmd_look
        || cmd_info[cmd_num].command_pointer == cmd_keyword
        || cmd_info[cmd_num].command_pointer == cmd_mercy
        || cmd_info[cmd_num].command_pointer == cmd_motd
        || cmd_info[cmd_num].command_pointer == cmd_news
        || cmd_info[cmd_num].command_pointer == cmd_nosave
        || cmd_info[cmd_num].command_pointer == cmd_ooc
        || cmd_info[cmd_num].command_pointer == cmd_iooc
        || cmd_info[cmd_num].command_pointer == cmd_pemote
        || cmd_info[cmd_num].command_pointer == cmd_recite
        || cmd_info[cmd_num].command_pointer == cmd_reset
        || cmd_info[cmd_num].command_pointer == cmd_return
        || cmd_info[cmd_num].command_pointer == cmd_save
        || cmd_info[cmd_num].command_pointer == cmd_say
        || cmd_info[cmd_num].command_pointer == cmd_score
        || cmd_info[cmd_num].command_pointer == cmd_shout
        || cmd_info[cmd_num].command_pointer == cmd_sing
        || cmd_info[cmd_num].command_pointer == cmd_skills
        || cmd_info[cmd_num].command_pointer == cmd_stat
        || cmd_info[cmd_num].command_pointer == cmd_stop
        || cmd_info[cmd_num].command_pointer == cmd_talk
        || cmd_info[cmd_num].command_pointer == cmd_tell
        || cmd_info[cmd_num].command_pointer == cmd_think
        || cmd_info[cmd_num].command_pointer == cmd_feel
        || cmd_info[cmd_num].command_pointer == cmd_time
        || cmd_info[cmd_num].command_pointer == cmd_typo
        || cmd_info[cmd_num].command_pointer == cmd_weather
        || cmd_info[cmd_num].command_pointer == cmd_whisper
        || cmd_info[cmd_num].command_pointer == cmd_who
        || cmd_info[cmd_num].command_pointer == cmd_wish)
        return 1;

    if (ch && IS_SWITCHED_IMMORTAL(ch) 
     && cmd_info[cmd_num].minimum_level > MORTAL)
        return 1;

    return 0;
}

int
is_oob_command(DESCRIPTOR_DATA * desc, const char *input) {
    char cmd[256];
    // Must have a character to enter oob commands, be connected, be not editing,
    // etc.
    if (!desc->character || desc->str || desc->editing || desc->connected != CON_PLYNG)
        return 0;

    if (desc->showstr_point)
        return 1;

    input = one_argument(input, cmd, sizeof(cmd));

    return is_cmd_oob_command( desc->character, cmd );
}


int
is_number(const char *str)
{
    if (*str == '\0')
        return (0);

    /* allow signed numbers */
    if (*str == '+' || *str == '-')
        str++;

    while (*str != '\0')
        if (!isdigit(*(str++)))
            return (0);

    return (1);
}


int
fill_word(char *argument)
{
    return (search_block(argument, fill, TRUE) >= 0);
}


/* determine if a given string is an abbreviation of another */
int
is_abbrev(const char *arg1, const char *arg2)
{
    if (!*arg1)
        return (0);

    for (; *arg1; arg1++, arg2++)
        if (LOWER(*arg1) != LOWER(*arg2))
            return (0);

    return (1);
}


int
special(CHAR_DATA * ch, Command cmd, const char *arg)
{
    register struct obj_data *i, *next_obj;
    register CHAR_DATA *k, *next_char;
    int j;

    /* special in room? */
    if (execute_room_program(ch->in_room, ch, cmd, arg))
        return TRUE;

    /* special in equipment list? */
    for (j = 0; j < MAX_WEAR; j++) {
        if (ch->equipment[j] != NULL) {
            if (execute_obj_program(ch->equipment[j], ch, 0, cmd, arg)) {
                return (TRUE);
            }
        }
    }

    /* special in inventory? */
    for (i = ch->carrying; i; i = next_obj) {
        next_obj = i->next_content;
        if (execute_obj_program(i, ch, 0, cmd, arg))
            return (TRUE);
    }

    if (!ch->in_room)
        return FALSE;

    /* special in npc present? */
    for (k = ch->in_room->people; k; k = next_char) {
        next_char = k->next_in_room;
        if (execute_npc_program(k, ch, cmd, arg))
            return TRUE;
    }

    /* special in object present? */
    for (i = ch->in_room->contents; i; i = next_obj) {
        next_obj = i->next_content;
        if (execute_obj_program(i, ch, 0, cmd, arg))
            return TRUE;
    }

    return FALSE;

}

void
assign_command_pointers(void)
{
//    FILE *cmdfile = fopen(COMMAND_FILE, "r");
//    int position;

    memset(&cmd_info, 0, sizeof(cmd_info));

/*
    if (cmdfile == NULL) {
        fprintf(stderr, "fopen %s failed: %s\n", COMMAND_FILE, strerror(errno));
//        exit( -1 );
    }

    while (cmdfile && !feof(cmdfile)) {
        char cmdline[255], cmdname[255], postext[255], cmdlang[255], scriptfunc[255],
            scriptfile[255];
        fgets(cmdline, sizeof(cmdline), cmdfile);
        sscanf(cmdline, "%[^,]s,%[^,]s,%[^,]s,%[^,]s,%[^,]s", cmdname, postext, cmdlang, scriptfunc,
               scriptfile);
    }
    fclose(cmdfile);
*/

    COMMANDO(CMD_NORTH, POSITION_STANDING, cmd_move, 0);        /* north  */
    COMMANDO(CMD_EAST, POSITION_STANDING, cmd_move, 0); /* east   */
    COMMANDO(CMD_SOUTH, POSITION_STANDING, cmd_move, 0);        /* south  */
    COMMANDO(CMD_WEST, POSITION_STANDING, cmd_move, 0); /* west   */
    COMMANDO(CMD_UP, POSITION_STANDING, cmd_move, 0);   /* up */
    COMMANDO(CMD_DOWN, POSITION_STANDING, cmd_move, 0); /* down */
    COMMANDO(CMD_EMPTY, POSITION_SITTING, cmd_empty, 0);
    COMMANDO(CMD_ENTER, POSITION_STANDING, cmd_enter, 0);
    COMMANDO(CMD_EXITS, POSITION_RESTING, cmd_exits, 0);
    COMMANDO(CMD_KISS, POSITION_RESTING, cmd_action, 0);        /* kiss */
    COMMANDO(CMD_GET, POSITION_RESTING, cmd_get, 0);
    COMMANDO(CMD_DRINK, POSITION_RESTING, cmd_drink, 0);
    COMMANDO(CMD_EAT, POSITION_RESTING, cmd_eat, 0);
    COMMANDO(CMD_WEAR, POSITION_RESTING, cmd_wear, 0);
    COMMANDO(CMD_WIELD, POSITION_RESTING, cmd_wield, 0);
    COMMANDO(CMD_LOOK, POSITION_RESTING, cmd_look, 0);
    COMMANDO(CMD_SCORE, POSITION_DEAD, cmd_score, 0);
    COMMANDO(CMD_SAY, POSITION_RESTING, cmd_say, 0);
    COMMANDO(CMD_SHOUT, POSITION_RESTING, cmd_shout, MORTAL);
    COMMANDO(CMD_REST, POSITION_RESTING, cmd_rest, MORTAL);
    COMMANDO(CMD_INVENTORY, POSITION_RESTING, cmd_inventory, 0);
    COMMANDO(CMD_QUI, POSITION_DEAD, cmd_qui, 0);
    COMMANDO(CMD_SNORT, POSITION_SLEEPING, cmd_not_here, 0);    /* snort */
    COMMANDO(CMD_SMILE, POSITION_RESTING, cmd_action, 0);
    COMMANDO(CMD_DANCE, POSITION_STANDING, cmd_not_here, 0);    /* dance */
    COMMANDO(CMD_KILL, POSITION_FIGHTING, cmd_kill, 0);
    COMMANDO(CMD_CACKLE, POSITION_RESTING, cmd_not_here, 0);
    COMMANDO(CMD_LAUGH, POSITION_RESTING, cmd_action, 0);       /* laugh */
    COMMANDO(CMD_GIGGLE, POSITION_RESTING, cmd_not_here, 0);    /* giggle */
    COMMANDO(CMD_SHAKE, POSITION_RESTING, cmd_action, 0);       /* shake */
    COMMANDO(CMD_SHUFFLE, POSITION_RESTING, cmd_not_here, MORTAL);   /* shuffle */
    COMMANDO(CMD_GROWL, POSITION_RESTING, cmd_not_here, 0);     /* growl */
    COMMANDO(CMD_SCREAM, POSITION_RESTING, cmd_not_here, 0);    /* scream */
    COMMANDO(CMD_INSULT, POSITION_RESTING, cmd_insult, CREATOR);        /* insult */
    COMMANDO(CMD_COMFORT, POSITION_RESTING, cmd_action, 0);     /* comfort */
    COMMANDO(CMD_NOD, POSITION_RESTING, cmd_action, 0); /* nod */
    COMMANDO(CMD_SIGH, POSITION_RESTING, cmd_action, 0);        /* sigh */
    COMMANDO(CMD_SLEE, POSITION_RESTING, cmd_slee, 0);
    COMMANDO(CMD_HELP, POSITION_DEAD, cmd_help, 0);
    COMMANDO(CMD_WHO, POSITION_DEAD, cmd_who, MORTAL);
    COMMANDO(CMD_EMOTE, POSITION_DEAD, cmd_emote, MORTAL);
    COMMANDO(CMD_ECHO, POSITION_SLEEPING, cmd_echo, CREATOR);
    COMMANDO(CMD_STAND, POSITION_RESTING, cmd_stand, 0);
    COMMANDO(CMD_SIT, POSITION_RESTING, cmd_sit, 0);
    COMMANDO(CMD_RS, POSITION_RESTING, cmd_rs, MORTAL);
    COMMANDO(CMD_SLEEP, POSITION_SLEEPING, cmd_sleep, 0);
    COMMANDO(CMD_WAKE, POSITION_SLEEPING, cmd_wake, 0);
    COMMANDO(CMD_FORCE, POSITION_SLEEPING, cmd_force, CREATOR);
    COMMANDO(CMD_TRANSFER, POSITION_SLEEPING, cmd_tran, MORTAL);
    COMMANDO(CMD_HUG, POSITION_RESTING, cmd_action, 0); /* hug */
    COMMANDO(CMD_SERVER, POSITION_RESTING, cmd_server, HIGHLORD);
    COMMANDO(CMD_CUDDLE, POSITION_RESTING, cmd_action, 0);      /* cuddle */
    COMMANDO(CMD_STOW, POSITION_RESTING, cmd_sheathe, MORTAL);	/* stow */
    COMMANDO(CMD_CRY, POSITION_RESTING, cmd_not_here, 0);       /* cry */
    COMMANDO(CMD_NEWS, POSITION_SLEEPING, cmd_news, 0);
    COMMANDO(CMD_EQUIPMENT, POSITION_RESTING, cmd_equipment, 0);
    COMMANDO(CMD_OLDBUY, POSITION_STANDING, cmd_not_here, 0);   /* oldbuy */
    COMMANDO(CMD_SELL, POSITION_STANDING, cmd_not_here, 0);     /* sell */
    COMMANDO(CMD_VALUE, POSITION_RESTING, cmd_skill, MORTAL);   /* value */
    COMMANDO(CMD_LIST, POSITION_RESTING, cmd_list, 0);
    COMMANDO(CMD_DROP, POSITION_RESTING, cmd_drop, 0);
    COMMANDO(CMD_GOTO, POSITION_SLEEPING, cmd_goto, CREATOR);
    COMMANDO(CMD_WEATHER, POSITION_RESTING, cmd_weather, 0);
    COMMANDO(CMD_READ, POSITION_RESTING, cmd_read, 0);
    COMMANDO(CMD_POUR, POSITION_SITTING, cmd_pour, 0);
    COMMANDO(CMD_GRAB, POSITION_RESTING, cmd_grab, 0);
    COMMANDO(CMD_REMOVE, POSITION_RESTING, cmd_remove, 0);
    COMMANDO(CMD_PUT, POSITION_RESTING, cmd_put, 0);
    COMMANDO(CMD_SHUTDOW, POSITION_DEAD, cmd_shutdow, OVERLORD);
    COMMANDO(CMD_SAVE, POSITION_SLEEPING, cmd_save, 0);
    COMMANDO(CMD_HIT, POSITION_FIGHTING, cmd_hit, 0);
    COMMANDO(CMD_OCOPY, POSITION_RESTING, cmd_ocopy, CREATOR);
    COMMANDO(CMD_GIVE, POSITION_RESTING, cmd_give, 0);
    COMMANDO(CMD_QUIT, POSITION_DEAD, cmd_quit, 0);
    COMMANDO(CMD_STAT, POSITION_DEAD, cmd_stat, MORTAL);
    COMMANDO(CMD_RSTAT, POSITION_DEAD, cmd_rstat, CREATOR);
    COMMANDO(CMD_TIME, POSITION_DEAD, cmd_time, 0);
    COMMANDO(CMD_PURGE, POSITION_DEAD, cmd_purge, CREATOR);
    COMMANDO(CMD_SHUTDOWN, POSITION_DEAD, cmd_shutdown, OVERLORD);
    COMMANDO(CMD_IDEA, POSITION_DEAD, cmd_idea, 0);
    COMMANDO(CMD_TYPO, POSITION_DEAD, cmd_typo, 0);
    COMMANDO(CMD_BUY, POSITION_RESTING, cmd_buy, 0);
    COMMANDO(CMD_WHISPER, POSITION_RESTING, cmd_whisper, 0);
    COMMANDO(CMD_CAST, POSITION_FIGHTING, cmd_cast, MORTAL);
    COMMANDO(CMD_AT, POSITION_DEAD, cmd_at, CREATOR);
    COMMANDO(CMD_ASK, POSITION_RESTING, cmd_tell, 0);   /* ask? */
    COMMANDO(CMD_ORDER, POSITION_RESTING, cmd_order, MORTAL);
    COMMANDO(CMD_SIP, POSITION_RESTING, cmd_sip, 0);
    COMMANDO(CMD_TASTE, POSITION_RESTING, cmd_taste, 0);
    COMMANDO(CMD_SNOOP, POSITION_DEAD, cmd_snoop, CREATOR);
    COMMANDO(CMD_FOLLOW, POSITION_RESTING, cmd_follow, 0);
    COMMANDO(CMD_RENT, POSITION_STANDING, cmd_not_here, MORTAL);
    COMMANDO(CMD_OFFER, POSITION_SITTING, cmd_offer, MORTAL);
    COMMANDO(CMD_POKE, POSITION_RESTING, cmd_not_here, 0);      /* poke */
    COMMANDO(CMD_ADVANCE, POSITION_DEAD, cmd_advance, OVERLORD);
    COMMANDO(CMD_ACCUSE, POSITION_SITTING, cmd_not_here, 0);    /* accuse */
    COMMANDO(CMD_GRIN, POSITION_RESTING, cmd_action, 0);
    COMMANDO(CMD_BOW, POSITION_STANDING, cmd_not_here, 0);      /* bow */
    COMMANDO(CMD_OPEN, POSITION_SITTING, cmd_open, 0);
    COMMANDO(CMD_CLOSE, POSITION_SITTING, cmd_close, 0);
    COMMANDO(CMD_LOCK, POSITION_SITTING, cmd_lock, 0);
    COMMANDO(CMD_UNLOCK, POSITION_SITTING, cmd_unlock, 0);
    COMMANDO(CMD_LEAVE, POSITION_STANDING, cmd_leave, 0);
    COMMANDO(CMD_APPLAUD, POSITION_RESTING, cmd_not_here, 0);   /* applaud */
    COMMANDO(CMD_BLUSH, POSITION_RESTING, cmd_not_here, 0);     /* blush */
    COMMANDO(CMD_REJUVENATE, POSITION_RESTING, cmd_skill, 0);   /* rejuvenate */
    COMMANDO(CMD_CHUCKLE, POSITION_RESTING, cmd_action, 0);
    COMMANDO(CMD_CLAP, POSITION_RESTING, cmd_not_here, 0);      /* clap */
    COMMANDO(CMD_COUGH, POSITION_RESTING, cmd_action, 0);       /* cough */
    COMMANDO(CMD_CURTSEY, POSITION_STANDING, cmd_not_here, 0);  /* curtsey */
    COMMANDO(CMD_CEASE, POSITION_RESTING, cmd_cease, 0);        /* cease */
    COMMANDO(CMD_FLIP, POSITION_STANDING, cmd_not_here, 0);     /* flip */
    COMMANDO(CMD_STOP, POSITION_RESTING, cmd_stop, 0);          /* stop */
    COMMANDO(CMD_FROWN, POSITION_RESTING, cmd_action, 0);       /* frown */
    COMMANDO(CMD_GASP, POSITION_RESTING, cmd_not_here, 0);      /* gasp */
    COMMANDO(CMD_GLARE, POSITION_RESTING, cmd_not_here, 0);     /* glare */
    COMMANDO(CMD_GROAN, POSITION_RESTING, cmd_not_here, 0);     /* groan */
    COMMANDO(CMD_SLIP, POSITION_RESTING, cmd_put, MORTAL);      /* slip */
    COMMANDO(CMD_HICCUP, POSITION_RESTING, cmd_not_here, 0);    /* hiccup */
    COMMANDO(CMD_LICK, POSITION_RESTING, cmd_not_here, 0);      /* lick */
    COMMANDO(CMD_GESTURE, POSITION_RESTING, cmd_not_here, 0);   /* gesture */
    COMMANDO(CMD_MOAN, POSITION_RESTING, cmd_not_here, 0);      /* moan */
    COMMANDO(CMD_READY, POSITION_RESTING, cmd_draw, MORTAL);	/* ready */
    COMMANDO(CMD_POUT, POSITION_RESTING, cmd_not_here, 0);      /* pout */
    COMMANDO(CMD_STUDY, POSITION_RESTING, cmd_not_here, 0);     /* study */
    COMMANDO(CMD_RUFFLE, POSITION_STANDING, cmd_not_here, 0);   /* ruffle */
    COMMANDO(CMD_SHIVER, POSITION_RESTING, cmd_not_here, 0);    /* shiver */
    COMMANDO(CMD_SHRUG, POSITION_RESTING, cmd_action, 0);       /* shrug */
    COMMANDO(CMD_SING, POSITION_RESTING, cmd_sing, MORTAL);     /* sing */
    COMMANDO(CMD_SLAP, POSITION_RESTING, cmd_not_here, 0);      /* slap */
    COMMANDO(CMD_SMIRK, POSITION_RESTING, cmd_not_here, 0);     /* smirk */
    COMMANDO(CMD_SNAP, POSITION_RESTING, cmd_not_here, 0);      /* snap */
    COMMANDO(CMD_SNEEZE, POSITION_RESTING, cmd_action, 0);      /* sneeze */
    COMMANDO(CMD_SNICKER, POSITION_RESTING, cmd_action, 0);     /* snicker */
    COMMANDO(CMD_SNIFF, POSITION_RESTING, cmd_sniff, 0);        /* sniff */
    COMMANDO(CMD_SNORE, POSITION_SLEEPING, cmd_action, 0);      /* snore */
    COMMANDO(CMD_SPIT, POSITION_STANDING, cmd_action, 0);       /* spit */
    COMMANDO(CMD_SQUEEZE, POSITION_RESTING, cmd_action, 0);     /* squeeze */
    COMMANDO(CMD_STARE, POSITION_RESTING, cmd_action, 0);       /* stare */
    COMMANDO(CMD_BIOGRAPHY, POSITION_DEAD, cmd_biography, MORTAL);      /* biography */
    COMMANDO(CMD_THANK, POSITION_RESTING, cmd_action, 0);       /* thank */
    COMMANDO(CMD_TWIDDLE, POSITION_RESTING, cmd_action, 0);     /* twiddle */
    COMMANDO(CMD_WAVE, POSITION_RESTING, cmd_action, 0);        /* wave */
    COMMANDO(CMD_WHISTLE, POSITION_RESTING, cmd_action, 0);     /* whistle */
    COMMANDO(CMD_WIGGLE, POSITION_STANDING, cmd_action, 0);     /* wiggle */
    COMMANDO(CMD_WINK, POSITION_RESTING, cmd_action, 0);        /* wink */
    COMMANDO(CMD_YAWN, POSITION_RESTING, cmd_action, 0);        /* yawn */
    COMMANDO(CMD_NOSAVE, POSITION_DEAD, cmd_nosave, MORTAL);
    COMMANDO(CMD_WRITE, POSITION_RESTING, cmd_write, MORTAL);
    COMMANDO(CMD_HOLD, POSITION_RESTING, cmd_grab, MORTAL);
    COMMANDO(CMD_FLEE, POSITION_FIGHTING, cmd_flee, MORTAL);
    COMMANDO(CMD_SNEAK, POSITION_STANDING, cmd_sneak, MORTAL);
    COMMANDO(CMD_HIDE, POSITION_STANDING, cmd_skill, MORTAL);   /* hide     */
    COMMANDO(CMD_BACKSTAB, POSITION_STANDING, cmd_skill, MORTAL);       /* backstab */
    COMMANDO(CMD_PICK, POSITION_STANDING, cmd_skill, MORTAL);   /* pick     */
    COMMANDO(CMD_STEAL, POSITION_STANDING, cmd_skill, MORTAL);  /* steal    */
    COMMANDO(CMD_BASH, POSITION_FIGHTING, cmd_skill, MORTAL);   /* bash     */
    COMMANDO(CMD_RESCUE, POSITION_FIGHTING, cmd_skill, MORTAL); /* rescue   */
    COMMANDO(CMD_KICK, POSITION_FIGHTING, cmd_skill, MORTAL);   /* kick     */
    COMMANDO(CMD_COERCE, POSITION_RESTING, cmd_skill, CREATOR); /* coerce   */
    COMMANDO(CMD_UNLATCH, POSITION_SITTING, cmd_open, MORTAL);   /* unlatch  */
    COMMANDO(CMD_LATCH, POSITION_SITTING, cmd_close, MORTAL);        /* latch  */
    COMMANDO(CMD_TICKLE, POSITION_RESTING, cmd_action, MORTAL); /* tickle   */
    COMMANDO(CMD_MESMERIZE, POSITION_RESTING, cmd_skill, CREATOR);      /* mesmerize */
    COMMANDO(CMD_EXAMINE, POSITION_RESTING, cmd_examine, MORTAL);
    COMMANDO(CMD_TAKE, POSITION_RESTING, cmd_get, 0);
    COMMANDO(CMD_DONATE, POSITION_RESTING, cmd_donate, 0);
    COMMANDO(CMD_SAY_SHORT, POSITION_RESTING, cmd_say, 0);
    COMMANDO(CMD_GUARDING, POSITION_STANDING, cmd_guard, MORTAL);
    COMMANDO(CMD_CURSE, POSITION_RESTING, cmd_action, 0);       /* curse    */
    COMMANDO(CMD_USE, POSITION_SITTING, cmd_use, MORTAL);
    COMMANDO(CMD_WHERE, POSITION_SLEEPING, cmd_where, STORYTELLER);
    COMMANDO(CMD_REROLL, POSITION_DEAD, cmd_reroll, MORTAL);
    COMMANDO(CMD_PRAY, POSITION_SITTING, cmd_action, 0);        /* pray     */
    COMMANDO(CMD_EMOTE_SHORT, POSITION_DEAD, cmd_emote, MORTAL);
    COMMANDO(CMD_BEG, POSITION_RESTING, cmd_action, 0); /* beg      */
    COMMANDO(CMD_BLEED, POSITION_RESTING, cmd_not_here, 0);     /* bleed    */
    COMMANDO(CMD_CRINGE, POSITION_RESTING, cmd_not_here, 0);    /* cringe   */
    COMMANDO(CMD_DAYDREAM, POSITION_SLEEPING, cmd_not_here, 0); /* daydream */
    COMMANDO(CMD_FUME, POSITION_RESTING, cmd_not_here, 0);      /* fume     */
    COMMANDO(CMD_GROVEL, POSITION_RESTING, cmd_action, 0);      /* grovel   */
    COMMANDO(CMD_VANISH, POSITION_RESTING, cmd_skill, MORTAL);  /* hop      */
    COMMANDO(CMD_NUDGE, POSITION_RESTING, cmd_action, 0);       /* nudge    */
    COMMANDO(CMD_PEER, POSITION_RESTING, cmd_action, 0);        /* peer     */
    COMMANDO(CMD_POINT, POSITION_RESTING, cmd_action, 0);       /* point    */
    COMMANDO(CMD_PONDER, POSITION_RESTING, cmd_action, 0);      /* ponder   */
    COMMANDO(CMD_PUNCH, POSITION_RESTING, cmd_action, 0);       /* punch    */
    COMMANDO(CMD_SNARL, POSITION_RESTING, cmd_action, 0);       /* snarl    */
    COMMANDO(CMD_SPANK, POSITION_RESTING, cmd_action, 0);       /* spank    */
    COMMANDO(CMD_STEAM, POSITION_RESTING, cmd_action, 0);       /* steam    */
    COMMANDO(CMD_TACKLE, POSITION_RESTING, cmd_action, 0);      /* tackle   */
    COMMANDO(CMD_TAUNT, POSITION_RESTING, cmd_action, 0);       /* taunt    */
    COMMANDO(CMD_THINK, POSITION_DEAD, cmd_think, 0);
    COMMANDO(CMD_WHINE, POSITION_RESTING, cmd_action, 0);       /* whine    */
    COMMANDO(CMD_WORSHIP, POSITION_RESTING, cmd_action, 0);     /* worship  */
    COMMANDO(CMD_FEEL, POSITION_DEAD, cmd_feel, 0);
    COMMANDO(CMD_BRIEF, POSITION_DEAD, cmd_brief, 0);
    COMMANDO(CMD_WIZLIST, POSITION_DEAD, cmd_wizlist, 0);
    COMMANDO(CMD_CONSIDER, POSITION_RESTING, cmd_consider, CREATOR);
    COMMANDO(CMD_CUNSET, POSITION_DEAD, cmd_cunset, CREATOR);
    COMMANDO(CMD_RESTORE, POSITION_DEAD, cmd_restore, CREATOR);
    COMMANDO(CMD_RETURN, POSITION_DEAD, cmd_return, 0);
    COMMANDO(CMD_SWITCH, POSITION_DEAD, cmd_switch, CREATOR);
    COMMANDO(CMD_QUAFF, POSITION_RESTING, cmd_quaff, 0);
    COMMANDO(CMD_RECITE, POSITION_RESTING, cmd_recite, 0);
    COMMANDO(CMD_USERS, POSITION_DEAD, cmd_users, HIGHLORD);
    COMMANDO(CMD_POSE, POSITION_STANDING, cmd_pose, 0);
    COMMANDO(CMD_AWARD, POSITION_SLEEPING, cmd_award, CREATOR);
    COMMANDO(CMD_WIZHELP, POSITION_SLEEPING, cmd_wizhelp, CREATOR);
    COMMANDO(CMD_CREDITS, POSITION_DEAD, cmd_credits, 0);
    COMMANDO(CMD_COMPACT, POSITION_DEAD, cmd_compact, 0);
    COMMANDO(CMD_SKILLS, POSITION_DEAD, cmd_skills, 0);
    COMMANDO(CMD_JUMP, POSITION_RESTING, cmd_not_here, MORTAL); /* jump    */
    COMMANDO(CMD_MESSAGE, POSITION_RESTING, cmd_not_here, MORTAL);      /* message */
    COMMANDO(CMD_RCOPY, POSITION_DEAD, cmd_rcopy, CREATOR);
    COMMANDO(CMD_DISARM, POSITION_FIGHTING, cmd_skill, MORTAL); /* disarm  */
    COMMANDO(CMD_KNOCK, POSITION_STANDING, cmd_knock, MORTAL);
    COMMANDO(CMD_LISTEN, POSITION_RESTING, cmd_listen, MORTAL);  /* listen  */
    COMMANDO(CMD_TRAP, POSITION_FIGHTING, cmd_skill, MORTAL);   /* trap    */
    COMMANDO(CMD_TENEBRAE, POSITION_FIGHTING, cmd_tenebrae, MORTAL);
    COMMANDO(CMD_HUNT, POSITION_FIGHTING, cmd_skill, MORTAL);   /* hunt    */
    COMMANDO(CMD_RSAVE, POSITION_DEAD, cmd_rsave, CREATOR);
    COMMANDO(CMD_KNEEL, POSITION_SLEEPING, cmd_not_here, MORTAL);
    COMMANDO(CMD_BITE, POSITION_FIGHTING, cmd_bite, MORTAL);
    COMMANDO(CMD_MAKE, POSITION_RESTING, cmd_make, MORTAL);
    COMMANDO(CMD_SMOKE, POSITION_RESTING, cmd_smoke, MORTAL);
    COMMANDO(CMD_PEEK, POSITION_RESTING, cmd_peek, MORTAL);
    COMMANDO(CMD_RZSAVE, POSITION_RESTING, cmd_rzsave, CREATOR);
    COMMANDO(CMD_RZCLEAR, POSITION_RESTING, cmd_rzclear, CREATOR);
    COMMANDO(CMD_MCOPY, POSITION_DEAD, cmd_mcopy, CREATOR);
    COMMANDO(CMD_RCREATE, POSITION_DEAD, cmd_rcreate, CREATOR);
    COMMANDO(CMD_DAMAGE, POSITION_DEAD, cmd_damage, CREATOR);
    COMMANDO(CMD_IMMCOM, POSITION_SLEEPING, cmd_immcom, MORTAL);
    COMMANDO(CMD_IMMCOM_SHORT, POSITION_SLEEPING, cmd_immcom, MORTAL);
    COMMANDO(CMD_ZASSIGN, POSITION_SLEEPING, cmd_zassign, OVERLORD);
    COMMANDO(CMD_SHOW, POSITION_SLEEPING, cmd_show, CREATOR);
    COMMANDO(CMD_OSAVE, POSITION_SLEEPING, cmd_osave, CREATOR);
    COMMANDO(CMD_OCREATE, POSITION_SLEEPING, cmd_ocreate, CREATOR);
    COMMANDO(CMD_TITLE, POSITION_SLEEPING, cmd_not_here, MORTAL);
    COMMANDO(CMD_PILOT, POSITION_SITTING, cmd_pilot, MORTAL);
    COMMANDO(CMD_OSET, POSITION_SLEEPING, cmd_oset, CREATOR);
    COMMANDO(CMD_NUKE, POSITION_SLEEPING, cmd_nuke, HIGHLORD);
    COMMANDO(CMD_SPLIT, POSITION_FIGHTING, cmd_skill, MORTAL);  /* split  */
    COMMANDO(CMD_ZSAVE, POSITION_RESTING, cmd_zsave, HIGHLORD);
    COMMANDO(CMD_BACKSAVE, POSITION_RESTING, cmd_backsave, CREATOR);
    COMMANDO(CMD_MSAVE, POSITION_RESTING, cmd_msave, CREATOR);
    COMMANDO(CMD_CSET, POSITION_RESTING, cmd_cset, CREATOR);
    COMMANDO(CMD_MCREATE, POSITION_RESTING, cmd_mcreate, CREATOR);
    COMMANDO(CMD_RZRESET, POSITION_RESTING, cmd_rzreset, CREATOR)
    COMMANDO(CMD_ZCLEAR, POSITION_RESTING, cmd_zclear, CREATOR);
    COMMANDO(CMD_ZRESET, POSITION_RESTING, cmd_zreset, CREATOR);
    COMMANDO(CMD_MDUP, POSITION_RESTING, cmd_mdup, CREATOR);
    COMMANDO(CMD_LOG, POSITION_RESTING, cmd_log, CREATOR);
    COMMANDO(CMD_BEHEAD, POSITION_RESTING, cmd_behead, MORTAL);
    COMMANDO(CMD_PLIST, POSITION_RESTING, cmd_plist, HIGHLORD);
    COMMANDO(CMD_CRIMINAL, POSITION_RESTING, cmd_criminal, CREATOR);
    COMMANDO(CMD_FREEZE, POSITION_RESTING, cmd_freeze, HIGHLORD);
    COMMANDO(CMD_BSET, POSITION_RESTING, cmd_bset, CREATOR);
    COMMANDO(CMD_GONE, POSITION_DEAD, cmd_gone, MORTAL);
    COMMANDO(CMD_DRAW, POSITION_RESTING, cmd_draw, MORTAL);
    COMMANDO(CMD_ZSET, POSITION_RESTING, cmd_zset, CREATOR);
    COMMANDO(CMD_DELETE, POSITION_RESTING, cmd_delete, MORTAL);
    COMMANDO(CMD_RACES, POSITION_RESTING, cmd_races, STORYTELLER);
    COMMANDO(CMD_HAYYEH, POSITION_RESTING, cmd_hayyeh, MORTAL);
    COMMANDO(CMD_APPLIST, POSITION_DEAD, cmd_nonet, STORYTELLER);
    COMMANDO(CMD_PULL, POSITION_RESTING, cmd_draw, MORTAL);
    COMMANDO(CMD_REVIEW, POSITION_DEAD, cmd_review, MORTAL);
    COMMANDO(CMD_BSAVE, POSITION_RESTING, cmd_bsave, OVERLORD);
    COMMANDO(CMD_CRAFT, POSITION_RESTING, cmd_craft, MORTAL);
    COMMANDO(CMD_PROMPT, POSITION_DEAD, cmd_prompt, MORTAL);
    COMMANDO(CMD_MOTD, POSITION_DEAD, cmd_motd, MORTAL);
    COMMANDO(CMD_TAN, POSITION_RESTING, cmd_nonet, MORTAL);
    COMMANDO(CMD_INVIS, POSITION_RESTING, cmd_invis, STORYTELLER);
    /* COMMANDO(CMD_ECHOALL            , POSITION_RESTING, cmd_asl, HIGHLORD); */
    COMMANDO(CMD_PATH, POSITION_RESTING, cmd_path, CREATOR);
    COMMANDO(CMD_EP, POSITION_RESTING, cmd_wield, MORTAL);
    COMMANDO(CMD_ES, POSITION_RESTING, cmd_grab, MORTAL);
    COMMANDO(CMD_REPLY, POSITION_RESTING, cmd_reply, MORTAL);
    COMMANDO(CMD_QUIET, POSITION_DEAD, cmd_quiet, BUILDER);
    COMMANDO(CMD__WSAVE, POSITION_DEAD, cmd_wsave, OVERLORD);
    COMMANDO(CMD_GODCOM, POSITION_SLEEPING, cmd_godcom, MORTAL);
    COMMANDO(CMD_GODCOM_SHORT, POSITION_SLEEPING, cmd_godcom, MORTAL);
    COMMANDO(CMD__WCLEAR, POSITION_DEAD, cmd_wclear, OVERLORD);
    COMMANDO(CMD__WRESET, POSITION_DEAD, cmd_wreset, OVERLORD);
    COMMANDO(CMD__WZSAVE, POSITION_DEAD, cmd_wzsave, OVERLORD);
    COMMANDO(CMD__WUPDATE, POSITION_DEAD, cmd_wupdate, OVERLORD);
    COMMANDO(CMD_BANDAGE, POSITION_SITTING, cmd_skill, MORTAL); /* bandage */
    COMMANDO(CMD_TEACH, POSITION_STANDING, cmd_teach, MORTAL);
    COMMANDO(CMD_DIG, POSITION_STANDING, cmd_not_here, MORTAL); /* dig    */
    COMMANDO(CMD_ALIAS, POSITION_DEAD, cmd_alias, MORTAL);
    COMMANDO(CMD_SYSTEM, POSITION_DEAD, cmd_system, HIGHLORD);
    COMMANDO(CMD_NONET, POSITION_DEAD, cmd_nonet, CREATOR);
  /*  COMMANDO(CMD_DNSTAT, POSITION_DEAD, cmd_dnstat, CREATOR); */
    COMMANDO(CMD_PLMAX, POSITION_DEAD, cmd_plmax, OVERLORD);
    COMMANDO(CMD_IDLE, POSITION_RESTING, cmd_idle, STORYTELLER);
    COMMANDO(CMD_IMMCMDS, POSITION_RESTING, cmd_immcmds, LEGEND);
    COMMANDO(CMD_FILL, POSITION_RESTING, cmd_fill, MORTAL);
    COMMANDO(CMD_LS, POSITION_RESTING, cmd_ls, CREATOR);
    COMMANDO(CMD_PAGE, POSITION_RESTING, cmd_page, CREATOR);
    COMMANDO(CMD_EDIT, POSITION_RESTING, cmd_edit, CREATOR);
    COMMANDO(CMD_RESET, POSITION_DEAD, cmd_reset, MORTAL);
    COMMANDO(CMD_BEEP, POSITION_RESTING, cmd_beep, MORTAL);
    COMMANDO(CMD_RECRUIT, POSITION_RESTING, cmd_recruit, MORTAL);
    COMMANDO(CMD_REBEL, POSITION_RESTING, cmd_rebel, MORTAL);
    COMMANDO(CMD_DUMP, POSITION_RESTING, cmd_dump, MORTAL);
    COMMANDO(CMD_CLANLEADER, POSITION_RESTING, cmd_clanleader, CREATOR);
    COMMANDO(CMD_THROW, POSITION_STANDING, cmd_skill, MORTAL);  /* throw   */
    COMMANDO(CMD_MAIL, POSITION_SLEEPING, cmd_mail, MORTAL);
    COMMANDO(CMD_SUBDUE, POSITION_STANDING, cmd_skill, MORTAL); /* subdue  */
    COMMANDO(CMD_PROMOTE, POSITION_SITTING, cmd_promote, MORTAL);
    COMMANDO(CMD_DEMOTE, POSITION_SITTING, cmd_demote, MORTAL);
    COMMANDO(CMD_WISH, POSITION_DEAD, cmd_wish, MORTAL);
    COMMANDO(CMD_VIS, POSITION_RESTING, cmd_vis, MORTAL);
    COMMANDO(CMD_JUNK, POSITION_RESTING, cmd_junk, MORTAL);
    COMMANDO(CMD_DEAL, POSITION_RESTING, cmd_not_here, MORTAL);
    COMMANDO(CMD_OUPDATE, POSITION_DEAD, cmd_oupdate, CREATOR);
    COMMANDO(CMD_MUPDATE, POSITION_DEAD, cmd_mupdate, CREATOR);
    COMMANDO(CMD_CWHO, POSITION_DEAD, cmd_cwho, MORTAL);
    COMMANDO(CMD_NOTITLE, POSITION_DEAD, cmd_not_here, MORTAL);
    COMMANDO(CMD_HEAR, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_WALK, POSITION_RESTING, cmd_walk, MORTAL);
    COMMANDO(CMD_RUN, POSITION_RESTING, cmd_run, MORTAL);
    COMMANDO(CMD_BALANCE, POSITION_SITTING, cmd_not_here, MORTAL);
    COMMANDO(CMD_DEPOSIT, POSITION_SITTING, cmd_not_here, MORTAL);
    COMMANDO(CMD_WITHDRAW, POSITION_SITTING, cmd_not_here, MORTAL);
    COMMANDO(CMD_APPROVE, POSITION_DEAD, cmd_nonet, STORYTELLER);
    COMMANDO(CMD_REJECT, POSITION_DEAD, cmd_nonet, STORYTELLER);
    COMMANDO(CMD_RUPDATE, POSITION_DEAD, cmd_rupdate, CREATOR);
    COMMANDO(CMD_REACH, POSITION_RESTING, cmd_not_here, MORTAL);
    COMMANDO(CMD_BREAK, POSITION_RESTING, cmd_break, MORTAL);
    COMMANDO(CMD_CONTACT, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_LOCATE, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD__UNAPPROVE, POSITION_DEAD, cmd_unapprove, OVERLORD);
    COMMANDO(CMD_NOWISH, POSITION_DEAD, cmd_nowish, CREATOR);
    COMMANDO(CMD_SYMBOL, POSITION_DEAD, cmd_symbol, MORTAL);
    COMMANDO(CMD_PUSH, POSITION_SITTING, cmd_not_here, MORTAL); /* push   */
    COMMANDO(CMD_MEMORY, POSITION_DEAD, cmd_memory, OVERLORD);
    COMMANDO(CMD_MOUNT, POSITION_FIGHTING, cmd_mount, MORTAL);
    COMMANDO(CMD_DISMOUNT, POSITION_FIGHTING, cmd_dismount, MORTAL);
    COMMANDO(CMD_ZOPEN, POSITION_DEAD, cmd_zopen, CREATOR);
    COMMANDO(CMD_ZCLOSE, POSITION_DEAD, cmd_zclose, CREATOR);
    COMMANDO(CMD_RDELETE, POSITION_DEAD, cmd_rdelete, CREATOR);
    COMMANDO(CMD_OSTAT, POSITION_DEAD, cmd_ostat, CREATOR);
    COMMANDO(CMD_BARTER, POSITION_RESTING, cmd_barter, MORTAL);
    COMMANDO(CMD_SETTIME, POSITION_DEAD, cmd_nonet, OVERLORD);
    COMMANDO(CMD_SKILLRESET, POSITION_DEAD, cmd_skillreset, CREATOR);
    COMMANDO(CMD_GRP, POSITION_DEAD, cmd_grp, OVERLORD);
    COMMANDO(CMD_CSTAT, POSITION_DEAD, cmd_cstat, CREATOR);
    COMMANDO(CMD_CALL, POSITION_SITTING, cmd_call, MORTAL);
    COMMANDO(CMD_RELIEVE, POSITION_SITTING, cmd_relieve, MORTAL);
    COMMANDO(CMD_INCRIMINATE, POSITION_RESTING, cmd_incriminate, MORTAL);
    COMMANDO(CMD_PARDON, POSITION_RESTING, cmd_pardon, MORTAL);
    COMMANDO(CMD_TALK, POSITION_RESTING, cmd_talk, MORTAL);
    COMMANDO(CMD_OOC, POSITION_DEAD, cmd_ooc, MORTAL);
    COMMANDO(CMD_IOOC, POSITION_DEAD, cmd_iooc, MORTAL);
    COMMANDO(CMD_APPLY, POSITION_STANDING, cmd_not_here, MORTAL);
    COMMANDO(CMD_SKIN, POSITION_STANDING, cmd_skin, MORTAL);
    COMMANDO(CMD_GATHER, POSITION_STANDING, cmd_skill, MORTAL);
    COMMANDO(CMD_RELEASE, POSITION_FIGHTING, cmd_release, MORTAL);
    COMMANDO(CMD_POISONING, POSITION_STANDING, cmd_skill, MORTAL);
    COMMANDO(CMD_SHEATHE, POSITION_RESTING, cmd_sheathe, MORTAL);
    COMMANDO(CMD_ASSESS, POSITION_RESTING, cmd_assess, MORTAL);
    COMMANDO(CMD_PACK, POSITION_STANDING, cmd_pack, MORTAL);
    COMMANDO(CMD_UNPACK, POSITION_STANDING, cmd_unpack, MORTAL);
    COMMANDO(CMD_ZORCH, POSITION_SLEEPING, cmd_zorch, HIGHLORD);
    COMMANDO(CMD_CHANGE, POSITION_SLEEPING, cmd_change, MORTAL);
    COMMANDO(CMD_SHOOT, POSITION_STANDING, cmd_shoot, MORTAL);
    COMMANDO(CMD_LOAD, POSITION_RESTING, cmd_load, MORTAL);
    COMMANDO(CMD_UNLOAD, POSITION_RESTING, cmd_unload, MORTAL);
    COMMANDO(CMD_DWHO, POSITION_SLEEPING, cmd_dwho, CREATOR);
    COMMANDO(CMD_SHIELD, POSITION_RESTING, cmd_not_here, MORTAL);       /* shield   */
    COMMANDO(CMD_CATHEXIS, POSITION_RESTING, cmd_skill, MORTAL);        /* cathexis */
    COMMANDO(CMD_PSI, POSITION_RESTING, cmd_psi, MORTAL);
    COMMANDO(CMD_ASSIST, POSITION_STANDING, cmd_assist, MORTAL);
    COMMANDO(CMD_POOFIN, POSITION_RESTING, cmd_poofin, CREATOR);
    COMMANDO(CMD_POOFOUT, POSITION_RESTING, cmd_poofout, CREATOR);
    COMMANDO(CMD_CD, POSITION_RESTING, cmd_cd, CREATOR);
    COMMANDO(CMD_RSET, POSITION_RESTING, cmd_rset, CREATOR);
    COMMANDO(CMD_RUNSET, POSITION_RESTING, cmd_runset, CREATOR);
    COMMANDO(CMD_OUNSET, POSITION_RESTING, cmd_ounset, CREATOR);
    COMMANDO(CMD_ODELETE, POSITION_RESTING, cmd_odelete, CREATOR);
    COMMANDO(CMD_CDELETE, POSITION_RESTING, cmd_cdelete, CREATOR);
    COMMANDO(CMD_BET, POSITION_RESTING, cmd_not_here, MORTAL);  /* bet     */
    COMMANDO(CMD_PLAY, POSITION_RESTING, cmd_play, MORTAL);
    COMMANDO(CMD_RM, POSITION_RESTING, cmd_rm, CREATOR);
    COMMANDO(CMD_DISENGAGE, POSITION_RESTING, cmd_disengage, MORTAL);
    COMMANDO(CMD_INFOBAR, POSITION_DEAD, cmd_infobar, MORTAL);
    COMMANDO(CMD_SCAN, POSITION_RESTING, cmd_scan, MORTAL);
    COMMANDO(CMD_SEARCH, POSITION_STANDING, cmd_skill, MORTAL); /* scan    */
    COMMANDO(CMD_TAILOR, POSITION_STANDING, cmd_not_here, MORTAL);      /* tailor  */
    COMMANDO(CMD_FLOAD, POSITION_DEAD, cmd_fload, OVERLORD);
    COMMANDO(CMD_PINFO, POSITION_DEAD, cmd_pinfo, CREATOR);
    COMMANDO(CMD_RP, POSITION_RESTING, cmd_rp, MORTAL);
    COMMANDO(CMD_SHADOW, POSITION_RESTING, cmd_follow, MORTAL);
    COMMANDO(CMD_HITCH, POSITION_RESTING, cmd_hitch, MORTAL);
    COMMANDO(CMD_UNHITCH, POSITION_RESTING, cmd_unhitch, MORTAL);
    COMMANDO(CMD_PULSE, POSITION_DEAD, cmd_pulse, CREATOR);
    COMMANDO(CMD_TAX, POSITION_DEAD, cmd_tax, DIRECTOR);
    COMMANDO(CMD_PEMOTE, POSITION_DEAD, cmd_pemote, MORTAL);
    COMMANDO(CMD_WSET, POSITION_DEAD, cmd_wset, DIRECTOR);
    COMMANDO(CMD_EXECUTE, POSITION_DEAD, cmd_execute, OVERLORD);
    COMMANDO(CMD_ETWO, POSITION_RESTING, cmd_etwo, MORTAL);
    COMMANDO(CMD_PLANT, POSITION_STANDING, cmd_plant, MORTAL);
    COMMANDO(CMD_TRACE, POSITION_RESTING, cmd_skill, MORTAL);   /* trace    */
    COMMANDO(CMD_BARRIER, POSITION_RESTING, cmd_skill, MORTAL); /* 406 is not defined in constants.c. */
    COMMANDO(CMD_DISPERSE, POSITION_RESTING, cmd_skill, MORTAL);        /* disperse */
    COMMANDO(CMD_BUG, POSITION_DEAD, cmd_bug, 0);
    COMMANDO(CMD_HARNESS, POSITION_STANDING, cmd_harness, HIGHLORD);
    COMMANDO(CMD_TOUCH, POSITION_STANDING, cmd_not_here, MORTAL);
    COMMANDO(CMD_UNHARNESS, POSITION_STANDING, cmd_touch, STORYTELLER);
    COMMANDO(CMD_TRADE, POSITION_STANDING, cmd_not_here, MORTAL);       /* trade */
    COMMANDO(CMD_EMPATHY, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_SHADOWWALK, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_ALLSPEAK, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_COMMUNE, POSITION_RESTING, cmd_commune, MORTAL);
    COMMANDO(CMD_EXPEL, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_EMAIL, POSITION_RESTING, cmd_email, HIGHLORD);
    COMMANDO(CMD_TELL, POSITION_RESTING, cmd_tell, MORTAL);
    COMMANDO(CMD_CLANSET, POSITION_RESTING, cmd_clanset, CREATOR);
    COMMANDO(CMD_CLANSAVE, POSITION_RESTING, cmd_clansave, DIRECTOR);
    COMMANDO(CMD_AREAS, POSITION_DEAD, cmd_areas, MORTAL);
    COMMANDO(CMD_TRAN, POSITION_DEAD, cmd_trans, CREATOR);
    COMMANDO(CMD_LAND, POSITION_STANDING, cmd_land, MORTAL);
    COMMANDO(CMD_FLY, POSITION_STANDING, cmd_fly, MORTAL);
    COMMANDO(CMD_LIGHT, POSITION_RESTING, cmd_light, MORTAL);
    COMMANDO(CMD_EXTINGUISH, POSITION_RESTING, cmd_extinguish, MORTAL);
    COMMANDO(CMD_SHAVE, POSITION_RESTING, cmd_shave, MORTAL);
    COMMANDO(CMD_HEAL, POSITION_DEAD, cmd_heal, CREATOR);
    COMMANDO(CMD_CAT, POSITION_DEAD, cmd_cat, CREATOR);
    COMMANDO(CMD_PROBE, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_MINDWIPE, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_COMPEL, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_RAISE, POSITION_RESTING, cmd_raise, MORTAL);
    COMMANDO(CMD_LOWER, POSITION_RESTING, cmd_lower, MORTAL);
    COMMANDO(CMD_CONTROL, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_CONCEAL, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_DOME, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_CLAIRAUDIENCE, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_DIE, POSITION_DEAD, cmd_not_here, MORTAL);
    COMMANDO(CMD_INJECT, POSITION_RESTING, cmd_nonet, MORTAL);
    COMMANDO(CMD_BREW, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_COUNT, POSITION_RESTING, cmd_count, MORTAL);
    COMMANDO(CMD_MASQUERADE, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_ILLUSION, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_COMMENT, POSITION_DEAD, cmd_rpcomment, MORTAL);
    COMMANDO(CMD_MONITOR, POSITION_DEAD, cmd_monitor, CREATOR);
    COMMANDO(CMD_CGIVE, POSITION_DEAD, cmd_cgive, OVERLORD);
    COMMANDO(CMD_CREVOKE, POSITION_DEAD, cmd_crevoke, OVERLORD);
    COMMANDO(CMD_PAGELENGTH, POSITION_DEAD, cmd_pagelength, MORTAL);
    COMMANDO(CMD_ROLL, POSITION_RESTING, cmd_roll, MORTAL);
    COMMANDO(CMD_DOAS, POSITION_DEAD, cmd_doas, CREATOR);
    COMMANDO(CMD_VIEW, POSITION_RESTING, cmd_view, MORTAL);
    COMMANDO(CMD_AINFO, POSITION_RESTING, cmd_ainfo, CREATOR);
    COMMANDO(CMD_ASET, POSITION_RESTING, cmd_aset, CREATOR);
    COMMANDO(CMD_PFIND, POSITION_RESTING, cmd_pfind, CREATOR);
    COMMANDO(CMD_AFIND, POSITION_RESTING, cmd_afind, CREATOR);
    COMMANDO(CMD_RESURRECT, POSITION_RESTING, cmd_resurrect, HIGHLORD);
    COMMANDO(CMD_SAP, POSITION_STANDING, cmd_skill, MORTAL);
    COMMANDO(CMD_STORE, POSITION_DEAD, cmd_store, HIGHLORD);
    COMMANDO(CMD_UNSTORE, POSITION_DEAD, cmd_unstore, OVERLORD);
    COMMANDO(CMD_TO_ROOM, POSITION_DEAD, cmd_not_here, 7);
    COMMANDO(CMD_FROM_ROOM, POSITION_DEAD, cmd_not_here, 7);
    COMMANDO(CMD_SCRIBBLE, POSITION_DEAD, cmd_scribble, MORTAL);
    COMMANDO(CMD_LAST, POSITION_DEAD, cmd_last, CREATOR);
    COMMANDO(CMD_WIZLOCK, POSITION_DEAD, cmd_wizlock, HIGHLORD);
    COMMANDO(CMD_TRIBES, POSITION_DEAD, cmd_tribes, MORTAL);
    COMMANDO(CMD_FORAGE, POSITION_DEAD, cmd_skill, MORTAL);
    COMMANDO(CMD_CITYSTAT, POSITION_DEAD, cmd_citystat, CREATOR);
    COMMANDO(CMD_CITYSET, POSITION_DEAD, cmd_cityset, HIGHLORD);
    COMMANDO(CMD_REPAIR, POSITION_SITTING, cmd_repair, MORTAL);
    COMMANDO(CMD_NESSKICK, POSITION_DEAD, cmd_skill, MORTAL);
    COMMANDO(CMD_CHARDELET, POSITION_DEAD, cmd_chardelet, OVERLORD);
    COMMANDO(CMD_CHARDELETE, POSITION_DEAD, cmd_chardelete, OVERLORD);
    COMMANDO(CMD_SENSE_PRESENCE, POSITION_DEAD, cmd_skill, MORTAL);
    COMMANDO(CMD_PALM, POSITION_DEAD, cmd_get, MORTAL);
    COMMANDO(CMD_SUGGEST, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_BABBLE, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_DISORIENT, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_ANALYZE, POSITION_DEAD, cmd_analyze, MORTAL);
    COMMANDO(CMD_CLAIRVOYANCE, POSITION_RESTING, cmd_skill, CREATOR);
    COMMANDO(CMD_IMITATE, POSITION_RESTING, cmd_skill, CREATOR);
    COMMANDO(CMD_PROJECT, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_SLEE, POSITION_DEAD, cmd_slee, MORTAL);
    COMMANDO(CMD_CLEAN, POSITION_RESTING, cmd_clean, MORTAL);
    COMMANDO(CMD_MERCY, POSITION_SLEEPING, cmd_mercy, MORTAL);
    COMMANDO(CMD_WATCH, POSITION_RESTING, cmd_watch, MORTAL);
    COMMANDO(CMD_OINDEX, POSITION_SLEEPING, cmd_oindex, CREATOR);
    COMMANDO(CMD_MAGICKSENSE, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_MINDBLAST, POSITION_RESTING, cmd_skill, MORTAL);
    COMMANDO(CMD_WHEN_LOADED, POSITION_DEAD, cmd_not_here, MORTAL);
    COMMANDO(CMD_ATTEMPT_GET_QUIET, POSITION_DEAD, cmd_not_here, MORTAL);
    COMMANDO(CMD_ATTEMPT_DROP, POSITION_DEAD, cmd_not_here, MORTAL);
    COMMANDO(CMD_ATTEMPT_DROP_QUIET, POSITION_DEAD, cmd_not_here, MORTAL);
    COMMANDO(CMD_SYSTAT, POSITION_DEAD, cmd_systat, STORYTELLER);
    COMMANDO(CMD_ON_DAMAGE, POSITION_DEAD, cmd_not_here, MORTAL);       /*CMD_ON_DAMAGE */
    COMMANDO(CMD_CHARGE, POSITION_STANDING, cmd_skill, MORTAL); /*CMD_CHARGE */
    COMMANDO(CMD_TRAMPLE, POSITION_FIGHTING, cmd_skill, MORTAL); /*CMD_TRAMPLE*/
    COMMANDO(CMD_RTWO, POSITION_RESTING, cmd_rtwo, MORTAL);     /*CMD_RTWO */
    COMMANDO(CMD_ON_EQUIP, POSITION_DEAD, cmd_not_here, MORTAL);        /* ON_EQUIP */
    COMMANDO(CMD_ON_REMOVE, POSITION_DEAD, cmd_not_here, MORTAL);       /* ON_REMOVE */
    COMMANDO(CMD_SEND, POSITION_DEAD, cmd_send, MORTAL);        /* imm-only send command, marked mortal for switched NPCs */
    COMMANDO(CMD_XPATH, POSITION_DEAD, cmd_xpath, STORYTELLER);
    COMMANDO(CMD_SALVAGE, POSITION_SITTING, cmd_salvage, MORTAL);
    COMMANDO(CMD_SILENT_CAST, POSITION_FIGHTING, cmd_cast, MORTAL);
    COMMANDO(CMD_KEYWORD, POSITION_DEAD, cmd_keyword, MORTAL);
    COMMANDO(CMD_HEMOTE, POSITION_DEAD, cmd_emote, MORTAL);
    COMMANDO(CMD_PHEMOTE, POSITION_DEAD, cmd_pemote, MORTAL);
    COMMANDO(CMD_SEMOTE, POSITION_DEAD, cmd_emote, MORTAL);
    COMMANDO(CMD_PSEMOTE, POSITION_DEAD, cmd_pemote, MORTAL);
    COMMANDO(CMD_ADDKEYWOR, POSITION_DEAD, cmd_addkeywor, MORTAL);
    COMMANDO(CMD_ADDKEYWORD, POSITION_DEAD, cmd_addkeyword, MORTAL);
    COMMANDO(CMD_ARRANGE, POSITION_STANDING, cmd_arrange, MORTAL);
    COMMANDO(CMD_DELAYED_EMOTE, POSITION_STANDING, cmd_delayed_emote, MORTAL);
    COMMANDO(CMD_JSEVAL, POSITION_DEAD, cmd_jseval, HIGHLORD);
    COMMANDO(CMD_SNUFF, POSITION_RESTING, cmd_extinguish, MORTAL);
    COMMANDO(CMD_IOOC_SHORT, POSITION_DEAD, cmd_iooc, MORTAL);
    COMMANDO(CMD_TDESC, POSITION_DEAD, cmd_tdesc, MORTAL);
    COMMANDO(CMD_DISCUSS, POSITION_RESTING, cmd_discuss, MORTAL); /*CMD_DICUSS*/
    COMMANDO(CMD_DIRECTIONS, POSITION_RESTING, cmd_directions, MORTAL);
    COMMANDO(CMD_BURY, POSITION_STANDING, cmd_bury, MORTAL);
    COMMANDO(CMD_BLDCOM, POSITION_SLEEPING, cmd_bldcom, MORTAL);
    COMMANDO(CMD_BLDCOM_SHORT, POSITION_SLEEPING, cmd_bldcom, MORTAL);
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */

int
_parse_name(char *arg, char *name)
{
    int i;

    /* skip whitespaces */
    for (; isspace(*arg); arg++);

    for (i = 0; (*name = *arg); arg++, i++, name++)
        if ((*arg < 0) || !isalpha(*arg) || i > 15)
            return (1);

    if (!i)
        return (1);

    return (0);
}



/*
 * new_imm_name()
 * allocate the space for a new imm_name
 */
IMM_NAME *
new_imm_name()
{
    IMM_NAME *imm;

    CREATE(imm, IMM_NAME, 1);
    return imm;
}

/*
 * load_immortal_names()
 * load the list of immortal names from data_files/immortal_list
 */
void
load_immortal_names()
{
    FILE *fp;
    IMM_NAME *imm;
    char buf[MAX_STRING_LENGTH];
    char account[MAX_STRING_LENGTH];
    char name[MAX_STRING_LENGTH];
    char *tmp;

    if ((fp = fopen("data_files/immortal_list", "r")) != NULL) {
        while (!feof(fp)) {
            tmp = buf;
            fgets(buf, MAX_STRING_LENGTH, fp);
            if (strlen(buf) == 0)
                continue;
            buf[strlen(buf) - 1] = '\0';
            while (!isprint(*tmp++));
            if (tmp == '\0')
                continue;
            imm = new_imm_name();
            two_arguments(buf, name, sizeof(name), account, sizeof(account));
            imm->name = strdup(name);
            imm->account = strdup(account);
            imm->next = immortal_name_list;
            immortal_name_list = imm;
        }
        fclose(fp);
    }

}


void
unload_immortal_names()
{
    IMM_NAME *imm, *imm_next;

    for (imm = immortal_name_list; imm; imm = imm_next) {
        imm_next = imm->next;
        free(imm->account);
        free(imm->name);
        free(imm);
    }
    immortal_name_list = 0;

}

/*
 * fwrite_immortal_name()
 * write an individual immortal's name to file, recursive to preserve
 * list order
 */
void
fwrite_immortal_name(IMM_NAME * imm, FILE * fp)
{
    if (imm->next)
        fwrite_immortal_name(imm->next, fp);

    fprintf(fp, "%s %s\n", imm->name, imm->account);
}


/*
 * save_immortal_names()
 * save the list of immortal names
 */
void
save_immortal_names()
{
    FILE *fp;

    if ((fp = fopen("data_files/immortal_list", "w")) != NULL)
        fwrite_immortal_name(immortal_name_list, fp);
    fclose(fp);
}


/*
 * is_immortal_name()
 * check to see if the given name is in the list of immortal names
 */
bool
is_immortal_name(const char *name)
{
    IMM_NAME *imm;

    for (imm = immortal_name_list; imm; imm = imm->next)
        if (!stricmp(name, imm->name))
            return TRUE;
    return FALSE;
}


/*
 * add_immortal_name()
 * adds an immortal name to the list of immortal names
 */
void
add_immortal_name(const char *name, const char *account)
{
    IMM_NAME *imm, *next;;

    if (!name)
        return;

    imm = new_imm_name();
    imm->name = strdup(name);
    imm->account = strdup(account);

    if (immortal_name_list) {
        /* alphebatize */
        for (next = immortal_name_list; next->next; next = next->next)
            if (strcmp(next->name, imm->name) > 0 || strcmp(next->next->name, imm->name) <= 0)
                break;

        imm->next = next->next;
        next->next = imm;
    } else {
        imm->next = immortal_name_list;
        immortal_name_list = imm;
    }
    return;
}

/*
 * remove_immortal_name()
 * removes a name from the immortal name list
 */
bool
remove_immortal_name(const char *name)
{
    IMM_NAME *imm;

    if (!name)
        return FALSE;

    if (!strcmp(immortal_name_list->name, name)) {
        imm = immortal_name_list;
        immortal_name_list = immortal_name_list->next;
        free(imm->name);
        free(imm->account);
        imm->next = NULL;
        return TRUE;
    }

    for (imm = immortal_name_list; imm->next; imm = imm->next) {
        if (!strcmp(imm->next->name, name)) {
            IMM_NAME *tmp = imm->next;

            imm->next = imm->next->next;
            free(tmp->name);
            free(tmp->account);
            tmp->next = NULL;
            return TRUE;
        }
    }

    return FALSE;
}

bool
is_immortal_account(char *account)
{
    IMM_NAME *imm;

    for (imm = immortal_name_list; imm; imm = imm->next)
        if (!stricmp(account, imm->account))
            return TRUE;

    return FALSE;
}

char *
lookup_immortal_account(char *name)
{
    IMM_NAME *imm;

    for (imm = immortal_name_list; imm; imm = imm->next)
        if (!stricmp(name, imm->name))
            return imm->account;

    return NULL;
}

char *
lookup_immortal_name(char *account)
{
    IMM_NAME *imm;

    for (imm = immortal_name_list; imm; imm = imm->next)
        if (!stricmp(account, imm->account))
            return imm->name;

    return NULL;
}

bool
is_guest_name(char *name)
{
    int i;

    for (i = 0; *mortal_access[i] != '\n'; i++)
        if (!stricmp(name, mortal_access[i]))
            return TRUE;
    return FALSE;
}

bool
is_banned_name(char *name)
{
    int i;

    for (i = 0; *banned_name[i] != '\n'; i++)
        if (!stricmp(name, banned_name[i]))
            return TRUE;
    return FALSE;
}

void
position(struct descriptor_data *d, int x, int y)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, VT_CURSPOS, y, x);
    write_to_descriptor(d->descriptor, buf);
}

void
clear_infobar(struct descriptor_data *d)
{
    int i, pagelen;
    char buf[MAX_STRING_LENGTH];

    pagelen = d->character ? GET_PAGELENGTH(d->character) : d->pagelength;

    position(d, 0, pagelen);

    buf[0] = '\0';

    for (i = 0; i < d->linesize; i++)
        strcat(buf, " ");

    write_to_descriptor(d->descriptor, buf);
    position(d, 0, pagelen - 2);
}

void
init_infobar(struct descriptor_data *d)
{
    char buf[MAX_STRING_LENGTH];
    int extra, pagelen;

    pagelen = d->character ? GET_PAGELENGTH(d->character) : d->pagelength;

    clear_infobar(d);

    if (!d->character)
        return;

    extra = MAX(0, d->linesize - 77) / 5;

    write_to_descriptor(d->descriptor, VT_CURSAVE);

    position(d, 0 + extra, pagelen);
    sprintf(buf, "%sHealth:%s ", VT_BOLDTEX, VT_NORMALT);
    write_to_descriptor(d->descriptor, buf);

    position(d, 21 + (extra * 2), pagelen);
    sprintf(buf, "%sMana:%s ", VT_BOLDTEX, VT_NORMALT);
    write_to_descriptor(d->descriptor, buf);

    position(d, 39 + (extra * 3), pagelen);
    sprintf(buf, "%sStamina:%s ", VT_BOLDTEX, VT_NORMALT);
    write_to_descriptor(d->descriptor, buf);

    position(d, 60 + (extra * 4), pagelen);
    sprintf(buf, "%sStun:%s ", VT_BOLDTEX, VT_NORMALT);
    write_to_descriptor(d->descriptor, buf);

    write_to_descriptor(d->descriptor, VT_CURREST);
}


void
adjust_infobar(struct descriptor_data *d, int stat, char *display)
{
    char buf[256];
    int xpos, ypos = d->character ? GET_PAGELENGTH(d->character)
        : d->pagelength;
    int extra;

    extra = MAX(0, d->linesize - 77) / 5;

    switch (stat) {
    case 1:                    /* "Health: " */
        xpos = 9 + extra;
        break;
    case 2:                    /* "Mana: " */
        xpos = 27 + (extra * 2);
        break;
    case 3:                    /* "Stamina: " */
        xpos = 48 + (extra * 3);
        break;
    case 4:                    /* "Stun: " */
        xpos = 66 + (extra * 4);
        break;
    default:
        gamelog("Attempting to display unknown item on general infobar");
        return;
    }

    sprintf(buf, "%s\033[%d;%dH            \033[%d;%dH%s%s", VT_CURSAVE, ypos, xpos, ypos, xpos,
            display, VT_CURREST);
    write_to_descriptor(d->descriptor, buf);
}


void
setup_infobar(struct descriptor_data *d)
{
    char buf[MAX_STRING_LENGTH];
    int i, pagelength, pagewidth;

    if (!d)
        return;
    if (!d->linesize)
        return;

    pagelength = d->character ? GET_PAGELENGTH(d->character) : d->pagelength;
    pagewidth = MIN(d->linesize, (sizeof(buf) - 1));

    write_to_descriptor(d->descriptor, VT_HOMECLR);
    sprintf(buf, VT_CURSPOS, pagelength - 1, 0);
    write_to_descriptor(d->descriptor, buf);

    write_to_descriptor(d->descriptor, VT_BOLDTEX);

    strcpy(buf, "");            // Empty the buffer before filling it
    for (i = 0; i < pagewidth; i++)
        strcat(buf, "-");
    write_to_descriptor(d->descriptor, buf);

    write_to_descriptor(d->descriptor, VT_NORMALT);
    sprintf(buf, VT_MARGSET, 1, pagelength - 2);
    write_to_descriptor(d->descriptor, buf);
}


void
display_infobar(struct descriptor_data *d)
{
    char buf[256];

    init_infobar(d);

    sprintf(buf, "%d(%d)", GET_HIT(d->character), GET_MAX_HIT(d->character));
    adjust_infobar(d, GINFOBAR_HP, buf);
    sprintf(buf, "%d(%d)", GET_MANA(d->character), GET_MAX_MANA(d->character));
    adjust_infobar(d, GINFOBAR_MANA, buf);
    sprintf(buf, "%d(%d)", GET_MOVE(d->character), GET_MAX_MOVE(d->character));
    adjust_infobar(d, GINFOBAR_STAM, buf);
    sprintf(buf, "%d(%d)", GET_STUN(d->character), GET_MAX_STUN(d->character));
    adjust_infobar(d, GINFOBAR_STUN, buf);
}


struct room_data *
home_room(CHAR_DATA * ch)
{
    struct room_data *r = ch->specials.was_in_room;

    if (IS_IMMORTAL(ch)) {
        if (!r || (r && !IS_SET(r->room_flags, RFL_IMMORTAL_ROOM))) {
            r = get_room_num(0);
        }
    } else if (!r) {
        r = get_room_num(city[ch->player.start_city].room);
    } else if ((r) && (r->city == CITY_TULUK) && 
              (IS_SET(r->room_flags, RFL_IMMORTAL_ROOM))) {
        r = get_room_num(1);
        qgamelogf(QUIET_CONNECT, "Moving %s (%s) to Morins because Tuluk is closed.", 
                 GET_NAME(ch), ch->account);
    } 

    if (r)
        return r;

    return get_room_num(1);
}

bool
has_mortal_access(CHAR_DATA * ch)
{
    char *name = MSTR(ch, name);

    int i;

    for (i = 0; *mortal_access[i] != '\n'; i++)
        if (!stricmp(name, mortal_access[i]))
            return TRUE;

    if (IS_TRIBE(ch, 33)        /* Galith */
        ||IS_TRIBE(ch, 24)      /* Gypsies */
        ||IS_TRIBE(ch, 25)      /* Blackwing */
        ||IS_TRIBE(ch, 7)       /* Mantis */
        ||IS_TRIBE(ch, 9)       /* Ptarken */
        ||IS_TRIBE(ch, 36)      /* Oash */
        ||IS_TRIBE(ch, 12)      /* Allanak */
        ||IS_TRIBE(ch, 4))      /* Kurac */
        return TRUE;

    return FALSE;
}

void
view_players(struct descriptor_data *t)
{
    CHAR_DATA *ch = t->character;
    send_to_char("Currently disabled.\n\r", ch);
    return;
}


void
mul_slave_choice(struct descriptor_data *d, char *choice)
{

    SEND_TO_Q("All muls are slaves, or ex-slaves.  Choose the city-state in\n\r"
              "which you would like to be a slave of.  When you enter the\n\r"
              "game, if you choose this city-state, you will start as a slave\n\r"
              "there.  If you choose a different location, you will start as\n\r"
              "a run-away slave.\n\r" "\n\r  Current options are:\n\rAllanak\n\r\n\r>", d);

    // Old version, replaced with the above 10/21/2000 by nessalin
    // So that muls no longer see Tuluk as an option for a home city
    //  SEND_TO_Q("All muls are slaves, or ex-slaves.  Choose the city-state in\n\r"
    //            "which you would like to be a slave of.  When you enter the\n\r"
    //            "game, if you choose this city-state, you will start as a slave\n\r"
    //            "there.  If you choose a different location, you will start as\n\r"
    //            "a run-away slave.\n\r"
    //            "\n\r  Current options are:\n\rAllanak\n\rTuluk\n\r\n\r>", d);

}

void
show_race_selection(struct descriptor_data *d)
{
    char gld_msg[MAX_STRING_LENGTH], tmp_msg[MAX_STRING_LENGTH];
    int count = 1, i;
    char c;

    struct race_num_name
    {
        char *name;
        int num;
    };

    struct race_num_name race_name[] = {
        {"Human", 1},           /* a */
        {"Elf", 2},             /* b */
        {"Dwarf", 3},           /* c */
        {"Halfling", 12},       /* d */
        {"Half-elf", 33},       /* e */
        {"Mul", 32},            /* f */
        {"Half-giant", 23},     /* g */
        {"Mantis", 4},          /* h */
        {"Gith", 20},           /* i */
        {"Dragon", 6},          /* j */
        {"Avangion", 24},       /* k */
        {"Desert-Elf", 98},     /* l */
        {"Blackwing Desert-Elf", 98},   /* m */
        {"\0", 0}
    };

    strcpy(gld_msg, "\n\rSelect a race:\n\r");

    for (i = 0; race_name[i].num != 0; i++) {
        c = (i + 97);
        sprintf(tmp_msg, "%c) %-20s ", c, karma_race_ok(d->player_info->karmaRaces | d->player_info->tempKarmaRaces, c) ? race_name[i].name : "");
        strcat(gld_msg, tmp_msg);
        count++;
        if (count == 4) {
            strcat(gld_msg, "\n\r");
            count = 1;
        }
    }
    strcat(gld_msg, "\n\r");
    SEND_TO_Q(gld_msg, d);
}

void
show_subguild(struct descriptor_data *d)
{
    SEND_TO_Q("\n\r" "Your sub-guild choices are:\n\r"
              "A) Bard             B) Thief             C) Hunter\n\r"
              "D) Forester         E) Armorcrafter      F) Stonecrafter\n\r"
              "G) Scavenger        H) House Servant     I) Guard\n\r"
              "J) Weaponcrafter    K) Physician         L) Mercenary\n\r"
              "M) Archer           N) General Crafter   O) Acrobat\n\r"
              "P) Caravan Guide    Q) Con Artist        R) Jeweler\n\r"
              "S) Linguist         T) Nomad             U) Rebel\n\r"
              "V) Tailor           W) Tinker            X) Thug\n\r"
              "Y) Clayworker\n\r\n\r", d);

    SEND_TO_Q("Please type the letter of your character's sub-guild: ", d);
}

void
show_guild(struct descriptor_data *d)
{
    char gld_msg[MAX_STRING_LENGTH];
    char tmp_msg[MAX_STRING_LENGTH];
    int count = 1, i;
    char c;

    char *guild_name[] = {
        " ",
        "Sorceror",
        "Burglar",
        "Pick-pocket",
        "Warrior",
        "Templar",
        "Ranger",
        "Merchant",
        "Assassin",
        "Fire Elementalist",
        "Water Elementalist",
        "Stone Elementalist",
        "Wind Elementalist",
        "Psionicist",
        "Shadow Elementalist",
        "Lightning Elementalist",
        "Void Elementalist",
        "Lirathu Templar",
        "Jihae Templar",
        "Noble",
        "\0"
    };

    strcpy(gld_msg, "\n\r");
    tmp_msg[0] = 0;
    if (d->character)
        sprintf(tmp_msg, "Select a guild for your %s:\n\r", race[(int) d->character->player.race].name);
    strcat(gld_msg, tmp_msg);

    for (i = 1; i < MAX_GUILDS; i++) {
        c = (i + 96);
        sprintf(tmp_msg, "%c) %-20s ", c,
                ((!d->character || race_guild_ok(d->character, i)) && karma_guild_ok(d->player_info->karmaGuilds | d->player_info->tempKarmaGuilds, i)) ? guild_name[i] : "");
        strcat(gld_msg, tmp_msg);
        count++;
        if (count == 4) {
            strcat(gld_msg, "\n\r");
            count = 1;
        }
    }
    strcat(gld_msg, "\n\r");
    SEND_TO_Q(gld_msg, d);
}


int
race_guild_ok(CHAR_DATA * ch, int i)
{
    if (i == GUILD_PSIONICIST && (GET_RACE(ch) != RACE_HUMAN))
        return 0;

    if (i == GUILD_NOBLE && (GET_RACE(ch) != RACE_HUMAN))
        return 0;

    if (i == GUILD_TEMPLAR && (GET_RACE(ch) != RACE_HUMAN))
        return 0;

    if (i == GUILD_LIRATHU_TEMPLAR && (GET_RACE(ch) != RACE_HUMAN))
        return 0;

    if (i == GUILD_JIHAE_TEMPLAR && (GET_RACE(ch) != RACE_HUMAN))
        return 0;

    if (GET_RACE(ch) == RACE_GITH) {
        if (is_guild_elementalist(i))
            return 0;
        else
            return 1;
    }

    /* restricting BW to desert elf ranger only at Eris' request -Morg 9/23/99 */
    if (GET_RACE(ch) == RACE_DESERT_ELF && IS_TRIBE(ch, TRIBE_BLACKWING)) {
        if (i == GUILD_RANGER) {
            return 1;
        } else {
            return 0;
        }
    }

    if (GET_RACE(ch) == RACE_DESERT_ELF) {
        if (i == GUILD_ASSASSIN || i == GUILD_BURGLAR || i == GUILD_PICKPOCKET)
            return 0;
        else
            return 1;
    }

    if (GET_RACE(ch) == RACE_HALFLING) {
        if (i == GUILD_WARRIOR || i == GUILD_RANGER || i == GUILD_MERCHANT
            || i == GUILD_WATER_ELEMENTALIST || i == GUILD_STONE_ELEMENTALIST)
            return 1;
        else
            return 0;
    }

    if (GET_RACE(ch) == RACE_MANTIS) {
        if (i == GUILD_WARRIOR || i == GUILD_RANGER)
            return 1;
        else
            return 0;
    }

    if (GET_RACE(ch) == RACE_ELF) {     /* City Elf */
        if ((i == GUILD_RANGER))
            return 0;
        else
            return 1;
    }

    if (GET_RACE(ch) == RACE_HALF_GIANT) {
        if ((i == GUILD_DEFILER))
            return 0;
        else
            return 1;
    }

    if (GET_RACE(ch) == RACE_MUL) {
        if ((i == GUILD_DEFILER) || (is_guild_elementalist(i)))
            return 0;
        else
            return 1;
    }

    return 1;
}

void
clearTempKarmaOptions(CHAR_DATA *ch) {
    PLAYER_INFO *pPInfo = ch->desc->player_info;
    // nothing to do if no temp set
    if(pPInfo->tempKarmaRaces) {
        switch GET_RACE(ch) {
        case RACE_HUMAN:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_HUMAN);
           break;
        case RACE_ELF:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_ELF);
           break;
        case RACE_DESERT_ELF:
           if( IS_TRIBE(ch, TRIBE_BLACKWING) ) {
               REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_BLACKWING_ELF);
           } else {
               REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_DESERT_ELF);
           }
           break;
        case RACE_HALF_ELF:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_HALF_ELF);
           break;
        case RACE_HALF_GIANT:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_HALF_GIANT);
           break;
        case RACE_HALFLING:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_HALFLING);
           break;
        case RACE_MUL:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_MUL);
           break;
        case RACE_MANTIS:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_MANTIS);
           break;
        case RACE_GITH:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_GITH);
           break;
        case RACE_DWARF:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_DWARF);
           break;
        case RACE_DRAGON:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_DRAGON);
           break;
        case RACE_AVANGION:
           REMOVE_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_AVANGION);
           break;
        default:
            break;
        }
    }

    if(ch->desc->player_info->tempKarmaGuilds ) {
        switch(GET_GUILD(ch)) {
        case GUILD_BURGLAR:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_BURGLAR);
            break;
        case GUILD_PICKPOCKET:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_PICKPOCKET);
            break;
        case GUILD_WARRIOR:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_WARRIOR);
            break;
        case GUILD_TEMPLAR:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_TEMPLAR);
            break;
        case GUILD_RANGER:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_RANGER);
            break;
        case GUILD_MERCHANT:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_MERCHANT);
            break;
        case GUILD_ASSASSIN:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_ASSASSIN);
            break;
        case GUILD_PSIONICIST:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_PSIONICIST);
            break;
        case GUILD_FIRE_ELEMENTALIST:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_FIRE_CLERIC);
            break;
        case GUILD_WATER_ELEMENTALIST:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_WATER_CLERIC);
            break;
        case GUILD_STONE_ELEMENTALIST:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_STONE_CLERIC);
            break;
        case GUILD_WIND_ELEMENTALIST:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_WIND_CLERIC);
            break;
        case GUILD_SHADOW_ELEMENTALIST:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_SHADOW_CLERIC);
            break;
        case GUILD_LIGHTNING_ELEMENTALIST:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_LIGHTNING_CLERIC);
            break;
        case GUILD_VOID_ELEMENTALIST:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_VOID_CLERIC);
            break;
        case GUILD_DEFILER:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_SORCERER);
            break;
        case GUILD_LIRATHU_TEMPLAR:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_LIRATHU_TEMPLAR);
            break;
        case GUILD_JIHAE_TEMPLAR:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_JIHAE_TEMPLAR);
            break;
        case GUILD_NOBLE:
            REMOVE_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_NOBLE);
            break;
        default:
            break;
        }
    }
}

/* deal with newcomers and other non-playing sockets */
int crash_menu_interpreter_state = -1;

void
menu_interpreter(struct descriptor_data *d, char *arg)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    PLAYER_INFO *pPInfo;
    CHAR_INFO *pChInfo;
    bool fOld;
    char *pwdnew;
    char *pPwd;
    const char *argument;
    struct descriptor_data *tmpd;

    int i, p;
    int min, max;
    char tmp_name[20];
    CHAR_DATA *tmp_ch;
    struct obj_data *tmpobj;
    char *et;

    int sguild = 0;

    int tmphometown;

    crash_menu_interpreter_state = STATE(d);

    if (STATE(d) != CON_PLYNG && STATE(d) != CON_PWDGET
        && STATE(d) != CON_PWDCNF && STATE(d) != CON_GET_NEW_ACCOUNT_PASSWORD
        && STATE(d) != CON_CONFIRM_NEW_ACCOUNT_PASSWORD && STATE(d) != CON_GET_ACCOUNT_PASSWORD) {
        sprintf(buf, "%d %s: %s", d->descriptor, d->hostname, arg);
        shhlog(buf);
    }

    switch (STATE(d)) {
    case CON_ANSI:
        while (*arg == ' ')
            arg++;

        if (*arg == 'y' || *arg == 'Y') {
            d->term = VT100;

            show_main_menu(d);
        } else {
            d->term = 0;

            show_main_menu(d);
        }
        STATE(d) = CON_SLCT;
        break;
    case CON_NME:
        free_char(d->character);

        CREATE(d->character, CHAR_DATA, 1);
        clear_char(d->character);
        d->character->desc = d;

        for (; isspace(*arg); arg++);
        trim_string(arg);

        int num = 0;
        if (!*arg) {
                if (d->player_info->num_chars_alive + d->player_info->num_chars_applying > 1)
			num = 1;
		else {
			close_socket(d);
			return;
		}
        }

        if (is_number(arg))
            num = atoi(arg);

        if (num) {
            CHAR_INFO *pChInfo;

            for (pChInfo = d->player_info->characters; pChInfo; pChInfo = pChInfo->next) {
                if (pChInfo->status == APPLICATION_ACCEPT || pChInfo->status == APPLICATION_DENY
                    || pChInfo->status == APPLICATION_NONE)
                    if (num-- == 1)
                        break;
            }

            if (pChInfo && pChInfo->name) {
                strcpy(arg, pChInfo->name);
            } else {
                SEND_TO_Q("\n\rNot a valid number.\n\rPress RETURN to continue.", d);
                free_char(d->character);
                d->character = NULL;
                STATE(d) = CON_WAIT_SLCT;
                return;
            }
        }

        if (is_banned_name(arg)) {
            SEND_TO_Q("That name is either banned or reserved.\n\r", d);
            free_char(d->character);
            d->character = NULL;
            STATE(d) = CON_WAIT_DISCONNECT;
            return;
        } else {
            if (_parse_name(arg, tmp_name)) {
                SEND_TO_Q("That name is illegal.  Please try another.\n\r", d);
                SEND_TO_Q("What name do you wish to call yourself? ", d);
                free_char(d->character);
                d->character = (CHAR_DATA *) 0;
                return;
            }

            /* if they exist, permissions are handled on enter */
            if (player_exists(tmp_name, d->player_info->name, -1)) {
                sprintf(buf, "Opening character %s.", tmp_name);
                shhlog(buf);
                load_char(tmp_name, d->character);

                sprintf(buf, "Opened character %s.", tmp_name);
                shhlog(buf);

                if (!d->character->account
                    || (d->character->account
                        && (*d->character->account == '\0'
                            || !strcmp(d->character->account, d->player_info->name)))) {
                    if (d->character->account)
                        free(d->character->account);

                    d->character->account = strdup(d->player_info->name);
                }

                STATE(d) = CON_CONNECT_SKIP_PASSWORD;
                menu_interpreter(d, "");
                break;
            }

            sprintf(buf, "You must first create '%s'.\n\r", tmp_name);
            SEND_TO_Q(buf, d);
            SEND_TO_Q("Please read the documentation menu first.\n\r", d);

            free_char(d->character);
            d->character = 0;

            show_main_menu(d);

            STATE(d) = CON_SLCT;
        }
        break;
    case CON_NEWNAME:
        for (; *arg == ' '; arg++);
        trim_string(arg);

        if (!*arg) {

            show_main_menu(d);
            STATE(d) = CON_SLCT;
            return;
        }

        if (is_banned_name(arg) || _parse_name(arg, tmp_name)) {
            SEND_TO_Q("Illegal name, please try another.\n\rBy what "
                      "name dost thou with to hail? ", d);
            return;
        }

        if (is_immortal_name(tmp_name)) {
            SEND_TO_Q("Illegal name, please try another.\n\rBy what "
                      "name dost thou with to hail? ", d);
            return;
        }

        if (player_exists(tmp_name, d->player_info->name, -1)) {
            SEND_TO_Q("You already have a player with that name.\n\r"
                      "What name do you wish to call yourself? ", d);
            return;
        }

        /* mandate that the name be capitalized appropriately */
        tmp_name[0] = UPPER(tmp_name[0]);

        if (d->character && d->character->application_status == APPLICATION_REVISING) {
            if (lookup_char_info(d->player_info, d->character->name)) {
                remove_char_info(d->player_info, lowercase(d->character->name));
                add_new_char_info(d->player_info, tmp_name, APPLICATION_REVISING);
                save_player_info(d->player_info);
            }

            sprintf(buf, "%s/%s/%s", ACCOUNT_DIR, d->player_info->name,
                    lowercase(d->character->name));
            unlink(buf);

            qgamelogf(QUIET_CONNECT, "Name Change: %s to %s (%s) [%s]", GET_NAME(d->character), tmp_name,
                      d->player_info->name, d->hostname);

            free(d->character->name);
            d->character->name = strdup(tmp_name);

            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
            return;
        }

        CREATE(d->character, CHAR_DATA, 1);
        clear_char(d->character);
        d->character->desc = d;

        free(d->character->name);
        d->character->name = strdup(tmp_name);

        free(d->character->account);
        d->character->account = strdup(d->player_info->name);

        qgamelogf(QUIET_CONNECT, "New character: %s (%s) [%s]", GET_NAME(d->character),
                  d->player_info->name, d->hostname);

        d->character->player.info[0] = strdup(d->player_info->email);

        SEND_TO_Q("What is your character's sex? (M/F) ", d);
        STATE(d) = CON_QSEX;
        break;

    case CON_CONNECT_SKIP_PASSWORD:
        if ((d->player_info->email && d->character->player.info[0]
             && strcmp(d->player_info->email, d->character->player.info[0]))
            || !d->character->player.info[0]) {
            if (d->character->player.info[0])
                free(d->character->player.info[0]);
            d->character->player.info[0] = strdup(d->player_info->email);
        }

        for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next) {
            if (!IS_NPC(tmp_ch) && GET_NAME(tmp_ch)
                && !affected_by_spell(tmp_ch, SPELL_POSSESS_CORPSE)
                && !strcmp(GET_NAME(d->character), GET_NAME(tmp_ch))
                && !strcmp(tmp_ch->player.info[0], d->player_info->email)) {
                DESCRIPTOR_DATA *tmpd;

                SEND_TO_Q("Reconnected.\n\r", d);

                /* if there's already a socket, close it -Morg */
                if (tmp_ch->desc)
                    close_socket(tmp_ch->desc);

                /* check to see if we were switched */
                for (tmpd = descriptor_list; tmpd; tmpd = tmpd->next) {
                    if (tmpd->original == tmp_ch) {
                        close_socket(tmpd);
                        break;      // Can only have one "Switched" target anyway, break here
                    }
                }

                if (affected_by_spell(tmp_ch, PSI_SHADOW_WALK))
                    affect_from_char(tmp_ch, PSI_SHADOW_WALK);

                tmp_ch->specials.timer = 0;
                free_char(d->character);

                tmp_ch->desc = d;
                d->character = tmp_ch;
                add_listener(d->character); /* if char is a listener, add
                                             * char to the listen_list */
                STATE(d) = CON_PLYNG;

                if (d->term && IS_SET(d->character->specials.act, CFL_INFOBAR)) {
                    setup_infobar(d);
                    display_infobar(d);
                }

                login_character(d->character);
                act("$n has reconnected.", TRUE, tmp_ch, 0, 0, TO_ROOM);

                sprintf(buf, "%s (%s) [%s] has reconnected.", GET_NAME(d->character),
                        d->player_info->name, d->hostname);
                shhlog(buf);
                sprintf(buf, "/* %s (%s) [%s] has reconnected. */\n\r", GET_NAME(d->character),
                        d->player_info->name, d->hostname);

                connect_send_to_higher(buf, STORYTELLER, d->character);
                return;
            }
        }

        shhlogf("%s (%s) [%s] has connected.", GET_NAME(d->character),
                d->player_info->name, d->hostname);
        sprintf(buf, "/* %s (%s) [%s] has connected. */\n\r", GET_NAME(d->character),
                d->player_info->name, d->hostname);

        connect_send_to_higher(buf, STORYTELLER, d->character);

        SEND_TO_Q("\n\r", d);
        sprintf(buf, "Welcome back, %s.\n\r\n\rPress RETURN to continue. ",
         GET_NAME(d->character));
        SEND_TO_Q(buf, d);

        STATE(d) = CON_WAIT_SLCT;
        break;

    case CON_QSEX:             /* query sex of new user        */
        for (; isspace(*arg); arg++);
        switch (*arg) {
        case 'm':
        case 'M':
            /* sex MALE */
            d->character->player.sex = SEX_MALE;
            break;

        case 'f':
        case 'F':
            /* sex FEMALE */
            d->character->player.sex = SEX_FEMALE;
            break;

        default:
            SEND_TO_Q("That's not a sex..\n\r", d);
            SEND_TO_Q("What is your sex? ", d);
            return;
        }

        if (d->character->application_status != APPLICATION_REVISING) {
            show_race_selection(d);
            SEND_TO_Q("\n\rPlease type the letter of your character's race: ", d);
            STATE(d) = CON_QRACE;
        } else {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
        }
        break;


    case CON_QGUILD:
        {
            /* Always zero out the subguild option when a main guild is
             * being selected. */
            d->character->player.sub_guild = 0;

            /* did they change to an invalid race/guild combo? */
            d->pass_wrong = race_guild_ok(d->character, GET_GUILD(d->character));

            for (; isspace(*arg); arg++);

            i = LOWER(*arg) - 'a' + 1;
            if ((i > 0) && (i < MAX_GUILDS) && guild[i].pl_guild && race_guild_ok(d->character, i)) {
                if (!karma_guild_ok(d->player_info->karmaGuilds | d->player_info->tempKarmaGuilds, i)) {
                    SEND_TO_Q
                        ("\n\rYou do not have enough karma to choose this profession.\n\r"
                         "Type the letter of your guild: ",
                         d);
                    STATE(d) = CON_QGUILD;
                } else {
                    SET_GUILD(d->character, i);
                    
                    if (d->character->player.hometown > 0) { 
                       tmphometown = d->character->player.hometown;     // Preserve the hometown
                       init_char(d->character, 0);                      // Init the character
                       d->character->player.hometown = tmphometown;     // Reset the hometown
                    } else {
                       init_char(d->character, 0);
                    }

                    if (GET_GUILD(d->character) == GUILD_DEFILER) {
                        SEND_TO_Q("\n\r"
                                  "As a Sorceror, you have learned some magicks after already having a \n\r"
                                  "normal upbringing.  Please choose you mundane profession.\n\r"
                                  "\n\rCurrent professions are:\n\r\n\r"
                                  "A) Ranger           B) Merchant            C) Pick-pocket\n\r"
                                  "D) Burglar          E) Warrior             F) Assassin\n\r\n\r", d);
                        SEND_TO_Q
                            ("\n\rPlease type the letter of your character's mundane profession: ",
                             d);

                        STATE(d) = CON_SORC_CHOICE;
                        break;
#define PSI_SECOND_PROF TRUE
#if PSI_SECOND_PROF
                    } else if (GET_GUILD(d->character) == GUILD_PSIONICIST) {
                        SEND_TO_Q("\n\r"
                                  "As a Psionicist, you will want to hide your powers until you are strong,\n\r"
                                  "as your kind are mercilessly hunted.  Many psionicists learn a side\n\r"
                                  "profession, so they can earn a living while practicing their secret\n\r"
                                  "dark arts.\n\r\n\rCurrent second professions are:\n\r\n\r"
                                  "A) Ranger           B) Merchant            C) Pick-pocket\n\r"
                                  "D) Burglar          E) Warrior             F) Archer\n\r"
                                  "G) Nomad            H) Assassin            I) Bard\n\r"
                                  "J) Mercenary\n\r\n\r", d);
                        SEND_TO_Q
                            ("\n\rPlease type the letter of your character's second profession: ",
                             d);
                        STATE(d) = CON_PSI_CHOICE;
                        break;
#endif
                    } else if (GET_GUILD(d->character) == GUILD_NOBLE) {
                        SEND_TO_Q("\n\r" "As a Noble, you get unique sub-guild options.\n\r"
                                  "\n\r\n\rCurrent second professions are:\n\r\n\r"
                                  "A) Aesthete         B) Athelete          C) Financier\n\r"
                                  "D) Musician         E) Linguist          F) Outdoorsman\n\r"
                                  "G) Physician        H) Scholar           I) Spy\n\r\n\r", d);
                        SEND_TO_Q
                            ("\n\rPlease type the letter of your character's noble sub-guild: ", d);

                        STATE(d) = CON_NOBLE_CHOICE;
                        break;
                    } else {
                        show_subguild(d);
                        STATE(d) = CON_SECOND_SKILL_PACK_CHOICE;
                        break;
                    }
                }
            } else {
                SEND_TO_Q("\n\rInvalid guild.\n\rType the letter of your guild: ", d);
                STATE(d) = CON_QGUILD;
            }
        }
        break;

    case CON_NOBLE_CHOICE:

        /* skip whitespaces */
        for (; isspace(*arg); arg++);
        *arg = LOWER(*arg);
        switch (*arg) {
        case 'a':              /* Aesthete */
            d->character->player.sub_guild = SUB_GUILD_NOBLE_AESTHETE;
            break;
        case 'b':              /* Athlete */
            d->character->player.sub_guild = SUB_GUILD_NOBLE_ATHLETE;
            break;
        case 'c':              /* Financier */
            d->character->player.sub_guild = SUB_GUILD_NOBLE_FINANCIER;
            break;
        case 'd':              /* Musician */
            d->character->player.sub_guild = SUB_GUILD_NOBLE_MUSICIAN;
            break;
        case 'e':              /* Linguist */
            d->character->player.sub_guild = SUB_GUILD_NOBLE_LINGUIST;
            break;
        case 'f':              /* Outdoorsman */
            d->character->player.sub_guild = SUB_GUILD_NOBLE_OUTDOORSMAN;
            break;
        case 'g':              /* Physician */
            d->character->player.sub_guild = SUB_GUILD_NOBLE_PHYSICIAN;
            break;
        case 'h':              /* Scholar */
            d->character->player.sub_guild = SUB_GUILD_NOBLE_SCHOLAR;
            break;
        case 'i':              /* Spy */
            d->character->player.sub_guild = SUB_GUILD_NOBLE_SPY;
            break;
        default:
            SEND_TO_Q("\n\rInvalid noble sub-guild choice.  Choose again.\n\r", d);
            STATE(d) = CON_NOBLE_CHOICE;
            return;
        }
        if (d->character->application_status == APPLICATION_REVISING && !d->pass_wrong) {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
            return;
        } else if (d->character->application_status == APPLICATION_REVISING) {
            d->pass_wrong = 0;
            print_height_info(d);
            STATE(d) = CON_HEIGHT;
            return;
        }

        SEND_TO_Q("\n\r", d);
        SEND_TO_Q(stat_order_info, d);
        SEND_TO_Q("\n\rEnter your priority: ", d);
        STATE(d) = CON_ORDER_STATS;
        break;

    case CON_SORC_CHOICE:
        /* skip whitespaces */
        for (; isspace(*arg); arg++);

        *arg = LOWER(*arg);

        switch (*arg) {
        case 'a':
             d->character->player.guild = GUILD_RANGER;
             break;
        case 'b':
             d->character->player.guild = GUILD_MERCHANT;
             break;
        case 'c':
             d->character->player.guild = GUILD_PICKPOCKET;
             break;
        case 'd':
             d->character->player.guild = GUILD_BURGLAR;
             break;
        case 'e':
             d->character->player.guild = GUILD_WARRIOR;
             break;
        case 'f':
             d->character->player.guild = GUILD_ASSASSIN;
             break;
        default:
            SEND_TO_Q("\n\rInvalid guild choice.  Choose again.\n\r", d);
            // Should already be in the proper state - 7/15/2005
            // STATE(d) = CON_SORC_CHOICE;
            return;
        }

        if (d->character->application_status == APPLICATION_REVISING && !d->pass_wrong) {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
            return;
        } else if (!d->character->application_status == APPLICATION_REVISING) {
            d->pass_wrong = 0;
            print_height_info(d);
            STATE(d) = CON_HEIGHT;
            return;
        }

        SEND_TO_Q("\n\r", d);
        SEND_TO_Q("Sorcerers, somewhere in their career, have gained secret arcane\n\r"
                  "knowledge that gives them some access to forbidden magicks.\n\r"
                  "The options currently available are:\n\r\n\r"
                  "A) Enchantment Magick    B) Combat Magick    C) Movement Magick\n\r"
                  "D) Enlightenment Magick\n\r", d);
        SEND_TO_Q("\n\rWhat kind of sorcerer are you? ", d);
        STATE(d) = CON_SORC_SUB_CHOICE;
        break;

    case CON_SORC_SUB_CHOICE:
               /* skip whitespaces */
        for (; isspace(*arg); arg++);

        *arg = LOWER(*arg);

        switch (*arg) {
        case 'a':
             d->character->player.sub_guild = SUB_GUILD_ENCHANTMENT_MAGICK;
             break;
        case 'b':
             d->character->player.sub_guild = SUB_GUILD_COMBAT_MAGICK;
             break;
        case 'c':
             d->character->player.sub_guild = SUB_GUILD_MOVEMENT_MAGICK;
             break;
        case 'd':
             d->character->player.sub_guild = SUB_GUILD_ENLIGHTENMENT_MAGICK;
             break;
        default:
             SEND_TO_Q("\n\rInvalid guild choice.  Choose again.\n\r", d);
             return;
        }
        // This will zero out and reset the guild skills on the character,
        // and add the skills for the subguild that was just set.
        reset_char_skills(d->character);

        if (d->character->application_status == APPLICATION_REVISING ) {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
            return;
        } else if (!d->character->application_status == APPLICATION_REVISING) {
            d->pass_wrong = 0;
            print_height_info(d);
            STATE(d) = CON_HEIGHT;
            return;
        }

        SEND_TO_Q("\n\r", d);
        SEND_TO_Q(stat_order_info, d);
        SEND_TO_Q("\n\rEnter your priority: ", d);
        STATE(d) = CON_ORDER_STATS;
        break;



    case CON_PSI_CHOICE:
        /* skip whitespaces */
        for (; isspace(*arg); arg++);

        *arg = LOWER(*arg);

        switch (*arg) {
        case 'a':              /* Ranger */
            d->character->player.sub_guild = SUB_GUILD_SORC_RANGER;
            break;
        case 'b':              /* Merchant */
            d->character->player.sub_guild = SUB_GUILD_SORC_MERCHANT;
            break;
        case 'c':              /* Pick-Pocket */
            d->character->player.sub_guild = SUB_GUILD_SORC_PICKPOCKET;
            break;
        case 'd':              /* Burglar */
            d->character->player.sub_guild = SUB_GUILD_SORC_BURGLAR;
            break;
        case 'e':              /* Warrior */
            d->character->player.sub_guild = SUB_GUILD_SORC_WARRIOR;
            break;
        case 'f':              /* Archer */
            d->character->player.sub_guild = SUB_GUILD_SORC_ARCHER;
            break;
        case 'g':              /* Nomad */
            d->character->player.sub_guild = SUB_GUILD_SORC_NOMAD;
            break;
        case 'h':              /* Assassin */
            d->character->player.sub_guild = SUB_GUILD_SORC_ASSASSIN;
            break;
        case 'i':              /* Bard */
            d->character->player.sub_guild = SUB_GUILD_SORC_BARD;
            break;
        case 'j':              /* Mercenary */
            d->character->player.sub_guild = SUB_GUILD_SORC_MERCENARY;
            break;
        default:
            SEND_TO_Q("\n\rInvalid guild choice.  Choose again.\n\r", d);
            // Should already be in the proper state - 7/15/2005
            // STATE(d) = CON_SORC_CHOICE;
            return;
        }

        // This will zero out and reset the guild skills on the character,
        // and add the skills for the subguild that was just set.
        reset_char_skills(d->character);

        if (d->character->application_status == APPLICATION_REVISING && !d->pass_wrong) {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
            return;
        } else if (d->character->application_status == APPLICATION_REVISING) {
            d->pass_wrong = 0;
            print_height_info(d);
            STATE(d) = CON_HEIGHT;
            return;
        }

        SEND_TO_Q("\n\r", d);
        SEND_TO_Q(stat_order_info, d);
        SEND_TO_Q("\n\rEnter your priority: ", d);
        STATE(d) = CON_ORDER_STATS;
        break;

    case CON_SECOND_SKILL_PACK_CHOICE:
        /* skip whitespaces */
        for (; isspace(*arg); arg++);

        *arg = LOWER(*arg);

        sguild = (*arg - 'a' + 1);      /* 'a' = 1 in the sub_guild array */
        if (sguild > 0 && sguild < MAX_SUB_GUILDS)
            d->character->player.sub_guild = sguild;
        else
            d->character->player.sub_guild = 0;

        // All lowercase since the LOWER() macro was used
        switch (*arg) {
        case 'a':              /* Bard */
            d->character->player.sub_guild = SUB_GUILD_BARD;
            break;
        case 'b':              /* Thief */
            d->character->player.sub_guild = SUB_GUILD_THIEF;
            break;
        case 'c':              /* Hunter */
            d->character->player.sub_guild = SUB_GUILD_HUNTER;
            break;
        case 'd':              /* Forester */
            d->character->player.sub_guild = SUB_GUILD_FORESTER;
            break;
        case 'e':              /* ARMORMAKER */
            d->character->player.sub_guild = SUB_GUILD_ARMOR_CRAFTER;
            break;
        case 'f':              /* STONECRAFTER */
            d->character->player.sub_guild = SUB_GUILD_STONE_CRAFTER;
            break;
        case 'g':              /* SCAVENGER */
            d->character->player.sub_guild = SUB_GUILD_SCAVENGER;
            break;
        case 'h':              /* HOUSE SERVANT */
            d->character->player.sub_guild = SUB_GUILD_HOUSE_SERVANT;
            break;
        case 'i':              /* GUARD */
            d->character->player.sub_guild = SUB_GUILD_GUARD;
            break;
        case 'j':              /* WEAPONSCRAFTER */
            d->character->player.sub_guild = SUB_GUILD_WEAPON_CRAFTER;
            break;
        case 'k':              /* PHYSICIAN */
            d->character->player.sub_guild = SUB_GUILD_PHYSICIAN;
            break;
        case 'l':              /* MERCENARY */
            d->character->player.sub_guild = SUB_GUILD_MERCENARY;
            break;
        case 'm':              /* ARCHER */
            d->character->player.sub_guild = SUB_GUILD_ARCHER;
            break;
        case 'n':              /* GENERAL CRAFTER */
            d->character->player.sub_guild = SUB_GUILD_GENERAL_CRAFTER;
            break;
            /* Added acrobat thru tailor, 8/11/2001 -San */
        case 'o':              /* ACROBAT */
            d->character->player.sub_guild = SUB_GUILD_ACROBAT;
            break;
        case 'p':              /* CARAVAN GUIDE */
            d->character->player.sub_guild = SUB_GUILD_CARAVAN_GUIDE;
            break;
        case 'q':              /* CON ARTIST */
            d->character->player.sub_guild = SUB_GUILD_CON_ARTIST;
            break;
        case 'r':              /* JEWELER */
            d->character->player.sub_guild = SUB_GUILD_JEWELER;
            break;
        case 's':              /* LINGUIST */
            d->character->player.sub_guild = SUB_GUILD_LINGUIST;
            break;
        case 't':              /* NOMAD */
            d->character->player.sub_guild = SUB_GUILD_NOMAD;
            break;
        case 'u':              /* REBEL */
            d->character->player.sub_guild = SUB_GUILD_REBEL;
            break;
        case 'v':              /* TAILOR */
            d->character->player.sub_guild = SUB_GUILD_TAILOR;
            break;
        case 'w':              /* TINKER */
            d->character->player.sub_guild = SUB_GUILD_TINKER;
            break;
        case 'x':              /* THUG */
            d->character->player.sub_guild = SUB_GUILD_THUG;
            break;
        case 'y':              /* CLAYWORKER */
          d->character->player.sub_guild = SUB_GUILD_CLAYWORKER;
          break;
        default:
            SEND_TO_Q("\n\rInvalid sub-guild choice.  Choose again.\n\r", d);
            STATE(d) = CON_SECOND_SKILL_PACK_CHOICE;
            return;
        }

        // This will zero out and reset the guild skills on the character,
        // and add the skills for the subguild that was just set.
        reset_char_skills(d->character);

        if (d->character->application_status == APPLICATION_REVISING && !d->pass_wrong) {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
            return;
        } else if (d->character->application_status == APPLICATION_REVISING) {
            d->pass_wrong = 0;
            print_height_info(d);
            STATE(d) = CON_HEIGHT;
            return;
        }

        SEND_TO_Q("\n\r", d);
        SEND_TO_Q(stat_order_info, d);
        SEND_TO_Q("\n\rEnter your priority: ", d);
        STATE(d) = CON_ORDER_STATS;
        break;

    case CON_ORDER_STATS:
        {
            // evaluate their input 
            int priority, attr;
            int choices[MAX_ATTRIBUTE];
            bool used[MAX_ATTRIBUTE];

            for (priority = 0; priority < MAX_ATTRIBUTE; priority++) {
                choices[priority] = -1;
                used[priority] = FALSE;
            }

            // Remove the stat info, if it already there
            rem_char_extra_desc_value(d->character, "[STAT_INFO]");

            argument = arg;
            for (priority = 0; priority < MAX_ATTRIBUTE; priority++) {
                argument = one_argument(argument, buf, sizeof(buf));

                if (!*buf && priority == 0) {
                    SEND_TO_Q("You didn't specify any attributes. If you don't"
                              " specify an order, your rolls\n\rwill be fully random.\n\r"
                              "\n\rAre you sure you want to do this? (y/n)", d);
                    STATE(d) = CON_ORDER_STATS_CONFIRM;
                    return;
                } else if (!*buf) {
                    bool found_attr = FALSE;

                    while (!found_attr) {
                        // Loop over each attribute, see if it's been used
                        for (attr = 0; attr < MAX_ATTRIBUTE; attr++) {
                            // if not used and random chance depending on # left
                            if (!used[attr]
                                && !(number(priority, MAX_ATTRIBUTE) - priority)) {
                                // set the current priority
                                choices[priority] = attr;
                                // set used
                                used[attr] = TRUE;
                                // found an attr
                                found_attr = TRUE;
                                // increment priority
                                break;
                            }
                        }
                    }
                    continue;
                } else if (!str_prefix(buf, "strength")) {
                    attr = ATT_STR;
                } else if (!str_prefix(buf, "agility")) {
                    attr = ATT_AGL;
                } else if (!str_prefix(buf, "wisdom")) {
                    attr = ATT_WIS;
                } else if (!str_prefix(buf, "endurance")) {
                    attr = ATT_END;
                } else {
                    sprintf(buf2,
                            "Invalid choice '%s'.\n\rValid choices are 'strength', 'agility', 'wisdom', 'endurance'\n\r",
                            buf);
                    SEND_TO_Q(buf2, d);
                    SEND_TO_Q("\n\rEnter your priority: ", d);
                    return;
                }

                if (used[attr]) {
                    SEND_TO_Q("\n\rEnter each attribute only once!\n\r", d);
                    SEND_TO_Q("\n\rEnter your priority: ", d);
                    return;
                }
                choices[priority] = attr;
                used[attr] = TRUE;
            }
            sprintf(buf, "%s %s %s %s", attributes[choices[0]], attributes[choices[1]],
                    attributes[choices[2]], attributes[choices[3]]);
            set_char_extra_desc_value(d->character, "[STAT_ORDER]", buf);

            sprintf(buf, "\n\rUsing stat order of: '%s %s %s %s'\n\r", attributes[choices[0]],
                    attributes[choices[1]], attributes[choices[2]], attributes[choices[3]]);
            SEND_TO_Q(buf, d);

            if (d->character->application_status != APPLICATION_REVISING) {
                SEND_TO_Q("\n\r", d);
                SEND_TO_Q(desc_info, d);
                SEND_TO_Q("Enter your Main Description.\n\r", d);
                string_edit(d->character, &d->character->description, MAX_DESC);
                STATE(d) = CON_AUTO_DESC_END;
            } else {
                print_revise_char_menu(d);
                STATE(d) = CON_REVISE_CHAR_MENU;
            }
            break;
        }

    case CON_ORDER_STATS_CONFIRM:
        if (LOWER(*arg) == 'y') {
            SEND_TO_Q("\n\r", d);
            SEND_TO_Q(desc_info, d);
            SEND_TO_Q("Enter your Main Description.\n\r", d);
            string_edit(d->character, &d->character->description, MAX_DESC);
            STATE(d) = CON_AUTO_DESC_END;
        } else {
            SEND_TO_Q("\n\rEnter your priority: ", d);
            STATE(d) = CON_ORDER_STATS;
        }
        break;

    case CON_WAIT_SLCT:
        show_main_menu(d);
        STATE(d) = CON_SLCT;
        break;

    case CON_SLCT:             /* get selection from main menu */
        for (; isspace(*arg); arg++);
        switch (*arg) {
        case '?':
            SEND_TO_Q("\n\r", d);

            show_main_menu(d);

            break;
        case 'r':
        case 'R':
            pPInfo = d->player_info;

            if (!pPInfo) {
                SEND_TO_Q("That option is invalid.\n\r", d);
                SEND_TO_Q("Choose thy fate:  ", d);
                return;
            } else if (IS_SET(pPInfo->flags, PINFO_NO_NEW_CHARS)) {
                SEND_TO_Q("That option is invalid.\n\r", d);
                SEND_TO_Q("Choose thy fate:  ", d);
                return;
            } else if (d->character && d->character->application_status == APPLICATION_DENY) {
                if (pPInfo->num_chars_alive + pPInfo->num_chars_applying >=
                    pPInfo->num_chars_allowed) {
                    SEND_TO_Q("That option is invalid.\n\r", d);
                    SEND_TO_Q("Choose thy fate:  ", d);
                }

                print_revise_char_menu(d);
                d->character->application_status = APPLICATION_REVISING;

                STATE(d) = CON_REVISE_CHAR_MENU;
                return;
            } else if (pPInfo->num_chars_alive + pPInfo->num_chars_applying >=
                       d->player_info->num_chars_allowed) {
                SEND_TO_Q("That option is invalid.\n\r", d);
                SEND_TO_Q("Choose thy fate:  ", d);
                return;
            }
            /* otherwise they have an open slot so can create a character */

            if (lock_new_char) {
                SEND_TO_Q("No new characters are being accepted at this time.", d);
                SEND_TO_Q("Choose thy fate: ", d);
                break;
            } else {
                free_char(d->character);
                d->character = 0;
                SEND_TO_Q(app_info, d);
                SEND_TO_Q("\n\rWhat name do you wish to call yourself? ", d);
                STATE(d) = CON_NEWNAME;

                break;
            }
            break;

        case 'z':
        case 'Z':
            if (d->character && d->character->application_status == APPLICATION_DENY) {
                SEND_TO_Q("You are about to delete your character, enter your"
                          " account password to confirm:  ", d);
                ECHO_OFF(d);
                STATE(d) = CON_CONFIRM_DELETE;
                return;
            }

            SEND_TO_Q("That option is invalid.\n\r", d);
            SEND_TO_Q("Choose thy fate:  ", d);
            break;

        case 'b':
        case 'B':
            d->brief_menus = !d->brief_menus;
            if (d->player_info && !d->brief_menus) {
                REMOVE_BIT(d->player_info->flags, PINFO_BRIEF_MENUS);
                save_player_info(d->player_info);
            } else if (d->player_info) {
                MUD_SET_BIT(d->player_info->flags, PINFO_BRIEF_MENUS);
                save_player_info(d->player_info);
            }
            SEND_TO_Q("Brief Menus Toggled\n\r", d);
            show_main_menu(d);
            break;

        case 'c':
        case 'C':
            if (d->player_info) {
                if (d->player_info->num_chars == 0) {
                    SEND_TO_Q("\n\rYou must first create a character to enter."
                              "\n\rPress RETURN to continue.", d);
                    STATE(d) = CON_WAIT_SLCT;
                    return;
                }

                if (d->character) {
                    SEND_TO_Q("Disconnecting from character...\n\r", d);
                    SEND_TO_Q("Press RETURN to continue.", d);
                    logout_character(d->character);
                    free_char(d->character);
                    d->character = 0;
                    STATE(d) = CON_WAIT_SLCT;
                    return;
                }

                if (!IS_SET(d->player_info->flags, PINFO_CAN_MULTIPLAY)) {
                    for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next) {
                        if (!IS_NPC(tmp_ch)
                            && tmp_ch->account && !strcmp(tmp_ch->account, d->player_info->name)) {
                            sprintf(buf, "Reconnecting to %s...\n\r", tmp_ch->name);
                            SEND_TO_Q(buf, d);
                            STATE(d) = CON_NME;
                            menu_interpreter(d, tmp_ch->name);
                            return;
                        }
                    }
                }

                if (d->player_info->num_chars_alive + d->player_info->num_chars_applying > 1) {
                    int count = 0;
                    CHAR_INFO *chinfo;

                    SEND_TO_Q("\n\rWhich character do you want to connect to:" "\n\r", d);
                    for (chinfo = d->player_info->characters; chinfo; chinfo = chinfo->next) {
                        if (chinfo->status == APPLICATION_ACCEPT
                            || chinfo->status == APPLICATION_DENY
                            || chinfo->status == APPLICATION_NONE) {
                            sprintf(buf, " %d) %s - %s\n\r", ++count, capitalize(chinfo->name),
                                    application_status(chinfo->status));
                            SEND_TO_Q(buf, d);
                        }
                    }

                    SEND_TO_Q("\n\rEnter name or number:  ", d);
                    STATE(d) = CON_NME;
                    return;
                } else {
                    CHAR_INFO *chinfo;

                    for (chinfo = d->player_info->characters; chinfo; chinfo = chinfo->next) {
                        if (chinfo->status == APPLICATION_ACCEPT
                            || chinfo->status == APPLICATION_DENY
                            || chinfo->status == APPLICATION_NONE) {
                            break;
                        }
                    }

                    if (!chinfo) {
                        d->player_info->num_chars_alive = 0;
                        SEND_TO_Q("\n\rYou must first create a character."
                                  "\n\rPress RETURN to continue.", d);
                        STATE(d) = CON_WAIT_SLCT;
                        return;
                    }
                    STATE(d) = CON_NME;
                    sprintf(buf, "Connecting to %s...\n\r", chinfo->name);
                    SEND_TO_Q(buf, d);
                    menu_interpreter(d, (char *) chinfo->name);
                }
            } else {            /* load account info */

                SEND_TO_Q("\n\rEnter your account name:  ", d);
                STATE(d) = CON_GET_ACCOUNT_NAME;
                return;
            }
            break;
        case 'n':
        case 'N':
            /* A direct option off the main menu for new players to
             * create an account. -Tiernan 2/29/2004 */
            if (!d->player_info) {
                SEND_TO_Q(account_info, d);
                SEND_TO_Q("\n\rPlease enter a name for your account:  ", d);
                STATE(d) = CON_GET_NEW_ACCOUNT_NAME;
            }
            break;
        case 'v':
        case 'V':
            d->term = !d->term;

            sprintf(buf, "The infobar and ANSI/VT100 graphics are now %s.\n\r",
                    d->term ? "enabled" : "disabled");
            SEND_TO_Q(buf, d);

            if (d->player_info && d->term) {
                MUD_SET_BIT(d->player_info->flags, PINFO_ANSI_VT100);
                save_player_info(d->player_info);
            } else if (d->player_info) {
                REMOVE_BIT(d->player_info->flags, PINFO_ANSI_VT100);
                save_player_info(d->player_info);
            }

            SEND_TO_Q("\n\rPress RETURN to continue.", d);
            STATE(d) = CON_WAIT_SLCT;
            break;

        case 'x':
        case 'X':
            close_socket(d);
            break;

        case 'l':
        case 'L':
            if (!d->player_info || !d->player_info->num_chars || !d->player_info->characters) {
                SEND_TO_Q("That option is invalid.\n\r", d);
                SEND_TO_Q("Choose thy fate:  ", d);
                return;
            } else if (d->player_info->num_chars > 1) {
                int count;
                CHAR_INFO *chinfo;

                SEND_TO_Q("\n\r\n\rCharacters registered to you:\n\r", d);

                count = 0;
                for (chinfo = d->player_info->characters; chinfo; chinfo = chinfo->next) {
                    sprintf(buf, "  %d) %s - %s\n\r", ++count, capitalize(chinfo->name),
                            application_status(chinfo->status));
                    SEND_TO_Q(buf, d);
                }

                SEND_TO_Q("\n\rPress RETURN to continue.", d);
                STATE(d) = CON_WAIT_SLCT;
                return;
            } else {
                SEND_TO_Q("\n\r\n\rRegistered to you:\n\r", d);

                sprintf(buf, "  %s - %s\n\r", d->player_info->characters->name,
                        application_status(d->player_info->characters->status));
                SEND_TO_Q(buf, d);

                SEND_TO_Q("\n\rPress RETURN to continue.", d);
                STATE(d) = CON_WAIT_SLCT;
                return;
            }
            break;
        case 'e':
        case 'E':
            if (!d->player_info) {
                SEND_TO_Q("That option is invalid.\n\r", d);
                SEND_TO_Q("Choose thy fate: ", d);
                return;
            } else if (!d->character) {
                SEND_TO_Q("\n\rYou are about to change the email associated"
                          " with your account.\n\rPress RETURN to abort\n\r", d);

                SEND_TO_Q("\n\rEnter a new email for your account:  ", d);
                STATE(d) = CON_EMAIL_VERIFY;
                return;
            }


            if (d->character && d->character->application_status == APPLICATION_REVISING) {
                SEND_TO_Q("That option is invalid.\n\r", d);
                SEND_TO_Q("Choose thy fate: ", d);
                return;
            }

            if ((pChInfo = lookup_char_info(d->player_info, d->character->name)) == NULL) {
                add_new_char_info(d->player_info, d->character->name,
                                  d->character->application_status);
                pChInfo = lookup_char_info(d->player_info, d->character->name);
            }

            /* review process functions */
            if (pChInfo->status == APPLICATION_DENY) {
                if (d->character->player.deny) {
                    SEND_TO_Q("Your character has been denied for the following reasons:\n\r", d);
                    SEND_TO_Q(d->character->player.deny, d);
                } else {
                    SEND_TO_Q("Your character has been denied from playing Armageddon.", d);
                }
                free_char(d->character);
                d->character = NULL;
                SEND_TO_Q("\n\r\n\rPress RETURN to continue.", d);
                STATE(d) = CON_WAIT_SLCT;
                return;
            } else if (pChInfo->status == APPLICATION_ACCEPT && d->character->player.deny) {
                SEND_TO_Q("The following note was attached to the approval of"
                          " your character:\n\r", d);
                SEND_TO_Q(d->character->player.deny, d);
            } else if (pChInfo->status == APPLICATION_APPLYING) {
                SEND_TO_Q("Your character has not been reviewed yet.\n\r"
                          "It will be reviewed within 24 hours of applying.\n\r", d);
                free_char(d->character);
                d->character = NULL;
                SEND_TO_Q("\n\r\n\rPress RETURN to continue.", d);
                STATE(d) = CON_WAIT_SLCT;
                return;
            } else if (pChInfo->status == APPLICATION_STORED) {
                SEND_TO_Q("Your character is in storage, you cannot login"
                          " with them until\n\ryou fix it.", d);
                free_char(d->character);
                d->character = NULL;
                SEND_TO_Q("\n\r\n\rPress RETURN to continue.", d);
                STATE(d) = CON_WAIT_SLCT;
                return;
            } else if (pChInfo->status == APPLICATION_DEATH) {
                SEND_TO_Q
                    ("     Your character has been killed.  If you want, you can create a new\n\r"
                     "character, but you still must play the new character as an individual, and\n\r"
                     "not as a \"ressurrection\" of your old character.  In order to do this,\n\r"
                     "select 'R' from the main menu.\n\r", d);
                free_char(d->character);
                d->character = NULL;
                SEND_TO_Q("\n\r\n\rPress RETURN to continue.", d);
                STATE(d) = CON_WAIT_SLCT;
                return;
            }

            if (IS_SET(d->character->specials.act, CFL_DEAD)) {
                SEND_TO_Q
                    ("     Your character has been killed.  If you want, you can create a new\n\r"
                     "character, but you still must play the new character as an individual, and\n\r"
                     "not as a \"ressurrection\" of your old character.  In order to do this, select 'R' from the main menu.\n\r",
                     d);
                update_char_info(d->player_info, d->character->name, APPLICATION_DEATH);
                save_player_info(d->player_info);
                free_char(d->character);
                d->character = NULL;
                SEND_TO_Q("\n\rPress RETURN to continue.", d);
                STATE(d) = CON_WAIT_SLCT;
                break;
            }

            if (lock_mortal)
                if ((!IS_IMMORTAL(d->character) && (!has_mortal_access(d->character)))) {
                    SEND_TO_Q("Armageddon is currently closed to mortals.\n\r", d);
                    SEND_TO_Q("\n\rChoose thy fate: ", d);
                    break;
                }

            /*if( diffport 
             * && !IS_IMMORTAL(d->character) 
             * && stricmp(d->host, HOMEHOST))
             * {
             * SEND_TO_Q("Armageddon is currently closed to mortals.\n\r", d);
             * SEND_TO_Q("\n\rChoose thy fate: ", d);
             * break;
             * } */

            /* check for multiplaying */
            if (!IS_SET(d->player_info->flags, PINFO_CAN_MULTIPLAY))
                for (tmpd = descriptor_list; tmpd; tmpd = tmpd->next)
                    if (tmpd->player_info == d->player_info && tmpd != d && tmpd->character
                        && strcmp(GET_NAME(tmpd->character), GET_NAME(d->character))) {
                        SEND_TO_Q("\n\rMultiplaying is not allowed!\n\r"
                                  "Press RETURN to continue.", d);
                        STATE(d) = CON_WAIT_SLCT;
                        return;
                    }


            if (!IS_IMMORTAL(d->character)
                && d->character->player.race == RACE_DWARF && (!d->character->player.info[1]
                                                               || (d->character->player.info[1]
                                                                   && d->character->player.
                                                                   info[1][0] == '\0'))) {
                SEND_TO_Q("\n\rYou must have an objective due to your dwarven nature in order"
                          " to begin play.\n\r\n\rChoose thy fate: ", d);
                break;
            }

            if (IS_IMMORTAL(d->character)) {
                GET_COND(d->character, FULL) = -1;
                GET_COND(d->character, THIRST) = -1;
            }


            SEND_TO_Q(motd, d);
            SEND_TO_Q("Press RETURN to continue", d);
            add_listener(d->character); /* if char is a listener, add
                                         * char to the listen_list */

            STATE(d) = CON_WAIT_MOTD;
            break;

        case 'w':
        case 'W':
            if (!d->character) {
                SEND_TO_Q("First you must enter thy character's name.\n\r", d);
                SEND_TO_Q("\n\rPress RETURN to continue: ", d);
                break;
            }
            view_players(d);
            STATE(d) = CON_WAIT_SLCT;
            break;

        case 's':
        case 'S':
            if (!d->character) {
                SEND_TO_Q("First you must enter thy character's name.\n\r", d);
                SEND_TO_Q("\n\rChoose thy fate: ", d);
            } else {
                strcpy(buf2, "");
                sprintf(buf, "\n\rThese are your character's text strings:\n\r");
                strcat(buf2, buf);
                sprintf(buf, "Name: %s\n\rKeywords: %s\n\r", d->character->name,
                        d->character->player.extkwds);
                strcat(buf2, buf);


                d->character->long_descr = strdup("temp long desc");

                /* I added the above line so that long_descr would have
                 * a value  -tenebrius 
                 * 
                 * This gave an error when a dude tried to create a char.
                 * -Tenebrius
                 */

                sprintf(buf, "Short desc: %s\n\rLong desc: %s\n\r", d->character->short_descr,
                        d->character->long_descr);
                strcat(buf2, buf);


                free(d->character->long_descr);
                d->character->long_descr = NULL;
                sprintf(buf, "Description:\n\r %s\n\r", d->character->description);
                strcat(buf2, buf);

                sprintf(buf, "Height: %d (inches) and Weight: %d (ten-stone)\n\r",
                        (int) d->character->player.height, (int) d->character->player.weight);
                strcat(buf2, buf);

                sprintf(buf, "Race: %s\n\rGuild: %s and Subguild: %s\n\r",
                        race[(int) d->character->player.race].name,
                        guild[(int) d->character->player.guild].name,
                        sub_guild[(int) d->character->player.sub_guild].name);
                strcat(buf2, buf);
                strcat(buf2, "Press return to continue.\n\r");
                SEND_TO_Q(buf2, d);
                STATE(d) = CON_WAIT_SLCT;
            }
            break;

        case 'o':
        case 'O':
            /* Options available for char creation */
            if (d->player_info) {
                if (IS_SET(d->player_info->flags, PINFO_NO_NEW_CHARS)) {
                    SEND_TO_Q("That option is invalid.\n\r", d);
                    SEND_TO_Q("Choose thy fate:  ", d);
                    break;
                }

                SEND_TO_Q("You have the following options available for character creation:\n\r",
                          d);

                SEND_TO_Q("Races:\n\r", d);
                show_race_selection(d);
                SEND_TO_Q("\n\rGuilds:\n\r", d);
                show_guild(d);

                SEND_TO_Q("\n\rPress return to continue.\n\r", d);
                STATE(d) = CON_WAIT_SLCT;
                break;
            }

            SEND_TO_Q("That option is invalid.\n\r", d);
            SEND_TO_Q("Choose thy fate:  ", d);
            break;
        case 'p':
        case 'P':
            if (d->player_info 
             && IS_SET(d->player_info->flags, PINFO_CONFIRMED_EMAIL)) {
                SEND_TO_Q("Changing account password.\n\r", d);
                SEND_TO_Q("Enter a password for your account: ", d);
                ECHO_OFF(d);
                STATE(d) = CON_GET_NEW_ACCOUNT_PASSWORD;
                break;
            }

            SEND_TO_Q("That option is invalid.\n\r", d);
            SEND_TO_Q("Choose thy fate: ", d);
            break;
        case 'd':
        case 'D':
#ifdef USE_DOCS
            if (d->term)
                SEND_TO_Q(doc_menu, d);
            else
                SEND_TO_Q(doc_menu_no_ansi, d);
#else
            SEND_TO_Q("\n\rHa Ha Ha, no docs.\n\r", d);
            SEND_TO_Q("\n\rPress RETURN to continue.", d);
#endif
            STATE(d) = CON_DOC;
            break;
        default:
            SEND_TO_Q("That option is invalid.\n\r", d);
            SEND_TO_Q("Choose thy fate: ", d);
            break;
        }
        break;
    case CON_DNET:
        break;

    case CON_QRACE:
        /* skip whitespaces */
        for (; isspace(*arg); arg++);

        *arg = LOWER(*arg);

        if (!karma_race_ok(d->player_info->karmaRaces | d->player_info->tempKarmaRaces, *arg)) {
            SEND_TO_Q
                ("\n\rYou do not have enough karma to play this race.\n\r"
                 "Please choose a different letter for your character's race: ",
                 d);
            STATE(d) = CON_QRACE;
            return;
        }

        /* clear tribe if they made a mul or BW desert elf */
        if (d->character->clan)
            remove_clan(d->character, d->character->clan->clan);

        switch (*arg) {
        case 'a':
            GET_RACE(d->character) = RACE_HUMAN;
            break;
        case 'b':
            GET_RACE(d->character) = RACE_ELF;
            break;
        case 'c':
            GET_RACE(d->character) = RACE_DWARF;
            break;

        case 'd':
            GET_RACE(d->character) = RACE_HALFLING;
/*
	      SEND_TO_Q("\n\rThat race is currently disabled.\n\rPlease type the letter of your character's race: ", d);
	      STATE(d) = CON_QRACE;
	      return;
*/
            break;

        case 'e':
            GET_RACE(d->character) = RACE_HALF_ELF;
            break;
        case 'f':
            GET_RACE(d->character) = RACE_MUL;
            break;
        case 'g':
            GET_RACE(d->character) = RACE_HALF_GIANT;
            break;

        case 'h':
            GET_RACE(d->character) = RACE_MANTIS;
/*
	     SEND_TO_Q( "\n\rThat race is currently disabled.\n\r"
              "Please type the letter of your character's race: ", d);
             STATE(d) = CON_QRACE;
             return;
*/
            break;

        case 'i':
            GET_RACE(d->character) = RACE_GITH;
            break;

        case 'j':
            GET_RACE(d->character) = RACE_DRAGON;
            break;

        case 'k':
            GET_RACE(d->character) = RACE_AVANGION;
            break;

        case 'l':
            GET_RACE(d->character) = RACE_DESERT_ELF;
            break;

        case 'm':
            GET_RACE(d->character) = RACE_DESERT_ELF;
            add_clan(d->character, TRIBE_BLACKWING);
            d->character->player.hometown = CITY_BW;
            break;

        default:
            SEND_TO_Q("\n\rThat is an invalid race selection.\n\r"
                      "Please type the letter of your character's race: ", d);
            STATE(d) = CON_QRACE;
            return;
        }

        if (GET_RACE(d->character) == RACE_MUL) {
            SEND_TO_Q("All muls are slaves, or ex-slaves.  Choose the city-state in\n\r"
                      "which you would like to be a slave of.  When you enter the\n\r"
                      "game, if you choose this city-state, you will start as a slave\n\r"
                      "there.  If you choose a different location, you will start as\n\r"
                      "a run-away slave.\n\r" "\n\r  Current options are:\n\rA) Allanak\n\r\n\rEnter Choice: ",
                      d);
            // replaced with the above line 10/21/2000 by nessalin
            // So that muls can no longer select Tuluk as their home city
            //"\n\rCurrent options are:\n\rA) Allanak\n\rB) Tuluk\n\r\n\r>", d); 
            STATE(d) = CON_MUL_SLAVE_CHOICE;
        } else if (GET_RACE(d->character) == RACE_HALF_ELF) {
            SEND_TO_Q("Half elves can appear to look like a human, elf or half-elf.\n\r"
                      "You may choose to appear as any of these options, however your\n\r"
                      "descriptions must reflect this choice.\n\r"
                      "Choose one of:\n\r"
                      "   A) human    B) half-elf   C) elf\n\r\n\r"
                      "Enter choice: ",
                      d);
            STATE(d) = CON_HALF_ELF_APP_CHOICE;
        } else {
            if (d->character->application_status == APPLICATION_REVISING
                && race_guild_ok(d->character, GET_GUILD(d->character))) {
                
                if (d->character->player.hometown > 0) {
                   tmphometown = d->character->player.hometown;     // Preserve the hometown
                   init_char(d->character, 0);                      // Init the character
                   d->character->player.hometown = tmphometown;     // Reset the hometown
                } else {
                   init_char(d->character, 0);
                }
               
                print_height_info(d);
                STATE(d) = CON_HEIGHT;
                return;
            }

            show_guild(d);
            SEND_TO_Q("Please type the letter of your character's guild: ", d);
            STATE(d) = CON_QGUILD;
        }

        break;

    case CON_HALF_ELF_APP_CHOICE:
        {
            int race = RACE_HALF_ELF;

            /* skip whitespaces */
            for (; isspace(*arg); arg++);

            *arg = LOWER(*arg);
            switch (*arg) {
            case 'a':    /* Human */
            case 'A':
               race = RACE_HUMAN;
               break;
            case 'b':    /* Half-elf */
            case 'B':
               race = RACE_HALF_ELF;
               break;
            case 'c':    /* Elf */
            case 'C':
               race = RACE_ELF;
               break;
            default:
               SEND_TO_Q("Invalid choice, valid options are:\n\r"
                      "   A) human    B) half-elf   C) elf\n\r\n\r"
                      "Enter choice: ",
                      d);
               STATE(d) = CON_HALF_ELF_APP_CHOICE;
               return;
            }   

            sprintf( buf, "%d", race);
            set_char_extra_desc_value(d->character, "[HALF_ELF_APP]", buf);

            if (d->character->application_status == APPLICATION_REVISING
                && race_guild_ok(d->character, GET_RACE(d->character))) {
                
                if (d->character->player.hometown > 0) {
                   tmphometown = d->character->player.hometown;     // Preserve the hometown
                   init_char(d->character, 0);                      // Init the character
                   d->character->player.hometown = tmphometown;     // Reset the hometown
                } else {
                   init_char(d->character, 0);
                }

                print_height_info(d);
                STATE(d) = CON_HEIGHT;
                return;
            }

            show_guild(d);
            SEND_TO_Q("Please type the letter of your character's guild: ", d);
            STATE(d) = CON_QGUILD;
            break;
        }
    case CON_MUL_SLAVE_CHOICE:
        {
            int clan;

            /* skip whitespaces */
            for (; isspace(*arg); arg++);

            *arg = LOWER(*arg);
            switch (*arg) {

            case 'a':          /* Allanak */
            case 'A':
                switch (number(1, 10)) {
                case 1:
                    clan = TRIBE_HOUSE_FALE_SLAVE;
                    break;
                case 2:
                    clan = TRIBE_ALLANAK_SLAVE;
                    break;
                default:
                    clan = TRIBE_HOUSE_BORSAIL_SLAVE;
                }
                break;
                // Removed 10/21/2000 by nessalin
                // So that muls can no longer pick Tuluk as their home state
                //        case 'b':  /* Tuluk */
                //        case 'B':
                //              clan =  TRIBE_HOUSE_REYNOLTE_SLAVE;
                //          break;
            default:
                SEND_TO_Q
                    ("Invalid slave city.  Please choose the letter that\n\r corresponds to the city-state you wish to be a slave from.\n\r",
                     d);
                STATE(d) = CON_MUL_SLAVE_CHOICE;
                return;
            }

            add_clan(d->character, clan);

            if (d->character->application_status == APPLICATION_REVISING
                && race_guild_ok(d->character, GET_RACE(d->character))) {

                if (d->character->player.hometown > 0) {
                   tmphometown = d->character->player.hometown;     // Preserve the hometown
                   init_char(d->character, 0);                      // Init the character
                   d->character->player.hometown = tmphometown;     // Reset the hometown
                } else {
                   init_char(d->character, 0);
                }

                print_height_info(d);
                STATE(d) = CON_HEIGHT;
                return;
            }

            show_guild(d);
            SEND_TO_Q("Please type the letter of your character's guild: ", d);
            STATE(d) = CON_QGUILD;
            break;
        }

    case CON_WAIT_DOC:
#ifdef USE_DOCS
        if (d->term)
            SEND_TO_Q(doc_menu, d);
        else
            SEND_TO_Q(doc_menu_no_ansi, d);
#endif
        STATE(d) = CON_DOC;
        break;

    case CON_DOC:
        /* Documentation menu is no longer supported in the code, only on
         * the web.  - Tiernan 3/05/03 */
#ifdef USE_DOCS
        for (; *arg == ' '; arg++);

        switch (*arg) {
        case 'a':
        case 'A':
            page_string(d, rp, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'b':
        case 'B':
            page_string(d, intro, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'c':
        case 'C':
            page_string(d, history, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'd':
        case 'D':
            page_string(d, tuluk, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'e':
        case 'E':
            page_string(d, allanak, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'f':
        case 'F':
            page_string(d, villages, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'g':
        case 'G':
            page_string(d, people, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'h':
        case 'H':
            page_string(d, magick, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'i':
        case 'I':
            page_string(d, start, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'j':
        case 'J':
            page_string(d, profession, 1);
            STATE(d) = CON_WAIT_DOC;
            break;
        case 'x':
        case 'X':
            show_main_menu(d);
            STATE(d) = CON_SLCT;
            break;
        case '?':
            if (d->term == VT100)
                SEND_TO_Q(doc_menu, d);
            else
                SEND_TO_Q(doc_menu_no_ansi, d);
            break;
        default:
            SEND_TO_Q("That option is invalid.\n\r", d);
            SEND_TO_Q("Choose a topic: ", d);
            break;
        }

#else
        show_main_menu(d);
        STATE(d) = CON_SLCT;
#endif /* USE_DOCS */
        break;

    case CON_GET_ACCOUNT_NAME:
        smash_tilde(arg);
        sprintf(buf, "%s", capitalize(arg));
        trim_string(buf);

        if (d->player_info) {
            PLAYER_INFO *pPInfo = d->player_info;

            d->player_info = NULL;
            free_player_info(pPInfo);
        }

        if (strlen(buf) < 3) {
            SEND_TO_Q("Your account name must be at least 3 characters long."
                      "\n\rPress RETURN to continue", d);
            STATE(d) = CON_WAIT_SLCT;
            return;
        }

        if ((pPInfo = find_player_info(buf)) == NULL) {
            if ((fOld = load_player_info(d, buf)) == FALSE) {
                if (_parse_name(buf, buf)) {
                    SEND_TO_Q("\n\rIllegal name for a new account.\n\r", d);
                    SEND_TO_Q("Enter your account name:  ", d);
                    return;
                }

                /* Didn't find them, must be new */
                d->player_info = alloc_player_info();
                d->player_info->name = strdup(capitalize(buf));

                SEND_TO_Q(account_info, d);

                SEND_TO_Q("\n\rAccount not found, is this a new account?  ", d);
                STATE(d) = CON_CONFIRM_NEW_ACCOUNT;
                return;
            }
        } else
            d->player_info = pPInfo;

        if (!d->brief_menus) {
            d->brief_menus = IS_SET(d->player_info->flags, PINFO_BRIEF_MENUS);
        } else if (d->brief_menus && !IS_SET(d->player_info->flags, PINFO_BRIEF_MENUS)) {
            MUD_SET_BIT(d->player_info->flags, PINFO_BRIEF_MENUS);
            save_player_info(d->player_info);
        }

        if (!d->term && IS_SET(d->player_info->flags, PINFO_ANSI_VT100)) {
            d->term = VT100;
        } else if (d->term && !IS_SET(d->player_info->flags, PINFO_ANSI_VT100)) {
            MUD_SET_BIT(d->player_info->flags, PINFO_ANSI_VT100);
            save_player_info(d->player_info);
        }


        /* we found them, make sure they're who they say they are */
        SEND_TO_Q("Password:  ", d);
        ECHO_OFF(d);
        STATE(d) = CON_GET_ACCOUNT_PASSWORD;
        break;

    case CON_GET_ACCOUNT_PASSWORD:
        SEND_TO_Q("\n\r", d);

        ECHO_ON(d);

        if (arg[0] == '\0') {
            PLAYER_INFO *pPInfo = d->player_info;

            d->player_info = NULL;
            free_player_info(pPInfo);

            show_main_menu(d);
            STATE(d) = CON_SLCT;
            return;
        }

        /* check for null password, and check to see if a blank entered
         * password would work */
        if (!d->player_info->password) {
//            || (d->player_info->password
//                && !strcmp(crypt("", d->player_info->password), d->player_info->password))) {
            SEND_TO_Q("Fatal error concerning your password.  Please contact"
                      " the game's administration to get a new password.", d);
            STATE(d) = CON_WAIT_DISCONNECT;
            return;
        }

        {
            const char *cryptedpass = arg;
            if (strcmp(cryptedpass, d->player_info->password) != 0) {
                SEND_TO_Q("Wrong password.\n\r", d);

                if (++d->pass_wrong < 3) {
                    SEND_TO_Q("Password: ", d);
                    ECHO_OFF(d);
                } else {
                    SEND_TO_Q("If you are having trouble remembering your"
                     " password,\n\rIt's probably abcdef\n\rDisconnecting...\n\r", d);
                    sprintf(buf, "Failed login for %s from %s", d->player_info->name, d->hostname);
                    gamelog(buf);
                    add_failed_login_data(d->player_info, d->hostname);
                    save_player_info(d->player_info);
                    close_socket(d);
                }
                return;
            }
        }


        d->pass_wrong = 0;

         if (d->player_info->last_on[0]) {
            sprintf(buf, "Last on: %s to %s from %s\n\r", d->player_info->last_on[0]->in,
                    d->player_info->last_on[0]->out, d->player_info->last_on[0]->site);
            SEND_TO_Q(buf, d);
        }

        if (d->player_info->last_failed[0]) {
            sprintf(buf, "Last failed login: %s from %s\n\r", d->player_info->last_failed[0]->in,
                    d->player_info->last_failed[0]->site);
            SEND_TO_Q(buf, d);
        }

        // add 'last' login information on the character
        add_in_login_data(d->player_info, d->descriptor, d->hostname);

        if (!IS_SET(d->player_info->flags, PINFO_CONFIRMED_EMAIL)) {
            MUD_SET_BIT(d->player_info->flags, PINFO_CONFIRMED_EMAIL);

            if (!d->player_info->characters)
                convert_player_information(d->player_info);

            save_player_info(d->player_info);
            SEND_TO_Q("Enter a new password for your account:  ", d);
            ECHO_OFF(d);
            STATE(d) = CON_GET_NEW_ACCOUNT_PASSWORD;
            return;
        }

        sprintf(buf, "\n\rWelcome back %s.\n\r", d->player_info->name);
        SEND_TO_Q(buf, d);
        if (d->player_info->num_chars_alive + d->player_info->num_chars_applying > 0) {
            STATE(d) = CON_SLCT;
            menu_interpreter(d, "c");
            return;
        }

        SEND_TO_Q("\n\rPress RETURN to continue.", d);
        STATE(d) = CON_WAIT_SLCT;
        break;

    case CON_GET_NEW_ACCOUNT_NAME:
        smash_tilde(arg);
        sprintf(buf, "%s", capitalize(arg));
        trim_string(buf);

        if (d->player_info) {
            PLAYER_INFO *pPInfo = d->player_info;

            d->player_info = NULL;
            free_player_info(pPInfo);
        }

        if (strlen(buf) < 3) {
            SEND_TO_Q("Your account name must be at least 3 characters long."
                      "\n\rPress RETURN to continue", d);
            STATE(d) = CON_WAIT_SLCT;
            return;
        }

        if ((pPInfo = find_player_info(buf)) == NULL) {
            if ((fOld = load_player_info(d, buf)) == FALSE) {
                if (_parse_name(buf, buf)) {
                    SEND_TO_Q("\n\rIllegal name for a new account.\n\r", d);
                    SEND_TO_Q("Please enter a name for your account:  ", d);
                    return;
                }

                /* Didn't find them, so we're good */
                d->player_info = alloc_player_info();
                d->player_info->name = strdup(capitalize(buf));

                SEND_TO_Q("\n\rAre you sure you want to create this account?  ", d);
                STATE(d) = CON_CONFIRM_NEW_ACCOUNT;
                return;
            }
        } else {                /* Someone beat 'em to it, try again. */

            SEND_TO_Q("\n\rThat name is currently in use, please choose another.\n\r", d);
            SEND_TO_Q("Please enter a name for your account:  ", d);
            return;
        }
        break;

    case CON_CONFIRM_NEW_ACCOUNT:
        if (arg[0] == 'y' || arg[0] == 'Y') {
            /*
             * set up a new player info
             */
            SEND_TO_Q("\n\r" "Email - not used in EasyArm. A file is created though.", d);
            SEND_TO_Q("\n\r", d);
            SEND_TO_Q("Enter your e-mail address:  ", d);
            STATE(d) = CON_EMAIL_VERIFY;
            return;
        }

        {
            PLAYER_INFO *pPInfo = d->player_info;

            d->player_info = NULL;
            free_player_info(pPInfo);
        }

        SEND_TO_Q("\n\rPress RETURN to continue.", d);
        STATE(d) = CON_WAIT_SLCT;
        break;

    case CON_GET_NEW_ACCOUNT_PASSWORD:
#if defined(unix)
        SEND_TO_Q("\n\r", d);
#endif

        ECHO_ON(d);
        if (strlen(arg) < 5) {
            sprintf(buf, "Password must be at least five characters long.\n\rPassword:  ");
            ECHO_OFF(d);
            SEND_TO_Q(buf, d);
            return;
        }

        pwdnew = arg;
        if (!pwdnew || (pwdnew && pwdnew[0] == '\0')) {
            sprintf(buf, "Password is not acceptable.  Please try again.\n\rPassword:  ");
            ECHO_OFF(d);
            SEND_TO_Q(buf, d);
            return;
        }

        for (pPwd = pwdnew; *pPwd != '\0'; pPwd++) {
            if (*pPwd == '~') {
                sprintf(buf, "Password is not acceptable.  Please try again.\n\rPassword:  ");
                ECHO_OFF(d);
                SEND_TO_Q(buf, d);
                return;
            }
        }

        free(d->player_info->password);
        d->player_info->password = strdup(pwdnew);
        SEND_TO_Q("Please confirm the password:  ", d);
        ECHO_OFF(d);
        STATE(d) = CON_CONFIRM_NEW_ACCOUNT_PASSWORD;
        break;

    case CON_CONFIRM_NEW_ACCOUNT_PASSWORD:

#if defined(unix)
        SEND_TO_Q("\n\r", d);
#endif

        ECHO_ON(d);

        /* check the password */
        if (strcmp(arg, d->player_info->password)) {
            sprintf(buf, "The passwords do not match.  Please try");
            strcat(buf, " again.\n\r");
            SEND_TO_Q(buf, d);
            SEND_TO_Q("Enter a password for your account:  ", d);
            ECHO_OFF(d);
            STATE(d) = CON_GET_NEW_ACCOUNT_PASSWORD;
            return;
        }

        save_player_info(d->player_info);
        SEND_TO_Q("\n\rPress RETURN to continue.", d);
        STATE(d) = CON_WAIT_SLCT;
        break;

    case CON_CONFIRM_DELETE:
        ECHO_ON(d);
        for (; isspace(*arg); arg++);

        if (!*arg) {
            SEND_TO_Q("Deletion cancelled.\n\rPress RETURN to continue.", d);
            STATE(d) = CON_WAIT_SLCT;
            return;
        } else {
            if (strcmp(arg, d->player_info->password)
                && !stricmp(d->host, HOMEHOST)) {
                SEND_TO_Q("Wrong password.\n\r", d);
                SEND_TO_Q("Deletion cancelled.\n\rPress RETURN to continue.", d);
                STATE(d) = CON_WAIT_SLCT;
                return;
            }

            sprintf(buf, "%s/%s/%s", ACCOUNT_DIR, d->player_info->name,
                    lowercase(d->character->name));
            unlink(buf);

            remove_char_info(d->player_info, lowercase(d->character->name));
            save_player_info(d->player_info);
            free_char(d->character);
            d->character = NULL;
            SEND_TO_Q("Character deleted.\n\rPress RETURN to continue.", d);
            STATE(d) = CON_WAIT_SLCT;
            return;
        }
        break;

    case CON_REVISE_CHAR_MENU:
        revise_char_menu(d, arg);
        return;

    case CON_OPTIONS_MENU:
        option_menu(d, arg);
        return;

    case CON_GET_PAGE_LEN:
        if (is_number(arg)) {
            if (d->character)
                d->character->pagelength = atoi(arg);

            print_option_menu(d);
            STATE(d) = CON_OPTIONS_MENU;
            break;
        } else {
            int i;

            for (i = d->character ? GET_PAGELENGTH(d->character)
                 : d->pagelength; i > 0; i--) {
                sprintf(buf, "%d\n\r", i);
                SEND_TO_Q(buf, d);
            }
            SEND_TO_Q("Enter the number at the top of your screen: ", d);
            return;
        }
        break;

    case CON_GET_EDIT_END:
        for (; isspace(*arg); arg++);

        if (arg[0] == '\0') {
            SEND_TO_Q("\n\rEnter the character you want to end editing with: ", d);
            return;
        }

        d->player_info->edit_end = arg[0];
        print_option_menu(d);
        d->connected = CON_OPTIONS_MENU;
        break;

    case CON_WAIT_DISCONNECT:
        close_socket(d);
        return;

    case CON_WAIT_MOTD:
        send_to_char(welcome_msg, d->character);

        if (d->term && IS_SET(d->character->specials.act, CFL_INFOBAR)) {
            setup_infobar(d);
            display_infobar(d);
        }

        d->character->prev = (CHAR_DATA *) 0;
        d->character->next = character_list;

        character_list = d->character;
        if (d->character->next)
            d->character->next->prev = d->character;

        char_to_room(d->character, home_room(d->character));

        if (IS_AFFECTED(d->character, CHAR_AFF_BURROW) ||
	    affected_by_spell(d->character, SPELL_BURROW))
            set_char_position(d->character, POSITION_RESTING);

        for (p = 0; p < MAX_WEAR; p++)
            d->character->equipment[p] = (struct obj_data *) 0;

        load_char_mnts_and_objs(d->character);

        if (d->character->carrying)
            p = 0;
        else
            for (p = 0; (p < MAX_WEAR) && !d->character->equipment[p]; p++);

        /* if the poor soul doesn't have any objects */
        if ((p == MAX_WEAR)
            && (!IS_IMMORTAL(d->character) && (GET_RACE_TYPE(d->character) == RTYPE_HUMANOID))) {
              tmpobj = read_object(4, VIRTUAL);
                if (tmpobj) {
                    tmpobj->obj_flags.value[0] = get_char_size(d->character);
                    equip_char(d->character, tmpobj, WEAR_BODY);
                    tmpobj = read_object(12, VIRTUAL);
                    tmpobj->obj_flags.value[2] = get_char_size(d->character);
                    equip_char(d->character, tmpobj, WEAR_LEGS);
                }
            
        } 
        d->character->specials.was_in_room = NULL;

        if (d->character->application_status == APPLICATION_ACCEPT) {
            /* set the character as alive and update the pinfo */
            d->character->application_status = APPLICATION_NONE;

            update_char_info(d->player_info, lowercase(d->character->name), APPLICATION_NONE);

            save_player_info(d->player_info);
            adjust_new_char_upon_login(d->character);

            if( d->player_info->num_chars == 1 ) {
               qgamelogf(QUIET_CONNECT, 
                "%s (%s) is a first-time player entering the game!",
                d->character->name, d->character->account);
            }
        }

        adjust_char_upon_login(d->character);

        login_character(d->character);

        new_event(EVNT_SAVE, SAVE_DELAY, d->character, 0, 0, 0, 0);
        new_event(EVNT_NPC_PULSE, PULSE_MOBILE, d->character, 0, 0, 0, 0);

        sprintf(buf, "%s (%s) [%s] has entered the world.", GET_NAME(d->character),
                d->player_info->name, d->hostname);
        shhlog(buf);
        sprintf(buf, "/* %s (%s) [%s] has entered the world. */\n\r", GET_NAME(d->character),
                d->player_info->name, d->hostname);

        connect_send_to_higher(buf, STORYTELLER, d->character);
        act("$n has entered the world.", TRUE, d->character, 0, 0, TO_ROOM);
        STATE(d) = CON_PLYNG;

        if (d->character->in_room) {
            snprintf(buf, sizeof(buf),
                     "%s has entered the world (Room: %d - %s), "
                     "(HP: %d, Stun: %d), Guild: %s\n\r", GET_NAME(d->character),
                     d->character->in_room->number, d->character->in_room->name,
                     GET_HIT(d->character), GET_STUN(d->character),
                     guild[(int) d->character->player.guild].name);
            send_to_monitor(d->character, buf, MONITOR_OTHER);

            if (d->character->in_room->number == city[CITY_NONE].room) {
                send_to_char("A voice whispers, \"Read the tales upon the walls.\"\n\r",
                             d->character);
                clearTempKarmaOptions(d->character);
            }
        }

        cmd_look(d->character, "", 15, 0);

        if( get_char_mood(d->character, buf, sizeof(buf))) {
            cprintf(d->character, "Resetting your mood to a neutral state.\n\r" );
            change_mood(d->character, NULL);
        }

        d->prompt_mode = 1;

        /* Temp fix.  -Nessalin - 6/20/97 */
        /*            if (!(d->character->specials.quiet_level))
         * d->character->specials.quiet_level = 0; */

        break;
    case CON_VT100:
        if (!d->character) {
            SEND_TO_Q("First you must enter thy character's name.\n\r", d);
            SEND_TO_Q("\n\rChoose thy fate: ", d);
            break;
        }

        d->term = 1 - d->term;

        show_main_menu(d);
        STATE(d) = CON_SLCT;

        break;

    case CON_AUTO_SDESC:{
            int i;
            char sdesc[MAX_STRING_LENGTH];

            for (; *arg == ' '; arg++);
            trim_string(arg);

            if (!*arg) {
                SEND_TO_Q("     Please enter your character's short description below:\n\r> ", d);
                break;
            }

            if (strlen(arg) > 35) {
                SEND_TO_Q("That is over 35 characters; try again: \n\r> ", d);
                break;
            }

            for (i = 0; i < strlen(arg); i++)
                arg[i] = LOWER(arg[i]);

            if (GET_RACE(d->character) != RACE_HALFLING && str_prefix("the", arg)) {
                sprintf(sdesc, "the %s", smash_article(arg));
            } else {
                strcpy(sdesc, arg);
            }

            if (d->character->short_descr)
                free((char *) d->character->short_descr);

	    /* get rid of any blanks at the end of it */
	    smash_blanks(sdesc);

            /* dupe their input and get rid of any periods in it */
            d->character->short_descr = smash_char(strdup(sdesc), '.');

            SEND_TO_Q("\n\r", d);

            if (d->character->application_status != APPLICATION_REVISING) {
                print_height_info(d);
                STATE(d) = CON_HEIGHT;
            } else {
                print_revise_char_menu(d);
                STATE(d) = CON_REVISE_CHAR_MENU;
            }
            break;
        }

    case CON_AUTO_EXTKWD:
        if (d->character->application_status == APPLICATION_REVISING) {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
            break;
        }

        print_height_info(d);
        STATE(d) = CON_HEIGHT;
        break;

    case CON_HEIGHT:
        switch (GET_RACE(d->character)) {
        case RACE_HUMAN:
            min = 62;
            max = 78;
            break;
        case RACE_ELF:
            min = 74;
            max = 90;
            break;
        case RACE_DESERT_ELF:
            min = 74;
            max = 90;
            break;
        case RACE_DWARF:
            min = 50;
            max = 58;
            break;
        case RACE_HALF_ELF:
            min = 70;
            max = 82;
            break;
        case RACE_HALFLING:
            min = 36;
            max = 44;
            break;
        case RACE_HALF_GIANT:
            min = 125;
            max = 155;
            break;
        case RACE_MUL:
            min = 60;
            max = 72;
            break;
        case RACE_MANTIS:
            min = 83;
            max = 86;
            break;
        case RACE_GITH:
            min = 78;
            max = 86;
            break;
        default:
            min = 62;
            max = 78;
            break;
        }

        i = atoi(arg);
        if ((i < min) || (i > max)) {
            sprintf(buf,
                    "Characters of your race must be between %d and %d inches in height.\n\rHow tall is your character? ",
                    min, max);
            SEND_TO_Q(buf, d);
            break;
        }

        d->character->player.height = i;

        print_weight_info(d);
        STATE(d) = CON_WEIGHT;
        break;

    case CON_WEIGHT:
        switch (GET_RACE(d->character)) {
        case RACE_HUMAN:
            min = 6;
            max = 9;
            break;
        case RACE_GITH:
        case RACE_ELF:
            min = 7;
            max = 9;
            break;
        case RACE_DESERT_ELF:
            min = 7;
            max = 9;
            break;
        case RACE_DWARF:
            min = 8;
            max = 10;
            break;
        case RACE_HALF_ELF:
            min = 6;
            max = 9;
            break;
        case RACE_HALFLING:
            min = 4;
            max = 6;
            break;
        case RACE_HALF_GIANT:
            min = 75;
            max = 90;
            break;
        case RACE_MUL:
            min = 10;
            max = 15;
            break;
        case RACE_MANTIS:
            min = 22;
            max = 23;
            break;
        default:
            min = 6;
            max = 9;
            break;
        }

        i = atoi(arg);
        if ((i < min) || (i > max)) {
            sprintf(buf,
                    "Characters of your race must be between %d and %d ten-stone in weight.\n\rHow much does your character weigh in ten-stone? ",
                    min, max);
            SEND_TO_Q(buf, d);
            break;
        }
        d->character->player.weight = i;

        print_age_info(d);
        STATE(d) = CON_AGE;
        break;

    case CON_AGE:
        min = (race[(int) d->character->player.race].max_age * 20) / 100;
        max = (race[(int) d->character->player.race].max_age * 85) / 100;
        i = atoi(arg);
        if ((i < min) || (i > max)) {
            sprintf(buf,
                    "That is not a valid age.\n"
                    "Please pick an age for you character between %d and %d.\n\n"
                    "How old would you like your character to be ?  ", min, max);
            SEND_TO_Q(buf, d);
            STATE(d) = CON_AGE;
            break;
        }

        d->character->player.time.starting_age = i;

        if (d->character->application_status == APPLICATION_REVISING) {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
            return;
        }

        SEND_TO_Q("\n\r\n\r", d);
        SEND_TO_Q(origin_info, d);

        SEND_TO_Q("Pick your character's origin.\n\r", d);

        if (d->character->player.race == RACE_DESERT_ELF) {
            SEND_TO_Q("g) Desert Elf tribal location.\n\r\n\r", d);
        } else if (d->character->player.race == RACE_ELF) {
            SEND_TO_Q("a) Allanak.\n\r"
                      "b) Tuluk.\n\r"
                      "c) Luir's Outpost.\n\r"
                      "d) Red Storm Village.\n\r"
                      "e) The Labyrinth.\n\r", d);
        } else if (d->character->player.race == RACE_MUL) {
            SEND_TO_Q("a) Allanak.\n\r\n\r", d);
        } else if (d->character->player.race == RACE_GITH) {
            SEND_TO_Q("h) Gith Mesa.\n\r\n\r", d);
        } else {
            SEND_TO_Q("a) Allanak.\n\r"
                      "b) Tuluk.\n\r"
                      "c) Luir's Outpost.\n\r"
                      "d) Red Storm Village.\n\r"
                      "e) The Labyrinth.\n\r"
                      "f) A nomadic location.\n\r\n\r", d);
        }
        SEND_TO_Q("What is your character's origin location?\n\r", d);
        STATE(d) = CON_LOCATION;
        break;

    case CON_LOCATION:

        switch (*arg) {
            case 'a':
            case 'A':
                d->character->player.hometown = 1;
                SEND_TO_Q("Origin set to Allanak.\n\r", d);
                break;
            case 'b':
            case 'B':
                d->character->player.hometown = 2;
                SEND_TO_Q("Origin set to Tuluk.\n\r\n\r", d);
                break;
            case 'c':
            case 'C':
                d->character->player.hometown = 4;
                SEND_TO_Q("Origin set to Luir's Outpost.\n\r", d);
                break;
            case 'd':
            case 'D':
                d->character->player.hometown = 5;
                SEND_TO_Q("Origin set to Red Storm Village.\n\r", d);
                break;
            case 'e':
            case 'E':
                d->character->player.hometown = 18;
                SEND_TO_Q("Origin set to The Labyrinth.\n\r", d);
                break;
            case 'g':
            case 'G':
                d->character->player.hometown = CITY_DELF_OUTPOST;
                SEND_TO_Q("Origin set to Desert Elf tribal location.\n\r", d);
                break;
            case 'h':
            case 'H':
                d->character->player.hometown = CITY_CAVERN;
                SEND_TO_Q("Origin set to Gith Mesa Clan location.\n\r", d);
                break;
            case 'f':
            case 'F':
            default:
                d->character->player.hometown = 0;
                SEND_TO_Q("Origin set to Tribal.", d);
                break;
        }

        if (d->character->player.hometown == 2) {
            SEND_TO_Q(tattoo_info, d);
            SEND_TO_Q("You can get your Gol Krathu citizen tattoo on your choice of:\n\r", d);
            SEND_TO_Q("a) Your neck.\n\r", d);
            SEND_TO_Q("b) Your throat.\n\r", d);
            SEND_TO_Q("c) Your face.\n\r", d);
            SEND_TO_Q("d) Your right shoulder.\n\r", d);
            SEND_TO_Q("e) Your left shoulder.\n\r\n\r", d);
            SEND_TO_Q("Where is your Gol Krathu tattoo?\n\r", d);
            STATE(d) = CON_TULUK_TATTOO;
        } else {

            if (d->character->application_status != APPLICATION_REVISING) {
                SEND_TO_Q(background_info, d);
                if (GET_RACE(d->character) == RACE_DESERT_ELF)
                    SEND_TO_Q("To be approved, you _must_ include information about" " your tribe.\n\r\n\r",
                              d);
                if (GET_RACE(d->character) == RACE_DWARF)
                    SEND_TO_Q("To be approved, you _must_ include information about" " your focus.\n\r\n\r",
                              d);
                SEND_TO_Q("Enter your Background.\n\r", d);
                string_edit(d->character, &d->character->background, MAX_BACKGROUND_LENGTH);
                d->connected = CON_AUTO_BACKGROUND_END;
            } else {
                print_revise_char_menu(d);
                STATE(d) = CON_REVISE_CHAR_MENU;
            }
        }
        break;

    case CON_TULUK_TATTOO:{

        char band_tatt_sdesc[MAX_STRING_LENGTH];
        strcpy(band_tatt_sdesc, "a blue and purple inked band");
        char band_tatt_ldesc[MAX_STRING_LENGTH];
        strcpy(band_tatt_ldesc, "This long band stretches wide across the bearer's flesh, a colorful tone of "
                                 "light purple making up the fill for the small wedges that follow all along the "
                                 "tattoo's length. Interspersed along the inking and set between every purple "
                                 "marking, a simple blue crescent has been inked in. Drawn with painstaking care "
                                 "despite its simplicity, the banded design is comprised of nothing more than a "
                                 "long series of the two lively and colorful shapes, each one after the other.\n~");
        char star_tatt_sdesc[MAX_STRING_LENGTH];
        strcpy(star_tatt_sdesc, "a tattoo of a six-pronged star");
        char star_tatt_ldesc[MAX_STRING_LENGTH];
        strcpy(star_tatt_ldesc, "Lying in the center of this six-pronged star is a brilliant red sun, its image "
                                 "detailed in varying shades of crimson and maroon. Surrounding the image are "
                                 "six triangular prongs, their blackened edges licked by the flames of the sun "
                                 "within.\n~");

        switch (*arg) {
            case 'a':
            case 'A':
                set_char_extra_desc_value(d->character, "[NECK_LDESC]", band_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[NECK_SDESC]", band_tatt_sdesc);
                set_char_extra_desc_value(d->character, "[HANDS_LDESC]", star_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[HANDS_SDESC]", star_tatt_sdesc);
                break;
            case 'b':
            case 'B':
                set_char_extra_desc_value(d->character, "[THROAT_LDESC]", band_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[THROAT_SDESC]", band_tatt_sdesc);
                set_char_extra_desc_value(d->character, "[HANDS_LDESC]", star_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[HANDS_SDESC]", star_tatt_sdesc);
                break;

            case 'c':
            case 'C':
                set_char_extra_desc_value(d->character, "[FACE_LDESC]", band_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[FACE_SDESC]", band_tatt_sdesc);
                set_char_extra_desc_value(d->character, "[HANDS_LDESC]", star_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[HANDS_SDESC]", star_tatt_sdesc);
                break;

            case 'd':
            case 'D':
                set_char_extra_desc_value(d->character, "[SHOULDER_R_LDESC]", band_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[SHOULDER_R_SDESC]", band_tatt_sdesc);
                set_char_extra_desc_value(d->character, "[HANDS_LDESC]", star_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[HANDS_SDESC]", star_tatt_sdesc);
                break;

            case 'e':
            case 'E':
                set_char_extra_desc_value(d->character, "[SHOULDER_L_LDESC]", band_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[SHOULDER_L_SDESC]", band_tatt_sdesc);
                set_char_extra_desc_value(d->character, "[HANDS_LDESC]", star_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[HANDS_SDESC]", star_tatt_sdesc);
                break;

            default:
                set_char_extra_desc_value(d->character, "[NECK_LDESC]", band_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[NECK_SDESC]", band_tatt_sdesc);
                set_char_extra_desc_value(d->character, "[HANDS_LDESC]", star_tatt_ldesc);
                set_char_extra_desc_value(d->character, "[HANDS_SDESC]", star_tatt_sdesc);
                break;
        }

        if (d->character->application_status != APPLICATION_REVISING) {
            SEND_TO_Q(background_info, d);
            if (GET_RACE(d->character) == RACE_DESERT_ELF)
                SEND_TO_Q("To be approved, you _must_ include information about" " your tribe.\n\r\n\r",
                          d);
            if (GET_RACE(d->character) == RACE_DWARF)
                SEND_TO_Q("To be approved, you _must_ include information about" " your focus.\n\r\n\r",
                          d);
            SEND_TO_Q("Enter your Background.\n\r", d);
            string_edit(d->character, &d->character->background, MAX_BACKGROUND_LENGTH);
            d->connected = CON_AUTO_BACKGROUND_END;
        } else {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
        }


   }
   break;

    case CON_BACKGROUND:

/* - Going to do this after guild selection, so sorc's can choose a lesser
     profession as well.

      init_char(d->character, 0);
*/
        save_char_waiting_auth(d->character);
        free_char(d->character);
        d->character = (CHAR_DATA *) 0;

        show_main_menu(d);
        d->connected = CON_DOC;


        break;

    case CON_EMAIL_VERIFY:
        smash_tilde(arg);
        trim_string(arg);

        for (i = 0; i < strlen(arg); i++) {
            arg[i] = LOWER(arg[i]);
            if (!isalnum(arg[i]) && arg[i] != '-' && arg[i] != '_' && arg[i] != '@'
                && arg[i] != '.') {
                SEND_TO_Q("Invalid Email.\n\rEnter your email: ", d);
                return;
            }
        }

        if (d->player_info->email && arg[0] == '\0' && !is_free_email(d->player_info->email)) {
            SEND_TO_Q("Aborting email change.\n\rPress RETURN to continue.", d);
            STATE(d) = CON_WAIT_SLCT;
            return;
        }

        if (strlen(arg) > 79) {
            SEND_TO_Q("Email address must be less than 80 characters\n\r", d);
            SEND_TO_Q("\n\rEnter a new email for your account:  ", d);
            break;
        }

        if (strlen(arg) < 5) {
            SEND_TO_Q("Email address must be more than 5 characters\n\r", d);
            SEND_TO_Q("\n\rEnter a new email for your account:  ", d);
            break;
        }

            struct stat stbuf;

            /* this shouldn't be here as at this point there should
             * never be a d->character   -Morg
             * d->character->player.info[0] = strdup(arg);  */

            SEND_TO_Q("\n\r", d);

            sprintf(buf, "%s/%s", ALT_ACCOUNT_DIR, arg);
            if (lstat(buf, &stbuf) != -1) {
                SEND_TO_Q("\n\rThat email is already registered.\n\r", d);

                SEND_TO_Q("\n\rEnter your e-mail address:  ", d);
                return;
            }

            /* have they already confirmed their email? */
            if (IS_SET(d->player_info->flags, PINFO_CONFIRMED_EMAIL)) {
                /* Remove the bit and tell them */
                REMOVE_BIT(d->player_info->flags, PINFO_CONFIRMED_EMAIL);
                SEND_TO_Q("\n\rWe have received your request to change your"
                          " email.\n\rWe have created a new password for you and mailed it"
                          " to the address\n\ryou provided.\n\r", d);
                sprintf( buf, "Player has changed their email from %s.", d->player_info->email );
                add_account_comment(NULL, d->player_info, buf);

            }

            if (d->player_info->email) {
                sprintf(buf, "%s/%s/%s", LIB_DIR, ALT_ACCOUNT_DIR, d->player_info->email);

                sprintf(buf2, "%s/%s/%s", LIB_DIR, ALT_ACCOUNT_DIR, arg);

                rename(buf, buf2);

                free(d->player_info->email);
                d->player_info->email = strdup(arg);

                /* scramble a new password and mail to account */
                scramble_and_mail_account_password(d->player_info, NULL);

                SEND_TO_Q("\n\rPress RETURN to disconnect", d);
                STATE(d) = CON_WAIT_DISCONNECT;
            } else {
                d->player_info->email = strdup(arg);
                STATE(d) = CON_WHERE_SURVEY_Q;
            }

        break;

    case CON_WHERE_SURVEY_Q:
        {
            long ct;
            char *tmstr;
            for (; isspace(*arg); arg++);
            trim_string(arg);
            /* answered or not, create the account */
            ct = time(0);
            tmstr = asctime(localtime(&ct));
            tmstr[strlen(tmstr) - 1] = '\0';

            if (d->player_info->created)
                free(d->player_info->created);

            d->player_info->created = strdup(tmstr);

            SEND_TO_Q("\n\rAccounts are auto-approved. abcdef is the starting password. Still have to logout to complete process.\n\r", d);


            /* scramble a new password and mail to account */
            scramble_and_mail_account_password(d->player_info, NULL);

            /* If creating new chars is open... */
            if (lock_new_char) {
                SEND_TO_Q("No new characters are being accepted at this time.\n\rPlease come back later and create your character.\n\r", d);
                SEND_TO_Q("\n\rPress RETURN to disconnect.", d);
                STATE(d) = CON_WAIT_DISCONNECT;
                break;
            } 
            /* let's move them on to char creation, they can validate their
               account later
             */
            else {
                free_char(d->character);
                d->character = 0;
                SEND_TO_Q(app_info, d);
                SEND_TO_Q("\n\rWhat name do you wish to call yourself? ", d);
                STATE(d) = CON_NEWNAME;

                break;
            }

            STATE(d) = CON_NEWNAME;
            break;
        }

    case CON_EMAIL:
        STATE(d) = CON_WAIT_SLCT;
        SEND_TO_Q("Press RETURN to continue.", d);
        break;

    case CON_DEAD_NAME:
        STATE(d) = CON_WAIT_SLCT;
        SEND_TO_Q("Press RETURN to continue.", d);
        break;

    case CON_DEAD_PASS:
        STATE(d) = CON_WAIT_SLCT;
        SEND_TO_Q("Press RETURN to continue.", d);
        break;

    case CON_OBJECTIVE:
        for (; isspace(*arg); arg++);
        trim_string(arg);

        if (!*arg)
            SEND_TO_Q("     Your objective has not been changed.\n\r", d);
        else {
            if (strlen(arg) > 70)
                arg[70] = '\0';

            if (d->character->player.info[1])
                free(d->character->player.info[1]);

            d->character->player.info[1] = strdup(arg);

            if (d->character->application_status == APPLICATION_REVISING) {
                SEND_TO_Q("\n\rObjective changed.\n\r", d);
                print_revise_char_menu(d);
                STATE(d) = CON_REVISE_CHAR_MENU;
                return;
            }

            SEND_TO_Q("\n\rYour objective has been changed to:\n\r    ", d);
            SEND_TO_Q(d->character->player.info[1], d);
            SEND_TO_Q("\n\r", d);

            {
                char name[200];
                int i;
                int in_game = TRUE;
                struct char_data *tch;

                for (i = 0; i < strlen(name); i++)
                    name[i] = LOWER(name[i]);

                strcpy(name, d->character->name);

                if (player_exists(name, d->player_info->name, APPLICATION_NONE)
                    || player_exists(name, d->player_info->name, APPLICATION_ACCEPT)) {

                    tch = (CHAR_DATA *) malloc(sizeof *tch);
                    tch->name = name;
                    in_game = 0;
                } else {
                    gamelogf("menu_change_objective: Couldn't find player %s",
                             d->player_info->name);
                    return;
                }

                sprintf(buf, "%s/%s", ACCOUNT_DIR, d->player_info->name);

                if (!open_char_file(tch, buf)) {
                    gamelogf("menu_change_objective: Unable to open char file" " for '%s'.", tch->name);
                    return;
                }

                if (!read_char_file()) {

                    gamelogf("cmd_pinfo: Unable to read char file %s", buf);
                    close_char_file();
                } else {
                    write_char_field_f("CHAR", 1, "INFO 1", "%s", arg);

                    write_char_file();

                    close_char_file();
                    return;
                }
            }
        }

        SEND_TO_Q("\n\r\n\rChoose thy fate: ", d);
        STATE(d) = CON_SLCT;
        break;

    case CON_WAIT_CONFIRM_EXIT:
        while (*arg == ' ')
           arg++;

        if (*arg == 'y' || *arg == 'Y') { 
            if (d->character && d->character->name
             && lookup_char_info(d->player_info, d->character->name))
                remove_char_info(d->player_info, d->character->name);

            save_player_info(d->player_info);
            free_char(d->character);
            d->character = NULL;

            show_main_menu(d);
            STATE(d) = CON_SLCT;
        } else {
            print_revise_char_menu(d);
            STATE(d) = CON_REVISE_CHAR_MENU;
        }
        break;

    default:
        gamelog("menu_interpreter: illegal state of con'ness");
        abort();
    }
}



void
print_height_info(DESCRIPTOR_DATA * d)
{
    int min;
    int max;
    char buf[MAX_STRING_LENGTH];

    switch (GET_RACE(d->character)) {
    case RACE_HUMAN:
        min = 62;
        max = 78;
        break;
    case RACE_ELF:
        min = 74;
        max = 90;
        break;
    case RACE_DESERT_ELF:
        min = 74;
        max = 90;
        break;
    case RACE_DWARF:
        min = 50;
        max = 58;
        break;
    case RACE_HALF_ELF:
        min = 70;
        max = 82;
        break;
    case RACE_HALFLING:
        min = 36;
        max = 44;
        break;
    case RACE_HALF_GIANT:
        min = 125;
        max = 155;
        break;
    case RACE_MUL:
        min = 60;
        max = 72;
        break;
    case RACE_MANTIS:
        min = 83;
        max = 86;
        break;
    case RACE_GITH:
        min = 78;
        max = 86;
        break;
    default:
        min = 62;
        max = 78;
        break;
    }

    SEND_TO_Q("\n\rPlease enter your character's height below.  On Zalanthas, length is\n\r"
              "measured in inches, which (for all practical purposes) are equivalent\n\r"
              "to the English measurement of the same name.  Character height is\n\r"
              "currently used for armor sizing and will probably be implemented more\n\r"
              "extensively in the future.  Please make your height consistent with\n\r"
              "your character descriptions.\n\r\n\r", d);

    sprintf(buf, "Characters of your race are typically between %d and %d" " inches tall.\n\r", min,
            max);
    SEND_TO_Q(buf, d);
    SEND_TO_Q("\n\rHow tall is your character? ", d);
    return;
}


void
print_weight_info(DESCRIPTOR_DATA * d)
{
    int min;
    int max;
    char buf[MAX_STRING_LENGTH];

    switch (GET_RACE(d->character)) {
    case RACE_HUMAN:
        min = 6;
        max = 9;
        break;
    case RACE_GITH:
    case RACE_ELF:
        min = 7;
        max = 9;
        break;
    case RACE_DESERT_ELF:
        min = 7;
        max = 9;
        break;
    case RACE_DWARF:
        min = 8;
        max = 10;
        break;
    case RACE_HALF_ELF:
        min = 6;
        max = 9;
        break;
    case RACE_HALFLING:
        min = 4;
        max = 6;
        break;
    case RACE_HALF_GIANT:
        min = 75;
        max = 90;
        break;
    case RACE_MUL:
        min = 10;
        max = 15;
        break;
    case RACE_MANTIS:
        min = 22;
        max = 23;
        break;
    default:
        min = 6;
        max = 9;
        break;
    }

    SEND_TO_Q("\n\rPlease enter your character's weight below.  On Zalanthas,"
              " weight is\n\rmeasured in units called \"stones\", where one stone is"
              " approximately\n\requal to a kilogram.  For larger weights, \"five-stone"
              "\" and \"ten-stone\"\n\rare often used instead.  Character weight is"
              " currently used for armor\n\rsizing and will probably be implemented"
              " more extensively in the future.\n\rPlease make your weight consistent"
              " with your character descriptions.\n\r\n\r", d);

    sprintf(buf, "Characters of your race typically weigh between %d and %d" " ten-stone.\n\r", min,
            max);
    SEND_TO_Q(buf, d);
    SEND_TO_Q("\n\rHow much does your character weigh? ", d);
    return;
}


void
print_age_info(DESCRIPTOR_DATA * d)
{
    int min, max;
    char buf[MAX_STRING_LENGTH];
    char *tmp;

    min = (race[(int) d->character->player.race].max_age * 20) / 100;
    max = (race[(int) d->character->player.race].max_age * 85) / 100;

    sprintf(buf,
            "\n\rCharacters of race %s usually live around %d years."
            "  Please pick an age for\n\ryou character between %d and %d.\n\r",
            race[(int) d->character->player.race].name,
            race[(int) d->character->player.race].max_age, min, max);

    tmp = format_string(strdup(buf));

    SEND_TO_Q(tmp, d);
    SEND_TO_Q("\n\r", d);
    free(tmp);

    SEND_TO_Q("Note that your age should be reflected in your descriptions above and"
              " that\n\ryour stats will be affected by your", d);

    if (GET_RACE(d->character) != RACE_HUMAN) {
        sprintf(buf,
                " proportional age.\n\r\n\r"
                "A %d year old %s(lifespan %d) will be at about the same stage of"
                " development\n\ras an %d year old human (lifespan %d).  Keep this"
                " in mind when choosing your\n\rage or you will accidentally end up"
                " with a statistically weak character.\n\r\n\r",
                (race[(int) d->character->player.race].max_age / 4),
                race[(int) d->character->player.race].name,
                race[(int) d->character->player.race].max_age, (race[RACE_HUMAN].max_age / 4),
                race[RACE_HUMAN].max_age);
        tmp = format_string(strdup(buf));

        SEND_TO_Q(tmp, d);
        free(tmp);
    } else {
        SEND_TO_Q(" age.\n\r\n\r"
                  "Humans age very similarly to humans in our world.  Keep this in mind"
                  " when\n\rchoosing your age or you will acccidentally end up with a"
                  " statistically\n\rweak character.\n\r\n\r", d);
    }

    SEND_TO_Q(" We strongly recommend that beginning players choose an age close\n\r"
              " to the middle of the range, for this choice will give your character"
              " the\n\rchance to survive more easily and consequently give you a"
              " chance to focus more\n\ron roleplaying than on staying alive.  As of"
              " yet, skills are -not- affected by\n\rage.\n\r", d);

    SEND_TO_Q("\n\rHow old would you like your character to be ?  ", d);
}

void
adjust_new_char_upon_login(CHAR_DATA * ch)
{
    if (!ch)
        return;

    // Elves can't ride
    if( GET_RACE(ch) == RACE_ELF || GET_RACE(ch) == RACE_DESERT_ELF ) {
       set_skill_percentage(ch, SKILL_RIDE, 0);
    }

    MUD_SET_BIT(ch->specials.nosave, NOSAVE_ARREST);
}

/* This is a gigantic hack of monumental proportions.
 *
 * This used to be infused in the menu_interpreter for when a character is
 * logging into the game world.  It's been extracted here so that all the
 * "temporary character updates" that have to be done to automatically
 * update characters to something that's new can be kept in one place and
 * (hopefully) removed easier when there's no further need for the adjustment.
 * - Tiernan 9/30/2005
 */
void
adjust_char_upon_login(CHAR_DATA * ch)
{
    int charbios = 0;
    int st, wi, ag;
    int i, accent;
    bool accent_ok = FALSE;

    if (!ch)
        return;

    // Apply any base-level guild and subguild skills which are missing
    for (i = 1; i < MAX_SKILLS; i++) {
        if (!has_skill(ch, i)) {
            if (guild[(int) ch->player.guild].skill_prev[i] == SPELL_NONE)
                init_skill(ch, i, guild[(int) ch->player.guild].skill_percent[i]);
            else if (sub_guild[(int) ch->player.sub_guild].skill_prev[i] == SPELL_NONE) {
                init_skill(ch, i, sub_guild[(int) ch->player.sub_guild].skill_percent[i]);
            }
        }
    }

    if( ch->skills[PSI_CONTACT] ) {
       if( ch->skills[PSI_CONTACT]->learned < guild[(int) ch->player.guild].skill_percent[PSI_CONTACT]) {
           ch->skills[PSI_CONTACT]->learned = guild[(int) ch->player.guild].skill_percent[PSI_CONTACT];
       }
    }

    // Alcohol tolerance isn't in any guild tree
    if (!ch->skills[TOL_ALCOHOL])
        init_skill(ch, TOL_ALCOHOL, get_char_size(ch));

    if (!ch->skills[TOL_PAIN]) {
        init_skill(ch, TOL_PAIN, GET_END(ch));
        set_skill_taught(ch, TOL_PAIN);
    }

    // Reach skills for magickers
    if (is_magicker(ch)) {
        if (!has_skill(ch, SKILL_REACH_NONE)) {
            init_skill(ch, SKILL_REACH_NONE, 100);
        }

        if (!has_skill(ch, SKILL_REACH_LOCAL)) {
            init_skill(ch, SKILL_REACH_LOCAL, 100);
        }
    }

    // Do they have any accents already?
    for (i = 0; accent_table[i].skillnum != LANG_NO_ACCENT; i++) {
        if (has_skill(ch, accent_table[i].skillnum)
            && ch->skills[accent_table[i].skillnum]->learned >= MIN_ACCENT_LEVEL) {
            // No accent set yet?  Set it!
            if (GET_ACCENT(ch) == LANG_NO_ACCENT)
                ch->specials.accent = accent_table[i].skillnum;
            accent_ok = TRUE;   // They have an accent, it's all good
        }
    }

    // No accent skills! See if they should have one and if so, set it
    if (!accent_ok && ((accent = default_accent(ch)) != LANG_NO_ACCENT)) {
        init_skill(ch, accent, 100);
        if (GET_ACCENT(ch) == LANG_NO_ACCENT)
            ch->specials.accent = accent;
    }

    /* Update any desert elves to give them the desert accent if they do
     * not already have it.
     */
    if (GET_RACE(ch) == RACE_DESERT_ELF && !has_skill(ch, LANG_DESERT_ACCENT))
       init_skill(ch, LANG_DESERT_ACCENT, 100);
 

    /* Remove shadowwalking affect since you're reconnecting from a lost
     * link or crash.  You can't maintain this affect w/o a shadow NPC to
     * be switched into.
     */
    if (affected_by_spell(ch, PSI_SHADOW_WALK))
        affect_from_char(ch, PSI_SHADOW_WALK);


    /* Convert Blackwing to desert elves */
    if (IS_TRIBE(ch, 25)
        && GET_RACE(ch) == RACE_ELF) {
        /* All these values could get changed by switching race */
        st = GET_STR(ch);
        wi = GET_WIS(ch);
        ag = GET_AGL(ch);
        GET_RACE(ch) = RACE_DESERT_ELF;
        roll_abilities(ch);
        ch->abilities.str = st;
        ch->abilities.wis = wi;
        ch->abilities.agl = ag;
        SET_STR(ch, st);
        SET_WIS(ch, wi);
        SET_AGL(ch, ag);
        send_to_char("The emerald-eyed elf appears before you.\n\r", ch);
        send_to_char("The emerald-eyed elf looks at you and frowns.\n\r", ch);
        send_to_char("The emerald-eyed elf says to you, in allundean:\n\r"
                     "     \"No, that body will not do!  I must correct you immediately!\"" "\n\r",
                     ch);
        send_to_char("You feel an incredible pain surge through your body.\n\r", ch);
        send_to_char("The emerald-eyed elf looks at you and nods.\n\r", ch);
        send_to_char("The emerald-eyed elf slowly fades from view.\n\r", ch);
        parse_command(ch, "save");
    }

    /* Give watch skill to those who don't have it */
    if (!has_skill(ch, SKILL_WATCH)) {
        // Give them 5%
        init_skill(ch, SKILL_WATCH, 5);
        // always set it taught, otherwise they would have got it above
        set_skill_taught(ch, SKILL_WATCH);
    }

    // ride > 50% and doesn't already have trample
    if (has_skill(ch, SKILL_RIDE) && get_skill_percentage(ch, SKILL_RIDE) > 50
     && !has_skill(ch, SKILL_TRAMPLE)) {
        // Give them 10%
        init_skill(ch, SKILL_TRAMPLE, 10);
        // always set it taught, otherwise they would have got it above
        set_skill_taught(ch, SKILL_TRAMPLE);
    }

    // Grandfather slashing skill for pick-pockets
    if (GET_GUILD(ch) == GUILD_PICKPOCKET && has_skill(ch, PROF_SLASHING)) {
        set_skill_taught(ch, PROF_SLASHING);
    }

    if (GET_LEVEL(ch) > MORTAL && GET_LEVEL(ch) < STORYTELLER ) {
        ch->specials.il = GET_LEVEL(ch);
    }

    PLAYER_INFO *pInfo = ch->desc->player_info;
    if( pInfo->karma <= 5 
     && IS_SET(pInfo->karmaGuilds, KINFO_GUILD_WIND_CLERIC ) ) {
       REMOVE_BIT(pInfo->karmaGuilds, KINFO_GUILD_WIND_CLERIC);
    }

    if( (pInfo->karma == 4 || pInfo->karma == 5)
     && !IS_SET(pInfo->karmaGuilds, KINFO_GUILD_SHADOW_CLERIC ) ) {
       MUD_SET_BIT(pInfo->karmaGuilds, KINFO_GUILD_SHADOW_CLERIC);
    }
}

