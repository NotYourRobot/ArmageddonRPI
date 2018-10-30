/*
 * File: OTHER.C
 * Usage: Assorted gobbledygook.
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
#include <assert.h>

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
#include "vt100c.h"
#include "cities.h"
#include "wagon_save.h"
#include "creation.h"
#include "event.h"
#include "modify.h"
#include "object_list.h"
#include "watch.h"
#include "io_thread.h"
#include "psionics.h"
#include "info.h"

extern void sflag_to_obj(OBJ_DATA * obj, long sflag);
extern int ness_analyze_from_crafted_item(OBJ_DATA * obj, CHAR_DATA * ch);
extern void spell_repel(byte level, sh_int magick_type, CHAR_DATA * ch, CHAR_DATA * victim,
                        OBJ_DATA * obj);
extern bool ch_starts_falling_check(CHAR_DATA *ch);

struct hand_data
{
    char *name;
    int pos;
    int keyword;
};

const char * const bogus_skill_messages[] = {
	"poor", // 0
	"average",
	"good",
	"very good",
	"amazing",
	"exceptional",
	"incredible",
	"fantasmic",
	"orgasmic",
	"lame",
	"pitiful", // 10
	"worth a cry",
	"lolz",
	"wtf",
	"zomg",
	"suX0rz",
	"peachy keen",
	"super duper",
	"awesomeness",
	"terrible",
	"don't bother", // 20
	"no worries",
	"oblierating",
	"wicked crazy",
	"like ur mom",
	"total sissy",
	"god-like",
	"demonic",
	"bombastic",
	"infantile",
	"bees knees", // 30
	"jolly good",
	"epic",
	"total fail",
	"mamby pamby",
	"hopeless",
	"staggering",
	"ghastly",
	"hideous",
	"radical",
	"bodacious", // 40
	"tubular",
	"bogus"
};

void
cmd_slee(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    send_to_char("You must type 'sleep' to sleep.\n\r", ch);
    return;
}

/* If you add one here you must add one to mercy_description.
 * Failure will not only be sloppy, but your new option won't be listed */
const char * const mercy_settings[] = {
    "status",
    "off",
    "all",
    "kill",
    "flee",
    "\n"
};

const char * const mercy_description_off[] = {
    "Show status of mercy options (this message)",
    "Show no mercy!",
    "Merciful on all counts.",
    "Will automatically take the killing blow.",
    "Will attack fleeing victims.",
    "\n"
};


const char * const mercy_description_on[] = {
    "Show status of mercy options (this message)",
    "Show no mercy!",
    "Merciful on all counts.",
    "Won't automatically take the killing blow.",
    "Won't attack fleeing victims.",
    "\n"
};

void
mercy_status(CHAR_DATA * ch, int hideStatus)
{
    int new;

    for (new = hideStatus ? 3 : 0; (*mercy_settings[new] != '\n')
         && (*mercy_description_on[new] != '\n')
         && (*mercy_description_off[new] != '\n'); new++) {
        cprintf(ch, "%8s : ", mercy_settings[new]);
        if (IS_SET(ch->specials.mercy, (1 << (new - 3))))
            cprintf(ch, "%s", mercy_description_on[new]);
        else
            cprintf(ch, "%s", mercy_description_off[new]);
        cprintf(ch, "\n\r");
    }
}


void
cmd_mercy(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[256];
    int which, flag;

    argument = one_argument(argument, arg, sizeof(arg));

    if (!(*arg)) {
        cprintf(ch, "You must provide one of the following toggles:\n\r");
        mercy_status(ch, 0);
        return;
    }

    if (!strcmp(arg, "on") ) {
       strcpy(arg, "all");
    }

    which = search_block(arg, mercy_settings, 0);

    switch (which) {
        case 0:                /* Status */
            mercy_status(ch, 1);
            break;
        case 1:                /* Turn all off */
            ch->specials.mercy = 0;
            cprintf(ch, "You will show no mercy!\n\r");
            break;
        case 2:                /* Turn all on  */
            ch->specials.mercy = ~0;
            cprintf(ch, "You will be merciful on all counts.\n\r");
            break;
        case 3: /* kill */
        case 4: /* flee */
            flag = (1 << (which - 3));

            if (IS_SET(ch->specials.mercy, flag)) {
                REMOVE_BIT(ch->specials.mercy, flag);
                cprintf(ch, "You %s\n\r", 
                 lowercase(mercy_description_off[which]));
            } else {
                MUD_SET_BIT(ch->specials.mercy, flag);
                cprintf(ch, "You %s\n\r", 
                 lowercase(mercy_description_on[which]));
            }
            break;
        default:
            cprintf(ch, "Invalid choice.  Type mercy with no arguments to see choices.\n\r");
            break;
    }
}


/* Started 8/4/2001 by Nessalin
   Added so players can remove some state flags from items.
*/
/* 
   12/1/2001

   added "int cmd" to arguments, needs to be of standard format -Tene
*/
void
cmd_clean(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    char oldarg[MAX_STRING_LENGTH];
    char arg1[256];
    char arg2[256];
    OBJ_DATA *dirty_obj = NULL;
    CHAR_DATA *dirty_ch;
    int item_count = 0, counter = 0, stain_chance = 0;

    strcpy(oldarg, arg);

    arg = one_argument(arg, arg1, sizeof(arg1));

    if (!strcmp(arg1, "")) {
        send_to_char("Clean what?\n\r", ch);
        return;
    }

    if (!((dirty_ch = get_char_room_vis(ch, arg1)) || (!strcmp("self", arg1))))
        if (!(dirty_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
            if (!(dirty_obj = get_obj_equipped(ch, arg1, FALSE))) {
                send_to_char("You're not carrying anything like that.\n\r", ch);
                return;
            }

    arg = one_argument(arg, arg2, sizeof(arg2));

    //    if (!ch->specials.to_process) {
    //        int wait;
    //        char monitormsg[256];
    //
    //        act("You start cleaning.", FALSE, ch, NULL, NULL, TO_CHAR);
    //        act("$n starts cleaning.", FALSE, ch, NULL, NULL, TO_ROOM);
    //
    //        sprintf(monitormsg, "%s starts cleaning.\n\r", MSTR(ch, name));
    //        send_to_monitor(ch, monitormsg, MONITOR_CRAFTING);
    //
    //        wait = 5;
    //
    //        add_to_command_q(ch, oldarg, cmd, 1, 0);
    //
    //        return;
    //    }

    if (dirty_ch) {
        if (dirty_ch == ch) {
            act("You dust yourself off.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n dusts $mself off.", FALSE, ch, 0, 0, TO_NOTVICT);

            for (counter = 0; counter < MAX_WEAR; counter++)
                if ((ch->equipment[counter])
                    && (IS_SET(ch->equipment[counter]->obj_flags.state_flags, OST_DUSTY))) {
                    item_count++;
                    REMOVE_BIT(ch->equipment[counter]->obj_flags.state_flags, OST_DUSTY);
                }
            //          if( !IS_IMMORTAL( ch ) )
            //       WAIT_STATE(ch, 1*item_count);
        } else {
            act("$n dusts $N off.", FALSE, ch, 0, dirty_ch, TO_NOTVICT);
            act("You dust off $N.", FALSE, ch, 0, dirty_ch, TO_CHAR);
            act("$n dusts you off.", FALSE, ch, 0, dirty_ch, TO_VICT);

            for (counter = 0; counter < MAX_WEAR; counter++)
                if ((dirty_ch->equipment[counter])
                    && (IS_SET(dirty_ch->equipment[counter]->obj_flags.state_flags, OST_DUSTY))) {
                    item_count++;
                    REMOVE_BIT(dirty_ch->equipment[counter]->obj_flags.state_flags, OST_DUSTY);
                }
            //          if( !IS_IMMORTAL( ch ) )
            //         WAIT_STATE(ch, 1*item_count);
        }
        return;
    }

    if (dirty_obj) {
        if (!strcmp(arg2, "")) {
            if ((IS_SET(dirty_obj->obj_flags.state_flags, OST_DUSTY))
                || (IS_SET(dirty_obj->obj_flags.state_flags, OST_ASH))
                || (IS_SET(dirty_obj->obj_flags.state_flags, OST_BLOODY))
                || (IS_SET(dirty_obj->obj_flags.state_flags, OST_MUDCAKED))
                || (IS_SET(dirty_obj->obj_flags.state_flags, OST_SWEATY))) {
                send_to_char("That item can be cleaned of the following:\n\r", ch);
                if (IS_SET(dirty_obj->obj_flags.state_flags, OST_DUSTY))
                    send_to_char("   dust\n\r", ch);
                if (IS_SET(dirty_obj->obj_flags.state_flags, OST_BLOODY))
                    send_to_char("   blood\n\r", ch);
                if (IS_SET(dirty_obj->obj_flags.state_flags, OST_SWEATY))
                    send_to_char("   sweat\n\r", ch);
                if (IS_SET(dirty_obj->obj_flags.state_flags, OST_ASH))
                    send_to_char("   ash\n\r", ch);
                if (IS_SET(dirty_obj->obj_flags.state_flags, OST_MUDCAKED))
                    send_to_char("   mud\n\r", ch);
            } else
                send_to_char("It's as clean as you can make it.\n\r", ch);
            return;
        } else {
            if (!strcmp(arg2, "dust")) {
                if (!IS_SET(dirty_obj->obj_flags.state_flags, OST_DUSTY))
                    send_to_char("But it isn't dusty?\n\r", ch);
                else {
                    REMOVE_BIT(dirty_obj->obj_flags.state_flags, OST_DUSTY);
                    act("You brush the dust off of $p.", FALSE, ch, dirty_obj, 0, TO_CHAR);
                    act("$n brushes the dust off of $p.", FALSE, ch, dirty_obj, 0, TO_NOTVICT);
                    //                  if( !IS_IMMORTAL( ch ) )
                    //                 WAIT_STATE(ch, 1);
                }
            } else if (!strcmp(arg2, "blood")) {
                if (!IS_SET(dirty_obj->obj_flags.state_flags, OST_BLOODY))
                    send_to_char("But it isn't bloodied?\n\r", ch);
                else {
                    if ((dirty_obj->obj_flags.type == ITEM_WEAPON)
                        || (dirty_obj->obj_flags.type == ITEM_TREASURE)) {
                        act("You quickly wipe the blood off of $p.", FALSE, ch, dirty_obj, 0,
                            TO_CHAR);
                        act("$n quickly wipes the blood off of $p.", FALSE, ch, dirty_obj, 0,
                            TO_NOTVICT);
                        REMOVE_BIT(dirty_obj->obj_flags.state_flags, OST_BLOODY);
                        //                      if( !IS_IMMORTAL( ch ) )
                        //                         WAIT_STATE(ch, 1);
                    } else {
                        act("You work at getting the blood out of $p.", FALSE, ch, dirty_obj, 0,
                            TO_CHAR);
                        act("$n works at cleaning $p.", FALSE, ch, dirty_obj, 0, TO_NOTVICT);
                        if (number(1, 100) < 41 || IS_IMMORTAL(ch)) {
                            REMOVE_BIT(dirty_obj->obj_flags.state_flags, OST_BLOODY);
                            switch (dirty_obj->obj_flags.material) {
                            case MATERIAL_SILK:
                            case MATERIAL_TISSUE:
                                stain_chance = 75;
                                break;
                            case MATERIAL_CLOTH:
                                stain_chance = 30;
                                break;
                            case MATERIAL_SKIN:
                            case MATERIAL_LEATHER:
                            case MATERIAL_GRAFT:
                            case MATERIAL_GWOSHI:
                                stain_chance = 25;
                                break;
                            case MATERIAL_WOOD:
                            case MATERIAL_CHITIN:
                            case MATERIAL_BONE:
                            case MATERIAL_IVORY:
                            case MATERIAL_HORN:
                            case MATERIAL_DUSKHORN:
                                stain_chance = 20;
                                break;
                            case MATERIAL_OBSIDIAN:
                            case MATERIAL_METAL:
                            case MATERIAL_STONE:
                            case MATERIAL_GEM:
                            case MATERIAL_GLASS:
                            case MATERIAL_CERAMIC:
                            case MATERIAL_TORTOISESHELL:
                                stain_chance = 0;
                                break;
                            default:
                                stain_chance = 40;
                                break;
                            }

                            if (number(1, 100) < stain_chance && !IS_IMMORTAL(ch)) {
                                act("You get most of the blood out $p, but some"
                                    " of the stains remain.", FALSE, ch, dirty_obj, 0, TO_CHAR);
                                MUD_SET_BIT(dirty_obj->obj_flags.state_flags, OST_STAINED);
                            }
                        }
                        //                      if( !IS_IMMORTAL( ch ) )
                        //                         WAIT_STATE(ch, 4);
                    }
                }
            } else if (!strcmp(arg2, "sweat")) {
                if (!IS_SET(dirty_obj->obj_flags.state_flags, OST_SWEATY))
                    send_to_char("But it isn't sweat-stained?\n\r", ch);
                else {
                    if ((dirty_obj->obj_flags.type == ITEM_WEAPON)
                        || (dirty_obj->obj_flags.type == ITEM_TREASURE)) {
                        act("You quickly wipe the sweat off of $p.", FALSE, ch, dirty_obj, 0,
                            TO_CHAR);
                        act("$n quickly wipes the sweat off of $p.", FALSE, ch, dirty_obj, 0,
                            TO_NOTVICT);
                        REMOVE_BIT(dirty_obj->obj_flags.state_flags, OST_SWEATY);
                        //                      if( !IS_IMMORTAL( ch ) )
                        //                         WAIT_STATE(ch, 1);
                    } else {
                        act("You work at getting the sweat stains out of $p.", FALSE, ch, dirty_obj,
                            0, TO_CHAR);
                        act("$n works at cleaning $p.", FALSE, ch, dirty_obj, 0, TO_NOTVICT);
                        if (number(1, 100) < 61 || IS_IMMORTAL(ch))
                            REMOVE_BIT(dirty_obj->obj_flags.state_flags, OST_SWEATY);
                        {
                            switch (dirty_obj->obj_flags.material) {
                            case MATERIAL_SILK:
                            case MATERIAL_TISSUE:
                                stain_chance = 55;
                                break;
                            case MATERIAL_CLOTH:
                                stain_chance = 20;
                                break;
                            case MATERIAL_SKIN:
                            case MATERIAL_LEATHER:
                            case MATERIAL_GRAFT:
                            case MATERIAL_GWOSHI:
                                stain_chance = 15;
                                break;
                            case MATERIAL_WOOD:
                            case MATERIAL_CHITIN:
                            case MATERIAL_BONE:
                            case MATERIAL_IVORY:
                            case MATERIAL_HORN:
                            case MATERIAL_DUSKHORN:
                                stain_chance = 5;
                                break;
                            case MATERIAL_OBSIDIAN:
                            case MATERIAL_METAL:
                            case MATERIAL_STONE:
                            case MATERIAL_GEM:
                            case MATERIAL_GLASS:
                            case MATERIAL_CERAMIC:
                            case MATERIAL_TORTOISESHELL:
                                stain_chance = 0;
                                break;
                            default:
                                stain_chance = 40;
                                break;
                            }
                            if (number(1, 100) < stain_chance && !IS_IMMORTAL(ch)) {
                                act("You get most of the sweat stains out of $p, "
                                    "but some remain.", FALSE, ch, dirty_obj, 0, TO_CHAR);
                                MUD_SET_BIT(dirty_obj->obj_flags.state_flags, OST_STAINED);
                            }
                        }
                        //                      if( !IS_IMMORTAL( ch ) )
                        //                        WAIT_STATE(ch, 4);
                    }
                }
            } else if (!strcmp(arg2, "mud")) {
                if (!IS_SET(dirty_obj->obj_flags.state_flags, OST_MUDCAKED))
                    send_to_char("But it isn't mud-caked?\n\r", ch);
                else {
                    act("You quickly scrape the caked mud off of $p.", FALSE, ch, dirty_obj, 0,
                        TO_CHAR);
                    act("$n quickly scrapes the caked mud off of $p.", FALSE, ch, dirty_obj, 0,
                        TO_NOTVICT);
                    REMOVE_BIT(dirty_obj->obj_flags.state_flags, OST_MUDCAKED);
                    //              if (!IS_IMMORTAL(ch))
                    //                WAIT_STATE(ch, 1);
                }
            } else if (!strcmp(arg2, "ash")) {
                if (!IS_SET(dirty_obj->obj_flags.state_flags, OST_ASH))
                    send_to_char("But it isn't covered in ash?\n\r", ch);
                else {
                    act("You quickly dust the ash off of $p.", FALSE, ch, dirty_obj, 0, TO_CHAR);
                    act("$n quickly dusts the ash off of $p.", FALSE, ch, dirty_obj, 0, TO_NOTVICT);
                    REMOVE_BIT(dirty_obj->obj_flags.state_flags, OST_ASH);
                    //              if (!IS_IMMORTAL(ch))
                    //                WAIT_STATE(ch, 1);
                }
            } else {
                send_to_char("It doesn't need that kind of cleaning.\n\r", ch);
            }
        }
    }
}

void
change_language(CHAR_DATA * ch, char *lang)
{
    int new = 0, old, i;

    if (*lang) {
        old = GET_SPOKEN_LANGUAGE(ch);
        for (i = 0; i < (MAX_TONGUES); i++) {
            new = language_table[i].spoken;
            if (!strncasecmp(lang, skill_name[new], strlen(lang)))
                break;
        }
        if (i < (MAX_TONGUES)) {
            if ((ch->skills[new])
                && ((GET_RACE(ch) != RACE_MANTIS)
                    || ((GET_RACE(ch) == RACE_MANTIS) && (new == LANG_MANTIS)))) {
                if (old != new) {
                    cprintf(ch, "You begin speaking %s.\n\r", skill_name[new]);
                    ch->specials.language = i;
                } else
                    cprintf(ch, "You are already speaking %s.\n\r", skill_name[new]);
            } else
                send_to_char("You know nothing of speaking that " "language.\n\r", ch);
        } else
            send_to_char("You've never heard of that language.\n\r", ch);
    } else
        cprintf(ch, "You are currently speaking %s.\n\r", skill_name[GET_SPOKEN_LANGUAGE(ch)]);

    return;
}

void
change_accent(CHAR_DATA * ch, char *accent)
{
    int new = 0, old, i, acc, acc_count = 0;

    if (*accent) {
        old = GET_ACCENT(ch);
        for (i = 0; accent_table[i].skillnum != LANG_NO_ACCENT; i++) {
            new = accent_table[i].skillnum;
            if (!strncasecmp(accent, accent_table[i].name, strlen(accent))) {
                if (has_skill(ch, new) && ch->skills[new]->learned >= MIN_ACCENT_LEVEL) {
                    if (old != new) {
                        cprintf(ch, "You begin speaking with %s%s accent.\n\r",
                                indefinite_article(accent_table[i].name), accent_table[i].name);
                        ch->specials.accent = new;
                    } else
                        cprintf(ch, "Your accent is already %s.\n\r", accent_table[i].name);
                } else {
                    send_to_char("You know nothing about that accent.\n\r", ch);
                }
                return;         // Done, no need to continue iterating
            } else if (!strncasecmp(accent, "default", strlen(accent))
                       && new == default_accent(ch)) {
                if (has_skill(ch, new)) {
                    cprintf(ch, "You begin speaking with %s%s accent.\n\r",
                            indefinite_article(accent_table[i].name), accent_table[i].name);
                    ch->specials.accent = new;
                } else {
                    send_to_char("You know nothing about that accent.\n\r", ch);
                }
                return;
            }
        }                       // Finishing the loop means no match was found.
        send_to_char("You know nothing about that accent.\n\r", ch);
    } else {
        send_to_char("You know about the following accents:\n\r", ch);
        for (i = 0; accent_table[i].skillnum != LANG_NO_ACCENT; i++) {
            acc = accent_table[i].skillnum;
            if (has_skill(ch, acc) && ch->skills[acc]->learned >= MIN_ACCENT_LEVEL) {
                cprintf(ch, "   %c %s%s\n\r", acc == GET_ACCENT(ch) ? '*' : ' ',
                        accent_table[i].name, acc == default_accent(ch) ? " (default)" : "");
                acc_count++;
            }
        }
        if (!acc_count)
            send_to_char("     none\n\r", ch);
    }
}

void
cmd_change(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *one_obj, *two_obj;
    CHAR_DATA *beast = 0;
    CHAR_DATA *target;
    struct affected_type af;
    int old, new, which, pname;
    char out[256], new_ld[256];
    char buf[256];
    char buf1[256], buf2[256], buf3[256], tempname[20];
    char msg[MAX_STRING_LENGTH];
    const char * const chng_what[] = {
        "language",
        "hands",
        "shape",
        "ldesc",
        "objective",
        "opponent",
        "locdescs",
        "accent",
        "mood",
        "tdesc",
        "arena",
        "\n"
    };

    struct hand_data hands[] = {
        {"ep", EP, 12},
        {"es", ES, 13},
        {"etwo", ETWO, 16},
        {"primary", EP, 12},
        {"secondary", ES, 13},
        {"both", ETWO, 16},
        {"two", ETWO, 16},
        {"", 0, 0}
    };

    const char * const shape_data[] = {
        "bat",
        "wolf",
        "elemental",
        "weretembo",
        "werejackal",
        "\n"
    };

    const char * const player_name[] = {
        "samah",
        "marit",
        "salkah",
        "kanali",
        "\n"
    };

    memset(&af, 0, sizeof(af));

    argument = one_argument(argument, buf1, sizeof(buf1));

    if (*buf1) {
        which = search_block(buf1, chng_what, 0);
        switch (which) {
        case 0:                /* change language */
            argument = one_argument(argument, buf2, sizeof(buf2));
            change_language(ch, buf2);
            break;
        case 1:                /* change hands */
            if (GET_POS(ch) == POSITION_SLEEPING)
                return;

            if (is_in_command_q(ch)) {
                act("Your concentration falters...", FALSE, ch, 0, 0, TO_CHAR);
                rem_from_command_q(ch);
            }

            argument = two_arguments(argument, buf2, sizeof(buf2), buf3, sizeof(buf3));

            /* default to ep es if you just type 'change hands' */
            if (buf2[0] == '\0') {
                strcpy(buf2, "ep");
                strcpy(buf3, "es");
            }

            if (*buf2 && *buf3) {
                for (old = 0; hands[old].pos; old++)
                    if (!strnicmp(buf2, hands[old].name, strlen(buf2)))
                        break;
                for (new = 0; hands[new].pos; new++)
                    if (!strnicmp(buf3, hands[new].name, strlen(buf3)))
                        break;
                if (hands[old].pos && hands[new].pos) {
                    if (hands[old].pos != hands[new].pos) {
                        one_obj = ch->equipment[hands[old].pos];
                        two_obj = ch->equipment[hands[new].pos];
                        if (!one_obj) {
                            send_to_char("You don't have anything in that" " hand.\n\r", ch);
                            return;
                        }
                        if (IS_SET(one_obj->obj_flags.extra_flags, OFL_NO_ES)) {
                            send_to_char("You don't have enough free hands to do " "that.\n\r", ch);
                            return;
                        }

                        if (hands[new].pos == ETWO && ch->equipment[EP] && ch->equipment[ES]) {
                            send_to_char("You need both hands free to change to" " both hands.\n\r",
                                         ch);
                            return;
                        }

                        if (two_obj)
                            obj_to_char(unequip_char(ch, hands[new].pos), ch);
                        obj_to_char(unequip_char(ch, hands[old].pos), ch);
                        wear(ch, one_obj, hands[new].keyword, "", "");
                        if (ch->equipment[hands[new].pos] == one_obj) {
                            if (two_obj)
                                wear(ch, two_obj, hands[old].keyword, "", "");
                        } else {
                            /* oops, something went wrong--put it back */
                            obj_from_char(one_obj);
                            equip_char(ch, one_obj, hands[old].pos);
                            if (two_obj) {
                                obj_from_char(two_obj);
                                equip_char(ch, two_obj, hands[new].pos);
                            }
                        }
                    } else {
                        send_to_char("What would be the sense of that?\n\r", ch);
                    }
                } else {
                    send_to_char("What hand is that?\n\r", ch);
                }
            } else {
                send_to_char("You'll have to specify the hands to swap.\n\r", ch);
            }
            break;

        case 2:                /* case change shape */
            if (GET_POS(ch) == POSITION_SLEEPING)
                return;

            if (is_in_command_q(ch)) {
                act("Your concentration falters...", FALSE, ch, 0, 0, TO_CHAR);
                rem_from_command_q(ch);
            }

            argument = one_argument(argument, buf2, sizeof(buf2));
            if (*buf2) {
                if (!ch->desc || ch->desc->snoop.snooping) {
                    send_to_char("Take on a new form while snooping?", ch);
                    return;
                }
                if (GET_POS(ch) != POSITION_STANDING) {
                    send_to_char("You are not in a position to change your shape.", ch);
                    return;
                }
                strcpy(tempname, GET_NAME(ch));
                pname = search_block(tempname, player_name, 1);
                if ((!IS_IMMORTAL(ch)) && ((pname > 4) || pname < 0)) {
                    send_to_char("You cannot change that.\n\r", ch);
                    break;
                }

                else {
                    which = search_block(buf2, shape_data, 0);
                    switch (which) {
                    case 0:    /* bat */
                        if (!IS_IMMORTAL(ch)) {
                            send_to_char("You cannot change that.\n\r", ch);
                            break;
                        }
                        beast = read_mobile(1800, VIRTUAL);
                        break;
                    case 1:    /* wolf   */
                        if (!IS_IMMORTAL(ch)) {
                            send_to_char("You cannot change that.\n\r", ch);
                            break;
                        }
                        beast = read_mobile(1801, VIRTUAL);
                        break;
                    case 2:    /* elemental *//*  Begin Azroen Hack */
                        if (GET_RACE(ch) != RACE_SUB_ELEMENTAL) {
                            send_to_char("You cannot change that.\n\r", ch);
                            break;
                        }
                        /* use search block for \n terminated terminated 
                         * the 1 is for search exact. (ie mar would return 1) */
                        strcpy(tempname, GET_NAME(ch));
                        pname = search_block(tempname, player_name, 1);

                        switch (pname) {

                        case 0:        /* samah */
                            beast = read_mobile(1401, VIRTUAL);
                            break;
                        case 1:        /* marit */
                            beast = read_mobile(1402, VIRTUAL);
                            break;
                        case 2:        /* Salkah */
                            beast = read_mobile(1403, VIRTUAL);
                            break;
                        case 3:        /* Kanali */
                            beast = read_mobile(1404, VIRTUAL);
                            break;
                        default:       /* just in case */
                            switch (GET_GUILD(ch)) {
                            case GUILD_FIRE_ELEMENTALIST:
                                beast = read_mobile(1110, VIRTUAL);
                                break;
                            case GUILD_WATER_ELEMENTALIST:
                                beast = read_mobile(1130, VIRTUAL);
                                break;
                            case GUILD_STONE_ELEMENTALIST:
                                beast = read_mobile(1120, VIRTUAL);
                                break;
                            case GUILD_WIND_ELEMENTALIST:
                                beast = read_mobile(1140, VIRTUAL);
                                break;
                            default:
                                beast = read_mobile(1110, VIRTUAL);
                                break;
                            }   /* End switch(GET_GUILD(ch)) */
                            break;
                        }       /* End switch(pname) */
                        break;
                    case 3:    /* were-tembo */
                        strcpy(tempname, GET_NAME(ch));
                        pname = search_block(tempname, player_name, 1);
                        switch (pname) {
                        case 2:        /* Salkah */
                            beast = read_mobile(1403, VIRTUAL);
                            break;
                        case 3:        /* Kanali */
                            beast = read_mobile(1404, VIRTUAL);
                            break;
                        default:       /* just in case */
                            beast = read_mobile(1403, VIRTUAL);
                            break;
                        }       /* End switch(pname) */

                    default:
                        break;
                    }           /* End switch(which) */
                }               /* End Azroen Hack */
                if ((beast) && (ch->desc)) {
                    if (ch->desc->snoop.snoop_by) {
                        ch->desc->snoop.snoop_by->desc->snoop.snooping = 0;
                        ch->desc->snoop.snoop_by = 0;
                    }
                    char_to_room(beast, ch->in_room);
                    if (GET_RACE(ch) == RACE_SUB_ELEMENTAL) {   /* Elemental changing messages */
                        switch (GET_GUILD(ch)) {
                        case GUILD_FIRE_ELEMENTALIST:
                            act("Flames dance around you, as your form alters"
                                " and you become $N.", FALSE, ch, 0, beast, TO_CHAR);
                            act("Flames rise up around $n, and when they disappear"
                                " $N stands before you.", FALSE, ch, 0, beast, TO_ROOM);
                            break;
                        case GUILD_STONE_ELEMENTALIST:
                            act("Your flesh solidifies as your form changes into" " $N.", FALSE, ch,
                                0, beast, TO_CHAR);
                            act("$n's flesh begins to change and solidify, and soon"
                                " $N stands before you.", FALSE, ch, 0, beast, TO_ROOM);
                            break;
                        case GUILD_WATER_ELEMENTALIST:
                            act("Your body becomes liquid, as your form changes" " into $N.", FALSE,
                                ch, 0, beast, TO_CHAR);
                            act("$n's body turns to liquid, and $N stands before" " you.", FALSE,
                                ch, 0, beast, TO_ROOM);
                            break;
                        case GUILD_WIND_ELEMENTALIST:
                            act("Your body becomes transparant, and changes" " into $N.", FALSE, ch,
                                0, beast, TO_CHAR);
                            act("$n's body fades into transparancy, leaving $N"
                                " standing before you.", FALSE, ch, 0, beast, TO_ROOM);
                            break;
                        default:
                            act("Your form blurs as you become $N.", FALSE, ch, 0, beast, TO_CHAR);
                            act("$n blurs and becomes $N.", TRUE, ch, 0, beast, TO_ROOM);
                        }
                    } else {
                        act("Your form blurs as you become $N.", FALSE, ch, 0, beast, TO_CHAR);
                        act("$n blurs and becomes $N.", TRUE, ch, 0, beast, TO_ROOM);
                    }
                    if (!ch->desc->original)
                        ch->desc->original = ch;
                    ch->desc->character = beast;
                    beast->desc = ch->desc;
                    ch->desc = 0;
                    char_from_room(ch);
                    char_to_room(ch, get_room_num(0));
                    if (!affected_by_spell(beast, TYPE_SHAPE_CHANGE)) {
                        af.type = TYPE_SHAPE_CHANGE;
                        af.duration = 99;
                        af.modifier = 0;
                        af.location = 0;
                        af.bitvector = 0;

                        affect_to_char(beast, &af);
                    }
                }
            } else {
                send_to_char("What shape would you like to take?\n\r", ch);
            }
            break;

        case 3:                /* ldesc */
            if (!*(argument))
                send_to_char("What do you want to change your ldesc to?\n\r", ch);
            else if (!strncasecmp(argument, "none", sizeof(argument))) {
                WIPE_LDESC(ch);
                send_to_char("Reverting to code-generated ldesc.\n\r", ch);
            } else {
                if (change_ldesc(argument, ch)) {
                    sprintf(out, "%s %s\n\r", PERS(ch, ch), argument);
                    out[0] = UPPER(out[0]);
                    sprintf(new_ld, "Your new ldesc is:\n\r%s", out);
                    send_to_char(new_ld, ch);
                } else
                    send_to_char("That ldesc is too long.\n\r", ch);
            }
            break;

        case 4:                /* objective */
            if (!*(argument))
                send_to_char("What do you want to change your objective to?\n\r", ch);
            else {
                if (change_objective(argument, ch)) {
                    sprintf(new_ld, "Your new objective is:\n\r%s\n\r", ch->player.info[1]);
                    send_to_char(new_ld, ch);
                    cmd_save(ch, "", CMD_SAVE, 0);
                } else
                    send_to_char("That objective is too long.\n\r", ch);
            }
            break;

        case 5:                /* opponent */
            argument = one_argument(argument, buf2, sizeof(buf2));

            if (is_in_command_q(ch)) {
                act("Your concentration falters...", FALSE, ch, 0, 0, TO_CHAR);
                rem_from_command_q(ch);
            }

            if (!*buf2) {
                send_to_char("Change to fighting whom?\n\r", ch);
                break;
            }
            if (!ch->specials.fighting) {
                send_to_char("But you aren't fighting anyone!\n\r", ch);
                break;
            }
            target = get_char_room_vis(ch, buf2);

            if (!target) {
                send_to_char("That person is not around to fight.\n\r", ch);
                break;
            }
            if (target->specials.fighting != ch) {
                send_to_char("That person is not fighting you.\n\r", ch);
                break;
            }

            send_to_char("You begin looking for an opening to change your" " opponent.\n\r", ch);
            WAIT_STATE(ch, PULSE_VIOLENCE * 2);

            /* guarding/rescue check */
            if ((ch->specials.fighting->specials.guarding == target)
                && has_skill(ch->specials.fighting, SKILL_RESCUE)) {
                if (get_skill_percentage(ch->specials.fighting, SKILL_RESCUE) > number(1, 101)) {
                    act("You attempt to break away from fighting $N, but $E" " blocks your way.",
                        FALSE, ch, 0, ch->specials.fighting, TO_CHAR);
                    sprintf(msg,
                            "$n attempts to break away and charge %s, but" " you block $m off.",
                            MSTR(target, short_descr));
                    act(msg, FALSE, ch, 0, ch->specials.fighting, TO_VICT);
                    break;
                }
            }

            for (beast = ch->in_room->people; beast; beast = beast->next_in_room) {
                if (beast == ch) {
                    cprintf(ch, "You change your attack to %s.\n\r",
                     PERS(target, ch));
                } else {
                    if (beast == ch->specials.fighting) {
                        sprintf(msg, "%s stops fighting you", PERS(beast, ch));
                        send_to_char(msg, beast);
                        sprintf(msg, ", and begins fighting %s.\n\r", PERS(beast, target));
                        send_to_char(msg, beast);
                        if( trying_to_disengage(beast) ) {
                           stop_fighting(beast, ch);
                        }
                    } /* end if beast = ch->specials.fighting */
                    else if (beast == target) {
                        sprintf(msg, "%s stops fighting ", PERS(beast, ch));
                        send_to_char(msg, beast);
                        sprintf(msg, " %s, and begins fighting you.\n\r",
                                PERS(beast, ch->specials.fighting));
                        send_to_char(msg, beast);
                    } /* end else if beast == target */
                    else {
                        sprintf(msg, "%s stops fighting ", PERS(beast, ch));
                        send_to_char(msg, beast);
                        sprintf(msg, "%s.\n\r", PERS(beast, ch->specials.fighting));
                        send_to_char(msg, beast);

                        sprintf(msg, "%s begins fighting ", PERS(beast, ch));
                        send_to_char(msg, beast);
                        sprintf(msg, "%s.\n\r", PERS(beast, target));
                        send_to_char(msg, beast);
                    }
                }
            }                   /* end for statement */

            clear_disengage_request(ch);
            ch->specials.fighting = target;
            ch->specials.alt_fighting = 0;
            break;
        case 6:                /* change locdescs */
            {
                if (!IS_SET(ch->in_room->room_flags, RFL_SAFE)
                    && (GET_GUILD(ch) != GUILD_RANGER)) {
                    act("You must be in a quit-safe room to change your location descriptions.\n\r",
                        FALSE, ch, 0, 0, TO_CHAR);
                    return;
                } else if (!IS_SET(ch->in_room->room_flags, RFL_SAFE)
                           && GET_GUILD(ch) == GUILD_RANGER) {
                    if (ch->in_room->number == 1023) {
                        send_to_char
                            ("You are in the room for viewing arena games.\n\rIf you would like to leave, type 'leave'.\n\r",
                             ch);
                        return;
                    }
                    if (ch->in_room->number == 1012) {
                        send_to_char
                            ("You are already in the room for changing your location descriptions.\n\rIf you would like to leave, type 'leave'.\n\r",
                             ch);
                        return;
                    }
                    if (room_in_city(ch->in_room)) {
                        send_to_char
                            ("As a ranger you can only change your location descriptions in the wilds or quit-safe rooms, not in cities.\n\r",
                             ch);
                        return;
                    } else if (ch->in_room->sector_type == SECT_SILT) {
                        send_to_char
                            ("As a ranger you can only change your location descriptions in the wilds or quit-safe rooms, not in silt.\n\r",
                             ch);
                        return;
                    } else if (ch->in_room->sector_type == SECT_CITY) {
                        send_to_char
                            ("As a ranger you can only change your location descriptions in the wilds or quit-safe rooms, not in cities.\n\r",
                             ch);
                        return;
                    } else if (find_closest_hates(ch, 10)) {
                        send_to_char
                            ("You're too close to something that is hunting you to change your location descriptions.\n\r",
                             ch);
                        return;
                    }
                }
                if (GET_POS(ch) < POSITION_STANDING) {
                    send_to_char("You must be standing change your location descriptions.\n\r", ch);
                    return;
                }

                if (GET_POS(ch) == POSITION_FIGHTING) {
                    send_to_char("You cannot change your location descriptions while fighting.\n\r",
                                 ch);
                    return;
                }

                if IS_AFFECTED
                    (ch, CHAR_AFF_SUBDUED) {
                    send_to_char("You cannot change your location descriptions while subdued.\n\r",
                                 ch);
                    return;
                    }

                if IS_AFFECTED
                    (ch, CHAR_AFF_NOQUIT) {
                    send_to_char("You too excited to change your location descriptions.\n\r", ch);
                    return;
                    }

                if (is_in_command_q(ch)) {
                    act("Your concentration falters...", FALSE, ch, 0, 0, TO_CHAR);
                    rem_from_command_q(ch);
                }

                sprintf(buf, "%d", ch->in_room->number);
                set_char_extra_desc_value(ch, "[LOC_DESC_RETURN_ROOM_VALUE]", buf);

                act("$n has departed from the land of Zalanthas.", TRUE, ch, 0, 0, TO_ROOM);
                char_from_room(ch);
                char_to_room(ch, get_room_num(1012));

                send_to_char
                    ("You are being moved to change your location descriptions, type 'leave' when you are done.\n\r",
                     ch);
                parse_command(ch, "look room");
                break;
            }

        case 7:                /* change accent */
            argument = one_argument(argument, buf2, sizeof(buf2));
            change_accent(ch, buf2);
            break;
        case 8:                /* change mood */
            if( GET_POS(ch) <= POSITION_STUNNED
             || affected_by_spell(ch, SPELL_FURY)
             || affected_by_spell(ch, AFF_MUL_RAGE)
             || affected_by_spell(ch, SPELL_CALM)
             || affected_by_spell(ch, SPELL_FEAR)
             || ch->specials.fighting) {
                cprintf(ch, "You can't control your mood right now.\n\r");
                return;
            }
 
            // no mood given
            if( argument[0] == '\0') {
                if( get_char_mood(ch, buf2, sizeof(buf2))) {
                    cprintf(ch, "Resetting your mood to a neutral state.\n\r");
                    change_mood(ch, NULL);
                } else {
                    cprintf(ch, 
                     "What do you want to change your mood to?.\n\r");
                }
                return;
            }

            // setting a mood
            strcpy( buf2, argument );
            change_mood(ch, buf2);
            cprintf( ch, "Your mood is now %s.\n\r", buf2 );
	    exhibit_emotion(ch, buf2);

            break;
        case 9:
            cmd_tdesc(ch, argument, CMD_TDESC, 0);
            break;
        case 10:                /* change arena */
        {
            if (lock_arena) {
               send_to_char("The arena games aren't open for viewing.\n\r", ch);
               return;
            }

            if (!IS_SET(ch->in_room->room_flags, RFL_SAFE)
             && (GET_GUILD(ch) != GUILD_RANGER)) {
                act("You must be in a quit-safe room to go view arena games.\n\r",
                 FALSE, ch, 0, 0, TO_CHAR);
                return;
            } else if (!IS_SET(ch->in_room->room_flags, RFL_SAFE)
                       && GET_GUILD(ch) == GUILD_RANGER) {
                if (ch->in_room->number == 1012) {
                    send_to_char("You are in the change locdescs room, you can't view arena games from here.\n\rIf you would like to leave, type 'leave'.\n\r", ch);
                    return;
                }

                if (ch->in_room->number == 1023) {
                    send_to_char("You are already in the arena viewing room.\n\rIf you would like to leave, type 'leave'.\n\r", ch);
                    return;
                }

                if (room_in_city(ch->in_room)) {
                    send_to_char ("As a ranger you can only go view arena games in the wilds or quit-safe rooms, not in cities.\n\r", ch);
                    return;
                } else if (ch->in_room->sector_type == SECT_SILT) {
                    send_to_char("As a ranger you can only go view arena games in the wilds or quit-safe rooms, not in silt.\n\r", ch);
                    return;
                } else if (ch->in_room->sector_type == SECT_CITY) {
                    send_to_char ("As a ranger you can only go view arena games in the wilds or quit-safe rooms, not in cities.\n\r", ch);
                    return;
                } else if (find_closest_hates(ch, 10)) {
                    send_to_char ("You're too close to something that is hunting you to go view arena games.\n\r", ch);
                    return;
                }
            }
            if (GET_POS(ch) < POSITION_STANDING) {
                send_to_char("You must be standing to go view arena games.\n\r", ch);
                return;
            }

            if (GET_POS(ch) == POSITION_FIGHTING) {
                send_to_char("You cannot view arena games while fighting.\n\r", ch);
                return;
            }

            if (IS_AFFECTED(ch, CHAR_AFF_SUBDUED)) {
                send_to_char("You cannot view arena games while subdued.\n\r", ch);
                return;
            }

            if (IS_AFFECTED(ch, CHAR_AFF_NOQUIT)) {
                send_to_char("You too excited to view arena games.\n\r", ch);
                return;
            }

            if (is_in_command_q(ch)) {
                act("Your concentration falters...", FALSE, ch, 0, 0, TO_CHAR);
                rem_from_command_q(ch);
            }

            sprintf(buf, "%d", ch->in_room->number);
            set_char_extra_desc_value(ch, "[ARENA_RETURN_ROOM_VALUE]", buf);

            act("$n has departed from the land of Zalanthas.", TRUE, ch, 0, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, get_room_num(1023));
            if (!IS_NPC(ch)) {
                gamelogf("%s (%s) is viewing the arena games.", ch->name, ch->account);
            }

            send_to_char("You are being moved to view arena games, type 'leave' when you are done.\n\r", ch);
            parse_command(ch, "look room");
            break;
        }
        default:
            send_to_char("You cannot change that.\n\r", ch);
            break;
        }
    } else {                    /* no argument was given */
        send_to_char("You can use \"change\" with the following:\n\r", ch);

        /*      for (new = 0; *chng_what[new] != '\n'; new++) {
         * sprintf(buffer, "     %s\n\r", chng_what[new]);
         * send_to_char(buffer, ch);   */

        send_to_char("     language\n\r", ch);
        send_to_char("     hands\n\r", ch);
        if ((IS_IMMORTAL(ch)) || (GET_RACE(ch) == RACE_SUB_ELEMENTAL))
            send_to_char("     shape\n\r", ch);
        send_to_char("     ldesc\n\r", ch);
        send_to_char("     objective\n\r", ch);
        send_to_char("     opponent\n\r", ch);
        //      if (IS_IMMORTAL(ch))
        send_to_char("     locdescs\n\r", ch);
        send_to_char("     accent\n\r", ch);
        send_to_char("     mood\n\r", ch);
        send_to_char("     tdesc\n\r", ch);

        /*  }  */

    }
}


void
cmd_analyze(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *object;
    char arg[MAX_INPUT_LENGTH];

    if (!has_skill(ch, SKILL_ANALYZE)) {
        send_to_char("What?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg, sizeof(arg));

    object = get_obj_in_list_vis(ch, arg, ch->carrying);

    if (!object) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    /* The [MULTIPLE] keyword is for items that can be made
     * from a variety of things, and therefore should not be
     * analyzable */
    if (isnamebracketsok("[multiple]", OSTR(object, name))) {
        send_to_char("This item's origin is indiscernable.\n\r", ch);
        return;
    }

    if (obj_index[object->nr].vnum == 0) {
        send_to_char("You can't tell how that item was made.\n\r", ch);
        return;
    }

    if (object->obj_flags.type == ITEM_CURE) {
        act("You cannot tell how $p was made.", TRUE, ch, object, 0, TO_CHAR);
    }

    ness_analyze_from_crafted_item(object, ch);

    return;
}

void
cmd_bite(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int hits;
    CHAR_DATA *person;
    char buf[256];
    struct affected_type *tmp_af;

    if (!(*argument)) {
        send_to_char("Bite who?\n\r", ch);
        return;
    }

    argument = one_argument(argument, buf, sizeof(buf));

    if ((person = get_char_room_vis(ch, buf))) {
        if (GET_RACE(ch) != RACE_VAMPIRE) {
            act("$n reaches out and bites you teasingly.", FALSE, ch, 0, person, TO_VICT);
            act("You reach out and bite $N teasingly.", FALSE, ch, 0, person, TO_CHAR);
            act("$n reaches out and bites $N teasingly.", TRUE, ch, 0, person, TO_NOTVICT);
        } else if (ch == person) {
            act("Bite yourself?.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        } else {
            if (IS_IMMORTAL(person)) {
                act("Go bite yourself!", FALSE, ch, 0, 0, TO_CHAR);
                return;
            }

            /* The vampir's offense + agil + 1-100 vs
             * the victim's defense + agil + 1-100 */
            hits =
                (ch->abilities.off + GET_AGL(ch) + number(1, 100)) >=
                (person->abilities.def + GET_AGL(ch) + number(1, 100));

            /* The person loses automatically if they're sitting 
             * or no-saved, or can't see the vampire, or subdued */
            if (!AWAKE(person) || is_paralyzed(person)
                || (IS_SET(person->specials.nosave, NOSAVE_SKILLS))
                || (!CAN_SEE(person, ch))
                || (IS_AFFECTED(person, CHAR_AFF_SUBDUED)))
                hits = 1;

            if (hits) {
                sprintf(buf, "%s drains blood from %s.\n\r", MSTR(ch, name), MSTR(person, name));
                send_to_monitors(ch, person, buf, MONITOR_OTHER);

                /* block if target has invulnerability */
                if (affected_by_spell(person, SPELL_INVULNERABILITY)) {
                    act("$n lunges at you with sharp teeth, trying to bite you.", FALSE, ch, 0,
                        person, TO_VICT);
                    act("You savagely lunge at $N, trying to sink your teeth into them.", FALSE, ch,
                        0, person, TO_CHAR);
                    act("$n savagely lunges at $N, sharp teeth bared in an attack.", TRUE, ch, 0,
                        person, TO_NOTVICT);
                    act("A cream-colored shell around $N flashes then fades away, blocking your attack.", FALSE, ch, 0, person, TO_CHAR);
                    act("The cream-colored shell around you flashes then fades away.", FALSE, ch, 0,
                        person, TO_VICT);
                    act("A cream-colored shell around $N flashes then fades away.", FALSE, ch, 0,
                        person, TO_NOTVICT);
                    affect_from_char(person, SPELL_INVULNERABILITY);
                    if (IS_AFFECTED(person, CHAR_AFF_INVULNERABILITY))
                        REMOVE_BIT(person->specials.affected_by, CHAR_AFF_INVULNERABILITY);
                    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                    if (IS_NPC(person))
                        hit(person, ch, TYPE_UNDEFINED);
                    return;
                }

                /* 10% chance per power level of wind armor for vampire to get repelled */
                if (affected_by_spell(person, SPELL_WIND_ARMOR))
                    for (tmp_af = person->affected; tmp_af; tmp_af = tmp_af->next)
                        if (tmp_af->type == SPELL_WIND_ARMOR)
                            if (number(0, 100) < (tmp_af->power * 10)) {
                                act("$n lunges at you with sharp teeth, trying to bite you.", FALSE,
                                    ch, 0, person, TO_VICT);
                                act("You savagely lunge at %N, trying to sink your teeth into them.", FALSE, ch, 0, person, TO_CHAR);
                                act("$n savagely lunges at $N, sharp teeth bared in an attack.",
                                    TRUE, ch, 0, person, TO_NOTVICT);
                                act("The winds around $N howl loudly, pushing back your attack.",
                                    FALSE, ch, 0, person, TO_CHAR);
                                act("The winds around you howl loudly, pushing back the attack.",
                                    FALSE, ch, 0, person, TO_VICT);
                                act("The winds around $N howl loudly, pushing back the attack.",
                                    FALSE, ch, 0, person, TO_NOTVICT);
                                spell_repel(tmp_af->power, SPELL_TYPE_SPELL, person, ch, 0);
                                return;
                            }
                if (ch->in_room != person->in_room)
                    return;

                act("$n digs sharp teeth into your flesh, draining blood as $e savages you.", FALSE,
                    ch, 0, person, TO_VICT);
                act("You savagely dig your teeth into $N, draining $S blood.", FALSE, ch, 0, person,
                    TO_CHAR);
                act("$n digs sharp teeth into $N, tearing away flesh and sinew.", TRUE, ch, 0,
                    person, TO_NOTVICT);

                if ((GET_RACE_TYPE(person) != RTYPE_HUMANOID)
                    || (IS_SET(person->specials.act, CFL_UNDEAD))
                    || (GET_RACE(person) == RACE_VAMPIRE)
                    || (GET_RACE(person) == RACE_DEMON)
                    || (GET_RACE(person) == RACE_GOLEM)
                    || (GET_RACE(person) == RACE_XILL)
                    || (GET_RACE(person) == RACE_DJINN)
                    || (GET_RACE(person) == RACE_ZOMBIE)
                    || (GET_RACE(person) == RACE_BANSHEE)
                    || (GET_RACE(person) == RACE_FIEND)
                    || (GET_RACE(person) == RACE_USELLIAM)
                    || (GET_RACE(person) == RACE_BEAST)
                    || (GET_RACE(person) == RACE_SHADOW)
                    || (GET_RACE(person) == RACE_GHOUL)
                    || (GET_RACE(person) == RACE_MEPHIT)) {
                    gain_condition(ch, THIRST, 3);
                    gain_condition(ch, FULL, 3);
                    adjust_hit(ch, 1);
                    adjust_move(ch, 2);
                    act("$N's blood seems bland and foreign to you, providing minimal satisfaction.", FALSE, ch, 0, person, TO_CHAR);
                } else {
                    gain_condition(ch, THIRST, 20);
                    gain_condition(ch, FULL, 20);
                    adjust_hit(ch, 10);
                    adjust_move(ch, 15);
                }

                if (affected_by_spell(person, SPELL_ENERGY_SHIELD)) {
                    act("$n shudders as a jolt of energy from $N enters $s body.", FALSE, ch, 0,
                        person, TO_ROOM);
                    act("A bolt of energy arcs from $N, striking you!", TRUE, ch, 0, person,
                        TO_CHAR);
                    act("The energy shield around you arcs into $n, and $e shudders.", TRUE, ch, 0,
                        person, TO_VICT);
                    do_damage(ch, 10, 50);
                } else if (affected_by_spell(person, SPELL_FIRE_ARMOR)) {
                    act("$n recoils in pain as the flames around $N burn $s face.", FALSE, ch, 0,
                        person, TO_ROOM);
                    act("The flames surrounding $N burn your face badly!", TRUE, ch, 0, person,
                        TO_CHAR);
                    act("The flames surrounding you burn $n in the face.", TRUE, ch, 0, person,
                        TO_VICT);
                    do_damage(ch, 10, 50);
                }

                do_damage(person, 10, 50);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            } else {
                sprintf(buf, "%s tries to drain blood from %s and fails.\n\r", MSTR(ch, name),
                        MSTR(person, name));
                send_to_monitors(ch, person, buf, MONITOR_OTHER);
                act("You try to bite $N but $E dodges out of your reach.", FALSE, ch, 0, person,
                    TO_CHAR);
                act("$n tries to bite you, but you nimbly avoid $m.", FALSE, ch, 0, person,
                    TO_VICT);
                act("$n tries to bite $N but misses completely.", TRUE, ch, 0, person, TO_NOTVICT);
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
                if (IS_NPC(person))
                    hit(person, ch, TYPE_UNDEFINED);
            }
        }
    } else
        send_to_char("That person isn't around for you to bite.\n\r", ch);
}

void
cmd_touch(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int hits;
    CHAR_DATA *person;
    char buf[256];

    if (!(*argument)) {
        send_to_char("Touch who?\n\r", ch);
        return;
    }

    argument = one_argument(argument, buf, sizeof(buf));
    if ((person = get_char_room_vis(ch, buf))) {
        hits =
            (ch->abilities.off + GET_AGL(ch) + number(1, 100)) >=
            (person->abilities.def + GET_AGL(ch) + number(1, 100));
        if (GET_POS(person) <= POSITION_SITTING)
            hits = 1;
        if (IS_SET(person->specials.nosave, NOSAVE_SKILLS))
            hits = 1;
        if (!CAN_SEE(person, ch))
            hits = 1;

        if (hits) {
            act("$n reaches out and touches you.", FALSE, ch, 0, person, TO_VICT);
            act("You reach out and touch $N.", FALSE, ch, 0, person, TO_CHAR);
            act("$n reaches out and touches $N.", TRUE, ch, 0, person, TO_NOTVICT);
        } else {
            act("You try to touch $N but $E dodges out of your reach.", FALSE, ch, 0, person,
                TO_CHAR);
            act("$n tries to touch you, but you nimbly avoid $m.", FALSE, ch, 0, person, TO_VICT);
            act("$n tries to touch $N but misses completely.", TRUE, ch, 0, person, TO_NOTVICT);
        }
    } else
        send_to_char("That person isn't around for you to touch.\n\r", ch);
}


void
cmd_tax(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *person;
    char buf1[256], buf2[256];
    int amount;

    if (!*argument) {
        send_to_char("Usage: tax <player> <amount>\n\r", ch);
        return;
    }

    argument = one_argument(argument, buf1, sizeof(buf1));
    if ((person = get_char(buf1))) {
        argument = one_argument(argument, buf2, sizeof(buf2));
        amount = atoi(buf2);
        if ((amount >= 0) && (amount <= person->points.in_bank)) {
            person->points.in_bank -= amount;
            send_to_char("Done.\n\r", ch);
            return;
        }
        send_to_char("That is an invalid tax amount.\n\r", ch);
    }
    send_to_char("No such person in the world to tax.\n\r", ch);
}

/* If you add one here you must add one to nosave_description.
 * Failure will not only be sloppy, but your new option won't be listed */
const char * const nosave_settings[] = {
    "status",
    "off",
    "all",
    "arrest",
    "climb",
    "magick",
    "psionics",
    "skills",
    "subdue",
    "theft",
    "combat",
    "\n"
};

const char * const nosave_description_off[] = {
    "Show status of nosave options (this message)",
    "Attempt all saving throws.",
    "Fail all saving throws.",
    "Resisting arrest.",
    "Not intentionally failing at climbing.",
    "Resisting magickal spells.",
    "Resisting psionic abilities.",
    "Resisting mundane skills.",
    "Resisting subdue attempts.",
    "Resisting theft attempts.",
    "Attacking back when attacked.",
    "\n"
};

const char * const nosave_description_on[] = {
    "Show status of nosave options (this message)",
    "Attempt all saving throws.",
    "Fail all saving throws.",
    "Not resisting arrest.",
    "Intentionally failing at climbing.",
    "Not saving against magickal spells.",
    "Not saving against psionic abilities.",
    "Not saving against mundane skills.",
    "Not saving against subdue attempts.",
    "Not saving against theft attempts.",
    "Not attacking back when attacked.",
    "\n"
};

void
nosave_status(CHAR_DATA * ch, int hideStatus)
{
    int new;

    for (new = hideStatus ? 3 : 0; (*nosave_settings[new] != '\n')
         && (*nosave_description_on[new] != '\n')
         && (*nosave_description_off[new] != '\n'); new++) {
        cprintf(ch, "%8s : ", nosave_settings[new]);
        if (IS_SET(ch->specials.nosave, (1 << (new - 3))))
            cprintf(ch, "%s", nosave_description_on[new]);
        else
            cprintf(ch, "%s", nosave_description_off[new]);
        cprintf(ch, "\n\r");
    }
}

void
cmd_nosave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[256];
    int which, flag;

    argument = one_argument(argument, arg, sizeof(arg));

    if (!(*arg)) {
        cprintf(ch, "You must provide one of the following toggles:\n\r");
        nosave_status(ch, 0);
        return;
    }

    if (*arg) {
        which = search_block(arg, nosave_settings, 0);

        switch (which) {
        case 0:                /* Status */
            nosave_status(ch, 1);
            break;
        case 1:                /* Turn all off */
            ch->specials.nosave = 0;
            cprintf(ch, "You are now attempting all saving throws.\n\r");
            break;
        case 2:                /* Turn all on  */
            ch->specials.nosave = ~0;
            cprintf(ch, "You will fail every saving throw.\n\r");
            break;
        case 3: /* arrest */
        case 4: /* climb */
        case 5: /* magick */
        case 6: /* psionics */
        case 7: /* skills */
        case 8: /* subdue */
        case 9: /* theft */
        case 10: /* combat */
            flag = (1 << (which - 3));

            if (IS_SET(ch->specials.nosave, flag)) {
                REMOVE_BIT(ch->specials.nosave, flag);
                cprintf(ch, "You are %s\n\r", lowercase(nosave_description_off[which]));
            } else {
                MUD_SET_BIT(ch->specials.nosave, flag);
                cprintf(ch, "You are %s\n\r", lowercase(nosave_description_on[which]));
            }
            break;
        default:
            cprintf(ch, "Invalid choice.  Type nosave with no arguments to see choices.\n\r");
            break;
        }

        // Remove barrier from people who don't want psionic saves
        if (IS_SET(ch->specials.nosave, NOSAVE_PSIONICS) && affected_by_spell(ch, PSI_BARRIER)) {
            cprintf(ch, "Removing your character's barrier.\n\r");
            affect_from_char(ch, PSI_BARRIER);
	}

        // Remove dome from people who don't want psionic saves
        if (IS_SET(ch->specials.nosave, NOSAVE_PSIONICS) && affected_by_spell(ch, PSI_DOME)) {
            cprintf(ch, "Removing your character's psionic dome.\n\r");
            affect_from_char(ch, PSI_DOME);
	}
        return;
    }
}


char * eval_prompt(char *pr, CHAR_DATA * ch);


void
cmd_prompt(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char prompt[512];
    snprintf(prompt, sizeof(prompt), "%s", argument);

    if (!*(prompt)) {
       if (ch->player.prompt
        && strcmp("(null)", ch->player.prompt)) {
          free(ch->last_prompt);
          ch->last_prompt = NULL;
       } else {
          send_to_char("What do you want your prompt to be?\n\r", ch);
       }
       return;
    }

    /* no real reason to do this anymore as players can specify newlines
     * to handle wrapping issues
     * if (strlen(prompt + 1) > 36) {
     *    prompt[36] = '\0';
     *    send_to_char("Truncated prompt to :\n\r\t", ch);
     *    send_to_char(prompt, ch);
     *    send_to_char("\n\r", ch);
     * } 
     */

    DESTROY(ch->player.prompt);
    if (!stricmp(prompt + 1, "none") || (!*(prompt + 1))) {
        ch->player.prompt = strdup(">");
    } else if (!stricmp(prompt + 1, "default")) {
        ch->player.prompt = NULL;
    } else {
        ch->player.prompt = strdup(prompt + 1);
    }
    return;
}

void
cmd_qui(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    if (cmd == 21)
        send_to_char("You have to write quit -- no less -- to quit.\n\r", ch);
    return;
}

void do_quit(CHAR_DATA *ch);

void
cmd_quit(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], arg1[256];
    bool pendingShutdown = heap_exists(0, event_shutdown_cmp);

    if (!ch->in_room)
        return;

    if ((IS_NPC(ch)) && (GET_RACE(ch) != RACE_SHADOW)) {
        act("$n has departed from the land of Zalanthas.", FALSE, ch, 0, 0, TO_ROOM);
        char_from_room(ch);
        char_to_room(ch, get_room_num(3900));
        send_to_char("Turning your back towards anyone who might be"
                     " watching, you tap your communicator and silently" " whisper:\n\r", ch);
        send_to_char("     \"Beam me up, Scotty!\"\n\r", ch);
        cmd_look(ch, "", 0, 0);
    }

    if (IS_NPC(ch) || !ch->desc)
        return;

    if (GET_POS(ch) == POSITION_FIGHTING) {
        send_to_char("No way! You are fighting.\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg1, sizeof(arg1));

    /*  Begin Kelvik quit-camp hack  */
    if (!stricmp(arg1, "test")) {
        if (pendingShutdown)
            cprintf(ch, "Yep, we're shutting down.  You can quit here.\n\r");

        if (GET_POS(ch) < POSITION_STUNNED) {
            send_to_char
                ("You are mortally wounded and will die if you quit!\n\rType 'quit die' to succumb to your wounds and die.\n\r",
                 ch);
            return;
        } else if (GET_POS(ch) < POSITION_RESTING) {
            send_to_char("Stand up first.\n\r", ch);
            return;
        } else if (IS_AFFECTED(ch, CHAR_AFF_NOQUIT)) {
            send_to_char("But you are too excited to leave just yet!\n\r", ch);
            return;
        } else if (!IS_SET(ch->in_room->room_flags, RFL_SAFE)
                   && (GET_GUILD(ch) != GUILD_RANGER)) {
            act("You are not in a safe room.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        } else if (IS_SET(ch->in_room->room_flags, RFL_SAFE)) {
            send_to_char("Yep.  You can quit here.\n\r", ch);
            return;
        }

        /*  Now they are a ranger and room is not 'safe'  */
        if (room_in_city(ch->in_room)) {
            send_to_char("But it's too noisy and busy to make camp here.\n\r", ch);
        } else if (ch->in_room->sector_type == SECT_CITY) {
            send_to_char("You better find a more secluded spot to camp.\n\r", ch);
        } else if (find_closest_hates(ch, 10)) {
            send_to_char("Looking around you decide this isn't a very "
                         "safe place to bed down.\n\r", ch);
        } else {
            send_to_char("This looks like a good place to make camp.\n\r", ch);
        }
        return;
    }
    /* Added per Sanvean's request -Morg 11/29/04 */
    else if (!stricmp(arg1, "die")
             && GET_POS(ch) < POSITION_STUNNED) {
        sprintf(buf, "%s %s%s%swas in room %d and quit while at negative hps, dying.", GET_NAME(ch),
                !IS_NPC(ch) ? "(" : "", !IS_NPC(ch) ? ch->account : "", !IS_NPC(ch) ? ") " : "",
                ch->in_room->number);

        if (!IS_NPC(ch)) {
            struct time_info_data playing_time = real_time_passed((time(0) - ch->player.time.logon)
                                                                  + ch->player.time.played, 0);

            if ((playing_time.day > 0 || playing_time.hours > 2
                 || (ch->player.dead && !strcmp(ch->player.dead, "rebirth")))
                && !IS_SET(ch->specials.act, CFL_NO_DEATH)) {
                if (ch->player.info[1])
                    free(ch->player.info[1]);
                ch->player.info[1] = strdup(buf);
            }

            death_log(buf);
        } else
            shhlog(buf);

        send_to_char("You die before your time!\n\r", ch);
        die(ch);
        return;
    } else if (!pendingShutdown) {
        if (!stricmp(arg1, "regardless")) {
            if IS_SET
                (ch->in_room->room_flags, RFL_SAFE) {
                send_to_char("Just type 'quit'.\n\r", ch);
                return;
                }
            act("Quit regardless has been disabled.  Read help quit.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        /*  End argument checking  */
        /* mortally wounded */
        if (GET_POS(ch) < POSITION_STUNNED) {
            send_to_char
                ("You are mortally wounded and will die if you quit!\n\rType 'quit die' to succumb to your wounds and die.\n\r",
                 ch);
            return;
        }
    }

    if (IS_AFFECTED(ch, CHAR_AFF_SUBDUED)) {    /*  Kel  */
        if (!pendingShutdown) {
            send_to_char("You cannot possibly leave while being held like this.\n\r", ch);
            return;
        } else {
            CHAR_DATA *tch;

            for (tch = ch->in_room->people; tch; tch = tch->next_in_room)
                if (tch->specials.subduing == ch)
                    break;

            if (tch) {
                tch->specials.subduing = (CHAR_DATA *) 0;
                act("$N releases $n so that $e can quit.", FALSE, ch, 0, tch, TO_NOTVICT);
                act("You release $n so that $e can quit.", FALSE, ch, 0, tch, TO_VICT);
                act("$N releases you so that you can quit.", FALSE, ch, 0, tch, TO_CHAR);
            }

            REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_SUBDUED);
        }
    }

    /* OOC quit, not dying or subdued */
    if (!pendingShutdown && !stricmp(arg1, "ooc")) {
        char character[MAX_STRING_LENGTH];
        char account[MAX_STRING_LENGTH];
        char reason[MAX_STRING_LENGTH];

        if( IS_SET(ch->quit_flags, QUIT_OOC_REVOKED)) {
            cprintf(ch, "You have lost the ability to quit ooc due to abuse.\n\rIf you have questions about this, contact staff through the request tool.\n\r");
            return;
        }

        if( IS_SET(ch->quit_flags, QUIT_OOC) ) {
            cprintf(ch, "You have already quit ooc.  You must quit safely before you can use\n\r'quit ooc' again.\n\r");
            return;
        }

        if( strlen(argument) < 5 ) { 
            cprintf(ch, "You must provide a valid reason why you need to quit ooc.\n\r");
            return;
        }

        /* Can't let them quit ooc in generated / removed rooms */
        if ( ch->in_room && ch->in_room->zone == RESERVED_Z ) {
             send_to_char("You may not quit ooc in this room due to code restrictions.\n\r", ch);
             return;
        }


        if (IS_AFFECTED(ch, CHAR_AFF_NOQUIT)) { 
        
        }
        MUD_SET_BIT(ch->quit_flags, QUIT_OOC);

        // Send a message to staff
        sprintf(buf, "/* %s (%s) has quit ooc because '%s'. */\n\r",
         GET_NAME(ch),
         ch->desc && ch->desc->player_info ? ch->desc->player_info->name
         : "(Unknown)", argument);
        connect_send_to_higher(buf, STORYTELLER, ch);

        do_quit(ch);
        return;
    }

    if (!pendingShutdown && !IS_IMMORTAL(ch)
     && IS_AFFECTED(ch, CHAR_AFF_NOQUIT)) { /*  Kel  */
        send_to_char("But you are too excited to leave just yet!\n\r", ch);
        return;
    }

    /* no no-quit for immortals, or safe checks */
    if (!IS_IMMORTAL(ch) && !pendingShutdown) {
        if (GET_POS(ch) < POSITION_RESTING) {   /* Needed this.. - Kelvik */
            send_to_char("Stand up first.\n\r", ch);
            return;
        }

        /*  Now just 'quit'  */
        if (!IS_SET(ch->in_room->room_flags, RFL_SAFE) && (GET_GUILD(ch) != GUILD_RANGER)) {
            act("You are not in a safe room.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        } else if (!IS_SET(ch->in_room->room_flags, RFL_SAFE) && GET_GUILD(ch) == GUILD_RANGER) {
            if (room_in_city(ch->in_room)) {
                send_to_char("But it's too noisy and busy to make camp " "here.\n\r", ch);
                return;
            } else if (ch->in_room->sector_type == SECT_SILT) {
                send_to_char("You better find a more stable spot to camp.\n\r", ch);
                return;
            } else if (ch->in_room->sector_type == SECT_CITY) {
                send_to_char("You better find a more secluded spot " "to camp.\n\r", ch);
                return;
            } else if (find_closest_hates(ch, 10)) {
                send_to_char("Looking around you decide this isn't a "
                             "very safe place to bed down.\n\r", ch);
                return;
            }
        }
    }

    // Safely quitting out, remove the QUIT_OOC flag
    REMOVE_BIT(ch->quit_flags, QUIT_OOC);
    do_quit(ch);
}

void
do_quit(CHAR_DATA *ch) {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf),
             "%s has left the world (Room: %d - %s), " "(HP: %d, Stun: %d), Guild: %s\n\r",
             GET_NAME(ch), ch->in_room->number, ch->in_room->name, GET_HIT(ch), GET_STUN(ch),
             guild[(int) ch->player.guild].name);
    send_to_monitor(ch, buf, MONITOR_OTHER);

    if (ch->desc) {
        MONITOR_LIST *pCl, *pCl_next;

        for (pCl = ch->desc->monitoring; pCl; pCl = pCl_next) {
            pCl_next = pCl->next;
            free(pCl);
        }
        ch->desc->monitoring = NULL;
        ch->desc->monitoring_all = FALSE;
        ch->desc->monitor_all_show = 0;
    }

    act("Come back soon!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n has departed from the land of Zalanthas.", TRUE, ch, 0, 0, TO_ROOM);

    sprintf(buf, "%s (%s) has left the world in room #%d (%s).", GET_NAME(ch), ch->desc
            && ch->desc->player_info ? ch->desc->player_info->name : "(Unknown)",
            ch->in_room->number, ch->in_room->name);
    shhlog(buf);
    sprintf(buf, "/* %s (%s) has left the world in room #%d (%s). */\n\r", GET_NAME(ch), ch->desc
            && ch->desc->player_info ? ch->desc->player_info->name : "(Unknown)",
            ch->in_room->number, ch->in_room->name);
    connect_send_to_higher(buf, STORYTELLER, ch);

    extract_char(ch);

    // in case the desc was lost (possess corpse, or other switch) -Morg
    if(ch->desc)
       ch->desc->connected = CON_SLCT;
}



void
cmd_save(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[100];

    if (IS_NPC(ch))
        return;

    if (!IS_CREATING(ch)) {
        sprintf(buf, "Saving %s.\n\r", GET_NAME(ch));
        send_to_char(buf, ch);
    }

    if (ch->in_room)
        save_char(ch);
#ifdef SAVE_CHAR_OBJS
    save_char_objs(ch);
#endif
}


void
cmd_not_here(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char("Sorry, you cannot do that here.\n\r", ch);
}

char weap_rtype_template[24][81] = {
    "Weapon per Race skills\n\r",
    "--------------------------------------------------------------------------\n\r",
    "       Humanoid   Insectine     Ophidian    Plant   Avian Flying   \n\r",
    "           | Mammalian | Flightless| Reptilian | Arachnid  |       \n\r",
    "           |     |     |     |     |     |     |     |     |       \n\r",
    "Hit     | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d |    \n\r",
    "Bludgeon| %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d |    \n\r",
    "Pierce  | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d |    \n\r",
    "Stab    | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d |    \n\r",
    "Chop    | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d |    \n\r",
    "Slash   | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d |    \n\r",
    "Hit     | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d |    \n\r",
    "Whip    | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d | %3d |    \n\r",

};

int
max_cap_for_skill(int sk) {
   int i;
   int max = 0;

   for (i = 1; i < MAX_GUILDS; i++) {
      if (guild[i].skill_max_learned[sk]
       && guild[i].skill_max_learned[sk] > max) {
         max = guild[i].skill_max_learned[sk];
      }
   }
   
   // If no guild gets it, use 100 as cap
   if (max == 0) max = 100;
   // otherwise bump it by 10
   else max += 10;

   return MIN(100, max);
}


void
list_skills(CHAR_DATA * ch, char *list, byte sk_class, bool immort, bool verbose, bool show_max)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int i, j, which = 0;
    bool found, is_accent;

    if ((sk_class == CLASS_SPICE) && (!immort))
        return;
    if ((sk_class == CLASS_TOLERANCE) && (!immort))
        return;

    if (immort) {
        if (sk_class == CLASS_COMBAT) {
            sprintf(buf, "Offense: %d (%d natural)\n\r", (int) (ch->tmpabilities.off),
                    (int) (ch->abilities.off));
            strcat(list, buf);
            sprintf(buf, "Defense: %d (%d natural)\n\r", (int) (ch->tmpabilities.def),
                    (int) (ch->abilities.def));
            strcat(list, buf);
        }
        if (sk_class == CLASS_MANIP) {
            sprintf(buf, "Willpower: %3d%%\n\r",
                    ((ch->skills[SKILL_WILLPOWER]) ? ch->skills[SKILL_WILLPOWER]->learned : 0));
            strcat(list, buf);
        }

        if (sk_class == CLASS_KNOWLEDGE) {
            int l, w;
            for (l = 0; l < 5; l++)
                strcat(list, weap_rtype_template[l]);

            for (w = 0; w < 8; w++, l++) {
                if (w != 6) {
                    sprintf(buf, weap_rtype_template[l],
                            ch->skills[skill_weap_rtype[0][w]] ? ch->
                            skills[skill_weap_rtype[0][w]]->learned : -1,
                            ch->skills[skill_weap_rtype[1][w]] ? ch->
                            skills[skill_weap_rtype[1][w]]->learned : -1,
                            ch->skills[skill_weap_rtype[2][w]] ? ch->
                            skills[skill_weap_rtype[2][w]]->learned : -1,
                            ch->skills[skill_weap_rtype[3][w]] ? ch->
                            skills[skill_weap_rtype[3][w]]->learned : -1,
                            ch->skills[skill_weap_rtype[4][w]] ? ch->
                            skills[skill_weap_rtype[4][w]]->learned : -1,
                            ch->skills[skill_weap_rtype[5][w]] ? ch->
                            skills[skill_weap_rtype[5][w]]->learned : -1,
                            ch->skills[skill_weap_rtype[6][w]] ? ch->
                            skills[skill_weap_rtype[6][w]]->learned : -1,
                            ch->skills[skill_weap_rtype[7][w]] ? ch->
                            skills[skill_weap_rtype[7][w]]->learned : -1,
                            ch->skills[skill_weap_rtype[8][w]] ? ch->
                            skills[skill_weap_rtype[8][w]]->learned : -1);

                    strcat(list, buf);
                };
            };
            /*last newline */
            sprintf(buf, "\n\r");
            strcat(list, buf);
        }
    }

    if (sk_class == CLASS_KNOWLEDGE) {
        return;
    };

    if (sk_class != CLASS_COMBAT) {
        for (i = 1, found = FALSE; (*skill_name[i] != '\n') && !found; i++)
            if ((ch->skills[i] && (skill[i].sk_class == sk_class))
                && (!is_skill_hidden(ch, i) || immort))
                found = TRUE;
        if (!found)
            return;
    }

    if (sk_class == CLASS_MAGICK)
        sprintf(buf, "Magickal spells\n\r");
    else if (sk_class == CLASS_PSIONICS)
        sprintf(buf, "Psionic powers\n\r");
    else if (sk_class == CLASS_REACH)
        sprintf(buf, "Known reaches\n\r");
    else
        sprintf(buf, "%s skills\n\r", class_name[(int) sk_class]);
    strcat(buf,
           "-----------------------------------------------------------------------------\n\r");
    strcat(list, buf);
    for (i = 1; (*skill_name[i] != '\n') && i < MAX_SKILLS; i++) {
        is_accent = FALSE;
        if (ch->skills[i] && (skill[i].sk_class == sk_class)) {
            if (!immort) {
                if (!is_skill_hidden(ch, i)) {
                    sprintf(buf, "%-23s", skill_name[i]);
                    strcat(list, buf);
                } else if (sk_class == CLASS_LANG) {
                    for (j = 0; accent_table[j].skillnum != LANG_NO_ACCENT; j++) {
                        if (accent_table[j].skillnum == i && ch->skills[i]->learned >= MIN_ACCENT_LEVEL) {
                            sprintf(buf2, "%s accent", accent_table[j].name);
	    		sprintf(buf, "%-23s", buf2);
                            strcat(list, buf);
	    		is_accent = TRUE;
			}
		    }
		}
	    } else {
                int raw, maxval;

                /* Duplicated because imm's lose it above */
                sprintf(buf, "%-23s", skill_name[i]);
                strcat(list, buf);
                raw = get_raw_skill(ch, i );
		maxval = get_innate_max_skill(ch, i);

                if( show_max) {
                    sprintf(buf, "%3d:%3d", ch->skills[i]->learned, maxval);
                    strcat(list, buf);
		} else if( ch->skills[i]->learned == raw ) {
                    sprintf(buf, "%3d    ", ch->skills[i]->learned);
                    strcat(list, buf);
                } else {
                    sprintf(buf, "%3d/%3d", ch->skills[i]->learned, raw);
                    strcat(list, buf);
                }
                sprintf(buf, " %c%c%c ",
                 is_skill_hidden(ch, i) ? 'h' : ' ', 
                 is_skill_taught(ch, i) ? 'l' : ' ',
                 is_skill_nogain(ch, i) ? 'n' : ' ');
                strcat(list, buf);
            }

            if (sk_class == CLASS_MAGICK && ch->skills[i]->rel_lev >= 0
                && ch->skills[i]->rel_lev < 7) {
                if (immort) 
                    sprintf(buf, "[%c]%s", 
                     toupper(Power[(int) ch->skills[i]->rel_lev][0]), 
                     !verbose && which ? "\n\r" : " ");
                else
                    sprintf(buf, "         [%-4s]%s", 
                     Power[(int) ch->skills[i]->rel_lev], 
                     !verbose && which ? "\n\r" : " ");
	    } else if (immort) {
                sprintf(buf, "   %s", !verbose && which ? "\n\r" : " ");
	    } else if (!IS_SET(ch->specials.brief, BRIEF_SKILLS) &&
			sk_class != CLASS_MAGICK &&
			sk_class != CLASS_REACH &&
                        !is_skill_hidden(ch, i) &&
                        !is_accent ) {
                int max_skill = max_cap_for_skill(i);
                int delimiter = max_skill / 5;
                int apt = get_raw_skill(ch, i) / delimiter;
		apt = MAX(0, MIN(apt, SKILL_LEVEL_RANGE_MAX));
		sprintf(buf2, "(%s)", how_good_skill[apt]);

		/* choose what message to show based on date */
		time_t tt = time(0);
		struct tm *tp = localtime(&tt);
		if (tp->tm_mon == 3 && tp->tm_mday == 1) {   /* April 1st */
		// Randomly give them one of the messages in the array.
			sprintf(buf2, "(%s)", bogus_skill_messages[number(0, sizeof(bogus_skill_messages) / sizeof(bogus_skill_messages[0]) - 1)]);
		}

                sprintf(buf, " %14s%s", buf2,
                 !verbose && which ? "\n\r" : " ");
	    } else 
                sprintf(buf, "%15s%s", "", !verbose && which ? "\n\r" : " ");

            if (!is_skill_hidden(ch, i) || immort || is_accent) {
                strcat(list, buf);
                which = 1 - which;
            }

            if( verbose ) {
                int wt = get_skill_wait_time(ch, i);
                time_t nexttime = ch->skills[i]->last_gain + (wt * SECS_PER_REAL_MIN);

                if( ch->skills[i]->last_gain ) {
                    sprintf( buf, "  Last Gain: %s\n\r%40s Can Gain At: %s\n\r", 
                     (char *) ctime(&ch->skills[i]->last_gain), "",
                     (char *) ctime(&nexttime));
                    strcat(list, buf);
                } else {
                    sprintf( buf, "Never gained!\n\r" );
                    strcat(list, buf);
                }
            }

        }
    }

    if (which)
        strcat(list, "\n\r\n\r");
    else
        strcat(list, "\n\r");
}



void
cmd_skills(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char list[MAX_STRING_LENGTH * 10], arg1[256];
    CHAR_DATA *view, *staff;
    int i;
    bool verbose = FALSE, show_max = FALSE;

    staff = SWITCHED_PLAYER(ch);
    view = ch;
    argument = one_argument(argument, arg1, sizeof(arg1));

    if (IS_IMMORTAL(staff) && GET_LEVEL(staff) > BUILDER) {
        if( !strcmp( arg1, "-v" ) ) {
           argument = one_argument(argument, arg1, sizeof(arg1));
           verbose = TRUE;
        } else if( !strcmp( arg1, "-m" ) ) {
           argument = one_argument(argument, arg1, sizeof(arg1));
           show_max = TRUE;
        }
        if ((view = get_char_vis(ch, arg1))) {
            argument = one_argument(argument, arg1, sizeof(arg1));
        } else {
            view = ch;
        }
    }

    if (*arg1 == '\0' || !stricmp(arg1, "all"))
        i = -1;
    else if (!str_prefix(arg1, "magick"))
        i = CLASS_MAGICK;
    else if (!str_prefix(arg1, "reach"))
        i = CLASS_REACH;
    else if (!str_prefix(arg1, "combat"))
        i = CLASS_COMBAT;
    else if (!str_prefix(arg1, "stealth"))
        i = CLASS_STEALTH;
    else if (!str_prefix(arg1, "manipulation"))
        i = CLASS_MANIP;
    else if (!str_prefix(arg1, "perception"))
        i = CLASS_PERCEP;
    else if (!str_prefix(arg1, "barter"))
        i = CLASS_BARTER;
    else if (!str_prefix(arg1, "language"))
        i = CLASS_LANG;
    else if (!str_prefix(arg1, "weapon"))
        i = CLASS_WEAPON;
    else if (!str_prefix(arg1, "psionic"))
        i = CLASS_PSIONICS;
    else if (!str_prefix(arg1, "knowledge"))
        i = CLASS_KNOWLEDGE;
    else if (!str_prefix(arg1, "craft"))
        i = CLASS_CRAFT;
    else if (!str_prefix(arg1, "tolerance"))
        i = CLASS_TOLERANCE;
    else if (IS_IMMORTAL(staff) && GET_LEVEL(staff) > BUILDER) {
        cprintf(ch, "Noone like '%s' in the world.\n\r", arg1);
        return;
    } else {
        send_to_char("There is no skill class of that name.\n\r", ch);
        return;
    }

    if (!i)
        return;

    *list = '\0';

    if (i == -1) {
        list_skills(view, list, CLASS_MAGICK, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_REACH, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_PSIONICS, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_COMBAT, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_WEAPON, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_STEALTH, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_MANIP, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_PERCEP, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_BARTER, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_LANG, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_SPICE, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_TOLERANCE, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_KNOWLEDGE, IS_IMMORTAL(staff), verbose, show_max);
        list_skills(view, list, CLASS_CRAFT, IS_IMMORTAL(staff), verbose, show_max);
    } else
        list_skills(view, list, i, IS_IMMORTAL(staff), verbose, show_max);

    if (ch->desc) {
        page_string(ch->desc, list, 1);
    }
}


void
error_report_helper(CHAR_DATA * ch, const char *argument, const char *reptype, int quietfl)
{
	send_to_char("Easy version does not have a database: no error reporting.\n", ch);
}


void
cmd_idea(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    error_report_helper(ch, argument, "Idea", QUIET_IDEA);
}


void
cmd_typo(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    error_report_helper(ch, argument, "Typo", QUIET_TYPO);
}


void
cmd_bug(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    error_report_helper(ch, argument, "Bug", QUIET_BUG);
}


void
cmd_brief(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[256];
    int new, which, flag;
    char buffer[256];
    char buf[256];

    /* If you add one here you must add one to brief_description.
     * Failure will not only be sloppy, but your new option won't be listed */
    const char * const brief_settings[] = {
        "off",
        "all",
        "combat",
        "room",
        "ooc",
        "names",
        "equip",
        "songs",
        "novice",
        "exits",
	"skills",
	"emote",
	"prompt",
        "\n"
    };

    char *brief_description[] = {
        "All brief settings off.",
        "Turn full brief on.",
        "Hide miss messages in combat.",
        "Hide room descriptions.",
        "Compact ooc into one line.",
        "Show character names instead of short descriptions.",
        "Show equipment as prose instead of lists.",
        "Show songs compacted.",
	"Hide display of helpful information for new players.",
	"Show exits compacted.",
	"Hide display of aptitudes in skills list.",
	"Hide display markers for hidden and silent emotes.",
	"Minimal prompt if nothing has changed.",
        "\n"
    };

    argument = one_argument(argument, arg, sizeof(arg));

    strcpy(buffer, "");

    if (!(*arg)) {
        send_to_char("You must provide one of the following toggles:\n\r", ch);
        for (new = 0; (*brief_settings[new] != '\n')
             && (*brief_description[new] != '\n'); new++) {
            if( (new == 5 || new == 6) && !IS_IMMORTAL(ch) ) continue;
            if (new - 1 > 0) {
                if (IS_SET(ch->specials.brief, (1 << (new - 2))))
                    sprintf(buffer, "[ON ] ");
                else
                    sprintf(buffer, "[OFF] ");
            } else
                sprintf(buffer, "      ");

            sprintf(buf, "%6s:", brief_settings[new]);
            strcat(buffer, buf);
            strcat(buffer, brief_description[new]);
            strcat(buffer, "\n\r");
            cprintf(ch, "%s", buffer);
        }
        return;
    }


    if (*arg) {
        which = search_block(arg, brief_settings, 0);

        switch (which) {
            /* case 0 and case 1 messages switched to match action  -Morgenes */
        case 0:                /* Turn all off */
            ch->specials.brief = 0;
            sprintf(buffer, "Brief mode off.\n\r");
            break;
        case 1:                /* Turn all on  */
            ch->specials.brief = ~0;
            if( !IS_IMMORTAL(ch) ) {
                REMOVE_BIT(ch->specials.brief, BRIEF_EQUIP);
                REMOVE_BIT(ch->specials.brief, BRIEF_STAFF_ONLY_NAMES);
            }
            sprintf(buffer, "Full brief mode on.\n\r");
            break;
        case 5:
        case 6:                /* Equip         */
            if( !IS_IMMORTAL(ch) ) {
                sprintf(buffer, "Invalid choice.  Type brief with no arguments to see choices.\n\r");
                break;
            }
        case 2:                /* Room          */
        case 3:                /* Combat        */
        case 4:                /* OOC           */
        case 7:                /* Songs         */
        case 8:                /* Novice        */
        case 9:                /* Exits         */
        case 10:               /* Skills        */
        case 11:               /* Emote         */
        case 12:               /* Prompt        */
            strcat(buffer, "Brief ");
            strcat(buffer, brief_settings[which]);
            strcat(buffer, " turned ");

            flag = (1 << (which - 2));

            if (IS_SET(ch->specials.brief, flag)) {
                REMOVE_BIT(ch->specials.brief, flag);
                strcat(buffer, "off");
            } else {
                MUD_SET_BIT(ch->specials.brief, flag);
                strcat(buffer, "on");
            }

            strcat(buffer, ".\n\r");
            break;
        default:
            sprintf(buffer, "Invalid choice.  Type brief with no arguments to see choices.\n\r");
            break;
        }
        send_to_char(buffer, ch);
        return;
    }
}


void
cmd_infobar(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    if (IS_SET(ch->specials.act, CFL_INFOBAR)) {
        send_to_char("Infobar off.\n\r", ch);
        REMOVE_BIT(ch->specials.act, CFL_INFOBAR);
    } else {
        send_to_char("Infobar on.\n\r", ch);
        MUD_SET_BIT(ch->specials.act, CFL_INFOBAR);
    }

    cmd_reset(ch, "", 0, 0);
}

void
cmd_compact(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->specials.act, CFL_COMPACT)) {
        send_to_char("You are now in the uncompacted mode.\n\r", ch);
        REMOVE_BIT(ch->specials.act, CFL_COMPACT);
    } else {
        send_to_char("You are now in compact mode.\n\r", ch);
        MUD_SET_BIT(ch->specials.act, CFL_COMPACT);
    }
}

/* Knocking on doors... */
void
cmd_knock(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    ROOM_DATA *tar_room;
    OBJ_DATA *tmp_obj;
    int door;
    char type[256];
    char dir[256];
    char buf[MAX_STRING_LENGTH];


    argument = two_arguments(argument, type, sizeof(type), dir, sizeof(dir));

    if (!*type) {
        send_to_char("Knock on what?\n\r", ch);
        return;
    }

    if ((door = find_door(ch, type, dir)) >= 0) {
        if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
            send_to_char("You amuse yourself by knocking on the wall.\n\r", ch);
        else {
            if (EXIT(ch, door)->keyword) {
                act("$n knocks on the $F.", FALSE, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
                act("Rap! Rap! Rap!  You knock on the $F.", FALSE, ch, 0, EXIT(ch, door)->keyword,
                    TO_CHAR);
            } else {
                act("$n knocks on the door.", FALSE, ch, 0, 0, TO_ROOM);
                act("Rap! Rap! Rap!  You knock on the door.", FALSE, ch, 0, 0, TO_CHAR);
            }

            if ((tar_room = ch->in_room->direction[door]->to_room)) {
                if (EXIT(ch, door)->keyword)
                    sprintf(buf, "%s knocks on the %s from the other side.\n\r",
                            IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) ? "Someone" : MSTR(ch,
                                                                                            short_descr),
                            EXIT(ch, door)->keyword);
                else
                    sprintf(buf, "%s knocks on the door from the other side\n\r",
                            IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) ? "Someone" : MSTR(ch,
                                                                                            short_descr));
                send_to_room(buf, tar_room);

                /* When knocking on any door in the 'entrance' of a wagon, the
                 * entire wagon should hear the knocking -Nessalin 2/16/97 */
                for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next)
                    if ((tmp_obj->obj_flags.type == ITEM_WAGON) && ((tmp_obj->obj_flags.value[2] > 0) &&        /* Make sure it's a 'wagon' 
                                                                                                                 * and not some form of camp */
                                                                    ((tmp_obj->obj_flags.value[0] ==
                                                                      ch->in_room->number)
                                                                     && (tmp_obj->obj_flags.
                                                                         value[0] > 0)))) {
                        sprintf(buf, "You hear someone knocking at the entrance.\n\r");
                        map_wagon_rooms(tmp_obj, (void *) room_to_send, (void *) buf);
                        break;  /* stop search, shouldn't be in more than one 'wagon' at a time */
                    }
            }
        }
    }
}


void
cmd_quaff(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[100];
    OBJ_DATA *temp;
    int i;
    int equipped;

    equipped = 0;

    argument = one_argument(argument, buf, sizeof(buf));

    if (!(temp = get_obj_in_list_vis(ch, buf, ch->carrying))) {
        if (!(temp = get_obj_equipped(ch, buf, FALSE))) {
            act("You do not have that item.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
        if (temp == ch->equipment[EP])
            equipped = EP;
        else if (temp == ch->equipment[ES])
            equipped = ES;
    }
    if (temp->obj_flags.type != ITEM_POTION) {
        act("You can only quaff potions.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    act("$n quaffs $p.", TRUE, ch, temp, 0, TO_ROOM);
    act("You quaff $p, which immediately dissolves.", FALSE, ch, temp, 0, TO_CHAR);

    for (i = 1; i < 4; i++)
        if ((temp->obj_flags.value[i] >= 1) && skill[temp->obj_flags.value[i]].function_pointer)
            (*skill[temp->obj_flags.value[i]].function_pointer)
                ((byte) temp->obj_flags.value[0], ch, "", SPELL_TYPE_POTION, ch, 0);


    if (equipped)
        unequip_char(ch, equipped);

    extract_obj(temp);
}

/* load a "crossbow" with a bolt */
void
cmd_load(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *bow;
    OBJ_DATA *bolt;
    OBJ_DATA *tmp;
    char arg1[256], arg2[256];
#ifdef RANGED_STAMINA_DRAIN
    int staminaCost = 0;
#endif

    if (!*argument) {
        send_to_char("Load what with what?\n\r", ch);
        return;
    }

    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    bow = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if (!bow) 
        bow = get_obj_equipped(ch, arg1, FALSE);

    bolt = get_obj_in_list_vis(ch, arg2, ch->carrying);
    if (!bolt)
        bolt = get_obj_equipped(ch, arg2, FALSE);

    if (!bow || !bolt) {
        send_to_char("You can't load a weapon you haven't got.\n\r", ch);
        return;
    }

    if (bow->obj_flags.type == ITEM_SCENT) {
        if (bow->obj_flags.value[1]) {
            send_to_char("You couldn't possibly load a thing like that.\n\r", ch);
            return;
        }
        switch (bolt->obj_flags.type) {
        case ITEM_CURE:
            if (bolt->obj_flags.value[1]) {
                sprintf(arg1, "%s won't fit in there.\n\r", OSTR(bolt, short_descr));
                send_to_char(arg1, ch);
                return;
            }
            break;
        case ITEM_SPICE:
            if (bolt->obj_flags.value[0] > 5) {
                send_to_char("You cannot fit that much spice in there."
                             " Try shaving off a smaller pinch.\n\r", ch);
                return;
            }
            if (bolt->obj_flags.value[2]) {
                sprintf(arg1, "%s won't fit in there.\n\r", OSTR(bolt, short_descr));
                send_to_char(arg1, ch);
                return;
            }
            break;
        case ITEM_DRINKCON:
            if (obj_index[bolt->nr].vnum != 13248) {
                sprintf(arg1, "%s won't fit in there.\n\r", OSTR(bolt, short_descr));
                send_to_char(arg1, ch);
                return;
            }
            break;
        default:
            sprintf(arg1, "%s won't fit in there.\n\r", OSTR(bolt, short_descr));
            send_to_char(arg1, ch);
            return;
        }

        bow->obj_flags.value[0] = obj_index[bolt->nr].vnum;
        obj_from_char(bolt);
        obj_to_obj(bolt, bow);
        sprintf(arg2, "You load %s with %s.", OSTR(bow, short_descr), OSTR(bolt, short_descr));
        act(arg2, FALSE, ch, 0, 0, TO_CHAR);
        act("$n loads $p.", FALSE, ch, bow, 0, TO_ROOM);
        return;
    }

    if ((bow->obj_flags.type != ITEM_BOW) || !bow->obj_flags.value[2]) {
        send_to_char("You couldn't possibly load a thing like that.\n\r", ch);
        return;
    }

    if ((tmp = bow->contains)) {
        sprintf(arg1, "%s is already loaded.\n\r", OSTR(bow, short_descr));
        send_to_char(arg1, ch);
        return;
    }

    if (!IS_SET(bolt->obj_flags.extra_flags, OFL_BOLT)) {
        sprintf(arg1, "%s won't fit in there.\n\r", OSTR(bolt, short_descr));
        send_to_char(arg1, ch);
        return;
    }
#ifdef RANGED_STAMINA_DRAIN
    /* allow up to 3 more, at an added stamina cost */
    if (bow->obj_flags.value[1] > GET_STR(ch) + 3) {
#else
    if (bow->obj_flags.value[1] > GET_STR(ch)) {
#endif
        sprintf(arg1, "You aren't strong enough to load %s.\n\r", OSTR(bow, short_descr));
        send_to_char(arg1, ch);
        return;
    }
#ifdef RANGED_STAMINA_DRAIN
    if (GET_MOVE(ch) < 10) {
        cprintf(ch, "You are too tired!\n\r");
        return;
    }

    /* stamina drain for loading a crossbow */
    staminaCost = number(1, 3);

    /* if you're stronger than you need to be */
    if (GET_STR(ch) - bow->obj_flags.value[1] > 0) {
        staminaCost -= MIN(3, GET_STR(ch) - bow->obj_flags.value[1]);
    } else {                    /* pulling too heavy of a bow (double penalty) */
        staminaCost -= MAX(-3, GET_STR(ch) - bow->obj_flags.value[1]) * 2;
    }

    staminaCost = MAX(0, staminaCost);

    adjust_move(ch, -staminaCost);
#endif

    bow->obj_flags.value[3] = obj_index[bolt->nr].vnum;
    obj_from_char(bolt);
    obj_to_obj(bolt, bow);
    sprintf(arg2, "You load %s with %s.", OSTR(bow, short_descr), OSTR(bolt, short_descr));
    act(arg2, FALSE, ch, 0, 0, TO_CHAR);
    act("$n loads $p.", FALSE, ch, bow, 0, TO_ROOM);
}

/* unload a bolt from a "crossbow" */
void
cmd_unload(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *bow;
    OBJ_DATA *bolt;
    char arg1[256], arg2[256];

    if (!*argument) {
        send_to_char("Unload what?\n\r", ch);
        return;
    }

    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    bow = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if (!bow) 
        bow = get_obj_equipped(ch, arg1, FALSE);

    if (!bow) {
        send_to_char("You do not carry that weapon.\n\r", ch);
        return;
    }

    if (bow->obj_flags.type == ITEM_SCENT) {
        if (!bow->contains && bow->obj_flags.value[0])
            bow->obj_flags.value[0] = 0;

        if (bow->obj_flags.value[0] == 0) {
            sprintf(arg1, "%s is not loaded.\n\r", OSTR(bow, short_descr));
            send_to_char(arg1, ch);
            return;
        }

        bolt = get_obj_in_list_vnum(bow->obj_flags.value[0], bow->contains);
        if (!bolt) {
            act("$p seems to be jammed.", FALSE, ch, bow, 0, TO_CHAR);
            errorlog("cmd_unload: attempting to unload nonexistant obj");
            return;
        }

        bow->obj_flags.value[0] = 0;
        obj_from_obj(bolt);
        obj_to_char(bolt, ch);
        sprintf(arg1, "You unload %s from %s.", OSTR(bolt, short_descr), OSTR(bow, short_descr));
        act(arg1, FALSE, ch, 0, 0, TO_CHAR);
        act("$n unloads $p.", FALSE, ch, bow, 0, TO_ROOM);
        return;
    }

    if (!bow->contains && bow->obj_flags.value[3])
        bow->obj_flags.value[3] = 0;

    if (bow->obj_flags.value[3] == 0) {
        sprintf(arg1, "%s is not loaded.\n\r", OSTR(bow, short_descr));
        send_to_char(arg1, ch);
        return;
    }

    bolt = get_obj_in_list_vnum(bow->obj_flags.value[3], bow->contains);
    if (!bolt) {
        act("$p seems to be jammed.", FALSE, ch, bow, 0, TO_CHAR);
        errorlog("cmd_unload: attempting to unload nonexistant obj");
        return;
    }

    bow->obj_flags.value[3] = 0;
    obj_from_obj(bolt);
    obj_to_char(bolt, ch);
    sprintf(arg1, "You unload %s from %s.", OSTR(bolt, short_descr), OSTR(bow, short_descr));
    act(arg1, FALSE, ch, 0, 0, TO_CHAR);
    act("$n unloads $p.", FALSE, ch, bow, 0, TO_ROOM);
}

/* Added 5/27/2000 -Nessalin  */
int
missile_wall_check(CHAR_DATA * ch,      /* Who is shooting        */
                   OBJ_DATA * missile,  /* What they're shooting  */
                   int dir,     /* Where they're shooting */
                   /* When? We don't care. */
                   int attacktype)
{                               /* How they're shooting   */
    char ch_buf[MAX_STRING_LENGTH], rm_buf[MAX_STRING_LENGTH];
    int blocked = 0;

    if (ch && ch->in_room && !ch->in_room->direction[dir])
        return FALSE;

    if (dir < 0)                /* -1 means no direction specified */
        return FALSE;

    if (dir > 5)                /* means dir_out OR WORSE! -Nessalin 1/11/2005 */
        return FALSE;

    if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_BLADE_BARRIER)) {
        sprintf(ch_buf, "You %s $p %s into a wall of whirling blades, which " "knocks it down.",
                (attacktype == CMD_THROW) ? "hurl" : "fire", dir_name[dir]);
        sprintf(rm_buf, "$n %s $p %s into a wall of whirling blades, which " "knocks it down.",
                (attacktype == CMD_THROW) ? "hurl" : "fire", dir_name[dir]);
        blocked = 1;
    }

    if (IS_SET(ch->in_room->direction[dir]->exit_info, EX_SPL_WIND_WALL)) {
        sprintf(ch_buf,
                "You %s $p %s into a wall of gusting wind, which knocks" " it to the ground.",
                (attacktype == CMD_THROW) ? "hurl" : "fire", dir_name[dir]);
        sprintf(rm_buf,
                "$n %s $p %s into a wall of gusting wind, which knocks " "it to the ground.",
                (attacktype == CMD_THROW) ? "hurl" : "fire", dir_name[dir]);
        blocked = 1;
    }

    if (!blocked)
        return FALSE;

    act(rm_buf, FALSE, ch, missile, 0, TO_ROOM);
    act(ch_buf, FALSE, ch, missile, 0, TO_CHAR);

    if (missile == ch->equipment[EP])
        obj_to_room(unequip_char(ch, EP), ch->in_room);
    else if (missile == ch->equipment[ES])
        obj_to_room(unequip_char(ch, ES), ch->in_room);

    return TRUE;
}

/* shoot a bow or crossbow at a target up to the range of the bow */
void
cmd_shoot(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    //  int shoot_dir = 0;
    //  struct affected_type af; 
    CHAR_DATA *victim = NULL;
    ROOM_DATA *tar_room;
    OBJ_DATA *bow;
    OBJ_DATA *arrow;
    int dir = 0, dist = 0, n = 0, dam, offense, defense, alive;
    int prior_hitpoints;
    bool is_hit = FALSE, is_primary = FALSE, loaded = FALSE;
    char arg1[256], arg2[256], arg3[256], new_ld[256], temp_ld[256];
    char buf[256], oldarg[MAX_STRING_LENGTH];
    OBJ_DATA *wagobj = NULL;
#ifdef RANGED_STAMINA_DRAIN
    int staminaCost;
#endif
    struct weather_node *wn;
    float max_cond = 0.0;

    strcpy(oldarg, argument);
    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));


    if ((!stricmp(arg1, "very")) && (!stricmp(arg2, "far"))) {
        sprintf(arg1, "%s %s", arg1, arg2);
        sprintf(arg2, "%s", arg3);
    }

    if (!*arg1) {
        send_to_char("Shoot who?\n\r", ch);
        return;
    }
#ifdef RANGED_STAMINA_DRAIN
    /* check for stamina */
    if (GET_MOVE(ch) < 10) {
        cprintf(ch, "You are too tired!\n\r");
        return;
    }
#endif

    if (IS_CALMED(ch)) {
        send_to_char("You feel much too at ease now to fire any arrows.\n\r", ch);
        return;
    }

    bow = ch->equipment[ES];
    if (!bow) {
      bow = ch->equipment[EP];
      if (!bow) {
        send_to_char("You don't have a bow!\n\r", ch);
        return;
      } else {
        is_primary = TRUE;
      }
    }
    
    if ((bow) && (bow->obj_flags.type != ITEM_BOW)) {
      act("$p is not a bow.", FALSE, ch, bow, 0, TO_CHAR);
      return;
    }
    
    // It doesn't load, so it's a regular bow
    if (bow->obj_flags.value[2] == 0) {
      if (is_primary) { // Wrong hand! Should be in ES
        send_to_char("You are holding the bow in the wrong hand.\n\r", ch);
        return;
      }
      if (!ch->equipment[EP]) { // No arrow!
        send_to_char("You'll need to wield an arrow first.\n\r", ch);
        return;
      }
      arrow = ch->equipment[EP];
      if ((arrow) && (!IS_SET(arrow->obj_flags.extra_flags, OFL_ARROW))) {
        act("$p is not an arrow.", FALSE, ch, arrow, 0, TO_CHAR);
        return;
      }
    } else { // It loads, must be a crossbow style weapon
      if (bow->obj_flags.value[3] == 0) { // Not loaded
        act("You'll have to load $p first.", FALSE, ch, bow, 0, TO_CHAR);
        return;
      }
      
      arrow = get_obj_in_list_vnum(bow->obj_flags.value[3], bow->contains);
      if (!arrow) {
        act("$p seems to be jammed.", FALSE, ch, bow, 0, TO_CHAR);
        errorlog("cmd_shoot: referenced obj not found in crossbow");
        return;
      }
      if ((arrow) && (!IS_SET(arrow->obj_flags.extra_flags, OFL_BOLT))) {
        act("$p is jammed with something other than a bolt.", FALSE, ch, bow, 0, TO_CHAR);
        errorlog("cmd_shoot: crossbow loaded with illegal obj");
        return;
      }
      loaded = TRUE;
    }
    
    if (!bow || !arrow) {
      send_to_char("You need both a bow and an arrow to shoot.\n\r", ch);
      errorlog("cmd_shoot: objects not found 'bow', 'arrow'");
      return;
    }
    
    /* I changed this to give reasonably strength'd people the ability to
     * shoot most bows -Hal */
    if (!bow->obj_flags.value[2]
        && bow->obj_flags.value[1] > (GET_STR(ch) + 3)) {
      send_to_char("You are not strong enough to use this bow.\n\r", ch);
      return;
    }

    if (ch_starts_falling_check(ch)) {
      return;
    }
    
    dir = get_direction(ch, arg2);
    
    n = 0;
    if (dir != -1) {
      dist = bow->obj_flags.value[0];
      if (dist > 100) {
        shhlog("warning, bow has distance more than 100");
        dist = 2;
      }
      for (; dist > 0; dist--) {
        tar_room = get_room_distance(ch, dist, dir);
        if (tar_room && (victim = get_char_other_room_vis(ch, tar_room, arg1))) {
          break;
        }
        n++;
      }
    } else if (!stricmp(arg2, "out")) {
      dir = DIR_OUT;
      tar_room = get_room_distance(ch, 1, DIR_OUT);
      if (!tar_room) {
        send_to_char("You're not inside anything!\n\r", ch);
        return;
      } else {
        victim = get_char_other_room_vis(ch, tar_room, arg1);
        if (!victim) {
          send_to_char("You don't see anyone like that out there.\n\r", ch);
          return;
        }
      }
    } else if ((!stricmp(arg2, "in")) || ((*arg2) && (wagobj = get_obj_room_vis(ch, arg2)))) {
      dir = DIR_IN;
      if ((!wagobj) && (!*arg3)) {
        send_to_char("Shoot them in what?\n\r", ch);
        return;
      }

      if (!wagobj) {
        wagobj = get_obj_room_vis(ch, arg3);
      }

      if (!wagobj) {
        send_to_char("You don't see anything like that here to shoot into.\n\r", ch);
        return;
      }
      
      if (wagobj->obj_flags.type != ITEM_WAGON 
          || wagobj->obj_flags.value[0] == NOWHERE) {        // Entrance room 
        send_to_char("You can't shoot into that.\n\r", ch);
        return;
      }
      
      if (obj_index[wagobj->nr].vnum == 73001) {      // vortex
        snprintf(buf, sizeof(buf),
                 "You fire %s at %s but as it approaches an unseen force causes it to fall to the ground.\n\r",
                 OSTR(arrow, short_descr), OSTR(wagobj, short_descr));
        send_to_char(buf, ch);
        snprintf(buf, sizeof(buf),
                 "$n fires %s at %s but as it approaches an unseen force causes it to fall to the ground.\n\r",
                 OSTR(arrow, short_descr), OSTR(wagobj, short_descr));
        act(buf, FALSE, ch, 0, 0, TO_NOTVICT);
        
        if (loaded) {
          obj_from_obj(arrow);
          bow->obj_flags.value[3] = 0; // Empty the crossbow
        } else if (ch->equipment[EP])
          unequip_char(ch, EP);
        obj_to_room(arrow, ch->in_room);
        return;
      }
      
      tar_room = get_room_num(wagobj->obj_flags.value[0]);
        victim = get_char_other_room_vis(ch, tar_room, arg1);
        if (!victim) {
            sprintf(buf, "You don't see anyone like that in %s.\n\r", OSTR(wagobj, short_descr));
            send_to_char(buf, ch);
            return;
        }
    } else {
        victim = get_char_room_vis(ch, arg1);
    }

    if ((!victim)
        && ((!stricmp("near", arg1)) || (!stricmp("far", arg1))
            || (!stricmp("very far", arg1)))) {
        if (!stricmp("near", arg1))
            dist = 1;
        if (!stricmp("far", arg1))
            dist = 2;
        if (!stricmp("very far", arg1))
            dist = 3;

        if ((bow->obj_flags.value[0]) < dist) {
            act("$p cannot shoot that far.", FALSE, ch, bow, 0, TO_CHAR);
            return;
        }

        tar_room = get_room_distance(ch, dist, dir);

        if (!tar_room) {
            send_to_char("You can't shoot that far in that direction.\n\r", ch);
            return;
        }

        if ((weather_info.wind_velocity > 50)
            && (!IS_SET(ch->in_room->room_flags, RFL_INDOORS))) {
            send_to_char("It would be impossible to shoot in this wind.\n\r", ch);
            return;
        }
#ifdef RANGED_STAMINA_DRAIN
        /* Stamina drain - only on bows. */
        if (!bow->obj_flags.value[2]) {
            staminaCost = number(1, (100 - get_skill_percentage(ch, SKILL_ARCHERY)) / 22 + 2);
            if (GET_STR(ch) - bow->obj_flags.value[1] > 0) {
                staminaCost -= MIN(3, GET_STR(ch) - bow->obj_flags.value[1]);
            } else {            /* pulling too heavy of a bow (double penalty) */
                staminaCost -= MAX(-3, GET_STR(ch) - bow->obj_flags.value[1]) * 2;
            }
            /* minimum of 1 cost */
            adjust_move(ch, -MAX(1, staminaCost));
        }
#endif

        sprintf(buf, "%s flies in from %s.\n\r", OSTR(arrow, short_descr), rev_dir_name[dir]);
        send_to_room(buf, tar_room);

        sprintf(buf, "%s fires %s %s.\n\r", MSTR(ch, short_descr), OSTR(arrow, short_descr),
                dir_name[dir]);
        send_to_room(buf, ch->in_room);

        if (ch->specials.fighting && ch->specials.fighting == victim)
            clear_disengage_request(ch);

        if (loaded) {
            obj_from_obj(arrow);
            bow->obj_flags.value[3] = 0; // Empty the crossbow
	    } else if (ch->equipment[EP])
            unequip_char(ch, EP);
        if (is_hit) {
            if (arrow)
                extract_obj(arrow);
        } else {
            if (arrow)
                obj_to_room(arrow, tar_room);
        }
        return;
    }

    if ((!victim) || (!CAN_SEE(ch, victim))) {
        send_to_char("There is no one like that in range.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("Shoot yourself? That might be painful.\n\r", ch);
        return;
    }

    if (weather_info.wind_velocity > 50 
     && (!IS_SET(ch->in_room->room_flags, RFL_INDOORS))) {
        send_to_char("It would be impossible to shoot in this wind.\n\r", ch);
        return;
    }

    tar_room = victim->in_room;

    if (!ch->specials.to_process) {
        if (ch->in_room && victim->in_room && (ch->in_room != victim->in_room)) {
            strcpy(new_ld, "is here, aiming ");
            sprintf(temp_ld, "%s ", OSTR(bow, short_descr));
            strcat(new_ld, temp_ld);
            switch (dir) {
            case 0:            /* NORTH */
                strcat(new_ld, "to the north.");
                break;
            case 1:            /* EAST */
                strcat(new_ld, "to the east.");
                break;
            case 2:            /* SOUTH */
                strcat(new_ld, "to the south.");
                break;
            case 3:            /* WEST */
                strcat(new_ld, "to the west.");
                break;
            case 4:            /* UP */
                strcat(new_ld, "upwards.");
                break;
            case 5:            /* DOWN */
                strcat(new_ld, "downwards.");
                break;
            case 6:            /* OUT */
                strcat(new_ld, "out.");
                break;
            case 7:            /* IN */
                sprintf(buf, "into %s", OSTR(wagobj, short_descr));
                strcat(new_ld, buf);
                break;
            }                   /* End switch(dir) */
        } /* End if ch && victim are in same room */
        else
            sprintf(new_ld, "is here, aiming %s carefully.", OSTR(bow, short_descr));
        if (!change_ldesc(new_ld, ch)) {
            shhlogf("ERROR: %s's ldesc is too long.   Other.c  Case: Shoot",
                    MSTR(ch, name));
        }

        add_to_command_q(ch, oldarg, cmd, 0, 0);
        return;
    }

    if (has_skill(ch, SKILL_ARCHERY))
        offense = number(1, 100) + ch->skills[SKILL_ARCHERY]->learned;
    else
        offense = number(1, 80);

    if (max_cond >= 2.5) {
        // look at weather condition
        for (wn = wn_list; wn; wn = wn->next) {
           // handle ch's room
           if (wn->zone == ch->in_room->zone) {
              max_cond = MAX(max_cond, wn->condition);
              offense -= MAX(0, (wn->condition - 1.5) * 10);
           }
    
           // if not in same room, other room
           if (ch->in_room != tar_room && wn->zone == tar_room->zone) {
              max_cond = MAX(max_cond, wn->condition);
              offense -= MAX(0, (wn->condition - 1.5) * 10);
           }
        }
    } 
    else {
       // if worst condition is stormy, don't also apply wind
       offense -= (weather_info.wind_velocity / 5) * 2;
    }

    offense += agl_app[GET_AGL(ch)].missile;

    defense = number(1, 100) + victim->abilities.def;
    defense += agl_app[GET_AGL(victim)].reaction;

    if (GET_RACE(victim) == RACE_MANTIS)
        defense *= 2;

    if (n > 0)
        offense -= (n * 10);

    if ((GET_RACE(victim) == RACE_GESRA) && (n > 0))
        offense = 0;

    // npcs that aren't marked [archery]
    if (IS_NPC(victim) && !isnamebracketsok("[archery]", MSTR(victim, name))) {
        // more than 1 room away auto-fail
        if (n > 1)
            offense = 0;

        // sentinels always fail
        if (IS_SET(victim->specials.act, CFL_SENTINEL))
            offense = 0;
    }

    WIPE_LDESC(ch);

    if ((dir < DIR_OUT) && (missile_wall_check(ch, arrow, dir, CMD_SHOOT)))
        return;

    prior_hitpoints = GET_HIT(victim);

#ifdef RANGED_STAMINA_DRAIN
    /* Stamina drain - only on bows. */
    if (!bow->obj_flags.value[2]) {
        staminaCost = number(1, (100 - get_skill_percentage(ch, SKILL_ARCHERY)) / 22 + 2);
        if (GET_STR(ch) - bow->obj_flags.value[1] > 0) {
            staminaCost -= MIN(3, GET_STR(ch) - bow->obj_flags.value[1]);
        } else {                /* pulling too heavy of a bow (double penalty) */
            staminaCost -= MAX(-3, GET_STR(ch) - bow->obj_flags.value[1]) * 2;
        }
        /* minimum of 1 cost */
        adjust_move(ch, -MAX(1, staminaCost));
    }
#endif

    if (ch->specials.fighting && ch->specials.fighting == victim)
        clear_disengage_request(ch);

    if (offense < defense) {
        if (ch->in_room != victim->in_room)
            alive = missile_damage(ch, victim, arrow, dir, 0, TYPE_BOW);
        else
            alive = ws_damage(ch, victim, 0, 0, 0, 0, TYPE_BOW, 0);

        /* high winds without skill, or offense = 0, no gain */
        if (has_skill(ch, SKILL_ARCHERY) && offense > 0
         && max_cond - 1.5 * 10 < get_skill_percentage(ch, SKILL_ARCHERY)) {
            /* if target is asleep or paralyzed, decrease chance of learning */
            if ((!AWAKE(victim) || is_paralyzed(victim)) && !number(0, 11)) {
                /* can only learn up to 40% on sparring dummies */
                if (get_skill_percentage(ch, SKILL_ARCHERY) <= 40)
                    gain_skill(ch, SKILL_ARCHERY, 1);
            } else {
                gain_skill(ch, SKILL_ARCHERY, 1);
            }
        }
    } else {
        is_hit = TRUE;
        /* calculate damage */
        if (arrow->obj_flags.type == ITEM_WEAPON) {
            dam =
                dice(arrow->obj_flags.value[1],
                     arrow->obj_flags.value[2]) + arrow->obj_flags.value[0];
        } else {                /* not a weapon -Morg 3/12/03 */

            dam = dice(1, 2);
        }

        /* strength bonus, limited str */
        if (GET_STR(ch) > bow->obj_flags.value[1] + 1)
            dam += str_app[bow->obj_flags.value[1]].todam;

        /* skill bonus */
        if (ch->skills[SKILL_ARCHERY]) {
            dam += ch->skills[SKILL_ARCHERY]->learned / 7;
            if (number(0, 500) <= ch->skills[SKILL_ARCHERY]->learned)
                dam *= 2;
        }

        if (ch->in_room != victim->in_room)
            alive = missile_damage(ch, victim, arrow, dir, dam, TYPE_BOW);
        else
            alive = ws_damage(ch, victim, dam + 2, 0, 0, dam + 2, TYPE_BOW, 0);

        if (alive && IS_SET(arrow->obj_flags.extra_flags, OFL_POISON)) {
            if (!does_save(victim, SAVING_PARA, 0)) {
                if (prior_hitpoints != GET_HIT(victim)) {
                    /* Changed to allow new poison types - Tiernan 4/23 */
                    poison_char(victim, arrow->obj_flags.value[5], number(1, (27 - GET_END(ch))),
                                0);

                    send_to_char("You feel very sick.\n\r", victim);
                }
            }
        }
    }

    /* We move the arrow here, before damage happens, so we don't have to worry about
     * the shot killing the target and leaving a dangling pointer on the arrow object
     */
    if (loaded) {
        obj_from_obj(arrow);
        bow->obj_flags.value[3] = 0; // Empty the crossbow
    } else if (ch->equipment[EP])
        unequip_char(ch, EP);
    if (is_hit && alive) {  // Only put arrow on character if alive, otherwise drop in room.
        if (arrow) {
            if (!(number(0, 3)))
                obj_to_char(arrow, victim);
            else
                extract_obj(arrow);
        }
    } else {
        if (arrow)
            obj_to_room(arrow, tar_room);
    }
}

void
cmd_speak(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{

    int i;
    char buf[80], buf2[256];
    argument = one_argument(argument, buf, sizeof(buf));

    if (!*buf) {
        sprintf(buf2, "You are currently speaking %s.\n\r", skill_name[GET_SPOKEN_LANGUAGE(ch)]);
        send_to_char(buf2, ch);
        return;
    }

    for (i = 0; i < (MAX_TONGUES); i++) {
        if (!strncmp(buf, skill_name[language_table[i].spoken], 4)) {
            if (!ch->skills[language_table[i].spoken]) {
                send_to_char("You know nothing of that language.\n\r", ch);
                return;
            }
            sprintf(buf, "You begin speaking %s.\n\r", skill_name[language_table[i].spoken]);
            send_to_char(buf, ch);
            ch->specials.language = i;
            return;
        }
    }
    send_to_char("You've never heard of that language.\n\r", ch);
}


void
make_bead(CHAR_DATA * ch, const char *argument)
{
    OBJ_DATA *bead;
    char buf[256];
    int dec, new_eco;

    if (GET_RACE(ch) != RACE_GALITH) {
        send_to_char("What?", ch);
        return;
    }

    argument = one_argument(argument, buf, sizeof(buf));

    if (!(dec = atoi(buf))) {
        send_to_char("How much heat will you sacrifice ?\n", ch);
        return;
    }
    if (dec < 0) {
        send_to_char("You can only sacrifice a positive amount.\n", ch);
        return;
    }
    if ((new_eco = ch->specials.eco - dec) < 0) {
        send_to_char("You don't have that much heat to sacrifice.\n", ch);
        return;
    }

    bead = read_object(45203, VIRTUAL);
    obj_to_char(bead, ch);

    ch->specials.eco = new_eco;
    sprintf(buf, "You have sacrificed %d hours of service to create a new bead.\n", dec);
    send_to_char(buf, ch);

    return;
}

void
cmd_make(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];

    argument = one_argument(argument, buf, sizeof(buf));

    if (!strncmp("bead", buf, strlen(buf)))
        make_bead(ch, argument);
    else if (!strncmp("smoke", buf, strlen(buf)))
        make_smoke(ch, argument, 0);
    else
        send_to_char("What?", ch);

    return;
}

void
cmd_donate(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], buf2[256];
    OBJ_DATA *stick;
    int mana;

    argument = two_arguments(argument, buf, sizeof(buf), buf2, sizeof(buf2));
    if (!isdigit(*buf) || strcmp("mana", buf2)) {
        send_to_char("donate <number> mana <object>\n\r", ch);
        return;
    }
    mana = atoi(buf);
    if ((GET_MANA(ch) < mana) || (mana < 1)) {
        send_to_char("You cannot donate that much mana.", ch);
        return;
    }

    argument = one_argument(argument, buf, sizeof(buf));
    if (!(stick = get_obj_equipped(ch, buf, FALSE))) {
        act("You do not hold that item in your hand.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    if ((stick->obj_flags.type != ITEM_STAFF) && (stick->obj_flags.type != ITEM_WAND)) {
        act("That item does not accept your magick.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    if (stick->obj_flags.value[2] + mana > stick->obj_flags.value[0]) {
        act("That item will not accept that much magick.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    stick->obj_flags.value[2] += mana;
    adjust_mana(ch, -mana);

    act("$n concentrates on $p.", TRUE, ch, stick, 0, TO_ROOM);
    sprintf(buf2, "You concentrate on $p, and put %d mana into it", mana);
    act(buf2, FALSE, ch, stick, 0, TO_CHAR);
}

// Used by cmd_use to add a seal to a note item.  This function handles
// all the checks and messages.
int
seal_obj_with_obj(OBJ_DATA * scroll, OBJ_DATA * stick, CHAR_DATA * ch)
{
    char *seal;
    char edesc[MAX_STRING_LENGTH];

    if ((!(scroll)) || (!(stick)) || (!(ch)))
        return FALSE;

    seal = find_ex_description("[seal_desc]", stick->ex_description, TRUE);

    if (!seal)
        return FALSE;

    if (scroll->obj_flags.type == ITEM_NOTE) {
      if (!(IS_SET(scroll->obj_flags.value[3], NOTE_SEALABLE))) {
        send_to_char("That is not suitable for being sealed.\n\r", ch);
        return TRUE;
      }
      
      if (IS_SET(scroll->obj_flags.value[3], NOTE_SEALED)) {
        send_to_char("That has already been sealed.\n\r", ch);
        return TRUE;
      }
      
      if (IS_SET(scroll->obj_flags.value[3], NOTE_BROKEN)) {
        send_to_char ("That has already been sealed, although the seal appears to be broken now.\n\r", ch);
        return TRUE;
      }
      
      MUD_SET_BIT(scroll->obj_flags.value[3], NOTE_SEALED);
    } else if (scroll->obj_flags.type == ITEM_CONTAINER) {
      if (!(IS_SET(scroll->obj_flags.value[1], CONT_CLOSEABLE))) {
        send_to_char("It cannot be sealed if it cannot be closed.\n\r", ch);
        return TRUE;
      }

      if (!(IS_SET(scroll->obj_flags.value[1], CONT_CLOSED))) {
        send_to_char("It must be closed before it can be sealed.\n\r", ch);
        return TRUE;
      }
      
      if (IS_SET(scroll->obj_flags.value[1], CONT_SEALED)) {
        send_to_char("That has already been sealed.\n\r", ch);
        return TRUE;
      }
      
      if (IS_SET(scroll->obj_flags.value[1], CONT_SEAL_BROKEN)) {
        send_to_char ("That has already been sealed, although the seal appears to be broken now.\n\r", ch);
        return TRUE;
      }
      
      MUD_SET_BIT(scroll->obj_flags.value[1], CONT_SEALED);
    } else {
      send_to_char("Only writable and closeable items can be sealed.\n\r", ch);
      return TRUE;
    }

    strcpy(edesc, seal);

    set_obj_extra_desc_value(scroll, "seal", edesc);

    sprintf(edesc, "%s uses %s to impress a seal upon %s.", MSTR(ch, short_descr),
            OSTR(stick, short_descr), OSTR(scroll, short_descr));
    act(edesc, FALSE, ch, 0, 0, TO_ROOM);

    sprintf(edesc, "You use %s to impress a seal upon %s.", OSTR(stick, short_descr),
            OSTR(scroll, short_descr));
    act(edesc, FALSE, ch, 0, 0, TO_CHAR);
    

    return TRUE;
}

void
cmd_use(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
  char buf[256], buf2[256], oldarg[MAX_INPUT_LENGTH];
    CHAR_DATA *tmp_char = NULL;
    OBJ_DATA *tmp_object = NULL, *stick = NULL, *scroll = NULL;
    char holder[MAX_INPUT_LENGTH];
    int bits, mana_cost;
    byte level;
    int MAX_MANA_REDUCTION_PER_USE_WAND = 100;

    if (strlen(argument) > 200)
        return;

    strcpy (oldarg, argument);

    argument = one_argument(argument, buf, sizeof(buf));

    if (!(stick = get_obj_equipped(ch, buf, FALSE))) {
        act("You do not hold that item in your hand.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    strcpy(holder, argument);

    argument = one_argument(argument, buf, sizeof(buf));        /* this is the power-word 
                                                                 * * one_argument will turn it
                                                                 * * to all lower-cases 
                                                                 */

    if (IS_AFFECTED(ch, CHAR_AFF_SILENCE) || affected_by_spell(ch, PSI_BABBLE))
        strcpy(buf, "'mrumph'");

    // Commented out by Nessalin 11/5/2000 so that some ITEM types
    // can be called form use without an argument.
    //  if (strlen (buf) < 1)
    //    {
    //      send_to_char ("How would you like to use it?\n\r", ch);
    //      return;
    //    }


    if (stick->obj_flags.type == ITEM_STAFF) {
        if ((strlen(buf) < 3) || (buf[0] != '\'') || (buf[strlen(buf) - 1] != '\'')) {
            send_to_char("The magick power word must be enclosed by two single quotes.\n\r", ch);
            return;
        }
        sprintf(buf2, "$n grips $p tightly and says %s.", buf);
        act(buf2, TRUE, ch, stick, 0, TO_ROOM);
        sprintf(buf2, "You grip $p tightly and say %s.", buf);
        act(buf2, FALSE, ch, stick, 0, TO_CHAR);
    } else if (stick->obj_flags.type == ITEM_WAND) {
        if ((strlen(buf) < 3) || (buf[0] != '\'') || (buf[strlen(buf) - 1] != '\'')) {
            send_to_char("The magick power word must be enclosed " "by two single quotes.\n\r", ch);
            return;
        }
        bits =
            generic_find(argument, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP,
                         ch, &tmp_char, &tmp_object);
        if (bits) {
            if (bits & FIND_CHAR_ROOM) {
                sprintf(buf2, "$n points $p at $N and says %s.", buf);
                act(buf2, TRUE, ch, stick, tmp_char, TO_ROOM);
                sprintf(buf2, "You point $p at $N and say %s.", buf);
                act(buf2, TRUE, ch, stick, tmp_char, TO_CHAR);
            } else if (bits & (FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP)) {
                sprintf(buf2, "$n points $p at $P and says %s.", buf);
                act(buf2, TRUE, ch, stick, tmp_object, TO_ROOM);
                sprintf(buf2, "You point $p at $P and say %s.", buf);
                act(buf2, TRUE, ch, stick, tmp_object, TO_CHAR);
            } else {
                gamelog("Unknown bits returned in cmd_use.");
            }
        } else {                /* wand pointed at nothing */
	  send_to_char("Use on who or what?\n\r", ch);
	  return;
	  //            sprintf(buf2, "$n waves $p and says %s.", buf);
	  //            act(buf2, TRUE, ch, stick, NULL, TO_ROOM);
	  //            sprintf(buf2, "You wave $p and say %s.", buf);
	  //            act(buf2, TRUE, ch, stick, NULL, TO_CHAR);
        }
    } else if (stick->obj_flags.type == ITEM_SCROLL) {
        if (!(char_can_see_obj(ch, stick))) {
            send_to_char("You can't see it, let alone read it.\n\r", ch);
            return;
        }

        if (affected_by_spell(ch, SPELL_FEEBLEMIND)) {
            send_to_char("The letters are all squiggly and confusing.\n\r", ch);
            return;
        }

        if (stick->obj_flags.value[1] < 1) {
            send_to_char("There is nothing written on it...\n\r", ch);
            return;
        }

        if (!(has_skill(ch, stick->obj_flags.value[5]))) {
            send_to_char("It is not written in a language you can read.\n\r", ch);
            return;
        }


        bits =
            generic_find(holder, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch,
                         &tmp_char, &tmp_object);
        if (bits) {
            if (bits & FIND_CHAR_ROOM) {
                if (tmp_char != ch) {
                    sprintf(buf2, "$n reads from $p and gestures at $N.");
                    act(buf2, TRUE, ch, stick, tmp_char, TO_ROOM);
                    sprintf(buf2, "You read from $p and gesture at $N.");
                    act(buf2, TRUE, ch, stick, tmp_char, TO_CHAR);
                    sprintf(buf2, "In $n's hands, $p crumbles away.");
                    act(buf2, TRUE, ch, stick, 0, TO_ROOM);
                    sprintf(buf2, "In your hands, $p crumbles away.");
                    act(buf2, TRUE, ch, stick, 0, TO_CHAR);
                } else {
                    act("$n reads from $p.", TRUE, ch, stick, 0, TO_ROOM);
                    act("You read from $p.", TRUE, ch, stick, 0, TO_CHAR);
                    sprintf(buf2, "In $n's hands, $p crumbles away.");
                    act(buf2, TRUE, ch, stick, 0, TO_ROOM);
                    sprintf(buf2, "In your hands, $p crumbles away.");
                    act(buf2, TRUE, ch, stick, 0, TO_CHAR);
                }
            } else if (bits & (FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP)) {
                sprintf(buf2, "$n reads from $p and gestures at $P.");
                act(buf2, TRUE, ch, stick, tmp_object, TO_ROOM);
                sprintf(buf2, "You read from $p and gesture at $P.");
                act(buf2, TRUE, ch, stick, tmp_object, TO_CHAR);
                sprintf(buf2, "In $n's hands, $p crumbles away.");
                act(buf2, TRUE, ch, stick, 0, TO_ROOM);
                sprintf(buf2, "In your hands, $p crumbles away.");
                act(buf2, TRUE, ch, stick, 0, TO_CHAR);
            } else {
                gamelog("Unknown bits returned in cmd_use.");
            }
        } else {
            act("$n reads from $p.", TRUE, ch, stick, 0, TO_ROOM);
            act("You read from $p.", TRUE, ch, stick, 0, TO_CHAR);
            // Commented out by Nessalin on 11/5/2000 to make scrolls usable
            // on rooms.
            //      send_to_char ("What would you like to use it on?\n\r", ch);
            //      return;
        }

        level = stick->obj_flags.value[0];
        ((*skill[stick->obj_flags.value[1]].function_pointer)
         (level, ch, argument, SPELL_TYPE_SCROLL, tmp_char, tmp_object));
        if (stick->obj_flags.value[2] > 0)
            ((*skill[stick->obj_flags.value[2]].function_pointer)
             (level, ch, argument, SPELL_TYPE_SCROLL, tmp_char, tmp_object));
        if (stick->obj_flags.value[3] > 0)
            ((*skill[stick->obj_flags.value[3]].function_pointer)
             (level, ch, argument, SPELL_TYPE_SCROLL, tmp_char, tmp_object));

        extract_obj(stick);
        return;

    } else {

        if (!(stricmp(buf, ""))) {
            send_to_char("Use it how?\n\r", ch);
            return;
        }

        one_argument(holder, buf, sizeof(buf));

        if (!(scroll = get_obj_in_list_vis(ch, buf, ch->carrying))) {
            sprintf(buf2, "You're not carrying %s to use $p on.", buf);
            act(buf2, TRUE, ch, stick, 0, TO_CHAR);
            return;
        }

        if (seal_obj_with_obj(scroll, stick, ch))
            return;

        // commented out 11/4/2000 by nessalin
        // More things use cmd_use than magick, so this is misleading
        //      send_to_char ("That is not very usable for magic.\n\r", ch);

        send_to_char("You're not sure how to use that.\n\r", ch);
        return;
    }

    if (!((stick->obj_flags.value[3] > 0) && (stick->obj_flags.value[3] < MAX_SKILLS)
          && skill[stick->obj_flags.value[3]].function_pointer))
        return;

    level = old_search_block(&buf[1], 0, strlen(buf) - 2, Power, 1);
    if (level == -1)
        return;                 /* maybe a message that they said it wrong */

    if (stick->obj_flags.value[1] < level) {
        send_to_char("It does not respond to that power-word.\n\r", ch);
        return;
    }

    mana_cost = 10 * mana_amount_level(stick->obj_flags.value[1], level, stick->obj_flags.value[3]);

    if (stick->obj_flags.type == ITEM_WAND) {
      mana_cost = 600 + (200 * (level - 1));
      if (!get_obj_extra_desc_value(stick, "[ARTIFACT]", buf , 100))
	if (stick->obj_flags.value[0] > (1100 + (stick->obj_flags.value[1] * 100)))
	  stick->obj_flags.value[0] = 1100 + (stick->obj_flags.value[1] * 100);
	}


    if (stick->obj_flags.value[2] < mana_cost) {        /* check to see if it
                                                         * has enough mana */
        send_to_char("It does not have enough mana in it.\n\r", ch);
        return;
    }

    stick->obj_flags.value[2] -= mana_cost;

    if (stick->obj_flags.type == ITEM_STAFF)
        ((*skill[stick->obj_flags.value[3]].function_pointer)
         (level, ch, argument, SPELL_TYPE_STAFF, 0, 0));
    else if (stick->obj_flags.type == ITEM_WAND) {
      if (!get_obj_extra_desc_value(stick, "[ARTIFACT]", buf , 100)) 
	stick->obj_flags.value[0] = stick->obj_flags.value[0] - MAX_MANA_REDUCTION_PER_USE_WAND;
      ((*skill[stick->obj_flags.value[3]].function_pointer)
       (level, ch, argument, SPELL_TYPE_WAND, tmp_char, tmp_object));
    }

    if (stick)
      if (stick->obj_flags.type == ITEM_WAND)
	WAIT_STATE(ch, 1);

}


void
cmd_tenebrae(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char(TYPO_MSG, ch);
    return;
}

void
cmd_hayyeh(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char(TYPO_MSG, ch);
    return;
}

void
sheath_delay(CHAR_DATA *ch, OBJ_DATA *obj, int newpos) {
    int delay = number(10, 15);
    int skill = 0;

    // give an agility bonus
    delay -= agl_app[GET_AGL(ch)].reaction / 2;

    if( obj ) {
        // heavier objects are harder to draw, below 4 stones it's easier
        delay += (GET_OBJ_WEIGHT(obj) - 4) / 5;

        // sheathed on the belt
        if( newpos == WEAR_ON_BELT_1
         || newpos == WEAR_ON_BELT_2) {
            if (is_cloak_open(ch, ch->equipment[WEAR_ABOUT]))
                delay -= 2;
            else
                delay += 1;
        } 
        // on the back
        else if( newpos == WEAR_ON_BACK ) {
            delay += 2;
        }

        skill = MAX(ch->abilities.off, 
         proficiency_bonus(ch, get_weapon_type(obj->obj_flags.value[3])));
    } else {
        skill = ch->abilities.off;
    }

    // 1 delay for every 5 skill
    delay -= skill / 10;

    // end cap the delay
    delay = MAX(1, MIN(delay, 20));

    if( !IS_IMMORTAL(ch) ) {
        qroomlogf(QUIET_COMBAT, ch->in_room, "sheath delay is %d", delay);
        ch->specials.act_wait += delay;
    }
}

void
cmd_sheathe(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], buf2[256];
    int pos = 0;
    OBJ_DATA *weapon = 0;
    CHAR_DATA *rch = NULL;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    bool quietly = FALSE;
    char orig_arg[MAX_STRING_LENGTH];

    strcpy(orig_arg, argument);
    // Add delay for Stow
    if (!ch->specials.to_process && cmd == CMD_STOW) {
        add_to_command_q(ch, orig_arg, cmd, 0, 0);
	return;
    }

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));

    argument = two_arguments(args, buf, sizeof(buf), buf2, sizeof(buf2));

    if (!*buf) {
        send_to_char("Sheathe what?\n\r", ch);
        return;
    }

    if (ch->equipment[ETWO] && isname(buf, OSTR(ch->equipment[ETWO], name))) {
        weapon = ch->equipment[ETWO];
        pos = ETWO;
    } else if (ch->equipment[EP] && isname(buf, OSTR(ch->equipment[EP], name))) {
        weapon = ch->equipment[EP];
        pos = EP;
    } else if (ch->equipment[ES] && isname(buf, OSTR(ch->equipment[ES], name))) {
        weapon = ch->equipment[ES];
        pos = ES;
    } else if (!strcasecmp(buf, "primary") || !strcasecmp(buf, "ep")) {
        weapon = ch->equipment[EP];
        pos = EP;
    } else if (!strcasecmp(buf, "secondary") || !strcasecmp(buf, "es")) {
        weapon = ch->equipment[ES];
        pos = ES;
    } else if (!strcasecmp(buf, "etwo") || !strcasecmp(buf, "two") || !strcasecmp(buf, "both")) {
        weapon = ch->equipment[ETWO];
        pos = ETWO;
    }


    if (!weapon) {
        send_to_char("You don't wield that weapon.\n\r", ch);
        return;
    }
//  if (weapon->obj_flags.type != ITEM_WEAPON)
    if ((weapon->obj_flags.type != ITEM_WEAPON) && (weapon->obj_flags.type != ITEM_BOW)) {
        send_to_char("That's not a weapon.\n\r", ch);
        return;
    }

    if (IS_SET(weapon->obj_flags.extra_flags, OFL_NO_SHEATHE)) {
        send_to_char("You can't sheathe that.\n\r", ch);
        return;
    }

    // Same as palm, if it's too big, tell them rather than make life harder
    if (cmd == CMD_STOW && GET_OBJ_WEIGHT(weapon) > (get_char_size(ch) / 3 + 1)) {
        send_to_char("It is way too heavy, surely someone would notice.\n\r", ch);
        return;
    }

    // Do the quiet check before we figure out where it's going
    if ((cmd == CMD_STOW) && (ch->skills[SKILL_SLEIGHT_OF_HAND])) {
        if (skill_success(ch, NULL, ch->skills[SKILL_SLEIGHT_OF_HAND]->learned))
	    quietly = TRUE;
    }

    if (!*buf2) {
        if ((ch->equipment[WEAR_ON_BELT_1]) && (ch->equipment[WEAR_ON_BELT_2])) {
            send_to_char("You can't wear anymore on your belt.\n\r", ch);
            return;
        }
        if (!ch->equipment[WEAR_BELT]) {
            send_to_char("You'll need to wear a belt to sheathe weapons.\n\r", ch);
            return;
        }
        if (IS_SET(weapon->obj_flags.extra_flags, OFL_NO_ES)) {
            send_to_char("That weapon is too large to hang on your belt.\n\r", ch);
            return;
        }
        if (weapon->obj_flags.type == ITEM_BOW) {
            send_to_char("You can't sheathe that on your belt.\n\r", ch);
            return;
        }

        sheath_delay(ch, weapon, WEAR_ON_BELT_1);
        unequip_char(ch, pos);
        affect_total(ch);

        if (ch->equipment[WEAR_ON_BELT_1]) {
            equip_char(ch, weapon, WEAR_ON_BELT_2);
        }
        else {
            equip_char(ch, weapon, WEAR_ON_BELT_1);
        }

        affect_total(ch);

        sprintf(to_char, "you %s %s",
			quietly ? "stow" : "sheathe",
			OSTR(weapon, short_descr));
        sprintf(to_room, "@ %s %s",
			quietly ? "stows" : "sheathes",
			OSTR(weapon, short_descr));
	if (quietly) {
            if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow,
               to_char, to_room, to_room, MONITOR_ROGUE, -20, SKILL_SLEIGHT_OF_HAND)) {
	    }
	} else {
            if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
               to_room, to_room,
	       (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
               act("You sheathe $p.", FALSE, ch, weapon, 0, TO_CHAR);
	       act("$n sheathes $p.", FALSE, ch, weapon, 0, TO_ROOM);
	    }
	}
    } else {
        if (strcmp(buf2, "back")) {
            send_to_char("Sheathe it where??\n\r", ch);
            return;
        }
//      if (ch->equipment[WEAR_BACK])
        if (ch->equipment[WEAR_ON_BACK]) {
            send_to_char("You cannot wear anymore on your back.\n\r", ch);
            return;
        }

        if (!CAN_SHEATH_BACK(weapon)) {
            send_to_char("That weapon is too small to wear on your back.\n\r", ch);
            return;
        }

        sheath_delay(ch, weapon, WEAR_ON_BACK);
        unequip_char(ch, pos);
        affect_total(ch);
//      equip_char (ch, weapon, WEAR_BACK);
        equip_char(ch, weapon, WEAR_ON_BACK);
        affect_total(ch);

        sprintf(to_char, "you %ssling %s across your back",
			quietly ? "quietly " : "",
			OSTR(weapon, short_descr));
        sprintf(to_room, "@ %sslings %s across %s back",
			quietly ? "quietly " : "",
                OSTR(weapon, short_descr), HSHR(ch));
	if (quietly) {
            if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow,
               to_char, to_room, to_room, MONITOR_ROGUE, -20, SKILL_SLEIGHT_OF_HAND)) {
	    }
	} else {
            if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
               to_room, to_room,
	       (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
	       act("You sling $p across your back.", FALSE, ch, weapon, 0, TO_CHAR);
	       act("$n slings $p across $s back.", FALSE, ch, weapon, 0, TO_ROOM);
	    }
	}
    }

    // So they failed to be quiet about it, tell folks they tried
    if (cmd == CMD_STOW && !quietly) {      // Failed to stow
       int witnesses = 0;
       for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
	   if (watch_success(rch, ch, NULL, 20, SKILL_SLEIGHT_OF_HAND)) {
	      cprintf(rch, "You notice %s was trying to be furtive about it.\n\r", PERS(rch, ch));
	      witnesses++;
	   }
       }
       if (witnesses)
	  gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
    }
}

void
draw_delay(CHAR_DATA *ch, OBJ_DATA *obj) {
    int delay = number(8, 15);
    int skill = 0;

    qroomlogf(QUIET_COMBAT, ch->in_room, "draw delay base: %d", delay);

    // give an agility bonus
    delay -= agl_app[GET_AGL(ch)].reaction / 2;
    qroomlogf(QUIET_COMBAT, ch->in_room, "agility:         %d", delay);

    // 1 less delay for every 10 skill in offense

    if( obj ) {
        // heavier objects are harder to draw, below 4 stones it's easier
        delay += (GET_OBJ_WEIGHT(obj) - 4) / 5;
        qroomlogf(QUIET_COMBAT, ch->in_room, "obj weight:      %d", delay);

        // sheathed on the belt
        if( ch->equipment[WEAR_ON_BELT_1] == obj
         || ch->equipment[WEAR_ON_BELT_2] == obj) {
            if (is_cloak_open(ch, ch->equipment[WEAR_ABOUT]))
                delay -= 2;
            else
                delay += 2;
            qroomlogf(QUIET_COMBAT, ch->in_room, "worn on belt:    %d", delay);
        } 
        // on the back
        else if( ch->equipment[WEAR_ON_BACK] == obj ) {
            delay += 2;
            qroomlogf(QUIET_COMBAT, ch->in_room, "worn on back:    %d", delay);
        }
 
        skill = MAX(ch->abilities.off, 
         proficiency_bonus(ch, get_weapon_type(obj->obj_flags.value[3])));
    } else {
        skill = ch->abilities.off;
    }

    // 1 delay for every 10 skill
    delay -= skill / 10;
    qroomlogf(QUIET_COMBAT, ch->in_room, "skill:           %d", delay);

    delay = MAX(1, MIN(delay, 18));
    qroomlogf(QUIET_COMBAT, ch->in_room, "limits:          %d", delay);

    if( !IS_IMMORTAL(ch) ) {
        qroomlogf(QUIET_COMBAT, ch->in_room, "draw delay is %d", delay);
        ch->specials.act_wait += delay;
    }
}


void
cmd_draw(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], mod[256];
    int pos = 0, newpos, two_hands = 0;
    OBJ_DATA *weapon = 0;
    CHAR_DATA *rch = NULL;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    bool quietly = FALSE;
    char orig_arg[MAX_STRING_LENGTH];

    strcpy(orig_arg, argument);
    // Add delay for Ready
    if (!ch->specials.to_process && cmd == CMD_READY) {
        add_to_command_q(ch, orig_arg, cmd, 0, 0);
	return;
    }

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow, sizeof(postHow));


    argument = one_argument(args, buf, sizeof(buf));

    if (!*buf) {
        send_to_char("Draw what?\n\r", ch);
        return;
    }

    // Do the quiet check first, then find reasons to negate it
    if ((cmd == CMD_READY) && (ch->skills[SKILL_SLEIGHT_OF_HAND])) {
        if (skill_success(ch, NULL, ch->skills[SKILL_SLEIGHT_OF_HAND]->learned))
	    quietly = TRUE;
    }
    if (ch->lifting) {
        sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
        send_to_char(buf, ch);
	quietly = FALSE; // Can't be quiet when dropping something
        if (!drop_lifting(ch, ch->lifting))
            return;
    }

    if ((ch->equipment[EP] && ch->equipment[ES]) || (ch->equipment[ETWO])) {
        send_to_char("You seem to have your hands full already.\n\r", ch);
        return;
    }

    if (ch->equipment[WEAR_ON_BELT_1] && isname(buf, OSTR(ch->equipment[WEAR_ON_BELT_1], name))) {
        pos = WEAR_ON_BELT_1;
        weapon = ch->equipment[WEAR_ON_BELT_1];
    } else if (ch->equipment[WEAR_ON_BELT_2]
               && isname(buf, OSTR(ch->equipment[WEAR_ON_BELT_2], name))) {
        pos = WEAR_ON_BELT_2;
        weapon = ch->equipment[WEAR_ON_BELT_2];
    } else if (ch->equipment[WEAR_ON_BACK]
               && isname(buf, OSTR(ch->equipment[WEAR_ON_BACK], name))) {
        pos = WEAR_ON_BACK;
        weapon = ch->equipment[WEAR_ON_BACK];
    }
    /*
     * else if (ch->equipment[WEAR_BACK] &&
     * isname (buf, OSTR (ch->equipment[WEAR_BACK], name)))
     * {
     * pos = WEAR_BACK;
     * weapon = ch->equipment[WEAR_BACK];
     * }
     */

    if (!weapon) {
        send_to_char("You don't have that weapon sheathed.\n\r", ch);
        return;
    }
//  if (weapon->obj_flags.type != ITEM_WEAPON)
    if ((weapon->obj_flags.type != ITEM_WEAPON) && (weapon->obj_flags.type != ITEM_BOW)) {
        act("$p isn't a weapon.", FALSE, ch, weapon, 0, TO_CHAR);
        return;
    }

    argument = one_argument(argument, mod, sizeof(mod));
    if (!stricmp(mod, "two") || !stricmp(mod, "etwo")
        || IS_SET(weapon->obj_flags.extra_flags, OFL_NO_ES))
        two_hands = 2;

    if (two_hands && (ch->equipment[EP] || ch->equipment[ES])) {
        send_to_char("Your hands are too full for that weapon.\n\r", ch);
        return;
    }

    if (ch->equipment[ETWO]) {
        act("You have enough trouble holding on to $p.", FALSE, ch, ch->equipment[ETWO], 0,
            TO_CHAR);
        return;
    }

    if (GET_OBJ_WEIGHT(weapon) > (GET_STR(ch) + two_hands)) {
        send_to_char("It's too heavy for you to use.\n\r", ch);
        return;
    }

    if (two_hands)
        newpos = ETWO;
    else if (ch->equipment[EP] || (!IS_SET(weapon->obj_flags.wear_flags, ITEM_EP)))
        newpos = ES;
    else
        newpos = EP;

    if ((newpos == EP) && (!IS_SET(weapon->obj_flags.wear_flags, ITEM_EP))) {
        send_to_char("That cannot be held in your primary hand.\n\r", ch);
        return;
    }

    if ((newpos == ES) && (!IS_SET(weapon->obj_flags.wear_flags, ITEM_ES))) {
        send_to_char("That cannot be held in your secondary hand.\n\r", ch);
        return;
    }

    if (newpos == ES && ch->equipment[ES]) {
        send_to_char("You already are holding something in your secondary hand.\n\r", ch);
        return;
    }

    /* Fix for drawing while subduing someone - Nessalin 3/29/2003 */
    if (ch->specials.subduing != (struct char_data *) 0) {
        act("$n releases $N, who immediately moves away.", FALSE, ch, 0, ch->specials.subduing,
            TO_NOTVICT);
        act("$n releases you, and you immediately move away.", FALSE, ch, 0, ch->specials.subduing,
            TO_VICT);
        act("You release $N, and $E immediately moves away.", FALSE, ch, 0, ch->specials.subduing,
            TO_CHAR);

        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
        ch->specials.subduing = (struct char_data *) 0;
	quietly = FALSE; // Can't be quiet because they're subduing someone
    }

    // Same as palm, if it's too big, tell them rather than make life harder
    if (cmd == CMD_READY && GET_OBJ_WEIGHT(weapon) > (get_char_size(ch) / 3 + 1)) {
        send_to_char("It is way too heavy, surely someone would notice.\n\r", ch);
        return;
    }
    draw_delay(ch, weapon);

    /* removes hide on draw from normal weapons, special sheaths & sleight
     * of hand should be considered separately
     */
    if (IS_AFFECTED(ch, CHAR_AFF_HIDE) && !quietly) {
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
    }

    /* I changed the TO_ROOM act messages below to TRUE so that people
     * would no longer see, "someone draws something" -nessalin 7/1/2000 */
//  if (pos == WEAR_BACK)
    if (pos == WEAR_ON_BACK) {
        sprintf(to_char, "you %sunsling %s from your back",
			quietly ? "quietly " : "",
			OSTR(weapon, short_descr));
        sprintf(to_room, "@ %sunslings %s from %s back",
			quietly ? "quietly " : "",
                OSTR(weapon, short_descr), HSHR(ch));
    } else {
        sprintf(to_char, "you %sdraw %s",
			quietly ? "quietly " : "",
			OSTR(weapon, short_descr));
        sprintf(to_room, "@ %sdraws %s",
			quietly ? "quietly " : "",
			OSTR(weapon, short_descr));
    }

    unequip_char(ch, pos);
    affect_total(ch);
    equip_char(ch, weapon, newpos);
    affect_total(ch);

    if (quietly) {
        if (!send_hidden_to_char_and_room_parsed(ch, preHow, postHow, to_char,
            to_room, to_room, MONITOR_ROGUE, -20, SKILL_SLEIGHT_OF_HAND)) {
	}
    } else  {
        if (!send_to_char_and_room_parsed(ch, preHow, postHow, to_char,
            to_room, to_room, 
            (!*preHow && !*postHow ? MONITOR_NONE : MONITOR_OTHER))) {
	    if (pos == WEAR_ON_BACK) {
		act("You unsling $p from your back.", FALSE, ch, weapon, 0, TO_CHAR);
		act("$n unslings $p from $s back.", TRUE, ch, weapon, 0, TO_ROOM);
	    } else {
		act("You draw $p.", FALSE, ch, weapon, 0, TO_CHAR);
		act("$n draws $p.", TRUE, ch, weapon, 0, TO_ROOM);
	    }
	}
        if (cmd == CMD_READY) {      // Failed to ready
	   int witnesses = 0;
	   for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
	      if (watch_success(rch, ch, NULL, 20, SKILL_SLEIGHT_OF_HAND)) {
		 cprintf(rch, "You notice %s was trying to be furtive about it.\n\r", PERS(rch, ch));
		 witnesses++;
	      }
	   }
	   if (witnesses)
	      gain_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
	}
    }
}

void
add_harness(CHAR_DATA * beast, OBJ_DATA * wagon) {
  struct harness_type *new_beast;
  WAGON_DATA *pWS;
  
  assert(!beast->specials.harnessed_to);
  
  if (done_booting && 
      (pWS = find_wagon_save_by_compare(wagon, wagon->in_room->number)) != NULL) {
    add_attached_mount(pWS, beast);
    save_wagon_saves();
  } else if (done_booting) {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Unable to find wagon save for wagon vnum %d in room %d", obj_index[wagon->nr].vnum, wagon->in_room->number);
    gamelog(buf);
  }
  
  beast->specials.harnessed_to = wagon;
  
  CREATE(new_beast, struct harness_type, 1);
  
  new_beast->beast = beast;
  new_beast->next = wagon->beasts;
  wagon->beasts = new_beast;
  act("You are now harnessed to $p.", FALSE, beast, wagon, 0, TO_CHAR);
}

void
remove_harness(CHAR_DATA * beast) {
  struct harness_type *j, *k;
  WAGON_DATA *pWS;
  
  if (!beast) {
    gamelog("WARNING! No beast was passed to remove_harness();");
    return;
  }
  
  if (!beast->specials.harnessed_to) {
    gamelog("WARNING! The beast passed to remove_harness(); isn't harnessed to anything.");
    return;
  }
  
  if (!beast->specials.harnessed_to->beasts) {
    gamelog("WARNING! The wagon in remove_harness(); has no knowledge of anything being harnessed to it.");
    return;
  }
  
  if (!beast->specials.harnessed_to->beasts->beast) {
    gamelog("WARNING! The list of beasts on the wagon in remove_harness(); has a bad 'beast' pointer.");
    return;
  }
  
  if ((pWS =
       find_wagon_save_by_compare(beast->specials.harnessed_to,
                                  beast->specials.harnessed_to->in_room->number)) != NULL) {
    remove_attached_mount(pWS, beast);
    save_wagon_saves();
  } else {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Unable to find wagon save for wagon vnum %d in room %d",
            obj_index[beast->specials.harnessed_to->nr].vnum,
            beast->specials.harnessed_to->in_room->number);
    gamelog(buf);
  }
  
  if (beast->specials.harnessed_to->beasts->beast == beast) {
    k = beast->specials.harnessed_to->beasts;
    beast->specials.harnessed_to->beasts = k->next;
    free(k);
  } else {
    for (k = beast->specials.harnessed_to->beasts; k->next->beast != beast; k = k->next);
    
    j = k->next;
    k->next = j->next;
    free(j);
  }
  
  beast->specials.harnessed_to = (OBJ_DATA *) 0;
}

void
cmd_unharness(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  send_to_char("Deprecated.\n\r", ch);
}

void
cmd_harness(CHAR_DATA * ch, const char *argument, Command cmd, int count) {
  send_to_char("Deprecated.\n\r", ch);
}


void
stop_gone(CHAR_DATA * ch)
{
    free(ch->specials.gone);
    ch->specials.gone = 0;
    send_to_char("Ok, you are no longer gone.\n\r", ch);
    return;
}

void
cmd_gone(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];

    if ((ch->specials.gone) && (!*argument)) {
        stop_gone(ch);
        return;
    }
    if (!*(argument)) {
        send_to_char("Why are you gone?\n\r", ch);
        return;
    }
    free(ch->specials.gone);
    CREATE(ch->specials.gone, char, strlen(argument) + 1);
    strcpy(ch->specials.gone, (argument + 1));
    sprintf(buf, "Ok, you are gone %s.\n\r", ch->specials.gone);
    send_to_char(buf, ch);
    sprintf(buf, "$n is gone %s.", ch->specials.gone);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
}


void
cmd_delete(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char("You cannot do that here.\n\r", ch);
}

void
cmd_reply(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char("You cannot do that here.\n\r", ch);
}

void
cmd_alias(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char alias_ref[256], alias_text[256], buf[MAX_INPUT_LENGTH];
    int i;
    bool flag;

    if (!*argument) {
        flag = FALSE;
        for (i = 0; i < MAX_ALIAS; i++) {
            if (ch->alias[i].text[0]) {
                if (!flag) {
                    flag = TRUE;
                    send_to_char("Your aliases\n\r------------\n\r", ch);
                }
                sprintf(buf, "%-15s %s\n\r", ch->alias[i].alias, ch->alias[i].text);
                send_to_char(buf, ch);
            }
        }
        if (flag)
            send_to_char("------------\n\r", ch);
        else
            send_to_char("You have no aliases defined.\n\r", ch);
        return;
    }

    argument = one_argument(argument, alias_ref, sizeof(alias_ref));
    strcpy(alias_text, argument);

    if (*alias_ref && strlen(alias_ref) > 15) {
        send_to_char("Your alias reference is too large. (MAX 15 CHAR)\n\r", ch);
        return;
    }

    if (*alias_text && strlen(alias_text) > 60) {
        send_to_char("Your alias text is too large. (MAX 60 CHAR)\n\r", ch);
        return;
    }

    if (!*alias_text) {         /* just an alias reference==>remove that alias */
        if (strlen(alias_ref) > 15) {
            send_to_char("Your alias reference is too large. (MAX 15 CHAR)\n\r", ch);
            return;
        }
        for (i = 0; i < MAX_ALIAS; i++)
            if (ch->alias[i].text[0])
                if (!strcmp(alias_ref, ch->alias[i].alias)) {
                    strcpy(ch->alias[i].text, "");
                    strcpy(ch->alias[i].alias, "");
                    send_to_char("Alias removed.\n\r", ch);
                    return;
                }
        send_to_char("No such alias!\n\r", ch);
        return;
    }

    for (i = 0; i < MAX_ALIAS; i++)     /* look for exising alias...if found */
        if (ch->alias[i].text[0])       /* redefine it..... */
            if (!strcmp(alias_ref, ch->alias[i].alias)) {
                send_to_char("Redefining alias...\n\r", ch);
                strcpy(ch->alias[i].alias, alias_ref);
                strcpy(ch->alias[i].text, alias_text);
                return;
            }

    for (i = 0; i < MAX_ALIAS; i++)     /* Prevents alias loops */
        if (!stricmp(alias_ref, ch->alias[i].text)) {
            send_to_char("You cannot make an alias reference with the text of another alias.\n\r",
                         ch);
            return;
        }


    for (i = 0; i < MAX_ALIAS; i++)     /* no previous def'n...make new one */
        if (!ch->alias[i].text[0]) {
            strcpy(ch->alias[i].alias, alias_ref);
            strcpy(ch->alias[i].text, alias_text);
            send_to_char("Ok.\n\r", ch);
            return;
        }

    sprintf(buf, "Sorry, you can only have %d aliases.\n\r", MAX_ALIAS);
    send_to_char(buf, ch);
    return;
}


void setup_infobar(struct descriptor_data *d);

void
cmd_reset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];

    if (!ch->desc || !ch->desc->term)
        return;

    if (!IS_SET(ch->specials.act, CFL_INFOBAR)) {
        write_to_descriptor(ch->desc->descriptor, VT_HOMECLR);
        sprintf(buf, VT_MARGSET, 1, GET_PAGELENGTH(ch));
        write_to_descriptor(ch->desc->descriptor, buf);
    } else {
        setup_infobar(ch->desc);
        display_infobar(ch->desc);
    }
}



void
cmd_beep(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH];
    CHAR_DATA *vict;

    argument = one_argument(argument, arg1, sizeof(arg1));

    if (!*arg1) {
        act("$b$n beeps loudly.", FALSE, ch, 0, 0, TO_ROOM);
        act("$bOk - BEEP", FALSE, ch, 0, 0, TO_CHAR);
    } else {
        if (!(vict = get_char_room_vis(ch, arg1))) {
            send_to_char("You don't see that person here.\n\r", ch);
            return;
        }
        act("$b$n beeps you.", FALSE, ch, 0, vict, TO_VICT);
        act("$bYou beep $M.", FALSE, ch, 0, vict, TO_CHAR);
    }
}


void
cmd_mail(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    /* char arg1[256]; unused -Morg */
    char name[256];
    char buf[MAX_INPUT_LENGTH + 10], buf2[512], all[MAX_STRING_LENGTH];
    char /* *tmstr, unused -Morg */ *tmp;
    /* long ct;  unused -Morg */
    int i;
    FILE *fp, *fp2;

//  if (!IS_SET (ch->in_room->room_flags, RFL_MAIL_ROOM) && !IS_IMMORTAL (ch))
//    {
//      send_to_char ("You cannot do that here.\n\r", ch);
//      return;
//    }

    strcpy(name, GET_NAME(ch));
    for (i = 0; name[i] != '\0'; i++)
        name[i] = LOWER(name[i]);

    sprintf(buf, "accountmail/%s/%s", ch->desc->player_info->name, name);

    if (!(fp = fopen(buf, "r"))) {
        send_to_char("You have no mail waiting.\n\r", ch);
        return;
    }


    /* get 2 lines */
    strcpy(buf, "");
    strcpy(all, "You only see one message at a time.\n\r--\n\r");
    fgets(buf, MAX_INPUT_LENGTH + 9, fp);
    strcat(all, buf);
    tmp = fgets(buf, MAX_INPUT_LENGTH + 9, fp);
    strcat(all, buf);
    strcat(all, "--\n\r");
    strcat(all, "You may have more mail waiting, type mail again.\n\r");
    page_string(ch->desc, all, 0);

    if (tmp == NULL) {
        fclose(fp);
        sprintf(buf, "accountmail/%s/%s", ch->desc->player_info->name, name);

        if (remove(buf))
            gamelog("error removing a mail file");
    } else {
        sprintf(buf2, "accountmail/%s/%s.new", ch->desc->player_info->name, name);
        if (!(fp2 = fopen(buf2, "w"))) {
            gamelog("error opening a mail temp file.");
            return;
        }

        /* write the rest to the new mail file */
        while (fgets(buf, MAX_INPUT_LENGTH + 9, fp))
            fputs(buf, fp2);

        fclose(fp);
        fclose(fp2);
        sprintf(buf, "accountmail/%s/%s", ch->desc->player_info->name, name);

        if (remove(buf)) {
            shhlog("ERROR removing someones mail file\n\r");
        }
        if (rename(buf2, buf));
        {
            shhlog("ERROR renaming someones mail file\n\r");
        }
    }
    return;
}

void
cmd_vis(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    // Rematerialize from invisibility
    if (IS_AFFECTED(ch, CHAR_AFF_INVISIBLE)) {
        if (!IS_IMMORTAL(ch))
            ch->specials.il = 0;
	appear(ch);
    }

    // Unhide
    if (IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
    }

    return;
}

void
remove_note_file(OBJ_DATA * obj)
{
    char filename[256];

    if (obj->obj_flags.value[1]) {
        sprintf(filename, "notes/note%d", obj->obj_flags.value[1]);
        remove(filename);
    }

    return;
}

void
cmd_junk(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *obj, *tmp;
    char buf[256];
    int damage = 0;
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];

    extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow,
                            sizeof(postHow));

    argument = one_argument(args, buf, sizeof(buf));

    if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
        send_to_char("You aren't carrying that item.\n\r", ch);
        return;
    }

    if ((IS_CORPSE(obj)) && (!IS_IMMORTAL(ch))) {
        send_to_char("Corpses cannot be junked, please wish up for help.\n\r", ch);
        return;
    }

    send_to_char("Ok.\n\r", ch);

    find_obj_keyword(obj, FIND_CHAR_ROOM | FIND_OBJ_INV, ch, buf, sizeof(buf));

    // build messages up
    if ((obj->obj_flags.type == ITEM_CONTAINER) && (obj->contains)) {
        sprintf(to_room, "@ discards ~%s, dropping its contents on the ground", buf);
        sprintf(to_char, "you discard ~%s, dropping its contents on the ground", buf);
    }
    // anything but a container
    else {
        sprintf(to_room, "@ discards ~%s", buf);
        sprintf(to_char, "you discard ~%s", buf);
    }

    // send the emote to the room, if the emote fails to parse, stop
    if (!send_to_char_and_room_parsed
        (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER))
        return;

    // dump the contents if it's a container with stuff in it
    if (obj->obj_flags.type == ITEM_CONTAINER && obj->contains) {
        dump_obj_contents(obj);
    }
    // clean up the note file
    if (obj->obj_flags.type == ITEM_NOTE)
        remove_note_file(obj);

    // if it's a trapped container
    if ((obj->obj_flags.type == ITEM_CONTAINER)
        && (IS_SET(obj->obj_flags.value[1], CONT_TRAPPED))) {
        damage = number(10, 25) + number(10, 25);
        // add damage done by stuff in the obj
        for (tmp = obj->contains; tmp; tmp = tmp->next_content) {
            /* Explosive stuff inside, set it off */
            if (tmp->obj_flags.type == ITEM_TOOL)
                damage += number(10, 25);
        }

        act("$p explodes!", FALSE, ch, obj, 0, TO_CHAR);
        act("$p explodes in $n's hands!", FALSE, ch, obj, 0, TO_ROOM);
        if (!generic_damage(ch, damage, 0, 0, number(10, 25) * number(1, 4))) {
            act("$n falls headlong, and dies before $e hits the " "ground.", FALSE, ch, obj, 0,
                TO_ROOM);
            {
                sprintf(buf, "%s %s%s%sjunked a trapped object and blew up in room %d, dying.",
                        GET_NAME(ch), !IS_NPC(ch) ? "(" : "", !IS_NPC(ch) ? ch->account : "",
                        !IS_NPC(ch) ? ") " : "", ch->in_room->number);

                if (!IS_NPC(ch)) {
                    struct time_info_data playing_time =
                        real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played,
                                         0);

                    if ((playing_time.day > 0 || playing_time.hours > 2
                         || (ch->player.dead && !strcmp(ch->player.dead, "rebirth")))
                        && !IS_SET(ch->specials.act, CFL_NO_DEATH)) {
                        if (ch->player.info[1])
                            free(ch->player.info[1]);
                        ch->player.info[1] = strdup(buf);
                    }

                    gamelog(buf);
                } else
                    shhlog(buf);

                die(ch);
            }
        }
    }
    extract_obj(obj);
}

void
stop_guarding(CHAR_DATA * ch)
{
/****
  stops the CH from guarding anyone else.
  stops the CH from guarding exits.
  stops the CH from guarding objects.
  ****/

    struct guard_type *another_guard, *temp_guard;
    CHAR_DATA *who_guarding;
    char buf[256];


    if (ch->specials.guarding) {
        if (!ch->specials.guarding->guards) {
            gamelog("WARNING! (!ch->specials.guarding->guards) in stop_guard!");
            return;
        }
        if (!ch->specials.guarding->guards->guard) {
            gamelog("WARNING! (!ch->specials.guarding->guards->guard) in stop_guard!");
            return;
        }
    }

    if (ch->specials.guarding) {
        who_guarding = NULL;
        if (ch != ch->specials.guarding) {
            if (ch->specials.guarding->guards->guard == ch) {
                who_guarding = ch->specials.guarding;
                temp_guard = ch->specials.guarding->guards;
                ch->specials.guarding->guards = temp_guard->next;
                free(temp_guard);
                ch->specials.guarding = (CHAR_DATA *) 0;
            } else {
                for (temp_guard = ch->specials.guarding->guards;
                     (temp_guard->next) && (temp_guard->next->guard != ch);
                     temp_guard = temp_guard->next);
                if (temp_guard->next) {
                    who_guarding = ch->specials.guarding;
                    another_guard = temp_guard->next;
                    temp_guard->next = another_guard->next;
                    free(another_guard);
                    ch->specials.guarding = (CHAR_DATA *) 0;
                }
            }
        }
        if (who_guarding) {
            act("You stop guarding $N.", TRUE, ch, 0, who_guarding, TO_CHAR);
            if (who_guarding->in_room == ch->in_room) {
                act("$n stops guarding $N.", TRUE, ch, 0, who_guarding, TO_NOTVICT);
                act("$n stops guarding you.", TRUE, ch, 0, who_guarding, TO_VICT);
            }


        };
        /* clear it one more time, just to be sure */
        ch->specials.guarding = (CHAR_DATA *) 0;
    }

    if (ch->specials.obj_guarding) {
        if (ch->specials.obj_guarding->guards->guard == ch) {
            temp_guard = ch->specials.obj_guarding->guards;
            ch->specials.obj_guarding->guards = temp_guard->next;
            free(temp_guard);
        } else {
            for (temp_guard = ch->specials.obj_guarding->guards; temp_guard->next->guard != ch;
                 temp_guard = temp_guard->next);

            another_guard = temp_guard->next;
            temp_guard->next = another_guard->next;
            free(another_guard);
        }
        act("You stop guarding $p.", TRUE, ch, ch->specials.obj_guarding, 0, TO_CHAR);
        act("$n stops guarding $p.", TRUE, ch, ch->specials.obj_guarding, 0, TO_NOTVICT);

        ch->specials.obj_guarding = (OBJ_DATA *) 0;
    }

    if (ch->specials.dir_guarding > -1) {
        if (ch->specials.dir_guarding == DIR_OUT) {
            send_to_char("You stop guarding the way out.\n\r", ch);
            act("$n stops guarding the way out.", FALSE, ch, 0, 0, TO_NOTVICT);
        } else {
            sprintf(buf, "You stop guarding the %s exit.\n\r", dirs[ch->specials.dir_guarding]);
            send_to_char(buf, ch);

            sprintf(buf, "$n stops guarding the %s exit.\n\r", dirs[ch->specials.dir_guarding]);
            act(buf, FALSE, ch, 0, 0, TO_ROOM);
        }
        ch->specials.dir_guarding = -1;
    }
    return;
}


/* This routine stops CH from guarding other CHs, objects, and exits. */
void
stop_exit_guard(CHAR_DATA * ch)
{
    char buf[256];              /* to store messages in            */

    if (ch->specials.dir_guarding == DIR_OUT) {
        send_to_char("You stop guarding the way out.\n\r", ch);
        act("$n stops guarding the way out.", FALSE, ch, 0, 0, TO_NOTVICT);
        ch->specials.dir_guarding = -1;
        return;
    }

    /* send message to the guard */
    sprintf(buf, "You stop guarding the %s exit.\n\r", dirs[ch->specials.dir_guarding]);
    send_to_char(buf, ch);

    /* send message to the room */
    sprintf(buf, "$n stops guarding the %s exit.\n\r", dirs[ch->specials.dir_guarding]);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    /* set the value on the guard */
    ch->specials.dir_guarding = -1;
}

void
stop_obj_guard(CHAR_DATA * ch)
{
    struct guard_type *temp_guard, *another_guard;

    if (ch->specials.obj_guarding->guards->guard == ch) {
        temp_guard = ch->specials.obj_guarding->guards;
        ch->specials.obj_guarding->guards = temp_guard->next;
        /* Either way temp_guard can now be freed since it should point to CH */
        free(temp_guard);
    } else {
        for (temp_guard = ch->specials.obj_guarding->guards; temp_guard->next->guard != ch;
             temp_guard = temp_guard->next);

        another_guard = temp_guard->next;
        temp_guard->next = another_guard->next;
        free(another_guard);
    }


    if ((ch->specials.obj_guarding->in_room)
        && (ch->specials.obj_guarding->in_room == ch->in_room)) {
        act("You stop guarding $p.", TRUE, ch, ch->specials.obj_guarding, 0, TO_CHAR);
        act("$n stops guarding $p.", TRUE, ch, ch->specials.obj_guarding, 0, TO_NOTVICT);
    }

    ch->specials.obj_guarding = (OBJ_DATA *) 0;
}

void
stop_ch_guard(CHAR_DATA * ch)
{
    CHAR_DATA *who_guarding = (CHAR_DATA *) 0;
    struct guard_type *another_guard = (struct guard_type *) 0;
    struct guard_type *temp_guard = (struct guard_type *) 0;

    if (!ch->specials.guarding) {
        gamelog("WARNING: stop_ch_guard, ch->specials.guarding is null.");
        return;
    };

    who_guarding = ch->specials.guarding;

    if (!who_guarding->guards) {
        ch->specials.guarding = (CHAR_DATA *) 0;
        gamelog("WARNING: stop_ch_guard, who_guarding->guards is null.");
        return;
    };

    if (who_guarding->guards->guard == ch) {
        temp_guard = who_guarding->guards;
        who_guarding->guards = temp_guard->next;

        free(temp_guard);
    } else {
        for (temp_guard = who_guarding->guards; temp_guard->next->guard != ch;
             temp_guard = temp_guard->next)
            /* walk through list till temp_guard->next is ch */ ;

        another_guard = temp_guard->next;
        temp_guard->next = another_guard->next;

        free(another_guard);
    };

    /* Send the messages */
    if (who_guarding) {
        act("You stop guarding $N.", TRUE, ch, 0, who_guarding, TO_CHAR);

        /* These should only be sent if they're in the same room */
        if (who_guarding->in_room == ch->in_room) {
            act("$n stops guarding $N.", TRUE, ch, 0, who_guarding, TO_NOTVICT);
            act("$n stops guarding you.", TRUE, ch, 0, who_guarding, TO_VICT);
        }
    }
    ch->specials.guarding = (CHAR_DATA *) 0;
    return;
}

void
stop_guard(CHAR_DATA * ch, bool showFailure)
{
    if (ch->specials.guarding) {
        if (!ch->specials.guarding->guards) {
            gamelog("WARNING! (!ch->specials.guarding->guards) in stop_guard!");
            return;
        }
        if (!ch->specials.guarding->guards->guard) {
            gamelog("WARNING! (!ch->specials.guarding->guards->guard) in stop_guard!");
            return;
        }
    }

    if (showFailure && !ch->specials.guarding && !ch->specials.obj_guarding
        && ch->specials.dir_guarding == -1) {
        cprintf(ch, "You aren't guarding anyone or anything at the moment.\n\r");
        return;
    }

    if (ch->specials.guarding)
        stop_ch_guard(ch);

    if (ch->specials.obj_guarding)
        stop_obj_guard(ch);

    if (ch->specials.dir_guarding > -1)
        stop_exit_guard(ch);

    return;
}

void
die_guard(CHAR_DATA * ch)
{
    struct guard_type *another_guard, *temp_guard;

    stop_guard(ch, 0);

    for (temp_guard = ch->guards; temp_guard; temp_guard = another_guard) {
        another_guard = temp_guard->next;
        stop_guard(temp_guard->guard, 0);
    }
}

void
add_exit_guard(CHAR_DATA * ch, int exit_to_guard)
{
    OBJ_DATA *wagon;
    char buf[256];              /* messages strings                    */

    if (exit_to_guard == DIR_OUT) {
        for (wagon = object_list; wagon; wagon = wagon->next) {
            if (!wagon)
                shhlog("Wagon not found in add_exit_gaurd, other.c");
            if ((wagon->obj_flags.type == ITEM_WAGON)
                && (wagon->obj_flags.value[0] == ch->in_room->number))
                break;
        }

        if (!wagon) {
            send_to_char("There's no way out to guard.\n\r", ch);
            return;
        }

        stop_guard(ch, 0);
        send_to_char("You begin guarding the way out.\n\r", ch);
        act("$n begins guarding the way out.", FALSE, ch, 0, 0, TO_NOTVICT);
        ch->specials.dir_guarding = DIR_OUT;
        return;
    }

    /* Make sure the direction is a valid one */
    if (!ch->in_room->direction[exit_to_guard])
        act("There is no exit from here in that direction.", FALSE, ch, 0, 0, TO_CHAR);

    else {
        /* make them stop guarding everything else */
        stop_guard(ch, 0);

        /* send message to the guard */
        sprintf(buf, "You begin guarding the %s exit.\n\r", dirs[exit_to_guard]);
        send_to_char(buf, ch);

        /* send message to the room */
        sprintf(buf, "$n begins guarding the %s exit.", dirs[exit_to_guard]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);

        ch->specials.dir_guarding = exit_to_guard;      /* set the value on the guard */

        /* set the flag on the exit */
        MUD_SET_BIT(ch->in_room->direction[exit_to_guard]->exit_info, EX_GUARDED);

        return;                 /* exit, you're done */
    }
    return;
}


void
add_obj_guard(CHAR_DATA * ch, OBJ_DATA * tar_obj)
{
    struct guard_type *new_guard;

    ch->specials.obj_guarding = tar_obj;

    CREATE(new_guard, struct guard_type, 1);

    new_guard->guard = ch;
    new_guard->next = tar_obj->guards;
    tar_obj->guards = new_guard;
    act("You begin guarding $p.", FALSE, ch, tar_obj, 0, TO_CHAR);
    act("$n begins guarding $p.", TRUE, ch, tar_obj, 0, TO_ROOM);
    return;
}


/* You should always call stop_guard(); before this function */
void
add_guard(CHAR_DATA * ch, CHAR_DATA * victim)
{
    struct guard_type *new_guard;

    ch->specials.guarding = victim;

    CREATE(new_guard, struct guard_type, 1);

    new_guard->guard = ch;
    new_guard->next = victim->guards;
    victim->guards = new_guard;

    act("You begin guarding $N.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n begins guarding you.", TRUE, ch, 0, victim, TO_VICT);
    act("$n begins guarding $N.", TRUE, ch, 0, victim, TO_NOTVICT);
    return;
}


void
cmd_guard(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *vict;            /* the victim if one is found          */
    OBJ_DATA *guard_obj;        /* the object if one is found          */
    char arg1[256];             /* to break up input string 'argument' */
    int dir = 0;                /* direction character is guarding     */

    /*get the first word typed */
    argument = one_argument(argument, arg1, sizeof(arg1));

    /* If nothing is typed return who they're guarding */
    /* if no argument, or if it's 'status' */
    if (!*arg1 || (*arg1 && !str_prefix(arg1, "status"))) {
        if ((ch->specials.guarding) && (ch->specials.guarding != ch)) {
            act("You are guarding $N.", TRUE, ch, 0, ch->specials.guarding, TO_CHAR);
        } else if (ch->specials.dir_guarding > -1) {
            cprintf(ch, "You are guarding the %s exit.\n\r", dirs[ch->specials.dir_guarding]);
        } else if (ch->specials.obj_guarding) {
            act("You are guarding $p.", TRUE, ch, ch->specials.obj_guarding, 0, TO_CHAR);
        } else {
            send_to_char("You aren't guarding anyone or anything at the" " moment.\n\r", ch);
            return;
        }
        send_to_char("To stop guarding, type: guard none\n\r", ch);
        return;
    }

    if (!str_prefix(arg1, "none")) {
        stop_guard(ch, 1);      /* make them un-guard other things */
        return;
    }

    /* No guarding if subduing */
    if (ch->specials.subduing) {
        send_to_char("Your hands are too full to guard anything.\n\r", ch);
        return;
    }


    /* end if character typed nothing */
    /* They're trying to guard an object */
    if ((guard_obj = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
        stop_guard(ch, 0);      /* make them un-guard other things */
        add_obj_guard(ch, guard_obj);   /* This sends all the message */
        return;                 /* exit */
    }

    /* They're trying to guard an exit */
    if (!(vict = get_char_room_vis(ch, arg1))) {
        /* attempt to guard exit */
        if (!strncasecmp(arg1, "north", strlen(arg1)))
            dir = DIR_NORTH;
        else if (!strncasecmp(arg1, "south", strlen(arg1)))
            dir = DIR_SOUTH;
        else if (!strncasecmp(arg1, "east", strlen(arg1)))
            dir = DIR_EAST;
        else if (!strncasecmp(arg1, "west", strlen(arg1)))
            dir = DIR_WEST;
        else if (!strncasecmp(arg1, "up", strlen(arg1)))
            dir = DIR_UP;
        else if (!strncasecmp(arg1, "down", strlen(arg1)))
            dir = DIR_DOWN;
        else if (!strncasecmp(arg1, "out", strlen(arg1)))
            dir = DIR_OUT;
        else {
            /* Target was not an exit, object, or character */
            act("Guard who?", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        /* Make them un-guard all other things */
        stop_guard(ch, 0);

        /* can't watch & guard a direction */
        if (is_char_watching_dir(ch, dir)) {
            stop_watching(ch, 0);
        }

        /* This sends all messages and does checks */
        add_exit_guard(ch, dir);
        return;
    }

    if (vict->specials.fighting) {
        act("$N is fighting right now!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    if (is_char_watching_char(ch, vict)) {
        stop_watching(ch, 0);
    }

    stop_guard(ch, 0);

    if (ch == vict)
        send_to_char("You begin guarding yourself.\n\r", ch);
    else
        add_guard(ch, vict);    /* This sends all the messages */

}                               /* end cmd_guard */

/* This procedure is should only be called by exit_guarded.  It contains
   all of the success/failure messages dependant on the values passed in */

void
exit_guarded_messages(CHAR_DATA * ch,   /* character challenging  */
                      CHAR_DATA * guard,        /* guard of the exit   */
                      int dir,  /* direction guarded      */
                      int cmd,  /* command used           */
                      int success)
{                               /* whether they succeeded *//* being exit_guarded_messages */
    char buf1[256];             /* for sending messages        */
    char buf2[256];             /* for sending messages        */
    char buf3[256];             /* for sending messages        */


    /* case statement breakdown of command used to challenge */
    switch (cmd) {
    case 0:                    /* North */
    case 1:                    /* West  */
    case 2:                    /* South */
    case 3:                    /* East  */
        {
            if (success) {
                sprintf(buf1, "$n blocks $N from going through the %s exit.", dirs[dir]);
                sprintf(buf2, "You block $N from going through the %s exit.", dirs[dir]);
                sprintf(buf3, "$n blocks you from going through the %s exit.", dirs[dir]);
            } else {
                sprintf(buf1, "$n tries to block $N from going %s, but fails.", dirs[dir]);
                sprintf(buf2, "You try to block $N from going %s, but fail.", dirs[dir]);
                sprintf(buf3, "$n tries to block you from going %s, but fails.", dirs[dir]);
            }
        }
        break;
    case 4:                    /* Up    */
    case 5:                    /* Down  */
        {
            if (success) {
                sprintf(buf1, "$n blocks $N from going %s.", dirs[dir]);
                sprintf(buf2, "You block $N from going %s.", dirs[dir]);
                sprintf(buf3, "$n blocks you from going %s.", dirs[dir]);
            } else {
                sprintf(buf1, "$n tries to block $N from going %s, but fails.", dirs[dir]);
                sprintf(buf2, "You try to block $N from going %s, but fail.", dirs[dir]);
                sprintf(buf3, "$n tries to block you from going %s, but fails.", dirs[dir]);
            }
        }
        break;


    case CMD_THROW:
        {                       /* Command throw is only checking against throwing characters,
                                 * not weapons, and it always fails -Nessalin 6/2/2001 */
            if (!ch->specials.subduing)
                return;
            sprintf(buf1, "$N throws %s %s where %s collides with $n before " "stopping.",
                    MSTR(ch->specials.subduing, short_descr), dirs[dir],
                    ((GET_SEX(ch->specials.subduing) ==
                      SEX_MALE) ? "he" : ((GET_SEX(ch->specials.subduing) ==
                                           SEX_FEMALE) ? "she" : "it")));
            sprintf(buf2, "You step into %s's path as $N throws %s %s.",
                    MSTR(ch->specials.subduing, short_descr),
                    ((GET_SEX(ch->specials.subduing) ==
                      SEX_MALE ? "he" : ((GET_SEX(ch->specials.subduing) == SEX_FEMALE) ? "she" :
                                         "it"))), dirs[dir]);
            sprintf(buf3, "You throw %s %s where %s collides with $n.",
                    MSTR(ch->specials.subduing, short_descr), dirs[dir],
                    (GET_SEX(ch->specials.subduing) ==
                     SEX_MALE ? "he" : ((GET_SEX(ch->specials.subduing) == SEX_FEMALE) ? "she" :
                                        "it")));

        }
        break;

    case CMD_CLOSE:
        {
            if (success) {
                sprintf(buf1, "$n prevents $N from closing the %s exit.", dirs[dir]);
                sprintf(buf2, "You stop $N from closing the %s exit.", dirs[dir]);
                sprintf(buf3, "$n prevents you from closing the %s exit.", dirs[dir]);
            } else {
                sprintf(buf1, "$n tries to prevent $N from closing the %s exit, " "but fails.",
                        dirs[dir]);
                sprintf(buf2, "You try to stop $N from closing the %s exit, but " "fail.",
                        dirs[dir]);
                sprintf(buf3, "$n tries to prevent you from closing the %s exit, " "but fails.",
                        dirs[dir]);
            }
        }
        break;

    case CMD_OPEN:
        {
            if (success) {
                sprintf(buf1, "$n prevents $N from opening the %s exit.", dirs[dir]);
                sprintf(buf2, "You stop $N from opening the %s exit.", dirs[dir]);
                sprintf(buf3, "$n prevents you from opening the %s exit.", dirs[dir]);
            } else {
                sprintf(buf1, "$n tries to prevent $N from opening the %s exit, " "but fails.\n\r",
                        dirs[dir]);
                sprintf(buf2, "You try to stop $N from opening the %s exit, but " "fail.\n\r",
                        dirs[dir]);
                sprintf(buf3, "$n tries to prevent you from opening the %s exit, " "but fails.\n\r",
                        dirs[dir]);
            }
        }
        break;

    case CMD_FLEE:
        {
            if (success) {
                sprintf(buf1, "$n prevents $N from fleeing out the %s exit.\n\r", dirs[dir]);
                sprintf(buf2, "You stop $N from fleeing out the %s exit.\n\r", dirs[dir]);
                sprintf(buf3, "$n prevents you from fleeing out the %s exit.\n\r", dirs[dir]);
            } else {
                sprintf(buf1,
                        "$n tries to prevent $N from fleeing out the %s exit, " "but fails.\n\r",
                        dirs[dir]);
                sprintf(buf2, "You try to stop $N from fleeing out the %s exit, but " "fail.\n\r",
                        dirs[dir]);
                sprintf(buf3,
                        "$n tries to prevent you from fleeing out the %s exit, " "but fails.\n\r",
                        dirs[dir]);
            }
        }
        break;

    case CMD_PICK:
        {
            if (success) {
                sprintf(buf1, "$n prevents $N from picking the %s exit.\n\r", dirs[dir]);
                sprintf(buf2, "You stop $N from picking the %s exit.\n\r", dirs[dir]);
                sprintf(buf3, "$n prevents you from picking the %s exit.\n\r", dirs[dir]);
            } else {
                sprintf(buf1, "$n tries to prevent $N from picking the %s exit, " "but fails.\n\r",
                        dirs[dir]);
                sprintf(buf2, "You try to stop $N from picking the %s exit, but " "fail.\n\r",
                        dirs[dir]);
                sprintf(buf3, "$n tries to prevent you from picking the %s exit, " "but fails.\n\r",
                        dirs[dir]);
            }
        }
        break;

    case CMD_TRAP:
        {
            if (success) {
                sprintf(buf1, "$n prevents $N from traping the %s exit.\n\r", dirs[dir]);
                sprintf(buf2, "You stop $N from traping the %s exit.\n\r", dirs[dir]);
                sprintf(buf3, "$n prevents you from traping the %s exit.\n\r", dirs[dir]);
            } else {
                sprintf(buf1, "$n tries to prevent $N from trapping the %s exit, " "but fails.\n\r",
                        dirs[dir]);
                sprintf(buf2, "You try to stop $N from trapping the %s exit, but " "fail.\n\r",
                        dirs[dir]);
                sprintf(buf3,
                        "$n tries to prevent you from trapping the %s exit, " "but fails.\n\r",
                        dirs[dir]);
            }
        }
        break;

    default:
        {
            if (success) {
            } else {
                sprintf(buf1, "$n tries to guard the %s exit against $N, " "but fails.\n\r",
                        dirs[dir]);
                sprintf(buf2, "You try to guard the %s exit against $N, " "fail.\n\r", dirs[dir]);
                sprintf(buf3, "$n tries to guard the %s exit against you, " "but fails.\n\r",
                        dirs[dir]);
            }
        }
        break;

    }                           /* end case statement */

    /* send the messages */
    act(buf1, FALSE, guard, 0, ch, TO_NOTVICT);
    act(buf2, FALSE, guard, 0, ch, TO_CHAR);
    act(buf3, FALSE, guard, 0, ch, TO_VICT);

    /* exit procedure */
    return;

}                               /* end exit_guarded_messages */

/* Call this procedure when an action is performed which should be blocked
   by the guard skill.  This procedure handles all messages, so the calling
   procedure doesn't need to account for failure or success messages */

int
exit_guarded(CHAR_DATA * ch,    /* character challenging exit        */
             int dir,           /* direction being challeneged       */
             int cmd)
{                               /* command used to challenge with    *//* begin exit_guarded */
    CHAR_DATA *guard;           /* to hold value of possible guard   */
    int guard_found = 0;        /* so the program knows what's found */
    int skill_perc = 0;         /* adjusted skill of players         */

    /* Exit immediately if the exit isn't even guarded */
    if (!(IS_SET((ch->in_room->direction[dir]->exit_info), EX_GUARDED)))
        return FALSE;

    /* Search the room to see who is guarding the exit */
    for (guard = ch->in_room->people; guard; guard = guard->next_in_room) {     /* being loop searching room for guard */
        if ((guard->specials.dir_guarding == dir) &&    /* guarding right exit   */
            (guard != ch) &&    /* guard isn't character */
            (CAN_SEE(guard, ch)) &&     /* guard sees character  */
            (GET_POS(guard) >= POSITION_STANDING)) {    /* guard is standing     *//* begin found guard */

            guard_found = 1;

            /* cmd throw is an auto failure, it only checks for thrown
             * characters, not weapons 6/2/2001 -nessalin */
            if (cmd == CMD_THROW) {
                exit_guarded_messages(ch, guard, dir, cmd, 0);
                return TRUE;
            }

            if (IS_IN_SAME_TRIBE(ch, guard)) {  /* don't stop same tribe */
                act("You nod at $N.", FALSE, guard, 0, ch, TO_CHAR);
                act("$n nods at you.", FALSE, guard, 0, ch, TO_VICT);
                act("$N nods at $n.", FALSE, guard, 0, ch, TO_NOTVICT);
            } else {            /* Begin not-same clan */
                /* This bit insures that everyone has a 15% chance, even if they 
                 * don't possess the skill at all. */

                /* You can't pass them if you're walking */
                if (GET_SPEED(ch) == SPEED_WALKING) {
                    exit_guarded_messages(ch, guard, dir, cmd, 1);
                    act("You must be running or sneaking to get past a guard.", FALSE, ch, 0, 0,
                        TO_CHAR);
                    return TRUE;
                }

                skill_perc =
                    MAX(15, guard->skills[SKILL_GUARD] ? guard->skills[SKILL_GUARD]->learned : 0);

                /* if they're watching a character, but not this character */
                if (is_char_watching_any_char(guard)
                    && !is_char_actively_watching_char(guard, ch)) {
                    skill_perc -= get_watched_penalty(guard);
                }

                /* Easier to run past someone than to walk past them.  Note that
                 * this only applies if they're trying to walk through an exit  */
                if (cmd == 0 || cmd == 1 || cmd == 2 || cmd == 3 || cmd == 4 || cmd == 5)
                    if (GET_SPEED(ch) == SPEED_RUNNING)
                        skill_perc -= 5;

                /* This checks to see if the player's adjusted skill succeeded. */

                if (skill_success(guard, ch, skill_perc)) {     /* success! */

                    /* send success messages */
                    exit_guarded_messages(ch, guard, dir, cmd, 1);

                    /* some lag for the aggressor so they don't just spam */
                    WAIT_STATE(ch, 3);

                    /* Have the procedure exit out here.  Remember, only one guard
                     * has to be successful in blocking a command.  There's no reason
                     * to check the rest of the room */
                    return TRUE;
                }
                /* end success */
                else {          /* failure... */

                    /* send failure messages */
                    exit_guarded_messages(ch, guard, dir, cmd, 0);

                    /* Note: The procedure doesn't exit here because the code needs
                     * to check ALL guards in the room.  Only if all of them fail 
                     * can the challenger successfully complete their task */
                }               /* end failure */
            }                   /* end not-same-clan */
        }                       /* end found guard */
    }                           /* end for loop searching for guard */

/* You can't say that no guard was found, if they didn't see the person, you
 * could end up here.  That doesn't mean the person has given up on guarding.
 *  if (!guard_found)
 *      // Since no one was found to be guarding the exit, go ahead and remove
 *      // the EX_GUARDED bit to keep this search from being performed repeatedly 
 *      REMOVE_BIT(ch->in_room->direction[dir]->exit_info, EX_GUARDED);
 */

    /* no guard was found */
    return FALSE;

}                               /* end exit_guarded */



// A kludge for blocking cmd_leave/the mythical 'out' exit 
// -Nessalin 4/17/2004
int
out_guarded(CHAR_DATA * ch)
{                               /* character challenging exit        *//* begin out_guarded */
    CHAR_DATA *guard;           /* to hold value of possible guard   */
    int guard_found = 0;        /* so the program knows what's found */
    int skill_perc = 0;         /* adjusted skill of players         */

    /* Search the room to see who is guarding the exit */
    for (guard = ch->in_room->people; guard; guard = guard->next_in_room) {
        if ((guard->specials.dir_guarding == DIR_OUT) && (guard != ch) && (CAN_SEE(guard, ch))
            && (GET_POS(guard) >= POSITION_STANDING)) {
            guard_found = 1;

            /* cmd throw is an auto failure, it only checks for thrown
             * characters, not weapons 6/2/2001 -nessalin */
            // I don't believe you can throw out of wagons, yet.
            // -Nessalin 4/17/2004
            //      if (cmd == CMD_THROW) {
            //        exit_guarded_messages(ch, guard, dir, cmd, 0);
            //        return TRUE;
            //      }

            if (IS_IN_SAME_TRIBE(ch, guard)) {  /* don't stop same tribe */
                act("You nod at $N.", FALSE, guard, 0, ch, TO_CHAR);
                act("$n nods at you.", FALSE, guard, 0, ch, TO_VICT);
                act("$N nods at $n.", FALSE, guard, 0, ch, TO_NOTVICT);
            } else {            /* Begin not-same clan */
                if (GET_SPEED(ch) == SPEED_WALKING) {
                    act("$n blocks $N from leaving.", FALSE, guard, 0, ch, TO_NOTVICT);
                    act("You block $N from leaving.", FALSE, guard, 0, ch, TO_CHAR);
                    act("$n blocks you from leaving.", FALSE, guard, 0, ch, TO_VICT);
                    act("You must be running or sneaking to get past a guard.", FALSE, ch, 0, 0,
                        TO_CHAR);
                    return TRUE;
                }

                skill_perc =
                    MAX(15, guard->skills[SKILL_GUARD] ? guard->skills[SKILL_GUARD]->learned : 0);

                if (GET_SPEED(ch) == SPEED_RUNNING)
                    skill_perc -= 5;

                if (skill_success(guard, ch, skill_perc)) {     /* success! */
                    act("$n blocks $N from leaving.", FALSE, guard, 0, ch, TO_NOTVICT);
                    act("You block $N from leaving.", FALSE, guard, 0, ch, TO_CHAR);
                    act("$n blocks you from leaving.", FALSE, guard, 0, ch, TO_VICT);

                    WAIT_STATE(ch, 3);

                    return TRUE;
                } else {        /* failure... */
                    act("$n tries to block $N from leaving, but fails.", FALSE, guard, 0, ch,
                        TO_NOTVICT);
                    act("You try to block block $N from leaving, but fails..", FALSE, guard, 0, ch,
                        TO_CHAR);
                    act("$n tries to block you from leaving, but fails.", FALSE, guard, 0, ch,
                        TO_VICT);

                }               /* end failure */
            }                   /* end not-same-clan */
        }                       /* end found guard */
    }                           /* end for loop searching for guard */

    /* no guard was found */
    return FALSE;
}                               /* end exit_guarded */



void
object_guarded_messages(CHAR_DATA * ch, /* character challenging  */
                        CHAR_DATA * guard,      /* guard of the object    */
                        OBJ_DATA * g_obj,       /* object guarded         */
                        int cmd,        /* command used           */
                        int success)
{                               /* whether they succeeded *//* being exit_guarded_messages */
    /* case statement breakdown of command used to challenge */
    switch (cmd) {
    case CMD_GET:              /* Get */
        if (success) {
            act("$n blocks $N from picking up $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from picking up $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from picking up $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from picking up $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from picking up $p, but fail.", FALSE, guard, g_obj, ch,
                TO_CHAR);
            act("$n tries to block you from picking up $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;

    case CMD_STEAL:            /* Steal  */
        if (success) {
            act("$n blocks $N from picking up $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from picking up $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from picking up $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from picking up $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from picking up $p, but fail.", FALSE, guard, g_obj, ch,
                TO_CHAR);
            act("$n tries to block you from picking up $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;

    case CMD_ENTER:            /* Enter  */
        if (success) {
            act("$n blocks $N from entering $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from entering $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from entering $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from entering $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from entering $p, but fail.", FALSE, guard, g_obj, ch,
                TO_CHAR);
            act("$n tries to block you from entering $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;

    case CMD_POISONING:        /* Poison */
        if (success) {
            act("$n blocks $N from poisoning $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from poisoning $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from poisoning $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from poisoning $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from poisoning $p, but fail.", FALSE, guard, g_obj, ch,
                TO_CHAR);
            act("$n tries to block you from poisoning $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;

    case CMD_OPEN:             /* Open */
        if (success) {
            act("$n blocks $N from opening $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from opening $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from opening $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from opening $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from opening $p, but fail.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n tries to block you from opening $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;

    case CMD_CLOSE:            /* Close */
        if (success) {
            act("$n blocks $N from closing $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from closing $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from closing $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from closing $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from closing $p, but fail.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n tries to block you from closing $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;

    case CMD_UNLOCK:           /* Unlock */
        if (success) {
            act("$n blocks $N from unlocking $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from unlocking $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from unlocking $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from unlocking $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from unlocking $p, but fail.", FALSE, guard, g_obj, ch,
                TO_CHAR);
            act("$n tries to block you from unlocking $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;

    case CMD_BREAK:            /* break */
        if (success) {
            act("$n blocks $N from destroying $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from destroying $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from destroying $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from destroying $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from destroying $p, but fail.", FALSE, guard, g_obj, ch,
                TO_CHAR);
            act("$n tries to block you from destroying $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;

    case CMD_LOCK:             /* lock */
        if (success) {
            act("$n blocks $N from locking $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from locking $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from locking $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from locking $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from locking $p, but fail.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n tries to block you from locking $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;
    case CMD_DRINK:            /* Drink  */
    case CMD_SIP:              /* Sip    */
        if (success) {
            act("$n blocks $N from drinking from $p.", FALSE, guard, g_obj, ch, TO_NOTVICT);
            act("You block $N from drinking from $p.", FALSE, guard, g_obj, ch, TO_CHAR);
            act("$n blocks you from drinking from $p.", FALSE, guard, g_obj, ch, TO_VICT);
        } else {
            act("$n tries to block $N from drinking from $p, but fails.", FALSE, guard, g_obj, ch,
                TO_NOTVICT);
            act("You try to block $N from drinking from $p, but fail.", FALSE, guard, g_obj, ch,
                TO_CHAR);
            act("$n tries to block you from drinking from $p, but fails.", FALSE, guard, g_obj, ch,
                TO_VICT);
        }
        break;

    default:
        break;
    }                           /* end case statement */

    /* exit procedure */
    return;

}                               /* end exit_guarded_messages */

/* Call this procedure when an action is performed which should be blocked
   by the guard skill.  This procedure handles all messages, so the calling
   procedure doesn't need to account for failure or success messages */

int
obj_guarded(CHAR_DATA * ch,     /* character challenging object      */
            OBJ_DATA * g_obj,   /* object being challeneged          */
            int cmd)
{                               /* command used to challenge with    *//* begin exit_guarded */
    CHAR_DATA *guard;           /* to hold value of possible guard   */
    struct guard_type *curr_guard;
    struct guard_type *next_guard;
    int skill_perc = 0;         /* adjusted skill of players         */

    if (g_obj->guards)
        /* Search the room to see who is guarding the object */
        for (curr_guard = g_obj->guards; curr_guard; curr_guard = next_guard) { /* being loop searching room for guard */
            next_guard = curr_guard->next;
            guard = curr_guard->guard;

            if ((guard != ch) &&        /* guard isn't character */
                (CAN_SEE(guard, ch)) && /* guard sees character  */
                (GET_POS(guard) >= POSITION_STANDING)) {        /* guard is standing *//* begin found guard */

                if (guard->in_room != g_obj->in_room)
                    stop_guard(guard, 0);
                else if (IS_IN_SAME_TRIBE(ch, guard)) { /* don't stop same tribe */
                    act("You nod at $N.", FALSE, guard, 0, ch, TO_CHAR);
                    act("$n nods at you.", FALSE, guard, 0, ch, TO_VICT);
                    act("$N nods at $n.", FALSE, guard, 0, ch, TO_NOTVICT);
                } else {        /* Begin not-same clan */
                    /* This bit insures that everyone has a 15% chance, even if they 
                     * don't possess the skill at all. */

                    skill_perc =
                        MAX(15,
                            guard->skills[SKILL_GUARD] ? guard->skills[SKILL_GUARD]->learned : 0);

                    /* This checks to see if the player's adjusted skill succeeded. */
                    if (skill_success(guard, ch, skill_perc)) { /* success! */

                        /* send success messages */
                        object_guarded_messages(ch, guard, g_obj, cmd, 1);

                        /* Have the procedure exit out here.  Remember, only one 
                         * guard has to be successful in blocking a command.  
                         * There's no reason to check the rest of them */
                        return TRUE;
                    }
                    /* end success */
                    else {      /* failure... */
                        /* send failure messages */
                        object_guarded_messages(ch, guard, g_obj, cmd, 0);

                        /* Note: The procedure doesn't exit here because the code 
                         * needs to check ALL guards in the room.  Only if all of 
                         * them fail can the challenger successfully complete their 
                         * task */
                    }           /* end failure */

                }               /* end not-same-clan */

            }
            /* end found guard */
        }

    /* end for loop searching for guard */
    /* no guard was found */
    return FALSE;

}                               /* end exit_guarded */

void
cmd_teach(CHAR_DATA * pcdCh, const char *pszArg, Command cmd, int count)
{
    char szBuf[256], szMsg[256];
    CHAR_DATA *pcdStudent;
    int nSkill, iLearn, iMod, maxSkill;

    /* get student */
    pszArg = one_argument(pszArg, szBuf, sizeof(szBuf));
    if (!(pcdStudent = get_char_room_vis(pcdCh, szBuf))) {
        send_to_char("Teach who?\n\r", pcdCh);
        return;
    }
    if (pcdStudent == pcdCh) {
        send_to_char("How can you teach yourself?\n\r", pcdCh);
        return;
    }

    if (!AWAKE(pcdStudent)) {
        act("How rude, $E's asleep!", FALSE, pcdCh, NULL, pcdStudent, TO_CHAR);
        return;
    }

    /* get skill */
    strcpy(szBuf, pszArg);
    if (!strlen(szBuf) || *szBuf == '\n') {
        send_to_char("Teach what skill?\n\r", pcdCh);
        return;
    }

    for (nSkill = 1; nSkill < MAX_SKILLS && stricmp(skill_name[nSkill], szBuf); nSkill++);
    if (nSkill == MAX_SKILLS) {
        send_to_char("Teach what skill?\n\r", pcdCh);
        return;
    }

    /* teacher has skill? */
    if (!pcdCh->skills[nSkill]) {
        send_to_char("You don't know anything about that.\n\r", pcdCh);
        return;
    }

    /* send msg to teacher and others */
    switch (skill[nSkill].sk_class) {
    case CLASS_LANG:
        send_to_char("A language is taught by speaking it.", pcdCh);
        return;
    case CLASS_REACH:
    case CLASS_MAGICK:
        strcpy(szBuf, "casting of");
        break;
    default:
        strcpy(szBuf, "skill of");
    }

    /* send to monitors */
    sprintf(szMsg, "%s instructs %s in the %s '%s'.\n\r", MSTR(pcdCh, name), MSTR(pcdStudent, name),
            szBuf, skill_name[nSkill]);
    send_to_monitor(pcdCh, szMsg, MONITOR_OTHER);

    sprintf(szMsg, "You instruct %s in the %s '%s'.\n\r", PERS(pcdCh, pcdStudent), szBuf,
            skill_name[nSkill]);
    send_to_char(szMsg, pcdCh);
    sprintf(szMsg, "%s instructs you in the %s '%s'.\n\r", PERS(pcdStudent, pcdCh), szBuf,
            skill_name[nSkill]);
    send_to_char(szMsg, pcdStudent);
/* commenting out for now, since it's OOC jarring for other viewers
   If it get abused, we can slap it back in. -San
  sprintf (szMsg, "%s instructs %s in the %s '%s'.\n\r",
	   MSTR (pcdCh, short_descr), MSTR (pcdStudent, short_descr),
	   szBuf, skill_name[nSkill]);
  send_to_room_except_two (szMsg, pcdCh->in_room, pcdCh, pcdStudent); */

    /* student understands language? */
    iMod = GET_SPOKEN_LANGUAGE(pcdCh);
    if (!pcdStudent->skills[iMod] || !pcdCh->skills[iMod]
        || ((pcdCh->skills[iMod]->learned + pcdStudent->skills[iMod]->learned) / 2 < 60)) {
        send_to_char("You can't understand enough of their language " "to learn anything.\n\r",
                     pcdStudent);
        return;
    }
    // Can't teach reach skills to non-magickers
    if (skill[nSkill].sk_class == CLASS_REACH) {
        // Non-magickers can't learn about reaches ever
        if (!is_magicker(pcdStudent)) {
            cprintf(pcdStudent,
                    "You don't have the necessary background to learn anything about" " that.\n\r");
            return;
        }
        // If they haven't learned about the skill before
        else if (!has_skill(pcdStudent, nSkill)) {
            // Teacher needs 50%+ in skill, and pass learn check
            if (pcdCh->skills[nSkill]->learned > 50
                && number(1, 100) < wis_app[GET_WIS(pcdStudent)].learn) {
                init_skill(pcdStudent, nSkill, 1);
                set_skill_taught(pcdStudent, nSkill);
            }
            // either not smart enough or not a good enough teacher
            else {
                cprintf(pcdStudent, "You can't figure out what they're talking about.\n\r");
            }
            return;
        }
        // otherwise they already know something about the reach, proceed
    }

    /* student has skill? */
    if (!pcdStudent->skills[nSkill]
        && !IS_SET(pcdStudent->specials.act, CFL_NEW_SKILL)) {
        send_to_char("You don't have the necessary background to learn " "anything about that.\n\r",
                     pcdStudent);
        return;
    } else if (!pcdStudent->skills[nSkill]
               && IS_SET(pcdStudent->specials.act, CFL_NEW_SKILL)) {
        REMOVE_BIT(pcdStudent->specials.act, CFL_NEW_SKILL);
        init_skill(pcdStudent, nSkill, 1);

        set_skill_taught(pcdStudent, nSkill);
    }

    /* check student and teacher guilds match */
    if (skill[nSkill].sk_class == CLASS_MAGICK) {
        if (GET_GUILD(pcdCh) != GET_GUILD(pcdStudent)) {
            send_to_char("The spellcasting technique doesn't make any sense " "to you.\n\r",
                         pcdStudent);
            return;
        }
    } else {
        switch (nSkill) {
        case SKILL_SNEAK:
        case SKILL_HIDE:
        case SKILL_HUNT:
            if (GET_GUILD(pcdCh) == GUILD_RANGER
                && (GET_GUILD(pcdStudent) == GUILD_BURGLAR
                    || GET_GUILD(pcdStudent) == GUILD_PICKPOCKET
                    || GET_GUILD(pcdStudent) == GUILD_ASSASSIN)) {
                send_to_char("You lose interest, as the technique deals "
                             "only with the wilderness.\n\r", pcdStudent);
                return;
            }
            if (GET_GUILD(pcdStudent) == GUILD_RANGER
                && (GET_GUILD(pcdCh) == GUILD_BURGLAR || GET_GUILD(pcdCh) == GUILD_PICKPOCKET
                    || GET_GUILD(pcdCh) == GUILD_ASSASSIN)) {
                send_to_char("You lose interest, as the technique deals "
                             "only with the city streets.\n\r", pcdStudent);
                return;
            }
            break;
        default:
            break;
        }
    }

    /* teach knows more than student? */
    if (pcdStudent->skills[nSkill]->learned + 20 >= pcdCh->skills[nSkill]->learned) {
        send_to_char("They don't seem to know anything more than you.\n\r", pcdStudent);
        return;
    }

    maxSkill = get_max_skill(pcdStudent, nSkill);

    /* gain_skill */
    iLearn =
        maxSkill - pcdStudent->skills[nSkill]->learned + wis_app[GET_WIS(pcdCh)].learn +
        wis_app[GET_WIS(pcdStudent)].learn + pcdCh->skills[nSkill]->learned -
        pcdStudent->skills[nSkill]->learned;

    if (number(1, 100) < iLearn) {
        if (pcdStudent->skills[nSkill]->learned < maxSkill / 4)
            iMod = number(3, 5);
        else
            iMod = 1;

        gain_skill(pcdStudent, nSkill, iMod);
    }
}

void
cmd_call(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256];
    int ct;

    CHAR_DATA *guard;

    if (!GET_TRIBE(ch)) {
        send_to_char("You cannot call anyone to service.\n\r", ch);
        return;
    }

    if ((GET_GUILD(ch) == GUILD_TEMPLAR) || (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
        || (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)) {
        for (ct = 0; ct < MAX_CITIES; ct++)
            if (IS_TRIBE(ch, city[ct].tribe))
                break;
        if (ct == MAX_CITIES) {
            send_to_char("You are not a Templar.\n\r", ch);
            return;
        }
    } else {
        if (GET_RACE(ch) != RACE_GITH) {
            send_to_char("You have no power to call anyone.\n\r", ch);
            return;
        }
    }

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!(guard = get_char_room_vis(ch, arg1))) {
        if (GET_RACE(ch) == RACE_GITH)
            cprintf(ch, "Who?\n");
        else
            cprintf(ch, "You do not see that soldier here.\n");
        return;
    }

    if ((guard->master == ch) && is_charmed(guard)) {
      act("$N is already at your command.", FALSE, ch, 0, guard, TO_CHAR);
      return;
    } else {                    /*  Kel  */
      if ((guard->master) && is_charmed(guard)) {
        act("$N is already under someone else's command.", FALSE, ch, 0, guard, TO_CHAR);
        return;
      }
    }

    if (GET_RACE(ch) == RACE_GITH && (!IS_NPC(guard) || !IS_IN_SAME_TRIBE(guard, ch))) {
        if (!IS_IN_SAME_TRIBE(guard, ch))
            act("$n glares at $N, but $E looks away.", FALSE, ch, 0, guard, TO_NOTVICT);
        act("You glare at $N, but $E looks away.", FALSE, ch, 0, guard, TO_CHAR);
        act("$n glares at you, but look away.", FALSE, ch, 0, guard, TO_VICT);
        return;
    } else {
        if (!IS_IN_SAME_TRIBE(guard, ch)
            || !IS_NPC(guard) || GET_GUILD(guard) != GUILD_WARRIOR) {
            act("$n asks $N for assistance, but $E refuses.", FALSE, ch, 0, guard, TO_NOTVICT);
            act("You ask $N for assistance, but $E refuses.", FALSE, ch, 0, guard, TO_CHAR);
            act("$n asks you for assistance, and you glare at $m strangely.", FALSE, ch, 0, guard,
                TO_VICT);
            return;
        }
    }

    if (ch->master == guard)
        stop_follower(ch);
    if (guard->master != ch) {
        if (guard->master)
            stop_follower(guard);
        add_follower(guard, ch);
    }

    MUD_SET_BIT(guard->specials.affected_by, CHAR_AFF_CHARM);

    if (GET_RACE(ch) == RACE_GITH) {
        act("$n glares at $N, and $E sulks to $s side.", FALSE, ch, 0, guard, TO_NOTVICT);
        act("You glare at $N and $E sulks to your side.", FALSE, ch, 0, guard, TO_CHAR);
        act("$n glares at you fiercely, you sulk to $s side.", FALSE, ch, 0, guard, TO_VICT);
    } else {
        act("$n calls to $N for aid, and $E strides to $s side.", FALSE, ch, 0, guard, TO_NOTVICT);
        act("$N is now at your service.", FALSE, ch, 0, guard, TO_CHAR);
        act("$n calls to you for aid.", FALSE, ch, 0, guard, TO_VICT);
    }
}


void
cmd_relieve(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256];
    CHAR_DATA *guard;

    argument = one_argument(argument, arg1, sizeof(arg1));

    if (!strcmp(arg1, "all") || !strcmp(arg1, "followers")) {
        for (guard = ch->in_room->people; guard; guard = guard->next_in_room) {
            if (guard->master == ch && 
                is_charmed(guard) &&
                IS_NPC(guard)) {
                REMOVE_BIT(guard->specials.act, CHAR_AFF_CHARM);
                stop_follower(guard);

                act("You relieve $N from $S duty.", FALSE, ch, 0, guard, TO_CHAR);
                act("$n relieves you from your duty.", FALSE, ch, 0, guard, TO_VICT);
            } else if (ch != guard) {
                act("$n relieves all $s followers from duty.", FALSE, ch, 0, guard, TO_VICT);
            }
        }

        return;
    }


    if (!(guard = get_char_room_vis(ch, arg1))) {
        cprintf(ch, "You do not see that soldier here.\n");
        return;
    }

    if (guard->master != ch || 
        !is_charmed(guard) ||
        !IS_IN_SAME_TRIBE(guard, ch) || 
        !IS_NPC(guard)) {
        act("$N is not a soldier under your command.", FALSE, ch, 0, guard, TO_CHAR);
        return;
    }

    REMOVE_BIT(guard->specials.act, CHAR_AFF_CHARM);
    stop_follower(guard);

    act("$n relieves $N from $S duty.", FALSE, ch, 0, guard, TO_NOTVICT);
    act("You relieve $N from $S duty.", FALSE, ch, 0, guard, TO_CHAR);
    act("$n relieves you from your duty.", FALSE, ch, 0, guard, TO_VICT);
}

void
cmd_incriminate(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], arg2[256], arg3[256], buf[256];
    int ct, hours, crime = 0;
    long permflags = 0;
    CHAR_DATA *victim;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (!GET_TRIBE(ch)
        || ((GET_GUILD(ch) != GUILD_TEMPLAR) && (GET_GUILD(ch) != GUILD_LIRATHU_TEMPLAR)
            && (GET_GUILD(ch) != GUILD_JIHAE_TEMPLAR))) {
        cprintf(ch, "You are not a Templar.\n");
        return;
    }

    for (ct = 0; ct < MAX_CITIES; ct++)
        if (IS_TRIBE(ch, city[ct].tribe))
            break;

    if (ct == MAX_CITIES) {
        cprintf(ch, "You are not a Templar.\n");
        return;
    }

    if (room_in_city(ch->in_room) != ct) {
        cprintf(ch, "You cannot incriminate anyone here.\n");
        return;
    }

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!*arg1) {
        send_to_char("Syntax is: incriminate <character> <duration> [<crime>]\n\r", ch);
        send_to_char("Possible crimes are:\n\r", ch);
        for (crime = 0; crime <= MAX_CRIMES; crime++) {
            sprintf(buf, "[%d] %s\n\r", crime, Crime[crime]);
            send_to_char(buf, ch);
        }

        return;
    }
    if (!(victim = get_char_room_vis(ch, arg1))) {
        cprintf(ch, "You do not see that person here.\n");
        return;
    }

    if (IS_IN_SAME_TRIBE(victim, ch)) {
        cprintf(ch, "You cannot incriminate that person.\n");
        return;
    }

    if (IS_SET(victim->specials.act, city[ct].c_flag)) {
        act("$N is already a criminal.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    /* Get duration of crim_flag */
    argument = one_argument(argument, arg2, sizeof(arg2));
    if (!*arg2) {
        send_to_char("Duration must be between 1 - 10 hours, or -1 for permanent.\n\r", ch);
        return;
    } else {
        hours = atoi(arg2);

        if (hours == -1) {      /* Permanent flag */
            /* This maintains a hidden edesc on the character for permanent
             * crimflags.  Tiernan 6/8/2004 */
            if (get_char_extra_desc_value(victim, "[CRIM_FLAGS]", buf, 100))
                permflags = atol(buf);
            MUD_SET_BIT(victim->specials.act, city[ct].c_flag);
            MUD_SET_BIT(permflags, city[ct].c_flag);
            sprintf(buf, "%ld", permflags);
            set_char_extra_desc_value(victim, "[CRIM_FLAGS]", buf);
        } else if ((hours < 1) || (hours > 10)) {     /* Temporary flag */
            send_to_char("Duration must be between 1 - 10 hours, or -1 for permanent.\n\r", ch);
            return;
        }

        argument = one_argument(argument, arg3, sizeof(arg3));
        if (!*arg3) {
            crime = CRIME_UNKNOWN;
        } else {
            crime = atoi(arg3);
            if (crime < 0 || crime > MAX_CRIMES)
                crime = CRIME_UNKNOWN;
        }

        if (hours > 0) {
            af.type = TYPE_CRIMINAL;
            af.location = CHAR_APPLY_CFLAGS;
            af.duration = hours;
            af.modifier = city[ct].c_flag;
            af.bitvector = 0;
            af.power = crime;
            affect_to_char(victim, &af);
        }
    }

    act("$n points a finger at $N, and gestures for nearby guards.", FALSE, ch, 0, victim,
        TO_NOTVICT);
    act("You point a finger at $N, and gesture for nearby guards.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n points a finger at you, and gestures for nearby guards.", FALSE, ch, 0, victim,
        TO_VICT);
}


void
cmd_pardon(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], arg2[256];
    int ct = 0;
    char buf[256];
    long permflags = 0;
    struct affected_type *p;

    CHAR_DATA *victim;

    if ((!GET_TRIBE(ch)
         || (GET_GUILD(ch) != GUILD_TEMPLAR && GET_GUILD(ch) != GUILD_LIRATHU_TEMPLAR
             && GET_GUILD(ch) != GUILD_JIHAE_TEMPLAR)) && !IS_IMMORTAL(ch)) {
        cprintf(ch, "You are not a Templar.\n");
        return;
    }

    if (!IS_IMMORTAL(ch)) {
        for (ct = 0; ct < MAX_CITIES; ct++)
            if (IS_TRIBE(ch, city[ct].tribe))
                break;

        if (ct == MAX_CITIES) {
            cprintf(ch, "You are not a Templar.\n");
            return;
        }
    }

    if (!IS_IMMORTAL(ch) && room_in_city(ch->in_room) != ct) {
        cprintf(ch, "You cannot pardon anyone here.\n");
        return;
    }

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!(victim = get_char_room_vis(ch, arg1))) {
        cprintf(ch, "You do not see that person here.\n");
        return;
    }

    argument = one_argument(argument, arg2, sizeof(arg2));
    if (IS_IMMORTAL(ch)) {
        ct = room_in_city(ch->in_room);
        if (*arg2) {
            for (ct = 0; ct < MAX_CITIES; ct++)
                if (!str_prefix(arg2, city[ct].name))
                    break;
        }
    }

    if (ct <= 0 || ct >= MAX_CITIES) {
        cprintf(ch, "No such city.\n");
        return;
    }

    if (!IS_SET(victim->specials.act, city[ct].c_flag)) {
        act("$N is not a criminal.", FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

    for (p = victim->affected; p; p = p->next)
        if ((p->type == TYPE_CRIMINAL) && (p->modifier == city[ct].c_flag)) {
            cprintf(ch, "Removing flag for %s\n", city[ct].name);
            affect_remove(victim, p);
            break;
        }
    /*
     * while (affected_by_spell(victim, TYPE_CRIMINAL))
     * affect_from_char(victim, TYPE_CRIMINAL);
     */

    REMOVE_BIT(victim->specials.act, city[ct].c_flag);

    // They've been explicitly pardoned, remove any permanent crimflag
    if (get_char_extra_desc_value(victim, "[CRIM_FLAGS]", buf, 100)) {
        permflags = atol(buf);
        REMOVE_BIT(permflags, city[ct].c_flag);
        sprintf(buf, "%ld", permflags);
        set_char_extra_desc_value(victim, "[CRIM_FLAGS]", buf);
    }

    act("$n pardons $N of $S crimes.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("You pardon $N of $S crimes.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n pardons you of your crimes.", FALSE, ch, 0, victim, TO_VICT);
}


void
cmd_skin(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256], char_mess[256], room_mess[256], skin_obj[10], skin_mod[5];
    char *message = 0;
    char *skin_info_override = 0;
    int chance, rc, j = 0, k = 0;
    struct skin_data *skin;
    OBJ_DATA *body, *obj, *knife;
    OBJ_DATA *tmp, *next_content;
    bool success = FALSE;
    bool skin_info_override_status = FALSE;

    if (ch->skills[SKILL_SKINNING])
        chance = ch->skills[SKILL_SKINNING]->learned;
    else if (GET_GUILD(ch) == GUILD_RANGER)
        chance = 20;
    else if (GET_GUILD(ch) == GUILD_WARRIOR)
        chance = 5;
    else if (GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST)
        chance = 5;
    else
        chance = 1;

    if (GET_RACE(ch) == RACE_MANTIS)
        chance += 15;
    else if (GET_RACE(ch) == RACE_HALFLING)
        chance += 30;
    else if (GET_RACE(ch) == RACE_DESERT_ELF)
        chance += 10;

    chance += wis_app[GET_WIS(ch)].wis_saves;
/* 5 at wis15 20 at wis18 - kel */

    knife = ch->equipment[EP];
    if (!knife) {
        act("You can't skin without a knife in your primary hand.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if ((knife->obj_flags.type != ITEM_WEAPON)
        || ((knife->obj_flags.value[3] != (TYPE_CHOP - 300))
            && (knife->obj_flags.value[3] != (TYPE_STAB - 300)))) {
        act("You can't skin with that.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (ch->specials.riding) {
        act("It will be hard to skin while mounted on $r.", FALSE, ch, 0, ch->specials.riding,
            TO_CHAR);
        return;
    }
    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!(body = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
        act("You see no such body here.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }


    /* Begin SKIN_INFO_OVERRIDE */

    skin_info_override = find_ex_description("[SKIN_INFO_OVERRIDE]", body->ex_description, TRUE);
    
    if (skin_info_override) {
      int msglen = strlen(skin_info_override);

      while (j <  msglen) {
        bzero(char_mess, sizeof(char_mess));
        bzero(room_mess, sizeof(room_mess));
        bzero(skin_obj, sizeof(skin_obj));
        bzero(skin_mod, sizeof(skin_mod));

        for (k=0;j <= msglen && isdigit(skin_info_override[j]); j++){
          skin_obj[k] = skin_info_override[j];
          k++;
        }


        while (j <= msglen && !isdigit(skin_info_override[j]))
          j++;
        
        for (k = 0; j <= msglen && isdigit(skin_info_override[j]); j++) {
          skin_mod[k] = skin_info_override[j];
          k++;
        }

        while (j <= msglen && isspace(skin_info_override[j]))
          j++;
        
        for (k = 0; j <= msglen && skin_info_override[j] != '\n' && skin_info_override[j] != '\r'; j++) {
          if (skin_info_override[j] == '$')
            j++;
          char_mess[k] = skin_info_override[j];
          k++;
        }

        while (j <= msglen && isspace(skin_info_override[j]))
          j++;
        
        for (k = 0; j <= msglen && skin_info_override[j] != '\n' && skin_info_override[j] != '\r'; j++) {
          if (skin_info_override[j] == '$')
            j++;
          room_mess[k] = skin_info_override[j];
          k++;
        }

        while ((j <= msglen) && ((skin_info_override[j] == '\n') || (skin_info_override[j] == '\r')))
          j++;

        obj = read_object(atoi(skin_obj), VIRTUAL);
        
        if (!obj) {
          errorlogf("Skin override was unable to load item #%d off of %s in room %d.",
                    atoi(skin_obj), OSTR(body, short_descr), body->in_room->number);
        }
        else if ((chance + atoi(skin_mod)) > number(1, 100)) {
          if (*room_mess && *char_mess) {
            obj_to_room(obj, ch->in_room);
            act(room_mess, FALSE, ch, body, 0, TO_ROOM);
            act(char_mess, FALSE, ch, body, 0, TO_CHAR);
            success = TRUE; 
          }
        }
        
      }
      skin_info_override_status = TRUE;
    }

    /* End:  SKIN_INFO_OVERRIDE */ 

    /* Now check for custom skin code */
    /* This must go before the 'normal' skin code, otherwise it will fail
     * for races that don't have default skin values. */
    message = find_ex_description("[SKIN_INFO]", body->ex_description, TRUE);

    if (message && skin_info_override_status == FALSE) {
        int msglen = strlen(message);
        bzero(char_mess, sizeof(char_mess));
        bzero(room_mess, sizeof(room_mess));
        bzero(skin_obj, sizeof(skin_obj));
        bzero(skin_mod, sizeof(skin_mod));

        // gamelog(message);

        for (j = 0; j <= msglen && isdigit(message[j]); j++)
            skin_obj[j] = message[j];

        //      gamelog (skin_obj);

        while (j <= msglen && !isdigit(message[j]))
            j++;

        for (k = 0; j <= msglen && isdigit(message[j]); j++) {
            skin_mod[k] = message[j];
            k++;
        }
        //      gamelog (skin_mod);

        while (j <= msglen && isspace(message[j]))
            j++;

        for (k = 0; j <= msglen && message[j] != '\n' && message[j] != '\r'; j++) {
            if (message[j] == '$')
                j++;
            char_mess[k] = message[j];
            k++;
        }

        //      gamelog (char_mess);

        while (j <= msglen && isspace(message[j]))
            j++;

        for (k = 0; j <= msglen && message[j] != '\n' && message[j] != '\r'; j++) {
            if (message[j] == '$')
                j++;
            room_mess[k] = message[j];
            k++;
        }

        //      gamelog (room_mess);

        obj = read_object(atoi(skin_obj), VIRTUAL);
        if (!obj) {
            errorlogf("Custom skin was unable to load item #%d off of %s in room %d.",
                      atoi(skin_obj), OSTR(body, short_descr), body->in_room->number);
        } else if ((chance + atoi(skin_mod)) > number(1, 100)) {
            obj_to_room(obj, ch->in_room);
            act(room_mess, FALSE, ch, body, 0, TO_ROOM);
            act(char_mess, FALSE, ch, body, 0, TO_CHAR);
            success = TRUE;
        }
    }

    rc = body->obj_flags.value[3];
    if (!IS_CORPSE(body) || !race[rc].skin || IS_CORPSE_HEAD(body)) {
        act("You can't skin the $o.", FALSE, ch, body, 0, TO_CHAR);
        return;
    }

    if (skin_info_override_status == FALSE)
      for (skin = race[rc].skin; skin; skin = skin->next) {
        if ((chance + skin->bonus) > number(1, 100)) {
          if (!(obj = read_object(skin->item, VIRTUAL)))
            continue;
          obj_to_room(obj, ch->in_room);
          act(skin->text[0], FALSE, ch, body, 0, TO_CHAR);
          act(skin->text[1], FALSE, ch, body, 0, TO_ROOM);
          success = TRUE;
        } else {
          if (ch->skills[SKILL_SKINNING])
            gain_skill(ch, SKILL_SKINNING, 1);
          /* possible wis check to init skill if you don't have it? */
        }
      }
    
    // bloody the knife
    sflag_to_obj(knife, OST_BLOODY);

    if (ch->equipment[WEAR_HANDS])
        sflag_to_obj(ch->equipment[WEAR_HANDS], OST_BLOODY);

    if (!success) {
        act("You ineptly hack the body to pieces.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n ineptly hacks the body to pieces.", FALSE, ch, body, 0, TO_ROOM);
    }

    for (tmp = body->contains; tmp; tmp = next_content) {
        next_content = tmp->next_content;
        obj_from_obj(tmp);
        obj_to_room(tmp, ch->in_room);
    }

    extract_obj(body);
}


void
cmd_behead(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[256];
    OBJ_DATA *body, *weapon;

    weapon = ch->equipment[EP];
    if (!weapon) {
        weapon = ch->equipment[ETWO];
    } 

    if( !weapon ) {
        cprintf(ch, "You can't behead without a weapon.\n\r");
        return;
    }

    if ((weapon->obj_flags.type != ITEM_WEAPON)
     || ((weapon->obj_flags.value[3] != (TYPE_CHOP - 300))
      && (weapon->obj_flags.value[3] != (TYPE_SLASH - 300)))) {
        cprintf(ch, "You can't behead with %s.\n\r", 
         format_obj_to_char(weapon, ch, 1));
        return;
    }

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (!(body = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
        cprintf(ch, "You see no such body here.\n\r");
        return;
    }

    // Make sure the object is a corpse
    if (!IS_CORPSE(body)
     // doesn't start with 'the body of'
     || str_prefix("the body of ", OSTR(body, short_descr))
     // already a head 
     || IS_SET(body->obj_flags.value[1], CONT_CORPSE_HEAD)) {
        cprintf(ch, "You can't behead %s.", format_obj_to_char(body, ch, 1));
        return;
    }

    // make the weapon bloody
    sflag_to_obj(weapon, OST_BLOODY);
    act("You behead $p.", FALSE, ch, body, 0, TO_CHAR);
    act("$n removes the head from $p.", FALSE, ch, body, 0, TO_ROOM);
    make_head_from_corpse(body);
}

void
cmd_play(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{                               /*  Kel  */
    OBJ_DATA *obj;

    if (!ch->equipment[ES]) {
        act("Play what?", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    obj = ch->equipment[ES];

    if ((obj->obj_flags.type == ITEM_TREASURE)
        || ((obj->obj_flags.value[0] == (TREASURE_WIND))
            && (obj->obj_flags.value[0] == (TREASURE_STRINGED))
            && (obj->obj_flags.value[0] == (TREASURE_PERCUSSION)))) {
        switch (obj->obj_flags.value[0]) {
        case (TREASURE_WIND):
            act("You play a simple tune on the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("Light sound flows from $a $o as $n plays a song on it.", FALSE, ch, obj, 0,
                TO_ROOM);
            break;
        case (TREASURE_STRINGED):
            act("You strum out a melodious tune on the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("The air is filled with melody as $n strums $a $o.", FALSE, ch, obj, 0, TO_ROOM);
            break;
        case (TREASURE_PERCUSSION):
            act("You beat a lively rhythm out of the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("A steady rhythm comes from the direction of $n as $s hands move " "over $s $o.",
                FALSE, ch, obj, 0, TO_ROOM);
            break;
        }
    } else if (obj->obj_flags.type == ITEM_MUSICAL) {
        switch (obj->obj_flags.value[1]) {
        case (INSTRUMENT_BELLS):
            act("A lively jingle comes from the $o as you shake it.", FALSE, ch, obj, 0, TO_CHAR);
            act("A steady jingle comes from the $o as $n shakes it.", FALSE, ch, obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_BRASS):
            act("A shrill metallic clamour from the $o fills the air as you play it.", FALSE, ch,
                obj, 0, TO_CHAR);
            act("A shrill metallic clamour from the $o fills the air as $n plays it.", FALSE, ch,
                obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_CTHULHU):
            act("A darkness fills the air, which chills at the shrill piping of the $o.", FALSE, ch,
                obj, 0, TO_CHAR);
            act("A darkness fills the air, which chills at the shrill piping of the $o.", FALSE, ch,
                obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_DRONE):
            act("An eerie, mesmerizing drone comes from the $o as you play it.", FALSE, ch, obj, 0,
                TO_CHAR);
            act("An eerie, mesmerizing drone comes from the $o as $n plays it.", FALSE, ch, obj, 0,
                TO_ROOM);
            break;
        case (INSTRUMENT_GONG):
            act("A single tone sounds from the $o as you ring it.", FALSE, ch, obj, 0, TO_CHAR);
            act("A single tone sounds from the $o as $n rings it.", FALSE, ch, obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_GOURD):
            act("A steady, hypnotic rhythm comes from the $o as you shake it.", FALSE, ch, obj, 0,
                TO_CHAR);
            act("A steady, hypnotic rhythm comes from the $o as $n shakes it.", FALSE, ch, obj, 0,
                TO_ROOM);
            break;
        case (INSTRUMENT_LARGE_DRUM):
            act("A steady boom comes from the $o as you beat it with your hands.", FALSE, ch, obj,
                0, TO_CHAR);
            act("A steady boom comes from the $o as $n beats it.", FALSE, ch, obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_MAGICK_FLUTE):
            act("A hypnotic, seductive stream of sound flows from the $o as you play it.", FALSE,
                ch, obj, 0, TO_CHAR);
            act("A hypnotic, seductive stream of sound flows from the $o as $n plays it.", FALSE,
                ch, obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_MAGICK_HORN):
            act("A wild, sweet compelling sound rings out from the $o as you sound it.", FALSE, ch,
                obj, 0, TO_CHAR);
            act("A wild, sweet compelling sound rings out from the $o as $n sounds it.", FALSE, ch,
                obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_PERCUSSION):
            act("You beat a lively rhythm out of the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("A steady rhythm comes from the direction of $n as $s hands move " "over $s $o.",
                FALSE, ch, obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_RATTLE):
            act("You shake a lively rhythm out of the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("A steady rhythm comes from the $o as $n's hands shake it.", FALSE, ch, obj, 0,
                TO_ROOM);
            break;
        case (INSTRUMENT_SMALL_DRUM):
            act("You beat a lively upbeat rhythm out of the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("A lively rhythm comes from the direction of $n as $s hands move " "over $s $o.",
                FALSE, ch, obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_STRINGED):
            act("You strum out a melodious tune on the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("The air is filled with melody as $n strums $a $o.", FALSE, ch, obj, 0, TO_ROOM);
            break;
        case (INSTRUMENT_WHISTLE):
            act("You sound a sweet, shrill tune upon the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("A shrill, sweet tune flows from $a $o as $n plays a song on it.", FALSE, ch, obj,
                0, TO_ROOM);
            break;
        case (INSTRUMENT_WIND):
            act("You play a simple tune on the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("Light sound flows from $a $o as $n plays a song on it.", FALSE, ch, obj, 0,
                TO_ROOM);
            break;
        default:
            act("You play a simple tune on the $o.", FALSE, ch, obj, 0, TO_CHAR);
            act("Light sound flows from $a $o as $n plays a song on it.", FALSE, ch, obj, 0,
                TO_ROOM);
            break;
        }
    } else
        act("You cannot play that.", FALSE, ch, obj, 0, TO_CHAR);
}

void
cmd_pagelength(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg, sizeof(arg));

    if (!ch->desc)
        return;

    if (arg[0] == '\0') {
        sprintf(buf, "Pagelength set to %d.\n\r", GET_PAGELENGTH(ch));
        send_to_char(buf, ch);
        return;
    } else if (!is_number(arg)) {
        send_to_char("Syntax: pagelength <number>\n\r", ch);
        return;
    }

    ch->pagelength = atoi(arg);

    if (ch->pagelength < 0 || ch->pagelength > 100) {
        ch->pagelength = 0;
        send_to_char("Pagelength must be between 0 (to use default)" " and 100.\n\r", ch);
    }

    cmd_reset(ch, "", CMD_RESET, 0);
    sprintf(buf, "Pagelength set to %d.\n\r", GET_PAGELENGTH(ch));
    send_to_char(buf, ch);
    return;
}

void
cmd_tdesc(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];

    // Bogus pointers?
    if (!ch || !ch->in_room)
        return;

    if( ch->temp_description ) {
        free(ch->temp_description);
        ch->temp_description = NULL;
    }

    if( *argument && stricmp("clear", argument)) {
        sprintf(buf, "%s clears their tdesc.", GET_NAME(ch));
        send_to_monitor(ch, buf, MONITOR_OTHER);
        cprintf( ch, "Temporary Description cleared." );
        return;
    }

    sprintf(buf, "%s changes their tdesc.", GET_NAME(ch));
    send_to_monitor(ch, buf, MONITOR_OTHER);

    ch->temp_description = strdup("");

    cprintf( ch, "\n\rEnter a temporary description.\n\r");
    string_edit(ch, &ch->temp_description, 320);
}

/* Put this in until we can save corefiles and figure out what's crashing
 * the game in scribble. - Tiernan 7/16/03
 */
//#define NO_SCRIBBLE

void
cmd_scribble(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *obj, *chalk = NULL;
    char buf[MAX_STRING_LENGTH];
    char how[MAX_STRING_LENGTH];
    char what[MAX_STRING_LENGTH];
    char past_what[MAX_STRING_LENGTH];
    char what_verb[MAX_STRING_LENGTH];
    int duration = 100;

#ifdef NO_SCRIBBLE
    send_to_char("Currently broken.\n\r", ch);
    return;
#endif

    // Bogus pointers?
    if (!ch || !ch->in_room)
        return;

    if (argument[0] == '\0') {
        send_to_char("What do you want to scribble?\n\r", ch);
        return;
    }

    strcpy(what, "scribble");
    strcpy(past_what, "scribbled");
    strcpy(what_verb, "scribbling");

    switch (ch->in_room->sector_type) {
    case SECT_DESERT:
        strcpy(how, "in the sand");
        break;
    case SECT_CITY:
    case SECT_INSIDE:
        strcpy(how, "on the ground");
        break;
    case SECT_ROAD:
        strcpy(how, "on the road");
        break;
    case SECT_CAVERN:
    case SECT_SOUTHERN_CAVERN:
        strcpy(how, "on the cavern wall");
        break;
    default:
        send_to_char("You can't find a good place to scribble here.\n\r", ch);
        return;
    }

    if (ch->in_room->sector_type != SECT_DESERT) {
        chalk = ch->equipment[ES];
        if (!chalk) {
            send_to_char("You aren't holding anything to scribble with.\n\r", ch);
            return;
        } else {
            switch (chalk->obj_flags.type) {
            case ITEM_MINERAL:
                if (!isname("chalk", OSTR(ch->equipment[ES], name))) {
                    cprintf(ch, "You aren't holding any chalk to scribble with.\n\r");
                    return;
                }
                duration = 300;
                break;

            case ITEM_WEAPON:
                /* weapon type */
                switch (chalk->obj_flags.value[3] + 300) {
                case TYPE_STAB:
                case TYPE_PIERCE:
                case TYPE_SLASH:
                    switch (chalk->obj_flags.material) {
                    case MATERIAL_CHITIN:
                    case MATERIAL_BONE:
                    case MATERIAL_IVORY:
                    case MATERIAL_HORN:
                    case MATERIAL_DUSKHORN:
                    case MATERIAL_TORTOISESHELL:
                    case MATERIAL_CERAMIC:
                        duration = 500;
                        break;
                    case MATERIAL_OBSIDIAN:
                    case MATERIAL_METAL:
                    case MATERIAL_STONE:
                    case MATERIAL_GEM:
                    case MATERIAL_GLASS:
                        duration = 1000;
                        break;
                    default:   /* made of a 'soft' material */
                        cprintf(ch, "%s is too soft to scribble with.\n\r",
                                capitalize(format_obj_to_char(chalk, ch, 1)));
                        return;
                        break;
                    }

                    break;
                default:       /* not stabbing or slashing weapon */
                    cprintf(ch, "%s is not sharp enough to scribble with.\n\r",
                            capitalize(format_obj_to_char(chalk, ch, 1)));
                    return;
                }
                strcpy(what, "crude etching");
                strcpy(past_what, "etched");
                strcpy(what_verb, "scratching");
                break;

            case ITEM_TOOL:
                switch (chalk->obj_flags.value[0]) {
                case TOOL_LOCKPICK:
                case TOOL_NEEDLE:
                case TOOL_CHISEL:
                case TOOL_AWL:
                case TOOL_SCRAPER:
                case TOOL_DRILL:
                case TOOL_WOODCARVING:
                    switch (chalk->obj_flags.material) {
                    case MATERIAL_CHITIN:
                    case MATERIAL_BONE:
                    case MATERIAL_IVORY:
                    case MATERIAL_HORN:
                    case MATERIAL_DUSKHORN:
                    case MATERIAL_TORTOISESHELL:
                    case MATERIAL_CERAMIC:
                        duration = 500;
                        break;
                    case MATERIAL_OBSIDIAN:
                    case MATERIAL_METAL:
                    case MATERIAL_STONE:
                    case MATERIAL_GEM:
                    case MATERIAL_GLASS:
                        duration = 1000;
                        break;
                    default:   /* made of a 'soft' material */
                        cprintf(ch, "%s is too soft to scribble with.\n\r",
                                capitalize(format_obj_to_char(chalk, ch, 1)));
                        return;
                        break;
                    }
                    break;
                default:       /* tool just isn't right type */
                    cprintf(ch, "%s is not sharp enough to scribble with.\n\r",
                            capitalize(format_obj_to_char(chalk, ch, 1)));
                    return;
                }
                strcpy(what, "crude etching");
                strcpy(past_what, "crudely etched");
                strcpy(what_verb, "scratching");
                break;

            default:
                cprintf(ch, "You can not scribble with %s.\n\r", format_obj_to_char(chalk, ch, 1));
                return;
            }
        }
    }

    for (; isspace(*argument); argument++);

    sprintf(buf, "%s scribbles %s %s.", GET_NAME(ch), argument, how);
    send_to_monitor(ch, buf, MONITOR_OTHER);

    CREATE(obj, OBJ_DATA, 1);
    clear_object(obj);

    obj->obj_flags.type = ITEM_OTHER;
    if (IS_NPC(ch)) {
        sprintf(buf, "%s %s [%s]", what, argument, GET_NAME(ch));
    } else {
        sprintf(buf, "%s %s [%s (%s)]", what, argument, ch->name, ch->account);
    }
    obj->name = strdup(buf);

    sprintf(buf, "%s%s of %s", indefinite_article(what), what, argument);
    obj->short_descr = strdup(buf);

    sprintf(buf, "%s is %s %s here.", argument, past_what, how);
    obj->long_descr = strdup(buf);

    obj->description = strdup("");

    if (ch->in_room->sector_type == SECT_DESERT) {
        act("$n starts scribbling in the sand with $s finger.", FALSE, ch, NULL, NULL, TO_ROOM);
        act("You start scribbling in the sand with your finger.", FALSE, ch, NULL, NULL, TO_CHAR);
    } else {
        sprintf(buf, "$n starts %s %s with $p.", what_verb, how);
        act(buf, FALSE, ch, chalk, NULL, TO_ROOM);
        sprintf(buf, "You start %s %s with $p.", what_verb, how);
        act(buf, FALSE, ch, chalk, NULL, TO_CHAR);
    }

    obj->obj_flags.cost = duration;
    obj->prev = (OBJ_DATA *) 0;
    obj->next = object_list;
    object_list = obj;
    if (obj->next)
        obj->next->prev = obj;

    obj->prev_num = (OBJ_DATA *) 0;
    obj->next_num = (OBJ_DATA *) 0;

    obj_to_room(obj, ch->in_room);

    sprintf(buf, "C pulse scribble_blow_away");
    obj->programs = set_prog(NULL, obj->programs, buf);
    new_event(EVNT_OBJ_PULSE, duration, 0, 0, obj, 0, duration);

    send_to_char
        ("\n\rEnter a description for your scribble.\n\rRemember, describe the drawing, do NOT use this to write.\n\r",
         ch);
    string_edit(ch, &obj->description, 320);
}

void
cmd_addkeywor(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    cprintf(ch, "You must type out 'addkeyword' to add keywords.\n\r");
    return;
}

void
cmd_addkeyword(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg[MAX_INPUT_LENGTH];
    char keywords[MAX_STRING_LENGTH];
    CHAR_INFO *c;
    char **keyword_ptr;

    one_argument(argument, arg, sizeof(arg));

    // No argument?  Give syntax
    if (!*arg) {
        cprintf(ch, "Syntax: addkeyword <keyword>\n\r");
        return;
    }

    // Make sure they don't already have the name
    if (isname(arg, MSTR(ch, name)) || isname(arg, ch->player.extkwds)) {
        cprintf(ch, "You already have that keyword.\n\r");
        return;
    }

    // make sure it's a valid name
    if (_parse_name(arg, arg)) {
        cprintf(ch, "Unable to add keyword, invalid name.\n\r");
        return;
    }

    // watch for immortal names
    if (is_immortal_name(arg) || !stricmp(arg, "me") || !stricmp(arg, "self")) {
        cprintf(ch, "Unable to add that keyword, it is reserved.\n\r");
        return;
    }

    // figure out what keywords to add to
    if( IS_NPC(ch) ) {
        if( ch->name == NULL ) {
            ch->name = strdup( npc_default[ch->nr].name );
        }

        keyword_ptr = &ch->name;
    } else {
        if (!ch->player.extkwds)
            ch->player.extkwds = strdup("");
        keyword_ptr = &ch->player.extkwds;
    }

    // No more than sizeof keywords in names
    if (strlen(*keyword_ptr) + strlen(arg) + 1 > sizeof(keywords)) {
        cprintf(ch, "Sorry, you have too many keywords, you can't add anymore.\n\r");
        return;
    }

    // if not an NPC
    if( !IS_NPC(ch) ) {
        // Look for an existing character with that name
        for (c = ch->desc->player_info->characters; c; c = c->next) {
            if (!stricmp(c->name, arg)) {
                cprintf(ch,
                    "Sorry, you have already used that name on a previous character.\n\r");
            gamelogf("%s (%s) tried to add an old character's name '%s' as a keyword", ch->name,
                     ch->account, arg);
                return;
            }
        }
    }

    strcpy(keywords, *keyword_ptr);
    string_safe_cat(keywords, " ", MAX_STRING_LENGTH);
    string_safe_cat(keywords, arg, MAX_STRING_LENGTH);
    free(*keyword_ptr);
    *keyword_ptr = strdup(keywords);
    cprintf(ch, "Added keyword '%s'.\n\r", arg);

    if( IS_NPC(ch) ) {
        gamelogf("%s [M%d] added keyword '%s'", MSTR(ch, name),
         npc_index[ch->nr].vnum, arg);
    }
    else  {
        gamelogf("%s (%s) added keyword '%s'", ch->name, ch->account, arg);
    }
    return;
}

