/*
 * File: CHARACTER.C
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

#include <stdio.h>
#include <string.h>
#include "character.h"
#include "core_structs.h"
#include "db.h"
#include "gamelog.h"
#include "modify.h"
#include "utils.h"
#include "xml.h"

// All the XML tags we recognize in a character block
char *character_tags[] = {
    "character",
    "name",
    "account",
    "sdesc",
    "desc",
    "keywords",
    "password",
    "objective",
    ""                          // Sentinel value
};

/* load_character_file_old()
 *
 * Helper function which allows us to use exiting CHAR_DATA objects as a
 * means to load data into a CHARACTER_DATA object from an xml file.
 */
int
load_character_file_old(const char *name, CHAR_DATA * ch)
{
    CHARACTER_DATA *cd = NULL;

    CREATE(cd, CHARACTER_DATA, 1);
    reset_character(cd);        // Initialize datavalues

    if (!load_character_file(name, ch->account, cd)) {
        free_character(cd);
        DESTROY(cd);
        return FALSE;
    }

    free_character(cd);
    DESTROY(cd);

    return TRUE;
}

/* save_character_file_old()
 *
 * Helper function which allows us to save out existing CHAR_DATA objects
 * as a CHARACTER_DATA object in an xml file.
 */
int
save_character_file_old(CHAR_DATA * ch)
{
    CHARACTER_DATA *cd = NULL;

    // Don't save NPCs into files
    if (IS_NPC(ch))
        return FALSE;

    /* We are really writing out a CHARACTER_DATA structure and not
     * the contents of the CHAR_DATA structure.  However, we need to
     * fill the CHARACTER_DATA structure from CHAR_DATA until a transition
     * to CHARACTER_DATA is made.
     */
    cd = ch_to_cd(ch);

    save_character_file(cd);
    free_character(cd);
    DESTROY(cd);

    return TRUE;
}

/* load_character_file()
 *
 * Load a character from an XML file
 */
int
load_character_file(const char *name, const char *account, CHARACTER_DATA * cd)
{
    FILE *fp;
    char filename[MAX_STRING_LENGTH];
    int retval;

    if (account && name) {
        snprintf(filename, sizeof(filename), "%s/%s/%s.xml", ACCOUNT_DIR, account, lowercase(name));
    } else {
        gamelogf("load_character_file(): No account specified.");
        return FALSE;
    }

    if (!(fp = fopen(filename, "r"))) {
        shhlogf("load_character_file(): Couldn't open %s.", filename);
        return FALSE;
    }

    retval = parse_character_file(fp, cd);

    if (retval == TRUE)
        fclose(fp);
    return retval;
}

/* save_character_file()
 *
 * Save a character to an xml file
 */
int
save_character_file(CHARACTER_DATA * cd)
{
    FILE *fp;
    char filename[MAX_STRING_LENGTH];

    if (cd->account && cd->name) {
        snprintf(filename, sizeof(filename), "%s/%s/%s.xml", ACCOUNT_DIR, cd->account,
                 lowercase(cd->name));
    } else {
        gamelogf("save_character_file(): No account specified.");
        return FALSE;
    }

    if (!(fp = fopen(filename, "w"))) {
        gamelogf("save_character_file(): Couldn't open %s.", filename);
        return FALSE;
    }
    // <character>
    write_start_element(fp, character_tags[CHARACTER_TAG_CHARACTER], NULL);

    // Save the information of a character

    // <name>...</name>
    if (cd->name)
        write_full_element(fp, character_tags[CHARACTER_TAG_NAME], NULL, "%s", cd->name);

    // <account>...</account>
    if (cd->account)
        write_full_element(fp, character_tags[CHARACTER_TAG_ACCOUNT], NULL, "%s", cd->account);

    // <sdesc>...</sdesc>
    if (cd->short_description)
        write_full_element(fp, character_tags[CHARACTER_TAG_SDESC], NULL, "%s",
                           cd->short_description);

    // <desc>...</desc>
    if (cd->description) {
        strip_crs(cd->description);     // Need to clip these out
        write_full_element(fp, character_tags[CHARACTER_TAG_DESC], NULL, "%s", cd->description);
    }
    // <keywords>...</keywords>
    if (cd->keywords)
        write_full_element(fp, character_tags[CHARACTER_TAG_KEYWORDS], NULL, "%s", cd->keywords);

    // <objective>..</objective>
    if (cd->objective)
        write_full_element(fp, character_tags[CHARACTER_TAG_OBJECTIVE], NULL, "%s", cd->objective);

    // </character>
    write_end_element(fp, character_tags[CHARACTER_TAG_CHARACTER]);

    fclose(fp);
    return TRUE;
}

/* character_start_handler()
 *
 * This function is called by the parser when it locates a start tag, sending
 * in the name of the tag and any attributes specified in the tag.
 */
void
character_start_handler(void *userData, const XML_Char * name, const XML_Char ** atts)
{
    PARSE_DATA *parse_data = (PARSE_DATA *) userData;
    int i, tag = CHARACTER_TAG_NONE;

    for (i = 0; strlen(character_tags[i]) > 0; i++)
        if (!stricmp(character_tags[i], name))
            tag = i;

    switch (parse_data->char_parse_data.state) {
    case PROCESSING_CHARACTER:
    default:
        switch (tag) {
        case CHARACTER_TAG_CHARACTER:
        case CHARACTER_TAG_NAME:
        case CHARACTER_TAG_ACCOUNT:
        case CHARACTER_TAG_SDESC:
        case CHARACTER_TAG_DESC:
        case CHARACTER_TAG_KEYWORDS:
        case CHARACTER_TAG_PASSWORD:
        case CHARACTER_TAG_OBJECTIVE:
            break;
        case CHARACTER_TAG_NONE:
        default:
            //gamelogf("character_start_handler(): Unknown tag \"%s\"", name);
            break;
        }
        break;
    }

    // It's a new element, free up whatever's in the tmp buffer
    DESTROY(parse_data->tmp);
}

/* character_end_handler()
 *
 * This function is called by the parser when it locates an end tag, sending
 * in the name of the tag.
 */
void
character_end_handler(void *userData, const XML_Char * name)
{
    PARSE_DATA *parse_data = (PARSE_DATA *) userData;
    int i, tag = CHARACTER_TAG_NONE;

    for (i = 0; strlen(character_tags[i]) > 0; i++)
        if (!stricmp(character_tags[i], name))
            tag = i;

    switch (parse_data->char_parse_data.state) {
    case PROCESSING_CHARACTER:
    default:
        switch (tag) {
        case CHARACTER_TAG_NAME:
            // If there's actual data in the tmp field, we can use it
            if (parse_data->tmp) {
                DESTROY(parse_data->char_parse_data.character.name);
                parse_data->char_parse_data.character.name = strdup(parse_data->tmp);
            }
            break;
        case CHARACTER_TAG_ACCOUNT:
            if (parse_data->tmp) {
                DESTROY(parse_data->char_parse_data.character.account);
                parse_data->char_parse_data.character.account = strdup(parse_data->tmp);
            }
            break;
        case CHARACTER_TAG_SDESC:
            if (parse_data->tmp) {
                DESTROY(parse_data->char_parse_data.character.short_description);
                parse_data->char_parse_data.character.short_description = strdup(parse_data->tmp);
            }
            break;
        case CHARACTER_TAG_DESC:
            if (parse_data->tmp) {
                DESTROY(parse_data->char_parse_data.character.description);
                parse_data->char_parse_data.character.description = add_crs(parse_data->tmp);
            }
            break;
        case CHARACTER_TAG_KEYWORDS:
            if (parse_data->tmp) {
                DESTROY(parse_data->char_parse_data.character.keywords);
                parse_data->char_parse_data.character.keywords = strdup(parse_data->tmp);
            }
            break;
        case CHARACTER_TAG_OBJECTIVE:
            if (parse_data->tmp) {
                DESTROY(parse_data->char_parse_data.character.objective);
                parse_data->char_parse_data.character.objective = strdup(parse_data->tmp);
            }
            break;
        case CHARACTER_TAG_CHARACTER:
            break;
        case CHARACTER_TAG_NONE:
        default:
            //gamelogf("character_end_handler(): Unknown tag \"%s\"", name);
            break;
        }
        break;
    }

    // Reset our data
    DESTROY(parse_data->tmp);
}

/* parse_character_file()
 *
 * This parses a character file into a CHARACTER_DATA object.
 */
int
parse_character_file(FILE * fp, CHARACTER_DATA * cd)
{
    PARSE_DATA parse_data;

    memset(&parse_data, 0, sizeof(parse_data));
    parse_data.char_parse_data.state = PROCESSING_CHARACTER;

    if (!parse_xml_file(fp, &parse_data, character_start_handler, character_end_handler))
        return FALSE;

    // Store the parsing results in the provided data structure
    copy_character(cd, &parse_data.char_parse_data.character);
    free_character(&parse_data.char_parse_data.character);

    return TRUE;
}

/* copy_character()
 *
 * Makes a copy of a CHARACTER_DATA object.
 */
void
copy_character(CHARACTER_DATA * to, CHARACTER_DATA * from)
{
    if (from->name)
        to->name = strdup(from->name);
    if (from->account)
        to->account = strdup(from->account);
    if (from->short_description)
        to->short_description = strdup(from->short_description);
    if (from->description)
        to->description = strdup(from->description);
    if (from->keywords)
        to->keywords = strdup(from->keywords);
    if (from->objective)
        to->objective = strdup(from->objective);
}


/* ch_to_cd()
 *
 * Creates a new CHARACTER_DATA object from a CHAR_DATA object
 */
CHARACTER_DATA *
ch_to_cd(CHAR_DATA * ch)
{
    CHARACTER_DATA *cd = NULL;

    CREATE(cd, CHARACTER_DATA, 1);
    reset_character(cd);

    if (ch->name)
        cd->name = strdup(ch->name);
    if (ch->account)
        cd->account = strdup(ch->account);
    if (ch->short_descr)
        cd->short_description = strdup(ch->short_descr);
    if (ch->description)
        cd->description = strdup(ch->description);
    if (ch->player.extkwds)
        cd->keywords = strdup(ch->player.extkwds);
    if (ch->player.info[1] && strlen(ch->player.info[1]) > 0)
        cd->objective = strdup(ch->player.info[1]);
    return cd;
}

/* reset_character()
 *
 * Sets all the values of a CHARACTER_DATA to their default.
 *
 * Does not attempt to release existing memory! BEWARE!
 */
void
reset_character(CHARACTER_DATA * cd)
{
    cd->name = NULL;
    cd->account = NULL;
    cd->short_description = NULL;
    cd->description = NULL;
    cd->keywords = NULL;
    cd->objective = NULL;
}

/* free_character()
 *
 * Releases any memory used by a CHARACTER_DATA object, not including its own
 * reference.
 */
void
free_character(CHARACTER_DATA * cd)
{
    DESTROY(cd->name);
    DESTROY(cd->account);
    DESTROY(cd->short_description);
    DESTROY(cd->description);
    DESTROY(cd->keywords);
    DESTROY(cd->objective);
}

