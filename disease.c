/*
 * File: DISEASE.C
 * Usage: Commands involving character diseases.
 *
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 * ---------------------------------------------------------------------
 */

/* Revision history.  Pls update as changes made.
 * Created - 08/09/2006 - Halaster
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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

/*  extern procedures  */
extern int get_normal_condition(CHAR_DATA * ch, int cond);


/* Return if a char is affected by a disease (DISEASE_XXX)
 * returns the affect if found, otherwise NULL indicates not affected */
struct affected_type *
affected_by_disease(CHAR_DATA * ch, int skill)
{
    struct affected_type *aff_node = NULL;

    if (ch->affected)
        for (aff_node = ch->affected; aff_node; aff_node = aff_node->next)
            if (aff_node->type == skill)
                break;

    return (aff_node);
}


/* Returns TRUE if they resist the disease and should not be affected,
 * returns FALSE otherwise */
bool
resist_disease(CHAR_DATA *ch, int disease) {

    int resist_chance = 0;

    resist_chance = end_app[GET_END(ch)].end_saves;

    switch (disease) {
    case DISEASE_KRATHS_TOUCH:
        resist_chance += ch->specials.eco / 2;
        break;
    case DISEASE_SCRUB_FEVER:
        resist_chance += ch->specials.eco / 2;
        resist_chance += GET_COND(ch, DRUNK);
        break;
    default:
        break;
    }

    /* Minimum of 5% chance to resist*/
    resist_chance = MAX(5, resist_chance);

    /* Maximum of 95% chance to resist */
    resist_chance = MIN(95, resist_chance);

    if (number(1, 100) <= resist_chance)
        return TRUE;

    return FALSE;

}


/* Return FALSE if ch is immune to disease, TRUE otherwise */
bool
can_be_diseased(CHAR_DATA * ch)
{

    if ((IS_IMMORTAL(ch))
        || (IS_SET(ch->specials.act, CFL_UNDEAD))
        || (GET_RACE(ch) == RACE_THORNWALKER)
        || (GET_RACE(ch) == RACE_ELEMENTAL)
        || (GET_RACE(ch) == RACE_SUB_ELEMENTAL)
        || (GET_RACE(ch) == RACE_MEPHIT)
        || (GET_RACE(ch) == RACE_DEMON)
        || (GET_RACE(ch) == RACE_FIEND)
        || (GET_RACE(ch) == RACE_VAMPIRE))

        return FALSE;

    return TRUE;

}


/* Apply Krath's Touch if they meet the chance.  Returns FALSE if
 * it gets applied, TRUE if it does not get applied. */
int
apply_kraths_touch(CHAR_DATA * ch)
{

    struct affected_type af;
    int chance = 1;

    memset(&af, 0, sizeof(af));

    switch (ch->in_room->sector_type) {
    case SECT_SILT:
    case SECT_SHALLOWS:
    case SECT_SALT_FLATS:
    case SECT_DESERT:
        chance += 2;
        break;
    case SECT_FIELD:
    case SECT_COTTONFIELD:
        chance += 1;
        break;
    case SECT_INSIDE:
    case SECT_CAVERN:
    case SECT_SEWER:
    case SECT_SOUTHERN_CAVERN:
        chance = 0;
        break;
    default:
        break;
    }

    if (number(1, 500) <= chance) {

        if (resist_disease(ch, DISEASE_KRATHS_TOUCH))
            return TRUE;

        af.type = DISEASE_KRATHS_TOUCH;
        af.location = 0;
        af.duration = 9;       /* one day */
        af.modifier = 0;
        af.bitvector = 0;
        af.power = 0;
        af.magick_type = 0;
        affect_to_char(ch, &af);

        shhlogf("Applying the disease Krath's Touch to %s", GET_NAME(ch));
        parse_command(ch, "feel suddenly overwhelmed and ill from the heat.");
        return FALSE;

    }

    return TRUE;

}


/* Apply Scrub Fever if they meet the chance.  Returns FALSE if
 * it gets applied, TRUE if it does not get applied. */
int
apply_scrub_fever(CHAR_DATA * ch) {

    struct affected_type af;
    int chance = 1;

    memset(&af, 0, sizeof(af));

    switch (ch->in_room->sector_type) {
    case SECT_GRASSLANDS:
        chance += 1;
        break;
    case SECT_LIGHT_FOREST:
    case SECT_HEAVY_FOREST:
        break;
    default:
        chance = 0;
        break;
    }

    /* Best chance is if wind is between 5 and 9, then 0-4 and 10-14,
     * then 15 to 24 is base chance. */
    if (weather_info.wind_velocity < 5)
         chance += 1;
    else if (weather_info.wind_velocity < 10)
         chance += 2;
    else if (weather_info.wind_velocity < 15)
         chance += 1;

    if (number(1, 60000) <= chance) {

        if (resist_disease(ch, DISEASE_SCRUB_FEVER))
            return TRUE;

        af.type = DISEASE_SCRUB_FEVER;
        af.location = 0;
        af.duration = 27;       /* three days */
        af.modifier = 0;
        af.bitvector = 0;
        af.power = 0;
        af.magick_type = 0;
        affect_to_char(ch, &af);

        set_char_extra_desc_value(ch, "[SCRUB_FEVER_STAGE]", "1");
        shhlogf("Applying the disease Scrub Fever to %s", GET_NAME(ch));
        send_to_monitorf(ch, MONITOR_FIGHT, "Applying the disease Scrub Fever to %s", GET_NAME(ch));
        return FALSE;
    }

    return TRUE;
}


/* Apply the various diseases if the conditions are right. Returns
 * TRUE if they do not get a disease, returns FALSE if they get a disease */
int
disease_check(CHAR_DATA * ch)
{
    int heat = 0;

    if (!ch || !ch->in_room)
        return TRUE;

    struct weather_node *wn = find_weather_node(ch->in_room);

    if (wn)
        heat = wn->temp;

    if (!can_be_diseased(ch))
        return TRUE;

    if ((!affected_by_disease(ch, DISEASE_KRATHS_TOUCH))
        && (heat >= 95)
        && (GET_GUILD(ch) != GUILD_FIRE_ELEMENTALIST)
        && (GET_COND(ch, THIRST) <= (get_normal_condition(ch, THIRST) * .25))
        && (!IS_SET(ch->in_room->room_flags, RFL_INDOORS))
        && (!IS_SET(ch->in_room->room_flags, RFL_RESTFUL_SHADE))
        && (!IS_SET(ch->in_room->room_flags, RFL_DARK))
        && ((time_info.hours >= RISING_SUN) && (time_info.hours <= SETTING_SUN))) {
        return apply_kraths_touch(ch);
    }

    if ((!affected_by_disease(ch, DISEASE_SCRUB_FEVER))
            && (time_info.month == 2)
            && (GET_COND(ch,DRUNK) < 5)
            && (GET_RACE(ch) != RACE_MANTIS)
            && (GET_RACE(ch) != RACE_HALFLING)
            && (weather_info.wind_velocity < 25)
            && ((ch->in_room->sector_type == SECT_GRASSLANDS)
                || (ch->in_room->sector_type == SECT_LIGHT_FOREST)
                || (ch->in_room->sector_type == SECT_HEAVY_FOREST))) {
        return apply_scrub_fever(ch);
    }

    return TRUE;

}


void
update_kraths_touch(CHAR_DATA * ch, struct affected_type *af)
{

    struct affected_type newaf;

    memset(&newaf, 0, sizeof(newaf));

    /* They're hydrated, so start quickly reducing the affect */
    if (GET_COND(ch, THIRST) >= (get_normal_condition(ch, THIRST) * .33)) {
        if (!number(0, 10))
            af->expiration -= RT_ZAL_HOUR;
        /* Extra chance if also indoors and hydrated */
        if (IS_SET(ch->in_room->room_flags, RFL_INDOORS)) {
            if (!number(0, 10))
                af->expiration -= RT_ZAL_HOUR;
        }
        /* Another extra chance if also in a restful shade while hydrated */
        if (IS_SET(ch->in_room->room_flags, RFL_RESTFUL_SHADE)) {
            if (!number(0, 10))
                af->expiration -= RT_ZAL_HOUR;
        }
        return;
    }

    /* Less than 5% of max thirst */
    if (GET_COND(ch, THIRST) <= (get_normal_condition(ch, THIRST) * .05)) {
        switch (number(0, 300)) {
        case 0:
            parse_command(ch, "feel quite faint and out of breath.");
            act("$n looks pale and clammy as $s breathing grows shallow.", TRUE, ch, 0, 0,
                TO_ROOM);
            break;
        case 1:
            parse_command(ch, "feel an overwhelming wave of nausea causing you to gag.");
            act("$n spasms a little, gagging.", FALSE, ch, 0, 0, TO_ROOM);
            break;
        case 2:
            parse_command(ch, "feel a tremendous headache at the front of your brow.");
            /* 1 wisdom loss for 1 to 3 hours */
            newaf.type = TYPE_HUNGER_THIRST;
            newaf.duration = number(1, 3);
            newaf.power = 0;
            newaf.magick_type = 0;
            newaf.modifier = -1;
            newaf.location = CHAR_APPLY_WIS;
            newaf.bitvector = 0;
            affect_to_char(ch, &newaf);
            break;
        case 3:
            parse_command(ch, "feel a strong dizzyness, barely able to concentrate.");
            act("$n looks disoriented and confused.", TRUE, ch, 0, 0, TO_ROOM);
            /* 5 to 10 points stun loss for 2 to 4 hours */
            newaf.type = TYPE_HUNGER_THIRST;
            newaf.duration = number(2, 4);
            newaf.power = 0;
            newaf.magick_type = 0;
            newaf.modifier = -(number(5, 10));
            newaf.location = CHAR_APPLY_STUN;
            newaf.bitvector = 0;
            affect_to_char(ch, &newaf);
            break;
        case 4:
            parse_command(ch, "feel strong, painful muscle cramps in the legs.");
            /* 1 to 2 endurance loss for 1 to 3 hours */
            newaf.type = TYPE_HUNGER_THIRST;
            newaf.duration = number(1, 3);
            newaf.power = 0;
            newaf.magick_type = 0;
            newaf.modifier = -(number(1, 2));
            newaf.location = CHAR_APPLY_END;
            newaf.bitvector = 0;
            affect_to_char(ch, &newaf);
            break;
        case 5:
            parse_command(ch, "feel strong, painful muscle cramps in the arms and neck.");
            /* 1 to 2 strength loss for 1 to 3 hours */
            newaf.type = TYPE_HUNGER_THIRST;
            newaf.duration = number(1, 3);
            newaf.power = 0;
            newaf.magick_type = 0;
            newaf.modifier = -(number(1, 2));
            newaf.location = CHAR_APPLY_STR;
            newaf.bitvector = 0;
            affect_to_char(ch, &newaf);
            break;
        default:
            break;
        }
    }

    /* Less than 12% of max thirst */
    else if (GET_COND(ch, THIRST) <= (get_normal_condition(ch, THIRST) * .12)) {
        switch (number(0, 300)) {
        case 0:
            parse_command(ch, "feel quite faint and out of breath.");
            act("$n looks pale as $s breathing grow a little shallow.", TRUE, ch, 0, 0, TO_ROOM);
            break;
        case 1:
            parse_command(ch, "feel a sudden, strong wave of nausea.");
            break;
        case 2:
            parse_command(ch, "feel a strong headache at the front of your brow.");
            /* 1 wisdom loss for 1 to 3 hours */
            newaf.type = TYPE_HUNGER_THIRST;
            newaf.duration = number(1, 3);
            newaf.power = 0;
            newaf.magick_type = 0;
            newaf.modifier = -1;
            newaf.location = CHAR_APPLY_WIS;
            newaf.bitvector = 0;
            affect_to_char(ch, &newaf);
            break;
        case 3:
            parse_command(ch, "feel a strong sensation of dizzyness and vertigo.");
            /* 5 to 10 points stun loss for 2 to 4 hours */
            newaf.type = TYPE_HUNGER_THIRST;
            newaf.duration = number(2, 4);
            newaf.power = 0;
            newaf.magick_type = 0;
            newaf.modifier = -(number(5, 10));
            newaf.location = CHAR_APPLY_STUN;
            newaf.bitvector = 0;
            affect_to_char(ch, &newaf);
            break;
        case 4:
            parse_command(ch, "feel faint as a growing weakness takes over.");
            break;
        default:
            break;
        }
    }

    /* Less than 20% of max thirst */
    else if (GET_COND(ch, THIRST) <= (get_normal_condition(ch, THIRST) * .20)) {
        switch (number(0, 300)) {
        case 0:
            parse_command(ch, "feel faint and begin to sweat profusely.");
            act("$n looks pale as sweat pours profusely from $s brow.", TRUE, ch, 0, 0, TO_ROOM);
            break;
        case 1:
            parse_command(ch, "feel a sudden wave of nausea.");
            break;
        case 2:
            parse_command(ch, "feel a headache at the front of your brow.");
            break;
        case 3:
            parse_command(ch, "feel a sensation of dizzyness and vertigo.");
            /* 5 to 10 points stun loss for 2 to 4 hours */
            newaf.type = TYPE_HUNGER_THIRST;
            newaf.duration = number(2, 4);
            newaf.power = 0;
            newaf.magick_type = 0;
            newaf.modifier = -(number(5, 10));
            newaf.location = CHAR_APPLY_STUN;
            newaf.bitvector = 0;
            affect_to_char(ch, &newaf);
            break;
        case 4:
            parse_command(ch, "feel faint as a growing weakness takes over.");
            break;
        default:
            break;
        }
    }

    /* Less than 25% of max thirst */
    else if (GET_COND(ch, THIRST) <= (get_normal_condition(ch, THIRST) * .25)) {
        switch (number(0, 300)) {
        case 0:
            parse_command(ch, "feel a little faint as sweat pours from your brow.");
            act("$n looks a little pale as sweat pours from $s brow.", TRUE, ch, 0, 0, TO_ROOM);
            break;
        case 1:
            parse_command(ch, "feel a sudden, mild wave of nausea.");
            break;
        case 2:
            parse_command(ch, "feel a mild headache at the front of your brow.");
            break;
        case 3:
            parse_command(ch, "feel a mild sensation of dizzyness.");
            break;
        default:
            break;
        }
    }
}


void
update_scrub_fever(CHAR_DATA * ch, struct affected_type *af)
{

    struct affected_type newaf;
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int sfstage = 0;

    memset(&newaf, 0, sizeof(newaf));

    /* Their drunk value as a % to get rid of an hour of it */
    if (number(1,100) <= GET_COND(ch, DRUNK))
        af->expiration -= RT_ZAL_HOUR;

    /* If they have alcohol in their system, and they're at rest an additional
     * chance to lose an hour of the affect */
    if (((GET_COND(ch,DRUNK) >= 5) && (GET_POS(ch) <= POSITION_SITTING))) {
        if (!number(0, 10))
            af->expiration -= RT_ZAL_HOUR;
    }

    if (GET_COND(ch,DRUNK) < 5) {
        if (TRUE == get_char_extra_desc_value(ch, "[SCRUB_FEVER_STAGE]", buf1, MAX_STRING_LENGTH))
            sfstage = atoi(buf1);
        else
            set_char_extra_desc_value(ch, "[SCRUB_FEVER_STAGE]", "1");

        if (sfstage == 1) {
            switch (number(0, 1800)) {
            case 0:
            case 1:
                parse_command(ch, "feel a wave of mild nausea.");
                break;
            case 2:
            case 3:
                parse_command(ch, "feel moisture as snot runs out your nose and over your upper lip.");
                act("Snot runs out of $n's nose and over $s upper lip.", TRUE, ch, 0, 0, TO_ROOM);
                break;
            case 4:
            case 5:
                parse_command(ch, "feel a bit of watery mucus trickle out of your nose.");
                act("A bit of watery mucus trickles out of $n's nose.", TRUE, ch, 0, 0, TO_ROOM);
                break;
            case 6:
                parse_command(ch, "feel a wave of extreme nausea.");
                sfstage += 1;
                sprintf(buf2, "%d", sfstage);
                set_char_extra_desc_value(ch, "[SCRUB_FEVER_STAGE]", buf2);
                break;
            default:
                break;
            }
        }
        else if (sfstage == 2) {
            switch (number(0, 1800)) {
            case 0:
            case 1:
                parse_command(ch, "feel like sniffling as your nose runs freely.");
                act("Snot runs out of $n's nose and over $s upper lip.", TRUE, ch, 0, 0, TO_ROOM);
                break;
            case 2:
            case 3:
                parse_command(ch, "feel a little weak from a sudden wave of nausea.");
                break;
            case 4:
            case 5:
                parse_command(ch, "feel your nose itch as runny mucus dries in your nostrils.");
                break;
            case 6:
                send_to_char("Your stomach heaves, causing you to curl over and vomit.\n\r", ch);
                act("$n bends over and vomits all around $s feet.", FALSE, ch, 0, 0, TO_ROOM);
                GET_COND(ch, FULL) = MAX(0, GET_COND(ch, FULL) - 25);
                sfstage += 1;
                sprintf(buf2, "%d", sfstage);
                set_char_extra_desc_value(ch, "[SCRUB_FEVER_STAGE]", buf2);
                break;
            default:
                break;
            }
        }
        else if (sfstage == 3) {
            switch (number(0, 1800)) {
            case 0:
            case 1:
                parse_command(ch, "feel moisture as snot runs out your nose and over your upper lip.");
                act("Snot runs out of $n's nose and over $s upper lip.", TRUE, ch, 0, 0, TO_ROOM);
                break;
            case 2:
            case 3:
                parse_command(ch, "feel mildly disoriented and a little confused.");
                break;
            case 4:
            case 5:
                parse_command(ch, "feel an annoying discomfort in your lower jaw.");
                break;
            case 6:
                parse_command(ch, "feel disoriented and have difficulty concentrating.");
                act("$n looks disoriented and confused.", TRUE, ch, 0, 0, TO_ROOM);
                /* 5 to 10 points stun loss for 2 to 4 hours */
                newaf.type = TYPE_HUNGER_THIRST;
                newaf.duration = number(2, 4);
                newaf.power = 0;
                newaf.magick_type = 0;
                newaf.modifier = -(number(10, 15));
                newaf.location = CHAR_APPLY_MOVE;
                newaf.bitvector = 0;
                affect_to_char(ch, &newaf);
                sfstage += 1;
                sprintf(buf2, "%d", sfstage);
                set_char_extra_desc_value(ch, "[SCRUB_FEVER_STAGE]", buf2);
                break;
            default:
                break;
            }
        } else if (sfstage == 4) {
            switch (number(0, 1800)) {
            case 0:
            case 1:
                parse_command(ch, "feel moisture as snot runs out your nose and over your upper lip.");
                act("Snot runs out of $n's nose and over $s upper lip.", TRUE, ch, 0, 0, TO_ROOM);
                break;
            case 2:
            case 3:
                parse_command(ch, "feel a throbbing pain in your lower jaw.");
                break;
            case 4:
            case 5:
                parse_command(ch, "feel like sniffling as your nose runs freely.");
                act("Snot runs out of $n's nose and over $s upper lip.", TRUE, ch, 0, 0, TO_ROOM);
                break;
            case 6:
                if (FALSE == get_char_extra_desc_value(ch, "[BLURRED_VISION]", buf1, MAX_STRING_LENGTH)) {
                    set_char_extra_desc_value(ch, "[BLURRED_VISION]", "Scrub Fever");
                    parse_command(ch, "feel a moment of panic as your vision grows a little blurry.");
                }
                break;
            default:
                break;
            }
        }
        else
            set_char_extra_desc_value(ch, "[SCRUB_FEVER_STAGE]", "1");
    }

}


/* Update the various diseases */
int
update_disease(CHAR_DATA * ch, struct affected_type *af)
{

    char buf1[MAX_STRING_LENGTH];

    if (!ch || !af)
        return FALSE;

    if (af->duration < 1) {
        switch (af->type) {
        case DISEASE_SCRUB_FEVER:
            if (TRUE == get_char_extra_desc_value(ch, "[BLURRED_VISION]", buf1, MAX_STRING_LENGTH)) {
                if (!strcmp(buf1, "Scrub Fever")) {
                    rem_char_extra_desc_value(ch, "[BLURRED_VISION]");
                    send_to_char("Your vision returns to normal.\n\r", ch);
                }
            }
            rem_char_extra_desc_value(ch, "[SCRUB_FEVER_STAGE]");
            break;
        default:
            break;
        }
        sprintf(buf1, "%s%s", spell_wear_off_msg[af->type], "\n\r");
        send_to_char(buf1, ch);
        affect_remove(ch, af);
        return TRUE;
    }

    switch (af->type) {
    case DISEASE_BOILS:
        break;
    case DISEASE_CHEST_DECAY:
        break;
    case DISEASE_CILOPS_KISS:
        break;
    case DISEASE_GANGRENE:
        break;
    case DISEASE_HEAT_SPLOTCH:
        break;
    case DISEASE_IVORY_SALT_SICKNESS:
        break;
    case DISEASE_KANK_FLEAS:
        break;
    case DISEASE_KRATHS_TOUCH:
        update_kraths_touch(ch, af);
        break;
    case DISEASE_MAAR_POX:
        break;
    case DISEASE_PEPPERBELLY:
        break;
    case DISEASE_RAZA_RAZA:
        break;
    case DISEASE_SAND_FEVER:
        break;
    case DISEASE_SAND_FLEAS:
        break;
    case DISEASE_SAND_ROT:
        break;
    case DISEASE_SCRUB_FEVER:
        update_scrub_fever(ch, af);
        break;
    case DISEASE_SLOUGH_SKIN:
        break;
    case DISEASE_YELLOW_PLAGUE:
        break;
    default:                   /* unknown disease */
        break;
    }

    return TRUE;

}

