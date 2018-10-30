/*
 *  File:       show_menu.c
 *  Author:     Morgenes - Neal Haggard
 *  Purpose:    show the menu for Armageddon
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

#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "show_menu.h"

#include "structs.h"
#include "comm.h"
#include "parser.h"
#include "db.h"
#include "utils.h"
#include "vt100c.h"

const MENU_KEY menu_keys[MAX_MENU_KEYS] = {
    {'N', "Create a new account", MENU_KEY_NO_ACCOUNT},
    {'C', "Connect to your account", MENU_KEY_NO_ACCOUNT},
    {'C', "Connect to your character", MENU_KEY_HAS_CHARS_NOT_CONNECTED},
    {'C', "Disconnect from character", MENU_KEY_CONNECTED_TO_CHAR},
    {'R', "Create a new character", MENU_KEY_CAN_CREATE_NEW},
    {'R', "Revise your character", MENU_KEY_CAN_REVISE_CHAR},
    {'L', "List your characters", MENU_KEY_HAS_CHARS},
    {'Z', "Delete your character", MENU_KEY_CAN_REVISE_CHAR},
    {'V', "Toggle ANSI/VT100 mode", MENU_KEY_SHOW_ALWAYS},
    {'B', "Toggle 'brief' menus", MENU_KEY_SHOW_ALWAYS},
    {'O', "Show Race/Guild Options", MENU_KEY_HAS_ACCOUNT},
    {'P', "Change account password", MENU_KEY_CONFIRMED_EMAIL},
    {'D', "Documentation menu", MENU_KEY_SHOW_ALWAYS},
    {'S', "Stats of your character", MENU_KEY_CONNECTED_TO_CHAR},
    {'E', "Enter Zalanthas", MENU_KEY_CONNECTED_TO_CHAR},
    {'E', "Change your Email", MENU_KEY_NOT_CONNECTED_TO_CHAR},
    {'X', "Exit Armageddon", MENU_KEY_SHOW_ALWAYS},
    {'?', "Read menu options", MENU_KEY_SHOW_ALWAYS}
};

char *mantis_top[NUM_MENU_MANTIS] = {   /* normal mantis */
    "                                _______                                ___\n\r" "                              /\\\\_____//~-_                        _-~\\\\__\n\r" "                             (~)       ~-_ ~-_                  _-~ _-~   \n\r" "                            (~)           ~-_ ~-_            _-~ /-~      \n\r" "Welcome to Armageddon!     (~)              `~-_ ~_======_--~~ __~        \n\r" "                          (~)               _~_\\__\\____/__/_--\\ ~`-_   \n\r" "                           \\           _-~~            _-~~~-_ \\_  ~-_  \n\r" "You may:                    ~-       __--~`_    /   _-~         ~.     ~_ \n\r",        /* christmas mantis - Sandy Claws */
    "                                     ________________________             \n\r" "                                  __/                        \\__          \n\r" "                               __/                              \\         \n\r" "                              /       ~~~                        \\_       \n\r" "                             / __~/`\\~~~~~                         \\      \n\r" "Happy Holidays and          / /       ~\\~~~   ***********************     \n\r" "Welcome to Armageddon!     ***          \\~*****************************   \n\r" "                          *****         ********************************* \n\r" "                          *****        ********         _-~~~-_  \\_****** \n\r" "You may:                   ***       __--~`_    /   _-~         ~. *****_ \n\r",  /* April 1st - Pikachu */
    "                                                                          \n\r"
        "                                                                          \n\r"
        "                        .uu..__                                             \n\r"
        "Welcome to Armageddon!   SSb  `---.__                                       \n\r"
        "                         \"SSb        `--.                          ___.---uuudP\n\r"
        "                          `SSb           `.__.------.__     __.---'      SSSS\"\n\r"
        "You may:                    \"Sb          -'            `-.-'            SSS\" \n\r",
    // Nov. 23rd - Cyberman (Doctor Who anniversary)
	"                                               .....\n\r"
	"                                             .,I?+++I..\n\r"
	"Welcome to Armageddon!            ..=+++++++=+??$$$7~~=,..........\n\r"
	"                                  ,?+..,::~=+8II?I?O7I$7II???+?+:.\n\r"
	"You will be upgraded.            .:=       ..I77?7I=$==..      .+~\n\r"
	"                                 .:+      .I?===+I7++???:..    .~~.\n\r"
	"You may:                          :?     ~==+=+=++++====??.    .==\n\r"
};


#define MANTIS_MID_LINES 12

const char *mantis_mid[NUM_MENU_MANTIS][MANTIS_MID_LINES] = {
/* normal mantis */
    {
     "     -~        \\     _~       ___,  \\ ~-_  \\ \n\r",
     "   ,~ _-,       ~  _~         \\   \\  | , \\ \\  \n\r",
     "  / /~/      -~   /            ~  /  /  \\~  \\ \n\r",
     "  | | \\     _~    |        __-~  / _/  \\~  '\\ \n\r",
     "  \\ ~-_~   -   |  _      ~-____-~ .~  \\~    | \n\r",
     "  /`.  __~~   ~   `_          __-~   \\~    \\~ \n\r",
     "  \\_ ~~   .  |  .   ~-____--~~   \\ \\_~   _/~  \n\r",
     "  /\\___--~       ~--_      /   ____~ _/~~     \n\r",
     "  \\         /        ~~~___  /     _-\\\\~\\     \n\r",
     "  /\\       /                   _-\\~\\\\~\\\\~\\    \n\r",
     " / | \\   \\ | /    /         _-~ )\\\\~\\\\~\\\\~~\\  \n\r",
     "{ /\\ \\             /      _~ \\  ~`~~\\\\~~\\\\~~\\ \n\r"},
/* Christmas mantis - Sandy Claws */
    {
     "     -~        \\     _~       ___,  \\ ~-_  \\ \n\r",
     "   ,~ _-,       ~  _~         \\   \\  | , \\ \\  \n\r",
     "  / /~/      -~   /            ~  /  /  \\~  \\ \n\r",
     "  | | \\     _~    |        __-~  / _/  \\~  '\\ \n\r",
     "  \\ ~-_~   -   |  _      ~-____-~ .~  \\~    | \n\r",
     "  /`.  __~~   ~   `_          __-~   \\~    \\~ \n\r",
     "  \\_ ~~   .  |  .   ~-____--~~   \\ \\_~   _/~  \n\r",
     "  /\\___--~       ~--_      /   ____~ _/~~     \n\r",
     "  \\         /        ~~~___  /     _-\\\\~\\     \n\r",
     "  /\\       /                   _-\\~\\\\~\\\\~\\    \n\r",
     " / | \\   \\ | /    /         _-~ )\\\\~\\\\~\\\\~~\\  \n\r",
     "{ /\\ \\             /      _~ \\  ~`~~\\\\~~\\\\~~\\ \n\r"},
/* April 1st - Pikachu */
    {
     ".                                       dS\"\n\r",
     " `.   /                              ...\"\n\r",
     "   `./                           ..::-'\n\r",
     "    /                         .:::-'            .\n\r",
     "    :                          ::''\\          _.'\n\r",
     "   .' .-.             .-.           `.      .' \n\r",
     "   : /'SS|           .@\"S\\           `.   .' \n\r",
     "  .'|SuSS|          |SS,SS|           |  < \n\r",
     "  | `:SS:'          :SSSSS:           `.  `. \n\r",
     "  :                  `\"--'             |   `-.\n\r",
     "  :##.       ==             .###.       `.     `.\n\r",
     "  |##:                      :###:        | \n\r"},
/* Nov. 23rd, - Cyberman */
    {
	"    ~?  ..=+==~~~~~==~:~~=++?..  .==\n\r",
	"  ..~I..:~=++=:,,:~=~:,,,:+=I+.. .+=\n\r",
	"   .~I.~~=I?=~,,,:~=~:,.,7++?=+=+.?=\n\r",
	"   .=$.~??I+=.,I,:~+=:,.I.=+?+??+.?~\n\r",
	"  ..=D+~?7I?,,,,,~+?++.,,,,+III?=:?~\n\r",
	"  .~+??=?7I+,MMM=~=?+~:MMMZ,II7??=?I.\n\r",
	"  ...+?=?II~:MMM+~=??~:MMMM,I?7?++?.\n\r",
	"    ...+?I7+?:::~~=I?~~:~:8:I?7?=...\n\r",
	"     .==++7?=~:~~~=I?=~~:~=+I?I+=.\n\r",
	"      .+I7$+=~~~~=+I?=~~~~=+7+I=,.\n\r",
	"      ...??===:~~=+I?=~~:~==?+..\n\r",
	"        .=====~~~=+??=~~~====I.\n\r",
    }
};

char *mantis_bottom[NUM_MENU_MANTIS] = {        /* normal mantis */
    "Read the documentation        { |\\     __ _           _-\\   \\  \\\\~~\\\\~~\\\\~~\\\n\r" "menu before creating your     | ||~_ /`    ~\\  /    _/~  )   | |\\\\~~\\\\~~\\\\~~\n\r" "character, please.            | ||  \\|\"\"\"\"\"\"\"|_ __-~     ;   | |~\\\\~~\\\\~~\\\\~\n\r" "                              \\ \\\\  ({\"\"\"\"\"\"\"}\\\\        _~  /  /~~\\\\~~\\\\~~\\\\\n\r",       /* Christmas mantis - Sandy Claws */
    "Read the documentation        { |\\     __ _           _-\\   \\  \\\\~~\\\\~~\\\\~~\\\n\r" "menu before creating your     | ||~_ /`    ~\\  /    _/~  )   | |\\\\~~\\\\~~\\\\~~\n\r" "character, please.            | ||  \\|\"\"\"\"\"\"\"|_ __-~     ;   | |~\\\\~~\\\\~~\\\\~\n\r" "                              \\ \\\\  ({\"\"\"\"\"\"\"}\\\\        _~  /  /~~\\\\~~\\\\~~\\\\\n\r",       /* April 1st - Pikachu */
    "Read the documentation          |#'     `..'`..'          `###'        x:     /\n\r"
        "menu before creating your        \\                                   xXX|    /\n\r"
        "character, please.                \\                                xXXX'|   /\n\r"
        "                                  /`-.                                  `. /\n\r"
        "                                :    `-  ...........,                   |/  .'\n\r"
        "                                |         ``:::::::'       .            |<    `\n\r",
/* Nov. 23rd, - Cyberman */
	"                                        O===~~:M$M7M~~~===.\n\r"
	"Read the documentation                   +=+~~~=+++=~~~?=?.\n\r"
	"menu before creating your               .,++~~~=+++=~~~I+..\n\r"
	"character, please.                         ?++======~+++.\n\r"
	"                                           .:7???????$...\n\r"
	"                                             ..........\n\r"
};

char *mantis_tail[NUM_MENU_MANTIS] = { "", "", "" }

;


bool
can_do_menu_key(DESCRIPTOR_DATA * d, int type)
{
    PLAYER_INFO *pinfo = d->player_info;

    switch (type) {
    case MENU_KEY_NO_ACCOUNT:
        if (!pinfo)
            return TRUE;
        break;
    case MENU_KEY_HAS_ACCOUNT:
        if (pinfo)
            return TRUE;
        break;
    case MENU_KEY_CONFIRMED_EMAIL:
        if (pinfo && IS_SET(pinfo->flags, PINFO_CONFIRMED_EMAIL))
            return TRUE;
        break;
    case MENU_KEY_HAS_CHARS:
        if (pinfo && IS_SET(pinfo->flags, PINFO_CONFIRMED_EMAIL)
            && pinfo->num_chars)
            return TRUE;
        break;
    case MENU_KEY_HAS_CHARS_NOT_CONNECTED:
        if (pinfo && IS_SET(pinfo->flags, PINFO_CONFIRMED_EMAIL)
            && pinfo->num_chars && !d->character)
            return TRUE;
        break;
    case MENU_KEY_CAN_CREATE_NEW:
        if (pinfo && IS_SET(pinfo->flags, PINFO_CONFIRMED_EMAIL)
            && !IS_SET(pinfo->flags, PINFO_NO_NEW_CHARS)
            && pinfo->num_chars_alive + pinfo->num_chars_applying < pinfo->num_chars_allowed
            && !d->character)
            return TRUE;
        break;
    case MENU_KEY_CONNECTED_TO_CHAR:
        if (pinfo && d->character)
            return TRUE;
        break;
    case MENU_KEY_NOT_CONNECTED_TO_CHAR:
        if (pinfo && !d->character)
            return TRUE;
        break;
    case MENU_KEY_CAN_REVISE_CHAR:
        if (pinfo && d->character && d->character->application_status == APPLICATION_DENY)
            return TRUE;
        break;
    case MENU_KEY_SHOW_ALWAYS:
        return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}


void
show_main_menu(struct descriptor_data *d)
{
    int index, mantis_index, show_mantis;
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    time_t tt;
    struct tm *tp;

    if (!d)
        return;

    buf2[0] = '\0';
    mantis_index = 0;

    /* choose what mantis to show based on date */
    tt = time(0);
    tp = localtime(&tt);

    /* tm_mon = Month, 0 indexed, tm_mday is day of month 1..31 */
    if (tp->tm_mon == 11 && tp->tm_mday == 25) {        /* Christmas */
        show_mantis = MANTIS_SANDY_CLAWS;
    } else if (tp->tm_mon == 3 && tp->tm_mday == 1) {   /* April 1st */
        show_mantis = MANTIS_PIKACHU;
    } else if (tp->tm_mon == 10 && tp->tm_mday == 23) {   /* Nov 23rd */
        show_mantis = MANTIS_CYBERMAN;
    } else {                    /* normal */

        show_mantis = MANTIS_NORMAL;
    }

    for (index = 0; index < MAX_MENU_KEYS; index++) {
        if (can_do_menu_key(d, menu_keys[index].show)) {
            sprintf(buf, "%s(%s%c%s) %-25s %s", d->brief_menus ? "     " : "",
                    d->term ? VT_BOLDTEX : "", menu_keys[index].key, d->term ? VT_NORMALT : "",
                    menu_keys[index].text,
                    d->brief_menus ? mantis_index %
                    2 ? "\n\r" : "" : mantis_mid[show_mantis][mantis_index]);
            mantis_index++;
            strcat(buf2, buf);
        }
    }

    if (d->brief_menus && mantis_index % 2)
        strcat(buf2, "\n\r");

    if (!d->brief_menus) {
        for (; mantis_index < MANTIS_MID_LINES; mantis_index++) {
            sprintf(buf, "                              %s", mantis_mid[show_mantis][mantis_index]);
            strcat(buf2, buf);
        }

        SEND_TO_Q(mantis_top[show_mantis], d);
    } else
        SEND_TO_Q("\n\rYou may:\n\r", d);

    SEND_TO_Q(buf2, d);

    if (!d->brief_menus)
        SEND_TO_Q(mantis_bottom[show_mantis], d);
    else
        SEND_TO_Q("\n\r", d);

    sprintf(buf, "Armageddon is %-6s", lock_mortal ? "CLOSED." : "OPEN.");
    SEND_TO_Q(buf, d);
    SEND_TO_Q("\n\r", d);

    if (!d->brief_menus) {
        SEND_TO_Q(mantis_tail[show_mantis], d);
    } else {
        SEND_TO_Q("\n\r\n\r", d);
    }

    SEND_TO_Q("Choose thy fate: ", d);
}

