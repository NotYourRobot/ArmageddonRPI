/*
 * File: CITIES.H
 * Usage: Definitions for the cities of Zalanthas.
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

#ifndef __CITIES_INCLUDED
#define __CITIES_INCLUDED

#define CITY_NONE                0
#define CITY_ALLANAK             1
#define CITY_TULUK               2
#define CITY_CENYR               3
#define CITY_LUIR                4
#define CITY_RS                  5
#define CITY_CAVERN              6
#define CITY_BW                  7
#define CITY_PTARKEN             8
#define CITY_CAI_SHYZN           9
#define CITY_MAL_KRIAN          10
#define CITY_LANTHIL            11
#define CITY_TYN_DASHRA         12
#define CITY_FREILS_REST        13
#define CITY_REYNOLTE           14
#define CITY_DELF_OUTPOST       15
#define CITY_ALLANAK_GLADIATORS 16
#define CITY_ANYALI             17
#define CITY_LABYRINTH          18
#define CITY_UNDERTULUK         19

#define MAX_CITIES		2

#define COMMOD_NONE             0
#define COMMOD_SPICE            1
#define COMMOD_GEM              2
#define COMMOD_FUR              3
#define COMMOD_WEAPON           4
#define COMMOD_ARMOR            5
#define COMMOD_METAL            6
#define COMMOD_WOOD             7
#define COMMOD_MAGICK           8
#define COMMOD_ART              9
#define COMMOD_OBSIDIAN        10
#define COMMOD_CHITIN          11
#define COMMOD_BONE            12
#define COMMOD_GLASS           13
#define COMMOD_SILK            14
#define COMMOD_IVORY           15
#define COMMOD_TORTOISESHELL   16
#define COMMOD_DUSKHORN        17
#define COMMOD_ISILT           18
#define COMMOD_SALT            19
#define COMMOD_GWOSHI          20
#define COMMOD_PAPER           21
#define COMMOD_FOOD            22
#define COMMOD_FEATHER         23

#define MAX_COMMOD    24

struct city_data
{
    char *name;
    char *of_city;

    long c_flag;
#ifdef USE_ZONE_BYTE
    byte zone;
#else
    int zone;

#endif

    int tribe;
    int room;

    float price[MAX_COMMOD];
};

extern struct city_data city[];

#endif

