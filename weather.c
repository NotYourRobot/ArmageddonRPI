/*
 * File: WEATHER.C
 * Usage: Routines for changing the weather and its effects.
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

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "handler.h"
#include "parser.h"
#include "guilds.h"
#include "db.h"
#include "modify.h"

/* local variables */
int wset_weather_change = 0;


/* sunlight and sunset by zone */
void
sunrise_and_sunset(int mode)
{
    int tmp = 0;
    int tmp1 = 0;
    char buf[256];
    struct descriptor_data *d;
    char *sunrise_by_zone[] = {
        "The giant crimson sun rises in the east.",     /* 0 *//* default sunrise */
        "The enormous sun rises above the barren plains in the east.",  /* 1 */
        "The sun rises over the spires of Allanak's east wall.",        /* 2 */
        "The great sun rises in the east, turning the scrub plains to gold.",   /* 3 */
        "The immense sun rises up over the Shield Wall in the east.",   /* 4 */
        "Light streams in from above as the sun rises in the east.",    /* 5 */
        "The crystals far above begin to sparkle in the first light of the day.",       /* 6 */
        "The great chasm is set alight as the burning sun rises in the east.",  /* 7 */
        "The crimson sun rises, washing over the Sea of Silt.", /* 8 */
        "The sun rises, filling the sweltering streets of the 'Rinth with heat.",       /* 9 */
        "The crimson sun rises, spilling its light over the dunes.",    /* 10 */
        "The sun rises, catching glints off the black stone of the mountains.", /* 11 */
        "The sun rises, filling the forest with a dim, greenish light.",        /* 12 */
        "The sun rises to the east, touching the rocky cliffs with light.",     /* 13 */
        "With a splash of reflected color, Suk-Krath brings morning to the city.",      /* 14 */
        "Suk-Krath rises in the east, bathing the gloomy mountains in pale light.",     /* 15 */
        ""
    };
    char *sunset_by_zone[] = {
        "The giant crimson sun sets low in the west.",  /* 0 *//* default sunset */
        "The last rays of the red sun fade over the Grey Forest.",      /* 1 */
        "The giant red sun sets over Allanak's west wall.",     /* 2 */
        "The sun sets on the Shield Wall, and the land falls in to darkness.",  /* 3 */
        "The sun sinks into the rocky terrain to the west.",    /* 4 */
        "The light from the crack in the cavern dome fades as night falls.",    /* 5 */
        "The last bit of light fades from the crystals far above.",     /* 6 */
        "Darkness descends as the great sun falls behind huge mountains.",      /* 7 */
        "The crimson sun sets, and darkness washes over the Sea of Silt.",      /* 8 */
        "The sun sinks, and the Labyrinth descends into darkness.",     /* 9 */
        "The crimson sun sinks into the west, as the desert darkens.",  /* 10 */
        "The sun sets, and the mountains darken in somber silence.",    /* 11 */
        "The sun sets, and the forest fills with dark silence.",        /* 12 */
        "As the crimson sun sets, the rocky cliffs glimmer with its last red light.",   /* 13 */
        "The last spire fades to darkness as Suk-Krath abandons the city to night.",    /* 14 */
        "Freezing darkness consumes the mountains as the red rays of Suk-Krath fade.",  /* 15 */
        ""
    };

    for (d = descriptor_list; d; d = d->next) {
        if ((!d->connected) && (d->character->in_room)
            && (!IS_SET(d->character->in_room->room_flags, RFL_INDOORS)) && (AWAKE(d->character))
            && (!IS_AFFECTED(d->character, CHAR_AFF_BLIND))) {
            tmp = d->character->in_room->zone;
            switch (tmp) {
            case 20:
            case 28:
            case 29:
            case 58:
                tmp1 = 1;
                break;
            case 5:
            case 50:
                tmp1 = 2;
                break;
            case 64:
                tmp1 = 3;
                break;
            case 70:
                tmp1 = 4;
                break;
            case 68:
                tmp1 = 4;
                break;
            case 71:
                tmp1 = 5;
                break;
            case 45:
                tmp1 = 6;
                break;
            case 14:
                tmp1 = 7;
                break;
            case 62:
            case 53:
                tmp1 = 8;
                break;
            case 77:
                tmp1 = 9;
                break;
            case 60:
            case 67:
                tmp1 = 10;
                break;
            case 72:
                tmp1 = 11;
                break;
            case 39:
                tmp1 = 12;
                break;
            case 6:
                tmp1 = 13;
                break;
            case 59:
                tmp1 = 14;
                break;
            case 25:
                tmp1 = 15;
                break;
            default:
                tmp1 = 0;
                break;
            }
            if (tmp != 34) {    /* No sunrise/sunsets in elemental planes */
                if (mode)
                    sprintf(buf, "%s\n\r", sunset_by_zone[tmp1]);
                else
                    sprintf(buf, "%s\n\r", sunrise_by_zone[tmp1]);
                if (!d->str)
                    write_to_q(buf, &d->output);
            }
        }
    }
}


/* lirathulight and lirathuset by zone */
void
lirathurise_and_lirathuset(int mode)
{
    int tmp = 0;
    int tmp1 = 0;
    char buf[256];
    struct descriptor_data *d;
    char *lirathurise_by_zone[] = {
        /* Default */
        "Lirathu, the white moon, slowly rises in the southeast.",
        /* z5, z50 */
        "The white moon, Lirathu, rises over the streets of Allanak.",
        /* z6 */
        "Bathing the land in its pale white pall, Lirathu rises from the horizon.",
        /* z10 */
        "The white moon, Lirathu, lifts over the horizon and into the sky.",
        /* z14  */
        "Lirathu's soft glow emerges from below the horizon, illuminating the chasm.",
        /* z17 */
        "With silent grace, Lirathu lifts into the air, its pale form silvery.",
        /* z20, z28, z29, z38 */
        "The pale face of the white moon, Lirathu, rises over the agafari trees.",
        /* z21 */
        "The pale white orb of the white moon, Lirathu, ascends into the sky.",
        /* z23 */
        "Gleaming with dull luster, the white moon, Lirathu, crests the horizon.",
        /* z24 */
        "With silent grace, the white moon, Lirathu, rises past the horizon.",
        /* z39, z55 */
        "Lifting above the forest, the white moon ascends into the heavens.",
        /* z45 */
        "The crystals far above show a glint of white light as Lirathu rises.",
        /* z48, z49, z60, z61, z67 */
        "Gleaming silver, the white moon Lirathu crests above the dunes.",
        /* z52 */
        "Silver light spills across the salty earth as Lirathu rises.",
        /* z56 */
        "Her form pale and elegant, Lirathu gracefully rises from the horizon.",
        /* z62 */
        "The pale white shine of Lirathu crests over the dunes.",
        /* z64 */
        "The soft light of Lirathu rises over the outpost's southern walls.",
        /* z68, z70 */
        "Lirathu rises, its pale light gleaming above the sands in the southeast.",
        /* z71 */
        "The pallid white moon, Lirathu, ascends into the heavens.",
        /* z72 */
        "Gradually, Lirathu rises past the mountains into the sky.",
        /* z77 */
        "The pale orb of Lirathu ascends over the horizon.",
        /* z25 */
        "Pearly Lirathu rises into the sky, gleaming with soft white light.",
        ""
    };
    char *lirathuset_by_zone[] = {
        /* Default */
        "The white moon, Lirathu, slowly sets in the northwest.",
        /* z5, z50 */
        "A final glimmer of light marks the white moon Lirathu's slow descent.",
        /* z6 */
        "Lirathu slips near the northwest horizon, its pale white light vanishing.",
        /* z10 */
        "The white moon, Lirathu, moves with slow grace from the sky, as it sets.",
        /* z14 */
        "The soft glow of Lirathu disappears past the chasm's northern edge.",
        /* z17 */
        "With silent grace, Lirathu descends, its pale form silvery.",
        /* z20, z28, z29, z38 */
        "The pale orb of the white moon, Lirathu, vanishes as it slowly sets.",
        /* z21 */
        "The white orb of the white moon, Lirathu, descends, slipping from the sky.",
        /* z23 */
        "Gleaming with dull luster, Lirathu slips near the horizon.",
        /* z24 */
        "With silent grace, the white moon, Lirathu, nears the horizon.",
        /* z39, z55 */
        "The white moon vanishes into the forest's shadows as it sets.",
        /* z45 */
        "Far above, a last trace of light touches the crystals as Lirathu sets.",
        /* z48, z49, z60, z61, z67 */
        "Silently, the white moon Lirathu slips near the dunes.",
        /* z52 */
        "Slowly, Lirathu slips near the distant horizon.",
        /* z56 */
        "Descending from view, the pale orb of Lirathu departs beyond the hills.",
        /* z62 */
        "Passing beyond the endless dunes, Lirathu fades from sight.",
        /* z64 */
        "Lirathu sets with a last flicker of light across the outpost's north wall.",
        /* z68, z70 */
        "Lirathu slips noiselessly from the sky.",
        /* z71 */
        "The white form of Lirathu departs from the sky's arena.",
        /* z72 */
        "Consumed by the rugged mountains' rim, Lirathu passes from view.",
        /* z77 */
        "Slipping beyond the horizon, Lirathu fades from the sky's stage.",
        /* z25 */
        "The pale orb of Lirathu gracefully slips beneath the northern peaks.",
        ""
    };

    for (d = descriptor_list; d; d = d->next) {
        if ((!d->connected) && (d->character->in_room)
            && (!IS_SET(d->character->in_room->room_flags, RFL_INDOORS)) && (AWAKE(d->character))
            && (!IS_AFFECTED(d->character, CHAR_AFF_BLIND))) {
            tmp = d->character->in_room->zone;
            switch (tmp) {
            case 5:
            case 50:
                tmp1 = 1;
                break;
            case 6:
                tmp1 = 2;
                break;
            case 10:
                tmp1 = 3;
                break;
            case 14:
                tmp1 = 4;
                break;
            case 17:
                tmp1 = 5;
                break;
            case 20:
            case 28:
            case 29:
            case 38:
            case 58:
                tmp1 = 6;
                break;
            case 21:
                tmp1 = 7;
                break;
            case 23:
                tmp1 = 8;
                break;
            case 24:
                tmp1 = 9;
                break;
            case 25:
                tmp1 = 21;
                break;
            case 39:
            case 55:
                tmp1 = 10;
                break;
            case 45:
                tmp1 = 11;
                break;
            case 48:
            case 49:
            case 60:
            case 61:
            case 67:
                tmp1 = 12;
                break;
            case 52:
                tmp1 = 13;
                break;
            case 56:
                tmp1 = 14;
                break;
            case 62:
                tmp1 = 15;
                break;
            case 64:
                tmp1 = 16;
                break;
            case 68:
            case 70:
                tmp1 = 17;
                break;
            case 71:
                tmp1 = 18;
                break;
            case 72:
                tmp1 = 19;
                break;
            case 77:
                tmp1 = 20;
                break;
            case 34:
                tmp1 = 21;
                break;
            default:
                tmp1 = 0;
                break;
            }
            if (tmp != 34) {    /* No Lirathu on elemental planes */
                if (mode)
                    sprintf(buf, "%s\n\r", lirathuset_by_zone[tmp1]);
                else
                    sprintf(buf, "%s\n\r", lirathurise_by_zone[tmp1]);
                if (!d->str)
                    write_to_q(buf, &d->output);
            }
        }
    }
}

/* jihaerise and jihaeet by zone */
void
jihaerise_and_jihaeset(int mode)
{
    int tmp = 0;
    int tmp1 = 0;
    char buf[256];
    struct descriptor_data *d;
    char *jihaerise_by_zone[] = {
        /* Default */
        "Jihae, the red moon, rises up into the sky.",
        /* z5, z50 */
        "The red moon, Jihae, rises over the streets of Allanak.",
        /* z6 */
        "Bathing the land in its reddish light, Jihae rises from the horizon.",
        /* z10 */
        "The red moon, Jihae, lifts over the horizon and into the sky.",
        /* z14 */
        "Jihae's red glow emerges from below the horizon, illuminating the chasm.",
        /* z17 */
        "With ponderous slowness, Jihae's red orb ascends into the sky.",
        /* z20, z28, z29, z38 */
        "The scarlet face of Jihae rises, staring down from the sky.",
        /* z21 */
        "The reddish orb of the red moon, Jihae, ascends into the sky.",
        /* z23 */
        "Gleaming with a bloody light, the red moon, Jihae, crests the horizon.",
        /* z24 */
        "Silently, the red moon, Jihae, rises past the horizon.",
        /* z39, z55 */
        "Lifting above the forest, the red moon ascends into the heavens.",
        /* z45 */
        "The crystals far above show a glint of red light as Jihae rises.",
        /* z48, z49, z60, z61, z67 */
        "Gleaming a tarnished red, the moon Jihae crests above the dunes.",
        /* z52 */
        "Blood-red light spills across the salty earth as Jihae rises.",
        /* z56 */
        "Her form bathed in dull red light, Jihae rises from the horizon.",
        /* z62 */
        "The ruddy red gleam of Jihae crests over the dunes.",
        /* z64 */
        "The red light of Jihae rises over the outpost's southern walls.",
        /* z68, z70 */
        "Jihae rises, its red light gleaming above the sands in the southeast.",
        /* z71 */
        "The smoldering red moon, Jihae, ascends into the heavens.",
        /* z72 */
        "Gradually, Jihae rises past the mountains into the sky.",
        /* z77 */
        "The red orb of Jihae ascends over the horizon.",
        /* z25 */
        "Jihae rises into the sky, bathing the land in bloody light.",
        /* Sentinel */
        "\n"
    };
    char *jihaeset_by_zone[] = {
        /* Default */
        "The red moon, Jihae, begins to disappear below the horizon.",
        /* z5, z50 */
        "A final glimmer of red light marks the red moon Jihae's slow descent.",
        /* z6 */
        "Jihae nears the horizon, its angry red light fading.",
        /* z10 */
        "The red moon, Jihae, moves with ponderous grace from the sky, setting.",
        /* z14 */
        "The red glow of Jihae disappears past the chasm's northern edge.",
        /* z17 */
        "With ponderous slowness, Jihae's red orb descends from the sky.",
        /* z20, z28, z29, z38 */
        "The red orb of Jihae, the red moon, begins to vanish as it slowly sets.",
        /* z21 */
        "The reddish orb of the red moon, Jihae, descends, vanishing from the sky.",
        /* z23 */
        "Gleaming with a bloody light, Jihae nears the horizon.",
        /* z24 */
        "Silently, the red moon, Jihae, nears the horizon.",
        /* z39, z55 */
        "The red moon vanishes into the forest's shadows as it sets.",
        /* z45 */
        "Far above, a last trace of red light touches the crystals as Jihae sets.",
        /* z48, z49, z60, z61, z67 */
        "Silently, the red moon Jihae begins to slip below the dunes.",
        /* z52 */
        "Slowly, Jihae begins to slip below the distant horizon.",
        /* z56 */
        "Descending from view, the red orb of Jihae departs beyond the hills.",
        /* z62 */
        "Passing beyond the endless dunes, Jihae fades from sight.",
        /* z64 */
        "Jihae sets with a last gleam of red across the outpost's north wall.",
        /* z68, z70 */
        "In a sullen red glow, Jihae begins to slip from the sky.",
        /* z71 */
        "The red form of Jihae departs from the sky's arena.",
        /* z72 */
        "Consumed by the rugged mountains' rim, Jihae passes from view.",
        /* z77 */
        "Slipping beyond the horizon, Jihae fades from the sky's stage.",
        /* z25 */
        "Gibbous Jihae droops beneath the mountaintops, glowing morbidly.",
        /* Sentinel */
        "\n"
    };

    for (d = descriptor_list; d; d = d->next) {
        if ((!d->connected) && (d->character->in_room)
            && (!IS_SET(d->character->in_room->room_flags, RFL_INDOORS)) && (AWAKE(d->character))
            && (!IS_AFFECTED(d->character, CHAR_AFF_BLIND))) {
            tmp = d->character->in_room->zone;
            switch (tmp) {
            case 5:
            case 50:
                tmp1 = 1;
                break;
            case 6:
                tmp1 = 2;
                break;
            case 10:
                tmp1 = 3;
                break;
            case 14:
                tmp1 = 4;
                break;
            case 17:
                tmp1 = 5;
                break;
            case 20:
            case 28:
            case 29:
            case 38:
            case 58:
                tmp1 = 6;
                break;
            case 21:
                tmp1 = 7;
                break;
            case 23:
                tmp1 = 8;
                break;
            case 24:
                tmp1 = 9;
                break;
            case 39:
            case 55:
                tmp1 = 10;
                break;
            case 45:
                tmp1 = 11;
                break;
            case 48:
            case 49:
            case 60:
            case 61:
            case 67:
                tmp1 = 12;
                break;
            case 52:
                tmp1 = 13;
                break;
            case 56:
                tmp1 = 14;
                break;
            case 62:
                tmp1 = 15;
                break;
            case 64:
                tmp1 = 16;
                break;
            case 68:
            case 70:
                tmp1 = 17;
                break;
            case 71:
                tmp1 = 18;
                break;
            case 72:
                tmp1 = 19;
                break;
            case 77:
                tmp1 = 20;
                break;
            case 25:
                tmp1 = 21;
                break;
            default:
                tmp1 = 0;
                break;
            }
            if (tmp != 34) {    /* No Jihae on elemental planes */
                if (mode)
                    sprintf(buf, "%s\n\r", jihaeset_by_zone[tmp1]);
                else
                    sprintf(buf, "%s\n\r", jihaerise_by_zone[tmp1]);
                if (!d->str)
                    write_to_q(buf, &d->output);
            }
        }
    }
}

/* echririse and echriset by zone */
void
echririse_and_echriset(int mode)
{
    int tmp = 0;
    int tmp1 = 0;
    char buf[256];
    struct descriptor_data *d;
    char *echririse_by_zone[] = {
        /* Default */
        "The black moon rises up into the sky.",
        /* z5, z50 */
	"",
        /* z6 */
	"",
        /* z10 */
	"",
        /* z14 */
	"",
        /* z17 */
	"",
        /* z20, z28, z29, z38 */
	"",
        /* z21 */
	"",
        /* z23 */
	"",
        /* z24 */
	"",
        /* z39, z55 */
	"",
        /* z45 */
	"",
        /* z48, z49, z60, z61, z67 */
	"",
        /* z52 */
	"",
        /* z56 */
	"",
        /* z62 */
	"",
        /* z64 */
	"",
        /* z68, z70 */
	"",
        /* z71 */
	"",
        /* z72 */
	"",
        /* z77 */
	"",
        /* z25 */
	"",
        /* Sentinel */
        "\n"
    };
    char *echriset_by_zone[] = {
        /* Default */
        "The black moon begins to disappear below the horizon.",
        /* z5, z50 */
	"",
        /* z6 */
	"",
        /* z10 */
	"",
        /* z14 */
	"",
        /* z17 */
	"",
        /* z20, z28, z29, z38 */
	"",
        /* z21 */
	"",
        /* z23 */
	"",
        /* z24 */
	"",
        /* z39, z55 */
	"",
        /* z45 */
	"",
        /* z48, z49, z60, z61, z67 */
	"",
        /* z52 */
	"",
        /* z56 */
	"",
        /* z62 */
	"",
        /* z64 */
	"",
        /* z68, z70 */
	"",
        /* z71 */
	"",
        /* z72 */
	"",
        /* z77 */
	"",
        /* z25 */
	"",
        /* Sentinel */
        "\n"
    };

    for (d = descriptor_list; d; d = d->next) {
        if ((!d->connected) && (d->character->in_room)
            && (!IS_SET(d->character->in_room->room_flags, RFL_INDOORS)) && (AWAKE(d->character))
            && (!IS_AFFECTED(d->character, CHAR_AFF_BLIND))) {
            tmp = d->character->in_room->zone;
            switch (tmp) {
#if 0 // Take this out when there are custom rise & set msgs above
            case 5:
            case 50:
                tmp1 = 1;
                break;
            case 6:
                tmp1 = 2;
                break;
            case 10:
                tmp1 = 3;
                break;
            case 14:
                tmp1 = 4;
                break;
            case 17:
                tmp1 = 5;
                break;
            case 20:
            case 28:
            case 29:
            case 38:
            case 58:
                tmp1 = 6;
                break;
            case 21:
                tmp1 = 7;
                break;
            case 23:
                tmp1 = 8;
                break;
            case 24:
                tmp1 = 9;
                break;
            case 39:
            case 55:
                tmp1 = 10;
                break;
            case 45:
                tmp1 = 11;
                break;
            case 48:
            case 49:
            case 60:
            case 61:
            case 67:
                tmp1 = 12;
                break;
            case 52:
                tmp1 = 13;
                break;
            case 56:
                tmp1 = 14;
                break;
            case 62:
                tmp1 = 15;
                break;
            case 64:
                tmp1 = 16;
                break;
            case 68:
            case 70:
                tmp1 = 17;
                break;
            case 71:
                tmp1 = 18;
                break;
            case 72:
                tmp1 = 19;
                break;
            case 77:
                tmp1 = 20;
                break;
            case 25:
                tmp1 = 21;
                break;
#endif
            default:
                tmp1 = 0;
                break;
            }
            if (tmp != 34) {    /* No Echri on elemental planes */
                if (mode)
                    sprintf(buf, "%s\n\r", echriset_by_zone[tmp1]);
                else
                    sprintf(buf, "%s\n\r", echririse_by_zone[tmp1]);
                if (!d->str && use_shadow_moon)
                    write_to_q(buf, &d->output);
                    ;
            }
        }
    }
}
/* Adjusts Zalanthan time to real time */
int
check_time(struct time_info_data *data)
{

    struct time_info_data now;
    time_t t1 = time(0);
    int changed = 0;

    now.hours = (t1 / RT_ZAL_HOUR) % 9;
    now.day = (t1 / RT_ZAL_DAY) % 231;
    now.month = (t1 / RT_ZAL_MONTH) % 3;
    now.year = ZT_EPOCH + t1 / RT_ZAL_YEAR;

    changed = (now.hours != time_info.hours);

    if (data) {
        data->hours = now.hours;
        data->day = now.day;
        data->month = now.month;
        data->year = now.year;
    }

    return changed;
}

/* increment mud time by 1 hour */
void
update_time(void)
{
    FILE *fp;
    struct timeval start;

    weather_info.moon[JIHAE] = time_info.day % 7;
    weather_info.moon[LIRATHU] = time_info.day % 11;
    weather_info.moon[ECHRI] = time_info.day % 33;

    static int jihae_rise = -1;
    static int jihae_set = -1;
    static int lirathu_rise = -1;
    static int lirathu_set = -1;
    static int echri_rise = -1;
    static int echri_set = -1;

    perf_enter(&start, "update_time()");
    // Make sure our values are within normal limits
    if (jihae_rise < NIGHT_EARLY || jihae_rise > NIGHT_LATE)
        jihae_rise = number(NIGHT_EARLY, NIGHT_LATE);
    if (jihae_set < NIGHT_EARLY || jihae_rise > NIGHT_LATE)
        jihae_set = number(NIGHT_EARLY, NIGHT_LATE);
    if (lirathu_rise < NIGHT_EARLY || jihae_rise > NIGHT_LATE)
        lirathu_rise = number(NIGHT_EARLY, NIGHT_LATE);
    if (lirathu_set < NIGHT_EARLY || jihae_rise > NIGHT_LATE)
        lirathu_set = number(NIGHT_EARLY, NIGHT_LATE);
    if (echri_rise < DAWN || jihae_rise > DUSK)
        echri_rise = number(DAWN, DUSK);
    if (echri_set < DAWN || jihae_rise > DUSK)
        echri_set = number(DAWN, DUSK);

    switch (time_info.hours) {
    case NIGHT_EARLY:
        weather_info.sunlight = SUN_DARK;
        break;
    case DAWN:
        weather_info.sunlight = SUN_LOW;
	if (!use_global_darkness)
            sunrise_and_sunset(0);
        break;
    case RISING_SUN:
        weather_info.sunlight = SUN_LOW;
	if (!use_global_darkness)
            send_to_outdoor("The sun begins its long voyage across the heavens.\n\r");
        break;
    case MORNING:
        weather_info.sunlight = SUN_HIGH;
	if (!use_global_darkness)
            send_to_outdoor("The burning sun rises high into the sky, searing the earth.\n\r");
        break;
    case HIGH_SUN:
        weather_info.sunlight = SUN_ZENITH;
	if (!use_global_darkness)
            send_to_outdoor("The sun reaches its highest point in the sky.\n\r");
        break;
    case AFTERNOON:
        weather_info.sunlight = SUN_HIGH;
	if (!use_global_darkness)
            send_to_outdoor("The mighty sun begins to crawl across the western sky.\n\r");
        break;
    case SETTING_SUN:
        weather_info.sunlight = SUN_LOW;
	if (!use_global_darkness)
            send_to_outdoor("The late, red sun descends toward the western horizon.\n\r");
        break;
    case DUSK:
        weather_info.sunlight = SUN_LOW;
	if (!use_global_darkness)
            sunrise_and_sunset(1);
        break;
    case NIGHT_LATE:
        weather_info.sunlight = SUN_DARK;
	if (!use_global_darkness)
            send_to_outdoor("The night has begun.\n\r");
        break;
    default:
        break;
    }

    if (!use_global_darkness) {
        // Echo whether Jihae rises or sets
        if (time_info.hours == jihae_rise && weather_info.moon[JIHAE] == JIHAE_RISE)
            jihaerise_and_jihaeset(0);
        if (time_info.hours == jihae_set && weather_info.moon[JIHAE] == JIHAE_SET)
            jihaerise_and_jihaeset(1);

        // Echo whether Lirathu rises or sets
        if (time_info.hours == lirathu_rise && weather_info.moon[LIRATHU] == LIRATHU_RISE)
            lirathurise_and_lirathuset(0);
        if (time_info.hours == lirathu_set && weather_info.moon[LIRATHU] == LIRATHU_SET)
            lirathurise_and_lirathuset(1);

        // Echo whether Echri rises or sets
        if (time_info.hours == echri_rise && weather_info.moon[ECHRI] == ECHRI_RISE)
            echririse_and_echriset(0);
        if (time_info.hours == echri_set && weather_info.moon[ECHRI] == ECHRI_SET)
            echririse_and_echriset(1);
    }

    /* Calculate and update lunar tide value */
    static int tidal_jihae = 40;     // Lirathu and Jihae have the same 'pull'
    static int tidal_lirathu = 40;   // even though they are differnt sizes.
    static int tidal_echri = 20;     // Echri is further away or lower mass.
    int jihae_factor = 0;
    int lirathu_factor = 0;
    int echri_factor = 0;

    /* Calculate the phase factor.  */
    if (weather_info.moon[JIHAE] == JIHAE_RISE || weather_info.moon[JIHAE] == JIHAE_SET)
        jihae_factor = 5;
    else if (weather_info.moon[JIHAE] < JIHAE_RISE && weather_info.moon[JIHAE] > JIHAE_SET)
        jihae_factor = 0;
    else if (weather_info.moon[JIHAE] > JIHAE_RISE && weather_info.moon[JIHAE] < JIHAE_SET)
        jihae_factor = 10;

    if (weather_info.moon[LIRATHU] == LIRATHU_RISE || weather_info.moon[LIRATHU] == LIRATHU_SET)
        lirathu_factor = 5;
    else if (weather_info.moon[LIRATHU] < LIRATHU_RISE && weather_info.moon[LIRATHU] > LIRATHU_SET)
        lirathu_factor = 0;
    else if (weather_info.moon[LIRATHU] > LIRATHU_RISE && weather_info.moon[LIRATHU] < LIRATHU_SET)
        lirathu_factor = 10;

    if (weather_info.moon[ECHRI] == ECHRI_RISE || weather_info.moon[ECHRI] == ECHRI_SET)
        echri_factor = 5;
    else if (weather_info.moon[ECHRI] < ECHRI_RISE && weather_info.moon[ECHRI] > ECHRI_SET)
        echri_factor = 0;
    else if (weather_info.moon[ECHRI] > ECHRI_RISE && weather_info.moon[ECHRI] < ECHRI_SET)
        echri_factor = 10;

//    int testing;
//    testing = ((tidal_jihae * jihae_factor)/10) + ((tidal_lirathu * lirathu_factor)/10) + ((tidal_echri * echri_factor)/10);
    /* Will be a value between 0 and 100 where 0 is lowest tide and 100 is highest. */
    weather_info.tides = ((tidal_jihae * jihae_factor)/10) + ((tidal_lirathu * lirathu_factor)/10) + ((tidal_echri * echri_factor)/10);

    /* save time to file */
    if (!lock_mortal && (fp = fopen(TIME_FILE, "w")) != NULL) {
        fprintf(fp, "%d %d %d %d %d %d\n", time_info.hours, time_info.day, time_info.month,
                time_info.year, weather_info.moon[JIHAE], weather_info.moon[LIRATHU]);
        fclose(fp);
    }
    perf_exit("update_time()", start);
}


/* update the condition of weather */
void
update_weather(void)
{
    struct weather_node *wn, *nb;
    double w_change;
    float ht_mid, w_mid, ht_change;
    int dir, percent, j; 
    int wind_change = 0;
    struct timeval start;

    /* maybe the wind will change */

    perf_enter(&start, "update_weather()");
    weather_info.pressure += (number(50, 250) - 100);
    weather_info.pressure = MAX(weather_info.pressure, 1);

    if (weather_info.pressure >= 1000) {
        j = weather_info.wind_direction;
        weather_info.wind_direction = number(0, 3);
        while (j == weather_info.wind_direction)
            weather_info.wind_direction = number(0, 3);
        if (!number(0, 1))
            send_to_outdoor("A fresh wind blows from a new direction.\n\r");
        else
            send_to_outdoor("The wind changes direction.\n\r");
        weather_info.pressure = 1;

        // did the wind completely reverse?
        if( j == rev_dir[weather_info.wind_direction]) {
           // cut wind in half if it completely reverses
           wind_change = weather_info.wind_velocity / -2;
        }
        else {
           // otherwise force a wind speed change
           wind_change = number(5, 15) - 10;
        }
    }

    if (wind_change != 0 || !number(0, 3)) {
        if( wind_change == 0 ) 
            wind_change = number(5, 15) - 10;

        if (wind_change > 0) {
            switch (number (1, 3)) {
            case 1:
                send_to_outdoor("The wind picks up some speed.\n\r");
                break;
            case 2:
                send_to_outdoor("The wind strengthens a little.\n\r");
                break;
            default:
                send_to_outdoor("The wind grows stronger.\n\r");
                break;
            }
        }
	    else if (wind_change < 0) {
            switch (number (1, 3)) {
            case 1:
                send_to_outdoor("The wind loses some momentum.\n\r");
                break;
            case 2:
                send_to_outdoor("The wind slows down a little.\n\r");
                break;
            default:
                send_to_outdoor("The wind grows weaker.\n\r");
                break;
            }
        }
        weather_info.wind_velocity += wind_change;
        weather_info.wind_velocity = MIN(weather_info.wind_velocity, 90);
        weather_info.wind_velocity = MAX(weather_info.wind_velocity, 0);
    }

    for (wn = wn_list; wn; wn = wn->next) {
        /* hotter and stormier in day, cooler at night */
        ht_change = ((wn->max_temp - wn->min_temp) / 20);
        ht_mid = ((wn->max_temp + wn->min_temp) / 2);
        w_change = ((wn->max_condition - wn->min_condition) / 10);
        w_mid = wn->median_condition;
        dir = weather_info.wind_direction;

	if (use_global_darkness)
            wn->temp -= (int) (2 * ht_change);
	else switch (weather_info.sunlight) {
        case SUN_DARK:
            wn->temp -= (int) (2 * ht_change);
            break;
        case SUN_LOW:
            wn->temp = (int) ((wn->temp + ht_mid) / 2);
            break;
        case SUN_HIGH:
            wn->temp += (int) ht_change;
            break;
        case SUN_ZENITH:
            wn->temp += (int) (2 * ht_change);
            break;
        default:
            gamelog("Illegal state of sun in weather_info.");
            weather_info.sunlight = SUN_DARK;
        }

        if (!wset_weather_change) {     /* change cond by winds */
            // if the winds increased
            if (wind_change > 0) {
                // chance of conditions worseneing
                // decreases with distance from median condition
                if (number(w_mid, wn->max_condition) >= wn->condition)
                    wn->condition += w_change;
            } 
            // if the winds died down
            else if( wind_change < 0 ) {
                // chance of conditions improving
                // based on how far from median
                if (number(wn->min_condition, w_mid) <= wn->condition)
                    wn->condition -= w_change;
            }
        } else {                /* change cond by brute force */
            wn->condition += (wset_weather_change * w_change);
            wset_weather_change = 0;
        }

        wn->temp = MIN(wn->max_temp, wn->temp);
        wn->temp = MAX(wn->min_temp, wn->temp);

        if (wn->condition > wn->max_condition)
            wn->condition = wn->max_condition;
        if (wn->condition < wn->min_condition)
            wn->condition = wn->min_condition;

        /* push weather and heat to near zone */

        for (j = 0; j < wn->neighbors[dir]; j++) {
            nb = wn->near_node[dir][j];
            percent = weather_info.wind_velocity;
            if (nb->temp < wn->temp) {
                ht_change = (((wn->temp - nb->temp) * percent) / 100);
                nb->temp += (int) ht_change;
            } else if (nb->temp > wn->temp) {
                ht_change = (((nb->temp - wn->temp) * percent) / 100);
                nb->temp -= (int) ht_change;
            }
            if (wn->condition > 0) {
                w_change = ((wn->condition * percent) / 100);
                nb->condition += w_change;
                wn->condition -= w_change;
            }

            if (nb->condition > nb->max_condition)
                nb->condition = nb->max_condition;
            if (nb->condition < nb->min_condition)
                nb->condition = nb->min_condition;

            nb->temp = MIN(nb->max_temp, nb->temp);
            nb->temp = MAX(nb->min_temp, nb->temp);
        }

        if (wn->condition > wn->max_condition)
            wn->condition = wn->max_condition;
        if (wn->condition < wn->min_condition)
            wn->condition = wn->min_condition;

        // Update the lifeforce in the zone
        if ((wn->life < wn->max_life) && (!number(0, 99)))
            wn->life = MIN(wn->max_life, wn->life + 1);
    }
    perf_exit("update_weather()", start);
}


int
room_visibility(ROOM_DATA *room)
{
    struct weather_node *wn;
    int vis = 3;
    int zone;
    float viscond;

    if (IS_SET(room->room_flags, RFL_INDOORS))
        return vis;

    zone = room->zone;

    if (IS_DARK(room))
        vis = -1;

    if (use_global_darkness)
        vis = -1;
    else if (weather_info.sunlight == SUN_DARK) {
        vis = -1;
        if ((weather_info.moon[JIHAE] >= JIHAE_RISE)
         && (weather_info.moon[JIHAE] <= JIHAE_SET))
            vis += 1;
        if ((weather_info.moon[LIRATHU] >= LIRATHU_RISE)
         && (weather_info.moon[LIRATHU] <= LIRATHU_SET))
            vis += 1;
    }
    if (room->light)
        vis = MAX(vis, 0);

    for (wn = wn_list; wn; wn = wn->next) {
        if (wn->zone == zone) {
            viscond = wn->condition;
            if (viscond >= 8.50)
                vis = -1;
            else if (viscond >= 7.00)
                vis -= 3;
            else if (viscond >= 5.00)
                vis -= 2;
            else if (viscond >= 2.50)
                vis -= 1;
            break;
        }
    }

    vis = MAX(vis, -1);
    return vis;

}

int
visibility(CHAR_DATA * ch)
{
    struct weather_node *wn;
    int vis = 3;
    int zone;
    float viscond;

    if (IS_AFFECTED(ch, CHAR_AFF_BLIND))        /*   kel   */
        return -1;

    if (IS_SET(ch->in_room->room_flags, RFL_INDOORS))
        return vis;

    if (IS_IMMORTAL(ch))
        return vis;

    /* if can't see in the dark */
    if (!IS_AFFECTED(ch, CHAR_AFF_INFRAVISION) 
     && GET_RACE(ch) != RACE_GITH
     && GET_RACE(ch) != RACE_VAMPIRE
     && GET_RACE(ch) != RACE_HALFLING) {
        if (IS_DARK(ch->in_room))
            vis = -1;

        if (use_global_darkness) 
            vis = -1;
        else if (weather_info.sunlight == SUN_DARK) {
            vis = -1;
            if (weather_info.moon[JIHAE] >= JIHAE_RISE
             && weather_info.moon[JIHAE] <= JIHAE_SET)
                vis += 1;
            if (weather_info.moon[LIRATHU] >= LIRATHU_RISE
             && weather_info.moon[LIRATHU] <= LIRATHU_SET)
                vis += 1;
        }

        if (ch->in_room->light)
            vis = MAX(vis, 0);
    }

    zone = ch->in_room->zone;

    /* lightning elementalists have a natural talent for seeing in
     * stormy weather, so don't reduce their visibility due to storms.
     * Also, Wind Armor will improve visibility loss due to condition */
    if (GET_GUILD(ch) != GUILD_LIGHTNING_ELEMENTALIST
     && !affected_by_spell(ch, SPELL_ETHEREAL)) {
        for (wn = wn_list; wn; wn = wn->next) {
            if (wn->zone == zone) {

                viscond = wn->condition;
                if (affected_by_spell(ch, SPELL_WIND_ARMOR))
                    viscond -= 3.00;

                if (IS_AFFECTED(ch, CHAR_AFF_INFRAVISION))
                    viscond -= 3.00;

                if (viscond >= 8.50)
                    vis = -1;
                else if (viscond >= 7.00)
                    vis -= 3;
                else if (viscond >= 5.00)
                    vis -= 2;
                else if (viscond >= 2.50)
                    vis -= 1;
                break;
            }
        }
    }

    vis = MAX(vis, -1);
    return vis;

}


/* Commands used to change weather settings in the zone you're standing in */
void
cmd_wset(CHAR_DATA * ch,        /* The character changing the weather */
         const char *argument,  /* What they typed                    */
         Command cmd, int count)
{                               /* What command they used to get here */
    /* Local variables and structures */
    char tmp[256], buf[256];
    int zoneto;
    int keyword, n, dir;
    bool save_weather = FALSE;
    struct weather_node *wn;
    struct weather_node *wnn;
    const char * const set_list[] = {        /* A list of what the valid settings are */
        "wind",
        "temperature",
        "condition",
        "lifeforce",
        "create",
        "neighbor",
        "con_max",
        "con_min",
        "con_med",
        "\n"
    };

    const char * const wind_list[] = {       /* A list of valid wind settings */
        "speed",
        "direction",
        "\n"
    };


    const char * const weather_dir_list[] = {        /* A list of valid weather dirs */
        "n", "e", "s", "w", "u", "d", "\n"
    };


    if (!ch->in_room)
        return;

    argument = one_argument(argument, buf, sizeof(buf));
    keyword = search_block(buf, set_list, FALSE);
    if (keyword == -1) {        /* No argument after wset */
        send_to_char("You can only use wset with the following arguments:\n\r"
                     "wind, temperature, condition, and lifeforce.\n\r", ch);
        send_to_char("  Usage: wset wind direction [north | south | east | west]" "\n\r", ch);

        send_to_char("  Usage: wset wind speed N\n\r", ch);
        send_to_char("  Usage: wset temperature N\n\r", ch);
        send_to_char("  Usage: wset condition [clearer | stormier]\n\r", ch);
        send_to_char("  Usage: wset lifeforce N\n\r", ch);
        send_to_char("  Usage: wset create\n\r", ch);
        send_to_char("  Usage: wset neighbor [N | S | E | W] [zone#] (again to toggle off)\n\r",
                     ch);
        send_to_char("  Usage: wset con_min\n\r", ch);
        send_to_char("  Usage: wset con_max\n\r", ch);
        send_to_char("  Usage: wset con_med\n\r", ch);

        /* write the neighbors */
        for (wn = wn_list; wn; wn = wn->next) {
            int i;

            if (wn->zone == ch->in_room->zone)
                for (dir = 0; dir < MAX_DIRECTIONS; dir++)
                    for (i = 0; i < wn->neighbors[dir]; i++) {
                        sprintf(buf, "Direction %s leads to zone %d.\n\r", weather_dir_list[dir],
                                wn->near_node[dir][i]->zone);
                        send_to_char(buf, ch);
                    }
        };
        return;
    };

    /* Find the zone the person is in */
    for (wn = wn_list; wn; wn = wn->next)
        if (wn->zone == ch->in_room->zone)
            break;

    if (!wn && (keyword != 4)) {
        send_to_char("You're not in a zone with weather.\n\r", ch);
        return;
    }

    switch (keyword) {
    case 0:                    /* set wind */
        argument = one_argument(argument, buf, sizeof(buf));
        keyword = search_block(buf, wind_list, FALSE);
        if (keyword == -1) {
            send_to_char("You can set the 'speed' or the 'direction' " "of the wind.\n\r", ch);
            send_to_char("  Usage: wset wind N.\n\r", ch);
            send_to_char("  Usage: wset direction N.\n\r", ch);
        }
        switch (keyword) {
        case 0:                /* set speed */
            argument = one_argument(argument, tmp, sizeof(tmp));
            n = atoi(tmp);
            if ((n >= 0) && (n <= 100)) {
                weather_info.wind_velocity = n;
                send_to_char("Wind velocity changed.\n\r", ch);
                send_to_outdoor("The wind changes suddenly.\n\r");
            } else
                send_to_char("That speed is invalid, range is 0-100\n\r", ch);
            break;
        case 1:                /* set direction */
            argument = one_argument(argument, tmp, sizeof(tmp));
            dir = search_block(tmp, dir_name, FALSE);
            if ((dir >= DIR_NORTH) && (dir <= DIR_WEST)) {
                weather_info.wind_direction = dir;
                send_to_char("Wind direction changed.\n\r", ch);
                send_to_outdoor("The wind changes suddenly.\n\r");
            } else {
                send_to_char("That direction is invalid.\n\r", ch);
                send_to_char("Use north, south, east, or west only.\n\r", ch);
            }
            break;
        default:
            gamelog("something wrong with 'wset wind'");
            break;
        }
        break;
    case 1:                    /* set temperature */
        argument = one_argument(argument, buf, sizeof(buf));
        n = atoi(buf);
        if ((n >= wn->min_temp) && (n <= wn->max_temp)) {
            wn->temp = n;
            send_to_char("Temperature changed.\n\r", ch);
            send_to_outdoor("The temperature changes suddenly.\n\r");
        } else {
            send_to_char("That is an invalid temperature.\n\r", ch);
            send_to_char("Type 'weather' to see the Min and Max temperature " "for this zone.\n\r",
                         ch);
            send_to_char("  Usage: wset temperature N\n\r", ch);
        }
        break;
    case 2:                    /* set condition */
        argument = one_argument(argument, buf, sizeof(buf));
        if (!strnicmp(buf, "clearer", strlen(buf))) {
            wset_weather_change = -1;
            wn->condition--;
            send_to_char("Weather change initiated...\n\r", ch);
        } else if (!strnicmp(buf, "stormier", strlen(buf))) {
            wset_weather_change = 1;
            wn->condition++;
            send_to_char("Weather change initiated...\n\r", ch);
        } else
            send_to_char("You can only change the weather to 'clearer' or " "'stormier'.\n\r", ch);
        break;
    case 3:                    /* set life */
        if (!has_privs_in_zone(ch, wn->zone, MODE_MODIFY))
            send_to_char("You do not have privileges to modify the lifeforce " "in this zone.\n\r",
                         ch);
        else {
            argument = one_argument(argument, buf, sizeof(buf));
            n = atoi(buf);
            if ((n >= 0) && (n <= 9999)) {
                wn->life = n;
                send_to_char("Lifeforce changed.\n\r", ch);
            } else {
                send_to_char("That is an invalid life value.\n\r", ch);
                send_to_char("Values from 0 to 9999 are valid.\n\r", ch);
                send_to_char("  Usage: wset lifeforce N\n\r", ch);
            }
        }
        break;
    case 4:                    /* set create */
        if (wn) {
            send_to_char("This zone already has a weather node.\n\r", ch);
        } else {
            /* create a new weather node */
            create_node(ch->in_room->zone, 0, 60, 100, 5, 10, 7);
            send_to_char("Added a weather node to this zone.\n\r", ch);
        };

        break;
    case 5:                    /* set nextto */
        argument = one_argument(argument, buf, sizeof(buf));
        shhlog(buf);
        shhlog(argument);

        dir = search_block(buf, weather_dir_list, FALSE);
        if (dir == -1) {
            send_to_char("Not a valid dir use n, s, e, or w.\n\r", ch);
        } else {
            /* find wn for neighbor */
            argument = one_argument(argument, buf, sizeof(buf));
            zoneto = atoi(buf);

            /* Find the zone the person wants */
            for (wnn = wn_list; wnn; wnn = wnn->next)
                if (wnn->zone == zoneto)
                    break;

            if (!wnn) {
                send_to_char("That zone did not have weather.\n\r", ch);
            } else {
                /* if cannot remove relationship, it wasnt set, so set */
                if (!remove_neighbor(wn, wnn, dir)) {
                    /* add a neighboring zone */
                    add_neighbor(wn, wnn, dir);
                    send_to_char("Added neighbor weather.\n\r", ch);
                } else {
                    /* remove neighboring zone */
                    send_to_char("Removed neighbor weather.\n\r", ch);
                }
            };
        };
        break;
    case 6:                    /* set con_max */
        argument = one_argument(argument, buf, sizeof(buf));
        n = atoi(buf);
        wn->max_condition = n;
        send_to_char("Max condition set.\n\r", ch);
        save_weather = TRUE;
        break;
    case 7:                    /* set con_min */
        argument = one_argument(argument, buf, sizeof(buf));
        n = atoi(buf);
        wn->min_condition = n;
        send_to_char("Min condition set.\n\r", ch);
        save_weather = TRUE;
        break;
    case 8:                    /* set con_med */
        argument = one_argument(argument, buf, sizeof(buf));
        float f = atof(buf);
        wn->median_condition = f;
        send_to_char("Median condition set.\n\r", ch);
        save_weather = TRUE;
        break;
    default:                   /* something wrong here */
        gamelog("Invalid attempt to alter weather patterns.");
        break;
    }

    if( save_weather ) {
        save_weather_data();
    }
}

