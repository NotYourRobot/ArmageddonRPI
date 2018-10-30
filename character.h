/*
 * File: CHARACTER.H
 * Usage: Routines and commands for handling character files
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
/* Revision history:
 * 1/11/2006 -- File created.  (Tiernan)
 */

/* Example of a character XML document
 *
 * <character>
 * <name>Leroy</name>
 * <account>Jenkins</account>
 * </character>
 *
 */

#ifndef  __CHARACTER_INCLUDED
#define  __CHARACTER_INCLUDED

#include <time.h>
#include "core_structs.h"

// This enumeration maps directly as an index into character_tags[]
enum
{
    CHARACTER_TAG_CHARACTER,
    CHARACTER_TAG_NAME,
    CHARACTER_TAG_ACCOUNT,
    CHARACTER_TAG_SDESC,
    CHARACTER_TAG_DESC,
    CHARACTER_TAG_KEYWORDS,
    CHARACTER_TAG_PASSWORD,
    CHARACTER_TAG_OBJECTIVE,
    CHARACTER_TAG_NONE
};

// States of processing a character xml file
enum
{
    PROCESSING_CHARACTER
};

/* CHARACTER_DATA:  This is an object which contains all of the relative
 * information about a character.
 *
 * The typedef for this is in core_structs.h
 */
typedef struct character_data
{
    char *name;                 // Name of this character
    char *account;              // Player account of this character
    char *short_description;    // Short string describing character 
    char *description;          // Extra descriptions 
    char *keywords;             // Keywords to access this character
    char *objective;            // Objective of this character
}
CHARACTER_DATA;

/* This is a structure that is used for common data */
typedef struct character_parse_data
{
    CHARACTER_DATA character;   // character being updated
    char *tag;                  // character tag being parsed
    int state;                  // state of processing
}
CHARACTER_PARSE_DATA;

int load_character_file_old(const char *name, CHAR_DATA * ch);
int save_character_file_old(CHAR_DATA * ch);
CHARACTER_DATA *ch_to_cd(CHAR_DATA * ch);

int load_character_file(const char *name, const char *account, CHARACTER_DATA * cd);
int parse_character_file(FILE * fp, CHARACTER_DATA * cd);
int save_character_file(CHARACTER_DATA * cd);

void free_character(CHARACTER_DATA * cd);
void reset_character(CHARACTER_DATA * cd);
void copy_character(CHARACTER_DATA * to, CHARACTER_DATA * from);
#endif

