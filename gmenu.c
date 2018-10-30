/*
 * File: GMENU.C
 * Usage: Character generation revising menu
 * Created by Morgenes for a more 'beautified' character generation system
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

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "constants.h"
#include "structs.h"
#include "comm.h"
#include "cities.h"
#include "utils.h"
#include "modify.h"
#include "utility.h"
#include "creation.h"

#define STATE(d) ((d)->connected)

void show_race_selection(DESCRIPTOR_DATA * d);
void show_guild(DESCRIPTOR_DATA * d);

void
revise_char_menu(DESCRIPTOR_DATA * d, char *argument)
{
    CHAR_DATA *ch = d->character;
    char buf[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];

    if (!d->player_info || !d->character) {
        SEND_TO_Q("\n\rPress RETURN to continue.", d);
        STATE(d) = CON_WAIT_SLCT;
        return;
    }

    if (argument[0] == '\0') {
        print_revise_char_menu(d);
        return;
    }


    switch (LOWER(argument[0])) {
    case 'a':
        print_age_info(d);
        STATE(d) = CON_AGE;
        break;

    case 'b':

        if (IS_SET(d->player_info->flags, PINFO_VERBOSE_HELP)) {
            SEND_TO_Q("\n\r", d);
            SEND_TO_Q(background_info, d);
        }

        if (GET_RACE(d->character) == RACE_DESERT_ELF)
            SEND_TO_Q("To be approved, you _must_ include information about" " your tribe.\n\r\n\r",
                      d);
        if (GET_RACE(d->character) == RACE_DWARF)
            SEND_TO_Q("To be approved, you _must_ include information about" " your focus.\n\r\n\r",
                      d);

        if (d->character->background && strlen(d->character->background) > 0) {
            sprintf(buf, "Revise your Background, use '.c' to clear.\n\r");
            SEND_TO_Q(buf, d);
            string_append(d->character, &d->character->background, MAX_NOTE_LENGTH);
            STATE(d) = CON_AUTO_BACKGROUND_END;
        } else {
            if (d->character->background)
                d->character->background[0] = '\0';
            else
                d->character->background = strdup("");
            SEND_TO_Q("Enter your Background.\n\r", d);
            string_edit(d->character, &d->character->background, MAX_NOTE_LENGTH);
            STATE(d) = CON_AUTO_BACKGROUND_END;
        }
        break;

    case 'e':
        SEND_TO_Q("What is your character's sex? (M/F) ", d);
        STATE(d) = CON_QSEX;
        break;

    case 'g':
        show_guild(d);
        SEND_TO_Q("Please type the letter of your character's guild: ", d);
        STATE(d) = CON_QGUILD;
        break;

    case 'i':
        if (get_char_extra_desc_value(ch, "[SHOULDER_L_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[SHOULDER_L_LDESC]");
            rem_char_extra_desc_value(ch, "[SHOULDER_L_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        } else if (get_char_extra_desc_value(ch, "[SHOULDER_R_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[SHOULDER_R_LDESC]");
            rem_char_extra_desc_value(ch, "[SHOULDER_R_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        } else if (get_char_extra_desc_value(ch, "[FACE_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[FACE_LDESC]");
            rem_char_extra_desc_value(ch, "[FACE_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        } else if (get_char_extra_desc_value(ch, "[NECK_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[NECK_LDESC]");
            rem_char_extra_desc_value(ch, "[NECK_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        } else if (get_char_extra_desc_value(ch, "[THROAT_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[THROAT_LDESC]");
            rem_char_extra_desc_value(ch, "[THROAT_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        }

        SEND_TO_Q("Pick your character's origin.\n\r", d);

        if (d->character->player.race == RACE_DESERT_ELF) {
        SEND_TO_Q("g) Desert Elf tribal location.\n\r\n\r", d);
        } else if (d->character->player.race == RACE_ELF) {
        SEND_TO_Q("a) Allanak.\n\r"
                  "b) Tuluk.\n\r"
                  "c) Luir's Outpost.\n\r"
                  "d) Red Storm Village.\n\r"
                  "e) The Labyrinth.\n\r", d);
        } else if (d->character->player.race == RACE_GITH) {
        SEND_TO_Q("h) Gith Mesa Clan.\n\r\n\r", d);
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

    case 'j':
        if (d->character->player.info[1]) {
            SEND_TO_Q("Your old objective\n\r    ", d);
            SEND_TO_Q(d->character->player.info[1], d);
        }
        SEND_TO_Q("\n\r", d);
        SEND_TO_Q("Enter new objective, or nothing to abort change\n\r", d);
        STATE(d) = CON_OBJECTIVE;
        break;

    case 'l':
        if (get_char_extra_desc_value(ch, "[SHOULDER_L_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[SHOULDER_L_LDESC]");
            rem_char_extra_desc_value(ch, "[SHOULDER_L_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        } else if (get_char_extra_desc_value(ch, "[SHOULDER_R_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[SHOULDER_R_LDESC]");
            rem_char_extra_desc_value(ch, "[SHOULDER_R_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        } else if (get_char_extra_desc_value(ch, "[FACE_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[FACE_LDESC]");
            rem_char_extra_desc_value(ch, "[FACE_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        } else if (get_char_extra_desc_value(ch, "[NECK_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[NECK_LDESC]");
            rem_char_extra_desc_value(ch, "[NECK_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        } else if (get_char_extra_desc_value(ch, "[THROAT_LDESC]", buf3, sizeof(buf3))) {
            rem_char_extra_desc_value(ch, "[THROAT_LDESC]");
            rem_char_extra_desc_value(ch, "[THROAT_SDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_LDESC]");
            rem_char_extra_desc_value(ch, "[HANDS_SDESC]");
        }

        SEND_TO_Q("You can get your Gol Krathu citizen tattoo on your choice of:\n\r", d);
        SEND_TO_Q("a) Your neck.\n\r", d);
        SEND_TO_Q("b) Your throat.\n\r", d);
        SEND_TO_Q("c) Your face.\n\r", d);
        SEND_TO_Q("d) Your right shoulder.\n\r", d);
        SEND_TO_Q("e) Your left shoulder.\n\r\n\r", d);
        SEND_TO_Q("Where is your Gol Krathu tattoo?\n\r", d);
        STATE(d) = CON_TULUK_TATTOO;
        break;

    case 'm':
        if (IS_SET(d->player_info->flags, PINFO_VERBOSE_HELP)) {
            SEND_TO_Q("\n\r", d);
            SEND_TO_Q(desc_info, d);
        }

        if (ch->description && strlen(ch->description) > 0) {
            sprintf(buf, "Revise your Main Description, use '.c' to clear.\n\r");
            SEND_TO_Q(buf, d);
            string_append(d->character, &d->character->description, MAX_DESC);
            STATE(d) = CON_AUTO_DESC_END;
        } else {
            if (ch->description)
                ch->description[0] = '\0';
            else
                ch->description = strdup("");

            SEND_TO_Q("Enter your Main Description.\n\r", d);
            string_edit(d->character, &d->character->description, MAX_DESC);
            STATE(d) = CON_AUTO_DESC_END;
        }
        break;

    case 'n':

        if (IS_SET(d->player_info->flags, PINFO_VERBOSE_HELP)) {
            SEND_TO_Q(app_info, d);
        }
        SEND_TO_Q("\n\rBy what name dost thou wish to hail? ", d);

        STATE(d) = CON_NEWNAME;
        break;

    case 'o':
        print_option_menu(d);
        STATE(d) = CON_OPTIONS_MENU;
        break;

    case 'p':
        if (IS_SET(d->player_info->flags, PINFO_VERBOSE_HELP)) {
            SEND_TO_Q(stat_order_info, d);
        }

        SEND_TO_Q("\n\rEnter your attribute priority: ", d);
        STATE(d) = CON_ORDER_STATS;
        break;

    case 'r':
        show_race_selection(d);
        SEND_TO_Q("Please type the letter of your character's race: ", d);
        STATE(d) = CON_QRACE;
        break;

    case 's':
        if (IS_SET(d->player_info->flags, PINFO_VERBOSE_HELP)) {
            SEND_TO_Q("\n\r", d);
            SEND_TO_Q(short_descr_info, d);
        } else
            SEND_TO_Q("Enter your short description: ", d);

        if (ch->short_descr)
            ch->short_descr[0] = '\0';

        STATE(d) = CON_AUTO_SDESC;
        break;

    case 't':
        if (!ch->clan || GET_RACE(ch) != RACE_MUL) {
            send_to_char("Invalid option.\n\r", ch);
            return;
        }

        remove_clan(ch, ch->clan->clan);
        SEND_TO_Q("All muls are slaves, or ex-slaves.  Choose the city-state in\n\r"
                  "which you would like to be a slave of.  When you enter the\n\r"
                  "game, if you choose this city-state, you will start as a slave\n\r"
                  "there.  If you choose a different location, you will start as\n\r"
                  "a run-away slave.\n\r"
                  "\n\r  Current options are:\n\rA) Allanak\n\rB) Tuluk\n\r\n\r>", d);
        STATE(d) = CON_MUL_SLAVE_CHOICE;
        break;

    case 'u':
        if (ch->short_descr == NULL || ch->short_descr[0] == '\0') {
            send_to_char("You must enter a short description to submit an application.\n\r", ch);
            print_revise_char_menu(d);
            return;
        } else if (ch->name == NULL || ch->name[0] == '\0') {
            send_to_char("You must enter a name to submit an application.\n\r", ch);
            print_revise_char_menu(d);
            return;
        } else if (ch->description == NULL || ch->description[0] == '\0') {
            send_to_char("You must enter a main description to submit an application.\n\r", ch);
            print_revise_char_menu(d);
            return;
        } else if (ch->player.race == -1) {
            send_to_char("You must choose a race to submit an application.\n\r", ch);
            print_revise_char_menu(d);
            return;
        } else if (ch->player.sex == 0) {
            send_to_char("You must choose a sex to submit an application.\n\r", ch);
            print_revise_char_menu(d);
            return;
        } else if (GET_RACE(ch) == RACE_DWARF && ch->player.info[1] == '\0') {
            send_to_char("You must provide an objective to submit an application.\n\r", ch);
            print_revise_char_menu(d);
            return;
        }

        sprintf(buf,
                "\n\rWelcome, %s.  Your character will be approved "
                "(or not) within 24 hours.\n\r\n\r", GET_NAME(d->character));
        SEND_TO_Q(buf, d);
        SEND_TO_Q(end_app_info, d);
        // Reset their keywords to empty
        free(d->character->player.extkwds);
        d->character->player.extkwds = generate_keywords_from_string(MSTR(d->character, short_descr));

        save_char_waiting_auth(d->character);
        free_char(d->character);
        d->character = (struct char_data *) 0;

#ifdef USE_DOCS
        if (d->term == VT100)
            SEND_TO_Q(doc_menu, d);
        else
            SEND_TO_Q(doc_menu_no_ansi, d);
        d->connected = CON_DOC;
#else
        SEND_TO_Q("\n\rPress RETURN to continue.", d);
        STATE(d) = CON_DOC;

        /* 
         * show_main_menu( d );
         * STATE(d) = CON_SLCT;
         */
#endif
        break;

    case 'x':
        {
            SEND_TO_Q(" If you exit your character will be lost, perhaps you meant to choose S<U>bmit?\n\r", d);
            SEND_TO_Q("Are you sure you want to do this (y/n)?", d);
            STATE(d) = CON_WAIT_CONFIRM_EXIT;
            break;
        }

    default:
        send_to_char("Invalid Option, Try again:\n\r", ch);
        print_revise_char_menu(d);
        break;
    }
}



void
print_revise_char_menu(DESCRIPTOR_DATA * d)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];
    CHAR_DATA *ch = d->character;

    if (!ch) {
        SEND_TO_Q("\n\rPress RETURN to continue.", d);
        STATE(d) = CON_WAIT_SLCT;
        return;
    }

    sprintf(buf, "\n\rName: %-32s\n\r", ch->name);
    sprintf(buf2, "Race: %-15s  Gender: %-7s\n\r",
            ch->player.race == -1 ? "(none given)" : race[ch->player.race].name,
            ch->player.sex == SEX_FEMALE ? "Female" : ch->player.sex ==
            SEX_MALE ? "Male" : "Neuter");
    strcat(buf, buf2);

    sprintf(buf2, "Guild: %-15s ",
            ch->player.guild <= GUILD_NONE ? "(none given)" : guild[(int) ch->player.guild].name);
    strcat(buf, buf2);

    // Show subguild if it exits
    if (ch->player.sub_guild > 0) {
        sprintf(buf2, "Sub-guild: %-15s\n\r",
                ch->player.sub_guild <= SUB_GUILD_NONE ? "(none given)" : sub_guild[(int) ch->player.sub_guild].name);
        strcat(buf, buf2);
    } else                      // Terminate the Guild line when no Sub-guild is shown
        strcat(buf, "\n\r");

    if (ch->clan) {
        sprintf(buf2, "Tribe: %-15s\n\r", clan_table[ch->clan->clan].name);
        strcat(buf, buf2);
    }

    sprintf(buf2, "Height: %d (inches)  Weight: %d (ten-stone)  Age: %d\n\r", ch->player.height,
            ch->player.weight, ch->player.time.starting_age);
    strcat(buf, buf2);


    sprintf(buf2, "Origin City: %s\n\r", city[ch->player.hometown].name);
    strcat(buf, buf2);

    if (ch->player.hometown == CITY_TULUK) {
        char tatt_loc[12];

        if (get_char_extra_desc_value(ch, "[SHOULDER_L_LDESC]", buf3, sizeof(buf3))) strcpy(tatt_loc, "Left Shoulder");
        if (get_char_extra_desc_value(ch, "[SHOULDER_R_LDESC]", buf3, sizeof(buf3))) strcpy(tatt_loc, "Right Shoulder");
        if (get_char_extra_desc_value(ch, "[FACE_LDESC]", buf3, sizeof(buf3)))       strcpy(tatt_loc, "Face");
        if (get_char_extra_desc_value(ch, "[NECK_LDESC]", buf3, sizeof(buf3)))       strcpy(tatt_loc, "Neck");
        if (get_char_extra_desc_value(ch, "[THROAT_LDESC]", buf3, sizeof(buf3)))     strcpy(tatt_loc, "Throat");

        sprintf(buf2, "Tuluk Tattoo Location: %s\n\r", tatt_loc);
        strcat(buf, buf2);

    } 

    sprintf(buf2, "Short Description: %s\n\r", ch->short_descr == NULL
            || ch->short_descr[0] == '\0' ? "(none given)" : ch->short_descr);
    strcat(buf, buf2);

    sprintf(buf2, "Main Description:\n\r%s\n\r", ch->description == NULL
            || ch->description[0] == '\0' ? "(none given)" : ch->description);
    strcat(buf, buf2);

    if (GET_RACE(ch) == RACE_DWARF) {
        sprintf(buf2, "Objective:\n\r%s\n\r", ch->player.info[1]);
        strcat(buf, buf2);
    }

    if (get_char_extra_desc_value(ch, "[STAT_ORDER]", buf3, sizeof(buf3))) {
        sprintf(buf2, "Attribute Order: %s\n\r", buf3);
        strcat(buf, buf2);
    } else {
        strcat(buf, "Attribute Order: Random\n\r");
    }

    strcat(buf, "-----------------------------------------------------------------------\n\r");
    strcat(buf, "    <N>ame                         |" "     <S>hort Description\n\r");
    strcat(buf, "    <R>ace                         |     G<e>nder\n\r");
    strcat(buf, "    <G>uild                        |     <M>ain Description\n\r");
    strcat(buf, "    <B>ackground                   |     <A>ge\n\r");

    if (GET_RACE(ch) == RACE_DWARF)
        strcat(buf, "    Ob<J>ective\n\r");
    else if (GET_RACE(ch) == RACE_MUL)
        strcat(buf, "    <T>ribe\n\r");

    strcat(buf, "    <O>ptions                      |     Attribute <P>riority\n\r");
    strcat(buf, "    Or<I>gin                       |");

    if (ch->player.hometown == CITY_TULUK) {
        strcat(buf, "     Tu<L>uki Tattoo\n\r");
    } else {
        strcat(buf, "\n\r");
    }

    if (ch->short_descr == NULL || ch->short_descr[0] == '\0' || ch->description == NULL
        || ch->description[0] == '\0' || ch->name == NULL || ch->name[0] == '\0'
        || ch->player.race == -1 || ch->player.sex == 0) {
        strcat(buf, "    E<x>it to Main Menu\n\r");

        if (GET_RACE(ch) == RACE_DWARF)
            strcat(buf, "Change One Of (N, S, R, E, G, M, A, J, O, P, I, X): ");
        else if (GET_RACE(ch) == RACE_MUL)
            strcat(buf, "Change One Of (N, S, R, E, G, M, A, T, O, P, I, X): ");
        else if (ch->player.hometown == CITY_TULUK)
            strcat(buf, "Change One Of (N, S, R, E, G, M, A, T, O, P, I, L, X): ");
        else
            strcat(buf, "Change One Of (N, S, R, E, G, M, A, O, P, I, X): ");
    } else {
        strcat(buf, "    E<x>it to Main Menu            |     S<U>bmit Application\n\r");
        if (GET_RACE(ch) == RACE_DWARF)
            strcat(buf, "Change One Of (N, S, R, E, G, M, A, J, O, P, I, X, U): ");
        else if (GET_RACE(ch) == RACE_MUL)
            strcat(buf, "Change One Of (N, S, R, E, G, M, A, T, O, P, I, X, U): ");
        else if (ch->player.hometown == CITY_TULUK)
            strcat(buf, "Change One Of (N, S, R, E, G, M, A, T, O, P, I, L, U, X): ");
        else
            strcat(buf, "Change One Of (N, S, R, E, G, M, A, O, P, I, X, U): ");
    }
    send_to_char(buf, ch);
}



void
option_menu(DESCRIPTOR_DATA * d, char *argument)
{
    int len;

    if (argument[0] == '\0') {
        print_option_menu(d);
        return;
    }

    switch (LOWER(argument[0])) {
    case 'b':
        print_revise_char_menu(d);
        STATE(d) = CON_REVISE_CHAR_MENU;
        return;

    case 'e':
        SEND_TO_Q("\n\rEnter the character you want to end editing with: ", d);
        STATE(d) = CON_GET_EDIT_END;
        return;

    case 'i':
        if (IS_SET(d->player_info->flags, PINFO_VERBOSE_HELP)) {
            REMOVE_BIT(d->player_info->flags, PINFO_VERBOSE_HELP);
            SEND_TO_Q("\n\rInstructions Disabled.\n\r", d);
        } else {
            MUD_SET_BIT(d->player_info->flags, PINFO_VERBOSE_HELP);
            SEND_TO_Q("\n\rInstructions Enabled.\n\r", d);
        }
        break;

    case 'p':
        for (len = d->character ? GET_PAGELENGTH(d->character)
             : d->pagelength; len > 0; len--) {
            char buf[MAX_STRING_LENGTH];

            sprintf(buf, "%d\n\r", len);
            SEND_TO_Q(buf, d);
        }
        SEND_TO_Q("Enter the number at the top of your screen: ", d);
        STATE(d) = CON_GET_PAGE_LEN;
        return;

    case 'v':
        if (d->term == VT100) {
            d->term = 0;
            REMOVE_BIT(d->player_info->flags, PINFO_ANSI_VT100);
            SEND_TO_Q("\n\rANSI/VT-100 Terminal Emulation Disabled.\n\r", d);
        } else {
            d->term = VT100;
            MUD_SET_BIT(d->player_info->flags, PINFO_ANSI_VT100);
            SEND_TO_Q("\n\rANSI/VT-100 Terminal Emulation Enabled.\n\r", d);
        }
        save_player_info(d->player_info);
        break;

    default:
        SEND_TO_Q("\n\rInvalid Choice.  Try Again.\n\r", d);
        break;
    }

    print_option_menu(d);
    return;
}


void
print_option_menu(DESCRIPTOR_DATA * d)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    if (d->term)
        sprintf(buf, "\n\r\tANSI/VT-100 Enabled.\n\r");
    else
        sprintf(buf, "\n\r\tANSI/VT-100 Disabled.\n\r");

    if (IS_SET(d->player_info->flags, PINFO_VERBOSE_HELP))
        sprintf(buf2, "\tInstructions Enabled.\n\r");
    else
        sprintf(buf2, "\tInstructions Disabled.\n\r");
    strcat(buf, buf2);

    sprintf(buf2, "\tPage length is set to %d lines.\n\r",
            d->character ? GET_PAGELENGTH(d->character) : d->pagelength);
    strcat(buf, buf2);

    sprintf(buf2, "\tEnd editing character set to '%c'\n\r\n\r", d->player_info->edit_end);

    strcat(buf, buf2);

    strcat(buf, "-----------------------------------------------------------------------\n\r");
    strcat(buf, "    Toggle ANSI/<V>T-100 Emulation |     Toggle <I>nstructions\n\r");
    strcat(buf, "    Set <E>nd Editing Character    |     Set <P>age Length\n\r");
    strcat(buf, "    <B>ack to Character Generation\n\r");

    strcat(buf, "Choose one of (B, E, I, P, V): ");
    SEND_TO_Q(buf, d);
    return;
}

