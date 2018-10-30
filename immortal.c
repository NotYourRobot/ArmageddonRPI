/*
 * File: IMMORTAL.C
 * Usage: Privileged immortal commands.
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
 * 05/05/2000 More spells (godspeed, detect invisible, detect etheral,
 *            identify) show up on stat.  -Sanvean
 * 05/15/2000 Added slow to things that show up on stat.  -San
 * 05/18/2000 Added Shield of Nilaz as well.  -San
 * 07/22/2000 Modified command snoop -San.
 * 07/29/2000 Energy shield and infravision show on stat.  -San
 * 08/03/2000 Fury also shows on stat.  -San
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/times.h>
#include <errno.h>
#define _XOPEN_SOURCE           /* needed for crypt() */
#include <unistd.h>

#include "constants.h"
#include "structs.h"
#include "barter.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "echo.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "db_file.h"
#include "forage.h"
#include "skills.h"
#include "limits.h"
#include "guilds.h"
#include "clan.h"
#include "event.h"
#include "cities.h"
#include "pc_file.h"
#include "vt100c.h"
#include "creation.h"
#include "modify.h"
#include "object_list.h"
#include "io_thread.h"
#include "player_accounts.h"
#include "immortal.h"
#include "watch.h"
#include "cmd_getopt.h"
#include "info.h"
#include "special.h"

extern int get_room_max_weight(ROOM_DATA * room);
extern int get_room_base_weight(ROOM_DATA * room);
extern void unset_medesc(CHAR_DATA * ch, CHAR_DATA * tch, char *arg);
extern void set_medesc(CHAR_DATA * ch, CHAR_DATA * tch, char *arg);

FILE *obj_f;
FILE *mob_f;

#define CHAR_CAN_REROLL

char *
find_email_site(char *s)
{
    struct ban_t *ef;
    char email[256];

    for (ef = email_list; ef; ef = ef->next) {
        one_argument(ef->name, email, sizeof(email));
        if (!strcasecmp(email, s))
            return (ef->name);
    }
    return NULL;
}


void
immlog(CHAR_DATA * ch, const char *argument, int cmd)
{
    char buf[512];
    int roomnum = -1;
    if (ch->in_room)
        roomnum = ch->in_room->number;

    sprintf(buf, "[%d] (%s) %s: %s %s", roomnum, ch->desc ? ch->name : "linkdead",
            ch->desc ? ch->desc->player_info->name : "linkdead", command_name[cmd], argument);
    log_to_file(buf, "immlog");
}

void
cmd_email(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], site[256], output[512], email[256];
    struct ban_t *et;


    argument = one_argument(argument, buf, sizeof(buf));

    switch (buf[0]) {
    case 's':                  /* show addresses */
        for (et = email_list; et; et = et->next) {
            sprintf(site, "%s", et->name);
            send_to_char(site, ch);
        }
        break;
    case 'q':                  /* query */
        argument = one_argument(argument, site, sizeof(site));
        for (et = email_list; et; et = et->next) {
            one_argument(et->name, email, sizeof(email));
            if (string_pos(email, site) >= 0) {
                sprintf(output, "%s\n\r", et->name);
                send_to_char(output, ch);
            }
        }
        send_to_char("--end of list--\n\r", ch);
        break;
    case 'a':                  /* add an address */
        send_to_char("The add email option has been disabled.", ch);
        break;
    default:                   /* no good argument */
        send_to_char("Email sq {regex|address}\n\r(S)how all addresses, (Q)uery regex\n\r", ch);
        break;
    }
}

/* begin on-line reviewing stuff */

/*
 * Command to allow players to ask for feedback
 */
void
cmd_review(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    PLAYER_INFO *pPInfo;
    CHAR_DATA *orig;
    char buf[MAX_STRING_LENGTH];

    if (!ch || !ch->desc)
        return;

    while (isspace(*arg))
        arg++;

    pPInfo = ch->desc->player_info;

    if (ch->desc && ch->desc->original)
        orig = ch->desc->original;
    else
        orig = ch;

    if (IS_IMMORTAL(orig)) {
        char arg1[MAX_STRING_LENGTH];
        bool loaded_account = FALSE;
        CHAR_DATA *vict;

        if (!arg || arg[0] == '\0') {
            send_to_char("Who do you want to set up to be reviewed?\n\r", ch);
            return;
        }

        arg = one_argument(arg, arg1, sizeof(arg1));
        arg1[0] = UPPER(arg1[0]);

        /* look for a char in the world with that name */
        if ((vict = get_char_world(ch, arg1)) != NULL) {
            /* if it's an npc */
            if (IS_NPC(vict)) {
                send_to_char("Not on NPCs.\n\r", ch);
                return;
            }

            /* if the vict is linkless */
            if (!vict->desc) {
                /* see if the account is gamelogged in, but not in the game */
                if ((pPInfo = find_player_info(vict->account)) == NULL) {
                    DESCRIPTOR_DATA d;

                    /* it's not, so load the account */
                    if (!load_player_info(&d, vict->account)) {
                        sprintf(buf, "Error!  Unable to load account '%s'.\n\r", vict->account);
                        send_to_char(buf, ch);
                        return;
                    }

                    /* remember that we had to load it */
                    loaded_account = TRUE;
                    pPInfo = d.player_info;
                }
            } else {            /* grab the pinfo from their descriptor */

                pPInfo = vict->desc->player_info;
            }
        } else {                /* no player with that name on, try it as an account name */

            /* see if the account is gamelogged in */
            if ((pPInfo = find_player_info(arg1)) == NULL) {
                DESCRIPTOR_DATA d;

                /* it's not, so load the account */
                if (!load_player_info(&d, arg1)) {
                    sprintf(buf, "No such character or account '%s'\n\r", arg1);
                    send_to_char(buf, ch);
                    return;
                }

                /* remember that we had to load it */
                loaded_account = TRUE;
                pPInfo = d.player_info;
            }
        }

        /* got the pPInfo, now toggle the bit and give an output */
        if (IS_SET(pPInfo->flags, PINFO_WANTS_RP_FEEDBACK)) {
            /* only HL+ can remove it --commenting out for now, awaiting feedback
             * if( GET_LEVEL( ch ) < HIGHLORD )
             * {
             * send_to_char( "You don't have permission to remove review"
             * " requests.\n\rAsk a Highlord or higher to do this for you.\n\r",
             * ch );
             * 
             * // cleanup
             * if( loaded_account )
             * free_player_info( pPInfo );
             * 
             * return; 
             * } */

            REMOVE_BIT(pPInfo->flags, PINFO_WANTS_RP_FEEDBACK);
            sprintf(buf, "Roleplay review request removed for account '%s'.\n\r", pPInfo->name);
            send_to_char(buf, ch);
        } else {                /* not set, set it */

            MUD_SET_BIT(pPInfo->flags, PINFO_WANTS_RP_FEEDBACK);
            sprintf(buf, "Roleplay review request added to account '%s'.\n\r", pPInfo->name);
            send_to_char(buf, ch);
        }

        save_player_info(pPInfo);
        /* cleanup */
        if (loaded_account)
            free_player_info(pPInfo);
        return;
    } else {                    /* mortal requesting feedback */

        bool turnOn = !IS_SET(pPInfo->flags, PINFO_WANTS_RP_FEEDBACK);

        if (arg != NULL && *arg != '\0' && !str_prefix(arg, "status")) {
            if (!IS_SET(pPInfo->flags, PINFO_WANTS_RP_FEEDBACK))
                send_to_char("Roleplay Review is currently off.\n\r", ch);
            else
                send_to_char("Roleplay Review is currently on.\n\r", ch);
            return;
        }

        if (arg != NULL && *arg != '\0' && !str_prefix(arg, "on")) {
            if (IS_SET(pPInfo->flags, PINFO_WANTS_RP_FEEDBACK)) {
                send_to_char("Roleplay Review is already on.\n\r", ch);
                return;
            }
            turnOn = TRUE;
        } else if (arg != NULL && *arg != '\0' && !str_prefix(arg, "off")) {
            if (!IS_SET(pPInfo->flags, PINFO_WANTS_RP_FEEDBACK)) {
                send_to_char("Roleplay Review is already off.\n\r", ch);
                return;
            }
            turnOn = FALSE;
        } else if (arg != NULL && *arg != '\0') {
            send_to_char("Syntax:  review [on|off|status]\n\r", ch);
            return;
        }

        if (turnOn) {
            send_to_char("You have requested role-play feedback.\n\r", ch);
            send_to_char("Note that this is not an automatic thing, but"
                         " staff will be watching you and\n\rwill provide feedback as"
                         " they feel your play warrants it.\n\r", ch);

            MUD_SET_BIT(pPInfo->flags, PINFO_WANTS_RP_FEEDBACK);
            save_player_info(pPInfo);
        } else {
            send_to_char("Your role-play review request has been removed.\n\r", ch);

            REMOVE_BIT(pPInfo->flags, PINFO_WANTS_RP_FEEDBACK);
            save_player_info(pPInfo);
        }
    }
    return;
}

struct emote_data
{
    char *token;
    CHAR_DATA *ch;
    OBJ_DATA *obj;
    int loc;
    struct emote_data *next;
};

/* used by get_emote_token to determine if it's one of our special emotive
 * characters 
 * ~ - replace the following string with the named character's sdesc
 * ! - replace the following string with the named character's 
 * third or second-person objective pronoun.
 * % - as ~ but possessive
 * ^ - as ! but possessive
 * # - replace the following string with 'he/she'
 * @ - the calling *ch's short description
 */
bool
is_special_char(char c)
{
    if (c == '~'                /* sdesc */
        || c == '!'             /* third objective pronoun */
        || c == '%'             /* possessive ~ */
        || c == '^'             /* possessive ! */
        || c == '#'             /* he/she/it */
	|| c == '&'		/* himself/herself/itself */
        || c == '+'             /* his/hers/its */
        || c == '='             /* sdesc + */
        || c == '@'             /* ch's short desc */
        || c == ','             /* want to break on ',' */
        || c == ';'             /* and ';' */
        || c == ':'             /* and ':' */
        || c == '\''            /* and ''' allows ~object's for possessive */
        || c == '$' /* $ is special formatter used later */ )
        return 1;

    return 0;
}

/* chops off the next sequence of non-whitespace/non-punctuation characters
   and returns it */
char *
get_emote_token(char *arg)
{
    int i, n;
    int arglen = strlen(arg);
    char *token;

    if (arglen == 0)
        return NULL;

    /* for( ;isspace(*arg); arg++) ; */

    /* loop over the argument, looking for special characters or spaces */
    for (i = 1; !is_special_char(arg[i]) && !isspace(arg[i]) && (i < arglen); i++) {
        /* handle periods after things other than periods */
        if (arg[i] == '.' && arg[i + 1] == '\0' )
            break;
    }


    /* is punct will check for $ and other special characters, going to
     * switch it to isspecial()  -Tenebrius
     * 
     * Doh.  It has to check for pucntuation to be able to 'emote test ~bram.'
     * I'm just going to have it replace $'s with S's 
     */
    if (arg[i] == '$')
        arg[i] = 'S';

    token = (char *) malloc((i + 1) * (sizeof *token));
    strncpy(token, arg, i);
    token[i] = '\0';

    for (n = 0; arg[n + i] != '\0'; n++)
        arg[n] = arg[n + i];
    arg[n] = arg[n + i];

    return token;
}


void
free_emote_list(struct emote_data *token_head)
{
    struct emote_data *fp;

    while ((fp = token_head)) {
        token_head = token_head->next;
        if (fp->token)
            free(fp->token);
        free(fp);
    }
}

/* add char to end of char list to be passed to gvprintf */
struct obj_list_type *
add_to_char_list(void *ch, struct obj_list_type *list)
{
    struct obj_list_type *new, *p;

    new = (struct obj_list_type *) malloc(sizeof *new);
    new->obj = ch;
    new->next = NULL;

    if (list) {
        for (p = list; p->next; p = p->next);
        p->next = new;
    } else {
        list = new;
    }
    return list;
}

/* emote_parse appends the argument to the character's name and sends
   it to the room, recognizing the following special characters:
   ~ - replace the following string with the named character's sdesc
   ! - replace the following string with the named character's 
   third or second-person objective pronoun.
   % - as ~ but possessive
   ^ - as ! but possessive
   # - replace the following string with 'he/she'
   @ - the calling *ch's short description
   & - as ! but with the -self suffix, e.g. himself/herself/itself
   + = his/hers/its

   if the pos parameter is set to true, emote_parse adds an "'s" to
   the character's name at the beginning of the emote, in the style of
   the pemote command.
 */
void
emote_parse(CHAR_DATA * ch, const char *arg, int pos, int monitor, bool hidden, bool silent)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    struct watching_room_list *wlist;
    char msg[MAX_STRING_LENGTH];
    char tmp_msg[MAX_STRING_LENGTH];
    char *token;
    struct obj_list_type *p, *char_list = NULL;
    struct emote_data *token_head = (struct emote_data *) malloc(sizeof *token_head);
    struct emote_data *tp, *tail;
    char *argument = strdup(arg);
    char *orig_arg = argument;
    bool fSdesc = FALSE;

    token_head->token = NULL;
    token_head->ch = NULL;
    token_head->next = NULL;
    tail = token_head;

    /* tokenize the list and stick it into the emote struct */
    while ((token = get_emote_token(argument))) {
        tp = (struct emote_data *) malloc(sizeof *tp);
        tp->token = token;
        tp->ch = NULL;
        tp->obj = NULL;
        tp->next = NULL;
        tail->next = tp;
        tail = tp;
    }

    /* return if there are no arguments */
    if (token_head->next == NULL) {
        send_to_char("What do you want to emote ?\n", ch);
        free_emote_list(token_head);
        free(orig_arg);
        return;
    }

    /* find the characters to whom the personal emotes refer */
    for (tp = token_head->next; tp != NULL; tp = tp->next) {
        if (isspecial(tp->token[0]) && (strlen(tp->token) > 1)) {
            if ((tp->loc =
                 generic_find(tp->token + 1,
                              FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch,
                              &tp->ch, &tp->obj)) == 0) {
                send_to_char("You do not see that person.\n\r", ch);
                free_emote_list(token_head);
                free(orig_arg);
                return;
            }
        } else if (tp->token[0] == '@' || !strcasecmp(tp->token, "me")
                   || !strcmp(tp->token, " me")) {
            fSdesc = TRUE;
            tp->ch = ch;
        }
    }

    /* if using the new moveable sdesc code, drop the leading space */
    if (fSdesc && token_head->next->token && isspace(token_head->next->token[0])) {
        int i;

        for (i = 1; i < strlen(token_head->next->token); i++) {
            token_head->next->token[i - 1] = token_head->next->token[i];
        }
        token_head->next->token[i - 1] = '\0';
        token_head->next->token[0] = UPPER(token_head->next->token[0]);
    }

    /* format it for monitor */
    buf[0] = '\0';
    if (hidden) {
        strcat(buf, "[hidden emote] ");
    }

    if (silent) {
        strcat(buf, "[silent emote] ");
    }

    if (!fSdesc) {
        sprintf(buf2, "%s", MSTR(ch, name));
        sprintf(buf2, "#%s%s#", MSTR(ch, name), pos
                && buf2[strlen(buf2) - 1] != 's' ? "'s" : pos ? "'" : "");
        strcat(buf, buf2);
    }

    for (tp = token_head->next; tp; tp = tp->next) {
        if (((tp->ch || tp->obj) && !fSdesc)    /* normal emote */
            ||((tp->ch || tp->obj) && fSdesc    /* variably placed sdesc */
               && tp == token_head->next->next  /* is the first argument */
               && (!strcmp(tp->token, " me") || !strcasecmp(tp->token, "me")
                   || tp->token[0] == '@'))
            || ((tp->ch || tp->obj) && fSdesc && tp != token_head->next->next)) {
            if (fSdesc && (!strcasecmp(tp->token, "me") || !strcmp(tp->token, " me"))) {
                char tmpbuf[MAX_STRING_LENGTH];

                sprintf(tmpbuf, "%s", MSTR(tp->ch, name));
                sprintf(buf2, " #%s%s#", tmpbuf, pos
                        && tmpbuf[strlen(tmpbuf) - 1] != 's' ? "'s" : pos ? "'" : "");
            } else {
                switch (tp->token[0]) {
                case '~':
                    if (tp->obj) {
                        if (tp->loc != FIND_OBJ_ROOM) {
                            sprintf(buf2, "%s ", HSHR(ch));
                            strcat(buf, buf2);
                            sprintf(buf2, "%s", smash_article(OSTR(tp->obj, short_descr)));
                        } else {
                            sprintf(buf2, "%s", OSTR(tp->obj, short_descr));
                        }
                    } else
                        sprintf(buf2, "%s", IS_NPC(tp->ch)
                                ? PERS(ch, tp->ch) : MSTR(tp->ch, name));
                    break;
                case '%':
                    if (tp->ch) {
                        sprintf(buf2, "%s", IS_NPC(tp->ch)
                                ? PERS(ch, tp->ch) : MSTR(tp->ch, name));
                        strcat(buf, buf2);

                        sprintf(buf2, "%s", buf[strlen(buf)] == 's' ? "'" : "'s");
                    } else if (tp->obj) {
                        if (tp->loc != FIND_OBJ_ROOM) {
                            sprintf(buf2, "%s", HSHR(ch));
                            strcat(buf, buf2);
                        }
                        sprintf(buf2, "%s", OSTR(tp->obj, short_descr));
                        strcat(buf, buf2);

                        sprintf(buf2, "%s", buf[strlen(buf)] == 's' ? "'" : "'s");
                    }
                    break;
                case '!':
                    if (tp->ch)
                        sprintf(buf2, "%s", HMHR(tp->ch));
                    break;
		case '&':
                    if (tp->ch)
                        sprintf(buf2, "%s", HMFHRF(tp->ch));
                    break;
		case '=':
                    if (tp->ch) {
                        sprintf(buf2, "%s", IS_NPC(tp->ch)
                                ? PERS(ch, tp->ch) : MSTR(tp->ch, name));
                        strcat(buf, buf2);

                        sprintf(buf2, "%s", buf[strlen(buf)] == 's' ? "'" : "'s");
                    } else if (tp->obj) {
                        if (tp->loc != FIND_OBJ_ROOM) {
                            sprintf(buf2, "%s", HSHRS(ch));
                            strcat(buf, buf2);
                        }
                        sprintf(buf2, "%s", OSTR(tp->obj, short_descr));
                        strcat(buf, buf2);

                        sprintf(buf2, "%s", buf[strlen(buf)] == 's' ? "'" : "'s");
                    }
                    break;
		case '+':
                    if (tp->ch)
                        sprintf(buf2, "%s", HSHRS(tp->ch));
                    break;
                case '^':
                    if (tp->ch)
                        sprintf(buf2, "%s", HSHR(tp->ch));
                    break;
                case '#':
                    if (tp->ch)
                        sprintf(buf2, "%s", HSSH(tp->ch));
                    break;
                case '@':
                    if (tp->ch) {
                        char tmpbuf[MAX_STRING_LENGTH];

                        sprintf(tmpbuf, "%s", MSTR(tp->ch, name));
                        sprintf(buf2, "#%s%s#", tmpbuf, pos
                                && tmpbuf[strlen(tmpbuf) - 1] != 's' ? "'s" : pos ? "'" : "");

                    }
                    break;
                }
            }
        } else
            sprintf(buf2, "%s", tp->token);

        strcat(buf, buf2);

        if (!tp->next && !ispunct(tp->token[strlen(tp->token) - 1])) {
            strcat(buf, ".\n\r");
        } else if (!tp->next)
            strcat(buf, "\n\r");
    }
    send_to_monitor(ch, buf, monitor);

    /* send the beginning of the string to everyone */
    *msg = '\0';
    if (!fSdesc) {
        sprintf(msg, "$ctc");
        if (pos) {
            strcat(msg, "p");
        }
        char_list = add_to_char_list((void *) ch, char_list);
    }

    /* send the tokens to the rooms one at a time, translating as necessary */
    for (tp = token_head->next; tp != NULL; tp = tp->next) {
        if (((tp->ch || tp->obj) && !fSdesc)    /* normal emote */
            ||((tp->ch || tp->obj) && fSdesc    /* variably placed sdesc */
               && tp == token_head->next->next  /* is the first argument */
               && (!strcmp(tp->token, " me") || !strcasecmp(tp->token, "me")
                   || tp->token[0] == '@'))
            || ((tp->ch || tp->obj) && fSdesc && tp != token_head->next->next)) {
            if (fSdesc && (!strcasecmp(tp->token, "me") || !strcmp(tp->token, " me"))) {
                sprintf(buf, "%s$ct%s", isspace(*tp->token) ? " " : "", pos ? "p" : "");
                string_safe_cat(msg, buf, MAX_STRING_LENGTH);
                char_list = add_to_char_list((void *) ch, char_list);
            } else {
                switch (tp->token[0]) {
                case '~':
                    if (tp->obj) {
                        if (tp->loc != FIND_OBJ_ROOM) {
                            string_safe_cat(msg, "$cp $oa", MAX_STRING_LENGTH);
                            char_list = add_to_char_list((void *) ch, char_list);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        } else {
                            string_safe_cat(msg, "$os", MAX_STRING_LENGTH);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        }
                    } else {
                        string_safe_cat(msg, "$cs", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
                    break;
                case '%':
                    if (tp->ch) {
                        string_safe_cat(msg, "$csp", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    } else if (tp->obj) {
                        if (tp->loc != FIND_OBJ_ROOM) {
                            string_safe_cat(msg, "$cp $oap", MAX_STRING_LENGTH);
                            char_list = add_to_char_list((void *) ch, char_list);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        } else {
                            string_safe_cat(msg, "$otp", MAX_STRING_LENGTH);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        }
                    }
                    break;
                case '^':
                    if (tp->ch) {
                        string_safe_cat(msg, "$cp", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
#ifndef EMOTE_PARSE_FIX
                    else if (tp->obj) {
                        string_safe_cat(msg, "$op", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->obj, char_list);
                    }
#endif
                    break;
                case '!':
                    if (tp->ch) {
                        string_safe_cat(msg, "$cm", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
#ifndef EMOTE_PARSE_FIX
                    else if (tp->obj) {
                        string_safe_cat(msg, "$om", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->obj, char_list);
                    }
#endif
                    break;
		case '&':
                    if (tp->ch) {
                        string_safe_cat(msg, "$cl", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
#ifndef EMOTE_PARSE_FIX
                    else if (tp->obj) {
                        string_safe_cat(msg, "$ol", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->obj, char_list);
                    }
#endif
                    break;
                case '=':
                    if (tp->ch) {
                        string_safe_cat(msg, "$csg", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    } else if (tp->obj) {
                        if (tp->loc != FIND_OBJ_ROOM) {
                            string_safe_cat(msg, "$cp $oag", MAX_STRING_LENGTH);
                            char_list = add_to_char_list((void *) ch, char_list);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        } else {
                            string_safe_cat(msg, "$otg", MAX_STRING_LENGTH);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        }
                    }
		    break;
		case '+':
                    if (tp->ch) {
                        string_safe_cat(msg, "$cg", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
#ifndef EMOTE_PARSE_FIX
                    else if (tp->obj) {
                        string_safe_cat(msg, "$og", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->obj, char_list);
                    }
#endif
		    break;
                case '#':
                    if (tp->ch) {
                        string_safe_cat(msg, "$ce", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
#ifndef EMOTE_PARSE_FIX
                    else if (tp->obj) {
                        string_safe_cat(msg, "$oe", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) tp->obj, char_list);
                    }
#endif
                    break;
                case '@':
                    if (tp->ch) {
                        string_safe_cat(msg, "$ct", MAX_STRING_LENGTH);
                        char_list = add_to_char_list((void *) ch, char_list);
                    }

                    if (tp == token_head->next->next)
                        string_safe_cat(msg, "c", MAX_STRING_LENGTH);

                    if (pos)
                        string_safe_cat(msg, "p", MAX_STRING_LENGTH);
                    break;
                default:
                    sprintf(msg, "Bad punct mark in token '%s' in emote '%s'", tp->token, arg);
                    gamelog(msg);
                    send_to_char("Error, bad emote.  Read 'help emote' and try again.\n\r", tp->ch);
                    free_emote_list(token_head);
                    free(orig_arg);
                    return;     /* added this return -tenebrius */

                }
            }
        } else {
            string_safe_cat(msg, tp->token, MAX_STRING_LENGTH);
        }

        /* Added by Morgenes to tack on a '.' if punctuaction is left off */
        if (tp->next == NULL && !ispunct(msg[strlen(msg) - 1])) {
            string_safe_cat(msg, ".", MAX_STRING_LENGTH);
        }
    }

    gvprintf(ch, NULL, ch->in_room, msg, char_list, hidden, silent);

    /* for rooms watching this room */
    for (wlist = ch->in_room->watched_by; wlist; wlist = wlist->next_watched_by) {
	if (IS_SET(wlist->watch_type, WATCH_VIEW)) {
            strcpy(tmp_msg, wlist->view_msg_string);
            string_safe_cat(tmp_msg, msg, MAX_STRING_LENGTH);
            gvprintf(ch, NULL, wlist->room_watching, tmp_msg, char_list, hidden, silent);
	}
    }

    /* free the token_head and its contents */
    while (((p) = (char_list))) {
        char_list = char_list->next;
        free(p);
    }

    free_emote_list(token_head);
    free(orig_arg);
    return;
}


void
cmd_delayed_emote(CHAR_DATA * ch, const char *arg, Command cmd, int count) {
    char arg1[MAX_INPUT_LENGTH];
    const char *orig_arg;
    char args[MAX_INPUT_LENGTH];
    int hidden = 0;
    int silent = 0;
    int possessive = 0;
    int i;
  
    if( !IS_NPC(ch) && !IS_IMMORTAL(ch) ) {
        cprintf(ch, TYPO_MSG );
        return;
    }

    args[0] = '\0';
    orig_arg = arg;
    do {
        orig_arg = one_argument( orig_arg, arg1, sizeof(arg1));
        if( *arg1 && arg1[0] == '-' ) {
            for( i = 1; i < strlen(arg1); i++ ) {
                if( arg1[i] == 'h') {
                    hidden = 1;
                }
                else if( arg1[i] == 's') {
                    silent = 1;
                }
                else if( arg1[i] == 'p') {
                    possessive = 1;
                } else {
                    strcat( args, " " );
                    strcat( args, arg1 );
                    break;
                }
            }
        } else {
            strcat( args, " " );
            strcat( args, arg1 );
        }
    } while( *orig_arg );
    emote_parse(ch, args, possessive, MONITOR_COMMS, hidden, silent);
}

void
cmd_emote(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    emote_parse(ch, arg, 0, MONITOR_COMMS, cmd == CMD_HEMOTE, cmd == CMD_SEMOTE);
}


void
cmd_pemote(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    emote_parse(ch, arg, 1, MONITOR_COMMS, cmd == CMD_PHEMOTE, cmd == CMD_PSEMOTE);
}

void
cmd_poofin(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[400];
    char buf1[256];
    char buf2[256];
    int i;

    if (!*argument) {
        if (ch->player.poofin) {
            sprintf(buf, "Current poofin: %s\n\r", ch->player.poofin);
            send_to_char(buf, ch);
        }
        send_to_char("Change poofin to what?\n\r", ch);
        return;
    }

    for (i = 0; *(argument + i) == ' '; i++);
    if (strlen(argument + i) > 250) {
        sprintf(buf, "To long, please less than 250 characters.\n\r");
        send_to_char(buf, ch);
        return;
    };

    strcpy(buf1, argument + 1);
    strcat(buf1, "\n\r");

    if (ch->player.poofin) {
        if (strlen(ch->player.poofin) < 250)
            strcpy(buf2, ch->player.poofin);
        else
            strcpy(buf2, "TO LONG");

        free((char *) ch->player.poofin);
    };

    ch->player.poofin = strdup(buf1);

    sprintf(buf, "Poofin changed from : %s", buf2);
    send_to_char(buf, ch);
    sprintf(buf, "Poofin set to: %s\n\r", argument + i);
    send_to_char(buf, ch);

}

void
cmd_poofout(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[400];
    char buf1[256];
    char buf2[256];
    int i;

    if (!*argument) {
        if (ch->player.poofout) {
            sprintf(buf, "Current poofout: %s\n\r", ch->player.poofout);
            send_to_char(buf, ch);
        }
        send_to_char("Change poofout to what?\n\r", ch);
        return;
    }


    for (i = 0; *(argument + i) == ' '; i++);

    if (strlen(argument + i) > 250) {
        sprintf(buf, "To long, please less than 250 characters.\n\r");
        send_to_char(buf, ch);
        return;
    };

    strcpy(buf1, argument + 1);
    strcat(buf1, "\n\r");

    if (ch->player.poofout) {
        if (strlen(ch->player.poofout) < 250)
            strcpy(buf2, ch->player.poofout);
        else
            strcpy(buf2, "TO LONG");

        free((char *) ch->player.poofout);
    }

    ch->player.poofout = strdup(buf1);


    sprintf(buf, "Poofout changed from : %s", buf2);
    send_to_char(buf, ch);
    sprintf(buf, "Poofout set to: %s", buf1);
    send_to_char(buf, ch);

}


/* functions for cmd_echo (bram) */

void
echo_all(CHAR_DATA * ch, const char *echo)
{
    static char buf[MAX_STRING_LENGTH];
    struct descriptor_data *d;
    bool indoors = TRUE;        // Default to echo to anyone inside
    bool outdoors = TRUE;       // Default to echo to anyone outside
    char option[MAX_STRING_LENGTH];

    strcpy(buf, echo);          // Put the incoming buffer into buf

    echo = one_argument(echo, option, sizeof(option));  // cut off the first word for parsing
    if (!strcmp(option, "-o") || !strcmp(option, "-O")) {
        indoors = FALSE;        // Don't show indoors, just the outdoors
        strcpy(buf, echo);      // Update buf to remove the option
    } else if (!strcmp(option, "-i") || !strcmp(option, "-I")) {
        outdoors = FALSE;       // Don't shot outdoors, just the indoors
        strcpy(buf, echo);      // Update buf to remove the option
    }

    for (d = descriptor_list; d; d = d->next)
        if ((d->character)) {
            // If we want to echo to people outside, and he is outside
            // or we want to echo to people inside, and he is not outside
            if ((outdoors && OUTSIDE(d->character)) || (indoors && !OUTSIDE(d->character))) {
                echo_char(ch, d->character, buf);
            }
        }
}

void
echo_city(CHAR_DATA * ch, int city, const char *echo)
{
    static char buf[MAX_STRING_LENGTH];
    struct descriptor_data *d;
    bool indoors = TRUE;        // Default to echo to anyone inside
    bool outdoors = TRUE;       // Default to echo to anyone outside
    char option[MAX_STRING_LENGTH];
    strncpy(buf, echo, sizeof(buf));    // Put the incoming buffer into buf

    if (city == CITY_NONE) {
        if (ch)
            cprintf(ch, "You're not in a city.\n\r");
        return;
    }

    echo = one_argument(echo, option, sizeof(option));  // cut off the first word for parsing
    if (!strcmp(option, "-o") || !strcmp(option, "-O")) {
        indoors = FALSE;        // Don't show indoors, just the outdoors
        strcpy(buf, echo);      // Update buf to remove the option
    } else if (!strcmp(option, "-i") || !strcmp(option, "-I")) {
        outdoors = FALSE;       // Don't shot outdoors, just the indoors
        strcpy(buf, echo);      // Update buf to remove the option
    }

    for (d = descriptor_list; d; d = d->next)
        if ((d->character) && (d->character->in_room)
            && (room_in_city(d->character->in_room) == city)) {
            // If we want to echo to people outside, and he is outside
            // or we want to echo to people inside, and he is not outside
            if ((outdoors && OUTSIDE(d->character)) || (indoors && !OUTSIDE(d->character))) {
                echo_char(ch, d->character, buf);
            }
        }
}

void
echo_zone(CHAR_DATA * ch, int zone, const char *echo)
{
    static char buf[MAX_STRING_LENGTH];
    struct descriptor_data *d;
    bool indoors = TRUE;        // Default to echo to anyone inside
    bool outdoors = TRUE;       // Default to echo to anyone outside
    char option[MAX_STRING_LENGTH];
    strncpy(buf, echo, sizeof(buf));    // Put the incoming buffer into buf

    echo = one_argument(echo, option, sizeof(option));  // cut off the first word for parsing
    if (!strcmp(option, "-o") || !strcmp(option, "-O")) {
        indoors = FALSE;        // Don't show indoors, just the outdoors
        strcpy(buf, echo);      // Update buf to remove the option
    } else if (!strcmp(option, "-i") || !strcmp(option, "-I")) {
        outdoors = FALSE;       // Don't shot outdoors, just the indoors
        strcpy(buf, echo);      // Update buf to remove the option
    }

    for (d = descriptor_list; d; d = d->next)
        if ((d->character) &&
            /*      (!IS_SET(d->character->specials.act, CFL_QUIET)) && */
            (d->character->in_room) && (d->character->in_room->zone == zone)) {
            // If we want to echo to people outside, and he is outside
            // or we want to echo to people inside, and he is not outside
            if ((outdoors && OUTSIDE(d->character)) || (indoors && !OUTSIDE(d->character))) {
                echo_char(ch, d->character, buf);
            }
        }
}

void
echo_room(CHAR_DATA *ch, ROOM_DATA *room, const char *echo)
{
    CHAR_DATA *rch;

    if (!room) return;

    for (rch = room->people; rch; rch = rch->next_in_room) {
        echo_char(ch, rch, echo);
    }
}

void
echo_char(CHAR_DATA * ch, CHAR_DATA * recip, const char *echo)
{
    static char buf[MAX_STRING_LENGTH];
    static char immbuf[MAX_STRING_LENGTH];
    char *sender_name = "(script)";

    if (ch) {
        sender_name = MSTR(ch, name);
    }

    snprintf(buf, sizeof(buf), "%s\n\r", echo);
    snprintf(immbuf, sizeof(immbuf), "<%s>: %s", sender_name, buf);
    if (ch) {
        if (GET_LEVEL(SWITCHED_PLAYER(ch)) > GET_LEVEL(SWITCHED_PLAYER(recip)))
            send_to_char_parsed(ch, recip, buf, IS_IMMORTAL(recip));
        else
            send_to_char_parsed(ch, recip, immbuf, IS_IMMORTAL(recip));
    } else
        send_to_char(buf, recip);
}

void
echo_room_caller(ROOM_DATA *room, int depth, void *data)
{
    echo_room(NULL, room, (const char *)data);
}

void
echo_range(CHAR_DATA *ch, const char *argument)
{
    char depthbuf[32];
    argument = one_argument(argument, depthbuf, sizeof(depthbuf));

    if (!isdigit(*depthbuf)) {
        cprintf(ch, "You must provide a range.\n\r");
        return;
    }

    int depth = atoi(depthbuf);
    cprintf(ch, "Echoing to everyone within %d rooms.\n\r", depth);

    generic_astar(ch->in_room, 0, depth, (float)depth, 0, 0, 0, echo_room_caller, (void*)argument, 0, 0, 0);
}


/* cmd_echo checks for the validity of the arguments and calls the appropriate
 * echo function (see above) (bram)*/
void
cmd_echo(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *recip;
    CHAR_DATA *staff = SWITCHED_PLAYER(ch);;
    char echo_type[100];

    if (!*argument) {
        send_to_char("Usage: echo [zone | city | room | all | char | range] <-i | -o> string\n\r", ch);
        return;
    }

    argument = one_argument(argument, echo_type, sizeof(echo_type));

    for (; *argument == ' '; argument++);

    if (!*argument) {
        send_to_char("Usage: echo [zone | city | room | all | char | range] <-i | -o> string\n\r", ch);
        return;
    }

    else if (!strcmp(echo_type, "all")) {
        if (!IS_ADMIN(staff))
            send_to_char("Only Highlords and above may echo all", ch);
        else
            echo_all(ch, argument);
    } else if (!strcmp(echo_type, "room"))
        echo_room(ch, ch->in_room, argument);
    else if (!strcmp(echo_type, "zone"))
        echo_zone(ch, ch->in_room->zone, argument);
    else if (!strcmp(echo_type, "city"))
        echo_city(ch, room_in_city(ch->in_room), argument);
    else if (!strcmp(echo_type, "range"))
        echo_range(ch, argument);
    else {
        if (!(recip = get_char_room_vis(ch, echo_type)))
            send_to_char("That person isn't here.\n\r", ch);
        else {
            send_to_char("Ok.\n\r", ch);
            echo_char(ch, recip, argument);
        }
    }

    return;
}

void
cmd_pulse(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_STRING_LENGTH];
    struct obj_data *obj;
    CHAR_DATA *character;

    argument = one_argument(argument, arg1, sizeof(arg1));

    if (!strcmp(arg1, "room")) {
        strcpy(arg1, "");
        execute_room_program(ch->in_room, 0, CMD_PULSE, arg1);
        send_to_char("Room has been pulsed.\n\r", ch);
    } else {
        if ((obj = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
            if (!has_special_on_cmd(obj->programs, NULL, CMD_PULSE)) {
                send_to_char("That object has no pulse intercept.\n\r", ch);
                return;
            }

            strcpy(arg1, "");
            execute_obj_program(obj, 0, 0, CMD_PULSE, arg1);
            send_to_char("ok, object has been pulsed.\n\r", ch);
        } else {
            if ((character = get_char_vis(ch, arg1))) {
                if (is_merchant(character)) {
                    char cmd[256] = "none";
                    if (argument) {
                        one_argument(argument, cmd, sizeof(cmd));
                    }

                    if (strcmp(cmd, "add") == 0) {
                        add_random_items(character, get_shop_data(character));
                    } else if (strcmp(cmd, "rem") == 0) {
                        remove_random_items(character, get_shop_data(character));
                    }
                }

                if (!has_special_on_cmd(character->specials.programs, NULL, CMD_PULSE)) {
                    send_to_char("That character has no pulse intercept.\n\r", ch);
                    return;
                }

                strcpy(arg1, "");
                execute_npc_program(character, 0, CMD_PULSE, arg1);
                send_to_char("ok, character has been pulsed.\n\r", ch);
            } else
                send_to_char("pulse character/object/\"Room\"\n\r", ch);
        }
    }
}

void
cmd_tran(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    if (IS_IMMORTAL(ch))
        send_to_char("You need to write trans -- no less -- to transfer a character." "\n\r", ch);
    else
        send_to_char("What?\n\r", ch);
    return;
}

void
cmd_trans(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    struct descriptor_data *i;
    CHAR_DATA *victim;
    CHAR_DATA *target_mob;
    struct obj_data *target_obj;
    char buf[100];
    char loc_str[100];
    int loc_nr, silent = 0;
    struct room_data *location;

    SWITCH_BLOCK(ch);

    argument = one_argument(argument, buf, sizeof(buf));

    if (!*buf) {
        send_to_char("Whom do you wish to transfer?\n\r", ch);
        return;
    }

    if (!strcmp(buf, "-s")) {
        silent = 1;
        argument = one_argument(argument, buf, sizeof(buf));
    }

    target_mob = NULL;
    target_obj = NULL;
    argument = one_argument(argument, loc_str, sizeof(loc_str));

    location = ch->in_room;
    if (*loc_str) {
        if (isdigit(*loc_str)) {
            loc_nr = atoi(loc_str);
            if (!(location = get_room_num(loc_nr))) {
                send_to_char("No room exists with that number.\n\r", ch);
                return;
            }
        } else if ((target_mob = get_char_vis(ch, loc_str))) {
            location = target_mob->in_room;
        } else if ((target_obj = get_obj_vis(ch, loc_str))) {
            location = target_obj->in_room;
        } else {
            send_to_char("No such creature or object around.\n\r", ch);
            return;
        }
    }

    if (strcmp("all", buf)) {
        if (!(victim = get_char_vis(ch, buf)))
            send_to_char("No-one by that name around.\n\r", ch);
        else if (victim == ch) {
            send_to_char("How can you transfer yourself?\n\r", ch);
            return;
        } else {
            if ((GET_LEVEL(victim) > GET_LEVEL(ch)) && !IS_NPC(victim)) {
                send_to_char("Nothing seems to happen.\n\r", ch);
                return;
            }
            if (!silent)
                act("$n suddenly disappears with a surprised look on $s face.", TRUE, victim, 0, 0,
                    TO_ROOM);
            char_from_room(victim);
            if (victim->specials.riding)
                char_from_room(victim->specials.riding);
            char_to_room(victim, location);
            if (victim->specials.riding)
                char_to_room(victim->specials.riding, location);
            if (!silent) {
                act("$n suddenly appears with a surprised look on $s face.", TRUE, victim, 0, 0,
                    TO_ROOM);
                act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
                cmd_look(victim, "", 15, 0);
            }
            send_to_char("Ok.\n\r", ch);
        }
    } else {                    /* Trans All */
        for (i = descriptor_list; i; i = i->next)
            if (i->character != ch && !i->connected && CAN_SEE(ch, i->character)) {
                victim = i->character;
                if (!silent)
                    act("$n suddenly disappears with a surprised look on $s face.", FALSE, victim,
                        0, 0, TO_ROOM);
                char_from_room(victim);
                if (victim->specials.riding)
                    char_from_room(victim->specials.riding);
                char_to_room(victim, location);
                if (victim->specials.riding)
                    char_to_room(victim->specials.riding, location);
                if (!silent) {
                    act("$n suddenly appears with a surprised look on $s face.", FALSE, victim, 0,
                        0, TO_ROOM);
                    act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
                    cmd_look(victim, "", 15, 0);
                }
            }
        send_to_char("Ok.\n\r", ch);
    }
}

void
cmd_at(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char command[MAX_INPUT_LENGTH], loc_str[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct room_data *location, *original_loc;
    int loc_nr;
    CHAR_DATA *target_mob, *staff;
    OBJ_DATA *target_obj;
    OBJ_DATA *on_obj;

    target_obj = NULL;
    target_mob = NULL;
    staff = SWITCHED_PLAYER(ch);
    argument = one_argument(argument, loc_str, sizeof(loc_str));
    strcpy(command, argument);

    if (!*loc_str) {
        send_to_char("You must supply a room number or a name.\n\r", ch);
        return;
    }

    if (isdigit(*loc_str)) {
        loc_nr = atoi(loc_str);
        if (!(location = get_room_num(loc_nr))) {
            send_to_char("No room exists with that number.\n\r", ch);
            return;
        }
    } else if ((target_mob = get_char_vis(staff, loc_str))) {
        location = target_mob->in_room;
    } else if ((target_obj = get_obj_vis(staff, loc_str))) {
        location = target_obj->in_room;
    } else {
        send_to_char("No such creature or object around.\n\r", ch);
        return;
    }

    if (location == NULL) {
        if (target_mob != NULL) {
            sprintf(buf, "cmd_at:: Mob: %d isn't in a room.", npc_index[target_mob->nr].vnum);
            send_to_char("They aren't in a room.\n\r", ch);
        } else if (target_obj != NULL) {
            sprintf(buf, "cmd_at:: Obj: %d isn't in a room.", obj_index[target_obj->nr].vnum);
            send_to_char("It isn't in a room.\n\r", ch);
        } else {
            sprintf(buf, "cmd_at:: Room: %s isn't a room?", loc_str);
            send_to_char("That's not a room.\n\r", ch);
        }
        gamelog(buf);
        return;
    }

    /* hold onto what they were on */
    on_obj = ch->on_obj;

    /* a location has been found. */
    original_loc = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, location);
    parse_command(ch, command);

    /* check if the guy's still there */
    for (target_mob = location->people; target_mob; target_mob = target_mob->next_in_room)
        if (ch == target_mob) {
            char_from_room(ch);
            char_to_room(ch, original_loc);
            /* put them back on what they were on, if anything */
            if( on_obj ) 
                add_occupant(on_obj, ch);
        }
}

void
cmd_goto(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_INPUT_LENGTH];
    int loc_nr, i;
    struct room_data *location;
    CHAR_DATA *target_mob, *pers;
    struct obj_data *target_obj;

    argument = one_argument(argument, buf, sizeof(buf));
    if (!*buf) {
        send_to_char("You must supply a room number or a name.\n\r", ch);
        return;
    }

    if (is_number(buf)) {
        loc_nr = atoi(buf);
        if (!(location = get_room_num(loc_nr))) {
            send_to_char("There is no room of that number.\n\r", ch);
            return;
        }
    } else if ((target_mob = get_char_vis(ch, buf)))
        location = target_mob->in_room;
    else if ((target_obj = get_obj_vis(ch, buf))) {
        if (target_obj->in_room)
            location = target_obj->in_room;
        else {
            send_to_char("The object is not available.\n\r", ch);
            return;
        }
    } else {
        send_to_char("No such creature or object around.\n\r", ch);
        return;
    }

    if (!location) {
        gamelog("Location not found in cmd_at");
        return;
    }

    /* a location has been found. */
    if (IS_SET(location->room_flags, RFL_PRIVATE)) {
        for (i = 0, pers = location->people; pers; pers = pers->next_in_room, i++);
        if (i > 1) {
            send_to_char("There's a private conversation going on in that room.\n\r", ch);
            return;
        }
    }

    if (ch->player.poofout)
        act(ch->player.poofout, TRUE, ch, 0, 0, TO_ROOM);
    else if( !IS_NPC(ch) ) // Switched staff members
        act("$n disappears in a swirling cloud of red dust.", TRUE, ch, 0, 0, TO_ROOM);

    char_from_room(ch);
    char_to_room(ch, location);

    if (ch->player.poofin)
        act(ch->player.poofin, TRUE, ch, 0, 0, TO_ROOM);
    else if( !IS_NPC(ch) ) // switched staff members
        act("$n appears in a swirling cloud of red dust.", TRUE, ch, 0, 0, TO_ROOM);

    cmd_look(ch, "", 15, 0);

}

/* funcs related to stats */
void
show_spec_stats_to_char(CHAR_DATA * ch, CHAR_DATA * viewer)
{
    struct obj_data *obj;
    struct follow_type *k, *next_dude;
    char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
    char bufs[MAX_STRING_LENGTH];
    int ch_saves[5];
    int i, j, enc, wt, n, tmp1, tmp2, tmp3, ct, in_city, w;
    struct affected_type *af;
    CLAN_DATA *clan;
    char tmpbuf[MAX_STRING_LENGTH];
    int spice;

    /* Move here for mortals & NPCs to see their background.
     * May 4, 2000 -nessalin */
// Removed database access so will never have a biography, just a background
//   if (IS_NPC(ch) || (!IS_NPC(ch) && get_num_biographies(ch) < 1)) {
//        send_to_char(MSTR(ch, background), viewer);
//        send_to_char("\n\r", viewer);
//    }

    /* Immortal stuff */
    if (IS_SWITCHED_IMMORTAL(viewer)) {
        strcpy(buf1, "% Char flags: ");
        sprint_flag(ch->specials.act, char_flag, buf2);
        strcat(buf1, buf2);
        strcat(buf1, "\n\r");
        send_to_char(buf1, viewer);

        if (ch->affected) {
            send_to_char("% Affected by:\n\r", viewer);
            for (af = ch->affected; af; af = af->next) {
                if ((af->type < 1) || (af->type >= MAX_SKILLS))
                    sprintf(buf2, "  %% 'Unknown %d'\n\r", af->type);
                else
                    sprintf(buf2, "  %% '%s' :\n\r", skill_name[af->type]);
                send_to_char(buf2, viewer);
                sprint_attrib(af->location, obj_apply_types, buf3);
                sprintf(buf2, "     %% modifies %s by %d for %3d hours\n\r", buf3,
                        (int) af->modifier, (int) af->duration);
                send_to_char(buf2, viewer);
                strcpy(buf2, "     % bits : ");
                sprint_flag(af->bitvector, affect_flag, buf1);
                strcat(buf2, buf1);
                strcat(buf2, "\n\r");
                send_to_char(buf2, viewer);
            }
        }

        sprintf(buf, "%% Thirst: %d, Hunger: %d, Drunk: %d\n\r", ch->specials.conditions[THIRST],
                ch->specials.conditions[FULL], ch->specials.conditions[DRUNK]);
        send_to_char(buf, viewer);

        sprintf(buf, "%% State: %d, Eco: %d, Size: %d\n\r\n\r", GET_STATE(ch), ch->specials.eco,
                get_char_size(ch));
        send_to_char(buf, viewer);
        for (spice = MIN_SPICE; spice <= MAX_SPICE; spice++) {
	    if (spice_in_system(ch, spice)) {
	        sprintf(buf, "%% %-17s: %d\n\r", capitalize(skill_name[spice]), spice_in_system(ch, spice));
                send_to_char(buf, viewer);
	    }
	}
	sprintf(buf, "%% Spice overdose at: %d for any single spice.\n\r\n\r", (2 * GET_END(ch) + 1));
        send_to_char(buf, viewer);

    }

    enc = get_encumberance(ch);
    tmp1 = tmp2 = tmp3 = 0;

    /* Offense, defense, and parry bonuses */
    tmp1 = (ch->abilities.off + agl_app[GET_AGL(ch)].reaction);
    tmp2 = (ch->abilities.def + agl_app[GET_AGL(ch)].reaction);

    if (has_skill(ch, SKILL_PARRY))
        /*  if (ch->skills[SKILL_PARRY]) */
        tmp3 = ch->skills[SKILL_PARRY]->learned;
    else
        tmp3 = 0;

//   if (NULL != (obj = ch->equipment[EP]))
    obj = get_weapon(ch, &w);
    if (obj) {
        if (obj->obj_flags.type == ITEM_WEAPON) {
            tmp1 += proficiency_bonus(ch, obj->obj_flags.value[3] + 300);
            tmp3 += proficiency_bonus(ch, obj->obj_flags.value[3] + 300);
        }
    }

    if ((has_skill(ch, SKILL_SHIELD)) && (NULL != (obj = ch->equipment[EP])))
        if (obj->obj_flags.type == ITEM_ARMOR)
            tmp3 += get_skill_percentage(ch, SKILL_SHIELD);
    if ((has_skill(ch, SKILL_SHIELD)) && (NULL != (obj = ch->equipment[ES])))
        if (obj->obj_flags.type == ITEM_ARMOR)
            tmp3 += get_skill_percentage(ch, SKILL_SHIELD) / 3;


    tmp1 -= MAX(enc - 4, 0);
    tmp2 -= MAX(enc - 4, 0);
    tmp3 -= MAX((enc - 4) * 2, 0);

    wt = calc_carried_weight(ch);

    if (IS_SWITCHED_IMMORTAL(viewer)) {
        sprintf(buf, "%s %s carrying %d stone%s weight, which is %s.\n\r",
                (viewer ==
                 ch) ? "You" : ((IS_IMMORTAL(viewer)) ? CAP(MSTR(ch, name)) : MSTR(ch,
                                                                                   short_descr)),
                (viewer == ch) ? "are" : "is", wt, (wt == 1) ? "" : "s", encumb_class(enc));
        send_to_char(buf, viewer);
    } else {
        sprintf(buf, "%s%s encumbrance is %s.\n\r", (viewer == ch) ? "Your" : MSTR(ch, short_descr),
                (viewer == ch) ? "" : MSTR(ch, short_descr)
                [strlen(MSTR(ch, short_descr))
                 - 1] == 's' ? "'" : "'s", encumb_class(enc));
        send_to_char(buf, viewer);
    }

    if (IS_SWITCHED_IMMORTAL(viewer)) {
        send_to_char("Basic combat modifiers:\n\r", viewer);
        sprintf(buf,
                "    Offensive bonus: %4d%%\n\r    Parry bonus: %8d%%\n\r"
                "    Defensive bonus: %4d%%\n\r", tmp1, tmp3, tmp2);
        send_to_char(buf, viewer);
    }

    if (ch->clan) {
        sprintf(buf, "%s %s:\n\r", ch == viewer ? "You" : capitalize(HSSH(ch)),
                ch == viewer ? "are" : "is");
        send_to_char(buf, viewer);

        for (clan = ch->clan; clan; clan = clan->next) {
            int c = clan->clan;
            char *rname = get_clan_rank_name(c, clan->rank);

            if (c > 0 && c < MAX_CLAN) {
                char *flags = show_flags(clan_job_flags, clan->flags);
                sprintf(buf, "%s%s%s, jobs: %s\n\r", rname ? capitalize(rname) : "",
                        rname ? " of the " : "", clan_table[c].name, flags);
                free(flags);
            } else
                sprintf(buf, "%d (Unknown)\n\r", c);

            send_to_char(buf, viewer);
        }
    }

    /* Eco balance */
    if ((ch->specials.eco < 10) && (ch->specials.eco > -10))
        sprintf(bufs, "neutral");
    else if ((ch->specials.eco < 50) && (ch->specials.eco >= 10))
        sprintf(bufs, "good");
    else if ((ch->specials.eco < 90) && (ch->specials.eco >= 50))
        sprintf(bufs, "potent");
    else if ((ch->specials.eco < 100) && (ch->specials.eco >= 90))
        sprintf(bufs, "nearly a oneness");
    else if (ch->specials.eco == 100)
        sprintf(bufs, "as one");
    else if ((ch->specials.eco > -50) && (ch->specials.eco <= -10))
        sprintf(bufs, "harmful");
    else if ((ch->specials.eco > -90) && (ch->specials.eco <= -50))
        sprintf(bufs, "destructive");
    else if ((ch->specials.eco > -100) && (ch->specials.eco <= -90))
        sprintf(bufs, "almost malign");
    else if (ch->specials.eco == -100)
        sprintf(bufs, "malign");
    else
        sprintf(bufs, "WIERDOLA");

    sprintf(buf, "Relationship to the land is %s.\n\r", bufs);
    send_to_char(buf, viewer);

    /* Saving throws */
    for (n = 0; n < 5; n++)
        ch_saves[n] = ch->specials.apply_saving_throw[n];

    if (IS_SWITCHED_IMMORTAL(viewer)) {
        send_to_char("Resistances:\n\r", viewer);
        sprintf(buf1, "    Poison      (%d)", ch_saves[0]);
        sprintf(buf2, "Transmutation (%d)", (ch_saves[2]));
        sprintf(buf, "%-34s %s\n\r", buf1, buf2);
        send_to_char(buf, viewer);
        sprintf(buf1, "    Staves      (%d)", (ch_saves[1]));
        sprintf(buf2, "Breath        (%d)", (ch_saves[3]));
        sprintf(buf, "%-34s %s\n\r", buf1, buf2);
        send_to_char(buf, viewer);
        tmp1 = ch_saves[4] + wis_app[GET_WIS(ch)].wis_saves;
        sprintf(buf1, "    Enchantment (%d)", (tmp1));
        sprintf(buf2, "Elementalism  (%d)", (ch_saves[4]));
        sprintf(buf, "%-34s %s\n\r", buf1, buf2);
        send_to_char(buf, viewer);
    }
/*
   else
   {
   send_to_char("Resistances:\n\r", viewer);
   sprintf(buf1, "    Poison      (%s)", save_category(ch_saves[0]));
   sprintf(buf2, "Transmutation (%s)", save_category(ch_saves[2]));
   sprintf(buf, "%-34s %s\n\r", buf1, buf2);
   send_to_char(buf, viewer);
   sprintf(buf1, "    Staves      (%s)", save_category(ch_saves[1]));
   sprintf(buf2, "Breath        (%s)", save_category(ch_saves[3]));
   sprintf(buf, "%-34s %s\n\r", buf1, buf2);
   send_to_char(buf, viewer);
   tmp1 = ch_saves[4] + wis_app[GET_WIS(ch)].wis_saves;
   sprintf(buf1, "    Enchantment (%s)", save_category(tmp1));
   sprintf(buf2, "Elementalism  (%s)", save_category(ch_saves[4]));
   sprintf(buf, "%-34s %s\n\r", buf1, buf2);
   send_to_char(buf, viewer);
   }
 */

    i = GET_SPOKEN_LANGUAGE(ch);
    strcpy(tmpbuf, "unknown");
    for (j = 0; accent_table[j].skillnum != LANG_NO_ACCENT; j++)
        if (accent_table[j].skillnum == GET_ACCENT(ch))
            strcpy(tmpbuf, accent_table[j].name);

    sprintf(buf, "%s %s currently speaking %s with %s%s accent.\n\r",
            (ch ==
             viewer) ? "You" : ((IS_IMMORTAL(viewer)) ? MSTR(ch, name) : MSTR(ch, short_descr)),
            (ch == viewer) ? "are" : "is", skill_name[i], indefinite_article(tmpbuf), tmpbuf);
    send_to_char(buf, viewer);

    /* flags */
    buf1[0] = '\0';
    in_city = room_in_city(ch->in_room);

    for (ct = 0; ct < MAX_CITIES; ct++) {
        if (IS_SET(ch->specials.act, city[ct].c_flag)
            && (in_city == ct || IS_IMMORTAL(viewer))) {
            if (buf1[0] != '\0')
                strcat(buf1, ", ");

            strcat(buf1, city[ct].name);
            break;
        }
    }

    if (*buf1) {
        sprintf(buf, "%s %s wanted in: %s\n\r",
                (ch ==
                 viewer) ? "You" : ((IS_IMMORTAL(viewer)) ? MSTR(ch, name) : MSTR(ch, short_descr)),
                (ch == viewer) ? "are" : "is", buf1);
        send_to_char(buf, viewer);
    }

    strcpy(buf1, "");
/* poison and spice stuff */
    if ((is_poisoned_char(ch))
        && (is_poisoned_char(ch) != POISON_PLAGUE - 219)) {
        if( is_poisoned_char(ch) == POISON_GENERIC
         && !suffering_from_poison(ch, POISON_GENERIC) ) {
           strcat(buf1, " Poison Recovery,");
        } else {
           strcat(buf1, " Poison,");
        }
    }
    if ((is_poisoned_char(ch)) && (affected_by_spell(ch, POISON_PLAGUE)))
        strcat(buf1, " Plague,");
    if (is_spiced_char(ch))
        strcat(buf1, " Spice,");
/* spell affects  -- I alphabetized for easier tracking.  -San. */
    if (affected_by_spell(ch, SPELL_ARMOR))
        strcat(buf1, " Armor,");
    if (affected_by_spell(ch, SPELL_BLIND))
        strcat(buf1, " Blind,");
    if (affected_by_spell(ch, SPELL_BREATHE_WATER))
        strcat(buf1, " Breathe Water,");
    if (affected_by_spell(ch, SPELL_CALM))
        strcat(buf1, " Calm,");
    if (affected_by_spell(ch, SPELL_DETECT_ETHEREAL))
        strcat(buf1, " Detect Ethereal,");
    if (affected_by_spell(ch, SPELL_DETECT_INVISIBLE))
        strcat(buf1, " Detect Invisible,");
    if (affected_by_spell(ch, SPELL_DETECT_MAGICK))
        strcat(buf1, " Detect Magick,");
    if (affected_by_spell(ch, SPELL_DETECT_POISON))
        strcat(buf1, " Detect Poison,");
    if (affected_by_spell(ch, SPELL_ELEMENTAL_FOG))
        strcat(buf1, " Elemental Fog,");
    if (affected_by_spell(ch, SPELL_ENERGY_SHIELD))
        strcat(buf1, " Energy Shield,");
    if (IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))
        strcat(buf1, " Ethereal,");
    if (affected_by_spell(ch, SPELL_FEAR))
        strcat(buf1, " Fear,");
    if (affected_by_spell(ch, SPELL_FEATHER_FALL))
        strcat(buf1, " Feather Fall,");
    if (affected_by_spell(ch, SPELL_FIREBREATHER))
        strcat(buf1, " Fire breather,");
    if (affected_by_spell(ch, SPELL_FLY))
        strcat(buf1, " Fly,");
    if (affected_by_spell(ch, SPELL_FURY))
        strcat(buf1, " Fury,");
    if (affected_by_spell(ch, SPELL_GODSPEED))
        strcat(buf1, " Godspeed,");
    if (affected_by_spell(ch, SPELL_IDENTIFY))
        strcat(buf1, " Identify,");
    if (affected_by_spell(ch, SPELL_IMMOLATE))
        strcat(buf1, " Immolate,");
    if (affected_by_spell(ch, SPELL_INFRAVISION))
        strcat(buf1, " Infravision,");
    if (affected_by_spell(ch, SPELL_INSOMNIA))
        strcat(buf1, " Insomnia,");
    if (IS_AFFECTED(ch, CHAR_AFF_INVISIBLE))
        strcat(buf1, " Invisibility,");
    if (IS_AFFECTED(ch, CHAR_AFF_INVULNERABILITY))
        strcat(buf1, " Invulnerability,");
    if (affected_by_spell(ch, SPELL_LEVITATE))
        strcat(buf1, " Levitate,");
    if (affected_by_spell(ch, SPELL_PARALYZE))
        strcat(buf1, " Paralyze,");
    if (affected_by_spell(ch, SPELL_QUICKENING))
        strcat(buf1, " Quickening,");
    if (affected_by_spell(ch, SPELL_REGENERATE))
        strcat(buf1, " Regenerate,");
    if (IS_AFFECTED(ch, CHAR_AFF_SANCTUARY))
        strcat(buf1, " Sanctuary,");
    if (affected_by_spell(ch, SPELL_SHIELD_OF_NILAZ))
        strcat(buf1, " Shield of Nilaz,");
    if (affected_by_spell(ch, SPELL_SILENCE))
        strcat(buf1, " Silence,");
    if (affected_by_spell(ch, SPELL_SLOW))
        strcat(buf1, " Slow,");
    if (affected_by_spell(ch, SPELL_STONE_SKIN))
        strcat(buf1, " Stoneskin,");
    if (affected_by_spell(ch, SPELL_STRENGTH))
        strcat(buf1, " Strength,");
    if (affected_by_spell(ch, SPELL_TURN_ELEMENT))
        strcat(buf1, " Turn Element,");
    if (affected_by_spell(ch, SPELL_WEAKEN))
        strcat(buf1, " Weaken,");
    if (affected_by_spell(ch, SPELL_WIND_ARMOR))
        strcat(buf1, " Wind Armor,");
    if (affected_by_spell(ch, SPELL_FIRE_ARMOR))
        strcat(buf1, " Fire Armor,");
    if (affected_by_spell(ch, SPELL_SHADOW_ARMOR))
        strcat(buf1, " Shadow Armor,");
    if (affected_by_spell(ch, SPELL_TONGUES))
        strcat(buf1, " Tongues,");
/* combat affects */
    if (IS_AFFECTED(ch, CHAR_AFF_SUBDUED))
        strcat(buf1, " Subdue,");
    if (affected_by_spell(ch, AFF_MUL_RAGE))
        strcat(buf1, " Rage,");
/* psionic affects */
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_COMPREHEND_LANGS))
        strcat(buf1, " Allspeak,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_BABBLE))
        strcat(buf1, " Babble,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_BARRIER))
        strcat(buf1, " Psionic Barrier,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_CATHEXIS))
        strcat(buf1, " Cathexis,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_DOME))
        strcat(buf1, " Psionic Dome,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_DISORIENT))
        strcat(buf1, " Disorientation,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_HEAR))
        strcat(buf1, " Psionic Hear,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_MINDWIPE))
        strcat(buf1, " Mindwipe,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, SPELL_PSI_SUPPRESSION))
        strcat(buf1, " Psionic Suppression,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_MAGICKSENSE))
        strcat(buf1, " Magicksense,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_CONCEAL))
        strcat(buf1, " Psionic Concealment,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_SENSE_PRESENCE))
        strcat(buf1, " Sense Presence,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_CLAIRAUDIENCE))
        strcat(buf1, " Clairaudience,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_CLAIRVOYANCE))
        strcat(buf1, " Clairvoyance,");
    if (IS_AFFECTED(ch, CHAR_AFF_PSI) && affected_by_spell(ch, PSI_VANISH))
        strcat(buf1, " Psionic Cloak,");
    /*  diseases  */
    if (affected_by_spell(ch, DISEASE_BOILS))
        strcat(buf1, " Boils,");
    if (affected_by_spell(ch, DISEASE_CHEST_DECAY))
        strcat(buf1, " Unknown Disease,");
    if (affected_by_spell(ch, DISEASE_CILOPS_KISS))
        strcat(buf1, " Cilop's Kiss,");
    if (affected_by_spell(ch, DISEASE_GANGRENE))
        strcat(buf1, " Gangrene,");
    if (affected_by_spell(ch, DISEASE_HEAT_SPLOTCH))
        strcat(buf1, " Heat Splotch,");
    if (affected_by_spell(ch, DISEASE_IVORY_SALT_SICKNESS))
        strcat(buf1, " Unknown Disease,");
    if (affected_by_spell(ch, DISEASE_KANK_FLEAS))
        strcat(buf1, " Kank Fleas,");
    if (affected_by_spell(ch, DISEASE_KRATHS_TOUCH))
        strcat(buf1, " Krath's Touch,");
    if (affected_by_spell(ch, DISEASE_MAAR_POX))
        strcat(buf1, " Maar Pox,");
    if (affected_by_spell(ch, DISEASE_PEPPERBELLY))
        strcat(buf1, " Pepperbelly,");
    if (affected_by_spell(ch, DISEASE_RAZA_RAZA))
        strcat(buf1, " Unknown Disease,");
    if (affected_by_spell(ch, DISEASE_SAND_FEVER))
        strcat(buf1, " Sand Fever,");
    if (affected_by_spell(ch, DISEASE_SAND_FLEAS))
        strcat(buf1, " Sand Fleas,");
    if (affected_by_spell(ch, DISEASE_SAND_ROT))
        strcat(buf1, " Sand Rot,");
    if (affected_by_spell(ch, DISEASE_SCRUB_FEVER))
        strcat(buf1, " Scrub Fever,");
    if (affected_by_spell(ch, DISEASE_SLOUGH_SKIN))
        strcat(buf1, " Unknown Disease,");
    if (affected_by_spell(ch, DISEASE_YELLOW_PLAGUE))
        strcat(buf1, " Unknown Disease,");

    if (*buf1) {
        buf1[strlen(buf1) - 1] = 0;     // Take off last comma
        if ((strlen(buf1) + 85) <= MAX_STRING_LENGTH) {
            sprintf(buf, "%s %s affected by:\n\r  %s\n\r",
                    (ch ==
                     viewer) ? "You" : ((IS_IMMORTAL(viewer)) ? MSTR(ch, name) : MSTR(ch,
                                                                                      short_descr)),
                    (ch == viewer) ? "are" : "is", buf1);
            send_to_char(buf, viewer);
        }
    }

    if( get_char_mood(ch, buf1, sizeof(buf1))){
        cprintf(viewer, "%s%s mood is %s.\n\r",
         (ch == viewer) ? "Your" 
          : ((IS_IMMORTAL(viewer)) ? MSTR(ch, name) : MSTR(ch, short_descr)),
         (ch == viewer) ? "" : "'s", buf1);
    } else {
        cprintf(viewer, "%s%s mood is neutral.\n\r",
         (ch == viewer) ? "Your" 
          : ((IS_IMMORTAL(viewer)) ? MSTR(ch, name) : MSTR(ch, short_descr)),
         (ch == viewer) ? "" : "'s");
    }

    sprintf(buf, "%s %s ",
            (ch ==
             viewer) ? "You" : ((IS_IMMORTAL(viewer)) ? MSTR(ch, name) : MSTR(ch, short_descr)),
            (ch == viewer) ? "are" : "is");
    switch (GET_POS(ch)) {
    case POSITION_DEAD:
        strcat(buf, "dead.\n\r");
        break;
    case POSITION_MORTALLYW:
        strcat(buf, "mortally wounded, and will probably die soon if not" " aided.\n\r");
        break;
    case POSITION_STUNNED:
        strcat(buf, "stunned, unable to move.\n\r");
        break;
    case POSITION_SLEEPING:
        strcat(buf, "asleep.\n\r");
        break;
    case POSITION_RESTING:
        strcat(buf, "resting.\n\r");
        break;
    case POSITION_SITTING:
        strcat(buf, "sitting down.\n\r");
        break;
    case POSITION_FIGHTING:
        if (ch->specials.fighting) {
            sprintf(buf1, "fighting %s.\n\r", PERS(viewer, ch->specials.fighting));
            strcat(buf, buf1);
        } else
            strcat(buf, "standing.\n\r");
        break;
    case POSITION_STANDING:
    default:
        if (ch->specials.riding) {
            sprintf(buf1, "riding %s.\n\r", PERS(viewer, ch->specials.riding));
            strcat(buf, buf1);
        } else
            strcat(buf, "standing.\n\r");
        break;
    }
    send_to_char(buf, viewer);

    if (ch->specials.contact) {
        sprintf(buf, "%s mind is in contact with %s.\n\r",
                (ch ==
                 viewer) ? "Your" : ((GET_SEX(ch) ==
                                      SEX_MALE) ? "His" : ((GET_SEX(ch) ==
                                                            SEX_FEMALE) ? "Her" : "Its")),
                ((IS_IMMORTAL(viewer)) ? MSTR(ch->specials.contact, name) :
                 MSTR(ch->specials.contact, short_descr)));
        send_to_char(buf, viewer);
    }

    if (ch->followers) {
        for (k = ch->followers; k; k = next_dude) {
            next_dude = k->next;
            if (IS_SET(k->follower->specials.act, CFL_MOUNT)) {
                sprintf(buf, "%s %s leading %s.\n\r", (ch == viewer) ? "You" : MSTR(ch, name),
                        (ch == viewer) ? "are" : "is", PERS(viewer, k->follower));
                send_to_char(buf, viewer);
            }                   /* End if IS_SET */
        }                       /* End for loop */
    }
    /* End if ch->followers */
    if (ch->obj_master) {
        sprintf(buf, "%s %s following %s.\n\r", (ch == viewer) ? "You" : MSTR(ch, name),
                (ch == viewer) ? "are" : "is", OSTR(ch->obj_master, short_descr));
        if (IS_IMMORTAL(viewer))
            send_to_char(buf, viewer);
    }

    if (ch->specials.nosave) {
        cprintf(viewer, "%s %s refusing saves on: %s.\n\r", (ch == viewer) ? "You" : MSTR(ch, name),
                (ch == viewer) ? "are" : "is", show_flags(nosave_flag, ch->specials.nosave));
    } else {
        cprintf(viewer, "%s %s accepting all saves (nosave off).\n\r",
                (ch == viewer) ? "You" : MSTR(ch, name), (ch == viewer) ? "are" : "is");
    }

    if (ch->specials.mercy) {
        cprintf(viewer, "%s %s merciful on: %s.\n\r", 
         (ch == viewer) ? "You" : MSTR(ch, name),
         (ch == viewer) ? "are" : "is", show_flags(mercy_flag, ch->specials.mercy));
    } else {
        cprintf(viewer, "%s %s not being merciful.\n\r",
         (ch == viewer) ? "You" : MSTR(ch, name), (ch == viewer) ? "are" : "is");
    }

    watch_status(ch, viewer);
}                               /* End cmd_stat */


void
cmd_rstat(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    struct room_data *rm = 0;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char wtype[MAX_STRING_LENGTH];
    int i;
    struct obj_data *j;
    CHAR_DATA *k;
    struct extra_descr_data *desc;
    struct watching_room_list *tmp_watch;
    char *watch_msg = NULL;
    int bhelp = 0;
    int bchars = 0;
    int bhchars = 0;
    int bdescs = 0;
    int bhdescs = 0;
    int bexits = 0;
    int bhexits = 0;
    int btracks = 0;
    int bhtracks = 0;
    int bobjects = 0;
    int bhobjects = 0;
    int bspecials = 0;
    int bhspecials = 0;
    int bwatching = 0;
    int bhwatching = 0;
    int bforage = 0;
    int bshowall = 0;

    arg_struct rstat_args[] = {
        {'r', "room", ARG_PARSER_REQUIRED_ROOM, {&rm}, {0},
         "The room to stat."},
        {'s', "specials", ARG_PARSER_BOOL, {&bspecials}, {0},
         "Show specials in the output."},
        {'S', "hide_specials", ARG_PARSER_BOOL, {&bhspecials}, {0},
         "Hide specials from the output."},
        {'f', "forage", ARG_PARSER_BOOL, {&bforage}, {0},
         "Show forage in the output."},
        {'w', "watching", ARG_PARSER_BOOL, {&bwatching}, {0},
         "Show watching in the output."},
        {'W', "hide_watching", ARG_PARSER_BOOL, {&bhwatching}, {0},
         "Hide watching from the output."},
        {'d', "descriptions", ARG_PARSER_BOOL, {&bdescs}, {0},
         "Show descriptions in the output."},
        {'D', "hide_descriptions", ARG_PARSER_BOOL, {&bhdescs}, {0},
         "Hide descriptions from the output."},
        {'c', "chars", ARG_PARSER_BOOL, {&bchars}, {0},
         "Show characters in the output."},
        {'C', "hide_chars", ARG_PARSER_BOOL, {&bhchars}, {0},
         "Hide characters from the output."},
        {'o', "objs", ARG_PARSER_BOOL, {&bobjects}, {0},
         "Show objects in the output."},
        {'O', "hide_objs", ARG_PARSER_BOOL, {&bhobjects}, {0},
         "Hide objects from the output."},
        {'e', "exits", ARG_PARSER_BOOL, {&bexits}, {0},
         "Show exits in the output."},
        {'E', "hide_exits", ARG_PARSER_BOOL, {&bhexits}, {0},
         "Hide exits from the output."},
        {'t', "tracks", ARG_PARSER_BOOL, {&btracks}, {0},
         "Show tracks in the output."},
        {'T', "hide_tracks", ARG_PARSER_BOOL, {&bhtracks}, {0},
         "Hide tracks from the output."},
        {'h', "help", ARG_PARSER_BOOL, {&bhelp}, {0}, "Show this help text."},
    };

    cmd_getopt(rstat_args, sizeof(rstat_args) / sizeof(rstat_args[0]), argument);

    if (bhelp) {
        cprintf(ch, "Options for rstat:\n\r");
        page_help(ch, rstat_args, sizeof(rstat_args) / sizeof(rstat_args[0]));
        return;
    }

    if (bforage) {
        forage_stat(ch);
        return;
    }

    // Show 'All' if they didn't say to show specifically any one area.
    bshowall = !bchars && !bobjects && !bexits && !btracks && !bspecials && !bwatching && !bdescs;

    if (!rm || (rm && rm->number == 0)) {
        rm = ch->in_room;
    }

    cprintf(ch, "Room name: %s, Zone: %d. Number: %d\n\r", rm->name, rm->zone, rm->number);

    if (bshowall) {
        cprintf(ch, "In city: %s (%d)\n\r", city[room_in_city(rm)].name, room_in_city(rm));
    }

    if ((bshowall && !bhspecials) || bspecials) {
        display_specials(buf, ARRAY_LENGTH(buf), rm->specials, 1);
        send_to_char(buf, ch);
    }

    if (bshowall) {
        send_to_char("Room flags: ", ch);
        sprint_flag((long) rm->room_flags, room_flag, buf);
        strcat(buf, "\n\r");
        send_to_char(buf, ch);
    }

    if (bshowall) {
        sprint_attrib(rm->sector_type, room_sector, buf2);
        sprintf(buf, "Sector type : %s\n\r", buf2);
        send_to_char(buf, ch);
    }

    if (bshowall) {
        cprintf(ch, "Number of light sources: %d\n\r", rm->light);
    }

    if (bshowall) {
        if ((IS_SET(rm->room_flags, RFL_CARGO_BAY))
            || ((rm->number / 1000) == 3)
            || ((rm->number / 1000) == 13)
            || ((rm->number / 1000) == 33)
            || ((rm->number / 1000) == 73)) {
            int base = get_room_base_weight(rm);
            int used = get_used_room_weight(rm);
            int current = get_room_weight(rm);
            int max = get_room_max_weight(rm);
            cprintf(ch, "Base/Max Room Capacity: %d <= %d\n\rUsed/Current Room Weight: %d <= %d\n\r",
             base, max, used, current);
        }
        else
            cprintf(ch, "Base/Max Room Capacity: unlimited\n\rUsed/Current Room Weight: %d <= %d\n\r",
             get_used_room_weight(rm), get_room_weight(rm));
    }

    if ((bshowall && !bhwatching) || bwatching) {
        if (rm->watching) {
            cprintf(ch, "This room is watching\n\r");
            cprintf(ch, "---------------------\n\r");
            for (tmp_watch = rm->watching; tmp_watch; tmp_watch = tmp_watch->next_watching) {
                if (IS_SET(tmp_watch->watch_type, WATCH_VIEW)) {
                    sprintf(wtype, "WATCH");
		    watch_msg = tmp_watch->view_msg_string;
                    if (tmp_watch->room_watched) {
                        cprintf(ch, "%s(%d):%s: with message \"%s\"\n\r", tmp_watch->room_watched->name,
                            tmp_watch->room_watched->number, wtype, watch_msg);
                    } else {
                        cprintf(ch, "unloaded room %d:%s: with message \"%s\"\n\r",
                            tmp_watch->watch_room_num, wtype, watch_msg);
                    }
		}
		if (IS_SET(tmp_watch->watch_type, WATCH_LISTEN)) {
                    sprintf(wtype, "LISTEN");
		    watch_msg = tmp_watch->listen_msg_string;
                    if (tmp_watch->room_watched) {
                        cprintf(ch, "%s(%d):%s: with message \"%s\"\n\r", tmp_watch->room_watched->name,
                            tmp_watch->room_watched->number, wtype, watch_msg);
                    } else {
                        cprintf(ch, "unloaded room %d:%s: with message \"%s\"\n\r",
                            tmp_watch->watch_room_num, wtype, watch_msg);
                    }
		}
            }
        }

        if (rm->watched_by) {
            cprintf(ch, "This room is watched by\n\r");
            cprintf(ch, "-----------------------\n\r");
            for (tmp_watch = rm->watched_by; tmp_watch; tmp_watch = tmp_watch->next_watched_by) {
		if (IS_SET(tmp_watch->watch_type, WATCH_VIEW)) {
                    sprintf(wtype, "WATCH");
		    watch_msg = tmp_watch->view_msg_string;
                    cprintf(ch, "%d:%s: with message \"%s\"\n\r", tmp_watch->room_watching->number, wtype, watch_msg);
		}
		if (IS_SET(tmp_watch->watch_type, WATCH_LISTEN)) {
                    sprintf(wtype, "LISTEN");
		    watch_msg = tmp_watch->listen_msg_string;
                    cprintf(ch, "%d:%s: with message \"%s\"\n\r", tmp_watch->room_watching->number, wtype, watch_msg);
		}
	    }
        }
    }

    if ((bshowall && !bhdescs) || bdescs) {
        cprintf(ch, "Background Description (index %d):\n\r",
                (rm->zone * MAX_BDESC) + rm->bdesc_index);
        cprintf(ch, "%s", bdesc_list[rm->zone][rm->bdesc_index].desc);

        cprintf(ch, "\n\r");

        cprintf(ch, "Foreground Description:\n\r");
        cprintf(ch, "%s", rm->description);

        cprintf(ch, "Extra description keywords(s): ");
        if (rm->ex_description) {
            cprintf(ch, "\n\r");
            for (desc = rm->ex_description; desc; desc = desc->next) {
                if (desc->keyword) {
                    cprintf(ch, "%s%s\n\r", (desc->def_descr ? "  -  " : "     "), desc->keyword);
                }
            }
            cprintf(ch, "\n\r");
        } else {
            cprintf(ch, "     None\n\r");
        }
    }

    if ((bshowall && !bhchars) || bchars) {
        cprintf(ch, "------- Characters Present -------\n\r");
        for (k = rm->people; k; k = k->next_in_room) {
            if (CAN_SEE(ch, k)) {
                cprintf(ch, "%s", GET_NAME(k));
                if (!IS_NPC(k)) {
                    cprintf(ch, " (PC)\n\r");
                } else {
                    cprintf(ch, " (%s %d)\n\r", IS_MOB(k) ? "MOB" : "NPC", npc_index[k->nr].vnum);
                }
            }
        }
        cprintf(ch, "\n\r");
    }

    if ((bshowall && !bhobjects) || bobjects) {
        cprintf(ch, "--------- Objects Present ---------\n\r");
        for (j = rm->contents; j; j = j->next_content) {
            cprintf(ch, "%s\n\r", OSTR(j, name));
        }
        cprintf(ch, "\n\r");
    }

    if ((bshowall && !bhexits) || bexits) {
        cprintf(ch, "------- Exits Defined -------\n\r");
        for (i = 0; i <= 5; i++) {
            if (rm->direction[i]) {
                cprintf(ch, "%s to %d. Keyword : %s\n\r", capitalize(dirs[i]),
                        rm->direction[i]->to_room->number, rm->direction[i]->keyword);

                if (rm->direction[i]->general_description) {
                    cprintf(ch, "   Description:\n\r      %s",
                            rm->direction[i]->general_description);
                } else
                    cprintf(ch, "   No description set.\n\r");

                sprint_flag(rm->direction[i]->exit_info, exit_flag, buf2);
                cprintf(ch, "   Exit flags: %s, Key no: %d\n\r", buf2, rm->direction[i]->key);
            }
        }
    }

    if ((bshowall && !bhtracks) || btracks) {
        cprintf(ch, "------- TRACKS -------\n\r");
        for (i = 0; i < MAX_TRACKS; i++) {
            sprint_attrib(rm->tracks[i].type, track_types, buf2);

            cprintf(ch, "%s : skill %d: time %s", buf2, rm->tracks[i].skill,
                    rm->tracks[i].time == 0 ? "n/a\n\r" : (char *) ctime(&rm->tracks[i].time));

            switch (rm->tracks[i].type) {
            case TRACK_NONE:
                break;
            case TRACK_BATTLE:
                cprintf(ch, " BATTLE:number=%d\n\r", rm->tracks[i].track.battle.number >= 0);
                break;
            case TRACK_TENT:
                cprintf(ch, " TENT \n\r");
                break;
            case TRACK_WAGON:
                cprintf(ch, " WAGON: Weight %d %s %s.\n\r", (int) rm->tracks[i].track.wagon.weight,
                        rm->tracks[i].track.wagon.into ? "from" : "to",
                        ((rm->tracks[i].track.wagon.dir >= 0)
                         && (rm->tracks[i].track.wagon.dir < 6))
                        ? dir_name[(int) rm->tracks[i].track.wagon.dir] : "no_dir");
                break;
            case TRACK_WALK_CLEAN:
            case TRACK_RUN_CLEAN:
            case TRACK_SNEAK_CLEAN:
            case TRACK_WALK_BLOODY:
            case TRACK_RUN_BLOODY:
            case TRACK_SNEAK_BLOODY:
                cprintf(ch, "  Race %s, Weight(%d), %s %s.\n\r",
                        (rm->tracks[i].track.ch.race >= 0
                         && rm->tracks[i].track.ch.race < MAX_RACES)
                        ? race[rm->tracks[i].track.ch.race].name : "Invalid",
                        (int) rm->tracks[i].track.ch.weight,
                        rm->tracks[i].track.ch.into ? "from" : "to",
                        (rm->tracks[i].track.ch.dir >= 0 && rm->tracks[i].track.ch.dir < 6)
                        ? dir_name[(int) rm->tracks[i].track.ch.dir] : "no_dir");
                break;
            }
        }
    }

    if (bshowall) {
        heap_print(buf, rm, event_room_cmp);
        send_to_char(buf, ch);
    }
}

void
cmd_stop(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    cprintf(ch, "%d commands purged.\n\r", flush_queued_commands(ch, FALSE));
}

void
cmd_stat(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_STRING_LENGTH];
    char buf3[256], buf[MAX_STRING_LENGTH];
    CHAR_DATA *k = 0, *staff;
    int g;

    staff = SWITCHED_PLAYER(ch);
    argument = two_arguments(argument, arg1, sizeof(arg1), buf3, sizeof(buf3));

    if (!IS_IMMORTAL(staff) || (!*arg1)) {
        show_spec_stats_to_char(ch, ch);
    } else if (!stricmp(arg1, "room")) {
        send_to_char("Use rstat\n", ch);
        return;
    } else if (!stricmp(arg1, "guild")) {
        for (g = 1; g < MAX_GUILDS; g++)
            if (!strnicmp(buf3, guild[g].name, strlen(buf3)))
                break;
        if (g == MAX_GUILDS) {
            send_to_char("That guild does not exist.\n\r", ch);
            return;
        }

        sprintf(buf, "Min stats - Str: [%d], Agl: [%d], Wis: [%d], " "End: [%d]\n\r",
                guild[g].strmod, guild[g].aglmod, guild[g].wismod, guild[g].endmod);
        send_to_char(buf, ch);
        return;
    } else if (!stricmp(arg1, "race")) {
        for (g = 1; g < MAX_RACES; g++)
            if (!strnicmp(buf3, race[g].name, strlen(buf3)))
                break;
        if (g == MAX_RACES) {
            send_to_char("That race does not exist.\n\r", ch);
            return;
        }
        sprintf(buf, "Stat mods - Str: [%d], Agl: [%d], Wis: [%d], " "End: [%d]\n\r",
                race[g].strplus, race[g].aglplus, race[g].wisplus, race[g].endplus);
        send_to_char(buf, ch);
        sprintf(buf, "Offense base: [%d], Defense base: [%d], Natural " "Armor: [%d]\n\r",
                race[g].off_base, race[g].def_base, race[g].natural_armor);
        send_to_char(buf, ch);
        return;
    } else if (!stricmp(arg1, "obj")) {
        send_to_char("Use ostat\n\r", ch);
        return;
    } else if (!stricmp(arg1, "char")) {
        if (!(k = get_char_vis(ch, buf3))) {
            send_to_char("No such character in the world.\n\r", ch);
            return;
        }
/*  This command was causing malloc errors. Removed for the time being */
        show_spec_stats_to_char(k, ch);

    } else if (!stricmp(arg1, "klog") || !stricmp(arg1, "karmalog")
               || !stricmp(arg1, "karma")) {
        PLAYER_INFO *pPInfo;
        bool fOld = FALSE;

        if (!(k = get_char_vis(ch, buf3))) {
            send_to_char("No such character in the world.\n\r", ch);
            return;
        }

        if (IS_NPC(k)) {
            send_to_char("Not on NPC's!\n\r", ch);
            return;
        }

        sprintf(buf, "Karma log for %s:\n\r", MSTR(k, name));
        send_to_char(buf, ch);
        if (k->desc)
            pPInfo = k->desc->player_info;
        else {
            DESCRIPTOR_DATA d;

            if ((pPInfo = find_player_info(k->player.info[0])) == NULL) {
                if ((fOld = load_player_info(&d, buf)) == FALSE) {
                    /* didn't find them */
                    sprintf(buf,
                            "Error displaying karma log, unable to find account" " for %s.\n\r",
                            k->name);
                    send_to_char(buf, ch);
                    return;
                }
                pPInfo = d.player_info;
                d.player_info = NULL;
            }
        }

        page_string(ch->desc, pPInfo->karmaLog, 1);

        if (fOld)
            free_player_info(pPInfo);
    } else {
        if (!(k = get_char_vis(ch, buf3))) {
            send_to_char("No such character in the world.\n\r", ch);
            return;
        }
        show_spec_stats_to_char(k, ch);
    }

#if defined(COMMENT_NOT_WORKING)
    sprintf(buf, "Age - Years: [%d], Months: [%d], Days: [%d], Hours: [%d]\n\r", age(k).year,
            age(k).month, age(k).day, age(k).hours);
    send_to_char(buf, ch);

    sprintf(buf, "Height: [%d cm], Weight: [%d kg]\n\r", GET_HEIGHT(k), GET_WEIGHT(k));
    send_to_char(buf, ch);

    strcat(buf, ", Char flags: ");
    sprint_flag(k->specials.act, char_flag, buf2);

    sprintf(buf, "Thirst: %d, Hunger: %d, Drunk: %d\n\r", k->specials.conditions[THIRST],
            k->specials.conditions[FULL], k->specials.conditions[DRUNK]);
    send_to_char(buf, ch);

    if (k->affected) {
        send_to_char("\n\rAffecting Spells:\n\r--------------\n\r", ch);
        for (aff = k->affected; aff; aff = aff->next) {
            sprintf(buf, "Spell : '%s'\n\r", skill_name[aff->type]);
            send_to_char(buf, ch);



            sprintf(buf, "     Modifies %s by %d points\n\r", apply_types[aff->location],
                    aff->modifier);
            send_to_char(buf, ch);
            sprintf(buf, "     Expires in %3d hours, Bits set ", aff->duration);
            send_to_char(buf, ch);

            sprint_flag(aff->bitvector, affect_flag, buf);
            strcat(buf, "\n\r");
            send_to_char(buf, ch);
        }
    }
    return;
#endif
}

void
cmd_shutdow(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char("If you want to shut something down - say so!\n\r", ch);
}

void
cmd_shutdown(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{

    int opt, time;
    char buf[100], arg[MAX_INPUT_LENGTH];
    char show_list[1000];
    const char * const options[] = {
        "noreboot",
        "reboot",
        "halt",
        "now",
        "check",
        "urgent",
        "abort",
        "\n"
    };

    SWITCH_BLOCK(ch);

    argument = one_argument(argument, arg, sizeof(arg));
    if ((!*arg) || (-1 == (opt = old_search_block(arg, 0, strlen(arg), options, 1)))) {
        send_to_char("Syntax is 'shutdown reboot #min'\n\r" " or       'shutdown noreboot #min'\n\r"
                     " or       'shutdown halt'\n\r" " or       'shutdown check'\n\r"
                     " or       'shutdown now'.\n\r" " or       'shutdown abort'.\n\r"
                     " or       'shutdown urgent'.\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg, sizeof(arg));
    time = atoi(arg);
    switch (opt) {
    case 1:
        sprintf(buf, "Shutdown by %s in %d minutes.\n\r", GET_NAME(ch), time);
        send_to_all(buf);
        sprintf(buf, "Shutdown by %s in %d minutes.", GET_NAME(ch), time);
        shhlog(buf);
        new_event(EVNT_SHUTDOWN, 60, 0, 0, 0, 0, time);
        break;
    case 2:
        sprintf(buf, "Reboot by %s in %d minutes.\n\r", GET_NAME(ch), time);
        send_to_all(buf);
        sprintf(buf, "Reboot by %s in %d minutes.", GET_NAME(ch), time);
        shhlog(buf);
        new_event(EVNT_SHUTDOWN, 60, 0, 0, 0, 0, time);
        break;
    case 3:
        sprintf(buf, "Reboot halted by %s.\n\r", GET_NAME(ch));
        send_to_all(buf);
        sprintf(buf, "Reboot halted by %s.", GET_NAME(ch));
        heap_delete(0, event_shutdown_cmp);
        shhlog(buf);
        break;
    case 4:                    /*  Immediate  */
        sprintf(buf, "Shutdown by %s NOW.\n\r", GET_NAME(ch));
        send_to_all(buf);
        sprintf(buf, "Shutdown by %s NOW.", GET_NAME(ch));
        shhlog(buf);
        shutdown_game = 1;
        break;
    case 5:                    /*  check */
        heap_print(show_list, 0, event_shutdown_cmp);
        send_to_char(show_list, ch);
        break;
    case 6:                    /*  urgent, no saving no waiting, go NOW */
        _exit(0);
        break;
    case 7:                    /*  dump core and die */
        abort();
        break;
    }
}

void
cmd_snoop(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    static char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    SWITCH_BLOCK(ch);

    if (!ch->desc)
        return;

    argument = one_argument(argument, arg, sizeof(arg));

    if (!*arg) {
        if (ch->desc->snoop.snooping) {
            send_to_char("Ok, you stop snooping.\n\r", ch);
            ch->desc->snoop.snooping->desc->snoop.snoop_by = 0;
            ch->desc->snoop.snooping = 0;
        } else {
            send_to_char("Snoop who?", ch);
        }
        return;
    }
    if (!(victim = get_char_vis(ch, arg))) {
        send_to_char("No such person around.\n\r", ch);
        return;
    }
    if ((!victim->desc) || IS_NPC(victim)) {
        send_to_char("There's no link.. nothing to snoop.\n\r", ch);
        return;
    }
    if (victim == ch) {
        send_to_char("How can you snoop yourself?\n\r", ch);
        return;
    }
    if (victim->desc->snoop.snoop_by) {
        send_to_char("Busy already. \n\r", ch);
        return;
    }
    if ((GET_LEVEL(ch) != OVERLORD) && (GET_LEVEL(victim) >= GET_LEVEL(ch))) {
        send_to_char("You failed.\n\r", ch);
        return;
    }
    send_to_char("Ok. \n\r", ch);

    if (ch->desc->snoop.snooping)

/* Adding echo for when they change snoops, per bug by Savak */
/* To change back, comment out the send_to_char.  -San       */
    {
        send_to_char("Ok, you switch targets.\n\r", ch);
        ch->desc->snoop.snooping->desc->snoop.snoop_by = 0;
    }

    ch->desc->snoop.snooping = victim;
    victim->desc->snoop.snoop_by = ch;
    return;
}

void
cmd_switch(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    static char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    argument = one_argument(argument, arg, sizeof(arg));

    if (!*arg) {
        send_to_char("Switch with who?\n\r", ch);
    } else {
        if (!(victim = get_char_vis(ch, arg)))
            send_to_char("They aren't here.\n\r", ch);
        else if (IS_NPC(victim)
                 && !has_privs_in_zone(ch, npc_index[victim->nr].vnum / 1000, MODE_CONTROL)) {
            send_to_char("You do not have controlling privileges in that " "zone.\n\r", ch);
            return;
        } else {
            if (ch == victim) {
                send_to_char("You feel strange as your other self takes over.\n\r", ch);
                return;
            }

            if (ch->desc->snoop.snooping) {
                send_to_char("Breaking snoop first.\n\r", ch);
                ch->desc->snoop.snooping->desc->snoop.snoop_by = 0;
                ch->desc->snoop.snooping = 0;
            }

            //  Previously, the above message was sent to gods who were being
            //  snooped and attempted to switch.  Made it so if this happens
            //  now, the snoop is just broken, and the god does the switch.
            //  I think thats preferable to the beans bein spilt.   - Kelvik
            if (ch->desc->snoop.snoop_by) {
                ch->desc->snoop.snoop_by->desc->snoop.snooping = 0;
                ch->desc->snoop.snoop_by = 0;
            }

            if (victim->desc || (!IS_NPC(victim))) {
                send_to_char("You can't do that, the body is already in use!\n\r", ch);
            } else {
                char buf[256];

                if( IS_SWITCHED(ch) ) {
                    CHAR_DATA *orig = SWITCHED_PLAYER(ch);

                    cprintf(ch, "Returning first.\n\r");
                    cmd_return(ch, "", CMD_RETURN, 0);
                    ch = orig;
                }

                send_to_char("Ok.\n\r", ch);
                ch->desc->character = victim;
                ch->desc->original = ch;

                if (get_char_extra_desc_value(ch, "[AUTO_NOSTAND]", buf, 256)) {
                    set_char_extra_desc_value(victim, "[NO_STAND]", "1");
                    cprintf( ch, "Nostand set.\n\r" );
                }

                // If they set a 'switched prompt' use it.
                if (!get_char_extra_desc_value(ch, "[SWITCHED_PROMPT]", buf, 256)) {
                    // if the prompt is already set, keep it
                    if( victim->player.prompt != NULL ) {
                        strcpy( buf, victim->player.prompt );
                    }
                    // otherwise give it a reasonable value
                    else {
                        strcpy( buf, "[#%D] %N %r > ");
                    }
                } else {
                    cprintf( ch, "Prompt auto-set.\n\r" );
                }
                DESTROY(victim->player.prompt);
                victim->player.prompt = strdup(buf);

                // stop cset level exploit
                GET_LEVEL(victim) = MIN(GET_LEVEL(victim), GET_LEVEL(ch));

                victim->desc = ch->desc;
                ch->desc = 0;

                /* Work-around for switching into an NPC that's already
                 * shadowwalked.  The main assumption is that the shadow
                 * has already dissipated.
                 */
                if (affected_by_spell(victim, PSI_SHADOW_WALK)) {
                    affect_from_char(victim, PSI_SHADOW_WALK);
                    af.type = SPELL_NONE;
                    af.duration = number(1, 2);
                    af.power = 0;
                    af.magick_type = MAGICK_NONE;
                    af.location = CHAR_APPLY_STUN;
                    af.modifier = GET_MAX_STUN(victim) / -number(2, 3);
                    af.bitvector = 0;
                    affect_to_char(victim, &af);
                }
            }
        }
    }
}

void
cmd_return(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!ch->desc)
        return;

    if (!ch->desc->original) {
        send_to_char(TYPO_MSG, ch);
        return;
    } else {
        if ((!affected_by_spell(ch, TYPE_SHAPE_CHANGE))
            && (!affected_by_spell(ch, SPELL_SEND_SHADOW))
            && (!affected_by_spell(ch, PSI_SHADOW_WALK))
            && (!affected_by_spell(ch, SPELL_POSSESS_CORPSE))) {
            send_to_char("You return to your original body.\n\r", ch);
        } else if (affected_by_spell(ch, PSI_SHADOW_WALK) && IS_NPC(ch)
                   && (npc_index[ch->nr].vnum != 1200 && npc_index[ch->nr].vnum != 1203)) {
            send_to_char("You return to your original body.\n\r", ch);
            affect_from_char(ch, PSI_SHADOW_WALK);
            af.type = SPELL_NONE;
            af.duration = number(1, 2);
            af.power = 0;
            af.magick_type = MAGICK_NONE;
            af.location = CHAR_APPLY_STUN;
            af.modifier = GET_MAX_STUN(ch) / -number(2, 3);
            af.bitvector = 0;
            affect_to_char(ch, &af);
        } else {
            if (affected_by_spell(ch, TYPE_SHAPE_CHANGE)) {
                char_from_room(ch->desc->original);
                char_to_room(ch->desc->original, ch->in_room);

                act("Your form blurs as you become $N.", FALSE, ch, 0, ch->desc->original, TO_CHAR);
                act("$n blurs and becomes $N.", TRUE, ch, 0, ch->desc->original, TO_ROOM);
            }
            if ((affected_by_spell(ch, SPELL_SEND_SHADOW))
                || (affected_by_spell(ch, PSI_SHADOW_WALK))
                || (affected_by_spell(ch, SPELL_POSSESS_CORPSE))) {
                act("You return to your physical form.", FALSE, ch, 0, ch->desc->original, TO_CHAR);
                act("$n dissipates into nothing.", TRUE, ch, 0, ch->desc->original, TO_ROOM);

                af.type = SPELL_NONE;
                af.duration = number(1, 2);
                af.power = 0;
                af.magick_type = MAGICK_NONE;
                af.location = CHAR_APPLY_STUN;
                af.modifier = GET_MAX_STUN(ch->desc->original) / -number(2, 3);
                af.bitvector = 0;

                if (!IS_IMMORTAL(ch->desc->original))
                    affect_to_char(ch->desc->original, &af);

                // Shadowwalkers also leave the affect on the original PC
                affect_from_char(ch->desc->original, PSI_SHADOW_WALK);

                // Reset their contact back to NULL
                ch->specials.contact = NULL;
            }
        }

        ch->desc->character = ch->desc->original;
        ch->desc->original = 0;

        ch->desc->character->desc = ch->desc;
        ch->desc = 0;

        if (argument && (strcmp(argument, "in_extract_hack") == 0))
            return;             // This character is about to be extracted already

        if ((affected_by_spell(ch, TYPE_SHAPE_CHANGE))
            || (affected_by_spell(ch, SPELL_SEND_SHADOW))
            || (affected_by_spell(ch, PSI_SHADOW_WALK) && IS_NPC(ch)
                && (npc_index[ch->nr].vnum == 1200 || npc_index[ch->nr].vnum == 1203))
            || (affected_by_spell(ch, SPELL_POSSESS_CORPSE)))
            extract_char(ch);

    }
}

void
cmd_force(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    struct descriptor_data *i;
    CHAR_DATA *vict, *staff;
    char name[100], to_force[400], buf[MAX_STRING_LENGTH];
    int silent = 0;

    staff = SWITCHED_PLAYER(ch);

    buf[0] = '\0';
    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, name, sizeof(name));

    if (!strcmp(name, "-s")) {
        silent = 1;
        argument = one_argument(argument, name, sizeof(name));
    }
    strcpy(to_force, argument);

    if (!*name || !*to_force)
        send_to_char("Who do you wish to force to do what?\n\r", ch);
    else if (strcmp("all", name)) {
        if (!strcmp("room", name)) {
            CHAR_DATA *vict_next;

            for (vict = ch->in_room->people; vict; vict = vict_next) {
                vict_next = vict->next_in_room;

                if (IS_NPC(vict)) {
                    if (!silent) {
                        sprintf(buf, "%s has forced you to '%s'.",
                                char_can_see_char(vict, staff) ? MSTR(ch,
                                                                   short_descr) : "A staff member",
                                to_force);
                        act(buf, FALSE, ch, 0, vict, TO_VICT);
                    }
                    parse_command(vict, to_force);
                }
            }
            send_to_char("Ok.\n\r", ch);
            return;
        } else if (!(vict = get_char_vis(ch, name)))
            send_to_char("No-one by that name here..\n\r", ch);
        else {
            if ((GET_LEVEL(staff) <= GET_LEVEL(SWITCHED_PLAYER(vict))))
                send_to_char("Oh no you don't!!\n\r", ch);
            else {

                if (!IS_ADMIN(staff) && !IS_NPC(vict)) {
                   cprintf(ch, "You cannot force '%s', they are a PC.\n\r",
                    PERS(vict, ch));
                   return;
                }
                if (!silent) {
                    sprintf(buf, "%s has forced you to '%s'.",
                            char_can_see_char(vict, staff) ? MSTR(staff, short_descr) : "A staff member",
                            to_force);
                    act(buf, FALSE, staff, 0, vict, TO_VICT);
                }
                send_to_char("Ok.\n\r", ch);
                parse_command(vict, to_force);
            }
        }
    } else if (IS_ADMIN(staff)) {
        for (i = descriptor_list; i; i = i->next)
            if (i->character != ch && !i->connected) {
                vict = i->character;
                if (GET_LEVEL(staff) < GET_LEVEL(vict))
                    send_to_char("Oh no you don't!!\n\r", ch);
                else {
                    if (!silent){
                        sprintf(buf, "%s has forced you to '%s'.",
                                char_can_see_char(vict, staff) ? MSTR(staff,
                                                                   short_descr) : "A staff member",
                                to_force);
                        act(buf, FALSE, staff, 0, vict, TO_VICT);
                    }
                    parse_command(vict, to_force);
                }
            }
        send_to_char("Ok.\n\r", ch);
    }
}


/* clean a room of all mobiles and objects */
void
cmd_purge(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vict, *next_v, *staff;
    struct obj_data *obj, *next_o;

    char name[100];

    staff = SWITCHED_PLAYER(ch);

    argument = one_argument(argument, name, sizeof(name));

    /* Adding in code to check for "room", as in purge room.
       This must be used to purge save zone rooms. Obj/Chars cannot be purged
       with this keyword anymore (unlikely to happen anyhow)
       Non save zone rooms can still be purged with no argument, just purge */

    if (*name && strcmp("room", name)) {
        if ((vict = get_char_room_vis(ch, name))) {
            if (!IS_NPC(vict) && (!IS_ADMIN(staff))) {
                send_to_char("Fuuuuuuuuu!\n\r", ch);
                return;
            }

            if( ch == vict ) {
                send_to_char("No purging yourself!\n\r", ch);
                return;
            }

            if (!IS_NPC(vict) && (GET_LEVEL(staff) < GET_LEVEL(vict))) {
                act("$N looks at you in disdain.", TRUE, ch, 0, vict, TO_CHAR);
                act("$n attempts to purge you, you laugh at his feeble attempt.", TRUE, ch, 0, vict,
                    TO_VICT);
                return;
            }
            act("$n disintegrates $N.", TRUE, staff, 0, vict, TO_NOTVICT);

            if (IS_NPC(vict)) {
                extract_char(vict);
            } else {
                if (vict->desc) {
                    close_socket(vict->desc);
                    vict->desc = 0;
                    extract_char(vict);
                } else {
                    extract_char(vict);
                }
            }
        } else if ((obj = get_obj_in_list_vis(ch, name, ch->in_room->contents))) {
            act("$n destroys $p.", TRUE, staff, obj, 0, TO_ROOM);
            obj_from_room(obj);
            extract_obj(obj);
        } else {
            send_to_char("I don't know anyone or anything by that name.\n\r", ch);
            return;
        }

        send_to_char("Ok.\n\r", ch);
    } else {                    /* no argument or "room", clean out the room */

        if ((ch->in_room) && is_save_zone(ch->in_room->zone) && !(*name)) {
            send_to_char("This is a save room, use 'purge room' if you are sure.\n\r", ch);
            return;
        }

        int doneall;

        act("$n makes a quick gesture...  A whirling wind billows around the room!", FALSE, staff, 0,
            0, TO_ROOM);
        send_to_char("A whirling wind billows around, leaving the area empty.\n\r", ch);


        doneall = FALSE;

        while (!doneall) {
            doneall = TRUE;

            for (vict = ch->in_room->people; vict && doneall; vict = next_v) {
                next_v = vict->next_in_room;
                if (vict != ch && IS_NPC(vict)) {
                    extract_char(vict);
                    doneall = FALSE;
                }
            }
        }

        for (obj = ch->in_room->contents; obj; obj = next_o) {
            next_o = obj->next_content;
            if (obj->nr >= 0 && !is_board(obj_index[obj->nr].vnum)) {
                obj_from_room(obj);
                extract_obj(obj);
            }
        }
    }
}


/* average attribute value */
#define AVERAGE_ATTRIBUTE_VALUE 28

char *attribute_priority[MAX_ATTRIBUTE] = {
    "first",
    "second",
    "third",
    "fourth",
};

float
convert_attribute_percent_to_range(int percent, int min, int max)
{
    float range = max - min;

    return (range * percent / 100) + min;
}


void
roll_abilities_preferred(int r, int choice[MAX_ATTRIBUTE], int ability[MAX_ATTRIBUTE], bool log)
{
    int rollSum = 0;
    int rolls[MAX_ATTRIBUTE];
    int minRange[MAX_ATTRIBUTE];
    int maxRange[MAX_ATTRIBUTE];
    int i;

    /* determine the roll range for the attributes */
    minRange[ATT_STR] = compute_stat_rating(r, ATT_STR, 0);
    maxRange[ATT_STR] = compute_stat_rating(r, ATT_STR, 1);
    minRange[ATT_AGL] = compute_stat_rating(r, ATT_AGL, 0);
    maxRange[ATT_AGL] = compute_stat_rating(r, ATT_AGL, 1);
    minRange[ATT_WIS] = compute_stat_rating(r, ATT_WIS, 0);
    maxRange[ATT_WIS] = compute_stat_rating(r, ATT_WIS, 1);
    minRange[ATT_END] = compute_stat_rating(r, ATT_END, 0);
    maxRange[ATT_END] = compute_stat_rating(r, ATT_END, 1);

    /* generate 4 rolls 1..100% */
    do {
        if (rollSum > 0) {
            qgamelogf(QUIET_REROLL, "Rerolling below-average rolls of %d %d %d %d", rolls[0],
                      rolls[1], rolls[2], rolls[3]);
            rollSum = 0;
        }

        for (i = 0; i < MAX_ATTRIBUTE; i++) {
            rolls[i] = number(1, 100);
            rollSum += rolls[i];
        }
        /* keep doing it until the sum of the rolls is at least average */
    } while (rollSum < MAX_ATTRIBUTE * AVERAGE_ATTRIBUTE_VALUE);

    /* sort the rolls based on priority */
    qsort(rolls, MAX_ATTRIBUTE, sizeof(int), revintcmp);

    /* translate the rolls into ranges */
    ability[ATT_STR] =
        convert_attribute_percent_to_range(rolls[choice[ATT_STR]], minRange[ATT_STR],
                                           maxRange[ATT_STR]);
    ability[ATT_AGL] =
        convert_attribute_percent_to_range(rolls[choice[ATT_AGL]], minRange[ATT_AGL],
                                           maxRange[ATT_AGL]);
    ability[ATT_WIS] =
        convert_attribute_percent_to_range(rolls[choice[ATT_WIS]], minRange[ATT_WIS],
                                           maxRange[ATT_WIS]);
    ability[ATT_END] =
        convert_attribute_percent_to_range(rolls[choice[ATT_END]], minRange[ATT_END],
                                           maxRange[ATT_END]);

 //   if (log) {
    	qgamelogf(QUIET_REROLL, "roll_abilities_preferred");
        qgamelogf(QUIET_REROLL, "  Attribute   Order    Roll   Range    Attribute (Text)");
        qgamelogf(QUIET_REROLL, "  strength    %-6s   %2d     %2d-%2d    %d (%s)",
                  attribute_priority[choice[ATT_STR]], rolls[choice[ATT_STR]], minRange[ATT_STR],
                  maxRange[ATT_STR], ability[ATT_STR],
                  how_good_stat[stat_num_raw(r, ATT_STR, ability[ATT_STR])]);
        qgamelogf(QUIET_REROLL, "  agility     %-6s   %2d     %2d-%2d    %d (%s)",
                  attribute_priority[choice[ATT_AGL]], rolls[choice[ATT_AGL]], minRange[ATT_AGL],
                  maxRange[ATT_AGL], ability[ATT_AGL],
                  how_good_stat[stat_num_raw(r, ATT_AGL, ability[ATT_AGL])]);
        qgamelogf(QUIET_REROLL, "  wisdom      %-6s   %2d     %2d-%2d    %d (%s)",
                  attribute_priority[choice[ATT_WIS]], rolls[choice[ATT_WIS]], minRange[ATT_WIS],
                  maxRange[ATT_WIS], ability[ATT_WIS],
                  how_good_stat[stat_num_raw(r, ATT_WIS, ability[ATT_WIS])]);
        qgamelogf(QUIET_REROLL, "  endurance   %-6s   %2d     %2d-%2d    %d (%s)",
                  attribute_priority[choice[ATT_END]], rolls[choice[ATT_END]], minRange[ATT_END],
                  maxRange[ATT_END], ability[ATT_END],
                  how_good_stat[stat_num_raw(r, ATT_END, ability[ATT_END])]);
 //   }
}

// parse the order of attributes
bool
parse_ability_order(const char *text, int order[MAX_ATTRIBUTE])
{
    char arg[MAX_INPUT_LENGTH];
    int i;

    // loop over the # of attributes (we're expecting that many arguments
    for (i = 0; i < MAX_ATTRIBUTE; i++) {
        // grab the next item
        text = one_argument(text, arg, sizeof(arg));

        // if nothing, we're done, unable to parse
        if (*arg == '\0') {
            qgamelogf(QUIET_REROLL, "parse_ability_order: Not enough attributes");
            return FALSE;
        }
        // if strength
        if (!str_prefix(arg, "strength,")) {
            order[ATT_STR] = i;
        }
        // if agility
        else if (!str_prefix(arg, "agility,")) {
            order[ATT_AGL] = i;
        }
        // if wisdom
        else if (!str_prefix(arg, "wisdom,")) {
            order[ATT_WIS] = i;
        }
        // if endurance
        else if (!str_prefix(arg, "endurance,")) {
            order[ATT_END] = i;
        }
        // otherwise something else
        else {
            qgamelogf(QUIET_REROLL, "parse_ability_order: unable to parse '%s'", arg);
            return FALSE;
        }
    }

    // success!
    return TRUE;
}

// do a roll of abilities purely based on race (ignores guild & age)
void
roll_abilities_raw(int r, int attr[MAX_ATTRIBUTE])
{
  //  char buf [MAX_STRING_LENGTH];
  //  sprintf(buf, "Rolling stats for %s", race[r].name);
  //  shhlog (buf);
    attr[ATT_STR] = dice(race[r].strdice, race[r].strsize) + race[r].strplus;
    attr[ATT_AGL] = dice(race[r].agldice, race[r].aglsize) + race[r].aglplus;
    attr[ATT_WIS] = dice(race[r].wisdice, race[r].wissize) + race[r].wisplus;
    attr[ATT_END] = dice(race[r].enddice, race[r].endsize) + race[r].endplus;
}

// Roll a character's abilities, watches for [STAT_ORDER] and [UNIQUE_STATS]
void
roll_abilities(CHAR_DATA * ch)
{
    int rolls[MAX_ATTRIBUTE];
    int abilities[MAX_ATTRIBUTE];
    int age_g;
    int r;                      /* character's race */
    int g;                      /* character's guild */
    const char *stat_str = NULL;
    char str_str[5], agl_str[5], wis_str[5], end_str[5];
    char height_str[256];
    char weight_str[256];
    char buf[MAX_STRING_LENGTH];
    int minRange[MAX_ATTRIBUTE];
    int maxRange[MAX_ATTRIBUTE];

    r = GET_RACE(ch);
    g = GET_GUILD(ch);

    /* determine the roll range for the attributes */
    minRange[ATT_STR] = compute_stat_rating(r, ATT_STR, 0);
    maxRange[ATT_STR] = compute_stat_rating(r, ATT_STR, 1);
    minRange[ATT_AGL] = compute_stat_rating(r, ATT_AGL, 0);
    maxRange[ATT_AGL] = compute_stat_rating(r, ATT_AGL, 1);
    minRange[ATT_WIS] = compute_stat_rating(r, ATT_WIS, 0);
    maxRange[ATT_WIS] = compute_stat_rating(r, ATT_WIS, 1);
    minRange[ATT_END] = compute_stat_rating(r, ATT_END, 0);
    maxRange[ATT_END] = compute_stat_rating(r, ATT_END, 1);

    if (!IS_NPC(ch)) {
        qgamelogf(QUIET_REROLL, "Rerolling %s (%s) - %s %s; Age %d", GET_NAME(ch), ch->account,
                  race[r].name, guild[g].name, GET_AGE(ch));
    }
    // if they have a race
    if (r) {
        // look for a stat order edesc
        if (get_char_extra_desc_value(ch, "[STAT_ORDER]", buf, sizeof(buf))) {
            int order[MAX_ATTRIBUTE];
            int len = strlen(buf) - 1;

            qgamelogf(QUIET_REROLL, "Rerolling with preference '%s'", buf);

            while (buf[len] == '\n' || buf[len] == '\r') {
                buf[len--] = '\0';
            }

            // parse buf into order
            if (!parse_ability_order(buf, order)) {
                qgamelogf(QUIET_REROLL, "roll_abilities: unable to parse order '%s'", buf);
                roll_abilities_raw(r, rolls);
            }
            // roll the new abilities
            else {
                roll_abilities_preferred(r, order, rolls, !IS_NPC(ch));
            }
        }
        // no STAT_ORDER edesc
        else {
            roll_abilities_raw(GET_RACE(ch), rolls);
        }
    } else {                    // no race
        rolls[ATT_STR] = 10;
        rolls[ATT_AGL] = 10;
        rolls[ATT_WIS] = 10;
        rolls[ATT_END] = 10;
    }

    abilities[ATT_STR] = rolls[ATT_STR] + guild[g].strmod;
    abilities[ATT_AGL] = rolls[ATT_AGL] + guild[g].aglmod;
    abilities[ATT_WIS] = rolls[ATT_WIS] + guild[g].wismod;
    abilities[ATT_END] = rolls[ATT_END] + guild[g].endmod;

    if (GET_RACE(ch) && (race[r].max_age > 0)) {
        age_g = (age(ch).year * 20) / race[r].max_age;
        age_g = MIN(age_g, 20);
        age_g = MAX(age_g, 0);

        abilities[ATT_STR] *= age_stat_table.str[age_g];
        abilities[ATT_STR] /= 1000;
        abilities[ATT_AGL] *= age_stat_table.agl[age_g];
        abilities[ATT_AGL] /= 1000;
        abilities[ATT_END] *= age_stat_table.end[age_g];
        abilities[ATT_END] /= 1000;
        abilities[ATT_WIS] *= age_stat_table.wis[age_g];
        abilities[ATT_WIS] /= 1000;
    }

    abilities[ATT_STR] = MIN(abilities[ATT_STR], 100);
    abilities[ATT_STR] = MAX(abilities[ATT_STR], 1);

    abilities[ATT_AGL] = MIN(abilities[ATT_AGL], MAX_ABILITY);
    abilities[ATT_AGL] = MAX(abilities[ATT_AGL], 1);

    abilities[ATT_WIS] = MIN(abilities[ATT_WIS], MAX_ABILITY);
    abilities[ATT_WIS] = MAX(abilities[ATT_WIS], 1);

    abilities[ATT_END] = MIN(abilities[ATT_END], MAX_ABILITY);
    abilities[ATT_END] = MAX(abilities[ATT_END], 1);

    if (!IS_NPC(ch)) {
        qgamelogf(QUIET_REROLL, "  Attribute   Range    Roll  Guild Mod   Age Mod   Final (Text)");
        qgamelogf(QUIET_REROLL, "  %9s   %2d-%2d      %2d         %2d        %2d   %2d (%s)",
                  attributes[ATT_STR], minRange[ATT_STR], maxRange[ATT_STR], rolls[ATT_STR],
                  guild[g].strmod, abilities[ATT_STR] - (rolls[ATT_STR] + guild[g].strmod),
                  abilities[ATT_STR], how_good_stat[stat_num_raw(r, ATT_STR, abilities[ATT_STR])]);
        qgamelogf(QUIET_REROLL, "  %9s   %2d-%2d      %2d         %2d        %2d   %2d (%s)",
                  attributes[ATT_AGL], minRange[ATT_AGL], maxRange[ATT_AGL], rolls[ATT_AGL],
                  guild[g].aglmod, abilities[ATT_AGL] - (rolls[ATT_AGL] + guild[g].aglmod),
                  abilities[ATT_AGL], how_good_stat[stat_num_raw(r, ATT_AGL, abilities[ATT_AGL])]);
        qgamelogf(QUIET_REROLL, "  %9s   %2d-%2d      %2d         %2d        %2d   %2d (%s)",
                  attributes[ATT_WIS], minRange[ATT_WIS], maxRange[ATT_WIS], rolls[ATT_WIS],
                  guild[g].wismod, abilities[ATT_WIS] - (rolls[ATT_WIS] + guild[g].wismod),
                  abilities[ATT_WIS], how_good_stat[stat_num_raw(r, ATT_WIS, abilities[ATT_WIS])]);
        qgamelogf(QUIET_REROLL, "  %9s   %2d-%2d      %2d         %2d        %2d   %2d (%s)",
                  attributes[ATT_END], minRange[ATT_END], maxRange[ATT_END], rolls[ATT_END],
                  guild[g].endmod, abilities[ATT_END] - (rolls[ATT_END] + guild[g].endmod),
                  abilities[ATT_END], how_good_stat[stat_num_raw(r, ATT_END, abilities[ATT_END])]);
    }

    ch->abilities.str = abilities[ATT_STR];
    ch->abilities.end = abilities[ATT_END];
    ch->abilities.agl = abilities[ATT_AGL];
    ch->abilities.wis = abilities[ATT_WIS];

    if (IS_NPC(ch))
        if (find_ex_description("[UNIQUE_STATS]", ch->ex_description, TRUE)) {
            stat_str = find_ex_description("[UNIQUE_STATS]", ch->ex_description, TRUE);
            stat_str = one_argument(stat_str, str_str, sizeof(str_str));
            stat_str = one_argument(stat_str, agl_str, sizeof(agl_str));
            stat_str = one_argument(stat_str, wis_str, sizeof(wis_str));
            stat_str = one_argument(stat_str, end_str, sizeof(end_str));
            stat_str = one_argument(stat_str, height_str, sizeof(height_str));
            stat_str = one_argument(stat_str, weight_str, sizeof(weight_str));
            if (strlen(str_str) && strlen(agl_str) && strlen(wis_str) && strlen(end_str)) {
                SET_STR(ch, atoi(str_str));
                ch->abilities.str = atoi(str_str);
                SET_AGL(ch, atoi(agl_str));
                ch->abilities.agl = atoi(agl_str);
                SET_WIS(ch, atoi(wis_str));
                ch->abilities.wis = atoi(wis_str);
                SET_END(ch, atoi(end_str));
                ch->abilities.end = atoi(end_str);
            } else {
                gamelogf
                    ("Found [UNIQUE_STATS] edesc on NPC #%d but it was incorrectly formatted.  Rolling random stats.",
                     npc_index[ch->nr].vnum);
            }
            if (strlen(height_str)) {
                if (!strlen(weight_str)) {
                    gamelogf
                        ("Found height in [UNIQUE_STATS edesc on NPC #%d but not weight.  Generating random height & weight.",
                         npc_index[ch->nr].vnum);
                } else {
                    ch->player.height = atoi(height_str);
                    ch->player.weight = atoi(weight_str);
                }
            }
        }
    ch->tmpabilities = ch->abilities;
}


void
cmd_advance(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *victim;
    char name[100], level[100], buf[240];
    int adv, newlevel, i;

    SWITCH_BLOCK(ch);

    argument = two_arguments(argument, name, sizeof(name), level, sizeof(level));

    if (*name) {
        if (!(victim = get_char_room_vis(ch, name))) {
            send_to_char("That player is not here.\n\r", ch);
            return;
        }
    } else {
        send_to_char("Advance who?\n\r", ch);
        return;
    }

    if (IS_NPC(victim)) {
        send_to_char("NO! Not on NPC's.\n\r", ch);
        return;
    }

    if (!victim->desc) {
        send_to_char("They must be online.\n\r", ch);
        return;
    }

    else if (!*level) {
        send_to_char("You must supply a level number.\n\r", ch);
        return;
    } else {
        if (!isdigit(*level)) {
            send_to_char("Second argument must be a positive integer.\n\r", ch);
            return;
        }
        if ((newlevel = atoi(level)) <= GET_LEVEL(victim)) {
            send_to_char("Can't diminish a players status (yet).\n\r", ch);
            return;
        }
        adv = newlevel - GET_LEVEL(victim);
    }

    if (newlevel > GET_LEVEL(ch)) {
        send_to_char("Thou art not godly enough.\n\r", ch);
        return;
    }

    if ((adv + GET_LEVEL(victim)) > OVERLORD) {
        send_to_char("5 is the highest possible level.\n\r", ch);
        return;
    }

    send_to_char("You feel generous.\n\r", ch);
    act("$n makes some strange gestures.\n\rA strange feeling comes uppon you,"
        "\n\rLike a giant hand, light comes down from\n\rabove, grabbing your "
        "body, that begins\n\rto pulse with coloured lights from inside.\n\rYo"
        "ur head seems to be filled with deamons\n\rfrom another plane as your"
        " body dissolves\n\rto the elements of time and space itself.\n\rSudde"
        "nly a silent explosion of light snaps\n\ryou back to reality. You fee"
        "l slightly\n\rdifferent.", FALSE, ch, 0, victim, TO_VICT);

    sprintf(buf, "%s advances %s to level %d.", GET_NAME(ch), GET_NAME(victim), newlevel);
    shhlog(buf);

    if (GET_LEVEL(victim) == MORTAL) {
        add_immortal_name(victim->name, victim->desc->player_info->name);
        save_immortal_names();
    }

    GET_LEVEL(victim) = newlevel;
    GET_COND(victim, FULL) = -1;
    GET_COND(victim, THIRST) = -1;
    GET_COND(victim, DRUNK) = -1;
    victim->specials.il = (newlevel + 1);

    /* added to stop legends and builders from seeing the total skill list */
    if (GET_LEVEL(victim) > BUILDER) {
        for (i = 0; i < MAX_SKILLS; i++)
            if (has_skill(ch, i))
                /* if (ch->skills[i]) */
            {
                ch->skills[i]->learned = 100;
                ch->skills[i]->rel_lev = 0;
            } else
                init_skill(ch, i, 100);
    } else if (GET_LEVEL(victim) > MORTAL && GET_LEVEL(victim) < STORYTELLER) {
        /*
         * Builders & Legends don't need to see the gamelogin/gamelogout/db
         * messages
         */
        ch->specials.quiet_level = ~0;
    }
}


void
cmd_reroll(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *victim, *staff;
    char buf[100], buf2[100];
    struct time_info_data playing_time;
    time_t now;
    struct tm *currentTime;
    int oldStr, oldAgl, oldWis, oldEnd;

    staff = SWITCHED_PLAYER(ch);
    argument = one_argument(argument, buf, sizeof(buf));

    /* Everything passed */
    now = time(0);
    currentTime = localtime(&now);

#ifdef CHAR_CAN_REROLL
    if (!IS_IMMORTAL(staff)) {
        // check play time first
        playing_time =
            real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
        if (playing_time.day > 0 || playing_time.hours > 3) {
            /* Cannot reroll yourself if too much time has passed */
            cprintf(ch, "You may only reroll yourself within the first 3 hours of play.\n\r");
            return;
        }

        if (*buf && !strcmp("undo", buf)) {
           // reroll undo
           // see if they have the edesc
           if (!get_char_extra_desc_value(ch, "[FIRST_ROLL]", buf, sizeof(buf))) {
               cprintf(ch, "You have not rerolled yet, type 'reroll self' to reroll.\n\r");
               return;
           }

           if (get_char_extra_desc_value(ch, "[REROLL_UNDO]", buf2, sizeof(buf2))) {
               cprintf(ch, "You have already undone your reroll.\n\r");
               return;
           }

           // store the 'undone' roll in i14
           sprintf(buf2, "Reroll undone %d/%d: Second Roll - Str: %d  Agl: %d  Wis: %d  End: %d.",
                currentTime->tm_mon + 1, currentTime->tm_mday, GET_STR(ch), GET_AGL(ch),
                GET_WIS(ch), GET_END(ch));

           if (ch->player.info[14])
               free((char *) ch->player.info[14]);
           ch->player.info[14] = strdup(buf2);

           qgamelogf(QUIET_REROLL, "%s (%s) has reverted their reroll:",
            GET_NAME(ch), ch->account);
           qgamelogf(QUIET_REROLL, "  From Str: %2d Agl: %2d  Wis: %2d  End %d",
            GET_STR(ch), GET_AGL(ch), GET_WIS(ch), GET_END(ch));

           // read the stats from the edesc
           sscanf(buf, "%d %d %d %d", &oldStr, &oldAgl, &oldWis, &oldEnd);

           // revert the stats
           ch->abilities.str = oldStr;
           ch->abilities.agl = oldAgl;
           ch->abilities.wis = oldWis;
           ch->abilities.end = oldEnd;
           ch->tmpabilities = ch->abilities;

           qgamelogf(QUIET_REROLL, "    To Str: %2d Agl: %2d  Wis: %2d End %2d",
            GET_STR(ch), GET_AGL(ch), GET_WIS(ch), GET_END(ch));

           // reset hp/stun/mana
           init_hp(ch);
           init_stun(ch);
           set_mana(ch, GET_MAX_MANA(ch));

           // tell them the result
           cprintf(ch, "Your strength is now %s, your agility is now %s,\n\r",
                STAT_STRING(ch, ATT_STR), STAT_STRING(ch, ATT_AGL));
           cprintf(ch, "  your wisdom is now %s, and your endurance is now %s.\n\r",
                STAT_STRING(ch, ATT_WIS), STAT_STRING(ch, ATT_END));

           set_char_extra_desc_value(ch, "[REROLL_UNDO]", "---");
           // force a save
           cmd_save(ch, "", CMD_SAVE, 0);
           return;
        }

        if (!*buf || strcmp("self", buf)) {
            /* Tell them the syntax -- makes them type "reroll self" */
            cprintf(ch, "You have to write 'reroll self' -- no less -- to reroll.\n\r");
            return;
        }


        if (ch->in_room && ch->in_room->number == 1003) {       /* Hall of Kings */
            cprintf(ch, "You are automatically rerolled when you 'point' at the map.\n\r");
            return;
        }

        /* We're going to use the FIRST_ROLL edesc as the marker for whether they have
         * already performed a reroll */
        if (get_char_extra_desc_value(ch, "[FIRST_ROLL]", buf, sizeof(buf))) {
            /* Already got something there, so no reroll allowed */
            cprintf(ch,
                    "You have already rerolled this character.  "
                    "Only one reroll is allowed.\n\r");
            return;
        }

        sprintf(buf, "Rerolled %d/%d: Before - Str: %d  Agl: %d  Wis: %d  End: %d.\n\r",
                currentTime->tm_mon + 1, currentTime->tm_mday, GET_STR(ch), GET_AGL(ch),
                GET_WIS(ch), GET_END(ch));

        oldStr = GET_STR(ch);
        oldAgl = GET_AGL(ch);
        oldWis = GET_WIS(ch);
        oldEnd = GET_END(ch);

        sprintf(buf, "%d %d %d %d", oldStr, oldAgl, oldWis, oldEnd);
        set_char_extra_desc_value(ch, "[FIRST_ROLL]", buf);

        /* Add text to pinfo #13 */
        if (ch->player.info[14])
            free((char *) ch->player.info[14]);
        ch->player.info[14] = strdup(buf);

        /* Do the reroll */
        roll_abilities(ch);
        init_hp(ch);
        init_stun(ch);

        // This resets their mana so non-casters can't gather after a reroll, which
        // can result in their max_mana going up.  -Nessalin 1/17/2004
        set_mana(ch, GET_MAX_MANA(ch));

        sprintf(buf, "%s rerolled: Str: %+d Agl: %+d Wis: %+d End: %+d\n\r", MSTR(ch, name),
                GET_STR(ch) - oldStr, GET_AGL(ch) - oldAgl, GET_WIS(ch) - oldWis,
                GET_END(ch) - oldEnd);
        send_to_monitor(ch, buf, MONITOR_OTHER);

        cprintf(ch, "Your strength is now %s, your agility is now %s,\n\r",
                STAT_STRING(ch, ATT_STR), STAT_STRING(ch, ATT_AGL));
        cprintf(ch, "  your wisdom is now %s, and your endurance is now %s.\n\r",
                STAT_STRING(ch, ATT_WIS), STAT_STRING(ch, ATT_END));

        cmd_save(ch, "", CMD_SAVE, 0);

        cprintf(ch, "\n\rIf you are unhappy with this roll, type 'reroll undo' to revert to your\n\r"
                    "original roll.  You have until the character has 3 hours of playtime to decide.\n\r");
        return;
    }
#endif

    /* allow testing the new reroll functionality */
    if (*buf && !strcmp(buf, "test")) {
        int r;
        int i;
        int choices[MAX_ATTRIBUTE];
        int abilities[MAX_ATTRIBUTE];

        argument = one_argument(argument, buf, sizeof(buf));

        if (!*buf) {
            cprintf(ch, "Syntax: reroll test <race> <attr 1> <attr 2> <attr 3> <attr 4>\n\r");
            return;
        }

        for (r = 1; r < MAX_RACES; r++)
            if (!(str_prefix(buf, race[r].name)))
                break;

        if (r == MAX_RACES) {
            cprintf(ch, "Can't find the race '%s' to test.\n\r", buf);
            return;
        }

        for (i = 0; i < MAX_ATTRIBUTE; i++) {
            argument = one_argument(argument, buf, sizeof(buf));

            if (!*buf) {
                cprintf(ch,
                        "You must specify all four attributes in the order the characte prefers.\n\r");
                return;
            }

            if (!str_prefix(buf, "strength,"))
                choices[ATT_STR] = i;
            else if (!str_prefix(buf, "agility,"))
                choices[ATT_AGL] = i;
            else if (!str_prefix(buf, "wisdom,"))
                choices[ATT_WIS] = i;
            else if (!str_prefix(buf, "endurance,"))
                choices[ATT_END] = i;
            else {
                cprintf(ch,
                        "Invalid choice '%s'.\n\rValid choices are 'strength', 'agility', 'wisdom', 'endurance'\n\r",
                        buf);
                return;
            }
        }

        cprintf(ch,
                "Make sure you have quiet log off, all echos in this command are done through the log.\n\r");

        roll_abilities_preferred(r, choices, abilities, TRUE);
        return;
    }

    /* I put this check in here because I've changed the permission in
     * parser.c to allow mortals+ to access this function.  By leaving this
     * outside the #ifdef CHAR_CAN_REROLL block, if CHAR_CAN_REROLL is
     * undefined, it will work as it used to (for HL+ only).  It is not in
     * a #else block because we still want only HL+ immortals to be able to
     * reroll characters regardless of whether we allow mortals to reroll
     * themselves. */

    if (!IS_ADMIN(staff)) {
        /* An immortal tried to reroll, but they don't have permission. */
        send_to_char("What?\n\r", ch);
        return;
    }

    if (!*buf)
        send_to_char("Who do you wish to reroll?\n\r", ch);
    else if (!(victim = get_char(buf)))
        send_to_char("No-one by that name in the world.\n\r", ch);
    else {
        sprintf(buf, "Before - Str: %d  Agl: %d  Wis: %d  End: %d.\n\r", GET_STR(victim),
                GET_AGL(victim), GET_WIS(victim), GET_END(victim));
        strcpy(buf2, buf);
        send_to_char(buf, ch);
        qgamelogf(QUIET_REROLL, "%s (%s): %s", GET_NAME(ch), ch->account, buf);
        roll_abilities(victim);
        init_hp(ch);
        init_stun(ch);
        sprintf(buf, "After  - Str: %d  Agl: %d  Wis: %d  End: %d.\n\r", GET_STR(victim),
                GET_AGL(victim), GET_WIS(victim), GET_END(victim));
        qgamelogf(QUIET_REROLL, "%s", buf);
        send_to_char(buf, ch);

        if (IS_NPC(victim))
            return;

        /* Save reroll info on a PC */
        now = time(0);
        currentTime = localtime(&now);
        sprintf(buf, "%s performed reroll %d/%d: %s", GET_NAME(ch),
                currentTime->tm_mon + 1, currentTime->tm_mday, buf2);

        /* Add text to pinfo #13 */
        if (victim->player.info[14])
            free((char *) victim->player.info[14]);
        victim->player.info[14] = strdup(buf);

    }
}

void
cmd_restore(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *victim, *staff;
    char buf[100];
    int i;

    staff = SWITCHED_PLAYER(ch);
 
    argument = one_argument(argument, buf, sizeof(buf));
    if (!*buf)
        send_to_char("Who do you wish to restore?\n\r", ch);
    else if (!(victim = get_char_vis(ch, buf)))
        send_to_char("There is no one by that name here.\n\r", ch);
    else {
        set_mana(victim, GET_MAX_MANA(victim));
        set_hit(victim, GET_MAX_HIT(victim));
        set_move(victim, GET_MAX_MOVE(victim));
        set_stun(victim, GET_MAX_STUN(victim));

        if (IS_IMMORTAL(victim)) {
            for (i = 1; i < MAX_SKILLS; i++) {
                if (i == PSI_IMMERSION) // Let's not set this one
                    continue;
                if (!has_skill(victim, i))
                    /* if (!victim->skills[i]) */
                    init_skill(victim, i, 0);
                victim->skills[i]->learned = 100;
                set_skill_taught(victim, i);
                /* victim->skills[i]->taught = TRUE; */
            }

            victim->abilities.str = 100;
            victim->abilities.agl = 100;
            victim->abilities.wis = 100;
            victim->abilities.end = 100;

            victim->tmpabilities = victim->abilities;
        }

        update_pos(victim);
        send_to_char("Done.\n\r", ch);
        act("You have been fully healed by $N!", FALSE, victim, 0, staff, TO_CHAR);
    }
}

void
cmd_award(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vict;
    char type[256], person[256], buf[256], plus_minus[256];
    PLAYER_INFO *pPInfo;
    time_t now;
    struct tm *currentTime;
    char time_str[10];

    now = time(0);
    currentTime = localtime(&now);

    argument = two_arguments(argument, type, sizeof(type), person, sizeof(type));

    if (!*type || !*argument) {
        send_to_char("Usage: award <type> <person> <reason>\n\r", ch);
        send_to_char("Possible awards: life, skill, karma\n\r", ch);
        return;
    }

    if (!(vict = get_char_vis(ch, person))) {
        send_to_char("No one playing by that name.\n\r", ch);
        return;
    }

    if (IS_NPC(vict)) {
        send_to_char("NPCs don't need awards.\n\r", ch);
        return;
    }

    if (IS_IMMORTAL(vict)) {
        act("$N doesn't need any awards, being immortal.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    if (!strcmp(type, "life")) {
        if (IS_SET(vict->specials.act, CFL_NO_DEATH)) {
            send_to_char("That person is already allowed one death.\n\r", ch);
            return;
        }
        MUD_SET_BIT(vict->specials.act, CFL_NO_DEATH);
        send_to_char("Done.\n\r", ch);
        send_to_char("You feel infused with the energy of life.\n\r", vict);
        return;
    }

    if (!strcmp(type, "skill")) {
        if (IS_SET(vict->specials.act, CFL_NEW_SKILL)) {
            send_to_char("That person is already capable of learning a new skill.\n\r", ch);
            return;
        }
        MUD_SET_BIT(vict->specials.act, CFL_NEW_SKILL);
        send_to_char("Done.\n\r", ch);
        send_to_char("You suddenly feel a new capacity for knowledge.\n\r", vict);
        return;
    }

    if (!strcmp(type, "karma")) {
        char buf2[MAX_STRING_LENGTH * 10];
        bool fOld = FALSE;
        int prev;

        argument = one_argument(argument, plus_minus, sizeof(plus_minus));
        if (!*argument || !*plus_minus) {
            send_to_char("You must add a reason why.\n\r", ch);
            return;
        }

        if (vict->desc && vict->desc->player_info) {
            pPInfo = vict->desc->player_info;
        } else {
            DESCRIPTOR_DATA d;

            if ((pPInfo = find_player_info(vict->account)) == NULL) {
                if ((fOld = load_player_info(&d, vict->account)) == FALSE) {
                    sprintf(buf, "Error awarding karma for '%s', no account" " '%s' found.",
                            vict->name, vict->account);
                    gamelog(buf);
                    return;
                }
                pPInfo = d.player_info;
                d.player_info = NULL;
            }
        }

        prev = pPInfo->karma;
        if (STRINGS_SAME(plus_minus, "+")) {
            pPInfo->karma = MIN(255, pPInfo->karma + 1);
            sprintf(buf, "added 1 karma, making %d total", pPInfo->karma);
        } else if (STRINGS_SAME(plus_minus, "-")) {
            pPInfo->karma = MAX(0, pPInfo->karma - 1);
            sprintf(buf, "removed 1 karma, making %d total", pPInfo->karma);
        } else {
            send_to_char("Syntax:  award karma <person> +/- <reason>\n\r", ch);
            return;
        }

        sprintf(time_str, "%2d/%02d/%02d", currentTime->tm_mon + 1, currentTime->tm_mday,
                currentTime->tm_year - 100);

        /* store the reason in their karmaLog */
        sprintf(buf2, "%s%s %s, for %s - %s.\n\r", pPInfo->karmaLog ? pPInfo->karmaLog : "",
                MSTR(SWITCHED_PLAYER(ch), name), buf, argument, time_str);

        if (pPInfo->karmaLog || (pPInfo->karmaLog && !strcmp(pPInfo->karmaLog, "(null)")))
            free(pPInfo->karmaLog);
        pPInfo->karmaLog = strdup(buf2);

        // Good place to call something to update the guilds/races
        update_karma_options(ch, pPInfo, prev);
        save_player_info(pPInfo);
        if (fOld)
            free_player_info(pPInfo);

        strcat(buf, ".\n\r");
        send_to_char(capitalize(buf), ch);
        return;
    }

}

void
cmd_nowish(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vict;
    char buf[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, buf, sizeof(buf));
    if (!(vict = get_char_vis(ch, buf)))
        send_to_char("Couldn't find any such player.\n\r", ch);
    else if (IS_NPC(vict))
        send_to_char("Can't do that to an NPC.\n\r", ch);
    else if (GET_LEVEL(vict) > GET_LEVEL(SWITCHED_PLAYER(ch)))
        act("$E might object to that.. better not.", 0, ch, 0, vict, TO_CHAR);
    else if (IS_SET(vict->specials.act, CFL_NOWISH)) {
        send_to_char("You can wish again.\n\r", vict);
        send_to_char("NOWISH removed.\n\r", ch);
        REMOVE_BIT(vict->specials.act, CFL_NOWISH);
    } else {
        send_to_char("The gods take away your ability to wish!\n\r", vict);
        send_to_char("NOWISH set.\n\r", ch);
        MUD_SET_BIT(vict->specials.act, CFL_NOWISH);
    }
}

/*
 * The following functions are for cmd_show
 */
void
cat_skills_prev_to_string(int gn, int indent, int prev_skill, char *text)
{
    int i;
    char temp[256];
    char prefix[256] = "  ";
    for (i = 0; i < indent; i++) {
        strcat(prefix, ". ");
    }

    for (i = 1; i < MAX_SKILLS; i++)
        if (guild[gn].skill_prev[i] == prev_skill) {
            if (skill[i].sk_class == CLASS_MAGICK) {
                sprintf(temp, "%s%s   (st=%d, max=%d, min_mana=%d, go on=%d)\n\r", prefix,
                        skill_name[i], guild[gn].skill_percent[i], guild[gn].skill_max_learned[i],
                        mana_amount_level(POWER_MON, POWER_WEK + 1, i),
                        guild[gn].skill_max_learned[i] - 10);
            } else {
                sprintf(temp, "%s%s   (st=%d, max=%d, go on=%d)\n\r", prefix, skill_name[i],
                        guild[gn].skill_percent[i], guild[gn].skill_max_learned[i],
                        guild[gn].skill_max_learned[i] - 10);
            }
            strcat(text, temp);

            if (!((i == SPELL_NONE) || (i == TYPE_UNDEFINED)))
                cat_skills_prev_to_string(gn, indent + 1, i, text);
        }
}


void
cat_subskills_prev_to_string(int gn, int indent, int prev_skill, char *text)
{
    int i;
    char temp[256];
    char prefix[256] = "  ";
    for (i = 0; i < indent; i++) {
        strcat(prefix, ". ");
    }
    shhlog(temp);
    for (i = 1; i < MAX_SKILLS; i++)
        if (sub_guild[gn].skill_prev[i] == prev_skill) {
            if (skill[i].sk_class == CLASS_MAGICK) {
                sprintf(temp, "%s%s   (st=%d, max=%d, min_mana=%d, go on=%d)\n\r", prefix,
                        skill_name[i], sub_guild[gn].skill_percent[i],
                        sub_guild[gn].skill_max_learned[i], mana_amount_level(POWER_MON,
                                                                              POWER_WEK + 1, i),
                        sub_guild[gn].skill_max_learned[i] - 10);
            } else {
                sprintf(temp, "%s%s   (st=%d, max=%d, go on=%d)\n\r", prefix, skill_name[i],
                        sub_guild[gn].skill_percent[i], sub_guild[gn].skill_max_learned[i],
                        sub_guild[gn].skill_max_learned[i] - 10);
            }
            strcat(text, temp);

            if (!((i == SPELL_NONE) || (i == TYPE_UNDEFINED)))
                cat_subskills_prev_to_string(gn, indent + 1, i, text);
        }
}

int
count_rooms()
{
    int i, count = 0;

    for (i = 0; i <= top_of_zone_table; i++)
        count += count_rooms_in_zone(i);
    return count;
}

int
count_rooms_in_zone(int zone)
{
    return zone_table[zone].rcount;
}

int
count_urooms_in_zone(int zone)
{
    int amount = 0;
    struct room_data *room;

    struct descriptor_data *tmpdesc;

    for (room = zone_table[zone].rooms; room; room = room->next_in_zone)
        if (room->description && *room->description) {
            for (tmpdesc = descriptor_list; tmpdesc; tmpdesc = tmpdesc->next)
                if (tmpdesc->str == &room->description)
                    break;

            if (!tmpdesc)
                if (!strncmp(room->description, "Empty", 5))
                    amount++;
        }
    return amount;
}

int
count_objs_in_zone(int zone)
{
    int amount, obj;
    for (obj = 0, amount = 0; obj < top_of_obj_t; obj++)
        if ((obj_index[obj].vnum / 1000) == zone)
            amount++;
    return amount;
}

int
count_mobs_in_zone(int zone)
{
    int amount, mob;
    for (mob = 0, amount = 0; mob < top_of_npc_t; mob++)
        if ((npc_index[mob].vnum / 1000) == zone)
            amount++;
    return amount;
}

void
print_falling_list(char *string, int len)
{
    char temp[256];
    CHAR_DATA *ch;
    struct obj_data *obj;

    strcat(string, "Character falling list\n\r\n\r");
    for (ch = character_list; ch; ch = ch->next)
        if (is_char_falling(ch)) {
            sprintf(temp, "  %s in room %d\n\r", GET_NAME(ch), ch->in_room->number);
            strcat(string, temp);
        };

    strcat(string, "Object falling list\n\r\n\r");
    for (obj = object_list; obj; obj = obj->next)
        if (is_obj_falling(obj)) {
            sprintf(temp, "  %s in room %d\n\r", OSTR(obj, name),
                    obj->in_room ? obj->in_room->number : -1);
            strcat(string, temp);
        };
}

#define UPERCENT(z)  ((count_rooms_in_zone(z)) ?(100 - ((count_urooms_in_zone(z) * 100) / count_rooms_in_zone(z))) : 0)


void
show_rooms_to_player(CHAR_DATA * ch, int start, int end, int reverse, int unused, char *search,
                     int sector, int city_num, int room_flags)
{
    char tmp[256];
    char show_list[MAX_PAGE_STRING];
    struct descriptor_data *tmpdesc;
    struct room_data *room;
    int i = 0;
    int count = 0;

    strcpy(show_list, "");

    if (!reverse) {
        if (start % 1000 != 0 || end != start + 999) {
            sprintf(tmp, "Looking in range %d to %d.\n\r", start, end);
            strcat(show_list, tmp);
        }
    } else {
        if (end % 1000 != 0 || start != end + 999) {
            sprintf(tmp, "Looking in range %d to %d.\n\r", start, end);
            strcat(show_list, tmp);
        }
    }

    if (sector != -1) {
        sprintf(tmp, "Looking for sector %s.\n\r", show_attrib(sector, room_sector));
        strcat(show_list, tmp);
    }

    if (city_num != -1) {
        sprintf(tmp, "Looking for city %s.\n\r", city[city_num].name);
        strcat(show_list, tmp);
    }

    if (room_flags != 0) {
        sprintf(tmp, "Looking for flag(s) %s.\n\r", show_flags(room_flag, room_flags));
        strcat(show_list, tmp);
    }

    if (unused && *search != '\0') {
        sprintf(tmp,
                "Specifying a search phrase with unused is useless, it will" " be ignored.\n\r");
        strcat(show_list, tmp);
    } else if (*search != '\0') {
        sprintf(tmp, "Searching for phrase '%s'.\n\r", search);
        strcat(show_list, tmp);
    }

    for (i = start; reverse ? i >= end : i <= end; reverse ? i-- : i++) {
        room = get_room_num(i);
        if (unused && !room) {
            count++;
            sprintf(tmp, "[%d] [UNUSED]\n\r", i);

            if (strlen(show_list) + strlen(tmp) > sizeof(show_list)) {
                page_string(ch->desc, show_list, 1);
                cprintf(ch, "List too long, truncated.\n\r");
                return;
            }

            string_safe_cat(show_list, tmp, sizeof(show_list));
        } else if (room && !unused) {
            if (*search != '\0') {
                if (!isallnamebracketsok(search, room->name))
                    continue;
            }

            if (city_num != -1 && room->city != city_num)
                continue;

            if (sector != -1 && room->sector_type != sector)
                continue;

            if (room_flags > 0 && !IS_SET(room->room_flags, room_flags))
                continue;

            count++;
            sprintf(tmp, "[%d] %s", room->number, room->name);
            if (room->description) {
                for (tmpdesc = descriptor_list; tmpdesc; tmpdesc = tmpdesc->next)
                    if (tmpdesc->str == &room->description)
                        break;

                if (!tmpdesc) {
                    if (((strlen(room->description) > 4)
                         && (strncmp(room->description, "Empty", 5)))
                        || (nstrlen(bdesc_list[room->zone]
                                   [room->bdesc_index].desc) > 4))
                        strcat(tmp, "\n\r");
                    else
                        strcat(tmp, " *\n\r");
                } else
                    strcat(tmp, " *\n\r");
            } else
                strcat(tmp, "\n\r");

            if (strlen(show_list) + strlen(tmp) > sizeof(show_list)) {
                page_string(ch->desc, show_list, 1);
                cprintf(ch, "List too long, truncated.\n\r");
                return;
            }
            string_safe_cat(show_list, tmp, sizeof(show_list));
        }
    }

    if (count == 0) {
        strcat(show_list, "No matching results found.\n\r");
    }

    page_string(ch->desc, show_list, 1);
}


void show_matching_edescs_to_player (const char *prefix, 
                                  struct extra_descr_data *edescs, 
                                  const char *match, 
                                  char *outbuf, 
                                  size_t bufsize) {
  struct extra_descr_data *edescit;
  *outbuf = '\0';
      int i;

  if (!edescs) {
    return;
  }

  char *start = outbuf;

  for (edescit = edescs; edescit; edescit = edescit->next) {
    if (edescit->keyword) {
      if (start + 80 > outbuf + bufsize) {
        break;
      }

      char *matchtmp;
      matchtmp = strdup(match);

      for (i = 0; i <= strlen(matchtmp); i++)
        matchtmp[i] = UPPER(matchtmp[i]);

      char *keywordtmp;
      keywordtmp = strdup(edescit->keyword);

      for (i = 0; i <= strlen(keywordtmp); i++)
        keywordtmp[i] = UPPER(keywordtmp[i]);

      char *strptr = strstr(keywordtmp, matchtmp);

      if (!strptr) {
        continue;
      }
      
      int printed = sprintf(outbuf, "%s - %s\n\r", prefix, edescit->keyword);
      outbuf += printed;
    }
  }
}


void
show_specs_to_player(const char *prefix, struct specials_t *specs, const char *match, char *outbuf, size_t bufsize)
{
    *outbuf = '\0';
    if (!specs) return;

    char *start = outbuf;
    for (int i = 0; i < specs->count; i++) {
        const char *type;
        const char *pname;

        if (start + 80 > outbuf + bufsize)
            break;

        switch (specs->specials[i].type) {
          case SPECIAL_CODED:
              type = "C";
              pname = specs->specials[i].special.cspec->name;
              break;
          case SPECIAL_DMPL:
              type = "dmpl";
              pname = specs->specials[i].special.name;
              break;
          case SPECIAL_JAVASCRIPT:
              type = "JavaScript";
              pname = specs->specials[i].special.name;
              break;
          default:
              fprintf(stderr, "something terrible happened!");
              abort();
              break;
        }

        if (strcmp(match, pname))
            continue;

        int printed = sprintf(outbuf, "%s - %s (%s)\n\r", prefix, pname, command_name[specs->specials[i].cmd]);
        outbuf += printed;
    }
}

void
show_programs_to_player(struct char_data *ch,   /* ch to show the results to */
                        char prog_name[256])
{                               /* search for this program   *//* Show programs BEGIN */
    char tmp[MAX_STRING_LENGTH * 10];
    char show_list[MAX_STRING_LENGTH * 10];
    int i = 0;
    struct room_data *room;

    strcpy(show_list, "");
    strcpy(tmp, "");

    // ---------- NPCS  --------------

    char prefix[256];
    for (i = 0; i < top_of_npc_t; i++) {
        snprintf(prefix, sizeof(prefix), "[n%d] %s", npc_index[i].vnum, npc_default[i].short_descr);
        show_specs_to_player(prefix, npc_default[i].programs, prog_name, tmp, ARRAY_LENGTH(tmp));
        string_safe_cat(show_list, tmp, sizeof(show_list));
    }

    // ------------ OBJECTS -----------------------
    for (i = 0; i < top_of_obj_t; i++) {
        snprintf(prefix, sizeof(prefix), "[o%d] %s", obj_index[i].vnum, obj_default[i].short_descr);
        show_specs_to_player(prefix, obj_default[i].programs, prog_name, tmp, ARRAY_LENGTH(tmp));
        string_safe_cat(show_list, tmp, sizeof(show_list));
    }

    // --------------------  ROOMS

    for (room = room_list; room; room = room->next) {
        snprintf(prefix, sizeof(prefix), "[r%d] %s", room->number, room->name);
        show_specs_to_player(prefix, room->specials, prog_name, tmp, ARRAY_LENGTH(tmp));
        string_safe_cat(show_list, tmp, sizeof(show_list));
    }

    page_string(ch->desc, show_list, 1);
}



void
show_edescs_to_player(struct char_data *ch,   /* ch to show the results to */
                      char edesc_name[256]    /* search for this edesc */)
{   /* Show edescs BEGIN */
  char tmp[MAX_STRING_LENGTH * 10];
  char show_list[MAX_STRING_LENGTH * 10];
  int i = 0;
  struct room_data *room;
  
  strcpy(show_list, "");
  strcpy(tmp, "");
  
  // ---------- NPCS  --------------
  
  char prefix[256];
  for (i = 0; i < top_of_npc_t; i++) {
    snprintf(prefix, sizeof(prefix), "[n%d] %s", npc_index[i].vnum, npc_default[i].short_descr);
    show_matching_edescs_to_player(prefix, npc_default[i].exdesc, edesc_name, tmp, ARRAY_LENGTH(tmp));
    string_safe_cat(show_list, tmp, sizeof(show_list));
  }
  
  // ------------ OBJECTS -----------------------
  for (i = 0; i < top_of_obj_t; i++) {
    snprintf(prefix, sizeof(prefix), "[o%d] %s", obj_index[i].vnum, obj_default[i].short_descr);
    show_matching_edescs_to_player(prefix, obj_default[i].exdesc, edesc_name, tmp, ARRAY_LENGTH(tmp));
    string_safe_cat(show_list, tmp, sizeof(show_list));
  }
  
  // --------------------  ROOMS
  for (room = room_list; room; room = room->next) {
    snprintf(prefix, sizeof(prefix), "[r%d] %s", room->number, room->name);
    show_matching_edescs_to_player(prefix, room->ex_description, edesc_name, tmp, ARRAY_LENGTH(tmp));
    string_safe_cat(show_list, tmp, sizeof(show_list));
  }
  
  page_string(ch->desc, show_list, 1);
}


void
show_npcs_to_player(CHAR_DATA * ch,     /* ch to show the npcs to */
                    int start,  /* start at this vnum     */
                    int end,    /* end at this vnum       */
                    int reverse,        /* show in reverse?       */
                    int unused)
{                               /* show unused?           */
    //  char debug [256];
    char tmp[MAX_STRING_LENGTH * 10];
    char show_list[MAX_STRING_LENGTH * 10];
    int i = 0, found = FALSE, j = 0, high, low;

    low = MIN(start, end);
    high = MAX(start, end);

    strcpy(show_list, "");

    //  gamelog("show_npcs_to_player");
    //  sprintf(debug, "Start: %d  End: %d  Reverse: %d  Unused %d.", 
    //      start, end, reverse, unused);
    //  gamelog(debug);

    if (unused) {
        for (j = start; reverse ? j >= end : j <= end; reverse ? j-- : j++) {
            found = FALSE;
            for (i = 0; i < top_of_npc_t; i++)
                if (npc_index[i].vnum == j) {
                    found = TRUE;
                    break;
                }

            if (!found) {
                sprintf(tmp, "[%d] [UNUSED]\n\r", j);
                if ((strlen(show_list) + strlen(tmp)) > sizeof(show_list)) {
                    strcpy(show_list, "List too long, be more specific.\n\r");
                    break;
                } else
                    strcat(show_list, tmp);
            }
        }
        page_string(ch->desc, show_list, 1);
        return;
    }

    for (i = 0; i < top_of_npc_t; i++)
        if (npc_index[i].vnum >= low && npc_index[i].vnum <= high) {
            sprintf(tmp, "[%d] %s - %s\n\r", npc_index[i].vnum, npc_default[i].short_descr,
                    npc_default[i].name);

            if ((strlen(tmp) + strlen(show_list)) > sizeof(tmp)) {
                strcpy(tmp, "List too long, be more specific.\n\r");
                break;
            } else if (reverse) {       /* Put new item at start of string */
                strcat(tmp, show_list);
                strcpy(show_list, tmp);
            } else              /* Put new item at end of string */
                strcat(show_list, tmp);
        }
    page_string(ch->desc, show_list, 1);
}

void
show_objects_to_player(CHAR_DATA * ch,  /* ch to show object to */
                       int start,       /* start at thsi vnum   */
                       int end, /* end at this vnum     */
                       int reverse,     /* show in reverse?     */
                       int unused,      /* show unused?         */
                       int type,        /* search for obj type? */
                       int material,    /* search for material? */
                       long wflag,	/* search for wflag?    */
                       long eflag,	/* search for eflag?    */
                       int aff_type)	/* search for aff type? */
{

    //      char debug [256];
    char tmp[MAX_STRING_LENGTH * 10];
    char show_list[MAX_STRING_LENGTH * 10];
    int i = 0, found = FALSE, j = 0, high, low;

    low = MIN(start, end);
    high = MAX(start, end);

    strcpy(show_list, "");

    //   gamelog("show_objects_to_player");
    //    sprintf(debug, "Start: %d  End: %d  Reverse: %d  Unused %d  Type: %d  "
    //              "Material %d  Wflag %d", start, end, reverse, unused, type, material, wflag);
    //   gamelog(debug);

    if (unused) {
        for (j = start; reverse ? j >= end : j <= end; reverse ? j-- : j++) {
            found = FALSE;
            for (i = 0; i < top_of_obj_t; i++)
                if (obj_index[i].vnum == j) {
                    found = TRUE;
                    break;
                }

            if (!found) {
                sprintf(tmp, "[%d] [UNUSED]\n\r", j);
                if ((strlen(show_list) + strlen(tmp)) > sizeof(show_list)) {
                    strcpy(show_list, "List too long, be more specific.\n\r");
                    break;
                } else
                    strcat(show_list, tmp);
            }
        }
        page_string(ch->desc, show_list, 1);
        return;
    }

    for (i = 0; i < top_of_obj_t; i++)
        if (obj_index[i].vnum >= low && obj_index[i].vnum <= high) {
            /* Innocent! */
            found = TRUE;

            /* Until proven Guilty! */
            if ((type != -1) && (obj_default[i].type != type))
                found = FALSE;
            if ((material != -1) && (obj_default[i].material != material))
                found = FALSE;
            if ((wflag != -1) && (!IS_SET(obj_default[i].wear_flags, wflag)))
                found = FALSE;
            if ((eflag != -1) && (!IS_SET(obj_default[i].extra_flags, eflag)))
                found = FALSE;

            if (aff_type != -1) {
		bool afound = FALSE; // Needed since we're iterating a list
		for (j = 0; j < MAX_OBJ_AFFECT; j++) {
                    if (obj_default[i].affected[j].location == aff_type)
			afound = TRUE;
		}
		found = afound; // If any of the affects were true, we're good
	    }
			    
            if (found) {
                sprintf(tmp, "[%d] %s - %s\n\r", obj_index[i].vnum, obj_default[i].short_descr,
                        obj_default[i].name);

                if ((strlen(tmp) + strlen(show_list)) > sizeof(tmp)) {
                    strcpy(tmp, "List too long, be more specific.\n\r");
                    break;
                } else if (reverse) {   /* Put new item at start of string */
                    strcat(tmp, show_list);
                    strcpy(show_list, tmp);
                } else          /* Put new item at end of string */
                    strcat(show_list, tmp);
            }
        }
    page_string(ch->desc, show_list, 1);
}

void
cmd_show(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], buf1[256], buf2[256], buf3[256], tmp[256], show_list[MAX_PAGE_STRING];
    char buf5[256], buf6[256];
    char debug[256];
    char search[MAX_STRING_LENGTH];
    int type, i, start, end;
    /*  int zone = -1; */

    struct room_data *room;
    CHAR_DATA *tmpch, *staff;
    struct obj_data *tmpob;
    struct descriptor_data *tmpdesc;
    struct ban_t *tmpban;
    struct clan_rel_type *clr;

    /* Added June 10, 2000 for the -r and -u flags -Nessalin */
    int reverse = 0;
    int unused = 0;
    int show_zone = 0;
    char buf4[256];

    /* Added March 1, 2003 for the -t flag -Nessalin */
    int type_num = -1;
    int type_num2 = -1;

    /* Added March 2, 2003 for the -m flag -Nessalin */
    int material_num = -1;
    int material_num2 = -1;

    /* Added June 7, 2003 for the -w flag -Nessalin */
    int wflag_num = -1;
    long wflag_num2 = -1;

    /* Added February 11, 2014 for the -e flag -Tiernan */
    int eflag_num = -1;
    long eflag_num2 = -1;

    /* Added for the -s (room sector) flag -Morgenes */
    int sector_num = -1;
    int sector_num2 = -1;

    /* Added for the -c (room city) flag -Morgenes */
    int city_num = -1;

    /* Added for the -f (room flag) flag -Morgenes */
    int room_flag_num = -1;
    int room_flag_num2 = 0;

    /* Added August 25, 2014 for the -a setting - Tiernan */
    int atype = -1;
    int aff_type = -1;

    int n, zone, index;
    char old_arg[MAX_STRING_LENGTH];

    char *clan_type_name[] = {
        "None",
        "Merchant",
        "Noble",
        "Templar",
        "\n"
    };

    /*  removed by nessalin 1/12/97.
     * I'm making it all based on the integer value
     * 
     * char *clan_ally_type_name[] = {
     * "Neutral   ",
     * "Allied to ",
     * "Has vassal",
     * "Enemy of  ",
     * "\n"
     * };  */

    /* default search to nothing */
    search[0] = '\0';

    strcpy(show_list, "");

    staff = SWITCHED_PLAYER(ch);

    argument = one_argument(argument, buf1, sizeof(buf1));
    strcpy(buf2, argument);

    switch (*buf1) {
    case 'r':                  /* rooms */
      type = 1;
        if (!*buf2) {
            send_to_char
                ("show r [-r|-u] [-s sector] [-c city] [-f flag] [Z [N1 [N2]]] [keywords]\n\r", ch);
            send_to_char("Z is a zone number\n\r", ch);
            send_to_char("N1 and N2 are a room numbers within a zone Z\n\r", ch);
            send_to_char("Keywords are words in the name of the room.\n\r", ch);
            send_to_char("\n\r", ch);
            send_to_char("  Examples:\n\r", ch);
            send_to_char("show r 5\n\r", ch);
            send_to_char("   shows all rooms in zone 5\n\r", ch);
            send_to_char("show r -f ash\n\r", ch);
            send_to_char("   shows all rooms in all open zones that have the ash flag\n\r", ch);
            send_to_char("show r -c Tuluk 3\n\r", ch);
            send_to_char("   Shows all rooms in zone 3 set with the city 'Tuluk'.\n\r", ch);
            send_to_char("show r -s desert 5\n\r", ch);
            send_to_char("   Shows all rooms in zone 5 with sector desert.\n\r", ch);
            send_to_char("show r -u 5 \n\r", ch);
            send_to_char("   shows all unused rooms in zone 5\n\r", ch);
            send_to_char("show r -r 5\n\r", ch);
            send_to_char("  shows all rooms in zones 5, in reverse order\n\r", ch);
            send_to_char("show r -u -r 5\n\r", ch);
            send_to_char("   shows all unused rooms in zone 5 in reverse order\n\r", ch);
            send_to_char("show r 5 5100 5200 \n\r", ch);
            send_to_char("   shows all rooms in zone 5 between 5100 and 5200\n\r", ch);
            send_to_char("show r -r 5 5100 5200\n\r", ch);
            send_to_char("   shows all rooms in zone 5 between 5100 and 5200 in reverse order\n\r",
                         ch);
            send_to_char("show r -u 5 5100 5200\n\r", ch);
            send_to_char("   shows all unused rooms in zone 5 between 5100 and 5200\n\r", ch);
            send_to_char("show r -r -u 5 5100 5200\n\r", ch);
            send_to_char
                ("   shows all unused rooms in zone 5 between 5100 and 5200 in reverse order\n\r",
                 ch);
            send_to_char("show r 5 entrance\n\r", ch);
            send_to_char("   Shows all rooms in zone 5 with 'entrance' in the name.\n\r", ch);
            send_to_char("show r -f safe -s cavern 3\n\r", ch);
            send_to_char("   Shows all rooms in zone 3 with the safe flag and sector cavern.\n\r",
                         ch);
            send_to_char("show r 67 wastes between\n\r", ch);
            send_to_char
                ("   Shows all rooms in zone 67 with both 'wastes' and 'between' in the name.\n\r",
                 ch);

            /*      send_to_char ("The second arg must be a zone number, or "
             * "2 room numbers for the range.\n\r", ch); */
            return;
        }
        break;
    case 'o':                  /* objs */
        type = 2;
        if (!*buf2) {

            send_to_char("show o ([-a|-t|-r|-u] Z [N1 N2] | [-e|-w] FLAG Z [N1 N2]) | (Z [A1] [A2])\n\r", ch);
            send_to_char("Z is a zone number\n\r", ch);
            send_to_char("N1 and N2 are a room numbers within a zone Z\n\r", ch);
            send_to_char("A1 and A2 are keywords of items in the zone\n\r", ch);
            send_to_char("\n\r", ch);
            send_to_char("Examples:\n\r", ch);
            send_to_char("show o 5\n\r", ch);
            send_to_char("   shows all objects in zone 5\n\r", ch);
            send_to_char("show o -a AFFECT 5\n\r", ch);
            send_to_char("   shows all objects of a given affect in zone 5\n\r", ch);
            send_to_char("show o -t TYPE 5\n\r", ch);
            send_to_char("   shows all objects of a given type in zone 5\n\r", ch);
            send_to_char("show o -m MATERIAL 5\n\r", ch);
            send_to_char("   shows all objects of a given material in zone 5\n\r", ch);
            send_to_char("show o -u 5 \n\r", ch);
            send_to_char("   shows all unused objects in zone 5\n\r", ch);
            send_to_char("show o -r 5\n\r", ch);
            send_to_char("  shows all objects in zones 5, in reverse order\n\r", ch);
            send_to_char("show o -u -r 5\n\r", ch);
            send_to_char("   shows all unused objects in zone 5 in reverse order\n\r", ch);
            send_to_char("show o -r 5 5100 5200\n\r", ch);
            send_to_char
                ("   shows all objects in zone 5 between 5100 and 5200 in reverse order\n\r", ch);
            send_to_char("show o -u 5 5100 5200\n\r", ch);
            send_to_char("   shows all unused objects in zone 5 between 5100 and 5200\n\r", ch);
            send_to_char("show o -r -u 5 5100 5200\n\r", ch);
            send_to_char
                ("   shows all unused objects in zone 5 between 5100 and 5200 in reverse order\n\r",
                 ch);
            send_to_char("show o 5 hammer \n\r", ch);
            send_to_char("   shows all objects in zone 5 with the keyword hammer\n\r", ch);
            send_to_char("show o 5 hammer stone\n\r", ch);
            send_to_char("   shows all objects in zone 5 with the keywords hammer and stone\n\r",
                         ch);
            send_to_char("show o hammer\n\r", ch);
            send_to_char("   shows all objects in the database with the keyword hammer\n\r", ch);
            send_to_char("show o hammer stone\n\r", ch);
            send_to_char
                ("   shows all objects in the database with the keywords hammer and stone\n\r", ch);
            send_to_char("show o -e magic 5 5100 5200\n\r", ch);
            send_to_char
                ("   shows all objects in zone 5 between 5100 and 5200 with the 'magic' eflag\n\r", ch);
            send_to_char("show o -w about_head 5\n\r", ch);
            send_to_char
                ("   shows all objects in zone 5 with the 'about head' wflag\n\r", ch);

            // send_to_char ("The second arg must be a zone number or an object "
            //    "name.\n\r", ch);
            return;
        }
        break;

        /* *******************        NPCS: START              ************** */
    case 'n':
      type = 3;
      if (!*buf2) {
        
        send_to_char ("show n ([-r|-u] Z [N1 N2]) | (Z [A1] [A2])\n\r", ch);
        send_to_char ("Z is a zone number\n\r", ch);
        send_to_char ("N1 and N2 are a npc numbers within a zone Z\n\r",ch);
        send_to_char ("A1 and A2 are keywords of npcs in the zone\n\r", ch);
        send_to_char ("\n\r", ch);
        send_to_char ("Examples:\n\r", ch);
        send_to_char ("show n 5\n\r", ch);
        send_to_char ("   shows all npcs in zone 5\n\r", ch);
        send_to_char ("show n -u 5 \n\r", ch);
        send_to_char ("   shows all unused npcs in zone 5\n\r", ch);
        send_to_char ("show n -r 5\n\r", ch);
        send_to_char ("  shows all npcs in zones 5, in reverse order\n\r",ch);
        send_to_char ("show n -u -r 5\n\r", ch);
        send_to_char ("   shows all unused npcs in zone 5 in reverse order\n\r", ch);
        send_to_char ("show n -r 5 5100 5200\n\r", ch);
        send_to_char ("   shows all npcs in zone 5 between 5100 and 5200 in reverse order\n\r", ch);
        send_to_char ("show n -u 5 5100 5200\n\r", ch);
        send_to_char ("   shows all unused npcs in zone 5 between 5100 and 5200\n\r", ch);
        send_to_char ("show n -r -u 5 5100 5200\n\r", ch);
        send_to_char ("   shows all unused npcs in zone 5 between 5100 and 5200 in reverse order\n\r",  ch);
        send_to_char ("show n 5 brown \n\r", ch);
        send_to_char   ("   shows all npcs in zone 5 with the keyword brown\n\r", ch);
        send_to_char ("show n 5 brown human\n\r", ch);
        send_to_char  ("   shows all objects in zone 5 with the keywords human and brown\n\r",  ch);
        send_to_char ("show n brown\n\r", ch);
        send_to_char  ("   shows all objects in the database with the keyword brown\n\r", ch);
        send_to_char ("show n brown human\n\r", ch);
        send_to_char  ("   shows all npcs in the database with the keywords human and brown\n\r", ch);
        return;
      }
      break;
        /* *******************        NPCS: END               ************** */


    case 'z':                  /* zone */
        type = 4;
        break;
    case 'u':                  /* Undescribed rooms */
        type = 5;
        if (!*buf2) {
            send_to_char("The second arg must be a zone number.\n\r", ch);
            return;
        }
        break;
    case 'b':                  /* Banned sites */
        type = 6;
        break;
    case 'a':                  /* all zones */
        type = 7;
        break;
    case 'p':                  /* program */
        type = 8;
        if (!*buf2) {
            send_to_char("show p <program name>\n\r", ch);
            send_to_char("   Shows all rooms, objects, and characters in the game with <program name> attached.\n\r", ch);
        }
        break;
    case 'e':                  /* edesc */
      type = 21; 
      if (!*buf2) {
        send_to_char("show e <edesc>\n\r", ch);
        send_to_char("   Shows all rooms, objects, and characters in the game with <edesc>.\n\r", ch);
      }
      break;
    case 'q':                  /* queue */
        type = 9;
        break;
    case 'g':                  /* guilds */
        type = 10;
        if (GET_LEVEL(staff) < STORYTELLER) {
            send_to_char("You are not high enough to view that.\n\r", ch);
            return;
        }
        break;
    case 'l':                  /* list background descriptions */
        type = 12;
        if (!*buf2) {
            send_to_char("The second argument must be a zone number.\n\r", ch);
            return;
        }
        break;
    case 'd':                  /* Background Descriptions */
        type = 13;
        if (!*buf2) {
            send_to_char("The second argument must be a background " "description number.\n\r", ch);
            return;
        }
        break;
    case '#':
        /* Removing because it crashes us and...honestly I don't think
         * anyone uses it. -Nessalin 11/09/02 */
        /* Putting an exception in so Tenebrius can run it, it is to track
         * down database errors with bad pointers and whatnot, the code
         * in theory will take a little bit to run but will find and possibly
         * fix bad things in the database     -Tenebrius 08/23/2003 */

        if (strcmp(GET_NAME(ch), "Tenebrius")) {

            send_to_char("Removed. Buffy.\n\r", ch);
            return;
        } else

            type = 14;

        break;
    case 'f':
        type = 15;
        break;
    case 'c':                  /* clans */
        type = 16;
        break;
    case 'j':                  /* javascripts */
        type = 18;
        break;
    case 's':                  /* subguilds */
        type = 19;
        if (GET_LEVEL(staff) < STORYTELLER) {
            send_to_char("You are not high enough to view that.\n\r", ch);
            return;
        }
        break;
    case 'k':
        type = 20;
        break;
    default:
        send_to_char("Any of the following are valid arguments to show:\n\r", ch);
        send_to_char("  (a)ll zones\n\r  (b)anned sites\n\r  (c)lans,\n\r", ch);
        send_to_char("  background (d)escriptions,\n\r  (f)alling\n\r  (g)uilds,\n\r", ch);
        send_to_char
            ("  background description (l)ist\n\r  (n)PCs\n\r  (o)bjects\n\r  (p)rograms\n\r", ch);
        send_to_char("  (r)ooms\n\r  (u)ndescribed rooms\n\r", ch);
        send_to_char("  (s)ubguilds\n\r", ch);
        send_to_char("  open (z)ones\n\r  (#) db-check\n\r", ch);
        send_to_char("  (e)descs", ch);
        //        send_to_char("  (j)avascript list\n\r", ch);
        return;
    }

    if (atoi(buf2) > top_of_zone_table && type != 13 && type != 16) {
        send_to_char("That zone number is too high.\n\r", ch);
        return;
    }

    switch (type) {
    case 1:                    /* Show Room */
        argument = one_argument(argument, buf2, sizeof(buf2));

        unused = 0;
        reverse = 0;

        while (!(strncmp(buf2, "-", 1))) {
            if (!(strcasecmp(buf2, "-u")))
                unused = 1;
            else if (!(strcasecmp(buf2, "-r")))
                reverse = 1;
	    else if ((!(strcmp(buf2, "-s"))) || (!(strcmp(buf2, "-S")))) {
                argument = one_argument(argument, buf2, sizeof(buf2));

                if (!*buf2) {
                    send_to_char("You must include a sector after" " the -s flag. Try:\n\r", ch);
                    send_attribs_to_char(room_sector, ch);
                    return;
                }

                sector_num = get_attrib_num(room_sector, buf2);

                if (sector_num == -1) {
                    send_to_char("That sector type does not exist. Try:\n\r", ch);
                    send_attribs_to_char(room_sector, ch);
                    return;
                }

                sector_num2 = room_sector[sector_num].val;
            } else if ((!(strcmp(buf2, "-c"))) || (!(strcmp(buf2, "-C")))) {
                argument = one_argument(argument, buf2, sizeof(buf2));

                if (!*buf2) {
                    send_to_char("You must include a city after"
                                 " the -c flag. Valid cities are:\n\r", ch);
                    cmd_citystat(ch, "", CMD_CITYSTAT, 0);
                    return;
                }

                for (city_num = 0; city_num < MAX_CITIES; city_num++)
                    if (!str_prefix(buf2, city[city_num].name))
                        break;

                if (city_num == MAX_CITIES) {
                    send_to_char("That city does not exist. Valid cities are:\n\r", ch);
                    cmd_citystat(ch, "", CMD_CITYSTAT, 0);
                    return;
                }
            } else if ((!(strcmp(buf2, "-f"))) || (!(strcmp(buf2, "-F")))) {
                argument = one_argument(argument, buf2, sizeof(buf2));

                if (!*buf2) {
                    send_to_char("You must include a flag after" " the -f flag. Try:\n\r", ch);
                    send_flags_to_char(room_flag, ch);
                    return;
                }

                room_flag_num = get_flag_num(room_flag, buf2);

                if (room_flag_num == -1) {
                    send_to_char("That room flag does not exist. Try:\n\r", ch);
                    send_flags_to_char(room_flag, ch);
                    return;
                }

                MUD_SET_BIT(room_flag_num2, room_flag[room_flag_num].bit);
            } else
                cprintf(ch, "Ignoring unrecognized flag '%s'.\n\r", buf2);
            argument = one_argument(argument, buf2, sizeof(buf2));
        }

        if (!(strncmp(buf2, "-", 1)))
            argument = one_argument(argument, buf2, sizeof(buf2));

        if (buf2[0] != '\0' && is_number(buf2)) {
            show_zone = atoi(buf2);

            if ((show_zone) > top_of_zone_table && type != 13) {
                send_to_char("That zone number is too high.\n\r", ch);
                return;
            }

            if (!zone_table[show_zone].is_booted) {
                send_to_char("That zone is not open!\n\r", ch);
                return;
            }

            if (reverse) {
                end = show_zone * 1000;
                start = end + 999;
            } else {
                start = show_zone * 1000;
                end = start + 999;
            }

            argument = one_argument(argument, buf3, sizeof(buf3));
            argument = one_argument(argument, buf4, sizeof(buf4));

            if (*buf3 && is_number(buf3)) {
                if (reverse)
                    start = MAX(show_zone * 1000, MIN((show_zone * 1000 + 999), atoi(buf3)));
                else
                    start = MAX(show_zone * 1000, MAX((show_zone * 1000), atoi(buf3)));

                if (*buf4 && is_number(buf4)) {
                    if (reverse)
                        end = MAX(show_zone * 1000, MAX((show_zone * 1000), atoi(buf4)));
                    else
                        end = MAX(show_zone * 1000, MIN((show_zone * 1000 + 999), atoi(buf4)));
                } else if (*buf4) {     /* non-number argument */
                    strcat(search, buf4);
                }

                /* if they wanted reverse order and start < end, reverse them */
                if (reverse && start < end) {
                    int t = start;
                    start = end;
                    end = t;
                }
                /* normal order and end < start, reverse them */
                else if (!reverse && end < start) {
                    int t = start;
                    start = end;
                    end = t;
                }
            } else if (*buf3) { /* non-number arguments */
                strcat(search, buf3);
                if (*buf4) {
                    strcat(search, " ");
                    strcat(search, buf4);
                }
            }
        } else {
            strcat(search, buf2);
            if (!reverse) {
                start = 0;
                end = top_of_zone_table * 1000 + 999;
            } else {
                end = 0;
                start = top_of_zone_table * 1000 + 999;
            }
        }

        argument = one_argument(argument, buf3, sizeof(buf3));

        while (*buf3) {
            if (*search)
                strcat(search, " ");
            strcat(search, buf3);

            argument = one_argument(argument, buf3, sizeof(buf3));
        }

        show_rooms_to_player(ch, start, end, reverse, unused, search, sector_num2, city_num,
                             room_flag_num2);

        return;

    case 2:                    /* Objects */
        /* Init the variables */
        *buf3 = '\0';
        *buf4 = '\0';
        *debug = '\0';
        unused = 0;
        reverse = 0;

        argument = one_argument(argument, buf2, sizeof(buf2));

        while (!(strncmp(buf2, "-", 1))) {
            if ((!(strcmp(buf2, "-u"))) || (!(strcmp(buf2, "-U"))))
                unused = 1;
            else if ((!(strcmp(buf2, "-r"))) || (!(strcmp(buf2, "-R"))))
                reverse = 1;
            else if ((!(strcmp(buf2, "-m"))) || (!(strcmp(buf2, "-M")))) {
                argument = one_argument(argument, buf2, sizeof(buf2));

                if (!*buf2) {
                    send_to_char("You must include a material after" " the -m flag. Try:\n\r", ch);
                    send_attribs_to_char(obj_material, ch);
                    return;
                }

                material_num = get_attrib_num(obj_material, buf2);

                if (material_num == -1) {
                    send_to_char("That material type does not exist. Try:\n\r", ch);
                    send_attribs_to_char(obj_material, ch);
                    return;
                }

                material_num2 = obj_material[material_num].val;
            }

	    /* search on affect type */
            else if ((!(strcmp(buf2, "-a"))) || (!(strcmp(buf2, "-A")))) {
                argument = one_argument(argument, buf2, sizeof(buf2));

                if (!*buf2) {
                    send_to_char("You must include an affect type after" " the -a flag. Try:\n\r", ch);
		    send_attribs_to_char(obj_apply_types, ch);
                    return;
                }

                atype = get_attrib_num(obj_apply_types, buf2);

                if (atype  == -1) {
                    send_to_char("That affect type does not exist. Try:\n\r", ch);
		    send_attribs_to_char(obj_apply_types, ch);
                    return;
                }

		aff_type = obj_apply_types[atype].val;
	    }

            /* Search on item type */
            else if ((!(strcmp(buf2, "-t"))) || (!(strcmp(buf2, "-T")))) {
                argument = one_argument(argument, buf2, sizeof(buf2));
                if (!*buf2) {
                    send_to_char("You must include an object type after" " the -t flag.\n\r", ch);
                    return;
                }

                type_num = get_attrib_num(obj_type, buf2);
                if (type_num == -1) {
                    send_to_char("That object type is invalid.\n\r", ch);
                    send_to_char("The following types are available:\n\r", ch);
                    send_attribs_to_char(obj_type, ch);
                    return;
                }
                type_num2 = obj_type[type_num].val;
                if ((type_num == -1) || obj_type[type_num].priv > GET_LEVEL(staff)) {
                    send_to_char("The following types are available:\n\r", ch);
                    send_attribs_to_char(obj_type, ch);
                    return;
                }
            }

            /* Search on wear flag  */
            else if ((!(strcmp(buf2, "-w"))) || (!(strcmp(buf2, "-W")))) {
                argument = one_argument(argument, buf2, sizeof(buf2));
                if (!*buf2) {
                    send_to_char("You must include a wear flag after" " the -w flag.\n\r", ch);
                    return;
                }

                wflag_num = get_flag_num(obj_wear_flag, buf2);

                if (wflag_num == -1) {
                    send_to_char("That wear flag is invalid.\n\r", ch);
                    send_to_char("The following flags are available:\n\r", ch);
                    send_flags_to_char(obj_wear_flag, ch);
                    return;
                }

                wflag_num2 = obj_wear_flag[wflag_num].bit;

                if ((wflag_num == -1) || obj_wear_flag[wflag_num].priv > GET_LEVEL(staff)) {
                    send_to_char("The following flags are available:\n\r", ch);
                    send_flags_to_char(obj_wear_flag, ch);
                    return;

                }
                cprintf(ch, "Searching wear flag '%s'.\n\r", buf2);
            /* Search on extra flag  */
	    } else if ((!(strcmp(buf2, "-e"))) || (!(strcmp(buf2, "-E")))) {
                argument = one_argument(argument, buf2, sizeof(buf2));
                if (!*buf2) {
                    send_to_char("You must include an extra flag after" " the -e flag.\n\r", ch);
                    return;
                }

                eflag_num = get_flag_num(obj_extra_flag, buf2);

                if (eflag_num == -1) {
                    send_to_char("That extra flag is invalid.\n\r", ch);
                    send_to_char("The following flags are available:\n\r", ch);
                    send_flags_to_char(obj_extra_flag, ch);
                    return;
                }

                eflag_num2 = obj_extra_flag[eflag_num].bit;

                if ((eflag_num == -1) || obj_extra_flag[eflag_num].priv > GET_LEVEL(staff)) {
                    send_to_char("The following flags are available:\n\r", ch);
                    send_flags_to_char(obj_extra_flag, ch);
                    return;
                }
                cprintf(ch, "Searching extra flag '%s'.\n\r", buf2);
            } else
                send_to_char("Ignoring unrecognized flag.\n\r", ch);
            argument = one_argument(argument, buf2, sizeof(buf2));
        }

        argument = one_argument(argument, buf3, sizeof(buf3));
        argument = one_argument(argument, buf4, sizeof(buf4));
        argument = one_argument(argument, buf5, sizeof(buf5));
        argument = one_argument(argument, buf6, sizeof(buf6));

        if ((((*buf3) && (*buf4)) && (isdigit(*buf3) && isdigit(*buf4)))
	    || (wflag_num2 != -1) || (eflag_num2 != -1)
            || (reverse) || (unused) || (type_num2 != -1)
	    || (material_num2 != -1) || (aff_type != -1)) {

            show_zone = atoi(buf2);

            if ((show_zone) > top_of_zone_table && type != 13) {
                send_to_char("That zone number is too high.\n\r", ch);
                return;
            }

            if (reverse) {
                end = show_zone * 1000;
                start = end + 999;
            } else {
                start = show_zone * 1000;
                end = start + 999;
            }

	    if (((wflag_num2 != -1) || (eflag_num2 != -1) || aff_type != -1)
		    && (!*buf3)) {
                if (isdigit(*buf2)) {
                    start = show_zone * 1000;
                    end = start + 999;
		} else {
                    start = 0;
                    end = top_of_zone_table * 1000 + 999;
		}
	    }

            if ((*buf3) && (*buf4)) {
                if (reverse) {
                    start =
                        MAX(show_zone * 1000,
                            MIN((show_zone * 1000 + 999), MAX(atoi(buf3), atoi(buf4))));
                    end =
                        MAX(show_zone * 1000, MAX((show_zone * 1000), MIN(atoi(buf3), atoi(buf4))));
                } else {
                    start =
                        MAX(show_zone * 1000, MAX((show_zone * 1000), MIN(atoi(buf3), atoi(buf4))));
                    end =
                        MAX(show_zone * 1000,
                            MIN((show_zone * 1000 + 999), MAX(atoi(buf3), atoi(buf4))));
                }
            }

            show_objects_to_player(ch, start, end, reverse, unused, type_num2, material_num2,
                                   wflag_num2, eflag_num2, aff_type);
            return;
        } else {
            int zone = -1;

            /* watch to see if they specified a zone */
            if (isdigit(*buf2)) {
                zone = atoi(buf2);
                argument = one_argument(argument, buf2, sizeof(buf2));  /* grab off the number */
                strcpy(buf2, argument);
            }

            strcat(buf2, " ");
            strcat(buf2, buf3);
            strcat(buf2, " ");
            strcat(buf2, buf4);
            strcat(buf2, " ");
            strcat(buf2, buf5);
            strcat(buf2, " ");
            strcat(buf2, buf6);
            strcat(buf2, " ");

            for (i = 0; i < top_of_obj_t; i++) {
                if ((buf2[0] == '\0'
                     || (buf2[0] != '\0' && isallnamebracketsok(buf2, obj_default[i].name)))
                    && (zone == -1 || (obj_index[i].vnum / 1000) == zone)) {
                    sprintf(tmp, "[%d] %s - %s\n\r", obj_index[i].vnum, obj_default[i].short_descr,
                            obj_default[i].name);
                    if ((strlen(show_list) + strlen(tmp)) > sizeof(show_list)) {
                        strcpy(show_list, "List too long, be more specific.\n\r");
                        break;
                    } else
                        strcat(show_list, tmp);
                }
            }
        }
        break;

    case 3:                    /* NPCs */
        reverse = 0;
        unused = 0;

        argument = one_argument(argument, buf2, sizeof(buf2));

        while (!(strncmp(buf2, "-", 1))) {
            if ((!(strcmp(buf2, "-u"))) || (!(strcmp(buf2, "-U"))))
                unused = 1;
            else if ((!(strcmp(buf2, "-r"))) || (!(strcmp(buf2, "-R"))))
                reverse = 1;
            else
                send_to_char("Ignoring unrecognized flag.\n\r", ch);
            argument = one_argument(argument, buf2, sizeof(buf2));
        }

        argument = one_argument(argument, buf3, sizeof(buf3));
        argument = one_argument(argument, buf4, sizeof(buf4));

        if ((((*buf3) && (*buf4)) && (isdigit(*buf3) && isdigit(*buf4))) || (reverse) || (unused)) {

            show_zone = atoi(buf2);

            if ((show_zone) > top_of_zone_table && type != 13) {
                send_to_char("That zone number is too high.\n\r", ch);
                return;
            }

            if (reverse) {
                end = show_zone * 1000;
                start = end + 999;
            } else {
                start = show_zone * 1000;
                end = start + 999;
            }


            if ((*buf3) && (*buf4)) {
                if (reverse) {
                    start =
                        MAX(show_zone * 1000,
                            MIN((show_zone * 1000 + 999), MAX(atoi(buf3), atoi(buf4))));
                    end =
                        MAX(show_zone * 1000, MAX((show_zone * 1000), MIN(atoi(buf3), atoi(buf4))));
                } else {
                    start =
                        MAX(show_zone * 1000, MAX((show_zone * 1000), MIN(atoi(buf3), atoi(buf4))));

                    end =
                        MAX(show_zone * 1000,
                            MIN((show_zone * 1000 + 999), MAX(atoi(buf3), atoi(buf4))));
                }
            }

            show_npcs_to_player(ch, start, end, reverse, unused);
            return;
        }

        if (isdigit(*buf2)) {
            show_zone = atoi(buf2);

            if ((show_zone) > top_of_zone_table && type != 13) {
                send_to_char("That zone number is too high.\n\r", ch);
                return;
            }

            strcpy(buf2, argument);

            if (*buf3) {
                strcat(buf2, " ");
                strcat(buf2, buf3);
            }
            if (*buf4) {
                strcat(buf2, " ");
                strcat(buf2, buf4);
            }


            for (i = 0; i < top_of_npc_t; i++) {
                if ((npc_index[i].vnum / 1000) == show_zone) {
                    if (buf2[0] == '\0'
                        || (buf2[0] != '\0' && isallnamebracketsok(buf2, npc_default[i].name))) {
                        sprintf(tmp, "[%d] %s - %s\n\r", npc_index[i].vnum,
                                npc_default[i].short_descr, npc_default[i].name);
                        if ((strlen(show_list) + strlen(tmp)) > sizeof(show_list)) {
                            strcpy(show_list, "List too long, be more specific.\n\r");
                            break;
                        } else
                            strcat(show_list, tmp);
                    }
                }
            }
        } else {                /* Searching by name */

            strcat(buf2, " ");
            strcat(buf2, buf3);
            strcat(buf2, " ");
            strcat(buf2, buf4);
            strcat(buf2, " ");
            strcat(buf2, argument);


            for (i = 0; i < top_of_npc_t; i++) {
                if (isallnamebracketsok(buf2, npc_default[i].name)) {
                    sprintf(tmp, "[%d] %s - %s\n\r", npc_index[i].vnum, npc_default[i].short_descr,
                            npc_default[i].name);
                    if ((strlen(show_list) + strlen(tmp)) > sizeof(show_list)) {
                        strcpy(show_list, "List too long, be more specific.\n\r");
                        break;
                    } else
                        strcat(show_list, tmp);
                }
            }
        }
        break;
    case 4:                    /* here */
        if (!*buf2) {
            for (i = 0; i <= top_of_zone_table; i++) {
                if (zone_table[i].is_booted) {
                    sprintf(tmp, "(%2d) %s [%d, %d, %d]\n\r", i, zone_table[i].name,
                            count_rooms_in_zone(i), count_objs_in_zone(i), count_mobs_in_zone(i));
                    strcat(show_list, tmp);
                }
            }
            sprintf(tmp, "\n\rThere are %d rooms, %d objects, and %d npcs in the game.\n\r",
                    count_rooms(), top_of_obj_t, top_of_npc_t);
            strcat(show_list, tmp);
        } else {
            int zone = atoi(buf2);

            if (zone > top_of_zone_table) {
                send_to_char("There is no zone of that number.\n\r", ch);
                return;
            }

            sprintf(tmp, "Zone %d: %s\n\rAge: [%d minutes], Time: [%d minutes]," " Mode: [%d]\n\r",
                    zone, zone_table[zone].name, zone_table[zone].age, zone_table[zone].lifespan,
                    zone_table[zone].reset_mode);

            strcat(show_list, tmp);

            sprintf(tmp,
                    "Total rooms: [%d], Total objs: [%d], Total npcs: [%d]\n\r%d%%"
                    " of the rooms in this zone are described\n\r\n\rZone file" " commands:\n\r",
                    count_rooms_in_zone(zone), count_objs_in_zone(zone), count_mobs_in_zone(zone),
                    UPERCENT(zone));

            strcat(show_list, tmp);


#ifdef ZONE_SHOW
            lastmob = -1;
            for (i = 0; (zone_table[atoi(buf2)].cmd[i].command != 'S')
                 && (zone_table[atoi(buf2)].cmd[i].command); i++) {
                switch (zone_table[atoi(buf2)].cmd[i].command) {
                case 'M':
                    tmpch = read_mobile(zone_table[atoi(buf2)].cmd[i].arg1, REAL);
                    lastmob = -1;
                    if (tmpch) {
                        sprintf(tmp, "[M] Load %s to room %d.\n\r", MSTR(tmpch, short_descr),
                                zone_table[atoi(buf2)].cmd[i].arg3);
                        lastmob = tmpch->nr;
                        extract_char(tmpch);
                        strcat(show_list, tmp);
                    }
                    break;
                case 'O':
                    tmpob = read_object(zone_table[atoi(buf2)].cmd[i].arg1, REAL);
                    if (tmpob) {
                        sprintf(tmp, "[O] Load %s to room %d.\n\r", OSTR(tmpob, short_descr),
                                zone_table[atoi(buf2)].cmd[i].arg3);
                        extract_obj(tmpob);
                        strcat(show_list, tmp);
                    }
                    break;
                case 'P':
                    tmpob = read_object(zone_table[atoi(buf2)].cmd[i].arg1, REAL);
                    if (tmpob) {
                        in_obj = read_object(zone_table[atoi(buf2)].cmd[i].arg3, REAL);
                        if (in_obj) {
                            sprintf(tmp, "[P] Put %s in %s.\n\r", OSTR(tmpob, short_descr),
                                    OSTR(in_obj, short_descr));
                            strcat(show_list, tmp);
                            extract_obj(in_obj);
                        }
                        extract_obj(tmpob);
                    }
                    break;
                case 'G':
                    tmpob = read_object(zone_table[atoi(buf2)].cmd[i].arg1, REAL);
                    if (tmpob && (lastmob != -1)) {
                        tmpch = read_mobile(lastmob, REAL);
                        if (tmpch) {
                            sprintf(tmp, "[G] Give %s to %s.\n\r", OSTR(tmpob, short_descr),
                                    MSTR(tmpch, short_descr));
                            extract_char(tmpch);
                        }
                        extract_obj(tmpob);
                        strcat(show_list, tmp);
                    }
                    break;
                case 'E':
                    tmpob = read_object(zone_table[atoi(buf2)].cmd[i].arg1, REAL);
                    if (tmpob && (lastmob != -1)) {
                        tmpch = read_mobile(lastmob, REAL);
                        if (tmpch) {
                            sprintf(tmp, "[E] Make %s equip %s.\n\r", MSTR(tmpch, short_descr),
                                    OSTR(tmpob, short_descr));
                            extract_char(tmpch);
                        }
                        extract_obj(tmpob);
                        strcat(show_list, tmp);
                    }
                    break;
                case 'D':
                    if (zone_table[atoi(buf2)].cmd[i].arg3 == 0)
                        sprintf(tmp, "[D] Open and unlock door leading %s from room %d.\n\r",
                                dirs[zone_table[atoi(buf2)].cmd[i].arg2],
                                zone_table[atoi(buf2)].cmd[i].arg1);
                    else if (zone_table[atoi(buf2)].cmd[i].arg3 == 1)
                        sprintf(tmp, "[D] Close and unlock door leading %s from room %d.\n\r",
                                dirs[zone_table[atoi(buf2)].cmd[i].arg2],
                                zone_table[atoi(buf2)].cmd[i].arg1);
                    else if (zone_table[atoi(buf2)].cmd[i].arg3 == 2)
                        sprintf(tmp, "[D] Close and lock door leading %s from room %d.\n\r",
                                dirs[zone_table[atoi(buf2)].cmd[i].arg2],
                                zone_table[atoi(buf2)].cmd[i].arg1);
                    strcat(show_list, tmp);
                    break;
                default:
                    break;
                }
            }
#endif
        }
        break;
    case 5:
        if ((*buf2) && (atoi(buf2) >= 0) && (atoi(buf2) <= top_of_zone_table)) {
            for (room = zone_table[atoi(buf2)].rooms; room; room = room->next_in_zone) {
                for (tmpdesc = descriptor_list; tmpdesc; tmpdesc = tmpdesc->next)
                    if (tmpdesc->str == &room->description)
                        break;

                if (!tmpdesc) {
                    if ((room->name) && (room->description))
                        if (!(strncmp(room->description, "Empty", 5))) {
                            sprintf(tmp, "[%d] %s\n\r", room->number, room->name);
                            send_to_char(tmp, ch);
                            /*                    strcat(show_list, tmp); */
                        }
                }
            }
        }
        break;
    case 6:
        strcat(show_list, "Banned Sites\n\r------------\n\r");
        for (tmpban = ban_list; tmpban; tmpban = tmpban->next) {
            if (*tmpban->name) {
                sprintf(tmp, "[%s]\n\r", tmpban->name);
                strcat(show_list, tmp);
            }
        }
        break;
    case 7:
        for (i = 0; i <= top_of_zone_table; i++) {
            sprintf(tmp, "%s(%2d) %s [%d, %d, %d]\n\r", zone_table[i].is_booted ? "* " : "  ", i,
                    zone_table[i].name, count_rooms_in_zone(i), count_objs_in_zone(i),
                    count_mobs_in_zone(i));
            strcat(show_list, tmp);
        }
        sprintf(tmp, "\n\rThere are %d objects and %d npcs in the game.\n\r", top_of_obj_t,
                top_of_npc_t);
        strcat(show_list, tmp);
        break;
    case 8:
        strcpy(show_list, "List of programs\n");
        show_programs_to_player(ch, buf2);
        return;
        //      write_programs_to_buffer(show_list);
    case 9:
        if (!*buf2) {
            strcpy(show_list, "You must specify a char or obj in " "the room, or 'room'.\n\r");
        } else if (!strcmp(buf2, "room")) {
            heap_print(show_list, ch->in_room, event_room_cmp);
        } else {
            if ((tmpch = get_char_room_vis(ch, buf2)))
                heap_print(show_list, tmpch, event_ch_tar_ch_cmp);
            else if ((tmpob = get_obj_in_list_vis(ch, buf2, ch->in_room->contents)))
                heap_print(show_list, tmpob, event_obj_cmp);
            else
                strcpy(show_list, "Could not find that character or object" "in this room\n\r");
        }
        break;
    case 10:
        strcpy(show_list, "");
        if (!*buf2) {
            strcat(show_list, "You can list the following Guilds\n\r");
            for (i = 1; i < MAX_GUILDS; i++) {
                strcat(show_list, guild[i].name);

                strcat(show_list, "\n\r");
            }
        } else {
            for (i = 1; i < MAX_GUILDS; i++)
                if (!str_prefix(buf2, guild[i].name))
                    break;
            strcpy(show_list, "");
            if (i == MAX_GUILDS)
                strcpy(show_list, "That is not a guild_name\n\r");
            else {
                sprintf(show_list, "List for %s\n\r", guild[i].name);

                cat_skills_prev_to_string(i, 0, SPELL_NONE, show_list);
            }
        }
        break;
    case 12:
        zone = atoi(buf2);
        if ((zone < 1) || (zone > 80) || !zone_table[zone].is_booted) {
            send_to_char("You can only show open zones.\n\r", ch);
            return;
        }

        for (index = 0; index < MAX_BDESC; index++) {
            sprintf(buf, "[%d] ", (MAX_BDESC * zone) + index);
            strcat(show_list, buf);
            strcat(show_list, bdesc_list[zone][index].name);
            if (!strcmp(bdesc_list[zone][index].desc, ""))
                strcat(show_list, "*");
            strcat(show_list, "\n\r");
        }
        break;
    case 13:
        n = atoi(buf2);
        zone = n / MAX_BDESC;
        index = n % MAX_BDESC;

        if ((zone < 1) || (zone > 80) || !zone_table[zone].is_booted) {
            send_to_char("That zone is closed.\n\r", ch);
            return;
        }

        sprintf(buf, "%s {%d}\n\r", bdesc_list[zone][index].name, n);
        send_to_char(buf, ch);
        send_to_char(bdesc_list[zone][index].desc, ch);

        return;
    case 14:
        check_db();
        break;
    case 15:
        strcpy(show_list, "");
        print_falling_list(show_list, MAX_STRING_LENGTH);
        break;
    case 16:
        strcpy(show_list, "");

        strcpy(old_arg, buf2);
        one_argument(buf2, tmp, sizeof(tmp));

        if (tmp[0] == '\0') {
            for (start = 0; start < MAX_CLAN; start++) {
#ifndef TIERNAN_CLANLIST
                sprintf(tmp, "[%2d] %s \n\r", start, clan_table[start].name);
#else
                /* Old one, removed by nessalin 1/12/97
                 * sprintf(tmp, "%s (%d)\n\r    Type: %s, Status: %d, Store: %d, Liege: %d\n\r",
                 */

                sprintf(tmp, "%s (%d)\n\r  Type: %s, Store: %d, Liege: %d\n\r",
                        clan_table[start].name, start, clan_type_name[clan_table[start].type],
                        /* Ness 1/12/97
                         * clan_table[start].status, */
                        clan_table[start].store, clan_table[start].liege);
#endif
                strcat(show_list, tmp);
                tmp[0] = '\0';
            }
            page_string(ch->desc, show_list, 1);
            return;
        } else {
            if (!is_number(tmp)) {
                start = lookup_clan_by_name( old_arg );

                if (start == -1) {
                    send_to_char("No such clan.\n\r", ch);
                    return;
                }
            } else {
                start = atoi(tmp);
                if ((start < 0) || (start >= MAX_CLAN)) {
                    send_to_char("That clan number is out of range.\n\r", ch);
                    return;
                }
            }

            sprintf(tmp, "%s (%d)\n\r  Type: %s, Store: %d, Liege: %s(%d), In Bank: %ld\n\r",
                    clan_table[start].name, start,
                    clan_type_name[clan_table[start].type], clan_table[start].store,
                    clan_table[clan_table[start].liege].name, clan_table[start].liege,
                    clan_table[start].account);
            strcat(show_list, tmp);

            for (clr = clan_table[start].relations; clr; clr = clr->next) {
                /* removed 1/12/97 by nessalin
                 * sprintf(tmp, "    %s : %d\n\r",
                 * clan_ally_type_name[clr->type], clr->nr); */

                sprintf(tmp, "--%s(%d) : %d\n\r", clan_table[clr->nr].name, clr->nr, clr->rel_val);

                strcat(show_list, tmp);
            }

            page_string(ch->desc, show_list, 1);

            return;
        }
        break;

    case 17:                   /* smells */
        show_list[0] = '\0';
#ifdef SMELLS
        for (i = 0; smells_table[i].name[0] != '\0'; i++) {
            if (buf2[0] == '\0' || !str_prefix(buf2, smells_table[i].name)) {
                sprintf(tmp, " %15s - %15s\n\r", smells_table[i].name, smells_table[i].description);
                strcat(show_list, tmp);
            }
        }
#else
        sprintf(show_list, "Smells disabled\n\r");
#endif
        page_string(ch->desc, show_list, 1);
        return;

    case 18:                   /* javascripts */
        strncpy(show_list, "JavaScripts temporarily disabled\n\r", sizeof(show_list));
        page_string(ch->desc, show_list, 1);
        return;

    case 19:
        strcpy(show_list, "");
        if (!*buf2) {
            strcat(show_list, "You can list the following Sub Guilds\n\r");
            for (i = 1; i < MAX_SUB_GUILDS; i++) {
                strcat(show_list, sub_guild[i].name);

                strcat(show_list, "\n\r");
            }
        } else {
            for (i = 1; i < MAX_SUB_GUILDS; i++) {
                if (!str_prefix(buf2, sub_guild[i].name))
                    break;
            }
            strcpy(show_list, "");
            if (i == MAX_SUB_GUILDS)
                strcpy(show_list, "That is not a sub_guild_name\n\r");
            else {
                sprintf(show_list, "List for Subguild: %s\n\r", sub_guild[i].name);

                cat_subskills_prev_to_string(i, 0, SPELL_NONE, show_list);
            }
        }
        break;
    case 20:
        strcpy(show_list, "");
        if (!*buf2) {
            strcat(show_list, "You must specify a skill to show which guilds get it.\n\r");
        } else {
            int sk;
            for (sk = 1; sk < MAX_SKILLS; sk++) {
                if (!str_prefix(buf2, skill_name[sk]))
                    break;
            }

            if (sk == MAX_SKILLS) {
                cprintf(ch, "Unable to find skill '%s'.\n\r", buf2);
                return;
            }

            sprintf(tmp, "Showing (sub)guilds that get the skill '%s':\n\r", skill_name[sk]);
            string_safe_cat(show_list, tmp, sizeof(show_list));

            for (i = 1; i < MAX_GUILDS; i++) {
                if (guild[i].skill_max_learned[sk]) {
                    if (skill[sk].sk_class == CLASS_MAGICK) {
                        sprintf(tmp,
                                "%-15s (st=%d, max=%d, min_mana=%d, go on=%d) - branches from %s\n\r",
                                guild[i].name, guild[i].skill_percent[sk],
                                guild[i].skill_max_learned[sk], 
                                mana_amount_level(POWER_MON, POWER_WEK + 1, sk),
                                guild[i].skill_max_learned[sk] - 10,
                                skill_name[guild[i].skill_prev[sk]]);
                    } else {
                        sprintf(tmp, "%-15s (st=%d, max=%d, go on=%d) - branches from %s\n\r",
                                guild[i].name, guild[i].skill_percent[sk],
                                guild[i].skill_max_learned[sk], guild[i].skill_max_learned[sk] - 10,
                                skill_name[guild[i].skill_prev[sk]]);
                    }

                    string_safe_cat(show_list, tmp, sizeof(show_list));
                }
            }
            for (i = 1; i < MAX_SUB_GUILDS; i++) {
                if (sub_guild[i].skill_max_learned[sk]) {
                    if (skill[sk].sk_class == CLASS_MAGICK) {
                        sprintf(tmp,
                                "%-15s (st=%d, max=%d, min_mana=%d, go on=%d) - branches from %s\n\r",
                                sub_guild[i].name, sub_guild[i].skill_percent[sk],
                                sub_guild[i].skill_max_learned[sk], 
                                mana_amount_level(POWER_MON, POWER_WEK + 1, sk),
                                sub_guild[i].skill_max_learned[sk] - 10,
                                skill_name[sub_guild[i].skill_prev[sk]]);
                    } else {
                        sprintf(tmp, "%-15s (st=%d, max=%d, go on=%d) - branches from %s\n\r",
                                sub_guild[i].name, sub_guild[i].skill_percent[sk],
                                sub_guild[i].skill_max_learned[sk],
                                sub_guild[i].skill_max_learned[sk] - 10,
                                skill_name[sub_guild[i].skill_prev[sk]]);
                    }

                    string_safe_cat(show_list, tmp, sizeof(show_list));
                }
            }
            page_string(ch->desc, show_list, 1);

            return;
        }
        break;
    case 21:
      if (!*buf2) {
        return;
      }

      strcpy(show_list, "List of edescs:\n");
      show_edescs_to_player(ch, buf2);
      return;
    }

    page_string(ch->desc, show_list, 1);
    return;
}

void
cmd_nuke(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[256], buf[512];
    CHAR_DATA *victim;          /* The soon-to-be nuked */

    argument = one_argument(argument, name, sizeof(name));

    victim = get_char_vis(ch, name);
    if (!victim) {
        send_to_char("Try nuking someone who's actually around.\n\r", ch);
        return;
    }

    if (IS_IMMORTAL(victim) || IS_NPC(victim)) {
        send_to_char("Quit horsing around, will ya?\n\r", ch);
        return;
    }

    send_to_char("Nuked.\n\r", ch);

    act("BOOM.  A bolt of lightning streaks out of the sky and hits you.", FALSE, victim, 0, 0,
        TO_CHAR);
    act("A bolt of lightning comes down from the sky and hits $n.", FALSE, victim, 0, 0, TO_ROOM);

    if (victim->short_descr)
        free(victim->short_descr);
    if (victim->long_descr)
        free(victim->long_descr);
    if (victim->description)
        free(victim->description);

    GET_RACE(victim) = RACE_RODENT;
    strcpy(buf, "the small white rat");
    victim->short_descr = strdup(buf);
    strcpy(buf, "scurries here.");
    victim->long_descr = strdup(buf);
    sprintf(buf, "This is what remains of %s.\n\r", GET_NAME(victim));
    victim->description = strdup(buf);

    reset_char_skills(victim);
}

void
cmd_log(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];

    sprintf(buf, "SYSLOG msg from %s: %s", GET_NAME(ch), argument);
    gamelog(buf);
}

void
cmd_plist(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char *list(char *filename);

    return;

}

/* Begin Azroen's attempt at a criminal command */
void
cmd_criminal(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int cnum, which, new, hours, silent = 0;
    CHAR_DATA *victim, *staff;
    struct affected_type af;
    char message[256], buf1[256], buf2[256], buf3[256], buf4[256];
    const char * const ct_name[] = {
        "allanak",
        "tuluk",
        "red_storm",
        "blackwing",
        "cai_shyzn",
        "mal_krian",
        "luirs",
        "\n"
    };
    int crime = 0;
    char buf[256];
    long permflags = 0;

    memset(&af, 0, sizeof(af));

    staff = SWITCHED_PLAYER(ch);
/*
   send_to_char("Routine currently disabled.\n\r", ch);
   return; 
 */
    argument = one_argument(argument, buf1, sizeof(buf1));

    if (!*buf1) {
        send_to_char("Usage is: criminal <-s> <character> <city> <duration> <crime>\n\r", ch);
        send_to_char("Possible crimes are:\n\r", ch);
        for (crime = 0; crime <= MAX_CRIMES; crime++) {
            sprintf(buf4, "[%d] %s\n\r", crime, Crime[crime]);
            send_to_char(buf4, ch);
        }
        return;
    }

    if (!strcmp(buf1, "-s")) {
        silent = 1;
        argument = one_argument(argument, buf1, sizeof(buf1));
    }

    if (!(victim = get_char_vis(ch, buf1))) {
        send_to_char("That player is not in the game.\n\r", ch);
        return;
    }
    /* We have a victim.  What city? */

    argument = one_argument(argument, buf2, sizeof(buf2));

    if (!*buf2) {
        send_to_char("The following cities have criminality:\n\r", ch);
        for (new = 0; *ct_name[new] != '\n'; new++) {
            sprintf(message, "     %s\n\r", ct_name[new]);
            send_to_char(message, ch);
        }
        return;
    }

    which = search_block(buf2, ct_name, 0);
    switch (which) {
    case 0:                    /* Allanak */
        cnum = CITY_ALLANAK;
        break;
    case 1:                    /* Tuluk */
        cnum = CITY_TULUK;
        break;
    case 2:                    /* Red Storm */
        cnum = CITY_RS;
        break;
    case 3:                    /* Blackwing */
        cnum = CITY_BW;
        break;
    case 4:                    /* Cai-shyzn */
        cnum = CITY_CAI_SHYZN;
        break;
    case 5:                    /* Mal-Krian */
        cnum = CITY_MAL_KRIAN;
        break;
    case 6:                    /* Luir's */
        cnum = CITY_LUIR;
        break;
    default:
        send_to_char("That city is invalid.\n\r", ch);
        return;
    }
    /* We have a city.  How long is the sentance? */

    argument = one_argument(argument, buf3, sizeof(buf3));

    hours = atoi(buf3);

    /* -Will do this check later, so you can extend/reduce durations when I */
    /* figure out how */
    if (IS_SET(victim->specials.act, city[cnum].c_flag) && hours != -1) {
        sprintf(message, "%s is already a criminal in %s.\n\r", GET_NAME(victim), ct_name[which]);
        send_to_char(message, ch);
        return;
    }
    if ((hours <= 0 || hours > 1000)
        && ((GET_LEVEL(staff) > STORYTELLER) && (hours != -1))) {
        send_to_char("Duration has to be between 1-1000 hours.\n\r", ch);
        return;
    }

    if (!IS_ADMIN(staff))       /* Giving the command to creators, but I dont want */
        hours = MIN(hours, 50); /* insane time limits from them. */

    argument = one_argument(argument, buf4, sizeof(buf4));
    crime = atoi(buf4);
    if (crime < 0 || crime > MAX_CRIMES)
        crime = CRIME_UNKNOWN;

    /* We have the hours.  Crim-them */
    if (hours != -1) {
        if (city[cnum].c_flag) {
            af.type = TYPE_CRIMINAL;
            af.location = CHAR_APPLY_CFLAGS;
            af.duration = hours;
            af.modifier = city[cnum].c_flag;
            af.bitvector = 0;
            af.power = crime;
            affect_to_char(victim, &af);

            sprintf(message, "%s is a criminal in %s for %d hours.\n\r", GET_NAME(victim),
                    ct_name[which], hours);
            send_to_char(message, ch);
        } else {
            gamelog("Screw-up in command Criminal");
            return;
        }
    } /* end if hours != -1 */
    else if (hours == -1) {
        /* This maintains a hidden edesc on the character for permanent
         * crimflags.  Tiernan 6/8/2004 */
        if (get_char_extra_desc_value(victim, "[CRIM_FLAGS]", buf, 100))
            permflags = atol(buf);
        switch (which) {
        case 0:                /* Allanak */
            MUD_SET_BIT(victim->specials.act, CFL_CRIM_ALLANAK);
            MUD_SET_BIT(permflags, CFL_CRIM_ALLANAK);
            break;
        case 1:                /* Tuluk */
            MUD_SET_BIT(victim->specials.act, CFL_CRIM_TULUK);
            MUD_SET_BIT(permflags, CFL_CRIM_TULUK);
            break;
        case 2:                /* Red Storm */
            MUD_SET_BIT(victim->specials.act, CFL_CRIM_RS);
            MUD_SET_BIT(permflags, CFL_CRIM_RS);
            break;
        case 3:                /* Blackwing */
            MUD_SET_BIT(victim->specials.act, CFL_CRIM_BW);
            MUD_SET_BIT(permflags, CFL_CRIM_BW);
            break;
        case 4:                /* Cai-Shyzn */
            MUD_SET_BIT(victim->specials.act, CFL_CRIM_CAI_SHYZN);
            MUD_SET_BIT(permflags, CFL_CRIM_CAI_SHYZN);
            break;
        case 5:                /* Mal-krian */
            MUD_SET_BIT(victim->specials.act, CFL_CRIM_MAL_KRIAN);
            MUD_SET_BIT(permflags, CFL_CRIM_MAL_KRIAN);
            break;
        case 6:                /* Luir's */
            MUD_SET_BIT(victim->specials.act, CFL_CRIM_LUIRS);
            MUD_SET_BIT(permflags, CFL_CRIM_LUIRS);
            break;
        }                       /* end switch(which) */
        // Update the edesc crimflag bitvector
        sprintf(buf, "%ld", permflags);
        set_char_extra_desc_value(victim, "[CRIM_FLAGS]", buf);
        sprintf(message, "%s is permanently wanted in %s.\n\r", GET_NAME(victim), ct_name[which]);
        send_to_char(message, ch);
    }
    /* end else if hours == -1 */
    if (silent == 0) {          /* Not-silent criminality */
        send_to_char("You are now wanted!\n\r", victim);
        cprintf(ch, "%s was notified of being a criminal.\n\r", GET_NAME(victim));
    } 
    /* Dont send the message to the player */
    else {
        cprintf(ch, "%s was not notified of being a criminal.\n\r", GET_NAME(victim));
    }
}

void
cmd_freeze(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *victim;
    char buf[256];

    argument = one_argument(argument, buf, sizeof(buf));
    if (!(victim = get_char_vis(ch, buf))) {
        send_to_char("No such player in the game.\n\r", ch);
        return;
    }
    if (IS_NPC(victim)) {
        send_to_char("No such player in the game.\n\r", ch);
        return;
    }
    if (IS_SET(victim->specials.act, CFL_FROZEN)) {
        act("$N is no longer frozen.", FALSE, ch, 0, victim, TO_CHAR);
        send_to_char("You are no longer frozen.\n\r", victim);
        REMOVE_BIT(victim->specials.act, CFL_FROZEN);
    } else if (victim == ch) {
        send_to_char("Believe me, you don't want to do this to yourself.\n\r", ch);
        return;
    } else {
        gamelog("Setting freeze bit in command freeze.");
        act("$N is now frozen, ouch.", FALSE, ch, 0, victim, TO_CHAR);
        send_to_char("You are now frozen, ouch.\n\r", victim);
        MUD_SET_BIT(victim->specials.act, CFL_FROZEN);
    }
    cmd_save(victim, "", 0, 0);
}


void
cmd_unban(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], tmp[256];
    struct ban_t *tmpban, *newban;

    SWITCH_BLOCK(ch);
    argument = one_argument(argument, buf, sizeof(buf));

    if (!*buf) {
        send_to_char("You must specify a site.\n\r", ch);
        return;
    }
    for (tmpban = ban_list; tmpban; tmpban = tmpban->next)
        if (!stricmp(tmpban->name, buf))
            break;
    if (!tmpban) {
        send_to_char("That site is not banned.\n\r", ch);
        return;
    }
    if (tmpban == ban_list) {
        ban_list = tmpban->next;
        free((char *) tmpban->name);
        free((char *) tmpban);
    } else {
        for (newban = ban_list; newban; newban = newban->next)
            if (newban->next == tmpban)
                break;
        if (!newban) {
            gamelog("Fuckup in banrm.");
            return;
        }
        if (tmpban->next)
            newban->next = tmpban->next;
        else
            newban->next = 0;
        free((char *) tmpban->name);
        free((char *) tmpban);
    }

    sprintf(tmp, "All players from [%s] are no longer banned.", buf);
    gamelog(tmp);
}

void
cmd_ban(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], tmp[256];
    struct ban_t *tmpban, *newban;
    struct descriptor_data *tmpdesc, *nextdesc;

    SWITCH_BLOCK(ch);
    argument = one_argument(argument, buf, sizeof(buf));

    if (!*buf) {
        send_to_char("You must specify a site.\n\r", ch);
        return;
    }
    if (wild_match(buf, HOMEHOST)) {
        send_to_char("Not a good idea dude.\n\r", ch);
        return;
    }
    for (tmpban = ban_list; tmpban; tmpban = tmpban->next)
        if (!stricmp(tmpban->name, buf)) {
            send_to_char("That site is already banned.\n\r", ch);
            return;
        }
    CREATE(newban, struct ban_t, 1);
    newban->next = ban_list;
    newban->name = strdup(buf);
    ban_list = newban;

    sprintf(tmp, "All players from [%s] are now banned.", buf);
    gamelog(tmp);

    for (tmpdesc = descriptor_list; tmpdesc; tmpdesc = nextdesc) {
        nextdesc = tmpdesc->next;

        if (wild_match(buf, tmpdesc->host) && tmpdesc->character && tmpdesc->character->in_room) {
            send_to_char("Your site has been banned.\n\r", tmpdesc->character);
            act("$n explodes into little tiny bits.", FALSE, tmpdesc->character, 0, 0, TO_ROOM);
            close_socket(tmpdesc);
            extract_char(tmpdesc->character);
            tmpdesc = 0;
        }
    }
}

void
cmd_bsave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    void save_bans(void);

    SWITCH_BLOCK(ch);
    send_to_char("Ban save in progress...\n\r", ch);
    save_bans();
    send_to_char("Ban save completed.\n\r", ch);
}

void
cmd_invis(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[256], buf[256];
    int lev, old_lev;
    CHAR_DATA *tmpch;

    SWITCH_BLOCK(ch);

    argument = one_argument(argument, arg, sizeof(arg));
    if (!isdigit(*arg)) {
        send_to_char("You must specify an invisibility level.\n\r", ch);
        return;
    }
    if (((lev = atoi(arg)) > (GET_LEVEL(ch) + 1)) || (lev > OVERLORD)) {
        send_to_char("You may not choose an invis level that high.\n\r", ch);
        return;
    }

    old_lev = ch->specials.il;

    for (tmpch = ch->in_room->people; tmpch; tmpch = tmpch->next_in_room)
        if ((lev > GET_LEVEL(tmpch)) && (old_lev <= GET_LEVEL(tmpch)) && (ch != tmpch)) {
            sprintf(buf, "%s slowly fades out of existence.\n\r", capitalize(PERS(tmpch, ch)));
            send_to_char(buf, tmpch);
        }

    ch->specials.il = lev;

    for (tmpch = ch->in_room->people; tmpch; tmpch = tmpch->next_in_room)
        if ((lev <= GET_LEVEL(tmpch)) && (old_lev > GET_LEVEL(tmpch)) && (ch != tmpch)) {
            sprintf(buf, "%s slowly fades into existence.\n\r", capitalize(PERS(tmpch, ch)));
            send_to_char(buf, tmpch);
        }

    sprintf(buf, "Your invis level is now %d.\n\r", lev);
    send_to_char(buf, ch);
}


extern int visited_nodes;

void
cmd_directions(CHAR_DATA *ch, const char *argument, Command cmd, int count) {
    // not implemented in easy version
   send_to_char("Command not implemented in this version.\n", ch);

}

void
cmd_path(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], arg2[256], arg3[256], buf[MAX_STRING_LENGTH];
    struct room_data *r1, *r2;
    CHAR_DATA *tch;

    CHAR_DATA *path_for;
    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));

    if (!strcmp(arg1, "for")) {
        path_for = get_char_vis(ch, arg2);
        if (!path_for) {
            send_to_char("That character doesn't exist, to make a path for.\n\r", ch);
            return;
        };

        if (!path_for->in_room) {
            send_to_char("That character isnt in a room, to make a path for.\n\r", ch);
            return;
        };

        strcpy(arg1, arg2);
        strcpy(arg2, arg3);
    } else {
        path_for = ch;
    };

    if (!*arg1 || !*arg2) {
        send_to_char("Usage: path <char1|room1> <char2|room2>.\n\r", ch);
        send_to_char("Usage: path for <char1> <char2|room2>(commands char1 would type).\n\r", ch);
        return;
    }


    if (!(isdigit(*arg1) && (r1 = get_room_num(atoi(arg1))))
        && !((tch = get_char_vis(ch, arg1)) && (r1 = tch->in_room))) {
        send_to_char("That room doesn't exist.\n\r", ch);
        return;
    }
    if (!(isdigit(*arg2) && (r2 = get_room_num(atoi(arg2))))
        && !((tch = get_char_vis(ch, arg2)) && (r2 = tch->in_room))) {
        send_to_char("That room doesn't exist.\n\r", ch);
        return;
    }

    visited_nodes = 0;
    clock_t starttime, endtime;
    struct tms tbuf;
    times(&tbuf);
    starttime = tbuf.tms_utime;
    if (path_from_to_for(r1, r2, 7000, MAX_STRING_LENGTH, buf, path_for) == -1) {
        times(&tbuf);
        endtime = tbuf.tms_utime;
        cprintf(ch, "No path found, User Time: %llums", (unsigned long long)endtime - starttime);
        return;
    }
    times(&tbuf);
    endtime = tbuf.tms_utime;

    cprintf(ch, "Path found, depth: %d, (%d nodes), User Time: %llums:\n\r  %s\n\r", r2->path_list.distance,
            visited_nodes, (unsigned long long)endtime - starttime, buf);
}

void
cmd_quiet(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[256];
    int new, which, flag;
    char buffer[256];
    char buf[256];

    /* If you add one here you must add one to quiet_description.
     * Failure will not only be sloppy, but your new option won't be listed */
    const char * const quiet_settings[] = {
        "off",
        "all",
        "log",
        "wish",
        "icomm",
        "gcomm",
        "psi",
        "typo",
        "bug",
        "idea",
        "contact",
        "death",
        "connect",
        "zone",
        "reroll",
        "combat",
        "misc",
        "calc",
        "bank",
        "npc_ai",
	"debug",
        "bcomm",
        "\n"
    };

    char *quiet_description[] = {
        "Listen to all messages.",
        "Ignore all messages.",
        "Ignore Log messages like who has connected.",
        "Ignore The wishes of players to the gods.",
        "Ignore Immortal comm channel.",
        "Ignore God comm channel.",
        "Ignore psi messages from players to linkdead players or mobiles.",
        "Ignore when player logs a typo error.",
        "Ignore when player logs a bug.",
        "Ignore when player logs an idea.",
        "Ignore the psi messages people send to you.",
        "Ignore death logs.",
        "Ignore connect/disconnect messages.",
        "Ignore zone save/reset/warning messages.",
        "Ignore character reroll messages.",
        "Ignore combat debug.",
        "Ignore miscellaneous logs.",
        "Ignore calculations for things like skills, etc., other than combat.",
        "Ignore bank transactions.",
        "Ignore NPC AI remarks, like spellcasting.",
	"Ignore debug/troubleshooting messages.",
        "Ignore Builder comm channel.",
        "\n"
    };

    argument = one_argument(argument, arg, sizeof(arg));

    strcpy(buffer, "");

    if (!(*arg)) {
        send_to_char("You must provide one of the following toggles:\n\r", ch);
        for (new = 0; (*quiet_settings[new] != '\n')
             && (*quiet_description[new] != '\n'); new++) {
            if (new - 1 > 0) {
                if (IS_SET(ch->specials.quiet_level, (1 << (new - 2))))
                    sprintf(buffer, "[ON ] ");
                else
                    sprintf(buffer, "[OFF] ");
            } else
                sprintf(buffer, "      ");

            sprintf(buf, "%7s:", quiet_settings[new]);
            strcat(buffer, buf);
            strcat(buffer, quiet_description[new]);
            strcat(buffer, "\n\r");
            send_to_char(buffer, ch);
        }
        return;
    }


    if (*arg) {
        which = search_block(arg, quiet_settings, 0);

        switch (which) {
            /* case 0 and case 1 messages switched to match action  -Morgenes */
        case 0:                /* Turn all off */
            ch->specials.quiet_level = 0;
            sprintf(buffer, "You will now hear all messages.\n\r");
            break;
        case 1:                /* Turn all on  */
            ch->specials.quiet_level = ~0;
            sprintf(buffer, "You will now ignore all messages.\n\r");
            break;
        case 2:                /* Log           */
        case 3:                /* Wish          */
        case 4:                /* Immortal Comm */
        case 5:                /* God Comm      */
        case 6:                /* Psi           */
        case 7:                /* Typo          */
        case 8:                /* Bug           */
        case 9:                /* Idea          */
        case 10:               /* Contact       */
        case 11:               /* Death         */
        case 12:               /* Connect       */
        case 13:               /* Zone          */
        case 14:               /* Reroll        */
        case 15:               /* Combat        */
        case 16:               /* Misc          */
        case 17:               /* Calc          */
        case 18:               /* Bank          */
        case 19:               /* NPC_AI        */
        case 20:               /* Debug/Info    */
        case 21:               /* Builder Comm  */
            strcat(buffer, "You will");

            flag = (1 << (which - 2));

            if (IS_SET(ch->specials.quiet_level, flag)) {
                REMOVE_BIT(ch->specials.quiet_level, flag);
                strcat(buffer, " now hear ");
            } else {
                MUD_SET_BIT(ch->specials.quiet_level, flag);
                strcat(buffer, " no longer hear ");
            }

            strcat(buffer, quiet_settings[which]);
            strcat(buffer, " messages.\n\r");

            break;
        default:
            sprintf(buffer, "Invalid choice.  Type quiet with no arguments to see choices.\n\r");
            break;
        }
        send_to_char(buffer, ch);
        return;
    }
}

void
cmd_nonet(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char("What?\n\r", ch);
}

extern bool should_do_perf_logging;

void
cmd_server(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    char buf[MAX_INPUT_LENGTH];
    int option, i;
    const char * const option_list[] = {
	    "shadow_moon",
	    "global_darkness",
	    "performance",
	    "\n"
    };
		    
    arg = one_argument(arg, buf, sizeof(buf));
    option = search_block(buf, option_list, TRUE);
    switch (option) {
	case 0: // shadow_moon
	    use_shadow_moon = !use_shadow_moon;
	    cprintf(ch, "The shadow moon will%sbe displayed.\n\r",
			    use_shadow_moon ? " " : " not "); 
	    break;
	case 1: // global_darkness
	    use_global_darkness = !use_global_darkness;
	    cprintf(ch, "The Known World will%sbe in total darkness.\n\r",
			    use_global_darkness ? " " : " not "); 
	    break;
    case 2: // performance
        should_do_perf_logging = !should_do_perf_logging;
        cprintf(ch, "Peformance logging %s.\n\r", 
         should_do_perf_logging ? "started" : "stopped");
        gamelogf("Performance logging %s.",
         should_do_perf_logging ? "started" : "stopped");
        break;
 
	default:
            cprintf(ch, "Server options:\n\r");
	    for (i = 0; i < (sizeof(option_list) / sizeof(option_list[0]) - 1); i++) 
                cprintf(ch, "\t%s\n\r",option_list[i]);
	    break;
    }
}

void
cmd_plmax(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    char buf[MAX_INPUT_LENGTH];


    int newlimit;
    struct descriptor_data *point;
    int players;

    arg = one_argument(arg, buf, sizeof(buf));


    if (*buf == 0) {
        players = 0;
        for (point = descriptor_list; point; point = point->next)
            players++;
        sprintf(buf, "Current player limit: %d\n\r", avail_descs);
        send_to_char(buf, ch);
        sprintf(buf, "Maximum player limit: %d\n\r", real_avail_descs);
        send_to_char(buf, ch);
        sprintf(buf, "Highest descriptor in use: %d\n\r", maxdesc);
        send_to_char(buf, ch);
        sprintf(buf, "Players currently playing: %d\n\r", players);
        send_to_char(buf, ch);
    } else {
        newlimit = atoi(buf);
        if (newlimit > real_avail_descs)
            newlimit = real_avail_descs;
        if (newlimit < 0)
            newlimit = 0;
        avail_descs = MAX(newlimit, 1);
        sprintf(buf, "Game now limited to %d players.\n\r", avail_descs);
        send_to_char(buf, ch);
        sprintf(buf, "Player limit adjusted to %d by %s.", avail_descs, GET_NAME(ch));
        gamelog(buf);
    }
}



/* this function is too good as a utility to let it pass as a command */
void
reset_char_skills(CHAR_DATA * victim)
{
    int i;

    // Clean out the old skills
    for (i = 1; i < MAX_SKILLS; i++) {
        if (victim->skills[i]) {
            free((char *) victim->skills[i]);
            victim->skills[i] = (struct char_skill_data *) 0;
        }
    }

    // Set their specialized skills based on race, etc.
    init_racial_skills(victim);
    init_languages(victim);
    init_psi_skills(victim);

    // Now apply guild and subguild skills
    for (i = 1; i < MAX_SKILLS; i++) {
        if (guild[(int) victim->player.guild].skill_prev[i] == SPELL_NONE)
            init_skill(victim, i, guild[(int) victim->player.guild].skill_percent[i]);
        else if (sub_guild[(int) victim->player.sub_guild].skill_prev[i] == SPELL_NONE) {
            init_skill(victim, i, sub_guild[(int) victim->player.sub_guild].skill_percent[i]);
        }
    }

    // Reach skills for magickers
    if (is_magicker(victim)) {
        if (!has_skill(victim, SKILL_REACH_NONE)) {
            init_skill(victim, SKILL_REACH_NONE, 100);
        }

        if (!has_skill(victim, SKILL_REACH_LOCAL)) {
            init_skill(victim, SKILL_REACH_LOCAL, 100);
        }
    }

    // Give psionicists a boost
    if (GET_GUILD(victim) == GUILD_PSIONICIST) {
        if (victim->skills[PSI_CONTACT]->learned < 20)
            victim->skills[PSI_CONTACT]->learned = 20;
        if (victim->skills[PSI_BARRIER]->learned < 10)
            victim->skills[PSI_BARRIER]->learned = 10;
    }
}

void
cmd_skillreset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256];
    CHAR_DATA *victim, *staff;

    staff = SWITCHED_PLAYER(ch);
    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!(victim = get_char_vis(ch, arg1))) {
        send_to_char("No such character in the world.\n\r", ch);
        return;
    }

    if (!IS_NPC(victim)) {
        if (!IS_ADMIN(staff)) {
            send_to_char("Player resets are restricted to greater immortals.\n\r", ch);
            return;
        }
    } else if ((!IS_ADMIN(staff)) && IS_MOB(victim)) {
        if (!has_privs_in_zone(ch, npc_index[victim->nr].vnum / 1000, MODE_STR_EDIT)) {
            send_to_char("You do not have priveleges in that zone.\n\r", ch);
            return;
        }
    }

    reset_char_skills(victim);
    send_to_char("Done.\n\r", ch);
}

void
cmd_grp(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], arg2[256], arg3[256], arg4[256];
    char buffer[256];
    int g, zone, uid;


    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));
    argument = one_argument(argument, arg4, sizeof(arg4));

    if (!*arg1) {
        send_to_char("Usage: grp <field> <add/delete> <zone#> <UID#>\r\n", ch);

        for (g = 0; strlen(grp_priv[g].name); g++) {
            sprintf(buffer, "       %s\r\n", grp_priv[g].name);
            send_to_char(buffer, ch);
        };
        return;
    }
    for (g = 0; strlen(grp_priv[g].name); g++)
        if (!strnicmp(arg1, grp_priv[g].name, strlen(arg1)))
            break;

    if (!strlen(grp_priv[g].name)) {
        send_to_char("Unknown field.\r\n", ch);
    }

    if (!*arg2 || ((*arg2 != 'a') && (*arg2 != 'd'))) {
        send_to_char("Usage: grp <field> {a|d}\n", ch);
    }

    if (!*arg3 || !*arg4) {
        send_to_char("Usage: grp <field> {a|d} <zone #> <UID #> \n", ch);
    }

    zone = atoi(arg3);
    uid = atoi(arg4);
}

void
cmd_zorch(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int which;
    char arg1[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *d;
    CHAR_DATA *staff;

    staff = SWITCHED_PLAYER(ch);

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!*arg1) {
        cprintf(ch, "Usage: zorch <connection number|name>\n");
        return;
    }

    which = atoi(arg1);
    for (d = descriptor_list; d; d = d->next) {
        if ((d->descriptor == which) && (!d->character || CAN_SEE(ch, d->character)))
            break;
    }
    if (!d)
        for (d = descriptor_list; d; d = d->next) {
            if (d->character && CAN_SEE(ch, d->character)
                && !stricmp(arg1, GET_NAME(d->character)))
                break;
        }

    if (!d) {
        cprintf(ch, "No such connection.\n");
        return;
    }

    if (d->character 
     && (GET_LEVEL(SWITCHED_PLAYER(d->character)) > GET_LEVEL(staff))) {
        cprintf(ch, "You haven't the powers to do that!\n");
        return;
    }

    if (d->character && !d->connected) {
        sprintf(buf, "%s (%s) zorched\n\r", GET_NAME(d->character), d->player_info->name);
        send_to_char(buf, ch);

        act("$n gasps as $e is suddenly sucked into the sky.", FALSE, d->character, 0, 0, TO_ROOM);
        extract_char(d->character);
    } else {
        sprintf(buf, "Descriptor %d disconnected.\n\r", which);
        send_to_char(buf, ch);

        close_socket(d);
    }
}

void
convert_player_file(char *filename)
{

    CHAR_DATA *ch = (CHAR_DATA *) malloc(sizeof *ch);
    char buf[256];
    int i;

    clear_char(ch);
    for (i = 0; i < MAX_WEAR; i++)
        ch->equipment[i] = (struct obj_data *) 0;

    sprintf(buf, "Loading %s", filename);
    gamelog(buf);

    load_char(ch->name, ch);

    extract_char(ch);

    return;
}

void
convert_players(void)
{
    DIR *dp;
    struct direct *entry;

    if (!(dp = opendir("players"))) {
        gamelog("Can't find players directory.\n\r");
        return;
    }

    while ((entry = readdir(dp))) {
        if ((*entry->d_name != '\0') && (*entry->d_name != '.')) {
            convert_player_file(entry->d_name);
        }
    }

    gamelog("Players converted.");
    closedir(dp);
}

void javascript_remove_compiled_programs(void);

void
cmd_fload(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[256];

    /*  FILE *help_fl; */

    argument = one_argument(argument, arg1, sizeof(arg1));

    if (!stricmp(arg1, "motd")) {
        file_to_string(MOTD_FILE, &motd);
        gamelog("Loading motd.");
    } else if (!stricmp(arg1, "credits")) {
        file_to_string(CREDITS_FILE, &credits);
        gamelog("Loading credits.");
    } else if (!stricmp(arg1, "areas")) {
        file_to_string(AREAS_FILE, &areas);
        gamelog("Loading area credits.");
    } else if (!stricmp(arg1, "news")) {
        file_to_string(NEWS_FILE, &news);
        gamelog("Loading news.");
    } else if (!stricmp(arg1, "wizlist")) {
        file_to_string(WIZLIST_FILE, &wizlist);
        gamelog("Loading wizlist.");
    } else if (!stricmp(arg1, "menu")) {
        file_to_string(MENUFILE, &menu);
        strcat(menu, "\n\rChoose thy fate: ");
        sprintf(buf, "%s.no_ansi", MENUFILE);
        file_to_string(buf, &menu_no_ansi);
        strcat(menu_no_ansi, "\n\rChoose thy fate: ");
        gamelog("Loading main menus.");
    } else if (!stricmp(arg1, "socials")) {
        gamelog("Loading socials.");
        unboot_social_messages();
        boot_social_messages();
    } else if (!stricmp(arg1, "crafting")) {
        gamelog("Loading crafts. (data_files/crafting.xml)");
        unboot_crafts();
        boot_crafts();
    } else if (!strnicmp(arg1, "emails", 4)) {
        load_email_addresses();
        gamelog("Loading email adresses.");
    } else if (!stricmp(arg1, "guilds")) {
        gamelog("Loading guilds file.");
        boot_guilds();
    } else if (!stricmp(arg1, "subguilds")) {
        gamelog("Loading subguilds file.");
        boot_subguilds();
    } else if (!stricmp(arg1, "clans")) {
        gamelog("Loading clans.");
        reboot_clans();
    } else if (!stricmp(arg1, "shops")) {
        gamelog("Loading shopkeepers file.");
        boot_shops();
    } else if (!stricmp(arg1, "races")) {
        gamelog("Loading races file.");
	boot_races();
        // boot_races_from_db();
    } else if (!stricmp(arg1, "cities")) {
        gamelog("Loading cities file.");
        boot_cities();
    } else if (!stricmp(arg1, "convert")) {
        gamelog("Converting Characters ...");
        convert_players();
    } else if (!stricmp(arg1, "char")) {
        gamelog("Loading char.");
        load_char(ch->name, ch);
    } else if (!stricmp(arg1, "nonews")) {
        gamelog("Loading nonews (disabled).");
//        unboot_nonews();
//        boot_nonews();
    } else if (!stricmp(arg1, "warnnews")) {
        gamelog("Loading warnnews (disabled).");
//       unboot_warnnews();
//        boot_warnnews();
    } else if (!stricmp(arg1, "forage")) {
        gamelog("Loading forage data.");
        unboot_forage();
        boot_forage();
    } else if (!stricmp(arg1,"apartments")) {
    	gamelog("Loading apartments data (disabled).");
 //   	unboot_apartments();
 //   	boot_apartments();

    } else
        send_to_char("You may only load the motd, the news, the credits, programs,\n\r"
                     "emails, guilds, shops, races, cities, areas, socials, menu,\n\r"
                     "specials, clans, crafting, nonews, forage, apartments or the wizlist.\n\r", ch);
        return;

}

void
threaded_pfind(io_t_msg * msg)
{
    int i;
    char dn[256], name[256];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    bool post = FALSE, pre = FALSE;
    struct dirent entry, entry2, *result;
    DIR *dp, *dp2;

    strncpy(name, (char *)msg->data, sizeof(name));

    /* is there a star at the end? */
    if (name[strlen(name) - 1] == '*') {
        post = TRUE;
        /* clobber the star */
        name[strlen(name) - 1] = '\0';
    }

    /* is there a star at the beginning? */
    if (name[0] == '*') {
        pre = TRUE;

        /* shift the string down one */
        for (i = 1; name[i]; i++)
            name[i - 1] = name[i];

        name[i - 1] = name[i];
    }

    sprintf(dn, "%s/", ACCOUNT_DIR);

    if (!(dp = opendir(dn))) {
        snprintf(buf, sizeof(buf), "Could not open '%s'", dn);
        io_thread_send_reply(msg, IO_T_MSG_LOG, buf, strlen(buf) + 1);
        return;
    }

    buf2[0] = '\0';

    sprintf(buf, "Beginning threaded pfind....\n\r");
    io_thread_send_reply(msg, IO_T_MSG_SEND, buf, strlen(buf) + 1);

    /* look at all the accounts */
    int skippedDirs = 0;
    while (readdir_r(dp, &entry, &result) == 0 && result) {
        if (*entry.d_name != '.') {
            sprintf(dn, "%s/%s/", ACCOUNT_DIR, entry.d_name);

            if (!(dp2 = opendir(dn))) {
                skippedDirs++;
                continue;
            }

            while (readdir_r(dp2, &entry2, &result) == 0 && result) {
                if (*entry2.d_name != '.' && str_suffix(".xml", entry2.d_name)) {
                    if ((pre && post && !str_infix(name, entry2.d_name))
                        || (pre && !str_suffix(name, entry2.d_name))
                        || (post && !str_prefix(name, entry2.d_name))
                        || (!pre && !post && !strcasecmp(name, entry2.d_name))) {
                        sprintf(buf, "%-19s - %-20s\n\r", entry.d_name, entry2.d_name);

                        if (strlen(buf) + strlen(buf2) > MAX_STRING_LENGTH)
                            break;

                        strcat(buf2, buf);
                    }
                }
            }
            closedir(dp2);
        }
    }
    closedir(dp);

    if (buf2[0] == '\0') {
        sprintf(buf, "No accounts found with characters named '%s'.\n\r", name);
        io_thread_send_reply(msg, IO_T_MSG_SEND, buf, strlen(buf) + 1);
//        send_to_char(buf, ch);
        return;
    } else {
        sprintf(buf, "Account             - Character\n\r_______" "               _________\n\r%s",
                buf2);
        io_thread_send_reply(msg, IO_T_MSG_PAGED_SEND, buf, strlen(buf) + 1);
//        page_string(ch->desc, buf, 1);
    }

    if (skippedDirs) {
        // No newlines for shhlog()
        sprintf(buf,
                "/* %d directories were skipped due to an error, you may wish to rerun the command. */",
                skippedDirs);
        io_thread_send_reply(msg, IO_T_MSG_SHHLOG, buf, strlen(buf) + 1);
        io_thread_send_reply(msg, IO_T_MSG_SEND, buf, strlen(buf) + 1);
    }
}

/*
 * Search for players with names like or exact match of argument
 */
void
cmd_pfind(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[256];
    if (!ch->desc) {
        return;
    }

    argument = one_argument(argument, name, sizeof(name));

    if (strlen(name) < 1) {
        send_to_char("Who would you like to find?\n\r", ch);
        return;
    }

    cprintf(ch, "Queuing pfind request for '%s'.\n", name);
    io_thread_send_msg(ch->name, ch->account, ch->desc->descriptor, IO_T_MSG_PFIND, name,
                       strlen(name) + 1);
}

/*
 * list all the accounts
 */
void
cmd_afind(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int i;
    char dn[256], name[256];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    bool post = FALSE, pre = FALSE;
    struct direct *entry;
    DIR *dp;

    argument = one_argument(argument, name, sizeof(name));

    /* is there a star at the end? */
    if (name[strlen(name) - 1] == '*') {
        post = TRUE;
        /* clobber the star */
        name[strlen(name) - 1] = '\0';
    }

    /* is there a star at the beginning? */
    if (name[0] == '*') {
        pre = TRUE;

        /* shift the string down one */
        for (i = 1; name[i]; i++)
            name[i - 1] = name[i];

        name[i - 1] = name[i];
    }

    sprintf(dn, "%s/", ACCOUNT_DIR);

    if (!(dp = opendir(dn))) {
        gamelogf("Could not open '%s'", dn);
        return;
    }

    buf2[0] = '\0';

    /* look at all the accounts */
    while ((entry = readdir(dp))) {
        if (*entry->d_name != '.') {
            if (name[0] == '\0' || (pre && post && !str_infix(name, entry->d_name))
                || (pre && !str_suffix(name, entry->d_name))
                || (post && !str_prefix(name, entry->d_name))
                || (!pre && !post && !strcasecmp(name, entry->d_name))) {
                sprintf(buf, "%s\n\r", entry->d_name);

                if (strlen(buf2) + strlen(buf) > MAX_STRING_LENGTH)
                    break;

                strcat(buf2, buf);
            }
        }
    }
    closedir(dp);

    if (buf2[0] == '\0' && name[0] != '\0') {
        sprintf(buf, "No accounts found named '%s'.\n\r", name);
        send_to_char(buf, ch);
        return;
    } else if (name[0] == '\0')
        sprintf(buf, "All Accounts:\n\r%s", buf2);
    else
        sprintf(buf, "Accounts:\n\r%s", buf2);

    page_string(ch->desc, buf, 1);

}

/* account information */
void
cmd_ainfo(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH * 2];
    PLAYER_INFO *pPInfo;
    CHAR_INFO *pCInfo;
    DESCRIPTOR_DATA d;
    bool fOld = FALSE;
    int cnt = 0;
    char *flags;
    int bhelp = 0;
    int bchars = 0;
    int bhchars = 0;
    int bcomments = 0;
    int bhcomments = 0;
    int bkudos = 0;
    int bhkudos = 0;
    int bkarma = 0;
    int bhkarma = 0;
    int blogins = 0;
    int bhlogins = 0;
    int boptions = 0;
    int bhoptions = 0;
    int bshowall = 0;

    arg_struct ainfo_args[] = {
        {'c', "chars", ARG_PARSER_BOOL, {&bchars}, {0},
         "Show characters in the output."},
        {'C', "hide_chars", ARG_PARSER_BOOL, {&bhchars}, {0},
         "Hide characters from the output."},
        {'m', "comments", ARG_PARSER_BOOL, {&bcomments}, {0},
         "Show comments in the output."},
        {'M', "hide_comments", ARG_PARSER_BOOL, {&bhcomments}, {0},
         "Hide comments from the output."},
        {'k', "kudos", ARG_PARSER_BOOL, {&bkudos}, {0},
         "Show kudos in the output."},
        {'K', "hide_kudos", ARG_PARSER_BOOL, {&bhkudos}, {0},
         "Hide kudos from the output."},
        {'l', "logins", ARG_PARSER_BOOL, {&blogins}, {0},
         "Show logins in the output."},
        {'L', "hide_logins", ARG_PARSER_BOOL, {&bhlogins}, {0},
         "Hide logins from the output."},
        {'o', "options", ARG_PARSER_BOOL, {&boptions}, {0},
         "Show karma options in the output."},
        {'O', "hide_options", ARG_PARSER_BOOL, {&bhoptions}, {0},
         "Hide karma options from the output."},
        {'r', "karma", ARG_PARSER_BOOL, {&bkarma}, {0},
         "Show karma log in the output."},
        {'R', "hide_karma", ARG_PARSER_BOOL, {&bhkarma}, {0},
         "Hide karma log from the output."},
        {'h', "help", ARG_PARSER_BOOL, {&bhelp}, {0}, "Show this help text."},
    };

    one_argument(argument, buf, sizeof(buf));
    cmd_getopt(ainfo_args, sizeof(ainfo_args) / sizeof(ainfo_args[0]), argument);

    if (buf[0] == '\0' || bhelp) {
        cprintf(ch, "Options for ainfo:\n\r");
        page_help(ch, ainfo_args, sizeof(ainfo_args) / sizeof(ainfo_args[0]));
        cprintf(ch, "Syntax: ainfo <account> <options>\n\r");
        return;
    }

    bshowall = !bchars && !bcomments && !bkudos && !blogins && !boptions;

    buf[0] = toupper(buf[0]);
    if ((pPInfo = find_player_info(buf)) == NULL) {
        if ((fOld = load_player_info(&d, buf)) == FALSE) {
            sprintf(buf2, "Account '%s' not found.\n\r", buf);
            send_to_char(buf2, ch);
            return;
        }
        pPInfo = d.player_info;
        d.player_info = NULL;
    }

    buf2[0] = '\0';
    sprintf(buf, "Player: %s     Email: %s\n\r", pPInfo->name, pPInfo->email);
    strcat(buf2, buf);

    if ((bshowall && !bhlogins) || blogins) {
        if ((ch->player.level > CREATOR || !is_immortal_account(pPInfo->name))
            && pPInfo->last_on[0]) {
            sprintf(buf, "Last on: %s to %s from %s\n\r", 
             pPInfo->last_on[0]->in, pPInfo->last_on[0]->out,
             pPInfo->last_on[0]->site);
            strcat(buf2, buf);
        }

        if ((ch->player.level > CREATOR || !is_immortal_account(pPInfo->name))
            && pPInfo->last_failed[0]) {
            sprintf(buf, "Last failed login: %s from %s\n\r",
             pPInfo->last_failed[0]->in, pPInfo->last_failed[0]->site);
            strcat(buf2, buf);
        }
    }

    if( bshowall ) {
        if (pPInfo->created && pPInfo->created[0] != '\0') {
            sprintf(buf, "Created on: %s\n\r", pPInfo->created);
            strcat(buf2, buf);
        }
    }

    if ((bshowall && !bhcomments) || bcomments) {
        if (pPInfo->comments && strcmp(pPInfo->comments, "(null)")) {
            sprintf(buf, "Comments:\n\r%s\n\r", pPInfo->comments);
            strcat(buf2, buf);
        }
    }

    if ((bshowall && !bhkudos) || bkudos) {
        if (pPInfo->kudos && strcmp(pPInfo->kudos, "(null)")) {
            sprintf(buf, "Kudos:\n\r%s\n\r", pPInfo->kudos);
            strcat(buf2, buf);
        }
    }

    if( bshowall ) {
        sprintf(buf, "# of Characters (Total/Alive/Applying/Allowed):"
         "  %d/%d/%d/%d\n\r",
         pPInfo->num_chars, pPInfo->num_chars_alive, pPInfo->num_chars_applying,
         pPInfo->num_chars_allowed);
        strcat(buf2, buf);
    }

    if( bshowall ) {
        flags = show_flags(pinfo_flags, pPInfo->flags);
        sprintf(buf, "Flags : %s\n\r", flags);
        strcat(buf2, buf);
        free(flags);
    }

    if((bshowall && !bhkarma) || bkarma) {
        if (pPInfo->karma) {
            sprintf(buf, "Karma : %d\n\r", pPInfo->karma);
            strcat(buf2, buf);
        }
    }

    if((bshowall && !bhoptions) || boptions) {
        if (ch->player.level > BUILDER || !is_immortal_account(pPInfo->name)) {
            if (pPInfo->karmaRaces == 0)
                sprintf(buf, "Races : none\n\r");
            else if ((pPInfo->karmaRaces & KINFO_RACE_ALL) == KINFO_RACE_ALL)
                sprintf(buf, "Races : all\n\r");
            else {
                flags = show_flags(kinfo_races, pPInfo->karmaRaces);
                sprintf(buf, "Races : %s\n\r", flags);
                free(flags);
            }
            strcat(buf2, buf);
    
            if (pPInfo->karmaGuilds == 0)
                sprintf(buf, "Guilds: none\n\r");
            else if ((pPInfo->karmaGuilds & KINFO_GUILD_ALL) == KINFO_GUILD_ALL)
                sprintf(buf, "Guilds: all\n\r");
            else {
                flags = show_flags(kinfo_guilds, pPInfo->karmaGuilds);
                sprintf(buf, "Guilds: %s\n\r", flags);
                free(flags);
            }
            strcat(buf2, buf);
    
            if( pPInfo->tempKarmaRaces != 0 ) {
                if ((pPInfo->tempKarmaRaces & KINFO_RACE_ALL) == KINFO_RACE_ALL)
                    sprintf(buf, "Special App Races : all\n\r");
                else {
                    flags = show_flags(kinfo_races, pPInfo->tempKarmaRaces);
                    sprintf(buf, "Special App Races : %s\n\r", flags);
                    free(flags);
                }
                strcat(buf2, buf);
            }

            if( pPInfo->tempKarmaGuilds != 0 ) {
                if ((pPInfo->tempKarmaGuilds & KINFO_GUILD_ALL) == KINFO_GUILD_ALL)
                    sprintf(buf, "Special App Guilds : all\n\r");
                else {
                    flags = show_flags(kinfo_guilds, pPInfo->tempKarmaGuilds);
                    sprintf(buf, "Special App Guilds : %s\n\r", flags);
                    free(flags);
                }
                strcat(buf2, buf);
            }
        }
    }

    if((bshowall && !bhkarma) || bkarma) {
        if (pPInfo->karmaLog && strcmp(pPInfo->karmaLog, "(null)")) {
            sprintf(buf, "Karma Log:\n\r%s\n\r", pPInfo->karmaLog);
            strcat(buf2, buf);
        }
    }

    if((bshowall && !bhchars) || bchars) {
        sprintf(buf, "Current Characters:\n\r ");
        strcat(buf2, buf);

        for (pCInfo = pPInfo->characters; pCInfo; pCInfo = pCInfo->next) {
            char tmp[MAX_STRING_LENGTH];

            sprintf(tmp, "(%s)", show_attrib(pCInfo->status, application_type));
            sprintf(buf, " %-15s %-10s", pCInfo->name, tmp);
            strcat(buf2, buf);

            if (!(++cnt % 2))
                strcat(buf2, "\n\r ");
        }

        if (cnt % 2)
            strcat(buf2, "\n\r");
    }

    page_string(ch->desc, buf2, 1);

    if (fOld) {
        free_player_info(pPInfo);
    }
}


void
cmd_last(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    PLAYER_INFO *pPInfo;
    DESCRIPTOR_DATA d;
    bool fOld = FALSE;
    bool failed = FALSE;
    int i;

    argument = one_argument(argument, buf, sizeof(buf));

    if (buf[0] == '\0') {
        send_to_char("Syntax: last [failed] <account>\n\r", ch);
        return;
    }

    if (!strcmp(buf, "failed")) {
        failed = TRUE;

        argument = one_argument(argument, buf, sizeof(buf));
        if (buf[0] == '\0') {
            send_to_char("Syntax: last [failed] <account>\n\r", ch);
            return;
        }
    }

    buf[0] = toupper(buf[0]);

    if (is_immortal_account(buf) && ch->player.level == CREATOR) {
        send_to_char("You can't do that on that account.\n\r", ch);
        return;
    }

    if ((pPInfo = find_player_info(buf)) == NULL) {
        if ((fOld = load_player_info(&d, buf)) == FALSE) {
            sprintf(buf2, "Account '%s' not found.\n\r", buf);
            send_to_char(buf2, ch);
            return;
        }
        pPInfo = d.player_info;
        d.player_info = NULL;
    }

    buf2[0] = '\0';
    sprintf(buf, "Last %s logins for %s:\n\r", failed ? "failed " : "", pPInfo->name);
    strcat(buf2, buf);

    if (!failed)
        for (i = 0; i < MAX_LOGINS_SAVED && pPInfo->last_on[i]; i++) {
            sprintf(buf, "  %s to %s from %s\n\r", pPInfo->last_on[i]->in, pPInfo->last_on[i]->out,
                    pPInfo->last_on[i]->site);
            strcat(buf2, buf);
    } else
        for (i = 0; i < MAX_LOGINS_SAVED && pPInfo->last_failed[i]; i++) {
            sprintf(buf, "  %s from %s\n\r", pPInfo->last_failed[i]->in,
                    pPInfo->last_failed[i]->site);
            strcat(buf2, buf);
        }

    page_string(ch->desc, buf2, 1);

    if (fOld) {
        free_player_info(pPInfo);
    }
}

void
update_karma_options(CHAR_DATA * ch, PLAYER_INFO * pPInfo, int prev)
{
    bool awarded = FALSE;
    int i, start, finish;
    unsigned long karmaRaces;
    unsigned long karmaGuilds;
    unsigned long mod;
    char *flags;

    // Record their current settings before we adjust.
    karmaRaces = pPInfo->karmaRaces;
    karmaGuilds = pPInfo->karmaGuilds;

    if (pPInfo->karma > prev) {
        awarded = TRUE;
        start = prev;
        finish = pPInfo->karma;
    } else {
        start = pPInfo->karma;
        finish = prev;
    }

    for (i = start + 1; i <= finish; i++) {
        switch (i) {
        case 0:
            /* If they are being set to 0, assume this is a complete
             * erasure and restore the default settings.
             */
            pPInfo->karmaGuilds = 0;
            pPInfo->karmaRaces = 0;
            MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_DEFAULT);
            MUD_SET_BIT(pPInfo->karmaRaces, KINFO_RACE_DEFAULT);
            break;
        case 1:
            if (awarded) {
                MUD_SET_BIT(pPInfo->karmaRaces, KINFO_RACE_DESERT_ELF);
            } else {
                REMOVE_BIT(pPInfo->karmaRaces, KINFO_RACE_DESERT_ELF);
            }
            break;
        case 2:
            if (awarded) {
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_WATER_CLERIC);
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_STONE_CLERIC);
            } else {
                REMOVE_BIT(pPInfo->karmaGuilds, KINFO_GUILD_WATER_CLERIC);
                REMOVE_BIT(pPInfo->karmaGuilds, KINFO_GUILD_STONE_CLERIC);
            }
            break;
        case 3:
            if (awarded) {
                MUD_SET_BIT(pPInfo->karmaRaces, KINFO_RACE_HALF_GIANT);
            } else {
                REMOVE_BIT(pPInfo->karmaRaces, KINFO_RACE_HALF_GIANT);
            }
            break;
        case 4:
            if (awarded) {
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_SHADOW_CLERIC);
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_FIRE_CLERIC);
            } else {
                REMOVE_BIT(pPInfo->karmaGuilds, KINFO_GUILD_SHADOW_CLERIC);
                REMOVE_BIT(pPInfo->karmaGuilds, KINFO_GUILD_FIRE_CLERIC);
            }
            break;
        case 5:
            if (awarded) {
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_LIGHTNING_CLERIC);
            } else {
                REMOVE_BIT(pPInfo->karmaGuilds, KINFO_GUILD_LIGHTNING_CLERIC);
            }
            break;
        case 6:
            if (awarded) {
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_WIND_CLERIC);
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_VOID_CLERIC);
            } else {
                REMOVE_BIT(pPInfo->karmaGuilds, KINFO_GUILD_WIND_CLERIC);
                REMOVE_BIT(pPInfo->karmaGuilds, KINFO_GUILD_VOID_CLERIC);
            }
            break;
        case 7:
            if (awarded) {
                MUD_SET_BIT(pPInfo->karmaRaces, KINFO_RACE_MUL);
            } else {
                REMOVE_BIT(pPInfo->karmaRaces, KINFO_RACE_MUL);
            }
            break;
        case 8:
            if (awarded) {
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_SORCERER);
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_PSIONICIST);
            } else {
                REMOVE_BIT(pPInfo->karmaGuilds, KINFO_GUILD_SORCERER);
                REMOVE_BIT(pPInfo->karmaGuilds, KINFO_GUILD_PSIONICIST);
            }
            break;
        default:
            break;
        }
    }
    if ((mod = (pPInfo->karmaGuilds | karmaGuilds) ^ karmaGuilds)) {
        flags = show_flags(kinfo_guilds, mod);
        cprintf(ch, "Added: %s\n\r", flags);
        free(flags);
    }
    if ((mod = (pPInfo->karmaGuilds | karmaGuilds) ^ pPInfo->karmaGuilds)) {
        flags = show_flags(kinfo_guilds, mod);
        cprintf(ch, "Removed: %s\n\r", flags);
        free(flags);
    }
    if ((mod = (pPInfo->karmaRaces | karmaRaces) ^ karmaRaces)) {
        flags = show_flags(kinfo_races, mod);
        cprintf(ch, "Added: %s\n\r", flags);
        free(flags);
    }
    if ((mod = (pPInfo->karmaRaces | karmaRaces) ^ pPInfo->karmaRaces)) {
        flags = show_flags(kinfo_races, mod);
        cprintf(ch, "Removed: %s\n\r", flags);
        free(flags);
    }
}

void
add_account_comment(CHAR_DATA * ch, PLAYER_INFO * pPInfo, char *comment)
{
    time_t now;
    struct tm *currentTime;
    char time_str[10];
    now = time(0);
    currentTime = localtime(&now);
    char buf[MAX_STRING_LENGTH];

    strncat(comment, " -", MAX_INPUT_LENGTH);
    if( ch != NULL ) {
       CHAR_DATA *staff = SWITCHED_PLAYER(ch);

       strncat(comment, staff->name, MAX_INPUT_LENGTH);
       strncat(comment, " ", MAX_INPUT_LENGTH);
    }

    sprintf(time_str, "%2d/%02d/%02d", currentTime->tm_mon + 1, currentTime->tm_mday,
            currentTime->tm_year - 100);
    strncat(comment, time_str, MAX_INPUT_LENGTH);

    if (!strcmp(pPInfo->comments, "(null)")) {
        free(pPInfo->comments);
        pPInfo->comments = strdup(comment);
    } else {
        if (strlen(comment) + strlen(pPInfo->comments) + 1 >= MAX_STRING_LENGTH) {
            cprintf(ch, "Comments limited to %d characters, not added.\n\r", MAX_STRING_LENGTH);
            return;
        }

        sprintf(buf, "%s\n%s", pPInfo->comments, comment);
        free(pPInfo->comments);
        pPInfo->comments = strdup(buf);
    }

    if( ch != NULL )
       cprintf(ch, "Comments added.\n\r");
}

void
add_account_kudos(CHAR_DATA * ch, PLAYER_INFO * pPInfo, char *kudos)
{
    time_t now;
    struct tm *currentTime;
    char time_str[10];
    now = time(0);
    currentTime = localtime(&now);
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *staff = SWITCHED_PLAYER(ch);

    strncat(kudos, " -", MAX_INPUT_LENGTH);
    strncat(kudos, staff->name, MAX_INPUT_LENGTH);
    strncat(kudos, " ", MAX_INPUT_LENGTH);
    sprintf(time_str, "%2d/%02d/%02d", currentTime->tm_mon + 1, currentTime->tm_mday,
            currentTime->tm_year - 100);
    strncat(kudos, time_str, MAX_INPUT_LENGTH);

    if (!strcmp(pPInfo->kudos, "(null)")) {
        free(pPInfo->kudos);
        pPInfo->kudos = strdup(kudos);
    } else {
        if (strlen(kudos) + strlen(pPInfo->kudos) + 1 >= MAX_STRING_LENGTH) {
            cprintf(ch, "Kudos limited to %d characters, not added.\n\r", MAX_STRING_LENGTH);
            return;
        }

        sprintf(buf, "%s\n%s", pPInfo->kudos, kudos);
        free(pPInfo->kudos);
        pPInfo->kudos = strdup(buf);
    }
    cprintf(ch, "Kudos added.\n\r");
}

#define CHOOSE_KARMA_GUILDS(pPInfo, specApp) ((specApp) ? (pPInfo)->tempKarmaGuilds : (pPInfo)->karmaGuilds)
void
aset_guild_option(CHAR_DATA *ch, PLAYER_INFO *pPInfo, char *arg, bool specApp) {    
    char buf[MAX_STRING_LENGTH];
/* Grant / Deny a guild to a player's account */
    int guild = -1;
    CHAR_DATA *staff = SWITCHED_PLAYER(ch);

    if (arg[0] == '\0') {
        cprintf(ch, "Syntax: %suild <flag>\n\r<flag> may be ", 
         specApp ? "specAppG" : "g");
        send_to_char("default, all, magicker, none, or one of:\n\r", ch);
        send_flags_to_char(kinfo_guilds, ch);
    } else {
        if (!strncasecmp(arg, "default", strlen(arg)) && IS_ADMIN(staff)) {
            if( specApp ) 
                MUD_SET_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_DEFAULT);
            else 
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_DEFAULT);
            sprintf(buf, "Default %suild set", specApp ? "specAppG" : "g");
            guild = 0;
        } else if (!strncasecmp(arg, "all", strlen(arg)) && IS_ADMIN(staff)) {

            if( specApp )
                MUD_SET_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_ALL);
            else
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_ALL);
            sprintf(buf, "All %suild set", specApp ? "specAppG" : "g");
            guild = 0;
        } else if (!strncasecmp(arg, "magicker", strlen(arg))
                   && IS_ADMIN(staff)) {
            if( specApp )
                MUD_SET_BIT(pPInfo->tempKarmaGuilds, KINFO_GUILD_MAGICKER);
            else
                MUD_SET_BIT(pPInfo->karmaGuilds, KINFO_GUILD_MAGICKER);
            sprintf(buf, "All magick cleric %suild set", specApp ? "specAppG" : "g");
            guild = 0;
        } else if (!strncasecmp(arg, "none", strlen(arg)) && IS_ADMIN(staff)) {
            if( specApp )
                pPInfo->tempKarmaGuilds = 0;
            else
                pPInfo->karmaGuilds = 0;
            sprintf(buf, "All %suild removed", specApp ? "specAppG" : "g");
            guild = 0;
        } else
            guild = get_flag_bit(kinfo_guilds, arg);
        if (guild == 0) {
            /* Yes, we do nothing. This is so we can do "default", "all"
             * and "none" settings properly */
        } else if (guild == -1) {
            cprintf(ch, "Bad flag '%s'.\n\rValid flags are:\n\r", arg);
            send_flags_to_char(kinfo_guilds, ch);
            return;
        } else if (!IS_SET(CHOOSE_KARMA_GUILDS(pPInfo, specApp), guild)
                   && staff->player.level >= kinfo_guilds[get_flag_num(kinfo_guilds, arg)].priv) {
            if( specApp )
                MUD_SET_BIT(pPInfo->tempKarmaGuilds, guild);
            else
                MUD_SET_BIT(pPInfo->karmaGuilds, guild);
            sprintf(buf, "%suild flag %s set", specApp ? "SpecAppG" : "G",
                    kinfo_guilds[get_flag_num(kinfo_guilds, arg)].name);
        } else if (staff->player.level >= kinfo_guilds[get_flag_num(kinfo_guilds, arg)].priv) {
            if( specApp )
                REMOVE_BIT(pPInfo->tempKarmaGuilds, guild);
            else
                REMOVE_BIT(pPInfo->karmaGuilds, guild);
            sprintf(buf, "%suild flag %s removed", specApp ? "SpecAppG" : "G",
                    kinfo_guilds[get_flag_num(kinfo_guilds, arg)].name);
        }
        add_account_comment(ch, pPInfo, buf);
        strcat( buf, ".\n\r");
        send_to_char(buf, ch);
    }
}

#define CHOOSE_KARMA_RACES(pPInfo, specApp) ((specApp) ? (pPInfo)->tempKarmaRaces : (pPInfo)->karmaRaces)
void
aset_race_option(CHAR_DATA *ch, PLAYER_INFO *pPInfo, char *arg, bool specApp) {
    /* Grant / Deny a race to a player's account */
    char buf[MAX_STRING_LENGTH];
    int race = -1;
    CHAR_DATA *staff = SWITCHED_PLAYER(ch);

    if (arg[0] == '\0') {
        cprintf(ch, "Syntax: %sace <flag>\n\r<flag> may be ", specApp ? "specAppR" : "r");
        send_to_char("default, common, all, none or one of:\n\r", ch);
        send_flags_to_char(kinfo_races, ch);
    } else {
        if (!strncasecmp(arg, "default", strlen(arg)) && IS_ADMIN(staff)) {
            if( specApp )
                 MUD_SET_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_DEFAULT);
            else 
                 MUD_SET_BIT(pPInfo->karmaRaces, KINFO_RACE_DEFAULT);
            sprintf(buf, "Default %saces set", specApp ? "specAppR" : "r");
            race = 0;
        } else if (!strncasecmp(arg, "all", strlen(arg)) 
         && IS_ADMIN(staff)) {
            if( specApp )
                 MUD_SET_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_ALL);
            else 
                 MUD_SET_BIT(pPInfo->karmaRaces, KINFO_RACE_ALL);
            sprintf(buf, "All %saces set", specApp ? "specAppR" : "r");
            race = 0;
        } else if (!strncasecmp(arg, "common", strlen(arg)) 
         && IS_ADMIN(staff)) {
            if( specApp )
                 MUD_SET_BIT(pPInfo->tempKarmaRaces, KINFO_RACE_COMMON);
            else 
                 MUD_SET_BIT(pPInfo->karmaRaces, KINFO_RACE_COMMON);
            sprintf(buf, "Common %saces set", specApp ? "specAppR" : "r");
            race = 0;
        } else if (!strncasecmp(arg, "none", strlen(arg)) 
         && IS_ADMIN(staff)) {
            if( specApp )
                 pPInfo->tempKarmaRaces = 0;
            else 
                 pPInfo->karmaRaces = 0;
            sprintf(buf, "All %saces set", specApp ? "specAppR" : "r");
            race = 0;
        } else
            race = get_flag_bit(kinfo_races, arg);
        if (race == 0) {
            /* Yes, we do nothing. This is so we can do "default", "all"
             * and "none" settings properly */
        } else if (race == -1) {
            cprintf(ch, "Bad flag '%s'.\n\rValid flags are:\n\r", arg);
            send_flags_to_char(kinfo_races, ch);
            return;
        } else if (!IS_SET(CHOOSE_KARMA_RACES(pPInfo, specApp), race)
                   && staff->player.level >= kinfo_races[get_flag_num(kinfo_races, arg)].priv) {
            if( specApp )
                 MUD_SET_BIT(pPInfo->tempKarmaRaces, race);
            else 
                 MUD_SET_BIT(pPInfo->karmaRaces, race);
            sprintf(buf, "%sace flag %s set", 
             specApp ? "SpecAppR" : "R",
             kinfo_races[get_flag_num(kinfo_races, arg)].name);
        } else if (staff->player.level >= kinfo_races[get_flag_num(kinfo_races, arg)].priv) {
            if( specApp )
                 REMOVE_BIT(pPInfo->tempKarmaRaces, race);
            else 
                 REMOVE_BIT(pPInfo->karmaRaces, race);
            sprintf(buf, "%sace flag %s removed", 
             specApp ? "SpecAppR" : "R",
              kinfo_races[get_flag_num(kinfo_races, arg)].name);
        }
        add_account_comment(ch, pPInfo, buf);
        strcat(buf, ".\n\r");
        send_to_char(buf, ch);
    }
}   

void
cmd_aset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    PLAYER_INFO *pPInfo;
    DESCRIPTOR_DATA d;
    bool fPlayerInfo = FALSE;
    struct stat stbuf;
    time_t now;
    struct tm *currentTime;
    char time_str[10];
    CHAR_DATA *staff = SWITCHED_PLAYER(ch);

    now = time(0);
    currentTime = localtime(&now);

    argument = one_argument(argument, arg1, sizeof(arg1));
    arg1[0] = toupper(arg1[0]);
    argument = one_argument(argument, arg2, sizeof(arg2));
    strcpy(arg3, argument);
    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Syntax: aset <account> comments <string>\n\r", ch);
        send_to_char("                       kudos <string>\n\r", ch);
        if (IS_ADMIN(staff)) {
            send_to_char("                       karma <number> <reason>\n\r", ch);
            send_to_char("                       flags <flag>\n\r", ch);
            send_to_char("                       guild <flag>\n\r", ch);
            send_to_char("                       race <flag>\n\r", ch);
            send_to_char("                       specAppGuild <flag>\n\r", ch);
            send_to_char("                       specAppRace <flag>\n\r", ch);
            send_to_char("                       chars_allowed <number>\n\r", ch);
            send_to_char("                       email <string>\n\r", ch);
            send_to_char("                       pwdreset\n\r", ch);
        }
        return;
    }

    /* find their pinfo */
    if ((pPInfo = find_player_info(arg1)) == NULL) {
        if ((load_player_info(&d, arg1)) == FALSE) {
            cprintf(ch, "Account '%s' not found.\n\r", arg1);
            return;
        }
        pPInfo = d.player_info;
        d.player_info = NULL;
    } else {
        fPlayerInfo = TRUE;
    }

    if (!strncasecmp(arg2, "guild", strlen(arg2))
     && IS_ADMIN(staff)) {    
        aset_guild_option(ch, pPInfo, arg3, FALSE );
    } else if (!strncasecmp(arg2, "race", strlen(arg2)) 
     && IS_ADMIN(staff)) {
        aset_race_option(ch, pPInfo, arg3, FALSE );
    } else if (!strncasecmp(arg2, "specApp", strlen(arg2))
     && IS_ADMIN(staff)) {
        int racebit = get_flag_bit(kinfo_races, arg3);
        int racenum = get_flag_num(kinfo_races, arg3);
        int guildbit = get_flag_bit(kinfo_guilds, arg3);
        int guildnum = get_flag_num(kinfo_guilds, arg3);

        if( racebit == -1 && guildbit == -1 ) {
            cprintf(ch, 
             "Invalid special app flag '%s', must be a race or guild.\n\r",
             arg3 );
            return;
        }
        else if( racebit != -1 ) {
            if (!IS_SET(pPInfo->tempKarmaRaces, racebit)
             && staff->player.level >= kinfo_races[racenum].priv) {
                MUD_SET_BIT(pPInfo->tempKarmaRaces, racebit);
                cprintf(ch, "Special App Race flag %s set.\n\r", 
                 kinfo_races[racenum].name);
                sprintf( buf, "Special App Race flag %s set",
                 kinfo_races[racenum].name);
                add_account_comment(ch, pPInfo, buf);
            } else if (staff->player.level >= kinfo_races[racenum].priv) {
                REMOVE_BIT(pPInfo->tempKarmaRaces, racebit);
                cprintf(ch, "Special App Race flag %s removed.\n\r",
                    kinfo_races[racenum].name);
                sprintf( buf, "Special App Race flag %s removed",
                 kinfo_races[racenum].name);
                add_account_comment(ch, pPInfo, buf);
            }
            else {
                cprintf(ch, 
                 "You don't have permission to set the '%s' flag.\n\r", 
                 kinfo_races[racenum].name);
                return;
            }
        } else { // was a guild
            if (!IS_SET(pPInfo->tempKarmaGuilds, guildbit)
             && staff->player.level >= kinfo_guilds[guildnum].priv) {
                MUD_SET_BIT(pPInfo->tempKarmaGuilds, guildbit);
                cprintf(ch, "Special App Guild flag %s set.\n\r", 
                 kinfo_guilds[guildnum].name);
                sprintf( buf, "Special App Guild flag %s set",
                 kinfo_guilds[guildnum].name);
                add_account_comment(ch, pPInfo, buf);
            } else if (staff->player.level >= kinfo_guilds[guildnum].priv) {
                REMOVE_BIT(pPInfo->tempKarmaGuilds, guildbit);
                cprintf(ch, "Special App Guild flag %s removed.\n\r",
                    kinfo_guilds[guildnum].name);
                sprintf( buf, "Special App Guild flag %s removed",
                 kinfo_guilds[guildnum].name);
                add_account_comment(ch, pPInfo, buf);
            }
            else {
                cprintf(ch, 
                 "You don't have permission to set the '%s' flag.\n\r", 
                 kinfo_guilds[guildnum].name);
                return;
            }
        }
    } else if (!strncasecmp(arg2, "specAppGuild", strlen(arg2))
      && IS_ADMIN(staff)) {    
        aset_guild_option(ch, pPInfo, arg3, TRUE );
    } else if (!strncasecmp(arg2, "specAppRace", strlen(arg2))
      && IS_ADMIN(staff)) {
        aset_race_option(ch, pPInfo, arg3, TRUE );
    } else if (!strncasecmp(arg2, "flags", strlen(arg2))) {
        int which;
        if (arg3[0] == '\0') {
            cprintf(ch, "Syntax: flag <flag>\n\r<flag> is one of:\n\r");
            send_flags_to_char(pinfo_flags, ch);
        } else {
            which = get_flag_bit(pinfo_flags, arg3);
            if (which == -1) {
                cprintf(ch, "Bad flag '%s'.\n\rValid flags are:\n\r", arg3);
                send_flags_to_char(pinfo_flags, ch);
            } else if (!IS_SET(pPInfo->flags, which)
                       && ch->player.level >= pinfo_flags[get_flag_num(pinfo_flags, arg3)].priv) {
                MUD_SET_BIT(pPInfo->flags, which);
                cprintf(ch, "Flag %s set.\n\r", pinfo_flags[get_flag_num(pinfo_flags, arg3)].name);
            } else if (ch->player.level >= pinfo_flags[get_flag_num(pinfo_flags, arg3)].priv) {
                REMOVE_BIT(pPInfo->flags, which);
                cprintf(ch, "Flag %s removed.\n\r",
                        pinfo_flags[get_flag_num(pinfo_flags, arg3)].name);
            }
        }
    } else if (!strncasecmp(arg2, "comments", strlen(arg2))) {
        if (arg3[0] == '\0') {
            cprintf(ch, "Syntax:  aset <person> comments <string>\n\r");
            return;
        }

        add_account_comment(ch, pPInfo, arg3);
    } else if (!strncasecmp(arg2, "kudos", strlen(arg2))) {
        if (arg3[0] == '\0') {
            cprintf(ch, "Syntax:  aset <person> kudos <string>\n\r");
            return;
        }

        add_account_kudos(ch, pPInfo, arg3);
    } else if (staff->player.level >= STORYTELLER && !strncasecmp(arg2, "karma", strlen(arg2))) {
        int value = atoi(arg3);
        int prev = pPInfo->karma;
        /* get the reason - use one_arg because arg3 was strcpy'd */
        argument = one_argument(argument, arg3, sizeof(arg3));
        if (arg3[0] == '\0' || !is_number(arg3)) {
            cprintf(ch, "Syntax: karma <number> <reason>\n\r");
        } else {
            sprintf(time_str, "%2d/%02d/%02d", currentTime->tm_mon + 1, currentTime->tm_mday,
                    currentTime->tm_year - 100);
            pPInfo->karma = value;
            sprintf(buf, "%s%s set account %s's karma to %d%s%s - %s.\n\r",
                    pPInfo->karmaLog ? pPInfo->karmaLog : "", MSTR(staff, name), pPInfo->name,
                    pPInfo->karma, argument[0] == '\0' ? "" : ", ",
                    argument[0] == '\0' ? "" : argument, time_str);
            if (pPInfo->karmaLog || (pPInfo->karmaLog && !strcmp(pPInfo->karmaLog, "(null)")))
                free(pPInfo->karmaLog);
            pPInfo->karmaLog = strdup(buf);
            // Good place to call something to update the guilds/races
            update_karma_options(ch, pPInfo, prev);
            cprintf(ch, "Karma set.\n\r");
        }
    } else if (IS_ADMIN(staff) 
      && (!strncasecmp(arg2, "chars_allowed", strlen(arg2))
       || !strncasecmp(arg2, "ca", strlen(arg2)))) {
        int value = atoi(arg3);
        if (arg3[0] == '\0' || !is_number(arg3)) {
            cprintf(ch, "Syntax: chars_allowed <number>\n\r");
        } else {
            pPInfo->num_chars_allowed = value;
            cprintf(ch, "# of characters allowed set\n\r");
        }
    } else if (!strncasecmp(arg2, "email", strlen(arg2)) && IS_ADMIN(staff)) {
        if (arg3[0] == '\0') {
            cprintf(ch, "Syntax: email <string>\n\r");
        } else if (strlen(arg3) > 79) {
            cprintf(ch, "Email address must be less than 80 characters.\n\r");
        } else if (strlen(arg3) < 5) {
            cprintf(ch, "Email address must be more than 5 characters.\n\r");
        } else {

            sprintf(buf, "%s/%s", ALT_ACCOUNT_DIR, arg3);
            if (lstat(buf, &stbuf) != -1) {
                cprintf(ch, "That email is already registered to another account.\n\r");
            } else {
                sprintf(buf, "%s/%s/%s", LIB_DIR, ALT_ACCOUNT_DIR, pPInfo->email);
                sprintf(buf2, "%s/%s/%s", LIB_DIR, ALT_ACCOUNT_DIR, arg3);
                rename(buf, buf2);

                /* save the old email for the comment */
                strcpy(buf2, pPInfo->email);
                free(pPInfo->email);
                pPInfo->email = strdup(arg3);
                cprintf(ch, "Email set for account '%s'.\n\r", pPInfo->name);
                sprintf(buf, "Email changed from '%s'", buf2);
                add_account_comment(ch, pPInfo, buf);
                save_player_info(pPInfo);
            }
        }
    } else if (!strncasecmp(arg2, "pwdreset", strlen(arg2))
               && IS_ADMIN(staff)) {
        REMOVE_BIT(pPInfo->flags, PINFO_CONFIRMED_EMAIL);
        scramble_and_mail_account_password(pPInfo, pwdreset_text);
        cprintf(ch, "Password reset for account '%s'.\n\r", pPInfo->name);
        sprintf(buf, "Account password reset.");
        add_account_comment(ch, pPInfo, buf);
        save_player_info(pPInfo);
    } else {
        cprintf(ch, "Unknown option, see \"aset <account>\" for usage info.\n\r");
    }

    save_player_info(pPInfo);
    if (!fPlayerInfo) {
        free_player_info(pPInfo);
    }
}


/* character information */
void
cmd_pinfo(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int num, i;
    int in_game = 1;
    int data_len = 0;
    static const char * const fields[] = {
        "email", "objective", "i1", "i2", "i3", "i4", "i5", "i6", "i7", "i8", "i9", "i10", "i11",
        "i12", "i13", "\n"
    };
    char name[MAX_INPUT_LENGTH], field[MAX_INPUT_LENGTH], data[MAX_INPUT_LENGTH];
    char imm_name[128];
    char time_str[128];
    char buf[MAX_INPUT_LENGTH], account[MAX_INPUT_LENGTH];
    char dir[MAX_STRING_LENGTH];
    time_t now;
    struct tm *currentTime;
    CHAR_DATA *tch = NULL, *staff;
    /*
     * send_to_char( "Currently Disabled...\n\r", ch );
     * return; */
    if (ch == NULL)
        return;

    staff = SWITCHED_PLAYER(ch);

    now = time(0);
    currentTime = localtime(&now);
    argument = one_argument(argument, name, sizeof(name));
    argument = one_argument(argument, account, sizeof(account));
    snprintf(imm_name, sizeof(imm_name), "%s", staff->name);

    /*
     * old search block will search for the field and return -1 if it was
     * invalid, otherwize return the index
     */
    if (-1 != (num = old_search_block(account, 0, strlen(account), fields, 1))) {
        /* found, make it an index its off by one */
        num--;
    }

    if (!*name) {
        send_to_char("Syntax is pinfo <name> <account> {<field> <message>}\n\r"
                     " where field can be email, i1, i2,...i13\n\r"
                     " only overlords can edit the email field\n\r"
                     " or no field & message will view the info\n\r", ch);
        return;
    }

    for (i = 0; i < strlen(name); i++)
        name[i] = LOWER(name[i]);
    if (account[0]) {           /* account/field specified */
        if (num == -1 && (tch = get_char_pc_account(name, account)) != NULL) {
            /* account is not a field && online with name & account */
            argument = one_argument(argument, field, sizeof(field));
            strcpy(data, argument);
            if (-1 != (num = old_search_block(field, 0, strlen(field), fields, 1))) {
                /* found, make it an index its off by one */
                num--;
            }

            /* examine if they gave a field */
            if (field[0] && num == -1) {
                send_to_char("Syntax is pinfo <name> {<field> <message>}\n\r"
                             " where field can be email, i1, i2,...i13\n\r"
                             " only overlords can edit the email field\n\r"
                             " or no field & message will view the info\n\r", ch);
            }
        } else if (num != -1 && (tch = get_char_pc(name)) != NULL) {
            /* account is indeed a field & online with name only */
            strcpy(field, account);     /* switch account into field */
            strcpy(data, argument);     /* copy data */
        } else if (num == -1 && !tch) {
            tch = (CHAR_DATA *) malloc(sizeof *tch);
            clear_char(tch);
            tch->name = name;
            in_game = 0;
            /* account is account, not online */
            sprintf(buf, "%s not found online, checking account.\n\r", capitalize(name));
            send_to_char(buf, ch);
            argument = one_argument(argument, field, sizeof(field));
            strcpy(data, argument);
            if (-1 != (num = old_search_block(field, 0, strlen(field), fields, 1))) {
                /* found, make it an index its off by one */
                num--;
            }

            if (player_exists(name, capitalize(account), -1)) {
                sprintf(dir, "%s/%s", ACCOUNT_DIR, capitalize(account));
                if (!open_char_file(tch, dir)) {
                    gamelogf("cmd_pinfo: Unable to open char file '%s/%s'", account, tch->name);
                    return;
                }

                if (!read_char_file()) {
                    gamelogf("cmd_pinfo: Unable to read char file '%s/%s'", account, name);
                    close_char_file();
                    return;
                }
            } else if (player_exists(name, account, -1)) {
                sprintf(dir, "%s/%s", ACCOUNT_DIR, account);
                if (!open_char_file(tch, dir)) {
                    gamelogf("cmd_pinfo: Unable to open char file '%s/%s'", account, tch->name);
                    return;
                }

                if (!read_char_file()) {
                    gamelogf("cmd_pinfo: Unable to read char file '%s/%s'", account, name);
                    close_char_file();
                    return;
                }
            } else {
                sprintf(buf, "Cannot find character named %s (%s)\n", name, account);
                send_to_char(buf, ch);
                return;
            }
        } else {
            sprintf(buf, "'%s' not found under account '%s'", name, account);
            send_to_char(buf, ch);
            return;
        }
    } else {                    /* account not specified, check if online by name only */

        if (((tch = get_char_pc(name)) != NULL) && CAN_SEE(ch, tch)) {
            field[0] = '\0';
            num = -1;
        } else {
            sprintf(buf, "'%s' not found online, specify an account.\n\r", name);
            send_to_char(buf, ch);
            return;
        }
    }

    if (!*field) {
        time_t time;
        if (!in_game) {
            tch->specials.il = read_char_field_num("CHAR", 1, "INVIS LEVEL");
            time = read_char_field_num("CHAR", 1, "LAST LOGON");
            tch->specials.uid = read_char_field_num("CHAR", 1, "UID");
            read_char_field_f("CHAR", 1, "ACCOUNT", "%a", data);
            tch->account = strdup(data);
            for (i = 0; i < 15; i++) {
                data[0] = '\0';
                sprintf(field, "INFO %d", i);
                read_char_field_f("CHAR", 1, field, "%a", data);
                tch->player.info[i] = strdup(data);
            }

            tch->player.time.starting_time = read_char_field_num("CHAR", 1, "STARTING TIME");
        }

        if (tch->specials.il <= ch->player.level && !in_game) {
            sprintf(buf, "Last On: %s\r", (char *) ctime(&time));
            send_to_char(buf, ch);
        } else if (tch->specials.il <= ch->player.level) {
            send_to_char("Currently Online\n\r", ch);
        }

        if (tch->account) {
            sprintf(buf, "Account: %s\n\r", tch->account);
            send_to_char(buf, ch);
        }

        if (tch->player.time.starting_time) {
            sprintf(buf, "Created: %s\r", ctime(&tch->player.time.starting_time));
            send_to_char(buf, ch);
        }

        for (i = 0; i < 15; i++) {
            sprintf(buf, "%s:%s\n\r", fields[i], tch->player.info[i]);
            send_to_char(buf, ch);
        }

        if (tch->specials.uid > 0) {
            sprintf(buf, "%s:%d\n\r", "UID", tch->specials.uid);
            send_to_char(buf, ch);
        }

        if (!in_game) {
            close_char_file();
            for (i = 0; i < 15; i++)
                free(tch->player.info[i]);
            free(tch->account);
            free(tch);
        }

        return;
    } else {                    /* setting a field */

        if ((num == 0) && !(GET_LEVEL(staff) == OVERLORD)) {
            send_to_char("Only overlords can change the email field\n\r", ch);
            if (!in_game)
                close_char_file();
            return;
        }

        strcat(data, " -");
        strcat(data, imm_name);
        strcat(data, " ");
        snprintf(time_str, sizeof(time_str), "%2d/%02d/%02d", currentTime->tm_mon + 1,
                 currentTime->tm_mday, currentTime->tm_year - 100);
        strcat(data, time_str);
        /* 79 is how long the field can be
         * 8 characters for the timestamp
         * 2 for the leading " -" before the immortal's name
         * 1 for the space between the immortals name and the timestamp */
#define MAX_INFO_FIELD_LEN  256
        data_len = MAX_INFO_FIELD_LEN - 8 - 2 - 1 - strlen(imm_name);
        if (strlen(data) > data_len) {
            snprintf(time_str, sizeof(time_str), "You can only have up to %d characters.\n\r",
                     data_len);
            send_to_char(time_str, ch);
            if (!in_game)
                close_char_file();
            return;
        }

        if (!in_game) {
            sprintf(field, "INFO %d", num);
            write_char_field_f("CHAR", 1, field, "%s", data);
            write_char_file();
        }

        if (in_game) {
            free(tch->player.info[num]);
            tch->player.info[num] = strdup(data);
            /* save them without letting them know they're being saved */
            if (ch->in_room)
                save_char(ch);
#ifdef SAVE_CHAR_OBJS
            save_char_objs(ch);
#endif
        } else {
            close_char_file();
            for (i = 0; i < 15; i++)
                free(tch->player.info[i]);
            free(tch);
        }

        sprintf(buf, "Info%d set.\n\r", num - 1);
        send_to_char(buf, ch);
        return;
    }
}


long vector_from_flags(struct flag_data *flag, char *str);
char *str_from_flags(struct flag_data *flags, long vector);
void
cmd_resurrect(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[MAX_INPUT_LENGTH];
    char account[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    PLAYER_INFO *pPInfo;
    CHAR_INFO *pChInfo;
    bool fOld = FALSE;
    DESCRIPTOR_DATA d;
    CHAR_DATA tch;
    argument = two_arguments(argument, name, sizeof(name), account, sizeof(account));
    strcpy(name, lowercase(name));
    account[0] = toupper(account[0]);
    if (name[0] == '\0' || account[0] == '\0') {
        send_to_char("Syntax: resurrect <character> <account>\n\r", ch);
        return;
    }

    if ((pPInfo = find_player_info(account)) == NULL) {
        if ((fOld = load_player_info(&d, account)) == FALSE) {
            sprintf(buf, "Account '%s' not found.\n\r", account);
            send_to_char(buf, ch);
            return;
        }
        pPInfo = d.player_info;
        d.player_info = NULL;
    }

    if ((pChInfo = lookup_char_info(pPInfo, name)) == NULL) {
        sprintf(buf, "No character '%s' in account '%s'.\n\r", name, account);
        send_to_char(buf, ch);
        return;
    }

    if (pChInfo->status != APPLICATION_DEATH) {
        sprintf(buf, "%s (%s) is not dead yet, hurry up and kill them first!\n\r", name, account);
        send_to_char(buf, ch);
        return;
    }

    tch.name = strdup(name);
    sprintf(buf, "%s/%s", ACCOUNT_DIR, account);
    if (!open_char_file(&tch, buf)) {
        sprintf(buf, "cmd_resurrect: unable to open char '%s' (%s).\n", name, account);
        gamelog(buf);
        free(tch.name);
        return;
    }

    if (!read_char_file()) {
        close_char_file();
        return;
    }

    /* write the new app status */
    write_char_field_f("CHAR", 1, "APPLICATIONSTATUS", "%d", APPLICATION_NONE);
    /* load the flags */
    read_char_field_f("CHAR", 1, "CFLAGS", "%a", buf);
    tch.specials.act = vector_from_flags(char_flag, buf);
    /* remove dead flag */
    REMOVE_BIT(tch.specials.act, CFL_DEAD);
    /* write remainder of flags */
    write_char_field_f("CHAR", 1, "CFLAGS", "%s", str_from_flags(char_flag, tch.specials.act));
    /* set to 1 hitpoint */
    write_char_field_f("CHAR", 1, "HITS", "%d", 1);
    write_char_file();
    close_char_file();

    /* update the character info in player_info */
    update_char_info(pPInfo, name, APPLICATION_NONE);

    /* save the player_info */
    save_player_info(pPInfo);

    /* if we loaded the player_info, free it */
    if (fOld)
        free_player_info(pPInfo);

    name[0] = UPPER(name[0]);
    gamelogf("%s (%s) has been resurrected.", name, account);
    cprintf(ch, "%s (%s) has been resurrected.\n\r", name, account);

    sprintf(buf, "%s comment %s resurrected", account, name);
    cmd_aset(ch, buf, CMD_ASET, 0);
    return;
}

void
cmd_store(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[MAX_INPUT_LENGTH];
    char account[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char reason[MAX_STRING_LENGTH];
    PLAYER_INFO *pPInfo;
    CHAR_INFO *pChInfo;
    bool fOld = FALSE;
    DESCRIPTOR_DATA d;
    CHAR_DATA tch, *vict;
    argument = two_arguments(argument, name, sizeof(name), account, sizeof(account));
    strcpy(name, lowercase(name));
    account[0] = toupper(account[0]);

    if (account[0] == '\0' || name[0] == '\0') {
        send_to_char("Syntax: store <character> <account> [reason]\n\r", ch);
        return;
    }

    if ((pPInfo = find_player_info(account)) == NULL) {
        if ((fOld = load_player_info(&d, account)) == FALSE) {
            cprintf(ch, "Account '%s' not found.\n\r", account);
            return;
        }
        pPInfo = d.player_info;
        d.player_info = NULL;
    }

    if ((pChInfo = lookup_char_info(pPInfo, name)) == NULL) {
        cprintf(ch, "No character '%s' in account '%s'.\n\r", name, account);
        return;
    }

    if (pChInfo->status != APPLICATION_NONE) {
        cprintf(ch, "%s (%s) is not playing, no point in storing them yet!\n\r", name, account);
        return;
    }

    /* reason given ? */
    if (argument[0] != '\0') {
       sprintf(reason, "Stored %s %s", capitalize(name), argument);
    } else {
       sprintf(reason, "Stored %s at player's request.", capitalize(name));
    }

    vict = NULL;
    /* account on-line, is the PC? */
    if (fOld || (vict = get_char_pc_account(name, account)) == NULL) {
        /* if not load the char */
        tch.name = strdup(name);
        sprintf(buf, "%s/%s", ACCOUNT_DIR, account);
        if (!open_char_file(&tch, buf)) {
            sprintf(buf, "cmd_store: unable to open char '%s' (%s).\n", name, account);
            gamelog(buf);
            free(tch.name);
            return;
        }

        if (!read_char_file()) {
            close_char_file();
            return;
        }

        /* write the new app status */
        write_char_field_f("CHAR", 1, "APPLICATIONSTATUS", "%d", APPLICATION_STORED);
        write_char_file();
        close_char_file();
    } else {
        vict->application_status = APPLICATION_STORED;
        save_char(vict);
#ifdef SAVE_CHAR_OBJS
        save_char_objs(vict);
#endif
        send_to_char("You have been put into storage.\n\r", vict);
    }

    /* update the character info in player_info */
    update_char_info(pPInfo, name, APPLICATION_STORED);

    /* add an account comment */
    add_account_comment(ch, pPInfo, reason);

    /* save the player_info */
    save_player_info(pPInfo);

    if (vict && vict->desc) {
        close_socket(vict->desc);
        vict->desc = 0;
        extract_char(vict);
    }

    /* if we loaded the player_info, free it */
    if (fOld)
        free_player_info(pPInfo);
    sprintf(buf, "%s (%s) has been put in storage.", name, account);
    gamelog(buf);
    strcat(buf, "\n\r");
    send_to_char(buf, ch);
    return;
}


void
cmd_unstore(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[MAX_INPUT_LENGTH];
    char account[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char reason[MAX_STRING_LENGTH];
    PLAYER_INFO *pPInfo;
    CHAR_INFO *pChInfo;
    bool fOld = FALSE;
    DESCRIPTOR_DATA d;
    CHAR_DATA tch;
    argument = two_arguments(argument, name, sizeof(name), account, sizeof(account));
    strcpy(name, lowercase(name));
    account[0] = toupper(account[0]);
    if (account[0] == '\0' || name[0] == '\0') {
        send_to_char("Syntax: unstore <character> <account>\n\r", ch);
        return;
    }

    if ((pPInfo = find_player_info(account)) == NULL) {
        if ((fOld = load_player_info(&d, account)) == FALSE) {
            cprintf(ch, "Account '%s' not found.\n\r", account);
            return;
        }
        pPInfo = d.player_info;
        d.player_info = NULL;
    }

    if ((pChInfo = lookup_char_info(pPInfo, name)) == NULL) {
        cprintf(ch, "No character '%s' in account '%s'.\n\r", name, account);
        return;
    }

    if (pChInfo->status != APPLICATION_STORED) {
        cprintf(ch, "%s (%s) is not in storage!\n\r", name, account);
        return;
    }

    if (argument[0] != '\0') {
       sprintf( reason, "Unstored %s %s", capitalize(name), argument);
    } else {
       sprintf( reason, "Unstored %s", capitalize(name));
    }

    tch.name = strdup(name);
    sprintf(buf, "%s/%s", ACCOUNT_DIR, account);
    if (!open_char_file(&tch, buf)) {
        sprintf(buf, "cmd_unstore: unable to open char '%s' (%s).\n", name, account);
        gamelog(buf);
        free(tch.name);
        return;
    }

    if (!read_char_file()) {
        close_char_file();
        return;
    }

    /* write the new app status */
    write_char_field_f("CHAR", 1, "APPLICATIONSTATUS", "%d", APPLICATION_NONE);
    write_char_file();
    close_char_file();
    /* update the character info in player_info */
    update_char_info(pPInfo, name, APPLICATION_NONE);
    add_account_comment(ch, pPInfo, reason);

    /* save the player_info */
    save_player_info(pPInfo);
    /* if we loaded the player_info, free it */
    if (fOld)
        free_player_info(pPInfo);
    sprintf(buf, "%s (%s) has been pulled from storage.", name, account);
    gamelog(buf);
    strcat(buf, "\n\r");
    send_to_char(buf, ch);
    return;
}

void
cmd_cat(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char(TYPO_MSG, ch);
}


/* see ya, wouldn't wanna be ya */
void
cmd_unapprove(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];
    char name[MAX_STRING_LENGTH];
    char account[MAX_STRING_LENGTH];
    CHAR_DATA *vict;
    CHAR_DATA tch;
    PLAYER_INFO *pPInfo;
    CHAR_INFO *pChInfo;
    bool fOld = FALSE;
    DESCRIPTOR_DATA d, *victDesc;
    argument = two_arguments(argument, name, sizeof(name), account, sizeof(account));
    if (account[0] == '\0' && (vict = get_char_vis(ch, name)) != NULL && !IS_NPC(vict)
        && vict->desc) {
        write_to_descriptor(vict->desc->descriptor,
                            "Your character has been unauthorized, possibly meaning you have\n\r"
                            "been authorized by mistake.  Please wait for a new review.\n\r");
        save_char_waiting_auth(vict);
        victDesc = vict->desc;
        sprintf(buf, "%s (%s) has been unauthorized.", GET_NAME(vict),
                vict->desc->player_info->name);
        extract_char(vict);
        /* Disconnect the character from the player, and put them at the
         * game menu 
         victDesc->character = NULL;
         victDesc->connected = CON_SLCT;
         */
        send_to_char("Character unauthorized.\n\r", ch);
        gamelog(buf);
        close_socket(vict->desc);
        return;
    } else if (account[0] == '\0') {
        send_to_char("You must supply an account if they're not online.\n\r", ch);
        return;
    }

    strcpy(name, lowercase(name));
    account[0] = toupper(account[0]);
    if ((pPInfo = find_player_info(account)) == NULL) {
        if ((fOld = load_player_info(&d, account)) == FALSE) {
            sprintf(buf, "Account '%s' not found.\n\r", account);
            send_to_char(buf, ch);
            return;
        }
        pPInfo = d.player_info;
        d.player_info = NULL;
    }

    if ((pChInfo = lookup_char_info(pPInfo, name)) == NULL) {
        sprintf(buf, "No character '%s' in account '%s'.\n\r", name, account);
        send_to_char(buf, ch);
        return;
    }

    if (pChInfo->status == APPLICATION_APPLYING) {
        sprintf(buf, "%s (%s) is already applying.\n\r", name, account);
        send_to_char(buf, ch);
        return;
    }

    tch.name = strdup(name);
    sprintf(buf, "%s/%s", ACCOUNT_DIR, account);
    if (!open_char_file(&tch, buf)) {
        sprintf(buf, "cmd_unapprove: unable to open char '%s' (%s).\n", name, account);
        gamelog(buf);
        free(tch.name);
        return;
    }

    if (!read_char_file()) {
        close_char_file();
        return;
    }

    /* write the new app status */
    write_char_field_f("CHAR", 1, "APPLICATIONSTATUS", "%d", APPLICATION_APPLYING);
    write_char_file();
    close_char_file();
    /* update the character info in player_info */
    update_char_info(pPInfo, name, APPLICATION_APPLYING);
    /* save the player_info */
    save_player_info(pPInfo);
    /* if we loaded the player_info, free it */
    if (fOld)
        free_player_info(pPInfo);
    /* tack account & character name into external file */
    sprintf(buf, "%s %s\n", account, name);
    appendfile("applications", buf);
    sprintf(buf, "%s (%s) has been unauthorized.", name, account);
    gamelog(buf);
    strcat(buf, "\n\r");
    send_to_char(buf, ch);
    return;
}


void
cmd_chardelet(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char("You must type 'chardelete' and no less to delete a character.\n\r", ch);
    return;
}

void
cmd_chardelete(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[MAX_INPUT_LENGTH];
    char account[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    PLAYER_INFO *pPInfo;
    CHAR_INFO *pChInfo;
    bool fOld = FALSE;
    DESCRIPTOR_DATA d;
    CHAR_DATA *vict;
    argument = two_arguments(argument, name, sizeof(name), account, sizeof(account));
    strcpy(name, lowercase(name));
    account[0] = toupper(account[0]);
    if (account[0] == '\0' || name[0] == '\0') {
        send_to_char("Syntax: chardelete <character> <account>\n\r", ch);
        return;
    }

    if ((pPInfo = find_player_info(account)) == NULL) {
        if ((fOld = load_player_info(&d, account)) == FALSE) {
            sprintf(buf, "Account '%s' not found.\n\r", account);
            send_to_char(buf, ch);
            return;
        }
        pPInfo = d.player_info;
        d.player_info = NULL;
    }

    if ((pChInfo = lookup_char_info(pPInfo, name)) == NULL) {
        sprintf(buf, "No character '%s' in account '%s'.\n\r", name, account);
        send_to_char(buf, ch);
        return;
    }

    vict = NULL;
    /* account on-line, is the PC? */
    if (fOld && (vict = get_char_pc_account(name, account)) != NULL) {
        send_to_char("You have been deleted.\n\r", vict);
        parse_command(vict, "quit");
    }

    sprintf(buf, "%s/%s/%s", ACCOUNT_DIR, account, name);
    unlink(buf);
    /* remove the character info in player_info */
    remove_char_info(pPInfo, name);
    /* save the player_info */
    save_player_info(pPInfo);
    /* if we loaded the player_info, free it */
    if (fOld)
        free_player_info(pPInfo);
    sprintf(buf, "%s (%s) has been deleted.", name, account);
    gamelog(buf);
    strcat(buf, "\n\r");
    send_to_char(buf, ch);
    return;
}

/* utility function for cmd_damage. does damage to victim and kills them if 
 * necessary */
void
do_damage(CHAR_DATA * victim, int phys_dam, int stun_dam)
{
    char buf[256];
    generic_damage_immortal(victim, phys_dam, 0, 0, stun_dam);

    if (GET_POS(victim) == POSITION_DEAD) {
        stop_all_fighting(victim);
        sprintf(buf, "%s %s%s%shas died from immortal fury in room #%d, come revel in your power.",
                IS_NPC(victim) ? MSTR(victim, short_descr) : GET_NAME(victim),
                !IS_NPC(victim) ? "(" : "", !IS_NPC(victim) ? victim->account : "",
                !IS_NPC(victim) ? ") " : "", victim->in_room->number);
        if (!IS_NPC(victim)) {
            struct time_info_data playing_time =
                real_time_passed((time(0) - victim->player.time.logon) + victim->player.time.played,
                                 0);
            if ((playing_time.day > 0 || playing_time.hours > 2
                 || (victim->player.dead && !strcmp(victim->player.dead, "rebirth")))
                && !IS_SET(victim->specials.act, CFL_NO_DEATH)) {
                if (victim->player.info[1])
                    free(victim->player.info[1]);
                victim->player.info[1] = strdup(buf);
            }
            death_log(buf);
        } else
            shhlog(buf);
        die(victim);
    } else if (GET_POS(victim) <= POSITION_SLEEPING) {
        stop_all_fighting(victim);
    }
}




/* Plagarized from echo_all, -Nessalin 5/1/2004 */

void
damage_room(CHAR_DATA * ch, int phys_dam, int stun_dam, int mana_dam, int stam_dam)
{
    struct char_data *victim, *next = NULL;
    for (victim = ch->in_room->people; victim; victim = next)
        if (victim && victim->in_room) {
            next = victim->next_in_room;
            if (!IS_IMMORTAL(victim)) {
                adjust_mana(victim, -mana_dam);
                adjust_move(victim, -stam_dam);
                do_damage(victim, phys_dam, stun_dam);
            }
        }
}

/* Plagarized from echo_all, -Nessalin 5/1/2004 */

void
damage_all(CHAR_DATA * ch, int phys_dam, int stun_dam, int mana_dam, int stam_dam)
{
    struct char_data *victim, *next = NULL;
    for (victim = character_list; victim; victim = next)
        if (victim && victim->in_room) {
            next = victim->next;
            if (!IS_IMMORTAL(victim)) {
                adjust_mana(victim, -mana_dam);
                adjust_move(victim, -stam_dam);
                do_damage(victim, phys_dam, stun_dam);
            }
        }
}

/* creator/above cmd to take damage away from a mortal */
void
cmd_damage(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int phys_dam = 0, stun_dam = 0, mana_dam = 0, stam_dam = 0;
    char char_name[256];
    char buf[256];
    char arg1[256], arg2[256], arg3[256], arg4[256];
    char real_name_buf[256];
    CHAR_DATA *victim;
    if (!*argument) {
        send_to_char
            ("Usage: damage <name>|all <hit points> [<stun points>] [<mana points>] [<stamina points>]\n\r",
             ch);
        return;
    }

    argument = one_argument(argument, char_name, sizeof(char_name));
    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));
    argument = one_argument(argument, arg4, sizeof(arg4));
    if (!*arg1 || !is_number(arg1)) {
        send_to_char
            ("Usage: damage <name>|all <hit points> [<stun points>] [<mana points>] [<stamina points>]\n\r",
             ch);
        return;
    }

    phys_dam = atoi(arg1);
    if (phys_dam < 0) {
        send_to_char("You cannot do negative hit point damage.\n\r", ch);
        return;
    }
    if (is_number(arg2))
        stun_dam = atoi(arg2);
    if (stun_dam < 0) {
        send_to_char("You cannot do negative stun point damage.\n\r", ch);
        return;
    }
    if (is_number(arg3))
        mana_dam = atoi(arg3);
    if (mana_dam < 0) {
        send_to_char("You cannot do negative mana point damage.\n\r", ch);
        return;
    }
    if (is_number(arg4))
        stam_dam = atoi(arg4);
    if (stam_dam < 0) {
        send_to_char("You cannot do negative stamina point damage.\n\r", ch);
        return;
    }


    if (!(stricmp(char_name, "all"))) {
        if (ch->player.level < 4) {
            send_to_char("Only HL+ staff members can damage 'all'.\n\r", ch);
            return;
        }
        send_to_char("You have taken ", ch);
        sprintf(buf,
                "%d hps, %d stun, %d mana, and %d movement points "
                "from everyone in the game.\n\r", phys_dam, stun_dam, mana_dam, stam_dam);
        send_to_char(buf, ch);
        damage_all(ch, phys_dam, stun_dam, mana_dam, stam_dam);
        return;
    }


    if (!(stricmp(char_name, "room"))) {
        send_to_char("You have taken ", ch);
        sprintf(buf,
                "%d hps, %d stun, %d mana, and %d movement points "
                "from everyone in the room.\n\r", phys_dam, stun_dam, mana_dam, stam_dam);
        send_to_char(buf, ch);
        damage_room(ch, phys_dam, stun_dam, mana_dam, stam_dam);
        return;
    }


    if (!(victim = get_char_room_vis(ch, char_name)))
        send_to_char("That person isn't here.\n\r", ch);
    else if ((GET_LEVEL(victim) > MORTAL) && (!IS_NPC(victim)))
        send_to_char("Damage doesn't work against immortals.\n\r", ch);
    else {
        send_to_char("You have taken ", ch);
        sprintf(buf, "%d hps, %d stun, %d mana, and %d movement points from %s.\n\r", phys_dam,
                stun_dam, mana_dam, stam_dam, REAL_NAME(victim));
        send_to_char(buf, ch);
        do_damage(victim, phys_dam, stun_dam);
        adjust_mana(victim, -mana_dam);
        adjust_move(victim, -stam_dam);
    }
    return;
}

void
cmd_heal(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int phys_dam = 0, stun_dam = 0, mana_dam = 0, stam_dam = 0;
    char char_name[256];
    char buf[256];
    char arg1[256], arg2[256], arg3[256], arg4[256];
    char real_name_buf[256];
    CHAR_DATA *victim;
    if (!*argument) {
        send_to_char
            ("Usage: heal <name> <hit points> [<stun points>] [<mana points>] [<stamina points>]\n\r",
             ch);
        return;
    }

    argument = one_argument(argument, char_name, sizeof(char_name));
    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));
    argument = one_argument(argument, arg4, sizeof(arg4));
    if (!*arg1 || !is_number(arg1)) {
        send_to_char
            ("Usage: heal <name> <hit points> [<stun points>] [<mana points>] [<stamina points>]\n\r",
             ch);
        return;
    }

    phys_dam = atoi(arg1);
    if (is_number(arg2))
        stun_dam = atoi(arg2);
    if (is_number(arg3))
        mana_dam = atoi(arg3);
    if (is_number(arg4))
        stam_dam = atoi(arg4);
    if ((phys_dam < 0) || (stun_dam < 0) || (mana_dam < 0) || (stam_dam < 0)) {
        send_to_char("Healing amount must be greater than 0.\n\r", ch);
        return;
    }

    if (!(victim = get_char_room_vis(ch, char_name)))
        send_to_char("That person isn't here.\n\r", ch);
    else if ((GET_LEVEL(victim) > MORTAL) && (!IS_NPC(victim)))
        send_to_char("Heal doesn't work on immortals.\n\r", ch);
    else {
        send_to_char("You have given ", ch);
        sprintf(buf, "%d hps, %d stun, %d mana, and %d movement points " "to %s.\n\r", phys_dam,
                stun_dam, mana_dam, stam_dam, REAL_NAME(victim));
        send_to_char(buf, ch);
        adjust_hit(victim, phys_dam);
        adjust_stun(victim, stun_dam);
        adjust_mana(victim, mana_dam);
        adjust_move(victim, stam_dam);
    }
    update_pos(victim);
    return;
}


/*
 * Uses new account structure
 * Uses old mail system to send a comment to a player about their RP
 * Will be an anonymous message and the players can decide to take it or not
 */

void
cmd_rpcomment(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *sch, *vict = NULL;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char *tmstr;
    long ct;
    int i;
    FILE *fp;

    sch = SWITCHED_PLAYER(ch);

    if (!IS_NPC(sch) && !IS_IMMORTAL(sch)) {
        send_to_char("What?\n\r", ch);
        return;
    }

    if (!*argument) {
        send_to_char("Syntax: comment <character> <account>\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    for (i = 0; arg1[i] != '\0'; i++)
        arg1[i] = LOWER(arg1[i]);
    if (arg2[0] != '\0' && !player_exists(arg1, arg2, APPLICATION_NONE)) {
        if ((vict = get_char(arg1)) == NULL) {
            sprintf(buf, "Can't find anyone named '%s'", arg1);
            send_to_char(buf, ch);
            return;
        }
    } else if (arg2[0] == '\0' && (vict = get_char(arg1)) == NULL) {
        send_to_char("You must supply an account if the character is not"
                     " online.\n\rUse 'pfind' to search for accounts with matching player"
                     " names.\n\r", ch);
        return;
    }

    if (vict) {
        /* added check, vict->name can be null pointer on NPCs */
        if (!vict->name) {
            send_to_char("That vict has no name!(probably an NPC)\n\r", ch);
            return;
        };
        strcpy(arg1, vict->name);
        if (vict->desc)
            strcpy(arg2, vict->desc->player_info->name);
        else if (!arg2[0]) {
            DESCRIPTOR_DATA d;
            /* hope their email is correct on the char */
            if (!load_player_info(&d, vict->player.info[0])) {
                send_to_char("Error, please specify account.\n\r", ch);
                return;
            }

            if (d.player_info)
                strcpy(arg2, d.player_info->name);
            free_player_info(d.player_info);
        }
    }

    ct = time(0);
    tmstr = (char *) asctime(localtime(&ct));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    sprintf(buf, "accountmail/%s/%s", capitalize(arg2), sch->name);
    if (!(fp = fopen(buf, "a"))) {
        send_to_char("Unable to open mail file.\n\r", ch);
        return;
    }

    ch->desc->editing = fp;
    ch->desc->editing_filename = strdup(buf);
    /* intentionally putting the Comment from ... on the file, you
     * can't edit it out
     */
    CREATE(ch->desc->editing_header, char, MAX_STRING_LENGTH);
    sprintf(ch->desc->editing_header, "\nComment from %s for '%s'\nReceived on %s:\n", sch->name,
            capitalize(arg1), tmstr);
    CREATE(ch->desc->tmp_str, char, MAX_STRING_LENGTH);
    if (*argument) {
        sprintf(ch->desc->tmp_str, "%s\n", argument);
        string_append(ch, &ch->desc->tmp_str, MAX_STRING_LENGTH);
    } else {
        ch->desc->tmp_str[0] = '\0';
        string_edit(ch, &ch->desc->tmp_str, MAX_STRING_LENGTH);
    }
}


#define COMMAND_UNKNOWN -1

bool
has_extra_cmd(EXTRA_CMD * commands, Command cmd, int level, bool granted)
{
    EXTRA_CMD *pCmd;
    if (!commands)
        return FALSE;
    for (pCmd = commands; pCmd; pCmd = pCmd->next)
        if (pCmd->cmd == cmd && ((granted && pCmd->level >= level) || (!granted)))
            return TRUE;
    return FALSE;
}


int
granted_cmd_level(EXTRA_CMD * commands, int cmd)
{
    EXTRA_CMD *pCmd;
    if (!commands)
        return FALSE;
    for (pCmd = commands; pCmd; pCmd = pCmd->next)
        if (pCmd->cmd == cmd)
            return pCmd->level;
    return -1;
}


/*
 * cgive - command give
 * give a command you have access to to another player
 */
void
cmd_cgive(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[MAX_STRING_LENGTH];
    char command[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    EXTRA_CMD *pCmd;
    CHAR_DATA *vict, *staff;
    int level = -1;
    int cmd_no;

    staff = SWITCHED_PLAYER(ch);

    argument = one_argument(argument, name, sizeof(name));
    argument = one_argument(argument, command, sizeof(command));
    argument = one_argument(argument, buf, sizeof(buf));
    if (name[0] == '\0') {
        send_to_char("Give to who?\n\r", ch);
        return;
    }

    if ((vict = get_char_room_vis(ch, name)) == NULL) {
        send_to_char("You can't find anyone like that here.\n\r", ch);
        return;
    }

    if (IS_NPC(vict)) {
        send_to_char("Not on NPC's\n\r", ch);
        return;
    }

    if (command[0] == '\0') {
        sprintf(buf, "%s has the following commands granted to them:\n\r", MSTR(vict, name));
        send_to_char(buf, ch);
        for (pCmd = vict->granted_commands; pCmd; pCmd = pCmd->next) {
            sprintf(buf, "%s - %d\n\r", command_name[pCmd->cmd], pCmd->level);
            send_to_char(buf, ch);
        }

        if (!vict->granted_commands)
            send_to_char("None\n\r", ch);
        return;
    }

    if ((cmd_no = get_command(command)) == COMMAND_UNKNOWN) {
        send_to_char("No such command.\n\r", ch);
        return;
    }

    if (GET_LEVEL(staff) < cmd_info[cmd_no].minimum_level) {
        send_to_char("No such command.\n\r", ch);
        return;
    }

    if (has_extra_cmd(vict->granted_commands, cmd_no, -1, TRUE)) {
        if (vict->granted_commands->cmd == cmd_no) {
            pCmd = vict->granted_commands;
            vict->granted_commands = vict->granted_commands->next;
            free(pCmd);
        } else {
            EXTRA_CMD *pNext;
            for (pCmd = vict->granted_commands, pNext = pCmd->next; pNext; pCmd = pNext) {
                pNext = pCmd->next;
                if (pNext->cmd == cmd_no) {
                    pCmd->next = pNext->next;
                    free(pNext);
                    break;
                }
            }
        }
        send_to_char("Granted command removed.\n\r", ch);
        return;
    } else {
        if (buf[0] == '\0' && !is_number(buf)) {
            level = cmd_info[cmd_no].minimum_level;
        } else {
            level = atoi(buf);
        }

        if (GET_LEVEL(staff) < level) {
            send_to_char("You can't give it at a level higher than your own.\n\r", ch);
            sprintf(buf, "%s tried to give %s at %d, he's %d.", MSTR(staff, name), command, level,
                    GET_LEVEL(staff));
            gamelog(buf);
            return;
        }

        CREATE(pCmd, EXTRA_CMD, 1);
        pCmd->cmd = cmd_no;
        pCmd->level = level;
        pCmd->next = vict->granted_commands;
        vict->granted_commands = pCmd;
        sprintf(buf, "Command '%s' granted to %s at level %d.\n\r", command_name[cmd_no],
                MSTR(vict, name), level);
        send_to_char(buf, ch);
    }
    return;
}


void
cmd_crevoke(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[MAX_STRING_LENGTH];
    char command[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    EXTRA_CMD *pCmd;
    CHAR_DATA *vict, *staff;
    int cmd_no;

    staff = SWITCHED_PLAYER(ch);
 
    argument = one_argument(argument, name, sizeof(name));
    argument = one_argument(argument, command, sizeof(command));
    argument = one_argument(argument, buf, sizeof(buf));
    if (name[0] == '\0') {
        send_to_char("Revoke from whom?\n\r", ch);
        return;
    }

    if ((vict = get_char_room_vis(ch, name)) == NULL) {
        send_to_char("You can't find anyone like that here.\n\r", ch);
        return;
    }

    if (command[0] == '\0') {
        sprintf(buf, "%s has the following commands revoked from them:\n\r", MSTR(vict, name));
        send_to_char(buf, ch);
        for (pCmd = vict->revoked_commands; pCmd; pCmd = pCmd->next) {
            sprintf(buf, "%s\n\r", command_name[pCmd->cmd]);
            send_to_char(buf, ch);
        }

        if (!vict->revoked_commands)
            send_to_char("None\n\r", ch);
        return;
    }

    if ((cmd_no = get_command(command)) == COMMAND_UNKNOWN) {
        send_to_char("No such command.\n\r", ch);
        return;
    }

    if (GET_LEVEL(staff) < cmd_info[cmd_no].minimum_level) {
        send_to_char("No such command.\n\r", ch);
        return;
    }

    if (has_extra_cmd(vict->revoked_commands, cmd_no, -1, FALSE)) {
        if (vict->revoked_commands->cmd == cmd_no) {
            pCmd = vict->revoked_commands;
            vict->revoked_commands = vict->revoked_commands->next;
            free(pCmd);
        } else {
            EXTRA_CMD *pNext;
            for (pCmd = vict->revoked_commands, pNext = pCmd->next; pNext; pCmd = pNext) {
                pNext = pCmd->next;
                if (pNext->cmd == cmd_no) {
                    pCmd->next = pNext->next;
                    free(pNext);
                    break;
                }
            }
        }
        send_to_char("Revoked command removed.\n\r", ch);
        return;
    } else {
        CREATE(pCmd, EXTRA_CMD, 1);
        pCmd->cmd = cmd_no;
        pCmd->level = -1;
        pCmd->next = vict->revoked_commands;
        vict->revoked_commands = pCmd;
        sprintf(buf, "Command '%s' revoked from %s.\n\r", command_name[cmd_no], MSTR(vict, name));
        send_to_char(buf, ch);
    }
    return;
}


/* Switch into another (perhaps live player) and execute a command,
 * then switch back  (do as)
 */
void
cmd_doas(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[MAX_STRING_LENGTH];
    char comm[MAX_STRING_LENGTH];
    char orig_comm[MAX_STRING_LENGTH];
    CHAR_DATA *victim, *staff;
    DESCRIPTOR_DATA *orig;

    staff = SWITCHED_PLAYER(ch);

    argument = one_argument(argument, arg, sizeof(arg));
    strcpy(orig_comm, argument);
    argument = one_argument(argument, comm, sizeof(comm));
    if (arg[0] == '\0') {
        send_to_char("Do what as who?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(staff, arg)) == NULL 
     || !CAN_SEE(staff, victim)) {
        send_to_char("You can't find anyone like that.\n\r", ch);
        return;
    }

    if (victim == ch) {
        parse_command(ch, orig_comm);
        send_to_char("Ok.\n\r", ch);
        return;
    }

    if (!IS_NPC(victim) && !IS_ADMIN(staff)) {
        send_to_char("NPC's only for you!\n\r", ch);
        return;
    }

    if (victim->player.level > GET_LEVEL(staff)) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (comm[0] == '\0') {
        send_to_char("What do you want to do as them?\n\r", ch);
        return;
    }

    // Handle potentially aliased commands$
    if (get_alias(victim, comm)) {
        one_argument(get_alias(victim, comm), comm, sizeof(comm));
    }

    //  menu prompt after someone quits out.$
    int cmd_num = get_command(comm);

    switch(cmd_num) {
    case CMD_QUIT:
        send_to_char("Sorry, no quitting with doas.\n\r", ch);
        return;
    case CMD_SAVE:
        send_to_char("Use force -s if you want to make them save.\n\r", ch);
        return;
    case CMD_RETURN:
        send_to_char("Use force -s if you want to make them return.\n\r", ch);
        return;
    case CMD_SWITCH:
        send_to_char("Sorry, no switching with doas.\n\r", ch);
        return;
    case CMD_DOAS:
        send_to_char("Sorry, no doas with doas.\n\r", ch);
        return;
    }
 
    orig = victim->desc;
    victim->desc = ch->desc;
    ch->desc = NULL;
    parse_command(victim, orig_comm);
    ch->desc = victim->desc;
    victim->desc = orig;
    send_to_char("Ok.\n\r", ch);
    return;
}


void
cmd_wizlock(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    for (; isspace(*argument); argument++);
    /* lock perf */
    if (argument && argument[0] == 'p') {
        if (should_do_perf_logging) {
            send_to_char("Performance logging stopped.\n\r", ch);
            gamelog("Performance logging stopped.");
        } else {
            send_to_char("Performance logging started.\n\r", ch);
            gamelog("Performance logging started.");
        }

        should_do_perf_logging = !should_do_perf_logging;
        return;
    }
    /* lock arena */
    if (argument && argument[0] == 'a') {
        if (lock_arena) {
            send_to_char("Unlocking arena viewing.\n\r", ch);
            gamelog("Arena Games open for viewing.");
        } else {
            send_to_char("Locking arena viewing.\n\r", ch);
            gamelog("Arena Games closed for viewing.");
        }

        lock_arena = !lock_arena;
        return;
    }
    /* lock new */
    if (argument && argument[0] == 'n') {
        if (lock_new_char) {
            send_to_char("Unlocking new characters.\n\r", ch);
            gamelog("New characters allowed to be created.");
        } else {
            send_to_char("Locking new characters.\n\r", ch);
            gamelog("New characters not allowed to be created.");
        }

        lock_new_char = !lock_new_char;
        return;
    }

    if (lock_mortal) {
        send_to_char("Unlocking mortal characters.\n\r", ch);
        gamelog("Mortals allowed into the game.");
    } else {
        send_to_char("Locking mortal characters.\n\r", ch);
        gamelog("Mortals no longer allowed into the game.");
    }

    lock_mortal = !lock_mortal;
    return;
}


char *
show_city(int ct)
{
    static char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int commod;
    ROOM_DATA *hroom;
    if (ct < 0 || ct > MAX_CITIES)
        return NULL;
    buf[0] = '\0';
    sprintf(buf2, "%2d:  %-20s        Of Name: %-20s\n\r", ct, city[ct].name, city[ct].of_city);
    strcat(buf, buf2);
    if (city[ct].c_flag) {
        int flag;
        for (flag = 0; char_flag[flag].name[0] != '\0'; flag++)
            if (char_flag[flag].bit == city[ct].c_flag)
                break;
        sprintf(buf2, "Criminal Flag: %-15s   ", char_flag[flag].name);
        strcat(buf, buf2);
    }

    if (city[ct].tribe > 0 && city[ct].tribe < MAX_CLAN) {
        sprintf(buf2, "Tribe: %s", clan_table[city[ct].tribe].name);
    } else if (city[ct].tribe) {
        sprintf(buf2, "Tribe: %d", city[ct].tribe);
    }
    strcat(buf, buf2);
    if (city[ct].c_flag || city[ct].tribe) {
        strcat(buf, "\n\r");
    }

    if (city[ct].room > 0 && (hroom = get_room_num(city[ct].room)) != NULL) {
        sprintf(buf2, "Homeroom: %s (%d)\n\r", hroom->name, city[ct].room);
    } else {
        sprintf(buf2, "Homeroom: <NOT A ROOM> (%d)\n\r", city[ct].room);
    }
    strcat(buf, buf2);
    for (commod = 0; commodities[commod].name[0] != '\0'; commod++) {
        if (commodities[commod].val != COMMOD_NONE) {
            sprintf(buf2, "Tax %-10s %3.0f%%\n\r", commodities[commod].name,
                    (city[ct].price[commodities[commod].val] * 100.0f) - 100.0f);
            strcat(buf, buf2);
        }
    }

    return buf;
}


void
cmd_citystat(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH * 16];
    char buf2[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH] = "";
    char arg2[MAX_INPUT_LENGTH] = "";
    char old_args[MAX_INPUT_LENGTH];
    int ct = 0;
    while (isspace(*argument))
        argument++;
    strcpy(old_args, argument);
    argument = one_argument(argument, arg, sizeof(arg));
    if (arg[0] != '\0') {
        if (is_number(arg))
            ct = atoi(arg);
        else
            for (ct = 0; ct < MAX_CITIES; ct++)
                if (!str_prefix(old_args, city[ct].name))
                    break;
        if (ct <= 0 || ct >= MAX_CITIES) {
            send_to_char("No such city.\n\r", ch);
            return;
        }
    }

    argument = one_argument(argument, arg2, sizeof(arg2));
    buf[0] = '\0';
    if (!ct)
        for (ct = 1; ct < MAX_CITIES; ct++) {
            sprintf(buf2, "City %2d: %-20s\n\r", ct, city[ct].name);
            strcat(buf, buf2);
        }
    else
        strcat(buf, show_city(ct));
    page_string(ch->desc, buf, 1);
    return;
}


void
cmd_cityset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char name[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    int ct, commod, loc, price;
    const char * const field[] = {
        "crim_flag", "name", "of", "tax", "tribe", "homeroom", "\n"
    };
    char *desc[] = {
        "(flag to be set on criminals in this city)", "(Name of the city)",
        "(Of name for the city ie: Allanaki)", "(taxed commodities)",
        "(governing tribe for the city)", "(room newbie's repop to for the city)", "\n"
    };
    argument = one_argument(argument, arg1, sizeof(arg1));      /* city */
    argument = one_argument(argument, arg2, sizeof(arg2));      /* what to set */
    strcpy(name, argument);     /* save the name/tribe/of_name */
    argument = one_argument(argument, arg3, sizeof(arg3));      /* commodity name for tax */
    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Syntax: cityset <city> <field> <value>\n\r", ch);
        send_to_char("<field> is one of:\n\r", ch);
        for (ct = 0; *field[ct] != '\n'; ct++) {
            sprintf(buf, "  %-15s%s\n\r", field[ct], desc[ct]);
            send_to_char(buf, ch);
        }
        return;
    }

    if (is_number(arg1))
        ct = atoi(arg1);
    else
        for (ct = 1; ct < MAX_CITIES; ct++)
            if (!str_prefix(arg1, city[ct].name))
                break;
    if (ct < 1 || ct >= MAX_CITIES) {
        send_to_char("No such city.\n\r", ch);
        return;
    }

    switch ((loc = old_search_block(arg2, 0, strlen(arg2), field, 0))) {
    case 1:                    /* crim_flag */
        if (arg3[0] == '\0') {
            int shown = 0;
            send_to_char("Valid crim_flags are:\n\r", ch);
            for (commod = 0; char_flag[commod].name[0] != '\0'; commod++)
                if (!str_prefix("crim", char_flag[commod].name)) {
                    shown++;
                    sprintf(buf, "   %-15s%s", char_flag[commod].name,
                            shown % 3 == 0 ? "\n\r" : "");
                    send_to_char(buf, ch);
                }

            if (shown % 3 != 0)
                send_to_char("\n\r", ch);
            return;
        }

        if (str_prefix("crim", arg3)) {
            send_to_char("Crim_flag must start with 'crim'.\n\r", ch);
            return;
        }

        if ((commod = get_flag_num(char_flag, arg3)) == -1) {
            send_to_char("No such crim_flag\n\r", ch);
            return;
        }

        city[ct].c_flag = char_flag[commod].bit;
        sprintf(buf, "Crim_flag set to %s.\n\r", char_flag[commod].name);
        send_to_char(buf, ch);
        break;
    case 2:                    /* name */
        if (arg3[0] == '\0') {
            send_to_char("Syntax: cityset <city> name <name>\n\r", ch);
            return;
        }

        if (city[ct].name)
            free(city[ct].name);
        city[ct].name = strdup(name);
        sprintf(buf, "City %d's name changed to '%s'.\n\r", ct, name);
        send_to_char(buf, ch);
        break;
    case 3:                    /* of */
        if (arg3[0] == '\0') {
            send_to_char("Syntax: cityset <city> of <name>\n\r", ch);
            return;
        }

        if (city[ct].of_city)
            free(city[ct].of_city);
        city[ct].of_city = strdup(name);
        sprintf(buf, "City %d's of name changed to '%s'.\n\r", ct, name);
        send_to_char(buf, ch);
        break;
    case 4:                    /* tax */

        strcpy(arg2, arg3);
        argument = one_argument(argument, arg3, sizeof(arg3));
        if (arg2[0] == '\0' || arg3[0] == '\0') {
            send_to_char("Syntax: cityset <city> tax <commodity> <%>\n\r", ch);
            send_to_char("Commodity is one of:\n\r", ch);
            send_attribs_to_char(commodities, ch);
            return;
        }

        if ((commod = get_attrib_num(commodities, arg2)) == -1) {
            send_to_char("Commodity is one of:\n\r", ch);
            send_attribs_to_char(commodities, ch);
            return;
        }

        if (arg3[strlen(arg3) - 1] == '%')
            arg3[strlen(arg3) - 1] = '\0';
        if (!is_number(arg3)) {
            send_to_char("Syntax: cityset <city> tax <commodity> <%>\n\r", ch);
            send_to_char("<%> must be a number.\n\r", ch);
            return;
        }

        price = atoi(arg3);
        /* convert % to float */
        city[ct].price[commodities[commod].val] = (((float) price) + 100.0f) / 100.0f;
        sprintf(buf, "City %d's tax for %s set to %.0f%%.\n\r", ct, commodities[commod].name,
                (city[ct].price[commodities[commod].val] * 100.0f) - 100.0f);
        send_to_char(buf, ch);
        break;
    case 5:                    /* tribe */
        if (arg3[0] == '\0') {
            send_to_char("Syntax: cityset <city> tribe <tribe name>\n\r", ch);
            return;
        }

        if (is_number(arg3))
            commod = atoi(arg3);
        else
            commod = lookup_clan_by_name(name);

        if (commod < 0 || commod >= MAX_CLAN) {
            send_to_char("No such tribe.\n\r", ch);
            return;
        }

        city[ct].tribe = commod;
        if (commod > 0 && commod < MAX_CLAN)
            sprintf(buf, "City %d's tribe set to %s.\n\r", ct, clan_table[commod].name);
        else
            sprintf(buf, "City %d's tribe set to %d.\n\r", ct, commod);
        send_to_char(buf, ch);
        break;
    case 6:                    /* homeroom */
        if (arg3[0] == '\0' || !is_number(arg3)) {
            send_to_char("Syntax: cityset <city> homeroom <room number>\n\r", ch);
            return;
        }

        city[ct].room = atoi(arg3);
        sprintf(buf, "City %d's homeroom changed to '%d'.\n\r", ct, city[ct].room);
        send_to_char(buf, ch);
        break;
    default:
        send_to_char("Invalid choice.\n\r", ch);
        return;
    }

    save_cities();
    return;
}


void
cmd_send(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *sCh, *tCh;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    /* to start, use ch as the sender */
    sCh = ch;
    /* if they're switched */
    if (ch->desc && ch->desc->original && IS_IMMORTAL(ch->desc->original)) {
        /* use the original as the sending char */
        sCh = ch->desc->original;
    } else if (!IS_IMMORTAL(ch)) {
        /* send unknown command message to mortals */
        send_to_char("What?\n\r", ch);
        return;
    }

    /* pull off one argument */
    argument = one_argument(argument, arg, sizeof(arg));
    /* if they didn't specify any arguments, give a syntax */
    if (arg[0] == '\0') {
        send_to_char("Syntax:  send <name> <message>\n\r", ch);
        return;
    }

    /* find a target */
    tCh = get_char_world(ch, arg);
    /* did we find a target? */
    if (tCh == NULL || !CAN_SEE(sCh, tCh)) {
        sprintf(buf, "Cannot find '%s' in the world.\n\r", arg);
        send_to_char(buf, ch);
        return;
    }

    /* did they specify a message? */
    if (*argument == '\0') {
        send_to_char("What message do you want to send to them?\n\r", ch);
        return;
    }

    if (IS_CREATING(tCh)) {
        act("$E's creating right now, leave $M alone.", FALSE, ch, 0, tCh, TO_CHAR);
        return;
    }

    if (!tCh->desc) {
        DESCRIPTOR_DATA *d;
        for (d = descriptor_list; d; d = d->next) {
            if (d->original == tCh) {
                snprintf(buf, sizeof(buf), "You send to %s [M%d]:\n\r     \"%s\"\n\r",
                         PERS(sCh, d->original), npc_index[d->character->nr].vnum, argument);
                send_to_char(buf, sCh);
                if (ch == sCh) {
                    sprintf(buf, "%s sends:\n\r     \"%s\"\n\r", capitalize(PERS(tCh, sCh)),
                            argument);
                    send_to_char(buf, d->character);
                } else {
                    sprintf(buf, "%s [M%d] sends:\n\r     \"%s\"\n\r", capitalize(PERS(tCh, sCh)),
                            npc_index[ch->nr].vnum, argument);
                    send_to_char(buf, d->character);
                }
                return;
            }
        }

        send_to_char("Message not sent, they aren't connected.\n\r", ch);
        return;
    }

    /* give the sender a message */
    sprintf(buf, "You send to %s:\n\r     \"%s\"\n\r", PERS(sCh, tCh), argument);
    send_to_char(buf, ch);
    /* if they're not switched */
    if (ch == sCh) {
        /* normal output */
        snprintf(buf, sizeof(buf), "%s sends:\n\r     \"%s\"\n\r",
                 char_can_see_char(tCh, ch) ? MSTR(ch, short_descr) : "A staff member", argument);
        send_to_char(buf, tCh);
    } else if (IS_IMMORTAL(tCh)) {
        /* note that they're switched in the output */
        sprintf(buf, "$n [M%d] sends:\n\r     \"%s\"\n\r", npc_index[ch->nr].vnum, argument);
        act(buf, FALSE, sCh, 0, tCh, TO_VICT);
    } else {
        /* normal output */
        snprintf(buf, sizeof(buf), "%s sends:\n\r     \"%s\"\n\r",
                 char_can_see_char(tCh, sCh) ? MSTR(sCh, short_descr) : "A staff member", argument);
        send_to_char(buf, tCh);
    }
    if (!IS_IMMORTAL(tCh)) {
        snprintf(buf, sizeof(buf), "%s %s", GET_NAME(tCh), argument);
        immlog(ch, buf, CMD_SEND);
    }

    /* done! */
    return;
}

void                     
cmd_jseval(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
  //  extern int exec_js_string(CHAR_DATA *ch, const char *str);
  //  exec_js_string(ch, argument);
}



