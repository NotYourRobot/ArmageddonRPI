/*
 * File: COMM.C
 * Usage: Commands for communication between players.
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
#include <stdlib.h>
#include <ctype.h>

#include "constants.h"
#include "structs.h"
#include "limits.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "guilds.h"
#include "modify.h"
#include "object_list.h"
#include "psionics.h"
#include "cities.h"
#include "watch.h"
#include "reporting.h"
#include "special.h"

/* external functions */
extern void send_empathic_msg(CHAR_DATA *, char *);

/* local functions */
void error_log(char *str);
void staff_comm_helper(CHAR_DATA *ch, const char *argument, const char *markers, int min_level, int invis_warn, int room_limited);


/* --------------------------------------------------------------------- */

char
sub_char_speak(char m, int race)
{
    int per_racial, i;
    char ch;

    ch = (char) number('a', 'z');

    switch (LOWER(m)) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
    case 'y':
        i = number(0, 5);
        ch = vowel[i];
        break;
    default:
        per_racial = number(1, 100);

        switch (race) {
        case RACE_MANTIS:
            if (per_racial < 40)
                ch = (char) number('t', 'z');
            else if (per_racial < 50)
                ch = 'k';
            break;
        case RACE_GITH:
            if (per_racial < 30)
                ch = (char) number('d', 'i');
            break;
        default:
            ch = (char) number('a', 'z');
            break;
        }
        break;
    }

    /* 50/50 chance to maintain case with upper case */
    if (m >= 'A' && m <= 'Z' && number(0, 100) < 50)
        ch = UPPER(ch);

    return ch;
}

char
sub_char_hear(char m,           /* THe letter   */
              int tongue)
{                               /* THe language */
    int per_lang, i;
    char ch;

    ch = (char) number('a', 'z');

    switch (m) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
    case 'y':
        switch (tongue) {
        case VLORAN:
            switch (number(1, 8)) {
            case 1:
            case 2:
            case 3:
                ch = 'a';
                break;
            case 4:
            case 5:
            case 6:
                ch = 'u';
                break;
            case 7:
            case 8:
                ch = 'o';
                break;
            }
            break;
        default:
            i = number(0, 5);
            ch = vowel[i];
            break;
        }
        break;
    default:
        per_lang = number(1, 100);

        switch (tongue) {
        case BENDUNE:
            if (per_lang < 35)
                ch = (char) number('l', 'o');
            else if (per_lang < 60)
                ch = (char) number('q', 'u');
            break;
        case ALLUNDEAN:
            if (per_lang < 40)
                ch = (char) number('a', 'j');
            break;
        case MIRUKKIM:
            if (per_lang < 30)
                ch = (char) number('h', 'n');
            break;
        case KENTU:
            if (per_lang < 45)
                ch = (char) number('l', 'r');
            break;
        case NRIZKT:
            if (per_lang < 50)
                ch = (char) number('t', 'z');
            break;
        case UNTAL:
            if (per_lang < 35)
                ch = (char) number('e', 'k');
            break;
        case CAVILISH:
            if (per_lang < 25)
                ch = (char) number('c', 'f');
            else if (per_lang < 50)
                ch = (char) number('n', 'r');
            break;
        case TATLUM:
            if (per_lang < 30)
                ch = (char) number('g', 'k');
            else if (per_lang < 60)
                ch = (char) number('o', 's');
            break;
        case VLORAN:
            if (per_lang < 25)
                ch = 'h';
            else if (per_lang < 50)
                ch = 'g';
            else if (per_lang < 75)
                ch = 'b';
            else if (per_lang < 100)
                ch = 'l';
            break;
        default:
            ch = (char) number('a', 'z');
            break;
        }
    }

    return ch;
}

void
send_translated(CHAR_DATA * ch, /* the person talking */
                CHAR_DATA * tar_ch,     /* THe person listening */
                const char *text_pure)
{                               /* What was originally said */
    int spoken_language, j, percent;
    char text[MAX_STRING_LENGTH];
    char newcomm[MAX_STRING_LENGTH];
    char *m;


    strcpy(text, text_pure);

    /* If the listener is immortal, or the speaker is affected by comprehend 
     * languages then they will understand what is said perfectly */
    if (IS_IMMORTAL(tar_ch) || (affected_by_spell(tar_ch, PSI_COMPREHEND_LANGS) && can_use_psionics(ch))) {
        for (j = 0; *(text + j) == ' '; j++);

        sprintf(newcomm, "     \"%s\"\n\r", text + j);
        send_to_char(newcomm, tar_ch);

        return;
    }

    /* Alter string due to speaker's proficiency in language */

    spoken_language = GET_SPOKEN_LANGUAGE(ch);

    if (!affected_by_spell(ch, SPELL_TONGUES)) {
        j = 0;
        if (has_skill(ch, spoken_language))
            /*      if (ch->skills[spoken_language]) */
            j = ch->skills[spoken_language]->learned;

        for (m = text; *m != '\0'; m++) {
            percent = MIN(number(1, 100), number(1, 125));

            /* if random percent > skill (adjusted by drunkness)
             * and it's an alphabetic character
             */
            if ((percent > j) && isalpha(*m))
                /* was only lower case -Morg 
                 * && (*m >= 'a' && *m <= 'z'))
                 */
                *m = sub_char_speak(*m, GET_RACE(ch));

        }
    }

    /* Alter string due to listener's proficieny in language */

    j = 0;

    if ((has_skill(tar_ch, spoken_language)) && (tar_ch->skills[spoken_language]->learned > 0))
        j = (tar_ch->skills[spoken_language]->learned);

    if (affected_by_spell(ch, SPELL_TONGUES))
        j = 125;

    for (m = text; *m != '\0'; m++) {
        percent = MIN(number(1, 100), number(1, 125));

        if ((percent > j) && isalpha(*m))
            *m = sub_char_hear(*m, GET_LANGUAGE(ch));
    }

    /* Send string to listener */

    for (j = 0; *(text + j) == ' '; j++);

    sprintf(newcomm, "     \"%s\"\n\r", text + j);
    send_to_char(newcomm, tar_ch);
}


void
send_psi_translated(CHAR_DATA * ch, CHAR_DATA * tar_ch, char *text_pure)
{
    /*  int i; */
    int j, percent;
    char text[MAX_STRING_LENGTH];
    char newcomm[MAX_STRING_LENGTH];
    char *m;


    strcpy(text, text_pure);
    if (IS_IMMORTAL(tar_ch) || IS_IMMORTAL(ch)) {
        for (j = 0; *(text + j) == ' '; j++);

        sprintf(newcomm, "     \"%s\"\n\r", text + j);
        send_to_char(newcomm, tar_ch);
        return;
    };

    /* Alter string due to speaker's proficiency in language */

/*************************************/
    /* garble  for the person speaking   */
/*************************************/

    if (!IS_NPC(ch) && !affected_by_spell(ch, SPELL_TONGUES)) {
        if (has_skill(ch, PSI_CONTACT))
            /*      if (ch->skills[PSI_CONTACT]) */
            j = ch->skills[PSI_CONTACT]->learned;
        else
            j = 2;

        for (m = text; *m != '\0'; m++) {
            percent = number(1, 10);
            /* in psionics just replace with a space */
            if ((percent > j) && isalpha(*m))
                *m = ' ';
        }
    }

    /* Alter string due to listener's proficieny in language */


    for (m = text; *m != '\0'; m++) {
        if (has_skill(tar_ch, PSI_CONTACT))
            /*      if (tar_ch->skills[PSI_CONTACT]) */
            j = tar_ch->skills[PSI_CONTACT]->learned;
        else
            j = 2;

        percent = number(1, 10);
        /* in psionics just replace with a space */
        if ((percent > j) && isalpha(*m))
            *m = ' ';
    }

    /* Send string to listener */

    for (j = 0; *(text + j) == ' '; j++);

    sprintf(newcomm, "     \"%s\"\n\r", text + j);
    send_to_char(newcomm, tar_ch);
}


struct speech_emote_data
{
    char *token;
    CHAR_DATA *ch;
    OBJ_DATA *obj;
    int loc;
    struct speech_emote_data *next;
};

char *get_emote_token(char *arg);
struct obj_list_type *add_to_char_list(void *ch, struct obj_list_type *list);

void
free_speech_emote_list(struct speech_emote_data *token_head)
{
    struct speech_emote_data *fp;

    while ((fp = token_head)) {
        token_head = token_head->next;
        if (fp->token)
            free(fp->token);
        free(fp);
    }
}

/* parse_emote_message
 * 
 * Parse an emote-like message from 'ch's point of view
 *
 *    ch       - person who's perspective should be used to find objects
 *    to_ch    - person to send the message to
 *    arg      - message to parse
 *    use_name - use names instead of short descriptions
 *    msg      - place to store the parsed message
 *    ch_list  - place to store the list of characters found
 *
 * Formatting codes useable in 'arg':
 *    ~ - replace the following string with the named character's sdesc
 *    ! - replace the following string with the named character's
 *    third or second-person objective pronoun.
 *    % - as ~ but possessive
 *    ^ - as ! but possessive
 *    # - replace the following string with 'he/she'
 *    @ - the calling *ch's short description
 */
bool
parse_emote_message(CHAR_DATA * ch, const char *arg, const bool use_name, char *msg, size_t msg_len,
                    struct obj_list_type **ch_list)
{
    char buf[MAX_STRING_LENGTH];
    char *token;
    struct obj_list_type *char_list = NULL;
    struct speech_emote_data *token_head = (struct speech_emote_data *) malloc(sizeof *token_head);
    struct speech_emote_data *tp, *tail;
    char *argument = strdup(arg);
    char *orig_arg = argument;
    bool fSdesc = FALSE;

    token_head->token = NULL;
    token_head->ch = NULL;
    token_head->next = NULL;
    tail = token_head;

    /* tokenize the list and stick it into the emote struct */
    while ((token = get_emote_token(argument))) {
        tp = (struct speech_emote_data *) malloc(sizeof *tp);
        tp->token = token;
        tp->ch = NULL;
        tp->obj = NULL;
        tp->loc = 0;
        tp->next = NULL;
        tail->next = tp;
        tail = tp;
    }

    /* find the characters to whom the personal emotes refer */
    for (tp = token_head->next; tp != NULL; tp = tp->next) {
        if (isspecial(tp->token[0]) && (strlen(tp->token) > 1)) {
            if ((tp->loc =
                 generic_find(tp->token + 1,
                              FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch,
                              &tp->ch, &tp->obj)) == 0) {
                sprintf(buf, "You do not see '%s' here.\n\r", tp->token + 1);
                send_to_char(buf, ch);

                free_speech_emote_list(token_head);
                free(orig_arg);
                return FALSE;
            }
        } else if (tp->token[0] == '@' || !strcmp(tp->token, "me") || !strcmp(tp->token, " me")) {
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

    *msg = '\0';

    /* format the tokens one at a time, translating as necessary */
    for (tp = token_head->next; tp != NULL; tp = tp->next) {
        if (((tp->ch || tp->obj) && !fSdesc)    /* normal emote */
            ||((tp->ch || tp->obj) && fSdesc    /* variably placed sdesc */
               && tp == token_head->next->next  /* is the first argument */
               && (!strcmp(tp->token, " me") || !strcmp(tp->token, "me")
                   || tp->token[0] == '@'))
            || ((tp->ch || tp->obj) && fSdesc && tp != token_head->next->next)) {
            if (fSdesc && (!strcmp(tp->token, "me") || !strcmp(tp->token, " me"))) {
                sprintf(buf, "%s$c%c", isspace(*tp->token) ? " " : "", use_name ? 'n' : 's');
                string_safe_cat(msg, buf, msg_len);
                char_list = add_to_char_list((void *) ch, char_list);
            } else {
                switch (tp->token[0]) {
                case '~':
                    if (tp->obj) {
                        if (tp->loc != FIND_OBJ_ROOM) {
                            string_safe_cat(msg, "$cp $oa", msg_len);
                            char_list = add_to_char_list((void *) ch, char_list);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        } else {
                            string_safe_cat(msg, "$os", msg_len);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        }
                    } else {
                        sprintf(buf, "$c%c",
                                (use_name ? 'n' : (tp == token_head->next ? 't' : 's')));
                        string_safe_cat(msg, buf, msg_len);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
                    break;
                case '%':
                    if (tp->ch) {
                        sprintf(buf, "$c%cp",
                                (use_name ? 'n' : (tp == token_head->next ? 't' : 's')));
                        string_safe_cat(msg, buf, msg_len);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    } else if (tp->obj) {
                        if (tp->loc != FIND_OBJ_ROOM) {
                            string_safe_cat(msg, "$cp ", msg_len);
                            char_list = add_to_char_list((void *) ch, char_list);
                            string_safe_cat(msg, "$oap", msg_len);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        } else {
                            string_safe_cat(msg, "$otp", msg_len);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        }
                    }
                    break;
                case '^':
                    if (tp->ch) {
                        string_safe_cat(msg, "$cp", msg_len);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
                    break;
                case '!':
                    if (tp->ch) {
                        string_safe_cat(msg, "$cm", msg_len);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
                    break;
		case '&':
                    if (tp->ch) {
                        string_safe_cat(msg, "$cl", msg_len);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
                    break;
		case '=':
                    if (tp->ch) {
                        sprintf(buf, "$c%cg",
                                (use_name ? 'n' : (tp == token_head->next ? 't' : 's')));
                        string_safe_cat(msg, buf, msg_len);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    } else if (tp->obj) {
                        if (tp->loc != FIND_OBJ_ROOM) {
                            string_safe_cat(msg, "$cg ", msg_len);
                            char_list = add_to_char_list((void *) ch, char_list);
                            string_safe_cat(msg, "$oag", msg_len);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        } else {
                            string_safe_cat(msg, "$otg", msg_len);
                            char_list = add_to_char_list((void *) tp->obj, char_list);
                        }
                    }
                    break;
		case '+':
                    if (tp->ch) {
                        string_safe_cat(msg, "$cg", msg_len);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
		    break;
                case '#':
                    if (tp->ch) {
                        string_safe_cat(msg, "$ce", msg_len);
                        char_list = add_to_char_list((void *) tp->ch, char_list);
                    }
                    break;
                case '@':
                    if (tp->ch) {
                        sprintf(buf, "$c%c", use_name ? 'n' : 's');
                        string_safe_cat(msg, buf, msg_len);
                        char_list = add_to_char_list((void *) ch, char_list);

                        if (tp == token_head->next->next)
                            string_safe_cat(msg, "c", msg_len);
                    }
                    break;
                default:
                    sprintf(msg, "Bad punct mark in token '%s' in emote '%s'", tp->token, arg);
                    gamelog(msg);
                    send_to_char("Error, bad emote.  Read 'help emote' and try again.\n\r", ch);

                    free_speech_emote_list(token_head);
                    free(orig_arg);
                    return FALSE;       /* added this return -tenebrius */

                }
            }

            /* initial capitalize if first */
            if (tp == token_head->next)
                string_safe_cat(msg, "c", msg_len);
        } else {
            string_safe_cat(msg, tp == token_head->next ? capitalize(tp->token) : tp->token,
                            msg_len);
        }
    }

    if (msg[strlen(msg) - 1] == '\r' && msg[strlen(msg) - 2] == '\n') {
       msg[strlen(msg) - 2] = '\0';
    }

    if(!ispunct(msg[strlen(msg) - 1])) {
        string_safe_cat(msg, ".", msg_len);
    }

    /* store the char_list in their pointer */
    *ch_list = char_list;

    free_speech_emote_list(token_head);
    free(orig_arg);
    return TRUE;
}                               // end parse_emote_message


bool
send_to_char_vict_and_room_parsed(CHAR_DATA * ch, CHAR_DATA * vict, char *preHow, char *postHow,
                                  char *to_char, char *to_vict, char *to_room, char *to_monitor,
                                  int monitor_channel)
{
    char message[MAX_STRING_LENGTH];

    /* prep it with 'how' it was done */
    prepare_emote_message(preHow, to_char, postHow, message);

    /*
     * try and send the message, if it fails, abort
     * send_to_char_parsed will handle sending an error message if
     * the emote portion fails.
     */
    if (!send_to_char_parsed(ch, ch, message, FALSE))
        return FALSE;

    /* Generate a message for the victim */
    prepare_emote_message(preHow, to_vict, postHow, message);
    send_to_char_parsed(ch, vict, message, FALSE);

    /* Generate a message for the room */
    prepare_emote_message(preHow, to_room, postHow, message);
    send_to_room_ex_parsed(ch, vict, ch->in_room, message, FALSE, FALSE, 0, 0);

    /* send to monitoring */
    if (monitor_channel != MONITOR_NONE) {
        prepare_emote_message(preHow, to_monitor, postHow, message);
        send_to_monitor_parsed(ch, message, monitor_channel);
    }

    return TRUE;
}

bool
send_hidden_to_char_vict_and_room_parsed(CHAR_DATA * ch, CHAR_DATA * vict, char *preHow, char *postHow, char *to_char, char *to_vict, char *to_room, char *to_monitor, int monitor_channel, int skillMod, int opposingSkill)
{
    char message[MAX_STRING_LENGTH];

    /* prep it with 'how' it was done */
    prepare_emote_message(preHow, to_char, postHow, message);

    /*
     * try and send the message, if it fails, abort
     * send_to_char_parsed will handle sending an error message if
     * the emote portion fails.
     */
    if (!send_to_char_parsed(ch, ch, message, FALSE))
        return FALSE;

    /* Generate a message for the victim */
    if (to_vict) {
        prepare_emote_message(preHow, to_vict, postHow, message);
        send_to_char_parsed(ch, vict, message, FALSE);
    }

    /* Generate a message for the room */
    prepare_emote_message(preHow, to_room, postHow, message);
    send_to_room_ex_parsed(ch, vict, ch->in_room, message, FALSE, TRUE, skillMod, opposingSkill);

    /* send to monitoring */
    if (monitor_channel != MONITOR_NONE) {
        prepare_emote_message(preHow, to_monitor, postHow, message);
        send_to_monitor_parsed(ch, message, monitor_channel);
    }

    return TRUE;
}
		

bool
send_hidden_to_char_and_room_parsed(CHAR_DATA * ch, char *preHow, char *postHow, char *to_char, char *to_room, char *to_monitor, int monitor_channel, int skillmod, int opposingSkill)
{
    char message[MAX_STRING_LENGTH];

    if (to_char) {
        /* prep it with 'how' it was done */
        prepare_emote_message(preHow, to_char, postHow, message);

        /*
         * try and send the message, if it fails, abort
         * send_to_char_parsed will handle sending an error message if
         * the emote portion fails.
         */
        if (!send_to_char_parsed(ch, ch, message, FALSE))
            return FALSE;
    }
    /* Generate a message for the room */
    prepare_emote_message(preHow, to_room, postHow, message);
    send_hidden_to_room_parsed(ch, ch->in_room, message, FALSE, skillmod, opposingSkill);

    /* send to monitoring */
    if (monitor_channel != MONITOR_NONE) {
        prepare_emote_message(preHow, to_monitor, postHow, message);
        send_to_monitor_parsed(ch, message, monitor_channel);
    }

    return TRUE;
}

bool
send_to_char_and_room_parsed(CHAR_DATA * ch, char *preHow, char *postHow, char *to_char,
                             char *to_room, char *to_monitor, int monitor_channel)
{
    char message[MAX_STRING_LENGTH];

    if (to_char) {
        /* prep it with 'how' it was done */
        prepare_emote_message(preHow, to_char, postHow, message);

        /*
         * try and send the message, if it fails, abort
         * send_to_char_parsed will handle sending an error message if
         * the emote portion fails.
         */
        if (!send_to_char_parsed(ch, ch, message, FALSE))
            return FALSE;
    }

    /* Generate a message for the room */
    prepare_emote_message(preHow, to_room, postHow, message);
    send_to_room_parsed(ch, ch->in_room, message, FALSE);

    /* send to monitoring */
    if (monitor_channel != MONITOR_NONE) {
        prepare_emote_message(preHow, to_monitor, postHow, message);
        send_to_monitor_parsed(ch, message, monitor_channel);
    }

    return TRUE;
}

/* 
 * Sends a specified message to everyone in the room except ch
 * 
 * ch   - person who did the action to be sent
 * room - room full of people who are to be shown the message
 * arg  - message to be sent
 * show_invis - should the message be shown to those who can't see ch?
 */
void
send_to_room_parsed(CHAR_DATA * ch, ROOM_DATA * room, char *arg, bool show_invis)
{
    send_to_room_ex_parsed(ch, NULL, room, arg, show_invis, FALSE, 0, 0);
}

void
send_hidden_to_room_parsed(CHAR_DATA * ch, ROOM_DATA * room, char *arg, bool show_invis, int skillMod, int opposingSkill)
{
    send_to_room_ex_parsed(ch, NULL, room, arg, show_invis, TRUE, skillMod, opposingSkill);
}

void
send_to_room_ex_parsed(CHAR_DATA * ch, CHAR_DATA *vict, ROOM_DATA * room, char *arg,
 bool show_invis, bool show_hidden, int skillMod, int opposingSkill)
{
    CHAR_DATA *rch;             /* pointer to a character in the room */
    char msg[MAX_STRING_LENGTH];        /* placeholder for the parsed message */
    struct obj_list_type *p, *char_list = NULL; /* list of objects to format */

    /* pass the arguments off to be parsed for us */
    if (!parse_emote_message(ch, arg, FALSE, msg, sizeof(msg), &char_list))
        /* parse fails, we fail */
        return;

    /* iterate over the people in the room */
    for (rch = room->people; rch; rch = rch->next_in_room) {
        /* skip over the person who did it && victim */
        if (rch == ch || rch == vict)
            continue;

        /* if not supposed to show to those who can't see ch */
        if (!show_invis && !CAN_SEE(rch, ch))
            continue;

        if( show_hidden && !watch_success(rch, ch, vict, skillMod, opposingSkill ) )
            continue;

        /* use gvprintf to format and send to rch */
        gvprintf(ch, rch, NULL, msg, char_list, 0, 0);
    }

    /* free the char_list */
    while (((p) = (char_list))) {
        char_list = char_list->next;
        free(p);
    }

}

bool
send_how_to_char_parsed(CHAR_DATA * ch, CHAR_DATA *to_ch, char *to_char,
 char *preHow, char *postHow)
{
    char message[MAX_STRING_LENGTH];

    /* prep it with 'how' it was done */
    prepare_emote_message(preHow, to_char, postHow, message);

    return send_to_char_parsed(ch, to_ch, message, FALSE);
}

/* send_to_char_parsed appends the argument to the character's name and sends
   it to the room, recognizing the following special characters:
   ~ - replace the following string with the named character's sdesc
   ! - replace the following string with the named character's
   third or second-person objective pronoun.
   % - as ~ but possessive
   ^ - as ! but possessive
   # - replace the following string with 'he/she'
   @ - the calling *ch's short description

   if the pos parameter is set to true, send_to_char_parsed adds an "'s" to
   the character's name at the beginning of the emote, in the style of
   the pemote command.
 */
bool
send_to_char_parsed(CHAR_DATA * ch, CHAR_DATA * to_ch, char *arg, bool use_name)
{
    char msg[MAX_STRING_LENGTH];
    struct obj_list_type *p, *char_list = NULL;

    /* pass the arguments off to be parsed for us */
    if (!parse_emote_message(ch, arg, use_name, msg, sizeof(msg), &char_list))
        /* parse fails, we fail */
        return FALSE;

    /* use game value print formatted (gvprintf) to format and send to to_ch */
    gvprintf(ch, to_ch, NULL, msg, char_list, 0, 0);

    /* free the char_list */
    while (((p) = (char_list))) {
        char_list = char_list->next;
        free(p);
    }

    /* success */
    return TRUE;
}


void
send_comm_to_reciters(CHAR_DATA * ch, const char *output)
{
    CHAR_DATA *lis = NULL;;
    struct affected_type *tmp_af;

    if (!ch)
        return;

    // Look at the list of descriptors for a potential listener
    for (lis = character_list; lis; lis = lis->next) {
        if (!lis)
            continue;
        if (lis->specials.contact != ch)
            continue;
        if (!lis->in_room)
            continue;

        /* We have someone in contact with the speaker, check for reciting creatures. */
        if (lis) {
            if (!(strstr(MSTR(lis, short_descr), "headless"))) {
                tmp_af = affected_by_spell(lis, SPELL_RECITE);
                if (tmp_af)
                    if (number(0, 100) < ((tmp_af->power * 10) + 30))
                        cmd_say(lis, output, 0, 0);
            } else
                parse_command(lis, "emote squirts blood from the hole in its neck.");
        }
    }
}


void
cmd_say(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int j, k;
    struct room_data *room;
    CHAR_DATA *lis;
    char sex[256];
    char buf[MAX_STRING_LENGTH];
    char text[MAX_STRING_LENGTH];
    char out[MAX_STRING_LENGTH];
    char msg[MAX_STRING_LENGTH];
    char how[MAX_STRING_LENGTH];
    char tmpbuf[MAX_STRING_LENGTH];
    struct watching_room_list *tmp_wlist;

    /* i counter removed for brevity  -Morg */
    for (; *(argument) == ' '; argument++);

    /* figure out 'how' it was said         */
    /* ie: say (in a quiet voice) hey man   */
    /*  or  say -hurridly got any pot?      */

    if (argument[0] == '-' || argument[0] == '(' || argument[0] == '[' || argument[0] == '*')
        argument = one_argument_delim(argument, how, MAX_STRING_LENGTH, argument[0]);
    else
        how[0] = '\0';

    if (!*(argument))
        send_to_char("Yes, but what do you want to say?\n\r", ch);
    else if (IS_AFFECTED(ch, CHAR_AFF_SILENCE)) {
        gprintf(NULL, ch->in_room, "$csc attempt$cy to mumble something.\n", ch, ch);
    } else if (affected_by_spell(ch, PSI_BABBLE)) {
        gprintf(NULL, ch->in_room, "$csc babble$cy incoherently.\n", ch, ch);
    } else {
        strcpy(out, argument);
        strcpy(text, skill_name[GET_SPOKEN_LANGUAGE(ch)]);
        send_comm_to_monitor(ch, how, "says", "exclaims", "asks", argument);
        send_comm_to_reciters(ch, argument);
        send_comm_to_clairsentient(ch, ch, how, "says", "exclaims", "asks", argument, TRUE);

        /* changed to incorprecite 'how' it was said & exclaim/ask/say  -Morg */
        sprintf(buf, "%s%s %s, in %s:\n\r", how[0] != '\0' ? capitalize(how) : "",
                how[0] != '\0' ? ", you" : "You",
                out[strlen(out) - 1] == '!' ? "exclaim" : out[strlen(out) - 1] ==
                '?' ? "ask" : "say", text);

        if (send_to_char_parsed(ch, ch, buf, FALSE)) {
            sprintf(buf, "     \"%s\"\n\r", out);
            send_to_char(buf, ch);
        } else
            return;

        for (lis = ch->in_room->people; lis; lis = lis->next_in_room) {
            if ((lis) && (ch != lis) && AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                sprintf(text, "%s", out);

                buf[0] = '\0';  /* prep the buffer */

                sprintf(buf, "%s%s", how[0] != '\0' ? capitalize(how) : "",
                        how[0] != '\0' ? ", " : "");

                sprintf(msg, "%s %s, in ", VOICE(lis, ch),
                        text[strlen(text) - 1] == '!' ? "exclaims" : text[strlen(text) - 1] ==
                        '?' ? "asks" : "says");
                strcat(buf, msg);
                k = apply_accent(tmpbuf, ch, lis);
                strcat(buf, tmpbuf);
                strcat(buf, ":\n\r");

                if (send_to_char_parsed(ch, lis, buf, FALSE))
                    send_translated(ch, lis, text);
                send_comm_to_clairsentient(lis, ch, how, "says", "exclaims", "asks", argument,
                                           TRUE);
                learn_language(ch, lis, k);
            }
        }

        for (j = 0; j < 6; j++) {
            if (ch->in_room->direction[j] && (room = ch->in_room->direction[j]->to_room)
                && !IS_SET(EXIT(ch, j)->exit_info, EX_SPL_SAND_WALL)) {
                for (lis = room->people; lis; lis = lis->next_in_room) {
                    if ((IS_AFFECTED(lis, CHAR_AFF_LISTEN)
                         || (GET_RACE(lis) == RACE_VAMPIRE))
                        && !lis->specials.fighting
                        && AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                        int percent = 0;
                        int learned = has_skill(lis, SKILL_LISTEN) 
                         ? lis->skills[SKILL_LISTEN]->learned : 0;

                        learned += wis_app[GET_WIS(lis)].percep;

                        if (IS_SET(EXIT(ch, j)->exit_info, EX_ISDOOR)
                            && IS_SET(EXIT(ch, j)->exit_info, EX_CLOSED) && number(0, 2))
                            learned /= 2;

                        if (IS_IMMORTAL(lis) || (GET_RACE(lis) == RACE_VAMPIRE))
                            percent = 0;
                        else {
                            percent += number(1, 101);
                        }
                        if (percent <= learned / 2) {
                            sprintf(text, "%s", out);


                            if (GET_SEX(ch) == SEX_MALE)
                                strcpy(sex, "a man");
                            else if (GET_SEX(ch) == SEX_FEMALE)
                                strcpy(sex, "a woman");
                            else
                                strcpy(sex, "someone");
    
                            k = apply_accent(tmpbuf, ch, lis);
                            sprintf(buf, "You hear %s's voice from %s say, in %s:\n\r", sex,
                                    rev_dir_name[j], tmpbuf);
    
                            /* Replaced by the above, 4/21/2001 -Nessalin
                             * sprintf (buf, "A voice from %s says:\n\r",
                             * rev_dir_name[j]);  */
    
                            send_to_char(buf, lis);
                            send_translated(ch, lis, text);
                            send_comm_to_clairsentient(lis, ch, "", "says", "exclaims", "asks",
                                                       argument, FALSE);
                            learn_language(ch, lis, k);
                        } else if( has_skill(lis, SKILL_LISTEN))
                            gain_skill(lis, SKILL_LISTEN, 1);
                    }
                }
            }
        }

        /* start listening rooms  - Tenebrius 2005/03/27  */
        for (tmp_wlist = ch->in_room->watched_by; tmp_wlist; tmp_wlist = tmp_wlist->next_watched_by) {
            room = tmp_wlist->room_watching;
            if (IS_SET(tmp_wlist->watch_type, WATCH_LISTEN)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                        sprintf(text, "%s", out);
                        k = apply_accent(tmpbuf, ch, lis);

                        if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                            if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                                sprintf(buf, "%s%s%s%s %s, in %s:\n\r",
                                    tmp_wlist->view_msg_string,
			            how[0] != '\0' ? how : "",
                                    how[0] != '\0' ? ", " : "",
			            PERS(lis, ch),
                                    text[strlen(text) - 1] == '!' ? "exclaims" : text[strlen(text) - 1] == '?' ? "asks" : "says", tmpbuf);
			    }
			} else {
                            if (GET_SEX(ch) == SEX_MALE)
                                strcpy(sex, "a man");
                            else if (GET_SEX(ch) == SEX_FEMALE)
                                strcpy(sex, "a woman");
                            else
                                strcpy(sex, "someone");

                            sprintf(buf, "%syou hear %s's voice say, in %s:\n\r",
                                tmp_wlist->listen_msg_string, sex, tmpbuf);
			}

                        if (send_to_char_parsed(ch, lis, buf, FALSE))
                            send_translated(ch, lis, text);
                        send_comm_to_clairsentient(lis, ch, "", "says", "exclaims", "asks",
                                                   argument, FALSE);
                        learn_language(ch, lis, k);
                    }
            } else if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                        sprintf(text, "%s", out);
                        sprintf(buf, "%s%s%s%s %s something.\n\r",
                            tmp_wlist->view_msg_string,
			    how[0] != '\0' ? how : "",
                            how[0] != '\0' ? ", " : "",
			    PERS(lis, ch),
                            text[strlen(text) - 1] == '!' ? "exclaims" : text[strlen(text) - 1] == '?' ? "asks" : "says");
			send_to_char_parsed(ch, lis, buf, FALSE);
		    }
	    }
        }
        /* end listening rooms */
    }
}

/* Nessalin Singing 8-21-99, trying to fix it so NPCs can have the emotes
   when they sing */
void
cmd_sing(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int i, j, k, line_count;
    struct room_data *room;
    CHAR_DATA *lis;
    char sex[256];
    char buf[MAX_STRING_LENGTH];
    char text[MAX_STRING_LENGTH];
    char out[MAX_STRING_LENGTH];
    char out_parsed[MAX_STRING_LENGTH];
    char msg[MAX_STRING_LENGTH];
    char how[MAX_STRING_LENGTH];
    char tmpbuf[MAX_STRING_LENGTH];
    struct watching_room_list *tmp_wlist;

    /* i counter removed for brevity  -Morg */
    for (; *(argument) == ' '; argument++);

    /* figure out 'how' it was said       */
    /* ie: say (in a quiet voice) hey man */
    /*  or  say -hurridly got any pot?    */

    if (argument[0] == '-' || argument[0] == '(' || argument[0] == '[')
        argument = one_argument_delim(argument, how, MAX_STRING_LENGTH, argument[0]);
    else
        how[0] = '\0';

    if (!*(argument))
        send_to_char("Yes, but what do you want to sing?\n\r", ch);
    else if (IS_AFFECTED(ch, CHAR_AFF_SILENCE)) {
        gprintf(NULL, ch->in_room, "$csc attempt$cy to mumble something.\n", ch, ch);
    } else if (affected_by_spell(ch, PSI_BABBLE)) {
        gprintf(NULL, ch->in_room, "$csc babble$cy incoherently.\n", ch, ch);
    } else {
        strcpy(out, argument);
        strcpy(text, skill_name[GET_SPOKEN_LANGUAGE(ch)]);
        send_comm_to_monitor(ch, how, "sings", "sings", "sings", argument);
        send_comm_to_reciters(ch, argument);
        send_comm_to_clairsentient(ch, ch, how, "sings", "sings", "sings", argument, TRUE);

        line_count = 0;
        for( i = 0, j = 0; i < strlen(out); i++, j++ ) {
            if( out[i] == '|' || out[i] == '\\' || out[i] == '/' ) {
                line_count++;
                if( line_count > 4 ) {
                    cprintf( ch, "Sorry, there is a limit of 5 lines at a time with songs.\n\r");
                    return;
                }
                out_parsed[j++] = '\n';
                out_parsed[j++] = '\r';
                out_parsed[j++] = ' ';
                out_parsed[j++] = ' ';
                out_parsed[j++] = ' ';
                out_parsed[j++] = ' ';
                out_parsed[j++] = ' ';
                out_parsed[j] = ' ';
                while( out[i + 1] != '\0' && isspace(out[i + 1])) i++;
            } else {
                out_parsed[j] = out[i];
            }
        }
        out_parsed[j] = '\0';

        /* changed to incorprecite 'how' it was said & exclaim/ask/say  -Morg */
        sprintf(buf, "%s%s %s, in %s:\n\r", how[0] != '\0' ? capitalize(how) : "",
                how[0] != '\0' ? ", you" : "You",
                out[strlen(out) - 1] == '!' ? "sing" : out[strlen(out) - 1] ==
                '?' ? "sing" : "sing", text);

        if (send_to_char_parsed(ch, ch, buf, FALSE)) {
            sprintf(buf, "     \"%s\"\n\r", 
             !IS_SET( ch->specials.brief, BRIEF_SONGS ) ? out_parsed : out );
            send_to_char(buf, ch);
        } else
            return;

        for (lis = ch->in_room->people; lis; lis = lis->next_in_room) {
            if ((lis) && (ch != lis) && AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                sprintf(text, "%s", 
                 !IS_SET(lis->specials.brief, BRIEF_SONGS) ? out_parsed : out);

                buf[0] = '\0';  /* prep the buffer */

                sprintf(buf, "%s%s", how[0] != '\0' ? capitalize(how) : "",
                        how[0] != '\0' ? ", " : "");

                sprintf(msg, "%s %s, in ", VOICE(lis, ch),
                        text[strlen(text) - 1] == '!' ? "sings" : text[strlen(text) - 1] ==
                        '?' ? "sings" : "sings");
                strcat(buf, msg);
                k = apply_accent(tmpbuf, ch, lis);
                strcat(buf, tmpbuf);
                strcat(buf, ":\n\r");

                if (send_to_char_parsed(ch, lis, buf, FALSE))
                    send_translated(ch, lis, text);
                send_comm_to_clairsentient(lis, ch, how, "sings", "sings", "sings", argument, TRUE);
                learn_language(ch, lis, k);
            }
        }

        for (j = 0; j < 6; j++) {
            if (ch->in_room->direction[j] && (room = ch->in_room->direction[j]->to_room)
                && !IS_SET(EXIT(ch, j)->exit_info, EX_SPL_SAND_WALL)) {
                for (lis = room->people; lis; lis = lis->next_in_room) {
                    if ((IS_AFFECTED(lis, CHAR_AFF_LISTEN)
                         || (GET_RACE(lis) == RACE_VAMPIRE))
                        && !lis->specials.fighting
                        && AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                        int percent = 0;
                        int learned = has_skill(lis, SKILL_LISTEN)
                         ? lis->skills[SKILL_LISTEN]->learned : 0;

                        learned += wis_app[GET_WIS(lis)].percep;

                        if (IS_SET(EXIT(ch, j)->exit_info, EX_ISDOOR)
                            && IS_SET(EXIT(ch, j)->exit_info, EX_CLOSED) && number(0, 2))
                            learned /= 2;

                        if (IS_IMMORTAL(lis) || (GET_RACE(lis) == RACE_VAMPIRE))
                            percent = 0;
                        else {
                            percent += number(1, 101);
                        }

                        if (percent <= learned / 2) {
                            sprintf(text, "%s", 
                             !IS_SET(lis->specials.brief, BRIEF_SONGS)
                              ? out_parsed : out);

                            if (GET_SEX(ch) == SEX_MALE)
                                strcpy(sex, "a man");
                            else if (GET_SEX(ch) == SEX_FEMALE)
                                strcpy(sex, "a woman");
                            else
                                strcpy(sex, "someone");

                            k = apply_accent(tmpbuf, ch, lis);
                            sprintf(buf, "You hear %s's voice from %s sing, in %s:\n\r", sex,
                                    rev_dir_name[j], tmpbuf);
    
                            /* Replaced with the above 4/21/2001 -Nessalin 
                             * sprintf (buf, "A voice from %s sings:\n\r", 
                             * rev_dir_name[j]); */
    
                            send_to_char(buf, lis);
                            send_translated(ch, lis, text);
                            send_comm_to_clairsentient(lis, ch, "", "sings", "sings", "sings", argument, FALSE);
                            learn_language(ch, lis, k);
                        } else if( has_skill(lis, SKILL_LISTEN) ) {
                            gain_skill(lis, SKILL_LISTEN, 1);
                        }
                    }
                }
            }
        }

        /* start listening rooms  - Tenebrius 2005/03/27  */
        for (tmp_wlist = ch->in_room->watched_by; tmp_wlist; tmp_wlist = tmp_wlist->next_watched_by) {
            room = tmp_wlist->room_watching;
            if (IS_SET(tmp_wlist->watch_type, WATCH_LISTEN)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                        sprintf(text, "%s", !IS_SET(lis->specials.brief, BRIEF_SONGS) ? out_parsed : out);
                        k = apply_accent(tmpbuf, ch, lis);

                        if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                            if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                                sprintf(buf, "%s%s%s%s %s, in %s:\n\r",
                                    tmp_wlist->view_msg_string,
			            how[0] != '\0' ? how : "",
                                    how[0] != '\0' ? ", " : "",
			            PERS(lis, ch),
                                    text[strlen(text) - 1] == '!' ? "sings" : text[strlen(text) - 1] == '?' ? "sings" : "sings", tmpbuf);
			    }
			} else {
                            if (GET_SEX(ch) == SEX_MALE)
                                strcpy(sex, "a man");
                            else if (GET_SEX(ch) == SEX_FEMALE)
                                strcpy(sex, "a woman");
                            else
                                strcpy(sex, "someone");

                            sprintf(buf, "%syou hear %s's voice sing, in %s:\n\r",
                                tmp_wlist->listen_msg_string, sex, tmpbuf);
			}

                        if (send_to_char_parsed(ch, lis, buf, FALSE))
                            send_translated(ch, lis, text);
                        send_comm_to_clairsentient(lis, ch, "", "sings", "sings", "sings",
                                                   argument, FALSE);
                        learn_language(ch, lis, k);
                    }
            } else if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                        sprintf(text, "%s", !IS_SET(lis->specials.brief, BRIEF_SONGS) ? out_parsed : out);
                        sprintf(buf, "%s%s%s%s %s something.\n\r",
                            tmp_wlist->view_msg_string,
			    how[0] != '\0' ? how : "",
                            how[0] != '\0' ? ", " : "",
			    PERS(lis, ch),
                            text[strlen(text) - 1] == '!' ? "sings" : text[strlen(text) - 1] == '?' ? "sings" : "sings");
                        send_to_char_parsed(ch, lis, buf, FALSE);
		    }
	    }
        }
        /* end listening rooms */
    }
}

void
cmd_recite(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int i, j, k, line_count;
    struct room_data *room;
    CHAR_DATA *lis;
    char sex[256];
    char buf[MAX_STRING_LENGTH];
    char text[MAX_STRING_LENGTH];
    char out[MAX_STRING_LENGTH];
    char out_parsed[MAX_STRING_LENGTH];
    char msg[MAX_STRING_LENGTH];
    char how[MAX_STRING_LENGTH];
    char tmpbuf[MAX_STRING_LENGTH];
    struct watching_room_list *tmp_wlist;

    /* i counter removed for brevity  -Morg */
    for (; *(argument) == ' '; argument++);

    /* figure out 'how' it was said       */
    /* ie: say (in a quiet voice) hey man */
    /*  or  say -hurridly got any pot?    */

    if (argument[0] == '-' || argument[0] == '(' || argument[0] == '[')
        argument = one_argument_delim(argument, how, MAX_STRING_LENGTH, argument[0]);
    else
        how[0] = '\0';

    if (!*(argument))
        send_to_char("Yes, but what do you want to recite?\n\r", ch);
    else if (IS_AFFECTED(ch, CHAR_AFF_SILENCE)) {
        gprintf(NULL, ch->in_room, "$csc attempt$cy to mumble something.\n", ch, ch);
    } else if (affected_by_spell(ch, PSI_BABBLE)) {
        gprintf(NULL, ch->in_room, "$csc babble$cy incoherently.\n", ch, ch);
    } else {
        strcpy(out, argument);
        strcpy(text, skill_name[GET_SPOKEN_LANGUAGE(ch)]);
        send_comm_to_monitor(ch, how, "recites", "recites", "recites", argument);
        send_comm_to_reciters(ch, argument);
        send_comm_to_clairsentient(ch, ch, how, "recites", "recites", "recites", argument, TRUE);

        line_count = 0;
        for( i = 0, j = 0; i < strlen(out); i++, j++ ) {
            if( out[i] == '|' || out[i] == '\\' || out[i] == '/' ) {
                line_count++;
                if( line_count > 4 ) {
                    cprintf( ch, "Sorry, there is a limit of 5 lines at a time with orations.\n\r");
                    return;
                }
                out_parsed[j++] = '\n';
                out_parsed[j++] = '\r';
                out_parsed[j++] = ' ';
                out_parsed[j++] = ' ';
                out_parsed[j++] = ' ';
                out_parsed[j++] = ' ';
                out_parsed[j++] = ' ';
                out_parsed[j] = ' ';
                while( out[i + 1] != '\0' && isspace(out[i + 1])) i++;
            } else {
                out_parsed[j] = out[i];
            }
        }
        out_parsed[j] = '\0';

        /* changed to incorprecite 'how' it was said & exclaim/ask/say  -Morg */
        sprintf(buf, "%s%s %s, in %s:\n\r", how[0] != '\0' ? capitalize(how) : "",
                how[0] != '\0' ? ", you" : "You",
                out[strlen(out) - 1] == '!' ? "recite" : out[strlen(out) - 1] ==
                '?' ? "recite" : "recite", text);

        if (send_to_char_parsed(ch, ch, buf, FALSE)) {
            sprintf(buf, "     \"%s\"\n\r", 
             !IS_SET( ch->specials.brief, BRIEF_SONGS ) ? out_parsed : out );
            send_to_char(buf, ch);
        } else
            return;

        for (lis = ch->in_room->people; lis; lis = lis->next_in_room) {
            if ((lis) && (ch != lis) && AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                sprintf(text, "%s", 
                 !IS_SET(lis->specials.brief, BRIEF_SONGS) ? out_parsed : out);

                buf[0] = '\0';  /* prep the buffer */

                sprintf(buf, "%s%s", how[0] != '\0' ? capitalize(how) : "",
                        how[0] != '\0' ? ", " : "");

                sprintf(msg, "%s %s, in ", VOICE(lis, ch),
                        text[strlen(text) - 1] == '!' ? "recites" : text[strlen(text) - 1] ==
                        '?' ? "recites" : "recites");
                strcat(buf, msg);
                k = apply_accent(tmpbuf, ch, lis);
                strcat(buf, tmpbuf);
                strcat(buf, ":\n\r");

                if (send_to_char_parsed(ch, lis, buf, FALSE))
                    send_translated(ch, lis, text);
                send_comm_to_clairsentient(lis, ch, how, "recites", "recites", "recites", argument, TRUE);
                learn_language(ch, lis, k);
            }
        }

        for (j = 0; j < 6; j++) {
            if (ch->in_room->direction[j] && (room = ch->in_room->direction[j]->to_room)
                && !IS_SET(EXIT(ch, j)->exit_info, EX_SPL_SAND_WALL)) {
                for (lis = room->people; lis; lis = lis->next_in_room) {
                    if ((IS_AFFECTED(lis, CHAR_AFF_LISTEN)
                         || (GET_RACE(lis) == RACE_VAMPIRE))
                        && !lis->specials.fighting
                        && AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                        int percent = 0;
                        int learned = has_skill(lis, SKILL_LISTEN)
                         ? lis->skills[SKILL_LISTEN]->learned : 0;

                        learned += wis_app[GET_WIS(lis)].percep;

                        // harder to listen through doors
                        if (IS_SET(EXIT(ch, j)->exit_info, EX_ISDOOR)
                            && IS_SET(EXIT(ch, j)->exit_info, EX_CLOSED)) 
                            learned /= 2;

                        if (IS_IMMORTAL(lis) || (GET_RACE(lis) == RACE_VAMPIRE))
                            percent = 0;
                        else {
                            percent += number(1, 101);
                        }
                        if (percent <= learned / 2) {
                            sprintf(text, "%s", 
                             !IS_SET(lis->specials.brief, BRIEF_SONGS)
                              ? out_parsed : out);

                            if (GET_SEX(ch) == SEX_MALE)
                                strcpy(sex, "a man");
                            else if (GET_SEX(ch) == SEX_FEMALE)
                                strcpy(sex, "a woman");
                            else
                                strcpy(sex, "someone");
    
                            k = apply_accent(tmpbuf, ch, lis);
                            sprintf(buf, "You hear %s's voice from %s recite, in %s:\n\r", sex,
                                    rev_dir_name[j], tmpbuf);
    
                            /* Replaced with the above 4/21/2001 -Nessalin 
                             * sprintf (buf, "A voice from %s recites:\n\r", 
                             * rev_dir_name[j]); */
    
                            send_to_char(buf, lis);
                            send_translated(ch, lis, text);
                            send_comm_to_clairsentient(lis, ch, "", "recites", "recites", "recites", argument, FALSE);
                            learn_language(ch, lis, k);
                        } else if( has_skill(lis, SKILL_LISTEN) ) {
                            gain_skill(lis, SKILL_LISTEN, 1);
                        }
                    }
                }
            }
        }

        /* start listening rooms  - Tenebrius 2005/03/27  */
        for (tmp_wlist = ch->in_room->watched_by; tmp_wlist; tmp_wlist = tmp_wlist->next_watched_by) {
            room = tmp_wlist->room_watching;
            if (IS_SET(tmp_wlist->watch_type, WATCH_LISTEN)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                        sprintf(text, "%s", !IS_SET(lis->specials.brief, BRIEF_SONGS) ? out_parsed : out);
                        k = apply_accent(tmpbuf, ch, lis);

                        if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                            if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                                sprintf(buf, "%s%s%s%s %s, in %s:\n\r",
                                    tmp_wlist->view_msg_string,
			            how[0] != '\0' ? how : "",
                                    how[0] != '\0' ? ", " : "",
			            PERS(lis, ch),
                                    text[strlen(text) - 1] == '!' ? "recites" : text[strlen(text) - 1] == '?' ? "recites" : "recites", tmpbuf);
			    }
			} else {
                            if (GET_SEX(ch) == SEX_MALE)
                                strcpy(sex, "a man");
                            else if (GET_SEX(ch) == SEX_FEMALE)
                                strcpy(sex, "a woman");
                            else
                                strcpy(sex, "someone");

                            sprintf(buf, "%syou hear %s's voice recite, in %s:\n\r",
                                tmp_wlist->listen_msg_string, sex, tmpbuf);
			}

                        if (send_to_char_parsed(ch, lis, buf, FALSE))
                            send_translated(ch, lis, text);
                        send_comm_to_clairsentient(lis, ch, "", "recites", "recites", "recites",
                                                   argument, FALSE);
                        learn_language(ch, lis, k);
                    }
            } else if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                        sprintf(text, "%s", !IS_SET(lis->specials.brief, BRIEF_SONGS) ? out_parsed : out);
                        sprintf(buf, "%s%s%s%s %s something.\n\r",
                            tmp_wlist->view_msg_string,
			    how[0] != '\0' ? how : "",
                            how[0] != '\0' ? ", " : "",
			    PERS(lis, ch),
                            text[strlen(text) - 1] == '!' ? "recites" : text[strlen(text) - 1] == '?' ? "recites" : "recites");
                        send_to_char_parsed(ch, lis, buf, FALSE);
		    }
	    }
        }
        /* end listening rooms */
    }
}


void
char_tell_char(CHAR_DATA * ch, CHAR_DATA * tCh, const char *argument)
{
    int j, k, lang;
    struct room_data *room;
    CHAR_DATA *lis;
    char sex[256];
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char text[MAX_STRING_LENGTH];
    char mess[MAX_STRING_LENGTH], out[MAX_STRING_LENGTH];
    char how[MAX_STRING_LENGTH];
    char to_text[MAX_STRING_LENGTH];
    char from_text[MAX_STRING_LENGTH];
    char tmpbuf[MAX_STRING_LENGTH];
    struct watching_room_list *tmp_wlist;

    if (argument[0] == '(' || argument[0] == '[' || argument[0] == '-' || argument[0] == '*')
        argument = one_argument_delim(argument, how, MAX_STRING_LENGTH, argument[0]);
    else
        how[0] = '\0';

    send_comm_to_monitors(ch, tCh, how, "says to", "exclaims to", "asks", argument);
    send_comm_to_reciters(ch, argument);
    send_comm_to_clairsentients(ch, ch, tCh, how, "says to", "exclaims to", "asks", argument);

    /* ch's message */
    lang =
        affected_by_spell(ch, SPELL_TONGUES) ? GET_SPOKEN_LANGUAGE(tCh) : GET_SPOKEN_LANGUAGE(ch);
    sprintf(mess, "%s%s %s %s, in %s:\n\r", how[0] != '\0' ? capitalize(how) : "",
            how[0] != '\0' ? ", you" : "You",
            argument[strlen(argument) - 1] == '!' ? "exclaim to" : argument[strlen(argument) - 1] ==
            '?' ? "ask" : "say to", PERS(ch, tCh), skill_name[lang]);

    if (send_to_char_parsed(ch, ch, mess, FALSE)) {
        sprintf(mess, "     \"%s\"\n\r", argument);
        send_to_char(mess, ch);
    } else
        return;

    /* target's message */
    if (IS_AFFECTED(tCh, CHAR_AFF_DEAFNESS)) {
        act("$n says something to you, but you can't make it out.", TRUE, ch, 0, tCh, TO_VICT);
    } else {
        k = apply_accent(buf1, ch, tCh);
        sprintf(mess, "%s%s%s %s you, in %s:\n\r", how[0] != '\0' ? capitalize(how) : "",
                how[0] != '\0' ? ", " : "", VOICE(tCh, ch),
                argument[strlen(argument) - 1] ==
                '!' ? "exclaims to" : argument[strlen(argument) - 1] == '?' ? "asks" : "says to",
                buf1);

        strcpy(out, argument);
        if (send_to_char_parsed(ch, tCh, mess, FALSE))
            send_translated(ch, tCh, out);

        send_comm_to_clairsentients(tCh, ch, tCh, how, "says to", "exclaims to", "asks", argument);
        learn_language(ch, tCh, k);
    }

    /* room message */
    for (lis = ch->in_room->people; lis; lis = lis->next_in_room) {
        k = apply_accent(buf1, ch, lis);
        if (CAN_SEE(lis, tCh)) {
            strcpy(from_text, VOICE(lis, ch));
            strcpy(to_text, PERS(lis, tCh));
            sprintf(mess, "%s%s%s %s %s, in %s:\n\r", how[0] != '\0' ? capitalize(how) : "",
                    how[0] != '\0' ? ", " : "", from_text,
                    argument[strlen(argument) - 1] ==
                    '!' ? "exclaims to" : argument[strlen(argument) - 1] ==
                    '?' ? "asks" : "says to", to_text, buf1);
        } else {
            sprintf(mess, "%s%s%s %s, in %s:\n\r", how[0] != '\0' ? capitalize(how) : "",
                    how[0] != '\0' ? ", " : "", VOICE(lis, ch),
                    argument[strlen(argument) - 1] ==
                    '!' ? "exclaims" : argument[strlen(argument) - 1] == '?' ? "asks" : "says",
                    buf1);
        }

        if ((lis != ch) && (lis != tCh) && AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
            strcpy(out, argument);
            if (send_to_char_parsed(ch, lis, mess, FALSE))
                send_translated(ch, lis, out);

            send_comm_to_clairsentients(lis, ch, tCh, how, "says to", "exclaims to", "asks",
                                        argument);
            learn_language(ch, lis, k);
        }
    }

    for (j = 0; j < 6; j++) {
        if (ch->in_room->direction[j] && (room = ch->in_room->direction[j]->to_room)
            && !IS_SET(EXIT(ch, j)->exit_info, EX_SPL_SAND_WALL)) {
            for (lis = room->people; lis; lis = lis->next_in_room) {
                if ((IS_AFFECTED(lis, CHAR_AFF_LISTEN)
                     || (GET_RACE(lis) == RACE_VAMPIRE))
                    && !lis->specials.fighting
                    && AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                    int percent = 0;
                    int learned = has_skill(lis, SKILL_LISTEN)
                     ? lis->skills[SKILL_LISTEN]->learned : 0;

                    learned += wis_app[GET_WIS(lis)].percep;

                    // harder to listen through doors
                    if (IS_SET(EXIT(ch, j)->exit_info, EX_ISDOOR)
                        && IS_SET(EXIT(ch, j)->exit_info, EX_CLOSED))
                        learned /= 2;

                    if (IS_IMMORTAL(lis) || (GET_RACE(lis) == RACE_VAMPIRE))
                        percent = 0;
                    else {
                        percent += number(1, 101);
                    }
                    if (percent <= learned / 2) {
                        sprintf(text, "%s", out);

                        if (GET_SEX(ch) == SEX_MALE)
                            strcpy(sex, "a man");
                        else if (GET_SEX(ch) == SEX_FEMALE)
                            strcpy(sex, "a woman");
                        else
                            strcpy(sex, "someone");
    
                        k = apply_accent(buf1, ch, lis);
                        sprintf(buf, "You hear %s's voice from %s say, in %s:\n\r", sex,
                                rev_dir_name[j], buf1);
    
                        /* Replaced by the above 4/21/2001 -Nessalin
                         * sprintf (buf, "A voice from %s says:\n\r",
                         * rev_dir_name[j]); */
    
                        send_to_char(buf, lis);
                        send_translated(ch, lis, text);
                        send_comm_to_clairsentient(lis, ch, "", "says", "exclaims", "asks", argument,
                                                   FALSE);
                        learn_language(ch, lis, k);
                    } else if( has_skill(lis, SKILL_LISTEN) ) {
                        gain_skill(lis, SKILL_LISTEN, 1);
                    }
                }
            }
        }
    }
        /* start listening rooms  - Tenebrius 2005/03/27  */
        for (tmp_wlist = ch->in_room->watched_by; tmp_wlist; tmp_wlist = tmp_wlist->next_watched_by) {
            room = tmp_wlist->room_watching;
            if (IS_SET(tmp_wlist->watch_type, WATCH_LISTEN)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                        sprintf(text, "%s", out);
                        k = apply_accent(tmpbuf, ch, lis);

                        if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                            if (AWAKE(lis) && char_can_see_char(lis, ch)) {
				strcpy(buf1, PERS(lis, tCh));
                                sprintf(buf, "%s%s%s%s %s %s, in %s:\n\r",
                                    tmp_wlist->view_msg_string,
			            how[0] != '\0' ? how : "",
                                    how[0] != '\0' ? ", " : "",
			            PERS(lis, ch),
                                    text[strlen(text) - 1] == '!' ? "exclaims to" : text[strlen(text) - 1] == '?' ? "asks" : "says to",
				    buf1,
				    tmpbuf);
			    }
			} else {
                            if (GET_SEX(ch) == SEX_MALE)
                                strcpy(sex, "a man");
                            else if (GET_SEX(ch) == SEX_FEMALE)
                                strcpy(sex, "a woman");
                            else
                                strcpy(sex, "someone");

                            sprintf(buf, "%syou hear %s's voice %s, in %s:\n\r",
                                tmp_wlist->listen_msg_string, sex, text[strlen(text) - 1] == '!' ? "exclaim" : text[strlen(text) - 1] == '?' ? "ask" : "say", tmpbuf);
			}

                        if (send_to_char_parsed(ch, lis, buf, FALSE))
                            send_translated(ch, lis, text);
                        send_comm_to_clairsentient(lis, ch, "", "says", "exclaims", "asks",
                                                   argument, FALSE);
                        learn_language(ch, lis, k);
                    }
            } else if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                        sprintf(text, "%s", out);
			sprintf(buf1, "%s %s%s",
                            text[strlen(text) - 1] == '!' ? "exclaims something to" : text[strlen(text) - 1] == '?' ? "asks" : "says something to",
			    PERS(lis, tCh),
                            text[strlen(text) - 1] == '?' ? " something" : "");
                        sprintf(buf, "%s%s%s%s %s.\n\r",
                            tmp_wlist->view_msg_string,
			    how[0] != '\0' ? how : "",
                            how[0] != '\0' ? ", " : "",
			    PERS(lis, ch), buf1);
                        send_to_char_parsed(ch, lis, buf, FALSE);
		    }
	    }
        }
        /* end listening rooms */
}

void
cmd_tell(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char pzName[MAX_INPUT_LENGTH];
    CHAR_DATA *tCh;

    if (!(*argument)) {
        send_to_char("Tell what to whom?\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, CHAR_AFF_SILENCE)) {
        act("$n tries to mumble something.", TRUE, ch, 0, 0, TO_ROOM);
        act("You can't seem to say anything!", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (affected_by_spell(ch, PSI_BABBLE)) {
        gprintf(NULL, ch->in_room, "$csc babble$cy incoherently.\n", ch, ch);
        return;
    }

    argument = one_argument(argument, pzName, sizeof(pzName));
    tCh = get_char_room_vis(ch, pzName);

    if (!tCh) {
        send_to_char("You don't see that person here.\n\r", ch);
        return;
    }

    if (ch == tCh) {
        act("Why talk to yourself?", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!AWAKE(tCh)) {
        act("Don't bother $M, $E's sleeping.", FALSE, ch, 0, tCh, TO_CHAR);
        return;
    }

    /* i counter removed for brevity -Morg */
    for (; *(argument) == ' '; argument++);

    if( !*argument ) {
        act("What do you want to say?", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    char_tell_char(ch, tCh, argument);
}


void
cmd_ooc(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *lis;
    char buf[MAX_STRING_LENGTH];

    for (; *(argument) == ' '; argument++);

    if (!*(argument))
        send_to_char("Yes, but what do you want to say?\n\r", ch);
    else {
        snprintf(buf, sizeof(buf), "%s OOCs: %s\n\r", MSTR(ch, name), argument);
        send_to_monitor(ch, buf, MONITOR_COMMS);

        if( !IS_SET(ch->specials.brief, BRIEF_OOC) ) {
            cprintf(ch, "You say, out of character:\n\r     \"%s\"\n\r", argument);
        } else {
            cprintf(ch, "You OOC: %s\n\r", argument);
        }

        for (lis = ch->in_room->people; lis; lis = lis->next_in_room) {
            if (ch != lis) {
                if( !IS_SET(lis->specials.brief, BRIEF_OOC)) {
                    cprintf(lis, "%s says, out of character:\n\r     \"%s\"\n\r", capitalize(PERS(lis, ch)), argument);
                } else {
                    cprintf(lis, "%s OOCs: %s\n\r", capitalize(PERS(lis, ch)), argument);
                }
            }
        }
    }
}


void
cmd_iooc(CHAR_DATA *ch, const char *argument, Command cmd, int count)
{
    staff_comm_helper(ch, argument, "__", 1, 1, 1);
}


void
shout_into_wagon(CHAR_DATA * ch,        /* The person shouting   */
                 const char *message,   /* what is being shouted */
                 const char *sex)
{                               /* gender-string of ch   *//* Shout for wagons in the room */
    struct obj_data *tmp_obj;
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    CHAR_DATA *listener;
    int k;

    for (tmp_obj = ch->in_room->contents; tmp_obj; tmp_obj = tmp_obj->next_content) {
        if (tmp_obj->obj_flags.type == ITEM_WAGON) {
            /* send shout to 'pilot' room (if it exists) */
            if (((tmp_obj->obj_flags.value[2]) > -1) && get_room_num(tmp_obj->obj_flags.value[2])) {
                for (listener = get_room_num(tmp_obj->obj_flags.value[2])->people; listener;
                     listener = listener->next_in_room)
                    if (AWAKE(listener) && !IS_AFFECTED(listener, CHAR_AFF_DEAFNESS)) {
                        k = apply_accent(buf2, ch, listener);
                        sprintf(buf, "You hear %s's voice shout from outside in %s:", sex, buf2);
                        act(buf, FALSE, listener, 0, 0, TO_CHAR);

                        send_translated(ch, listener, message);
                        send_comm_to_clairsentient(listener, ch, "", "shouts", "shouts", "shouts",
                                                   message, FALSE);
                        learn_language(ch, listener, k);
                    }
            }
            if (((tmp_obj->obj_flags.value[0]) > -1)
                && (tmp_obj->obj_flags.value[0] != tmp_obj->obj_flags.value[2])
                && (get_room_num(tmp_obj->obj_flags.value[0]))) {
                for (listener = get_room_num(tmp_obj->obj_flags.value[0])->people; listener;
                     listener = listener->next_in_room)
                    if (AWAKE(listener) && !IS_AFFECTED(listener, CHAR_AFF_DEAFNESS)) {
                        k = apply_accent(buf2, ch, listener);
                        sprintf(buf, "You hear %s's voice shout from outside in %s:", sex, buf2);
                        act(buf, FALSE, listener, 0, 0, TO_CHAR);

                        send_translated(ch, listener, message);
                        send_comm_to_clairsentient(listener, ch, "", "shouts", "shouts", "shouts",
                                                   message, FALSE);
                        learn_language(ch, listener, k);
                    }
            }
        }
    }
}                               /* End shouting into wagons */

int
is_wagon_with_audible_room(OBJ_DATA * obj, void *room_num)
{
    if ((obj->obj_flags.type == ITEM_WAGON)
        && ((obj->obj_flags.value[0] == (long int) room_num)
            || (obj->obj_flags.value[2] == (long int) room_num))) {
        return 1;
    }

    return 0;
}

/* Goes through the list of objects in the world looking for wagons,
 * it then checks if the player is in either the entrance or front of the
 * wagon.  Players in those rooms have their shouts passed out of the 
 * wagon  -Nessalin 2/16/97 */
void
shout_out_of_wagon(CHAR_DATA * ch,      /* Character shouting */
                   const char *message, /* What is shouted    */
                   const char *sex)     /* Sex of ch          */
{
    OBJ_DATA *tmp_obj;          /* Used for finding wagons */
    CHAR_DATA *listener;        /* who hears the shout     */
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];       /* What listener hears     */
    int k;

    // FIXME:  object_list_foreach
    tmp_obj = object_list_find_by_func(is_wagon_with_audible_room, (void *)(long)ch->in_room->number);
    if (!tmp_obj)
        return;

    if (!tmp_obj->in_room)
        return;

    for (listener = tmp_obj->in_room->people; listener; listener = listener->next_in_room) {
        if (AWAKE(listener) && !IS_AFFECTED(listener, CHAR_AFF_DEAFNESS)) {
            k = apply_accent(buf2, ch, listener);
            sprintf(buf, "You hear %s's voice shout from inside %s in %s:", sex,
                    OSTR(tmp_obj, short_descr), buf2);
            act(buf, FALSE, listener, 0, 0, TO_CHAR);
            send_translated(ch, listener, message);
            send_comm_to_clairsentient(listener, ch, "", "shouts", "shouts", "shouts", message,
                                       FALSE);
            learn_language(ch, listener, k);
        }
    }
}

void
shout_to_nearby_rooms(CHAR_DATA * ch,   /* The shouter         */
                      const char *message,      /* What is shouted     */
                      const char *sex)
{                               /* gender string of ch */
    int exit, k;
    CHAR_DATA *listener;
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

    for (exit = 0; exit < 6; exit++)
        if (ch->in_room->direction[exit]) {
            for (listener = ch->in_room->direction[exit]->to_room->people; listener;
                 listener = listener->next_in_room)
                if (AWAKE(listener) && !IS_AFFECTED(listener, CHAR_AFF_DEAFNESS)) {
                    k = apply_accent(buf2, ch, listener);
                    sprintf(buf, "You hear %s's voice shout from %s in %s:", sex,
                            rev_dir_name[exit], buf2);
                    act(buf, FALSE, listener, 0, 0, TO_CHAR);

                    send_translated(ch, listener, message);
                    send_comm_to_clairsentient(listener, ch, "", "shouts", "shouts", "shouts",
                                               message, FALSE);
                    learn_language(ch, listener, k);
                }
        }
}

void
shout_to_room(CHAR_DATA * ch, const char *how, const char *message)
{
    CHAR_DATA *listener;
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    int k;

    for (listener = ch->in_room->people; listener; listener = listener->next_in_room) {
        if (ch != listener && AWAKE(listener)
            && !IS_AFFECTED(listener, CHAR_AFF_DEAFNESS)) {
            sprintf(buf, "%s%s", how[0] != '\0' ? capitalize(how) : "", how[0] != '\0' ? ", " : "");

            sprintf(buf2, "%s shouts, in ", VOICE(listener, ch));
            strcat(buf, buf2);

            // Apply the accent
            k = apply_accent(buf2, ch, listener);
            strcat(buf, buf2);

            strcat(buf, ":\n\r");

            if (send_to_char_parsed(ch, listener, buf, FALSE))
                send_translated(ch, listener, message);
            send_comm_to_clairsentient(listener, ch, how, "shouts", "shouts", "shouts", message,
                                       TRUE);
            learn_language(ch, listener, k);
        }
    }
}


void
shout_to_watching_rooms(CHAR_DATA * ch, const char *how, const char *argument)
{
    int k;
    struct room_data *room;
    CHAR_DATA *lis;
    char sex[256];
    char buf[MAX_STRING_LENGTH];
    char out[MAX_STRING_LENGTH];
    char text[MAX_STRING_LENGTH];
    char tmpbuf[MAX_STRING_LENGTH];
    struct watching_room_list *tmp_wlist;

        strcpy(out, argument);

        /* start listening rooms  - Tenebrius 2005/03/27  */
        for (tmp_wlist = ch->in_room->watched_by; tmp_wlist; tmp_wlist = tmp_wlist->next_watched_by) {
            room = tmp_wlist->room_watching;
            if (IS_SET(tmp_wlist->watch_type, WATCH_LISTEN)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
                        sprintf(text, "%s", out);
                        k = apply_accent(tmpbuf, ch, lis);

                        if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                            if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                                sprintf(buf, "%s%s%s%s shouts, in %s:\n\r",
                                    tmp_wlist->view_msg_string,
			            how[0] != '\0' ? how : "",
                                    how[0] != '\0' ? ", " : "",
			            PERS(lis, ch),
				    tmpbuf);
			    }
			} else {
                            if (GET_SEX(ch) == SEX_MALE)
                                strcpy(sex, "a man");
                            else if (GET_SEX(ch) == SEX_FEMALE)
                                strcpy(sex, "a woman");
                            else
                                strcpy(sex, "someone");

                            sprintf(buf, "%syou hear %s's voice shout, in %s:\n\r",
                                tmp_wlist->listen_msg_string, sex, tmpbuf);
			}

                        if (send_to_char_parsed(ch, lis, buf, FALSE))
                            send_translated(ch, lis, text);
                        send_comm_to_clairsentient(lis, ch, "", "shouts", "shouts", "shouts",
                                                   argument, FALSE);
                        learn_language(ch, lis, k);
                    }
            } else if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                        sprintf(text, "%s", out);
                        sprintf(buf, "%s%s%s%s shouts something.\n\r",
                            tmp_wlist->view_msg_string,
			    how[0] != '\0' ? how : "",
                            how[0] != '\0' ? ", " : "",
			    PERS(lis, ch));
                        send_to_char_parsed(ch, lis, buf, FALSE);
		    }
	    }
        }
        /* end listening rooms */
}

void
cmd_shout(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH], sex[256];
    char how[MAX_STRING_LENGTH];

    /* i counter removed for brevity  -Morg */
    for (; *(argument) == ' '; argument++);

    /* figure out 'how' it was shouted */
    /* ie: shout (in a quiet voice) hey man
     *  or  shout -hurridly got any pot?
     */
    if (argument[0] == '-' || argument[0] == '(' || argument[0] == '[')
        argument = one_argument_delim(argument, how, MAX_STRING_LENGTH, argument[0]);
    else
        how[0] = '\0';

    if (IS_AFFECTED(ch, CHAR_AFF_SILENCE)) {
        send_to_char("You can't seem to open your mouth.\n\r", ch);
        return;
    }

    if (affected_by_spell(ch, PSI_BABBLE)) {
        gprintf(NULL, ch->in_room, "$csc babble$cy incoherently.\n", ch, ch);
        return;
    }

    if (strlen(argument) > 400) {
        send_to_char("That is a little long for one shout, try shortening it" " up\n\r", ch);
        return;
    }

    if (!*argument) {
        act("What do you want to shout?", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* set gender of shouter */
    if (GET_SEX(ch) == SEX_MALE)
        strcpy(sex, "a man");
    else if (GET_SEX(ch) == SEX_FEMALE)
        strcpy(sex, "a woman");
    else
        strcpy(sex, "someone");

    send_comm_to_monitor(ch, how, "shouts", "shouts", "shouts", argument);
    send_comm_to_reciters(ch, argument);
    send_comm_to_clairsentient(ch, ch, how, "shouts", "shouts", "shouts", argument, TRUE);

    sprintf(buf, "%s%s shout in %s:\n\r", how[0] != '\0' ? capitalize(how) : "",
            how[0] != '\0' ? ", you" : "You", has_skill(ch,
                                                        GET_SPOKEN_LANGUAGE(ch)) ?
            skill_name[GET_SPOKEN_LANGUAGE(ch)] : "an unfamiliar tongue");

    if (send_to_char_parsed(ch, ch, buf, FALSE)) {
        sprintf(buf, "     \"%s\"\n\r", argument);
        send_to_char(buf, ch);
    } else
        return;

    shout_to_room(ch, how, argument);   /* Shout to current room     */

    shout_to_nearby_rooms(ch, argument, sex);   /* shout to surrounding rooms */

    shout_into_wagon(ch, argument, sex);        /* shout to wagons           */

    shout_out_of_wagon(ch, argument, sex);      /* shout out of wagon        */

    shout_to_watching_rooms(ch, how, argument);
}


void
cmd_whisper(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vict, *tch, *lis;
    ROOM_DATA *room;
    char name[100], message[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char out[MAX_STRING_LENGTH];
    char how[MAX_STRING_LENGTH];
    char text[MAX_STRING_LENGTH];
    int percent = 0;
    int k;
    char tmpbuf[MAX_STRING_LENGTH];
    struct watching_room_list *tmp_wlist;

    argument = one_argument(argument, name, sizeof(name));

    /* Find out how they said it */
    if (argument[0] == '-' || argument[0] == '[' || argument[0] == '(' || argument[0] == '*')
        argument = one_argument_delim(argument, how, MAX_STRING_LENGTH, argument[0]);
    else
        how[0] = '\0';

    strcpy(message, argument);

    if (!*name || !*message)
        send_to_char("Who do you want to whisper to?\n\r", ch);
    else if (IS_AFFECTED(ch, CHAR_AFF_SILENCE)) {
        act("$n tries to mumble something.", TRUE, ch, 0, 0, TO_ROOM);
        act("You can't seem to say anything!", FALSE, ch, 0, 0, TO_CHAR);
    } else if (affected_by_spell(ch, PSI_BABBLE)) {
        gprintf(NULL, ch->in_room, "$csc babble$cy incoherently.\n", ch, ch);
    } else if (!(vict = get_char_room_vis(ch, name)))
        send_to_char("No-one by that name here..\n\r", ch);
    else if( !*argument) {
        send_to_char("What do you want to whisper?\n\r", ch);
    } else if (IS_AFFECTED(vict, CHAR_AFF_DEAFNESS)) {
        if (CAN_SEE(vict, ch))
            act("$n whispers something to you, but you can't hear anything.", FALSE, ch, 0, vict,
                TO_VICT);

        act("You whisper to $N, but $E doesn't seem to notice.", FALSE, ch, 0, vict, TO_CHAR);
        act("$n whispers something to $N, but $E doesn't seem " "to notice.", FALSE, ch, 0, vict,
            TO_NOTVICT);
    } else {
        if (vict->specials.gone) {
            sprintf(buf, "%s is gone %s.\n\r", PERS(ch, vict), vict->specials.gone);
            send_to_char(buf, ch);
            return;
        }
        
        if (vict == ch) {
            send_comm_to_monitors(ch, vict, how, "mumbles to", "mumbles to", "mumbles to", argument);
            send_comm_to_reciters(ch, argument);

            sprintf(buf, "%s%s mumble to %s in %s:\n\r", how[0] == '\0' ? "" : capitalize(how),
                    how[0] == '\0' ? "You" : ", you", "yourself", has_skill(ch,
                                                                                GET_SPOKEN_LANGUAGE(ch))
                    ? skill_name[GET_SPOKEN_LANGUAGE(ch)] : "an unfamiliar tongue");

            if (send_to_char_parsed(ch, ch, buf, FALSE)) {
                sprintf(buf, "     \"%s\"\n\r", message);
                send_to_char(buf, ch);
            }
            send_comm_to_clairsentients(ch, ch, vict, how, "mumbles to", "mumbles to", "mumbles to",
                                        argument);
            strcpy(out, message);

        } else {
        
            send_comm_to_monitors(ch, vict, how, "whispers to", "whispers to", "whispers to", argument);
            send_comm_to_reciters(ch, argument);

            sprintf(buf, "%s%s whisper to %s in %s:\n\r", how[0] == '\0' ? "" : capitalize(how),
                    how[0] == '\0' ? "You" : ", you", PERS(ch, vict), has_skill(ch,
                                                                                GET_SPOKEN_LANGUAGE(ch))
                    ? skill_name[GET_SPOKEN_LANGUAGE(ch)] : "an unfamiliar tongue");
        
            if (send_to_char_parsed(ch, ch, buf, FALSE)) {
                sprintf(buf, "     \"%s\"\n\r", message);
                send_to_char(buf, ch);
            }
            send_comm_to_clairsentients(ch, ch, vict, how, "whispers to", "whispers to", "whispers to",
                                        argument);

            k = apply_accent(tmpbuf, ch, vict);
            strcpy(out, message);
            sprintf(buf, "%s%s%s whispers to you, in %s:\n\r", how[0] == '\0' ? "" : capitalize(how),
                    how[0] == '\0' ? "" : ", ", VOICE(vict, ch), tmpbuf);

            if (AWAKE(vict)) {
                if (send_to_char_parsed(ch, vict, buf, FALSE))
                    send_translated(ch, vict, out);
                send_comm_to_clairsentients(vict, ch, vict, how, "whispers to", "whispers to",
                                            "whispers to", argument);
                learn_language(ch, vict, k);
            }
        }
#ifndef DISABLE_EAVESDROP_ON_WHISPERS
        for (tch = ch->in_room->people; tch; tch = tch->next_in_room) {
            if ((tch != ch) && (tch != vict) && AWAKE(tch)
                && !tch->specials.fighting
                && !IS_AFFECTED(tch, CHAR_AFF_DEAFNESS)) {
                if ((IS_AFFECTED(tch, CHAR_AFF_LISTEN)) 
                     || (GET_RACE(tch) == RACE_VAMPIRE)) {
                    if (IS_IMMORTAL(tch) || (GET_RACE(tch) == RACE_VAMPIRE))
                        percent = 0;
                    else {
                        percent += number(1, 101);
                        if (tch->master && has_skill(tch, SKILL_LISTEN)
                            && ((ch == tch->master) || (vict == tch->master)))
                            percent -= tch->skills[SKILL_LISTEN]->learned / 10;
                    }

                    percent -= wis_app[GET_WIS(tch)].percep;

                    if (has_skill(tch, SKILL_LISTEN)
                        && (percent <= (tch->skills[SKILL_LISTEN]->learned / 3))) {
                        
                        sprintf(buf, "You overhear %s", VOICE(tch, ch));

                        k = apply_accent(tmpbuf, ch, tch);

                        if (vict == ch) {
                            sprintf(buf2, " mumble to %s%s%s, in %s:\n\r", HMFHRF(vict),
                                    (how[0] == '\0' ? "" : ", "), (how[0] == '\0' ? "" : how), tmpbuf);
                            send_comm_to_clairsentients(tch, ch, vict, how, "mumbles to",
                                                    "mumbles to", "mumbles to", argument);
                        } else {
                            sprintf(buf2, " whispers to %s%s%s, in %s:\n\r", PERS(tch, vict),
                                    (how[0] == '\0' ? "" : ", "), (how[0] == '\0' ? "" : how), tmpbuf);
                            send_comm_to_clairsentients(tch, ch, vict, how, "whispers to",
                                                    "whispers to", "whispers to", argument);
                        }

                        strcat(buf, buf2);

                        if (send_to_char_parsed(ch, tch, buf, FALSE))
                            send_translated(ch, tch, out);
                        learn_language(ch, tch, k);
                    } else {
                        if (vict == ch) {
                            sprintf(buf, "%s mumbles something to ", VOICE(tch, ch));
                            sprintf(buf2, "%s.\n\r", HMFHRF(vict));
                        } else {
                            sprintf(buf, "%s whispers something to ", VOICE(tch, ch));
                            sprintf(buf2, "%s.\n\r", PERS(tch, vict));
                        } 
                            strcat(buf, buf2);
                            send_to_char_parsed(ch, tch, buf, FALSE);                        
                        if( has_skill(tch, SKILL_LISTEN))
                            gain_skill(tch, SKILL_LISTEN, 1);                    
                    }
                } else {
#ifdef TALK_EMOTE_NONLISTENER
                    if (vict == ch) {
                        sprintf(buf, "%s%s%s mumbles something to ", (how[0] == '\0' ? "" : how),
                                (how[0] == '\0' ? "" : ", "), VOICE(tch, ch));
                    } else {
                        sprintf(buf, "%s%s%s whispers something to ", (how[0] == '\0' ? "" : how),
                                (how[0] == '\0' ? "" : ", "), VOICE(tch, ch));
                    }
#else
                    if (vict == ch) {
                        sprintf(buf, "%s mumbles something to ", VOICE(tch, ch));
                    } else {
                        sprintf(buf, "%s whispers something to ", VOICE(tch, ch));
                    }
#endif
                    if (vict == ch) {
                        sprintf(buf2, "%s.\n\r", HMFHRF(vict));
                    } else {
                        sprintf(buf2, "%s.\n\r", PERS(tch, vict));
                    }
                    strcat(buf, buf2);
                    send_to_char_parsed(ch, tch, buf, FALSE);
                }
            }
        }
#endif
        /* start listening rooms  - Tenebrius 2005/03/27  */
        for (tmp_wlist = ch->in_room->watched_by; tmp_wlist; tmp_wlist = tmp_wlist->next_watched_by) {
            room = tmp_wlist->room_watching;
            if (IS_SET(tmp_wlist->watch_type, WATCH_VIEW)) {
                for (lis = room->people; lis; lis = lis->next_in_room)
                    if (AWAKE(lis) && char_can_see_char(lis, ch)) {
                        sprintf(text, "%s", out);
                        if (vict == ch) {
                        strcpy(buf2, HMFHRF(vict));
                        sprintf(buf, "%s%s%s%s mumbles something to %s.\n\r",
                            tmp_wlist->view_msg_string,
                            how[0] != '\0' ? how : "",
                            how[0] != '\0' ? ", " : "",
                            HMFHRF(ch),
                            buf2);
                        } else {
			strcpy(buf2, PERS(lis, vict));
                        sprintf(buf, "%s%s%s%s whispers something to %s.\n\r",
                            tmp_wlist->view_msg_string,
			    how[0] != '\0' ? how : "",
                            how[0] != '\0' ? ", " : "",
			    PERS(lis, ch),
			    buf2);
                        }
			send_to_char_parsed(ch, lis, buf, FALSE);
		    }
	    }
        }
        /* end listening rooms */
    }
}


void
char_ask_char(CHAR_DATA * ch, CHAR_DATA * tCh, char *argument)
{
    int lang;
    char mess[MAX_STRING_LENGTH], out[MAX_STRING_LENGTH];
    CHAR_DATA *lis;

    lang =
        affected_by_spell(ch, SPELL_TONGUES) ? GET_SPOKEN_LANGUAGE(tCh) : GET_SPOKEN_LANGUAGE(ch);

    sprintf(mess, "%s asks %s: %s\n\r", MSTR(ch, name), MSTR(tCh, name), argument);
    send_to_monitor(ch, mess, MONITOR_COMMS);

    /* ch's message */
    sprintf(mess, "You ask $N, in %s:\n\r    \"%s\"", skill_name[lang], argument);
    act(mess, FALSE, ch, 0, tCh, TO_CHAR);

    /* target's message */
    if (IS_AFFECTED(tCh, CHAR_AFF_DEAFNESS)) {
        act("$n asks you something, but you can't make it out.", TRUE, ch, 0, tCh, TO_VICT);
        return;
    }

    if (has_skill(tCh, lang))
        /*   if (tCh->skills[lang]) */
        sprintf(mess, "$n asks you, in %s:", skill_name[lang]);
    else
        strcpy(mess, "$n asks you, in an unfamiliar tongue:");
    act(mess, FALSE, ch, 0, tCh, TO_VICT);
    strcpy(out, argument);
    send_translated(ch, tCh, out);

    /* room message */
    for (lis = ch->in_room->people; lis; lis = lis->next_in_room) {
        if (has_skill(lis, lang))
            /* if (lis->skills[lang]) */
            sprintf(mess, "$n asks %s, in %s:", PERS(lis, tCh), skill_name[lang]);
        else
            sprintf(mess, "$n talks to %s, in an unfamiliar tongue.", PERS(lis, tCh));

        if ((lis != ch) && (lis != tCh) && AWAKE(lis) && !IS_AFFECTED(lis, CHAR_AFF_DEAFNESS)) {
            act(mess, TRUE, ch, 0, lis, TO_VICT);
            strcpy(out, argument);
            send_translated(ch, lis, out);
        }
    }

}

void
cmd_write(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    struct obj_data *paper = 0, *pen = 0;
    char papername[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int pagenum;
    struct extra_descr_data *tmp_desc, *newdesc;

    argument = one_argument(argument, papername, sizeof(papername));

    if (!ch->desc)
        return;

    if (!*papername) {          /* nothing was delivered */
        send_to_char("Write? with what? ON what?" " What are you trying to do??\n\r", ch);
        return;
    }

    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
        cprintf(ch, "You have no %s to write on.\n\r", papername);
        return;
    }

    /* one object was found. Now for the other one. */
    if (!(pen = ch->equipment[ES])) {
        cprintf(ch, "You need something to write with.\n\r");
        return;
    }

    if (!CAN_SEE_OBJ(ch, pen)) {
        cprintf(ch, "You need something to write with.\n\r");
        return;
    }

    /* ok.. now let's see what kind of stuff we've found */
    if (pen->obj_flags.type != ITEM_PEN) {
        cprintf(ch, "%s is no good for writing.", capitalize(format_obj_to_char(pen, ch, 1)));
        return;
    } else if (paper->obj_flags.type != ITEM_NOTE) {
        cprintf(ch, "You can't write on %s.", format_obj_to_char(paper, ch, 1));
        return;
    } else if (paper->obj_flags.value[0] < 1) {
        cprintf(ch, "%s doesn't have any pages in it.",
                capitalize(format_obj_to_char(paper, ch, 1)));
        return;
    }
    /* doesn't have the RW skill of their language */
    else if (!has_skill(ch, GET_WRITTEN_LANGUAGE(ch))) {
        switch (GET_WRITTEN_LANGUAGE(ch)) {
        case 151:
            cprintf(ch, "You don't know how to write in sirihish.\n\r");
            break;
        case 152:
            cprintf(ch, "You don't know how to write in bendune.\n\r");
            break;
        case 153:
            cprintf(ch, "You don't know how to write in allundean.\n\r");
            break;
        case 154:
            cprintf(ch, "You don't know how to write in mirukkim.\n\r");
            break;
        case 155:
            cprintf(ch, "You don't know how to write in kentu.\n\r");
            break;
        case 156:
            cprintf(ch, "You don't know how to write in nrizkt.\n\r");
            break;
        case 157:
            cprintf(ch, "You don't know how to write in untal.\n\r");
            break;
        case 158:
            cprintf(ch, "You don't know how to write in cavilish.\n\r");
            break;
        case 159:
            cprintf(ch, "You don't know how to write in tatlum.\n\r");
            break;
        case 236:
            cprintf(ch, "You don't know how to write in anyar.\n\r");
            break;
        case 238:
            cprintf(ch, "You don't know how to write in heshrak.\n\r");
            break;
        case 240:
            cprintf(ch, "You don't know how to write in vlorak.\n\r");
            break;
        case 242:
            cprintf(ch, "You don't know how to write in domat.\n\r");
            break;
        case 244:
            cprintf(ch, "You don't know how to write in ghatti.\n\r");
            break;
        }
        return;
    }
    /* Since there is only one value for languages on ITEM_NOTE, don't
     * let them add to the item unless they're doing so in the same language
     * the item was started with. -Nessalin 8/10/96 */
    else if ((paper->obj_flags.value[2] != (GET_WRITTEN_LANGUAGE(ch)))
             && (paper->obj_flags.value[2] != 0)) {
        cprintf(ch,
                "You can only add to a written item in the same language it"
                " was originally written upon in.");
        return;

    } else {
        argument = one_argument(argument, arg2, sizeof(arg2));

        /* allow writing a title on the paper */
        if (!stricmp("title", arg2)) {
            if (paper->obj_flags.value[0] < 2) {
                cprintf(ch, "%s is just one piece of paper, it needs more to be titled.\n\r",
                        capitalize(format_obj_to_char(paper, ch, 1)));
                return;
            }

            if (find_ex_description("[BOOK_TITLE]", paper->ex_description, TRUE)) {
                cprintf(ch, "%s is already titled.\n\r",
                        capitalize(format_obj_to_char(paper, ch, 1)));
                return;
            }

            strcpy(arg2, argument);

            /* watch for no argument */
            if (arg2[0] == '\0') {
                cprintf(ch, "What do you want to title it?\n\r");
                return;
            }

            /* watch for too long */
            if (strlen(arg2) > 40) {
                cprintf(ch, "Titles are limited to 40 characters.\n\r");
                return;
            }

            set_obj_extra_desc_value(paper, "[BOOK_TITLE]", arg2);

            cprintf(ch, "You write the title on %s.\n\r", format_obj_to_char(paper, ch, 1));
            act("$n writes on $p.", FALSE, ch, paper, NULL, TO_ROOM);
            return;
        }

        /* probably can write */
        /* find the first free page */
        for (pagenum = 1; pagenum <= paper->obj_flags.value[0]; pagenum++) {
            snprintf(buf, sizeof(buf), "page_%d", pagenum);
            tmp_desc = find_edesc_info(buf, paper->ex_description);
            if (!tmp_desc) {
                global_game_stats.edescs++;
                paper->obj_flags.value[2] = (GET_WRITTEN_LANGUAGE(ch));
                CREATE(newdesc, struct extra_descr_data, 1);
                newdesc->description = (char *) 0;
                newdesc->next = paper->ex_description;
                paper->ex_description = newdesc;
                newdesc->keyword = strdup(buf);

                cprintf(ch, "Writing on page %d of %s.\n\rWrite your message.\n\r", pagenum,
                        format_obj_to_char(paper, ch, 1));

                act("$n begins to write on $p.", TRUE, ch, paper, 0, TO_ROOM);
                string_edit(ch, &newdesc->description, MAX_NOTE_LENGTH);
                return;
            }
        }

        cprintf(ch, "That appears to have been filled up already.\n\r");
    }
}

void
cmd_godcom(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    staff_comm_helper(ch, argument, "**", HIGHLORD, 0, 0);
}


void
cmd_immcom(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    staff_comm_helper(ch, argument, 0, STORYTELLER, 1, 0);
}

void
cmd_bldcom(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    staff_comm_helper(ch, argument, "()", BUILDER, 1, 0);
}

void
staff_comm_helper(CHAR_DATA *ch, const char *argument, const char *markers, int min_level, int invis_warn, int room_limited)
{
    CHAR_DATA *tCh, *pCh = 0;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    int quiet_level = QUIET_IMMCHAT;

    if (!ch->desc)
        return;

    if ((!IS_IMMORTAL(ch) && !ch->desc->original)
        || (ch->desc && ch->desc->original && !IS_IMMORTAL(ch->desc->original))) {
        send_to_char(TYPO_MSG, ch);
        return;
    } else {
        pCh = (ch->desc->original && IS_IMMORTAL(ch->desc->original)) ? ch->desc->original : ch;
    }

    if (!pCh || GET_LEVEL(pCh) < min_level) {
        send_to_char(TYPO_MSG, ch);
        return;
    }

    for (; *(argument) == ' '; argument++);

    if (ch->specials.il > 3 && invis_warn)
        send_to_char("FYI: You're i4, many people on immcomm cannot tell who you are.\n\r", ch);

    if( min_level >= HIGHLORD )
        quiet_level = QUIET_GODCHAT;

    if( min_level >= BUILDER )
        quiet_level = QUIET_BLDCHAT;

    for (d = descriptor_list; d; d = d->next) {
        if ((!d->connected)
            && (d->character) && (d->character->desc)
            && (!d->character->desc->str)) {
            tCh = ((d->original && IS_IMMORTAL(d->original)) ? d->original : d->character);

            // actor and listener should be in the same room if room_limited
            if (room_limited && d->character->in_room != ch->in_room )
                continue;

            if (GET_LEVEL(tCh) < min_level) continue;

            if (IS_IMMORTAL(tCh)
                && (!(IS_SET(tCh->specials.quiet_level, quiet_level)))) {
                if (pCh == ch) {
                    sprintf(buf, "%.1s%s%.1s: %s\n\r",
                            markers ? markers + 0 : "",
                            CAN_SEE(tCh, pCh) ? MSTR(pCh, name) : "Someone",
                            markers ? markers + 1 : "",
                            argument);
                    send_to_char(buf, d->character);
                } else {
                    sprintf(buf, "%.1s%s%.1s [M%d]: %s\n\r",
                            markers ? markers + 0 : "",
                            CAN_SEE(tCh, pCh) ? MSTR(pCh, name) : "Someone",
                            markers ? markers + 1 : "",
                            npc_index[ch->nr].vnum,
                            argument);
                    send_to_char(buf, d->character);
                }
            }
        }
    }
}

/* Currently not allowing immortals to SEE a mortal's "wish all <message>"
 * 12/6/00 -Savak
 *
 * Fixed 12/6/00 -Morg (who did not cause the problem in the first place)
 */
void
cmd_wish(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    DESCRIPTOR_DATA *d;
    char name[256], mesg[256], buf[512];
    CHAR_DATA *vict;

    if (IS_IMMORTAL(ch)) {
        send_to_char("But you're an Immortal yourself!\n\r", ch);
        return;
    }

    argument = one_argument(argument, name, sizeof(name));
    strcpy(mesg, argument);

    if (IS_SET(ch->specials.act, CFL_NOWISH)) {
        send_to_char("You lack the ability to wish.\n\r", ch);
        return;
    }

    if (!*name) {
        act("To whom do you want to wish? (use 'all' to communicate with all " "staff members.)",
            FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!*mesg) {
        act("What message do you want to send?", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!stricmp(name, "all")) {
        cprintf(ch, "You send this message to the staff:\n\r     \"%s\"\n\r", mesg);
        if (!ch->account) {
           sprintf(buf,
            "%s sends this message to the staff, from room #%d (%s)\n\r     \"%s\"\n\r",
            GET_NAME(ch), ch->in_room->number, ch->in_room->name, mesg);
        } else {
           sprintf(buf,
            "%s (%s) sends this message to the staff, from room #%d (%s)\n\r     \"%s\"\n\r",
            GET_NAME(ch), ch->account, ch->in_room->number, ch->in_room->name, mesg);
        }

        for (d = descriptor_list; d; d = d->next) {
            if (!d->connected   /* playing */
                && !d->str      /* not editing a string */
                && (IS_IMMORTAL(d->character)   /* is immortal */
                    ||(d->original ? IS_IMMORTAL(d->original) : FALSE))
                /* don't have quiet set */
                && (!IS_SET(d->character->specials.quiet_level, QUIET_WISH)
                    || ((d->original && IS_IMMORTAL(d->original))
                        ? !IS_SET(d->original->specials.quiet_level, QUIET_WISH) : FALSE))
                && (GET_LEVEL(d->character) > BUILDER)) { /* Have to be ST+ to hear the wish */
                send_to_char(buf, d->character);
            }
        }
    } else if ((vict = get_char_vis(ch, name)) && IS_IMMORTAL(vict)) {
        CHAR_DATA *show = NULL;

        if (IS_SET(vict->specials.quiet_level, QUIET_WISH)) {
            act("$N is not accepting wishes right now.", FALSE, ch, 0, vict, TO_CHAR);
            return;
        }
        if (IS_CREATING(vict)) {        /*  Dunno who removed this..  Kel  */
            act("$N is creating right now; try later.", FALSE, ch, 0, vict, TO_CHAR);
            return;
        }

        sprintf(buf, "You send this message to %s,\n\r     \"%s\"\n\r", GET_NAME(vict), mesg);
        send_to_char(buf, ch);

        if (vict->desc)
            show = vict;
        else {
            for (d = descriptor_list; d; d = d->next)
                if (d->original == vict)
                    show = d->character;
        }

        if (!ch->account) {
           sprintf(buf, "%s sends this message to you, from room #%d (%s),\n\r     \"%s\"\n\r",
                GET_NAME(ch), ch->in_room->number, ch->in_room->name, mesg);
        } else {
           sprintf(buf, "%s (%s) sends this message to you, from room #%d (%s),\n\r     \"%s\"\n\r",
                GET_NAME(ch), ch->account, ch->in_room->number, ch->in_room->name, mesg);
        }
        send_to_char(buf, show);
    } else {
        send_to_char("No staff member by that name is currently visible.\n\r", ch);
        return;
    }
}

/* Once again known as "old_cmd_wish". -Morg  */
void
old_cmd_wish(CHAR_DATA * ch, const char *argument, int cmd)
{
    DESCRIPTOR_DATA *d;
    char name[256], mesg[256], buf[512];
    CHAR_DATA *vict;

    if (IS_IMMORTAL(ch)) {
        send_to_char("But you're an Immortal yourself!\n\r", ch);
        return;
    }

    argument = one_argument(argument, name, sizeof(name));
    strcpy(mesg, argument);

    if (IS_SET(ch->specials.act, CFL_NOWISH)) {
        send_to_char("You lack the ability to commune with the Gods!\n\r", ch);
        return;
    }

    if (!*name) {
        act("To whom do you want to wish? (use 'all' to communicate with all " "Immortals.)", FALSE,
            ch, 0, 0, TO_CHAR);
        return;
    }

    if (!*mesg) {
        act("What message do you want to send?", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!stricmp(name, "all")) {
        sprintf(buf, "You send this message to the staff:\n\r     \"%s\"\n\r", mesg);
        send_to_char(buf, ch);

        for (d = descriptor_list; d; d = d->next)
            if ((!d->connected)
                && (!d->character->desc->str)
                && (IS_IMMORTAL(d->character))
                && (!IS_SET(d->character->specials.quiet_level, QUIET_WISH))) {
                sprintf(buf, "%s sends this message to the staff,\n\r     \"%s\"\n\r", GET_NAME(ch),
                        mesg);
                send_to_char(buf, d->character);
            }
    } else if ((vict = get_char_vis(ch, name)) && IS_IMMORTAL(vict)) {
        if (IS_SET(vict->specials.quiet_level, QUIET_WISH)) {
            act("$N is not accepting mesages right now.", FALSE, ch, 0, vict, TO_CHAR);
            return;
        }
        if (IS_CREATING(vict)) {        /*  Dunno who removed this..  Kel  */
            act("$N is creating right now; try later.", FALSE, ch, 0, vict, TO_CHAR);
            return;
        }

        sprintf(buf, "You send this message to %s,\n\r     \"%s\"\n\r", GET_NAME(vict), mesg);
        send_to_char(buf, ch);

        sprintf(buf, "%s sends this message to you,\n\r     \"%s\"\n\r", GET_NAME(ch), mesg);
        send_to_char(buf, vict);
    } else {
        send_to_char("No staff member by that name is currently visible.\n\r", ch);
        return;
    }
}


void
cmd_system(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];

    argument = skip_spaces(argument);

    sprintf(buf, "\aSYSTEM: %s\n\r", argument);
    send_to_all(buf);
}

void
slur_speech(char *comm, char *out)
{
    int cc = 0, oc = 0, i, max;

    while (comm[cc] != '\0') {
        if (IS_VOWEL(comm[cc])) {
            max = number(1, 3);
            for (i = 0; i < max; i++)
                out[oc++] = comm[cc];
            cc++;
        } else
            out[oc++] = comm[cc++];
    }

    out[oc] = '\0';
}


void
cmd_think(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *tar_ch = NULL, *psi = NULL;
    char how[MAX_STRING_LENGTH];
    char hbuf[MAX_STRING_LENGTH];
    char tbuf[MAX_STRING_LENGTH];
    int ts_num = 0, emp_num = 0;

    for (; *(argument) == ' '; argument++);

    // Look for an emotion within the thought
    if (argument[0] == '-' || argument[0] == '(' || argument[0] == '[' || argument[0] == '*')
        argument = one_argument_delim(argument, how, MAX_STRING_LENGTH, argument[0]);
    else
        how[0] = '\0';

    if (!*(argument)) {
        send_to_char("Yes, but what do you want to think?\n\r", ch);
        return;
    }

    if (strlen(how))
        sprintf(hbuf, "Feeling %s, ", how);
    else
        hbuf[0] = '\0';

    if (GET_POS(ch) == POSITION_SLEEPING)
        strcpy(tbuf, "dream");
    else
        strcpy(tbuf, "think");


    sprintf(buf, "%s%s %ss: %s\n\r", hbuf, MSTR(ch, name), tbuf, argument);
    send_to_monitor(ch, buf, MONITOR_COMMS);

    cprintf(ch, "%s%cou %s:\n\r     \"%s\"\n\r", hbuf, strlen(hbuf) ? 'y' : 'Y', tbuf, argument);

    // Show the thought to the appropriate people in the room
    for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room) {
        if (IS_IMMORTAL(tar_ch) && has_skill(tar_ch, PSI_THOUGHTSENSE)) {
            sprintf(buf, "%s$n %ss:\n\r     \"%s\"", hbuf, tbuf, argument);
            act(buf, TRUE, ch, 0, tar_ch, TO_VICT);
            /* THOUGHTSENSE check. */
        } else if ((has_skill(tar_ch, PSI_THOUGHTSENSE))
                   && (GET_RACE_TYPE(ch) == RTYPE_HUMANOID)
                   && can_use_psionics(tar_ch)
                   && can_use_psionics(ch)
                   && (tar_ch != ch)
                   && (!IS_IMMORTAL(ch))) {
	    // Set skill levels so we can modify based on domes in the room
	    if (has_skill(tar_ch, PSI_THOUGHTSENSE))
	        ts_num = tar_ch->skills[PSI_THOUGHTSENSE]->learned;
	    if (has_skill(tar_ch, PSI_EMPATHY))
                emp_num = tar_ch->skills[PSI_EMPATHY]->learned;
	    // Dome check
	    for (psi = tar_ch->in_room->people; psi; psi = psi->next_in_room) {
		if (psi != tar_ch && affected_by_spell(psi, PSI_DOME)) {
                    if (has_skill(psi, PSI_DOME)) {
		       ts_num -= psi->skills[PSI_DOME]->learned / 2;
		       emp_num -= psi->skills[PSI_DOME]->learned / 2;
		    }
		}
	    }

            /* If someone in the room has the latent psionic thoughtsense
             * ability, then we need to do a skillcheck and see if they
             * heard the thought.  Conditions which will prevent the
             * thought from being heard would be:
             * 1. thinker is not one of the 'humanoid' races
             * 2. psionicist has psionic_suppression or barrier affect
             * 3. thinker has barrier affect
             * 4. thinker is the psionicist
             * 5. thinker is an immortal
             */
            if (skill_success(tar_ch, NULL, ts_num)) {
                // They embedded an emotion in their thought, let an empath
                // feel it also.
                if (strlen(how) && has_skill(tar_ch, PSI_EMPATHY)
                    && skill_success(tar_ch, NULL, emp_num)) {
                    sprintf(buf, "%s feels %s.", PERS(tar_ch, ch), how);
                    send_empathic_msg(tar_ch, buf);     // Raw msg with formatting
                } else {        // They failed the skillcheck
                    gain_skill(tar_ch, PSI_EMPATHY, 1);
                }

                // Now show the thought
                cprintf(tar_ch, "%s %ss:\n\r     \"%s\"\n\r", capitalize(PERS(tar_ch, ch)), tbuf,
                        argument);
            } else {            // They failed the identifying skillcheck
                gain_skill(tar_ch, PSI_THOUGHTSENSE, 1);
                // See if they at least can tell someone
                if (skill_success(tar_ch, NULL, ts_num)) {
                    // They embedded an emotion in their thought, let an empath
                    // feel it also.
                    if (strlen(how) && has_skill(tar_ch, PSI_EMPATHY)
                        && skill_success(tar_ch, NULL, emp_num)) {
                        sprintf(buf, "Someone feels %s.", how);
                        send_empathic_msg(tar_ch, buf); // Raw msg with formatting
                    } else {    // They failed the skillcheck
                        gain_skill(tar_ch, PSI_EMPATHY, 1);
                    }

                    // Now show the thought
                    cprintf(tar_ch, "Someone %ss:\n\r     \"%s\"\n\r", tbuf, argument);
                }
            }
        }
    }

    // Send it to any psionicists who are in contact with them
    if (can_use_psionics(ch)) {
        if (strlen(hbuf)) {
            sprintf(buf, "%s feels %s.", MSTR(ch, short_descr), how);
            send_to_empaths(ch, buf);   // Checks for empaths & formats msg
        }
        send_thought_to_listeners(ch, argument);
        // Send it to anyone who is immersed in the Way
        if (strlen(hbuf)) {
            sprintf(buf, "Someone feels %s.", how);
	    send_emotion_to_immersed(ch, buf);
	}
	send_thought_to_immersed(ch, argument);
    }
}

void
exhibit_emotion(CHAR_DATA * ch, const char *feeling)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *tar_ch = NULL, *psi = NULL;
    int emp_num = 0;

    if (!ch || !feeling) // NULL check
        return;

    // Tell the monitors how the person feels
    sprintf(buf, "%s feels %s\n\r", MSTR(ch, name), feeling);
    send_to_monitor(ch, buf, MONITOR_COMMS);

    // Show the emotion to the appropriate people in the room
    for (tar_ch = ch->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room) {
        if (IS_IMMORTAL(tar_ch) && (tar_ch != ch)) {
            sprintf(buf, "%s feels %s", MSTR(ch, short_descr), feeling);
            send_empathic_msg(tar_ch, buf);     // Raw msg with formatting
        } else if ((has_skill(tar_ch, PSI_EMPATHY))
                   && (GET_RACE_TYPE(ch) == RTYPE_HUMANOID)
                   && can_use_psionics(tar_ch)
                   && can_use_psionics(ch)
                   && (tar_ch != ch)
                   && (!IS_IMMORTAL(ch))) {
            // Check to see if the psi is in mental contact with them already.
            if (tar_ch->specials.contact == ch)
                continue;

	    if (has_skill(tar_ch, PSI_EMPATHY))
                emp_num = tar_ch->skills[PSI_EMPATHY]->learned;
	    // Dome check
	    for (psi = tar_ch->in_room->people; psi; psi = psi->next_in_room) {
		if (psi != tar_ch && affected_by_spell(psi, PSI_DOME)) {
                    if (has_skill(psi, PSI_DOME))
		       emp_num -= psi->skills[PSI_DOME]->learned / 2;
		}
	    }

            /* If someone in the room has the latent psionic empathy
             * ability, then we need to do a skillcheck and see if they
             * sense the emotion.  Conditions which will prevent the
             * emotion from being sensed would be:
             * 1. feeler is not one of the 'humanoid' races
             * 2. psionicist has psionic_suppression or barrier affect
             * 3. feeler has barrier affect
             * 4. feeler is the psionicist
             * 5. feeler is an immortal
             */
            if (skill_success(tar_ch, NULL, emp_num)) {
                sprintf(buf, "%s feels %s", PERS(tar_ch, ch), feeling);
                send_empathic_msg(tar_ch, buf); // Raw msg with formatting
            } else {            // They failed the identifying skillcheck
                gain_skill(tar_ch, PSI_EMPATHY, 1);
                // See if they at least can tell someone
                if (skill_success(tar_ch, NULL, emp_num)) {
                    sprintf(buf, "Someone feels %s", feeling);
                    send_empathic_msg(tar_ch, buf);     // Raw msg with formatting
                }
            }
        }
    }

    // Send it to any psionicists who are in contact with them
    sprintf(buf, "%s feels %s", MSTR(ch, short_descr), feeling);
    send_to_empaths(ch, buf);   // Checks for empaths & formats msg

    // Send it to anyone who is immersed in the Way
    sprintf(buf, "Someone feels %s", feeling);
    send_emotion_to_immersed(ch, buf);
}

void
cmd_feel(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char feeling[MAX_STRING_LENGTH];

    for (; *(argument) == ' '; argument++);

    if (!*(argument)) {
        send_to_char("Yes, but how do you want to feel?\n\r", ch);
        return;
    }

    strncpy(feeling, argument, sizeof(feeling));
    if (feeling[strlen(feeling) - 1] != '!' && feeling[strlen(feeling) - 1] != '?'
        && feeling[strlen(feeling) - 1] != '.')
        strcat(feeling, ".");

    // Echo to the person how they feel
    sprintf(buf, "You feel %s\n\r", feeling);
    send_to_char(buf, ch);

    // Make 'em feel it, and tell whoever should know about it
    exhibit_emotion(ch, feeling);
}

int
default_accent(CHAR_DATA * ch)
{
    int i, accent = LANG_NO_ACCENT;

    // Nomads should have the bendune accent
    if (GET_SUB_GUILD(ch) == SUB_GUILD_NOMAD)
        accent = LANG_BENDUNE_ACCENT;

    // Look at where they're from
    if (accent == LANG_NO_ACCENT) {
        switch (GET_START(ch)) {
            // Northern cities    
        case CITY_TULUK:
        case CITY_FREILS_REST:
        case CITY_REYNOLTE:
        case CITY_LUIR:
            accent = LANG_NORTH_ACCENT;
            break;

            // Tribal villages
        case CITY_ANYALI:
            accent = LANG_ANYAR_ACCENT;
            break;
        case CITY_TYN_DASHRA:
            accent = LANG_BENDUNE_ACCENT;
            break;

            // Southern cities
        case CITY_ALLANAK:
        case CITY_ALLANAK_GLADIATORS:
        case CITY_RS:
            accent = LANG_SOUTH_ACCENT;
            break;
        case CITY_LABYRINTH:
            accent = LANG_RINTH_ACCENT;
            break;

	    // Red Desert, Salt Flats and eastern areas
        case CITY_CENYR:
        case CITY_LANTHIL:
	    accent = LANG_EASTERN_ACCENT;
	    break;

	    // Western areas
        case CITY_MAL_KRIAN:
	    accent = LANG_WESTERN_ACCENT;
	    break;

	    // Tablelands
        case CITY_BW:
        case CITY_CAVERN:
        case CITY_DELF_OUTPOST:
	    accent = LANG_DESERT_ACCENT;
	    break;

            // Everything else
        case CITY_NONE:
        case CITY_CAI_SHYZN:
        case CITY_PTARKEN:
        default:
            break;
        }
    }
    // They should have the appropriate accent by this point.  If they also
    // have the right accent skill, we're done.
    if (accent != LANG_NO_ACCENT && has_skill(ch, accent))
        return accent;
    else                        // Still don't have a good match, reset
        accent = LANG_NO_ACCENT;

    // If all else fails, look at their skills and take the first one
    for (i = 0; accent_table[i].skillnum != LANG_NO_ACCENT; i++) {
        if (has_skill(ch, accent_table[i].skillnum)
            && ch->skills[accent_table[i].skillnum]->learned >= MIN_ACCENT_LEVEL) {
            accent = accent_table[i].skillnum;
            break;
        }
    }

    return accent;
}

/* Returns the language that the listener hears.  This may be different
 * from the language the speaker is actually speaking.
 */
int
apply_accent(char *buf, CHAR_DATA * ch, CHAR_DATA * lis)
{
    int i, j;

    // If these two error conditions are possible, default to the common
    // language.  These should never really be used.

    if (!buf)
        return LANG_COMMON;
    if (!ch || !lis) {
        return LANG_COMMON;
    }

    /* Clear out the buffer */
    strcpy(buf, "");

    // What the speaker is speaking
    j = GET_SPOKEN_LANGUAGE(ch);

    /* Conditions which must be met in order for an accent to be applied:
     * 1.  The speaker has at least 70% proficiency in the langauge
     * 2. The listener has at least 70% proficiency in the language
     */
    // Do they understand the language well enough to notice accents?
    if ((has_skill(ch, j) && ch->skills[j]->learned >= MIN_LANG_LEVEL)
        && (has_skill(lis, j) && lis->skills[j]->learned >= MIN_LANG_LEVEL)) {
        // Is the speaker's accent different from the listener's accent
        if (GET_ACCENT(ch) != GET_ACCENT(lis)) {
            for (i = 0; accent_table[i].skillnum != LANG_NO_ACCENT; i++) {
                if (accent_table[i].skillnum == GET_ACCENT(ch)) {
                    strcat(buf, accent_table[i].msg);
                    break;
                }
            }
        }
    }
    // Those with TONGUES sound like they're speaking in the listener's tongue
    if (affected_by_spell(ch, SPELL_TONGUES)) {
        j = GET_SPOKEN_LANGUAGE(lis);
        strcpy(buf, "");        // Also use the same accent, regardless.
    }
    // Add the name of the language if the listener knows it
    if (has_skill(lis, j) || (affected_by_spell(lis, PSI_COMPREHEND_LANGS) && can_use_psionics(ch)))
        strcat(buf, skill_name[j]);
    else
        strcpy(buf, "an unfamiliar tongue");

    return j;
}

void
learn_language(CHAR_DATA * ch, CHAR_DATA * lis, int language)
{
    int accent, lang, accent_factor = NEW_LANGUAGE_ODDS, lang_factor = NEW_LANGUAGE_ODDS;

    if (!ch || !lis)
        return;

    if (affected_by_spell(lis, PSI_COMPREHEND_LANGS)
        || GET_SUB_GUILD(lis) == SUB_GUILD_LINGUIST
        || GET_SUB_GUILD(lis) == SUB_GUILD_BARD) {
        lang_factor /= 2;
    }

    if (GET_SUB_GUILD(lis) == SUB_GUILD_LINGUIST
     || GET_SUB_GUILD(lis) == SUB_GUILD_BARD) {
        accent_factor /= 2;
    }

    /* Work-around to get the index to the language_table for this language.
     * We'll need it for the call to gain_language()
     */
    for (lang = 0; lang < MAX_TONGUES; lang++) {
        if (language_table[lang].spoken == language)
            break;
    }
    // All else fails, default to what the speaker's language is set to
    if (lang >= MAX_TONGUES)
        lang = GET_LANGUAGE(ch);

    // Learn more about speaking the language
    if (has_skill(lis, language) && has_skill(ch, language)
        && (ch->skills[language]->learned >= MIN_LANG_LEVEL)
        && (ch->skills[language]->learned > lis->skills[language]->learned)
        && (!skill_success(lis, NULL, lis->skills[language]->learned)))
        gain_language(lis, lang, 1);

    // A chance to learn how to speak the language
    if (!has_skill(lis, language)
        && (number(1, lang_factor) <= wis_app[GET_WIS(lis)].learn))
        init_skill(lis, language, 1);

    accent = GET_ACCENT(ch);
    /* Do they understand the language well enough to notice accents and
     * is the speaker's accent is strong enough?
     */
    if (has_skill(lis, language) && lis->skills[language]->learned >= MIN_LANG_LEVEL
        && has_skill(ch, accent) && ch->skills[accent]->learned >= MIN_ACCENT_LEVEL) {
        // Learn a bit more about the accent
        if (has_skill(lis, accent)
            && (ch->skills[accent]->learned > lis->skills[accent]->learned)
            && (!skill_success(lis, NULL, lis->skills[accent]->learned)))
            gain_accent(lis, accent, 1);

        // A chance to learn how to speak with the accent
        if (!has_skill(lis, accent)
            && (number(1, accent_factor) <= wis_app[GET_WIS(lis)].learn))
            init_skill(lis, accent, 1);
    }
}


void
cmd_discuss(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *npc;
    char *message = 0;
    char tmp[MAX_INPUT_LENGTH] = "";
    char topic[MAX_STRING_LENGTH] = "";
    char arg1[MAX_INPUT_LENGTH] = "";
    char arg2[MAX_INPUT_LENGTH] = "";
    int go_on = 1;
    int counter;

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    
    // Did they say who they wanted to discuss with?
    if (!*arg1) {
        cprintf(ch,
         "You must specify with whom you wish to have a discussion.\n\r");
        return;
    }

    // Did they say what they wanted to discuss?
    if (!*arg2) {
        cprintf(ch, "What do you want to discuss?\n\r");
        return;
    }

    // Did we find an npc?
    if (!(npc = get_char_room_vis(ch, arg1))) {
        if (!(npc = get_char_room_vis(ch, arg2))) {
            cprintf(ch, "You don't see any '%s' here to discuss with.\n\r", arg1);
            return;
        }
        // arg1 was the topic, switch
        strcpy(arg2, arg1);
    }

    // if will only talk to clan mates
    if ((has_special_on_cmd(npc->specials.programs, "npc_talk_clan", CMD_TALK)
     || find_ex_description("[discuss_clan_only]", npc->ex_description, TRUE))
      && !IS_IN_SAME_TRIBE(ch, npc)) {
        // give the same message as no topic or not npc
        cprintf(ch, "%s doesn't respond about %s.\n\r",
         capitalize(PERS(ch, npc)), arg2);
        return;
    }

    // build the topic name
    sprintf(topic, "[talk_%s]", arg2);
    message = find_ex_description(topic, npc->ex_description, TRUE);

    // if they're not an npc or they don't have a message
    if (!IS_NPC(npc) || !message) {
        cprintf(ch, "%s doesn't respond about %s.\n\r",
         capitalize(PERS(ch, npc)), arg2);
        return;
    }

    // found the message, tell them.
    cprintf(ch, "You begin discussing %s with %s.\n\r", arg2, PERS(ch, npc));

    // start building the command
    strcat(tmp, "say ");

    // Do a length check
    if (strlen(message) > 256) {
        strncat(tmp, message, 256);
        gamelogf( "Warning: String too long in edesc '[talk_%s]' on NPC#%d.", 
         arg2, npc_index[npc->nr].vnum);
    } else
        strcat(tmp, message);

    // send the message
    parse_command(npc, tmp);

    // look for other related topics
    counter = 1;
    while (go_on) {
        counter++;

        sprintf(topic, "[talk_%s_%d]", arg2, counter);

        message = find_ex_description(topic, npc->ex_description, TRUE);

        tmp[0] = '\0';

        if (!message)
            go_on = 0;
        else {
            strcat(tmp, "say ");
            if (strlen(message) > 256) {
                strncat(tmp, message, 256);
                gamelogf(
                 "Warning: String too long in edesc '[talk_%s_%d]' on NPC#%d.",
                 arg2, counter, npc_index[ch->nr].vnum);
            } else
                strcat(tmp, message);

            parse_command(npc, tmp);
        }
    }

    return;
}

