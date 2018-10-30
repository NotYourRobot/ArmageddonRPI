/*
 * File: CRAFT.C
 * Usage: Routines and commands for crafty players.
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
/* Revision history
 * Please list changes to this file here for everyone's benefit.
 * 2002.12.12  created to read craft file as XML format.
 */

#define USE_CRAFTS_FILE


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include "constants.h"
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "parser.h"
#include "utils.h"
#include "utility.h"
#include "skills.h"
#include "barter.h"
#include "cities.h"
#include "event.h"
#include "guilds.h"
#include "clan.h"
#include "utils.h"
#include "utility.h"
#include "db_file.h"
#include "modify.h"
#include "limits.h"
#include "memory.h"
#include "expat.h"

/* master list of items makeable */
struct create_item_type *make_item_list = 0;

void init_crafting_list(struct create_item_type *list, int len);
int parse_crafting(FILE * fp, struct create_item_type **list);
void save_crafting_file(struct create_item_type *list, int len);

/*
  |<CRAFT>
  |  <ITEM>
  |    <NAME>                  </NAME>
  |    <DESC>                  </DESC>
  |    <FROM>                  </FROM>
  |    <INTO_SUCCESS>          </INTO_SUCCESS>
  |    <INTO_FAIL>             </INTO_FAIL>
  |    <SKILL>                 </SKILL>
  |    <PERC_MIN>              </PERC_MIN>
  |    <PERC_MOD>              </PERC_MOD>
  |    <DELAY_MIN>             </DELAY_MIN>
  |    <DELAY_MAX>             </DELAY_MAX>
  |    <TO_CHAR_SUCCESS>       </TO_CHAR_SUCCESS>
  |    <TO_ROOM_SUCCESS>       </TO_CHAR_SUCCESS>
  |    <TO_CHAR_FAIL>          </TO_CHAR_FAIL>
  |    <TO_ROOM_FAIL>          </TO_ROOM_FAIL>
  |    <ONLY_TRIBE>            </ONLY_TRIBE>
  |    <REQUIRE_FIRE>          </REQUIRE_FIRE>
  |    <TOOL_TYPE>             </TOOL_TYPE>
  |    <TOOL_QUALITY>          </TOOL_QUALITY>
  |  </ITEM>
  |</CRAFT>
*/

void
unboot_crafts(void)
{
    int count;
    int i;

    if (make_item_list != 0) {

      /***********************/
      /**** SAVE A BACKUP ****/
      /***********************/


        for (i = 0; make_item_list[i].skill != SPELL_NONE; i++) {       /* do nothing, establish count */
        }

        count = i;

        save_crafting_file(make_item_list, i);

      /***********************/
      /**** FREE MEMORY   ****/
      /***********************/

        for (i = 0; i <= count; i++) {  /* need to get count for sentinal as well */
            free(make_item_list[i].name);
            free(make_item_list[i].desc);
            free(make_item_list[i].to_char_succ);
            free(make_item_list[i].to_room_succ);
            free(make_item_list[i].to_char_fail);
            free(make_item_list[i].to_room_fail);
        }

        free(make_item_list);
    }

    make_item_list = 0;
}


void
boot_crafts(void)
{
    /*  char *tmp;   int shop_nr, i, j, l, m;
     * double k; */

    FILE *fp;

    unboot_crafts();


    if (!(fp = fopen(CRAFT_FILE, "r"))) {
        gamelogf("Couldn't open %s, skipping!", CRAFT_FILE);
        return;
    }

    if (parse_crafting(fp, &make_item_list))
        gamelog("Read Crafts:HUZZAH!");
    else
        gamelog("Read Crafts:FAIL");

    {
        char debugmsg[2000];


        int i;
        if (make_item_list) {
            for (i = 0; make_item_list[i].skill != SPELL_NONE; i++) {   /* do nothing, establish count */
            };
            sprintf(debugmsg, "Loaded %d items.", i);
            gamelog(debugmsg);
        } else {
            sprintf(debugmsg, "Make item list is still empty.");
            gamelog(debugmsg);
        }
    };

    fclose(fp);
}


/* This is a structure that is used for common data */
struct craft_parse
{
    int total_count;            /* total number of items in this file    */
    int reading;                /* what particular element is to be read */
    int current;                /* what index is currently being read    */
    struct create_item_type *new_list;  /* structure to read into */
    char *tmp;                  /* char pointer for data in between tags */
};


/*************************************************************************/
/**** COUNTING FUNCTIONS : used once to count number of items in file so */
/****                      can allocate the right amount of data space   */
/*************************************************************************/
void
start_craft_element_count(void *userData, const char *name, const char **atts)
{
    struct craft_parse *parse_data = userData;

    if (!strcmp(name, "ITEM"))
        parse_data->total_count++;
}

void
end_craft_element_count(void *userData, const char *name)
{
    /*  struct craft_parse *parse_data = userData; */
    /* do nothing */
}

/*************************************************************************/
/**** READING FUNCTIONS  : used for second pass to actually read data    */
/****                      The start element will have the tag that is   */
/****                      currently being read. So figgure out what     */
/****                      state it is, so when read the data in between */
/****                      next tag, can store it to appropriate field   */
/*************************************************************************/

char *crafting_tags[] = {
    "NAME",
    "DESC",
    "FROM",
    "INTO_SUCCESS",
    "INTO_FAIL",
    "SKILL",
    "PERC_MIN",
    "PERC_MOD",
    "DELAY_MIN",
    "DELAY_MAX",
    "TO_CHAR_SUCCESS",
    "TO_ROOM_SUCCESS",
    "TO_CHAR_FAIL",
    "TO_ROOM_FAIL",
    "ONLY_TRIBE",
    "REQUIRE_FIRE",
    "TOOL_TYPE",
    "TOOL_QUALITY",
    ""                          /* sentinal on the end */
};

#define CRAFT_MODE_NAME          0
#define CRAFT_MODE_DESC          1
#define CRAFT_MODE_FROM          2
#define CRAFT_MODE_INTO_SUCCESS  3
#define CRAFT_MODE_INTO_FAIL     4
#define CRAFT_MODE_SKILL         5
#define CRAFT_MODE_PERC_MIN      6
#define CRAFT_MODE_PERC_MOD      7
#define CRAFT_MODE_DELAY_MIN     8
#define CRAFT_MODE_DELAY_MAX     9
#define CRAFT_MODE_TO_CHAR_SUCC  10
#define CRAFT_MODE_TO_ROOM_SUCC  11
#define CRAFT_MODE_TO_CHAR_FAIL  12
#define CRAFT_MODE_TO_ROOM_FAIL  13
#define CRAFT_MODE_ONLY_TRIBE    14
#define CRAFT_MODE_REQUIRE_FIRE  15
#define CRAFT_MODE_TOOL_TYPE     16
#define CRAFT_MODE_TOOL_QUALITY  17
#define CRAFT_MODE_NONE          18

void
start_craft_element_load(void *userData, const char *name, const char **atts)
{
    int i;
    struct craft_parse *parse_data = userData;


    /* set the 'reading' element to be what element we just passed as the header
     * tag, so that when the inside of the tag pair is read, we know where to
     * store it.
     */

    parse_data->reading = CRAFT_MODE_NONE;
    for (i = 0; strlen(crafting_tags[i]) > 0; i++)
        if (!strcmp(crafting_tags[i], name))
            parse_data->reading = i;

}


void
end_craft_content_load(void *userData, const XML_Char * s, int len)
{
    struct craft_parse *parse_data = userData;
    char tmp[2000];

    /*******************************************************/
    /* basicly were getting the bits between the tags here */
    /* this function will be called multiple times if      */
    /* there are multiple calls to input xml, so catonate  */
    /* them all up here while getting to end tag           */
    /*******************************************************/

    if (parse_data->reading != CRAFT_MODE_NONE) {
        memcpy(tmp, s, MIN(len, 1999));
        tmp[MIN(len, 1999)] = 0;

        /* we want to save this stuff into tmp if were in a good part */
        if (parse_data->tmp) {
            parse_data->tmp = (char *)
                realloc(parse_data->tmp, strlen(parse_data->tmp) + strlen(tmp) + 20);
            strcat(parse_data->tmp, tmp);
        } else {
            parse_data->tmp = (char *)
                malloc(strlen(tmp) + 20);
            strcpy(parse_data->tmp, tmp);
        }
    }
}

void
end_craft_element_load(void *userData, const char *name)
{
    /* Only increase the current item after read the last item, otherwise  */
    /*   the first item read would be #1                                   */
    struct create_item_type *curr;
    struct craft_parse *parse_data = userData;
    const char *tmp;
    int i;

    /*******************************************************/
    /* Since we found an END tag, go ahead and use what we */
    /* read so far in the content_load part                */
    /*******************************************************/

    /* structure to read into */
    curr = &(parse_data->new_list[parse_data->current]);

    if (parse_data->tmp) {
        tmp = parse_data->tmp;

        switch (parse_data->reading) {
        case CRAFT_MODE_NAME:
            if (curr->name)
                free(curr->name);
            curr->name = strdup(tmp);
            break;
        case CRAFT_MODE_DESC:
            if (curr->desc)
                free(curr->desc);
            curr->desc = strdup(tmp);
            break;

#define READ_CRAFT_GROUP(list) \
    for (i = 0; i < CRAFT_GROUP_SIZE; i++) {         \
        char abuf[256];                              \
        tmp = one_argument(tmp, abuf, sizeof(abuf)); \
        if (isdigit(*abuf)) {                        \
            (list)[i] = atoi(abuf);                  \
        } else {                                     \
            (list)[i] = 0;                           \
        }                                            \
    }
        case CRAFT_MODE_FROM:
            READ_CRAFT_GROUP(curr->from);
            break;
        case CRAFT_MODE_INTO_SUCCESS:
            READ_CRAFT_GROUP(curr->into_success);
            break;
        case CRAFT_MODE_INTO_FAIL:
            READ_CRAFT_GROUP(curr->into_fail);
            break;
#undef READ_CRAFT_GROUP

        case CRAFT_MODE_SKILL:
            curr->skill = old_search_block(tmp, 0, strlen(tmp), skill_name, 0) - 1;
            break;
        case CRAFT_MODE_PERC_MIN:
            sscanf(tmp, " %d ", &(curr->percentage_min));
            break;
        case CRAFT_MODE_PERC_MOD:
            sscanf(tmp, " %d ", &(curr->percentage_mod));
            break;
        case CRAFT_MODE_DELAY_MIN:
            sscanf(tmp, " %d ", &(curr->delay_min));
            break;
        case CRAFT_MODE_DELAY_MAX:
            sscanf(tmp, " %d ", &(curr->delay_max));
            break;
        case CRAFT_MODE_TO_CHAR_SUCC:
            if (curr->to_char_succ)
                free(curr->to_char_succ);
            curr->to_char_succ = strdup(tmp);
            break;
        case CRAFT_MODE_TO_ROOM_SUCC:
            if (curr->to_room_succ)
                free(curr->to_room_succ);
            curr->to_room_succ = strdup(tmp);
            break;
        case CRAFT_MODE_TO_CHAR_FAIL:
            if (curr->to_char_fail)
                free(curr->to_char_fail);
            curr->to_char_fail = strdup(tmp);
            break;
        case CRAFT_MODE_TO_ROOM_FAIL:
            if (curr->to_room_fail)
                free(curr->to_room_fail);
            curr->to_room_fail = strdup(tmp);
            break;
        case CRAFT_MODE_ONLY_TRIBE:
            curr->only_tribe = lookup_clan_by_name(tmp);
            if (curr->only_tribe == -1)
                curr->only_tribe = TRIBE_ANY;
            break;
        case CRAFT_MODE_REQUIRE_FIRE:
          sscanf(tmp, " %d ", &(curr->require_fire));
          break;
        case CRAFT_MODE_TOOL_TYPE:
          curr->tool_type = old_search_block(tmp, 0, strlen(tmp), tool_name, 0) - 1;
          break;
        case CRAFT_MODE_TOOL_QUALITY:
          sscanf(tmp, " %d ", &(curr->tool_quality));
          break;
        case CRAFT_MODE_NONE:
            break;
        }

        free(parse_data->tmp);
        parse_data->tmp = 0;
    }

    parse_data->reading = CRAFT_MODE_NONE;

    if (!strcmp(name, "ITEM")) {
        parse_data->current += 1;
    }
}

/*************************************************************************/
/**** READING FUNCTIONS  : parse the file.                               */
/****                      Set up environment to read count in the file. */
/****                      read file and parse it.                       */
/****                      Allocate space for read.                      */
/****                      Set up environment to actually load data.     */
/****                      read file and parse it.                       */
/*************************************************************************/

int
parse_crafting(FILE * fp, struct create_item_type **ilist)
{
    char buf[BUFSIZ + 3];
    XML_Parser parser = 0;
    int done;
    struct craft_parse parse_data;
    char debugmsg[BUFSIZ + 2000];

    gamelog("starting to parse crafting");

  /**************************************/
    /* first pass count up the data items */
  /**************************************/
    parse_data.total_count = 0;

    parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, &parse_data);
    XML_SetElementHandler(parser, start_craft_element_count, end_craft_element_count);

    while (fgets(buf, sizeof(buf), fp)) {
        done = feof(fp);
        if (!XML_Parse(parser, buf, strlen(buf), done)) {
            sprintf(debugmsg, "%s at line %d\n", XML_ErrorString(XML_GetErrorCode(parser)),
                    XML_GetCurrentLineNumber(parser));
            gamelog(debugmsg);
            return (FALSE);
        }
    }

    XML_ParserFree(parser);

    rewind(fp);

  /**************************************/
    /* initialize data                    */
  /**************************************/

    sprintf(debugmsg, "Done loading, total items = %d", parse_data.total_count);
    gamelog(debugmsg);

    parse_data.current = 0;
    parse_data.tmp = 0;
    parse_data.reading = 0;
    parse_data.total_count += 1;        /* for sentinal */
    parse_data.new_list = (struct create_item_type *)
        malloc(sizeof(struct create_item_type) * parse_data.total_count);
    init_crafting_list(parse_data.new_list, parse_data.total_count);

  /**************************************/
    /* second pass actually read the data */
  /**************************************/
    parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, &parse_data);
    XML_SetElementHandler(parser, start_craft_element_load, end_craft_element_load);
    XML_SetCharacterDataHandler(parser, end_craft_content_load);

    while (fgets(buf, sizeof(buf), fp)) {
        done = feof(fp);
        if (!XML_Parse(parser, buf, strlen(buf), done)) {
            sprintf(debugmsg, "%s at line %d\n", XML_ErrorString(XML_GetErrorCode(parser)),
                    XML_GetCurrentLineNumber(parser));
            gamelog(debugmsg);
            return (FALSE);
        }
    }
    XML_ParserFree(parser);

    {
        int i;
        for (i = 0; parse_data.new_list[i].skill != SPELL_NONE; i++) {  /* do nothing, establish count */ }
        sprintf(debugmsg, "Loaded %d items.", i);
        gamelog(debugmsg);
    }

    *ilist = parse_data.new_list;
    gamelog("done parsing crafting");
    return (TRUE);
}

void
init_crafting_list(struct create_item_type *list, int len)
{
    int i, j;
    struct create_item_type *curr;

    for (i = 0; i < len; i++) {
        curr = &(list[i]);

        curr->name = strdup("");
        curr->desc = strdup("");
        curr->skill = SPELL_NONE;
        for (j = 0; j < CRAFT_GROUP_SIZE; j++) {
            curr->from[j] = -1;
            curr->into_success[j] = -1;
            curr->into_fail[j] = -1;
        }
        curr->percentage_min = 0;
        curr->percentage_mod = 0;
        curr->delay_min = 0;
        curr->delay_max = 0;

        curr->to_char_succ = strdup("");
        curr->to_room_succ = strdup("");

        curr->to_char_fail = strdup("");
        curr->to_room_fail = strdup("");

        curr->only_tribe = TRIBE_NONE;

        curr->require_fire = FALSE;

        curr->tool_type = -1;
        curr->tool_quality = 0;
    }
}


/*
  |<CRAFT>
  |  <ITEM>
  |    <NAME>                  </NAME>
  |    <DESC>                  </DESC>
  |    <FROM>                  </FROM>
  |    <INTO_SUCCESS>          </INTO_SUCCESS>
  |    <INTO_FAIL>             </INTO_FAIL>
  |    <SKILL>                 </SKILL>
  |    <PERC_MIN>              </PERC_MIN>
  |    <PERC_MOD>              </PERC_MOD>
  |    <DELAY_MIN>             </DELAY_MIN>
  |    <DELAY_MAX>             </DELAY_MAX>
  |    <TO_CHAR_SUCCESS>       </TO_CHAR_SUCCESS>
  |    <TO_ROOM_SUCCESS>       </TO_CHAR_SUCCESS>
  |    <TO_CHAR_FAIL>          </TO_CHAR_FAIL>
  |    <TO_ROOM_FAIL>          </TO_ROOM_FAIL>
  |    <ONLY_TRIBE>            </ONLY_TRIBE>
  |    <REQUIRE_FIRE>          </REQUIRE_FIRE>
  |    <TOOL_TYPE>             </TOOL_TYPE>
  |    <TOOL_QUALITY>          </TOOL_QUALITY>
  |  </ITEM>
  |</CRAFT>
*/



void
save_crafting_file(struct create_item_type *list, int len)
{
    FILE *fp;
    int i, j;
    struct create_item_type *curr;

    if (!(fp = fopen(CRAFT_FILE ".old", "w"))) {
        perror("Error in save_crafts");
    }
    fprintf(fp, "<CRAFT>\n");
    for (i = 0; i < len; i++) {
        curr = &(list[i]);
        fprintf(fp, "  <ITEM>\n");

        fprintf(fp, "    <NAME>%s</NAME>\n", curr->name);
        fprintf(fp, "    <FROM>");
        for (j = 0; j < CRAFT_GROUP_SIZE; j++) {
            if (curr->from[j])
                fprintf(fp, "%s%d", j?" ":"", curr->from[j]);
        }
        fprintf(fp, "</FROM>\n");

        fprintf(fp, "    <INTO_SUCCESS>");
        for (j = 0; j < CRAFT_GROUP_SIZE; j++) {
            if (curr->from[j])
                fprintf(fp, "%s%d", j?" ":"", curr->into_success[j]);
        }
        fprintf(fp, "</INTO_SUCCESS>\n");

        fprintf(fp, "    <INTO_FAIL>");
        for (j = 0; j < CRAFT_GROUP_SIZE; j++) {
            if (curr->from[j])
                fprintf(fp, "%s%d", j?" ":"", curr->into_fail[j]);
        }
        fprintf(fp, "</INTO_FAIL>\n");

        if ((curr->skill >= 0) && (curr->skill < MAX_SKILLS))
            fprintf(fp, "    <SKILL>%s</SKILL>\n", skill_name[curr->skill]);
        else
            fprintf(fp, "    <SKILL>%d</SKILL>\n", curr->skill);

        fprintf(fp, "    <PERC_MIN>%d</PERC_MIN>\n", curr->percentage_min);
        fprintf(fp, "    <PERC_MOD>%d</PERC_MOD>\n", curr->percentage_mod);
        fprintf(fp, "    <DELAY_MIN>%d</DELAY_MIN>\n", curr->delay_min);
        fprintf(fp, "    <DELAY_MAX>%d</DELAY_MAX>\n", curr->delay_max);
        fprintf(fp, "    <TO_CHAR_SUCCESS>%s</TO_CHAR_SUCCESS>\n", curr->to_char_succ);
        fprintf(fp, "    <TO_ROOM_SUCCESS>%s</TO_ROOM_SUCCESS>\n", curr->to_room_succ);
        fprintf(fp, "    <TO_CHAR_FAIL>%s</TO_CHAR_FAIL>\n", curr->to_char_fail);
        fprintf(fp, "    <TO_ROOM_FAIL>%s</TO_ROOM_FAIL>\n", curr->to_room_fail);

        if ((curr->only_tribe >= 0) && (curr->only_tribe < MAX_CLAN)) {
            fprintf(fp, "    <ONLY_TRIBE>%s</ONLY_TRIBE>\n", clan_table[curr->only_tribe].name);
        } else if (curr->only_tribe == TRIBE_ANY) {
            fprintf(fp, "    <ONLY_TRIBE>TRIBE_ANY</ONLY_TRIBE>\n");
        } else {
            fprintf(fp, "    <ONLY_TRIBE>%d</ONLY_TRIBE>\n", curr->only_tribe);
        }

        fprintf(fp, "    <REQUIRE_FIRE>%d</REQUIRE_FIRE>\n", curr->require_fire);

        //        if (curr->tool_type != -1) {
        //          fprintf(fp, "    <REQUIRE_FIRE>%d</REQUIRE_FIRE>\n", curr->tool_type);
        //        }

        fprintf(fp, "    <TOOL_QUALITY>%d</TOOL_QUALITY>\n", curr->tool_quality);

        fprintf(fp, "  </ITEM>\n");
    }
    fprintf(fp, "</CRAFT>\n");

    {
        char tmpstring[2999];

        sprintf(tmpstring, "last error was %s", strerror(errno));
        gamelog(tmpstring);
    }


    fclose(fp);
}

