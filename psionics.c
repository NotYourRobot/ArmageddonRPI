/*
 * File: PSIONICS.C
 * Usage: Commands involving psionics.
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

/* Revision history.  Pls update as changes made.
 * Modified line 1614 so only humanoid shout 'Mindbender! on a
 * failed compelled.  12/18/1999 -Sanvean
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>

#include "constants.h"
#include "structs.h"
#include "db.h"
#include "event.h"
#include "skills.h"
#include "comm.h"
#include "guilds.h"
#include "utils.h"
#include "handler.h"
#include "parser.h"
#include "utility.h"
#include "limits.h"
#include "modify.h"
#include "psionics.h"
#include "clan.h"
#include "cities.h"
#include "watch.h"
#include "pather.h"
#include "info.h"

#define PSI_BASE    77

/* Global externals */
extern int get_normal_condition(CHAR_DATA * ch, int cond);
int psionic_affects[] = {
    PSI_BARRIER,
    PSI_CATHEXIS,
    PSI_CLAIRAUDIENCE,
    PSI_CLAIRVOYANCE,
    PSI_COERCION,
    PSI_COMPREHEND_LANGS,
    PSI_CONCEAL,
    PSI_CONTACT,
    PSI_CONTROL,
    PSI_DOME,
    PSI_HEAR,
    PSI_IMITATE,
    PSI_MAGICKSENSE,
    PSI_MESMERIZE,
    PSI_SENSE_PRESENCE,
    PSI_SHADOW_WALK,
    PSI_VANISH,
    -1                          // Terminal value
};

/* local vars */
struct listen_data *listen_list = NULL;
void apply_psi_to_char(CHAR_DATA * ch, int sk);
void apply_psi_to_char_modified(CHAR_DATA * ch, int sk, int mod);

/* Determines if the target is within the range of the psionicist's ability */
bool
is_in_psionic_range(CHAR_DATA * pCh, CHAR_DATA * tCh, int range)
{
    // Use the new xpather
    return (generic_astar(pCh->in_room, tCh->in_room, range, 30.0f, 0, 0,
            WAGON_NO_PSI_PATH, 0, 0, 0, 0, 0) >= 0);
}

/* Generates a generic, anonymous psionic sensation message and
 * sends it to the target */
void
psionic_failure_msg(CHAR_DATA * pCh, CHAR_DATA * tCh)
{
    char buf[MAX_STRING_LENGTH];
    char real_name_buf[MAX_STRING_LENGTH];
    char *messages[] = {
        "You feel a light touch upon your mind.",
        "For a fleeting moment, something feels out of place.",
        "You experience a brief moment of vertigo.",
        "A dull ache pounds into the back of your head.",
        "A splitting pain stabs through your eyes.",
        "A cold chill runs down your spine.",
        "Suddenly everything feels familiar to you.",
        "You hear your pulse throbbing steadily in your ears.",
        "For a moment you are unable to remember what you're doing.",
        "You feel a heavy weight on your mind, but it disappears quickly.",
        "Your vision greys momentarily, but recovers quickly.",
        "Your eyes throb in their sockets in time with your pulse.",
        "Your eyes feel hypersensitive to light, causing you discomfort.",
        "You hear a faint ringing in your ears, which quickly passes.",
        "Your temples throb as a mild headache sets in.",
        "A sense of dizziness overcomes you, which subsides as quickly as it came.",
        "You feel a feathery touch brush over your mind.",
        "Something tugs at the edge of your consciousness.",
        "An elusive touch skitters across the surface of your mind.",
        "Like a flutter of wings, something echoes at the corner of your mind.",
        "A sound like the whisper of wind through the grass passes through your mind.",
        "Elusive as a flutter in the darkness, something touches your consciousness.",
        "Something bumps at the edges of your consciousness.",
        "A feeling of being watched comes over you as something touches your mind.",
        "You feel uneasy as a light touch passes through your mind."
    };

    if (is_detected_psi(pCh, tCh)) {
        if( !IS_NPC(pCh)) {
            if( IS_NPC(tCh) ) {
                qgamelogf(QUIET_PSI, "%s (%s) is discovered by NPC #%d (%s)",
                 GET_NAME(pCh), pCh->account,
                 npc_index[tCh->nr].vnum,
                 REAL_NAME(tCh));
            } else {
                qgamelogf(QUIET_PSI, "%s (%s) is discovered by %s (%s)",
                 GET_NAME(pCh), pCh->account,
                 GET_NAME(tCh), tCh->account);
            }
        }

        snprintf(buf, sizeof(buf), "The image of %s wavers slightly in your mind.\n\r",
                 MSTR(pCh, short_descr));
        send_to_char(buf, tCh);
        send_to_char("You sense your identity has been discovered.\n\r", pCh);
    } else {
        // Randomly give them one of the messages in the messages[] array.
        cprintf(tCh, "%s\n\r", messages[number(0, sizeof(messages) / sizeof(messages[0]) - 1)]);
    }
}

bool
is_psionic_command(int cmd)
{
    int nr;

    switch (cmd) {
    case CMD_CEASE:
    case CMD_PSI:
    case CMD_COMMUNE:
        return TRUE;
    }

    for (nr = 1; nr < MAX_SKILLS; nr++) {
        if (skill[nr].sk_class == CLASS_PSIONICS && !stricmp(command_name[cmd], skill_name[nr]))
            return TRUE;
    }
    return FALSE;
}

/* psionic listening functions */

/* add player to listen list if player has listen skill */
void
add_listener(CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    struct listen_data *new_listener = NULL;
    struct listen_data *p = NULL;
    struct listen_data *l = NULL;
    struct affected_type *tmp_af = NULL;

    new_listener = (struct listen_data *) 0;
    p = (struct listen_data *) 0;

    if (!ch->skills[PSI_HEAR])
        return;

    if (!(tmp_af = affected_by_spell(ch, PSI_HEAR)))
        return;

    // Make sure the power field contains a valid range for hear
    if (tmp_af->power < AREA_ROOM || tmp_af->power > AREA_WORLD)
        tmp_af->power = AREA_ROOM;

    snprintf(buf, sizeof(buf), "Adding %s to the listener list (range %d).", MSTR(ch, name),
             tmp_af->power);
    shhlog(buf);

    // See if they're on the list already
    for (l = listen_list; l != NULL; l = l->next)
        if (l->listener == ch)
            break;
    if (l) {                    // They are, adjust their range.
        l->area = tmp_af->power;
        return;
    }
    // Make a new node and add it to the list
    CREATE(new_listener, struct listen_data, 1);
    new_listener->listener = ch;
    new_listener->area = tmp_af->power;
    new_listener->next = NULL;
    if (listen_list == NULL) {
        listen_list = new_listener;
    } else {
        for (p = listen_list; p->next != NULL; p = p->next);
        p->next = new_listener;
    }

    return;
}

bool
valid_listen_data(struct listen_data * data)
{
    char testpad[64];

    memset(testpad, 0, sizeof(testpad));

    if (memcmp(data->pad1, testpad, sizeof(testpad)) != 0)
        return FALSE;
    if (memcmp(data->pad2, testpad, sizeof(testpad)) != 0)
        return FALSE;
    if (memcmp(data->pad3, testpad, sizeof(testpad)) != 0)
        return FALSE;
    if (memcmp(data->pad4, testpad, sizeof(testpad)) != 0)
        return FALSE;

    return TRUE;
}

/* if character is on the listener list, remove the character from the list */
void
remove_listener(CHAR_DATA * ch)
{
    struct listen_data *p;
    struct listen_data *pp = NULL;
    char buf[MAX_STRING_LENGTH];

    for (p = listen_list; p != NULL; p = p->next) {
        if (p->listener == ch)
            break;
        pp = p;
    }

    if (p) {
        snprintf(buf, sizeof(buf), "Removing %s from the listener list.\n",
                 MSTR(p->listener, name));
        shhlog(buf);
        if (!pp) {              // Char was the top of the list
            listen_list = p->next;
        } else {                // Set the node ahead to the node behind what we're taking out
            pp->next = p->next;
        }

        p->listener = NULL;     // Doesn't hurt to remove the reference
        if (!valid_listen_data(p) || (pp && !valid_listen_data(pp))) {
            gamelogf("Maybe detected buffer overflow that corrupted a listen object in %s, %d",
                     __FILE__, __LINE__);
        }
        strncpy(p->pad1, "blahblahblah", strlen("blahblahblah"));

        free(p);                // Clean up the memory
    }

    return;
}

/* returns true if the psionicist is able to hear the contact */
bool
pass_control(struct listen_data * l, struct room_data * rm1, struct room_data * rm2)
{
//    int skill_num = 0;
    bool in_range = FALSE;
    bool success = FALSE;

// ? compile warning is correct, set but not used
//    if (l && l->listener && l->listener->skills[PSI_HEAR])
//        skill_num = l->listener->skills[PSI_HEAR]->learned;

    switch (l->area) {
    case AREA_ROOM:
        in_range = ((l->listener->in_room == rm1) || (l->listener->in_room == rm2));
        break;
    case AREA_ZONE:
        if (char_in_city(l->listener)) {
            in_range = ((char_in_city(l->listener) == room_in_city(rm1))
                        || (char_in_city(l->listener) == room_in_city(rm2)));
        } else {
            in_range = ((l->listener->in_room->zone == rm1->zone)
                        || (l->listener->in_room->zone == rm2->zone));
        }
        break;
    case AREA_WORLD:
        in_range = TRUE;
        break;
    default:
        in_range = FALSE;
        break;
    }

    if (in_range) {
        success = psi_skill_success(l->listener, NULL, PSI_HEAR, 0);
        if (!success && !number(0, 4))
            gain_skill(l->listener, PSI_HEAR, 1);
    }

    return success;
}

/* Placed in cmd_psi, to allow clairaudients to hear psionic transmissions */
/* on a selected individual(ch).  Track indicates whether the message */
/* is outgoing(1) from ch, or incoming(0) _to_ ch, from tar_ch.  */
void
clairaudient_listener(CHAR_DATA * ch, CHAR_DATA * tar_ch, int track, const char *mesg)
{
    char buf[MAX_STRING_LENGTH];
    int skill_perc;
    CHAR_DATA *psi;
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d; d = d->next) {
	if (d->connected != CON_PLYNG) // Not playing
	    continue;
        psi = d->character;
        if (!psi)
            continue;
        if (!affected_by_spell(psi, PSI_CLAIRAUDIENCE))
            continue;

        if (track) {            // ch is transmitting
            if (psi->specials.contact != ch)
                continue;
        } else {                // tar_ch is receiving
            if (psi->specials.contact != tar_ch)
                continue;
        }

        if (is_detected_psi(ch, psi)) {
            snprintf(buf, sizeof(buf), "%s sends a telepathic message to %s:\n\r     \"%s\"\n\r",
                     capitalize(MSTR(ch, short_descr)), MSTR(tar_ch, short_descr), mesg);
        } else {
            snprintf(buf, sizeof(buf), "Someone sends a telepathic message to %s:\n\r     \"%s\"\n\r", MSTR(tar_ch, short_descr), mesg);
        }

        skill_perc =
            (psi->skills[PSI_CLAIRAUDIENCE] ? psi->skills[PSI_CLAIRAUDIENCE]->learned : 15);

        if (skill_success(ch, NULL, skill_perc))
            send_to_char(buf, psi);
    }
}

/* send psi contact to listeners */
void
send_to_listeners(const char *msg, CHAR_DATA * send, CHAR_DATA * rec)
{
    struct listen_data *p;
    char buf[MAX_STRING_LENGTH];

    for (p = listen_list; p != NULL; p = p->next) {
        if ((p->listener != send) && (p->listener != rec) &&
            (is_on_same_plane(p->listener->in_room, send->in_room)))
            if (pass_control(p, send->in_room, rec->in_room)
             && GET_RACE_TYPE(p->listener) == GET_RACE_TYPE(send)) {
                if (GET_GUILD(p->listener) != GUILD_PSIONICIST
                    && GET_GUILD(p->listener) != GUILD_LIRATHU_TEMPLAR)
                    snprintf(buf, sizeof(buf), "Someone whispers to you:\n\r     \"%s\"\n\r", msg);
                else
                    snprintf(buf, sizeof(buf), "You hear through the Way:\n\r     \"%s\"\n\r", msg);
                send_to_char(buf, p->listener);
            }
    }

    return;
}

/* Sends voice communication to clairaudient listeners when the speaker has
 * not directed the communication at anyone.
 *
 * lis = Victim who is hearing communications
 * ch  = Speaker
 */
void
send_comm_to_clairsentient(CHAR_DATA * lis, CHAR_DATA * ch, const char *how, const char *verb,
                           const char *exclaimVerb, const char *askVerb, const char *output,
                           bool seen)
{
    char msg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char tmpbuf[MAX_STRING_LENGTH];
    char out[MAX_STRING_LENGTH];
    struct obj_list_type *p, *char_list = NULL;
    CHAR_DATA *psi;
    DESCRIPTOR_DATA *d;
    bool can_hear, can_see;
    int j, k;

    if (!lis || !ch)
        return;

    // If they have a barrier up, don't let the psi eavesdrop
    if (!can_use_psionics(lis))
        return;

    // Look at the list of clairsentients
    for (d = descriptor_list; d; d = d->next) {
	if (d->connected != CON_PLYNG) // Not playing
	    continue;
        psi = d->character;
        if (!psi)
            continue;
        can_hear = affected_by_spell(psi, PSI_CLAIRAUDIENCE) ? TRUE : FALSE;
        can_see = affected_by_spell(psi, PSI_CLAIRVOYANCE) ? TRUE : FALSE;
        if (!can_hear && !can_see)
            continue;
        if (lis != psi->specials.contact)
            continue;

        // If they are using clairvoyance, show sdescs
        if (can_see && seen) {
            /* prepare who is doing what */
            snprintf(buf, sizeof(buf), "%s%s%s %s", how[0] != '\0' ? how : "",
                     how[0] != '\0' ? ", " : "", PERS(lis, ch),
                     output[strlen(output) - 1] == '!' ? exclaimVerb : output[strlen(output) - 1] ==
                     '?' ? askVerb : verb);
        } else {                // Use voices
            if (ch == lis)
                sprintf(buf, "%s %s", VOICE(lis, ch),
                        output[strlen(output) - 1] ==
                        '!' ? exclaimVerb : output[strlen(output) - 1] == '?' ? askVerb : verb);
            else
                sprintf(buf, "%s%s voice %s", indefinite_article(GENDER(ch)), GENDER(ch),
                        output[strlen(output) - 1] ==
                        '!' ? exclaimVerb : output[strlen(output) - 1] == '?' ? askVerb : verb);
        }

        /* pass the arguments off to be parsed for us */
        if (!parse_emote_message(ch, buf, TRUE, msg, sizeof(msg), &char_list))
            /* parse fails, we fail */
            return;

        if (can_hear) {         // combine the parsed emote with the spoken output 
            k = apply_accent(tmpbuf, ch, psi);
            snprintf(out, sizeof(out), "Over the Way: %s, in %s:", msg, tmpbuf);
            gvprintf(ch, psi, NULL, out, char_list, 0, 0);
            // Scramble up as appropriate and send
            for (j = 0; *(output + j) == ' '; j++);
            cprintf(psi, "     \"%s\"\n\r", output + j);

            /* give 'em a chance to learn it */
            learn_language(ch, psi, k);
        } else {                // Just show them that something was said.
            snprintf(out, sizeof(out), "Over the Way: %s something.", msg);
            gvprintf(NULL, psi, NULL, out, char_list, 0, 0);
        }

        /* free the char_list data */
        while (((p) = (char_list))) {
            char_list = char_list->next;
            free(p);
        }
    }
}

/* Sends voice communication to clairaudient listeners when the speaker has
 * directed the communication at someone.
 *
 * lis = Victim who is hearing communications
 * ch  = Speaker
 * tch = Target of speech
 */
void
send_comm_to_clairsentients(CHAR_DATA * lis, CHAR_DATA * ch, CHAR_DATA * tch, const char *how,
                            const char *verb, const char *exclaimVerb, const char *askVerb,
                            const char *output)
{
    char msg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char tmpbuf[MAX_STRING_LENGTH];
    char action[MAX_STRING_LENGTH];
    char out[MAX_STRING_LENGTH];
    struct obj_list_type *p, *char_list = NULL;
    CHAR_DATA *psi;
    DESCRIPTOR_DATA *d;
    bool can_hear, can_see;
    char ch_voice[MAX_STRING_LENGTH];
//    const char *arg = NULL;
    int k;

    if (!lis || !ch || !tch)
        return;

    // If they have a barrier up, don't let the psi eavesdrop
    if (!can_use_psionics(lis))
        return;

    // Look at the list of clairsentients
    for (d = descriptor_list; d; d = d->next) {
	if (d->connected != CON_PLYNG) // Not playing
	    continue;
        psi = d->character;
        if (!psi)
            continue;
        can_hear = affected_by_spell(psi, PSI_CLAIRAUDIENCE) ? TRUE : FALSE;
        can_see = affected_by_spell(psi, PSI_CLAIRVOYANCE) ? TRUE : FALSE;
        if (!can_hear && !can_see)
            continue;
        if (lis != psi->specials.contact)
            continue;

        /* Modify the action-verb in case the speech cannot be heard
         * so that clairvoyant psionicists will properly see the actio
         */
        if (output[strlen(output) - 1] == '!')
           // arg = one_argument(exclaimVerb, action, sizeof(action));
			one_argument(exclaimVerb, action, sizeof(action));
		else if (output[strlen(output) - 1] == '?')
            one_argument(askVerb, action, sizeof(action));
		// arg = one_argument(askVerb, action, sizeof(action));
        else
            one_argument(verb, action, sizeof(action));
		// arg = one_argument(verb, action, sizeof(action));
        if (!can_hear || IS_AFFECTED(tch, CHAR_AFF_DEAFNESS))
            strcat(action, " something");

        if (can_see) {          // If they are using clairvoyance, show sdescs
            /* prepare who is doing what */
            sprintf(buf, "%s%s%s %s %s", how[0] != '\0' ? how : "", how[0] != '\0' ? ", " : "",
                    PERS(lis, ch),
                    output[strlen(output) - 1] == '!' ? exclaimVerb : output[strlen(output) - 1] ==
                    '?' ? askVerb : verb, PERS(lis, tch));
        } else {                // Use voices
            if (lis == ch)
                strcpy(ch_voice, VOICE(lis, ch));
            else
                sprintf(ch_voice, "%s%s voice", indefinite_article(GENDER(ch)), GENDER(ch));

            sprintf(buf, "%s %s", ch_voice, action);
        }

        /* pass the arguments off to be parsed for us */
        if (!parse_emote_message(ch, buf, TRUE, msg, sizeof(msg), &char_list))
            /* parse fails, we fail */
            return;

        if (can_hear && !IS_AFFECTED(tch, CHAR_AFF_DEAFNESS)) {
            k = apply_accent(tmpbuf, ch, psi);
            sprintf(out, "Over the Way: %s, in %s:", msg, tmpbuf);
            gvprintf(ch, psi, NULL, out, char_list, 0, 0);
            send_translated(ch, psi, output);

            /* give 'em a chance to learn it */
            learn_language(ch, psi, k);
        } else if (can_see) {   // Just show them that something was said.
            sprintf(out, "Over the Way: %s.", msg);
            gvprintf(NULL, psi, NULL, out, char_list, 0, 0);
        }

        /* free the char_list data */
        while (((p) = (char_list))) {
            char_list = char_list->next;
            free(p);
        }
    }
}

/* This function returns TRUE if the command specified is not allowed as a
 * command/action because they are OOC commands.
 */
bool
is_restricted_cmd(int sk)
{
    switch (sk) {
        // No twinking
    case CMD_NOSAVE:
    case CMD_QUIT:
    case CMD_GONE:
    case CMD_OOC:
    case CMD_JUNK:
    case CMD_WISH:
    case CMD_IDEA:
    case CMD_TYPO:
    case CMD_BUG:
    case CMD_WRITE:
    case CMD_REROLL:
    case CMD_BEEP:
    case CMD_REBEL:
    case CMD_RECRUIT:
    case CMD_DUMP:
    case CMD_MERCY:
    case CMD_BIOGRAPHY:
    case CMD_ADDKEYWORD:
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

/* This function returns TRUE if the command specified is not allowed as a
 * command/action to compel someone else.
 */
bool
is_offensive_cmd(int sk)
{
    switch (sk) {
        // No combat
    case CMD_KILL:
    case CMD_HIT:
    case CMD_BACKSTAB:
    case CMD_SAP:
    case CMD_BASH:
    case CMD_KICK:
    case CMD_DISARM:
    case CMD_THROW:
    case CMD_SUBDUE:
    case CMD_SHOOT:
    case CMD_CHARGE:
    case CMD_RESCUE:
    case CMD_ASSIST:
        // No magick
    case CMD_CAST:
    case CMD_GATHER:
        // No stealing or object loss
    case CMD_STEAL:
    case CMD_GIVE:
    case CMD_DROP:
    case CMD_PUT:
    case CMD_PLANT:
    case CMD_SLIP:
        // No psionics
    case CMD_CEASE:
    case CMD_CONTACT:
    case CMD_ALLSPEAK:
    case CMD_CATHEXIS:
    case CMD_COMMUNE:
    case CMD_BABBLE:
    case CMD_BARRIER:
    case CMD_EXPEL:
    case CMD_DOME:
    case CMD_LOCATE:
    case CMD_TRACE:
    case CMD_HEAR:
    case CMD_CLAIRAUDIENCE:
    case CMD_CLAIRVOYANCE:
    case CMD_SENSE_PRESENCE:
    case CMD_EMPATHY:
    case CMD_PROBE:
    case CMD_COMPEL:
    case CMD_CONTROL:
    case CMD_COERCE:
    case CMD_MESMERIZE:
    case CMD_DISORIENT:
    case CMD_MINDWIPE:
    case CMD_MINDBLAST:
    case CMD_PROJECT:
    case CMD_CONCEAL:
    case CMD_MASQUERADE:
    case CMD_ILLUSION:
    case CMD_SUGGEST:
    case CMD_VANISH:
    case CMD_SHADOWWALK:
    case CMD_IMITATE:
    case CMD_MAGICKSENSE:
        return TRUE;
    default:
        break;
    }

    return is_restricted_cmd(sk);
}

int
psi_drain(CHAR_DATA * ch, int sk)
{
    int chance, skill_level, max;
    int stun_loss = 0;

    skill_level = (has_skill(ch, sk) ? ch->skills[sk]->learned : 1);
    chance = (GET_WIS(ch) + skill_level) / 3;

    if (GET_GUILD(ch) == GUILD_PSIONICIST) {
        max = get_max_skill(ch, sk);

        if ((max > 0) && (skill_level >= max - 10))
            return 0;
    } else if (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR && is_templar_in_city(ch))
        return 0;

    if (!number(0, chance))
        stun_loss = number(1, (101 - skill_level) / 10);

    return stun_loss;
}

/*
 * This function will randomly cause the psionicist to drain stun points
 * from any active psionic abilities.
 */
int
update_psionics(CHAR_DATA * ch)
{
    int i, stun_drain = 0;

    for (i = 0; psionic_affects[i] > 0; i++) {
        if (affected_by_spell(ch, psionic_affects[i]))
            stun_drain += psi_drain(ch, psionic_affects[i]);
    }

    if (stun_drain) {
        cprintf(ch, "You suffer from use of the Way.\n\r");
        if (!generic_damage(ch, 0, 0, 0, stun_drain)) {
            die(ch);
            return FALSE;
        }
    }

    if (knocked_out(ch) || !AWAKE(ch)) {        // Knocked 'em out?
        return remove_psionics(ch);
    }

    /* watch who they're in contact with, block appropriately */
    if (ch->specials.contact) {
        CHAR_DATA *temp = ch->specials.contact;

        if (affected_by_spell(temp, SPELL_PSI_SUPPRESSION)
            || affected_by_spell(temp, SPELL_SHIELD_OF_NILAZ)
            || !is_on_same_plane(temp->in_room, ch->in_room)
            || (temp->in_room && IS_SET(temp->in_room->room_flags, RFL_NO_PSIONICS))) {
            if (!affected_by_spell(ch, PSI_CONCEAL))
                send_to_char("You sense a foreign presence withdraw from your mind.\n\r", temp);
            act("You feel your mental contact withdrawing from the mind of your target.", FALSE,
                ch, 0, 0, TO_CHAR);
            remove_contact(ch);
        }

    }
    // Are they somehow affected by psionic-suppressing magicks?
    if (affected_by_spell(ch, SPELL_PSI_SUPPRESSION)
        || affected_by_spell(ch, SPELL_SHIELD_OF_NILAZ)) {
        return remove_psionics(ch);
    }
    // Is the room set for NO_PSIONICS?
    if (ch->in_room && IS_SET(ch->in_room->room_flags, RFL_NO_PSIONICS)) {
        return remove_psionics(ch);
    }

    /* Special case for shadowwalking since we have to update the original
     * character, not the shadow.
     */
    if (affected_by_spell(ch, PSI_SHADOW_WALK)) {
        if (ch->desc && ch->desc->original && has_skill(ch->desc->original, PSI_SHADOW_WALK)) {
            generic_damage(ch->desc->original, 0, 0, 0, psi_drain(ch->desc->original, PSI_SHADOW_WALK));
            if (knocked_out(ch->desc->original)) {
                send_to_char("Your physical body's mental strength wears out.\n\r", ch);
                parse_psionic_command(ch, "return");
                // NPC will be gone, so tell affect_update to stop processing
                return FALSE;
            }
        }
    }
    return TRUE;
}

/*
 * Halt any active psionic abilities.
 */
int
remove_psionics(CHAR_DATA * ch)
{
    if (affected_by_spell(ch, PSI_HEAR)) {
        affect_from_char(ch, PSI_HEAR);
        send_to_char("You cannot maintain your psionic eavesdropping.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_CATHEXIS)) {
        affect_from_char(ch, PSI_CATHEXIS);
        send_to_char("You cannot maintain your state of cathexis.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_DOME)) {
        affect_from_char(ch, PSI_DOME);
        send_to_char("You cannot maintain your psionic dome.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_MAGICKSENSE)) {
        affect_from_char(ch, PSI_MAGICKSENSE);
        send_to_char("You cannot maintain your magickal awareness.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_SENSE_PRESENCE)) {
        affect_from_char(ch, PSI_SENSE_PRESENCE);
        send_to_char("You cannot maintain your psychic awareness.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_CLAIRAUDIENCE)) {
        affect_from_char(ch, PSI_CLAIRAUDIENCE);
        send_to_char("You cannot maintain your auditory interception.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_CLAIRVOYANCE)) {
        affect_from_char(ch, PSI_CLAIRVOYANCE);
        send_to_char("You cannot maintain your visual interception.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_CONCEAL)) {
        affect_from_char(ch, PSI_CONCEAL);
        send_to_char("You cannot maintain your psionic concealment.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_VANISH)) {
        affect_from_char(ch, PSI_VANISH);
        send_to_char("You cannot maintain your psionic cloak.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_COMPREHEND_LANGS)) {
        affect_from_char(ch, PSI_COMPREHEND_LANGS);
        send_to_char("You cannot maintain your augmented psychic awareness.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_BARRIER)) {
        affect_from_char(ch, PSI_BARRIER);
        send_to_char("You cannot maintain your psychic barrier.\n\r", ch);
    }

    if (affected_by_spell(ch, PSI_CONTACT)) {
        remove_contact(ch);
        send_to_char("You cannot maintain your psionic contact.\n\r", ch);
    }
    // if shadow walking and run out of stun, need to return false to 
    // let our calling function know that this character is done
    if (affected_by_spell(ch, PSI_SHADOW_WALK)) {
        send_to_char("You cannot maintain your shadow walking.\n\r", ch);
        parse_psionic_command(ch, "return");
        return FALSE;
    }

    return TRUE;
}


/* Drain stun points from the character and echo.  Returns TRUE if ch
 * doesn't have enough moves to use the Way. */
bool
drain_char(CHAR_DATA * ch, int sk)
{
    send_to_char("You suffer from use of the Way.\n\r", ch);
    return standard_drain(ch, sk);
}


// Calculate how much stun points using the skill would cost
int
calc_standard_drain(CHAR_DATA * ch, int sk)
{
    int stun_loss, avg;

    // Figure out the base price based on skill
    switch (sk) {
        // Hard tier [9 - 27], avg 18
    case PSI_COERCION:
    case PSI_CONTROL:
    case PSI_DOME:
    case PSI_EXPEL:
    case PSI_ILLUSION:
    case PSI_MESMERIZE:
    case PSI_MINDBLAST:
    case PSI_MINDWIPE:
    case PSI_PROBE:
        stun_loss = 18;
        break;

        // Medium tier [7 - 21], avg 14
    case PSI_BABBLE:
    case PSI_BARRIER:
    case PSI_CATHEXIS:
    case PSI_CLAIRAUDIENCE:
    case PSI_CLAIRVOYANCE:
    case PSI_COMPEL:
    case PSI_CONCEAL:
    case PSI_DISORIENT:
    case PSI_HEAR:
    case PSI_IMITATE:
    case PSI_MASQUERADE:
    case PSI_TRACE:
    case PSI_VANISH:
        stun_loss = 14;
        break;

        // Easiest tier [4 - 12], avg 8
    case PSI_CONTACT:
        stun_loss = 8;
        break;

        // Easy tier [5 - 15], avg 10
    case PSI_COMPREHEND_LANGS:
    case PSI_EMPATHY:
    case PSI_LOCATE:
    case PSI_MAGICKSENSE:
    case PSI_PROJECT_EMOTION:
    case PSI_REJUVENATE:
    case PSI_SENSE_PRESENCE:
    case PSI_SUGGESTION:
    case PSI_THOUGHTSENSE:
    default:
        stun_loss = 10;
        break;
    }

    // skill & willpower modifier
    avg =
        ((ch->skills[SKILL_WILLPOWER] ? ch->skills[SKILL_WILLPOWER]->learned : 1) +
         (ch->skills[sk] ? ch->skills[sk]->learned : 1)) / 2;
    stun_loss += (stun_loss * (50 - avg)) / 100;
    stun_loss += number(0, 3);  // Add a random element

    return stun_loss;
}

/* Drain stun points from the character w/o echo.  Returns TRUE if ch
 * doesn't have enough moves to use the Way. */
bool
standard_drain(CHAR_DATA * ch, int sk)
{
    generic_damage(ch, 0, 0, 0, calc_standard_drain(ch, sk));

    return knocked_out(ch);
}

void
aggressive_psionics(CHAR_DATA * pCh)
{
    if (affected_by_spell(pCh, PSI_CONCEAL)) {
        send_to_char("You are unable to maintain your psionic concealment!\n\r", pCh);
        affect_from_char(pCh, PSI_CONCEAL);
    }
    if (affected_by_spell(pCh, PSI_VANISH)) {
        send_to_char("You are unable to maintain your psionic cloak!\n\r", pCh);
        affect_from_char(pCh, PSI_VANISH);
    }

}

/* Concealment check.  If the psionic person is concealed, look at
 * the difference between their concealment ability and the target's
 * concealment ability.  This will allow for comparably skilled
 * psionicists to detect each other, despite being concealed.
 * -Tiernan 8/30/05 */
bool
is_detected_psi(CHAR_DATA * pCh, CHAR_DATA * tCh)
{
    if (!pCh || !tCh)
        return FALSE;

    // They're not even trying to hide their identity
    if (!affected_by_spell(pCh, PSI_CONCEAL))
        return TRUE;

    // Does the target know about psionic concealment?
    if (pCh->skills[PSI_CONCEAL] && tCh->skills[PSI_CONCEAL]) {
        // Enough to notice who's in their mind?
        if (number(1, 100) <=
            (tCh->skills[PSI_CONCEAL]->learned * (100 - pCh->skills[PSI_CONCEAL]->learned)) / 100)
            return TRUE;
    }
    // 1% default chance of being noticed
    if (number(0, 100))
        return FALSE;

    // They fell into the unlucky 1% that get caught
    return TRUE;
}


bool
blocked_psionics(CHAR_DATA * pCh, CHAR_DATA * tCh)
{
    if (affected_by_spell(pCh, PSI_BARRIER)) {
        send_to_char("A barrier around your mind prevents you from using your psionic powers.\n\r",
                     pCh);
        return TRUE;
    }

    if (affected_by_spell(pCh, SPELL_PSI_SUPPRESSION)
        || affected_by_spell(pCh, SPELL_SHIELD_OF_NILAZ)) {
        send_to_char("You cannot seem to get in tune with your psionic abilities.\n\r", pCh);
        return TRUE;
    }

    if (pCh->in_room && IS_SET(pCh->in_room->room_flags, RFL_NO_PSIONICS)) {
        send_to_char("Something prevents you from using your psionic abilities.\n\r", pCh);
        return TRUE;
    }

    if (tCh && tCh->in_room && IS_SET(tCh->in_room->room_flags, RFL_NO_PSIONICS)) {
        send_to_char("Something is blocking your telepathy.\n\r", pCh);
        return TRUE;
    }

    return FALSE;
}

// Helper function
void
apply_psi_to_char(CHAR_DATA * ch, int sk)
{
    apply_psi_to_char_modified(ch, sk, 0);
}

/* standard psi-affect applier */
void
apply_psi_to_char_modified(CHAR_DATA * ch, int sk, int mod)
{
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    af.type = sk;

    /* Added for new power and magick types -Morg */
    af.power = 0;
    af.magick_type = MAGICK_PSIONIC;

    af.location = 0;            /* CHAR_APPLY_END; */
    af.modifier = mod;            /* -2; */
    af.bitvector = CHAR_AFF_PSI;

    /* Base minimum duration is 2 game hours, although we should never
     * use just the base because this function should only be called when a
     * person is using a psionic skill in their tree.  When that happens, the
     * duration is based off their proficiency in the skill and their wisdom.
     * The base duration is here as a catch-all to ensure that any affect
     * which is applied has a duration >0 at the minimum.
     */
    af.duration = 2;
    if (has_skill(ch, sk))
        af.duration = MAX(af.duration, ch->skills[sk]->learned * wis_app[GET_WIS(ch)].learn / 100);

    // Apply any unique adjustments as a result of what the skill is
    switch (sk) {
    case PSI_SHADOW_WALK:
        af.duration /= 2;
        break;
    default:
        break;
    }

    affect_to_char(ch, &af);
    update_pos(ch);
}

void
send_empathic_msg(CHAR_DATA * ch, const char *msg)
{
    char buf[MAX_STRING_LENGTH];

    // make a copy of the message so we can modify it
    char *tmpmsg = strdup(msg);

    // remove any newlines
    strip(tmpmsg);

    // capitalize
    tmpmsg[0] = UPPER(tmpmsg[0]);

    // send the message
    snprintf(buf, sizeof(buf), "<< %s >>\n\r", tmpmsg);
    send_to_char(buf, ch);

    // cleanup dup'd memory
    free(tmpmsg);
}

void
send_emotion_to_immersed(CHAR_DATA * ch, const char *msg)
{
    CHAR_DATA *psi = NULL;
    DESCRIPTOR_DATA *d;

    // Nobody can sense immortal feelings
    if (IS_IMMORTAL(ch))
        return;

    if (!can_use_psionics(ch))
        return;

    for (d = descriptor_list; d; d = d->next) {
	if (d->connected != CON_PLYNG) // Not playing
	    continue;
        psi = d->character;
        if (!psi)
            continue;
        // If contact with the actor, ignore
        if (ch == psi->specials.contact)
            continue;
        // If in the same room, ignore
        if (ch->in_room == psi->in_room)
            continue;
        // Immersion does not require contact
        if (!has_skill(psi, PSI_IMMERSION))
            continue;
        // Very limited range
        if (!is_in_psionic_range(psi, ch, MAX(0, psi->skills[PSI_IMMERSION]->learned / 30)))
            continue;
        // Pass the check?
        if (!skill_success(psi, NULL, psi->skills[PSI_IMMERSION]->learned)) {
            gain_skill(psi, PSI_IMMERSION, 1);
            continue;
        }
        send_empathic_msg(psi, msg);
    }
}

void
send_thought_to_immersed(CHAR_DATA * ch, const char *msg)
{
    CHAR_DATA *psi = NULL;
    DESCRIPTOR_DATA *d;

    // Nobody can hear immortal thoughts
    if (IS_IMMORTAL(ch))
        return;

    if (!can_use_psionics(ch))
        return;

    for (d = descriptor_list; d; d = d->next) {
	if (d->connected != CON_PLYNG) // Not playing
	    continue;
        psi = d->character;
        if (!psi)
            continue;
        // If contact with the actor, ignore
        if (ch == psi->specials.contact)
            continue;
        // If in the same room, ignore
        if (ch->in_room == psi->in_room)
            continue;
        // Immersion does not require contact
        if (!has_skill(psi, PSI_IMMERSION))
            continue;
        // Very limited range
        if (!is_in_psionic_range(psi, ch, MAX(0, psi->skills[PSI_IMMERSION]->learned / 30)))
            continue;
        // Pass the check?
        if (!skill_success(psi, NULL, psi->skills[PSI_IMMERSION]->learned)) {
            gain_skill(psi, PSI_IMMERSION, 1);
            continue;
        }
        cprintf(psi, "Over the Way: Someone %s:\n\r     \"%s\"\n\r",
                GET_POS(ch) == POSITION_SLEEPING ? "dreams" : "thinks", msg);
    }
}

void
send_to_empaths(CHAR_DATA * ch, const char *msg)
{
    CHAR_DATA *psi = NULL;
    DESCRIPTOR_DATA *d;

    if (!can_use_psionics(ch))
        return;

    for (d = descriptor_list; d; d = d->next) {
	if (d->connected != CON_PLYNG) // Not playing
	    continue;
        psi = d->character;
        if (!psi)
            continue;
        if (ch != psi->specials.contact)
            continue;
        if (!has_skill(psi, PSI_EMPATHY))
            continue;
        if (!skill_success(psi, NULL, psi->skills[PSI_EMPATHY]->learned)) {
            gain_skill(psi, PSI_EMPATHY, 1);
            continue;
        }
        send_empathic_msg(psi, msg);
    }
}

void
send_thought_to_listeners(CHAR_DATA * ch, const char *msg)
{
    CHAR_DATA *psi = NULL;
    DESCRIPTOR_DATA *d;
    bool amplified = FALSE;

    if (!can_use_psionics(ch))
        return;

    for (d = descriptor_list; d; d = d->next) {
	if (d->connected != CON_PLYNG) // Not playing
	    continue;
        psi = d->character;
        if (!psi)
            continue;
        if (ch != psi->specials.contact)
            continue;
        if (!affected_by_spell(psi, PSI_CLAIRAUDIENCE))
            continue;
        if (!has_skill(psi, PSI_CLAIRAUDIENCE))
            continue;
        if (!skill_success(psi, NULL, psi->skills[PSI_CLAIRAUDIENCE]->learned)) {
            gain_skill(psi, PSI_CLAIRAUDIENCE, 1);
            continue;
        }
        // Amplify the thought so that it is like a psi message
        if (!amplified) {
            send_to_listeners(msg, ch, psi);
            amplified = TRUE;
	}

	// Echo it to the psi
        cprintf(psi, "Over the Way: %s %s:\n\r     \"%s\"\n\r", capitalize(MSTR(ch, short_descr)),
                GET_POS(ch) == POSITION_SLEEPING ? "dreams" : "thinks", msg);
    }
}

/* Used with PSI_DOME, this function sends a psionicist who is using a
 * dome a message about activity against it.  If the psionicist has the
 * PSI_TRACE ability, it passively tries to figure out who is using
 * psionics against the dome.
 */
void
trace_dome(CHAR_DATA * pCh, CHAR_DATA * psi)
{
    // Can the psi tell who it is?
    if (has_skill(psi, PSI_TRACE)) {
        // Is the psionic-user concealed?
        if (affected_by_spell(pCh, PSI_CONCEAL)) {
            if (number(1, 100) <=
                (psi->skills[PSI_TRACE]->learned * (100 - pCh->skills[PSI_CONCEAL]->learned)) /
                100) {
                cprintf(psi, "You sense psionic activity against your dome by %s.\n\r",
                        MSTR(pCh, short_descr));
                send_to_char("You sense your identity has been observed.\n\r", pCh);
            } else {
                gain_skill(psi, PSI_TRACE, 1);
                send_to_char("You sense psionic activity against your dome.\n\r", psi);
            }
        } else {
            // Not concealed, but we still must do a skillcheck on trace
            if (skill_success(psi, NULL, psi->skills[PSI_TRACE]->learned)) {
                cprintf(psi, "You sense psionic activity against your dome by %s.\n\r",
                        MSTR(pCh, short_descr));
            } else {
                gain_skill(psi, PSI_TRACE, 1);
                send_to_char("You sense psionic activity against your dome.\n\r", psi);
            }
        }
    } else {
        send_to_char("You sense psionic activity against your dome.\n\r", psi);
    }
}

bool
psi_skill_success(CHAR_DATA * pCh, CHAR_DATA * tCh, int sk, int bonus) {
    int skill_num, att = 0, def = 0, barrier = 0, mods = 0;
    CHAR_DATA *psi = NULL;
    char buf[MAX_STRING_LENGTH];
    char real_name_buf[MAX_STRING_LENGTH];
    bool attack_passed = FALSE;

    if (!can_use_psionics(pCh))
        return FALSE;

    skill_num = pCh->skills[sk] ? pCh->skills[sk]->learned : 1;
    mods = bonus;

    if (affected_by_spell(pCh, PSI_MINDWIPE))
        mods -= 25;

    if (!pCh->in_room)          // This would be bad
        return FALSE;

    /* bonus to contact if your target is already in contact with you */
    if (sk == PSI_CONTACT && tCh && tCh->specials.contact == pCh
        && !affected_by_spell(tCh, PSI_CONCEAL)) {
        mods += 20;
    }

    // Dome check for psi initiator (pCh)
    for (psi = pCh->in_room->people; psi; psi = psi->next_in_room) {
        if (psi != pCh && affected_by_spell(psi, PSI_DOME)) {
            if (has_skill(psi, PSI_DOME))
                mods -= psi->skills[PSI_DOME]->learned / 2;
            if (sk != PSI_HEAR && // We don't want to spam eavesdropping
                sk != PSI_VANISH)     // Vanish is opposite from the rest
                trace_dome(pCh, psi);
        }
    }
    // Dome check for psi target (tCh)
    if (tCh && pCh->in_room != tCh->in_room) {  // Don't dome 'em twice
        for (psi = tCh->in_room->people; psi; psi = psi->next_in_room) {
            if (psi != pCh && affected_by_spell(psi, PSI_DOME)) {
                if (has_skill(psi, PSI_DOME))
                    mods -= psi->skills[PSI_DOME]->learned / 2;
                if (sk != PSI_HEAR &&     // We don't want to spam eavesdropping
                    sk != PSI_VANISH)     // Vanish is opposite from the rest
                    trace_dome(pCh, psi);
            }
        }
    }

    // We have a target, do more checking
    if (tCh) {
        if (IS_IMMORTAL(tCh) && !IS_IMMORTAL(pCh))
            return FALSE;
        if (affected_by_spell(tCh, SPELL_SHIELD_OF_NILAZ)
            || affected_by_spell(tCh, SPELL_PSI_SUPPRESSION))
            return FALSE;

        // Start calculating chances
	// Already adjusted for bonuses, domes.
        att = pCh->skills[sk] ? pCh->skills[sk]->learned : 1;
	att += mods;

        // Target has a barrier, we have some extra work to do
        if (affected_by_spell(tCh, PSI_BARRIER)) {
            barrier = 1;
            def += tCh->skills[PSI_BARRIER] ? tCh->skills[PSI_BARRIER]->learned : 1;
            def += tCh->skills[PSI_WILD_BARRIER] ? tCh->skills[PSI_WILD_BARRIER]->learned : 1;
            /* Recalculate the att bonus if this is a PSI_CONTACT action to
             * use the contacting person's barrier skill instead of contact
             * skill when determining if the barrier will get broken.
             * -Tiernan */
            if (sk == PSI_CONTACT) {
                // Barrier skill%
                att += pCh->skills[PSI_BARRIER] ? pCh->skills[PSI_BARRIER]->learned : 1;
                // Add wild contact%
                if (has_skill(pCh, PSI_WILD_CONTACT))
                    att += pCh->skills[PSI_WILD_CONTACT]->learned;
            }
        }
        // Mindwipe affects clarity, already factored in for pCh earlier
        if (affected_by_spell(tCh, PSI_MINDWIPE))
            def -= 25;

        /* Willpower bonus only applies for targets who have a barrier or 
         * for other psionic abilities besides "contact" because these
         * psionic actions being taken against them would be considered
         * "hostile" and they would want to prevent them from happening.
         */
        if (sk != PSI_CONTACT || affected_by_spell(tCh, PSI_BARRIER)) {
            att += pCh->skills[SKILL_WILLPOWER] ? pCh->skills[SKILL_WILLPOWER]->learned : 1;
            def += tCh->skills[SKILL_WILLPOWER] ? tCh->skills[SKILL_WILLPOWER]->learned : 1;
	    // Other psionic ability needs to be factored in here too
            def += tCh->skills[sk] ? tCh->skills[sk]->learned : 1;

        }

        /* harder to do things to fighting and sleeping people */
        if ((GET_POS(tCh) == POSITION_FIGHTING) || (GET_POS(tCh) == POSITION_SLEEPING))
            def += 25;

        if (!tCh->in_room)      // This would be bad
            return FALSE;

        att += number(-10, 10);
        def += number(-10, 10);

	// First, in a PvP contest, did they win?
	if (att > def)
            attack_passed = TRUE;

	/* Second, does the victim not know the skill being used on them and
	 * are not resisting with a barrier?  If so, a raw skillcheck works
	 * instead, more PvE in nature to allow skillgains.
	 */
	if ((!has_skill(tCh, sk) || sk == PSI_CONTACT) && !affected_by_spell(tCh, PSI_BARRIER))
            attack_passed = skill_success(pCh, NULL, skill_num + mods);

	// Last, is the victim even trying to resist?
	if (IS_SET(tCh->specials.nosave, NOSAVE_PSIONICS))
            attack_passed = TRUE;

        if (attack_passed) {
            if (barrier) {
                switch (sk) {
                case PSI_VANISH:
                    /* Vanish works opposite of most other psionic skills
                     * in that this skillcheck is performed when someone else
                     * is performing an action (e.g. "look") instead of the
                     * psionicist performing an action.  If the looker has a
                     * barrier up and fails their skillcheck, we don't want
                     * the barrier to suddenly break and give away the
                     * vanished psi.  We just want the looker not to see the
                     * psionicist.
                     */
                    break;
                default:
                    // For everything else, crush that barrier!
                    gain_skill(tCh, PSI_BARRIER, 1);
                    gain_skill(tCh, SKILL_WILLPOWER, 1);
                    if (affected_by_spell(pCh, PSI_CONCEAL)) {
                        cprintf(tCh, "%s\n\r", spell_wear_off_msg[PSI_BARRIER]);
                    } else
                        send_to_char("Your psychic barrier is crushed!\n\r", tCh);
                    affect_from_char(tCh, PSI_BARRIER);
                    snprintf(buf, sizeof(buf), "%s fails to maintain %s psionic barrier.",
                             MSTR(tCh, short_descr), HSHR(tCh));
                    // Tell all empaths already in contact about the barrier
                    send_to_empaths(tCh, buf);
                    // Now tell the contacting person if they're an empath
                    if (sk == PSI_CONTACT && has_skill(pCh, PSI_EMPATHY))
                        send_empathic_msg(pCh, buf);
                    // The price of breaking the barrier
                    standard_drain(pCh, sk);
                    break;
                }
            }
            // log psi skills done on npcs
            if (sk != PSI_CONTACT && sk != PSI_VANISH 
             && tCh && !IS_NPC(pCh) && IS_NPC(tCh)) {
               qgamelogf(QUIET_PSI, "%s (%s) succeeds at %s%s on NPC #%d (%s)",
                 GET_NAME(pCh), pCh->account, 
                (!affected_by_spell(tCh, PSI_CONCEAL) ? "" : "a concealed "),
                skill_name[sk], npc_index[tCh->nr].vnum,
                REAL_NAME(tCh));
            }

            return TRUE;
        } else {
            if (barrier) {      // Failed to break the target's barrier
                gain_skill(pCh, PSI_BARRIER, 1);
                gain_skill(pCh, SKILL_WILLPOWER, 1);
            }

            // log psi skills done on npcs
            if (sk != PSI_CONTACT && tCh && !IS_NPC(pCh) && IS_NPC(tCh)) {
               qgamelogf(QUIET_PSI, "%s (%s) failed %s%s on NPC #%d (%s)",
                 GET_NAME(pCh), pCh->account, 
                (!affected_by_spell(tCh, PSI_CONCEAL) ? "" : "concealed "),
                skill_name[sk], npc_index[tCh->nr].vnum,
                REAL_NAME(tCh));
            }

            return FALSE;
        }
    }

    /* See if the skill even _works_ with a % roll */
    return skill_success(pCh, NULL, skill_num + mods);
}


/* PSI COMMANDS */
void
cmd_commune(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    int i;
    char buf[MAX_STRING_LENGTH];
    char cname[MAX_STRING_LENGTH];
    char argument[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool silent = FALSE;
    struct affected_type *aff_node = NULL;
    int clan = 0;

    if (GET_RACE(ch) == RACE_MANTIS)
        return mantis_commune(ch, arg, cmd, count);

    if (!has_skill(ch, PSI_CATHEXIS)) {
        send_to_char("What?\n\r", ch);
        return;
    }

    for (i = 0; *(arg + i) == ' '; i++);
    strncpy(argument, arg + i, sizeof(argument));

    if (!(*argument)) {
        send_to_char("What message do you want to send?\n\r", ch);
        return;
    }

    aff_node = affected_by_spell(ch, PSI_CATHEXIS);
    if (!aff_node) {
        send_to_char("You are not in a cathexic state.\n\r", ch);
        return;
    }

    if (drain_char(ch, PSI_CATHEXIS))
        return;

    if (blocked_psionics(ch, NULL))
        return;

    clan = aff_node->modifier;
    if (clan > 0 && clan < MAX_CLAN)
        strcpy(cname, clan_table[clan].name);
    else
        sprintf(cname, "%d", clan);

    /* Rare case where the cathexis was started and they "rebelled" or were
     * otherwise removed from the clan.
     */
    if (!IS_TRIBE(ch, clan))
        return;

    silent = affected_by_spell(ch, PSI_CONCEAL) ? TRUE : FALSE;

    snprintf(buf, sizeof(buf), "%s %ssends across the Way to '%s': %s\n\r", MSTR(ch, name),
             silent ? "silently " : "", cname, argument);
    send_to_monitor(ch, buf, MONITOR_PSIONICS);

    snprintf(buf, sizeof(buf), "You %ssend a telepathic message to '%s':\n\r     \"%s\"\n\r",
                 silent ? "silently " : "", cname, argument);
    send_to_char(buf, ch);

    // Send it out
    for (d = descriptor_list; d; d = d->next) {
        if (!d->connected && d->character && GET_RACE(d->character) != RACE_MANTIS
            && d->character != ch
            && !IS_IMMORTAL(d->character)
            && IS_TRIBE(d->character, clan)
            && !d->str && can_use_psionics(d->character)
            && is_on_same_plane(d->character->in_room, ch->in_room)
            && AWAKE(d->character)
            && !IS_CREATING(d->character)
            && !IS_SET(d->character->specials.quiet_level, QUIET_CONT_MOB)) {
            if (silent) {
                snprintf(buf, sizeof(buf),
                         "Someone sends you a telepathic message:\n\r    \"%s\"\n\r", argument);
            } else {
                snprintf(buf, sizeof(buf), "%s sends you a telepathic message:\n\r    \"%s\"\n\r",
                         capitalize(MSTR(ch, short_descr)), argument);
            }
            send_to_char(buf, d->character);
            clairaudient_listener(ch, d->character, FALSE, argument);
        }
    }
}

void
mantis_commune(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    int i;
    char buf[MAX_STRING_LENGTH], real_name_buf[512];
    char argument[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;


    if (GET_RACE(ch) != RACE_MANTIS) {
        send_to_char("What?\n\r", ch);
        return;
    }

    for (i = 0; *(arg + i) == ' '; i++);
    strncpy(argument, arg + i, sizeof(argument));


    if (!(*argument)) {
        send_to_char("What message do you want to send?\n\r", ch);
        return;
    }

    snprintf(buf, sizeof(buf), "<%s>: %s\n\r", REAL_NAME(ch), argument);
    send_to_monitor(ch, buf, MONITOR_PSIONICS);

    for (d = descriptor_list; d; d = d->next) {
        if (!d->connected && d->character && GET_RACE(d->character) == RACE_MANTIS
            && d->character != ch
            && IS_IN_SAME_TRIBE(d->character, ch)
            && !d->str && can_use_psionics(d->character)
            && AWAKE(d->character)
            && !IS_CREATING(d->character)
            && !IS_SET(d->character->specials.quiet_level, QUIET_CONT_MOB))
            send_to_char(buf, d->character);
    }
}

void
cmd_psi(CHAR_DATA * pCh, const char *arg, Command cmd, int count)
{
    int i, chance;
    char buf[MAX_STRING_LENGTH], real_name_buf[512];
    char argument[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    const char *temp_arg;
    bool silent = FALSE;
    CHAR_DATA *tCh, *target;
    DESCRIPTOR_DATA *d;

    if (!(tCh = pCh->specials.contact)) {
        send_to_char("You aren't in contact with anyone.\n\r", pCh);
        return;
    }

    for (i = 0; *(arg + i) == ' '; i++);
    strncpy(argument, arg + i, sizeof(argument));

    if (!(*argument)) {
        send_to_char("What message do you want to send?\n\r", pCh);
        return;
    }

    silent = affected_by_spell(pCh, PSI_CONCEAL) ? TRUE : FALSE;

    temp_arg = argument;
    temp_arg = one_argument(temp_arg, arg1, sizeof(arg1));

    if (STRINGS_SAME(arg1, "-s")
        && ((GET_GUILD(pCh) == GUILD_PSIONICIST || GET_GUILD(pCh) == GUILD_LIRATHU_TEMPLAR))) {
        send_to_char("Silent psi is deprecated, please see 'psi conceal'.\n\r", pCh);
        return;
    }

    if (AWAKE(tCh) && can_use_psionics(tCh) && !IS_CREATING(tCh) && can_use_psionics(pCh)) {
        if (drain_char(pCh, PSI_CONTACT))
            return;
        update_pos(pCh);

        snprintf(buf, sizeof(buf), "%s %ssends to %s: %s\n\r", MSTR(pCh, name),
                 silent ? "silently " : "", MSTR(tCh, name), argument);
        send_to_monitors(pCh, tCh, buf, MONITOR_COMMS);
        snprintf(buf, sizeof(buf), "You %ssend a telepathic message to %s:\n\r     \"%s\"\n\r",
                 silent ? "silently " : "", MSTR(tCh, short_descr), argument);
        send_to_char(buf, pCh);
        clairaudient_listener(pCh, tCh, TRUE, argument);
        /* Put a check on intox level of the sender, and reciever.  If either
         * one is too badly blitzed, intercept the message. */
        chance = 0;
        switch (is_char_drunk(pCh)) {
        case 0:
            chance = 0;
            break;
        case DRUNK_LIGHT:
            chance = 5;
            break;
        case DRUNK_MEDIUM:
            chance = 25;
            break;
        case DRUNK_HEAVY:
            chance = 45;
            break;
        case DRUNK_SMASHED:
            chance = 65;
            break;
        case DRUNK_DEAD:
            chance = 85;
            break;
        }

        switch (is_char_drunk(tCh)) {
        case 0:
            break;
        case DRUNK_LIGHT:
            chance += 5;
            break;
        case DRUNK_MEDIUM:
            chance += 25;
            break;
        case DRUNK_HEAVY:
            chance += 45;
            break;
        case DRUNK_SMASHED:
            chance += 65;
            break;
        case DRUNK_DEAD:
            chance += 85;
            break;
        }

        // Feeblemind jumbles up the message
        if (affected_by_spell(pCh, SPELL_FEEBLEMIND))
            chance += 100;

        // Skellebain (phase 2 or phase 3) poisoning also jumbles up the message
        if (affected_by_spell(pCh, POISON_SKELLEBAIN_2) || affected_by_spell(tCh, POISON_SKELLEBAIN_2)) {
            chance += 100;
        } else if (affected_by_spell(pCh, POISON_SKELLEBAIN_3) || affected_by_spell(tCh, POISON_SKELLEBAIN_3)) {
            chance += 100;
        }

        target = tCh;
        /* Walk the descriptor list to find an appropriate descriptor
         * for switched people.
         */
        for (d = descriptor_list; d; d = d->next) {
            if (tCh == d->original) {   // They are switched 
                target = d->character;
                break;          // Found it, stop looking
            }
        }
        if (target->desc) {
            if (chance < number(1, 100)) {
                if (silent) {
                    snprintf(buf, sizeof(buf),
                             "Someone sends you a telepathic message:\n\r     \"%s\"\n\r",
                             argument);
                } else {
                    snprintf(buf, sizeof(buf),
                             "%s sends you a telepathic message:\n\r     \"%s\"\n\r",
                             capitalize(MSTR(pCh, short_descr)), argument);
                }
                send_to_char(buf, target);
                clairaudient_listener(pCh, target, FALSE, argument);
            } else {
                if (!silent) {
                    snprintf(buf, sizeof(buf), "The image of %s wavers slightly in your mind.\n\r",
                             MSTR(pCh, short_descr));
                    send_to_char(buf, target);
                }
            }
        } else {
            snprintf(buf, sizeof(buf), "%s to %s (unavail): \"%s\"\n\r", REAL_NAME(pCh),
                     REAL_NAME(tCh), argument);
            send_to_higher(buf, STORYTELLER, pCh);
        }
        send_to_listeners(argument, pCh, tCh);
    } else {
        send_to_char("Something is blocking your telepathy.\n\r", pCh);
    }
}

void
cmd_cease(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH];
    int i;
    // If nothing is specified, stop all of them
    if (!(*argument)) {
        if (!cease_all_psionics(ch, TRUE))
            send_to_char("Ok.\n\r", ch);
        return;
    }
    // Otherwise, stop the one they asked for
    argument = one_argument(argument, arg1, sizeof(arg1));
    if (strlen(arg1) > 0) {
        for (i = 1; (*skill_name[i] != '\n') && i < MAX_SKILLS; i++) {
            if (ch->skills[i] && (skill[i].sk_class == CLASS_PSIONICS)
                && !strncmp(arg1, skill_name[i], MIN(strlen(arg1), strlen(skill_name[i])))) {
                if (has_skill(ch, i)) {
                    if (!cease_psionic_skill(ch, i, TRUE))
                        send_to_char("Nothing to cancel.\n\r", ch);
                    return;
                }
                break;          // No need to go on, we have a match
            }
        }
    }
    send_to_char("Cease what?\n\r", ch);
}

void
cmd_break(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH];
    OBJ_DATA *tmp_obj;
    if (!(*argument)) {
        // send_to_char("Break what?\n\r", ch);
        send_to_char("Use the 'cease' command to cancel psionics.\n\r", ch);
        return;
    }
    
    argument = one_argument(argument, arg1, sizeof(arg1));
    if (strlen(arg1) > 0) {
      tmp_obj = get_obj_in_list_vis(ch, arg1, ch->carrying);
      if (tmp_obj) {
	if (tmp_obj->obj_flags.type == ITEM_NOTE) {
	  if (IS_SET(tmp_obj->obj_flags.value[3], NOTE_SEALED)) {
	    if (!(IS_SET(tmp_obj->obj_flags.value[3], NOTE_BROKEN))) {
	      MUD_SET_BIT(tmp_obj->obj_flags.value[3], NOTE_BROKEN);
	      send_to_char("You break the seal.\n\r", ch);
	    }
	  } else {
	    send_to_char("The seal has already been broken.\n\r", ch);
	  }
	} else if (tmp_obj->obj_flags.type == ITEM_CONTAINER) {
	  if (IS_SET(tmp_obj->obj_flags.value[1], CONT_SEALED)) {
	    if (!(IS_SET(tmp_obj->obj_flags.value[1], CONT_SEAL_BROKEN))) {
	      MUD_SET_BIT(tmp_obj->obj_flags.value[1], CONT_SEAL_BROKEN);
	      send_to_char("You break the seal.\n\r", ch);
	    } else {
	      send_to_char("The seal has already been broken.\n\r", ch);
	    }
	  }
	}
      } else {
	send_to_char("What do you want to break?\n\r", ch);
      }
    }
}

int
cease_all_psionics(CHAR_DATA * ch, bool echo)
{
    int i;
    int ceased = FALSE;
    for (i = 1; (*skill_name[i] != '\n') && i < MAX_SKILLS; i++) {
        if (ch->skills[i] && (skill[i].sk_class == CLASS_PSIONICS)) {
            ceased += cease_psionic_skill(ch, i, echo);
        }
    }
    remove_contact(ch);
    return ceased;
}

/* Fairly self-explanatory, this function will stop the specified psionic
 * skill / ability.  Moved out of cmd_break() to become a helper function
 * to allow psionicists to specify which ability to break. -Tiernan 8/23/05 */
int
cease_psionic_skill(CHAR_DATA * ch, int sk, bool echo)
{
    char buf[MAX_STRING_LENGTH];
    int ceased = FALSE;

    switch (sk) {
    case PSI_HEAR:
        while (affected_by_spell(ch, PSI_HEAR)) {
            affect_from_char(ch, PSI_HEAR);
            if (echo)
                send_to_char("You cease eavesdropping on the Way.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off psionic hear.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_CATHEXIS:
        while (affected_by_spell(ch, PSI_CATHEXIS)) {
            affect_from_char(ch, PSI_CATHEXIS);
            if (echo)
                send_to_char("You slip out of your cathexic state.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off cathexis.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_CLAIRAUDIENCE:
        while (affected_by_spell(ch, PSI_CLAIRAUDIENCE)) {
            affect_from_char(ch, PSI_CLAIRAUDIENCE);
            if (echo)
                send_to_char("You stop listening through their ears.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off clairaudience.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_CLAIRVOYANCE:
        while (affected_by_spell(ch, PSI_CLAIRVOYANCE)) {
            affect_from_char(ch, PSI_CLAIRVOYANCE);
            if (echo)
                send_to_char("You stop looking through their eyes.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off clairvoyance.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_MESMERIZE:
        while (affected_by_spell(ch, PSI_MESMERIZE)) {
            affect_from_char(ch, PSI_MESMERIZE);
            if (echo)
                send_to_char("You stop forcing them to feel calm.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off mesmerize.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_DOME:
        while (affected_by_spell(ch, PSI_DOME)) {
            affect_from_char(ch, PSI_DOME);
            if (echo)
                send_to_char("You lower your psionic dome.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s lowers psionic dome.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_COMPREHEND_LANGS:
        while (affected_by_spell(ch, PSI_COMPREHEND_LANGS)) {
            affect_from_char(ch, PSI_COMPREHEND_LANGS);
            if (echo)
                send_to_char("You let down your psychic awareness of languages.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off allspeak.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_COERCION:
        while (affected_by_spell(ch, PSI_COERCION)) {
            affect_from_char(ch, PSI_COERCION);
            if (echo)
                send_to_char("You stop coercing them.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s stops their coercion.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_BARRIER:
        while (affected_by_spell(ch, PSI_BARRIER)) {
            affect_from_char(ch, PSI_BARRIER);
            if (echo)
                send_to_char("You let down your psionic barrier.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s lets down %s barrier.\n\r", MSTR(ch, name), HSHR(ch));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
            snprintf(buf, sizeof(buf), "%s lowers %s psionic barrier.", MSTR(ch, short_descr),
                     HSHR(ch));
            send_to_empaths(ch, buf);
        }
        break;
    case PSI_CONTACT:
        while (affected_by_spell(ch, PSI_CONTACT)) {
            affect_from_char(ch, PSI_CONTACT);
            if (echo)
                send_to_char("You dissolve the psychic link.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s severs %s psychic contact.\n\r", MSTR(ch, name),
                     HSHR(ch));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            if (ch->specials.contact && !affected_by_spell(ch, PSI_CONCEAL)) {
                if (has_skill(ch->specials.contact, PSI_TRACE)) {
                    if (psi_skill_success(ch->specials.contact, 0, PSI_TRACE, 0)) {
                        cprintf(ch->specials.contact, "You sense %s withdraw from your mind.\n\r",
                                MSTR(ch, short_descr));
                    } else {
                        cprintf(ch->specials.contact,
                                "You sense a foreign presence withdraw from your mind.\n\r");
                        gain_skill(ch->specials.contact, PSI_TRACE, 1);
                    }
                } else {
                    cprintf(ch->specials.contact,
                            "You sense a foreign presence withdraw from your mind.\n\r");
                }
            }
            remove_contact(ch);
            ceased = TRUE;
            snprintf(buf, sizeof(buf), "%s dissolves %s psychic contact.", MSTR(ch, short_descr),
                     HSHR(ch));
            send_to_empaths(ch, buf);
        }
        break;
    case PSI_MAGICKSENSE:
        while (affected_by_spell(ch, PSI_MAGICKSENSE)) {
            affect_from_char(ch, PSI_MAGICKSENSE);
            if (echo)
                send_to_char("You let down your magickal awareness.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off magicksense.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_CONTROL:
        while (affected_by_spell(ch, PSI_CONTROL)) {
            affect_from_char(ch, PSI_CONTROL);
            if (echo)
                send_to_char("You stop trying to control them.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off control.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_CONCEAL:
        while (affected_by_spell(ch, PSI_CONCEAL)) {
            affect_from_char(ch, PSI_CONCEAL);
            if (echo)
                send_to_char
                    ("You stop trying to conceal your psionic abilities from detection.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off conceal.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_VANISH:
        while (affected_by_spell(ch, PSI_VANISH)) {
            affect_from_char(ch, PSI_VANISH);
            if (echo)
                send_to_char("You stop trying to psionically cloak your appearance.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off vanish.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    case PSI_SENSE_PRESENCE:
        while (affected_by_spell(ch, PSI_SENSE_PRESENCE)) {
            affect_from_char(ch, PSI_SENSE_PRESENCE);
            if (echo)
                send_to_char("You let down your psychic awareness of presences.\n\r", ch);
            snprintf(buf, sizeof(buf), "%s breaks off sense presence.\n\r", MSTR(ch, name));
            send_to_monitor(ch, buf, MONITOR_PSIONICS);
            ceased = TRUE;
        }
        break;
    default:
        ceased = FALSE;
        break;
    }
    return ceased;
}

void
remove_contact(CHAR_DATA * ch)
{
    // Stop the contact affect & null out the pointer
    affect_from_char(ch, PSI_CONTACT);
    ch->specials.contact = NULL;

    // Stop any psionics that only work with a contact also
    affect_from_char(ch, PSI_CLAIRAUDIENCE);
    affect_from_char(ch, PSI_CLAIRVOYANCE);
    affect_from_char(ch, PSI_MESMERIZE);
    affect_from_char(ch, PSI_COERCION);
    affect_from_char(ch, PSI_CONTROL);

    if (affected_by_spell(ch, SPELL_RECITE))
        affect_from_char(ch, SPELL_RECITE);
}

/* Started 05/20/2000 -nessalin */
void
psi_sense_presence(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
                   struct obj_data *tObj)
{
    int skill_num;
    char buf[MAX_STRING_LENGTH];

    if (!has_skill(pCh, PSI_SENSE_PRESENCE)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_SENSE_PRESENCE)) {
        send_to_char("You are already focussing on extending your psychic awareness.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_SENSE_PRESENCE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    send_to_char("You concentrate on extending your psychic awareness.\n\r", pCh);
    snprintf(buf, sizeof(buf), "%s tries to use sense presence.\n\r", MSTR(pCh, name));
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);

    skill_num = (pCh->skills[PSI_SENSE_PRESENCE]) ? pCh->skills[PSI_SENSE_PRESENCE]->learned : 1;
    if (skill_success(pCh, NULL, skill_num))
        apply_psi_to_char(pCh, PSI_SENSE_PRESENCE);
    else
        gain_skill(pCh, PSI_SENSE_PRESENCE, 1);
}

/* PSI SKILLS */

/* form mental contact--necessary for lots of things */
/* for imms this is direct communication */
void
psi_contact(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
            struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    bool knocked_out = FALSE;
    bool silent = FALSE;
    bool barriered = FALSE;
    bool too_foreign = FALSE;
    int bonus = 0;
    int truenamebonus = 0;
    char temp_string[MAX_INPUT_LENGTH];
    char seen_names[MAX_INPUT_LENGTH] = "";
    char real_name_buf[512];


    if (pCh == tCh) {
        send_to_char("You can't contact yourself!\n\r", pCh);
        return;
    }

    if (GET_POS(pCh) == POSITION_FIGHTING
        && (GET_GUILD(pCh) != GUILD_PSIONICIST && GET_GUILD(pCh) != GUILD_LIRATHU_TEMPLAR)) {
        send_to_char("Not while fighting!\n\r", pCh);
        return;
    }
    // We want to drain them first, but if they have some kind of error we
    // also want to tell them the error as well.  We'll check this status
    // later and return if there weren't any errors. - Tiernan
    knocked_out = drain_char(pCh, PSI_CONTACT);

    if (pCh->specials.contact) {
        send_to_char("You are already in contact with someone else.\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_CONTACT)) {
        send_to_char("You are already in contact with someone else.\n\r", pCh);
        return;
    }

    if (blocked_psionics(pCh, tCh))
        return;

    /* Make sure they're on same Plane of Existance - Halaster 07/24/2006 */
    if (!is_on_same_plane(pCh->in_room, tCh->in_room)) {
        send_to_char("You are unable to reach their mind.\n\r", pCh);
        return;
    }

    if (IS_SET(pCh->in_room->room_flags, RFL_IMMORTAL_ROOM)) {
        send_to_char("You are unable to reach their mind.\n\r", pCh);
        return;
    }

    // race types must match
    if (GET_RACE_TYPE(pCh) != GET_RACE_TYPE(tCh)) {
        too_foreign = TRUE;
    } else {
        // look at the psi user's race
        switch(GET_RACE(pCh)) {
            // elementals can only contact other elementals and sub-elementals
            case RACE_ELEMENTAL:
               switch(GET_RACE(tCh)) {
                  case RACE_ELEMENTAL:
                  case RACE_MEPHIT:
                  case RACE_SUB_ELEMENTAL:
                     // ok, elementals can contact elementals and sub-elementals
                     break;
                  default:
                     too_foreign = TRUE;
                     break;
               }
               break;
             case RACE_SUB_ELEMENTAL:
               // sub-elementals can contact both
               break;
             default:
               // matching race type, but if the target is elemental...
               switch(GET_RACE(tCh)) {
                  case RACE_ELEMENTAL:
                  case RACE_MEPHIT:
                     too_foreign = TRUE;
                     break;
               }
         }
    }
     
    if (too_foreign) {
        send_to_char("Their mind is too foreign for you.\n\r", pCh);
        return;
    }

    /* figure out who is talking (switch) */
    if (is_char_ethereal(tCh)) {
        send_to_char("You are unable to reach their mind.\n\r", pCh);
        return;
    }
    // Mortals can never use psionics to immortals
    if (IS_IMMORTAL(tCh)) {
        send_to_char("You are unable to reach their mind.\n\r", pCh);
        return;
    }
    // Figure out if the target has a barrier
    barriered = affected_by_spell(tCh, PSI_BARRIER) ? TRUE : FALSE;

    if( index(arg, '.') != NULL ) {
       replace_char(arg, '.', ' ', buf, sizeof(buf));
       arg = buf;
    }

    /* get the first word out of temp_str, and put it in str */
    while ((arg = one_argument(arg, temp_string, sizeof(temp_string))), strlen(temp_string)) {
        to_lower(temp_string);
        if (strstr(seen_names, temp_string) != NULL ) continue;
        if (!stricmp(temp_string, REAL_NAME(tCh))) {
           truenamebonus += 20;
        } else if (stricmp(temp_string, "the") && stricmp(temp_string, "a")
           && stricmp(temp_string, "an") && bonus < 20) {
           bonus += 5;
        }
        strcat( seen_names, temp_string );
        strcat( seen_names, " " );
    }

    bonus += truenamebonus;

    if (!psi_skill_success(pCh, tCh, PSI_CONTACT, bonus)
        || IS_SET(tCh->in_room->room_flags, RFL_IMMORTAL_ROOM)
        || IS_SET(tCh->in_room->room_flags, RFL_NO_PSIONICS)) {
        /* this is the same msg as cmd_skill() */
        send_to_char("You are unable to reach their mind.\n\r", pCh);
        gain_skill(pCh, PSI_CONTACT, 1);
        return;
    }

    // Does the target know about psionic concealment?
    if (affected_by_spell(tCh, PSI_CONCEAL)) {
        if (pCh->skills[PSI_CONCEAL] && tCh->skills[PSI_CONCEAL]) {
            // Enough to not get their mind noticed?
            if (number(1, 100) >
                (tCh->skills[PSI_CONCEAL]->learned * (100 - pCh->skills[PSI_CONCEAL]->learned)) / 100) {
                send_to_char("You are unable to reach their mind.\n\r", pCh);
                return;
	        }
        } else if (skill_success(tCh, NULL, PSI_CONCEAL)) {
            send_to_char("You are unable to reach their mind.\n\r", pCh);
            return;
        }
    }

    if (knocked_out)
        return;

    // Outgoing
    snprintf(buf, sizeof(buf), "%s contacts %s with the Way.", MSTR(pCh, short_descr),
             MSTR(tCh, short_descr));
    send_to_empaths(pCh, buf);

    // Incoming
    snprintf(buf, sizeof(buf), "A foreign presence contacts %s with the Way.\n\r", MSTR(tCh, short_descr));
    send_to_empaths(tCh, buf);

    // If there was a barrier, the contact was not silent.
    if (!barriered)
        silent = affected_by_spell(pCh, PSI_CONCEAL) ? TRUE : FALSE;

    pCh->specials.contact = tCh;
    if (!silent) {
        if (has_skill(tCh, PSI_TRACE)) {
            if (psi_skill_success(tCh, 0, PSI_TRACE, 0)) {
                cprintf(tCh, "%s contacts your mind.\n\r", capitalize(MSTR(pCh, short_descr)));
            } else {
                cprintf(tCh, "A foreign presence contacts your mind.\n\r");
                gain_skill(tCh, PSI_TRACE, 1);
            }
        } else
            cprintf(tCh, "A foreign presence contacts your mind.\n\r");
    }

    snprintf(buf, sizeof(buf), "You %scontact %s with the Way.\n\r", silent ? "silently " : "",
             MSTR(tCh, short_descr));
    send_to_char(buf, pCh);
    snprintf(buf, sizeof(buf), "%s %scontacts %s with the Way.\n\r", MSTR(pCh, name),
             silent ? "silently " : "", MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    apply_psi_to_char(pCh, PSI_CONTACT);
}


/* put up a shield */
void
psi_barrier(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
            struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(pCh->specials.nosave, NOSAVE_PSIONICS)) {
        cprintf(pCh,
                "You may not erect a barrier because you are not resisting psionic abilities.\n\r");
        return;
    }

    if (affected_by_spell(pCh, PSI_BARRIER)) {
        send_to_char("You are already maintaining a barrier around your mind.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_BARRIER))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    if (psi_skill_success(pCh, NULL, PSI_BARRIER, 0)) {
        // Cancel all other ongoing psionics
        cease_all_psionics(pCh, TRUE);
        act("You build a psychic barrier around your mind.", FALSE, pCh, 0, 0, TO_CHAR);
        snprintf(buf, sizeof(buf), "%s creates a psionic barrier.\n\r", MSTR(pCh, name));
        send_to_monitor(pCh, buf, MONITOR_PSIONICS);
        snprintf(buf, sizeof(buf), "%s creates a psionic barrier.", MSTR(pCh, short_descr));
        send_to_empaths(pCh, buf);
        apply_psi_to_char(pCh, PSI_BARRIER);
    } else {
        gain_skill(pCh, PSI_BARRIER, 1);
        act("You fail to build a psychic barrier around your mind.", FALSE, pCh, 0, 0, TO_CHAR);
    }
}

void
psi_clairaudience(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
                  struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];

    if (!pCh->skills[PSI_CLAIRAUDIENCE]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_CLAIRAUDIENCE)) {
        send_to_char("You are already listening through their ears.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_CLAIRAUDIENCE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    if (!psi_skill_success(pCh, tCh, PSI_CLAIRAUDIENCE, 0)) {
        act("You cannot seem to infiltrate $S mind.", FALSE, pCh, 0, tCh, TO_CHAR);
        psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_CLAIRAUDIENCE, 1);
        snprintf(buf, sizeof(buf), "%s fails to use clairaudience on %s.\n\r", MSTR(pCh, name),
                 MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    act("You focus your thoughts inside $N's head.", FALSE, pCh, 0, tCh, TO_CHAR);
    apply_psi_to_char(pCh, PSI_CLAIRAUDIENCE);
    snprintf(buf, sizeof(buf), "%s uses clairaudience on %s.\n\r", MSTR(pCh, name),
             MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

}

#ifdef DISABLE_CLAIRVOYANCE
void
psi_clairvoyance(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
                 struct obj_data *tObj)
{

    if (!has_skill(pCh, PSI_CLAIRVOYANCE)) {
        send_to_char("What?\n\r", pCh);
        return;
    }
    send_to_char("Not implemented.\n\r", pCh);
    return;
}
#else
psi_clairvoyance(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
                 struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    int skill_num;

    if (!has_skill(pCh, PSI_CLAIRVOYANCE)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_CLAIRVOYANCE)) {
        send_to_char("You are already looking through their eyes.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_CLAIRVOYANCE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    skill_num = (pCh->skills[PSI_CLAIRVOYANCE]) ? pCh->skills[PSI_CLAIRVOYANCE]->learned : 1;
    if (!is_in_psionic_range(pCh, tCh, MAX(1, skill_num / 3))) {
        act("$N is too far away for you to focus clearly inside $S head.", FALSE, pCh, 0, tCh,
            TO_CHAR);
        return;
    }

    if (!psi_skill_success(pCh, tCh, PSI_CLAIRVOYANCE, 0)) {
        act("You cannot seem to infiltrate $S mind.", FALSE, pCh, 0, tCh, TO_CHAR);
        psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_CLAIRVOYANCE, 1);
        snprintf(buf, sizeof(buf), "%s fails to use clairvoyance on %s.\n\r", MSTR(pCh, name),
                 MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    act("You focus your thoughts inside $N's head.", FALSE, pCh, 0, tCh, TO_CHAR);
    apply_psi_to_char(pCh, PSI_CLAIRVOYANCE);
    snprintf(buf, sizeof(buf), "%s uses clairvoyance on %s.\n\r", MSTR(pCh, name), MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
}
#endif

void
psi_suggestion(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
               struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char com_str[MAX_INPUT_LENGTH];
    const char *t;
    char how[MAX_STRING_LENGTH];

    if (!has_skill(pCh, PSI_SUGGESTION)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_SUGGESTION))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    t = arg;
    while (t && *t && isspace(*t))
        t++;                    // Clip out leading whitespace

    // Find any emotional part
    if (t[0] == '-' || t[0] == '(' || t[0] == '[' || t[0] == '*')
        t = one_argument_delim(t, how, MAX_STRING_LENGTH, t[0]);
    else
        how[0] = '\0';

    strncpy(com_str, t, sizeof(com_str));
    if (!strlen(com_str)) {
        send_to_char("What do you want to suggest?\n\r", pCh);
        return;
    }

    if (!psi_skill_success(pCh, tCh, PSI_SUGGESTION, 0)) {
        act("Your psionic suggestion fails to echo in $S mind.", FALSE, pCh, 0, tCh, TO_CHAR);
        gain_skill(pCh, PSI_SUGGESTION, 1);
        return;
    }
    // Default suggestion, no emotions at all
    snprintf(buf2, sizeof(buf2), "think %s", com_str);

    // If an emotional part was given, do a project emotion check if the
    // psionicist is capable.  If that's also good, let the emotion go thru.
    if (strlen(how) && has_skill(pCh, PSI_PROJECT_EMOTION)) {
        if (drain_char(pCh, PSI_PROJECT_EMOTION))
            return;             // They passed out trying.

        // Skillcheck
        if (psi_skill_success(pCh, tCh, PSI_PROJECT_EMOTION, 0))
            snprintf(buf2, sizeof(buf2), "think (%s) %s", how, com_str);
        else
            gain_skill(pCh, PSI_PROJECT_EMOTION, 1);
    }

    snprintf(buf, sizeof(buf), "%s psionically suggests to %s: %s\n\r", MSTR(pCh, name),
             MSTR(tCh, name), com_str);
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    act("Your psionic suggestion echoes in $S mind.", FALSE, pCh, 0, tCh, TO_CHAR);
    parse_psionic_command(tCh, buf2);

    // Send it to whomever might 'hear' the suggestion travelling over the Way
    send_to_listeners(com_str, pCh, tCh);
}

void
psi_babble(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
           struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    struct affected_type af;
    int skill_num;

    memset(&af, 0, sizeof(af));

    if (!has_skill(pCh, PSI_BABBLE)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_BABBLE)) {
        send_to_char("You are already concentrating on jumbling their thoughts.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_BABBLE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    skill_num = (pCh->skills[PSI_BABBLE]) ? pCh->skills[PSI_BABBLE]->learned : 1;
    if (!is_in_psionic_range(pCh, tCh, MAX(1, skill_num / 30))) {
        act("$N is too far away for you to jumble their vocal abilities.", FALSE, pCh, 0, tCh,
            TO_CHAR);
        return;
    }

    if (!psi_skill_success(pCh, tCh, PSI_BABBLE, 0)) {
        act("You attempt to jumble $N's vocal abilities, but $E resists.", FALSE, pCh, 0, tCh,
            TO_CHAR);
        psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_BABBLE, 1);
        snprintf(buf, sizeof(buf), "%s unsuccessfully tries to cause %s to babble.\n\r",
                 MSTR(pCh, name), MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    snprintf(buf, sizeof(buf), "%s causes %s to babble.\n\r", MSTR(pCh, name), MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    act("You jumble $N's vocal abilities.", FALSE, pCh, 0, tCh, TO_CHAR);

    // Already babbling? Remove it so we can reset it.
    if (affected_by_spell(tCh, PSI_BABBLE))
        affect_from_char(tCh, PSI_BABBLE);
    else
        act("Your tongue feels swollen, and tingles slightly.", FALSE, pCh, 0, tCh, TO_VICT);

    // Make 'em babble
    af.type = PSI_BABBLE;
    af.location = 0;
    af.modifier = 0;
    af.duration = number(1, 2);
    af.bitvector = CHAR_AFF_PSI;
    affect_to_char(tCh, &af);
}

void
psi_disorient(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
              struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    struct affected_type af;
    int skill_num;
    OBJ_DATA *obj;

    memset(&af, 0, sizeof(af));

    if (!has_skill(pCh, PSI_DISORIENT)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_DISORIENT))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    skill_num = (pCh->skills[PSI_DISORIENT]) ? pCh->skills[PSI_DISORIENT]->learned : 1;
    if (!is_in_psionic_range(pCh, tCh, MAX(1, skill_num / 30))) {
        act("$N is too far away for you to disorient.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }

    if (!psi_skill_success(pCh, tCh, PSI_DISORIENT, 0)) {
        act("You attempt to disorient $N, but $E resists.", FALSE, pCh, 0, tCh, TO_CHAR);
        if (is_detected_psi(pCh, tCh))
            psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_DISORIENT, 1);
        snprintf(buf, sizeof(buf), "%s unsuccessfully tries to disorient %s.\n\r", MSTR(pCh, name),
                 MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    snprintf(buf, sizeof(buf), "%s disorients %s.\n\r", MSTR(pCh, name), MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    act("You disorient $N's thoughts.", FALSE, pCh, 0, tCh, TO_CHAR);
    act("You feel a heavy weight on your mind, and feel highly disoriented.", FALSE, pCh, 0, tCh,
        TO_VICT);

    af.type = PSI_DISORIENT;
    af.location = CHAR_APPLY_AGL;
    af.modifier = -MAX(1, pCh->skills[PSI_DISORIENT]->learned / 20);
    af.duration = number(1, 2);
    af.bitvector = CHAR_AFF_PSI;
    affect_to_char(tCh, &af);

    // Drop their EP, ES, or ETWO equipment immediately
    if (tCh->equipment[ES]) {
        obj = tCh->equipment[ES];
        obj_to_room(unequip_char(tCh, ES), tCh->in_room);

        act("You lose your grip and drop $p.", FALSE, tCh, obj, 0, TO_CHAR);
        act("$n loses $s grip and drops $p.", TRUE, tCh, obj, 0, TO_ROOM);

        drop_light(obj, tCh->in_room);
    }
    if (tCh->equipment[EP]) {
        obj = tCh->equipment[EP];
        obj_to_room(unequip_char(tCh, EP), tCh->in_room);

        act("You lose your grip and drop $p.", FALSE, tCh, obj, 0, TO_CHAR);
        act("$n loses $s grip and drops $p.", TRUE, tCh, obj, 0, TO_ROOM);

        drop_light(obj, tCh->in_room);
    }
    if (tCh->equipment[ETWO]) {
        obj = tCh->equipment[ETWO];
        obj_to_room(unequip_char(tCh, ETWO), tCh->in_room);

        act("You lose your grip and drop $p.", FALSE, tCh, obj, 0, TO_CHAR);
        act("$n loses $s grip and drops $p.", TRUE, tCh, obj, 0, TO_ROOM);

        drop_light(obj, tCh->in_room);
    }
    // Stop subduing whoever they are subduing
    if (tCh->specials.subduing)
        parse_psionic_command(tCh, "release");

}

#ifdef DISABLE_IMITATE
void
psi_imitate(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
            struct obj_data *tObj)
{
    if (!has_skill(pCh, PSI_IMITATE)) {
        send_to_char("What?\n\r", pCh);
        return;
    }
    send_to_char("Not implemented.\n\r", pCh);
    return;
}
#else
void
psi_imitate(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
            struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    int skill_num;

    if (!has_skill(pCh, PSI_IMITATE)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_IMITATE)) {
        send_to_char("You're already projecting yourself in their image.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_IMITATE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    skill_num = (pCh->skills[PSI_IMITATE]) ? pCh->skills[PSI_IMITATE]->learned : 1;
    if (!is_in_psionic_range(pCh, tCh, MAX(1, skill_num / 30))) {
        act("$N is too far away for you to imitate.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }

    snprintf(buf, sizeof(buf), "%s psionically imitates %s's appearance.\n\r", MSTR(pCh, name),
             MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
}
#endif

void
psi_project_emotion(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
                    struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    char com_str[MAX_INPUT_LENGTH];
    const char *t;

    if (!has_skill(pCh, PSI_PROJECT_EMOTION)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_PROJECT_EMOTION))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    t = arg;
    while (t && *t && isspace(*t))
        t++;                    // Clip out leading whitespace
    strncpy(com_str, t, sizeof(com_str));
    if (!strlen(com_str)) {
        send_to_char("What do you want to project?\n\r", pCh);
        return;
    }

    if (!psi_skill_success(pCh, tCh, PSI_PROJECT_EMOTION, 0)) {
        act("Your psionic projection fails to invoke an emotional response in $S mind.", FALSE, pCh,
            0, tCh, TO_CHAR);
        gain_skill(pCh, PSI_PROJECT_EMOTION, 1);
        return;
    }

    act("Your psionic projection invokes an emotional response in $S mind.", FALSE, pCh, 0, tCh,
        TO_CHAR);
    snprintf(buf, sizeof(buf), "%s psionically projects to %s: %s\n\r", MSTR(pCh, name),
             MSTR(tCh, name), com_str);
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    snprintf(buf, sizeof(buf), "feel %s", com_str);
    parse_psionic_command(tCh, buf);
}

void
psi_vanish(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
           struct obj_data *tObj)
{
    char distance[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    const char * const target_distance[] = {
	    "room",
	    "near",
	    "far",
	    "very",
	    "\n"
    };
    int skill_num, target = VANISH_ROOM;
    struct affected_type *vanished = NULL;

    if (!has_skill(pCh, PSI_VANISH)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    arg = one_argument(arg, distance, sizeof(distance));
    target = search_block(distance, target_distance, TRUE);
    if (target < VANISH_ROOM || target > VANISH_VERY_FAR) {
	send_to_char("Usage: vanish <room|near|far|very far>\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_VANISH))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    vanished = affected_by_spell(pCh, PSI_VANISH);
    if (vanished && target == vanished->power) {
        cprintf(pCh, "You are already concentrating on cloaking your appearance.\n\r");
        return;
    }

    skill_num = (pCh->skills[PSI_VANISH]) ? pCh->skills[PSI_VANISH]->learned : 1;
    // Their skill is below the target distance, they cannot reach that far
    if (skill_num < target * 25) {
        send_to_char("You are unable to psionically cloak your appearance.\n\r", pCh);
	return;
    }

    if (!psi_skill_success(pCh, NULL, PSI_VANISH, 0)) {
        if (vanished) {
            send_to_char("You are unable to adjust your psionic cloak.\n\r", pCh);
        } else {
            send_to_char("You are unable to psionically cloak your appearance.\n\r", pCh);
            gain_skill(pCh, PSI_VANISH, 1);
        }
        return;
    }

    if (vanished)
        send_to_char("You adjust your psionic cloak.\n\r", pCh);
    else {
        send_to_char("You psionically cloak your appearance.\n\r", pCh);
        apply_psi_to_char(pCh, PSI_VANISH);
    }

    if ((vanished = affected_by_spell(pCh, PSI_VANISH)))
	vanished->power = target;     // Need to set the range on the affect

    snprintf(buf, sizeof(buf), "%s psionically cloaks %s appearance.\n\r", MSTR(pCh, name),
             HSHR(pCh));
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);

    /* handle visibility checks for watchers */
    watch_vis_check(pCh);
}

void
psi_dome(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
         struct obj_data *tObj)
{
    CHAR_DATA *target, *next_target;
    char buf[MAX_STRING_LENGTH];

    if (!pCh->skills[PSI_DOME]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (IS_SET(pCh->specials.nosave, NOSAVE_PSIONICS)) {
        cprintf(pCh,
                "You may not erect a dome because you are not resisting psionic abilities.\n\r");
        return;
    }

    if (affected_by_spell(pCh, PSI_DOME)) {
        send_to_char("You've already constructed a psionic dome around the area.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_DOME))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    if (!psi_skill_success(pCh, NULL, PSI_DOME, 0)) {
        send_to_char("You attempt to construct a barrier around the area, but cannot.\n\r", pCh);
        gain_skill(pCh, PSI_DOME, 1);
        snprintf(buf, sizeof(buf), "%s fails to establish a psionic dome.\n\r", MSTR(pCh, name));
        send_to_monitor(pCh, buf, MONITOR_PSIONICS);
        return;
    }

    /* Check out the room and make life a bit more difficult for those people
     * using psionic abilities.
     */
    for (target = pCh->in_room->people; target; target = next_target) {
        next_target = target->next_in_room;     // In case the target goes away

        // Don't whack the psionicist erecting a dome
        if (target == pCh)
            continue;

        // Leave immoratls alone
        if (IS_IMMORTAL(target))
            continue;

        /* Someone in the room is in contact with someone else */
        if (target->specials.contact) {
            if (has_skill(pCh, PSI_DOME) && has_skill(target, PSI_CONTACT)) {
                if (number(1, 100) <=
                    (pCh->skills[PSI_DOME]->learned *
                     (100 - target->skills[PSI_CONTACT]->learned)) / 100) {
                    send_to_char("Your psionic connection slowly fades away.\n\r", target);
                    remove_contact(target);
                } else
                    gain_skill(pCh, PSI_DOME, 1);
            }
        }

        /* Someone is shadow-walking in the room */
        if (target && affected_by_spell(target, PSI_SHADOW_WALK) && target->desc
            && target->desc->original) {
            send_to_char
                ("The air around you condenses and your form is squeezed to nothingness.\n\r",
                 target);
            parse_psionic_command(target, "return");
        }
    }

    if (affected_by_spell(pCh, PSI_HEAR)) {
        send_to_char("You stop eavesdropping and construct a barrier around the area.\n\r", pCh);
        affect_from_char(pCh, PSI_HEAR);
    } else
        send_to_char("You construct a barrier around the area.\n\r", pCh);

    apply_psi_to_char(pCh, PSI_DOME);

    snprintf(buf, sizeof(buf), "%s establishes a psionic dome.\n\r", MSTR(pCh, name));
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);
}

void
psi_locate(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
           struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *fly = NULL;

    if (!pCh->skills[PSI_LOCATE]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_LOCATE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    /* As good a place as any to tell monitoring folks what's happening */
    snprintf(buf, sizeof(buf), "%s attempts to locate %s.\n\r", MSTR(pCh, name), MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    if (!psi_skill_success(pCh, tCh, PSI_LOCATE, 0)) {
        act("You are unable to determine $S location.", FALSE, pCh, 0, tCh, TO_CHAR);
        psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_LOCATE, 1);
        return;
    }
    /* Create a 'fly' where the target is and then 'look' through its eyes.
     * We use the fly as a temporary NPC to look through more than any actual
     * physical creature in the room, as using the actual character may have
     * side-effects as a result of torches, etc.  The fly was an easy and
     * innocuous NPC with infravision, ideal for our purposes.
     */
    if (tCh->in_room && (fly = read_mobile(11017, VIRTUAL))) {
        char_to_room(fly, tCh->in_room);
        fly->desc = pCh->desc;
        /* I added this but then decided to take it back out until the
         * clairvoyance skill was added. - Tiernan 8/13/2007
         * parse_psionic_command(fly, "look room");
         */
        parse_psionic_command(fly, "look room");
        char_from_room(fly);
        fly->desc = NULL;
        extract_char(fly);
    } else {                    // Something went wrong, pretend it was a failure.
        act("You are unable to determine $S location.", FALSE, pCh, 0, tCh, TO_CHAR);
        gamelogf("psi_locate: %s had an error attempting to locate %s.", GET_NAME(pCh),
                 GET_NAME(tCh));
    }
}

/* Compelling a victim */
void
psi_compel(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
           struct obj_data *tObj)
{
    char com_str[MAX_INPUT_LENGTH];
    char com1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int new_com, skill_num;
    const char *t;

    if (!has_skill(pCh, PSI_COMPEL)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_COMPEL))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    /* What they're compelling the target to do.  copy it over to com_str */
    t = arg;
    bzero(com_str, sizeof(com_str));
    while (t && *t && isspace(*t))
        t++;                    // Clip out leading whitespace
    strncpy(com_str, t, sizeof(com_str));

    /* Grab the first word */
    arg = one_argument(arg, com1, sizeof(com1));

    snprintf(buf, sizeof(buf), "%s attempts to compel %s to \"%s\".\n\r", MSTR(pCh, name),
             MSTR(tCh, name), com_str);
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
    if (!*com_str) {
        act("Compel $N to do what?", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }

    skill_num = (pCh->skills[PSI_COMPEL]) ? pCh->skills[PSI_COMPEL]->learned : 1;
    if (!is_in_psionic_range(pCh, tCh, MAX(1, skill_num / 30))) {
        act("$N is too far away for you to compel.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }
    // Can't compel someone who's gone.
    if (tCh->specials.gone) {
        act("You attempt to compel $N, but $E resists.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }

    new_com = get_command(com1);
    if (is_offensive_cmd(new_com) || !psi_skill_success(pCh, tCh, PSI_COMPEL, 0)) {
        act("You attempt to compel $N, but $E resists.", FALSE, pCh, 0, tCh, TO_CHAR);
        psionic_failure_msg(pCh, tCh);
        // They failed the psi skill, not that the directive was false
        if (!is_offensive_cmd(new_com))
            gain_skill(pCh, PSI_COMPEL, 1);
        return;
    }

    snprintf(buf, sizeof(buf), "You compel %s.\n\r", MSTR(tCh, short_descr));
    send_to_char(buf, pCh);
    send_to_char("A voice in your mind compels you despite your resistance.\n\r", tCh);
    parse_psionic_command(tCh, com_str);
    return;
}

/* probe for info */
void
psi_probe(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
          struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    int skill_num, g;
    struct time_info_data player_age;
    extern char *age_names[];
    CLAN_DATA *clan;
    int idx;
    char timestr[80];
    char buffer[MAX_STRING_LENGTH];


    if (!pCh->skills[PSI_PROBE]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_PROBE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    skill_num = (pCh->skills[PSI_PROBE]) ? pCh->skills[PSI_PROBE]->learned : 1;
    if (!is_in_psionic_range(pCh, tCh, MAX(1, skill_num / 20))) {
        act("$N is too far away for you to probe $S mind.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }

    if (GET_RACE_TYPE(pCh) != GET_RACE_TYPE(tCh)) {
        act("$N's mind seems too alien for you to understand.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }

    if (!psi_skill_success(pCh, tCh, PSI_PROBE, 0)) {
        act("You cannot discover anything about $M.", FALSE, pCh, 0, tCh, TO_CHAR);
        snprintf(buf, sizeof(buf), "%s unsuccesfully tries to probe %s.\n\r", MSTR(pCh, name),
                 MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_PROBE, 1);
        snprintf(buf, sizeof(buf), "%s fails to probe %s.\n\r", MSTR(pCh, name), MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }
    act("You probe $S mind...", FALSE, pCh, 0, tCh, TO_CHAR);
    snprintf(buf, sizeof(buf), "%s probes %s.\n\r", MSTR(pCh, name), MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    if (skill_num >= 10) {      // Only the most basic information
        player_age = age(tCh);
        cprintf(pCh, "%s is %d years, %d months, and %d days old,\n\r",
                capitalize(MSTR(tCh, short_descr)), player_age.year, player_age.month,
                player_age.day);
        g = age_class(get_virtual_age(player_age.year, GET_RACE(tCh)));
        g = MAX(0, g);
        g = MIN(5, g);
        cprintf(pCh, " which is %s for %s%s.\n\r", age_names[g],
                indefinite_article(race[tCh->player.race].name), lowercase(race[tCh->player.race].name));
        cprintf(pCh, "%s is %d inches tall, and weighs %d ten-stone.\n\r", capitalize(HSSH(tCh)),
                GET_HEIGHT(tCh), GET_WEIGHT(tCh));
    } else
        cprintf(pCh, "You cannot discover anything about %s.\n\r", HMHR(tCh));

    if (skill_num >= 30) {      // A little more information
        cprintf(pCh, "%s strength is %s, %s agility is %s,\n\r", capitalize(HSHR(tCh)),
                STAT_STRING(tCh, ATT_STR), HSHR(tCh), STAT_STRING(tCh, ATT_AGL));
        cprintf(pCh, "  %s wisdom is %s, and %s endurance is %s.\n\r", HSHR(tCh),
                STAT_STRING(tCh, ATT_WIS), HSHR(tCh), STAT_STRING(tCh, ATT_END));
    }

    if (skill_num >= 50) {      // Even more information
        if (tCh->player.info[1])
            cprintf(pCh, "%s objective: %s\n\r", capitalize(HSHR(tCh)), tCh->player.info[1]);

        if (!IS_NPC(tCh))
            cprintf(pCh, "%s truename is \"%s\".\n\r", capitalize(HSHR(tCh)), MSTR(tCh, name));

        if (tCh->clan) {
            cprintf(pCh, "%s is:\n\r", capitalize(HSSH(tCh)));
            for (clan = tCh->clan; clan; clan = clan->next) {
                int c = clan->clan;
                char *rname = get_clan_rank_name(c, clan->rank);

                if (clan->clan > 0 && clan->clan < MAX_CLAN) {
                    char *flags = show_flags(clan_job_flags, clan->flags);
                    cprintf(pCh, "%s%s%s, jobs: %s\n\r", rname ? capitalize(rname) : "",
                            rname ? " of the " : "", clan_table[c].name, flags);
                    free(flags);
                } else
                    cprintf(pCh, "%d (Unknown)\n\r", clan->clan);

            }
        }

        if (tCh->specials.contact)
            cprintf(pCh, "%s mind is in contact with %s.\n\r", capitalize(HSHR(tCh)),
                    MSTR(tCh->specials.contact, short_descr));
    }

    if (skill_num >= 60) {      // See shadowwalk description
        if (TRUE == get_char_extra_desc_value(tCh, "[SHADOWWALK_SDESC]", buf, MAX_STRING_LENGTH)) {
                cprintf(pCh, "%s walks the shadows as %s.\n\r", capitalize(HSSH(tCh)), buf);
        }
    }

}

/* traceback for contacts */
void
psi_trace(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
          struct obj_data *tObj)
{
    int counter = 0;
    CHAR_DATA *czCh;
    CHAR_DATA *zombie = NULL;
    CHAR_DATA *tar_ch = NULL;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char edesc[MAX_STRING_LENGTH];
    char name_string[MAX_INPUT_LENGTH];
    char account_string[MAX_INPUT_LENGTH];

    if (!pCh->skills[PSI_TRACE]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_TRACE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    snprintf(buf, sizeof(buf), "%s tries to trace the contact with %s mind.\n\r", MSTR(pCh, name),
             HSHR(pCh));
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);
    for (czCh = character_list; czCh; czCh = czCh->next) {
        if (czCh->specials.contact == pCh) {
            if (psi_skill_success(pCh, NULL, PSI_TRACE, 0)) {
                // See if the other person is psionically concealed
                if (!affected_by_spell(czCh, PSI_CONCEAL) || is_detected_psi(czCh, pCh)) {
                    counter++;
                    snprintf(buf, sizeof(buf),
                             "You sense that %s is in contact with your mind.\n\r", MSTR(czCh,
                                                                                         short_descr));
                    /* Find out if they're the victim of Dead Speak */
                    zombie = NULL;
                    for (tar_ch = czCh->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
                        if (tar_ch != czCh)
                            if (IS_SET(tar_ch->specials.act, CFL_UNDEAD))
                                if (tar_ch->master == czCh)
                                    if (get_char_extra_desc_value
                                        (tar_ch, "[DEADSPEAK_VICT_PC_info]", edesc,
                                         sizeof(edesc))) {
                                        sprintf(name_string, "%s", edesc);
                                        if (IS_NPC(pCh)) {
                                            sprintf(buf2, "%d", npc_index[pCh->nr].vnum);
                                            if (!strcmp(name_string, buf2))
                                                zombie = tar_ch;
                                        } else {
                                            if (get_char_extra_desc_value
                                                (tar_ch, "[DEADSPEAK_VICT_ACCOUNT_info]", edesc,
                                                 sizeof(edesc)))
                                                sprintf(account_string, "%s", edesc);
                                            if (!strcmp(name_string, pCh->name)
                                                && !strcmp(account_string, pCh->account))
                                                zombie = tar_ch;

                                        }
                                    }
                    send_to_char(buf, pCh);
                    if (zombie)
                        send_to_char
                            ("- A deathly chill strengthens the link between your mind and theirs.\n\r",
                             pCh);
                }
            } else
                gain_skill(pCh, PSI_TRACE, 1);
        }
    }
    if (!counter)
        send_to_char("You cannot sense any contacts with your mind.\n\r", pCh);
}

/* push someone out of your mind */
void
psi_expel(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
          struct obj_data *tObj)
{
    CHAR_DATA *czCh = 0, *next_vict;
    CHAR_DATA *zombie = NULL;
    CHAR_DATA *tar_ch = NULL;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char edesc[MAX_STRING_LENGTH];
    char name_string[MAX_INPUT_LENGTH];
    char account_string[MAX_INPUT_LENGTH];
    int hit_dam = 0, stun_dam = 0, skill_num;

    if (!pCh->skills[PSI_EXPEL]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_EXPEL))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    act("You attempt to clear your mind.", FALSE, pCh, 0, czCh, TO_CHAR);
    snprintf(buf, sizeof(buf), "%s tries to expel all contacts with %s mind.\n\r", MSTR(pCh, name),
             HSHR(pCh));
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);

    skill_num = (pCh->skills[PSI_EXPEL]) ? pCh->skills[PSI_EXPEL]->learned : 1;

    // Look for people in contact with the psi and toss 'em out!
    for (czCh = character_list; czCh; czCh = next_vict) {
        next_vict = czCh->next;
        if (czCh->specials.contact == pCh) {
            if (blocked_psionics(pCh, czCh))
                continue;
            // Stop if it might be too taxing
            if (calc_standard_drain(pCh, PSI_EXPEL) > GET_STUN(pCh))
                return;
            if (psi_skill_success(pCh, czCh, PSI_EXPEL, 0)) {
                snprintf(buf, sizeof(buf), "%s expels %s from %s mind.\n\r", MSTR(pCh, name),
                         MSTR(czCh, name), HSHR(pCh));
                send_to_monitors(pCh, czCh, buf, MONITOR_PSIONICS);

                /* Find out if they're the victim of Dead Speak, if so, sever ties. */
                zombie = NULL;
                for (tar_ch = czCh->in_room->people; tar_ch; tar_ch = tar_ch->next_in_room)
                    if (tar_ch != czCh)
                        if (IS_SET(tar_ch->specials.act, CFL_UNDEAD))
                            if (tar_ch->master == czCh)
                                if (get_char_extra_desc_value
                                    (tar_ch, "[DEADSPEAK_VICT_PC_info]", edesc, sizeof(edesc))) {
                                    sprintf(name_string, "%s", edesc);
                                    if (IS_NPC(pCh)) {
                                        sprintf(buf2, "%d", npc_index[pCh->nr].vnum);
                                        if (!strcmp(name_string, buf2))
                                            zombie = tar_ch;
                                    } else {
                                        if (get_char_extra_desc_value
                                            (tar_ch, "[DEADSPEAK_VICT_ACCOUNT_info]", edesc,
                                             sizeof(edesc)))
                                            sprintf(account_string, "%s", edesc);
                                        if (!strcmp(name_string, pCh->name)
                                            && !strcmp(account_string, pCh->account))
                                            zombie = tar_ch;

                                    }
                                }
                if (zombie) {
                    if (find_ex_description
                        ("[DEADSPEAK_VICT_PC_info]", zombie->ex_description, TRUE))
                        rem_char_extra_desc_value(zombie, "[DEADSPEAK_VICT_PC_info]");
                    if (find_ex_description
                        ("[DEADSPEAK_VICT_ACCOUNT_info]", zombie->ex_description, TRUE))
                        rem_char_extra_desc_value(zombie, "[DEADSPEAK_VICT_ACCOUNT_info]");
                    parse_psionic_command(zombie, "emote spasms violently for a few moments.");
                }

                remove_contact(czCh);
                // Apply some damage
                cprintf(czCh, "Your mind reels as %s forces you from %s mind!\n\r",
                        MSTR(pCh, short_descr), HSHR(pCh));
                hit_dam = MAX(1, (number(1, skill_num / 9) * GET_MAX_HIT(czCh)) / 100);
                stun_dam = MAX(1, (number(1, skill_num / 7) * GET_MAX_STUN(czCh)) / 100);
                if (!generic_damage(czCh, hit_dam, 0, 0, stun_dam)) {
                    snprintf(buf, sizeof(buf),
                             "%s %s%s%swas expelled to death by %s %s%s%sin room #%d.",
                             IS_NPC(czCh) ? MSTR(czCh, short_descr)
                             : GET_NAME(czCh), !IS_NPC(czCh) ? "(" : "",
                             !IS_NPC(czCh) ? czCh->account : "", !IS_NPC(czCh) ? ") " : "",
                             IS_NPC(pCh) ? MSTR(pCh, short_descr) : GET_NAME(pCh),
                             !IS_NPC(pCh) ? "(" : "", !IS_NPC(pCh) ? pCh->account : "",
                             !IS_NPC(pCh) ? ") " : "", czCh->in_room->number);
                    gamelog(buf);
                    free(czCh->player.info[1]);
                    czCh->player.info[1] = strdup(buf);
                    die(czCh);
                }
                // It's also taxing on the psionicist
                if (standard_drain(pCh, PSI_EXPEL))     // Knocked 'em out
                    return;
            } else {
                gain_skill(pCh, PSI_EXPEL, 1);
            }
        }
    }
}

void
psi_cathexis(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
             struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    char cname[MAX_STRING_LENGTH];
    int clan = 0;

    if (!pCh->skills[PSI_CATHEXIS])
        send_to_char("What?\n\r", pCh);

    if (affected_by_spell(pCh, PSI_CATHEXIS)) {
        send_to_char("You are already in a cathexic state.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_CATHEXIS))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    // Let's see if they specified a clan to cathexis with
    for (; *arg == ' '; arg++);

    strcpy(buf, arg);
    if (buf[0] != '\0') {
        for (clan = 0; clan < MAX_CLAN; clan++)
            /* does the string match and are they a member of that tribe? */
            if (!str_prefix(buf, clan_table[clan].name) && IS_TRIBE(pCh, clan))
                break;

        if (clan == MAX_CLAN) {
            send_to_char("You aren't a member of any such tribe.\n\r", pCh);
            return;
        }
    } else {
        if (pCh->clan)
            clan = pCh->clan->clan;

        if (!clan) {
            send_to_char("You aren't a member of any clan.\n\r", pCh);
            return;
        }
    }

    if (clan > 0 && clan < MAX_CLAN)
        strcpy(cname, clan_table[clan].name);
    else
        sprintf(cname, "%d", clan);

    snprintf(buf, sizeof(buf), "%s tries to enter a cathexic state with '%s'.\n\r", MSTR(pCh, name), cname);
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);

    if (!psi_skill_success(pCh, NULL, PSI_CATHEXIS, 0)) {
        send_to_char("You are unable to enter a cathexic state.\n\r", pCh);
        gain_skill(pCh, PSI_CATHEXIS, 1);
        return;
    }

    cprintf(pCh, "You enter a cathexic state with '%s'.\n\r", cname);
    apply_psi_to_char_modified(pCh, PSI_CATHEXIS, clan);
}

void
psi_comprehend_langs(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
                     struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];

    if (!pCh->skills[PSI_COMPREHEND_LANGS]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_COMPREHEND_LANGS)) {
        send_to_char("Your psychic awareness is currently augmented.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_COMPREHEND_LANGS))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    if (!psi_skill_success(pCh, NULL, PSI_COMPREHEND_LANGS, 0)) {
        send_to_char("You are unable to augment your psychic awareness.\n\r", pCh);
        gain_skill(pCh, PSI_COMPREHEND_LANGS, 1);
        return;
    }

    send_to_char("You augment your psychic awareness.\n\r", pCh);
    apply_psi_to_char(pCh, PSI_COMPREHEND_LANGS);

    snprintf(buf, sizeof(buf), "%s uses allspeak.\n\r", MSTR(pCh, name));
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);
}

void
psi_hear(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
         struct obj_data *tObj)
{
    struct listen_data *l;
    char area[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int target = -1;
    int skill_num, range, max;
    const char * const target_areas[] = {
        "none",
        "room",
        "region",
        "world",
        "\n"
    };
    struct affected_type *tmp_af = NULL;

    if (!has_skill(pCh, PSI_HEAR)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    arg = one_argument(arg, area, sizeof(area));
    target = search_block(area, target_areas, TRUE);
    if (target < AREA_ROOM || target > AREA_WORLD) {
        send_to_char("Usage: hear <room|region|world>\n\r", pCh);
        return;
    }


    if (affected_by_spell(pCh, PSI_DOME)) {
        send_to_char("You cannot do that while maintaining a psionic dome.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_HEAR))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    // Need to know what their maximum range is for their guild / subguild
    max = MAX(35 + wis_app[GET_WIS(pCh)].learn, get_max_skill(pCh, PSI_HEAR));

    /* Figure out the range for the target area
     * -1 since it's 0-based, and subtract the target since the array
     * lists the most diffcult area first.
     */
    range = (max / AREA_WORLD) * (target - 1);

    skill_num = (pCh->skills[PSI_HEAR]) ? pCh->skills[PSI_HEAR]->learned : 1;

    if (!affected_by_spell(pCh, PSI_HEAR)) {
        send_to_char("You attempt to eavesdrop on the Way...\n\r", pCh);

        snprintf(buf, sizeof(buf), "%s just tried to psionically hear the %s.\n\r", MSTR(pCh, name),
                 target_areas[target]);
        send_to_monitor(pCh, buf, MONITOR_PSIONICS);

        // Out of range
        if (range > skill_num) {
            return;
        }
        // Didn't pass the skillcheck
        if (!skill_success(pCh, NULL, skill_num)) {
            gain_skill(pCh, PSI_HEAR, 1);
            return;
        }
        // It's good
        apply_psi_to_char(pCh, PSI_HEAR);
        if ((tmp_af = affected_by_spell(pCh, PSI_HEAR)))
            tmp_af->power = target;     // Need to set the range on the affect

        // Put them on the listening list
        add_listener(pCh);
        for (l = listen_list; l != NULL; l = l->next)
            if (l->listener == pCh)
                break;
        if (l)
            l->area = target;
        return;
    }
    // Range is too high
    if (range > skill_num) {
        send_to_char("You cannot seem to change your listening range.\n\r", pCh);
        return;
    }
    // Didn't pass the skillcheck
    if (!skill_success(pCh, NULL, skill_num)) {
        send_to_char("You cannot seem to change your listening range.\n\r", pCh);
        gain_skill(pCh, PSI_HEAR, 1);
        return;
    }

    for (l = listen_list; l != NULL; l = l->next)
        if (l->listener == pCh)
            break;
    if (l) {
        if (l->area == target) {
            cprintf(pCh, "You are already concentrating on listening to the %s.\n\r",

                    target_areas[target]);
            return;
        }
        l->area = target;
        if ((tmp_af = affected_by_spell(pCh, PSI_HEAR)))
            tmp_af->power = target;     // Need to set the range on the affect
        snprintf(buf, sizeof(buf), "You adjust your listening range to the %s.\n\r",
                 target_areas[target]);
        send_to_char(buf, pCh);
        snprintf(buf, sizeof(buf), "%s is now psionically hearing the %s.\n\r", MSTR(pCh, name),
                 target_areas[target]);
        send_to_monitor(pCh, buf, MONITOR_PSIONICS);
    } else
        send_to_char("You cannot seem to change your listening range.\n\r", pCh);
}

void
psi_rejuvenate(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
               struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    int target = -1;
    char attribute[MAX_STRING_LENGTH];
    const char * const target_attributes[] = {
        "health",
        "hunger",
        "thirst",
        "sobriety",
        "stamina",
        "\n"
    };
    int delta = 0;
    int gain = 0;
    int drain = 0;
    int cost = 0;
    int stun_dam = 0;
    int min_attr = 0;
    int max_attr = 0;
    int cur_attr = 0;
    int skill_num, i;
    bool success = FALSE;
    extern int get_max_condition(CHAR_DATA * ch, int cond);

    // Find the attribute they want to rejuvenate
    arg = one_argument(arg, attribute, sizeof(attribute));
    target = search_block(attribute, target_attributes, FALSE);
    if (target == -1) {
        send_to_char("Usage: rejuvenate <", pCh);
        for (i = 0; strcmp(target_attributes[i], "\n"); i++)
            cprintf(pCh, "%s%c", target_attributes[i],
                    strcmp(target_attributes[i + 1], "\n") ? '|' : '>');
        send_to_char("\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_REJUVENATE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    // Figure out the damage as best we can up front
    skill_num = (pCh->skills[PSI_REJUVENATE]) ? pCh->skills[PSI_REJUVENATE]->learned : 1;
    switch (target) {
    case 0:                    // hitpoints
        min_attr = 1;
        max_attr = GET_MAX_HIT(pCh);
        cur_attr = GET_HIT(pCh);
        break;
    case 1:                    // hunger points
        min_attr = 0;
        max_attr = get_max_condition(pCh, FULL);
        cur_attr = GET_COND(pCh, FULL);
        break;
    case 2:                    // thirst points
        min_attr = 0;
        max_attr = get_max_condition(pCh, THIRST);
        cur_attr = GET_COND(pCh, THIRST);
        break;
    case 3:                    // intoxication points
        // Drunken points work opposite to everything else
        min_attr = 0;
        max_attr = get_max_condition(pCh, DRUNK);
        cur_attr = max_attr - GET_COND(pCh, DRUNK);
        break;
    case 4:                    // moves
        min_attr = 0;
        max_attr = GET_MAX_MOVE(pCh);
        cur_attr = GET_MOVE(pCh);
        break;
    default:
        // We should never be here, but if we do, abort
        gamelogf("psi_rejuvenate: unknown target (%d) specified", target);
        cprintf(pCh, "Nothing happens.\n\r");
        return;
    }

    // How far off are they from their ideal amount?
    delta = max_attr - cur_attr;

    // They're at the ideal amount, don't bother.
    if (delta == 0) {
        cprintf(pCh, "You cannot rejevunate any more %s.\n\r", target_attributes[target]);
        return;
    }
    // Figure out how much stun there is to take off
    stun_dam = MIN(MAX(0, GET_STUN(pCh) - 1), (skill_num * GET_MAX_STUN(pCh) / 100));

    gain = stun_dam * 100 / (200 - skill_num);
    drain = gain * 2 / 3;

    // Make sure we stay within the allowed range
    if ((cur_attr + gain < min_attr) || (cur_attr + gain > max_attr)) {
        gain = delta;
    }
    // Make sure we stay within the allowed range
    if ((cur_attr - drain < min_attr) || (cur_attr - drain > max_attr)) {
        drain = cur_attr - min_attr;
    }

    success = psi_skill_success(pCh, NULL, PSI_REJUVENATE, 0);
    if (success) {
        cost = (gain * (200 - skill_num)) / 100;        // (1 + (1-p)) * gain
        snprintf(buf, sizeof(buf), "%s rejuvenates %d %s.\n\r", MSTR(pCh, name), abs(gain),
                 target_attributes[target]);
    } else {
        cost = (drain * (200 - skill_num)) / 100;       // (1 + (1-p)) * drain
        snprintf(buf, sizeof(buf), "%s fails to rejuvenate and loses %d %s.\n\r", MSTR(pCh, name),
                 abs(drain), target_attributes[target]);
        gain_skill(pCh, PSI_REJUVENATE, 1);
    }
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);

    // If there are no damages to be done, don't do anything
    if (cost == 0) {
        cprintf(pCh, "Nothing happens.\n\r");
        return;
    }
    // Time to apply damages and give the char some echoes
    switch (target) {
    case 0:                    // health
        if (success) {
            snprintf(buf, sizeof(buf), "%s feels healthier.", MSTR(pCh, short_descr));
            adjust_hit(pCh, gain);
            act("Your wounds close naturally but at an extraordinarily accelerated rate.", FALSE,
                pCh, NULL, NULL, TO_CHAR);
            act("$n's wounds close naturally but at an extraordinarily accelerated rate.", TRUE,
                pCh, NULL, NULL, TO_ROOM);
        } else {
            snprintf(buf, sizeof(buf), "%s feels less healthy.", MSTR(pCh, short_descr));
            adjust_hit(pCh, -drain);
            act("Your wounds tear further apart.", FALSE, pCh, NULL, NULL, TO_CHAR);
            act("$n's wounds tear further apart.", TRUE, pCh, NULL, NULL, TO_ROOM);
        }
        adjust_stun(pCh, -cost);
        break;
    case 1:                    // hunger
        if (success) {
            snprintf(buf, sizeof(buf), "%s feels less hungry.", MSTR(pCh, short_descr));
            GET_COND(pCh, FULL) += gain;
            act("Your stomach relaxes as your hunger pains diminish in intensity.", FALSE, pCh,
                NULL, NULL, TO_CHAR);
        } else {
            snprintf(buf, sizeof(buf), "%s feels hungrier.", MSTR(pCh, short_descr));
            GET_COND(pCh, FULL) -= drain;
            act("Your stomach tightens up as your hunger pains rise in intensity.", FALSE, pCh,
                NULL, NULL, TO_CHAR);
        }
        adjust_stun(pCh, -cost);
        break;
    case 2:                    // thirst
        if (success) {
            snprintf(buf, sizeof(buf), "%s feels less thirsty.", MSTR(pCh, short_descr));
            GET_COND(pCh, THIRST) += gain;
            act("Your tongue and throat feel less parched.", FALSE, pCh, NULL, NULL, TO_CHAR);
        } else {
            snprintf(buf, sizeof(buf), "%s feels thirstier.", MSTR(pCh, short_descr));
            GET_COND(pCh, THIRST) -= drain;
            act("Your tongue and throat feel coarse and dry.", FALSE, pCh, NULL, NULL, TO_CHAR);
        }
        adjust_stun(pCh, -cost);
        break;
    case 3:                    // sobriety
        if (success) {
            snprintf(buf, sizeof(buf), "%s feels more sober.", MSTR(pCh, short_descr));
            GET_COND(pCh, DRUNK) -= gain;       // Works opposite to the rest
            act("Your head begins to clear.", FALSE, pCh, NULL, NULL, TO_CHAR);
        } else {
            snprintf(buf, sizeof(buf), "%s feels intoxicated.", MSTR(pCh, short_descr));
            GET_COND(pCh, DRUNK) += drain;      // Works opposite to the rest
            act("You feel your mental clarity diminishing as a result of intoxication.", FALSE, pCh,
                NULL, NULL, TO_CHAR);
        }
        adjust_stun(pCh, -cost);
        break;
    case 4:                    // stamina
        if (success) {
            snprintf(buf, sizeof(buf), "%s feels less tired.", MSTR(pCh, short_descr));
            adjust_move(pCh, gain);
            act("A warm flush of color suffuses your features, banishing a few of your signs of fatigue.", FALSE, pCh, NULL, NULL, TO_CHAR);
            act("A warm flush of color suffuses $n's features, banishing a few of $s signs of fatigue.", TRUE, pCh, NULL, NULL, TO_ROOM);
        } else {
            snprintf(buf, sizeof(buf), "%s feels more tired.", MSTR(pCh, short_descr));
            adjust_move(pCh, -drain);
            act("The color drains from your face as a wave of exhaustion washes over your body.",
                FALSE, pCh, NULL, NULL, TO_CHAR);
            act("The color drains from $n's face as a wave of exhaustion washes over $s body.",
                TRUE, pCh, NULL, NULL, TO_ROOM);
        }
        adjust_stun(pCh, -cost);
        break;
    default:
        snprintf(buf, sizeof(buf), "%s feels awkward for a brief moment.", MSTR(pCh, short_descr));
        break;
    }

    // Update their position/stats based on the changes we just applied
    update_pos(pCh);

    // Let empaths in contact with them feel what's happened.
    send_to_empaths(pCh, buf);
}

#ifdef DISABLE_MESMERIZE
void
psi_mesmerize(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
              struct obj_data *tObj)
{
    if (!has_skill(pCh, PSI_MESMERIZE)) {
        send_to_char("What?\n\r", pCh);
        return;
    }
    send_to_char("Not implemented.\n\r", pCh);
    return;
}
#else
void
psi_mesmerize(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
              struct obj_data *tObj)
{
    CHAR_DATA *tmp_ch = NULL;
    char buf[MAX_STRING_LENGTH];
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!has_skill(pCh, PSI_MESMERIZE)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_MESMERIZE)) {
        send_to_char("You're already hypnotizing them with peaceful thoughts.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_MESMERIZE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    if (pCh->in_room && tCh->in_room && pCh->in_room != tCh->in_room) {
        act("$N is too far away for you to mesmerize $S mind.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }
    if (!psi_skill_success(pCh, tCh, PSI_MESMERIZE, 0)) {
        act("You attempt to calm $N's mind, but $E resists.", FALSE, pCh, 0, tCh, TO_CHAR);
        if (is_detected_psi(pCh, tCh))
            psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_MESMERIZE, 1);
        snprintf(buf, sizeof(buf), "%s unsuccessfully tries to mesmerize %s.\n\r", MSTR(pCh, name),
                 MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    snprintf(buf, sizeof(buf), "%s mesmerizes %s.\n\r", MSTR(pCh, name), MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    act("You overwhelm $N's thoughts with a sense of peace and tranquility.", FALSE, pCh, 0, tCh,
        TO_CHAR);
    act("You are overwhelmed by a strong sense of peace and tranquility.", TRUE, pCh, 0, tCh,
        TO_VICT);

    af.type = PSI_MESMERIZE;
    af.duration = MAX(1, pCh->skills[PSI_MESMERIZE]->learned / 20);
    af.power = level;
    af.magick_type = MAGICK_PSIONIC;
    af.modifier = 0;
    af.location = CHAR_APPLY_NONE;
    af.bitvector = CHAR_AFF_CALM;
    affect_join(tCh, &af, FALSE, FALSE);

    stop_all_fighting(tCh);
    stop_fighting(pCh, tCh);

    for (tmp_ch = tCh->in_room->people; tmp_ch; tmp_ch = tmp_ch->next)
        stop_fighting(pCh, tCh);
    if (affected_by_spell(tCh, AFF_MUL_RAGE))
        affect_from_char(tCh, AFF_MUL_RAGE);
}
#endif

#ifdef DISABLE_COERCE
void
psi_coercion(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
             struct obj_data *tObj)
{
    if (!has_skill(pCh, PSI_COERCION)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    send_to_char("Not implemented.\n\r", pCh);
    return;
}
#else
void
psi_coercion(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
             struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    int new, which, amount;
    struct affected_type af;

    char *coerce_what[] = {
        "fatigue",
        "refresh",
        "thirst",
        "quench",
        "hunger",
        "full",
        "thought",
        "\n"
    };

    memset(&af, 0, sizeof(af));

    if (!has_skill(pCh, PSI_COERCION)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_COERCION)) {
        send_to_char("You are already coercing their thoughts.\n\r", pCh);
        return;
    }

    arg = one_argument(arg, arg1, sizeof(arg1));

    if (!*arg1) {
        send_to_char("You can use \"coerce\" with the following:\n\r", pCh);
        for (new = 0; *coerce_what[new] != '\n'; new++) {
            snprintf(buf, sizeof(buf), "     %s\n\r", coerce_what[new]);
            send_to_char(buf, pCh);
        }
        return;
    }

    if (drain_char(pCh, PSI_COERCION))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    /* Look for argument */
    which = search_block(arg1, coerce_what, 0);

    if (!psi_skill_success(pCh, tCh, PSI_COERCION, 0)) {
        if (has_skill(tCh, PSI_CONTACT)
            && ((get_skill_percentage(pCh, PSI_COERCION) + number(-50, 50)) >
                get_skill_percentage(tCh, PSI_CONTACT)))
            send_to_char("Your coercive thought did not take hold.\n\r", pCh);
        else {
            send_to_char("Your coercive thought did not take hold.\n\r", pCh);
            send_to_char("You feel someone trying to influence your thoughts.\n\r", tCh);
        }
        snprintf(buf, sizeof(buf), "%s unsuccessfully tries to coerce %s with %s.\n\r",
                 MSTR(pCh, name), MSTR(tCh, name), coerce_what[which]);
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    which = search_block(arg1, coerce_what, 0);
    switch (which) {
    case 0:                    /* Fatigue */
        gamelog("Coerce -- Fatigue");
        af.type = PSI_COERCION;
        af.duration = number(1, 2);
        af.power = 0;
        af.magick_type = MAGICK_PSIONIC;
        af.modifier = -GET_MAX_MOVE(tCh) * number(1, 5) / 100;
        af.location = CHAR_APPLY_MOVE;;
        af.bitvector = CHAR_AFF_PSI;
        affect_to_char(tCh, &af);
        send_to_char("You feel tired.\n\r", tCh);
        break;
    case 1:                    /* Alertness */
        gamelog("Coerce -- Refresh");
        af.type = PSI_COERCION;
        af.duration = number(1, 2);
        af.power = 0;
        af.magick_type = MAGICK_PSIONIC;
        af.modifier = GET_MAX_MOVE(tCh) * number(1, 5) / 100;
        af.location = CHAR_APPLY_MOVE;
        af.bitvector = CHAR_AFF_PSI;
        affect_to_char(tCh, &af);
        send_to_char("You feel refreshed.\n\r", tCh);
        break;
    case 2:                    /* Thirst */
        gamelog("Coerce -- Thirst");
        af.type = PSI_COERCION;
        af.duration = number(1, 2);
        af.power = 1;
        af.magick_type = MAGICK_PSIONIC;
        af.modifier = number(get_char_size(tCh), (get_char_size(tCh) * 2));
        af.location = CHAR_APPLY_THIRST;
        af.bitvector = CHAR_AFF_PSI;
        affect_to_char(tCh, &af);
        send_to_char("You are thirsty.\n\r", tCh);
        break;
    case 3:                    /* Quench */
        gamelog("Coerce -- Quench");
        af.type = PSI_COERCION;
        af.duration = number(1, 2);
        af.power = 1;
        af.magick_type = MAGICK_PSIONIC;
        af.modifier = amount;
        af.location = CHAR_APPLY_THIRST;
        af.bitvector = CHAR_AFF_PSI;
        affect_to_char(tCh, &af);
        break;
    case 4:                    /* Hunger */
        gamelog("Coerce -- Hunger");
        af.type = PSI_COERCION;
        af.duration = number(1, 2);
        af.power = 1;
        af.magick_type = MAGICK_PSIONIC;
        af.modifier = number(get_char_size(tCh), (get_char_size(tCh) * 2));
        af.location = CHAR_APPLY_HUNGER;
        af.bitvector = CHAR_AFF_PSI;
        affect_to_char(tCh, &af);
        send_to_char("You are hungry.\n\r", tCh);
        break;
    case 5:                    /* Full */
        gamelog("Coerce -- Full");
        af.type = PSI_COERCION;
        af.duration = number(1, 2);
        af.power = 1;
        af.magick_type = MAGICK_PSIONIC;
        af.modifier = amount;
        af.location = CHAR_APPLY_HUNGER;
        af.bitvector = CHAR_AFF_PSI;
        affect_to_char(tCh, &af);
        break;
    case 6:                    /* Thought */
        gamelog("Coerce -- Thought");
        snprintf(buf, sizeof(buf), "think %s", arg);
        parse_psionic_command(tCh, buf);
        break;
    default:
        send_to_char("You can't coerce that effect.\n\r", pCh);
        return;
    }

    snprintf(buf, sizeof(buf), "%s coerces %s with %s.\n\r", MSTR(pCh, name), MSTR(tCh, name),
             coerce_what[which]);
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    if (drain_char(pCh, PSI_COERCION))
        return;

    snprintf(buf, sizeof(buf), "You send your coerced suggestion to %s's mind.\n\r",
             MSTR(tCh, short_descr));
    send_to_char(buf, pCh);
}
#endif

void
psi_control(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
            struct obj_data *tObj)
{
    char com_str[MAX_INPUT_LENGTH];
    char com1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
   // int skill_num
	int new_com;
    const char *t;

    if (!has_skill(pCh, PSI_CONTROL)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_CONTROL))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    /* What they're forcing the target to do.  copy it over to com_str */
    t = arg;
    bzero(com_str, sizeof(com_str));
    while (t && *t && isspace(*t))
        t++;                    // Clip out leading whitespace
    strncpy(com_str, t, sizeof(com_str));

    /* Grab the first word */
    arg = one_argument(arg, com1, sizeof(com1));

    snprintf(buf, sizeof(buf), "%s attempts to control %s and order %s to \"%s\".\n\r",
             MSTR(pCh, name), HMHR(tCh), MSTR(tCh, name), com_str);
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
    if (!*com_str) {
        act("Take control and order $N to do what?", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }
// compile warning - set not used
 //   skill_num = (pCh->skills[PSI_CONTROL]) ? pCh->skills[PSI_CONTROL]->learned : 1;
    if (pCh->in_room != tCh->in_room) {
        act("$N is too far away for you to control.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }
    // Can't compel someone who's gone.
    if (tCh->specials.gone) {
        act("You attempt to take control of $N, but $E resists.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }

    new_com = get_command(com1);
    if (is_restricted_cmd(new_com) || !psi_skill_success(pCh, tCh, PSI_CONTROL, 0)) {
        act("You attempt to take control of $N, but $E resists.", FALSE, pCh, 0, tCh, TO_CHAR);
        psionic_failure_msg(pCh, tCh);
        // They failed the psi skill, not that the directive was false
        if (!is_restricted_cmd(new_com))
            gain_skill(pCh, PSI_CONTROL, 1);
        return;
    }

    snprintf(buf, sizeof(buf), "You take control of %s and give %s an order.\n\r",
             MSTR(tCh, short_descr), HMHR(tCh));
    send_to_char(buf, pCh);
    send_to_char("You fail to maintain mental control and obey a voice in your mind.\n\r", tCh);
    parse_psionic_command(tCh, com_str);
    return;
}

void
psi_conceal(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
            struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];

    if (!has_skill(pCh, PSI_CONCEAL)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_CONCEAL)) {
        send_to_char("You're already focussing on concealing your psionic abilities.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_CONCEAL))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    snprintf(buf, sizeof(buf), "%s tries to psionically conceal %s identity.\n\r", MSTR(pCh, name),
             HSHR(pCh));
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);

    if (psi_skill_success(pCh, NULL, PSI_CONCEAL, 0)) {
        apply_psi_to_char(pCh, PSI_CONCEAL);
        act("You intently focus on concealing your psionic abilities from detection.", FALSE, pCh,
            0, 0, TO_CHAR);
    } else {
        gain_skill(pCh, PSI_CONCEAL, 1);
        act("You cannot focus on concealing your psionic abilities from detection.", FALSE, pCh, 0,
            0, TO_CHAR);
    }
}

void
psi_empathy(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
            struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    char drunkmsg[MAX_STRING_LENGTH];
    int percent = 0;

    if (!has_skill(pCh, PSI_EMPATHY)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_EMPATHY))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    if (!psi_skill_success(pCh, tCh, PSI_EMPATHY, 0)) {
        act("You are unable to determine $S mood.", FALSE, pCh, 0, tCh, TO_CHAR);
        psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_EMPATHY, 1);
        snprintf(buf, sizeof(buf), "%s fails to determine %s's mood.\n\r", MSTR(pCh, name),
                 MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    snprintf(buf, sizeof(buf), "%s determines %s's mood.\n\r", MSTR(pCh, name), MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    memset(buf, 0, sizeof(buf));        // Clear out the buffer

    // They're near death
    if ((GET_POS(tCh) == POSITION_STUNNED) || (GET_POS(tCh) == POSITION_MORTALLYW)) {
        cprintf(pCh, "%s is dying.\n\r", capitalize(MSTR(tCh, short_descr)));
        // No need to say anything else, they're dying already!
        return;
    }

    if (get_char_mood(tCh, buf, sizeof(buf))) {
        cprintf(pCh, "%s is %s.\n\r", capitalize(MSTR(tCh, short_descr)), buf);
    }
    // Fury & raging muls, watch out!
    if (affected_by_spell(tCh, SPELL_FURY) || affected_by_spell(tCh, AFF_MUL_RAGE)) {
        cprintf(pCh, "%s is consumed by rage.\n\r", capitalize(MSTR(tCh, short_descr)));
    }
    // Calm
    if (affected_by_spell(tCh, SPELL_CALM)) {
        cprintf(pCh, "%s is very calm.\n\r", capitalize(MSTR(tCh, short_descr)));
    }
    // Fear
    if (affected_by_spell(tCh, SPELL_FEAR)) {
        cprintf(pCh, "%s is terrified.\n\r", capitalize(MSTR(tCh, short_descr)));
        send_to_char(buf, pCh);
    }
    // Paralyze
    if (is_paralyzed(tCh)) {
        cprintf(pCh, "%s is paralyzed.\n\r", capitalize(MSTR(tCh, short_descr)));
    }
    // They're in combat
    if (tCh->specials.fighting) {
        snprintf(buf, sizeof(buf), "%s is in an aggressive mood.\n\r",
                 capitalize(MSTR(tCh, short_descr)));
        send_to_char(buf, pCh);
    }
    // Hitpoints
    if (GET_HIT(tCh) < (GET_MAX_HIT(tCh) / 2)) {
        snprintf(buf, sizeof(buf), "%s is in a lot of pain.\n\r",
                 capitalize(MSTR(tCh, short_descr)));
        send_to_char(buf, pCh);
    }
    // Moves (Stamina)
    if (GET_MAX_MOVE(tCh) > 0)
        percent = (100 * GET_MOVE(tCh)) / GET_MAX_MOVE(tCh);
    else
        percent = -1;           /* How could MAX_MOVE be < 1?? */
    if (percent <= 80) {
        if (percent <= 10)
            snprintf(buf, sizeof(buf), "%s is exhausted.\n\r", capitalize(MSTR(tCh, short_descr)));
        else if (percent <= 30)
            snprintf(buf, sizeof(buf), "%s is very tired.\n\r", capitalize(MSTR(tCh, short_descr)));
        else if (percent <= 60)
            snprintf(buf, sizeof(buf), "%s is tired.\n\r", capitalize(MSTR(tCh, short_descr)));
        else
            snprintf(buf, sizeof(buf), "%s is a bit winded.\n\r",
                     capitalize(MSTR(tCh, short_descr)));
        send_to_char(buf, pCh);
    }
    // Hunger & thirst
    sprintf(buf, "%s is neither hungry nor thirsty.\n\r", capitalize(MSTR(tCh, short_descr)));
    // See if they're hungry and/or thirsty
    if (GET_COND(tCh, FULL) >= 0 && GET_COND(tCh, THIRST) >= 0) {
        if (GET_COND(tCh, FULL) < 10 && GET_COND(tCh, THIRST) < 10)
            sprintf(buf, "%s is hungry and thirsty.\n\r", capitalize(MSTR(tCh, short_descr)));
        else if (GET_COND(tCh, FULL) < 10)
            sprintf(buf, "%s is hungry.\n\r", capitalize(MSTR(tCh, short_descr)));
        else if (GET_COND(tCh, THIRST) < 10)
            sprintf(buf, "%s is thirsty.\n\r", capitalize(MSTR(tCh, short_descr)));
    }
    send_to_char(buf, pCh);

    // Intoxication
    if (is_char_drunk(tCh)) {
        switch (is_char_drunk(tCh)) {
        case DRUNK_LIGHT:
            strcpy(drunkmsg, "slightly ");
            break;
        case DRUNK_MEDIUM:
            strcpy(drunkmsg, "");
            break;
        case DRUNK_HEAVY:
            strcpy(drunkmsg, "very ");
            break;
        case DRUNK_SMASHED:
            strcpy(drunkmsg, "extremely ");
            break;
        case DRUNK_DEAD:
            strcpy(drunkmsg, "completely ");
            break;
        }
        snprintf(buf, sizeof(buf), "%s is %sintoxicated.\n\r", capitalize(MSTR(tCh, short_descr)),
                 drunkmsg);
        send_to_char(buf, pCh);
    }
}

void
psi_masquerade(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
               struct obj_data *tObj)
{
    char msg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int skill_num;

    if (!pCh->skills[PSI_MASQUERADE]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (!*arg) {
        send_to_char("Yes, but masquerade what to them?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_MASQUERADE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    skill_num = (pCh->skills[PSI_MASQUERADE]) ? pCh->skills[PSI_MASQUERADE]->learned : 1;
    if (!is_in_psionic_range(pCh, tCh, MAX(1, skill_num / 3))) {
        act("$N is too far away for you to send an image into $S mind.", FALSE, pCh, 0, tCh,
            TO_CHAR);
        return;
    }

    for (; *arg == ' '; arg++);
    if (!psi_skill_success(pCh, tCh, PSI_MASQUERADE, 0)) {
        act("You cannot seem to send your masquerade to $N's mind.", FALSE, pCh, 0, tCh, TO_CHAR);
        if (is_detected_psi(pCh, tCh))
            psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_MASQUERADE, 1);
        snprintf(buf, sizeof(buf), "%s fails to give %s the vision: %s\n\r", MSTR(pCh, name),
                 MSTR(tCh, name), arg);
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    sprintf(msg, "You masquerade the following to $N:\n\r  \"%s\"", arg);
    act(msg, FALSE, pCh, 0, tCh, TO_CHAR);
    act(arg, FALSE, pCh, 0, tCh, TO_VICT);
    snprintf(buf, sizeof(buf), "%s masquerades the following to %s: %s\n\r", MSTR(pCh, name),
             MSTR(tCh, name), arg);
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
}

void
psi_illusion(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
             struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    char msg[MAX_STRING_LENGTH];
    char illusion[MAX_STRING_LENGTH];
    struct room_data *rm;
    CHAR_DATA *target;

    if (!pCh->skills[PSI_ILLUSION]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (!(rm = pCh->in_room)) {
        sprintf(msg, "Error in psi_illusion: %s is not in a room.", MSTR(pCh, name));
        gamelog(msg);
        send_to_char("You are unable to transmit your psionic illusion.\n\r", pCh);
        return;
    }

    if (!*arg) {
        send_to_char("What illusion do you wish to transmit?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_ILLUSION))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    for (; *arg == ' '; arg++); /* Get rid of that space */
    strncpy(illusion, capitalize(arg), sizeof(illusion));       // Make a copy, we want to tweak some
    if (strlen(illusion) && !ispunct(illusion[strlen(illusion) - 1]))
        strncat(illusion, ".", sizeof(illusion));

    /* Biffs the illusion altogether */
    if (!psi_skill_success(pCh, NULL, PSI_ILLUSION, 0)) {
        send_to_char("You are unable to transmit your psionic illusion.\n\r", pCh);
        gain_skill(pCh, PSI_ILLUSION, 1);
        snprintf(buf, sizeof(buf), "%s failed to create the illusion: %s\n\r", MSTR(pCh, name),
                 illusion);
        send_to_monitor(pCh, buf, MONITOR_PSIONICS);
        return;
    }

    /* Check each person in the room to see if they are suceptible or not */
    for (target = rm->people; target; target = target->next_in_room) {
        if (target == pCh)
            continue;

        // Leave immortals alone
        if (IS_IMMORTAL(target)) {
            cprintf(target, "<%s>: %s\n\r", MSTR(pCh, name), illusion);
            continue;
        }

        if (!psi_skill_success(pCh, target, PSI_ILLUSION, 0)) {
            if (is_detected_psi(pCh, target))
                psionic_failure_msg(pCh, target);
        } else
            act(illusion, FALSE, pCh, 0, target, TO_VICT);
    }

    snprintf(msg, sizeof(msg), "You create the following illusion:\n\r  \"%s\"", illusion);
    act(msg, FALSE, pCh, 0, 0, TO_CHAR);

    snprintf(buf, sizeof(buf), "%s creates the illusion: %s\n\r", MSTR(pCh, name), illusion);
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);
}

/* Added for Lirathuan Templars per Laeris' request */
void
psi_mindblast(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
              struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    int hit_loss, stun_loss, skill_num;
    if (!pCh->skills[PSI_MINDBLAST]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_MINDBLAST))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    skill_num = (pCh->skills[PSI_MINDBLAST]) ? pCh->skills[PSI_MINDBLAST]->learned : 1;
    if (!is_in_psionic_range(pCh, tCh, MAX(1, skill_num / 30))) {
        act("$N is too far away for you to blast $S mind.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }
    // Mindblast is so aggressive, that discovery is automatic
    aggressive_psionics(pCh);

    if (!psi_skill_success(pCh, tCh, PSI_MINDBLAST, 0)) {
        act("You attempt to blast $N's mind, but $E resists.", FALSE, pCh, 0, tCh, TO_CHAR);
        psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_MINDBLAST, 1);
        snprintf(buf, sizeof(buf), "%s unsuccessfully tries to mindblast %s.\n\r", MSTR(pCh, name),
                 MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    act("Focusing your thoughts, you lash out at $N's mind with a concentrated "
        "bolt of mental energy.", FALSE, pCh, 0, tCh, TO_CHAR);
    act("Your body shudders as your consciousness is assaulted by a vibrant blast of mental energy.", FALSE, pCh, 0, tCh, TO_VICT);
    act("$N's body shudders as $E emits a wail of pain.", FALSE, 0, 0, tCh, TO_NOTVICT);
    snprintf(buf, sizeof(buf), "%s successfully mindblasts %s.\n\r", MSTR(pCh, name),
             MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    hit_loss = (GET_MAX_HIT(tCh) / 4);
    stun_loss = (GET_MAX_STUN(tCh) / 4);
    if (!generic_damage(tCh, hit_loss, 0, 0, stun_loss)) {
        snprintf(buf, sizeof(buf),
                 "%s %s%s%shas been mindblasted to death by %s %s%s%sin room #%d.",
                 IS_NPC(tCh) ? MSTR(tCh, short_descr)
                 : GET_NAME(tCh), !IS_NPC(tCh) ? "(" : "", !IS_NPC(tCh) ? tCh->account : "",
                 !IS_NPC(tCh) ? ") " : "", IS_NPC(pCh) ? MSTR(pCh, short_descr) : GET_NAME(pCh),
                 !IS_NPC(pCh) ? "(" : "", !IS_NPC(pCh) ? pCh->account : "",
                 !IS_NPC(pCh) ? ") " : "", tCh->in_room->number);
        gamelog(buf);
        free(tCh->player.info[1]);
        tCh->player.info[1] = strdup(buf);
        die(tCh);
    }
    update_pos(tCh);
}

/* Added for Lirathuan Templars per Laeris' request */
/* PSI MAGICKSENSE: Allows the recipient to see magickal affects.  */
void
psi_magicksense(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
                struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];

    if (!has_skill(pCh, PSI_MAGICKSENSE)) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_MAGICKSENSE)) {
        send_to_char("You are already focussing on your magickal awareness.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_MAGICKSENSE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    snprintf(buf, sizeof(buf), "%s tries to psionically sense magick.\n\r", MSTR(pCh, name));
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);

    if (psi_skill_success(pCh, NULL, PSI_MAGICKSENSE, 0)) {
        apply_psi_to_char(pCh, PSI_MAGICKSENSE);
        send_to_char("Your mind tingles as your awareness expands.\n\r", pCh);
    } else {
        gain_skill(pCh, PSI_MAGICKSENSE, 1);
        send_to_char("You have difficulty focussing your mind.\n\r", pCh);
    }
}

void
psi_mindwipe(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
             struct obj_data *tObj)
{
    char buf[MAX_STRING_LENGTH];
    struct affected_type af;
    int time, stam_loss, stun_loss, skill_num;

    memset(&af, 0, sizeof(af));

    if (!pCh->skills[PSI_MINDWIPE]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_MINDWIPE))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    skill_num = (pCh->skills[PSI_MINDWIPE]) ? pCh->skills[PSI_MINDWIPE]->learned : 1;
    if (!is_in_psionic_range(pCh, tCh, MAX(1, skill_num / 30))) {
        act("$N is too far away for you to wipe $S mind.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }
    // Mindwipe is so aggressive, that discovery is automatic
    aggressive_psionics(pCh);

    if (affected_by_spell(tCh, PSI_MINDWIPE)) {
        act("$N's mind is already unfocussed.", FALSE, pCh, 0, tCh, TO_CHAR);
        return;
    }

    if (!psi_skill_success(pCh, tCh, PSI_MINDWIPE, 0)) {
        act("You attempt to wipe $N's mind, but $E resists.", FALSE, pCh, 0, tCh, TO_CHAR);
        if (is_detected_psi(pCh, tCh))
            psionic_failure_msg(pCh, tCh);
        gain_skill(pCh, PSI_MINDWIPE, 1);
        snprintf(buf, sizeof(buf), "%s unsuccessfully tries to mindwipe %s.\n\r", MSTR(pCh, name),
                 MSTR(tCh, name));
        send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);
        return;
    }

    snprintf(buf, sizeof(buf), "%s successfully mindwipes %s.\n\r", MSTR(pCh, name),
             MSTR(tCh, name));
    send_to_monitors(pCh, tCh, buf, MONITOR_PSIONICS);

    act("You scramble $N's thoughts.", FALSE, pCh, 0, tCh, TO_CHAR);
    act("You feel a heavy weight on your mind, and it becomes very difficult to" " think.", FALSE,
        pCh, 0, tCh, TO_VICT);
    time = ((pCh->skills[PSI_MINDWIPE]->learned / 10) - 1);
    time = MAX(1, time);
    time = MIN(time, 8);
    af.type = PSI_MINDWIPE;
    af.location = 0;
    af.modifier = 0;
    af.duration = time;
    af.bitvector = CHAR_AFF_PSI;
    affect_to_char(tCh, &af);
    stam_loss = (GET_MAX_MOVE(tCh) / 4);
    adjust_move(tCh, -stam_loss);
    stun_loss = (GET_MAX_STUN(tCh) / 4);
    generic_damage(tCh, 0, 0, 0, stun_loss);
    update_pos(tCh);
}

void
psi_shadow_walk(byte level, CHAR_DATA * pCh, const char *arg, int si, CHAR_DATA * tCh,
                struct obj_data *tObj)
{
    int i;
    char bug[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char shadow_name[MAX_STRING_LENGTH];
    char *f;
    CHAR_DATA *shadow;
    ROOM_DATA *in_room = NULL;
    struct affected_type *tmp_af, af;
    char *shadowwalk_mdesc = NULL;
    char *shadowwalk_sdesc = NULL;

    memset(&af, 0, sizeof(af));

    extern int shadow_walk_into_dome_room(struct room_data *tar_room);

    if (!pCh->skills[PSI_SHADOW_WALK]) {
        send_to_char("What?\n\r", pCh);
        return;
    }

    if (affected_by_spell(pCh, PSI_SHADOW_WALK)) {
        send_to_char("You are already walking the shadows.\n\r", pCh);
        return;
    }

    if (drain_char(pCh, PSI_SHADOW_WALK))
        return;

    if (blocked_psionics(pCh, tCh))
        return;

    snprintf(buf, sizeof(buf), "%s attempts to shadowwalk.\n\r", MSTR(pCh, name));
    send_to_monitor(pCh, buf, MONITOR_PSIONICS);

    if (!pCh->desc) {
        send_to_char("You attempt to walk the shadows, but are unsuccessful.\n\r", pCh);
        return;
    }

    if (pCh->desc->original) {
        send_to_char("You cannot shadowwalk while switched.\n\r", pCh);
        return;
    }

    if (pCh->desc->snoop.snoop_by) {
        pCh->desc->snoop.snoop_by->desc->snoop.snooping = 0;
        pCh->desc->snoop.snoop_by = 0;
    }

    if (pCh->desc->snoop.snooping) {
        send_to_char("You attempt to walk the shadows, but are unsuccessful.\n\r", pCh);
        return;
    }

    if (!psi_skill_success(pCh, NULL, PSI_SHADOW_WALK, 0)) {
        send_to_char("You attempt to walk the shadows, but are unsuccessful.\n\r", pCh);
        gain_skill(pCh, PSI_SHADOW_WALK, 1);
        return;
    }

    in_room = pCh->specials.contact ? pCh->specials.contact->in_room : pCh->in_room;
    assert(in_room);            // This would be bad
    if (shadow_walk_into_dome_room(in_room)) {
        send_to_char("Something prevents you from entering the shadows.\n\r", pCh);
        return;
    }
    // Success, send the shadow off
    shadow = read_mobile(1203, VIRTUAL);
    char_to_room(shadow, in_room);
    sprintf(shadow_name, "%s [%s]", MSTR(shadow, name), MSTR(pCh, name));
    f = strdup(shadow_name);
    free((char *) shadow->name);
    shadow->name = f;
    /* Also a good spot to move this "shadow" char onto the lists of those
     * who are monitoring the char */
    add_monitoring_for_mobile(pCh, shadow);

    send_to_char("You enter the shadows with your mind.\n\r", pCh);
    if (!pCh->desc->original)
        pCh->desc->original = pCh;
    pCh->desc->character = shadow;
    shadow->desc = pCh->desc;
    pCh->desc = 0;
    /* Give psionics & languages to shadow */
    for (i = 0; i < MAX_SKILLS; i++) {
        if (pCh->skills[i]
            && (skill[i].sk_class == CLASS_LANG || i == PSI_SHADOW_WALK || i == PSI_THOUGHTSENSE
                || i == PSI_EMPATHY || i == PSI_IMMERSION)) {
            if (!has_skill(shadow, i))
                init_skill(shadow, i, pCh->skills[i]->learned);
            else
                shadow->skills[i]->learned = pCh->skills[i]->learned;
            if (IS_SET(pCh->skills[i]->taught_flags, SKILL_FLAG_HIDDEN))
                MUD_SET_BIT(shadow->skills[i]->taught_flags, SKILL_FLAG_HIDDEN);
        }
    }
    /* Add new mdesc and sdesc if applicable */
    shadowwalk_mdesc = find_ex_description("[SHADOWWALK_DESC]", pCh->ex_description, TRUE);
    shadowwalk_sdesc = find_ex_description("[SHADOWWALK_SDESC]", pCh->ex_description, TRUE);
    if (shadowwalk_mdesc && shadowwalk_sdesc) {
	free(shadow->description);
        shadow->description = strdup(shadowwalk_mdesc);
	free(shadow->short_descr);
	shadow->short_descr = strdup(shadowwalk_sdesc);
    }

    // The mind is leaving the body, no psionic affects should remain on the
    // body at this point.
    cease_all_psionics(pCh, FALSE);

    // Apply the affect to the psionicist's body to reflect the effort
    // required to walk the shadows.
    apply_psi_to_char(pCh, PSI_SHADOW_WALK);

    // Copy the shadowwalk affect from the PC so we can use the same
    // settings for the shadowwalk affect on the NPC
    if ((tmp_af = affected_by_spell(pCh, PSI_SHADOW_WALK))) {
        memcpy(&af, tmp_af, sizeof(af));
        // Adjust their endurance so it falls in the range [25-75]%
        af.modifier =
            -(MIN
              (MAX
               (GET_END(shadow) / 4,
                GET_END(shadow) * (100 - pCh->skills[PSI_SHADOW_WALK]->learned) / 100),
               GET_END(shadow) * 3 / 4));
        af.location = CHAR_APPLY_END;
        affect_to_char(shadow, &af);
    }
    // Configure their language and stats
    shadow->specials.language = pCh->specials.language;
    shadow->specials.contact = pCh;
    set_hit(shadow, GET_MAX_HIT(shadow));
    set_move(shadow, GET_MAX_MOVE(shadow));
    set_stun(shadow, GET_MAX_STUN(shadow));

    strncpy(bug, "prompt St:%t(%T) Mv:%v(%V):", sizeof(bug));
    parse_psionic_command(shadow, bug);
}

/* make_contacts()
 *
 * This emulates the virtual psionics going on in the world and adds to the
 * general clutters and chaos of psionic hear.
 */
void
make_contacts()
{
    static time_t last_update;
    time_t now = time(NULL);
    struct listen_data *l;
    int msg, weight, in_city = 0;
    char *messages[] = {
        "When I mentioned the appearance of the one I'd seen...",
        "Plenty of breeze?",
        "Another tower.",
        "By the way, we are to maintain good relations with them.",
        "I'm not worried about it much, though",
        "Hello my friend, it is I.",
        "I'll be careful, no problem *smirk*",
        "I must know your side on the matter!",
        "...you pick good friends, then.",
        "I can do that, but I don't think I can handle the trip alone.",
        "Isn't there an army of gith surrounding the city?",
        "Who knows ...",
        "Good...good.  How did you do?",
        "Much better profit that way if you ask me.",
        "Actually I have to take care of something.",
        "Thanks.",
        "How are things going?",
        "Fine, but slow.",
        "Ahmm, I'd need some more things.",
        "I think I did well.",
        "So I believe we shouldn't.",
        "But I should retire soon.",
        "'ello brother.",
        "ACK!! I just killed them both...",
        "I wish to speak with you at my estate.",
        "As soon as I wrap up some business here.",
        "I shall await you.",
        "I have not forgotten, I shall speak with you in a bit.",
        "Certainly.",
        "Sorry!",
        "Something strange just happened to me.",
        "The artist is ready.",
        "Please forgive my disappearance, though...are you ready?",
        "I've been trying to reach you.",
        "Hello again. I have returned.",
        "Come home now!",
        "It has been long since we have spoken.",
        "When will you brave the wastes again?",
        "What will we eat tonight?",
        "This man is horrid.  Take me away from him.",
        "Together, we can take her down.  She is not so big.",
        "Yes.",
        "When?  I must leave soon.",
        "The child is tired.",
        "We must leave now.",
        "Quiet.  it will hear us.",
        "NOW!",
        "Go, go!",
        "*smile*",
        "*sigh*",
        "Of course, I'll come see you.",
        "Lost in the desert?",
        "Should I find someone to go find you?",
        "That's good news!",
        "I prefer to speak this way, I think.",
        "*assenting thought*",
        "It is secure, and less likely to cause offense... and is nicer as well.",
        "The Kuraci all said much the same...",
        "*concern*",
        "Found more coin near gate.",
        "I'll be careful to choose my time well if and when I travel to the northlands.",
        "I thought as much. *smiling*",
        "At some point, I'd like to speak more about our house, when you have the time...",
        "Now is good for me.",
        "Shall we go to the rooftops?",
        "Picture it, if you will.",
        "As you wish, the items you are looking for are rare.",
        "I am sad, that you would think our prices inflated.",
        "Is there any aback the inix?",
        "After what happened...",
        "Until they suddenly set upon you.",
        "Hmm...",
        "I wonder what was further up...",
        "It continued to rise, quite a distance...",
        "You're right, we didn't.",
        "I'll be there shortly.",
        "Meet me at the regular spot.",
        "Are you prowling the streets tonight?",
        "No, no...I don't think I'll be able to arrange that.",
        "Come now, surely you jest?",
        "Have patience, you will know more soon.",
        "Is it as you intended?",
        "Yes, very good then.",
        "Everyone knows that!",
        "None shall know of it.",
        "Do you wish to know my intentions?",
        "My intentions are for me to know and none other.",
        "I interpreted it as such.",
        "You truly wish that of me?",
        "If you insist.",
        "Be vigilant!",
        "Of course I'll wait for you, my love.",
        "Hurry, the time is 'nigh.",
        "Filthy elves!",
        "Stupid humans!",
        "Obstinant dwarves!",
        "Disgusting half-breeds!",
        "Would you truly?",
        "Are you certain?",
        "Yes, I like the plan.",
        "I search for meaning.",
        "I've lost it!",
        "I found it!",
        "Hurry, they approach....",
        "Move swiftly.",
        "I don't believe so, no.",
        "Could it be true?",
        "Be wary, sister...",
        "As always, brother...",
        "Without it, I am nothing.",
        "Join me now...tonight...we flee!",
        "*fuzzily* Hmmm?",
        "I'll be there shortly.",
        "We should discuss this in person.",
        "Leave me alone!",
        "Begone from my mind!",
        "Come to me...",
        "Tonight is the night...",
        "Be prepared, we strike soon.",
        "Eventually vengeance will be mine...",
        "I will have my revenge.",
        "Are you the one I seek?",
        "You are not the one I seek.",
        "Shall we go, you and I, while we can?",
        "It's steel, I tell you!  True steel!",
        "Is it truly made of iron?",
        "Steinal?  'Tis naught but a rumor.",
        "You've found it?  Truly?",
        "He listens...he always listens...",
        "I will kill him, I tell you!",
        "He is everywhere, fear him!",
        "She has eyes everywhere, beware.",
        "Her spies hear all, take care.",
        "She's a danger to us all, she must be eliminated.",
        "He's an abomination...he frightens me...he must die.",
        "Shhhh!  He'll hear.",
        "The Way is not safe, meet me in person.",
        "Yes, that's the target...strike now!",
        "I've just awoke, let's meet at the place.",
        "Shhh...I suspect he's a mindbender...",
        "Hush!  I think she's a Master of the Way, she could be listening.",
        "We will return and we will conquer our enemies, have patience.",
        "I truly hate him.",
        "I've some information you might be interested in, meet me at our spot.",
        "Yes, she is one of our spies.",
        "He is trustworthy, believe his words.",
        "She is a liar, don't believe what she tells you.",
        "I hear she's a spy for the gypsies.",
        "Sam will kill him if he tries anything.",
        "I think that she works for the Kuraci.",
        "I'm not sure but I heard he's on the Guild's payroll.",
        "I regret the day our paths ever crossed.",
        "Who are you and what do you want of me?",
        "It was an odd little song, monotonous and consisting of three notes.",
        "No, no, it was a cactus, walking like you or I.",
        "The silt is swallowing my skimmer.",
        "I must say goodbye now.",
        "All I found were ashes and bones.",
        "She's not sure who the child's father is.",
        "He always insisted there was a third moon, forever in shadow.",
        "I want to learn to read.",
        "She wears her hair in braids normally.",
        "I think it is a mortal wound.",
        "He smiles, but all the while he plots treachery.",
        "He will keep his peace, for I slit his throat.",
        "Oh, she was lovely as a blooming rose.",
        "Sweet as Blackwing honey, his kisses.",
        "And all the while, they stood there grinning like fools at each other.",
        "She proposed it, but he said no and broke her heart.",
        "Luckily, she was a sturdy lass.",
        "What can I say?  We were young.",
        "The lady likes a little on the side, if you know what I mean.",
        "I keep thinking about Allanak whiskey.",
        "I'll never see the like of that again.",
        "That blonde woman over there - she's the one I'm gonna kill.",
        "See that black-haired woman?  She's the one I'm gonna kill.",
        "I found it!  I found it!  We're gonna be rich now!",
        "Who's that fine-lookin' woman?  I think I'll buy 'er a drink.",
        "I need more components, my friend.. please bring me the usual ones.",
        "I beheaded the bastard!",
        "I have said it many times now... pants are optional.",
        "Southside is not very exciting.",
        "Meet me outside after sundown, an' bring the poison.",
        "Crap, he's here!  This isn' good.. ",
        "Throw me another one.",
        "Meet me in the Sanctuary at dusk.",
        "Oh no, not again!",
        "I'm really sick and tired of that man.  Everytime I try to do something for him, he just pisses all over it.  To the hellpits with him!",
        "Eww, that doesn't look good.",
        "I really miss you, my love.  Give the baby my love.",
        "I don't want to die.. please, save me from this templar!",
        "Damn.",
	// Maybe throw in a few topical ones from time to time here
	"I hear they're on the prowl again.",
	"Where's my coin?   Damn elves!",
	"I'm so hungry... don't you have anything to spare?",
	"It's been days since I've eaten, why is everything so expensive?",
	"I haven't seen them in weeks, I fear they may be dead.",
	"Where are my pants?  What did you do with my pants?!",
	"I wonder if things will ever be the same again?",
	"Why?  Why did he have to die?",
	"He let this happen.  He is supposed to protect us.",
	"Where were they when we needed them most?",
	"Just goes to show that He isn't that great, yeah?",
	"I've got a bad feeling about this...",
	"It's not my fault!"
    };

    // We already processed a message this second
    if (now == last_update)
        return;

    // The modulo decides the frequency (in seconds) we send a message
    if (now % 33)
        return;

    // Set our static timestamp so we don't send more than one message
    last_update = now;

    // Select a message that all the listeners may hear.
    msg = number(0, sizeof(messages) / sizeof(messages[0]) - 1);

    // Select a city/region that the message is in
    in_city = number(1, MAX_CITIES) - 1;

    /* We want the two city-states to have more traffic than the rest of
     * the cities, so we'll weight them at 40% per city-state and 20% for
     * the rest of the smaller outposts, villages, etc.
     */
    if (in_city) {
        weight = number(1, 100);
        if (weight <= 40)
            in_city = CITY_ALLANAK;
        if (weight > 40 && weight <= 80)
            in_city = CITY_TULUK;
    }

    for (l = listen_list; l != NULL; l = l->next) {
        if (!l->listener)
            break;
        if (!l->listener->in_room)
            break;

        switch (l->area) {
        case AREA_ROOM:
        default:
            continue;
        case AREA_ZONE:
            if (char_in_city(l->listener) != in_city)
                continue;
            break;
        case AREA_WORLD:
            break;
        }

        // No auto-generated messages on elemental planes
        switch (l->listener->in_room->sector_type) {
        case SECT_FIRE_PLANE:
        case SECT_WATER_PLANE:
        case SECT_AIR_PLANE:
        case SECT_EARTH_PLANE:
        case SECT_SHADOW_PLANE:
        case SECT_LIGHTNING_PLANE:
        case SECT_NILAZ_PLANE:
            continue;
        }

        if (!psi_skill_success(l->listener, NULL, PSI_HEAR, 0))
            continue;

        if (GET_GUILD(l->listener) != GUILD_PSIONICIST
            && GET_GUILD(l->listener) != GUILD_LIRATHU_TEMPLAR)
            send_to_char("Someone whispers to you:\n\r     \"", l->listener);
        else
            send_to_char("You hear through the Way:\n\r     \"", l->listener);

        cprintf(l->listener, "%s\"\n\r", messages[msg]);
    }
}

int
parse_psionic_command(CHAR_DATA * ch, const char *cmd)
{
    return parse_command_with_flags(ch, cmd, QFL_INSERT_HEAD | QFL_NO_STOP);
}

