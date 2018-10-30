/*
 * File: DELAY.C
 * Usage: Handle delayed commands.
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

/* Revision history.  Pls remember to list changes.
 * 12/17/1999   added cases for light and heavy forest sectors
 * along with this revision hist.     sanvean
 * 05/18/2000   Put a delay on command make, will affect pitching
 *              tents and rolling smokes.  -San
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "skills.h"
#include "parser.h"
#include "cities.h"
#include "delay.h"
#include "guilds.h"
#include "clan.h"
#include "handler.h"
#include "utility.h"
#include "modify.h"

/* prototypes */
bool is_in_command_q_with_cmd(CHAR_DATA * ch, int cmd);
void remove_delayed_com(struct delayed_com_data *target);

/* local variables */
struct delayed_com_data *d_com_list;
static struct delayed_com_data *d_next;

void
process_command_q(void)
{
    struct delayed_com_data *d = NULL;
    struct timeval start;
    perf_enter(&start, "process_command_q()");

    for (d = d_com_list; d; d = d_next) {
        d_next = d->next;
        if (--(d->timeout) <= 0) {
            CHAR_DATA *ch = d->character;
            int cmd = d->command;
            const char *argument = d->argument;
            int count = d->count;

            d->argument = NULL;
            remove_delayed_com(d);
            ch->specials.to_process = TRUE;
            perform_command(ch, cmd, argument, count);
            ch->specials.to_process = FALSE;
            free((void*)argument);
        }
    }
    perf_exit("process_command_q()", start);
}

void
remove_delayed_com(struct delayed_com_data *target)
{
    /*
     * We must only alter d_next here if it matches the character we're
     * removing.
     */
    if (d_next == target)
        d_next = target->next;

    struct delayed_com_data *d;
    for (d = d_com_list; d; d = d->next) {
        if (d->next == target) {
            d->next = target->next;
            break;
        }
    }

    if (d_com_list == target)
        d_com_list = target->next;

    free(target->argument);
    free(target);
}

/* the integer math on the old version of skill_delay below was screwy,
   I wonder if anyone ever got any kind of delay -Morg */
/* commenting this out for now, cause it's gonna really stretch out delays */
/* #define NEW_SKILL_DELAY */
#ifdef NEW_SKILL_DELAY
int
skill_delay(int learned)
{
    int n;

    n = (1000.0 * (1.0 / (float) learned));

    return (MIN(MAX_DELAY, n));
}
#else
int
skill_delay(int learned)
{
    int n;
    n = (int) (1000 * (1 / (learned ? learned : 1)));
    return (MIN(MAX_DELAY, n));
}
#endif


void
rem_from_command_q(CHAR_DATA *ch)
{
    struct delayed_com_data *d, *d_next_local;
	struct delayed_com_data *d_prev;

    for (d = d_com_list; d; d_prev = d, d = d_next_local) {
        d_next_local = d->next;
        if (d->character == ch) {
            remove_delayed_com(d);
            ch->specials.to_process = FALSE;
            WIPE_LDESC(ch);
            return;
        } else {
            d_prev = d;
        }
    }
    WIPE_LDESC(ch);
}


bool
is_in_command_q(CHAR_DATA * ch)
{
    struct delayed_com_data *d;

    if (!ch)
        return FALSE;

    for (d = d_com_list; d; d = d->next)
        if (d && d->character && (d->character == ch))
            return TRUE;

    return FALSE;
}

bool
is_in_command_q_with_cmd(CHAR_DATA * ch, int cmd)
{
    /*  char buf[MAX_STRING_LENGTH]; */
    struct delayed_com_data *d;
#ifdef COMMAND_Q_BUG_LOG
    gamelog("Entering is_in_command_q_with_cmd");
#endif

    if (!is_in_command_q(ch)) {
#ifdef COMMAND_Q_BUG_LOG
        gamelog("Ch not in command_q");
#endif
        return FALSE;
    }

    for (d = d_com_list; d; d = d->next)
        if (d->character == ch)
            break;

    if (!d) {
#ifdef COMMAND_Q_BUG_LOG
        gamelog("No d");
#endif
        return FALSE;
    }

    if (!d->character) {
#ifdef COMMAND_Q_BUG_LOG
        gamelog("No d->character");
#endif
        return FALSE;
    }

#ifdef COMMAND_Q_BUG_LOG
    if (d->command) {
        sprintf(buf, "d->command is: %s", command_name[cmd]);
        gamelog(buf);
    }
#endif
    if (d->command == cmd)
        return TRUE;
    return FALSE;
}

#ifdef WHATSHIT
int
skill_delay(int learned)
{
    int n;
    n = (int) (1000 * (1 / learned));
    return (MIN(MAX_DELAY, n));
}
#endif

#define LDESC_TOO_LONG(name) \
    shhlogf("ERROR: %s's ldesc too long, %s:%d", (name), __FILE__, __LINE__)

void
add_to_command_q(CHAR_DATA * ch, const char *arg, Command cmd, int spell, int count)
{
    CHAR_DATA *victim;
    struct delayed_com_data *d;
    char target[MAX_INPUT_LENGTH], tmp_char[MAX_INPUT_LENGTH];
    char *cast_msg_self = NULL;
    char *cast_msg_room = NULL;
    char *cast_msg_ldesc = NULL;
    int n;
    int d_coms[] = {
        CMD_HIT,
        CMD_CAST,
        CMD_BACKSTAB,
        CMD_PICK,
        CMD_STEAL,
        CMD_HUNT,
        CMD_SHOOT,
        CMD_SEARCH,
        CMD_BREW,
        CMD_SAP,
        CMD_HIDE,
        CMD_FORAGE,
        CMD_MAKE,
        CMD_THROW,
        CMD_CRAFT,
        CMD_REPAIR,
        CMD_SENSE_PRESENCE,
        CMD_PILOT,
        CMD_HARNESS,
        CMD_UNHARNESS,
        CMD_UNLOCK,
        CMD_LOCK,
        CMD_CLEAN,
        CMD_BITE,
        CMD_SPLIT,
        CMD_DIG,                /* Added for Vendyra's javascript, I am not sure why
                                 * we even need to have this list restricted -Tene    */
        CMD_SALVAGE,
        CMD_SILENT_CAST,
        CMD_REJUVENATE,
	CMD_SLIP,
	CMD_PALM,
	CMD_UNLATCH,
	CMD_LATCH,
	CMD_READY,
	CMD_STOW,
        -1
    };


    /* don't want multiple instances...bleah */
    if (is_in_command_q(ch))
        return;
    for (n = 0; d_coms[n] != -1; n++)
        if (d_coms[n] == cmd)
            break;

    if (d_coms[n] == -1) {
        shhlog("Invalid attempt to invoke delayed command.");
        return;
    }

    CREATE(d, struct delayed_com_data, 1);
    d->character = ch;
    d->command = cmd;
    d->count = count;
    CREATE(d->argument, char, strlen(arg) + 1);
    strcpy(d->argument, arg);

    switch (cmd) {
    case CMD_DIG:
        d->timeout = spell;
        break;

    case CMD_BITE:
        d->timeout = spell;
        break;
    case CMD_CLEAN:
        d->timeout = spell;
        break;
    case CMD_CRAFT:            /* Stop making things so fast! */
        d->timeout = spell;
        break;
    case CMD_SALVAGE:
        d->timeout = 3;
        break;
    case CMD_HIT:              /* npc attack delays */
        d->timeout = MAX(1, (75 - ch->abilities.off) / 5);
        break;
    case CMD_CAST:
        if (find_ex_description("[NO_CAST_MESSAGE]", ch->ex_description, TRUE)) {
            d->character->specials.to_process = TRUE;
            perform_command(d->character, d->command, d->argument, d->count);
            d->character->specials.to_process = FALSE;
            rem_from_command_q(ch);
            free(d->argument);
            free(d);
            return;
        }

       // Look for custom messages
       cast_msg_self = find_ex_description("[CAST_MESSAGE_SELF]", ch->ex_description, TRUE);
       cast_msg_room = find_ex_description("[CAST_MESSAGE_ROOM]", ch->ex_description, TRUE);
       cast_msg_ldesc = find_ex_description("[CAST_MESSAGE_LDESC]", ch->ex_description, TRUE);
       
       // Must have all three custom messages to proceed
       if (cast_msg_self && cast_msg_room && cast_msg_ldesc) {
           act(cast_msg_self, FALSE, ch, ch, ch, TO_CHAR);
           act(cast_msg_room, FALSE, ch, ch, ch, TO_ROOM);
           if (!change_ldesc(cast_msg_ldesc, ch))
               LDESC_TOO_LONG(MSTR(ch, name));
       } else { // Tremendously huge switch statement
        switch (GET_GUILD(ch)) {
        case GUILD_WIND_ELEMENTALIST:
            if (IS_TRIBE(ch, 24)) {
                act("An airy apparation appears near $n as $e begins " "casting.", FALSE, ch, 0, 0,
                    TO_ROOM);
                act("You call upon the efreetkin within you to power your spell.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here talking with an airy apparation.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                /* hack for alkyone to give a different message,
                 * indicating that Talazu is the source of her power =hal */
                if (!strcasecmp(MSTR(ch, name), "alkyone")) {
                    act("A silt-filled wind blows throught the area.", TRUE, ch, 0, 0, TO_ROOM);
                    act("You feel Talazu's silt pass through you as you draw " "his power.", FALSE,
                        ch, 0, 0, TO_CHAR);
                    if (!change_ldesc("is here, saturated with silt.", ch))
                        LDESC_TOO_LONG(MSTR(ch, name));
                    break;
                }
                act("Winds swirl around $n as $e summons $s magick.", TRUE, ch, 0, 0, TO_ROOM);
                act("Winds swirl around you as you draw upon the power of Whira.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here, surrounded by winds.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            }
            break;
        case GUILD_STONE_ELEMENTALIST:
            if (IS_TRIBE(ch, 24)) {
                act("$n swells in size and $s skin takes on a stonelike cast"
                    " as $e begins to cast a spell.", FALSE, ch, 0, 0, TO_ROOM);
                act("You call upon the daoling within you to power your spell.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is standing here with stone-like skin.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                act("$n begins a spell, and the earth trembles in response.", FALSE, ch, 0, 0,
                    TO_ROOM);
                act("The earth trembles in response to your call.", FALSE, ch, 0, 0, TO_CHAR);
                if (!change_ldesc("is here, channeling the power of Ruk.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            }
            break;
        case GUILD_WATER_ELEMENTALIST:
            if (IS_TRIBE(ch, 24)) {
                act("A watery apparation appears near $n as $e begins " "casting.", FALSE, ch, 0, 0,
                    TO_ROOM);
                act("You call upon the maridan within you to power your spell.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here talking to a watery spirit.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                act("A fine mist condenses near $n as $e begins a spell.", FALSE, ch, 0, 0,
                    TO_ROOM);
                act("Mist surrounds you as you call upon the power of Vivadu.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here, surrounded by a fine mist.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            }
            break;
        case GUILD_FIRE_ELEMENTALIST:
            if (IS_TRIBE(ch, 24)) {
                act("A fiery apparation appears near $n as $e begins " "casting.", FALSE, ch, 0, 0,
                    TO_ROOM);
                act("You call upon the djinnling within you to power your spell.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here, talking with a fiery apparition.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                act("Flames erupt near $n as $e starts an incantation.", FALSE, ch, 0, 0, TO_ROOM);
                act("Flames erupt in response to your summons.", FALSE, ch, 0, 0, TO_CHAR);
                if (!change_ldesc("is here, surrounded by flames.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            }
            break;
        case GUILD_SHADOW_ELEMENTALIST:
            if (IS_TRIBE(ch, 24)) {
                act("A shadowy apparation appears near $n as $e begins " "casting.", FALSE, ch, 0,
                    0, TO_ROOM);
                act("You call upon the shadows within you to power your spell.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here, talking with a shadowy apparition.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                act("The air near $n grows dark as $e starts an incantation.", FALSE, ch, 0, 0,
                    TO_ROOM);
                act("The area dims as you begin your summons.", FALSE, ch, 0, 0, TO_CHAR);
                if (!change_ldesc("is here, in the midst of darkness.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            }
            break;
        case GUILD_LIGHTNING_ELEMENTALIST:
            if (IS_TRIBE(ch, 24)) {
                act("A figure made of energy appears before $n as $e begins " "casting.", FALSE, ch,
                    0, 0, TO_ROOM);
                act("You call upon a lightning djinn to power your spell.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here next to a being of energy.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                act("Swirls of energy dance around $n as $e starts an incantation.", FALSE, ch, 0,
                    0, TO_ROOM);
                act("Energy begins to swirl about you, as you call upon its power.", FALSE, ch, 0,
                    0, TO_CHAR);
                if (!change_ldesc("is here, surrounded by energy.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            }
            break;
        case GUILD_VOID_ELEMENTALIST:
            act("A stillness descends around $n as $e begins " "casting.", FALSE, ch, 0, 0,
                TO_ROOM);
            act("The air around you grows still as you call upon Nilaz to " "power your spell.",
                FALSE, ch, 0, 0, TO_CHAR);
            if (!change_ldesc("is here, surrounded in an airy silence.", ch))
                LDESC_TOO_LONG(MSTR(ch, name));
            break;
        case GUILD_PSIONICIST:
            act("$n closes $s eyes, focusing deeply, swirls of " "energy surrounding $m.", FALSE,
                ch, 0, 0, TO_ROOM);
            act("You reach deep within your mind towards the " "swirling energies there.", FALSE,
                ch, 0, 0, TO_CHAR);
            if (!change_ldesc("is here, swirls of energy dancing in the air.", ch))
                LDESC_TOO_LONG(MSTR(ch, name));
            break;
        case GUILD_LIRATHU_TEMPLAR:
        case GUILD_JIHAE_TEMPLAR:
        case GUILD_TEMPLAR:

#define USE_DYRINIS_MESSAGES 1

#ifdef USE_DYRINIS_MESSAGES
            if (ch->player.hometown == CITY_ALLANAK) {
                char temp[100];
                char command1[100];
                /* char errmsg[200]; unused -Morg */
                char symbols[MAX_STRING_LENGTH];
                int power_return, reach_return, spell, qend, i;

/*
          gamelog("DYRINIS MESSAGES START");
          gamelog(arg);
*/

                while (arg && *arg == ' ')
                    arg++;

                /* now for the all new, better than ever spell parser */
                if (*arg == '\'') {
                    if ((qend = close_parens(arg, '\''))) {
                        sub_string(arg, symbols, 1, qend - 1);
                        for (i = 0; *(symbols + i) != '\0'; i++)
                            *(symbols + i) = LOWER(*(symbols + i));
                        find_spell(symbols, &power_return, &reach_return, &spell);
                    }
                }

                if (power_return >= POWER_SUL) {
                    strcpy(temp, "shout");
                } else {
                    strcpy(temp, "say");
                }

                switch (spell) {

                case SPELL_IMMOLATE:
                    sprintf(command1,
                            "%s Burn, heathen, with the wrath of the Highlord Tektolnes!", temp);
                    parse_command(ch, command1);
                    break;
                case SPELL_PARCH:
                    sprintf(command1,
                            "%s By the power of Tektolnes, drain the water of life from the enemies of the King!", temp);
                    parse_command(ch, command1);
                    break;
                case SPELL_CURSE:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "I cast a curse upon you!", temp);
                    parse_command(ch, command1);
                    break;


                case SPELL_DETECT_INVISIBLE:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "let those hidden by magicks be rendered visible!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_DETECT_POISON:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "let the venoms concealed in this substance be revealed!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_DETECT_MAGICK:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "let my eyes see true foul magicks hidden from me!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_DETECT_ETHEREAL:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "let those concealed by Drovian shadows be revealed!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_CREATE_WATER:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "bestow upon me sacred and pure waters!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_LIGHT:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "let this light penetrate the darkness!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_INFRAVISION:
                    sprintf(command1,
                            "%s Mighty Tektolnes, allow my eyes "
                            "the power to penetrate all darkness!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_CREATE_FOOD:
                    sprintf(command1,
                            "%s In the name of the Highlord, " "the feast of Ruk shall appear!",
                            temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_CURE_BLINDNESS:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "purge this magickal ailment of the eyes!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_CURE_POISON:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "strike this sickness from the blood of the King's allies!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_STRENGTH:
                    sprintf(command1,
                            "%s By the power of Tektolnes, "
                            "allow your servant the strength of a half-giant!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_WEAKEN:
                    sprintf(command1,
                            "%s In the name of Mighty King "
                            "Tektolnes, strike the strength from the limbs of this heretic!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_ARMOR:
                    sprintf(command1,
                            "%s In the name of the Highlord, I "
                            "call forth the protection of the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_REPEL:
                    sprintf(command1,
                            "%s In the name of the Highlord, " "remove this infidel from my sight!",
                            temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_FLAMESTRIKE:
                    sprintf(command1,
                            "%s In the name of Tektolnes, let "
                            "these flames strike dead the enemies of the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_ALARM:
                    sprintf(command1,
                            "%s In the name of the Highlord, I "
                            "command magickal glyphs of warning upon this place!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_AURA_DRAIN:
                    sprintf(command1,
                            "%s In the name of the Highlord, I "
                            "command thee to surrender thy magick to the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_FEATHER_FALL:
                    sprintf(command1,
                            "%s By the power of Tektolnes, " "allow me the buoyancy of a feather!",
                            temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_BLIND:
                    sprintf(command1,
                            "%s In the name of Mighty King "
                            "Tektolnes, strike blind the enemies of the Highlord!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_FIREBALL:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "let this ball of fire consume the enemies of the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_DISPEL_MAGICK:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "let the foul magicks present here be cleansed!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_TONGUES:
                    sprintf(command1,
                            "%s Mighty Tektolnes, allow me to "
                            "comprehend the impure languages of these mortals!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_BANISHMENT:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "these heretics shall begone from Tektolnes' sacred ground!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_CALM:
                    sprintf(command1,
                            "%s By the might of the Highlord, "
                            "let this savage creature be rendered tame!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_HANDS_OF_WIND:
                    sprintf(command1,
                            "%s In the name of the Highlord, I "
                            "command Whira to submit her guiding winds to me!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_FURY:
                    sprintf(command1,
                            "%s In the name of Mighty "
                            "Tektolnes, allow your servant the endurance of ten elves!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_GLYPH:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "enchant this item with protective glyphs!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_SLOW:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "strike lethargy into the limbs of the enemies of the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_PSI_SUPPRESSION:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "render the psychic enemy of this heretic useless!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_INSOMNIA:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "render this disbeliever incapable of sleep!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_SLEEP:
                    sprintf(command1,
                            "%s In the name of Mighty King "
                            "Tektolnes, render unconscious the enemies of the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_WALL_OF_SAND:
                    sprintf(command1,
                            "%s By the might of Tektolnes, "
                            "erect a wall of sand on these premises!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_DEAFNESS:
                    sprintf(command1,
                            "%s In the name of Mighty King "
                            "Tektolnes, strike deaf the enemies of the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_LIGHTNING_BOLT:
                    sprintf(command1,
                            "%s By the might of Tektolnes, let "
                            "this lightning strike dead the enemies of the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_SILENCE:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "let a spell of silence descend on the enemies of the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_STAMINA_DRAIN:
                    sprintf(command1,
                            "%s In the name of Tektolnes, I "
                            "command thee to surrender thy energy to the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_EARTHQUAKE:
                    sprintf(command1,
                            "%s By the will of Mighty King "
                            "Tektolnes, the earth shall split asunder at my command!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_INVISIBLE:
                    sprintf(command1,
                            "%s By the power of Tektolnes, "
                            "render me invisible to the eyes of others!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_POISON:
                    sprintf(command1,
                            "%s By the name of the Mighty "
                            "Tektolnes, venom shall afflict the enemies of the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_SANCTUARY:
                    sprintf(command1,
                            "%s By the will of Tektolnes, "
                            "Vivadu shall allow me her protective power!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_STONE_SKIN:
                    sprintf(command1,
                            "%s By the will of Tektolnes, Ruk "
                            "shall shield me with skin of stone!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_ETHEREAL:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "allow me to walk the shadows of Drov!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_GUARDIAN:
                    sprintf(command1,
                            "%s In the name of Mighty King "
                            "Tektolnes, Whira shall bring her servants to my side!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_THUNDER:
                    sprintf(command1,
                            "%s In the name of Tektolnes, let "
                            "thunder issue forth proclaiming his might!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_HEALTH_DRAIN:
                    sprintf(command1,
                            "%s In the name of Tektolnes, I "
                            "command thee to surrender thy health to the King!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_GODSPEED:
                    sprintf(command1,
                            "%s By the might of Tektolnes, "
                            "allow me to move with the speed of the wind!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_DETERMINE_RELATIONSHIP:
                    sprintf(command1,
                            "%s Mighty Tektolnes, how stands "
                            "this lowly mortal in relationship to the land?", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_FEAR:
                    sprintf(command1,
                            "%s Mighty Tektolnes, let thy might "
                            "strike fear into the hearts of thy enemies!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_HEAL:
                    sprintf(command1,
                            "%s In the name of the Highlord, I " "command these wounds to close!",
                            temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_LEVITATE:
                    sprintf(command1,
                            "%s In the name of the Highlord, "
                            "the winds of Whira shall lift me off my feet!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_FEEBLEMIND:
                    sprintf(command1,
                            "%s In the name of Mighty King "
                            "Tektolnes, render this infidel's mind that of an insect!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_RAIN_OF_FIRE:
                    sprintf(command1,
                            "%s In the name of Mighty King "
                            "Tektolnes, flames from the heavens shall pour forth death upon the "
                            "enemies of the Highlord!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_SUMMON:
                    sprintf(command1,
                            "%s In the name of Tektolnes, I "
                            "command the winds of Whira to summon this mortal!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_RELOCATE:
                    sprintf(command1, "%s Mighty Tektolnes, move me as " "your will commands!",
                            temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_ANIMATE_DEAD:
                    sprintf(command1,
                            "%s These mortal corpses shall rise "
                            "to do the bidding of the Highlord Tektolnes!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_GATE:
                    sprintf(command1,
                            "%s In the name of Mighty King "
                            "Tektolnes, the demons of Nilaz shall obey my commands!", temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_DEMONFIRE:
                    sprintf(command1,
                            "%s By the might of Tektolnes, I "
                            "shall summon fires from the Hell Pits to strike dead the enemies of the King!",
                            temp);
                    parse_command(ch, command1);
                    break;

                case SPELL_FLY:
                    sprintf(command1,
                            "%s In the name of the Highlord,"
                            "Whira shall lend me her wings of flight!", temp);
                    parse_command(ch, command1);
                    break;

                default:
                    act("$n calls out the name of the Highlord.", FALSE, ch, 0, 0, TO_ROOM);
                    act("You call out the name of the Highlord Tektolnes.", FALSE, ch, 0, 0,
                        TO_CHAR);
                    if (!change_ldesc("is here, invoking Tektolnes' power.", ch))
                        LDESC_TOO_LONG(MSTR(ch, name));
                    break;
                };

/*
          sprintf(errmsg, "command = %s", command1);
          gamelog(errmsg);
*/

                if (!change_ldesc("is here, invoking Tektolnes' power.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
                break;

            } else if (ch->player.hometown == CITY_TULUK) {
                act("$n utters the name of Muk Utep.", FALSE, ch, 0, 0, TO_ROOM);
                act("You draw power from Muk Utep for your spell.", FALSE, ch, 0, 0, TO_CHAR);
                if (!change_ldesc("is here, calling on the power of Utep.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else if (ch->player.hometown == CITY_MAL_KRIAN) {
                act("$n invokes the name of the council.", FALSE, ch, 0, 0, TO_ROOM);
                act("You summon power from the council to cast your spell.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here, calling on the council's" " power.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                act("$n calls upon the power of $s King.", FALSE, ch, 0, 0, TO_ROOM);
                act("You call upon the power of your King.", FALSE, ch, 0, 0, TO_CHAR);
                if (!change_ldesc("is here, calling upon the King's power.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            }
            break;
#else

            if (ch->player.hometown == CITY_ALLANAK) {
                act("$n calls out the name of the Highlord Tektolnes.", FALSE, ch, 0, 0, TO_ROOM);
                act("You call upon the Highlord Tektolnes for your magick.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here, invoking Tektolnes power.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else if (ch->player.hometown == CITY_TULUK) {
                act("$n utters the name of Muk Utep.", FALSE, ch, 0, 0, TO_ROOM);
                act("You draw power from Muk Utep for your spell.", FALSE, ch, 0, 0, TO_CHAR);
                if (!change_ldesc("is here, calling on the power of Utep.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else if (ch->player.hometown == CITY_MAL_KRIAN) {
                act("$n invokes the name of the council.", FALSE, ch, 0, 0, TO_ROOM);
                act("You summon power from the council to cast your spell.", FALSE, ch, 0, 0,
                    TO_CHAR);
                if (!change_ldesc("is here, calling on the council's" " power.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                act("$n calls upon the power of $s King.", FALSE, ch, 0, 0, TO_ROOM);
                act("You call upon the power of your King.", FALSE, ch, 0, 0, TO_CHAR);
                if (!change_ldesc("is here, calling upon the King's power.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            }
            break;
#endif
        case GUILD_DEFILER:
            if (IS_TRIBE(ch, TRIBE_AZIA)) {

                act("Eerie apparitions coil in the air around $n as $e draws " "upon $s magick.",
                    FALSE, ch, 0, 0, TO_ROOM);
                act("Eerie apparitions coil in the air around you as you draw " "upon your magick.",
                    FALSE, ch, 0, 0, TO_CHAR);
                if (!change_ldesc("is here, surrounded by wavering " "apparitions.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                if (IS_TRIBE(ch, 89)) {
                    act("An aura of white light surrounds $n as $e begins " "casting $s magick.",
                        FALSE, ch, 0, 0, TO_ROOM);
                    act("An aura of white light surrounds you as you being " "casting your magick.",
                        FALSE, ch, 0, 0, TO_CHAR);
                    if (!change_ldesc("is here, surrounded by a white aura.", ch))
                        LDESC_TOO_LONG(MSTR(ch, name));
                } else {


                    act("Magickal currents begin to swirl around $n.", FALSE, ch, 0, 0, TO_ROOM);
                    act("Currents of magick begin to gather around you.", FALSE, ch, 0, 0, TO_CHAR);
                    if (!change_ldesc("is here surrounded by currents of magick.", ch))
                        LDESC_TOO_LONG(MSTR(ch, name));
                }
            }
            break;

        default:
            if (GET_RACE(ch) == RACE_PLAINSOX) {
                act("$n stamps a heavy hoof on the ground, which rumbles in " "answer.", FALSE, ch,
                    0, 0, TO_ROOM);
                act("You stamp a heavy hoof on the ground, which rumbles in " "answer.", FALSE, ch,
                    0, 0, TO_CHAR);
                if (!change_ldesc("is standing here, the ground shaking " "beneath him.", ch))
                    LDESC_TOO_LONG(MSTR(ch, name));
            } else {
                if (IS_TRIBE(ch, 89)) {
                    act("An aura of white light surrounds $n as $e begins " "casting $s magick.",
                        FALSE, ch, 0, 0, TO_ROOM);
                    act("An aura of white light surrounds you as you being " "casting your magick.",
                        FALSE, ch, 0, 0, TO_CHAR);
                    if (!change_ldesc("is here, surrounded by a white aura.", ch))
                        LDESC_TOO_LONG(MSTR(ch, name));
                } else {
                    act("Magickal currents begin to swirl around $n.", FALSE, ch, 0, 0, TO_ROOM);
                    act("Currents of magick begin to gather around you.", FALSE, ch, 0, 0, TO_CHAR);
                    if (!change_ldesc("is here surrounded by currents of magick.", ch))
                        LDESC_TOO_LONG(MSTR(ch, name));
                }
            }
            break;
        } // End switch
       } // End if/else
       if (spell && has_skill(ch, spell)) {
         if (GET_GUILD(ch) != GUILD_LIRATHU_TEMPLAR && 
             GET_GUILD(ch) != GUILD_JIHAE_TEMPLAR &&
             !IS_IMMORTAL(ch) &&
             room_in_city(ch->in_room) == CITY_TULUK) {
           int delay = MAX(skill_delay(ch->skills[spell]->learned), 3);
           d->timeout = delay;
         } else {
           d->timeout = skill_delay(ch->skills[spell]->learned);
         }
       } else {
         d->timeout = MAX_DELAY;
       }
       break;

    case CMD_SILENT_CAST:
        act("You quietly focus on channelling the necessary magickal energies.", FALSE, ch, 0, 0,
            TO_CHAR);
        if (spell && has_skill(ch, spell)) {
          if (GET_GUILD(ch) != GUILD_LIRATHU_TEMPLAR && 
              GET_GUILD(ch) != GUILD_JIHAE_TEMPLAR &&
             !IS_IMMORTAL(ch) &&
              room_in_city(ch->in_room) == CITY_TULUK) {
            int delay = MAX(skill_delay(ch->skills[spell]->learned), 3);
            d->timeout = delay;
          } else {
            d->timeout = skill_delay(ch->skills[spell]->learned);
          }
        } else {
          d->timeout = MAX_DELAY;
        }
        break;

    case CMD_REPAIR:
        d->timeout = 3;
        break;

    case CMD_SENSE_PRESENCE:
        d->timeout = 3;
        break;

    case CMD_SPLIT:            /* npc attack delays */
        if (!ch->specials.alt_fighting)
            act("You begin looking for an opportunity...\n\r", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_SPLIT_OPPONENTS))
            d->timeout = skill_delay(ch->skills[SKILL_SPLIT_OPPONENTS]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_HARNESS:
        d->timeout = spell;
        break;

    case CMD_PILOT:
    case CMD_UNHARNESS:
        d->timeout = spell;
        break;

    case CMD_REJUVENATE:
        act("You start focussing your mental energies within yourself.", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, PSI_REJUVENATE))
            d->timeout = skill_delay(ch->skills[PSI_REJUVENATE]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_HIDE:
        act("You search for a good place to hide.", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_HIDE))
            d->timeout = skill_delay(ch->skills[SKILL_HIDE]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_BACKSTAB:
        act("You begin moving silently toward your victim.", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_BACKSTAB))
            d->timeout = skill_delay(ch->skills[SKILL_BACKSTAB]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_SAP:
        act("You begin moving silently toward your victim.", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_SAP))
            d->timeout = skill_delay(ch->skills[SKILL_SAP]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_PICK:
        act("$n sets to work on a lock.", TRUE, ch, 0, 0, TO_ROOM);
        act("You set to work on the lock.", FALSE, ch, 0, 0, TO_CHAR);
        if (!change_ldesc("is standing here, working on a lock.", ch))
            LDESC_TOO_LONG(MSTR(ch, name));
        if (has_skill(ch, SKILL_PICK_LOCK))
            d->timeout = skill_delay(ch->skills[SKILL_PICK_LOCK]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_SLIP:
        act("You take your time and focus on slipping it away.", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_SLEIGHT_OF_HAND))
            d->timeout = skill_delay(ch->skills[SKILL_SLEIGHT_OF_HAND]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_READY:
        act("You take your time and focus on readying it.", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_SLEIGHT_OF_HAND))
            d->timeout = skill_delay(ch->skills[SKILL_SLEIGHT_OF_HAND]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_STOW:
        act("You take your time and focus on stowing it.", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_SLEIGHT_OF_HAND))
            d->timeout = skill_delay(ch->skills[SKILL_SLEIGHT_OF_HAND]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_PALM:
        act("You take your time and focus on palming it.", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_SLEIGHT_OF_HAND))
            d->timeout = skill_delay(ch->skills[SKILL_SLEIGHT_OF_HAND]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_LATCH:
      act("You take your time and focus on latching it.", FALSE, ch, 0, 0, TO_CHAR);
      if (has_skill(ch, SKILL_SLEIGHT_OF_HAND)) {
        d->timeout = skill_delay(ch->skills[SKILL_SLEIGHT_OF_HAND]->learned);
      } else {
        d->timeout = MAX_DELAY;
      }
      break;
      
    case CMD_UNLATCH:
        act("You take your time and focus on unlatching it.", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_SLEIGHT_OF_HAND))
            d->timeout = skill_delay(ch->skills[SKILL_SLEIGHT_OF_HAND]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_STEAL:
        act("You quietly approach your target.", FALSE, ch, 0, 0, TO_CHAR);
        arg = one_argument(arg, tmp_char, sizeof(tmp_char));
        arg = one_argument(arg, target, sizeof(target));
        if (((victim = (get_char_room_vis(ch, target))))
            && (GET_POS(victim) <= POSITION_SLEEPING)) {
            /*            send_to_char("Vict is sleeping, and found.", ch); */
            if (has_skill(ch, SKILL_STEAL))
                d->timeout = skill_delay(20);
            else
                d->timeout = skill_delay(6);
        } else {
            if (has_skill(ch, SKILL_STEAL))
                d->timeout = skill_delay(ch->skills[SKILL_STEAL]->learned);
            else
                d->timeout = MAX_DELAY;
        }
        break;

    case CMD_HUNT: {
        char emote[MAX_STRING_LENGTH];
        char ldesc[MAX_STRING_LENGTH];
        
        if( !get_char_extra_desc_value(ch, "[HUNT_EMOTE]", emote, MAX_STRING_LENGTH) )
            strcpy( emote, " crouches down and looks for tracks.");

        if( !get_char_extra_desc_value(ch, "[HUNT_LDESC]", ldesc, MAX_STRING_LENGTH) )
            strcpy( ldesc, "is crouching here, looking for tracks.");

        cmd_emote(ch, emote, CMD_EMOTE, 0);
        if (!change_ldesc(ldesc, ch))
            LDESC_TOO_LONG(MSTR(ch, name));
        if (has_skill(ch, SKILL_HUNT))
            d->timeout = skill_delay(ch->skills[SKILL_HUNT]->learned);
        else {
            if (GET_RACE(ch) == RACE_DESERT_ELF)
                d->timeout = skill_delay(30);
            else
                d->timeout = MAX_DELAY;
        }
        break;
    }

    case CMD_SHOOT:
        act("$n steadies $mself and takes aim.", TRUE, ch, 0, 0, TO_ROOM);
        act("You take aim at your target...", FALSE, ch, 0, 0, TO_CHAR);
        if (has_skill(ch, SKILL_ARCHERY))
            /* if (ch->skills[SKILL_ARCHERY]) */
            d->timeout = skill_delay(ch->skills[SKILL_ARCHERY]->learned);
        else
            d->timeout = skill_delay(15);
        break;

    case CMD_THROW:
        if (ch->specials.subduing) {
            /* if throwing something subdued, just do it */
            d->character->specials.to_process = TRUE;
            perform_command(d->character, d->command, d->argument, d->count);
            d->character->specials.to_process = FALSE;
            rem_from_command_q(ch);
            free(d->argument);
            free(d);
            return;
        }

        act("$n steadies $mself and takes aim.", TRUE, ch, 0, 0, TO_ROOM);
        act("You take aim at your target...", FALSE, ch, 0, 0, TO_CHAR);

        if (has_skill(ch, SKILL_THROW))
            /* if (ch->skills[SKILL_THROW]) */
            d->timeout = skill_delay(ch->skills[SKILL_THROW]->learned);
        else
            d->timeout = skill_delay(15);
        break;

    case CMD_SEARCH:
        act("$n begins searching the area intently.", TRUE, ch, 0, 0, TO_ROOM);
        act("You begin to search the area...", FALSE, ch, 0, 0, TO_CHAR);
        if (!change_ldesc("is here searching the area carefully.", ch))
            LDESC_TOO_LONG(MSTR(ch, name));
        if (has_skill(ch, SKILL_SEARCH))
            /* if (ch->skills[SKILL_SEARCH]) */
            d->timeout = skill_delay(ch->skills[SKILL_SEARCH]->learned);
        else
            d->timeout = MAX_DELAY;
        break;

    case CMD_BREW:
        d->timeout = 30;        /* This should take a long time to do */
        break;

    case CMD_MAKE:
        d->timeout = 5;         /* rolling smokes, pitching tents */
        break;

    case CMD_LOCK:
    case CMD_UNLOCK:
        d->timeout = ch->specials.act_wait / WAIT_SEC;
        ch->specials.act_wait = 0;
        break;

    case CMD_FORAGE:
        {
            char arg1[MAX_INPUT_LENGTH];

            arg = one_argument(arg, arg1, sizeof(arg1));

            if (has_skill(ch, SKILL_FORAGE))
                /* if (ch->skills[SKILL_FORAGE]) */
                d->timeout = skill_delay(ch->skills[SKILL_FORAGE]->learned);
            else {
                if (GET_RACE(ch) == RACE_DESERT_ELF)
                    d->timeout = skill_delay(30);
                else
                    d->timeout = MAX_DELAY;
            }

            if (!change_ldesc("is here searching the area carefully.", ch))
                LDESC_TOO_LONG(MSTR(ch, name));

            act("$n begins searching the area intently.", TRUE, ch, 0, 0, TO_ROOM);
            act("You begin searching the area intently.", FALSE, ch, 0, 0, TO_CHAR);
            d->timeout = number(1, 2);
            break;
        }

    default:
        gamelog("Invalid command in add_to_command_q() switch block.");
        break;
    }
    d->next = d_com_list;
    d_com_list = d;
}

